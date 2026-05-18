use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More tests => 32;

# Set up nodes
my $node_primary = get_new_node('primary');
$node_primary->init();

$node_primary->append_conf(
	'postgresql.conf', qq[
track_commit_timestamp = on
ivorysql.undo_retention = 600

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

my $timepoint_0 = $node_primary->safe_psql($test_db, 'SELECT now();', extra_params => [ '-d', $connstr ]);

my $res;

$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo;], 
	extra_params => [ '-d', $connstr ]);
is($res, 60, qq[Query returned $res. Expected value 60.]);

$node_primary->safe_psql($test_db, 'update foo set val=val+1 where pk=1;', extra_params => [ '-d', $connstr ]);
my $timepoint_1 = $node_primary->safe_psql($test_db, 'SELECT now();', extra_params => [ '-d', $connstr ]);
$node_primary->safe_psql($test_db, 'update foo set val=val+1 where pk=2;', extra_params => [ '-d', $connstr ]);
my $timepoint_2 = $node_primary->safe_psql($test_db, 'SELECT now();', extra_params => [ '-d', $connstr ]);
$node_primary->safe_psql($test_db, 'update foo set val=val+1 where pk=3;', extra_params => [ '-d', $connstr ]);
my $timepoint_3 = $node_primary->safe_psql($test_db, 'SELECT now();', extra_params => [ '-d', $connstr ]);
$node_primary->safe_psql($test_db, 'update foo set val=val+1 where pk=1;', extra_params => [ '-d', $connstr ]);
my $timepoint_4 = $node_primary->safe_psql($test_db, 'SELECT now();', extra_params => [ '-d', $connstr ]);
$node_primary->safe_psql($test_db, 'update foo set val=val+1 where pk=2;', extra_params => [ '-d', $connstr ]);
my $timepoint_5 = $node_primary->safe_psql($test_db, 'SELECT now();', extra_params => [ '-d', $connstr ]);
$node_primary->safe_psql($test_db, 'update foo set val=val+1 where pk=3;', extra_params => [ '-d', $connstr ]);
my $timepoint_6 = $node_primary->safe_psql($test_db, 'SELECT now();', extra_params => [ '-d', $connstr ]);

$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of timestamp '$timepoint_5';], 
	extra_params => [ '-d', $connstr ]);
is($res, 65, qq[Query returned $res. Expected value 65.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of timestamp '$timepoint_5' where pk=3;],
	extra_params => [ '-d', $connstr ]);
is($res, 31, qq[Query returned $res. Expected value 31.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select new_foo.val - old_foo.val from foo as old_foo as of timestamp '$timepoint_5' join foo as new_foo on old_foo.pk=new_foo.pk where old_foo.pk=3;],
	extra_params => [ '-d', $connstr ]);
is($res, 1, qq[Query returned $res. Expected value 1.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of timestamp '$timepoint_4';],
	extra_params => [ '-d', $connstr ]);
is($res, 64, qq[Query returned $res. Expected value 64.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of timestamp '$timepoint_4' where pk=2;],
	extra_params => [ '-d', $connstr ]);
is($res, 21, qq[Query returned $res. Expected value 21.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of timestamp '$timepoint_3';],
	extra_params => [ '-d', $connstr ]);
is($res, 63, qq[Query returned $res. Expected value 63.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of timestamp '$timepoint_3' where pk=1;],
	extra_params => [ '-d', $connstr ]);
is($res, 11, qq[Query returned $res. Expected value 11.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of timestamp '$timepoint_2';],
	extra_params => [ '-d', $connstr ]);
is($res, 62, qq[Query returned $res. Expected value 62.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of timestamp '$timepoint_2' where pk=3;],
	extra_params => [ '-d', $connstr ]);
is($res, 30, qq[Query returned $res. Expected value 30.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of timestamp '$timepoint_1';],
	extra_params => [ '-d', $connstr ]);
is($res, 61, qq[Query returned $res. Expected value 61.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of timestamp '$timepoint_1' where pk=2;],
	extra_params => [ '-d', $connstr ]);
is($res, 20, qq[Query returned $res. Expected value 20.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of timestamp '$timepoint_0';],
	extra_params => [ '-d', $connstr ]);
is($res, 60, qq[Query returned $res. Expected value 60.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of timestamp '$timepoint_0' where pk=1;],
	extra_params => [ '-d', $connstr ]);
is($res, 10, qq[Query returned $res. Expected value 10.]);

$node_primary->safe_psql($test_db, 
	qq[vacuum foo;],
	extra_params => [ '-d', $connstr ]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of timestamp '$timepoint_6';],
	extra_params => [ '-d', $connstr ]);
is($res, 66, qq[Query returned $res. Expected value 66.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo as of timestamp '$timepoint_0';],
	extra_params => [ '-d', $connstr ]);
is($res, 60, qq[Query returned $res. Expected value 60.]);
$node_primary->safe_psql($test_db, 
	qq[drop table foo;],
	extra_params => [ '-d', $connstr ]);

# 2. test flashback table to timestamp.
$node_primary->safe_psql($test_db, qq[
		create table foo(pk int primary key, val int);
		create table foo_audit(operation text, changed_at timestamptz, row_data jsonb);
		create or replace function foo_audit_fn()
			returns trigger
			as \$\$
			BEGIN
			if TG_OP = 'INSERT' THEN
				insert into foo_audit values('INSERT', now(), to_jsonb(NEW));
				return NEW;
			elsif TG_OP = 'DELETE' THEN
				insert into foo_audit values('DELETE', now(), to_jsonb(OLD));
				return OLD;
			end if;
			return null;
			END;
			\$\$ LANGUAGE plpgsql;
		create trigger foo_audit_trigger
			after insert or DELETE
			on foo
			for each row
			execute function foo_audit_fn();
		], 
	extra_params => [ '-d', $connstr ]);
$node_primary->safe_psql($test_db, 'insert into foo values(1, 10);', extra_params => [ '-d', $connstr ]);
$timepoint_1 = $node_primary->safe_psql($test_db, 'SELECT now();', extra_params => [ '-d', $connstr ]);
$node_primary->safe_psql($test_db, 'insert into foo values(2, 20);', extra_params => [ '-d', $connstr ]);
$timepoint_2 = $node_primary->safe_psql($test_db, 'SELECT now();', extra_params => [ '-d', $connstr ]);
$node_primary->safe_psql($test_db, 'insert into foo values(3, 30);', extra_params => [ '-d', $connstr ]);
$timepoint_3 = $node_primary->safe_psql($test_db, 'SELECT now();', extra_params => [ '-d', $connstr ]);
$node_primary->safe_psql($test_db, 'delete from foo where pk = 1;', extra_params => [ '-d', $connstr ]);
$timepoint_4 = $node_primary->safe_psql($test_db, 'SELECT now();', extra_params => [ '-d', $connstr ]);
$node_primary->safe_psql($test_db, 'delete from foo where pk = 2;', extra_params => [ '-d', $connstr ]);
$timepoint_5 = $node_primary->safe_psql($test_db, 'SELECT now();', extra_params => [ '-d', $connstr ]);

$node_primary->safe_psql($test_db, 
	qq[flashback table foo to timestamp '$timepoint_5';],
	extra_params => [ '-d', $connstr ]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(pk) from foo;],
	extra_params => [ '-d', $connstr ]);
is($res, 3, qq[Query returned $res. Expected value 3.]);
$node_primary->safe_psql($test_db, 
	qq[flashback table foo to timestamp '$timepoint_4';],
	extra_params => [ '-d', $connstr ]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(pk) from foo;],
	extra_params => [ '-d', $connstr ]);
is($res, 5, qq[Query returned $res. Expected value 5.]);
$res = $node_primary->safe_psql($test_db, 
	qq[select val from foo where pk = 2;],
	extra_params => [ '-d', $connstr ]);
is($res, 20, qq[Query returned $res. Expected value 20.]);
$node_primary->safe_psql($test_db, 
	qq[flashback table foo to timestamp '$timepoint_3';],
	extra_params => [ '-d', $connstr ]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(pk) from foo;],
	extra_params => [ '-d', $connstr ]);
is($res, 6, qq[Query returned $res. Expected value 6.]);
$node_primary->safe_psql($test_db, 
	qq[flashback table foo to timestamp '$timepoint_2';],
	extra_params => [ '-d', $connstr ]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(pk) from foo;],
	extra_params => [ '-d', $connstr ]);
is($res, 3, qq[Query returned $res. Expected value 3.]);
$node_primary->safe_psql($test_db, 
	qq[flashback table foo to timestamp '$timepoint_1';],
	extra_params => [ '-d', $connstr ]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(pk) from foo;],
	extra_params => [ '-d', $connstr ]);
is($res, 1, qq[Query returned $res. Expected value 1.]);

$res = $node_primary->safe_psql($test_db, 
	qq[select count(*) from foo_audit;],
	extra_params => [ '-d', $connstr ]);
is($res, 5, qq[Audit count returned $res. Expected value 5.]);

$node_primary->safe_psql($test_db, 'truncate foo;', extra_params => [ '-d', $connstr ]);
$node_primary->safe_psql($test_db, 'insert into foo values (1,10);', extra_params => [ '-d', $connstr ]);
$timepoint_1 = $node_primary->safe_psql($test_db, 'SELECT now();', extra_params => [ '-d', $connstr ]);
$node_primary->safe_psql($test_db, 'delete from foo where pk = 1;', extra_params => [ '-d', $connstr ]);
$node_primary->safe_psql($test_db, 'insert into foo values (1,11);', extra_params => [ '-d', $connstr ]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo;],
	extra_params => [ '-d', $connstr ]);
is($res, 11, qq[Query returned $res. Expected value 11.]);
$node_primary->safe_psql($test_db, 'truncate foo_audit;', extra_params => [ '-d', $connstr ]);

$node_primary->safe_psql($test_db, 
	qq[flashback table foo to timestamp '$timepoint_1' enable triggers;],
	extra_params => [ '-d', $connstr ]);
$res = $node_primary->safe_psql($test_db, 
	qq[select sum(val) from foo;],
	extra_params => [ '-d', $connstr ]);
is($res, 10, qq[Query returned $res. Expected value 10.]);

$res = $node_primary->safe_psql($test_db, 
	qq[select operation, row_data from foo_audit order by changed_at;],
	extra_params => [ '-d', $connstr ]);
is($res,
	join("\n",
		"DELETE|{\"pk\": 1, \"val\": 11}",
		"INSERT|{\"pk\": 1, \"val\": 10}"
	),
	'verify flashback table enable triggers result');

$node_primary->safe_psql($test_db, qq[
		drop table foo;
		drop table foo_audit;
		],
	extra_params => [ '-d', $connstr ]);

$node_primary->safe_psql($test_db, qq[
	create table fb_timepoint_alter(pk int primary key, val int);
	insert into fb_timepoint_alter values (1, 10);
	],
	extra_params => [ '-d', $connstr ]);
my $timepoint_before_alter = $node_primary->safe_psql($test_db, 'SELECT clock_timestamp();', extra_params => [ '-d', $connstr ]);
$node_primary->safe_psql($test_db, qq[
	select pg_sleep(0.01);
	alter table fb_timepoint_alter add column extra int;
	],
	extra_params => [ '-d', $connstr ]);
my ($ret, $stdout, $stderr) = $node_primary->psql($test_db,
	qq[flashback table fb_timepoint_alter to timestamp '$timepoint_before_alter';],
	extra_params => [ '-d', $connstr ]);
is($ret, 3, 'flashback table before column add fails');
like($stderr,
	qr/cannot flashback relation "fb_timepoint_alter" to a time before its current catalog version/,
	'verify flashback table rejects timepoint before catalog change');
$node_primary->safe_psql($test_db, 'drop table fb_timepoint_alter;', extra_params => [ '-d', $connstr ]);

$node_primary->safe_psql($test_db, qq[
	create table fb_timepoint_undo(pk int primary key, val int);
	insert into fb_timepoint_undo values (1, 10);
	],
	extra_params => [ '-d', $connstr ]);
($ret, $stdout, $stderr) = $node_primary->psql($test_db,
	qq[flashback table fb_timepoint_undo to timestamp '2000-01-01 00:00:00+00';],
	extra_params => [ '-d', $connstr ]);
is($ret, 3, 'flashback table older than undo_retention fails');
like($stderr,
	qr/The flashback timepoint is older than current undo_retention cutoff/,
	'verify flashback table rejects timepoint older than undo_retention');
$node_primary->safe_psql($test_db, 'drop table fb_timepoint_undo;', extra_params => [ '-d', $connstr ]);

# 3. test versions between timestamp
$node_primary->safe_psql($test_db, qq[
	create table foo(pk int primary key, val int);
	insert into foo values (1,10);
	insert into foo values (2,20);
	], 
	extra_params => [ '-d', $connstr ]);

$timepoint_0 = $node_primary->safe_psql($test_db, 'SELECT now();', extra_params => [ '-d', $connstr ]);

$node_primary->safe_psql($test_db, 'update foo set val=val+1 where pk=1;', extra_params => [ '-d', $connstr ]);
$timepoint_1 = $node_primary->safe_psql($test_db, 'SELECT now();', extra_params => [ '-d', $connstr ]);
$node_primary->safe_psql($test_db, 'delete from foo where pk=2;', extra_params => [ '-d', $connstr ]);
$timepoint_2 = $node_primary->safe_psql($test_db, 'SELECT now();', extra_params => [ '-d', $connstr ]);
$node_primary->safe_psql($test_db, 'insert into foo values (2,21);', extra_params => [ '-d', $connstr ]);
$timepoint_3 = $node_primary->safe_psql($test_db, 'SELECT now();', extra_params => [ '-d', $connstr ]);

$res = $node_primary->safe_psql($test_db, 
	qq[select versions_starttime is null, versions_endtime is null, 
			versions_xid is null, versions_operation, * 
			from foo 
			versions between timestamp 
			'-infinity' 
			and '$timepoint_3'
			order by 6;], 
	extra_params => [ '-d', $connstr ]);
is($res,
	join("\n",
		"f|f|f|D|1|10",
		"f|t|f|U|1|11",
		"f|f|f|D|2|20",
		"f|t|f|I|2|21"
	),
	'verify versions between result');

$node_primary->safe_psql($test_db, 'delete from foo where pk=1;', extra_params => [ '-d', $connstr ]);
$timepoint_4 = $node_primary->safe_psql($test_db, 'SELECT now();', extra_params => [ '-d', $connstr ]);

$res = $node_primary->safe_psql($test_db,
	qq[select versions_operation, *
			from foo
			versions between timestamp
			'-infinity'
			and '$timepoint_4'
			where pk = 1
			order by versions_starttime;],
	extra_params => [ '-d', $connstr ]);
is($res,
	join("\n",
		"D|1|10",
		"D|1|11"
	),
	'verify updated row reports delete after later delete');

$node_primary->safe_psql($test_db, 
	qq[drop table foo;],
	extra_params => [ '-d', $connstr ]);

$node_primary->stop();
