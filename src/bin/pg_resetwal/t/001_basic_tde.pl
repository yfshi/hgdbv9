
# Copyright (c) 2021, PostgreSQL Global Development Group

use strict;
use warnings;

use PostgresNode;
use TestLib;
use Test::More tests => 11;

program_help_ok('pg_resetwal');
program_version_ok('pg_resetwal');
program_options_handling_ok('pg_resetwal');

my $node = get_new_node('main');
$node->init(allows_streaming => 1,
		extra => [ '-C', 'echo 123|sha256sum|cut -d \' \' -f1', '-z', 'all' ]);

command_like([ 'pg_resetwal', '-n', $node->data_dir, '-C', 'echo 123|sha256sum|cut -d \' \' -f1' ],
	qr/checkpoint/, 'pg_resetwal -n produces output');

