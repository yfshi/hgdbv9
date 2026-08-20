# ----------------------------------------------------------------------
# 005_restart.pl
#
# Restart-resilience tests:
#
#   * After a clean cluster restart the collector must re-discover the
#     latest non-rotated sessions/queries/samples files and *append* to
#     them, not silently truncate them.  Therefore samples captured
#     before the restart must still be readable after the restart, and
#     new samples must land in the same file (or, if rotated, in a new
#     one) without producing "corrupt or truncated" warnings.
#
#   * After an immediate (kill -9 style) crash the collector must
#     refuse to mix old and new records in a single file: either the
#     prior file is reopened cleanly, or a brand new file is created.
#     Either way, the reader functions must continue to return a
#     consistent view (no half-records, no errors).
#
#   * The shared-state worker_pid lock must be cleared on shutdown so
#     a second collector can take over after restart.
# ----------------------------------------------------------------------
use strict;
use warnings;

use PostgreSQL::Test::Cluster;
use PostgreSQL::Test::Utils;
use Test::More;
use Time::HiRes qw(sleep);

my $node = PostgreSQL::Test::Cluster->new('hg_ws_restart');
$node->init;
$node->append_conf(
	'postgresql.conf', qq{
shared_preload_libraries = 'hg_wait_states'
hg_wait_states.database = postgres
hg_wait_states.enable_collection = on
});
$node->start;
$node->safe_psql('postgres', 'CREATE EXTENSION hg_wait_states');

# ------------------------------------------------ phase 1: pre-restart
my $pre = $node->background_psql('postgres');
$pre->query_until(qr/pre/, q{
SELECT 'pre';
SELECT pg_sleep(5);
});
sleep(3);

my $pre_samples = $node->safe_psql(
	'postgres', 'SELECT count(*) FROM hg_wait_states_samples()');
cmp_ok($pre_samples, '>=', 1,
	'at least one sample was captured before the restart');

# Grab the list of log files now; we will check that they still exist
# (or have been appended to) after the restart.
my $data_dir = $node->data_dir;
opendir(my $dh, "$data_dir/hg_wait_states")
	or die "cannot open log dir: $!";
my @before_files = sort grep { /^hg_ws_/ } readdir($dh);
closedir($dh);
cmp_ok(scalar @before_files, '>=', 3,
	'pre-restart log directory contains at least sessions/queries/samples files');

my %sizes_before;
for my $f (@before_files)
{
	$sizes_before{$f} = -s "$data_dir/hg_wait_states/$f";
}

$pre->quit;

# ------------------------------------------------ clean restart
$node->restart;
sleep(2);

# Collector must have re-registered and cleared worker_pid.
my $worker_pid = $node->safe_psql(
	'postgres', q{
SELECT count(*)
FROM pg_stat_activity
WHERE backend_type = 'hg_wait_states collector'
});
is($worker_pid, '1',
	'exactly one collector backend is alive after restart');

# Older samples must still be readable.
my $post_samples = $node->safe_psql(
	'postgres', q{SELECT count(*) FROM hg_wait_states_samples()});
cmp_ok($post_samples, '>=', $pre_samples,
	'samples captured before the restart remain readable afterwards');

# ------------------------------------------------ phase 2: post-restart
my $post = $node->background_psql('postgres');
$post->query_until(qr/post/, q{
SELECT 'post';
SELECT pg_sleep(5);
});
sleep(3);

my $grown_samples = $node->safe_psql(
	'postgres', q{SELECT count(*) FROM hg_wait_states_samples()});
cmp_ok($grown_samples, '>', $post_samples,
	'new samples are written after the restart');

# Every pre-restart log file must either still exist (possibly with
# strictly greater size if appended to) or have been rotated away to a
# new file.  We never want to see it shrink.
opendir($dh, "$data_dir/hg_wait_states")
	or die "cannot open log dir: $!";
my @after_files = readdir($dh);
closedir($dh);
my %after = map { $_ => -s "$data_dir/hg_wait_states/$_" } @after_files;

my $shrunk = 0;
foreach my $f (keys %sizes_before)
{
	next unless exists $after{$f};
	$shrunk++ if $after{$f} < $sizes_before{$f};
}
is($shrunk, 0,
	'no pre-restart log file was truncated by the post-restart collector');

# ----------------------------------- corrupt/truncate warnings absent
my $log = slurp_file($node->logfile);
unlike($log, qr/appears truncated or corrupt/,
	'no "truncated or corrupt" warning in the postmaster log');
unlike($log, qr/could not roll back partial write/,
	'no rollback-failure warning in the postmaster log');

# ------------------------------------------------ phase 3: crash test
# Send SIGKILL-equivalent stop.  PostgreSQL::Test::Cluster::stop with
# mode 'immediate' simulates a crash from the collector's point of view.
$post->quit;
$node->stop('immediate');
$node->start;
sleep(2);

my $post_crash_samples = $node->safe_psql(
	'postgres', q{SELECT count(*) FROM hg_wait_states_samples()});
cmp_ok($post_crash_samples, '>=', $pre_samples,
	'samples written before the immediate stop are still readable after restart');

my $crash_log = slurp_file($node->logfile);
unlike($crash_log, qr/could not write to hg_wait_states/,
	'no write-error from collector after immediate-stop restart');

# Trigger one more sampling cycle and make sure the collector is alive.
my $final = $node->background_psql('postgres');
$final->query_until(qr/final/, q{
SELECT 'final';
SELECT pg_sleep(3);
});
sleep(2);
my $final_samples = $node->safe_psql(
	'postgres', q{SELECT count(*) FROM hg_wait_states_samples()});
cmp_ok($final_samples, '>', $post_crash_samples,
	'collector resumes sampling after an immediate stop / start cycle');

$final->quit;
$node->stop;
done_testing();
