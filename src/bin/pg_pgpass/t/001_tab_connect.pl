use strict;
use warnings;
use PostgresNode;
use TestLib;
use strict;
use warnings;
use File::Copy;
use Config;
use Test::More tests => 8;

my $tempdir       = TestLib::tempdir;
my $tempdir_short = TestLib::tempdir_short;

# Delete pg_hba.conf from the given node, add a new entry to it
# and then execute a reload to refresh it.
sub reset_pg_hba
{
	my $node       = shift;
	my $hba_method = shift;

	unlink($node->data_dir . '/pg_hba.conf');
	# just for testing purposes, use a continuation line
	$node->append_conf('pg_hba.conf', "local all all\\\n $hba_method");
	$node->append_conf('pg_hba.conf', "host all all 127.0.0.1/32\\\n $hba_method");
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
	"SET password_encryption='scram-sha-256'; CREATE user user1 PASSWORD '123456';"
);

my $pgpassfile = "${tempdir}/pgpass";
copy("pgpass/.pgpass", "${pgpassfile}");
chmod 0600, "${pgpassfile}";

delete $ENV{"PGPASSWORD"};
delete $ENV{"PGCHANNELBINDING"};
$ENV{"PGPASSFILE"} = $pgpassfile;
$ENV{"PASSIDENT"} = $node->data_dir;
$ENV{'PASSCUSTOM'} = '1234567';

#$ENV{"PGPASSWORD"}='123456';

#=begin
# plaintext
reset_pg_hba($node, 'md5');
test_role(
	$node,
	'user1',
	'md5',
	0,
	log_like => [
		qr/connection authenticated: identity="user1" method=md5/
	]);
#=cut

#=begin
command_ok(
	[ 'pg_pgpass', '-D', $node->data_dir, '-C', "1234567", "${pgpassfile}" ],
	'encrypt .pgpass');

# cipher
reset_pg_hba($node, 'scram-sha-256');
test_role(
	$node,
	'user1',
	'scram-sha-256',
	0,
	log_like => [
		qr/connection authenticated: identity="user1" method=scram-sha-256/
	]);
#=cut

#=begin
command_ok(
	[ 'pg_pgpass', '-d', '-D', $node->data_dir, '-C', "1234567", "${pgpassfile}" ],
	'encrypt .pgpass');

# cipher
reset_pg_hba($node, 'scram-sha-256');
test_role(
	$node,
	'user1',
	'scram-sha-256',
	0,
	log_like => [
		qr/connection authenticated: identity="user1" method=scram-sha-256/
	]);
#=cut

