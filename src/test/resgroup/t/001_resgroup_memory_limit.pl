# Copyright (c) 2021, PostgreSQL Global Development Group
#
# Testing of resgroup memory limit.
#

use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More;

sub is_centos7
{
    open my $fh, '<', '/etc/os-release' or return 0;
    my ($id, $version);

    while (<$fh>)
    {
        chomp;
        $id      = $1 if /^ID=(.+)/;
        $version = $1 if /^VERSION_ID=(.+)/;
    }
    close $fh;

    $id      =~ s/"//g if defined $id;
    $version =~ s/"//g if defined $version;

    return ($id && $id eq 'centos' && $version =~ /^7/);
}

unless (is_centos7())
{
    plan skip_all => 'This test case is only executed on CentOS 7';
}
else
{
    plan tests => 4;
}

# Initialize primary node
my $node = get_new_node('primary');
$node->init(allows_streaming => 'logical');
$node->append_conf('postgresql.conf', 'pg_resource_manager = group');
$node->start;

# Check whether the resource group is enabled.
my ($stdout) = $node->safe_psql('postgres',
		qq[SHOW pg_resource_manager;]);
is($stdout, 'group', 'Verify that pg_resource_manager is group');

# Create resource group
my ($result, $stderr) = $node->safe_psql('postgres',
		qq[CREATE RESOURCE GROUP gp_test with (CONCURRENCY=10, MEMORY_LIMIT=50, CPU_MAX_PERCENT=10, IO_LIMIT=10);]);
is($result, '', 'Create resource group should succeed');

# Create test user
$node->safe_psql('postgres',
		qq[CREATE USER gp_user with password '123456';]);

# Add the user to the resource group
$result = $node->safe_psql('postgres',
		qq[ALTER ROLE gp_user RESOURCE GROUP gp_test;]);
is($result, '', 'Alter resource group to user');

$node->safe_psql(
        'postgres', 'create table tb(id int, name text);');

# SQL executed by the user errors out when the memory limit is exceeded
($result, $stderr, $stdout) = $node->psql(
        'postgres', "insert into tb values (1, repeat('a', 534288001));",
        extra_params => [ '-U', 'gp_user' ]);
ok($stdout =~ /SQL resource limit-maxmum memory allocation is/,
        "The encrypt property of  of a tablespace cannot be altered.");

# Done with the node
$node->stop;
