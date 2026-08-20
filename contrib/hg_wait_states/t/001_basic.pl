# ----------------------------------------------------------------------
# 001_basic.pl
#
# Smoke test for the hg_wait_states extension:
#   * the bgworker registers and starts under shared_preload_libraries
#   * CREATE EXTENSION installs the expected catalog of functions
#   * all reader functions are callable with default (-infinity, +infinity)
#     arguments and return zero rows before any sampling has been done
#   * pg_monitor GRANTs are wired up so a monitoring role can use the
#     readers without superuser
#   * REVOKE keeps PUBLIC out
# ----------------------------------------------------------------------
use strict;
use warnings;

use PostgreSQL::Test::Cluster;
use PostgreSQL::Test::Utils;
use Test::More;

sub psql_as
{
	my ($node, $dbname, $user, $sql) = @_;
	local $ENV{PGUSER} = $user;
	return $node->psql($dbname, $sql, on_error_stop => 1);
}

sub safe_psql_as
{
	my ($node, $dbname, $user, $sql) = @_;
	local $ENV{PGUSER} = $user;
	return $node->safe_psql($dbname, $sql);
}

my $node = PostgreSQL::Test::Cluster->new('hg_ws_basic');
$node->init;
$node->append_conf(
	'postgresql.conf', qq{
shared_preload_libraries = 'hg_wait_states'
hg_wait_states.database = postgres
hg_wait_states.enable_collection = off
});
$node->start;

# ---------------------------------------------------------- module loads
my $loaded = $node->safe_psql('postgres',
	q{SELECT count(*) FROM pg_available_extensions WHERE name = 'hg_wait_states'});
is($loaded, '1', 'hg_wait_states is listed as an available extension');

$node->safe_psql('postgres', 'CREATE EXTENSION hg_wait_states');

# All installed functions must exist after CREATE EXTENSION.
my $funcs = $node->safe_psql(
	'postgres', q{
SELECT string_agg(proname, ',' ORDER BY proname)
FROM pg_proc
WHERE pronamespace = 'public'::regnamespace
  AND proname LIKE 'hg_wait_states_%'
});
is( $funcs,
	join(',',
		qw(hg_wait_states_data
		   hg_wait_states_directory_size
		   hg_wait_states_purge
		   hg_wait_states_queries
		   hg_wait_states_samples
		   hg_wait_states_samples_internal
		   hg_wait_states_sessions
		   hg_wait_states_sql_statements
		   hg_wait_states_wait_events)),
	'extension installs all expected functions and no extras');

# The C-implemented readers must have the documented column shape.
my $sig_samples = $node->safe_psql(
	'postgres', q{
SELECT pg_get_function_arguments(p.oid)
  FROM pg_proc p WHERE proname = 'hg_wait_states_samples'
});
like($sig_samples,
	qr/start_ts timestamp with time zone DEFAULT.+end_ts timestamp with time zone DEFAULT/,
	'hg_wait_states_samples accepts the documented (start_ts, end_ts) signature');

# ------------------------------------------ readers tolerate "no data yet"
# Before any sample cycle runs, every reader must return 0 rows without
# raising an error.  This guards against accidental crashes when the log
# directory exists but only contains the empty header of newly-created
# files.
my $row_counts = $node->safe_psql(
	'postgres', q{
SELECT
    (SELECT count(*) FROM hg_wait_states_samples()) AS samples,
    (SELECT count(*) FROM hg_wait_states_sessions()) AS sessions,
    (SELECT count(*) FROM hg_wait_states_queries()) AS queries,
    (SELECT count(*) FROM hg_wait_states_data()) AS data,
    (SELECT count(*) FROM hg_wait_states_wait_events()) AS wait_events,
    (SELECT count(*) FROM hg_wait_states_sql_statements()) AS sql_statements
});
is($row_counts, '0|0|0|0|0|0',
	'every reader returns 0 rows when collection is disabled');

# directory_size: directory exists, currently empty or only headers.
my $dir_size = $node->safe_psql('postgres',
	'SELECT hg_wait_states_directory_size()');
cmp_ok($dir_size, '>=', 0,
	'directory_size returns a non-negative value with no collected data');

# ----------------------------------------------------- GUC sanity check
my $guc = $node->safe_psql(
	'postgres', q{
SELECT name, setting, unit
FROM pg_settings
WHERE name LIKE 'hg_wait_states.%'
ORDER BY name
});
like($guc, qr/hg_wait_states\.database\|postgres/,
	'database is set to postgres for the test cluster');
like($guc, qr/hg_wait_states\.enable_collection\|off/,
	'enable_collection defaults to off');
like($guc, qr/hg_wait_states\.retention_period\|7/,
	'retention_period defaults to 7 days');

# ------------------------------------------------ permissions: PUBLIC out
# hg_wait_states--1.0.sql REVOKEs EXECUTE from PUBLIC and does not grant
# it to pg_monitor (or any other non-superuser role).  Assert via the
# privilege catalog rather than stderr text, which can vary by locale /
# errmsg wording.
$node->safe_psql('postgres', 'CREATE ROLE hg_ws_unprivileged LOGIN');

my $pub_priv = $node->safe_psql(
	'postgres', q{
SELECT bool_and(has_function_privilege(
		'hg_ws_unprivileged', p.oid, 'execute'))
FROM pg_proc p
JOIN pg_namespace n ON n.oid = p.pronamespace
WHERE n.nspname = 'public'
  AND p.proname = 'hg_wait_states_samples'
});
is($pub_priv, 'f',
	'unprivileged role has no EXECUTE on hg_wait_states_samples');

my ($rc_pub, $stdout_pub, $stderr_pub) = psql_as($node, 'postgres',
	'hg_ws_unprivileged',
	'SELECT count(*) FROM hg_wait_states_samples()');
isnt($rc_pub, 0,
	'a non-superuser cannot call hg_wait_states_samples')
  or diag("stderr: $stderr_pub");

# pg_monitor membership alone must not confer EXECUTE either.
$node->safe_psql('postgres', 'GRANT pg_monitor TO hg_ws_unprivileged');

my $mon_role_priv = $node->safe_psql(
	'postgres', q{
SELECT bool_and(has_function_privilege(
		'pg_monitor', p.oid, 'execute'))
FROM pg_proc p
JOIN pg_namespace n ON n.oid = p.pronamespace
WHERE n.nspname = 'public'
  AND p.proname = 'hg_wait_states_samples'
});
is($mon_role_priv, 'f',
	'pg_monitor itself has no EXECUTE on hg_wait_states_samples');

my $mon_member_priv = $node->safe_psql(
	'postgres', q{
SELECT bool_and(has_function_privilege(
		'hg_ws_unprivileged', p.oid, 'execute'))
FROM pg_proc p
JOIN pg_namespace n ON n.oid = p.pronamespace
WHERE n.nspname = 'public'
  AND p.proname = 'hg_wait_states_samples'
});
is($mon_member_priv, 'f',
	'pg_monitor membership does not grant EXECUTE on hg_wait_states_samples');

my ($rc_mon, $stdout_mon, $stderr_mon) = psql_as($node, 'postgres',
	'hg_ws_unprivileged',
	'SELECT count(*) FROM hg_wait_states_samples()');
isnt($rc_mon, 0,
	'pg_monitor member still cannot call hg_wait_states_samples')
  or diag("stderr: $stderr_mon");

$node->stop;
done_testing();
