# Copyright (c) 2026
# Test for multiple databases configuration

use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More tests => 4;

# Create primary node
my $node = get_new_node('multi_db');
$node->init(allows_streaming => 'logical');
$node->append_conf('postgresql.conf', q[
shared_preload_libraries = 'hg_flashback_tx'
wal_level = logical
max_replication_slots = 8
max_wal_senders = 4
hg_flashback_tx.enable_flashback_transaction_query = on
hg_flashback_tx.databases = 'db1,db2'
]);
$node->start;

# Create databases (workers will fail initially but this is expected)
$node->safe_psql('postgres', q[
CREATE DATABASE db1;
CREATE DATABASE db2;
]);

# Restart - workers will now connect successfully
$node->restart;

# Wait for workers to initialize
sleep 2;

# Setup test objects in each database
$node->safe_psql('db1', q[
CREATE EXTENSION IF NOT EXISTS hg_flashback_tx;
CREATE TABLE IF NOT EXISTS hg_test(
    id int PRIMARY KEY,
    v int
);
]);

$node->safe_psql('db2', q[
CREATE EXTENSION IF NOT EXISTS hg_flashback_tx;
CREATE TABLE IF NOT EXISTS hg_test(
    id int PRIMARY KEY,
    v int
);
]);

# Verify replication slots are created for both databases
$node->poll_query_until(
    'postgres',
    "SELECT count(*) = 2 FROM pg_replication_slots WHERE slot_name LIKE 'hg_flashback_tx_slot_db%'"
) or die "timed out waiting for replication slots";

my $slot_count = $node->safe_psql(
    'postgres',
    "SELECT slot_name FROM pg_replication_slots WHERE slot_name LIKE 'hg_flashback_tx_slot_db%' ORDER BY slot_name"
);

like($slot_count, qr/hg_flashback_tx_slot_db1/m, 'slot for db1 created');
like($slot_count, qr/hg_flashback_tx_slot_db2/m, 'slot for db2 created');

# Test: Verify undo records are generated for both databases
$node->safe_psql('db1', q[
TRUNCATE hg_flashback_tx.flashback_transaction_query;
INSERT INTO hg_test(id, v) VALUES (1, 100);
]);

$node->safe_psql('db2', q[
TRUNCATE hg_flashback_tx.flashback_transaction_query;
INSERT INTO hg_test(id, v) VALUES (1, 200);
]);

$node->poll_query_until(
    'db1',
    "SELECT count(*) >= 1 FROM hg_flashback_tx.flashback_transaction_query"
) or die "timed out waiting for db1 undo records";

$node->poll_query_until(
    'db2',
    "SELECT count(*) >= 1 FROM hg_flashback_tx.flashback_transaction_query"
) or die "timed out waiting for db2 undo records";

my $db1_count = $node->safe_psql('db1', "SELECT count(*) FROM hg_flashback_tx.flashback_transaction_query");
my $db2_count = $node->safe_psql('db2', "SELECT count(*) FROM hg_flashback_tx.flashback_transaction_query");

is($db1_count, '1', 'db1: undo record generated');
is($db2_count, '1', 'db2: undo record generated');

$node->stop;
