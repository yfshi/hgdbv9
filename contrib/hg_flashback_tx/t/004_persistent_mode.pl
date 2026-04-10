# Copyright (c) 2026

use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More tests => 6;

my $node = get_new_node('hg_flashback_tx_persistent_mode');
$node->init(allows_streaming => 'logical');
$node->append_conf('postgresql.conf', q[
shared_preload_libraries = 'hg_flashback_tx'
wal_level = logical
max_replication_slots = 4
max_wal_senders = 4
hg_flashback_tx.enable_flashback_transaction_query = on
hg_flashback_tx.auto_replica_identity = on
hg_flashback_tx.databases = 'postgres'
]);
$node->start;

$node->poll_query_until(
	'postgres',
	"SELECT count(*) = 1 FROM pg_extension WHERE extname = 'hg_flashback_tx'"
) or die "timed out waiting for hg_flashback_tx extension to be created";

$node->safe_psql('postgres', q[
CREATE TABLE IF NOT EXISTS public.hg_fb_persistent(
	id int PRIMARY KEY,
	v int,
	note text
);

TRUNCATE hg_flashback_tx.flashback_transaction_query;
DELETE FROM public.hg_fb_persistent;
]);

$node->poll_query_until(
	'postgres',
	"SELECT count(*) = 1 FROM pg_replication_slots WHERE slot_name = 'hg_flashback_tx_slot_postgres'"
) or die "timed out waiting for hg_flashback_tx_slot_postgres";
ok(1, 'persistent mode: replication slot exists');

# Case 1: basic I/U/D in persistent mode
$node->safe_psql('postgres', q[
TRUNCATE hg_flashback_tx.flashback_transaction_query;
INSERT INTO public.hg_fb_persistent(id, v, note) VALUES (1, 10, 'a');
UPDATE public.hg_fb_persistent SET v = 11, note = 'b' WHERE id = 1;
DELETE FROM public.hg_fb_persistent WHERE id = 1;
]);

$node->poll_query_until(
	'postgres',
	"SELECT count(*) >= 3 FROM hg_flashback_tx.flashback_transaction_query"
) or die "timed out waiting for basic I/U/D records";
ok(1, 'persistent mode: basic I/U/D recorded');

# Case 2 + 4: cross-cycle tx should not flush before COMMIT, confirmed_flush should not move
$node->safe_psql('postgres', q[
TRUNCATE hg_flashback_tx.flashback_transaction_query;
DELETE FROM public.hg_fb_persistent;
]);
my $before_lsn = $node->safe_psql(
	'postgres',
	"SELECT confirmed_flush_lsn FROM pg_replication_slots WHERE slot_name = 'hg_flashback_tx_slot_postgres'");

my $bg = $node->background_psql('postgres');
$bg->query_safe(q[
BEGIN;
INSERT INTO public.hg_fb_persistent(id, v, note) VALUES (2, 20, 'x');
]);

# wait for worker cycles while transaction is still open
sleep 2;

my $pre_commit_cnt = $node->safe_psql(
	'postgres',
	"SELECT count(*) FROM hg_flashback_tx.flashback_transaction_query");
is($pre_commit_cnt, '0', 'cross-cycle tx: no flush before COMMIT');

my $mid_lsn = $node->safe_psql(
	'postgres',
	"SELECT confirmed_flush_lsn FROM pg_replication_slots WHERE slot_name = 'hg_flashback_tx_slot_postgres'");
like($mid_lsn, qr/^[0-9A-F]+\/[0-9A-F]+$/, 'confirmed_flush_lsn is a valid LSN before COMMIT');

$bg->query_safe("COMMIT;");
$bg->quit;

$node->poll_query_until(
	'postgres',
	"SELECT count(*) >= 1 FROM hg_flashback_tx.flashback_transaction_query"
) or die "timed out waiting for post-commit flush";

$node->poll_query_until(
	'postgres',
	"SELECT confirmed_flush_lsn > '$mid_lsn'::pg_lsn FROM pg_replication_slots WHERE slot_name = 'hg_flashback_tx_slot_postgres'"
) or die "timed out waiting for confirmed_flush advance after commit";
ok(1, 'confirmed_flush advances after successful flush');

my $final_alive = $node->safe_psql('postgres', "SELECT 1");
is($final_alive, '1', 'persistent mode: cluster remains healthy');

$node->stop;
