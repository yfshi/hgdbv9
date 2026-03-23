
# Copyright (c) 2021, PostgreSQL Global Development Group

# Test CREATE INDEX CONCURRENTLY with concurrent modifications
use strict;
use warnings;

use Config;
use PostgresNode;
use TestLib;
use FindBin;
use Test::More tests => 1;

my ($node, $result);
#
# Test set-up
#
$node = get_new_node('integrity_test');
$node->init;
$node->append_conf('postgresql.conf', 'wal_level = replica');
$node->append_conf('postgresql.conf', 'archive_mode = on');

my $path   = $node->archive_dir;

# On Windows, the path specified in the restore command needs to use
# double back-slashes to work properly and to be able to detect properly
# the file targeted by the copy command, so the directory value used
# in this routine, using only one back-slash, need to be properly changed
# first. Paths also need to be double-quoted to prevent failures where
# the path contains spaces.
$path =~ s{\\}{\\\\}g if ($TestLib::windows_os);
my $copy_command =
	$TestLib::windows_os
	? qq{copy %p $path\\\\%f}
	: qq{cp %p $path/%f};

# Enable archive_mode and archive_command on node
$node->append_conf(
	'postgresql.conf', qq(
archive_command = '$copy_command'
));

$node->start;
$node->safe_psql(
	'postgres', q{
	ALTER SYSTEM SET hg_wal_integrity.enable TO ON;
	SELECT pg_reload_conf();
	select pg_sleep(2);
});

$node->safe_psql('postgres', q(create table td(a int)));
$node->safe_psql('postgres', q(insert into td select generate_series(1,1000000)));
$node->safe_psql('postgres', q(checkpoint));
$node->safe_psql('postgres', q(select pg_sleep(2)));
system("cat tmp_check/t_001_integrity_check_integrity_test_data/pgdata/pg_wal/checklog.txt");
is( $node->safe_psql(
		'postgres', q{
		select hg_security.wal_integrity_check()
	}),
	"t",
	"integrity check");

$node->safe_psql('postgres', q(select pg_sleep(2)));
$node->stop;

