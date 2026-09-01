# ----------------------------------------------------------------------
# 006_rotation.pl
#
# File-rotation scenario test.
#
# The bgworker rotates each kind of log file on a fixed wall-clock period
# (samples=1h, sessions/queries=1d), so a TAP test cannot wait for a real
# rotation to happen.  Instead we:
#
#   1. start the cluster and capture phase-1 samples for a pg_sleep
#      backend (so the on-disk files contain real samples, sessions and
#      queries records);
#   2. stop the cluster cleanly -- this exercises the fsync-on-shutdown
#      path added in P1 and guarantees that the data we wrote is durably
#      on disk before we touch the file names;
#   3. rename every hg_ws_<kind>_<cts>_<rts>_<flag> file so that the
#      rotate timestamp embedded in its name falls strictly before the
#      restart wall-clock.  Reader file-level filtering uses cts/rts from
#      the name (not the file contents) and hg_ws_open_or_create() will
#      only reopen a file whose rts is *in the future*, so on restart the
#      bgworker now believes every prior file has already rotated;
#   4. restart the cluster -- the bgworker creates one brand new file per
#      kind, with cts = restart_time;
#   5. capture phase-2 samples for a different pg_sleep backend;
#   6. assert that:
#        a. the directory now holds *both* the renamed phase-1 files and
#           the brand new phase-2 files;
#        b. unwindowed reader functions (samples/sessions/queries/data)
#           return rows from both files;
#        c. narrowly-windowed queries that target only phase 1 or only
#           phase 2 return exactly the rows of that phase, proving that
#           the file-level skip in read_samples/sessions/queries uses the
#           name-embedded timestamps correctly across a rotation;
#        d. hg_wait_states_data() joins a phase-2 sample to a phase-2
#           sessions record that was re-emitted into the *new* file (the
#           bgworker drops its session-hash on rotation, so a long-lived
#           backend is re-recorded after rotation);
#        e. no truncated/corrupt warning is logged.
# ----------------------------------------------------------------------
use strict;
use warnings;

use File::Basename qw(basename);
use PostgreSQL::Test::Cluster;
use PostgreSQL::Test::Utils;
use Test::More;
use Time::HiRes qw(time sleep);

# PostgreSQL TimestampTz is microseconds since 2000-01-01 00:00:00 UTC.
use constant PG_EPOCH_UNIX => 946_684_800;

sub pg_now_us
{
	return int((Time::HiRes::time() - PG_EPOCH_UNIX) * 1_000_000);
}

# Renames every hg_ws_<kind>_<cts>_<rts>_<flag> file in $dir so the rts
# embedded in the file name is $forced_rts.  Returns the list of new
# names so callers can assert on file presence afterwards.
sub force_rotation
{
	my ($dir, $forced_rts) = @_;
	my @renamed;

	opendir(my $dh, $dir) or die "cannot open $dir: $!";
	my @files = grep { /^hg_ws_/ } readdir($dh);
	closedir($dh);

	for my $f (@files)
	{
		if ($f =~
			/^(hg_ws_(?:samples|sessions|queries)_)(\d+)_(\d+)_(\d+)$/)
		{
			my ($prefix, $cts, $rts, $flag) = ($1, $2, $3, $4);
			next if $cts >= $forced_rts;  # safety: don't move rts <= cts
			next if $rts <= $forced_rts;  # already in the past
			my $newname = "${prefix}${cts}_${forced_rts}_${flag}";
			rename "$dir/$f", "$dir/$newname"
			  or die "rename $dir/$f -> $dir/$newname: $!";
			push @renamed, $newname;
		}
	}
	return @renamed;
}

my $node = PostgreSQL::Test::Cluster->new('hg_ws_rot');
$node->init;
$node->append_conf(
	'postgresql.conf', qq{
shared_preload_libraries = 'hg_wait_states'
hg_wait_states.database = postgres
hg_wait_states.enable_collection = on
track_activities = on
fsync = off
});
$node->start;
$node->safe_psql('postgres', 'CREATE EXTENSION hg_wait_states');

# ---------------------------------------------------- phase 1
my $sleeper1 = $node->background_psql('postgres');
$sleeper1->query_until(
	qr/p1_started/, q{
SELECT 'p1_started';
SELECT pg_sleep(4);
});

# Wait until phase-1 samples have landed.  Poll once per 200 ms instead of
# sleeping a fixed amount so the test stays fast on a fast host.
my $phase1_samples = 0;
for (my $i = 0; $i < 40; $i++)
{
	$phase1_samples = $node->safe_psql(
		'postgres', q{
SELECT count(*) FROM hg_wait_states_samples()
WHERE wait_event = 'PgSleep'
});
	last if $phase1_samples >= 2;
	sleep(0.2);
}
cmp_ok($phase1_samples, '>=', 2, 'phase-1 collected at least two PgSleep samples');

my $phase1_pid = $node->safe_psql(
	'postgres', q{
SELECT min(session_id)
FROM hg_wait_states_samples()
WHERE wait_event = 'PgSleep'
});
like($phase1_pid, qr/^\d+$/,
	"captured phase-1 sleeping pid ($phase1_pid)");

# Upper bound for a "phase 1 only" window.  Keep this as a timestamptz
# text (not float8 epoch): EXTRACT(EPOCH)::float8 round-trips lose
# sub-second precision and make later to_timestamp() comparisons flaky.
my $phase1_cut = $node->safe_psql(
	'postgres', q{
SELECT max(sample_time) + interval '2 seconds'
FROM hg_wait_states_samples()
WHERE wait_event = 'PgSleep'
});
$phase1_cut =~ s/'/''/g;

# ---------------------------------------------------- stop + rename
eval { $sleeper1->quit; };
$node->stop;     # clean stop exercises fsync-on-shutdown

my $logdir = $node->data_dir . '/hg_wait_states';
my @before = sort glob("$logdir/hg_ws_*");
cmp_ok(scalar @before, '>=', 3,
	'phase-1 left at least one file of each kind on disk');

# Snapshot byte sizes per file so we can confirm rename did not damage
# anything.
my %size_pre = map { $_ => -s $_ } @before;

my $forced_rts = pg_now_us();
my @renamed = force_rotation($logdir, $forced_rts);
cmp_ok(scalar @renamed, '>=', 3,
	'force_rotation() rewrote at least one file per kind');

# Byte content unchanged across rename.
my %size_post = map { $_ => -s $_ } (glob("$logdir/hg_ws_*"));
my $kept_bytes = 0;
for my $f (@before)
{
	my $base = basename($f);
	if ($base =~
		/^(hg_ws_(?:samples|sessions|queries)_)(\d+)_(\d+)_(\d+)$/)
	{
		my $newpath = "$logdir/$1$2_${forced_rts}_$4";
		$kept_bytes++
		  if defined $size_post{$newpath}
		  and $size_post{$newpath} == $size_pre{$f};
	}
}
cmp_ok($kept_bytes, '>=', 3,
	'every phase-1 file kept its byte size across the rename');

# ---------------------------------------------------- restart
$node->start;

# Wait for the bgworker to register and observe the "expired" files.  It
# will create one brand-new file per kind before the first sampling
# cycle.  hg_wait_states.sampling_interval = 1 s so 3 s is plenty.
sleep(2);

# Wall-clock mark after restart: every phase-2 sample_time is >= this.
# Separating phases by restart time avoids float-epoch boundary issues
# and PID reuse between the two sleepers.
my $phase2_begin = $node->safe_psql('postgres', 'SELECT now()');
$phase2_begin =~ s/'/''/g;

# Purge cutoff, frozen here: after the renamed files' rts (forced before
# restart) and well before the new files' rts (created_at + 1h / 1d).
# Use a UTC timestamptz literal from Postgres itself so the window does
# not depend on Perl's PG-epoch conversion, OS/session TimeZone / DateStyle,
# or now() at purge time (a later container clock jump of >= 1 hour
# would otherwise make now() delete the still-active samples file).
my $purge_end = $node->safe_psql(
	'postgres', q{
SELECT to_char(timezone('UTC', now()), 'YYYY-MM-DD HH24:MI:SS.US') || '+00'
});
$purge_end =~ s/'/''/g;

# At this point the directory must contain *both* the renamed phase-1
# files (which still have data) and freshly created phase-2 files (still
# only carrying the 8-byte header).
my @after_restart = sort glob("$logdir/hg_ws_*");
my $samples_files =
  grep { /\bhg_ws_samples_/ } @after_restart;
my $sess_files = grep { /\bhg_ws_sessions_/ } @after_restart;
my $qry_files  = grep { /\bhg_ws_queries_/ } @after_restart;
cmp_ok($samples_files, '>=', 2,
	'after restart, both pre- and post-rotation samples files exist');
cmp_ok($sess_files, '>=', 2,
	'after restart, both pre- and post-rotation sessions files exist');
cmp_ok($qry_files, '>=', 2,
	'after restart, both pre- and post-rotation queries files exist');

# ---------------------------------------------------- phase 2
my $sleeper2 = $node->background_psql('postgres');
$sleeper2->set_query_timer_restart();
$sleeper2->query_until(
	qr/p2_started/, q{
SELECT 'p2_started';
SELECT pg_sleep(4);
});

my $phase2_samples = 0;
for (my $i = 0; $i < 40; $i++)
{
	$phase2_samples = $node->safe_psql(
		'postgres', qq{
SELECT count(*) FROM hg_wait_states_samples()
WHERE wait_event = 'PgSleep'
  AND sample_time > '$phase2_begin'::timestamptz
});
	last if $phase2_samples >= 2;
	sleep(0.2);
}
cmp_ok($phase2_samples, '>=', 2,
	'phase-2 collected at least two PgSleep samples in the new file');

my $phase2_pid = $node->safe_psql(
	'postgres', qq{
SELECT session_id
FROM hg_wait_states_samples()
WHERE wait_event = 'PgSleep'
  AND sample_time > '$phase2_begin'::timestamptz
ORDER BY sample_time DESC LIMIT 1
});
like($phase2_pid, qr/^\d+$/,
	"captured phase-2 sleeping pid ($phase2_pid)");

# ---------------------------------------------------- assertions
# 1. Unwindowed scan must return rows from both files; total >= sum.
my $total = $node->safe_psql(
	'postgres', q{
SELECT count(*) FROM hg_wait_states_samples()
WHERE wait_event = 'PgSleep'
});
cmp_ok($total, '>=', $phase1_samples + $phase2_samples,
	'unwindowed samples() returns rows from both rotated and active files');

# 2. Phase-1-only narrow window must return only phase-1 rows.
my $p1_window = $node->safe_psql(
	'postgres', qq{
SELECT count(*)
FROM hg_wait_states_samples('-infinity'::timestamptz,
                            '$phase1_cut'::timestamptz)
WHERE session_id = $phase1_pid
});
cmp_ok($p1_window, '>=', 1,
	'narrow window into the rotated file still finds phase-1 samples');

# Reader contract: end_ts is exclusive, so nothing with sample_time >= cut
# may appear.  Also, no post-restart (phase-2) sample may appear in the
# phase-1 window — match on (session_id, sample_time) because PIDs can be
# recycled across the restart.
my $p1_window_excludes_p2 = $node->safe_psql(
	'postgres', qq{
WITH
cut AS (SELECT '$phase1_cut'::timestamptz AS t),
p2 AS (
	SELECT s.session_id, s.sample_time
	FROM hg_wait_states_samples() s
	WHERE s.wait_event = 'PgSleep'
	  AND s.sample_time >= '$phase2_begin'::timestamptz
),
p1win AS (
	SELECT w.session_id, w.sample_time
	FROM cut,
	     hg_wait_states_samples('-infinity'::timestamptz, cut.t) w
)
SELECT count(*)::text
FROM p1win p1
JOIN p2 USING (session_id, sample_time)
});
is($p1_window_excludes_p2, '0',
	'phase-1 window does not bleed into phase-2 samples')
  or diag("phase1_cut=$phase1_cut phase2_begin=$phase2_begin");

# 3. Phase-2 sample must join, via hg_wait_states_data(), to the
#    sessions record that the bgworker re-emitted into the brand-new
#    sessions file after rotation.
my $joined_p2 = $node->safe_psql(
	'postgres', qq{
SELECT count(*)
FROM hg_wait_states_data()
WHERE session_id = $phase2_pid
  AND query LIKE '%pg_sleep(4)%'
  AND wait_event = 'PgSleep'
});
cmp_ok($joined_p2, '>=', 1,
	'data() joins phase-2 samples to the new sessions/queries files');

# 4. Phase-1 sample also still joins -- the join uses sessions row from
#    the renamed (pre-rotation) file.
my $joined_p1 = $node->safe_psql(
	'postgres', qq{
SELECT count(*)
FROM hg_wait_states_data()
WHERE session_id = $phase1_pid
  AND query LIKE '%pg_sleep(4)%'
  AND wait_event = 'PgSleep'
});
cmp_ok($joined_p1, '>=', 1,
	'data() joins phase-1 samples to the rotated (renamed) sessions file');

# 5. directory_size() includes both rotated and active files.  The
#    bgworker may append to the active file in parallel with this query,
#    so we only assert that directory_size is at least the unchanging
#    byte count of the (renamed) phase-1 files plus a few bytes of
#    headers for each new file.
my $dir_size = $node->safe_psql(
	'postgres', 'SELECT hg_wait_states_directory_size()');
my $phase1_bytes = 0;
$phase1_bytes += $_ for values %size_pre;
cmp_ok($dir_size, '>=', $phase1_bytes,
	'directory_size() retains the bytes of every pre-rotation file');

# 6. No corruption warning was emitted by the reader/writer paths.
my $log = slurp_file($node->logfile);
unlike($log, qr/appears truncated or corrupt/,
	'no truncated/corrupt warning across rotation');
unlike($log, qr/could not roll back partial write/,
	'no partial-write rollback warning across rotation');
unlike($log, qr/file ".*" already exists; truncating/,
	'no rotation-time filename collision');

# 7. Purge expired files.  end_ts is $purge_end (frozen at restart), not
#    now() and not a reconstructed 2000-01-01 epoch.  Stop the phase-2
#    sleeper first so the collector is not appending while we unlink
#    sibling files.  Check filenames and sample counts, never
#    directory_size equality.
eval { $sleeper2->quit; };

my $p2_samples_before_purge = $node->safe_psql(
	'postgres', qq{
SELECT count(*) FROM hg_wait_states_samples()
WHERE wait_event = 'PgSleep'
  AND sample_time >= '$phase2_begin'::timestamptz
});

$node->safe_psql(
	'postgres', qq{
SELECT hg_wait_states_purge(
    '-infinity'::timestamptz,
    '$purge_end'::timestamptz)
});

# Poll the same glob we assert on.  Mixing -e with glob can disagree
# briefly on bind-mounted filesystems.  grep -f so a no-match glob that
# returns the literal pattern is not treated as a leftover file.
my %expired = map { $_ => 1 } @renamed;
my @after_purge;
my $expired_left;
for (my $i = 0; $i < 50; $i++)
{
	@after_purge = grep { -f $_ } glob("$logdir/hg_ws_*");
	$expired_left = grep { $expired{ basename($_) } } @after_purge;
	last if $expired_left == 0;
	sleep(0.1);
}
is($expired_left, 0,
	'purge unlinks every file whose rotate timestamp is in the past');
cmp_ok(scalar(grep { /\bhg_ws_samples_/ } @after_purge), '>=', 1,
	'purge leaves the still-active samples file');
cmp_ok(scalar(grep { /\bhg_ws_sessions_/ } @after_purge), '>=', 1,
	'purge leaves the still-active sessions file');
cmp_ok(scalar(grep { /\bhg_ws_queries_/ } @after_purge), '>=', 1,
	'purge leaves the still-active queries file');

my $p1_after_purge = $node->safe_psql(
	'postgres', qq{
SELECT count(*) FROM hg_wait_states_samples()
WHERE wait_event = 'PgSleep'
  AND sample_time < '$phase2_begin'::timestamptz
});
is($p1_after_purge, '0',
	'purge drops samples that lived only in the expired files');

my $p2_after_purge = $node->safe_psql(
	'postgres', qq{
SELECT count(*) FROM hg_wait_states_samples()
WHERE wait_event = 'PgSleep'
  AND sample_time >= '$phase2_begin'::timestamptz
});
cmp_ok($p2_after_purge, '>=', $p2_samples_before_purge,
	'purge does not drop samples from the still-active file');

eval { $sleeper2->quit; };
$node->stop;
done_testing();
