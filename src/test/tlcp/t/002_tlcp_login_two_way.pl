# Copyright (c) 2026, PostgreSQL Global Development Group

# Test TLCP mutual authentication with client certificate settings supplied
# through the libpq environment variables.

use strict;
use warnings;

use File::Path qw(rmtree);
use PostgresNode;
use TestLib;
use Test::More;

if (($ENV{with_gmssl} || '') ne 'yes')
{
	plan skip_all => 'GmSSL/TLCP not supported by this build';
}

my $SERVERHOSTADDR = '127.0.0.1';
my $SERVERHOSTCIDR = '127.0.0.1/32';
my $pass = '1234';

my $gmssl = 'gmssl';

sub generate_tlcp_files
{
	my ($node) = @_;
	my $pgdata = $node->data_dir;

	command_ok(
		[
			$gmssl, 'sm2keygen', '-pass', $pass, '-out',
			"$pgdata/rootcakey.pem"
		],
		'generate TLCP root CA key');
	command_ok(
		[
			$gmssl, 'certgen', '-C', 'CN', '-ST', 'Beijing', '-L',
			'Haidian', '-O', 'PostgreSQL', '-OU', 'TLCP', '-CN',
			'ROOTCA', '-days', '3650', '-key', "$pgdata/rootcakey.pem",
			'-pass', $pass, '-out', "$pgdata/rootcacert.pem",
			'-key_usage', 'keyCertSign', '-key_usage', 'cRLSign', '-ca'
		],
		'generate TLCP root CA certificate');
	command_ok(
		[
			$gmssl, 'sm2keygen', '-pass', $pass, '-out',
			"$pgdata/cakey.pem"
		],
		'generate TLCP intermediate CA key');
	command_ok(
		[
			$gmssl, 'reqgen', '-C', 'CN', '-ST', 'Beijing', '-L',
			'Haidian', '-O', 'PostgreSQL', '-OU', 'TLCP', '-CN',
			'Sub CA', '-key', "$pgdata/cakey.pem", '-pass', $pass,
			'-out', "$pgdata/careq.pem"
		],
		'generate TLCP intermediate CA request');
	command_ok(
		[
			$gmssl, 'reqsign', '-in', "$pgdata/careq.pem", '-days',
			'365', '-key_usage', 'keyCertSign', '-ca',
			'-path_len_constraint', '0', '-cacert',
			"$pgdata/rootcacert.pem", '-key', "$pgdata/rootcakey.pem",
			'-pass', $pass, '-out', "$pgdata/cacert.pem"
		],
		'sign TLCP intermediate CA certificate');

	command_ok(
		[
			$gmssl, 'sm2keygen', '-pass', $pass, '-out',
			"$pgdata/signkey.pem"
		],
		'generate TLCP server signing key');
	command_ok(
		[
			$gmssl, 'reqgen', '-C', 'CN', '-ST', 'Beijing', '-L',
			'Haidian', '-O', 'PostgreSQL', '-OU', 'TLCP', '-CN',
			'localhost', '-key', "$pgdata/signkey.pem", '-pass', $pass,
			'-out', "$pgdata/signreq.pem"
		],
		'generate TLCP server signing request');
	command_ok(
		[
			$gmssl, 'reqsign', '-in', "$pgdata/signreq.pem", '-days',
			'365', '-key_usage', 'digitalSignature', '-cacert',
			"$pgdata/cacert.pem", '-key', "$pgdata/cakey.pem", '-pass',
			$pass, '-out', "$pgdata/signcert.pem"
		],
		'sign TLCP server signing certificate');
	command_ok(
		[
			$gmssl, 'sm2keygen', '-pass', $pass, '-out',
			"$pgdata/enckey.pem"
		],
		'generate TLCP server encryption key');
	command_ok(
		[
			$gmssl, 'reqgen', '-C', 'CN', '-ST', 'Beijing', '-L',
			'Haidian', '-O', 'PostgreSQL', '-OU', 'TLCP', '-CN',
			'localhost', '-key', "$pgdata/enckey.pem", '-pass', $pass,
			'-out', "$pgdata/encreq.pem"
		],
		'generate TLCP server encryption request');
	command_ok(
		[
			$gmssl, 'reqsign', '-in', "$pgdata/encreq.pem", '-days',
			'365', '-key_usage', 'keyEncipherment', '-cacert',
			"$pgdata/cacert.pem", '-key', "$pgdata/cakey.pem", '-pass',
			$pass, '-out', "$pgdata/enccert.pem"
		],
		'sign TLCP server encryption certificate');

	append_to_file("$pgdata/double_certs.pem", slurp_file("$pgdata/signcert.pem"));
	append_to_file("$pgdata/double_certs.pem", slurp_file("$pgdata/enccert.pem"));
	append_to_file("$pgdata/double_certs.pem", slurp_file("$pgdata/cacert.pem"));

	command_ok(
		[
			$gmssl, 'sm2keygen', '-pass', $pass, '-out',
			"$pgdata/clientkey.pem"
		],
		'generate TLCP client key');
	command_ok(
		[
			$gmssl, 'reqgen', '-C', 'CN', '-ST', 'Beijing', '-L',
			'Haidian', '-O', 'PostgreSQL', '-OU', 'TLCP', '-CN',
			'Client', '-key', "$pgdata/clientkey.pem", '-pass', $pass,
			'-out', "$pgdata/clientreq.pem"
		],
		'generate TLCP client request');
	command_ok(
		[
			$gmssl, 'reqsign', '-in', "$pgdata/clientreq.pem", '-days',
			'365', '-key_usage', 'digitalSignature', '-cacert',
			"$pgdata/cacert.pem", '-key', "$pgdata/cakey.pem", '-pass',
			$pass, '-out', "$pgdata/clientcert.pem"
		],
		'sign TLCP client certificate');

	append_to_file("$pgdata/tlcpBackPassfile", "$pass\n$pass\n");
	append_to_file("$pgdata/tlcpClientPassfile", "$pass\n");
	chmod(0600, "$pgdata/signkey.pem", "$pgdata/enckey.pem",
		"$pgdata/clientkey.pem")
	  or die "could not chmod TLCP key files: $!";
}

sub configure_tlcp_server
{
	my ($node) = @_;

	generate_tlcp_files($node);

	$node->append_conf(
		'postgresql.conf', qq(
listen_addresses = '$SERVERHOSTADDR'
ssl = on
ssl_min_protocol_version = 'TLCP'
tlcp_cert_file = 'double_certs.pem'
tlcp_sign_key_file = 'signkey.pem'
tlcp_enc_key_file = 'enckey.pem'
tlcp_ca_cert_file = 'cacert.pem'
tlcp_pass_file = 'tlcpBackPassfile'
log_connections = on
max_wal_senders = 4
wal_level = replica
));

	my $pgdata = $node->data_dir;
	open my $hba, '>', "$pgdata/pg_hba.conf"
	  or die "could not open $pgdata/pg_hba.conf: $!";
	print $hba
	  "# TYPE  DATABASE        USER            ADDRESS                 METHOD\n";
	print $hba
	  "hostssl trustdb         tlcp_nopass     $SERVERHOSTCIDR         trust\n";
	print $hba
	  "hostssl trustdb         tlcp_pass       $SERVERHOSTCIDR         md5\n";
	print $hba
	  "hostssl postgres        tlcp_pass       $SERVERHOSTCIDR         md5\n";
	print $hba
	  "hostssl replication     tlcp_pass       $SERVERHOSTCIDR         md5\n";
	close $hba;

	$node->restart;
}

sub tlcp_connstr
{
	my (%params) = @_;
	my $user = $params{user};
	my $dbname = $params{dbname} || 'trustdb';

	return
	  "user=$user dbname=$dbname hostaddr=$SERVERHOSTADDR ssltlcp=1 sslmode=require";
}

note "setting up TLCP mutual authentication test server";
my $node = get_new_node('primary');
$node->init;
$node->start;

$node->safe_psql(
	'postgres', qq(
CREATE ROLE tlcp_nopass LOGIN;
CREATE ROLE tlcp_pass LOGIN REPLICATION;
CREATE DATABASE trustdb;
SET password_encryption = 'md5';
ALTER ROLE tlcp_pass PASSWORD 'tlcp_pass';
));

configure_tlcp_server($node);

# Supply all client-side mutual authentication material through libpq's
# environment variables.  The connection strings below intentionally omit
# the corresponding connection parameters.
my $pgdata = $node->data_dir;
local $ENV{PGTLCPCACERT} = "$pgdata/rootcacert.pem";
local $ENV{PGTLCPCERT} = "$pgdata/clientcert.pem";
local $ENV{PGTLCPKEY} = "$pgdata/clientkey.pem";
local $ENV{PGTLCPPASSFILE} = "$pgdata/tlcpClientPassfile";

my $nopass_connstr = tlcp_connstr(user => 'tlcp_nopass');
my $pass_connstr = tlcp_connstr(user => 'tlcp_pass');

delete $ENV{PGPASSWORD};
$node->connect_ok(
	$nopass_connstr,
	'TLCP mutual authentication succeeds for user without password',
	sql => 'SELECT current_user',
	expected_stdout => qr/^tlcp_nopass\r?\n?$/);

$ENV{PGPASSWORD} = 'tlcp_pass';
$node->connect_ok(
	$pass_connstr,
	'TLCP mutual authentication succeeds for user with password',
	sql => 'SELECT current_user',
	expected_stdout => qr/^tlcp_pass\r?\n?$/,
	log_like => [
		qr/connection authenticated: identity="tlcp_pass" method=md5/
	]);

my $tlcp_in_use = $node->safe_psql(
	'trustdb',
	"SELECT ssl FROM pg_stat_ssl WHERE pid = pg_backend_pid()",
	connstr => $pass_connstr);
is($tlcp_in_use, 't', 'hostssl connection uses TLCP transport');

$node->connect_fails(
	"$pass_connstr ssltlcp=0 sslmode=disable",
	'hostssl pg_hba.conf entry rejects non-TLCP login',
	expected_stderr => qr/\Qno pg_hba.conf entry\E/);

my $backup_dir = "$TestLib::tmp_check/tlcp_two_way_basebackup";
rmtree($backup_dir);
$node->command_ok(
	[
		'pg_basebackup', '-D', $backup_dir, '--checkpoint=fast',
		'--no-sync', '--wal-method=none', '--no-manifest',
		'-d', tlcp_connstr(user => 'tlcp_pass', dbname => 'postgres')
	],
	'pg_basebackup runs over mutually authenticated TLCP');

my $dump_file = "$TestLib::tmp_check/tlcp_two_way_dump.sql";
unlink $dump_file;
$node->safe_psql(
	'trustdb',
	"CREATE TABLE tlcp_two_way_dump_test (id int PRIMARY KEY, payload text);
	 INSERT INTO tlcp_two_way_dump_test VALUES (1, 'tlcp dump');",
	connstr => $pass_connstr);
$node->command_ok(
	[ 'pg_dump', '--no-sync', "--file=$dump_file", $pass_connstr ],
	'pg_dump runs over mutually authenticated TLCP');
ok(-s $dump_file, 'pg_dump produced a dump file');

done_testing();
