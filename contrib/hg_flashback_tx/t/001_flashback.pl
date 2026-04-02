# Copyright (c) 2026

use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More tests => 14;

my $node = get_new_node('hg_flashback_tx');
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

# Wait for background worker to create extension
$node->poll_query_until(
	'postgres',
	"SELECT count(*) = 1 FROM pg_extension WHERE extname = 'hg_flashback_tx'"
) or die "timed out waiting for hg_flashback_tx extension to be created";

# Setup test objects
$node->safe_psql('postgres', q[

	-- Main test table with single-column primary key
	CREATE TABLE IF NOT EXISTS public.hg_flashback_t(
		id int PRIMARY KEY,
		i int,
		note text
	);

	-- Table with single-column primary key for coverage tests
	CREATE TABLE IF NOT EXISTS public.hg_fb_pk(
		id int PRIMARY KEY,
		v int,
		note text
	);

		-- Table with PK only: should be tracked
		CREATE TABLE IF NOT EXISTS public.hg_fb_norowid(
			id int PRIMARY KEY,
			v int
		);

		-- Table with composite primary key
		CREATE TABLE IF NOT EXISTS public.hg_fb_cpk(
			id1 int,
			id2 int,
			v int,
			note text,
			PRIMARY KEY (id1, id2)
		);

		-- Table without primary key: row locator falls back to full row
		CREATE TABLE IF NOT EXISTS public.hg_fb_nopk(
			id int,
			v int,
			note text
		);

	-- Table in hg_security schema: should be filtered
	CREATE SCHEMA IF NOT EXISTS hg_security;
	CREATE TABLE IF NOT EXISTS hg_security.hg_fb_sec(
		id int PRIMARY KEY,
		v int
	);

-- Clean up existing data
TRUNCATE hg_flashback_tx.flashback_transaction_query;
DELETE FROM public.hg_flashback_t;
	DELETE FROM public.hg_fb_pk;
		DELETE FROM public.hg_fb_norowid;
		DELETE FROM public.hg_fb_cpk;
		DELETE FROM public.hg_fb_nopk;
		DELETE FROM hg_security.hg_fb_sec;
]);

$node->safe_psql('postgres',
	"ALTER SYSTEM SET hg_flashback_tx.enable_flashback_transaction_query = on");
$node->safe_psql('postgres', "SELECT pg_reload_conf()");

# Test 1: Verify replication slot is created
$node->poll_query_until(
	'postgres',
	"SELECT count(*) = 1 FROM pg_replication_slots WHERE slot_name = 'hg_flashback_tx_slot_postgres'"
) or die "timed out waiting for hg_flashback_tx_slot_postgres";
ok(1, 'replication slot created');

# Test 2: Basic DML undo SQL generation (INSERT/UPDATE/DELETE)
$node->safe_psql('postgres', q[
TRUNCATE hg_flashback_tx.flashback_transaction_query;
DELETE FROM public.hg_flashback_t;
]);

$node->safe_psql('postgres', q[
INSERT INTO public.hg_flashback_t(id, i, note) VALUES (1, 1000, 'a');
UPDATE public.hg_flashback_t SET i = 1001, note = 'b' WHERE id = 1;
DELETE FROM public.hg_flashback_t WHERE id = 1;
]);

$node->poll_query_until(
	'postgres',
	"SELECT count(*) >= 3 FROM hg_flashback_tx.flashback_transaction_query"
) or die "timed out waiting for undo records";

my $result = $node->safe_psql(
	'postgres',
	q[
SELECT operation, undo_sql
  FROM hg_flashback_tx.flashback_transaction_query
 ORDER BY commit_timestamp DESC
 LIMIT 3;
]);

# Verify undo SQL format for each operation
my $all = join("\n", split(/\n/, $result));
like($all,
	qr/^D\|.*INSERT INTO public\.hg_flashback_t \(id, i, note\) VALUES \(1, 1001, 'b'\)$/m,
	'DELETE generates INSERT undo SQL');
like($all,
	qr/^U\|.*UPDATE public\.hg_flashback_t SET id=1, i=1000, note='a' WHERE id = 1$/m,
	'UPDATE generates UPDATE undo SQL using primary key with correct old values');
like($all,
	qr/^I\|.*DELETE FROM public\.hg_flashback_t WHERE id = 1$/m,
	'INSERT generates DELETE undo SQL using primary key');

# Test 3: composite primary key undo SQL generation
$node->safe_psql('postgres', q[
TRUNCATE hg_flashback_tx.flashback_transaction_query;
DELETE FROM public.hg_fb_cpk;
INSERT INTO public.hg_fb_cpk(id1, id2, v, note) VALUES (1, 2, 100, 'x');
UPDATE public.hg_fb_cpk SET v = 101, note = 'y' WHERE id1 = 1 AND id2 = 2;
DELETE FROM public.hg_fb_cpk WHERE id1 = 1 AND id2 = 2;
]);

$node->poll_query_until(
	'postgres',
	"SELECT count(*) >= 3 FROM hg_flashback_tx.flashback_transaction_query"
) or die "timed out waiting for composite pk undo records";

my $cpk_result = $node->safe_psql(
	'postgres',
	q[
SELECT operation, undo_sql
  FROM hg_flashback_tx.flashback_transaction_query
 ORDER BY commit_timestamp DESC
 LIMIT 3;
]);
my $cpk_all = join("\n", split(/\n/, $cpk_result));
like($cpk_all,
	qr/^U\|.*UPDATE public\.hg_fb_cpk SET id1=1, id2=2, v=100, note='x' WHERE id1 = 1 AND id2 = 2$/m,
	'composite primary key: UPDATE undo SQL contains full PK predicate and old values');

# Test 4: no primary key fallback uses full-row locator
$node->safe_psql('postgres', q[
TRUNCATE hg_flashback_tx.flashback_transaction_query;
DELETE FROM public.hg_fb_nopk;
INSERT INTO public.hg_fb_nopk(id, v, note) VALUES (7, 70, 'p');
UPDATE public.hg_fb_nopk SET v = 71, note = 'q' WHERE id = 7 AND v = 70 AND note = 'p';
DELETE FROM public.hg_fb_nopk WHERE id = 7 AND v = 71 AND note = 'q';
]);

$node->poll_query_until(
	'postgres',
	"SELECT count(*) >= 3 FROM hg_flashback_tx.flashback_transaction_query"
) or die "timed out waiting for no-pk undo records";

my $nopk_result = $node->safe_psql(
	'postgres',
	q[
SELECT operation, undo_sql
  FROM hg_flashback_tx.flashback_transaction_query
 ORDER BY commit_timestamp DESC
 LIMIT 3;
]);
my $nopk_all = join("\n", split(/\n/, $nopk_result));
like($nopk_all,
	qr/^U\|.*UPDATE public\.hg_fb_nopk SET id=7, v=70, note='p' WHERE id = 7 AND v = 71 AND note = 'q'$/m,
	'no primary key: UPDATE undo SQL falls back to full-row locator');

# Test 6: Multi-change transaction keeps per-xid sequential undo_change#
$node->safe_psql('postgres', q[
TRUNCATE hg_flashback_tx.flashback_transaction_query;
BEGIN;
INSERT INTO public.hg_flashback_t(id, i, note) VALUES (2, 2000, 'c');
INSERT INTO public.hg_flashback_t(id, i, note) VALUES (3, 3000, 'd');
UPDATE public.hg_flashback_t SET note = 'e' WHERE id = 3;
COMMIT;
]);

$node->poll_query_until(
	'postgres',
	q[
WITH latest AS (
	SELECT xid
	  FROM hg_flashback_tx.flashback_transaction_query
	 ORDER BY commit_timestamp DESC
	 LIMIT 1
)
SELECT count(*) = 3
  FROM hg_flashback_tx.flashback_transaction_query
 WHERE xid = (SELECT xid FROM latest);
]
) or die "timed out waiting for multi-change transaction rows";

my $multi_check = $node->safe_psql(
	'postgres',
	q[
WITH latest AS (
	SELECT xid
	  FROM hg_flashback_tx.flashback_transaction_query
	 ORDER BY commit_timestamp DESC
	 LIMIT 1
)
SELECT
	count(*) = 3
	AND min("undo_change#") = 1
	AND max("undo_change#") = 3
	AND count(DISTINCT "undo_change#") = 3
  FROM hg_flashback_tx.flashback_transaction_query
 WHERE xid = (SELECT xid FROM latest);
]);
is($multi_check, 't', 'multi-change txn: same xid with sequential undo_change#');

# Test 7: Verify all records have valid row_id
my $rowid_check = $node->safe_psql(
	'postgres',
	q[
SELECT bool_and(row_id IS NOT NULL)
  FROM hg_flashback_tx.flashback_transaction_query;
]);
is($rowid_check, 't', 'row_id populated for all undo records');

# Test 8: table_owner field is captured correctly
my $owner_check = $node->safe_psql(
	'postgres',
	q[
SELECT bool_and(table_owner = current_user)
  FROM hg_flashback_tx.flashback_transaction_query;
]);
is($owner_check, 't', 'table_owner captured for all records');

# Test 9: GUC off - no recording when disabled
$node->safe_psql('postgres',
	"ALTER SYSTEM SET hg_flashback_tx.enable_flashback_transaction_query = off");
$node->safe_psql('postgres', "SELECT pg_reload_conf()");
$node->safe_psql('postgres', q[
TRUNCATE hg_flashback_tx.flashback_transaction_query;
INSERT INTO public.hg_fb_pk(id, v, note) VALUES (10, 100, 'off');
UPDATE public.hg_fb_pk SET v = 101 WHERE id = 10;
DELETE FROM public.hg_fb_pk WHERE id = 10;
]);

$node->poll_query_until(
	'postgres',
	"SELECT count(*) = 0 FROM hg_flashback_tx.flashback_transaction_query"
) or die "expected no records while GUC is off";
ok(1, 'GUC off: no undo records generated');

# Test 10: GUC on - basic tracking works
$node->safe_psql('postgres',
	"ALTER SYSTEM SET hg_flashback_tx.enable_flashback_transaction_query = on");
$node->safe_psql('postgres', "SELECT pg_reload_conf()");
$node->safe_psql('postgres', q[
TRUNCATE hg_flashback_tx.flashback_transaction_query;
INSERT INTO public.hg_fb_pk(id, v, note) VALUES (20, 200, 'on');
]);

$node->poll_query_until(
	'postgres',
	"SELECT count(*) >= 1 FROM hg_flashback_tx.flashback_transaction_query"
) or die "timed out waiting for records with GUC on";
ok(1, 'GUC on: undo records generated');

# Test 11: table with PK is tracked
$node->safe_psql('postgres', q[
TRUNCATE hg_flashback_tx.flashback_transaction_query;
INSERT INTO public.hg_fb_norowid(id, v) VALUES (1, 1);
UPDATE public.hg_fb_norowid SET v = 2 WHERE id = 1;
DELETE FROM public.hg_fb_norowid WHERE id = 1;
]);

$node->poll_query_until(
	'postgres',
	"SELECT count(*) >= 3 FROM hg_flashback_tx.flashback_transaction_query"
) or die "expected records for table with primary key";
ok(1, 'table with primary key is tracked');

# Test 12: hg_security schema is filtered
$node->safe_psql('postgres', q[
TRUNCATE hg_flashback_tx.flashback_transaction_query;
INSERT INTO hg_security.hg_fb_sec(id, v) VALUES (1, 1);
UPDATE hg_security.hg_fb_sec SET v = 2 WHERE id = 1;
DELETE FROM hg_security.hg_fb_sec WHERE id = 1;
]);

$node->poll_query_until(
	'postgres',
	"SELECT count(*) = 0 FROM hg_flashback_tx.flashback_transaction_query"
) or die "expected no records for hg_security schema";
ok(1, 'hg_security schema is filtered');

# Test 13: Multiple operations in single transaction (from coverage)
$node->safe_psql('postgres', q[
TRUNCATE hg_flashback_tx.flashback_transaction_query;
BEGIN;
INSERT INTO public.hg_fb_pk(id, v, note) VALUES (30, 300, 'x');
INSERT INTO public.hg_fb_pk(id, v, note) VALUES (31, 310, 'y');
UPDATE public.hg_fb_pk SET note = 'z' WHERE id = 31;
COMMIT;
]);

$node->poll_query_until(
	'postgres',
	q[
WITH latest AS (
	SELECT xid
	  FROM hg_flashback_tx.flashback_transaction_query
	 ORDER BY commit_timestamp DESC
	 LIMIT 1
)
SELECT count(*) = 3
  FROM hg_flashback_tx.flashback_transaction_query
 WHERE xid = (SELECT xid FROM latest);
]
) or die "timed out waiting for multi-change transaction rows";

my $multi_check2 = $node->safe_psql(
	'postgres',
	q[
WITH latest AS (
	SELECT xid
	  FROM hg_flashback_tx.flashback_transaction_query
	 ORDER BY commit_timestamp DESC
	 LIMIT 1
)
SELECT
	count(*) = 3
	AND min("undo_change#") = 1
	AND max("undo_change#") = 3
	AND count(DISTINCT "undo_change#") = 3
  FROM hg_flashback_tx.flashback_transaction_query
 WHERE xid = (SELECT xid FROM latest);
]);
is($multi_check2, 't', 'multi-change txn (coverage): sequential undo_change# for same xid');

$node->stop;
