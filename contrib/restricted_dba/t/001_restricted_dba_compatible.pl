# Copyright (c) 2021, PostgreSQL Global Development Group

use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More;

if ($ENV{enable_compatible_ak} ne 'yes')
{
	plan skip_all => 'compatible ak not supported by this build';
}
else
{
	plan tests => 5;
}

my $node = get_new_node('test');
$node->init;

# The legacy hg_accessctrl.enable GUC is registered in _PG_init, which only
# runs when restricted_dba is loaded via shared_preload_libraries.
$node->append_conf('postgresql.conf', "shared_preload_libraries = 'restricted_dba'");

$node->start;

# hg_accessctrl.enable is a PGC_USERSET parameter whose check hook reports that
# the parameter has been renamed. A session SET reaches the hook directly.
my ($ret, $stdout, $stderr);

($ret, $stdout, $stderr) = $node->psql('postgres', "SET hg_accessctrl.enable = on;");
like($stderr, qr/parameter "hg_accessctrl.enable" has been renamed/,
	 'hg_accessctrl.enable reports renamed');

# The legacy ssl_* TLCP parameters are PGC_SIGHUP, so a session SET is blocked
# by the context gate before the hook runs. ALTER SYSTEM validates the value
# directly (PGC_S_FILE) and reaches the check hook, which reports the
# parameter has been removed.
($ret, $stdout, $stderr) = $node->psql('postgres', "ALTER SYSTEM SET ssl_sign_cert_file = '/tmp/cert.pem';");
like($stderr, qr/parameter "ssl_sign_cert_file" has been removed/,
	 'ssl_sign_cert_file reports removed');

($ret, $stdout, $stderr) = $node->psql('postgres', "ALTER SYSTEM SET ssl_sign_key_file = '/tmp/key.pem';");
like($stderr, qr/parameter "ssl_sign_key_file" has been removed/,
	 'ssl_sign_key_file reports removed');

($ret, $stdout, $stderr) = $node->psql('postgres', "ALTER SYSTEM SET ssl_enc_cert_file = '/tmp/cert.pem';");
like($stderr, qr/parameter "ssl_enc_cert_file" has been removed/,
	 'ssl_enc_cert_file reports removed');

($ret, $stdout, $stderr) = $node->psql('postgres', "ALTER SYSTEM SET ssl_enc_key_file = '/tmp/key.pem';");
like($stderr, qr/parameter "ssl_enc_key_file" has been removed/,
	 'ssl_enc_key_file reports removed');

$node->stop;

done_testing();
