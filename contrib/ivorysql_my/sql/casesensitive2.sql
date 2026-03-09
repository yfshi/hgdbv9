/* The situation when lower_case_table_names is 2 */
--preparation
set ivorysql.compatible_mode to mysql;
set mysql.enable_case_switch to on;
set lower_case_table_names = 2;

/*
 * test schema and database
 */
create schema TESTSCHEMA2;
create schema testschema2; --err
create schema MYSQL_TEST;

--select objname, caseobjname, refclassid, objkind from pg_object;
alter schema TESTSCHEMA2 rename to newTESTSCHEMA2;

--select objname, caseobjname, refclassid, objkind from pg_object;
alter schema NEWTESTSCHEMA2 rename to TESTSCHEMA2;

/* 
 * test table.
 */
create table TEST2(Id INT, name TEXT);
create table test2(Id int, name text); --err
insert into test2 values (1, 'zhangsanfeng');
insert into Test2 values (2, 'songyuanqiao');

--Specify schema creation table
create table TESTschema2.TB2(ID INT);
select * from testschema2.tb2;
drop table testschema2.tb2;

--query
select * from test2;
select id, NAME from test2;
--select objname, caseobjname, refclassid, objkind from pg_myobject;

--rename
alter table test2 rename to test2;
--select objname, caseobjname, refclassid, objkind from pg_myobject;
alter table test2 rename to test2;
--select objname, caseobjname, refclassid, objkind from pg_myobject;

--alter schema
alter table test2 set schema TESTSCHEMA2;
--select objname, caseobjname, refclassid, objkind from pg_myobject;

select * from TESTSCHEMA2.test2;

alter table testschema2.test2 set schema public;
alter table test2 set schema testschema2;
--select objname, caseobjname, refclassid, objkind from pg_myobject;
alter table TESTSCHEMA2.test2 set schema public;

--select objname, caseobjname, refclassid, objkind from pg_myobject;
select * from PUBLIC.Test2;

/*
 * test view.
 */
create view v_test2 as select * from teST2;

--query
select * from v_test2;
select * from v_test2;
--select objname, caseobjname, refclassid, objkind from pg_myobject;

create view v_TEST2 as select * from teST2; --err
--rename view
alter view v_test2 rename to v_test22;
--select objname, caseobjname, refclassid, objkind from pg_myobject;
--alter view schema
alter view v_test22 set schema Mysql_Test;
--select objname, caseobjname, refclassid, objkind from pg_myobject;
select * from MYSQL_TEST.v_test22;
--rename view
alter view MYSQL_TEST.v_test22 rename to v_test2;
--select objname, caseobjname, refclassid, objkind from pg_myobject;
--alter view schema
alter view MYSQL_TEST.v_test2 set schema PUBLIC; 

--select objname, caseobjname, refclassid, objkind from pg_myobject;
insert into v_test2 values (3, 'yulianzhou');
select * from test2;

--drop object
drop view v_test2;
drop table test2;
drop schema TESTSCHEMA2;
drop schema MYSQL_TEST;
drop schema testschema2 cascade;

--create table and index
create table test2(sId int, sName text collate utf8mb4_general_ci);
create index idx_Text on test2(sname);

--insert data
insert into test2 select generate_series(1,100000) as i, 'zhangsan' operator(pg_catalog.||) generate_series(1,100000);
insert into test2 values (100001, 'zhangsan99999');
insert into test2 values (100002, 'ZHANGsan99999');

--query table
select * from test2 where sNAme = 'zhangsan99999';
explain (costs off) select * from test2 where sNAme = 'zhangsan99999';

--drop table
drop table test2;

CREATE SCHEMA sc;
CREATE SEQUENCE SERIAL START 101;
CREATE SEQUENCE sc.SERIAL START 101;
CREATE TABLE TESTB(id int, name text);
CREATE SEQUENCE S_SEQQ MINVALUE 1 START WITH 1 OWNED BY TESTB.id;
CREATE TABLE sc.TESTTB(id int, name text);
CREATE SEQUENCE sc.S_SEQTB MINVALUE 1 START WITH 1 OWNED BY sc.TESTTB.id;
CREATE TABLE sC.TESTTB1(id int, name text);

insert into sc.TESTTB values (nextval('sc.S_SEQTB'), 'aaaaa');
insert into sc.TESTTB values (nextval('sc.S_SEQTB'), 'bbb');
insert into sc.TESTTB values (nextval('sc.S_SEQTB'), 'ccccc');
insert into sc.TESTTB values (nextval('sc.S_SEQTb'), 'ddddd');
select currval('sc.S_SEQTB');

select nextval('SERIAL');
select currval('SERIAL');

select nextval('sc.SERIAL');
select nextval('sc.SERIAL');
select currval('sc.SERIAL');

select nextval('S_SEQQ');
select nextval('S_SEQQ');
select nextval('S_SEQQ');
select currval('S_SEQQ');

DROP SEQUENCE SERIAL;
DROP SEQUENCE sc.SERIAL;
DROP SEQUENCE S_SEQQ;
DROP TABLE TESTB;
DROP SEQUENCE sc.S_SEQTB;
DROP TABLE sc.TESTTB;
DROP TABLE sC.TESTTB1;
DROP SCHEMA sc;


CREATE SCHEMA sC;
CREATE SEQUENCE SERIAL START 101;
CREATE SEQUENCE sC.SERIAL START 101;
CREATE TABLE TESTB(id int, name text);
CREATE SEQUENCE S_SEQQ MINVALUE 1 START WITH 1 OWNED BY TESTB.id;
CREATE TABLE sC.TESTTB(id int, name text);
CREATE SEQUENCE sC.S_SEQTB MINVALUE 1 START WITH 1 OWNED BY sC.TESTTB.id;
CREATE TABLE sc.TESTTB1(id int, name text);

insert into sC.TESTTB values (nextval('sC.S_SEQTB'), 'aaaaa');
insert into sC.TESTTB values (nextval('sC.S_SEQTB'), 'bbb');
insert into sC.TESTTB values (nextval('sC.S_SEQTB'), 'ccccc');
insert into sC.TESTTB values (nextval('sC.S_SEQTb'), 'ddddd');
select currval('sC.S_SEQTB');

select nextval('SERIAL');
select currval('SERIAL');

select nextval('sC.SERIAL');
select nextval('sC.SERIAL');
select currval('sC.SERIAL');

select nextval('S_SEQQ');
select nextval('S_SEQQ');
select nextval('S_SEQQ');
select currval('S_SEQQ');

DROP SEQUENCE SERIAL;
DROP SEQUENCE sC.SERIAL;
DROP SEQUENCE S_SEQQ;
DROP TABLE TESTB;
DROP SEQUENCE sC.S_SEQTB;
DROP TABLE sC.TESTTB;
DROP TABLE sc.TESTTB1;
DROP SCHEMA sC;


CREATE SCHEMA Sc;
CREATE SEQUENCE SERIAL START 101;
CREATE SEQUENCE Sc.SERIAL START 101;
CREATE TABLE TESTB(id int, name text);
CREATE SEQUENCE S_SEQQ MINVALUE 1 START WITH 1 OWNED BY TESTB.id;
CREATE TABLE Sc.TESTTB(id int, name text);
CREATE SEQUENCE Sc.S_SEQTB MINVALUE 1 START WITH 1 OWNED BY Sc.TESTTB.id;
CREATE TABLE SC.TESTTB1(id int, name text);

insert into Sc.TESTTB values (nextval('Sc.S_SEQTB'), 'aaaaa');
insert into Sc.TESTTB values (nextval('Sc.S_SEQTB'), 'bbb');
insert into Sc.TESTTB values (nextval('Sc.S_SEQTB'), 'ccccc');
insert into Sc.TESTTB values (nextval('Sc.S_SEQTb'), 'ddddd');
select currval('Sc.S_SEQTB');

select nextval('SERIAL');
select currval('SERIAL');

select nextval('Sc.SERIAL');
select nextval('Sc.SERIAL');
select currval('Sc.SERIAL');

select nextval('S_SEQQ');
select nextval('S_SEQQ');
select nextval('S_SEQQ');
select currval('S_SEQQ');

DROP SEQUENCE SERIAL;
DROP SEQUENCE Sc.SERIAL;
DROP SEQUENCE S_SEQQ;
DROP TABLE TESTB;
DROP SEQUENCE Sc.S_SEQTB;
DROP TABLE Sc.TESTTB;
DROP TABLE SC.TESTTB1;
DROP SCHEMA Sc;



CREATE SCHEMA SC;
CREATE SEQUENCE SERIAL START 101;
CREATE SEQUENCE SC.SERIAL START 101;
CREATE TABLE TESTB(id int, name text);
CREATE SEQUENCE S_SEQQ MINVALUE 1 START WITH 1 OWNED BY TESTB.id;
CREATE TABLE SC.TESTTB(id int, name text);
CREATE SEQUENCE SC.S_SEQTB MINVALUE 1 START WITH 1 OWNED BY SC.TESTTB.id;
CREATE TABLE sC.TESTTB(id int, name text);  --err

insert into SC.TESTTB values (nextval('SC.S_SEQTB'), 'aaaaa');
insert into SC.TESTTB values (nextval('SC.S_SEQTB'), 'bbb');
insert into SC.TESTTB values (nextval('SC.S_SEQTB'), 'ccccc');
insert into SC.TESTTB values (nextval('SC.S_SEQTb'), 'ddddd');
select currval('SC.S_SEQTB');

select nextval('SERIAL');
select currval('SERIAL');

select nextval('SC.SERIAL');
select nextval('SC.SERIAL');
select currval('SC.SERIAL');

select nextval('S_SEQQ');
select nextval('S_SEQQ');
select nextval('S_SEQQ');
select currval('S_SEQQ');

DROP SEQUENCE SERIAL;
DROP SEQUENCE SC.SERIAL;
DROP SEQUENCE S_SEQQ;
DROP TABLE TESTB;
DROP SEQUENCE SC.S_SEQTB;
DROP TABLE SC.TESTTB;
DROP SCHEMA SC;


CREATE SCHEMA sc;
CREATE SCHEMA sC;	--err
CREATE SCHEMA Sc;	--err
CREATE SCHEMA SC;	--err

CREATE SEQUENCE sc.SERIAL START 101;
CREATE SEQUENCE sC.SERIAL START 101;	--err
CREATE SEQUENCE Sc.SERIAL START 101;	--err
CREATE SEQUENCE SC.SERIAL START 101;	--err


DROP SCHEMA sc cascade;
DROP SCHEMA sC cascade;		--err 
DROP SCHEMA Sc cascade;		--err
DROP SCHEMA SC cascade;		--err


--rename sequence and set new schema
CREATE SEQUENCE SERIAL START 101;
select count(*) from pg_object where objname = 'SERIAL'; --1
select count(*) from pg_object where objname = 'serial'; --0
select nextval('SERIAL');
select nextval('serial');
select currval('SERIAL');

alter SEQUENCE SERIAL rename to NEWSEQUENCE;
select count(*) from pg_object where objname = 'newsequence';
select nextval('NEWSEQUENCE');
select currval('NEWSEQUENCE');

create schema SC;
alter SEQUENCE NEWSEQUENCE set schema sc;
alter SEQUENCE sc.NEWSEQUENCE set schema SC;
select count(*) from pg_object ob, pg_namespace n where ob.objname = 'NEWSEQUENCE' and ob.schemaoid = n.oid and n.nspname = 'sc'; --1
select count(*) from pg_object ob, pg_namespace n where ob.objname = 'NEWSEQUENCE' and ob.schemaoid = n.oid and n.nspname = 'SC'; --0
select nextval('SC.NEWSEQUENCE');
select currval('SC.NEWSEQUENCE');

DROP SEQUENCE sc.NEWSEQUENCE;
DROP SEQUENCE SC.NEWSEQUENCE;	--err
DROP SCHEMA SC;




reset ivorysql.compatible_mode;
reset mysql.enable_case_switch;
reset lower_case_table_names;