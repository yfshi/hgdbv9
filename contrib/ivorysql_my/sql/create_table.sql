--test LAST_INSERT_ID 
SELECT LAST_INSERT_ID(0);
SELECT LAST_INSERT_ID();
SELECT LAST_INSERT_ID(200);
--test key ===========================================================================================================
--test PRIMARY KEY
CREATE TABLE test_table (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(50),
    value INT
);
INSERT INTO test_table (name, value) VALUES ('A', 1), ('B', 2), ('C', 3);
SELECT * FROM test_table;
INSERT INTO test_table (name, value) VALUES ('D', 4), ('E', 5), ('F', 6), ('G', 7), ('H', 8);
SELECT * FROM test_table LIMIT 0, 3;
SELECT * FROM test_table LIMIT 3, 3;
drop table test_table;

CREATE TABLE test_table (
    id INT AUTO_INCREMENT,
    name VARCHAR(50),
    value INT,
    PRIMARY KEY(id)
);
INSERT INTO test_table (name, value) VALUES ('A', 1), ('B', 2), ('C', 3);
SELECT * FROM test_table;
INSERT INTO test_table (name, value) VALUES ('D', 4), ('E', 5), ('F', 6), ('G', 7), ('H', 8);
SELECT * FROM test_table LIMIT 0, 3;
SELECT * FROM test_table LIMIT 3, 3;
drop table test_table;

CREATE TABLE test_replace (
    id INT AUTO_INCREMENT PRIMARY KEY,
    uid INT UNIQUE,
    name VARCHAR(50)
);
INSERT INTO test_replace (uid, name) VALUES (1001, 'A'), (1002, 'B');
REPLACE INTO test_replace (uid, name) VALUES (1001, 'New A'), (1003, 'C');
SELECT * FROM test_replace;
drop table test_replace;

--test UNIQUE KEY
CREATE TABLE test_table (
    id INT AUTO_INCREMENT UNIQUE KEY,
    name VARCHAR(50),
    value INT
);
INSERT INTO test_table (name, value) VALUES ('A', 1), ('B', 2), ('C', 3);
SELECT * FROM test_table;
INSERT INTO test_table (name, value) VALUES ('D', 4), ('E', 5), ('F', 6), ('G', 7), ('H', 8);
SELECT * FROM test_table LIMIT 0, 3;
SELECT * FROM test_table LIMIT 3, 3;
drop table test_table;

CREATE TABLE test_table (
    id INT AUTO_INCREMENT,
    name VARCHAR(50),
    value INT,
    UNIQUE KEY(id)
);
INSERT INTO test_table (name, value) VALUES ('A', 1), ('B', 2), ('C', 3);
SELECT * FROM test_table;
INSERT INTO test_table (name, value) VALUES ('D', 4), ('E', 5), ('F', 6), ('G', 7), ('H', 8);
SELECT * FROM test_table LIMIT 0, 3;
SELECT * FROM test_table LIMIT 3, 3;
drop table test_table;

CREATE TABLE test_table (
    id INT AUTO_INCREMENT UNIQUE,
    name VARCHAR(50),
    value INT
);
INSERT INTO test_table (name, value) VALUES ('A', 1), ('B', 2), ('C', 3);
SELECT * FROM test_table;
INSERT INTO test_table (name, value) VALUES ('D', 4), ('E', 5), ('F', 6), ('G', 7), ('H', 8);
SELECT * FROM test_table LIMIT 0, 3;
SELECT * FROM test_table LIMIT 3, 3;
drop table test_table;

CREATE TABLE test_table (
    id INT AUTO_INCREMENT,
    name VARCHAR(50),
    value INT,
    UNIQUE(id)
);
INSERT INTO test_table (name, value) VALUES ('A', 1), ('B', 2), ('C', 3);
SELECT * FROM test_table;
INSERT INTO test_table (name, value) VALUES ('D', 4), ('E', 5), ('F', 6), ('G', 7), ('H', 8);
SELECT * FROM test_table LIMIT 0, 3;
SELECT * FROM test_table LIMIT 3, 3;
drop table test_table;

-- test not key
CREATE TABLE test_table (
    id INT AUTO_INCREMENT,
    name VARCHAR(50),
    value INT
);
CREATE TABLE test_table (
    id INT AUTO_INCREMENT,
    name int AUTO_INCREMENT ,
    value INT,
    UNIQUE(id),
    UNIQUE(name)
);

--test insert value > last_value ===========================================================================================================
SET SQL_MODE=NO_AUTO_VALUE_ON_ZERO;
CREATE TABLE test_table (
    id INT AUTO_INCREMENT,
    name VARCHAR(50),
    value INT,
    UNIQUE(id)
);
INSERT INTO test_table (name, value) VALUES ('A', 1), ('B', 2), ('C', 3);
SELECT * FROM test_table;
INSERT INTO test_table VALUES (0, 'D', 4);
SELECT * FROM test_table;
drop table test_table;

set sql_mode = default;
CREATE TABLE test_table (
    id INT AUTO_INCREMENT,
    name VARCHAR(50),
    value INT,
    UNIQUE(id)
);
INSERT INTO test_table (name, value) VALUES ('A', 1), ('B', 2), ('C', 3);
SELECT * FROM test_table;
INSERT INTO test_table VALUES (0, 'D', 4);
SELECT * FROM test_table;
drop table test_table;

--test LAST_INSERT_ID ===========================================================================================================
-- test LAST_INSERT_ID() LAST_INSERT_ID(EXPR)
CREATE TABLE test_table (
    id INT AUTO_INCREMENT,
    name VARCHAR(50),
    value INT,
    UNIQUE(id)
);
INSERT INTO test_table (name, value) VALUES ('A', 1), ('B', 2), ('C', 3);
SELECT * FROM test_table;
SELECT LAST_INSERT_ID();
SELECT LAST_INSERT_ID(200);
SELECT LAST_INSERT_ID();
INSERT INTO test_table VALUES (LAST_INSERT_ID(), 'D', 4);
SELECT * FROM test_table;
drop table test_table;

SELECT LAST_INSERT_ID();
CREATE TABLE test_table (
    id INT AUTO_INCREMENT,
    name VARCHAR(50),
    value INT,
    UNIQUE(id)
);
INSERT INTO test_table (name, value) VALUES ('A', 1), ('B', 2), ('C', 3);
SELECT * FROM test_table;
SELECT LAST_INSERT_ID();
SELECT * FROM test_table;
drop table test_table; 
SELECT LAST_INSERT_ID();

-- last_insert_id test
-- test NO_AUTO_VALUE_ON_ZERO
SET SQL_MODE=NO_AUTO_VALUE_ON_ZERO;
CREATE TABLE test_last_insert_id(col int8 unsigned auto_increment PRIMARY KEY) AUTO_INCREMENT = 10;
INSERT INTO test_last_insert_id VALUES(DEFAULT);
select * from test_last_insert_id;
SELECT LAST_INSERT_ID();
INSERT INTO test_last_insert_id VALUES(0), (NULL), (50), (DEFAULT);
SELECT LAST_INSERT_ID();
SELECT LAST_INSERT_ID(100 + 100);
SELECT LAST_INSERT_ID();
SELECT LAST_INSERT_ID(NULL);
SELECT LAST_INSERT_ID();
SELECT LAST_INSERT_ID(1.1);
SELECT LAST_INSERT_ID();
select * from test_last_insert_id;
DROP TABLE test_last_insert_id;
SET SQL_MODE=default;
CREATE TABLE test_last_insert_id(col int8 unsigned auto_increment PRIMARY KEY) AUTO_INCREMENT = 10;
INSERT INTO test_last_insert_id VALUES(DEFAULT);
select * from test_last_insert_id;
SELECT LAST_INSERT_ID();
INSERT INTO test_last_insert_id VALUES(0), (NULL), (50), (DEFAULT);
SELECT LAST_INSERT_ID();
SELECT LAST_INSERT_ID(100 + 100);
SELECT LAST_INSERT_ID();
SELECT LAST_INSERT_ID(NULL);
SELECT LAST_INSERT_ID();
SELECT LAST_INSERT_ID(1.1);
SELECT LAST_INSERT_ID();
select * from test_last_insert_id;
DROP TABLE test_last_insert_id;
set sql_mode = default;

--test GUC ===========================================================================================================
--TEST GUC: AUTO_INCREMENT_INCREMENT  AUTO_INCREMENT_OFFSET
CREATE TABLE test_table (
    id INT AUTO_INCREMENT,
    name VARCHAR(50),
    value INT,
    UNIQUE(id)
);
INSERT INTO test_table (name, value) VALUES ('A', 1), ('B', 2), ('C', 3);
SELECT * FROM test_table;
SET AUTO_INCREMENT_INCREMENT = 10;
SET AUTO_INCREMENT_OFFSET = 100;
INSERT INTO test_table (name, value) VALUES ('D', 4);
SELECT * FROM test_table;
INSERT INTO test_table (name, value) VALUES ('D', 4);
SELECT * FROM test_table;
INSERT INTO test_table (name, value) VALUES ('D', 4);
SELECT * FROM test_table;
SET AUTO_INCREMENT_INCREMENT = 1;
SET AUTO_INCREMENT_OFFSET = 1;
INSERT INTO test_table (name, value) VALUES ('D', 4);
SELECT * FROM test_table;
drop table test_table;

--test mysql  ===========================================================================================================
create table t1 (a int not null auto_increment,b int, primary key (a)) auto_increment=3;
insert into t1 values (1,1);
insert into t1 values (NULL,3);
insert into t1 values (NULL,4);
delete from t1 where a=4;
insert into t1 values (NULL,5),(NULL,6);
select * from t1 order by a;
delete from t1 where a=6;

replace t1 values (3,1);
ALTER TABLE t1 add c int;
replace t1 values (3,3,3);
insert into t1 values (NULL,7,7);
update t1 set a=8,b=b+1,c=c+1 where a=7;
insert into t1 values (NULL,9,9);
select * from t1 order by a;
drop table t1;

create table t1 (
  skey tinyint unsigned NOT NULL auto_increment PRIMARY KEY,
  sval char(20)
);
insert into t1 values (NULL, 'hello');
insert into t1 values (NULL, 'hey');
select * from t1 order by skey;
drop table t1;

--
-- Test auto_increment on sub key
--
create table t1 (a char(10) not null, b int not null auto_increment, primary key(b));
insert into t1 values ('a',1),('b',2),('a',2),('c',1);
insert into t1 values ('a',NULL),('b',NULL),('c',NULL),('e',NULL);
insert into t1 (a) values ('a'),('b'),('c'),('d');
insert into t1 (a) values ('k'),('d');
insert into t1 (a) values ('a');
insert into t1 values ('d',last_insert_id());
select * from t1 order by a;
drop table t1;

create table t1 (ordid int(8) not null auto_increment, ord  varchar(50) not null, primary key (ordid)); 
insert into t1 (ordid,ord) values (NULL,'sdj'),(NULL,'sdj');
select * from t1 order by ordid;
drop table t1;

create table t1 (ordid int(8) not null auto_increment, ord  varchar(50) not null, primary key (ordid));
insert into t1 values (NULL,'sdj'),(NULL,'sdj'),(NULL,'abc'),(NULL,'abc'),(NULL,'zzz'),(NULL,'sdj'),(NULL,'abc');
select * from t1 order by ordid;
drop table t1;

--
-- Test of auto_increment columns when they are set to 0
--

create table t1 (a int not null primary key auto_increment);
insert into t1 values (0);
update t1 set a=0;
select * from t1 order by a;
drop table t1;

--
-- Test negative values
--
create table t1 (a int not null auto_increment primary key);
insert into t1 values (NULL);
insert into t1 values (-1);
select last_insert_id();
insert into t1 values (NULL);
select * from t1 order by a;
drop table t1;

create table t1 (a int not null auto_increment primary key);
insert into t1 values (NULL);
insert into t1 values (-1);
select last_insert_id();
insert into t1 values (NULL);
select * from t1 order by a;
drop table t1;
--
-- last_insert_id() madness
--
SET sql_mode = 'NO_ENGINE_SUBSTITUTION';
create table t1 (i tinyint unsigned not null auto_increment primary key);
insert into t1 set i = 254;
insert into t1 set i = null;
select last_insert_id();
--error ER_DUP_ENTRY
insert into t1 set i = 254;
select last_insert_id();
--error ER_DUP_ENTRY
insert into t1 set i = null;
select last_insert_id();
drop table t1;

create table t1 (i tinyint unsigned not null auto_increment, primary key (i));
insert into t1 set i = 254;
insert into t1 set i = null;
select last_insert_id();
insert into t1 set i = null;
select last_insert_id();
drop table t1;

create table t1 (i tinyint unsigned not null auto_increment primary key, b int, unique (b));
insert into t1 values (NULL, 10);
select last_insert_id();
insert into t1 values (NULL, 15);
select last_insert_id();
--error ER_DUP_ENTRY
insert into t1 values (NULL, 10);
select last_insert_id();
drop table t1;

-- test
SET sql_mode = default;

create table t1(a int auto_increment,b int null,primary key(a));
SET SQL_MODE=NO_AUTO_VALUE_ON_ZERO;
insert into t1(a,b)values(NULL,1);
insert into t1(a,b)values(200,2);
insert into t1(a,b)values(0,3);
insert into t1(b)values(4);
insert into t1(b)values(5);
insert into t1(b)values(6);
insert into t1(b)values(7);
select * from t1 order by b;
create table t2 (a int);
insert t2 values (1),(2);
alter table t2 add b int auto_increment primary key;
select * from t2;
drop table t2;
delete from t1 where a=0;
update t1 set a=0 where b=5;
select * from t1 order by b;
delete from t1 where a=0;
update t1 set a=NULL where b=6;
update t1 set a=300 where b=7;
SET SQL_MODE=default;
insert into t1(a,b)values(NULL,8);
insert into t1(a,b)values(400,9);
insert into t1(a,b)values(0,10);
insert into t1(b)values(11);
insert into t1(b)values(12);
insert into t1(b)values(13);
insert into t1(b)values(14);
select * from t1 order by b;
delete from t1 where a=0;
update t1 set a=0 where b=12;
select * from t1 order by b;
delete from t1 where a=0;
update t1 set a=NULL where b=13;
update t1 set a=500 where b=14;
select * from t1 order by b;
drop table t1;

--
-- Test of behavior of ALTER TABLE when coulmn containing NULL or zeroes is
-- converted to AUTO_INCREMENT column
--
create table t1 (a bigint);
insert into t1 values (1), (2), (3), (NULL), (NULL);
alter table t1 alter a type bigint not null auto_increment primary key; 
select * from t1 order by a;
drop table t1;

create table t1 (a bigint);
insert into t1 values (1), (2), (3), (0), (0);
alter table t1 alter a type bigint not null auto_increment primary key; 
select * from t1 order by a;
drop table t1;

-- We still should be able to preserve zero in NO_AUTO_VALUE_ON_ZERO mode
create table t1 (a bigint);
insert into t1 values (0), (1), (2), (3);
set sql_mode=NO_AUTO_VALUE_ON_ZERO;
alter table t1 alter a type bigint not null auto_increment primary key; 
set sql_mode= default;
select * from t1 order by a;
drop table t1;

-- It also sensible to preserve zeroes if we are converting auto_increment
-- column to auto_increment column (or not touching it at all, which is more
-- common case probably)
create table t1 (a int auto_increment primary key , b int null);
set sql_mode=NO_AUTO_VALUE_ON_ZERO;
insert into t1 values (0,1),(1,2),(2,3);
select * from t1 order by a;
set sql_mode= default;
alter table t1 alter b type varchar(255);
insert into t1 values (0,4);
select * from t1 order by a;
drop table t1;

--
-- LAST_INSERT_ID() within a statement
--
create table t1(a int not null auto_increment primary key);              
create table t2(a int not null auto_increment primary key, t1a int);     
insert into t1 values(NULL);
insert into t2 values (NULL, LAST_INSERT_ID()), (NULL, LAST_INSERT_ID());
insert into t1 values (NULL);
insert into t2 values (NULL, LAST_INSERT_ID()), (NULL, LAST_INSERT_ID()),
(NULL, LAST_INSERT_ID());
insert into t1 values (NULL);
insert into t2 values (NULL, LAST_INSERT_ID()), (NULL, LAST_INSERT_ID()),
(NULL, LAST_INSERT_ID()), (NULL, LAST_INSERT_ID());
select * from t2;
drop table t1, t2;

--
-- Multi-row REPLACE fails on a duplicate key error
-- 
CREATE TABLE t1 ( a int(11) NOT NULL auto_increment, b int(11) default NULL,PRIMARY KEY  (a),UNIQUE KEY (b));
insert into t1 (b) values (1);
replace into t1 (b) values (2), (1), (3);
select * from t1 order by a;
truncate table t1;
insert into t1 (b) values (1);
replace into t1 (b) values (2);
replace into t1 (b) values (1);
replace into t1 (b) values (3);
select * from t1 order by a;
drop table t1;

create table t1 (rowid int not null auto_increment, val int not null,primary
key (rowid), unique(val));
replace into t1 (val) values ('1'),('2');
replace into t1 (val) values ('1'),('2');
--error ER_DUP_ENTRY
insert into t1 (val) values ('1'),('2');
select * from t1 order by rowid;
drop table t1;

--
-- Test that update changes internal auto-increment value
--
create table t1 (a int not null auto_increment primary key, val int);
insert into t1 (val) values (1);
insert into t1 (val) values (2);
update t1 set a=2 where a=1;
update t1 set a=20 where a=2;
insert into t1 (val) values (11);
insert into t1 (val) values (11);
insert into t1 (val) values (11);
select * from t1 order by a;
drop table t1;

--
-- Test key duplications with auto-increment in ALTER TABLE
--
CREATE TABLE t1 (t1 INT(10) PRIMARY KEY, t2 INT(10));
INSERT INTO t1 VALUES(0, 0);
INSERT INTO t1 VALUES(1, 1);
--error ER_DUP_ENTRY
ALTER TABLE t1 alter t1 type INT(10) auto_increment;
DROP TABLE t1;

-- Test of REPLACE when it does INSERT+DELETE and not UPDATE:
-- see if it sets LAST_INSERT_ID() ok
create table t1 (a int primary key auto_increment, b int, c int, d timestamp default current_timestamp, unique(b),unique(c));
insert into t1 values(null,1,1,now());
insert into t1 values(null,0,0,null);
-- this will delete two rows
replace into t1 values(null,1,0,null);
select last_insert_id();
drop table t1;

-- Test of REPLACE when it does a INSERT+DELETE for all the conflicting rows
-- (i.e.) when there are three rows conflicting in unique key columns with
-- a row that is being inserted, all the three rows will be deleted and then
-- the new rows will be inserted.
create table t1 (a int primary key auto_increment, b int, c int, e int, d timestamp default current_timestamp, unique(b),unique(c),unique(e));
insert into t1 values(null,1,1,1,now());
insert into t1 values(null,0,0,0,null);
replace into t1 values(null,1,0,2,null);
select last_insert_id();
drop table t1;

--test ALTER TABLE  ===========================================================================================================
--TEST ALTER TABLE AUTO_INCREMENT value
CREATE TABLE test_table (
    id INT AUTO_INCREMENT,
    name VARCHAR(50),
    value INT,
    UNIQUE(id)
);
INSERT INTO test_table (name, value) VALUES ('A', 1), ('B', 2), ('C', 3);
SELECT * FROM test_table;
ALTER TABLE test_table AUTO_INCREMENT=100;
INSERT INTO test_table (name, value) VALUES ('D', 4);
SELECT * FROM test_table;
ALTER TABLE test_table AUTO_INCREMENT=-1;
INSERT INTO test_table (name, value) VALUES ('D', 4);
SELECT * FROM test_table;
ALTER TABLE test_table AUTO_INCREMENT=50;
INSERT INTO test_table (name, value) VALUES ('D', 4);
SELECT * FROM test_table;
ALTER TABLE test_table AUTO_INCREMENT=500.5;
INSERT INTO test_table (name, value) VALUES ('D', 4);
SELECT * FROM test_table;
drop table test_table;

--TEST ALTER TABLE add column
CREATE TABLE test_alter_autoinc(
    a int,
    b varchar(32),
    primary KEY (a,b)
);
ALTER TABLE test_alter_autoinc alter a type int AUTO_INCREMENT;
\d test_alter_autoinc
DROP TABLE test_alter_autoinc;

CREATE TABLE test_alter_autoinc(col int unsigned);
INSERT INTO test_alter_autoinc VALUES(1);
INSERT INTO test_alter_autoinc VALUES(2);

ALTER TABLE test_alter_autoinc ADD COLUMN id int unsigned AUTO_INCREMENT primary key;
SELECT id, col FROM test_alter_autoinc ORDER BY 1, 2;
insert into test_alter_autoinc(col) values (3),(4),(5);
SELECT id, col FROM test_alter_autoinc ORDER BY 1, 2;
insert into test_alter_autoinc values (1, 1);
insert into test_alter_autoinc values (6, 0);
select last_insert_id();
ALTER TABLE test_alter_autoinc AUTO_INCREMENT = 1000;
INSERT INTO test_alter_autoinc VALUES(3,DEFAULT);
SELECT id, col FROM test_alter_autoinc ORDER BY 1, 2;
TRUNCATE TABLE test_alter_autoinc;
INSERT INTO test_alter_autoinc VALUES(4,DEFAULT);
SELECT id, col FROM test_alter_autoinc ORDER BY 1, 2;
ALTER TABLE test_alter_autoinc DROP COLUMN id;
SELECT col FROM test_alter_autoinc ORDER BY 1;

-- err gram
ALTER TABLE test_alter_autoinc ADD id int unsigned AUTO_INCREMENT, ADD primary key(id), AUTO_INCREMENT = 100;
DROP TABLE test_alter_autoinc;
--test alter table add AUTO_INCREMENT NULL UNIQUE
CREATE TABLE test_alter_autoinc(col int1 unsigned);
INSERT INTO test_alter_autoinc VALUES(1);
INSERT INTO test_alter_autoinc VALUES(2);
ALTER TABLE test_alter_autoinc ADD COLUMN id int unsigned AUTO_INCREMENT NULL UNIQUE;
INSERT INTO test_alter_autoinc VALUES(3,0);
SELECT id, col FROM test_alter_autoinc ORDER BY 1, 2;
DROP TABLE test_alter_autoinc;

--test temp table  ===========================================================================================================
-- auto_increment in local temp table PRIMARY KEY
CREATE TEMPORARY TABLE ltemp_single_autoinc(col int unsigned auto_increment PRIMARY KEY);
INSERT INTO ltemp_single_autoinc VALUES(NULL);
SELECT LAST_INSERT_ID();
INSERT INTO ltemp_single_autoinc VALUES(1 - 1);
SELECT LAST_INSERT_ID();
INSERT INTO ltemp_single_autoinc VALUES(100);
SELECT LAST_INSERT_ID();
INSERT INTO ltemp_single_autoinc VALUES(DEFAULT);
SELECT LAST_INSERT_ID();
ALTER TABLE ltemp_single_autoinc AUTO_INCREMENT = 1000;
SELECT LAST_INSERT_ID();
INSERT INTO ltemp_single_autoinc VALUES(DEFAULT);
SELECT LAST_INSERT_ID();
SELECT col FROM ltemp_single_autoinc ORDER BY 1;
INSERT INTO ltemp_single_autoinc VALUES(500);
SELECT LAST_INSERT_ID();
INSERT INTO ltemp_single_autoinc VALUES(1);
SELECT LAST_INSERT_ID();
INSERT INTO ltemp_single_autoinc VALUES(2100);
INSERT INTO ltemp_single_autoinc VALUES(NULL);
INSERT INTO ltemp_single_autoinc VALUES(2000);
INSERT INTO ltemp_single_autoinc VALUES(DEFAULT);
INSERT INTO ltemp_single_autoinc VALUES(2200);
INSERT INTO ltemp_single_autoinc VALUES(NULL);
SELECT LAST_INSERT_ID();
SELECT col FROM ltemp_single_autoinc ORDER BY 1;
UPDATE ltemp_single_autoinc SET col=NULL WHERE col=2201;
SELECT col FROM ltemp_single_autoinc ORDER BY 1;
UPDATE ltemp_single_autoinc SET col=0 WHERE col=2200;
SELECT col FROM ltemp_single_autoinc ORDER BY 1;
UPDATE ltemp_single_autoinc SET col=DEFAULT WHERE col=2101;
UPDATE ltemp_single_autoinc SET col=3000 WHERE col=1;
SELECT LAST_INSERT_ID();
INSERT INTO ltemp_single_autoinc VALUES(DEFAULT);
SELECT LAST_INSERT_ID();
SELECT col FROM ltemp_single_autoinc ORDER BY 1;
SELECT LAST_INSERT_ID();
SELECT col FROM ltemp_single_autoinc ORDER BY 1;
TRUNCATE TABLE ltemp_single_autoinc;
INSERT INTO ltemp_single_autoinc VALUES(DEFAULT);
SELECT LAST_INSERT_ID();
SELECT col FROM ltemp_single_autoinc ORDER BY 1;
DROP TABLE ltemp_single_autoinc;

-- auto_increment in local temp table NULL UNIQUE
CREATE TEMPORARY TABLE single_autoinc_uk(col int unsigned auto_increment NULL UNIQUE) AUTO_INCREMENT = 10;
INSERT INTO single_autoinc_uk VALUES(NULL);
SELECT LAST_INSERT_ID();
SELECT col FROM single_autoinc_uk ORDER BY 1;
INSERT INTO single_autoinc_uk VALUES(1 - 1);
SELECT LAST_INSERT_ID();
SELECT col FROM single_autoinc_uk ORDER BY 1;
INSERT INTO single_autoinc_uk VALUES(100);
SELECT col FROM single_autoinc_uk ORDER BY 1;
INSERT INTO single_autoinc_uk VALUES(DEFAULT);
SELECT LAST_INSERT_ID();
SELECT col FROM single_autoinc_uk ORDER BY 1;
ALTER TABLE single_autoinc_uk AUTO_INCREMENT = 1000;
INSERT INTO single_autoinc_uk VALUES(0);
SELECT LAST_INSERT_ID();
SELECT col FROM single_autoinc_uk ORDER BY 1;
UPDATE single_autoinc_uk SET col=NULL WHERE col=11;
SELECT col FROM single_autoinc_uk ORDER BY 1;
UPDATE single_autoinc_uk SET col=0 WHERE col=100;
SELECT col FROM single_autoinc_uk ORDER BY 1;
UPDATE single_autoinc_uk SET col=DEFAULT WHERE col=1000;
SELECT col FROM single_autoinc_uk ORDER BY 1;
UPDATE single_autoinc_uk SET col=3000 WHERE col=0;
INSERT INTO single_autoinc_uk VALUES(0);
SELECT LAST_INSERT_ID();
SELECT col FROM single_autoinc_uk ORDER BY 1;
DROP TABLE single_autoinc_uk;

CREATE TABLE ltemp_single_autoinc(col int unsigned auto_increment PRIMARY KEY);
INSERT INTO ltemp_single_autoinc VALUES(NULL);
INSERT INTO ltemp_single_autoinc VALUES(NULL);
INSERT INTO ltemp_single_autoinc VALUES(2100);
UPDATE ltemp_single_autoinc SET col=0 WHERE col=2100;
INSERT INTO ltemp_single_autoinc VALUES(2101);
UPDATE ltemp_single_autoinc SET col=DEFAULT WHERE col=2101;
SELECT col FROM ltemp_single_autoinc ORDER BY 1;
drop table  ltemp_single_autoinc;

-----------------------------------------------------------------------------------------------------
SET SQL_MODE=default;
CREATE TEMPORARY TABLE single_autoinc_uk(col int unsigned auto_increment UNIQUE) AUTO_INCREMENT = 10;
INSERT INTO single_autoinc_uk VALUES(NULL);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(DEFAULT);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(0);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES();
SELECT LAST_INSERT_ID();
SELECT col FROM single_autoinc_uk ORDER BY 1;
drop TABLE single_autoinc_uk;

CREATE TABLE single_autoinc_uk(col int unsigned auto_increment UNIQUE) AUTO_INCREMENT = 10;
INSERT INTO single_autoinc_uk VALUES(NULL);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(DEFAULT);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(0);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES();
SELECT LAST_INSERT_ID();
SELECT col FROM single_autoinc_uk ORDER BY 1;
drop TABLE single_autoinc_uk;

CREATE TEMPORARY TABLE single_autoinc_uk(col int unsigned auto_increment NOT NULL UNIQUE) AUTO_INCREMENT = 10;
INSERT INTO single_autoinc_uk VALUES(NULL);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(DEFAULT);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(0);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES();
SELECT LAST_INSERT_ID();
SELECT col FROM single_autoinc_uk ORDER BY 1;
drop TABLE single_autoinc_uk;

CREATE TABLE single_autoinc_uk(col int unsigned auto_increment  NOT NULL UNIQUE) AUTO_INCREMENT = 10;
INSERT INTO single_autoinc_uk VALUES(NULL);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(DEFAULT);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(0);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES();
SELECT LAST_INSERT_ID();
SELECT col FROM single_autoinc_uk ORDER BY 1;
drop TABLE single_autoinc_uk;

CREATE TEMPORARY TABLE single_autoinc_uk(col int unsigned auto_increment NULL UNIQUE) AUTO_INCREMENT = 10;
INSERT INTO single_autoinc_uk VALUES(NULL);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(DEFAULT);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(0);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES();
SELECT LAST_INSERT_ID();
SELECT col FROM single_autoinc_uk ORDER BY 1;
drop TABLE single_autoinc_uk;

CREATE TABLE single_autoinc_uk(col int unsigned auto_increment NULL UNIQUE) AUTO_INCREMENT = 10;
INSERT INTO single_autoinc_uk VALUES(NULL);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(DEFAULT);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(0);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES();
SELECT LAST_INSERT_ID();
SELECT col FROM single_autoinc_uk ORDER BY 1;
drop TABLE single_autoinc_uk;

CREATE TEMPORARY TABLE single_autoinc_uk(col int unsigned auto_increment PRIMARY KEY ) AUTO_INCREMENT = 10;
INSERT INTO single_autoinc_uk VALUES(NULL);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(DEFAULT);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(0);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES();
SELECT LAST_INSERT_ID();
SELECT col FROM single_autoinc_uk ORDER BY 1;
drop TABLE single_autoinc_uk;

CREATE TABLE single_autoinc_uk(col int unsigned auto_increment PRIMARY KEY ) AUTO_INCREMENT = 10;
INSERT INTO single_autoinc_uk VALUES(NULL);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(DEFAULT);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(0);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES();
SELECT LAST_INSERT_ID();
SELECT col FROM single_autoinc_uk ORDER BY 1;
drop TABLE single_autoinc_uk;

SET SQL_MODE=NO_AUTO_VALUE_ON_ZERO;
CREATE TEMPORARY TABLE single_autoinc_uk(col int unsigned auto_increment UNIQUE) AUTO_INCREMENT = 10;
INSERT INTO single_autoinc_uk VALUES(NULL);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(DEFAULT);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(0);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES();
SELECT LAST_INSERT_ID();
SELECT col FROM single_autoinc_uk ORDER BY 1;
drop TABLE single_autoinc_uk;

CREATE TABLE single_autoinc_uk(col int unsigned auto_increment UNIQUE) AUTO_INCREMENT = 10;
INSERT INTO single_autoinc_uk VALUES(NULL);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(DEFAULT);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(0);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES();
SELECT LAST_INSERT_ID();
SELECT col FROM single_autoinc_uk ORDER BY 1;
drop TABLE single_autoinc_uk;

CREATE TEMPORARY TABLE single_autoinc_uk(col int unsigned auto_increment NOT NULL UNIQUE) AUTO_INCREMENT = 10;
INSERT INTO single_autoinc_uk VALUES(NULL);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(DEFAULT);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(0);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES();
SELECT LAST_INSERT_ID();
SELECT col FROM single_autoinc_uk ORDER BY 1;
drop TABLE single_autoinc_uk;

CREATE TABLE single_autoinc_uk(col int unsigned auto_increment  NOT NULL UNIQUE) AUTO_INCREMENT = 10;
INSERT INTO single_autoinc_uk VALUES(NULL);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(DEFAULT);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(0);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES();
SELECT LAST_INSERT_ID();
SELECT col FROM single_autoinc_uk ORDER BY 1;
drop TABLE single_autoinc_uk;

CREATE TEMPORARY TABLE single_autoinc_uk(col int unsigned auto_increment NULL UNIQUE) AUTO_INCREMENT = 10;
INSERT INTO single_autoinc_uk VALUES(NULL);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(DEFAULT);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(0);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES();
SELECT LAST_INSERT_ID();
SELECT col FROM single_autoinc_uk ORDER BY 1;
drop TABLE single_autoinc_uk;

CREATE TABLE single_autoinc_uk(col int unsigned auto_increment NULL UNIQUE) AUTO_INCREMENT = 10;
INSERT INTO single_autoinc_uk VALUES(NULL);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(DEFAULT);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(0);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES();
SELECT LAST_INSERT_ID();
SELECT col FROM single_autoinc_uk ORDER BY 1;
drop TABLE single_autoinc_uk;

CREATE TEMPORARY TABLE single_autoinc_uk(col int unsigned auto_increment PRIMARY KEY ) AUTO_INCREMENT = 10;
INSERT INTO single_autoinc_uk VALUES(NULL);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(DEFAULT);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(0);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES();
SELECT LAST_INSERT_ID();
SELECT col FROM single_autoinc_uk ORDER BY 1;
drop TABLE single_autoinc_uk;

CREATE TABLE single_autoinc_uk(col int unsigned auto_increment PRIMARY KEY ) AUTO_INCREMENT = 10;
INSERT INTO single_autoinc_uk VALUES(NULL);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(DEFAULT);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES(0);
SELECT LAST_INSERT_ID();
INSERT INTO single_autoinc_uk VALUES();
SELECT LAST_INSERT_ID();
SELECT col FROM single_autoinc_uk ORDER BY 1;
drop TABLE single_autoinc_uk;
SET SQL_MODE=default;

-- update test
SET SQL_MODE=NO_AUTO_VALUE_ON_ZERO;
CREATE TABLE ltemp_single_autoinc(col int unsigned auto_increment NULL UNIQUE);
INSERT INTO ltemp_single_autoinc VALUES(2100);
INSERT INTO ltemp_single_autoinc VALUES(2101);
INSERT INTO ltemp_single_autoinc VALUES(2102);
UPDATE ltemp_single_autoinc SET col=0 WHERE col=2100;
SELECT col FROM ltemp_single_autoinc ORDER BY 1;
UPDATE ltemp_single_autoinc SET col=NULL WHERE col=2101;
SELECT col FROM ltemp_single_autoinc ORDER BY 1;
UPDATE ltemp_single_autoinc SET col=DEFAULT WHERE col=2102;
SELECT col FROM ltemp_single_autoinc ORDER BY 1;
DROP TABLE ltemp_single_autoinc;

CREATE TABLE ltemp_single_autoinc(col int unsigned auto_increment not NULL UNIQUE);
INSERT INTO ltemp_single_autoinc VALUES(2100);
INSERT INTO ltemp_single_autoinc VALUES(2101);
INSERT INTO ltemp_single_autoinc VALUES(2102);
UPDATE ltemp_single_autoinc SET col=0 WHERE col=2100;
SELECT col FROM ltemp_single_autoinc ORDER BY 1;
UPDATE ltemp_single_autoinc SET col=NULL WHERE col=2101;
SELECT col FROM ltemp_single_autoinc ORDER BY 1;
UPDATE ltemp_single_autoinc SET col=DEFAULT WHERE col=2102;
SELECT col FROM ltemp_single_autoinc ORDER BY 1;
DROP TABLE ltemp_single_autoinc;

SET SQL_MODE=default;
CREATE TABLE ltemp_single_autoinc(col int unsigned auto_increment NULL UNIQUE);
INSERT INTO ltemp_single_autoinc VALUES(2100);
INSERT INTO ltemp_single_autoinc VALUES(2101);
INSERT INTO ltemp_single_autoinc VALUES(2102);
UPDATE ltemp_single_autoinc SET col=0 WHERE col=2100;
SELECT col FROM ltemp_single_autoinc ORDER BY 1;
UPDATE ltemp_single_autoinc SET col=NULL WHERE col=2101;
SELECT col FROM ltemp_single_autoinc ORDER BY 1;
UPDATE ltemp_single_autoinc SET col=DEFAULT WHERE col=2102;
SELECT col FROM ltemp_single_autoinc ORDER BY 1;
DROP TABLE ltemp_single_autoinc;

CREATE TABLE ltemp_single_autoinc(col int unsigned auto_increment not NULL UNIQUE);
INSERT INTO ltemp_single_autoinc VALUES(2100);
INSERT INTO ltemp_single_autoinc VALUES(2101);
INSERT INTO ltemp_single_autoinc VALUES(2102);
UPDATE ltemp_single_autoinc SET col=0 WHERE col=2100;
SELECT col FROM ltemp_single_autoinc ORDER BY 1;
UPDATE ltemp_single_autoinc SET col=NULL WHERE col=2101;
SELECT col FROM ltemp_single_autoinc ORDER BY 1;
UPDATE ltemp_single_autoinc SET col=DEFAULT WHERE col=2102;
SELECT col FROM ltemp_single_autoinc ORDER BY 1;
DROP TABLE ltemp_single_autoinc;

--test type  auto_increment ===========================================================================================================
create table auto_inc_int1 (
    col1 int1 auto_increment primary key,
    col2 int
);
insert into auto_inc_int1 values(127, 1);
insert into auto_inc_int1(col2) values(2);
insert into auto_inc_int1(col2) values(3);
select * from auto_inc_int1 order by 1,2;
delete from auto_inc_int1;
insert into auto_inc_int1(col2) values(4);
select * from auto_inc_int1 order by 1,2;
drop table auto_inc_int1;

create table auto_inc_uint1 (
    col1 int1 unsigned auto_increment primary key,
    col2 int
);
insert into auto_inc_uint1 values(255, 1);
insert into auto_inc_uint1(col2) values(2);
insert into auto_inc_uint1(col2) values(3);
select * from auto_inc_uint1 order by 1,2;
delete from auto_inc_uint1;
insert into auto_inc_uint1(col2) values(4);
select * from auto_inc_uint1 order by 1,2;
drop table auto_inc_uint1;

create table auto_inc_int2 (
    col1 int2 auto_increment primary key,
    col2 int
);
insert into auto_inc_int2 values(32767, 1);
insert into auto_inc_int2(col2) values(2);
insert into auto_inc_int2(col2) values(3);
select * from auto_inc_int2 order by 1,2;
delete from auto_inc_int2;
insert into auto_inc_int2(col2) values(4);
select * from auto_inc_int2 order by 1,2;
drop table auto_inc_int2;

create table auto_inc_uint2 (
    col1 int2 unsigned auto_increment primary key,
    col2 int
);
insert into auto_inc_uint2 values(65535, 1);
insert into auto_inc_uint2(col2) values(2);
insert into auto_inc_uint2(col2) values(3);
select * from auto_inc_uint2 order by 1,2;
delete from auto_inc_uint2;
insert into auto_inc_uint2(col2) values(4);
select * from auto_inc_uint2 order by 1,2;
drop table auto_inc_uint2;

create table auto_inc_int4 (
    col1 int4 auto_increment primary key,
    col2 int
);
insert into auto_inc_int4 values(2147483647, 1);
insert into auto_inc_int4(col2) values(2);
insert into auto_inc_int4(col2) values(3);
select * from auto_inc_int4 order by 1,2;
delete from auto_inc_int4;
insert into auto_inc_int4(col2) values(4);
select * from auto_inc_int4 order by 1,2;
drop table auto_inc_int4;

create table auto_inc_uint4 (
    col1 int4 unsigned auto_increment primary key,
    col2 int
);
insert into auto_inc_uint4 values(4294967295, 1);
insert into auto_inc_uint4(col2) values(2);
insert into auto_inc_uint4(col2) values(3);
select * from auto_inc_uint4 order by 1,2;
delete from auto_inc_uint4;
insert into auto_inc_uint4(col2) values(4);
select * from auto_inc_uint4 order by 1,2;
drop table auto_inc_uint4;

create table auto_inc_int8 (
    col1 int8 auto_increment primary key,
    col2 int
);
insert into auto_inc_int8 values(9223372036854775807, 1);
insert into auto_inc_int8(col2) values(2);
insert into auto_inc_int8(col2) values(3);
select * from auto_inc_int8 order by 1,2;
delete from auto_inc_int8;
insert into auto_inc_int8(col2) values(4);
select * from auto_inc_int8 order by 1,2;
drop table auto_inc_int8;

create table auto_inc_uint8 (
    col1 int8 unsigned auto_increment primary key,
    col2 int
);
insert into auto_inc_uint8 values(18446744073709551615, 1);
insert into auto_inc_uint8(col2) values(2);
insert into auto_inc_uint8(col2) values(3);
--select * from auto_inc_uint8 order by 1,2;
delete from auto_inc_uint8;
insert into auto_inc_uint8(col2) values(4);
--select * from auto_inc_uint8 order by 1,2;
drop table auto_inc_uint8;


create table t1( id int) default character set = utf8mb4;
\d t1
drop table t1;
create table t1( id int) default character set utf8mb4;
\d t1
drop table t1;
create table t1( id int) character set = utf8mb4;
\d t1
drop table t1;
create table t1( id int) character set utf8mb4;
\d t1
drop table t1;

create table t1( id int) default character set utf32 COLLATE =utf32_unicode_ci;
\d t1
drop table t1;
create table t1( id int) default character set utf32 COLLATE = utf32_unicode_ci;
\d t1
drop table t1;
create table t1( id int) character set = utf32 COLLATE=utf32_unicode_ci;
\d t1
drop table t1;
create table t1( id int) character set utf32 COLLATE=utf32_unicode_ci;
\d t1
drop table t1;

create table t1( id int) default character set utf32 default character set utf8mb4  default COLLATE=utf32_unicode_ci;
\d t1
drop table t1;
create table t1( id int) default character set utf32 default COLLATE=utf32_unicode_ci  default COLLATE=utf32_unicode_ci;
\d t1
drop table t1;

-----------------------------------------------------------------------------------------------------
-- create partitioned table
CREATE TABLE partitioned_users (
id int auto_increment,
name VARCHAR(50),
created_year INT,
PRIMARY KEY (id, created_year)
) PARTITION BY RANGE (created_year);

-- create partitioned subtable
CREATE TABLE users_2020 PARTITION OF partitioned_users
FOR VALUES FROM (2020) TO (2021);
CREATE TABLE users_2021 PARTITION OF partitioned_users
FOR VALUES FROM (2021) TO (2022);
CREATE TABLE users_2022 PARTITION OF partitioned_users
FOR VALUES FROM (2022) TO (2023);

SELECT * FROM partitioned_users_id_seq;
SELECT schemaname, sequencename, last_value, start_value, increment_by FROM pg_sequences WHERE sequencename = 'partitioned_users_id_seq';

-- test1: insert diff partitioned subtable
INSERT INTO partitioned_users (name, created_year) VALUES
('Alice', 2021),
('Bob', 2021),
('Charlie', 2022);
SELECT * FROM partitioned_users ORDER BY id;

-- test2: insert diff partitioned subtable
INSERT INTO partitioned_users (name, created_year) VALUES
('David', 2020),
('Eve', 2022),
('Frank', 2021);
-- check auto_increment
SELECT id, name, created_year
FROM partitioned_users
ORDER BY id;

SELECT * FROM partitioned_users_id_seq;
SELECT schemaname, sequencename, last_value, start_value, increment_by FROM pg_sequences WHERE sequencename = 'partitioned_users_id_seq';

-- test3: insert partitioned one subtable
INSERT INTO partitioned_users (name, created_year) VALUES
('Grace', 2022),
('Henry', 2022),
('Ivy', 2022);
-- select one subtable
SELECT * FROM users_2022 ORDER BY id;

drop TABLE partitioned_users;

-----------------------------------------------------------------------------------------------------
-- create partitioned table
CREATE TABLE partitioned_users (
id int auto_increment,
name VARCHAR(50),
created_year INT,
PRIMARY KEY (id, created_year)
) PARTITION BY RANGE (created_year);

-- create partitioned subtable
CREATE TABLE users_2020 PARTITION OF partitioned_users
FOR VALUES FROM (2020) TO (2021);
CREATE TABLE users_2021 PARTITION OF partitioned_users
FOR VALUES FROM (2021) TO (2022);
CREATE TABLE users_2022 PARTITION OF partitioned_users
FOR VALUES FROM (2022) TO (2023);

SELECT * FROM partitioned_users_id_seq;
SELECT schemaname, sequencename, last_value, start_value, increment_by FROM pg_sequences WHERE sequencename = 'partitioned_users_id_seq';

-- test1: insert diff subtable
INSERT INTO partitioned_users (id, name, created_year) VALUES
(1, 'Alice', 2021),
(2, 'Bob', 2021),
(3, 'Charlie', 2022);
SELECT * FROM partitioned_users ORDER BY id;

-- test2: insert diff subtable
INSERT INTO partitioned_users (id, name, created_year) VALUES
(4, 'David', 2020),
(5, 'Eve', 2022),
(6, 'Frank', 2021);
-- check auto_increment
SELECT id, name, created_year FROM partitioned_users ORDER BY id;

-- test3: insert one subtable
INSERT INTO partitioned_users (id, name, created_year) VALUES
(100, 'Grace', 2022),
(101, 'Henry', 2022),
(102, 'Ivy', 2022);
INSERT INTO partitioned_users (id, name, created_year) VALUES
(100, 'Grace', 2022);--err
-- select one subtable
SELECT * FROM users_2022 ORDER BY id;
INSERT INTO partitioned_users ( name, created_year) VALUES
( 'greed', 2021),
( 'greed', 2021),
( 'greed', 2021);
SELECT * FROM users_2021 ORDER BY id;
-- test3: insert other subtable
INSERT INTO partitioned_users (id, name, created_year) VALUES
(100, 'Grace', 2021),
(101, 'Henry', 2021),
(102, 'Ivy', 2021);
SELECT * FROM users_2021 ORDER BY id;
SELECT * FROM users_2022 ORDER BY id;
SELECT * FROM partitioned_users_id_seq;
SELECT schemaname, sequencename, last_value, start_value, increment_by FROM pg_sequences WHERE sequencename = 'partitioned_users_id_seq';
SELECT * FROM partitioned_users ORDER BY id , created_year;
drop TABLE partitioned_users;

-----------------------------------------------------------------------------------------------------
-- create partitioned table
CREATE TABLE partitioned_users (
id int auto_increment,
name VARCHAR(50),
PRIMARY KEY (id)
) PARTITION BY RANGE (id);

-- create partitioned subtable
CREATE TABLE p100 PARTITION OF partitioned_users
FOR VALUES FROM (0) TO (100);
CREATE TABLE p200 PARTITION OF partitioned_users
FOR VALUES FROM (100) TO (200);
CREATE TABLE p300 PARTITION OF partitioned_users
FOR VALUES FROM (200) TO (65535);

INSERT INTO partitioned_users ( name) VALUES
( 'Alice'),
( 'Bob'),
( 'Charlie');

INSERT INTO partitioned_users (id, name) VALUES(50, 'Alice');
INSERT INTO partitioned_users (name) VALUES('Alice');
SELECT * FROM p100 ORDER BY id;

INSERT INTO partitioned_users (id, name) VALUES(150, 'Bob');
INSERT INTO partitioned_users (name) VALUES('Bob');
SELECT * FROM p200 ORDER BY id;

INSERT INTO partitioned_users (id, name) VALUES(250, 'Charlie');
INSERT INTO partitioned_users (name) VALUES('Charlie');
SELECT * FROM p300 ORDER BY id;

SELECT * FROM partitioned_users ORDER BY id;
INSERT INTO partitioned_users ( name) VALUES
( 'Alice'),
( 'Bob'),
( 'Charlie');

SELECT * FROM p100 ORDER BY id;
SELECT * FROM p200 ORDER BY id;
SELECT * FROM p300 ORDER BY id;
SELECT * FROM partitioned_users ORDER BY id;
INSERT INTO partitioned_users (id, name) VALUES(250, 'Charlie');

drop TABLE partitioned_users;
-----------------------------------------------------------------------------------------------------

create table test1 (id1 serial, id2 INT AUTO_INCREMENT PRIMARY KEY);
insert into test1 values();
select * from test1;
insert into test1 values();
select * from test1;
insert into test1(id1) values(3);
select * from test1;
insert into test1(id1) values(77);
select * from test1;
insert into test1(id2) values(77);
select * from test1;
ALTER TABLE test1 AUTO_INCREMENT=100;
insert into test1() values();
insert into test1() values();
select * from test1;
drop table test1;