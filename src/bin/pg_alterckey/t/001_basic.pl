
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
		extra => [ '-C', 'echo 123|sha256sum|cut -d \' \' -f1', '-z', 'all' ]);
$node->start;

command_ok([ 'pg_alterckey', 
	'echo 123|sha256sum|cut -d \' \' -f1', 
	'echo 321|sha256sum|cut -d \' \' -f1',
	 $node->data_dir ], 'have changed the cluster key');

$node->stop;
$node->append_conf('postgresql.conf', qq(cluster_key_command = 'echo 321|sha256sum|cut -d \\\' \\\' -f1'));
$node->start;
is($node->safe_psql('postgres', 'SHOW file_encryption_method'),
	'AES', 'Cluster file encryption enabled');

