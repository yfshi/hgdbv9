# Copyright (c) 2026

use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More tests => 9;

my $node = get_new_node('hg_flashback_tx_edge_cases');
$node->init(allows_streaming => 'logical');
$node->append_conf('postgresql.conf', q[
shared_preload_libraries = 'hg_flashback_tx'
wal_level = logical
max_replication_slots = 4
max_wal_senders = 4
hg_flashback_tx.enable_flashback_transaction_query = on
hg_flashback_tx.auto_replica_identity = on
]);
$node->start;

$node->poll_query_until(
	'postgres',
	"SELECT count(*) = 1 FROM pg_extension WHERE extname = 'hg_flashback_tx'"
) or die "timed out waiting for hg_flashback_tx extension to be created";

$node->safe_psql('postgres', q[
CREATE TABLE IF NOT EXISTS public.hg_fb_json(
	id int PRIMARY KEY,
	note text,
	extra text
);

CREATE TABLE IF NOT EXISTS public.hg_fb_nopk_dup(
	a int,
	b text
);

CREATE TABLE IF NOT EXISTS public.hg_fb_cpk_order(
	a int,
	b int,
	v text,
	PRIMARY KEY (b, a)
);
]);

$node->safe_psql('postgres',
	"ALTER SYSTEM SET hg_flashback_tx.enable_flashback_transaction_query = on");
$node->safe_psql('postgres',
	"ALTER SYSTEM SET hg_flashback_tx.enable_flashback_transaction_query_auto_purge = off");
$node->safe_psql('postgres',
	"ALTER SYSTEM SET hg_flashback_tx.undo_retention = 0");
$node->safe_psql('postgres', "SELECT pg_reload_conf()");

$node->poll_query_until(
	'postgres',
	"SELECT count(*) = 1 FROM pg_replication_slots WHERE slot_name = 'hg_flashback_tx_slot_postgres'"
) or die "timed out waiting for hg_flashback_tx_slot_postgres";
ok(1, 'slot created for edge-case node');

# Case 1: JSON special characters survive decoder->worker round trip.
$node->safe_psql('postgres', q[
TRUNCATE hg_flashback_tx.flashback_transaction_query;
DELETE FROM public.hg_fb_json;
INSERT INTO public.hg_fb_json(id, note, extra)
VALUES (1, E'old\tline1\nline2\"q\"\\slash\\\\end', 'x');
UPDATE public.hg_fb_json
   SET note = E'new\tv\ntext', extra = 'y'
 WHERE id = 1;
]);

$node->poll_query_until(
	'postgres',
	"SELECT count(*) >= 2 FROM hg_flashback_tx.flashback_transaction_query WHERE table_name = 'hg_fb_json'"
) or die "timed out waiting for json edge records";

my $undo_special = $node->safe_psql(
	'postgres',
	q[
SELECT undo_sql
  FROM hg_flashback_tx.flashback_transaction_query
 WHERE table_name = 'hg_fb_json'
   AND operation = 'U'
 ORDER BY commit_timestamp DESC
 LIMIT 1;
]);
$node->safe_psql('postgres', $undo_special);

my $special_roundtrip = $node->safe_psql(
	'postgres',
	q[
SELECT note = E'old\tline1\nline2\"q\"\\slash\\\\end' AND extra = 'x'
  FROM public.hg_fb_json
 WHERE id = 1;
]);
is($special_roundtrip, 't', 'special characters are correctly restored by undo SQL');

# Case 2: NULL / empty string / literal "null" are distinct.
$node->safe_psql('postgres', q[
TRUNCATE hg_flashback_tx.flashback_transaction_query;
DELETE FROM public.hg_fb_json WHERE id = 2;
INSERT INTO public.hg_fb_json(id, note, extra) VALUES (2, NULL, '');
UPDATE public.hg_fb_json SET note = '', extra = 'null' WHERE id = 2;
]);

$node->poll_query_until(
	'postgres',
	"SELECT count(*) >= 1 FROM hg_flashback_tx.flashback_transaction_query WHERE table_name = 'hg_fb_json' AND operation = 'U'"
) or die "timed out waiting for null/empty edge records";

my $undo_null_empty = $node->safe_psql(
	'postgres',
	q[
SELECT undo_sql
  FROM hg_flashback_tx.flashback_transaction_query
 WHERE table_name = 'hg_fb_json'
   AND operation = 'U'
   AND undo_sql LIKE '%WHERE id = 2%'
 ORDER BY commit_timestamp DESC
 LIMIT 1;
]);
$node->safe_psql('postgres', $undo_null_empty);

my $null_empty_check = $node->safe_psql(
	'postgres',
	q[
SELECT note IS NULL AND extra = ''
  FROM public.hg_fb_json
 WHERE id = 2;
]);
is($null_empty_check, 't', 'NULL, empty string and literal null are not conflated');

# Case 3: no primary key fallback can touch duplicate rows (lock current behavior).
$node->safe_psql('postgres', q[
TRUNCATE hg_flashback_tx.flashback_transaction_query;
DELETE FROM public.hg_fb_nopk_dup;
INSERT INTO public.hg_fb_nopk_dup(a, b) VALUES (1, 'x'), (1, 'x');
UPDATE public.hg_fb_nopk_dup SET b = 'y' WHERE a = 1 AND b = 'x';
]);

$node->poll_query_until(
	'postgres',
	"SELECT count(*) >= 2 FROM hg_flashback_tx.flashback_transaction_query WHERE table_name = 'hg_fb_nopk_dup' AND operation = 'U'"
) or die "timed out waiting for nopk duplicate update records";

my $undo_nopk = $node->safe_psql(
	'postgres',
	q[
SELECT undo_sql
  FROM hg_flashback_tx.flashback_transaction_query
 WHERE table_name = 'hg_fb_nopk_dup'
   AND operation = 'U'
 ORDER BY commit_timestamp DESC
 LIMIT 1;
]);
$node->safe_psql('postgres', $undo_nopk);

my $dup_back = $node->safe_psql(
	'postgres',
	"SELECT count(*) FROM public.hg_fb_nopk_dup WHERE a = 1 AND b = 'x'");
is($dup_back, '2', 'no-PK fallback predicate matches duplicate rows as current behavior');

# Case 4: composite PK predicate order follows index key order.
$node->safe_psql('postgres', q[
TRUNCATE hg_flashback_tx.flashback_transaction_query;
DELETE FROM public.hg_fb_cpk_order;
INSERT INTO public.hg_fb_cpk_order(a, b, v) VALUES (10, 20, 'old');
UPDATE public.hg_fb_cpk_order SET v = 'new' WHERE a = 10 AND b = 20;
]);

$node->poll_query_until(
	'postgres',
	"SELECT count(*) >= 1 FROM hg_flashback_tx.flashback_transaction_query WHERE table_name = 'hg_fb_cpk_order' AND operation = 'U'"
) or die "timed out waiting for cpk-order update record";

my $cpk_where_order = $node->safe_psql(
	'postgres',
	q[
SELECT undo_sql
  FROM hg_flashback_tx.flashback_transaction_query
 WHERE table_name = 'hg_fb_cpk_order'
   AND operation = 'U'
 ORDER BY commit_timestamp DESC
 LIMIT 1;
]);
like(
	$cpk_where_order,
	qr/WHERE b = 20 AND a = 10$/,
	'composite PK WHERE predicate uses key order (b, a)');

# Case 5: long transaction pressure should not crash worker.
$node->safe_psql('postgres', q[
TRUNCATE hg_flashback_tx.flashback_transaction_query;
DELETE FROM public.hg_fb_json WHERE id >= 10000;
BEGIN;
INSERT INTO public.hg_fb_json(id, note, extra)
SELECT 10000 + g, 'bulk_' || g::text, 'payload'
  FROM generate_series(1, 3000) g;
COMMIT;
]);

$node->poll_query_until(
	'postgres',
	"SELECT count(*) >= 3000 FROM hg_flashback_tx.flashback_transaction_query WHERE table_name = 'hg_fb_json'"
) or die "timed out waiting for long transaction undo records";

my $alive_after_long_tx = $node->safe_psql('postgres', "SELECT 1");
is($alive_after_long_tx, '1', 'worker remains healthy after long transaction load');

# Case 6: CREATE TABLE without explicit schema should not crash utility hook path.
$node->safe_psql('postgres', q[
DROP TABLE IF EXISTS hg_fb_no_schema;
CREATE TABLE hg_fb_no_schema(id int PRIMARY KEY, v text);
INSERT INTO hg_fb_no_schema(id, v) VALUES (1, 'a');
UPDATE hg_fb_no_schema SET v = 'b' WHERE id = 1;
DELETE FROM hg_fb_no_schema WHERE id = 1;
]);

$node->poll_query_until(
	'postgres',
	"SELECT count(*) >= 3 FROM hg_flashback_tx.flashback_transaction_query WHERE table_name = 'hg_fb_no_schema'"
) or die "timed out waiting for no-schema table records";
ok(1, 'CREATE TABLE without schema name is handled');

# Case 7: purge uses commit_timestamp policy.
$node->safe_psql('postgres',
	"ALTER SYSTEM SET hg_flashback_tx.enable_flashback_transaction_query_auto_purge = on");
$node->safe_psql('postgres',
	"ALTER SYSTEM SET hg_flashback_tx.undo_retention = 1");
$node->safe_psql('postgres', "SELECT pg_reload_conf()");

$node->safe_psql('postgres', q[
TRUNCATE hg_flashback_tx.flashback_transaction_query;
INSERT INTO public.hg_fb_json(id, note, extra) VALUES (3, 'purge', 't')
ON CONFLICT (id) DO UPDATE SET note = EXCLUDED.note;
UPDATE hg_flashback_tx.flashback_transaction_query
   SET commit_timestamp = now() - interval '5 seconds';
]);

$node->poll_query_until(
	'postgres',
	"SELECT count(*) = 0 FROM hg_flashback_tx.flashback_transaction_query"
) or die "timed out waiting for commit_timestamp based purge";
ok(1, 'auto purge removes rows by commit_timestamp retention');

# Case 8: restart continuity with existing logical slot.
$node->safe_psql('postgres',
	"ALTER SYSTEM SET hg_flashback_tx.enable_flashback_transaction_query_auto_purge = off");
$node->safe_psql('postgres',
	"ALTER SYSTEM SET hg_flashback_tx.undo_retention = 0");
$node->safe_psql('postgres', "SELECT pg_reload_conf()");

$node->safe_psql('postgres', q[
TRUNCATE hg_flashback_tx.flashback_transaction_query;
INSERT INTO public.hg_fb_json(id, note, extra) VALUES (4, 'before_restart', 'a')
ON CONFLICT (id) DO UPDATE SET note = EXCLUDED.note;
]);

$node->poll_query_until(
	'postgres',
	"SELECT count(*) >= 1 FROM hg_flashback_tx.flashback_transaction_query"
) or die "timed out waiting for pre-restart record";

my $before_restart = $node->safe_psql(
	'postgres',
	"SELECT count(*) FROM hg_flashback_tx.flashback_transaction_query");

$node->restart;

$node->poll_query_until(
	'postgres',
	"SELECT count(*) = 1 FROM pg_extension WHERE extname = 'hg_flashback_tx'"
) or die "timed out waiting for extension after restart";

$node->safe_psql('postgres', q[
INSERT INTO public.hg_fb_json(id, note, extra) VALUES (5, 'after_restart', 'b')
ON CONFLICT (id) DO UPDATE SET note = EXCLUDED.note;
]);

$node->poll_query_until(
	'postgres',
	"SELECT count(*) > $before_restart FROM hg_flashback_tx.flashback_transaction_query"
) or die "timed out waiting for post-restart record";

my $after_restart = $node->safe_psql(
	'postgres',
	"SELECT count(*) > $before_restart FROM hg_flashback_tx.flashback_transaction_query");
is($after_restart, 't', 'recording continues correctly after restart');

$node->stop;
