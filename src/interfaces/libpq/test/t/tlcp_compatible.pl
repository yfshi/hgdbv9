# Copyright (c) 2021, PostgreSQL Global Development Group

# TAP test for the removed ssl_* TLCP connection options in libpq.
#
# When a connection string contains the legacy "sslenccert" or "sslenckey"
# options, libpq's conninfo parser (conninfo_storeval in fe-connect.c) rejects
# them with a specific "remove the connection option ..." message instead of
# the generic "invalid connection option ..." message.  This is a client-side
# parse error, so it is triggered simply by asking psql to connect with such a
# string; no running server is required.

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
	plan tests => 2;
}

my $node = get_new_node('test');

# The parse error is produced client-side by libpq before any connection is
# attempted, so the node does not need to be initialized or started.  We only
# use it to obtain the correct psql binary and environment.
my ($ret, $stdout, $stderr);

($ret, $stdout, $stderr) =
  $node->psql('postgres', '', connstr => 'sslenccert=/tmp/enc_cert.pem');
like($stderr, qr/remove the connection option "sslenccert"/,
	 'sslenccert connection option reports removed');

($ret, $stdout, $stderr) =
  $node->psql('postgres', '', connstr => 'sslenckey=/tmp/enc_key.pem');
like($stderr, qr/remove the connection option "sslenckey"/,
	 'sslenckey connection option reports removed');

done_testing();
