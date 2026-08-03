use strict;
use warnings FATAL => 'all';

use PostgreSQL::Test::Cluster;
use PostgreSQL::Test::Utils;
use Test::More;

my $admin_db   = 'postgres';
my $login_role = 'graceuser';
my $super_role = 'graceadmin';
my $test_db    = 'postgres';

my $node = PostgreSQL::Test::Cluster->new('grace_period');
$node->init;
unlink($node->data_dir . '/pg_hba.conf');
$node->append_conf(
	'pg_hba.conf', qq{
local all $login_role scram-sha-256
local all $super_role scram-sha-256
local all all trust
});
$node->append_conf(
	'postgresql.conf', q{
shared_preload_libraries = 'credcheck'
});
$node->start;

$node->safe_psql($admin_db,
	qq{ALTER SYSTEM SET credcheck.enable TO on;
	   ALTER SYSTEM SET credcheck.password_grace_time TO 3;
	   ALTER SYSTEM SET credcheck.login_msg_display TO off;
	   SELECT pg_reload_conf();});

$node->safe_psql($admin_db,
	qq{CREATE ROLE $login_role LOGIN PASSWORD 'pw_grace'});
$node->safe_psql($admin_db,
	qq{CREATE ROLE $super_role SUPERUSER LOGIN PASSWORD 'pw_admin'});

my ($ret, $stdout, $stderr);

($ret, $stdout, $stderr) = $node->psql(
	$admin_db,
	q{SET credcheck.password_valid_until = 5;
	  SET credcheck.password_valid_max = 4;},
	on_error_stop => 0);

like($stderr,
	qr/`password_valid_max` must be greater than `password_valid_until`/i,
	'GUC check rejects invalid valid-until bounds');

($ret, $stdout, $stderr) = $node->psql(
	$admin_db,
	q{SET credcheck.password_valid_max = 5;
	  SET credcheck.password_reset_valid_until = 6;},
	on_error_stop => 0);
like($stderr,
	qr/credcheck.password_reset_valid_until must be less than or equal to credcheck.password_valid_max/i,
	'GUC check rejects invalid reset-valid-until upper bound');

$node->safe_psql($admin_db,
	qq{
	DO \$\$ BEGIN
		EXECUTE format('ALTER ROLE $login_role VALID UNTIL %L',
					   current_timestamp - interval '2 days');
	END \$\$;} );

($ret, $stdout, $stderr) = $node->psql(
	$test_db,
	undef,
	connstr => "user=$login_role password=pw_grace dbname=$test_db",
	extra_params => ['-w'],
	on_error_stop => 0);

is($ret, 0, 'grace period allows expired user to log in');
like($stderr,
	qr/password has expired[\s\S]*Grace period remains active/i,
	'psql prints grace notice');

$node->connect_ok(
	"user=$login_role password=pw_grace dbname=$test_db",
	'grace notice function returns message for grace login',
	sql => q{SELECT hg_security.pg_password_grace_notice()},
	expected_stdout =>
	  qr/password has expired\..*Grace period remains active until .*Change your password immediately\./i);

$node->safe_psql($admin_db,
	qq{SET credcheck.password_valid_until = 0;
	DO \$\$ BEGIN
		EXECUTE format('ALTER ROLE $login_role VALID UNTIL %L',
					   current_timestamp - interval '4 days');
	END \$\$;} );

$node->connect_fails(
	"user=$login_role password=pw_grace dbname=$test_db",
	'login fails after grace period expires',
	expected_stderr =>
	  qr/password authentication failed for user "$login_role"/i,
	log_like => [qr/User "$login_role" has an expired password\./i]);

$node->safe_psql($admin_db,
	qq{ALTER ROLE $login_role PASSWORD 'pw_grace_new';} );

is(
	$node->safe_psql(
		$admin_db,
		qq{SELECT (rolvaliduntil > now())::int
		   FROM pg_authid
		   WHERE rolname = '$login_role';}),
	'1',
	'password reset without explicit VALID UNTIL refreshes rolvaliduntil');

$node->connect_ok(
	"user=$login_role password=pw_grace_new dbname=$test_db",
	'password reset restores normal login');

$node->safe_psql($admin_db,
	qq{ALTER SYSTEM SET credcheck.password_expiration TO 2;
	   ALTER SYSTEM SET credcheck.password_reset_valid_until TO 1;
	   SELECT pg_reload_conf();});

$node->safe_psql($admin_db,
	"ALTER ROLE $login_role PASSWORD 'pw_grace_new';",
	{ role => $login_role} );

($ret, $stdout, $stderr) = $node->psql(
	$test_db,
	undef,
	connstr => "user=$login_role password=pw_grace_new dbname=$test_db",
	extra_params => ['-w'],
	on_error_stop => 0);

like($stderr,
	qr/Your account will expire in:/i,
	'expiry notice is shown');


$node->safe_psql($admin_db,
	qq{SET credcheck.password_valid_until = 0;
	DO \$\$ BEGIN
		EXECUTE format('ALTER ROLE $super_role VALID UNTIL %L',
					   current_timestamp - interval '5 days');
	END \$\$;} );

($ret, $stdout, $stderr) = $node->psql(
	$test_db,
	undef,
	connstr => "user=$super_role password=pw_admin dbname=$test_db",
	extra_params => ['-w'],
	on_error_stop => 0);
is($ret, 0, 'expired superuser can still log in after grace period');
like($stderr,
	qr/NOTICE:\s+password has expired\. Change your password immediately\./i,
	'superuser login prints restricted notice');

($ret, $stdout, $stderr) = $node->psql(
	$test_db,
	q{SELECT 1;},
	connstr => "user=$super_role password=pw_admin dbname=$test_db",
	extra_params => ['-w'],
	on_error_stop => 0);
like($stderr,
	qr/you must change your password first\./i,
	'restricted superuser is forced to rotate password');

($ret, $stdout, $stderr) = $node->psql(
	$test_db,
	qq{ALTER ROLE $super_role PASSWORD 'pw_admin_new';},
	connstr => "user=$super_role password=pw_admin dbname=$test_db",
	extra_params => ['-w'],
	on_error_stop => 0);
is($ret, 0, 'restricted superuser can change own password');

$node->connect_ok(
	"user=$super_role password=pw_admin_new dbname=$test_db",
	'superuser regains normal login after password rotation',
	sql => q{SELECT 1},
	expected_stdout => qr/1/);

$node->stop;

done_testing();