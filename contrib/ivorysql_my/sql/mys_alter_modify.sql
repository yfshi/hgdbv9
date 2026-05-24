------------- alter table modify
-------- test case: table or col did not exist
ALTER TABLE tttt MODIFY aaa int;
CREATE TABLE tttt(id int);
ALTER TABLE tttt MODIFY idd varchar(20);

-------- test case: modify data type
-- varchar  --> varchar
-- there is no data in the table
CREATE TABLE test_dt(id varchar(20));
SHOW CREATE TABLE test_dt;
-- success
ALTER TABLE test_dt MODIFY id varchar(30); 
SHOW CREATE TABLE test_dt;

-- insert some values
-- fail as 'asd' could not change to int
insert into test_dt values('123'),('asd');
select * from test_dt;
alter table test_dt modify id int;
drop table test_dt;

-- int --> varchar
CREATE TABLE test_dt(id int);
insert into test_dt values(123),(345);
alter table test_dt modify id varchar(20);
show create table test_dt;
select * from test_dt;
drop table test_dt;

-- float --> int
CREATE TABLE test_dt(id int, age float);
insert into test_dt values(11,11.11),(22,22.22);
alter table test_dt modify column age int;
select * from test_dt;
\d+ test_dt;
drop table test_dt;
-------- test case: NULL|NOT NULL
-- null --> not null  --> null
create table test_null(id int, name varchar(20));
show create table test_null;
insert into test_null values(10,'asd');
insert into test_null(name) values('fgh');
select * from test_null;
alter table test_null modify id int not null;
delete from test_null where name = 'fgh';
alter table test_null modify id int not null;
insert into test_null(name) values('fgh');
alter table test_null modify id int null;
insert into test_null(name) values('fgh');
show create table test_null;
select * from test_null;
drop table test_null;

-- not null --> null --> not null
create table test_null(id int, name varchar(20) not null);
insert into test_null values(10,'asd');
insert into test_null(id) values(20);
alter table test_null modify name varchar(20) null;
show create table test_null;
insert into test_null(id) values(20);
select * from test_null;
alter table test_null modify name varchar(20) not null;
show create table test_null;
insert into test_null(id) values(20);
drop table test_null;

-------- test case: DEFAULT
-- success: add default
create table test_def(id int, age int, name varchar(20));
insert into test_def values(100,100,'ssss');
alter table test_def modify id default 10;
alter table test_def modify id int default 10;
alter table test_def modify name varchar(20) default 'highgo';
show create table test_def;
insert into test_def values();
select * from test_def;

-- fail: default could not refer the other col in thes same table
alter table test_def modify age int default id*2;

-- success: remove default
alter table test_def modify name varchar(20);
show create table test_def;
insert into test_def(id) values(200);
select * from test_def;

-------- test case: UNIQUE
-- modify to two unique keys for one column, success
create table test_uk(id int, name varchar(20));
alter table test_uk modify id int unique;
show create table test_uk;
alter table test_uk modify id int unique key;
show create table test_uk;

-- modify to primary key for the column which has unique
alter table test_uk modify id int primary key;
show create table test_uk;

-- unique and primary key could not remove by modify command, the same with mysql
--     as id is primary key
--     so when modify id int to null, will keep not null
alter table test_uk modify id int;
show create table test_uk;
drop table test_uk;

-- unique key unique key, add a new one, not two, the same with mysql
create table test_uk(id int unique key, name varchar(20));
show create table test_uk;
alter table test_uk modify id int unique key unique key;
show create table test_uk;
drop table test_uk;

-- check the unique rebuild when datatype changed
-- constraint and index oid are diff between before and after modify datatype
create table test_un_reb(id int, name varchar(20));
insert into test_un_reb values(11,'11'),(21,'21'),(31,'31');
alter table test_un_reb modify name varchar(20) unique key;
show create table test_un_reb;
create table tmp_oid as select conrelid, oid as constraint_oid, conindid as index_oid from pg_constraint where contype='u' and conrelid = 'test_un_reb'::regclass;
alter table test_un_reb modify name int;
select (old.conrelid = new.conrelid) AS rel_oid, (old.constraint_oid = new.constraint_oid) AS constraint_name, (old.index_oid = new.index_oid) AS index_name FROM tmp_oid old JOIN (select conrelid, oid AS constraint_oid, conindid AS index_oid FROM pg_constraint WHERE contype = 'u' AND conrelid = 'test_un_reb'::regclass) new ON true;
-- select data from index hardly by using enable_seqscan = off
set enable_seqscan = off;
explain (COSTS OFF) select * from test_un_reb where name = 21;
select * from test_un_reb where name = 21;
drop table test_un_reb;
drop table tmp_oid;

-------- test case: primary key
create table test_pk(id int, name varchar(20));
alter table test_pk modify id int key;
show create table test_pk;

-- primary key(id), modify id as priamry key
-- report an error
--	mysql report: Multiple primary key defined
--	V9 report: multiple primary keys for table xxxx are not allowed
alter table test_pk modify id int primary key;
alter table test_pk modify id varchar(20) not null primary key;
drop table test_pk;

-- primary key(id,age), modify id as priamry key
-- report an error
--	mysql report: Multiple primary key defined
--	V9 report: multiple primary keys for table xxxx are not allowed
create table test_pk (id int, name varchar(20), age int, primary key(id,name));
alter table test_pk modify id int primary key;
alter table test_pk modify age int primary key;
drop table test_pk;

-------- test case: fk check
create table parent(id int unique key, name varchar(20));
create table child (id int, parent_id int, name varchar(20), constraint fk_parent foreign key(parent_id) references parent(id));
-- mysql error: Cannot change column 'id': used in a foregin key constraint 'fk_parent' of table 'mysql.child'
-- v9 error: Cannot change column 'id': used in a foregin key constraint
alter table parent modify id bigint;
alter table parent modify id float;
alter table parent modify id varchar(20);
alter table parent modify id enum('1','2','3');
alter table child modify parent_id bigint;
alter table child modify parent_id float;
alter table child modify parent_id varchar(20);
alter table child modify parent_id enum('1','2','3');

-- mysql error: add auto_increment
alter table parent modify id int auto_increment;
alter table child modify parent_id int auto_increment;

-- mysql success: use the same datatype to modify
alter table parent modify id int;
alter table child modify parent_id int;

-- mysql success: add/remove comment
alter table parent modify id int comment 'test_parent';
alter table child modify parent_id int comment 'test_child';
show create table parent;
show create table child;
alter table parent modify id int;
alter table child modify parent_id int;
show create table parent;
show create table child;
select count(*) from pg_description where objoid = 'parent'::regclass and objsubid = (select attnum from pg_attribute where attrelid = 'parent'::regclass and attname = 'id');
select count(*) from pg_description where objoid = 'child'::regclass and objsubid = (select attnum from pg_attribute where attrelid = 'child'::regclass and attname = 'parent_id');
drop table child, parent;

-------- test case: auto_increment
-- modify is the same with create table
create table test_auto(id int auto_increment unique key, name varchar(20));
show create table test_auto;
drop table test_auto;
create table test_auto(id int, name varchar(20));
alter table test_auto modify id int auto_increment unique key;
show create table test_auto;
drop table test_auto;

-- insert some values,check the next value is right
create table test_auto(id int, name varchar(20));
insert into test_auto values(10,'asd'),(20,'fgh');
select * from test_auto;
alter table test_auto modify id int auto_increment unique key;
show create table test_auto;
insert into test_auto(name) values('highgo');
select * from test_auto;

-- remove the auto_increment
alter table test_auto modify id int;
insert into test_auto values(100,'go');
show create table test_auto;
-- check the seq oid removed
select pg_get_serial_sequence('public.test_auto', 'id')::regclass::oid;
drop table test_auto;

-- if there is no auto, modify one
create table test_auto(a int, b int, c varchar(20));
alter table test_auto modify c int auto_increment unique key;
\d+ test_auto;
show create table test_auto;
drop table test_auto;

-- if there has one, modify the same one
-- refer alter_table_modify_multi

-- alter modify diff col auto_increment as there already has one
create table test_auto(a int,b int,c int, idd int auto_increment primary key);
alter table test_auto modify c int auto_increment unique key;
drop table test_auto;

-- modify to unique key or primary key, then modify to auto_increment
create table test_auto(id int unique key, name varchar(20));
alter table test_auto modify id int auto_increment;
show create table test_auto;
drop table test_auto;
-- success
create table test_auto(id int, name varchar(20));
alter table test_auto modify id int primary key;
alter table test_auto modify id int auto_increment;
show create table test_auto;
drop table test_auto;

-- fail as id is not a primary key or a unique key
create table test_auto(id int, name varchar(20));
alter table test_auto modify id int auto_increment;
-- mysql success, but v9 fail, canot support
alter table test_auto modify id int primary key, modify id int auto_increment;
alter table test_auto modify id int unique key, modify id int auto_increment;
drop table test_auto;

-- varchar(20)  --> smallint auto
-- smallint max is 32767, report error with alter type
create table test_auto(id varchar(20));
insert into test_auto values('32768');
alter table test_auto modify id smallint unique key;
drop table test_auto;

create table test_auto(id varchar(20));
insert into test_auto values('11.21');
insert into test_auto values();
insert into test_auto values('31.51');
insert into test_auto values();
insert into test_auto values('21.49');
insert into test_auto values();
alter table test_auto modify id smallint unique key auto_increment;
select * from test_auto;
show create table test_auto;
insert into test_auto values();
select * from test_auto;
drop table test_auto;

-- varchar(20) --> int auto
create table test_auto(id varchar(20));
insert into test_auto values('11.21');
insert into test_auto values();
insert into test_auto values('31.51');
insert into test_auto values();
insert into test_auto values('21.49');
insert into test_auto values();
select * from test_auto;
alter table test_auto modify id varchar(20) primary key;
alter table test_auto modify id varchar(20) unique key;
select * from test_auto;
alter table test_auto modify id int auto_increment;
show create table test_auto;
insert into test_auto values();
select * from test_auto;
drop table test_auto;

-- varchar(20) --> bigint auto
create table test_auto(id varchar(20));
insert into test_auto values('11.21');
insert into test_auto values();
insert into test_auto values('31.51');
insert into test_auto values();
insert into test_auto values('21.49');
insert into test_auto values();
select * from test_auto;
alter table test_auto modify id bigint unique key auto_increment;
show create table test_auto;
insert into test_auto values();
select * from test_auto;
drop table test_auto;

-- varchar(20) --> float auto
-- pg could not alter varchar to float, need using USING
--     add a pg_cast to convert from varchar to float
create table test_auto(id varchar(20));
alter table test_auto alter column id TYPE float;
show create table test_auto;
drop table test_auto;

create table test_auto(id varchar(20));
insert into test_auto values('11.21');
insert into test_auto values();
insert into test_auto values('31.51');
insert into test_auto values();
insert into test_auto values('21.49');
insert into test_auto values();
select * from test_auto;
alter table test_auto modify id float unique key auto_increment;
show create table test_auto;
insert into test_auto values();
select * from test_auto;
drop table test_auto;

-- varchar(20) --> float8 auto
create table test_auto(id varchar(20));
insert into test_auto values('11.21');
insert into test_auto values();
insert into test_auto values('31.51');
insert into test_auto values();
insert into test_auto values('21.49');
insert into test_auto values();
select * from test_auto;
alter table test_auto modify id float8 unique key auto_increment;
show create table test_auto;
insert into test_auto values();
select * from test_auto;
drop table test_auto;

-- text --> smallint auto
create table test_auto(id text);
insert into test_auto values('11.21');
insert into test_auto values();
insert into test_auto values('31.51');
insert into test_auto values();
insert into test_auto values('21.49');
insert into test_auto values();
alter table test_auto modify id smallint unique key auto_increment;
select * from test_auto;
show create table test_auto;
insert into test_auto values();
select * from test_auto;
drop table test_auto;

-- text --> int auto
create table test_auto(id text);
insert into test_auto values('11.21');
insert into test_auto values();
insert into test_auto values('31.51');
insert into test_auto values();
insert into test_auto values('21.49');
insert into test_auto values();
select * from test_auto;
alter table test_auto modify id int unique key auto_increment;
show create table test_auto;
insert into test_auto values();
select * from test_auto;
drop table test_auto;

-- text --> bigint auto
create table test_auto(id text);
insert into test_auto values('11.21');
insert into test_auto values();
insert into test_auto values('31.51');
insert into test_auto values();
insert into test_auto values('21.49');
insert into test_auto values();
select * from test_auto;
alter table test_auto modify id bigint unique key auto_increment;
show create table test_auto;
insert into test_auto values();
select * from test_auto;
drop table test_auto;

-- text --> float auto
create table test_auto(id text);
insert into test_auto values('11.21');
insert into test_auto values();
insert into test_auto values('31.51');
insert into test_auto values();
insert into test_auto values('21.49');
insert into test_auto values();
select * from test_auto;
alter table test_auto modify id float unique key auto_increment;
show create table test_auto;
insert into test_auto values();
select * from test_auto;
drop table test_auto;

-- text --> float8 auto
create table test_auto(id text);
insert into test_auto values('11.21');
insert into test_auto values();
insert into test_auto values('31.51');
insert into test_auto values();
insert into test_auto values('21.49');
insert into test_auto values();
select * from test_auto;
alter table test_auto modify id float8 unique key auto_increment;
show create table test_auto;
insert into test_auto values();
select * from test_auto;
drop table test_auto;

-- enum --> int auto
create table test_auto(a int, color enum('aaa', 'bbb', 'ccc', 'ddd', 'eee'));
-- error, the same with mysql
alter table test_auto modify color enum primary key;
--succ, there is no data
alter table test_auto modify color enum('q','w','e','r','t') primary key;
drop table test_auto;

create table test_auto(a int, color enum('aaa', 'bbb', 'ccc', 'ddd', 'eee'));
insert into test_auto values(11,'bbb'),(21,4);
-- fail, auto_increment must be smallint, int, bigint, float4, float8
alter table test_auto modify color enum('ddd','q','w','e','r','t') primary key auto_increment;
alter table test_auto modify color int primary key auto_increment;
show create table test_auto;
select * from test_auto;
drop table test_auto;

-- enum --> float auto
create table test_auto(a int, color enum('aaa', 'bbb', 'ccc', 'ddd', 'eee'));
insert into test_auto values(11,'bbb'),(21,4);
alter table test_auto modify color float unique key auto_increment;
show create table test_auto;
select * from test_auto;
drop table test_auto;

-- float --> auto_increment
create table test_auto (name varchar(20),id float unique key);
insert into test_auto values('aaa',11.23),('bbb',21.51);
alter table test_auto modify column id float auto_increment;
show create table test_auto;
insert into test_auto(name) values('ccc');
select * from test_auto;
show create table test_auto;
drop table test_auto;

-------- test case: auto_increment must be a pri key, uni key or composite key
-- mysql: for InnoDB engine, if auto column is a composite key, it should be leftmost of the index
-- V9: if it is a composite key, did not should be leftmost of the index, the same with create table
create table test_auto(id1 int, id2 int, id3 int, id4 int, id5 int auto_increment, unique key(id2,id3),unique key(id4,id5));
show create table test_auto;
drop table test_auto;
create table test_auto(id1 int, id2 int, id3 int, id4 int, id5 int, unique key(id2,id3),unique key(id4,id5));
alter table test_auto modify id5 int auto_increment;
show create table test_auto;
drop table test_auto;

-------- test case: auto_increment, sql_mode NO_AUTO_VALUE_ON_ZERO
-- mysql: if sql_mode not set NO_AUTO_VALUE_ON_ZERO (default), 0 need fill by next value
--        if set NO_AUTO_VALUE_ON_ZERO, 0 is 0
-- not set NO_AUTO_VALUE_ON_ZERO
create table test_auto(id varchar(20),age int);
insert into test_auto(age) values(101);
insert into test_auto value('102',102);
insert into test_auto values('0',103);
insert into test_auto(age) values(104);
select * from test_auto;
alter table test_auto modify id int unique key auto_increment;
select * from test_auto;
show create table test_auto;
drop table test_auto;

-- set NO_AUTO_VALUE_ON_ZERO
set sql_mode='ANSI_QUOTES,NO_AUTO_VALUE_ON_ZERO';
create table test_auto(id varchar(20), age int);
insert into test_auto(age) values(101);
insert into test_auto value('102',102);
insert into test_auto values('0',103);
insert into test_auto(age) values(104);
select * from test_auto;
alter table test_auto modify id int unique key auto_increment;
select * from test_auto;
show create table test_auto;
reset sql_mode;
drop table test_auto;

-------- test case: auto_increment, refer create_table sql
-- auto_increment, LAST_INSERT_ID()
CREATE TABLE test_t (
    id INT,
    name VARCHAR(50),
    value INT
);
INSERT INTO test_t (name, value) VALUES ('A', 1), ('B', 2), ('C', 3), ('D', 4);
select last_insert_id();
alter table test_t modify id int auto_increment unique key;
select last_insert_id();
INSERT INTO test_t (name, value) VALUES ('E', 5);
select last_insert_id();
select * from test_t;
drop table test_t;

-- auto_increment, GUC: AUTO_INCREMENT_INCREMENT  AUTO_INCREMENT_OFFSET
-- mysql return:4,14,24,34,44,54,60,61,62
CREATE TABLE test_t (
    id INT,
    name VARCHAR(50),
    value INT
);
INSERT INTO test_t (name, value) VALUES ('A', 1), ('B', 2), ('C', 3);
SELECT * FROM test_t;
SET AUTO_INCREMENT_INCREMENT = 10;
SET AUTO_INCREMENT_OFFSET = 100;
alter table test_t modify id int auto_increment unique key;
select * from test_t;
show create table test_t;
INSERT INTO test_t (name, value) VALUES ('D', 4);
SELECT * FROM test_t;
show create table test_t;
INSERT INTO test_t (name, value) VALUES ('D', 4);
SELECT * FROM test_t;
INSERT INTO test_t (name, value) VALUES ('D', 4);
SELECT * FROM test_t;
SET AUTO_INCREMENT_INCREMENT = 1;
SET AUTO_INCREMENT_OFFSET = 1;
INSERT INTO test_t (name, value) VALUES ('D', 4);
SELECT * FROM test_t;
INSERT INTO test_t (name, value) VALUES ('D', 4);
INSERT INTO test_t (name, value) VALUES ('D', 4);
SELECT * FROM test_t;
drop table test_t;

-- modify then update set
create table t1 (a int not null primary key);
insert into t1 values (10);
alter table t1 modify a int auto_increment;
show create table t1;
update t1 set a = 5 where a = 10;
select * from t1 order by a;
show create table t1;
insert into t1 values();
select * from t1 order by a;
update t1 set a = 20;
update t1 set a = 20 where a = 5;
select * from t1 order by a;
drop table t1;

create table t1 (a int not null primary key);
insert into t1 values (10);
alter table t1 modify a int auto_increment;
show create table t1;
update t1 set a = 15;
select * from t1 order by a;
show create table t1;
insert into t1 values();
select * from t1 order by a;
update t1 set a = 20;
update t1 set a = 20 where a = 5;
select * from t1 order by a;
drop table t1;

-------- test case: comment
-- success: add comment
create table test_comm(id int, name varchar(20));
alter table test_comm modify name int not null comment 'not_null';
show create table test_comm;

-- success: change comment
alter table test_comm modify name int not null comment 'changecomment';
show create table test_comm;
select count(*) from pg_description where objoid = 'test_comm'::regclass and objsubid = (select attnum from pg_attribute where attrelid = 'test_comm'::regclass and attname = 'name');

-- success: remove comment
alter table test_comm modify name varchar(20);
show create table test_comm;
select count(*) from pg_description where objoid = 'test_comm'::regclass and objsubid = (select attnum from pg_attribute where attrelid = 'test_comm'::regclass and attname = 'name');
drop table test_comm;

-- mysql col comment less then 1024, but v9 no this check
-- this comment is 1026
create table t1 (
    c1 VARCHAR(10) NOT NULL COMMENT 'c1 comment',
    c2 INTEGER,
    c3 INTEGER,
    c4 INTEGER);

alter table t1 modify c3 int COMMENT 'abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcde';
drop table t1;

-------- test case: collate
-- create fail: Incorrect integer value: 'a' for column
create table test_coll(id int CHARACTER SET utf8 NOT NULL DEFAULT 'a');
-- the same fail: Incorrect integer value: 'a' for column
create table test_coll(id int, name varchar(20), address varchar(20));
alter table test_coll modify id int CHARACTER SET utf8 NOT NULL DEFAULT 'a';
-- add new collate
alter table test_coll modify name VARCHAR(255) CHARACTER SET utf8 NOT NULL DEFAULT 'name';
alter table test_coll modify address VARCHAR(255) COLLATE utf8_unicode_ci NULL DEFAULT 'address';
\d+ test_coll;

-- change collate
alter table test_coll modify address text COLLATE "utf8mb3_unicode_ci";
\d+ test_coll;

-- support collate default
alter table test_coll modify address text COLLATE "default"
\d+ test_coll;

-- remove  collate
alter table test_coll modify address text;
\d+ test_coll;

drop table test_coll;

-------- test case: modify view
create table test_view(id int, name varchar(20));
create view test_view_m AS select * from test_view;
-- fail: could not modify view
alter table test_view_m modify id varchar(20);
drop view test_view_m;
drop table test_view;

-------- test case: temp table
create temp table test_tmp(a int, id int, name varchar(20));
alter table test_tmp modify id varchar(20);
show create table test_tmp;
insert into test_tmp(id) values(21),(21),(31);
-- fail as duplicated
alter table test_tmp modify id int primary key;
delete from test_tmp;
-- success
insert into test_tmp(id) values(21),(31);
alter table test_tmp modify id varchar(20) primary key;
alter table test_tmp modify id int auto_increment;
ALTER TABLE test_tmp AUTO_INCREMENT = 1000;
insert into test_tmp values();
show create table test_tmp;
select * from test_tmp;
drop table test_tmp;

-------- test case: partition table
-- using pg syntax to create partition table
create table test_par(id int, name varchar(20)) partition by range(id);
create table test_par_1 partition of test_par for values from(0) to (100);
create table test_par_2 partition of test_par for values from(100) to (200);
create table test_par_3 partition of test_par for values from(200) to (MAXVALUE);
alter table test_par modify id varchar(20);
alter table test_par modify name float default 222;
show create table test_par;
\d+ test_par;
\d+ test_par_1;
insert into test_par(id) values(101);
select * from test_par;
alter table test_par modify name float default 333 comment 'chil_new';
\d+ test_par;
\d+ test_par_1;
-- mysql success, the same datatype or add auto_increment
-- dolphin fail, v9 fail
alter table test_par modify id int;
alter table test_par modify id int auto_increment;
alter table test_par modify id int primary key;
drop table test_par;

-------- test case: inherited table
-- mysql did not support inherited table, so did not test

-------- test case: modify index
create table test_i(id int, name varchar(20));
create index test_i_index on test_i(id);
alter table test_i_index modify id int;
drop index test_i_index;
drop table test_i;

-------- test case: generated always as
-- mysql: no stored, it means virtual, but v9 did not support no stored
create table test_gen(a int, b int, c int generated always as (a+b));

-- show create table generate, compare with modify generate
create table test_create_gen(a int, b int, c int generated always as (a+b) STORED);
\d+ test_create_gen;
create table test_gen(a int, b int, c int);
alter table test_gen modify c int generated always as (a+b) STORED;
\d+ test_gen;
drop table test_gen;

-- if there have value, then modify
-- success, the generate worked for the new insert data
create table test_gen(a int, b int, c int);
insert into test_gen values(1,1,1),(2,2,2);
alter table test_gen modify c int generated always as (a+b) STORED;
select * from test_gen;
insert into test_gen values(3,3,3);
insert into test_gen values(3,3);
select * from test_gen;
drop table test_gen;

-- remove the generate
create table test_gen(a int, b int, c int generated always as (a+b+10) STORED);
insert into test_gen values(1,1),(2,2);
-- fail: a+b+10 --> varchar(20)
-- 	mysql success
--	v9 report error, this error is the same with generated.out
alter table test_gen modify c varchar(20);
-- fail with the same error: alter table xxx alter column c TYPE float;
alter table test_gen modify c float;
-- fail with the same error: alter table xxx alter column c TYPE int;
alter table test_gen modify c int;
drop table test_gen;

-------- test case: enum type
create table test_enum(a int, color enum('aaa', 'bbb', 'ccc', 'ddd', 'eee'));
show create table test_enum;
insert into test_enum values(10,2),(11,3);
select * from test_enum;
-- fail
--	v9 report: Incorrect integer value
-- 	mysql report: ERROR 1265 (01000): Data truncated for column xxx
alter table test_enum modify color enum('111', '222', '333', '444');
-- succ
alter table test_enum modify color enum('111', 'bbb', '333', 'ccc', '444');
select * from test_enum;
-- enum --> int
alter table test_enum modify color int;
select * from test_enum;
drop table test_enum;

-- enum --> varchar
create table test_enum(a int, color enum('aaa', 'bbb', 'ccc', 'ddd', 'eee'));
insert into test_enum values(10,2),(11,3);
select * from test_enum;
alter table test_enum modify color varchar(20);
select * from test_enum;

-------- test case: trigger
-- v9 trigger syntax is not the same with mysql
-- mysql trigger: just fired for DDL(INSERT/UPDATE/DELETE)
-- V9 trigger: the same with mysql, so modify did not fired the trigger
-- after change the data type, the next insert will fail, need clean the cache plan or using a new session
create table t1(id serial, age text);
create table t1_log(log_id serial primary key, action text, old_age text, new_age bigint);
CREATE OR REPLACE FUNCTION trg_fn_insert()
RETURNS trigger AS $$
BEGIN
INSERT INTO t1_log(action, old_age, new_age) values('insert', NULL, NEW.age);
RETURN NEW;
END;
$$ LANGUAGE plpgsql;

create trigger trg_t1_after_insert
after insert on t1
for each row
execute function trg_fn_insert();

insert into t1(age) values('22');
select * from t1_log;

alter table t1 modify age bigint unique key auto_increment;
show create table t1;
select * from t1_log;
drop table t1;
drop function trg_fn_insert();
drop table t1_log;

-------- test case: column auto_increment and  table option auto_increment
-- alter table AUTO_INCREMENT
create table test_tab_op(id int, name varchar(20));
insert into test_tab_op values(10,'ssss'),(20,'dddd');
alter table test_tab_op modify id int unique key;
alter table test_tab_op modify id int auto_increment;
alter table test_tab_op AUTO_INCREMENT = 10;
show create table test_tab_op;
alter table test_tab_op AUTO_INCREMENT = 30;
show create table test_tab_op;
select * from test_tab_op;
drop table test_tab_op;

create table test_tab_op(id int, name varchar(20));
insert into test_tab_op values(10,'ssss'),(20,'dddd');
-- AUTO_INCREMENT = 30, mysql success
-- v9 fail as "no owned sequence found", expected
alter table test_tab_op modify id int unique key auto_increment, AUTO_INCREMENT = 30;
drop table test_tab_op;

-------- test case: reference did not support
create table test_1(a int, b int, c int);
create table test_2(id_1 int, id_2 int, id_3 int);
insert into test_1 values(11,11,11),(22,22,22),(33,33,33);
insert into test_2 values(1,1,1),(2,2,2),(3,3,3);
alter table test_2 modify id_2 varchar(20) reference test_1(b);
drop table test_1, test_2;

------- test case: all option
create table test_all(a int, b int, c int);
alter table test_all modify b float not null auto_increment unique comment 'modify_1' collate utf8_general_ci;
alter table test_all modify b text not null auto_increment unique comment 'modify_1' collate utf8_general_ci;
alter table test_all modify b float null auto_increment key comment 'modify_1';
\d+ test_all;
alter table test_all modify b varchar(20);
\d+ test_all;
alter table test_all modify c varchar(20) default 'aaaa' unique comment 'modify_c' collate C;
\d+ test_all;
alter table test_all modify c varchar(20) default 'aaaa' comment 'modify_c';
\d+ test_all;
alter table test_all modify c varchar(20) default 'aaaa';
\d+ test_all;
alter table test_all modify c float;
\d+ test_all;
drop table test_all;

------------- alter table change
-------- test case: table or col did not exist
alter table test_name change id idddd int;
create table test_name(id int, name varchar(20));
alter table test_name change idd idddd int;
alter table test_name change id int;
drop table test_name;

-------- test case:  column name
-- new col name = old col name
create table test_no(id int, name varchar(20));
alter table test_no change name name varchar(20);
\d+ test_no;
drop table test_no;

-- change col name
create table test_name(id int, name varchar(20));
insert into test_name values(1, 'aaa'),(2, 'bbb');
alter table test_name change id iddd int;
\d+ test_name;
select * from test_name;
drop table test_name;

-- change col name, primary key, unique key, index for diff column
create table test_name(id int, name varchar(20), age float,
			primary key(id),
			unique key(name),
			index(age));
\d+ test_name;
insert into test_name values(1, 'aaa', 11.11),(2, 'bbb', 22.22);
alter table test_name change id id2 int;
alter table test_name change name name2 varchar(20);
alter table test_name change age age2 float;
\d+ test_name;
select indexname, indexdef from pg_indexes where tablename = 'test_name';
select * from test_name;
drop table test_name;

-- test case: change col name, primary key, unique key, index for the same column
-- then change data type, keep the name
create table test_name(id int, name varchar(20), age float,
			primary key(age),
			unique key(age),
			index(age));
insert into test_name values(10,'highgo',12.34),(20,'go',45.67);
alter table test_name change age age2 float;
\d+ test_name;
alter table test_name change age2 age2 int;
\d+ test_name;
select * from test_name;
drop table test_name;

-------- test case: comment
create table test_co (a int, b int, c int);
alter table test_co change b b_new int comment 'b_new';
show create table test_co;
select count(*) from pg_description where objoid = 'test_co'::regclass and objsubid = (select attnum from pg_attribute where attrelid = 'test_co'::regclass and attname = 'b');
select count(*) from pg_description where objoid = 'test_co'::regclass and objsubid = (select attnum from pg_attribute where attrelid = 'test_co'::regclass and attname = 'b_new');

alter table test_co change b_new b_new int comment 'b_new_new';
show create table test_co;
select count(*) from pg_description where objoid = 'test_co'::regclass and objsubid = (select attnum from pg_attribute where attrelid = 'test_co'::regclass and attname = 'b_new');

alter table test_co change b_new b int;
show create table test_co;
select count(*) from pg_description where objoid = 'test_co'::regclass and objsubid = (select attnum from pg_attribute where attrelid = 'test_co'::regclass and attname = 'b_new');
select count(*) from pg_description where objoid = 'test_co'::regclass and objsubid = (select attnum from pg_attribute where attrelid = 'test_co'::regclass and attname = 'b');
drop table test_co;

-------- test case: NULL/NOT NULL
create table test_cn (a int, b int, c int);
alter table test_cn change b b_new int comment 'b_new' NOT NULL;
show create table test_cn;
alter table test_cn change b_new b_new int comment 'b_new';
alter table test_cn change b_new b_new int;
alter table test_cn change b_new b;
alter table test_cn change b_new float comment 'b_new' NOT NULL;
alter table test_cn change b_new b float comment 'b_new' NOT NULL;
show create table test_cn;
drop table test_cn;

-------- test case: enum
-- change enum name
create table test_name(id int, color enum('aaa', 'bbb', 'ccc', 'ddd'));
insert into test_name values(10,2),(11,3);
\d+ test_name;
alter table test_name change color color enum('aaa', 'bbb', 'ccc', 'ddd');
\d+ test_name;
alter table test_name change color color_new enum('aaa', 'bbb', 'ccc', 'ddd');
\d+ test_name;
alter table test_name change color_new color_new enum('111','bbb','ccc','444','555');
\d+ test_name;
alter table test_name change color_new color enum('123','bbb','456','ccc');
drop table test_name;

-- change enum name, but enum value not match
-- fail
--      v9 report: Incorrect integer value
--      mysql report: ERROR 1265 (01000): Data truncated for column xxx
create table test_name(id int, color enum('aaa', 'bbb', 'ccc', 'ddd'));
insert into test_name values(10,2),(11,3);
alter table test_name change color color_new enum('aaa', 'bbb', 'ddd');
\d+ test_name;
select * from test_name;
drop table test_name;

-- change enum --> int
create table test_name(id int, color enum('aaa', 'bbb', 'ccc', 'ddd'));
insert into test_name values(10,2),(11,3);
alter table test_name change color color_new int;
show create table test_name;
select * from test_name;
drop table test_name;

-- change enum --> varchar
create table test_name(id int, color enum('aaa', 'bbb', 'ccc', 'ddd'));
insert into test_name values(10,2),(11,3);
alter table test_name change color color_new varchar(20);
show create table test_name;
select * from test_name;
drop table test_name;

-------- test case: change view
create table test_view(id int, name varchar(20));
create view test_view_m AS select * from test_view;
-- fail: could not modify view
alter table test_view_m change id iddd varchar(20);
drop view test_view_m;
drop table test_view;

------ test case: change a auto
create table test_c2(a int, id int, name varchar(20));
alter table test_c2 change id idd int auto_increment unique key;
alter table test_c2 change idd idd int auto_increment unique key;
show create table test_c2;
drop table test_c2;

-------- test case: temp table
create temp table test_tmp(a int, id int, name varchar(20));
alter table test_tmp change id id varchar(20);
show create table test_tmp;
insert into test_tmp(id) values(21),(21),(31);
-- fail as duplicated
alter table test_tmp change id id_1 int primary key;
delete from test_tmp;
-- success
insert into test_tmp(id) values(21),(31);
alter table test_tmp change id id_2 varchar(20) primary key;
alter table test_tmp change id_2 id_2 int auto_increment;
ALTER TABLE test_tmp AUTO_INCREMENT = 1000;
insert into test_tmp values();
show create table test_tmp;
select * from test_tmp;
drop table test_tmp;

-------- test case: partition table
-- using pg syntax to create partition table
create table test_par(id int, name varchar(20)) partition by range(id);
create table test_par_1 partition of test_par for values from(0) to (100);
create table test_par_2 partition of test_par for values from(100) to (200);
create table test_par_3 partition of test_par for values from(200) to (MAXVALUE);
alter table test_par change id id_1 varchar(20);
alter table test_par change name name_2 varchar(20) default 'sdf';
\d+ test_par;
\d+ test_par_1;
show create table test_par;
show create table test_par_1;
insert into test_par(id) values(101);
select * from test_par;
-- mysql success, the same datatype or add auto_increment
-- dolphin fail, v9 fail
alter table test_par change id id int;
alter table test_par change id id_1 int auto_increment;
alter table test_par change id id_1 int primary key;

drop table test_par;

-------- test case: inherited table
-- mysql did not support inherited table, so did not test

-------- test case: trigger
-- v9 trigger syntax is not the same with mysql
-- mysql trigger: just fired for DDL(INSERT/UPDATE/DELETE)
-- V9 trigger: the same with mysql, so modify did not fired the trigger
-- after change the data type, the next insert will fail, need clean the cache plan or using a new session
create table t1(id serial, age text);
create table t1_log(log_id serial primary key, action text, old_age text, new_age bigint);
CREATE OR REPLACE FUNCTION trg_fn_insert()
RETURNS trigger AS $$
BEGIN
INSERT INTO t1_log(action, old_age, new_age) values('insert', NULL, NEW.age);
RETURN NEW;
END;
$$ LANGUAGE plpgsql;

create trigger trg_t1_after_insert
after insert on t1
for each row
execute function trg_fn_insert();

insert into t1(age) values('22');
select * from t1_log;

alter table t1 change age age bigint unique key auto_increment;
\d+ t1;
show create table t1;
alter table t1 change age age_new bigint unique key;
\d+ t1;
show create table t1;
select * from t1_log;
drop table t1;
drop function trg_fn_insert();
drop table t1_log;

-------- test case: fk check
create table parent(id int unique key, name varchar(20));
create table child (id int, parent_id int, name varchar(20), constraint fk_parent foreign key(parent_id) references parent(id));
-- mysql error: Cannot change column 'id': used in a foregin key constraint 'fk_parent' of table 'mysql.child'
-- v9 error: Cannot change column 'id': used in a foregin key constraint
alter table parent change id id_new bigint;
alter table parent change id id varchar(20);
alter table child change parent_id parent_id bigint;
alter table child change parent_id parent_id varchar(20);

-- mysql error: add auto_increment
alter table parent change id id_new int auto_increment;
alter table child change parent_id  parent_id_new int auto_increment;

-- mysql success: use the same datatype to modify
alter table parent change id id_new int;
alter table child change parent_id parent_id_new int;
show create table parent;
show create table child;
\d+ parent;
\d+ child;

-- mysql success: add/remove comment
alter table parent change id_new id int comment 'test_parent';
alter table child change parent_id_new parent_id int comment 'test_child';
show create table parent;
show create table child;
alter table parent change id id int;
alter table child change parent_id parent_id int;
show create table parent;
show create table child;
select count(*) from pg_description where objoid = 'parent'::regclass and objsubid = (select attnum from pg_attribute where attrelid = 'parent'::regclass and attname = 'id');
select count(*) from pg_description where objoid = 'child'::regclass and objsubid = (select attnum from pg_attribute where attrelid = 'child'::regclass and attname = 'parent_id');
drop table child, parent;

-------- test case: column auto_increment and  table option auto_increment
-- alter table AUTO_INCREMENT
create table test_tab_op(id int, name varchar(20));
insert into test_tab_op values(10,'ssss'),(20,'dddd');
alter table test_tab_op change id id int unique key;
alter table test_tab_op change id id_1 int auto_increment;
alter table test_tab_op AUTO_INCREMENT = 10;
show create table test_tab_op;
alter table test_tab_op AUTO_INCREMENT = 30;
show create table test_tab_op;
select * from test_tab_op;
drop table test_tab_op;

create table test_tab_op(id int, name varchar(20));
insert into test_tab_op values(10,'ssss'),(20,'dddd');
-- AUTO_INCREMENT = 30, mysql success
-- v9 fail as "no owned sequence found", expected
alter table test_tab_op change id id_new int unique key auto_increment, AUTO_INCREMENT = 30;
drop table test_tab_op;

------------- alter table FIRST | AFTER col_name
------ test case: alter table add
create table test_add(id int);
alter table test_add add name varchar(20) FIRST;
alter table test_add add ts timestamp AFTER id;
\d+ test_add;

-- fail: FIRST AFTER could not use together
alter table test_add add address varchar(20) FIRST AFTER id;
-- fail: must in the end
alter table test_add add ts_2 FIRST int;
alter table test_add add ts_3 AFTER name int;
drop table test_add;

------ test case: alter table modify
create table test_m(id int, name varchar(20), age int);
\d+ test_m;
alter table test_m modify name varchar(20) FIRST;
alter table test_m modify id int AFTER nametest_m;
\d+ test_m;
alter table test_m modify id int AFTER name FIRST;
alter table test_m modify age FIRST int
alter table test_m modify age AFTER id int;
drop table test_m;

------ test case: alter table change
create table test_ch (id int, name varchar(20), age int);
show create table test_ch;
alter table test_ch change name name varchar(20) FIRST;
alter table test_ch change id idd int AFTER name;
show create table test_ch;
alter table test_ch change age age_new idd int AFTER name FIRST;
alter table test_ch change age age_new FIRST int;
alter table test_ch change age age_new AFTER name int;
drop table test_ch;

------ test case from mysql
---- multi command
create table t1 (
col1 int not null auto_increment primary key,
col2 varchar(30) not null,
col3 varchar (20) not null,
col4 varchar(4) not null,
col5 enum('PENDING', 'ACTIVE', 'DISABLED') not null,
col6 int not null,
to_be_deleted int);
insert into t1 values (2,4,3,5,'PENDING',1,7);
alter table t1
drop column to_be_deleted,
change column col2 fourth varchar(30) not null after col3,
modify column col6 int not null first;
show create table t1;
select * from t1;
drop table t1;

-- #
-- # BUG#6236 - ALTER TABLE MODIFY should set implicit NOT NULL on PK columns
-- #
create table t1 ( a varchar(10) not null primary key ) engine=myisam;
alter table t1 modify a varchar(10);
show create table t1;
alter table t1 modify a varchar(10) not null;
show create table t1;
drop table if exists t1;

-- #
-- # Bug#33873: Fast ALTER TABLE doesn't work with multibyte character sets
-- #
CREATE TABLE t1 (id int, c int) character set latin1;
INSERT INTO t1 VALUES (1,1);
--enable_info
ALTER TABLE t1 CHANGE c d int;
ALTER TABLE t1 CHANGE d c int;
ALTER TABLE t1 MODIFY c VARCHAR(10);
ALTER TABLE t1 CHANGE c d varchar(10);
ALTER TABLE t1 CHANGE d c varchar(10);
select * from t1;
DROP TABLE t1;

CREATE TABLE t1 (id int, c int) character set utf8;
INSERT INTO t1 VALUES (1,1);
--enable_info
ALTER TABLE t1 CHANGE c d int;
ALTER TABLE t1 CHANGE d c int;
ALTER TABLE t1 MODIFY c VARCHAR(10);
ALTER TABLE t1 CHANGE c d varchar(10);
ALTER TABLE t1 CHANGE d c varchar(10);
select * from t1;
DROP TABLE t1;

-- #
-- # Bug #23113: Different behavior on altering ENUM fields between 5.0 and 5.1
-- #
CREATE TABLE t1(a INT AUTO_INCREMENT PRIMARY KEY, 
  b ENUM('a', 'b', 'c') NOT NULL);
INSERT INTO t1 (b) VALUES ('a'), ('c'), ('b'), ('b'), ('a');
ALTER TABLE t1 MODIFY b ENUM('a', 'z', 'b', 'c') NOT NULL;
SELECT * FROM t1;
DROP TABLE t1;

CREATE TABLE t1 (a ENUM('a1','a2'));
INSERT INTO t1 VALUES ('a1'),('a2');
ALTER TABLE t1 MODIFY COLUMN a ENUM('a1','a2');
ALTER TABLE t1 MODIFY COLUMN a ENUM('a1','a2','a3');
ALTER TABLE t1 MODIFY COLUMN a ENUM('a1','a2','xx','a5');
ALTER TABLE t1 MODIFY COLUMN a ENUM('a1','a2','xx');
ALTER TABLE t1 MODIFY COLUMN a ENUM('a1','a2','a0','xx');
ALTER TABLE t1 MODIFY COLUMN a ENUM('a1','a2','a0','xx','a5','a6');
select * from t1;
DROP TABLE t1;

--
-- modify, drop, add
-- 
create table t1 (i int, j int) engine=myisam;
insert into t1 value (1, 2);
select * from t1;
alter table t1 modify column j int first;
select * from t1;
alter table t1 drop column i, add column k int default 0;
select * from t1;
drop table t1;

--
-- default ''
-- 
CREATE TABLE t1 (c1 int unsigned , c2 char(100) not null default '');
ALTER TABLE t1 ADD c3 char(16) NOT NULL DEFAULT '' AFTER c2,
               MODIFY c2 char(100) NOT NULL DEFAULT '' AFTER c1;
SHOW CREATE TABLE t1;
DROP TABLE t1;

--------------------- file: alter_table.test
-- #
-- # Bug#18038  MySQL server corrupts binary columns data
-- #
CREATE TABLE t1 (s CHAR(8) BINARY);
INSERT INTO t1 VALUES ('test');
SELECT LENGTH(s) FROM t1;
ALTER TABLE t1 MODIFY s CHAR(10) BINARY;
SELECT LENGTH(s) FROM t1;
DROP TABLE t1;

CREATE TABLE t1 (s BINARY(8));
INSERT INTO t1 VALUES ('test');
SELECT LENGTH(s) FROM t1;
SELECT HEX(s) FROM t1;
ALTER TABLE t1 MODIFY s BINARY(10);
SELECT HEX(s) FROM t1;
SELECT LENGTH(s) FROM t1;
DROP TABLE t1;

-- #
-- # BUG#29957 - alter_table.test fails
-- #
create table t1 (c char(10) default 'Two');
lock table t1 write;
insert into t1 values ();
alter table t1 modify c char(10) default 'Three';
unlock tables;
select * from t1;
check table t1;
drop table t1;

--------------------- auto_increment
-- file: auto_increment.test
-- #
-- # Test of behavior of ALTER TABLE when coulmn containing NULL or zeroes is
-- # converted to AUTO_INCREMENT column
-- #
create table t1 (a bigint);
insert into t1 values (1), (2), (3), (NULL), (NULL);
alter table t1 modify a bigint not null auto_increment primary key; 
select * from t1;
drop table t1;

create table t1 (a bigint);
insert into t1 values (1), (2), (3), (0), (0);
alter table t1 modify a bigint not null auto_increment primary key; 
select * from t1;
drop table t1;

-- # We still should be able to preserve zero in NO_AUTO_VALUE_ON_ZERO mode
create table t1 (a bigint);
insert into t1 values (0), (1), (2), (3);
set sql_mode=NO_AUTO_VALUE_ON_ZERO;
alter table t1 modify a bigint not null auto_increment primary key; 
set sql_mode= '';
select * from t1;
drop table t1;

-- # It also sensible to preserve zeroes if we are converting auto_increment
-- # column to auto_increment column (or not touching it at all, which is more
-- # common case probably)
create table t1 (a int auto_increment primary key , b int null);
set sql_mode=NO_AUTO_VALUE_ON_ZERO;
insert into t1 values (0,1),(1,2),(2,3);
select * from t1;
set sql_mode= '';
alter table t1 modify b varchar(255);
insert into t1 values (0,4);
select * from t1;
drop table t1;
