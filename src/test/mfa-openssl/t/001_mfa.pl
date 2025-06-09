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

# RSA publickey
reset_pg_hba($node, 'trust');
$node->safe_psql('postgres', "SELECT pg_set_2fa('user1', 'pkey', 'MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAneWk+P9ORnD44KeYh+c+0DY18NkW6Ahuld7o0NLnEjN/tIFH11QButUqm975lmoOV4+BRbno0whjaXhKpRhgLjW5MEc4UbO6ahU2bHMoiCL6nytpa/TfgjkFku0hBltYLqFjCN/u35P3h+FDG2tZxxciAhQuKr7lx+/6yPxTeUEXaOkorbTJ7eJbdPJ9Wiqpec+4Ju4qDYSnZoObMqxwfEH6D2EyBSMgQt4md4aCD+uhdMWZ253bXflBZ/ftHjuX97n/StykhVDtsG+rDMwojVQwsJ375s6DhB6MVk1Uc7ShgTh2krGx+wuhljNkHzbFjSKdPTBXBciEAfdqMk63NQIDAQAB');");
$ENV{"PGPRIVATEKEY"}="MIIEowIBAAKCAQEAneWk+P9ORnD44KeYh+c+0DY18NkW6Ahuld7o0NLnEjN/tIFH11QButUqm975lmoOV4+BRbno0whjaXhKpRhgLjW5MEc4UbO6ahU2bHMoiCL6nytpa/TfgjkFku0hBltYLqFjCN/u35P3h+FDG2tZxxciAhQuKr7lx+/6yPxTeUEXaOkorbTJ7eJbdPJ9Wiqpec+4Ju4qDYSnZoObMqxwfEH6D2EyBSMgQt4md4aCD+uhdMWZ253bXflBZ/ftHjuX97n/StykhVDtsG+rDMwojVQwsJ375s6DhB6MVk1Uc7ShgTh2krGx+wuhljNkHzbFjSKdPTBXBciEAfdqMk63NQIDAQABAoIBAD+0yg0UpTZSmne55xbePD+xv62/STixPj0FxRWMImL7mEjx6FR+tnv1Sc299yGzBLpbUiMZ0VBkEAgb2rIl1ZiPYHqUIasFQu9OQ7kZ9X6H7mqOD2jZUdyuuQsA1dREwp19zAFmqHBYuQR/5J2JgJ2NPdFs5vukLf2vW5YACIXhFQxMFZfrH2/t77WLrQgY4YCiOX6B5cgxpHiv+aXNNErRY8OSbTvbEM3wwYjE14WpTqc2ja/Id08V7om5b8sOSE4g2mrDFwbF+pkZfLgoHGnVw+UMQhrmdoTHp0eZdLoPxah+nSDyPUx7sqZbxKEdwdqzc/Xe/y/Dpcvztu/1v3kCgYEAzwWuEA82mAWgBDJFfyCVgKiaOdSEBSATNB4TKBnMLVhOAM/lrqa0GaJGaLl6s790VP1+dWmDxCLCLztWhNnVesu1tbgvxgMVHcKvyAfwqL5Piv5qXAk8DrzgWlFVCmitNksESLk5zTUnHvo3A5s1r9Cje/gWPm1b6ac0y0rIJvcCgYEAw0CyBo2j3MvNKJ6jIbNM144Kd6JyxwPy3fh8C3y5NLb2BT5qaF+6ZnIbKfK6xLSQvSKNcPI57rpcXjd5Xnb0jivUzb/arzS2mZXXEPOnqJt4xUT9+jjYK0h9fi6OBWy/+LcGLQpdyi4xMlRHtZupiOdfj8gpZu5oZ8gqq2qHrDMCgYATgYaADt7m/giwvIfdzZUolTpK3qI91WLZ00eczKl3UE2xN+V9SknH7IlTzr4n948ffu8OYw2+SkQfrSiexeOeeVk6sQsXTi0MK87iOxVK4Pufrg8ZculMvjEpcWZg37IKXpEdxQ4ULsDr40mRtmsdkLwGrz5xTV6OqmmIChjHywKBgHPn28Z4HmXbtZX/VeeTgHIVJ7GNBYWOmoWaFEjdaTQZyEGko5YB/a0C340vQh+y8JjWAhCiOjG0/1AOrkWdZ99dtZ3N3gtTx3G0uU6e1VAJus+f/QSk6e4Hwjc9e6FMxUjnnQrvtG1orluqHEd6eFGEx/2aXLl1nRS+hlXrj05xAoGBAIZoDhCQO3+ouQyoVKoEG3JRYnN0/Lt9zpj3Y7R+61bt/N5FMVT6SgkhARTace8ErM3oTqleu+AAehSiX+ryfuaXn4ArT/xFn6+2VftirPE+xJmRegbz8SEqAqDMXmmk5A5yGdMkR20lt1NKzeLBQjAcZgB1GlZdxZK2r6Cy1tXz";

$ENV{"PGPASSWORD"}='pass';

reset_pg_hba($node, 'scram-sha-256,pkey');
test_role(
	$node,
	'user1',
	'scram-sha-256',
	0,
	log_like => [
		qr/connection authenticated: identity="user1" method=scram-sha-256/
	]);

# RSA cert
my $tempdir       = TestLib::tempdir;
my $tempdir_short = TestLib::tempdir_short;

copy("data/test/rsa_cert_root.crt", $node->data_dir . "/rsa_cert_root.crt");
copy("data/test/rsa_client.crt", "${tempdir}/rsa_client.crt");
copy("data/test/rsa_client.key", "${tempdir}/rsa_client.key");

reset_pg_hba($node, 'trust');
$node->safe_psql('postgres', "SELECT pg_set_2fa('user1', 'cert', 'rsa');");

$ENV{"PGCLIENTKEYFILE"}="${tempdir}/rsa_client.key";
$ENV{"PGCLIENTCERTFILE"}="${tempdir}/rsa_client.crt";

$ENV{"PGPASSWORD"}='pass';

# RSA public key
reset_pg_hba($node, 'scram-sha-256,cert');
test_role(
	$node,
	'user1',
	'scram-sha-256',
	0,
	log_like => [
		qr/connection authenticated: identity="user1" method=scram-sha-256/
	]);
