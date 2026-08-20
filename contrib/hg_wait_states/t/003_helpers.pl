# ----------------------------------------------------------------------
# 003_helpers.pl
#
# Verifies the SQL helper functions defined in hg_wait_states--1.0.sql,
# focusing on the correctness-critical edges that earlier reviews flagged:
#
#   * hg_wait_states_wait_events must not divide by zero when the chosen
#     window is empty and must compute pct_dbtime as a true numeric
#     percentage (not an integer-divided 0).
#   * hg_wait_states_sql_statements must produce the right top wait
#     event per query (no cross-query "max waittime" matching), must
#     keep CPU-only and wait-only queries (LEFT JOIN, not implicit
#     inner join), and must not double-count dbtime.
#   * hg_wait_states_data must deduplicate properly when a single sample
#     could otherwise join to several sessions rows that share the same
#     pid (the PID-reuse case).
#   * hg_wait_states_purge with a future window must not delete anything;
#     with an all-encompassing past window it must NOT remove the file
#     currently being written (rotate timestamp is in the future) and must
#     not lose any already-collected sample.
#   * hg_wait_states_directory_size returns a non-negative number after
#     collection and is stable across no-op purges.
# ----------------------------------------------------------------------
use strict;
use warnings;

use PostgreSQL::Test::Cluster;
use PostgreSQL::Test::Utils;
use Test::More;
use Time::HiRes qw(sleep);

my $node = PostgreSQL::Test::Cluster->new('hg_ws_helpers');
$node->init;
$node->append_conf(
	'postgresql.conf', qq{
shared_preload_libraries = 'hg_wait_states'
hg_wait_states.database = postgres
hg_wait_states.enable_collection = on
track_activities = on
});
$node->start;
$node->safe_psql('postgres', 'CREATE EXTENSION hg_wait_states');

# ---------------------- wait_events: empty window, no divide-by-zero
my ($rc, $stdout, $stderr) = $node->psql(
	'postgres', q{
SELECT count(*)
FROM hg_wait_states_wait_events(
        '2000-01-01'::timestamptz,
        '2000-01-02'::timestamptz)
}, on_error_stop => 1);
is($rc, 0, 'wait_events on an empty window does not error');
is($stdout, '0', 'wait_events on an empty window returns 0 rows');
unlike($stderr, qr/division by zero/i,
	'wait_events does not trigger division by zero on empty data');

# ----------------------- Generate workload: a CPU-bound query AND a
# wait-bound query so sql_statements has both pure-CPU and pure-wait rows
# to coalesce.  pg_sleep produces a Timeout/PgSleep wait event with no CPU,
# generate_series produces CPU with no wait event.
my $sleeper = $node->background_psql('postgres');
$sleeper->query_until(
	qr/sleep_started/, q{
SELECT 'sleep_started';
SELECT pg_sleep(8);
});

my $burner = $node->background_psql('postgres');
$burner->query_until(
	qr/burn_started/, q{
SELECT 'burn_started';
-- keep the burner busy for serveral sampling cycles (1s each)
SELECT count(*) FROM generate_series(1, 50000000) g;
});

# ----------------------- sql_statements: CPU-only + wait-only present
my ($cpu_only, $wait_only, $bad) = (0, 0, 0);
for (my $i = 0; $i < 60; $i++)
{
	my $stmts = $node->safe_psql(
		'postgres', q{
SELECT
	coalesce(sum((cputime > 0 AND waittime = 0)::int), 0)  AS cpu_only,
	coalesce(sum((waittime > 0 AND cputime = 0)::int), 0)  AS wait_only,
	coalesce(sum((dbtime <= 0)::int), 0)                   AS bad_rows
FROM hg_wait_states_sql_statements()
});
	($cpu_only, $wait_only, $bad) = split /\|/, $stmts;
	last if $cpu_only >= 1 && $wait_only >= 1;
	sleep(0.2);
}

cmp_ok($cpu_only,  '>=', 1, 'sql_statements keeps at least one CPU-only query (LEFT JOIN works)');
cmp_ok($wait_only, '>=', 1, 'sql_statements keeps at least one wait-only query');
is($bad, '0', 'no row in sql_statements reports dbtime <= 0');

# Verify top_waitevent picks the highest-waittime event for ITS OWN query.
# (Bug we are guarding against: maxwt subquery without query_id projection
# would let one query's max match a completely different query.)
my $top_consistent = $node->safe_psql(
	'postgres', q{
WITH s AS (
    SELECT query_id, top_waitevent FROM hg_wait_states_sql_statements()
    WHERE top_waitevent IS NOT NULL
)
SELECT count(*)
FROM s
JOIN LATERAL (
    SELECT wait_event, count(*) AS w
    FROM hg_wait_states_samples()
    WHERE query_id = s.query_id AND wait_event IS NOT NULL
    GROUP BY wait_event
    ORDER BY w DESC LIMIT 1
) actual ON actual.wait_event = s.top_waitevent
});
my $tot_with_event = $node->safe_psql('postgres',
	q{SELECT count(*) FROM hg_wait_states_sql_statements() WHERE top_waitevent IS NOT NULL}
);
is($top_consistent, $tot_with_event,
	'sql_statements.top_waitevent matches the per-query top wait event');

# ----------------------- wait_events: pct_dbtime is a true percentage
my $pct = $node->safe_psql(
	'postgres', q{
SELECT max(pct_dbtime), min(pct_dbtime),
       count(*) FILTER (WHERE pct_dbtime < 0) AS neg_rows,
       count(*) FILTER (WHERE pct_dbtime > 100) AS over_rows
FROM hg_wait_states_wait_events()
});
my ($maxp, $minp, $neg, $over) = split /\|/, $pct;
cmp_ok($maxp, '>', 0,
	'wait_events.pct_dbtime is strictly positive for at least one event');
is($neg, '0',  'no negative pct_dbtime row');
is($over, '0', 'no pct_dbtime > 100 (rounding-safe within numeric)');

# ----------------------- data(): join sanity & PID-uniqueness per row
my $dups = $node->safe_psql(
	'postgres', q{
SELECT count(*)
FROM (
    SELECT session_id, sample_time, count(*) AS c
    FROM hg_wait_states_data()
    GROUP BY 1, 2
    HAVING count(*) > 1
) dup
});
is($dups, '0',
	'hg_wait_states_data() never produces two rows for the same (session_id, sample_time)');

# ----------------------- purge / directory_size
# Rotation intervals are hard-coded (samples=1h, sessions/queries=1d), so a
# fresh test cluster has no rotated files to purge.  We assert the function
# is safe and idempotent in that situation, and that samples readable from
# the files survive a no-op purge intact.
my $size_before = $node->safe_psql(
	'postgres', 'SELECT hg_wait_states_directory_size()');
cmp_ok($size_before, '>', 0,
	'directory_size > 0 after some collection');

my $samples_before = $node->safe_psql(
	'postgres', 'SELECT count(*) FROM hg_wait_states_samples()');

# A future-only window must not unlink anything.
$node->safe_psql(
	'postgres', q{
SELECT hg_wait_states_purge(now() + interval '1 day',
                            'infinity'::timestamptz)});
my $size_after_future_purge =
  $node->safe_psql('postgres', 'SELECT hg_wait_states_directory_size()');
is($size_after_future_purge, $size_before,
	'purging only future files leaves directory_size unchanged');

# An all-encompassing past window must not remove the file currently being
# written to (rts is in the future).  It also must not corrupt the file or
# lose any samples.
$node->safe_psql('postgres',
	q{SELECT hg_wait_states_purge('-infinity'::timestamptz, now())});
my $samples_after_purge = $node->safe_psql(
	'postgres', 'SELECT count(*) FROM hg_wait_states_samples()');
cmp_ok($samples_after_purge, '>=', $samples_before,
	'purge does not silently drop samples from the still-active file');

my $log = slurp_file($node->logfile);
unlike($log, qr/could not remove hg_wait_states file/,
	'purge does not log a removal failure for the active file');

$sleeper->quit;
$burner->quit;
$node->stop;
done_testing();
