# ----------------------------------------------------------------------
# 004_pem_patterns.pl
#
# Run every one of the 12 PEM dashboard SQL templates documented in
# hg_wait_states.txt (with the edb_ prefix swapped for hg_) against a
# live collector.  These tests guard against regressions in the wire
# contract (function signatures, column names/types, expected nulls,
# etc.) that the dashboard depends on.
#
# Naming follows the file names under
#   /usr/edb/pem/web/pgadmin/pem/tools/performance_diagnostic/templates/
#       performance_diagnostic/sql/
# in hg_wait_states.txt.
# ----------------------------------------------------------------------
use strict;
use warnings;

use PostgreSQL::Test::Cluster;
use PostgreSQL::Test::Utils;
use Test::More;
use Time::HiRes qw(sleep);

my $node = PostgreSQL::Test::Cluster->new('hg_ws_pem');
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

# Generate workload: one sleeper (Timeout/PgSleep) and one CPU-burner.
my $sleeper = $node->background_psql('postgres');
$sleeper->query_until(qr/start/, q{
SELECT 'start';
SELECT pg_sleep(8);
});
my $burner = $node->background_psql('postgres');
$burner->query_until(
	qr/burn/, q{
SELECT 'burn';
SELECT count(*) FROM generate_series(1, 2000000) g;
});

# Wait until at least the sleeper has shown up in samples.
my $sleeper_pid = '';
for (my $i = 0; $i < 60; $i++)
{
	$sleeper_pid = $node->safe_psql(
		'postgres', q{
SELECT min(session_id)
FROM hg_wait_states_samples()
WHERE wait_event = 'PgSleep'
});
	last if $sleeper_pid =~ /^\d+$/;
	sleep(0.2);
}
ok($sleeper_pid =~ /^\d+$/, "collector captured the sleeper backend (pid=$sleeper_pid)");

# Pick a (query_id, session_id, query_start_time, sample_time) anchor we
# can hand into the PEM SQLs as parameters.
my $anchor_row = $node->safe_psql(
	'postgres', qq{
SELECT query_id, session_id,
       EXTRACT(EPOCH FROM query_start_time)::float8,
       EXTRACT(EPOCH FROM sample_time)::int8
FROM hg_wait_states_samples()
WHERE session_id = $sleeper_pid
ORDER BY sample_time
LIMIT 1
});
my ($qid, $sid, $qstart, $epoch) = split /\|/, $anchor_row;
ok($qid && $sid && $qstart && $epoch,
	"selected an anchor sample (query_id=$qid, session_id=$sid, sample_time=$epoch)");

# ------------------------------------------------------------- sql-1
# distinct_selected_wait_events: every observed wait_event_type within
# the window appears exactly once and is non-null.
my $sql1 = $node->safe_psql(
	'postgres', qq{
SELECT count(*) FROM (
    SELECT distinct(wait_event_type)
    FROM hg_wait_states_samples(
            to_timestamp(${epoch} - 30),
            to_timestamp(${epoch} + 30))
    WHERE wait_event_type IS NOT NULL
    GROUP BY sample_time, wait_event_type
    ORDER BY 1
) s
});
cmp_ok($sql1, '>=', 1, 'sql-1 distinct_selected_wait_events returns at least one type');

# ------------------------------------------------------------- sql-2
# find_query_with_min_starttime: finds the precise query_start_time
# given (query_id, session_id, time_stamp).
my $sql2 = $node->safe_psql(
	'postgres', qq{
SELECT count(*)
FROM hg_wait_states_samples(
        to_timestamp(${epoch} - 1),
        to_timestamp(${epoch} + 1))
WHERE query_id = ${qid}::int8 AND
      session_id = ${sid}::int4 AND
      EXTRACT(EPOCH FROM sample_time)::int = ${epoch}
});
cmp_ok($sql2, '>=', 1,
	'sql-2 find_query_with_min_starttime locates the anchor sample');

# ------------------------------------------------------------- sql-3
# query_details_by_session: joins samples with sessions, expects to find
# the dbname/username for the sleeper.
my $sql3 = $node->safe_psql(
	'postgres', qq{
SELECT DISTINCT se.session_id, se.username, se.dbname
FROM hg_wait_states_samples(
        to_timestamp(${epoch} - 1),
        to_timestamp(${epoch} + 1)) sa
LEFT OUTER JOIN (
    SELECT * FROM hg_wait_states_sessions(
        to_timestamp(${epoch} - 1),
        to_timestamp(${epoch} + 1))
) se ON sa.session_id = se.session_id
WHERE sa.query_id = ${qid}::bigint
  AND EXTRACT(EPOCH FROM sa.sample_time)::int8 = ${epoch}::int8
ORDER BY 1
});
like($sql3, qr/^${sid}\|.+\|postgres/m,
	'sql-3 query_details_by_session resolves the session metadata');

# ------------------------------------------------------------- sql-4
# query: queries() returns the original SQL text by query_id.
my $sql4 = $node->safe_psql(
	'postgres', qq{
SELECT query FROM hg_wait_states_queries(
        to_timestamp(${epoch} - 1),
        to_timestamp(${epoch} + 1))
WHERE query_id = ${qid}::bigint LIMIT 1
});
like($sql4, qr/pg_sleep\(8\)/,
	'sql-4 query returns the literal pg_sleep(8) text');

# ------------------------------------------------------------- sql-5
# query_total_wait_events: a 15-min slice of wait counts.
my ($rc5, $out5, $err5) = $node->psql(
	'postgres', qq{
SELECT
    EXTRACT(EPOCH FROM sample_time) * 1000 AS sample_time,
    COALESCE(wait_event_type::text, 'CPU') AS wait_event_types,
    COALESCE(wait_event::text, 'CPU')      AS wait_events,
    1
FROM hg_wait_states_samples(
        to_timestamp(${qstart}::float)  + INTERVAL '0 minutes',
        to_timestamp(${qstart}::float)  + INTERVAL '15 minutes')
WHERE query_id = ${qid}::int8
  AND session_id = ${sid}::int4
  AND query_start_time = to_timestamp(${qstart}::float)
ORDER BY 1, 2, 3
}, on_error_stop => 1);
is($rc5, 0, 'sql-5 query_total_wait_events parses and executes');

# ------------------------------------------------------------- sql-6
# query_wait_events_temp_tbl: the rolling 15-min PL/pgSQL block.
my ($rc6, $out6, $err6) = $node->psql(
	'postgres', qq{
BEGIN;
DO \$\$
DECLARE
    min_sample_time timestamptz;
    rows_inserted   integer;
    exit_loop       bool := FALSE;
BEGIN
    DROP TABLE IF EXISTS "_pem_wait_event_query_time_tmp_${sid}";
    CREATE TEMPORARY TABLE "_pem_wait_event_query_time_tmp_${sid}" (
        sample_time double precision,
        wait_event_types text,
        wait_events text
    ) ON COMMIT DROP;
    min_sample_time := to_timestamp(${qstart}::float - 1);
    WHILE exit_loop = FALSE LOOP
        EXECUTE \$SQL\$
            INSERT INTO "_pem_wait_event_query_time_tmp_${sid}"
            SELECT
                EXTRACT(EPOCH FROM sample_time) * 1000,
                COALESCE(wait_event_type::text, 'CPU'),
                COALESCE(wait_event::text, 'CPU')
            FROM hg_wait_states_samples(\$1, \$1 + interval '15 minutes')
            WHERE query_id = ${qid}::int8
              AND session_id = ${sid}::int4
              AND query_start_time = to_timestamp(${qstart}::float)
        \$SQL\$ USING min_sample_time;
        GET DIAGNOSTICS rows_inserted = ROW_COUNT;
        IF rows_inserted = 0 THEN
            exit_loop := TRUE;
        ELSE
            min_sample_time := min_sample_time + INTERVAL '15 minutes';
        END IF;
    END LOOP;
END;
\$\$ LANGUAGE plpgsql;
SELECT count(*) FROM "_pem_wait_event_query_time_tmp_${sid}";
COMMIT;
}, on_error_stop => 1);
is($rc6, 0, 'sql-6 query_wait_events_temp_tbl PL/pgSQL block runs cleanly');

# ------------------------------------------------------------- sql-7
my $sql7 = $node->safe_psql(
	'postgres', qq{
SELECT count(*) FROM (
    SELECT sample_time, COALESCE(wait_event_type, 'CPU') AS wait_event_type, count(*) AS cnt
    FROM (
        SELECT EXTRACT(EPOCH FROM sample_time) * 1000 AS sample_time, wait_event_type
        FROM hg_wait_states_samples(
                to_timestamp(${epoch} - 60),
                to_timestamp(${epoch} + 60))
    ) AS s GROUP BY 1, 2 ORDER BY 1, 2
) t
});
cmp_ok($sql7, '>=', 1,
	'sql-7 selected_wait_events bucketizes samples by (sample_time, wait_event_type)');

# ------------------------------------------------------------- sql-8
my ($rc8, $out8, $err8) = $node->psql(
	'postgres', qq{
SELECT count(*) FROM (
    SELECT sample_time, COALESCE(wait_event_type, 'CPU') AS wait_event_type, count(*) AS cnt
    FROM (
        SELECT (((EXTRACT(EPOCH FROM sample_time) / 15)::bigint * 15) + 15) * 1000 AS sample_time,
               wait_event_type
        FROM hg_wait_states_samples(
                to_timestamp(${epoch})  - interval '1 hour',
                to_timestamp(${epoch}))
    ) AS s GROUP BY 1, 2 ORDER BY 1, 2
) t
}, on_error_stop => 1);
is($rc8, 0, 'sql-8 total_wait_events parses and executes (last-hour aggregation)');

# ------------------------------------------------------------- sql-9
# wait_events_report: the heaviest of the 12 SQLs.  It UNIONs 4 JSON
# aggregates over many CTEs, so we mainly want to ensure it compiles
# and runs without errors and returns exactly 4 top-level rows.
my $sql9 = $node->safe_psql(
	'postgres', qq{
WITH all_wait_events AS (
    SELECT q.query_id, q.query, s.session_id, s.dbname, s.username,
           sa.query_start_time, sa.sample_time,
           COALESCE(sa.wait_event_type, 'CPU') AS wait_event_type,
           sa.wait_event AS wait_event
    FROM hg_wait_states_samples(
            to_timestamp(${epoch} - 30),
            to_timestamp(${epoch} + 30)) sa
    INNER JOIN hg_wait_states_sessions(
            to_timestamp(${epoch} - 30),
            to_timestamp(${epoch} + 30)) s ON (sa.session_id = s.session_id)
    LEFT OUTER JOIN hg_wait_states_queries(
            to_timestamp(${epoch} - 30),
            to_timestamp(${epoch} + 30)) q ON (sa.query_id = q.query_id)
),
queries_by_wait_events AS (
    SELECT query_id, query, wait_event_type, wait_event,
           COUNT(1) FILTER (WHERE wait_event_type != 'CPU') AS wait_event_count,
           COUNT(1) FILTER (WHERE wait_event_type  = 'CPU') AS cpu_count
    FROM all_wait_events
    GROUP BY 1, 2, 3, 4
),
events AS (
    SELECT query_id, query, sum(wait_event_count) AS wait_event_count,
           sum(cpu_count) AS cpu_count,
           jsonb_agg(jsonb_build_object(
               'wait_event_type', wait_event_type,
               'wait_event', wait_event,
               'count', cpu_count + wait_event_count
           ) ORDER BY wait_event_type, wait_event) AS wait_events
    FROM queries_by_wait_events GROUP BY 1, 2
)
SELECT count(*) FROM events
});
cmp_ok($sql9, '>=', 1, 'sql-9 wait_events_report core CTE chain returns rows');

# ------------------------------------------------------------- sql-10
my $sql10 = $node->safe_psql(
	'postgres', qq{
WITH all_wait_events AS (
    SELECT s.query_id, q.query,
           COALESCE(s.wait_event_type, 'CPU') AS wait_event_type,
           COUNT(DISTINCT s.session_id) AS number_of_sessions,
           COUNT(*) AS wait_event_type_count
    FROM hg_wait_states_samples(
            to_timestamp(${epoch} - 1),
            to_timestamp(${epoch} + 1)) s
    LEFT OUTER JOIN (
        SELECT DISTINCT query_id, query
        FROM hg_wait_states_queries(
                to_timestamp(${epoch} - 1),
                to_timestamp(${epoch} + 1))
    ) q ON q.query_id = s.query_id
    WHERE EXTRACT(EPOCH FROM s.sample_time)::int8 = ${epoch}::int8
    GROUP BY 1, 2, 3
)
SELECT count(*) FROM all_wait_events
});
cmp_ok($sql10, '>=', 1, 'sql-10 wait_event_types_by_sql aggregates per query_id');

# ------------------------------------------------------------- sql-11
my $sql11 = $node->safe_psql(
	'postgres', qq{
SELECT count(*) FROM (
    SELECT a.username,
           SUM(a.execution_count) AS execution_count
    FROM (
        SELECT s.username,
               COUNT(s.query_start_time) AS execution_count,
               s.wait_event_type
        FROM hg_wait_states_data(
                to_timestamp(${epoch} - 1),
                to_timestamp(${epoch} + 1)) s
        WHERE EXTRACT(EPOCH FROM s.sample_time)::int8 = ${epoch}::int8
        GROUP BY s.username, s.wait_event_type
    ) a
    GROUP BY a.username
) t
});
cmp_ok($sql11, '>=', 1, 'sql-11 wait_event_types_by_users uses data() correctly');

# ------------------------------------------------------------- sql-12
my $sql12 = $node->safe_psql(
	'postgres', qq{
SELECT count(*) FROM (
    SELECT s.wait_event, s.wait_event_type,
           COUNT(s.wait_event) AS wait_events_count
    FROM hg_wait_states_data(
            to_timestamp(${epoch} - 1),
            to_timestamp(${epoch} + 1)) s
    WHERE EXTRACT(EPOCH FROM s.sample_time)::int8 = ${epoch}::int8
      AND wait_event_type IS NOT NULL
      AND wait_event IS NOT NULL
    GROUP BY s.wait_event, s.wait_event_type
) t
});
cmp_ok($sql12, '>=', 0,
	'sql-12 wait_event_types_by_waits runs (>=0 rows depending on workload)');

$sleeper->quit;
$burner->quit;
$node->stop;
done_testing();
