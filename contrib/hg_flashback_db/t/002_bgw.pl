
use strict;
use warnings;

use PostgresNode;
use TestLib;
use Test::More tests => 8;

my $node = get_new_node('bgw');
$node->init;
$node->append_conf('postgresql.conf', "wal_level = replica");
$node->append_conf('postgresql.conf', "shared_preload_libraries = 'hg_flashback_db'");
$node->append_conf('postgresql.conf', "hg_flashback_db.db_flashback_enabled = on");
$node->append_conf('postgresql.conf', "hg_flashback_db.db_flashback_bgw_database = 'postgres'");
$node->append_conf('postgresql.conf', "hg_flashback_db.db_flashback_logging_interval = 1");
$node->append_conf('postgresql.conf', "hg_flashback_db.db_flashback_retention_target = 60");
$node->start;

$node->safe_psql('postgres', 'CREATE EXTENSION hg_flashback_db;');

my $log_offset = -s $node->logfile;

$node->append_conf('postgresql.conf', "hg_flashback_db.pgbackrest_stanza = 'demo'");
$node->append_conf('postgresql.conf',
	"hg_flashback_db.pgbackrest_path = '/usr/local/bin/pgbackrest'");
$node->append_conf('postgresql.conf',
	"hg_flashback_db.pgbackrest_config_path = '/tmp/pgbackrest.conf'");
$node->append_conf('postgresql.conf', "hg_flashback_db.pgbackrest_dry_run = 1");

$node->reload;

$node->wait_for_log(
	qr/hg_flashback_db: dry-run backup command: "\/usr\/local\/bin\/pgbackrest" --stanza="demo" --config="\/tmp\/pgbackrest\.conf" backup --type=full/,
	$log_offset);
pass('bgw generated dry-run full backup command');

$node->wait_for_log(
	qr/hg_flashback_db: dry-run info command: "\/usr\/local\/bin\/pgbackrest" --stanza="demo" --config="\/tmp\/pgbackrest\.conf" info --output=json/,
	$log_offset);
pass('bgw generated dry-run info command');

ok($node->poll_query_until('postgres', q(
	SELECT EXISTS (
		SELECT 1
		FROM hg_flashback_db.restore_points_base
		WHERE type = 'auto' AND backup_type = 'full');)),
	'bgw inserted auto restore point metadata');

my $auto_name = $node->safe_psql('postgres', q(
	SELECT name
	FROM hg_flashback_db.restore_points_base
	WHERE type = 'auto'
	ORDER BY rsp_time DESC
	LIMIT 1;));
like($auto_name, qr/^auto_\d{8}_\d{6}_\d{3}$/, 'auto restore point name format');

my $auto_count = $node->safe_psql('postgres', q(
	SELECT count(*)
	FROM hg_flashback_db.restore_points_base
	WHERE type = 'auto';));
ok($auto_count >= 1, 'at least one auto restore point created');

my $expired_lsn = $node->safe_psql('postgres',
	"SELECT hg_flashback_db.create_restore_point('rp_expired_dry_run', 'full');");
like($expired_lsn, qr/^[0-9A-F]+\/[0-9A-F]+$/,
	'create restore point for expired cleanup dry-run');

$node->safe_psql('postgres', q(
	UPDATE hg_flashback_db.restore_points_base
	SET rsp_time = now() - interval '121 minutes'
	WHERE name = 'rp_expired_dry_run';));

$log_offset = -s $node->logfile;

$node->wait_for_log(
	qr/hg_flashback_db: dry-run would drop expired restore point "rp_expired_dry_run"/,
	$log_offset);
pass('bgw dry-run cleanup logged expired restore point');

is($node->safe_psql('postgres', q(
	SELECT count(*)
	FROM hg_flashback_db.restore_points_base
	WHERE name = 'rp_expired_dry_run';)),
	'1',
	'dry-run cleanup does not delete expired restore point metadata');
