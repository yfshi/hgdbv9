# Exercise the selected SQL parser, including its procedural SQL path.
use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More;

my $key = '0123456789abcdef' x 4;
for my $mode (qw(pg oracle mysql))
{
    my $node = get_new_node("column_$mode");
    # PostgresNode supplies -m pg first; the trailing -m selects this cluster.
    $node->init(allows_streaming => 1,
	extra => ['-m', $mode, '-K', 'AES', '-C', "echo $key", '-z', 'CUSTOM']);
    if ($mode ne 'pg')
    {
        my $other = get_new_node("port_$mode");
        my $setting = $mode eq 'oracle' ? 'ivorysql.port' : 'mysql.port';
        $node->append_conf('postgresql.conf', "$setting = " . $other->port . "\n");
    }
    $node->start;
    # safe_psql uses -f -, so SET is sent before the following statement is
    # parsed. Connecting to the PG port alone would exercise the wrong parser.
    my $sql = sub {
        return $node->safe_psql('postgres', "SET ivorysql.compatible_mode = '$mode';\n" . shift);
    };
    is($sql->('SHOW ivorysql.database_mode'), $mode, "$mode cluster selected by initdb");
    is($sql->('SHOW ivorysql.compatible_mode'), $mode, "$mode parser selected in session");
    my $plain = "IVORY_PLAIN_${mode}_98b31fd8";
    my $cipher = "IVORY_SECRET_${mode}_72c84a19";
    $sql->("CREATE TABLE column_test (plain text, cipher text encrypt);\nINSERT INTO column_test VALUES ('$plain', '$cipher');\nCHECKPOINT;");
    is($sql->('SELECT plain, cipher FROM column_test'), "$plain|$cipher", "$mode encrypted column round trip");
    my $bytes = sub {
        my $path = $sql->("SELECT pg_relation_filepath('column_test')");
        return slurp_file($node->data_dir . "/$path");
    };
    my $data = $bytes->();
    ok(index($data, $plain) >= 0, "$mode ordinary column is plaintext in heap");
    ok(index($data, $cipher) < 0, "$mode encrypted column hides heap plaintext");
    $sql->('ALTER TABLE column_test ALTER COLUMN cipher TYPE text;');
    $sql->("VACUUM column_test;\nCHECKPOINT;");
    ok(index($bytes->(), $cipher) >= 0, "$mode ALTER TYPE removes encryption and rewrites rows");
    $sql->("ALTER TABLE column_test ALTER COLUMN cipher TYPE text encrypt;\nCHECKPOINT;");
    ok(index($bytes->(), $cipher) < 0, "$mode ALTER TYPE encrypt rewrites existing rows");
    is($sql->('SELECT cipher FROM column_test'), $cipher, "$mode ALTER preserves logical value");
    $sql->("ALTER TABLE column_test ADD extra text encrypt DEFAULT 'extra_secret';");
    is($sql->('SELECT extra FROM column_test'), 'extra_secret', "$mode ADD encrypted default materializes existing rows");
    is($sql->("SELECT attencrypted FROM pg_attribute WHERE attrelid='column_test'::regclass AND attname='extra'"), 't', "$mode ADD preserves encryption policy");
    $sql->("CHECKPOINT;");
    ok(index($bytes->(), 'extra_secret') < 0, "$mode ADD default absent from heap plaintext");
    if ($mode eq 'oracle')
    {
        $sql->('ALTER TABLE column_test MODIFY cipher COLLATE "C";');
        is($sql->("SELECT attencrypted FROM pg_attribute WHERE attrelid='column_test'::regclass AND attname='cipher'"), 't', 'Oracle MODIFY COLLATE preserves encryption');
        $sql->("ALTER TABLE column_test MODIFY cipher text;\nCHECKPOINT;");
        ok(index($bytes->(), $cipher) >= 0, 'Oracle MODIFY removes encryption');
        $sql->("ALTER TABLE column_test MODIFY (cipher text encrypt);\nCHECKPOINT;");
        ok(index($bytes->(), $cipher) < 0, 'Oracle parenthesized MODIFY adds encryption');
        is($sql->('SELECT cipher FROM column_test'), $cipher, 'Oracle MODIFY preserves logical value');
        $sql->("CREATE TABLE invisible_columns (plain text, cipher text encrypt invisible);\nINSERT INTO invisible_columns (plain, cipher) VALUES ('visible', 'invisible_secret');");
        is($sql->('SELECT * FROM invisible_columns'), 'visible', 'Oracle encrypted invisible column stays invisible');
        is($sql->('SELECT cipher FROM invisible_columns'), 'invisible_secret', 'Oracle invisible column decrypts when explicitly selected');
        my ($status, $out, $err) = $node->psql('postgres', "SET ivorysql.compatible_mode = 'oracle';\nCREATE TABLE bad_virtual (plain text, cipher text encrypt GENERATED ALWAYS AS (plain) VIRTUAL);");
        isnt($status, 0, 'Oracle rejects encrypted virtual columns');
        like($err, qr/virtual columns cannot be encrypted/, 'virtual-column restriction reaches shared semantic check');

    }
    my $language = $mode eq 'oracle' ? 'plisql' : $mode eq 'mysql' ? 'plmysql' : 'plpgsql';
    $sql->(q{DO $$ BEGIN
        EXECUTE 'CREATE TABLE dynamic_column (payload text encrypt)';
        EXECUTE 'INSERT INTO dynamic_column VALUES (''dynamic_secret'')';
        EXECUTE 'ALTER TABLE dynamic_column ALTER COLUMN payload TYPE text';
        EXECUTE 'ALTER TABLE dynamic_column ALTER COLUMN payload TYPE text encrypt';
    END $$ LANGUAGE } . $language . ';');
    is($sql->('SELECT payload FROM dynamic_column'), 'dynamic_secret', "$language executes column encryption DDL dynamically");
    is($sql->("SELECT attencrypted FROM pg_attribute WHERE attrelid='dynamic_column'::regclass AND attname='payload'"), 't', "$language dynamic ALTER preserves encryption policy");
    my $dump = $node->basedir . '/column_dump.sql';
    $node->command_ok(['pg_dump', '-h', $node->host, '-p', $node->port, '-d', 'postgres', '-t', 'column_test', '-f', $dump], "$mode pg_dump succeeds");
    like(slurp_file($dump), qr/\bcipher\b[^\n]*\bENCRYPT\b/i, "$mode dump retains encryption declaration");
    $node->safe_psql('postgres', 'CREATE DATABASE column_restore');
    $node->command_ok(['psql', '-X', '-v', 'ON_ERROR_STOP=1', '-h', $node->host, '-p', $node->port, '-d', 'column_restore', '-f', $dump], "$mode dump restores");
    is($node->safe_psql('column_restore', "SELECT attencrypted FROM pg_attribute WHERE attrelid='column_test'::regclass AND attname='cipher'"), 't', "$mode restored column stays encrypted");
    is($node->safe_psql('column_restore', 'SELECT cipher FROM column_test'), $cipher, "$mode restored data round trip");
    $node->restart;
    is($sql->('SELECT cipher FROM column_test'), $cipher, "$mode encrypted column survives restart");
    $node->stop;
}
my $ecpg_source = 'tmp_check/column_ecpg.pgc';
open my $ecpg_file, '>', $ecpg_source or die "open $ecpg_source: $!";
print $ecpg_file <<'ECPG';
int main(void)
{
 EXEC SQL CREATE TABLE ecpg_columns (plain text, cipher text encrypt);
 EXEC SQL ALTER TABLE ecpg_columns ALTER COLUMN cipher TYPE text;
 EXEC SQL ALTER TABLE ecpg_columns ALTER COLUMN cipher TYPE text encrypt;
 return 0;
}
ECPG
close $ecpg_file;
command_ok(['ecpg', '-o', 'tmp_check/column_ecpg.c', $ecpg_source], 'ECPG preprocesses CREATE and both ALTER policies');
done_testing();
