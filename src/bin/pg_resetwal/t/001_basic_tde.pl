
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
		extra => [ '-C', 'echo 181210f8f9c779c26da1d9b2075bde0127302ee0e3fca38c9a83f5b1dd8e5d3b', '-z', 'all' ]);

command_like([ 'pg_resetwal', '-n', '-C', 'echo 181210f8f9c779c26da1d9b2075bde0127302ee0e3fca38c9a83f5b1dd8e5d3b', $node->data_dir ],
	qr/checkpoint/, 'pg_resetwal -n produces output');

