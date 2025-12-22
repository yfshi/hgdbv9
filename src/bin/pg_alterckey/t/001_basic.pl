
# Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.

use strict;
use warnings;

use PostgresNode;
use TestLib;
use Test::More tests => 10;

program_help_ok('pg_alterckey');
program_version_ok('pg_alterckey');
program_options_handling_ok('pg_alterckey');

my $node = get_new_node('main');
$node->init(allows_streaming => 1,
		extra => [ '-C', 'echo 181210f8f9c779c26da1d9b2075bde0127302ee0e3fca38c9a83f5b1dd8e5d3b', '-z', 'all' ]);
$node->start;

command_ok([ 'pg_alterckey', 
	'echo 181210f8f9c779c26da1d9b2075bde0127302ee0e3fca38c9a83f5b1dd8e5d3b', 
	'echo ff6f81930943c96a37d7741cd547ad90295a9bd63b6194b2a834a1d32bc8f85d',
	 $node->data_dir ], 'have changed the cluster key');

$node->stop;
$node->append_conf('postgresql.conf', qq(cluster_key_command = 'echo ff6f81930943c96a37d7741cd547ad90295a9bd63b6194b2a834a1d32bc8f85d'));
$node->start;
is($node->safe_psql('postgres', 'SHOW file_encryption_method'),
	'AES', 'Cluster file encryption enabled');

