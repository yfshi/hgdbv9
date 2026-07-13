# Copyright (c) 2023, PostgreSQL Global Development Group
#
# Test we handle tde fix bug.

use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More;

my $node = get_new_node('test');
$node->init;

$node->start;

#Begin - BUG#Z8555

my $sql_create = q{CREATE TABLE t1 (id int) WITH (encrypt=on)};
my ($ret, $stdout, $stderr);
my $quota_warning_seen = 0;

($ret, $stdout, $stderr) = $node->psql(
                'postgres',
                q{CREATE TABLE t1 (id int) WITH (encrypt=on);},
                eon_error_stop => 0);

my $warning_expected = qr/TDE is disable, encrypt=on is invalid/i;

like($stderr, $warning_expected, 'Got TDE disabled warning');

#End - BUG#Z8555
done_testing();