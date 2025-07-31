use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More tests => 22;

# Set up nodes
my $node_primary = get_new_node('primary');
$node_primary->init();

$node_primary->append_conf(
	'postgresql.conf', qq[
track_commit_timestamp = on
vacuum_defer_cleanup_age = 1000
autovacuum = off

]);
$node_primary->start;

my $test_db = "test_db";
my $host = $node_primary->host;
my $ora_port = $node_primary->safe_psql('postgres', "SHOW ivorysql.port;");
my $connstr = "host=$host ";
	$connstr .= "port=$ora_port ";

$node_primary->safe_psql('postgres', "CREATE DATABASE $test_db", 
					extra_params => [ '-d', $connstr ]);

# 1. test flashback query.
$node_primary->safe_psql($test_db, qq[
	create table foo(pk int primary key, val int);
	insert into foo values (1,10);
	insert into foo values (2,20);
	insert into foo values (3,30);
	], 
	extra_params => [ '-d', $connstr ]);

my $res;

$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo;], 
	extra_params => [ '-d', $connstr ]);
is($res, 60, qq[Query returned $res. Expected value 60.]);

$node_primary->safe_psql($test_db, qq[
	select pg_sleep(1);
	update foo set val=val+1 where pk=1;
	select pg_sleep(1);
	update foo set val=val+1 where pk=2;
	select pg_sleep(1);
	update foo set val=val+1 where pk=3;
	select pg_sleep(1);
	update foo set val=val+1 where pk=1;
	select pg_sleep(1);
	update foo set val=val+1 where pk=2;
	select pg_sleep(1);
	update foo set val=val+1 where pk=3;
	], 
	extra_params => [ '-d', $connstr ]);

$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of now() - interval '1' second;], 
	extra_params => [ '-d', $connstr ]);
is($res, 65, qq[Query returned $res. Expected value 65.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of now() - interval '1' second where pk=3;],
	extra_params => [ '-d', $connstr ]);
is($res, 31, qq[Query returned $res. Expected value 31.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select new_foo.val - old_foo.val from foo as old_foo as of now() - interval '1' second join foo as new_foo on old_foo.pk=new_foo.pk where old_foo.pk=3;],
	extra_params => [ '-d', $connstr ]);
is($res, 1, qq[Query returned $res. Expected value 1.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of now() - interval '2' second;],
	extra_params => [ '-d', $connstr ]);
is($res, 64, qq[Query returned $res. Expected value 64.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of now() - interval '2' second where pk=2;],
	extra_params => [ '-d', $connstr ]);
is($res, 21, qq[Query returned $res. Expected value 21.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of now() - interval '3' second;],
	extra_params => [ '-d', $connstr ]);
is($res, 63, qq[Query returned $res. Expected value 63.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of now() - interval '3' second where pk=1;],
	extra_params => [ '-d', $connstr ]);
is($res, 11, qq[Query returned $res. Expected value 11.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of now() - interval '4' second;],
	extra_params => [ '-d', $connstr ]);
is($res, 62, qq[Query returned $res. Expected value 62.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of now() - interval '4' second where pk=3;],
	extra_params => [ '-d', $connstr ]);
is($res, 30, qq[Query returned $res. Expected value 30.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of now() - interval '5' second;],
	extra_params => [ '-d', $connstr ]);
is($res, 61, qq[Query returned $res. Expected value 61.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of now() - interval '5' second where pk=2;],
	extra_params => [ '-d', $connstr ]);
is($res, 20, qq[Query returned $res. Expected value 20.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of now() - interval '6' second;],
	extra_params => [ '-d', $connstr ]);
is($res, 60, qq[Query returned $res. Expected value 60.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of now() - interval '6' second where pk=1;],
	extra_params => [ '-d', $connstr ]);
is($res, 10, qq[Query returned $res. Expected value 10.]);

$node_primary->safe_psql($test_db, 
	qq[vacuum foo;],
	extra_params => [ '-d', $connstr ]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of now() - interval '0' second;],
	extra_params => [ '-d', $connstr ]);
is($res, 66, qq[Query returned $res. Expected value 66.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of now() - interval '6' second;],
	extra_params => [ '-d', $connstr ]);
is($res, 60, qq[Query returned $res. Expected value 60.]);
$node_primary->safe_psql($test_db, 
	qq[drop table foo;],
	extra_params => [ '-d', $connstr ]);

# 2. test flashback table to timestamp.
$node_primary->safe_psql($test_db, qq[
	create table foo(a int primary key, val int);
	insert into foo values(1, 10);
	select pg_sleep(1);
	insert into foo values(2, 20);
	select pg_sleep(1);
	insert into foo values(3, 30);
	select pg_sleep(1);
	delete from foo where a = 1;
	select pg_sleep(1);
	delete from foo where a = 2;
	select pg_sleep(1);
	],
	extra_params => [ '-d', $connstr ]);

$node_primary->safe_psql($test_db, 
	qq[flashback table foo to timestamp now() - interval '1' second;],
	extra_params => [ '-d', $connstr ]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(a) from foo;],
	extra_params => [ '-d', $connstr ]);
is($res, 3, qq[Query returned $res. Expected value 3.]);
$node_primary->safe_psql($test_db, 
	qq[flashback table foo to timestamp now() - interval '2' second;],
	extra_params => [ '-d', $connstr ]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(a) from foo;],
	extra_params => [ '-d', $connstr ]);
is($res, 5, qq[Query returned $res. Expected value 5.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select val from foo where a = 2;],
	extra_params => [ '-d', $connstr ]);
is($res, 20, qq[Query returned $res. Expected value 20.]);
$node_primary->safe_psql($test_db, 
	qq[flashback table foo to timestamp now() - interval '3' second;],
	extra_params => [ '-d', $connstr ]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(a) from foo;],
	extra_params => [ '-d', $connstr ]);
is($res, 6, qq[Query returned $res. Expected value 6.]);
$node_primary->safe_psql($test_db, 
	qq[flashback table foo to timestamp now() - interval '4' second;],
	extra_params => [ '-d', $connstr ]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(a) from foo;],
	extra_params => [ '-d', $connstr ]);
is($res, 3, qq[Query returned $res. Expected value 3.]);
$node_primary->safe_psql($test_db, 
	qq[flashback table foo to timestamp now() - interval '5' second;],
	extra_params => [ '-d', $connstr ]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(a) from foo;],
	extra_params => [ '-d', $connstr ]);
is($res, 1, qq[Query returned $res. Expected value 1.]);

$node_primary->safe_psql($test_db, 
	qq[drop table foo;],
	extra_params => [ '-d', $connstr ]);

$node_primary->stop();
