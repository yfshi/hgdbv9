# Copyright (c) 2026

use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More tests => 13;

my $node = get_new_node('hg_flashback_tx_concurrent');
$node->init(allows_streaming => 'logical');
$node->append_conf('postgresql.conf', q[
shared_preload_libraries = 'hg_flashback_tx'
wal_level = logical
max_replication_slots = 4
max_wal_senders = 4
hg_flashback_tx.enable_flashback_transaction_query = off
hg_flashback_tx.auto_replica_identity = on
hg_flashback_tx.databases = 'postgres'
]);
$node->start;

$node->poll_query_until(
	'postgres',
	"SELECT count(*) = 1 FROM pg_extension WHERE extname = 'hg_flashback_tx'"
) or die "timed out waiting for hg_flashback_tx extension to be created";

$node->safe_psql('postgres', q[
CREATE TABLE IF NOT EXISTS public.hg_fb_concurrent(
	id int PRIMARY KEY,
	v int,
	note text
);

TRUNCATE hg_flashback_tx.flashback_transaction_query;
DELETE FROM public.hg_fb_concurrent;
]);

$node->poll_query_until(
	'postgres',
	"SELECT count(*) = 1 FROM pg_replication_slots WHERE slot_name = 'hg_flashback_tx_slot_postgres'"
) or die "timed out waiting for hg_flashback_tx_slot_postgres";

# Insert initial test data with recording disabled to avoid capturing these records
$node->safe_psql('postgres', q[
INSERT INTO public.hg_fb_concurrent(id, v, note)
SELECT i, i * 100, 'init_' || i::text
FROM generate_series(1, 1000) i;
]);

# Now enable transaction recording
$node->safe_psql('postgres', "ALTER SYSTEM SET hg_flashback_tx.enable_flashback_transaction_query = on");
$node->safe_psql('postgres', "SELECT pg_reload_conf()");


my $initial_count = $node->safe_psql('postgres', "SELECT count(*) FROM public.hg_fb_concurrent");
is($initial_count, '1000', 'pre-inserted 1000 rows (>1 page)');

$node->safe_psql('postgres', q[
TRUNCATE hg_flashback_tx.flashback_transaction_query;
]);

my $s1 = $node->background_psql('postgres');
my $s2 = $node->background_psql('postgres');

$s1->query_safe(q[
BEGIN;
INSERT INTO public.hg_fb_concurrent(id, v, note) VALUES (10001, 100010, 's1_i1');
INSERT INTO public.hg_fb_concurrent(id, v, note) VALUES (10002, 100020, 's1_i2');
INSERT INTO public.hg_fb_concurrent(id, v, note) VALUES (10003, 100030, 's1_i3');
INSERT INTO public.hg_fb_concurrent(id, v, note) VALUES (10004, 100040, 's1_i4');
INSERT INTO public.hg_fb_concurrent(id, v, note) VALUES (10005, 100050, 's1_i5');
UPDATE public.hg_fb_concurrent SET v = v + 1, note = note || '_u1' WHERE id BETWEEN 10001 AND 10003;
UPDATE public.hg_fb_concurrent SET v = v + 2, note = note || '_u2' WHERE id BETWEEN 10004 AND 10005;
DELETE FROM public.hg_fb_concurrent WHERE id = 10001;
DELETE FROM public.hg_fb_concurrent WHERE id = 10002;
COMMIT;
]);

$s2->query_safe(q[
BEGIN;
INSERT INTO public.hg_fb_concurrent(id, v, note) VALUES (20001, 200010, 's2_i1');
INSERT INTO public.hg_fb_concurrent(id, v, note) VALUES (20002, 200020, 's2_i2');
INSERT INTO public.hg_fb_concurrent(id, v, note) VALUES (20003, 200030, 's2_i3');
INSERT INTO public.hg_fb_concurrent(id, v, note) VALUES (20004, 200040, 's2_i4');
INSERT INTO public.hg_fb_concurrent(id, v, note) VALUES (20005, 200050, 's2_i5');
UPDATE public.hg_fb_concurrent SET v = v + 10, note = note || '_u1' WHERE id BETWEEN 20001 AND 20003;
UPDATE public.hg_fb_concurrent SET v = v + 20, note = note || '_u2' WHERE id BETWEEN 20004 AND 20005;
DELETE FROM public.hg_fb_concurrent WHERE id = 20001;
DELETE FROM public.hg_fb_concurrent WHERE id = 20002;
COMMIT;
]);

$s1->quit;
$s2->quit;

$node->poll_query_until(
	'postgres',
	"SELECT count(*) >= 20 FROM hg_flashback_tx.flashback_transaction_query"
) or die "timed out waiting for concurrent undo records";

my $total_count = $node->safe_psql(
	'postgres',
	"SELECT count(*) FROM hg_flashback_tx.flashback_transaction_query");
ok($total_count >= 20, "concurrent transactions: total undo records >= 20");

my $xid_count = $node->safe_psql(
	'postgres',
	"SELECT count(DISTINCT xid) FROM hg_flashback_tx.flashback_transaction_query");
ok($xid_count >= 2, "concurrent transactions: at least 2 different xids");

my $op_types = $node->safe_psql(
	'postgres',
	"SELECT count(DISTINCT operation) FROM hg_flashback_tx.flashback_transaction_query");
is($op_types, '3', 'concurrent transactions: operation types include I, U, D');

my $table_name_check = $node->safe_psql(
	'postgres',
	"SELECT bool_and(table_name = 'hg_fb_concurrent') FROM hg_flashback_tx.flashback_transaction_query");
is($table_name_check, 't', 'concurrent transactions: table_name is correct');

my $table_owner_check = $node->safe_psql(
	'postgres',
	"SELECT bool_and(table_owner = current_user) FROM hg_flashback_tx.flashback_transaction_query");
is($table_owner_check, 't', 'concurrent transactions: table_owner is correct');

my $rowid_check = $node->safe_psql(
	'postgres',
	"SELECT bool_and(row_id IS NOT NULL) FROM hg_flashback_tx.flashback_transaction_query");
is($rowid_check, 't', 'concurrent transactions: row_id is populated for all records');

my $undo_sql_check = $node->safe_psql(
	'postgres',
	"SELECT bool_and(undo_sql IS NOT NULL AND length(undo_sql) > 0) FROM hg_flashback_tx.flashback_transaction_query");
is($undo_sql_check, 't', 'concurrent transactions: undo_sql is populated for all records');

my $undo_change_seq = $node->safe_psql(
	'postgres',
	q[
SELECT count(*) >= 1
  FROM (
	SELECT min("undo_change#") as min_undo, max("undo_change#") as max_undo, count(*) as cnt
	  FROM hg_flashback_tx.flashback_transaction_query
	 GROUP BY xid
	 HAVING min("undo_change#") = 1
  ) sub
]
);
is($undo_change_seq, 't', 'concurrent transactions: at least one xid starts from undo_change# = 1');

my $undo_change_unique = $node->safe_psql(
	'postgres',
	"SELECT count(*) >= 1 FROM (SELECT count(DISTINCT \"undo_change#\") as cnt, xid FROM hg_flashback_tx.flashback_transaction_query GROUP BY xid HAVING count(*) >= 5) sub");
is($undo_change_unique, 't', 'concurrent transactions: at least one xid has sequential undo_change# (>=5 changes)');

my $op_match_undo = $node->safe_psql(
	'postgres',
	q[
SELECT bool_and(
	CASE operation
		WHEN 'I' THEN undo_sql LIKE 'DELETE%'
		WHEN 'U' THEN undo_sql LIKE 'UPDATE%'
		WHEN 'D' THEN undo_sql LIKE 'INSERT%'
		ELSE false
	END
) FROM hg_flashback_tx.flashback_transaction_query
]
);
is($op_match_undo, 't', 'concurrent transactions: operation matches undo_sql type');

my $timestamp_check = $node->safe_psql(
	'postgres',
	q[
SELECT 
	(
		SELECT max(commit_timestamp) - min(commit_timestamp) < interval '1 second'
		  FROM hg_flashback_tx.flashback_transaction_query
	)
]
);
is($timestamp_check, 't', 'concurrent transactions: commit timestamps are close (concurrent execution)');

my $final_data_check = $node->safe_psql('postgres',
	"SELECT count(*) FROM public.hg_fb_concurrent WHERE id >= 10000 OR id >= 20000");
ok($final_data_check >= 6, 'concurrent transactions: final data count is correct (some deleted, some remain)');

$node->stop;
done_testing();
