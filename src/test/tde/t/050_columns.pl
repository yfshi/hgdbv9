# Column encryption contract, including ALTER TYPE removing encryption.
use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More;

my $node = get_new_node('columns');
my $key = '0123456789abcdef' x 4;
my $marker = 'COLUMN_SECRET_SENTINEL_5ff8571b1e43';
$node->init(allows_streaming => 1, extra => ['-K', 'AES', '-C', "echo $key", '-z', 'CUSTOM']);
$node->append_conf('postgresql.conf', "wal_level = logical\n");
$node->start;
my ($status, $stdout, $stderr) = $node->psql('postgres',
	'CREATE TABLE column_secrets (id int PRIMARY KEY, cipher text encrypt, plain text)');
is($status, 0, 'accept exact column syntax: cipher text encrypt') or diag($stderr);
if ($status != 0)
{
	$node->stop;
	done_testing();
	exit;
}
$node->safe_psql('postgres', qq{
	INSERT INTO column_secrets VALUES (1, NULL, 'null'), (2, '', 'empty'), (3, '$marker', 'normal');
	INSERT INTO column_secrets SELECT 4, string_agg(md5(i::text), ''), 'toast' FROM generate_series(1,5000) i;
	CREATE TABLE expected AS SELECT * FROM column_secrets;
	CHECKPOINT;
});
$node->safe_psql('postgres', q{
 CREATE TABLE test_table(plain text, cipher text encrypt);
 INSERT INTO test_table VALUES ('aaaaa', 'bbbbb');
 CHECKPOINT;
});
my $example_path = $node->safe_psql('postgres', "SELECT pg_relation_filepath('test_table')");
my $example_bytes = slurp_file($node->data_dir . "/$example_path");
ok(index($example_bytes, 'aaaaa') >= 0 && index($example_bytes, 'bbbbb') < 0,
 'CUSTOM stores only designated column encrypted');
is($node->safe_psql('postgres', 'SELECT plain, cipher FROM test_table'), 'aaaaa|bbbbb', 'SELECT transparently decrypts');
$node->safe_psql('postgres', 'ALTER TABLE test_table ALTER cipher TYPE text');
$node->safe_psql('postgres', 'VACUUM test_table');
$node->safe_psql('postgres', 'CHECKPOINT');
$example_path = $node->safe_psql('postgres', "SELECT pg_relation_filepath('test_table')");
$example_bytes = slurp_file($node->data_dir . "/$example_path");
ok(index($example_bytes, 'aaaaa') >= 0 && index($example_bytes, 'bbbbb') >= 0,
 'user ALTER VACUUM CHECKPOINT example restores plaintext');
is($node->safe_psql('postgres', q{SELECT length(cipher), octet_length(cipher), substring(cipher, 1800, 100) = substring((SELECT cipher FROM expected WHERE id=4),1800,100) FROM column_secrets WHERE id=4}),
 '160000|160000|t', 'long encrypted text lengths and slice');
$node->safe_psql('postgres', q{
 CREATE TABLE column_like (LIKE column_secrets INCLUDING ALL);
 INSERT INTO column_like SELECT * FROM column_secrets;
 ALTER TABLE column_like ADD extra text encrypt DEFAULT 'encrypted_default';
});
is($node->safe_psql('postgres', q{SELECT count(*) FROM column_like WHERE extra='encrypted_default'}), '4', 'LIKE and encrypted ADD DEFAULT preserve values');
is($node->safe_psql('postgres', q{SELECT attencrypted FROM pg_attribute WHERE attrelid='column_like'::regclass AND attname='cipher'}), 't', 'LIKE preserves encryption policy');
($status, $stdout, $stderr) = $node->psql('postgres', 'CREATE INDEX column_cipher_idx ON column_secrets(cipher)');
isnt($status, 0, 'reject plaintext index keys on encrypted columns');
$node->safe_psql('postgres', q{
 CREATE STATISTICS cipher_stats ON (lower(cipher)) FROM column_secrets;
 ANALYZE column_secrets;
 CHECKPOINT;
});
is($node->safe_psql('postgres', q{SELECT count(*) FROM pg_statistic_ext_data WHERE stxdexpr IS NOT NULL AND stxoid=(SELECT oid FROM pg_statistic_ext WHERE stxname='cipher_stats')}), '0', 'extended statistics do not expose encrypted expressions');
is($node->safe_psql('postgres', q{SELECT count(*) FROM pg_stats WHERE tablename='column_secrets' AND attname='cipher'}), '0', 'ANALYZE does not store plaintext column statistics');
my $toast_path = $node->safe_psql('postgres', q{SELECT pg_relation_filepath(reltoastrelid) FROM pg_class WHERE oid='column_secrets'::regclass});
my $long_prefix = $node->safe_psql('postgres', 'SELECT substring(cipher, 1, 64) FROM expected WHERE id=4');
ok(index(slurp_file($node->data_dir . "/$toast_path"), $long_prefix) < 0, 'external TOAST payload is encrypted');
$node->safe_psql('postgres', q{SELECT pg_create_logical_replication_slot('column_slot', 'test_decoding')});
$node->safe_psql('postgres', qq{
 CREATE TABLE logical_columns(cipher text encrypt);
 INSERT INTO logical_columns SELECT '$marker' || string_agg(md5(i::text), '') FROM generate_series(1,5000) i;
});
is($node->safe_psql('postgres', qq{SELECT count(*) FROM pg_logical_slot_get_changes('column_slot', NULL, NULL) WHERE data LIKE '%$marker%'}), '1', 'logical decoding safely reconstructs and decrypts expanded TOAST value');
$node->safe_psql('postgres', q{SELECT pg_drop_replication_slot('column_slot')});
for my $sql (
 'CREATE TABLE bad_type(cipher integer encrypt)',
 'CREATE TABLE bad_inherit() INHERITS (column_secrets)',
 'CREATE INDEX bad_expr ON column_secrets(lower(cipher))')
{
 ($status, $stdout, $stderr) = $node->psql('postgres', $sql);
 isnt($status, 0, "reject unsupported operation: $sql");
}
my $same = q{SELECT NOT EXISTS ((SELECT * FROM column_secrets EXCEPT SELECT * FROM expected) UNION ALL (SELECT * FROM expected EXCEPT SELECT * FROM column_secrets))};
is($node->safe_psql('postgres', $same), 't', 'NULL empty ordinary and long TOAST values round trip');
my $path = $node->safe_psql('postgres', "SELECT pg_relation_filepath('column_secrets')");
ok(index(slurp_file($node->data_dir . "/$path"), $marker) < 0, 'encrypted column absent from plaintext heap');
$node->safe_psql('postgres', "UPDATE column_secrets SET cipher = cipher || '-updated' WHERE id = 3; UPDATE expected SET cipher = cipher || '-updated' WHERE id = 3");
is($node->safe_psql('postgres', $same), 't', 'UPDATE preserves encrypted column values');
my $copy = $node->safe_psql('postgres', 'COPY column_secrets TO STDOUT');
$node->safe_psql('postgres', 'TRUNCATE column_secrets');
$node->safe_psql('postgres', "COPY column_secrets FROM STDIN;\n$copy\n\\.\n");
is($node->safe_psql('postgres', $same), 't', 'COPY exports and imports logical plaintext values');
$node->safe_psql('postgres', "BEGIN; UPDATE column_secrets SET cipher = 'rolled back'; ROLLBACK");
is($node->safe_psql('postgres', $same), 't', 'transaction rollback preserves encrypted values');
$node->restart;
is($node->safe_psql('postgres', $same), 't', 'encrypted column survives restart');
my $dump = $node->basedir . '/columns.sql';
$node->command_ok(['pg_dump', '-h', $node->host, '-p', $node->port, '-d', 'postgres', '-f', $dump], 'pg_dump encrypted column');
$node->safe_psql('postgres', 'CREATE DATABASE restored');
$node->command_ok(['psql', '-X', '-v', 'ON_ERROR_STOP=1', '-h', $node->host, '-p', $node->port, '-d', 'restored', '-f', $dump], 'restore encrypted schema and data');
is($node->safe_psql('restored', $same), 't', 'logical backup preserves values');
is($node->safe_psql('restored', q{SELECT attencrypted FROM pg_attribute WHERE attrelid='column_secrets'::regclass AND attname='cipher'}), 't', 'logical backup preserves encryption');
for my $plain (0, 1)
{
 my @options = ('-X', 'stream');
 push @options, '-y' if $plain;
 $node->backup("columns_$plain", backup_options => \@options);
 my $restore = get_new_node("columns_restore_$plain");
 $restore->init_from_backup($node, "columns_$plain");
 $restore->start;
 is($restore->safe_psql('postgres', $same), 't', "physical backup $plain preserves column keys and data");
 $restore->stop;
}

my $oldnode = $node->safe_psql('postgres', "SELECT pg_relation_filenode('column_secrets')");
$node->safe_psql('postgres', 'ALTER TABLE column_secrets ALTER COLUMN cipher TYPE text; CHECKPOINT');
my $newnode = $node->safe_psql('postgres', "SELECT pg_relation_filenode('column_secrets')");
isnt($newnode, $oldnode, 'ALTER cipher TYPE text forces a table rewrite');
is($node->safe_psql('postgres', $same), 't', 'removing column encryption preserves values');
$path = $node->safe_psql('postgres', "SELECT pg_relation_filepath('column_secrets')");
ok(index(slurp_file($node->data_dir . "/$path"), $marker) >= 0, 'rewritten text column is stored in plaintext');
$node->restart;
is($node->safe_psql('postgres', $same), 't', 'rewritten plaintext survives restart');
$node->safe_psql('postgres', 'ALTER TABLE column_secrets ALTER cipher TYPE text encrypt; CHECKPOINT');
is($node->safe_psql('postgres', $same), 't', 'enable encryption on existing values');
$path = $node->safe_psql('postgres', "SELECT pg_relation_filepath('column_secrets')");
ok(index(slurp_file($node->data_dir . "/$path"), $marker) < 0, 'enabling encryption rewrites plaintext');
$node->stop('immediate');
$node->start;
is($node->safe_psql('postgres', $same), 't', 'encrypted values survive crash recovery');
$node->stop;
done_testing();
