
use strict;
use warnings;

use PostgresNode;
use TestLib;
use Test::More tests => 2;

my $node = get_new_node('requirements');
$node->init;
$node->append_conf('postgresql.conf', "archive_mode = on");
$node->append_conf('postgresql.conf', "archive_command = '/bin/true'");
$node->append_conf('postgresql.conf', "wal_level = replica");
$node->start;

$node->safe_psql('postgres', 'CREATE EXTENSION hg_flashback_db;');

is($node->safe_psql('postgres', q(
	SELECT passed::text
	FROM hg_flashback_db.check_system_requirements()
	WHERE check_name = 'archive_mode';)),
	'true',
	'archive_mode=on passes requirement check');

like($node->safe_psql('postgres', q(
	SELECT message
	FROM hg_flashback_db.check_system_requirements()
	WHERE check_name = 'archive_mode';)),
	qr/archive_mode = on/,
	'archive_mode message reports on as valid');
