use strict;
use warnings;

use PostgreSQL::Test::Cluster;
use PostgreSQL::Test::Utils;
use Test::More tests => 41;

# Connect as $user via connstr (not PGUSER).  Bootstrap superuser is the OS
# user that ran initdb (e.g. jenkins); database name "postgres" is unrelated.
sub safe_psql_as
{
	my ($node, $dbname, $user, $sql) = @_;

	return $node->safe_psql(
		$dbname, $sql,
		connstr => $node->connstr($dbname) . " user=$user");
}

sub psql_as
{
	my ($node, $dbname, $user, $sql) = @_;

	return $node->psql(
		$dbname, $sql,
		connstr => $node->connstr($dbname) . " user=$user");
}

sub activate_trace_as
{
	my ($node, $dbname, $user, $users_sql, $dbs_sql) = @_;

	$users_sql = 'NULL' unless defined $users_sql;
	$dbs_sql = 'NULL' unless defined $dbs_sql;

	return safe_psql_as(
		$node, $dbname, $user, qq{
SELECT hg_p_activate(
	'optional filter test',
	$users_sql,
	$dbs_sql,
	NULL,
	NULL,
	NULL);
});
}

# Superuser helpers: default TAP connection is already the bootstrap
# superuser — do not set user=postgres (that role often does not exist).
sub activate_trace
{
	my ($node, $dbname, $users_sql, $dbs_sql) = @_;

	$users_sql = 'NULL' unless defined $users_sql;
	$dbs_sql = 'NULL' unless defined $dbs_sql;

	return $node->safe_psql(
		$dbname, qq{
SELECT hg_p_activate(
	'optional filter test',
	$users_sql,
	$dbs_sql,
	NULL,
	NULL,
	NULL);
});
}

sub trace_defaults_row
{
	my ($node, $trace_id) = @_;

	return $node->safe_psql(
		'postgres', qq{
SELECT max_log_limit || '|' || log_min_duration || '|' ||
	(extract(epoch from (end_time - start_time))::int / 60)
FROM hg_p_active_traces()
WHERE trace_id = '$trace_id';
});
}

sub trace_filter_row
{
	my ($node, $trace_id) = @_;

	return $node->safe_psql(
		'postgres', qq{
SELECT users::text || '|' || databases::text
FROM hg_p_active_traces()
WHERE trace_id = '$trace_id';
});
}

sub deactivate_trace_as
{
	my ($node, $user, $trace_id) = @_;

	return safe_psql_as(
		$node, 'postgres', $user,
		"SELECT hg_p_deactivate('$trace_id');");
}

sub deactivate_trace
{
	my ($node, $trace_id) = @_;

	return $node->safe_psql('postgres',
		"SELECT hg_p_deactivate('$trace_id');");
}

my $node = PostgreSQL::Test::Cluster->new('main');
$node->init;
$node->append_conf('postgresql.conf',
	"shared_preload_libraries = 'hg_profiler'");
$node->append_conf('postgresql.conf', "hg_profiler.enabled = on");
$node->start;

$node->safe_psql(
	'postgres', q{
CREATE ROLE trace_user LOGIN;
CREATE ROLE other_user LOGIN;
CREATE DATABASE traced_db;
CREATE EXTENSION hg_profiler;
GRANT EXECUTE ON FUNCTION hg_p_activate(text, oidvector, oidvector, integer, integer, interval) TO trace_user;
GRANT EXECUTE ON FUNCTION hg_p_deactivate(text) TO trace_user;
GRANT EXECUTE ON FUNCTION hg_p_load_trace(text) TO trace_user;
GRANT EXECUTE ON FUNCTION hg_p_cleanup(text, bool) TO trace_user;
GRANT EXECUTE ON FUNCTION hg_p_traces_list() TO trace_user;
});

$node->safe_psql('traced_db', q{
CREATE EXTENSION hg_profiler;
GRANT EXECUTE ON FUNCTION hg_p_activate(text, oidvector, oidvector, integer, integer, interval) TO trace_user;
GRANT EXECUTE ON FUNCTION hg_p_deactivate(text) TO trace_user;
GRANT EXECUTE ON FUNCTION hg_p_load_trace(text) TO trace_user;
GRANT EXECUTE ON FUNCTION hg_p_cleanup(text, bool) TO trace_user;
GRANT EXECUTE ON FUNCTION hg_p_traces_list() TO trace_user;
});

my $trace_user_oid =
  $node->safe_psql('postgres',
	"SELECT oid FROM pg_roles WHERE rolname = 'trace_user'");
my $other_user_oid =
  $node->safe_psql('postgres',
	"SELECT oid FROM pg_roles WHERE rolname = 'other_user'");
my $superuser_oid =
  $node->safe_psql('postgres',
	"SELECT oid FROM pg_roles WHERE rolname = current_user");
my $postgres_db_oid =
  $node->safe_psql('postgres',
	"SELECT oid FROM pg_database WHERE datname = 'postgres'");
my $traced_db_oid =
  $node->safe_psql('postgres',
	"SELECT oid FROM pg_database WHERE datname = 'traced_db'");

# ---- optional users/databases filters (NULL or empty oidvector) ----
{
	my $tid = activate_trace_as($node, 'postgres', 'trace_user');
	like($tid, qr/^\d{20}$/, 'trace_user may activate with NULL filters');
	is(
		trace_filter_row($node, $tid),
		"$trace_user_oid|$postgres_db_oid",
		'NULL,NULL defaults to current user and current database');
	is(
		trace_defaults_row($node, $tid),
		'100|5|10',
		'NULL max_log_limit/log_min_duration/time_limit use defaults');
	is(deactivate_trace_as($node, 'trace_user', $tid), 't',
		'cleanup NULL,NULL trace');
}

{
	my $tid = activate_trace_as(
		$node, 'postgres', 'trace_user', "''::oidvector", "''::oidvector");
	like($tid, qr/^\d{20}$/, 'trace_user may activate with empty oidvectors');
	is(
		trace_filter_row($node, $tid),
		"$trace_user_oid|$postgres_db_oid",
		'empty oidvectors default to current user and current database');
	is(deactivate_trace_as($node, 'trace_user', $tid), 't',
		'cleanup empty oidvector trace');
}

{
	my $tid = activate_trace_as(
		$node,
		'postgres',
		'trace_user',
		"(SELECT oid::text::oidvector FROM pg_roles WHERE rolname = 'trace_user')",
		'NULL');
	like($tid, qr/^\d{20}$/,
		'trace_user may activate with explicit user and NULL database');
	is(
		trace_filter_row($node, $tid),
		"$trace_user_oid|$postgres_db_oid",
		'NULL database defaults to current database');
	is(deactivate_trace_as($node, 'trace_user', $tid), 't',
		'cleanup user-only trace');
}

{
	my $tid = activate_trace_as(
		$node,
		'postgres',
		'trace_user',
		'NULL',
		"(SELECT oid::text::oidvector FROM pg_database WHERE datname = 'traced_db')");
	like($tid, qr/^\d{20}$/,
		'trace_user may activate with NULL user and explicit database');
	is(
		trace_filter_row($node, $tid),
		"|$traced_db_oid",
		'NULL user traces all users on the specified database');
	is(deactivate_trace_as($node, 'trace_user', $tid), 't',
		'cleanup database-only trace');
}

{
	my $tid = activate_trace($node, 'postgres');
	like($tid, qr/^\d{20}$/, 'superuser may activate with NULL filters');
	is(
		trace_filter_row($node, $tid),
		"$superuser_oid|$postgres_db_oid",
		'superuser NULL,NULL defaults to current user and current database');
	is(deactivate_trace($node, $tid), 't',
		'cleanup superuser NULL,NULL trace');
}

{
	my $tid = activate_trace(
		$node, 'postgres', "''::oidvector", "''::oidvector");
	like($tid, qr/^\d{20}$/,
		'superuser may activate with empty oidvectors');
	is(
		trace_filter_row($node, $tid),
		"$superuser_oid|$postgres_db_oid",
		'superuser empty oidvectors default to current user and current database');
	is(deactivate_trace($node, $tid), 't',
		'cleanup superuser empty oidvector trace');
}

{
	# Connected to traced_db: MyDatabaseId must become the default database.
	my $tid = activate_trace_as($node, 'traced_db', 'trace_user');
	like($tid, qr/^\d{20}$/,
		'trace_user may activate with NULL filters on another database');
	is(
		trace_filter_row($node, $tid),
		"$trace_user_oid|$traced_db_oid",
		'NULL,NULL on traced_db defaults to current user and that database');
	is(deactivate_trace_as($node, 'trace_user', $tid), 't',
		'cleanup traced_db NULL,NULL trace');
}

{
	my $tid = activate_trace(
		$node,
		'postgres',
		"(SELECT oid::text::oidvector FROM pg_roles WHERE rolname = 'other_user')",
		"(SELECT oid::text::oidvector FROM pg_database WHERE datname = 'postgres')");
	like($tid, qr/^\d{20}$/, 'superuser may activate a trace for another role');
	is(
		trace_filter_row($node, $tid),
		"$other_user_oid|$postgres_db_oid",
		'superuser may set users filter to another role');
	is(deactivate_trace($node, $tid), 't',
		'cleanup superuser other-role filter trace');
}

# Capture: both filters unset => only current user on current database.
{
	my $tid = $node->safe_psql(
		'postgres',
		q{
SELECT hg_p_activate(
	'default filter capture',
	NULL,
	NULL,
	10,
	0,
	interval '5 minutes');
});
	like($tid, qr/^\d{20}$/, 'superuser default-filter capture activate');
	is(
		trace_filter_row($node, $tid),
		"$superuser_oid|$postgres_db_oid",
		'default-filter capture stores current user and current database');

	$node->safe_psql('postgres', 'SELECT 401 AS marker_default_self_db;');
	safe_psql_as($node, 'postgres', 'other_user',
		'SELECT 402 AS marker_default_other_user;');
	$node->safe_psql('traced_db', 'SELECT 403 AS marker_default_other_db;');

	is(deactivate_trace($node, $tid), 't',
		'deactivate default-filter capture trace');

	my @out = split /\n/, $node->safe_psql(
		'postgres',
		qq{
SELECT hg_p_load_trace('$tid');
SELECT
	count(*) FILTER (WHERE query LIKE 'SELECT 401 AS marker_default_self_db%') || '|' ||
	count(*) FILTER (WHERE query LIKE 'SELECT 402 AS marker_default_other_user%') || '|' ||
	count(*) FILTER (WHERE query LIKE 'SELECT 403 AS marker_default_other_db%')
FROM _hg_tmp_tbl_p_profiler;
SELECT hg_p_cleanup('$tid', false);
});
	is($out[0], 't', 'default-filter capture rows load');
	is($out[1], '1|0|0',
		'NULL,NULL captures only current user on current database');
	is($out[2], 't', 'cleanup default-filter capture trace');
}

my ($rc, $stdout, $stderr) = psql_as(
	$node,
	'postgres',
	'trace_user',
	q{
SELECT hg_p_activate(
	'bad role filter',
	concat_ws(' ',
		(SELECT oid FROM pg_roles WHERE rolname = 'trace_user'),
		(SELECT oid FROM pg_roles WHERE rolname = 'other_user'))::oidvector,
	(SELECT oid::text::oidvector FROM pg_database WHERE datname = 'postgres'),
	10,
	0,
	interval '5 minutes');
});

isnt($rc, 0, 'non-superuser cannot trace another role');
like(
	$stderr,
	qr/non-superuser .* may only trace its own role/,
	'permission error mentions non-superuser role restriction');

($rc, $stdout, $stderr) = psql_as(
	$node,
	'postgres',
	'trace_user',
	q{
SELECT hg_p_activate(
	'other role only',
	(SELECT oid::text::oidvector FROM pg_roles WHERE rolname = 'other_user'),
	(SELECT oid::text::oidvector FROM pg_database WHERE datname = 'postgres'),
	10,
	0,
	interval '5 minutes');
});

isnt($rc, 0, 'non-superuser cannot activate with only another role');
like(
	$stderr,
	qr/non-superuser .* may only trace its own role/,
	'permission error for other-role-only users filter');

my $trace_id = safe_psql_as(
	$node,
	'postgres',
	'trace_user',
	q{
SELECT hg_p_activate(
	'cross database trace',
	(SELECT oid::text::oidvector FROM pg_roles WHERE rolname = 'trace_user'),
	concat_ws(' ',
		(SELECT oid FROM pg_database WHERE datname = 'postgres'),
		(SELECT oid FROM pg_database WHERE datname = 'traced_db'))::oidvector,
	10,
	0,
	interval '30 minutes');
});

like($trace_id, qr/^\d{20}$/, 'trace activation returns a 20-character trace_id');

safe_psql_as($node, 'postgres', 'trace_user',
	'SELECT 101 AS marker_from_postgres;');
safe_psql_as($node, 'traced_db', 'trace_user',
	'SELECT 202 AS marker_from_traced_db;');
safe_psql_as($node, 'traced_db', 'other_user',
	'SELECT 303 AS marker_from_other_user;');

is(
	safe_psql_as($node, 'postgres', 'trace_user',
		"SELECT hg_p_deactivate('$trace_id');"),
	't',
	'trace owner can deactivate the trace');
my @load_output = split /\n/,
  safe_psql_as(
	$node,
	'postgres',
	'trace_user',
	qq{
SELECT hg_p_load_trace('$trace_id');
SELECT
	count(*) FILTER (WHERE query LIKE 'SELECT 101 AS marker_from_postgres%') || '|' ||
	count(*) FILTER (WHERE query LIKE 'SELECT 202 AS marker_from_traced_db%') || '|' ||
	count(*) FILTER (WHERE query LIKE 'SELECT 303 AS marker_from_other_user%')
FROM _hg_tmp_tbl_p_profiler;
SELECT final_status FROM hg_p_traces_list() WHERE trace_id = '$trace_id';
SELECT hg_p_cleanup('$trace_id', false);
});

is($load_output[0], 't', 'trace rows load into the temp table');
is($load_output[1], '1|1|0', 'trace captures matching users across both databases only');
is($load_output[2], '2', 'deactivated trace is reported as stopped');
is($load_output[3], 't', 'trace owner can clean up an inactive trace');

done_testing();
