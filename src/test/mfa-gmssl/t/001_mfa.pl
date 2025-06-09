use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More;
use File::Copy;

if (!$use_unix_sockets) {
    plan skip_all => "mfa tests cannot run without Unix-domain sockets";
} else {
    plan tests => 4;
}
# Delete pg_hba.conf from the given node, add a new entry to it
# and then execute a reload to refresh it.
sub reset_pg_hba
{
	my $node       = shift;
	my $hba_method = shift;

	unlink($node->data_dir . '/pg_hba.conf');
	# just for testing purposes, use a continuation line
	$node->append_conf('pg_hba.conf', "local all all\\\n $hba_method");
	$node->reload;
	return;
}

# Test access for a single role, useful to wrap all tests into one.  Extra
# named parameters are passed to connect_ok/fails as-is.
sub test_role
{
	local $Test::Builder::Level = $Test::Builder::Level + 1;

	my ($node, $role, $method, $expected_res, %params) = @_;
	my $status_string = 'failed';
	$status_string = 'success' if ($expected_res eq 0);

	my $connstr = "user=$role";
	my $testname =
	  "authentication $status_string for method $method, role $role";

	if ($expected_res eq 0)
	{
		$node->connect_ok($connstr, $testname, %params);
	}
	else
	{
		# No checks of the error message, only the status code.
		$node->connect_fails($connstr, $testname, %params);
	}
}

# Initialize primary node
my $node = get_new_node('primary');
$node->init;
$node->append_conf('postgresql.conf', "log_connections = on\n");
$node->start;

$node->safe_psql('postgres',
	"SET password_encryption='scram-sha-256'; create user user1 PASSWORD 'pass';"
);

# SM2
reset_pg_hba($node, 'trust');
$node->safe_psql('postgres', "SELECT pg_set_2fa('user1', 'pkey', 'MFkwEwYHKoZIzj0CAQYIKoEcz1UBgi0DQgAEEHat/veWqIZln3259Os6YUVOrUhrAeaHMeyumAU5C0VwCbQHx5gE1XMILtelWptMbulgF/8oc+o9GXyI9KUtog==');");

$ENV{"PGPRIVATEKEY"}="MIIBBjBhBgkqhkiG9w0BBQ0wVDA0BgkqhkiG9w0BBQwwJwQQK75plr6RZP9puif2k7SAwwIDAQAAAgEQMAsGCSqBHM9VAYMRAjAcBggqgRzPVQFoAgQQsXtoqCpH4sQX9B9hNmSOZASBoNfvXiL0ZZGeKJhb9Kl3vibzUGFl9IFFLDn4JAfMTsbXBsZHDATaYONBo4FOpyDeOb00LHRgpQGcKgm0PmpVdWo7i3GmpRUHNC2yE135PdcuyghdO0qoy5lk0ifmNnY+RhDvpDP9p6DcF3XXggUAM7tV+nmjjorzlR3Kh7uV/MP2H0rVAgJkZ/WiWEQNCVxFXVVM7IiRjiEojH0loqEyQcA=";

$ENV{"PGPASSWORD"}='pass';

# SM2 public key
reset_pg_hba($node, 'scram-sha-256,pkey');
test_role(
	$node,
	'user1',
	'scram-sha-256',
	0,
	log_like => [
		qr/connection authenticated: identity="user1" method=scram-sha-256/
	]);

# SM2 cert
my $tempdir       = TestLib::tempdir;
my $tempdir_short = TestLib::tempdir_short;

copy("data/test/sm2_cert_root.crt", $node->data_dir . "/sm2_cert_root.crt");
copy("data/test/sm2_client.crt", "${tempdir}/sm2_client.crt");
copy("data/test/sm2_client.key", "${tempdir}/sm2_client.key");

reset_pg_hba($node, 'trust');
$node->safe_psql('postgres', "SELECT pg_set_2fa('user1', 'cert', 'sm2');");

$ENV{"PGCLIENTKEYFILE"}="${tempdir}/sm2_client.key";
$ENV{"PGCLIENTCERTFILE"}="${tempdir}/sm2_client.crt";

$ENV{"PGPASSWORD"}='pass';

# SM2 public key
reset_pg_hba($node, 'scram-sha-256,cert');
test_role(
	$node,
	'user1',
	'scram-sha-256',
	0,
	log_like => [
		qr/connection authenticated: identity="user1" method=scram-sha-256/
	]);
