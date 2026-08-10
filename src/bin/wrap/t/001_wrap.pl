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

plan skip_all => 'wrap tests require GmSSL or OpenSSL'
  unless $gmssl_configured || $openssl_configured;

my $node = get_new_node('wrap_file_execution');
$node->init(extra => ['-m', 'oracle']);
$node->start;

my $workdir = $node->basedir . '/wrap_files';
make_path($workdir);
my $oracle_port = $node->safe_psql('postgres', 'SHOW ivorysql.port');
my $oracle_user = $node->safe_psql('postgres',
	"SELECT rolname FROM pg_roles WHERE rolname = 'system' LIMIT 1");
my $oracle_connstr = "port=$oracle_port host=127.0.0.1".
	" dbname='postgres' user='$oracle_user'";
my ($ret, $stdout, $stderr);

# Initialize the schema through the Oracle listener.
($ret, $stdout, $stderr) = $node->psql(
	'postgres',
	q[
CREATE SCHEMA wrap_file_test;
CREATE TABLE wrap_file_test.proc_log(value number);
], connstr => $oracle_connstr, on_error_stop => 1);
is($ret, 0, 'Oracle test schema is created');

# Function: generate AES output and execute it with psql -f.
my $function_sql = "$workdir/file_function.sql";
my $function_plb = "$workdir/file_function.plb";
append_to_file($function_sql,
	q[CREATE OR REPLACE FUNCTION wrap_file_test.file_function
RETURN NUMBER
IS
BEGIN
  RETURN 101;
END;
]);

# PostgreSQL-style options accept separate values and quoted paths containing
# spaces.  The shell removes the quotes before wrap receives the file name.
my $spaced_sql = "$workdir/file name.sql";
my $spaced_plb = "$workdir/file name.plb";
append_to_file($spaced_sql, slurp_file($function_sql));
$node->command_ok(
	[ 'wrap', '-i', $spaced_sql, '-o', $spaced_plb, '-a', 'AES' ],
	'wrap accepts short PostgreSQL-style options and a path containing spaces');
like(slurp_file($spaced_plb), qr/aes:/,
	'short-option output contains an AES payload');

my $long_plb = "$workdir/file name long.plb";
$node->command_ok(
	[
		'wrap', "--iname=$spaced_sql", "--oname=$long_plb",
		'--algorithm=AES', '--keep_comments=no'
	],
	'wrap accepts long options with equals and paths containing spaces');
like(slurp_file($long_plb), qr/aes:/,
	'long-option output contains an AES payload');

my $legacy_case_plb = "$workdir/legacy_case.plb";
$node->command_ok(
	[
		'wrap', "INAME=$function_sql", "ONAME=$legacy_case_plb",
		'ALGORITHM=AES', 'KEEP_COMMENTS=NO'
	],
	'Oracle-compatible parameter names and values are case insensitive');

$node->command_ok(
	[ 'wrap', "iname=$function_sql", "oname=$function_plb", 'algorithm=aes' ],
	'wrap generates an AES function file');
like(slurp_file($function_plb), qr/\bWRAPPED\nivorysql wrapped v1\n'aes:/,
	'wrapped function file has an aes payload');
$node->command_ok(
	[
		'psql', '-X', '-v', 'ON_ERROR_STOP=1', '--dbname',
		$oracle_connstr, '-f', $function_plb
	],
	'psql -f executes the wrapped function file');
($ret, $stdout, $stderr) = $node->psql(
	'postgres', 'SELECT wrap_file_test.file_function()',
	connstr => $oracle_connstr, on_error_stop => 1);
is($ret, 0, 'function invocation through the Oracle listener succeeds');
chomp($stdout);
is($stdout, '101',
	'function imported with psql -f executes correctly');

($ret, $stdout, $stderr) = $node->psql(
	'postgres',
	q[SELECT prosrc LIKE 'ivorysql wrapped v1%'
  FROM pg_proc
 WHERE oid = 'wrap_file_test.file_function()'::regprocedure],
	connstr => $oracle_connstr, on_error_stop => 1);
is($ret, 0, 'function catalog query through the Oracle listener succeeds');
chomp($stdout);
is($stdout, 't', 'function imported with psql -f stores a wrapped payload');

# Procedure: generate AES output and execute it with an interactive \i command.
my $procedure_sql = "$workdir/file_procedure.sql";
my $procedure_plb = "$workdir/file_procedure.plb";
append_to_file($procedure_sql,
	q[CREATE OR REPLACE PROCEDURE wrap_file_test.file_procedure
IS
BEGIN
  INSERT INTO wrap_file_test.proc_log VALUES (202);
END;
]);
$node->command_ok(
	[ 'wrap', "iname=$procedure_sql", "oname=$procedure_plb", 'algorithm=aes' ],
	'wrap generates an AES procedure file');
like(slurp_file($procedure_plb), qr/\bWRAPPED\nivorysql wrapped v1\n'aes:/,
	'wrapped procedure file has an aes payload');
$node->command_ok(
	[
		'psql', '-X', '-v', 'ON_ERROR_STOP=1', '--dbname',
		$oracle_connstr, '-c', "\\i $procedure_plb"
	],
	'psql session executes the wrapped procedure file with \\i');

($ret, $stdout, $stderr) = $node->psql(
	'postgres', 'CALL wrap_file_test.file_procedure()',
	connstr => $oracle_connstr, on_error_stop => 1);
is($ret, 0, 'procedure invocation through the Oracle listener succeeds');
($ret, $stdout, $stderr) = $node->psql(
	'postgres', 'SELECT value FROM wrap_file_test.proc_log',
	connstr => $oracle_connstr, on_error_stop => 1);
is($ret, 0, 'procedure result query through the Oracle listener succeeds');
chomp($stdout);
is($stdout, '202',
	'procedure imported with \\i executes correctly');

# Package specification: execute a relative \ir include file.
my $package_spec_sql = "$workdir/file_package_spec.sql";
my $package_spec_plb = "$workdir/file_package_spec.plb";
append_to_file($package_spec_sql,
	q[CREATE OR REPLACE PACKAGE wrap_file_test.file_package
IS
  FUNCTION get_value RETURN NUMBER;
END;
]);
$node->command_ok(
	[ 'wrap', "iname=$package_spec_sql", "oname=$package_spec_plb", 'algorithm=aes' ],
	'wrap generates an AES package specification file');
like(slurp_file($package_spec_plb), qr/\bWRAPPED\nivorysql wrapped v1\n'aes:/,
	'wrapped package specification file has an aes payload');

my $include_driver = "$workdir/package_driver.sql";
append_to_file($include_driver, "\\ir file_package_spec.plb\n");
$node->command_ok(
	[
		'psql', '-X', '-v', 'ON_ERROR_STOP=1', '--dbname',
		$oracle_connstr, '-f', $include_driver
	],
	'psql executes the wrapped package specification with \\ir');

($ret, $stdout, $stderr) = $node->psql(
	'postgres',
	q[SELECT pkgsrc LIKE 'ivorysql wrapped v1%'
  FROM pg_package
 WHERE pkgname = 'file_package'],
	connstr => $oracle_connstr, on_error_stop => 1);
is($ret, 0, 'package specification catalog query through the Oracle listener succeeds');
chomp($stdout);
is($stdout, 't', 'package specification imported with \\ir stores a wrapped payload');

# Package body: execute the wrapped file through psql standard input.
my $package_body_sql = "$workdir/file_package_body.sql";
my $package_body_plb = "$workdir/file_package_body.plb";
append_to_file($package_body_sql,
	q[CREATE OR REPLACE PACKAGE BODY wrap_file_test.file_package
IS
  FUNCTION get_value RETURN NUMBER IS
  BEGIN
    RETURN 303;
  END;
END;
]);
$node->command_ok(
	[ 'wrap', "iname=$package_body_sql", "oname=$package_body_plb", 'algorithm=aes' ],
	'wrap generates an AES package body file');
like(slurp_file($package_body_plb), qr/\bWRAPPED\nivorysql wrapped v1\n'aes:/,
	'wrapped package body file has an aes payload');
($ret, $stdout, $stderr) = $node->psql(
	'postgres', slurp_file($package_body_plb), connstr => $oracle_connstr,
	on_error_stop => 1);
is($ret, 0, 'psql standard input executes the wrapped package body file');
($ret, $stdout, $stderr) = $node->psql(
	'postgres', 'SELECT wrap_file_test.file_package.get_value()',
	connstr => $oracle_connstr, on_error_stop => 1);
is($ret, 0, 'package invocation through the Oracle listener succeeds');
chomp($stdout);
is($stdout, '303',
	'package body imported through standard input executes correctly');

($ret, $stdout, $stderr) = $node->psql(
	'postgres',
	q[SELECT b.bodysrc LIKE 'ivorysql wrapped v1%'
  FROM pg_package_body b
  JOIN pg_package p ON p.oid = b.pkgoid
 WHERE p.pkgname = 'file_package'],
	connstr => $oracle_connstr, on_error_stop => 1);
is($ret, 0, 'package body catalog query through the Oracle listener succeeds');
chomp($stdout);
is($stdout, 't', 'package body imported through standard input stores a wrapped payload');

# Multi-subprogram package: wrap and execute two functions and one procedure.
my $multi_package_spec_sql = "$workdir/file_multi_package_spec.sql";
my $multi_package_spec_plb = "$workdir/file_multi_package_spec.plb";
append_to_file($multi_package_spec_sql,
	q[CREATE OR REPLACE PACKAGE wrap_file_test.file_multi_package
IS
  FUNCTION add_value(p_left NUMBER, p_right NUMBER) RETURN NUMBER;
  FUNCTION subtract_value(p_left NUMBER, p_right NUMBER) RETURN NUMBER;
  PROCEDURE write_log(p_value NUMBER);
END;
]);
$node->command_ok(
	[
		'wrap', "iname=$multi_package_spec_sql",
		"oname=$multi_package_spec_plb", 'algorithm=aes'
	],
	'wrap generates a multi-subprogram package specification');
like(slurp_file($multi_package_spec_plb),
	qr/\bWRAPPED\nivorysql wrapped v1\n'aes:/,
	'multi-subprogram package specification has an aes payload');

my $multi_package_body_sql = "$workdir/file_multi_package_body.sql";
my $multi_package_body_plb = "$workdir/file_multi_package_body.plb";
append_to_file($multi_package_body_sql,
	q[CREATE OR REPLACE PACKAGE BODY wrap_file_test.file_multi_package
IS
  FUNCTION add_value(p_left NUMBER, p_right NUMBER) RETURN NUMBER IS
  BEGIN
    RETURN p_left + p_right;
  END;

  FUNCTION subtract_value(p_left NUMBER, p_right NUMBER) RETURN NUMBER IS
  BEGIN
    RETURN p_left - p_right;
  END;

  PROCEDURE write_log(p_value NUMBER) IS
  BEGIN
    INSERT INTO wrap_file_test.proc_log VALUES (p_value);
  END;
END;
]);
$node->command_ok(
	[
		'wrap', "iname=$multi_package_body_sql",
		"oname=$multi_package_body_plb", 'algorithm=aes'
	],
	'wrap generates a multi-subprogram package body');
like(slurp_file($multi_package_body_plb),
	qr/\bWRAPPED\nivorysql wrapped v1\n'aes:/,
	'multi-subprogram package body has an aes payload');
$node->command_ok(
	[
		'psql', '-X', '-v', 'ON_ERROR_STOP=1', '--dbname',
		$oracle_connstr, '-f', $multi_package_spec_plb
	],
	'psql executes the wrapped multi-subprogram package specification');
$node->command_ok(
	[
		'psql', '-X', '-v', 'ON_ERROR_STOP=1', '--dbname',
		$oracle_connstr, '-f', $multi_package_body_plb
	],
	'psql executes the wrapped multi-subprogram package body');

($ret, $stdout, $stderr) = $node->psql(
	'postgres',
	'SELECT wrap_file_test.file_multi_package.add_value(12, 5)',
	connstr => $oracle_connstr, on_error_stop => 1);
is($ret, 0, 'first function in the wrapped package executes');
chomp($stdout);
is($stdout, '17', 'first function in the wrapped package returns the expected value');
($ret, $stdout, $stderr) = $node->psql(
	'postgres',
	'SELECT wrap_file_test.file_multi_package.subtract_value(12, 5)',
	connstr => $oracle_connstr, on_error_stop => 1);
is($ret, 0, 'second function in the wrapped package executes');
chomp($stdout);
is($stdout, '7', 'second function in the wrapped package returns the expected value');
($ret, $stdout, $stderr) = $node->psql(
	'postgres', 'CALL wrap_file_test.file_multi_package.write_log(1003)',
	connstr => $oracle_connstr, on_error_stop => 1);
is($ret, 0, 'procedure in the wrapped package executes');
($ret, $stdout, $stderr) = $node->psql(
	'postgres',
	'SELECT count(*) FROM wrap_file_test.proc_log WHERE value = 1003',
	connstr => $oracle_connstr, on_error_stop => 1);
is($ret, 0, 'wrapped package procedure result query succeeds');
chomp($stdout);
is($stdout, '1', 'procedure in the wrapped package writes the expected row');

# Mixed script: retain ordinary SQL and wrap every supported PL/SQL unit.
my $mixed_sql = "$workdir/mixed_script.sql";
my $mixed_plb = "$workdir/mixed_script.plb";
append_to_file($mixed_sql,
	q[-- This comment contains the PL/SQL keyword IS and must be retained.
CREATE TABLE wrap_file_test.mixed_log(value number);
INSERT INTO wrap_file_test.mixed_log VALUES (801);

CREATE OR REPLACE FUNCTION wrap_file_test.mixed_function
/* This declaration comment must not be retained. */
RETURN NUMBER
IS
  -- This source comment must not be retained.
BEGIN
  RETURN 505;
END;
/

CREATE OR REPLACE FUNCTION wrap_file_test.mixed_special_chars(p_input varchar2)
RETURN varchar2
IS
BEGIN
  -- This single-line comment must not affect the encrypted source.
  /* This block comment must not affect the encrypted source. */
  RETURN '中文单引号'' 双引号" 反斜杠\\ 与符号& 井号# @符号@ 百分号% 星号*'
         || chr(9) || '制表符' || chr(10) || '换行 数字123【括号】 输入值：' || p_input;
END;
/

CREATE OR REPLACE PROCEDURE wrap_file_test.mixed_procedure
IS
BEGIN
  INSERT INTO wrap_file_test.mixed_log VALUES (606);
END;
/

CREATE OR REPLACE PACKAGE wrap_file_test.mixed_package
IS
  FUNCTION get_value RETURN NUMBER;
END;
/

CREATE OR REPLACE PACKAGE BODY wrap_file_test.mixed_package
IS
  FUNCTION get_value RETURN NUMBER IS
  BEGIN
    RETURN 707;
  END;
END;
/

INSERT INTO wrap_file_test.mixed_log VALUES (808);
]);
$node->command_ok(
	[ 'wrap', "iname=$mixed_sql", "oname=$mixed_plb", 'algorithm=aes' ],
	'wrap generates one AES output file from a mixed script');
my $mixed_output = slurp_file($mixed_plb);
like($mixed_output, qr/CREATE TABLE wrap_file_test\.mixed_log\(value number\);/,
	'mixed output retains ordinary table DDL');
like($mixed_output, qr/INSERT INTO wrap_file_test\.mixed_log VALUES \(801\);/,
	'mixed output retains DML before PL/SQL units');
like($mixed_output, qr/INSERT INTO wrap_file_test\.mixed_log VALUES \(808\);/,
	'mixed output retains DML after PL/SQL units');
unlike($mixed_output, qr/This comment contains the PL\/SQL keyword IS/,
	'mixed output omits external comments by default');
unlike($mixed_output, qr/This declaration comment must not be retained/,
	'mixed output omits declaration comments by default');
my $mixed_payload_count = () = $mixed_output =~ /ivorysql wrapped v1/g;
is($mixed_payload_count, 5,
	'mixed output wraps functions, procedure, package specification, and package body');
my $mixed_rewrapped = "$workdir/mixed_script_rewrapped.plb";
$node->command_ok(
	[ 'wrap', "iname=$mixed_plb", "oname=$mixed_rewrapped", 'algorithm=aes' ],
	'wrap accepts an already wrapped mixed script');
is(slurp_file($mixed_rewrapped), $mixed_output,
	'wrapping an already wrapped mixed script preserves every byte');
my $mixed_keep_plb = "$workdir/mixed_script_keep_comments.plb";
$node->command_ok(
	[
		'wrap', "iname=$mixed_sql", "oname=$mixed_keep_plb",
		'keep_comments=yes', 'algorithm=aes'
	],
	'wrap accepts Oracle keep_comments=yes');
my $mixed_keep_output = slurp_file($mixed_keep_plb);
like($mixed_keep_output,
	qr/-- This comment contains the PL\/SQL keyword IS and must be retained\./,
	'keep_comments=yes retains comments outside wrapped units');
unlike($mixed_keep_output, qr/This declaration comment must not be retained/,
	'keep_comments=yes omits declaration comments inside wrapped units');
$node->command_ok(
	[
		'psql', '-X', '-v', 'ON_ERROR_STOP=1', '--dbname',
		$oracle_connstr, '-f', $mixed_plb
	],
	'psql -f executes the mixed wrapped script');
($ret, $stdout, $stderr) = $node->psql(
	'postgres', 'SELECT wrap_file_test.mixed_function()',
	connstr => $oracle_connstr, on_error_stop => 1);
is($ret, 0, 'mixed function invocation succeeds');
chomp($stdout);
is($stdout, '505', 'mixed function returns the expected value');
($ret, $stdout, $stderr) = $node->psql(
	'postgres',
	q[SELECT wrap_file_test.mixed_special_chars('输入42') =
  '中文单引号'' 双引号" 反斜杠\\ 与符号& 井号# @符号@ 百分号% 星号*'
  || chr(9) || '制表符' || chr(10) || '换行 数字123【括号】 输入值：输入42'],
	connstr => $oracle_connstr, on_error_stop => 1);
is($ret, 0, 'mixed special-character function invocation succeeds');
chomp($stdout);
is($stdout, 't', 'mixed special-character function preserves source content');
($ret, $stdout, $stderr) = $node->psql(
	'postgres', 'CALL wrap_file_test.mixed_procedure()',
	connstr => $oracle_connstr, on_error_stop => 1);
is($ret, 0, 'mixed procedure invocation succeeds');
($ret, $stdout, $stderr) = $node->psql(
	'postgres', 'SELECT wrap_file_test.mixed_package.get_value()',
	connstr => $oracle_connstr, on_error_stop => 1);
is($ret, 0, 'mixed package invocation succeeds');
chomp($stdout);
is($stdout, '707', 'mixed package returns the expected value');
($ret, $stdout, $stderr) = $node->psql(
	'postgres', 'SELECT count(*) FROM wrap_file_test.mixed_log',
	connstr => $oracle_connstr, on_error_stop => 1);
is($ret, 0, 'mixed script table query succeeds');
chomp($stdout);
is($stdout, '3', 'mixed script retains DML before and after wrapped units');

# Oracle defaults: extensionless input resolves to .sql and output defaults to .plb.
my $default_name = "$workdir/default_name";
my $default_sql = "$default_name.sql";
my $default_plb = "$default_name.plb";
append_to_file($default_sql,
	q[CREATE OR REPLACE FUNCTION wrap_file_test.default_name_function
RETURN NUMBER
IS
BEGIN
  RETURN 909;
END;
]);
$node->command_ok(
	[ 'wrap', "iname=$default_name", 'algorithm=aes' ],
	'wrap resolves an extensionless input name as .sql and defaults oname to .plb');
ok(-e $default_plb, 'extensionless input creates the default .plb output file');
like(slurp_file($default_plb), qr/aes:/,
	'default .plb output contains an aes wrapped payload');

# Oracle accepts explicitly named input files with arbitrary extensions.
my $text_input = "$workdir/arbitrary_extension.txt";
my $text_output = "$workdir/arbitrary_extension.plb";
append_to_file($text_input,
	q[CREATE OR REPLACE FUNCTION wrap_file_test.arbitrary_extension_function
RETURN NUMBER
IS
BEGIN
  RETURN 910;
END;
]);
$node->command_ok(
	[ 'wrap', "iname=$text_input", 'algorithm=aes' ],
	'wrap accepts an explicitly named .txt input file');
ok(-e $text_output,
	'explicit .txt input defaults to a same-basename .plb output file');
like(slurp_file($text_output), qr/aes:/,
	'.txt input output contains an aes wrapped payload');

done_testing();
