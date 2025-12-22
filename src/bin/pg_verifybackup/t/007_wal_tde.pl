
# Copyright (c) 2021, PostgreSQL Global Development Group

# Test pg_verifybackup's WAL verification.

use strict;
use warnings;
use Cwd;
use Config;
use File::Path qw(rmtree);
use PostgresNode;
use TestLib;
use Test::More tests => 5;

# Start up the server and take a backup.
my $primary = get_new_node('primary');
$primary->init(allows_streaming => 1,
			extra => [ '-C', 'echo 181210f8f9c779c26da1d9b2075bde0127302ee0e3fca38c9a83f5b1dd8e5d3b', '-z', 'all' ]);
$primary->start;
my $backup_path = $primary->backup_dir . '/test_wal';
$primary->command_ok([ 'pg_basebackup', '-D', $backup_path, '--no-sync' ],
	"base backup ok");

# Rename pg_wal.
my $original_pg_wal  = $backup_path . '/pg_wal';
my $relocated_pg_wal = $primary->backup_dir . '/relocated_pg_wal';
rename($original_pg_wal, $relocated_pg_wal) || die "rename pg_wal: $!";

# WAL verification should fail.
command_fails_like(
	[ 'pg_verifybackup', $backup_path ],
	qr/WAL parsing failed for timeline 1/,
	'missing pg_wal causes failure');

# Should work if we skip WAL verification.
command_ok(
	[ 'pg_verifybackup', '-n', $backup_path ],
	'missing pg_wal OK if not verifying WAL');

# Should also work if we specify the correct WAL location.
command_ok([ 'pg_verifybackup', '-w', $relocated_pg_wal, '-C', 'echo 181210f8f9c779c26da1d9b2075bde0127302ee0e3fca38c9a83f5b1dd8e5d3b', '-K', 'AES', $backup_path ],
	'-w can be used to specify WAL directory');
