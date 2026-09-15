
# Copyright (c) 2021, PostgreSQL Global Development Group

use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More;

if ($ENV{enable_audit_partition_table} ne 'yes')
{
	plan skip_all => 'audit partition table not supported by this build';
}
else
{
	plan tests => 4;
}

my $node = get_new_node('test');
$node->init;

$node->append_conf('postgresql.conf', "audit.enable = on");
$node->append_conf('postgresql.conf', "audit.systemopers = 'all'");

$node->start;

$node->safe_psql('postgres', "create database db1;");

$node->stop;

$node->start;
my $count_pg_audit_log_num = $node->safe_psql('db1', "select count(relid::regclass) from pg_partition_tree('hg_security.pg_audit_log');");
is($count_pg_audit_log_num, '3', 'count');

$node->safe_psql('postgres', "alter system set audit.interval_time = 8;");
$node->safe_psql('postgres', "alter system set audit.retentionpolicy = 'all';");
$node->stop;

$node->start;
my $interval_time = $node->safe_psql('db1', "show audit.interval_time;");
is($interval_time, '8', 'audit.interval_time');

my $retentionpolicy = $node->safe_psql('db1', "show audit.retentionpolicy;");
is($retentionpolicy, 'all', 'audit.retentionpolicy');

$node->stop;

my $node2 = get_new_node('test2');
$node2->init;

$node2->append_conf('postgresql.conf', "audit.enable = on");
$node2->append_conf('postgresql.conf', "audit.systemopers = 'all'");
$node2->append_conf('postgresql.conf', "audit.interval_time = 8");

$node2->start;
$node2->safe_psql('postgres', "create database db1;");
sleep(2);

my $count_pg_audit_log_num2 = $node2->safe_psql('db1', "select count(relid::regclass) from pg_partition_tree('hg_security.pg_audit_log');");
ok($count_pg_audit_log_num2 == 5 || $count_pg_audit_log_num2 == 6 || $count_pg_audit_log_num2 == 7, 'count');

$node2->stop;

done_testing();
