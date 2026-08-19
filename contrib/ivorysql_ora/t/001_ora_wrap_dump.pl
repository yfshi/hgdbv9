# Copyright (c) 2026, IvorySQL Global Development Group

use strict;
use warnings;

use File::Path qw(make_path);
use PostgresNode;
use TestLib;
use Test::More;

my $configure = '';
IPC::Run::run([ 'pg_config', '--configure' ], '>', \$configure)
  or BAIL_OUT('could not read the PostgreSQL configure options');

my $gmssl_configured =
  $configure =~ /(?:^|\s)'?--with-gmssl(?:=[^\s']+)?'?(?=\s|$)/;
my $openssl_configured =
  $configure =~ /(?:^|\s)'?--with-(?:openssl|ssl=openssl)'?(?=\s|$)/;

plan skip_all => 'PL/SQL wrap dump tests require GmSSL or OpenSSL'
  unless $gmssl_configured || $openssl_configured;

my $node = get_new_node('ora_wrap_dump');
$node->init(extra => [ '-m', 'oracle' ]);
$node->start;

my $workdir = $node->basedir . '/ora_wrap_dump_files';
my $dump_schema = 'ora_wrap_dump_test';
my $plain_restore_db = 'ora_wrap_dump_plain_restore';
my $restore_db = 'ora_wrap_dump_restore';
my $plain_dump = "$workdir/ora_wrap_dump.sql";
my $archive_dump = "$workdir/ora_wrap_dump.dump";
my $oracle_port = $node->safe_psql('postgres', 'SHOW ivorysql.port');
my $oracle_user = $node->safe_psql('postgres',
	"SELECT rolname FROM pg_roles WHERE rolname = 'system' LIMIT 1");
my $source_connstr = "port=$oracle_port host=" . $node->host .
	" dbname='postgres' user='$oracle_user'";
my $plain_restore_connstr = "port=$oracle_port host=127.0.0.1" .
	" dbname='$plain_restore_db' user='$oracle_user'";
my $restore_connstr = "port=$oracle_port host=" . $node->host .
	" dbname='$restore_db' user='$oracle_user'";
my ($ret, $stdout, $stderr);

make_path($workdir);

# Create wrapped and ordinary Oracle objects in one source schema.
($ret, $stdout, $stderr) = $node->psql(
	'postgres',
	qq[
CREATE SCHEMA $dump_schema;
CREATE TABLE $dump_schema.call_log(value number);
CREATE OR REPLACE FUNCTION $dump_schema.wrapped_fn RETURN NUMBER WRAPPED IS
BEGIN
  RETURN 801;
END;
/
CREATE OR REPLACE FUNCTION $dump_schema.plain_fn RETURN NUMBER IS
BEGIN
  RETURN 802;
END;
/
CREATE OR REPLACE PROCEDURE $dump_schema.wrapped_proc WRAPPED IS
BEGIN
  INSERT INTO $dump_schema.call_log VALUES (803);
END;
/
CREATE OR REPLACE PACKAGE $dump_schema.wrapped_pkg WRAPPED IS
  FUNCTION get_value RETURN NUMBER;
END;
/
CREATE OR REPLACE PACKAGE BODY $dump_schema.wrapped_pkg WRAPPED IS
  FUNCTION get_value RETURN NUMBER IS
  BEGIN
    RETURN 804;
  END;
END;
/
], connstr => $source_connstr, on_error_stop => 1);
is($ret, 0, 'source schema with wrapped and ordinary objects is created');

$node->command_ok(
	[
		'pg_dump', '--format=plain', "--schema=$dump_schema", '--file',
		$plain_dump, '--dbname', $source_connstr
	],
	'pg_dump writes a plain dump containing mixed wrapped and ordinary objects');
my $plain_dump_content = slurp_file($plain_dump);
like($plain_dump_content, qr/ivorysql wrapped v1/,
	'plain dump retains wrapped payloads');
unlike($plain_dump_content, qr/RETURN 801/,
	'plain dump does not expose wrapped function source');
unlike($plain_dump_content, qr/RETURN 804/,
	'plain dump does not expose wrapped package body source');
like($plain_dump_content, qr/RETURN 802/,
	'plain dump retains ordinary function source');

$node->safe_psql('postgres', "CREATE DATABASE $plain_restore_db");
$node->command_ok(
	[
		'psql', '-X', '-v', 'ON_ERROR_STOP=1', '--dbname',
		$plain_restore_connstr, '-f', $plain_dump
	],
	'psql restores mixed wrapped and ordinary objects from the plain dump');
($ret, $stdout, $stderr) = $node->psql(
	$plain_restore_db,
	qq[
SELECT $dump_schema.wrapped_fn();
SELECT $dump_schema.plain_fn();
CALL $dump_schema.wrapped_proc();
SELECT $dump_schema.wrapped_pkg.get_value();
SELECT value FROM $dump_schema.call_log;
], connstr => $plain_restore_connstr, on_error_stop => 1);
is($ret, 0, 'plain dump restored objects execute successfully');
like($stdout, qr/^801\n802\n804\n803$/m,
	'plain dump restored functions, procedure, and package preserve behavior');
($ret, $stdout, $stderr) = $node->psql(
	$plain_restore_db,
	qq[SELECT prosrc LIKE 'ivorysql wrapped v1%'
      FROM pg_proc
     WHERE oid = '$dump_schema.wrapped_fn()'::regprocedure],
	connstr => $plain_restore_connstr, on_error_stop => 1);
is($ret, 0, 'plain dump restored wrapped catalog query succeeds');
chomp($stdout);
is($stdout, 't', 'plain dump restored wrapped function remains hidden');
($ret, $stdout, $stderr) = $node->psql(
	$plain_restore_db,
	qq[SELECT prosrc NOT LIKE 'ivorysql wrapped v1%'
      FROM pg_proc
     WHERE oid = '$dump_schema.plain_fn()'::regprocedure],
	connstr => $plain_restore_connstr, on_error_stop => 1);
is($ret, 0, 'plain dump restored ordinary catalog query succeeds');
chomp($stdout);
is($stdout, 't', 'plain dump restored ordinary function remains ordinary');

$node->command_ok(
	[
		'pg_dump', '--format=custom', "--schema=$dump_schema", '--file',
		$archive_dump, '--dbname', $source_connstr
	],
	'pg_dump writes a custom archive for pg_restore');
$node->safe_psql('postgres', "CREATE DATABASE $restore_db");
$node->command_ok(
	[
		'pg_restore', '--no-owner', '--no-privileges', '--dbname',
		$restore_connstr, $archive_dump
	],
	'pg_restore restores mixed wrapped and ordinary objects');

($ret, $stdout, $stderr) = $node->psql(
	$restore_db,
	qq[
SELECT $dump_schema.wrapped_fn();
SELECT $dump_schema.plain_fn();
CALL $dump_schema.wrapped_proc();
SELECT $dump_schema.wrapped_pkg.get_value();
SELECT value FROM $dump_schema.call_log;
], connstr => $restore_connstr, on_error_stop => 1);
is($ret, 0, 'restored wrapped and ordinary objects execute successfully');
like($stdout, qr/^801\n802\n804\n803$/m,
	'restored functions, procedure, and package preserve their behavior');
($ret, $stdout, $stderr) = $node->psql(
	$restore_db,
	qq[SELECT prosrc LIKE 'ivorysql wrapped v1%'
      FROM pg_proc
     WHERE oid = '$dump_schema.wrapped_fn()'::regprocedure],
	connstr => $restore_connstr, on_error_stop => 1);
is($ret, 0, 'restored wrapped catalog query succeeds');
chomp($stdout);
is($stdout, 't', 'restored wrapped function remains hidden in pg_proc');
($ret, $stdout, $stderr) = $node->psql(
	$restore_db,
	qq[SELECT prosrc NOT LIKE 'ivorysql wrapped v1%'
      FROM pg_proc
     WHERE oid = '$dump_schema.plain_fn()'::regprocedure],
	connstr => $restore_connstr, on_error_stop => 1);
is($ret, 0, 'restored ordinary catalog query succeeds');
chomp($stdout);
is($stdout, 't', 'restored ordinary function remains ordinary in pg_proc');

done_testing();
