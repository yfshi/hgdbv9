# ----------------------------------------------------------------------
# 003_helpers.pl
#
# Four checks on the SQL helpers, and nothing else:
#   * wait_events on an empty window does not error and returns 0 rows
#   * sql_statements keeps pg_sleep with top_waitevent = PgSleep
#   * sql_statements keeps a different wait query whose top is not PgSleep
#   * every sql_statements row has dbtime = cputime + waittime
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

# 1. Empty window: NULLIF(dbtime,0) must keep this from erroring.
my ($rc, $stdout, $stderr) = $node->psql(
	'postgres', q{
SELECT count(*)
FROM hg_wait_states_wait_events(
        '2000-01-01'::timestamptz,
        '2000-01-02'::timestamptz)
}, on_error_stop => 1);
is($rc, 0, 'wait_events on an empty window does not error');
is($stdout, '0', 'wait_events on an empty window returns 0 rows');

# Two idle waits with distinct query_ids and wait events.  Take the
# advisory lock before advertising lock_held, so the waiter cannot
# acquire it first.  Identify rows by backend pid, not query text.
my $holder = $node->background_psql('postgres');
$holder->set_query_timer_restart();
$holder->query_until(
	qr/lock_held/, q{
SELECT pg_advisory_lock(981001);
SELECT 'lock_held';
SELECT pg_sleep(60);
});

my $waiter = $node->background_psql('postgres');
$waiter->set_query_timer_restart();
$waiter->query_until(
	qr/lock_wait_started/, q{
SELECT 'lock_wait_started';
SELECT pg_advisory_lock(981001) FROM generate_series(1, 1) g;
});

my $sleeper = $node->background_psql('postgres');
$sleeper->set_query_timer_restart();
$sleeper->query_until(
	qr/sleep_started/, q{
SELECT 'sleep_started';
SELECT pg_sleep(61);
});

my ($sleep_pid, $waiter_pid, $holder_pid) = (0, 0, 0);
for (my $i = 0; $i < 20; $i++)
{
	$sleep_pid = $node->safe_psql(
		'postgres', q{
SELECT coalesce((
    SELECT pid FROM pg_stat_activity
    WHERE query LIKE '%pg_sleep(61)%' AND pid <> pg_backend_pid()
    LIMIT 1), 0)
});
	$waiter_pid = $node->safe_psql(
		'postgres', q{
SELECT coalesce((
    SELECT pid FROM pg_stat_activity
    WHERE query LIKE '%generate_series(1, 1)%' AND pid <> pg_backend_pid()
    LIMIT 1), 0)
});
	$holder_pid = $node->safe_psql(
		'postgres', q{
SELECT coalesce((
    SELECT pid FROM pg_stat_activity
    WHERE query LIKE '%pg_sleep(60)%' AND pid <> pg_backend_pid()
    LIMIT 1), 0)
});
	last if $sleep_pid > 0 && $waiter_pid > 0;
	sleep(0.1);
}

# One sql_statements() scan per poll.  Accept any top other than PgSleep
# for the lock waiter so a rename of the advisory wait event cannot flake.
# Do not require cputime/waittime to be exactly zero.
my ($sleep_ok, $lock_ok, $bad_sum) = (0, 0, 0);
for (my $i = 0; $i < 25; $i++)
{
	my $row = $node->safe_psql(
		'postgres', qq{
WITH sleep_q AS (
    SELECT DISTINCT query_id
    FROM hg_wait_states_samples()
    WHERE session_id = ${sleep_pid}::int4
),
lock_q AS (
    SELECT DISTINCT query_id
    FROM hg_wait_states_samples()
    WHERE session_id = ${waiter_pid}::int4
),
stmts AS (
    SELECT * FROM hg_wait_states_sql_statements()
)
SELECT
    count(*) FILTER (
        WHERE query_id IN (SELECT query_id FROM sleep_q)
          AND waittime > 0
          AND top_waitevent = 'PgSleep'),
    count(*) FILTER (
        WHERE query_id IN (SELECT query_id FROM lock_q)
          AND waittime > 0
          AND top_waitevent IS DISTINCT FROM 'PgSleep'),
    count(*) FILTER (
        WHERE dbtime IS DISTINCT FROM (cputime + waittime))
FROM stmts
});
	($sleep_ok, $lock_ok, $bad_sum) = split /\|/, $row;
	last if $sleep_ok >= 1 && $lock_ok >= 1;
	sleep(0.2);
}

# pg_sleep(61) only keeps the backend in PgSleep until we have a sample.
# Terminate as soon as the snapshot is good so we do not wait for the
# sleep to finish (and so quit() is not blocked on pg_sleep(60/61)).
$node->safe_psql(
	'postgres', qq{
SELECT pg_terminate_backend(pid)
FROM pg_stat_activity
WHERE pid IN (
        ${sleep_pid}::int4,
        ${waiter_pid}::int4,
        ${holder_pid}::int4)
});
eval { $sleeper->quit; };
eval { $waiter->quit; };
eval { $holder->quit; };

cmp_ok($sleep_ok, '>=', 1,
	'sql_statements keeps pg_sleep with top_waitevent = PgSleep');
cmp_ok($lock_ok, '>=', 1,
	'sql_statements keeps the lock waiter with a non-PgSleep top');
is($bad_sum, '0',
	'sql_statements dbtime equals cputime + waittime');

$node->stop;
done_testing();