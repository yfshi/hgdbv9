use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More;
if (!$use_unix_sockets)
{
	plan skip_all =>
	  "authentication tests cannot run without Unix-domain sockets";
}
else
{
if ($ENV{'RUN_RESET_TIME_MIN'}) {
	plan tests => 10;
} else {
    plan tests => 5;
}
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

sub test_role {
    my ($node, $role, $testdesc, $expected_res, %opts) = @_;
    my $connstr = "user=$role";
    if ($expected_res == 0)
    {
        $node->connect_ok($connstr, $testdesc, %opts);
    }
    else
    {
        $node->connect_fails($connstr, $testdesc, %opts);
    }
}

my $node = get_new_node('lockout_test');
$node->init;

$node->append_conf('postgresql.conf', "log_connections = on\n");
$node->append_conf('postgresql.conf', "credcheck.enable = on\n");
$node->append_conf('postgresql.conf', "credcheck.max_auth_failure = 3\n");
$node->append_conf('postgresql.conf', "credcheck.reset_superuser = off\n");
$node->append_conf('postgresql.conf', "credcheck.reset_time_min = '1min'\n");
$node->start;
reset_pg_hba($node, 'trust');

$node->safe_psql('postgres', "DROP USER IF EXISTS lockuser");
$node->safe_psql('postgres', "CREATE USER lockuser LOGIN PASSWORD 'correctpass'");

reset_pg_hba($node, 'md5');

$ENV{"PGPASSWORD"} = 'badpass';
for my $i (1 .. 3) {
    test_role($node, 'lockuser', "Login attempt with incorrect password $i", 2);
}

$ENV{"PGPASSWORD"} = 'correctpass';
test_role($node, 'lockuser', "Login attempt with correct password, expected failure", 2);

reset_pg_hba($node, 'trust');

$node->safe_psql('postgres', "SELECT hg_security.pg_banned_role_reset('lockuser')");

reset_pg_hba($node, 'md5');
test_role($node, 'lockuser', "Login attempt with correct password, expected success", 0);

if ($ENV{'RUN_RESET_TIME_MIN'}) {

$ENV{"PGPASSWORD"} = 'badpass';
    for my $i (1 .. 3) {
        test_role($node, 'lockuser', "Login attempt with incorrect password $i", 2);
    }

    $ENV{"PGPASSWORD"} = 'correctpass';
    test_role($node, 'lockuser', "Login attempt with correct password, expected failure", 2);

    sleep(61);
    test_role($node, 'lockuser', "Login attempt with correct password, expected success", 0);
}

reset_pg_hba($node, 'trust');
$node->safe_psql('postgres', "DROP USER lockuser");
