
# Copyright (c) 2021, PostgreSQL Global Development Group

# Verify that ALTER TABLE optimizes certain operations as expected

use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More tests => 1;

# Initialize a test cluster
my $node = get_new_node('primary');
$node->init();
# Turn message level up to DEBUG1 so that we get the messages we want to see
$node->append_conf('postgresql.conf', 'client_min_messages = DEBUG1');
$node->start;

# Run a SQL command and return psql's stderr (including debug messages)
sub run_sql_command
{
	my $sql = shift;
	my $stdout;

	$node->psql(
		'postgres',
		$sql,
		stdout        => \$stdout,
		on_error_die  => 1,
		on_error_stop => 1);
	return $stdout;
}

run_sql_command(
	'alter system set nls_sort = binary;
	alter system set idle_session_timeout = 50000000;
	alter system set hg_session_locking_timeout = 4000000;
	alter system set count_idle_busy_time = false;
	alter system set default_collation = \'C\';
	alter system set nls_comp = binary;
	select pg_reload_conf();
	select pg_reload_conf();
	create table test_mdss(id integer);
');


my $output;

$output = run_sql_command('select count(*) from test_mdss');

ok($output =~ m/0/, 'select test_mdss success');

run_sql_command('drop table test_mdss');

$node->stop('fast');
