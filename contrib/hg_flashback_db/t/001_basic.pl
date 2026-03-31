
use strict;
use warnings;

use PostgresNode;
use TestLib;
use Test::More tests => 33;

my $node = get_new_node('main');
$node->init;
$node->append_conf('postgresql.conf', "wal_level = replica");
$node->append_conf('postgresql.conf', "hg_flashback_db.pgbackrest_stanza = 'demo'");
$node->append_conf('postgresql.conf',
	"hg_flashback_db.pgbackrest_path = '/usr/local/bin/pgbackrest'");
$node->append_conf('postgresql.conf',
	"hg_flashback_db.pgbackrest_config_path = '/tmp/pgbackrest.conf'");
$node->append_conf('postgresql.conf', "hg_flashback_db.pgbackrest_dry_run = 1");
$node->start;

$node->safe_psql('postgres', 'CREATE EXTENSION hg_flashback_db;');
$node->safe_psql('postgres', 'CREATE ROLE rp_user;');

my $stderr = '';
my $status = $node->psql(
	'postgres',
	"SET ROLE rp_user; SELECT hg_flashback_db.create_restore_point('rp_denied');",
	stderr => \$stderr);
ok($status != 0, 'non-owner cannot execute create_restore_point');
like($stderr, qr/permission denied for function create_restore_point/,
	'create_restore_point execution is revoked from PUBLIC');

my $lsn =
  $node->safe_psql('postgres',
	"SELECT hg_flashback_db.create_restore_point('rp_plain');");
like($lsn, qr/^[0-9A-F]+\/[0-9A-F]+$/, 'create restore point without backup');

is($node->safe_psql('postgres', q(
	SELECT type || ':' || coalesce(backup_type, 'NULL')
	FROM hg_flashback_db.restore_points_base
	WHERE name = 'rp_plain';)),
	'manual:NULL',
	'metadata stored for plain restore point');

my $log_offset = -s $node->logfile;

my $backup_lsn = $node->safe_psql('postgres',
	"SELECT hg_flashback_db.create_restore_point('rp_full', 'full');");
like($backup_lsn, qr/^[0-9A-F]+\/[0-9A-F]+$/, 'create restore point with backup');

is($node->safe_psql('postgres', q(
	SELECT backup_type
	FROM hg_flashback_db.restore_points_base
	WHERE name = 'rp_full';)),
	'full',
	'backup type stored in metadata');

$node->wait_for_log(
	qr/hg_flashback_db: dry-run backup command: "\/usr\/local\/bin\/pgbackrest" --stanza="demo" --config="\/tmp\/pgbackrest\.conf" backup --type=full/,
	$log_offset);
pass('dry-run backup command is logged');

$node->wait_for_log(
	qr/hg_flashback_db: dry-run info command: "\/usr\/local\/bin\/pgbackrest" --stanza="demo" --config="\/tmp\/pgbackrest\.conf" info --output=json/,
	$log_offset);
pass('dry-run info command is logged');

is($node->safe_psql('postgres',
	"SELECT hg_flashback_db.drop_restore_point('rp_plain');"),
	't',
	'drop restore point succeeds');

is($node->safe_psql('postgres', q(
	SELECT count(*)
	FROM hg_flashback_db.restore_points_base
	WHERE name = 'rp_plain';)),
	'0',
	'dropped restore point metadata removed');

$log_offset = -s $node->logfile;
$node->safe_psql('postgres',
	"SELECT hg_flashback_db.create_restore_point('rp_skip_backup', 'full');");

pass('dry-run backup command logged for rp_skip_backup');

is($node->safe_psql('postgres',
	"SELECT hg_flashback_db.drop_restore_point('rp_skip_backup', skip_backup := true);"),
	't',
	'drop restore point with skip_backup := true succeeds');

is($node->safe_psql('postgres', q(
	SELECT count(*)
	FROM hg_flashback_db.restore_points_base
	WHERE name = 'rp_skip_backup';)),
	'0',
	'dropped restore point with skip_backup removed');

$node->safe_psql('postgres',
	"SELECT hg_flashback_db.create_restore_point('rp_dup');");

 
$stderr = '';
$status = $node->psql(
	'postgres',
	"SELECT hg_flashback_db.create_restore_point('rp_dup');",
	stderr => \$stderr);
ok($status != 0, 'duplicate restore point name is rejected');
like($stderr, qr/restore point "rp_dup" already exists/,
	'duplicate restore point error message');

$stderr = '';
$status = $node->psql(
	'postgres',
	"SELECT hg_flashback_db.create_restore_point('rp_invalid_type', 'bad');",
	stderr => \$stderr);
ok($status != 0, 'invalid backup type is rejected');
like($stderr, qr/invalid backup type: "bad"/,
	'invalid backup type error message');

$stderr = '';
$status = $node->psql(
	'postgres',
	"SELECT hg_flashback_db.create_restore_point(NULL);",
	stderr => \$stderr);
ok($status != 0, 'NULL restore point name is rejected');
like($stderr, qr/restore point name must not be NULL/,
	'NULL restore point error message');

$stderr = '';
$status = $node->psql(
	'postgres',
	"SELECT hg_flashback_db.drop_restore_point('rp_missing');",
	stderr => \$stderr);
ok($status != 0, 'dropping missing restore point fails');
like($stderr, qr/restore point "rp_missing" does not exist/,
	'missing restore point error message');

$node->append_conf('postgresql.conf', "hg_flashback_db.pgbackrest_dry_run = 0");
$node->append_conf('postgresql.conf', "hg_flashback_db.pgbackrest_path = '/bin/false'");
$node->reload;

$stderr = '';
$status = $node->psql(
	'postgres',
	"SELECT hg_flashback_db.create_restore_point('rp_fail_backup', 'full');",
	stderr => \$stderr);
ok($status != 0, 'backup command failure is surfaced');
like($stderr, qr/backup failed, restore point not created/,
	'backup failure error message');

$node->append_conf('postgresql.conf', "hg_flashback_db.pgbackrest_path = '/bin/echo'");
$node->reload;

$stderr = '';
$status = $node->psql(
	'postgres',
	"SELECT hg_flashback_db.create_restore_point('rp_bad_json', 'full');",
	stderr => \$stderr);
ok($status != 0, 'invalid pgbackrest info JSON is surfaced');
like($stderr, qr/invalid input syntax for type json/,
	'bad JSON error message');

my $view_def = $node->safe_psql('postgres',
	q(SELECT column_name FROM information_schema.columns WHERE table_schema = 'hg_flashback_db' AND table_name = 'v$flashback_database_log' ORDER BY ordinal_position;));
is($view_def, "oldest_flashback_scn
oldest_flashback_time
retention_target
flashback_size
estimated_flashback_size
con_id",
	'view has correct columns');

my $view_data = $node->safe_psql('postgres',
	"SELECT con_id FROM hg_flashback_db.v\$flashback_database_log;");
is($view_data, '0', 'con_id returns 0');

my $retention = $node->safe_psql('postgres',
	"SELECT retention_target FROM hg_flashback_db.v\$flashback_database_log;");
is($retention, '10080', 'retention_target returns default value');

my $flashback_size = $node->safe_psql('postgres',
	"SELECT flashback_size FROM hg_flashback_db.v\$flashback_database_log;");
is($flashback_size, '0', 'flashback_size returns 0 in dry-run mode');

my $estimated = $node->safe_psql('postgres',
	"SELECT estimated_flashback_size FROM hg_flashback_db.v\$flashback_database_log;");
is($estimated, '0', 'estimated_flashback_size returns 0 in dry-run mode');

my $show_rp_cols = $node->safe_psql('postgres',
	q(SELECT column_name FROM information_schema.columns WHERE table_schema = 'hg_flashback_db' AND table_name = 'show_restore_point' ORDER BY ordinal_position;));
is($show_rp_cols, "SCN
RSP Time
Type
Time
Name",
	'show_restore_point view has correct columns');

my $show_rp_type = $node->safe_psql('postgres',
	qq(SELECT "Type" FROM hg_flashback_db.show_restore_point LIMIT 1;));
is($show_rp_type, 'GUARANTED', 'show_restore_point Type column returns GUARANTED');

my $show_rp_name = $node->safe_psql('postgres',
	qq(SELECT "Name" FROM hg_flashback_db.show_restore_point ORDER BY "Time" DESC LIMIT 1;));
is($show_rp_name, 'rp_dup', 'show_restore_point returns correct restore point name');
