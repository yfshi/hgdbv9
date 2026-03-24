# Copyright (c) 2021, PostgreSQL Global Development Group

# Testing of encrypted tablespace.
# ReqID:SRS-SECURITY-TABLESPACE
#
# Data files of tables and indexes located within an encrypted
# tablespace are encrypted.

use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More tests => 22;

# Initialize primary node
my $node = get_new_node('primary');
$node->init(allows_streaming => 'logical',
			extra => [ '-C', 'echo 181210f8f9c779c26da1d9b2075bde0127302ee0e3fca38c9a83f5b1dd8e5d3b' ]);
$node->start;

# Directory for the tablespace
my $tablespace_dir_1 = $node->basedir . '/tablespace_1';
my $tablespace_dir_2 = $node->basedir . '/tablespace_2';
mkdir $tablespace_dir_1 or die;
mkdir $tablespace_dir_2 or die;

# Create two tablespace using WITH encrypt
my ($result, $stdout, $stderr) = $node->safe_psql('postgres',
		qq[CREATE TABLESPACE regress_tblspace LOCATION '$tablespace_dir_1' WITH (encrypt = on);]);
$result = $node->safe_psql('postgres',
		qq[CREATE TABLESPACE regress_tblspace2 LOCATION '$tablespace_dir_2' WITH (encrypt = on);]);
is($result, '', 'Encrypted tablespace creation should succeed');

# Check to see the parameter was used
$result = $node->safe_psql('postgres',
	qq[SELECT spcoptions FROM pg_tablespace WHERE spcname = 'regress_tblspace';]
);
is($result, "{encrypt=on}", 'Check spcoptions');

# Try setting and resetting some properties for the new tablespace
($result, $stdout, $stderr) = $node->psql('postgres',
	qq[ALTER TABLESPACE regress_tblspace SET (encrypt = on);]
);
ok($stderr =~ /ERROR:  The encrypt attribute of the tablespace options cannot be modified./,
        "The encrypt property of  of a tablespace cannot be altered.");

($result, $stdout, $stderr) = $node->psql('postgres',
	qq[ALTER TABLESPACE regress_tblspace SET (encrypt = off);]
);
ok($stderr =~ /ERROR:  The encrypt attribute of the tablespace options cannot be modified./,
        "The encrypt property of  of a tablespace cannot be altered.");

($result, $stdout, $stderr) = $node->psql('postgres',
	qq[ALTER TABLESPACE regress_tblspace RESET (encrypt = on);]
);
ok($stderr =~ /ERROR:  The encrypt attribute of the tablespace options cannot be modified./,
        "The encrypt property of  of a tablespace cannot be altered.");

($result, $stdout, $stderr) = $node->psql('postgres',
	qq[ALTER TABLESPACE regress_tblspace RESET (encrypt = off);]
);
ok($stderr =~ /ERROR:  The encrypt attribute of the tablespace options cannot be modified./,
        "The encrypt property of  of a tablespace cannot be altered.");

($result, $stdout, $stderr) = $node->psql('postgres',
	qq[ALTER TABLESPACE regress_tblspace RESET (encrypt);]
);
ok($stderr =~ /ERROR:  The encrypt attribute of the tablespace options cannot be modified./,
        "The encrypt property of  of a tablespace cannot be altered.");

# Create objects within an encrypted tablespace
$node->safe_psql('postgres',
		qq[CREATE TABLE regress_tblspace_test_tbl (num1 bigint, num2 double precision, t text) TABLESPACE regress_tblspace;]);
$node->safe_psql('postgres',
		qq[INSERT INTO regress_tblspace_test_tbl (num1, num2, t) SELECT round(random()*100), random(), 'text' FROM generate_series(1, 10) s(i);]);
$node->safe_psql('postgres',
		qq[CREATE INDEX regress_tblspace_test_tbl_idx ON regress_tblspace_test_tbl (num1) TABLESPACE regress_tblspace;]);
$node->safe_psql('postgres',
		qq[CREATE MATERIALIZED VIEW regress_tblspace_test_tbl_mview TABLESPACE regress_tblspace AS SELECT * FROM regress_tblspace_test_tbl;]);

$result = $node->safe_psql('postgres',
		qq[SELECT reloptions from pg_class where relname = 'regress_tblspace_test_tbl';]);
like($result, qr/encrypt=on/, 'Check the encryption properties of a table');
$result = $node->safe_psql('postgres',
		qq[SELECT reloptions from pg_class where relname = 'regress_tblspace_test_tbl_idx';]);
like($result, qr/encrypt=on/, 'Check the encryption properties of an index');
$result = $node->safe_psql('postgres',
		qq[SELECT reloptions from pg_class where relname = 'regress_tblspace_test_tbl_mview';]);
like($result, qr/encrypt=on/, 'Check the encryption properties of an materialized view');

($result, $stdout, $stderr) = $node->psql('postgres',
	qq[CREATE TABLE regress_tblspace_test_tb2 (num1 bigint, num2 double precision, t text) WITH (encrypt = off) TABLESPACE regress_tblspace;]
);
ok($stderr =~ /ERROR:  Non-encrypted object cannot be created in an encrypted tablespace./,
        "Non-encrypted object cannot be created in an encrypted tablespace.");

# Modify the encrypt of the table in the encrypted tablespace
($result, $stdout, $stderr) = $node->psql('postgres',
	qq[ALTER TABLE regress_tblspace_test_tbl SET (encrypt = on);]
);
ok($stderr =~ /NOTICE:  table is located in encrypted tablespace and does not be encrypted again./,
        "Table is located in encrypted tablespace and does not be encrypted again.");

($result, $stdout, $stderr) = $node->psql('postgres',
	qq[ALTER TABLE regress_tblspace_test_tbl SET (encrypt = off);]
);
ok($stderr =~ /ERROR:  table in encrypted tablespace cannot set `encrypt=off` by ALTER TABLE command./,
        "Table in encrypted tablespace cannot set `encrypt=off` by ALTER TABLE command.");

# Move objects from encrypted to non-encrypted tablespace
($result, $stdout, $stderr) = $node->psql('postgres',
		qq[ALTER TABLE regress_tblspace_test_tbl SET tablespace pg_default;]);
like($stderr, qr/remains encrypted after being removed from its encrypted tablespace./,
		'Move objects from encrypted to non-encrypted tablespace');

# Move objects from non-encrypted to encrypted tablespace
$node->safe_psql('postgres',
	qq[ALTER TABLE regress_tblspace_test_tbl SET (encrypt = off);]);
$result = $node->safe_psql('postgres',
		qq[SELECT reloptions from pg_class where relname = 'regress_tblspace_test_tbl';]);
like($result, qr/encrypt=off/, 'Check the encryption properties of a table');

$result = $node->safe_psql('postgres',
		qq[ALTER TABLE regress_tblspace_test_tbl SET tablespace regress_tblspace;]);
is($result, '', 'Allow non-encrypted objects to be moved to encrypted tablespace');

$result = $node->safe_psql('postgres',
		qq[SELECT reloptions from pg_class where relname = 'regress_tblspace_test_tbl';]);
like($result, qr/encrypt=on/, 'Check the encryption properties of a table');

# Move objects from an encrypted tablespace to another encrypted tablespace
$result = $node->safe_psql('postgres',
		qq[ALTER TABLE regress_tblspace_test_tbl SET tablespace regress_tblspace2;]);
is($result, '', 'Allow encrypted objects to be moved to encrypted tablespace');

# When creating a database, if tablespace is encrypted, the objects
# created within this database will be encrypted
$node->safe_psql('postgres',
		qq[CREATE DATABASE regress_db TABLESPACE regress_tblspace;]);
$node->safe_psql('regress_db',
		qq[CREATE TABLE regress_tblspace_test_tbl2(num1 bigint, num2 double precision, t text);]);
$node->safe_psql('regress_db',
		qq[CREATE INDEX regress_tblspace_test_tbl_idx2 ON regress_tblspace_test_tbl2 (num1) TABLESPACE regress_tblspace;]);
$result = $node->safe_psql('regress_db',
		qq[SELECT reloptions from pg_class where relname = 'regress_tblspace_test_tbl2';]);
like($result, qr/encrypt=on/, 'All objects in a database within an encrypted tablespace are encrypted');
$result = $node->safe_psql('regress_db',
		qq[SELECT reloptions from pg_class where relname = 'regress_tblspace_test_tbl_idx2';]);
like($result, qr/encrypt=on/, 'All objects in a database within an encrypted tablespace are encrypted');

# Move database from non-encrypted to encrypted tablespace
$node->safe_psql('postgres',
		qq[CREATE DATABASE regress_db2;]);
($result, $stdout, $stderr) = $node->psql('postgres',
	qq[ALTER DATABASE regress_db2 SET TABLESPACE regress_tblspace;]);
like($stderr, qr/database located in non-encrypted tablespace cannot be moved to the encrypted tablespace/,
        "Moving a database from Non-encrypted tablespace to an encrypted one is not allowed.");

# Move database from encrypted to non-encrypted tablespace
($result, $stdout, $stderr) = $node->psql('postgres',
	qq[ALTER DATABASE regress_db SET TABLESPACE pg_default;]);
like($stderr, qr/remains encrypted after being removed from its encrypted tablespace./,
		'Moving a database from encrypted tablespace to an Non-encrypted one is allowed.');

# Done with the node
$node->stop;