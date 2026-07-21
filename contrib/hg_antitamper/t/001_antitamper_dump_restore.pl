# Copyright (c) 2026, IvorySQL Global Development Group

use strict;
use warnings;

use PostgresNode;
use TestLib;
use Test::More tests => 49;

my $node = get_new_node('main');
$node->init;
$node->append_conf('postgresql.conf', 'se_feature.mode = on');
$node->start;

my $backup_dir = $node->backup_dir;
my $dump_file = "$backup_dir/antitamper_dump_restore.sql";
my ($result, $stdout, $stderr);

#########################################
# Prepare the source database

is(
	$node->safe_psql('postgres', 'SHOW se_feature.mode'),
	'on',
	'security feature mode is enabled');

$node->safe_psql(
	'postgres',
	q{
CREATE USER u1;
GRANT ALL ON DATABASE postgres TO u1;
});

$node->safe_psql(
	'postgres',
	q{
CREATE SCHEMA su1 WITH ANTITAMPER;
CREATE SCHEMA su2;
CREATE TABLE su1.t1(id int, str text);
INSERT INTO su1.t1 VALUES (111, 'xxx11');
CREATE TABLE su2.t2(id int, str text);
INSERT INTO su2.t2 VALUES (222, 'xxx22');
CREATE TABLE t3 (id int, str text) WITH (antitamper=true);
INSERT INTO t3 VALUES (333, 'xxx33');
GRANT USAGE ON SCHEMA su1 TO syssso;
GRANT USAGE ON SCHEMA su2 TO syssso;
},
	extra_params => [ '-U', 'u1' ]);

#########################################
# Verify antitamper operations before the dump

is(
	$node->safe_psql(
		'postgres',
		q{SELECT hg_security.antitamper_hist_check('su1', 't1')},
		extra_params => [ '-U', 'syssso' ]),
	't',
	'history check succeeds before the dump');

is(
	$node->safe_psql(
		'postgres',
		q{SELECT hg_security.antitamper_hist_repair('su1', 't1')},
		extra_params => [ '-U', 'syssso' ]),
	't',
	'history repair succeeds before the dump');

is(
	$node->safe_psql(
		'postgres',
		q{SELECT hg_security.antitamper_gchain_check('su1', 't1')},
		extra_params => [ '-U', 'syssso' ]),
	't',
	'global chain check succeeds before the dump');

is(
	$node->safe_psql(
		'postgres',
		q{SELECT hg_security.antitamper_gchain_repair('su1', 't1')},
		extra_params => [ '-U', 'syssso' ]),
	't',
	'global chain repair succeeds before the dump');

is(
	$node->safe_psql(
		'postgres',
		q{SELECT hg_security.antitamper_gchain_archive('su1', 't1')},
		extra_params => [ '-U', 'syssso' ]),
	't',
	'global chain archive succeeds before the dump');

#########################################
# Verify access before the dump

($result, $stdout, $stderr) = $node->psql(
	'postgres',
	'SELECT *, __hg_hash__ FROM su1.t1',
	extra_params => [ '-U', 'syssso' ]);
isnt($result, 0, 'security officer cannot read antitamper table data');
like(
	$stderr,
	qr/permission denied/i,
	'antitamper table data access is denied by permissions');

($result, $stdout, $stderr) = $node->psql(
	'postgres',
	'SELECT * FROM su2.t2',
	extra_params => [ '-U', 'syssso' ]);
isnt($result, 0, 'security officer cannot read ordinary table data');
like(
	$stderr,
	qr/permission denied/i,
	'ordinary table data access is denied by permissions');

($result, $stdout, $stderr) = $node->psql(
	'postgres',
	'SELECT * FROM t3',
	extra_params => [ '-U', 'syssso' ]);
isnt($result, 0,
	'security officer cannot read table-level antitamper data');
like(
	$stderr,
	qr/permission denied/i,
	'table-level antitamper data access is denied by permissions');

	is(
	$node->safe_psql(
		'postgres',
		'SELECT str FROM su1.t1',
		extra_params => [ '-U', 'u1' ]),
	'xxx11',
	'ordinary owner can read antitamper table data');

is(
	$node->safe_psql(
		'postgres',
		'SELECT str FROM su2.t2',
		extra_params => [ '-U', 'u1' ]),
	'xxx22',
	'ordinary owner can read ordinary table data');

is(
	$node->safe_psql(
		'postgres',
		'SELECT str FROM t3',
		extra_params => [ '-U', 'u1' ]),
	'xxx33',
	'ordinary owner can read table-level antitamper data');

#########################################
# Dump and restore as the ordinary owner

$node->command_ok(
	[
		'pg_dump', '--no-sync', '-U', 'u1', '-f', $dump_file,
		$node->connstr('postgres')
	],
	'ordinary owner can dump the source database');

$node->safe_psql(
	'postgres',
	q{
CREATE DATABASE db1;
GRANT ALL ON DATABASE db1 TO u1;
});

$node->command_ok(
	[
		'psql', '-X', '-U', 'u1', '-d', $node->connstr('db1'),
		'--set=ON_ERROR_STOP=1', '-f', $dump_file
	],
	'ordinary owner can restore the dump into a new database');

#########################################
# Verify antitamper operations after the restore

is(
	$node->safe_psql(
		'db1',
		q{SELECT hg_security.antitamper_hist_check('su1', 't1')},
		extra_params => [ '-U', 'syssso' ]),
	't',
	'history check succeeds after the restore');

is(
	$node->safe_psql(
		'db1',
		q{SELECT hg_security.antitamper_hist_repair('su1', 't1')},
		extra_params => [ '-U', 'syssso' ]),
	't',
	'history repair succeeds after the restore');

is(
	$node->safe_psql(
		'db1',
		q{SELECT hg_security.antitamper_gchain_check('su1', 't1')},
		extra_params => [ '-U', 'syssso' ]),
	't',
	'global chain check succeeds after the restore');

is(
	$node->safe_psql(
		'db1',
		q{SELECT hg_security.antitamper_gchain_repair('su1', 't1')},
		extra_params => [ '-U', 'syssso' ]),
	't',
	'global chain repair succeeds after the restore');

is(
	$node->safe_psql(
		'db1',
		q{SELECT hg_security.antitamper_gchain_archive('su1', 't1')},
		extra_params => [ '-U', 'syssso' ]),
	't',
	'global chain archive succeeds after the restore');

#########################################
# Verify access after the restore

($result, $stdout, $stderr) = $node->psql(
	'db1',
	'SELECT *, __hg_hash__ FROM su1.t1',
	extra_params => [ '-U', 'syssso' ]);
isnt($result, 0,
	'security officer cannot read restored antitamper table data');
like(
	$stderr,
	qr/permission denied/i,
	'restored antitamper table data access is denied by permissions');

($result, $stdout, $stderr) = $node->psql(
	'db1',
	'SELECT * FROM su2.t2',
	extra_params => [ '-U', 'syssso' ]);
isnt($result, 0,
	'security officer cannot read restored ordinary table data');
like(
	$stderr,
	qr/permission denied/i,
	'restored ordinary table data access is denied by permissions');

($result, $stdout, $stderr) = $node->psql(
	'db1',
	'SELECT * FROM t3',
	extra_params => [ '-U', 'syssso' ]);
isnt($result, 0,
	'security officer cannot read restored table-level antitamper data');
like(
	$stderr,
	qr/permission denied/i,
	'restored table-level antitamper data access is denied by permissions');

is(
	$node->safe_psql(
		'db1',
		'SELECT str FROM su1.t1',
		extra_params => [ '-U', 'u1' ]),
	'xxx11',
	'ordinary owner can read restored antitamper table data');

is(
	$node->safe_psql(
		'db1',
		'SELECT str FROM su2.t2',
		extra_params => [ '-U', 'u1' ]),
	'xxx22',
	'ordinary owner can read restored ordinary table data');

is(
	$node->safe_psql(
		'db1',
		'SELECT str FROM t3',
		extra_params => [ '-U', 'u1' ]),
	'xxx33',
	'ordinary owner can read restored table-level antitamper data');

#########################################
# Tamper with the restored relation file

my $relation_path = $node->safe_psql(
	'db1', q{SELECT pg_relation_filepath('su1.t1')});
my $relation_file = $node->data_dir . "/$relation_path";
my $t3_relation_path = $node->safe_psql(
	'db1', q{SELECT pg_relation_filepath('public.t3')});
my $t3_relation_file = $node->data_dir . "/$t3_relation_path";

$node->stop('fast');
command_ok(
	[ 'sed', '-i', 's/xxx11/yyy11/g', $relation_file ],
	'physical table data is modified while the server is stopped');
command_ok(
	[ 'sed', '-i', 's/xxx33/yyy33/g', $t3_relation_file ],
	'physical table-level antitamper data is modified while the server is stopped');
$node->start;

is(
	$node->safe_psql(
		'db1',
		q{SELECT hg_security.antitamper_hist_check('su1', 't1')},
		extra_params => [ '-U', 'syssso' ]),
	'f',
	'history check detects the physical modification');

is(
	$node->safe_psql(
		'db1',
		q{SELECT hg_security.antitamper_hist_repair('su1', 't1')},
		extra_params => [ '-U', 'syssso' ]),
	't',
	'history repair succeeds after the physical modification');

is(
	$node->safe_psql(
		'db1',
		q{SELECT hg_security.antitamper_hist_check('su1', 't1')},
		extra_params => [ '-U', 'syssso' ]),
	't',
	'history check succeeds after the repair');

is(
	$node->safe_psql(
		'db1',
		q{SELECT hg_security.antitamper_gchain_check('su1', 't1')},
		extra_params => [ '-U', 'syssso' ]),
	'f',
	'global chain check detects the physical modification');

is(
	$node->safe_psql(
		'db1',
		q{SELECT hg_security.antitamper_gchain_repair('su1', 't1')},
		extra_params => [ '-U', 'syssso' ]),
	't',
	'global chain repair succeeds after the physical modification');

is(
	$node->safe_psql(
		'db1',
		q{SELECT hg_security.antitamper_gchain_check('su1', 't1')},
		extra_params => [ '-U', 'syssso' ]),
	't',
	'global chain check succeeds after the repair');

is(
	$node->safe_psql(
		'db1',
		q{SELECT hg_security.antitamper_gchain_archive('su1', 't1')},
		extra_params => [ '-U', 'syssso' ]),
	't',
	'global chain archive succeeds after the repair');

is(
	$node->safe_psql(
		'db1',
		'SELECT str FROM su1.t1',
		extra_params => [ '-U', 'u1' ]),
	'yyy11',
	'ordinary owner sees the physically modified value');

is(
	$node->safe_psql(
		'db1',
		q{SELECT hg_security.antitamper_hist_check('public', 't3')},
		extra_params => [ '-U', 'syssso' ]),
	'f',
	'history check detects the physical modification of t3');

is(
	$node->safe_psql(
		'db1',
		q{SELECT hg_security.antitamper_hist_repair('public', 't3')},
		extra_params => [ '-U', 'syssso' ]),
	't',
	'history repair succeeds after the physical modification of t3');

is(
	$node->safe_psql(
		'db1',
		q{SELECT hg_security.antitamper_hist_check('public', 't3')},
		extra_params => [ '-U', 'syssso' ]),
	't',
	'history check succeeds after the repair of t3');

is(
	$node->safe_psql(
		'db1',
		q{SELECT hg_security.antitamper_gchain_check('public', 't3')},
		extra_params => [ '-U', 'syssso' ]),
	'f',
	'global chain check detects the physical modification of t3');

is(
	$node->safe_psql(
		'db1',
		q{SELECT hg_security.antitamper_gchain_repair('public', 't3')},
		extra_params => [ '-U', 'syssso' ]),
	't',
	'global chain repair succeeds after the physical modification of t3');

is(
	$node->safe_psql(
		'db1',
		q{SELECT hg_security.antitamper_gchain_check('public', 't3')},
		extra_params => [ '-U', 'syssso' ]),
	't',
	'global chain check succeeds after the repair of t3');

is(
	$node->safe_psql(
		'db1',
		q{SELECT hg_security.antitamper_gchain_archive('public', 't3')},
		extra_params => [ '-U', 'syssso' ]),
	't',
	'global chain archive succeeds after the repair of t3');

is(
	$node->safe_psql(
		'db1',
		'SELECT str FROM t3',
		extra_params => [ '-U', 'u1' ]),
	'yyy33',
	'ordinary owner sees the physically modified value in t3');

$node->stop('fast');
