
use strict;
use warnings;

use PostgresNode;
use TestLib;
use Test::More tests => 4;

my $node = get_new_node('bgw_recovery');
$node->init;
$node->append_conf('postgresql.conf', "wal_level = replica");
$node->append_conf('postgresql.conf', "shared_preload_libraries = 'hg_flashback_db'");
$node->append_conf('postgresql.conf', "hg_flashback_db.db_flashback_enabled = on");
$node->append_conf('postgresql.conf', "hg_flashback_db.db_flashback_bgw_database = 'postgres'");
$node->append_conf('postgresql.conf', "hg_flashback_db.db_flashback_logging_interval = 1");
$node->append_conf('postgresql.conf', "hg_flashback_db.pgbackrest_stanza = 'demo'");
$node->append_conf('postgresql.conf', "hg_flashback_db.pgbackrest_path = '/bin/echo'");
$node->append_conf('postgresql.conf', "hg_flashback_db.pgbackrest_config_path = '/tmp/pgbackrest.conf'");
$node->append_conf('postgresql.conf', "hg_flashback_db.pgbackrest_dry_run = 2");
$node->start;

$node->safe_psql('postgres', 'CREATE EXTENSION hg_flashback_db;');

my $log_offset = -s $node->logfile;
$node->restart;
$node->wait_for_log(qr/invalid input syntax for type json/, $log_offset);
pass('bgw reports JSON parsing error without crashing');

$log_offset = -s $node->logfile;
$node->restart;
$node->wait_for_log(qr/invalid input syntax for type json/, $log_offset);
pass('bgw retries after previous error');

my $lsn = $node->safe_psql('postgres',
	"SELECT hg_flashback_db.create_restore_point('rp_manual_after_bgw_error');");
like($lsn, qr/^[0-9A-F]+\/[0-9A-F]+$/, 'manual create still works after bgw errors');

is($node->safe_psql('postgres', q(
	SELECT count(*)
	FROM hg_flashback_db.restore_points_base
	WHERE name = 'rp_manual_after_bgw_error';)),
	'1',
	'manual metadata insert succeeds after bgw errors');
