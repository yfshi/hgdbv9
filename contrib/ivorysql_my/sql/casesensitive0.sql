/*
 * MySQL case sensitive test.
 */
--parameter settings 
set ivorysql.compatible_mode to mysql;
set lower_case_table_names = 0;

--create schema
create schema TESTSCHEMA0;
create schema TESTschema0;
create schema testschema0;


--select oid, objname, caseobjname, refclassid, schemaoid, objkind from pg_myobject; --no data

--renmae schema
alter schema TESTSCHEMA0 rename to NEWTESTSCHEMA0;
alter schema TESTschema0 rename to NEWTESTschema0;
alter schema testschema0 rename to newtestschema0;


--select oid, objname, caseobjname, refclassid, schemaoid, objkind from pg_myobject; --no data


--create table, You can create tables with 
--the same name but different capitalization
--
create table TB0(sId int);
create table Tb0(sId int, sName text);
create table tB0(sId int, sName text, Hight INT);
create table tb0(sId int, sName text, Hight INT, Weight Int);

--insert data
insert into TB0 values (1);
insert into TB0 values (2);
insert into TB0 values (3,'zhangsanfeng'); --err

insert into Tb0 values (1, 'zhangsanfeng');
insert into Tb0 values (2, 'songyuanqiao');
insert into Tb0 values (3, 'yulianzhou', 180); --err

insert into tB0 values (1, 'zhangsanfeng', 180);
insert into tB0 values (2, 'songyuanqiao', 180);
insert into tB0 values (3, 'yulianzhou', 180, 90); --err

insert into tb0 values (1, 'zhangsanfeng', 180, 80);
insert into tb0 values (2, 'songyuanqiao', 180, 85);
insert into tb0 values (3, 'yulianzhou');

--query table
select * from TB0;  --two record
select * from Tb0;  --two record
select * from tB0;  --two record
select * from tb0;  --three record

select SID from TB0;
select SID, SNAME from Tb0;
select SID, SNAME, HIGHT from tB0;
select SID, SNAME, HIGHT, WEIGHT from tb0;
select sid from TB0;
select sid, sname from Tb0;
select sid, sname, hight from tB0;
select sid, sname, hight, weighT from tb0;


--alter table name;
alter table TB0 rename to NEWTB0;
alter table Tb0 rename to NEWTb0;
alter table tB0 rename to newtB0;
alter table tb0 rename to newtb0;

--query table
select * from NEWTB0;  --two record
select * from NEWTb0;  --two record
select * from newtB0;  --two record
select * from newtb0;  --three record

--use column query                       
select SID from NEWTB0;
select SID, SNAME from NEWTb0;
select SID, SNAME, HIGHT from newtB0;
select SID, SNAME, HIGHT, WEIGHT from newtb0;
select sid from NEWTB0;
select sid, sname from NEWTb0;
select sid, sname, hight from newtB0;
select sid, sname, hight, weight from newtb0;

--modify table schema
alter table NEWTB0 set schema newtestschema0;

--query
select * from NEWTB0; --err
select * from newtestschema0.NEWTB0;
select SID, sid, sID from newtestschema0.NEWTB0;
select SID, sid, sID from NEWTESTSCHEMA0.NEWTB0; --err

--create view
create view v_tb0 as select * from newtestschema0.NEWTB0;
create view V_tb0 as select * from newtestschema0.NEWTB0;
create view newtestschema0.V_tb0 as select * from newtestschema0.NEWTB0;

--query view
select * from v_tb0;
select SID, sid, sID  from v_tb0;

select * from V_tb0;
select SID, sid, sID  from V_tb0;

select * from newtestschema0.V_tb0;
select SID, sid, sID  from newtestschema0.V_tb0;

--modify view schema
alter view v_tb0 set schema newtestschema0;

--query view
select * from newtestschema0.v_tb0;
select SID, sid, sID  from newtestschema0.v_tb0;

select * from NEWTESTSCHEMA0.v_newtb; --err
select SID, sid, sID  from NEWTESTSCHEMA0.v_tb0; --err

--alter view name
alter view newtestschema0.v_tb0 rename to v_newtb;

--query view
select * from newtestschema0.v_tb0; --err
select * from newtestschema0.v_newtb;

--modify view schema
alter view newtestschema0.v_newtb set schema public;
alter view newtestschema0.V_tb0 set schema public; --err


--modify table schema
alter table newtestschema0.NEWTB0 set schema public;

--alter table name
alter table NEWTB0 rename to TB0; 
alter table NEWTb0 rename to Tb0;
alter table newtB0 rename to tB0;
alter table newtb0 rename to tb0;

--renmae schema
alter schema NEWTESTSCHEMA0 rename to TESTSCHEMA0;
alter schema NEWTESTschema0 rename to TESTschema0;
alter schema newtestschema0 rename to testschema0;   


--drop object
drop view newtestschema0.V_tb0;
drop view V_tb0;
drop view v_newtb;
drop view testschema0.V_tb0;
drop table TB0;
drop table Tb0;
drop table tB0;
drop table tb0;

drop schema TESTSCHEMA0;
drop schema TESTschema0;
drop schema testschema0;

--create table and index
create table test0(sId int, sName text collate utf8mb4_general_ci);
create index idx_Text on test0(sname);

--insert data
insert into test0 select generate_series(1,100000) as i, 'zhangsan' operator(pg_catalog.||) generate_series(1,100000);
insert into test0 values (100001, 'zhangsan99999');
insert into test0 values (100002, 'ZHANGsan99999');

--query table
select * from test0 where sNAme = 'zhangsan99999';
explain (costs off) select * from test0 where sNAme = 'zhangsan99999';

--drop table
drop table test0;


CREATE SCHEMA sc;
CREATE SEQUENCE SERIAL START 101;
CREATE SEQUENCE sc.SERIAL START 101;
CREATE TABLE TESTB(id int, name text);
CREATE SEQUENCE S_SEQQ MINVALUE 1 START WITH 1 OWNED BY TESTB.id;
CREATE TABLE sc.TESTTB(id int, name text);
CREATE SEQUENCE sc.S_SEQTB MINVALUE 1 START WITH 1 OWNED BY sc.TESTTB.id;
CREATE TABLE sC.TESTTB1(id int, name text); --err

insert into sc.TESTTB values (nextval('sc.S_SEQTB'), 'aaaaa');
insert into sc.TESTTB values (nextval('sc.S_SEQTB'), 'bbb');
insert into sc.TESTTB values (nextval('sc.S_SEQTB'), 'ccccc');
insert into sc.TESTTB values (nextval('sc.S_SEQTb'), 'ddddd'); --err
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
DROP SCHEMA sc;

--rename sequence and set new schema
CREATE SEQUENCE SERIAL START 101;
select count(*) from pg_object where objname = 'SERIAL';
select nextval('SERIAL');
select currval('SERIAL');

alter SEQUENCE SERIAL rename to NEWSEQUENCE;
select count(*) from pg_object where objname = 'NEWSEQUENCE';
select nextval('NEWSEQUENCE');
select currval('NEWSEQUENCE');

create schema sc;
alter SEQUENCE NEWSEQUENCE set schema SC; --err
alter SEQUENCE NEWSEQUENCE set schema sc;
select count(*) from pg_object ob, pg_namespace n where ob.objname = 'NEWSEQUENCE' and ob.schemaoid = n.oid and n.nspname = 'sc'; --1
select count(*) from pg_object ob, pg_namespace n where ob.objname = 'NEWSEQUENCE' and ob.schemaoid = n.oid and n.nspname = 'SC'; --0
select nextval('sc.NEWSEQUENCE');
select currval('sc.NEWSEQUENCE');

DROP SEQUENCE SC.NEWSEQUENCE;	--err
DROP SEQUENCE sc.NEWSEQUENCE;
DROP SCHEMA sc;


CREATE SCHEMA sC;
CREATE SEQUENCE SERIAL START 101;
CREATE SEQUENCE sC.SERIAL START 101;
CREATE TABLE TESTB(id int, name text);
CREATE SEQUENCE S_SEQQ MINVALUE 1 START WITH 1 OWNED BY TESTB.id;
CREATE TABLE sC.TESTTB(id int, name text);
CREATE SEQUENCE sC.S_SEQTB MINVALUE 1 START WITH 1 OWNED BY sC.TESTTB.id;
CREATE TABLE sc.TESTTB1(id int, name text);  --err

insert into sC.TESTTB values (nextval('sC.S_SEQTB'), 'aaaaa');
insert into sC.TESTTB values (nextval('sC.S_SEQTB'), 'bbb');
insert into sC.TESTTB values (nextval('sC.S_SEQTB'), 'ccccc');
insert into sC.TESTTB values (nextval('sC.S_SEQTb'), 'ddddd'); --err
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
DROP SCHEMA sC;



--rename sequence and set new schema
CREATE SEQUENCE SERIAL START 101;
select count(*) from pg_object where objname = 'SERIAL';
select nextval('SERIAL');
select currval('SERIAL');

alter SEQUENCE SERIAL rename to NEWSEQUENCE;
select count(*) from pg_object where objname = 'NEWSEQUENCE';
select nextval('NEWSEQUENCE');
select currval('NEWSEQUENCE');

create schema sC;
alter SEQUENCE NEWSEQUENCE set schema SC; --err
alter SEQUENCE NEWSEQUENCE set schema sC;
select count(*) from pg_object ob, pg_namespace n where ob.objname = 'NEWSEQUENCE' and ob.schemaoid = n.oid and n.nspname = 'sC'; --1
select count(*) from pg_object ob, pg_namespace n where ob.objname = 'NEWSEQUENCE' and ob.schemaoid = n.oid and n.nspname = 'SC'; --0
select nextval('sC.NEWSEQUENCE');
select currval('sC.NEWSEQUENCE');

DROP SEQUENCE SC.NEWSEQUENCE;	--err
DROP SEQUENCE sC.NEWSEQUENCE;
DROP SCHEMA sC;




CREATE SCHEMA Sc;
CREATE SEQUENCE SERIAL START 101;
CREATE SEQUENCE Sc.SERIAL START 101;
CREATE TABLE TESTB(id int, name text);
CREATE SEQUENCE S_SEQQ MINVALUE 1 START WITH 1 OWNED BY TESTB.id;
CREATE TABLE Sc.TESTTB(id int, name text);
CREATE SEQUENCE Sc.S_SEQTB MINVALUE 1 START WITH 1 OWNED BY Sc.TESTTB.id;
CREATE TABLE SC.TESTTB(id int, name text);  --err

insert into Sc.TESTTB values (nextval('Sc.S_SEQTB'), 'aaaaa');
insert into Sc.TESTTB values (nextval('Sc.S_SEQTB'), 'bbb');
insert into Sc.TESTTB values (nextval('Sc.S_SEQTB'), 'ccccc');
insert into Sc.TESTTB values (nextval('Sc.S_SEQTb'), 'ddddd'); --err
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
DROP SCHEMA Sc;


--rename sequence and set new schema
CREATE SEQUENCE SERIAL START 101;
select count(*) from pg_object where objname = 'SERIAL';
select nextval('SERIAL');
select currval('SERIAL');

alter SEQUENCE SERIAL rename to NEWSEQUENCE;
select count(*) from pg_object where objname = 'NEWSEQUENCE';
select nextval('NEWSEQUENCE');
select currval('NEWSEQUENCE');

create schema Sc;
alter SEQUENCE NEWSEQUENCE set schema SC; --err
alter SEQUENCE NEWSEQUENCE set schema Sc;
select count(*) from pg_object ob, pg_namespace n where ob.objname = 'NEWSEQUENCE' and ob.schemaoid = n.oid and n.nspname = 'Sc'; --1
select count(*) from pg_object ob, pg_namespace n where ob.objname = 'NEWSEQUENCE' and ob.schemaoid = n.oid and n.nspname = 'SC'; --0
select nextval('Sc.NEWSEQUENCE');
select currval('Sc.NEWSEQUENCE');

DROP SEQUENCE SC.NEWSEQUENCE;	--err
DROP SEQUENCE Sc.NEWSEQUENCE;
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
insert into SC.TESTTB values (nextval('SC.S_SEQTb'), 'ddddd'); --err
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



--rename sequence and set new schema
CREATE SEQUENCE SERIAL START 101;
select count(*) from pg_object where objname = 'SERIAL';
select nextval('SERIAL');
select currval('SERIAL');

alter SEQUENCE SERIAL rename to NEWSEQUENCE;
select count(*) from pg_object where objname = 'NEWSEQUENCE';
select nextval('NEWSEQUENCE');
select currval('NEWSEQUENCE');

create schema SC;
alter SEQUENCE NEWSEQUENCE set schema sc; --err
alter SEQUENCE NEWSEQUENCE set schema SC;
select count(*) from pg_object ob, pg_namespace n where ob.objname = 'NEWSEQUENCE' and ob.schemaoid = n.oid and n.nspname = 'SC'; --1
select count(*) from pg_object ob, pg_namespace n where ob.objname = 'NEWSEQUENCE' and ob.schemaoid = n.oid and n.nspname = 'sc'; --0
select nextval('SC.NEWSEQUENCE');
select currval('SC.NEWSEQUENCE');

DROP SEQUENCE sc.NEWSEQUENCE;	--err
DROP SEQUENCE SC.NEWSEQUENCE;
DROP SCHEMA SC;



---
CREATE SCHEMA sc;
CREATE SCHEMA sC;
CREATE SCHEMA Sc;
CREATE SCHEMA SC;

CREATE SEQUENCE sc.SERIAL START 101;
CREATE SEQUENCE sC.SERIAL START 101;
CREATE SEQUENCE Sc.SERIAL START 101;
CREATE SEQUENCE SC.SERIAL START 101;


DROP SCHEMA sc cascade;
DROP SCHEMA sC cascade;
DROP SCHEMA Sc cascade;
DROP SCHEMA SC cascade;


-- bug:z4315
-- ALTER TABLE TB RENAME COLUMN ID/Id/iD/id TO ID1_1/ Id2_2/ iD3_3/ id4_4;
create table TB(ID int, name varchar(30));
insert into TB values (1, 'zhangsan'), (2, 'lisi');
\d TB
ALTER TABLE TB RENAME COLUMN ID TO ID1_1;
\d TB
drop table TB;

create table TB(ID int, name varchar(30));
insert into TB values (1, 'zhangsan'), (2, 'lisi');
\d TB
ALTER TABLE TB RENAME COLUMN Id TO Id2_2;
\d TB
ALTER TABLE TB RENAME COLUMN ID2_2 TO iD3_3;
\d TB
ALTER TABLE TB RENAME COLUMN id3_3 TO id4_4;
\d TB
drop table TB;

--1
create table TB(ID int, name varchar(30));
\d TB
ALTER TABLE TB RENAME COLUMN ID TO ID1_1;
\d TB
drop table TB;

create table TB(ID int, name varchar(30));
\d TB
ALTER TABLE TB RENAME COLUMN ID TO Id2_2;
\d TB
drop table TB;

create table TB(ID int, name varchar(30));
\d TB
ALTER TABLE TB RENAME COLUMN ID TO iD3_3;
\d TB
drop table TB;

create table TB(ID int, name varchar(30));
\d TB
ALTER TABLE TB RENAME COLUMN ID TO id4_4;
\d TB
drop table TB;

--2
create table TB(ID int, name varchar(30));
\d TB TB
ALTER TABLE TB RENAME COLUMN Id TO ID1_1;
\d TB TB
drop table TB;

create table TB(ID int, name varchar(30));
\d TB
ALTER TABLE TB RENAME COLUMN Id TO Id2_2;
\d TB
drop table TB;

create table TB(ID int, name varchar(30));
\d TB
ALTER TABLE TB RENAME COLUMN Id TO iD3_3;
\d TB
drop table TB;

create table TB(ID int, name varchar(30));
\d TB
ALTER TABLE TB RENAME COLUMN Id TO id4_4;
\d TB
drop table TB;


--3
create table TB(ID int, name varchar(30));
\d TB
ALTER TABLE TB RENAME COLUMN iD TO ID1_1;
\d TB
drop table TB;

create table TB(ID int, name varchar(30));
\d TB
ALTER TABLE TB RENAME COLUMN iD TO Id2_2;
\d TB
drop table TB;

create table TB(ID int, name varchar(30));
\d TB
ALTER TABLE TB RENAME COLUMN iD TO iD3_3;
\d TB
drop table TB;

create table TB(ID int, name varchar(30));
\d TB
ALTER TABLE TB RENAME COLUMN iD TO id4_4;
\d TB
drop table TB;

--4
create table TB(ID int, name varchar(30));
\d TB
ALTER TABLE TB RENAME COLUMN id TO ID1_1;
\d TB
drop table TB;

create table TB(ID int, name varchar(30));
\d TB
ALTER TABLE TB RENAME COLUMN id TO Id2_2;
\d TB
drop table TB;

create table TB(ID int, name varchar(30));
\d TB
ALTER TABLE TB RENAME COLUMN id TO iD3_3;
\d TB
drop table TB;

create table TB(ID int, name varchar(30));
\d TB
ALTER TABLE TB RENAME COLUMN id TO id4_4;
\d TB
drop table TB;


--5
create table TB(ID int, name varchar(30));
\d TB
ALTER TABLE TB RENAME COLUMN ID TO id;
\d TB
ALTER TABLE TB RENAME COLUMN Id TO Id;
\d TB
ALTER TABLE TB RENAME COLUMN id TO id;
\d TB
drop table TB;



-- bug:z4316
create table TB(ID int, name varchar(30));
insert into TB values (1, 'zhangsan'), (2, 'lisi');
ALTER TABLE TB DROP COLUMN ID;
\d TB
drop table TB;

create table TB(ID int, name varchar(30));
\d TB
ALTER TABLE TB DROP COLUMN Id;
\d TB
drop table TB;

create table TB(ID int, name varchar(30));
\d TB
ALTER TABLE TB DROP COLUMN iD;
\d TB
drop table TB;

create table TB(ID int, name varchar(30));
\d TB
ALTER TABLE TB DROP COLUMN id;
\d TB
drop table TB;


--zijixz
create table TB(ID int, name varchar(30));
\d TB
ALTER TABLE TB add COLUMN IDD text, add COLUMN aaa text;
\d TB
drop table TB;


reset ivorysql.compatible_mode;
reset lower_case_table_names;