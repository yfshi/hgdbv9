
# Copyright (c) 2021-2023, PostgreSQL Global Development Group

use strict;
use warnings;

use PostgreSQL::Test::Cluster;
use PostgreSQL::Test::Utils;
use Test::More;

my $dbname1 = 'regression_src1';
my $dbname2 = 'regression_dest12';

my $node = PostgreSQL::Test::Cluster->new('main');
$node->init;
$node->start;

my $backupdir = $node->backup_dir;

$node->run_log([ 'createdb', $dbname1 ]);
$node->run_log([ 'createdb', $dbname2 ]);

$node->safe_psql(
	$dbname1,
	qq{
set ivorysql.compatible_mode to mysql;

CREATE TABLE report_scheme2(
 id float4 NOT NULL AUTO_INCREMENT COMMENT 'ID',
 name  varchar(255) NOT NULL COMMENT 'table name',
 category  varchar(100) DEFAULT '' COMMENT 'table type',
 PRIMARY KEY ( id ) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=3 ROW_FORMAT=DYNAMIC COMMENT='table config';

CREATE TABLE report_scheme3(
 id float8 NOT NULL AUTO_INCREMENT COMMENT 'ID',
 name  varchar(255) NOT NULL COMMENT 'table name',
 category  varchar(100) DEFAULT '' COMMENT 'table type',
 PRIMARY KEY ( id ) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=3 ROW_FORMAT=DYNAMIC COMMENT='table config';

CREATE TABLE report_scheme4(
 id int1 unsigned NOT NULL AUTO_INCREMENT COMMENT 'ID',
 name  varchar(255) NOT NULL COMMENT 'table name',
 category  varchar(100) DEFAULT '' COMMENT 'table type',
 PRIMARY KEY ( id ) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=3 ROW_FORMAT=DYNAMIC COMMENT='table config';

CREATE TABLE report_scheme5(
 id smallint unsigned NOT NULL AUTO_INCREMENT COMMENT 'ID',
 name  varchar(255) NOT NULL COMMENT 'table name',
 category  varchar(100) DEFAULT '' COMMENT 'table type',
 PRIMARY KEY ( id ) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=3 ROW_FORMAT=DYNAMIC COMMENT='table config';

CREATE TABLE report_scheme6(
 id uint3 NOT NULL AUTO_INCREMENT COMMENT 'ID',
 name  varchar(255) NOT NULL COMMENT 'table name',
 category  varchar(100) DEFAULT '' COMMENT 'table type',
 PRIMARY KEY ( id ) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=3 ROW_FORMAT=DYNAMIC COMMENT='table config';

CREATE TABLE report_scheme7(
 id integer unsigned NOT NULL AUTO_INCREMENT COMMENT 'ID',
 name  varchar(255) NOT NULL COMMENT 'table name',
 category  varchar(100) DEFAULT '' COMMENT 'table type',
 PRIMARY KEY ( id ) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=3 ROW_FORMAT=DYNAMIC COMMENT='table config';
insert into report_scheme2(name, category) values('aa', 'aa');

CREATE TABLE report_scheme8(
 id bigint unsigned NOT NULL AUTO_INCREMENT COMMENT 'ID',
 name  varchar(255) NOT NULL COMMENT 'table name',
 category  varchar(100) DEFAULT '' COMMENT 'table type',
 PRIMARY KEY ( id ) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=3 ROW_FORMAT=DYNAMIC COMMENT='table config';


CREATE TABLE report_scheme9(
 id int1 NOT NULL AUTO_INCREMENT COMMENT 'ID',
 name  varchar(255) NOT NULL COMMENT 'table name',
 category  varchar(100) DEFAULT '' COMMENT 'table type',
 PRIMARY KEY ( id ) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=3 ROW_FORMAT=DYNAMIC COMMENT='table config';

CREATE TABLE report_scheme10(
 id smallint NOT NULL AUTO_INCREMENT COMMENT 'ID',
 name  varchar(255) NOT NULL COMMENT 'table name',
 category  varchar(100) DEFAULT '' COMMENT 'table type',
 PRIMARY KEY ( id ) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=3 ROW_FORMAT=DYNAMIC COMMENT='table config';

CREATE TABLE report_scheme11(
 id mediumint NOT NULL AUTO_INCREMENT COMMENT 'ID',
 name  varchar(255) NOT NULL COMMENT 'table name',
 category  varchar(100) DEFAULT '' COMMENT 'table type',
 PRIMARY KEY ( id ) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=3 ROW_FORMAT=DYNAMIC COMMENT='table config';

CREATE TABLE report_scheme12(
 id mediumint unsigned NOT NULL AUTO_INCREMENT COMMENT 'ID',
 name  varchar(255) NOT NULL COMMENT 'table name',
 category  varchar(100) DEFAULT '' COMMENT 'table type',
 PRIMARY KEY ( id ) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=3 ROW_FORMAT=DYNAMIC COMMENT='table config';


CREATE TABLE report_scheme13(
 id integer NOT NULL AUTO_INCREMENT COMMENT 'ID',
 name  varchar(255) NOT NULL COMMENT 'table name',
 category  varchar(100) DEFAULT '' COMMENT 'table type',
 PRIMARY KEY ( id ) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=3 ROW_FORMAT=DYNAMIC COMMENT='table config';


CREATE TABLE report_scheme14(
 id bigint NOT NULL AUTO_INCREMENT COMMENT 'ID',
 name  varchar(255) NOT NULL COMMENT 'table name',
 category  varchar(100) DEFAULT '' COMMENT 'table type',
 PRIMARY KEY ( id ) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=3 ROW_FORMAT=DYNAMIC COMMENT='table config';

-- insert
insert into report_scheme2(name, category) values('aa', 'aa');
insert into report_scheme3(name, category) values('aa', 'aa');
insert into report_scheme4(name, category) values('aa', 'aa');
insert into report_scheme5(name, category) values('aa', 'aa');
insert into report_scheme6(name, category) values('aa', 'aa');
insert into report_scheme7(name, category) values('aa', 'aa');
insert into report_scheme8(name, category) values('aa', 'aa');
insert into report_scheme9(name, category) values('aa', 'aa');
insert into report_scheme10(name, category) values('aa', 'aa');
insert into report_scheme11(name, category) values('aa', 'aa');
insert into report_scheme12(name, category) values('aa', 'aa');
insert into report_scheme13(name, category) values('aa', 'aa');
insert into report_scheme14(name, category) values('aa', 'aa');
	});

$node->command_ok(
	[
		'pg_dump', '-Fc', '--no-sync', '-f', "$backupdir/dumpautoincrement1",
		$node->connstr($dbname1)
	],
	'autoincrement dump');

$node->command_ok(
	[
		'pg_restore', '-v',
		'-d',         $node->connstr($dbname2),
		"$backupdir/dumpautoincrement1"
	],
	'autoincrement restore');

done_testing();
