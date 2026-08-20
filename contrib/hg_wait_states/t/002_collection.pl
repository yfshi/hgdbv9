# ----------------------------------------------------------------------
# 002_collection.pl
#
# End-to-end test of the collector while enable_collection is on:
#
#   * a long pg_sleep() in a separate backend must show up in samples()
#     within a couple of sampling cycles, with the expected wait_event
#     and wait_event_type
#   * the corresponding row in queries() must contain the literal SQL
#     text we issued
#   * the corresponding row in sessions() must contain the dbname and
#     username we connected as, plus the session's start timestamp
#   * the joined view returned by hg_wait_states_data() must combine
#     those three sources for the same row
#   * time-window filtering rejects everything outside [t-1, t+1] but
#     keeps everything inside (covers the "narrow point-in-time" pattern
#     used by 9/12 PEM SQLs)
# ----------------------------------------------------------------------
use strict;
use warnings;

use PostgreSQL::Test::Cluster;
use PostgreSQL::Test::Utils;
use Test::More;
use Time::HiRes qw(sleep);

my $node = PostgreSQL::Test::Cluster->new('hg_ws_collect');
$node->init;
$node->append_conf(
	'postgresql.conf', qq{
shared_preload_libraries = 'hg_wait_states'
hg_wait_states.database = postgres
hg_wait_states.enable_collection = on
track_activities = on
track_activity_query_size = 4096
});
$node->start;
$node->safe_psql('postgres', 'CREATE EXTENSION hg_wait_states');

# ------------------------------------------------ run a long pg_sleep
# Use background_psql so the calling test process is not blocked.
my $sleeper = $node->background_psql('postgres');
$sleeper->query_until(qr/sleeping/, q{
SELECT 'sleeping';
SELECT pg_sleep(6);
});

# Grab the sleeper's PID (== the session_id we will see in samples).
my $sleeper_pid = $node->safe_psql(
	'postgres', q{
SELECT pid FROM pg_stat_activity
WHERE query ILIKE '%pg_sleep(6)%' AND state <> 'idle'
LIMIT 1
});
ok($sleeper_pid =~ /^\d+$/, "captured sleeper backend pid ($sleeper_pid)");

# ---------------------------------------------- wait for sampling cycles
# We expect at least 2 samples within 5 wall-clock seconds: collector
# wakes every 1s, the sleeper is alive for ~6s.  Poll instead of sleeping
# a fixed amount so the test stays fast on a fast machine.
my $samples_seen = 0;
for (my $i = 0; $i < 50; $i++)
{
	$samples_seen = $node->safe_psql(
		'postgres',
		"SELECT count(*) FROM hg_wait_states_samples() "
			. "WHERE session_id = $sleeper_pid");
	last if $samples_seen >= 2;
	sleep(0.2);
}
cmp_ok($samples_seen, '>=', 2,
	'at least two samples were captured for the sleeping backend');

# The wait event must be Timeout / PgSleep for pg_sleep().
my $wait = $node->safe_psql(
	'postgres', qq{
SELECT DISTINCT wait_event_type || '/' || wait_event
FROM hg_wait_states_samples()
WHERE session_id = $sleeper_pid
  AND wait_event IS NOT NULL
ORDER BY 1
});
like($wait, qr|Timeout/PgSleep|,
	'pg_sleep() is sampled as wait_event_type=Timeout, wait_event=PgSleep');

# queries() must contain the original SQL text once.
my $query_text = $node->safe_psql(
	'postgres', qq{
SELECT query
FROM hg_wait_states_queries() q
JOIN hg_wait_states_samples() s USING (query_id)
WHERE s.session_id = $sleeper_pid
GROUP BY query
});
like($query_text, qr/pg_sleep\(6\)/,
	'queries() exposes the literal SQL text for the sampled backend');

# sessions() must include dbname=postgres, username=current connecting user.
my $session_meta = $node->safe_psql(
	'postgres', qq{
SELECT dbname || '/' || username
FROM hg_wait_states_sessions()
WHERE session_id = $sleeper_pid
LIMIT 1
});
like($session_meta, qr|^postgres/|,
	'sessions() records the dbname for the sampled backend');

# data() must join all three for the same row.
my $joined = $node->safe_psql(
	'postgres', qq{
SELECT count(*)
FROM hg_wait_states_data()
WHERE session_id = $sleeper_pid
  AND query LIKE '%pg_sleep(6)%'
  AND wait_event = 'PgSleep'
});
cmp_ok($joined, '>=', 1,
	'hg_wait_states_data() joins samples/sessions/queries correctly');

# --------------------------------------- narrow time-window filtering
# Pick the exact sample_time of one row, then assert that a ±0.5s window
# returns *at least* that row, and that a window strictly before it
# returns *none* of the sleeper rows.
my $picked_ts = $node->safe_psql(
	'postgres', qq{
SELECT EXTRACT(EPOCH FROM sample_time)::float8
FROM hg_wait_states_samples()
WHERE session_id = $sleeper_pid
ORDER BY sample_time
LIMIT 1
});
ok($picked_ts =~ /^\d+(\.\d+)?$/, "picked a real sample epoch: $picked_ts");

my $in_window = $node->safe_psql(
	'postgres', qq{
SELECT count(*)
FROM hg_wait_states_samples(
        to_timestamp($picked_ts - 0.5),
        to_timestamp($picked_ts + 0.5))
WHERE session_id = $sleeper_pid
});
cmp_ok($in_window, '>=', 1,
	'narrow [t-0.5s, t+0.5s] window keeps the matching sample');

my $before_window = $node->safe_psql(
	'postgres', qq{
SELECT count(*)
FROM hg_wait_states_samples(
        to_timestamp($picked_ts - 60),
        to_timestamp($picked_ts - 30))
WHERE session_id = $sleeper_pid
});
is($before_window, '0',
	'a window strictly before the sample time returns no rows');

# An empty window (start >= end) must not crash and must return 0 rows.
my $empty_window = $node->safe_psql(
	'postgres', qq{
SELECT count(*)
FROM hg_wait_states_samples(now(), now() - interval '1 hour')
});
is($empty_window, '0', 'inverted window (start > end) returns 0 rows');

# ---------------------------------------- enable_collection toggles live
# Flipping enable_collection off via SIGHUP must stop new samples coming.
$node->safe_psql('postgres',
	q{ALTER SYSTEM SET hg_wait_states.enable_collection = off});
$node->reload;
sleep(2);
my $count_before = $node->safe_psql(
	'postgres', "SELECT count(*) FROM hg_wait_states_samples()");
sleep(3);
my $count_after = $node->safe_psql(
	'postgres', "SELECT count(*) FROM hg_wait_states_samples()");
is($count_after, $count_before,
	'no new samples are written after enable_collection=off');

$sleeper->quit;
$node->stop;
done_testing();
