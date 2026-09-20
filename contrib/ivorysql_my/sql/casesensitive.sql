/*
 * MySQL case sensitive test.
 */
--parameter settings 
set ivorysql.compatible_mode to mysql;

/*
 * test function and procedure
 */
---lower_case_table_names is 0
set lower_case_table_names = 0;
--
--create function
--same name function
CREATE FUNCTION TEST_ADD(integer, integer) RETURNS integer
AS 'select $1 + $2;'
LANGUAGE SQL
IMMUTABLE
RETURNS NULL ON NULL INPUT;

--err, function test_add exist
CREATE FUNCTION test_add(integer, integer, integer) RETURNS integer
AS 'select $1 + $2 + $3;'
LANGUAGE SQL
IMMUTABLE
RETURNS NULL ON NULL INPUT;

--use function test_add
select test_add(1, 2);
select test_add(1, 2, 3);

--alter function schema
create schema mysqlt;
alter function test_add(integer, integer) set schema mysqlt;
--select objname, caseobjname, refclassid, schemaoid, objkind from pg_myobject;

--use function test_add
select mysqlt.test_add(1, 2);
select mysqlt.test_add(1, 2, 3); --err
select MYSQLT.TEST_add(123, 456); --err
select mysqlT.TEST_add(123, 456); --err

--alter function name
alter function mysqlT.test_add rename to test_newadd; --err
alter function mysqlt.test_add rename to test_newadd;

--query function
--select objname, caseobjname, refclassid, schemaoid, objkind from pg_object;
select mysqlt.test_newadd(1, 2);
select mysqlt.test_newadd(1, 2, 3); --err
select MYSQLT.TEST_NEWadd(123, 456); --err
select mysqlT.TEST_NEWadd(123, 456); --err

--drop function
drop function MYSQLT.test_newadd; --err
drop function mysqlt.test_newadd;

--
--test procedure
create table test(sId int, sName varchar(30));

CREATE PROCEDURE insert_to_test(a integer, b varchar(30))
LANGUAGE SQL
BEGIN ATOMIC
INSERT INTO test VALUES (a, b);
END;

CREATE PROCEDURE update_to_test(a integer, b varchar(30))
LANGUAGE SQL
BEGIN ATOMIC
update test set sname = b where sid = a;
END;

call insert_to_test(1, 'zhangsanfeng');
select * from test;
call update_to_test(1, 'zhangsanfeng1');
select * from test;

--alter procedure name
alter procedure  INSERT_TO_TEST rename to INSERT_to_newtest;
alter procedure  UPDATE_TO_TEST rename to UPDATE_to_newtest;

--query data and call procedure
--select objname, caseobjname, refclassid, schemaoid, objkind from pg_object;
call insert_to_test(2, 'yulianzhou'); --err
call insert_to_newtest(2, 'yulianzhou');
select * from test;
call update_to_test(2, 'yudaiyan'); --err
call update_to_newtest(2, 'yudaiyan');
select * from test;

--alter procedure schema
alter procedure insert_to_newtest set schema MYSQLT; --err
alter procedure insert_to_newtest set schema mysqlt;
alter procedure update_to_newtest set schema MYSQLT; --err
alter procedure update_to_newtest set schema mysqlt;

--query data and call procedure
--select objname, caseobjname, refclassid, schemaoid, objkind from pg_object;
call mysqlt.insert_to_test(3, 'zhangsongxi'); --err
call mysqlt.insert_to_newtest(3, 'zhangsongxi');
select * from test;
call mysqlt.update_to_test(3, 'zhangsongxi'); --err
call mysqlt.update_to_newtest(2, 'zhangcuishan');
select * from test;

--drop procedure
drop procedure MYSQLT.insert_to_newtest; --err
drop procedure MYSQLT.update_to_newtest; --err
drop procedure mysqlt.INSERT_TO_NEWTEST;
drop procedure mysqlt.update_to_newtest;

--drop table
drop table test;

--drop schema
drop schema MYSQLT; --err
drop schema mysqlt;


---lower_case_table_names is 1
set lower_case_table_names = 1;
--
--create function
--same name function
CREATE FUNCTION TEST_ADD(integer, integer) RETURNS integer
AS 'select $1 + $2;'
LANGUAGE SQL
IMMUTABLE
RETURNS NULL ON NULL INPUT;

--err, function test_add exist
CREATE FUNCTION test_add(integer, integer, integer) RETURNS integer
AS 'select $1 + $2 + $3;'
LANGUAGE SQL
IMMUTABLE
RETURNS NULL ON NULL INPUT;

--use function test_add
select test_add(1, 2);
select test_add(1, 2, 3);

--alter function schema
create schema mysqlt;
alter function test_add(integer, integer) set schema mysqlt;
select objname, refclassid, objkind from pg_object where objname in ('test_add', 'mysqlt', 'TEST_ADD') order by refclassid, objname;

--use function test_add
select mysqlt.test_add(1, 2);
select mysqlt.test_add(1, 2, 3); --err
select MYSQLT.TEST_add(123, 456);
select mysqlT.TEST_add(123, 456);

--alter function name
alter function mysqlT.test_add rename to test_newadd;

--query function
--select objname, caseobjname, refclassid, schemaoid, objkind from pg_object;
select mysqlt.test_newadd(1, 2);
select mysqlt.test_newadd(1, 2, 3); --err
select MYSQLT.TEST_NEWadd(123, 456);
select mysqlT.TEST_NEWadd(123, 456);

--drop function
drop function mysqlt.test_newadd;

--
--test procedure
create table test(sId int, sName varchar(30));

CREATE PROCEDURE insert_to_test(a integer, b varchar(30))
LANGUAGE SQL
BEGIN ATOMIC
INSERT INTO test VALUES (a, b);
END;

CREATE PROCEDURE update_to_test(a integer, b varchar(30))
LANGUAGE SQL
BEGIN ATOMIC
update test set sname = b where sid = a;
END;

call insert_to_test(1, 'zhangsanfeng');
select * from test;
call update_to_test(1, 'zhangsanfeng1');
select * from test;

--alter procedure name
alter procedure  INSERT_TO_TEST rename to INSERT_to_newtest;
alter procedure  UPDATE_TO_TEST rename to UPDATE_to_newtest;

--query data and call procedure
--select objname, caseobjname, refclassid, schemaoid, objkind from pg_object;
call insert_to_test(2, 'yulianzhou'); --err
call insert_to_newtest(2, 'yulianzhou');
select * from test;
call update_to_test(2, 'yudaiyan'); --err
call update_to_newtest(2, 'yudaiyan');
select * from test;

--alter procedure schema
alter procedure insert_to_newtest set schema MYSQLT;
alter procedure insert_to_newtest set schema mysqlt; --err
alter procedure update_to_newtest set schema MYSQLT;
alter procedure update_to_newtest set schema mysqlt; --err

--query data and call procedure
--select objname, caseobjname, refclassid, schemaoid, objkind from pg_object;
call mysqlt.insert_to_test(3, 'zhangsongxi'); --err
call mysqlt.insert_to_newtest(3, 'zhangsongxi');
select * from test;
call mysqlt.update_to_test(3, 'zhangsongxi'); --err
call mysqlt.update_to_newtest(2, 'zhangcuishan');
select * from test;

--drop procedure
drop procedure MYSQLT.insert_to_newtest;
drop procedure MYSQLT.update_to_newtest;
drop procedure mysqlt.INSERT_TO_NEWTEST; --err
drop procedure mysqlt.update_to_newtest; --err

--drop table
drop table test;

--drop schema
drop schema MYSQLT;



---lower_case_table_names is 2
set lower_case_table_names = 2;
--
--create function
--same name function
CREATE FUNCTION TEST_ADD(integer, integer) RETURNS integer
AS 'select $1 + $2;'
LANGUAGE SQL
IMMUTABLE
RETURNS NULL ON NULL INPUT;

--err, function test_add exist
CREATE FUNCTION test_add(integer, integer, integer) RETURNS integer
AS 'select $1 + $2 + $3;'
LANGUAGE SQL
IMMUTABLE
RETURNS NULL ON NULL INPUT;

--use function test_add
select test_add(1, 2);
select test_add(1, 2, 3);

--alter function schema
create schema mysqlt;
alter function test_add(integer, integer) set schema mysqlt;
select objname, refclassid, objkind from pg_object where objname in ('test_add', 'mysqlt', 'TEST_ADD') order by refclassid, objname;

--use function test_add
select mysqlt.test_add(1, 2);
select mysqlt.test_add(1, 2, 3); --err
select MYSQLT.TEST_add(123, 456);
select mysqlT.TEST_add(123, 456);

--alter function name
alter function mysqlT.test_add rename to test_newadd;

--query function
--select objname, caseobjname, refclassid, schemaoid, objkind from pg_object;
select mysqlt.test_newadd(1, 2);
select mysqlt.test_newadd(1, 2, 3); --err
select MYSQLT.TEST_NEWadd(123, 456);
select mysqlT.TEST_NEWadd(123, 456);

--drop function
drop function mysqlt.test_newadd;


--
--test procedure
create table test(sId int, sName varchar(30));

CREATE PROCEDURE insert_to_test(a integer, b varchar(30))
LANGUAGE SQL
BEGIN ATOMIC
INSERT INTO test VALUES (a, b);
END;

CREATE PROCEDURE update_to_test(a integer, b varchar(30))
LANGUAGE SQL
BEGIN ATOMIC
update test set sname = b where sid = a;
END;

call insert_to_test(1, 'zhangsanfeng');
select * from test;
call update_to_test(1, 'zhangsanfeng1');
select * from test;

--alter procedure name
alter procedure  INSERT_TO_TEST rename to INSERT_to_newtest;
alter procedure  UPDATE_TO_TEST rename to UPDATE_to_newtest;

--query data and call procedure
--select objname, caseobjname, refclassid, schemaoid, objkind from pg_object;
call insert_to_test(2, 'yulianzhou'); --err
call insert_to_newtest(2, 'yulianzhou');
select * from test;
call update_to_test(2, 'yudaiyan'); --err
call update_to_newtest(2, 'yudaiyan');
select * from test;

--alter procedure schema
alter procedure insert_to_newtest set schema MYSQLT;
alter procedure insert_to_newtest set schema mysqlt; --err
alter procedure update_to_newtest set schema MYSQLT;
alter procedure update_to_newtest set schema mysqlt; --err

--query data and call procedure
--select objname, caseobjname, refclassid, schemaoid, objkind from pg_object;
call mysqlt.insert_to_test(3, 'zhangsongxi'); --err
call mysqlt.insert_to_newtest(3, 'zhangsongxi');
select * from test;
call mysqlt.update_to_test(3, 'zhangsongxi'); --err
call mysqlt.update_to_newtest(2, 'zhangcuishan');
select * from test;

--drop procedure
drop procedure MYSQLT.insert_to_newtest;
drop procedure MYSQLT.update_to_newtest;
drop procedure mysqlt.INSERT_TO_NEWTEST; --err
drop procedure mysqlt.update_to_newtest; --err

--drop table
drop table test;

--drop schema
drop schema MYSQLT;



------------------------------------------------
-- test collate(BTREE)
-- Create a table and specify the sorting method
create table test(sId int, sName text collate "binary");

--create index on test
create index idx_Text on test(sname);

--insert data into test
insert into test select generate_series(1,100000) as i, 'zhangsan' operator(pg_catalog.||) generate_series(1,100000);
insert into test values (100001, 'zhangsan99999');
insert into test values (100002, 'ZHANGsan99999');

--query data from test
select COUNT(*) from test;
select * from test where SNAME = 'zhangsan99999';

--explain
explain (costs off) select * from test where SNAME = 'zhangsan99999';

drop table test;

------------------------------------------------
-- Create a table and specify the sorting method
create table test(sId int, sName text collate "utf8mb4_general_ci");

--create index on test
create index idx_Text on test(sname);

--insert data into test
insert into test select generate_series(1,100000) as i, 'zhangsan' operator(pg_catalog.||) generate_series(1,100000);
insert into test values (100001, 'zhangsan99999');
insert into test values (100002, 'ZHANGsan99999');

--query data from test
select COUNT(*) from test;
select * from test where SNAME = 'zhangsan99999';

--explain
explain (costs off) select * from test where SNAME = 'zhangsan99999';

drop table test;

------------------------------------------------
-- Create a table and specify the sorting method
create table test(sId int, sName text collate "utf8mb4_unicode_ci");

--create index on test
create index idx_Text on test(sname);

--insert data into test
insert into test select generate_series(1,100000) as i, 'zhangsan' operator(pg_catalog.||) generate_series(1,100000);
insert into test values (100001, 'zhangsan99999');
insert into test values (100002, 'ZHANGsan99999');

--query data from test
select COUNT(*) from test;
select * from test where SNAME = 'zhangsan99999';

--explain
explain (costs off) select * from test where SNAME = 'zhangsan99999';

drop table test;

------------------------------------------------
-- Create a table and specify the sorting method
create table test(sId int, sName text collate "utf8mb4_bin");

--create index on test
create index idx_Text on test(sname);

--insert data into test
insert into test select generate_series(1,100000) as i, 'zhangsan' operator(pg_catalog.||) generate_series(1,100000);
insert into test values (100001, 'zhangsan99999');
insert into test values (100002, 'ZHANGsan99999');

--query data from test
select COUNT(*) from test;
select * from test where SNAME = 'zhangsan99999';

--explain
explain (costs off) select * from test where SNAME = 'zhangsan99999';

drop table test;

------------------------------------------------
-- Create a table and specify the sorting method
create table test(sId int, sName text collate "utf8_general_ci");

--create index on test
create index idx_Text on test(sname);
--insert data into test
insert into test select generate_series(1,100000) as i, 'zhangsan' operator(pg_catalog.||) generate_series(1,100000);
insert into test values (100001, 'zhangsan99999');
insert into test values (100002, 'ZHANGsan99999');

--query data from test
select COUNT(*) from test;
select * from test where SNAME = 'zhangsan99999';

--explain
explain (costs off) select * from test where SNAME = 'zhangsan99999';

drop table test;

------------------------------------------------
-- Create a table and specify the sorting method
create table test(sId int, sName text collate "utf8_unicode_ci");

--create index on test
create index idx_Text on test(sname);

--insert data into test
insert into test select generate_series(1,100000) as i, 'zhangsan' operator(pg_catalog.||) generate_series(1,100000);
insert into test values (100001, 'zhangsan99999');
insert into test values (100002, 'ZHANGsan99999');

--query data from test
select COUNT(*) from test;
select * from test where SNAME = 'zhangsan99999';

--explain
explain (costs off) select * from test where SNAME = 'zhangsan99999';

drop table test;

------------------------------------------------
-- Create a table and specify the sorting method
create table test(sId int, sName text collate "utf8_bin");

--create index on test
create index idx_Text on test(sname);

--insert data into test
insert into test select generate_series(1,100000) as i, 'zhangsan' operator(pg_catalog.||) generate_series(1,100000);
insert into test values (100001, 'zhangsan99999');
insert into test values (100002, 'ZHANGsan99999');

--query data from test
select COUNT(*) from test;
select * from test where SNAME = 'zhangsan99999';

--explain
explain (costs off) select * from test where SNAME = 'zhangsan99999';

drop table test;


------------------------------------------------
-- test collate(HASH)
-- Create a table and specify the sorting method
create table test(sId int, sName text collate "binary");

--create index on test
create index idx_Text on test using hash(sname);

--insert data into test
insert into test select generate_series(1,100000) as i, 'zhangsan' operator(pg_catalog.||) generate_series(1,100000); --err
insert into test values (100001, 'zhangsan99999'); --err
insert into test values (100002, 'ZHANGsan99999'); --err

--query data from test
select COUNT(*) from test;
select * from test where SNAME = 'zhangsan99999';

--explain
explain (costs off) select * from test where SNAME = 'zhangsan99999';

drop table test;

------------------------------------------------
-- Create a table and specify the sorting method
create table test(sId int, sName text collate "utf8mb4_general_ci");

--create index on test
create index idx_Text on test using hash(sname);

--insert data into test
insert into test select generate_series(1,100000) as i, 'zhangsan' operator(pg_catalog.||) generate_series(1,100000);
insert into test values (100001, 'zhangsan99999');
insert into test values (100002, 'ZHANGsan99999');

--query data from test
select COUNT(*) from test;
select * from test where SNAME = 'zhangsan99999';

--explain
explain (costs off) select * from test where SNAME = 'zhangsan99999';

drop table test;

------------------------------------------------
-- Create a table and specify the sorting method
create table test(sId int, sName text collate "utf8mb4_unicode_ci");

--create index on test
create index idx_Text on test using hash(sname);

--insert data into test
insert into test select generate_series(1,100000) as i, 'zhangsan' operator(pg_catalog.||) generate_series(1,100000);
insert into test values (100001, 'zhangsan99999');
insert into test values (100002, 'ZHANGsan99999');

--query data from test
select COUNT(*) from test;
select * from test where SNAME = 'zhangsan99999';

--explain
explain (costs off) select * from test where SNAME = 'zhangsan99999';

drop table test;

------------------------------------------------
-- Create a table and specify the sorting method
create table test(sId int, sName text collate "utf8mb4_bin");

--create index on test
create index idx_Text on test using hash(sname);

--insert data into test
insert into test select generate_series(1,100000) as i, 'zhangsan' operator(pg_catalog.||) generate_series(1,100000);
insert into test values (100001, 'zhangsan99999');
insert into test values (100002, 'ZHANGsan99999');

--query data from test
select COUNT(*) from test;
select * from test where SNAME = 'zhangsan99999';

--explain
explain (costs off) select * from test where SNAME = 'zhangsan99999';

drop table test;

------------------------------------------------
-- Create a table and specify the sorting method
create table test(sId int, sName text collate "utf8_general_ci");

--create index on test
create index idx_Text on test using hash(sname);

--insert data into test
insert into test select generate_series(1,100000) as i, 'zhangsan' operator(pg_catalog.||) generate_series(1,100000);
insert into test values (100001, 'zhangsan99999');
insert into test values (100002, 'ZHANGsan99999');

--query data from test
select COUNT(*) from test;
select * from test where SNAME = 'zhangsan99999';

--explain
explain (costs off) select * from test where SNAME = 'zhangsan99999';

drop table test;

------------------------------------------------
-- Create a table and specify the sorting method
create table test(sId int, sName text collate "utf8_unicode_ci");

--create index on test
create index idx_Text on test using hash(sname);

--insert data into test
insert into test select generate_series(1,100000) as i, 'zhangsan' operator(pg_catalog.||) generate_series(1,100000);
insert into test values (100001, 'zhangsan99999');
insert into test values (100002, 'ZHANGsan99999');

--query data from test
select COUNT(*) from test;
select * from test where SNAME = 'zhangsan99999';

--explain
explain (costs off) select * from test where SNAME = 'zhangsan99999';

drop table test;

------------------------------------------------
-- Create a table and specify the sorting method
create table test(sId int, sName text collate "utf8_bin");

--create index on test
create index idx_Text on test using hash(sname);

--insert data into test
insert into test select generate_series(1,100000) as i, 'zhangsan' operator(pg_catalog.||) generate_series(1,100000);
insert into test values (100001, 'zhangsan99999');
insert into test values (100002, 'ZHANGsan99999');

--query data from test
select COUNT(*) from test;
select * from test where SNAME = 'zhangsan99999';

--explain
explain (costs off) select * from test where SNAME = 'zhangsan99999';

drop table test;




--schema
CREATE SCHEMA SC;
ALTER SCHEMA SC RENAME TO SCC;
DROP SCHEMA SCC;


--type
CREATE SCHEMA SC;
CREATE TYPE SC.TESTTPY as (sId int, sName text);
CREATE TYPE SC.testty as (sId int, sName text);
ALTER TYPE SC.TESTTPY RENAME TO TESTTPY1;
ALTER TYPE SC.testty RENAME TO testty1;
DROP TYPE SC.TESTTPY1;
DROP TYPE SC.testty1;
DROP SCHEMA SC;


--function
CREATE FUNCTION TEST_ADD(integer, integer) RETURNS integer
AS 'select $1 + $2;'
LANGUAGE SQL
IMMUTABLE
RETURNS NULL ON NULL INPUT;

CREATE SCHEMA SC;

CREATE FUNCTION SC.TEST_ADD(integer, integer) RETURNS integer
AS 'select $1 + $2;'
LANGUAGE SQL
IMMUTABLE
RETURNS NULL ON NULL INPUT;

ALTER FUNCTION TEST_ADD(integer, integer) RENAME TO TEST_ADD1;
ALTER FUNCTION SC.TEST_ADD RENAME TO TEST_ADD1;
DROP FUNCTION TEST_ADD1;
DROP FUNCTION SC.TEST_ADD1;
DROP SCHEMA SC;



--agg function
CREATE TYPE AA AS (id int, id1 int);

CREATE FUNCTION AA_ADD(aa, aa) RETURNS aa
AS 'select $1;'
LANGUAGE SQL
IMMUTABLE
RETURNS NULL ON NULL INPUT;

CREATE AGGREGATE SUM1 (aa)
(
sfunc = aa_add,
stype = aa,
initcond = '(0,0)'
);

CREATE SCHEMA SC;

CREATE AGGREGATE SC.SUM1 (aa)
(
sfunc = aa_add,
stype = aa,
initcond = '(0,0)'
);

ALTER AGGREGATE SUM1(aA) RENAME TO SUM11;
ALTER AGGREGATE  SC.SUM1(aA) RENAME TO SUM11;


DROP AGGREGATE SUM11(aa);
DROP AGGREGATE SC.SUM11(aa);
DROP FUNCTION aa_add;
DROP TYPE aa;
DROP SCHEMA SC;


--op
CREATE OPERATOR === (
LEFTARG = int,
RIGHTARG = int,
FUNCTION = int4eq,
COMMUTATOR = ===,
NEGATOR = !==,
HASHES, MERGES
);

CREATE SCHEMA SC;
ALTER OPERATOR ===(int, int) set SCHEMA SC;

drop OPERATOR SC.===(int, int);
DROP SCHEMA SC;


--opc
CREATE OPERATOR CLASS OPC_OPS2
DEFAULT FOR TYPE _int4 USING gist AS
OPERATOR 6 = (int4, int4);

CREATE SCHEMA SC;

ALTER OPERATOR CLASS OPC_OPS2 USING gist RENAME TO OPC_OPS22;
ALTER OPERATOR CLASS OPC_OPS22 USING gist SET SCHEMA SC;

DROP OPERATOR CLASS SC.OPC_OPS22 USING gist;
drop OPERATOR FAMILY OPC_OPS2 USING gist;
DROP SCHEMA SC;


--opf
CREATE OPERATOR FAMILY OPC_OPS using gist;
CREATE SCHEMA SC;

ALTER OPERATOR FAMILY OPC_OPS USING gist RENAME TO OPC_OPS1;
ALTER OPERATOR FAMILY OPC_OPS1 USING gist SET SCHEMA SC;

DROP OPERATOR FAMILY SC.OPC_OPS1 USING gist;
DROP SCHEMA SC;


--collation
--CREATE COLLATION COLL (locale = 'fr_FR.utf8');
--CREATE SCHEMA SC;
--ALTER COLLATION COLL RENAME TO COLL1;
--ALTER COLLATION COLL1 SET SCHEMA SC;

--DROP COLLATION SC.COLL1;
--DROP SCHEMA SC;


--conversion
CREATE CONVERSION MYCONV FOR 'LATIN1' TO 'UTF8' FROM iso8859_1_to_utf8;
CREATE SCHEMA SC;
ALTER CONVERSION MYCONV RENAME TO MYCONV1;
ALTER CONVERSION MYCONV1 SET SCHEMA SC;

DROP CONVERSION SC.MYCONV1;
DROP SCHEMA SC;



--table
CREATE TABLE TEST(id int, name text);
CREATE SEQUENCE s_seq MINVALUE 1 START WITH 1 OWNED BY TEST.id;
CREATE SCHEMA SC;

ALTER TABLE TEST RENAME TO TEST1;
ALTER TABLE TEST1 SET SCHEMA SC;

DROP TABLE SC.TEST1;
DROP SCHEMA SC;



CREATE TABLE PKSLOW_PERSON_R (  
age int not null,  
city varchar not null  
) PARTITION BY RANGE (age); 

CREATE TABLE PKSLOW_PERSON_R1 PARTITION OF PKSLOW_PERSON_R FOR VALUES FROM (MINVALUE) TO (10);  
CREATE TABLE PKSLOW_PERSON_R2 PARTITION OF PKSLOW_PERSON_R FOR VALUES FROM (11) TO (20);  
CREATE TABLE PKSLOW_PERSON_R3 PARTITION OF PKSLOW_PERSON_R FOR VALUES FROM (21) TO (30);  
CREATE TABLE PKSLOW_PERSON_R4 PARTITION OF PKSLOW_PERSON_R FOR VALUES FROM (31) TO (MAXVALUE);

CREATE SCHEMA SC;
ALTER TABLE PKSLOW_PERSON_R RENAME TO PKSLOW_PERSON_R11;
ALTER TABLE PKSLOW_PERSON_R11 SET SCHEMA SC;

DROP TABLE SC.PKSLOW_PERSON_R11;
DROP SCHEMA SC;


--foreign table
/*CREATE EXTENSION postgres_fdw ;
GRANT USAGE ON FOREIGN DATA WRAPPER postgres_fdw TO markbo;
CREATE SERVER foreign_server
        FOREIGN DATA WRAPPER postgres_fdw
        OPTIONS (host 'localhost', port '3306', dbname 'postgres');
GRANT USAGE ON FOREIGN SERVER foreign_server TO markbo;
CREATE USER MAPPING FOR markbo
        SERVER foreign_server
        OPTIONS (user 'fdw_user', password 'secret');
CREATE FOREIGN TABLE FOREIGN_WEATHER (
      city        varchar(80),
      temp_low    int,
      temp_high   int,
      prcp        real,
      date        date
  )
SERVER foreign_server
OPTIONS (schema_name 'public', table_name 'weather');

CREATE SCHEMA SC;

ALTER FOREIGN TABLE FOREIGN_WEATHER RENAME TO FOREIGN_WEATHER1;
ALTER FOREIGN TABLE FOREIGN_WEATHER1 SET SCHEMA SC;

DROP FOREIGN TABLE SC.FOREIGN_WEATHER1;
DROP SCHEMA SC;
DROP USER MAPPING FOR markbo SERVER foreign_server;
DROP SERVER foreign_server;
*/


--index
CREATE TABLE TEST(id int, sName text);
CREATE INDEX idxtest on TEST(sname);
CREATE SCHEMA SC;

ALTER TABLE TEST RENAME TO TEST1;
ALTER TABLE TEST1 SET SCHEMA SC;

DROP INDEX SC.idxtest;
DROP TABLE SC.TEST1;
DROP SCHEMA SC;


--index attach
CREATE TABLE PKSLOW_PERSON_R (  
age int not null,  
city varchar not null  
) PARTITION BY RANGE (age); 

CREATE TABLE PKSLOW_PERSON_R1 PARTITION OF PKSLOW_PERSON_R FOR VALUES FROM (MINVALUE) TO (10);  
CREATE TABLE PKSLOW_PERSON_R2 PARTITION OF PKSLOW_PERSON_R FOR VALUES FROM (11) TO (20);  
CREATE INDEX idxtest ON PKSLOW_PERSON_R(age);

CREATE SCHEMA SC;
ALTER TABLE PKSLOW_PERSON_R RENAME TO PKSLOW_PERSON_R11;
ALTER TABLE PKSLOW_PERSON_R11 SET SCHEMA SC;

DROP TABLE PKSLOW_PERSON_R1;
DROP TABLE PKSLOW_PERSON_R2;
DROP INDEX SC.idxtest;
DROP TABLE SC.PKSLOW_PERSON_R11;
DROP SCHEMA SC;


--statsext
CREATE TABLE T1 (
	a int,
	b int
);

INSERT INTO T1 SELECT i/100, i/500
FROM generate_series(1,100000) s(i);
ANALYZE T1;
-- the number of matching rows will be drastically underestimated:
--EXPLAIN SELECT * FROM T1 WHERE (a = 1) AND (b = 0);
CREATE STATISTICS S1 (dependencies) ON a, b FROM T1;
ANALYZE T1;
-- now the row count estimate is more accurate:
--EXPLAIN SELECT * FROM T1 WHERE (a = 1) AND (b = 0);

CREATE SCHEMA SC;

ALTER STATISTICS S1 RENAME TO S11;
ALTER STATISTICS S11 SET SCHEMA SC;

DROP STATISTICS SC.S11;
DROP TABLE T1;
DROP SCHEMA SC;





--refresh_matview
CREATE TABLE T1 (
	a int,
	b int
);
INSERT INTO T1 SELECT i/100, i/500
FROM generate_series(1,100) s(i);

CREATE MATERIALIZED VIEW MV_T1 AS SELECT * FROM T1;
REFRESH MATERIALIZED VIEW MV_T1;

CREATE SCHEMA SC;

ALTER MATERIALIZED VIEW MV_T1 RENAME TO MV_T11;
ALTER MATERIALIZED VIEW MV_T11 SET SCHEMA SC;
REFRESH MATERIALIZED VIEW SC.MV_T11;

DROP MATERIALIZED VIEW SC.MV_T11;
DROP TABLE T1;
DROP SCHEMA SC;



--rule
--trigger
--event trigger
CREATE SCHEMA SC;

CREATE OR REPLACE FUNCTION SC.ABORT_ANY_COMMAND()
RETURNS event_trigger
LANGUAGE plpgsql
AS $$
BEGIN
RAISE EXCEPTION 'command % is disabled', tg_tag;
END;
$$;

CREATE EVENT TRIGGER ABORT_DDL1 ON ddl_command_start
EXECUTE FUNCTION SC.ABORT_ANY_COMMAND();

ALTER EVENT TRIGGER ABORT_DDL1 RENAME TO ABORT_DDL2;

DROP EVENT TRIGGER ABORT_DDL2;
DROP FUNCTION SC.ABORT_ANY_COMMAND;
DROP SCHEMA SC;


--constraint
--cast
--transform

--sequence
CREATE SEQUENCE SERIAL START 101;
CREATE SCHEMA SC;

ALTER SEQUENCE SERIAL RENAME TO SERIAL1;
ALTER SEQUENCE SERIAL1 SET SCHEMA SC;

DROP SEQUENCE SC.SERIAL1;
DROP SCHEMA SC;



--text search
CREATE TEXT SEARCH DICTIONARY MY_RUSSIAN (
template = snowball,
language = russian,
stopwords = russian
);

CREATE SCHEMA SC;

ALTER TEXT SEARCH DICTIONARY MY_RUSSIAN RENAME TO MY_RUSSIAN1;
ALTER TEXT SEARCH DICTIONARY MY_RUSSIAN1 SET SCHEMA SC;

DROP TEXT SEARCH DICTIONARY SC.MY_RUSSIAN1;
DROP SCHEMA SC;


-- text search template
CREATE OR REPLACE FUNCTION TRANSFER(internal, internal,internal,internal)
 RETURNS internal
 LANGUAGE internal
 IMMUTABLE
AS $function$int84$function$;

CREATE TEXT SEARCH TEMPLATE NAME (
LEXIZE = TRANSFER
);

CREATE SCHEMA SC;

ALTER TEXT SEARCH TEMPLATE NAME RENAME TO NAME1;
ALTER TEXT SEARCH TEMPLATE NAME1 SET SCHEMA SC;


DROP TEXT SEARCH TEMPLATE SC.NAME1;
DROP FUNCTION TRANSFER(internal, internal,internal,internal);
DROP SCHEMA SC;


--tsconfig
CREATE TEXT SEARCH CONFIGURATION ALT_TS_CONF (copy=english);
CREATE SCHEMA SC;

ALTER TEXT SEARCH CONFIGURATION ALT_TS_CONF SET SCHEMA SC;

DROP TEXT SEARCH CONFIGURATION SC.ALT_TS_CONF;
DROP SCHEMA SC;


--tsparser
CREATE TEXT SEARCH PARSER ALT_TS_PRS1
    (start = prsd_start, gettoken = prsd_nexttoken, end = prsd_end, lextypes = prsd_lextype);
CREATE SCHEMA SC;

ALTER TEXT SEARCH PARSER ALT_TS_PRS1 RENAME TO ALT_TS_PRS11;
ALTER TEXT SEARCH PARSER ALT_TS_PRS11 SET SCHEMA SC;

DROP TEXT SEARCH PARSER SC.ALT_TS_PRS11;
DROP SCHEMA SC;

set lower_case_table_names = 0;
CREATE TABLE serialTest2 (f1 text, f2 serial, f3 smallserial, f4 serial2, f5 bigserial, f6 serial8);
SELECT nextval('serialTest2_f2_seq');
drop table serialTest2;


set lower_case_table_names = 1;

create table comysql_test(id int,name varchar,age int);
select * from public.comysql_test where public.COMYSQL_TEST.ID = 1;
delete from public.COMYSQL_TEST where public.COMYSQL_TEST.ID = 1;
update public.COMYSQL_TEST set public.COMYSQL_TEST.ID = 2 where public.COMYSQL_TEST.ID = 1;
SHOW COLUMNS FROM COMYSQL_TEST;
drop table COMYSQL_TEST;

set lower_case_table_names = 2;

create table comysql_test(id int,name varchar,age int);
select * from public.comysql_test where public.COMYSQL_TEST.ID = 1;
delete from public.COMYSQL_TEST where public.COMYSQL_TEST.ID = 1;
update public.COMYSQL_TEST set public.COMYSQL_TEST.ID = 2 where public.COMYSQL_TEST.ID = 1; 
drop table COMYSQL_TEST;


reset ivorysql.compatible_mode;
reset lower_case_table_names;