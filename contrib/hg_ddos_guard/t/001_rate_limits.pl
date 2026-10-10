# Copyright (c) 2026, PostgreSQL Global Development Group

use strict;
use warnings;

use PostgresNode;
use TestLib;
use Test::More;

$TestLib::timeout_default = 30;

my $node = get_new_node('main');
$node->init;
$node->append_conf(
	'postgresql.conf',
	q{listen_addresses = '127.0.0.1'} . "\n" .
	q{shared_preload_libraries = 'hg_ddos_guard'} . "\n" .
	q{ddos.enable = on} . "\n" .
	q{ddos.login_enable = on} . "\n" .
	q{ddos.login_threshold = 3} . "\n" .
	q{ddos.login_ip_threshold = 2} . "\n" .
	q{ddos.sql_threshold = 20} . "\n" .
	q{ddos.sql_enable = off});
$node->start;
$node->safe_psql(
	'postgres',
q{CREATE EXTENSION hg_ddos_guard;
CREATE ROLE ddos_sql_user LOGIN;
CREATE ROLE ddos_observer LOGIN SUPERUSER;
	CREATE ROLE ddos_unlocker LOGIN SUPERUSER;});
$node->safe_psql('postgres', q{CREATE DATABASE ddos_other;});
my $setup_other_connstr = $node->connstr('ddos_other') .
	' user=ddos_observer connect_timeout=5';
$node->safe_psql(
	'ddos_other',
	q{CREATE EXTENSION hg_ddos_guard;},
	connstr => $setup_other_connstr);

my $missing_connstr = $node->connstr('postgres') .
	' user=ddos_missing_user connect_timeout=5';
for my $attempt (1 .. 3)
{
	my ($login_ret, $login_stdout, $login_stderr) = $node->psql(
		'postgres',
		undef,
		connstr => $missing_connstr,
		extra_params => ['-w'],
		timeout => 10);
	isnt($login_ret, 0,
		"nonexistent role authentication attempt $attempt fails");
}
my ($login_ret, $login_stdout, $login_stderr) = $node->psql(
	'postgres',
	undef,
	connstr => $missing_connstr,
	extra_params => ['-w'],
	timeout => 10);
isnt($login_ret, 0,
	'nonexistent role is rejected by the login guard');
like($login_stderr, qr/password authentication failed for user "ddos_missing_user"/,
	'nonexistent role is rejected by the login guard: matches');

my $observer_connstr = $node->connstr('postgres') .
	' user=ddos_observer connect_timeout=5';
my $login_state = $node->safe_psql(
	'postgres',
	q{SELECT userid || ',' || trigger_count || ',' || locked
        FROM hg_security.hg_ddos_login_status_view
       WHERE username = 'ddos_missing_user';},
	connstr => $observer_connstr);
is($login_state, '0,3,true',
	'login status records the nonexistent role lock');

my $second_missing_connstr = $node->connstr('postgres') .
	' user=ddos_second_missing_user connect_timeout=5';
for my $attempt (1 .. 3)
{
	my ($second_ret, $second_stdout, $second_stderr) = $node->psql(
		'postgres',
		undef,
		connstr => $second_missing_connstr,
		extra_params => ['-w'],
		timeout => 10);
	isnt($second_ret, 0,
		"second nonexistent role authentication attempt $attempt fails");
}
my ($second_ret, $second_stdout, $second_stderr) = $node->psql(
	'postgres',
	undef,
	connstr => $second_missing_connstr,
	extra_params => ['-w'],
	timeout => 10);
isnt($second_ret, 0,
	'second nonexistent role triggers the client IP lock');
like($second_stderr,
	qr/password authentication failed for user "ddos_second_missing_user"/,
	'second nonexistent role triggers the client IP lock: matches');

my $tcp_observer_connstr = 'host=127.0.0.1 port=' . $node->port .
	' dbname=postgres user=ddos_observer connect_timeout=5';
my $ip_state = $node->safe_psql(
	'postgres',
	q{SELECT locked_user_count
        FROM hg_security.hg_ddos_login_ip_status_view
       WHERE client_ip = 'local';},
	connstr => $tcp_observer_connstr);
is($ip_state, '2',
	'login IP status records two different locked users');
is(
	$node->safe_psql(
		'postgres',
		q{SELECT hg_security.hg_ddos_unlock_login_ip('local', 'only');},
		connstr => $tcp_observer_connstr),
	't',
	'login IP lock can be removed from another client address');

$node->append_conf(
	'postgresql.conf',
	q{ddos.login_enable = off} . "\n" .
	q{ddos.sql_enable = on});
$node->reload;
sleep 1;

my $disabled_login_connstr = $node->connstr('postgres') .
	' user=ddos_disabled_missing_user connect_timeout=5';
my ($disabled_login_ret, $disabled_login_stdout, $disabled_login_stderr) =
	$node->psql(
		'postgres',
		undef,
		connstr => $disabled_login_connstr,
		extra_params => ['-w'],
		timeout => 10);
isnt($disabled_login_ret, 0,
	'nonexistent role is rejected by PostgreSQL while the login guard is disabled');
like($disabled_login_stderr,
	qr/role "ddos_disabled_missing_user" does not exist/,
	'login guard does not replace the native authentication error while disabled');

my $sql_connstr = $node->connstr('postgres') .
	' user=ddos_sql_user connect_timeout=5';
my $allowed_sql = 0;
my ($ret, $stdout, $stderr);
for my $attempt (1 .. 21)
{
	($ret, $stdout, $stderr) = $node->psql(
		'postgres',
		'SELECT 1;',
		connstr => $sql_connstr,
		timeout => 10);
	last if $ret != 0;
	$allowed_sql++;
}
ok($allowed_sql > 0, 'SQL requests are allowed before the threshold');
isnt($ret, 0, 'threshold-crossing SQL request is rejected');
like($stderr, qr/SQL request temporarily rejected/,
	'SQL rejection reports the guard error');

my $sql_state = $node->safe_psql(
	'postgres',
	q{SELECT userid || ',' || request_count || ',' || locked
        FROM hg_security.hg_ddos_sql_status_view
       WHERE username = 'ddos_sql_user';},
	connstr => $observer_connstr);
like($sql_state, qr/^\d+,\d+,true$/,
	'SQL status records the locked user');

my $other_unlocker_connstr = $node->connstr('ddos_other') .
	' user=ddos_unlocker connect_timeout=5';
is(
	$node->safe_psql(
		'ddos_other',
		q{SELECT count(*) FROM hg_security.hg_ddos_sql_status_view
		   WHERE username = 'ddos_sql_user';},
		connstr => $other_unlocker_connstr),
	'0',
	'SQL status view hides records belonging to another database');
is(
	$node->safe_psql(
		'ddos_other',
		q{SELECT hg_security.hg_ddos_unlock_sql('local', 'ddos_sql_user');},
		connstr => $other_unlocker_connstr),
	'f',
	'SQL lock cannot be removed from another database');
is(
	$node->safe_psql(
		'postgres',
		q{SELECT hg_security.hg_ddos_unlock_sql('local', 'ddos_sql_user');},
		connstr => $observer_connstr),
	't',
	'SQL lock can be removed from its own database');

my $unlocker_connstr = $node->connstr('postgres') .
	' user=ddos_unlocker connect_timeout=5';
is(
	$node->safe_psql(
		'postgres',
		q{SELECT hg_security.hg_ddos_unlock_login('local', 'ddos_missing_user');},
		connstr => $unlocker_connstr),
	't',
	'login lock can be removed by another user');

$node->stop('fast');
done_testing();
