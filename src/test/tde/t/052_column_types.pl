# Scalar varlena, array and domain encryption across the SQL dialects.
use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More;

my $key = '0123456789abcdef' x 4;
my @pg = (
 ['varchar(80)', q{'secret_varchar_732fe1'}, q{length(cipher)=length('secret_varchar_732fe1')}],
 ['char(40)', q{'secret_char_732fe1'}, q{length(cipher)=length('secret_char_732fe1')}],
 ['bytea', q{decode('7365637265745f62797465615f373332666531','hex')}, q{get_byte(cipher,0)=115}],
 ['numeric(24,6)', q{12345678901234.123456}, q{cipher+1=12345678901235.123456}],
 ['decimal(18,4)', q{-1234567890.1250}, q{abs(cipher)=1234567890.1250}],
 ['json', q{'{"secret":"json_732fe1","n":7}'}, q{cipher->>'n'='7'}],
 ['jsonb', q{'{"secret":"jsonb_732fe1","n":7}'}, q{cipher @> '{"n":7}'::jsonb}],
 ['bit(16)', q{B'1010100011001101'}, q{bit_length(cipher)=16}],
 ['bit varying(32)', q{B'101010001100110100001111'}, q{bit_length(cipher)=24}],
 ['inet', q{'192.168.37.142/24'}, q{host(cipher)='192.168.37.142'}],
 ['cidr', q{'2001:db8:abcd::/48'}, q{masklen(cipher)=48}],
 ['integer[]', q{ARRAY[123456,987654,NULL]}, q{cipher[2]=987654}],
 ['text[]', q{ARRAY['array_secret_732fe1',NULL,'tail']}, q{cipher[1]='array_secret_732fe1'}],
 ['numeric[]', q{ARRAY[12.345::numeric,67.890]}, q{cipher[1]+cipher[2]=80.235}],
 ['date[]', q{ARRAY[DATE '2026-01-01',DATE '2026-10-16']}, q{cipher[2]=DATE '2026-10-16'}],
 ['uuid[]', q{ARRAY['d366019a-75bf-4ac4-8d82-77059d2a64a2'::uuid]}, q{cardinality(cipher)=1}],
 ['positive_secret', q{1234567.875}, q{cipher*2=2469135.750}],
 ['secret_array', q{ARRAY[14,28,42]}, q{cipher[3]=42}],
 ['xml', q{'<secret>xml_732fe1</secret>'}, q{xmlserialize(CONTENT cipher AS text) LIKE '%xml_732fe1%'}],
);
my @oracle = (
 ['varchar2(80 byte)', q{'oracle_varchar_byte_732fe1'}, q{length(cipher)>10}],
 ['varchar2(80 char)', q{'oracle_varchar_char_732fe1'}, q{length(cipher)>10}],
 ['nvarchar2(80)', q{'oracle_nvarchar_732fe1'}, q{length(cipher)>10}],
 ['char(40 byte)', q{'oracle_char_byte_732fe1'}, q{length(cipher)>10}],
 ['char(40 char)', q{'oracle_char_char_732fe1'}, q{length(cipher)>10}],
 ['nchar(40)', q{'oracle_nchar_732fe1'}, q{length(cipher)>10}],
 ['number(24,6)', q{12345678901234.123456}, q{cipher+1=12345678901235.123456}],
 ['clob', q{'oracle_clob_732fe1'}, q{length(cipher)>10}],
 ['nclob', q{'oracle_nclob_732fe1'}, q{length(cipher)>10}],
 ['blob', q{decode('7365637265745f626c6f625f373332666531','hex')}, q{octet_length(cipher)>10}],
 ['raw(80)', q{'7365637265745f7261775f373332666531'}, q{octet_length(cipher)>10}],
 ['long', q{'oracle_long_732fe1'}, q{length(cipher)>10}],
 ['long raw', q{decode('7365637265745f6c6f6e677261775f373332666531','hex')}, q{octet_length(cipher)>10}],
);
my @mysql = (
 ['varchar(80)', q{'mysql_varchar_732fe1'}, q{length(cipher)>10}],
 ['char(40)', q{'mysql_char_732fe1'}, q{length(cipher)>10}],
 ['decimal(24,6)', q{12345678901234.123456}, q{cipher+1=12345678901235.123456}],
 ['bytea', q{decode('7365637265745f62797465615f373332666531','hex')}, q{get_byte(cipher,0)=115}],
 ['json', q{'{"secret":"mysql_json_732fe1","n":7}'}, q{cipher->>'n'='7'}],
);
for my $mode (qw(pg oracle mysql))
{
 my $node=get_new_node("types_$mode");
 $node->init(allows_streaming=>1, extra=>['-m',$mode,'-K','AES','-C',"echo $key",'-z','CUSTOM']);
 $node->start;
 my $sql=sub { $node->safe_psql('postgres',"SET ivorysql.compatible_mode='$mode';\n".shift) };
 if ($mode eq 'pg')
 {
  $sql->('CREATE DOMAIN positive_secret AS numeric(20,3) CHECK (VALUE > 0); CREATE DOMAIN secret_array AS int[] CHECK (cardinality(VALUE)>0);');
 }
 my @cases=$mode eq 'pg'?@pg:$mode eq 'oracle'?@oracle:@mysql;
 my $i=0;
 for my $case (@cases)
 {
  my ($type,$value,$op)=@$case;
  my $table='type_case_'.++$i;
  my ($status,$out,$err)=$node->psql('postgres',"SET ivorysql.compatible_mode='$mode';\nCREATE TABLE $table (plain text, cipher $type encrypt, id int);");
  is($status,0,"$mode accepts encrypted $type") or diag($err);
  next if $status;
  $sql->("INSERT INTO $table VALUES ('CLEAR_$table',($value)::$type,1),('NULL_$table',NULL,2);");
  is($sql->("SELECT cipher::text = (($value)::$type)::text FROM $table WHERE id=1"),'t',"$mode $type output round trip");
  is($sql->("SELECT $op FROM $table WHERE id=1"),'t',"$mode $type operator sees plaintext");
  is($sql->("SELECT cipher IS NULL FROM $table WHERE id=2"),'t',"$mode $type preserves NULL");
  is($sql->("SELECT pg_column_size(cipher)>pg_column_size(($value)::$type) FROM $table WHERE id=1"),'t',"$mode $type stores expanded encryption envelope");
  $sql->("UPDATE $table SET cipher=cipher WHERE id=1; CHECKPOINT;");
  my $path=$sql->("SELECT pg_relation_filepath('$table')");
  ok(index(slurp_file($node->data_dir."/$path"),"CLEAR_$table")>=0,"$mode $type leaves ordinary column plaintext");
  my $old=$sql->("SELECT pg_relation_filenode('$table')");
  $sql->("ALTER TABLE $table ALTER cipher TYPE $type;");
  isnt($sql->("SELECT pg_relation_filenode('$table')"),$old,"$mode $type removing encryption rewrites");
  is($sql->("SELECT cipher::text = (($value)::$type)::text FROM $table WHERE id=1"),'t',"$mode $type removal preserves value");
  $sql->("ALTER TABLE $table ALTER cipher TYPE $type encrypt;");
  is($sql->("SELECT $op FROM $table WHERE id=1"),'t',"$mode $type reencryption preserves operator");
 }
 # Domains and length/precision limits still apply before storage encryption.
 if ($mode eq 'pg')
 {
  for my $statement (
   "INSERT INTO type_case_1 VALUES ('bad', repeat('x',81),3)",
   "INSERT INTO type_case_17 VALUES ('bad',-1,3)")
  {
   my ($status,$out,$err)=$node->psql('postgres',$statement);
   isnt($status,0,'normal typmod/domain validation is retained');
  }
 }
 my @fixed=$mode eq 'pg' ? ('smallint','integer','bigint','real','double precision','boolean','uuid','date','time','timestamp','timestamptz','interval') :
           $mode eq 'oracle' ? ('date','timestamp','binary_float','binary_double') : ('integer','double precision','date','timestamp');
 for my $type (@fixed)
 {
  my ($status,$out,$err)=$node->psql('postgres',"SET ivorysql.compatible_mode='$mode';\nCREATE TABLE rejected_fixed(v $type encrypt);");
  isnt($status,0,"$mode fixed scalar $type explicitly rejected");
  like($err,qr/not supported for column encryption/,"$mode $type reports storage restriction");
 }
 my $dump=$node->basedir.'/typed_dump.sql';
 $node->command_ok(['pg_dump','-h',$node->host,'-p',$node->port,'-d','postgres','-f',$dump],"$mode dumps typed encryption schema and data");
 $node->safe_psql('postgres','CREATE DATABASE typed_restore');
 $node->command_ok(['psql','-X','-v','ON_ERROR_STOP=1','-h',$node->host,'-p',$node->port,'-d','typed_restore','-f',$dump],"$mode restores typed encryption schema and data");
 is($node->safe_psql('typed_restore',q{SELECT count(*) FROM pg_attribute WHERE attrelid IN (SELECT oid FROM pg_class WHERE relname LIKE 'type_case_%') AND attencrypted}),scalar(@cases),"$mode dump preserves all encrypted type attributes");
 $node->restart;
 is($sql->('SELECT count(*) FROM type_case_1 WHERE cipher IS NOT NULL'),'1',"$mode typed values survive restart");
 $node->stop;
}
done_testing();
