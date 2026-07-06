/* 
 * The situation when lower_case_table_names is 1 
 */
set ivorysql.compatible_mode to mysql;
set lower_case_table_names = 1;

--create schema
create schema TESTSCHEMA1;
create schema TESTschema1; --err
create schema testschema1; --err


--select oid, objname, caseobjname, refclassid, schemaoid, objkind from pg_object; --no data

--renmae schema
alter schema TESTSCHEMA1 rename to NEWTESTSCHEMA1;
alter schema TESTschema1 rename to NEWTESTschema1; --err
alter schema testschema1 rename to newtestschema1; --err


--select oid, objname, caseobjname, refclassid, schemaoid, objkind from pg_object; --no data

alter schema NEWTESTSCHEMA1 rename to TESTSCHEMA1;
alter schema NEWTESTschema1 rename to TESTschema1; --err
alter schema newtestschema1 rename to testschema1; --err

--select oid, objname, caseobjname, refclassid, schemaoid, objkind from pg_object; --no data

--create table, You can create tables with 
--the same name but different capitalization
--
create table TB1(sId int);
create table Tb1(sId int, sName text); --err
create table tB1(sId int, sName text, Hight INT); --err
create table tb1(sId int, sName text, Hight INT, Weight Int); --err

--insert data
insert into TB1 values (1);
insert into TB1 values (2);
insert into TB1 values (3,'zhangsanfeng'); --err

insert into Tb1 values (1, 'zhangsanfeng'); --err
insert into Tb1 values (2, 'songyuanqiao'); --err
insert into Tb1 values (3, 'yulianzhou', 180); --err

insert into tB1 values (1, 'zhangsanfeng', 180); --err
insert into tB1 values (2, 'songyuanqiao', 180); --err
insert into tB1 values (3, 'yulianzhou', 180, 90); --err

insert into tb1 values (1, 'zhangsanfeng', 180, 80); --err
insert into tb1 values (2, 'songyuanqiao', 180, 85); --err
insert into tb1 values (3, 'yulianzhou'); --err

--query table
select * from TB1;  --two record
select * from Tb1;  --two record
select * from tB1;  --two record
select * from tb1;  --two record

select SID from TB1;
select SID, SNAME from Tb1; --err
select SID, SNAME, HIGHT from tB1; --err
select SID, SNAME, HIGHT, WEIGHT from tb1; --err
select sid from TB1;
select sid, sname from Tb1; --err
select sid, sname, hight from tB1; --err
select sid, sname, hight, weighT from tb1; --err

--alter table name;
alter table TB1 rename to NEWTB1;
alter table Tb1 rename to NEWTb1; --err
alter table tB1 rename to newtB1; --err
alter table tb1 rename to newtb1; --err

--query table
select * from NEWTB1;  --two record
select * from NEWTb1;  --two record
select * from newtB1;  --two record
select * from newtb1;  --two record

--use column query                       
select SID from NEWTB1;
select SID, SNAME from NEWTb1; --err
select SID, SNAME, HIGHT from newtB1; --err
select SID, SNAME, HIGHT, WEIGHT from newtb1; --err
select sid from NEWTB1;
select sid, sname from NEWTb1; --err
select sid, sname, hight from newtB1; --err
select sid, sname, hight, weight from newtb1; --err

--modify table schema
alter table NEWTB1 set schema testschema1;

--query
select * from NEWTB1; --err
select * from testschema1.NEWTB1;
select SID, sid, sID from testschema1.NEWTB1;
select SID, sid, sID from TESTSCHEMA1.NEWTB1;

--create view
create view v_tb1 as select * from testschema1.NEWTB1;
create view V_tb1 as select * from testschema1.NEWTB1; --err
create view testschema1.V_tb1 as select * from testschema1.NEWTB1;

--query view
select * from v_tb1;
select SID, sid, sID  from v_tb1;

select * from V_tb1;
select SID, sid, sID  from V_tb1;

select * from testschema1.V_tb1;
select SID, sid, sID  from testschema1.V_tb1;

--modify view schema
alter view v_tb1 set schema testschema1; --err

--query view
select * from testschema1.v_tb1;
select SID, sid, sID  from testschema1.v_tb1;

select * from TESTSCHEMA1.v_newtb1; --err
select SID, sid, sID  from TESTSCHEMA1.v_tb1; --err

--alter view name
alter view testschema1.v_tb1 rename to v_newtb1;

--query view
select * from testschema1.v_tb1; --err
select * from testschema1.v_newtb1;

--modify view schema
alter view testschema1.v_newtb1 set schema public;
alter view testschema1.V_tb1 set schema public; --err


--modify table schema
alter table testschema1.NEWTB1 set schema public;

--alter table name
alter table NEWTB1 rename to TB1; 
alter table NEWTb1 rename to Tb1; --err
alter table newtB1 rename to tB1; --err
alter table newtb1 rename to tb1; --err

--drop object
drop view newtestschema1.V_tb1; --err
drop view V_tb1;
drop view v_newtb1;

drop table TB1 cascade;
drop table Tb1; --err
drop table tB1; --err
drop table tb1; --err

drop schema TESTSCHEMA1;
drop schema TESTschema1; --err
drop schema testschema1; --err


--create table and index
create table test1(sId int, sName text collate utf8mb4_general_ci);
create index idx_Text on test1(sname);

--insert data
insert into test1 select generate_series(1,100000) as i, 'zhangsan' operator(pg_catalog.||) generate_series(1,100000);
insert into test1 values (100001, 'zhangsan99999');
insert into test1 values (100002, 'ZHANGsan99999');

--query table
select * from test1 where sNAme = 'zhangsan99999';
explain (costs off) select * from test1 where sNAme = 'zhangsan99999';

--drop table
drop table test1;


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


--rename sequence and set new schema
CREATE SEQUENCE SERIAL START 101;
select count(*) from pg_object where objname = 'SERIAL'; --0
select count(*) from pg_object where objname = 'serial'; --1
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
select count(*) from pg_object ob, pg_namespace n where ob.objname = 'newsequence' and ob.schemaoid = n.oid and n.nspname = 'sc'; --1
select count(*) from pg_object ob, pg_namespace n where ob.objname = 'NEWSEQUENCE' and ob.schemaoid = n.oid and n.nspname = 'sc'; --0
select nextval('SC.NEWSEQUENCE');
select currval('SC.NEWSEQUENCE');

DROP SEQUENCE sc.NEWSEQUENCE;
DROP SEQUENCE SC.NEWSEQUENCE;	--err
DROP SCHEMA SC;



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

/* Begin #Z8459 */
-- Test with lower_case_table_names = 1 (case-insensitive, store lowercase)
CREATE SCHEMA "RenSchema1";
ALTER SCHEMA "RenSchema1" RENAME TO "RenSchema1_New";
SELECT nspname AS pg_ns FROM pg_namespace WHERE nspname LIKE 'renschema1%' ORDER BY nspname;
SELECT objname AS pg_obj FROM pg_object WHERE objname LIKE '%renschema1%' OR objname LIKE '%RenSchema1%' ORDER BY objname;
DROP SCHEMA renschema1_new;
/* End #Z8459 */

reset ivorysql.compatible_mode;
reset lower_case_table_names;