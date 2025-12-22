
# Copyright (c) 2021, PostgreSQL Global Development Group

use strict;
use warnings;
use Cwd;
use Config;
use File::Basename qw(basename dirname);
use File::Path qw(rmtree);
use PostgresNode;
use TestLib;
use Test::More tests => 11;

program_help_ok('pg_basebackup');
program_version_ok('pg_basebackup');
program_options_handling_ok('pg_basebackup');

my $tempdir = TestLib::tempdir;

my $node = get_new_node('main');

# Set umask so test directories and files are created with default permissions
umask(0077);

# Initialize node without replication settings
$node->init(allows_streaming => 1,
		extra => [ '--data-checksums', '-C', 'echo 181210f8f9c779c26da1d9b2075bde0127302ee0e3fca38c9a83f5b1dd8e5d3b', '-z', 'all' ]);
$node->start;
my $pgdata = $node->data_dir;

# Run base backup.
$node->command_ok([ 'pg_basebackup', '-D', "$tempdir/backup1", '-X', 'none' ],
	'pg_basebackup runs');
# File encryption keys should be copied.
is_deeply(
	[ sort(slurp_dir("$tempdir/backup1/pg_cryptokeys")) ],
	[ sort qw(. .. live live_bak1 live_bak2 live_bak3) ],
	'pg_cryptokeys not copied');

# Run base backup and decrypt.
$node->backup('backup2', backup_options => ['-y']);
my $node2 = get_new_node('node2');
$node2->init_from_backup($node, 'backup2');
$node2->start;
is($node2->safe_psql('postgres', 'SHOW file_encryption_method'),
	'', 'Cluster file encryption disabled');
$node2->stop;