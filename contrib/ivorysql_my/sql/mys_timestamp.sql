set ivorysql.compatible_mode to mysql;
set  datestyle to 'ISO,YMD';

--MySQL timestamp type test case start
select cast('1.1111' as timestamp);--2001-11-11 00:00:00

--explict conversion 
select timestamp'20100231111';--ERROR
select timestamp'2010-02-31';--ERROR
select timestamp'2010-02-31 10:22:33';--ERROR
select timestamp'20100231';--ERROR
select timestamp'0000-00-00';--ERROR
select timestamp'11';--ERROR
select timestamp'100';--ERROR
select timestamp'101';--ERROR
select timestamp'111';--ERROR
select timestamp'1111';--ERROR
select timestamp'9999';--ERROR
select timestamp'11111';--ERROR
select timestamp'111111';--ERROR
select timestamp'1111111';--2011-11-11 01:00:00
select timestamp'11111111';--ERROR
select timestamp'11111111111';--2011-11-11 11:11:01
select timestamp'111111111111111';--ERROR
select timestamp'11.1';--ERROR
select timestamp'11.11';--ERROR
select timestamp'111.11';--ERROR
select timestamp'111.111';--2011-01-11 01:00:00
select timestamp'111.11111';--2011-01-11 11:01:00
select timestamp'111.11111111';--2011-01-11 11:11:11
select timestamp'1111.1';--ERROR
select timestamp'1111.11';--ERROR
select timestamp'1111.111';--ERROR
select timestamp'1111.1111';--ERROR
select timestamp'1111.11111';--1111-11-11 01:00:00
select timestamp'1111.111111';--1111-11-11 11:00:00
select timestamp'1111.1111111';--1111-11-11 11:01:00
select timestamp'11111.1';-- 2011-11-01 01:00:00 
select timestamp'11111.11';-- 2011-11-01 11:00:00
select timestamp'11111.111';-- 2011-11-01 11:01:00
select timestamp'11111.1111';-- 2011-11-01 11:11:00
select timestamp'11111.11111';--2011-11-01 11:11:01
select timestamp'111111.111111';--2011-11-11 11:11:11
select timestamp'1111111.111111';--ERROR
select timestamp'11111111.111111';--1111-11-11 11:11:11
select timestamp'111111111.111111';--ERROR
select timestamp'11111111111.111111';--2011-11-11 11:11:01.111111  Need to be modified
select timestamp'11111111111111.111111';--1111-11-11 11:11:11.111111 Need to be modified
select timestamp'1.1.1';--ERROR
select timestamp'11.11.11';--ERROR
select timestamp'11.11.1111';--2011-11-11 11:00:00
select timestamp'11.11.11111';--2011-11-11 11:01:00
select timestamp'11.11.1111111';--2011-11-11 11:11:01
select timestamp'11.111.1';--2011-11-01 01:00:00
select timestamp'11.111.11';--2011-11-01 11:00:00
select timestamp'11.111.111111';--2011-11-01 11:11:11
select timestamp'11.111.111111111';--ERROR
select timestamp'11.1111.11';--2011-11-11 11:00:00
select timestamp'11.1111.111111';--2011-11-11 11:11:11
select timestamp'111.1.1';--2011-01-01 01:00:00
select timestamp'111.11.1';--2011-01-11 01:00:00
select timestamp'111.11.11';--2011-01-11 11:00:00
select timestamp'111.11111.11';--2011-01-11 11:01:11
select timestamp'111.11.11111';--2011-01-11 11:11:01
select timestamp'1111.11.1';--ERROR
select timestamp'1111.11.11';--ERROR
select timestamp'1111.111.11';--1111-11-01 11:00:00
select timestamp'1111.11.11.11';--1111-11-11 11:00:00
select timestamp'1111.11.11.11.11';--1111-11-11 11:11:00
select timestamp'1111.11.11.11.11.11';--1111-11-11 11:11:11
select timestamp'1111.11.11.11.11.1111111';--ERROR

--set sql_mode
set sql_mode ='';
--strict mode :NO_ZERO_IN_DATE
create table test(dt timestamp);
insert into test values('2100-00-04 12:13:14');--warning
insert into test values('0000-00-00 00:00:00');
insert into test values('1111-01-01 25:00:00');--warning
insert into test values('9999-12-31 23:59:59.44');--warning
insert into test values('9999-12-31 23:59:59.99');--2 warning
insert into test values('2022-12-31 23:59:59');
select * from test;
delete from test;

--set NO_ZERO_DATE
set sql_mode ='NO_ZERO_DATE';
insert into test values('2100-00-04 00:00:00');--warning
insert into test values('0000-00-00 00:00:00');--warning
insert into test values('1111-01-01 25:00:00');--warning
insert into test values('9999-12-31 23:59:59.44');--warning
insert into test values('9999-12-31 23:59:59.99');--2 warning
insert into test values('2022-12-31 23:59:59');
select * from test;
delete from test;

--strict mode :NO_ZERO_DATE
set sql_mode ='NO_ZERO_DATE,STRICT_TRANS_TABLES';
insert into test values('2100-00-04 00:00:00');--ERROR
insert into test values('0000-00-00 00:00:00');--ERROR
insert into test values('1111-01-01 25:00:00');--ERROR
insert into test values('9999-12-31 23:59:59.44');--ERROR
insert into test values('9999-12-31 23:59:59.99');--ERROR
insert into test values('2022-12-31 23:59:59');
select * from test;
delete from test;

--set NO_ZERO_IN_DATE
set sql_mode ='NO_ZERO_IN_DATE';
insert into test values('2100-00-04 00:00:00');--warning
insert into test values('0000-00-00 00:00:00');--ok
insert into test values('1111-01-01 25:00:00');--warning
insert into test values('9999-12-31 23:59:59.44');--warning
insert into test values('9999-12-31 23:59:59.99');--2 warning
insert into test values('2022-12-31 23:59:59');
select * from test;
delete from test;

--strict mode :NO_ZERO_IN_DATE
set sql_mode ='NO_ZERO_IN_DATE,STRICT_TRANS_TABLES';
insert into test values('2100-00-04 00:00:00');--ERROR
insert into test values('0000-00-00 00:00:00');--ok
insert into test values('1111-01-01 25:00:00');--ERROR
insert into test values('9999-12-31 23:59:59.44');--ERROR Datetime function: datetime field overflow
insert into test values('9999-12-31 23:59:59.99');--ERROR
insert into test values('2022-12-31 23:59:59');
select * from test;
delete from test;

--set ALLOW_INVALID_DATES
set sql_mode ='ALLOW_INVALID_DATES';
insert into test values('2100-00-04 00:00:00');--warning
insert into test values('0000-00-00 00:00:00');--ok
insert into test values('1111-01-01 25:00:00');--warning
insert into test values('9999-12-31 23:59:59.44');--warning
insert into test values('9999-12-31 23:59:59.99');--2 warning
insert into test values('2022-12-31 23:59:59');
select * from test;
delete from test;

--strict mode :ALLOW_INVALID_DATES
set sql_mode ='ALLOW_INVALID_DATES,STRICT_TRANS_TABLES';
insert into test values('2100-00-04 00:00:00');--ERROR
insert into test values('0000-00-00 00:00:00');--ok
insert into test values('1111-01-01 25:00:00');--ERROR
insert into test values('9999-12-31 23:59:59.44');--ERROR
insert into test values('9999-12-31 23:59:59.99');--ERROR
insert into test values('2022-12-31 23:59:59');
select * from test;
delete from test;

--default strict+NO_ZERO_IN_DATE+NO_ZERO_DATE
set sql_mode = 'STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE';
insert into test values('2100-00-04 12:13:14');--ERROR
insert into test values('0000-00-00 12:13:14');--ERROR
insert into test values('1111-01-01 25:00:00');--ERROR
insert into test values('');--ERROR
insert into test values('99999-04-32 12:13:14');--ERROR
insert into test values('9999-04-32 12:13:14');--ERROR
insert into test values('9999-04-32 12:13:14');--ERROR
insert into test values('9999-02-31 12:13:14');--ERROR
insert into test values('9999-00-31 12:13:14');--ERROR
insert into test values('0000-00-00 12:13:14');--ERROR
insert into test values('2022-12-31 23:59:59');
select * from test;
delete from test;

--default strict+NO_ZERO_IN_DATE+NO_ZERO_DATE
set sql_mode = 'STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE';
delete from test;
insert into test values(0);--ERROR
insert into test values(2);--ERROR
insert into test values(21);--ERROR
insert into test values(222);--2000-02-22
insert into test values(2221);--ERROR
insert into test values(22211);--ERROR
insert into test values(21211);--2002-12-11
insert into test values(20200933);--ERROR
insert into test values(2020091423);--ERROR
insert into test values(202009142311);--ERROR
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--ERROR
insert into test values(0914231817.49999);--2000-09-14 
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--ERROR
insert into test values(231817.49999);--ERROR
insert into test values(99991231235959);--ERROR
insert into test values(99991231235959.44);--ERROR
insert into test values(99991231235959.99);--ERROR
select * from test;

set sql_mode ='';
delete from test;
insert into test values(0);--0000-00-00
insert into test values(2);--0000-00-00--warning
insert into test values(21);--0000-00-00--warning
insert into test values(222);--2000-02-22
insert into test values(2221);--0000-00-00--warning
insert into test values(22211);--0000-00-00--warning
insert into test values(21211);--2002-12-11
insert into test values(20200933);--0000-00-00--warning
insert into test values(2020091423);--0000-00-00--warning
insert into test values(202009142311);--0000-00-00--warning
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--0000-00-00--warning
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--0000-00-00--warning
insert into test values(231817.49999);--0000-00-00--warning
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--2 warning
select * from test;

set sql_mode ='NO_ZERO_DATE';
delete from test;
insert into test values(0);--0000-00-00--warning
insert into test values(2);--0000-00-00--warning
insert into test values(21);--0000-00-00--warning
insert into test values(222);--2000-02-22
insert into test values(2221);--0000-00-00--warning
insert into test values(22211);--0000-00-00--warning
insert into test values(21211);--2002-12-11
insert into test values(20200933);--0000-00-00--warning
insert into test values(2020091423);--0000-00-00--warning
insert into test values(202009142311);--0000-00-00--warning
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--0000-00-00--warning
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--0000-00-00--warning
insert into test values(231817.49999);--0000-00-00--warning
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--2 warning
select * from test;

set sql_mode ='NO_ZERO_DATE,STRICT_TRANS_TABLES';
delete from test;
insert into test values(0);--error
insert into test values(2);--error
insert into test values(21);--error
insert into test values(222);--2000-02-22
insert into test values(2221);--error
insert into test values(22211);--error
insert into test values(21211);--2002-12-11
insert into test values(20200933);--error
insert into test values(2020091423);--error
insert into test values(202009142311);--error
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--error
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--error
insert into test values(231817.49999);--error
insert into test values(230017);--ERROR
insert into test values(99991231235959);--ERROR
insert into test values(99991231235959.44);--ERROR
insert into test values(99991231235959.99);--ERROR Datetime function: datetime field overflow
select * from test;

set sql_mode ='NO_ZERO_IN_DATE';
delete from test;
insert into test values(0);--0000-00-00
insert into test values(2);--0000-00-00--warning
insert into test values(21);--0000-00-00--warning
insert into test values(222);--2000-02-22
insert into test values(2221);--0000-00-00--warning
insert into test values(22211);--0000-00-00--warning
insert into test values(21211);--2002-12-11
insert into test values(20200933);--0000-00-00--warning
insert into test values(2020091423);--0000-00-00--warning
insert into test values(202009142311);--0000-00-00--warning
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--0000-00-00--warning
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--0000-00-00--warning
insert into test values(231817.49999);--0000-00-00--warning
insert into test values(230017);--0000-00-00--warning
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--2 warning
select * from test;

set sql_mode ='NO_ZERO_IN_DATE,STRICT_TRANS_TABLES';
delete from test;
insert into test values(0);--0000-00-00
insert into test values(2);--error
insert into test values(21);--error
insert into test values(222);--2000-02-22
insert into test values(2221);--error
insert into test values(22211);--error
insert into test values(21211);--2002-12-11
insert into test values(20200933);--error
insert into test values(2020091423);--error
insert into test values(202009142311);--error
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--error
insert into test values(0914231817.49999);--2000-09-14 
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--error
insert into test values(231817.49999);--error
insert into test values(230017);--error
insert into test values(99991231235959);--ERROR
insert into test values(99991231235959.44);--ERROR
insert into test values(99991231235959.99);--ERROR
select *  from test;

set sql_mode ='ALLOW_INVALID_DATES';
delete from test;
insert into test values(0);--0000-00-00
insert into test values(2);--0000-00-00--warning
insert into test values(21);--0000-00-00--warning
insert into test values(222);--2000-02-22
insert into test values(2221);--0000-00-00--warning
insert into test values(22211);--0000-00-00--warning
insert into test values(21211);--2002-12-11
insert into test values(20200933);--0000-00-00--warning
insert into test values(2020091423);--0000-00-00--warning
insert into test values(202009142311);--0000-00-00--warning
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--0000-00-00--warning
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14 
insert into test values(790914231817.49999);--1979-09-14 
insert into test values(14231817.49999);--0000-00-00--warning
insert into test values(231817.49999);--0000-00-00--warning
insert into test values(230017);--warning
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--2 warning
select * from test;

set sql_mode ='ALLOW_INVALID_DATES,STRICT_TRANS_TABLES';
delete from test;
insert into test values(0);--0000-00-00
insert into test values(2);--error
insert into test values(21);--error
insert into test values(222);--2000-02-22
insert into test values(2221);--error
insert into test values(22211);--error
insert into test values(21211);--2002-12-11
insert into test values(20200933);--error
insert into test values(2020091423);--error
insert into test values(202009142311);--error
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14 
insert into test values(20200914231817.49999);--2020-09-14 
insert into test values(20200934231817.49999);--error
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14 
insert into test values(14231817.49999);--error
insert into test values(231817.49999);--error
insert into test values(230017);--ERROR
insert into test values(99991231235959);--ERROR
insert into test values(99991231235959.44);--ERROR
insert into test values(99991231235959.99);--ERROR
select * from test;

----20231013
set sql_mode ='ALLOW_INVALID_DATES,NO_ZERO_DATE';
delete from test;
insert into test values(0);--0000-00-00 --warning
insert into test values(2);--0000-00-00 --warning
insert into test values(21);--0000-00-00 --warning
insert into test values(222);--2000-02-22
insert into test values(2221);--0000-00-00 --warning
insert into test values(22211);--0000-00-00 --warning
insert into test values(21211);--2002-12-11
insert into test values(20200933);--0000-00-00 --warning
insert into test values(2020091423);--0000-00-00 --warning
insert into test values(202009142311);--0000-00-00 --warning
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--0000-00-00 --warning
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--0000-00-00 --warning
insert into test values(231817.49999);--0000-00-00 --warning
insert into test values(230017);--warning
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--2 warning
select * from test;

set sql_mode ='ALLOW_INVALID_DATES,NO_ZERO_IN_DATE';
delete from test;
insert into test values(0);--0000-00-00
insert into test values(2);--0000-00-00 --warning
insert into test values(21);--0000-00-00 --warning
insert into test values(222);--2000-02-22
insert into test values(2221);--0000-00-00 --warning
insert into test values(22211);--0000-00-00 --warning
insert into test values(21211);--2002-12-11
insert into test values(20200933);--0000-00-00 --warning
insert into test values(2020091423);--0000-00-00 --warning
insert into test values(202009142311);--0000-00-00 --warning
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--0000-00-00 --warning
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--0000-00-00 --warning
insert into test values(231817.49999);--0000-00-00 --warning
insert into test values(230017);--0000-00-00 --warning
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--2 warning
select * from test;

set sql_mode ='NO_ZERO_DATE,NO_ZERO_IN_DATE';
delete from test;
insert into test values(0);--0000-00-00 --warning
insert into test values(2);--0000-00-00 --warning
insert into test values(21);--0000-00-00 --warning
insert into test values(222);--2000-02-22
insert into test values(2221);--0000-00-00 --warning
insert into test values(22211);--0000-00-00 --warning
insert into test values(21211);--2002-12-11
insert into test values(20200933);--0000-00-00 --warning
insert into test values(2020091423);--0000-00-00 --warning
insert into test values(202009142311);--0000-00-00 --warning
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--0000-00-00 --warning
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--0000-00-00 --warning
insert into test values(231817.49999);--0000-00-00 --warning
insert into test values(230017);--0000-00-00 --warning
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--2 warning
select * from test;

set sql_mode ='NO_ZERO_DATE,NO_ZERO_IN_DATE,ALLOW_INVALID_DATES';
delete from test;
insert into test values(0);--0000-00-00 --warning
insert into test values(2);--0000-00-00 --warning
insert into test values(21);--0000-00-00 --warning
insert into test values(222);--2000-02-22
insert into test values(2221);--0000-00-00 --warning
insert into test values(22211);--0000-00-00 --warning
insert into test values(21211);--2002-12-11
insert into test values(20200933);--0000-00-00 --warning
insert into test values(2020091423);--0000-00-00 --warning
insert into test values(202009142311);--0000-00-00 --warning
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--0000-00-00 --warning
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--0000-00-00 --warning
insert into test values(231817.49999);--0000-00-00 --warning
insert into test values(230017);--0000-00-00 --warning
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--2 warning
select * from test;

drop table test;
set sql_mode to default;
--

-- oper
CREATE TABLE t2 (a int8, t timestamp);
insert into t2 values(1, '2022-12-11 01:01:01');
insert into t2 values(2, '2022-12-21 01:01:01');
insert into t2 values(20221231010101, '2022-12-31 01:01:01');
insert into t2 values(20221231111111, '2022-11-30 01:01:01');
---=
select * from t2 where t = 20221231010101;
--<>
select * from t2 where t <> 20221231010101;
--<
select * from t2 where t < 20221231010101;
-->
select * from t2 where t > 20221231010101;
--<=
select * from t2 where t <= 20221231010101;
-->=
select * from t2 where t >= 20221231010101;
--<=>
select * from t2 where t <=> 20221231010101;
--+
select t+a from t2;
select t+11 from t2;
---
select t-a from t2;
select t-11 from t2;
-- *
select t*a from t2;
select t*11 from t2;
-- /
select t/a from t2;
/*
select t/a from t2;
     ?column?      
-------------------
 20221211010101.0000
 10110610505050.5000
              1.0000
              1.0000
(4 rows)
*/
select t/11 from t2;
/*
select t/11 from t2;
      ?column?      
--------------------
 1838291910009.1818
 1838292819100.0909
 1838293728191.0000
 1838284546372.8182
(4 rows)
*/
-- div
select t div a from t2;
select t div 11 from t2;
-- %
select t % a from t2;
select t % 11 from t2;
-- mod
select t mod a from t2;
select t mod 11 from t2;
-- &
select t & a from t2;
select t & 11 from t2;
-- |
select t | a from t2;
select t | 11 from t2;
-- ~
-- Need to be modified: 64-bit unsigned number after the number is inverted in mysql !!!!!!!
select ~t from t2;

-- ^
select t ^ a from t2;
select t ^ 11 from t2;
-- >>
select t >> a from t2;
select t >> 11 from t2;
-- <<
select t << a from t2;
select t << 11 from t2;
-- bit_count
select bit_count(t) from t2;
-- &&
select t && a from t2;
select t && 11 from t2;
-- ||
set sql_mode='ANSI_QUOTES,ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION';
select t || true from t2;
select t || false from t2;
-- !
select !t from t2;

-- and or xor
select t::boolean and 11::boolean from t2;
select t::boolean or 11::boolean from t2;
select t::boolean xor 11::boolean from t2;

drop table t2;

-- between and
create table t1 (f1 timestamp );
insert into t1 values('2001-01-01 01:01:01');
insert into t1 values('2001-02-05 00:00:00');
insert into t1 values('2001-03-09 01:01:01');
insert into t1 values('2001-04-15 00:00:00');
insert into t1 values('2001-05-20 01:01:01');
select * from t1 where f1 between '2001-02-05 00:00:00' and '2001-04-15 00:00:00';

drop table t1;

-- in
create table t1(f2 timestamp);
insert into t1 values('01-01-01 00:00:00'),('01-02-03 12:34:56'),('02-04-06 11:22:33');
select * from t1 where f2 in ('01-01-01','01-02-03 12:34:56','01-02-03');
select * from t1 where f2 not in ('01-01-01','01-02-03 12:34:56','01-02-03');
drop table t1;

-- is
CREATE TABLE t1 (a timestamp, b INT);
INSERT INTO t1 VALUES ('01-02-03 12:34:56',1), ('1999-05-10 00:00:00',2);
SELECT * FROM t1 WHERE a IS true;
SELECT * FROM t1 WHERE a IS not true;
SELECT * FROM t1 WHERE a IS NULL;
SELECT * FROM t1 WHERE a IS not NULL;
drop table t1;

-- like
create table t1 (f1 timestamp);
insert into t1 values('2001-01-01 01:01:01');
insert into t1 values('2001-02-01 01:01:01');
insert into t1 values('2001-02-01 01:01:01');

select f1 from t1 where f1 like '2001-01-01 01:01:01'; --1
select f1 from t1 where f1 like 010101010101; --0
select f1 from t1 where f1 like '010101010101'; --0--Need to be modified: text type conversion bit timestamp in mysql!!!!!!
select f1 from t1 where f1 like 20010101010101; --0
select f1 from t1 where f1 like '20010101010101'; --0--Need to be modified: text type conversion bit timestamp in mysql!!!!!!

drop table t1;


--btree and hash index
create table idx_1 (idx timestamp);
create index idx1 on idx_1 using btree (idx);
insert into idx_1 values ('2022-01-01'),('2022-02-01'),('2022-03-01');
vacuum full analyze idx_1;
explain (COSTS OFF) select * from idx_1 where idx < '2022-02-01';
explain (COSTS OFF) select * from idx_1 where idx > '2022-02-01';
explain (COSTS OFF) select * from idx_1 where idx = '2022-02-01';
explain (COSTS OFF) select * from idx_1 where idx <= '2022-02-01';
explain (COSTS OFF) select * from idx_1 where idx >= '2022-02-01';

create table idx_2 (idx timestamp);
create index idx2 on idx_2 using hash (idx);
vacuum full analyze idx_2;
insert into idx_2 values ('2022-01-01'),('2022-02-01'),('2022-03-01');
explain (COSTS OFF) select * from idx_2 where idx = '2022-02-01';
explain (COSTS OFF) select * from idx_2 where idx > '2022-02-01';

drop table idx_1,idx_2;
create table test_date(c1 timestamp, c2 timestamp);
insert into test_date values('2012-01-01','2012-01-02'),('2013-01-01','2013-01-02');
create table test_tmp(c1 timestamp, c2 timestamp);
insert into test_tmp values('2012-01-01','2012-01-02'),('2013-02-01','2013-02-02');
select * from test_date where(c1,c2) not in (select * from test_tmp);
drop table test_date,test_tmp;

create table tb1 (a timestamp);
create or replace procedure proc1()
as
$$
declare
i int := 1970;
j int := 1;
k int := 1;
begin
    while i <= 2038 loop
	j = 1;
		while j <=12 loop 
		k = 1;
			while k <= 28 loop
				insert into tb1 (a) values (i*10000+j*100+k);
				k = k+1;
			end loop;
			j = j+1;
		end loop;
    i = i+1;
    end loop;
end;
$$ language plmysql;
call proc1();
create index idx1 on tb1 using hash(a);
vacuum full analyze tb1;
explain (COSTS OFF) select * from tb1 where a = '1970-04-04';

drop index idx1;
create index idx_tree on tb1(a);
vacuum full analyze tb1;
set enable_bitmapscan =false;
set enable_seqscan =false;
explain (COSTS OFF) select * from tb1 where a < '1970-01-04';
explain (COSTS OFF) select * from tb1 where a <= '1970-01-04';
explain (COSTS OFF) select * from tb1 where a = '1970-01-04';
explain (COSTS OFF) select * from tb1 where a != '1970-01-04';
explain (COSTS OFF) select * from tb1 where a >= '2037-12-04';
explain (COSTS OFF) select * from tb1 where a > '2037-12-04';
set enable_seqscan =true;
set enable_bitmapscan =true;
drop procedure proc1;
drop table tb1;

set sql_mode to default;
CREATE TABLE tb1 (a int, t timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP);
insert into tb1 values(2,'2002-03-03');
insert into tb1 (a) values(4);
drop table tb1;

set sql_mode='';
create table tb1 (t1 timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, t2 int, t3 timestamp NOT NULL DEFAULT '0000-00-00 00:00:00');
insert into tb1 values ();
select pg_sleep(1);
update tb1 set t2=111;
drop table tb1;

create table tb1 (t1 timestamp , t2 int, t3 timestamp DEFAULT CURRENT_TIMESTAMP);
insert into tb1 values ();
select pg_sleep(1);
update tb1 set t2=111;
drop table tb1;

set sql_mode to default;

-- timestamp(3)
select cast(111 as timestamp(3));
select cast('111.11.11.23.34.50' as timestamp(3));

create table test(dt timestamp(3));
insert into test values('2100-00-04 12:13:14.444444');
insert into test values('0000-00-00 00:00:00');
insert into test values('1111-01-01 25:00:00');
insert into test values('9999-12-31 23:59:59.44');
insert into test values('9999-12-31 23:59:59.99');
insert into test values('2022-12-31 23:59:59');
insert into test values('2022-12-31 10:59:59.5555');
select * from test;

delete from test;
insert into test values(0);--ERROR
insert into test values(2);--ERROR
insert into test values(21);--ERROR
insert into test values(222);--ok
insert into test values(2221);--ERROR
insert into test values(22211);--ERROR
insert into test values(21211);--ok
insert into test values(20200933);--ERROR
insert into test values(2020091423);--ERROR
insert into test values(202009142311);--ERROR
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--ERROR
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--ERROR
insert into test values(231817.49999);--ERROR
insert into test values(230017);--ERROR
insert into test values(99991231235959);--ERROR
insert into test values(99991231235959.44);--ERROR
insert into test values(99991231235959.99);--ERROR
select * from test;

drop table test;
set sql_mode to default;


CREATE TABLE t2 (a int8, t timestamp(3));
insert into t2 values(1, '2022-12-11 01:01:01.444444');
insert into t2 values(2, '2022-12-21 01:01:01.444444');
insert into t2 values(20221231010101, '2022-12-31 01:01:01.444444');
insert into t2 values(20221231111111, '2022-11-30 01:01:01.444444');
---=
select * from t2 where t = 20221231010101.444;
--<>
select * from t2 where t <> 20221231010101.444;
--<
select * from t2 where t < 20221231010101.444;
-->
select * from t2 where t > 20221231010101.444;
--<=
select * from t2 where t <= 20221231010101.444;
-->=
select * from t2 where t >= 20221231010101.444;
--<=>
select * from t2 where t <=> 20221231010101.444;
--+
select t+a from t2;
select t+11 from t2;
---
select t-a from t2;
select t-11 from t2;
-- *
select t*a from t2;
select t*11 from t2;
-- /
select t/a from t2;
select t/11 from t2;
select t/11.11 from t2;
select t/20221231010101.11 from t2;
select t/20221231010101.1111 from t2;
select 20221231010101/t from t2;
select 20221231010101.99/t from t2;
select 20221231010101.9999/t from t2;
select 66666666666666/t from t2;
-- div
select t div a from t2;
select t div 11 from t2;
select t div 11.11 from t2;
select t div 20221231010101.11 from t2;
select 20221231010101 div t from t2;
select 20221231010101.99 div t from t2;
select 66666666666666 div t from t2;
-- %
select t % a from t2;
select t % 11 from t2;
-- mod
select t mod a from t2;
select t mod 11 from t2;
-- &
select t & a from t2;
select t & 11 from t2;
-- |
select t | a from t2;
select t | 11 from t2;
-- ~
-- Need to be modified: 64-bit unsigned number after the number is inverted in mysql !!!!!!!
select ~t from t2;

-- ^
select t ^ a from t2;
select t ^ 11 from t2;
-- >>
select t >> a from t2;
select t >> 11 from t2;
-- <<
select t << a from t2;
select t << 11 from t2;
-- bit_count
select bit_count(t) from t2;
-- &&
select t && a from t2;
select t && 11 from t2;
-- ||
set sql_mode='ANSI_QUOTES,ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION';
select t || true from t2;
select t || false from t2;
-- !
select !t from t2;

-- and or xor
select t::boolean and 11::boolean from t2;
select t::boolean or 11::boolean from t2;
select t::boolean xor 11::boolean from t2;

drop table t2;

-- between and
create table t1 (f1 timestamp(4) );
insert into t1 values('2001-01-01 01:01:01.555555');
insert into t1 values('2001-02-05 00:00:00.555555');
insert into t1 values('2001-03-09 01:01:01.555555');
insert into t1 values('2001-04-15 00:00:00.555555');
insert into t1 values('2001-05-20 01:01:01.555555');
select * from t1 where f1 between '2001-02-05 00:00:00' and '2001-04-15 00:00:00';

drop table t1;

-- in
create table t1(f2 timestamp(4));
insert into t1 values('01-01-01 00:00:00.555555'),('01-02-03 12:34:56.555555'),('02-04-06 11:22:33.555555');
select * from t1 where f2 in ('01-01-01 00:00:00.5556','01-02-03 12:34:56','01-02-03');
select * from t1 where f2 not in ('01-01-01 00:00:00.5556','01-02-03 12:34:56','01-02-03');
drop table t1;

-- is
CREATE TABLE t1 (a timestamp(4), b INT);
INSERT INTO t1 VALUES ('01-02-03 12:34:56.555555',1), ('1999-05-10 00:00:00.555555',2);
SELECT * FROM t1 WHERE a IS true;
SELECT * FROM t1 WHERE a IS not true;
SELECT * FROM t1 WHERE a IS NULL;
SELECT * FROM t1 WHERE a IS not NULL;
drop table t1;

-- like
create table t1 (f1 timestamp(4));
insert into t1 values('2001-01-01 01:01:01.555555');
insert into t1 values('2001-02-01 01:01:01.555555');
insert into t1 values('2001-02-01 01:01:01.555555');

select f1 from t1 where f1 like '2001-01-01 01:01:01.555555'; --0
select f1 from t1 where f1 like '2001-01-01 01:01:01.5556';
select f1 from t1 where f1 like 010101010101.555555; --0
select f1 from t1 where f1 like '010101010101.555555'; --0
select f1 from t1 where f1 like 20010101010101.555555; --0
select f1 from t1 where f1 like '20010101010101.555555'; --0
select f1 from t1 where f1 like '20010101010101.5556'; --Need to be modified
select f1 from t1 where f1 like '20010101010101.5556'::text;
drop table t1;

--btree and hash index
create table idx_1 (idx timestamp(4));
create index idx1 on idx_1 using btree (idx);
insert into idx_1 values ('2022-01-01 01:01:01.555555'),('2022-02-01 01:01:01.555555'),('2022-03-01 01:01:01.555555');
vacuum full analyze idx_1;
explain (COSTS OFF) select * from idx_1 where idx < '2022-02-01';
explain (COSTS OFF) select * from idx_1 where idx > '2022-02-01';
explain (COSTS OFF) select * from idx_1 where idx = '2022-02-01';
explain (COSTS OFF) select * from idx_1 where idx <= '2022-02-01';
explain (COSTS OFF) select * from idx_1 where idx >= '2022-02-01';

create table idx_2 (idx timestamp(4));
create index idx2 on idx_2 using hash (idx);
vacuum full analyze idx_2;
insert into idx_2 values ('2022-01-01 01:01:01.555555'),('2022-02-01 01:01:01.555555'),('2022-03-01 01:01:01.555555');
explain (COSTS OFF) select * from idx_2 where idx = '2022-02-01';
explain (COSTS OFF) select * from idx_2 where idx > '2022-02-01';

drop table idx_1,idx_2;
create table test_date(c1 timestamp(4), c2 timestamp(2));
insert into test_date values('2012-01-01 01:01:01.555555','2012-01-02 01:01:01.555555'),('2013-01-01 01:01:01.555555','2013-01-02 01:01:01.555555');
create table test_tmp(c1 timestamp(4), c2 timestamp(2));
insert into test_tmp values('2012-01-01 01:01:01.555555','2012-01-02 01:01:01.555555'),('2013-02-01 01:01:01.555555','2013-02-02 01:01:01.555555');
select * from test_date where(c1,c2) not in (select * from test_tmp);
drop table test_date,test_tmp;

create table tb1 (a timestamp(4));
create or replace procedure proc1()
as
$$
declare
i int := 1970;
j int := 1;
k int := 1;
begin
    while i <= 2038 loop
	j = 1;
		while j <=12 loop 
		k = 1;
			while k <= 28 loop
				insert into tb1 (a) values (i*10000+j*100+k);
				k = k+1;
			end loop;
			j = j+1;
		end loop;
    i = i+1;
    end loop;
end;
$$ language plmysql;
call proc1();
create index idx1 on tb1 using hash(a);
vacuum full analyze tb1;
explain (COSTS OFF) select * from tb1 where a = '1970-04-04';

drop index idx1;
create index idx_tree on tb1(a);
vacuum full analyze tb1;
set enable_bitmapscan =false;
set enable_seqscan =false;
explain (COSTS OFF) select * from tb1 where a < '1970-01-04';
explain (COSTS OFF) select * from tb1 where a <= '1970-01-04';
explain (COSTS OFF) select * from tb1 where a = '1970-01-04';
explain (COSTS OFF) select * from tb1 where a != '1970-01-04';
explain (COSTS OFF) select * from tb1 where a >= '2037-12-04';
explain (COSTS OFF) select * from tb1 where a > '2037-12-04';
set enable_seqscan =true;
set enable_bitmapscan =true;
drop procedure proc1;
drop table tb1;

--mysql-check
SET sql_mode = 'NO_ENGINE_SUBSTITUTION';

CREATE TABLE t1 (a int, t timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP);
CREATE TABLE t2 (a int, t datetime);
insert into t1 values(2,"2002-03-03");
insert into t1 (a) values(4);
insert into t2 values(5,"2002-03-04"),(7,"2002-03-05"),(8,"00-00-00");
insert into t1 select * from t2;
insert into t2 values(6,NULL);
insert into t1 select * from t2;  --PG is not supported
insert into t1 (a) select a+1 from t2 where a=8;
select * from t2;
--select * from t1;
drop table t1,t2;


CREATE TABLE t1 (value TEXT NOT NULL, id VARCHAR(32) NOT NULL, stamp timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, PRIMARY KEY (id));
INSERT INTO t1 VALUES ("my value", "myKey","1999-04-02 00:00:00");
SELECT stamp FROM t1 WHERE id="myKey";
UPDATE t1 SET value="my value" WHERE id="myKey";
SELECT stamp FROM t1 WHERE id="myKey";
UPDATE t1 SET id="myKey" WHERE value="my value";
SELECT stamp FROM t1 WHERE id="myKey";
drop table t1;

/*
create table t1 (a timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP);
insert into t1 values (now());
select date_format(a,"%Y %y"),year(a),year(now()) from t1;
drop table t1;*/

create table t1 (ix timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP);
insert into t1 values (19991101000000),(19990102030405),(19990630232922),(19990601000000),(19990930232922),(19990531232922),(19990501000000),(19991101000000),(19990501000000);
select ix+0 from t1;
truncate table t1;
insert into t1 values ("19991101000000"),("19990102030405"),("19990630232922"),("19990601000000");
select ix+0 from t1;
drop table t1;

CREATE TABLE t1 (date date, date_time datetime, time_stamp timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP);
INSERT INTO t1 VALUES ("1998-12-31","1998-12-31 23:59:59",19981231235959);
INSERT INTO t1 VALUES ("1999-01-01","1999-01-01 00:00:00",19990101000000);
INSERT INTO t1 VALUES ("1999-09-09","1999-09-09 23:59:59",19990909235959);
INSERT INTO t1 VALUES ("2000-01-01","2000-01-01 00:00:00",20000101000000);
INSERT INTO t1 VALUES ("2000-02-28","2000-02-28 00:00:00",20000228000000);
INSERT INTO t1 VALUES ("2000-02-29","2000-02-29 00:00:00",20000229000000);
INSERT INTO t1 VALUES ("2000-03-01","2000-03-01 00:00:00",20000301000000);
INSERT INTO t1 VALUES ("2000-12-31","2000-12-31 23:59:59",20001231235959);
INSERT INTO t1 VALUES ("2001-01-01","2001-01-01 00:00:00",20010101000000);
INSERT INTO t1 VALUES ("2004-12-31","2004-12-31 23:59:59",20041231235959);
INSERT INTO t1 VALUES ("2005-01-01","2005-01-01 00:00:00",20050101000000);
INSERT INTO t1 VALUES ("2030-01-01","2030-01-01 00:00:00",20300101000000);
SELECT * FROM t1;
drop table t1;

create table t1 (ix timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP);
insert into t1 values (0),(20030101010160),(20030101016001),(20030101240101),(20030132010101),(20031301010101),(20031200000000),(20030000000000);
select ix+0 from t1;
truncate table t1;
insert into t1 values ("00000000000000"),("20030101010160"),("20030101016001"),("20030101240101"),("20030132010101"),("20031301010101"),("20031200000000"),("20030000000000");
select ix+0 from t1;
truncate table t1;
insert into t1 values ("0000-00-00 00:00:00 some trailer"),("2003-01-01 00:00:00 some trailer");
select ix+0 from t1;
drop table t1;

--replace_result $DEFAULT_ENGINE ENGINE

create table t1 (t1 timestamp not null default '2003-01-01 00:00:00', t2 datetime, t3 timestamp NOT NULL DEFAULT '0000-00-00 00:00:00');
insert into t1 values ();
update t1 set t2=now();
insert into t1 (t1,t3) values (default, default);
--select * from t1;--now not select 

--replace_result $DEFAULT_ENGINE ENGINE
\d+ t1;
drop table t1;

create table t1 (t1 timestamp not null default now(), t2 datetime, t3 timestamp NOT NULL DEFAULT '0000-00-00 00:00:00');
insert into t1 values ();
update t1 set t2=now();
insert into t1 (t1,t3) values (default, default);
--select * from t1;

--replace_result $DEFAULT_ENGINE ENGINE
\d+ t1;
drop table t1;

create table t1 (t1 timestamp not null default '2003-01-01 00:00:00' on update now(), t2 datetime);
insert into t1 values ();
--select * from t1;
update t1 set t2=now();
insert into t1 (t1) values (default);
--select * from t1;
--select * from t1;
--         t1          |         t2          
---------------------+---------------------
-- 2025-02-21 01:30:35 | 2025-02-21 01:30:35   --t1 ==t2
-- 2003-01-01 00:00:00 | 
--(2 rows)
--replace_result $DEFAULT_ENGINE ENGINE
\d+ t1;
drop table t1;

create table t1 (t1 timestamp not null default now() on update now(), t2 datetime);
insert into t1 values ();
--select * from t1;
update t1 set t2=now();
insert into t1 (t1) values (default);
--select * from t1;

--replace_result $DEFAULT_ENGINE ENGINE
\d+ t1;
drop table t1;

create table t1 (t1 timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, t2 datetime, t3 timestamp NOT NULL DEFAULT '0000-00-00 00:00:00');
insert into t1 values ();
--select * from t1;
update t1 set t2=now();
insert into t1 (t1,t3) values (default, default);
--select * from t1;

--replace_result $DEFAULT_ENGINE ENGINE
\d+ t1;
drop table t1;

create table t1 (t1 timestamp not null default current_timestamp on update current_timestamp, t2 datetime);
insert into t1 values ();
--select * from t1;
update t1 set t2=now();
insert into t1 (t1) values (default);
--select * from t1;

--replace_result $DEFAULT_ENGINE ENGINE
\d+ t1;
truncate table t1;

insert into t1 values ('2004-04-01 00:00:00', '2004-04-01 00:00:00');
update t1 set t1= '2004-04-02 00:00:00';
select * from t1;
update t1 as ta, t1 as tb set tb.t1= '2004-04-03 00:00:00';
select * from t1;
drop table t1;

create table t1 (pk int primary key, t1 timestamp not null default current_timestamp on update current_timestamp, bulk int);
insert into t1 values (1, '2004-04-01 00:00:00', 10);
--replace into t1 set pk = 1, bulk= 20; 
select * from t1;
drop table t1;
create table t1 (pk int primary key, t1 timestamp not null default '2003-01-01 00:00:00' on update current_timestamp, bulk int);
insert into t1 values (1, '2004-04-01 00:00:00', 10);
--replace into t1 set pk = 1, bulk= 20; 
select * from t1;
drop table t1;
create table t1 (pk int primary key, t1 timestamp not null default current_timestamp on update current_timestamp, bulk int);
insert into t1 values (1, '2004-04-01 00:00:00', 10);
--replace into t1 set pk = 1, bulk= 20; 
select * from t1;
drop table t1;

create table t1 (t1 timestamp not null default current_timestamp on update current_timestamp);
insert into t1 values ('2004-04-01 00:00:00');
alter table t1 add i int default 10;
select * from t1;
drop table t1;

create table t1 (a timestamp null, b timestamp null);
insert into t1 values (NULL, NULL);
insert into t1 values ();
select * from t1;
drop table t1;

create table t1 (a timestamp null default current_timestamp on update current_timestamp, b timestamp null);
insert into t1 values (NULL, NULL);
insert into t1 values ();
--select * from t1;
drop table t1;

create table t1 (a timestamp null default null, b timestamp null default '2003-01-01 00:00:00');
insert into t1 values (NULL, NULL);
insert into t1 values (DEFAULT, DEFAULT);
select * from t1;
drop table t1;

create table t1 (a bigint, b bigint);
insert into t1 values (NULL, NULL), (20030101000000, 20030102000000);
--alter table t1 modify a timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, modify b timestamp NOT NULL DEFAULT '0000-00-00 00:00:0';
--alter table t1 ALTER COLUMN a type timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, ALTER COLUMN b type timestamp NOT NULL DEFAULT '0000-00-00 00:00:0';
alter table t1 ALTER COLUMN a type timestamp, ALTER COLUMN b type timestamp;
select * from t1;
drop table t1;

create table t1 (a char(2), t timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP);
insert into t1 values ('a', '2004-01-01 00:00:00'), ('a', '2004-01-01 01:00:00'),
                      ('b', '2004-02-01 00:00:00');
select max(t::pg_catalog.timestamp) from t1 group by a;
drop table t1;

set sql_mode='';

create table t1 (a int primary key, b int, c timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP);
insert into t1 (a, b, c) values (1, 0, '2001-01-01 01:01:01'),
  (2, 0, '2002-02-02 02:02:02'), (3, 0, '2003-03-03 03:03:03');
--select * from t1 order by a;
update t1 set b = 2, c = c where a = 2;
--select * from t1 order by a;
insert into t1 (a) values (4);
--select * from t1 order by a;
update t1 set c = '2004-04-04 04:04:04' where a = 4;
--select * from t1 order by a;
insert into t1 (a) values (3), (5) on duplicate key update b = 3, c = c;
--select * from t1 order by a;
insert into t1 (a, c) values (4, '2004-04-04 00:00:00'),
  (6, '2006-06-06 06:06:06') on duplicate key update b = 4;
--select * from t1 order by a;
drop table t1;

--echo End of 4.1 tests

--echo #
--echo # Bug #55779: select does not work properly in mysql server
--echo #             Version "5.1.42 SUSE MySQL RPM"
--echo #

CREATE TABLE t1 (a TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP primary key);

INSERT INTO t1 VALUES ('2000-01-01 00:00:00'), ('2000-01-01 00:00:00'),
                      ('2000-01-01 00:00:01'), ('2000-01-01 00:00:01');

SELECT a FROM t1 WHERE a >=  20000101000000;
SELECT a FROM t1 WHERE a >= '20000101000000';

DROP TABLE t1;

--echo #
--echo # Bug#50774: failed to get the correct resultset when timestamp values 
--echo # are appended with .0
--echo #
CREATE TABLE t1 ( a TIMESTAMP primary key );

INSERT INTO t1 VALUES( '2010-02-01 09:31:01' );
INSERT INTO t1 VALUES( '2010-02-01 09:31:02' );
INSERT INTO t1 VALUES( '2010-02-01 09:31:03' );
INSERT INTO t1 VALUES( '2010-02-01 09:31:04' );

SELECT * FROM t1 WHERE a >= '2010-02-01 09:31:02.0';
SELECT * FROM t1 WHERE '2010-02-01 09:31:02.0' <= a;
SELECT * FROM t1 WHERE a <= '2010-02-01 09:31:02.0';
SELECT * FROM t1 WHERE '2010-02-01 09:31:02.0' >= a;

--replace_column 1 x 2 x 3 x 6 x 7 x 8 x 9 x 10 x 12 x
EXPLAIN
SELECT * FROM t1 WHERE a >= '2010-02-01 09:31:02.0';
SELECT * FROM t1 WHERE a >= '2010-02-01 09:31:02.0';

CREATE TABLE t2 ( a TIMESTAMP primary key);

INSERT INTO t2 VALUES( '2010-02-01 09:31:01' );
INSERT INTO t2 VALUES( '2010-02-01 09:31:02' );
INSERT INTO t2 VALUES( '2010-02-01 09:31:03' );
INSERT INTO t2 VALUES( '2010-02-01 09:31:04' );
INSERT INTO t2 VALUES( '2010-02-01 09:31:05' );
INSERT INTO t2 VALUES( '2010-02-01 09:31:06' );
INSERT INTO t2 VALUES( '2010-02-01 09:31:07' );
INSERT INTO t2 VALUES( '2010-02-01 09:31:08' );
INSERT INTO t2 VALUES( '2010-02-01 09:31:09' );
INSERT INTO t2 VALUES( '2010-02-01 09:31:10' );
INSERT INTO t2 VALUES( '2010-02-01 09:31:11' );

--echo # The bug would cause the range optimizer's comparison to use an open
--echo # interval here. This reveals itself only in the number of reads 
--echo # performed.
vacuum t2;
--replace_column 1 x 2 x 3 x 6 x 7 x 8 x 9 x 10 x 12 x
EXPLAIN
SELECT * FROM t2 WHERE a < '2010-02-01 09:31:02.0';
SELECT * FROM t2 WHERE a < '2010-02-01 09:31:02.0';

DROP TABLE t1, t2;

--echo End of 5.1 tests

--echo
--echo Bug#50888 valgrind warnings in Field_timestamp::val_str
--echo

CREATE TABLE t1(a timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP);
INSERT INTO t1 VALUES ('2008-02-23 09:23:45'), ('2010-03-05 11:08:02');
vacuum t1;
SELECT max(a::pg_catalog.timestamp) FROM t1;
SELECT a FROM t1;
DROP TABLE t1;

--echo End of Bug#50888

--echo #
--echo # Bug59330: Incorrect result when comparing an aggregate
--echo #           function with TIMESTAMP
--echo #
CREATE TABLE t1 (dt DATETIME, ts TIMESTAMP);
INSERT INTO t1 VALUES('2011-01-06 12:34:30', '2011-01-06 12:34:30');
SELECT max(dt::pg_catalog.timestamp), max(ts::pg_catalog.timestamp) FROM t1;
SELECT max(ts::pg_catalog.timestamp) < '2010-01-01 00:00:00' FROM t1;
SELECT max(dt::pg_catalog.timestamp) < '2010-01-01 00:00:00' FROM t1;
SELECT max(ts::pg_catalog.timestamp) > '2010-01-01 00:00:00' FROM t1;
SELECT max(dt::pg_catalog.timestamp) > '2010-01-01 00:00:00' FROM t1;
SELECT max(ts::pg_catalog.timestamp) = '2011-01-06 12:34:30' FROM t1;
SELECT max(dt::pg_catalog.timestamp) = '2011-01-06 12:34:30' FROM t1;
DROP TABLE t1;

--echo End of 5.5 tests

--echo #
--echo # Bug#13596893 - "ERROR 1690 (22003): BIGINT UNSIGNED VALUE IS OUT OF RANGE" ON DATE OPERATION
--echo #
CREATE TABLE t1 (
  `c1` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `c2` TIMESTAMP NOT NULL DEFAULT '0000-00-00 00:00:00'
);
INSERT INTO t1 VALUES ('2003-05-16 23:53:29','2000-01-27 23:13:41');
SELECT c2-c1 FROM t1;
SELECT * FROM t1;
SELECT TIMESTAMP'2000-01-27 23:13:41' - TIMESTAMP'2003-05-16 23:53:29'; 
--SELECT TIMESTAMP('2000-01-27','23:13:41') - TIMESTAMP('2003-05-16','23:53:29');  ---func
DROP TABLE t1;
SET sql_mode = default;
--echo #
--echo # Test for bug#11747847 - 34280: create table fails if NO_ZERO_DATE
--echo # or NO_ZERO_IN_DATE SQL mode is set.

--disable_warnings
DROP TABLE IF EXISTS t1, t2, t3;
--enable_warnings

--echo #Table creation in strict mode
SET sql_mode='NO_ZERO_DATE,STRICT_ALL_TABLES';

CREATE TABLE t1 (c1 TIMESTAMP DEFAULT 0);
CREATE TABLE t1 (c1 TIMESTAMP DEFAULT '0000-00-00 00:00:00');
CREATE TABLE t1 (c1 TIMESTAMP DEFAULT '2012-02-00 12:12:12');

--echo #Table creation in non-strict mode but with NO_ZERO_DATE/NO_ZERO_IN_DATE
SET sql_mode='NO_ZERO_DATE';
CREATE TABLE t1 (c1 TIMESTAMP DEFAULT 0);
CREATE TABLE t2 (c1 TIMESTAMP DEFAULT '0000-00-00 00:00:00');
SET sql_mode='NO_ZERO_IN_DATE';
CREATE TABLE t3 (c1 TIMESTAMP DEFAULT '2012-02-00 12:12:12');
DROP TABLE t1, t2, t3;

--echo #Table creation with out any SQL modes
SET sql_mode='';
CREATE TABLE t1 (c1 TIMESTAMP DEFAULT 0);
CREATE TABLE t2 (c1 TIMESTAMP DEFAULT '0000-00-00 00:00:00');
CREATE TABLE t3 (c1 TIMESTAMP DEFAULT '2012-02-00 12:12:12');
DROP TABLE t1, t2, t3;

CREATE TABLE t1 (c1 INT);
--echo #Alter table in strict mode  with NO_ZERO_DATE/NO_ZERO_IN_DATE
SET sql_mode='NO_ZERO_DATE,STRICT_ALL_TABLES';
ALTER TABLE t1 ADD c2 TIMESTAMP DEFAULT 0;
ALTER TABLE t1 ADD c2 TIMESTAMP DEFAULT '0000-00-00';
SET sql_mode='NO_ZERO_IN_DATE,STRICT_ALL_TABLES';
ALTER TABLE t1 ADD c2 TIMESTAMP DEFAULT '2012-02-00';


--echo #Alter table with out any SQL modes
SET sql_mode='';
ALTER TABLE t1 ADD c2 TIMESTAMP DEFAULT 0;
ALTER TABLE t1 ADD c3 TIMESTAMP DEFAULT '0000-00-00';
ALTER TABLE t1 ADD c4 TIMESTAMP DEFAULT '2012-02-00';
DROP TABLE t1;

set sql_mode to default;
--echo # END of Test for bug#11747847 - 34280

--echo #
--echo # End of 5.6 tests
--echo #
SET sql_mode = default;
--Type Conversion
select cast(111111111111 as timestamp );
select cast('111111111111' as timestamp );
select cast('11-11-11 11:11:11'::pg_catalog.timestamp as timestamp );
select cast('11-11-11 11:11:11'::pg_catalog.timestamptz as timestamp );
select cast('11-11-11 11:11:11'::pg_catalog.date as timestamp );

select cast(111111111111 as timestamp(3) );
select cast('111111111111' as timestamp(3) );
select cast('11-11-11 11:11:11'::pg_catalog.timestamp as timestamp(3) );
select cast('11-11-11 11:11:11'::pg_catalog.timestamptz as timestamp(3) );
select cast('11-11-11 11:11:11'::pg_catalog.date as timestamp(3) );


-- mysql type to mysql type
create table t1(id timestamp);
insert into t1 values ( cast('11-11-11 11:11:11.111111' as date) );
insert into t1 values ( cast('11-11-11 11:11:11.111111' as datetime) );
insert into t1 values ( cast('11-11-11 11:11:11.111111' as datetime(4)) );
insert into t1 values ( cast('11-11-11 11:11:11.111111' as timestamp) );
insert into t1 values ( cast('11-11-11 11:11:11.111111' as timestamp(4))  );
--insert into t1 values ( cast('11-11-11 11:11:11.111111' as time  ));
--insert into t1 values ( cast('11-11-11 11:11:11.111111' as time(4)  ));

insert into t1 values ( cast('11-11-11 11:11:11.111111' as date)  );
insert into t1 values ( cast('11-11-11 11:11:11.111111' as datetime)  );
insert into t1 values ( cast('11-11-11 11:11:11.111111' as datetime(5))  );
insert into t1 values ( cast('11-11-11 11:11:11.111111' as timestamp)  );
insert into t1 values ( cast('11-11-11 11:11:11.111111' as timestamp(5)) );
--insert into t1 values ( cast('11-11-11 11:11:11.111111' as time)  );
--insert into t1 values ( cast('11-11-11 11:11:11.111111' as time(5))  );

select * from t1;

drop table t1;

create table t1(id timestamp(5));
insert into t1 values ( cast('11-11-11 11:11:11.111111' as date) );
insert into t1 values ( cast('11-11-11 11:11:11.111111' as datetime) );
insert into t1 values ( cast('11-11-11 11:11:11.111111' as datetime(4)) );
insert into t1 values ( cast('11-11-11 11:11:11.111111' as timestamp) );
insert into t1 values ( cast('11-11-11 11:11:11.111111' as timestamp(4))  );
--insert into t1 values ( cast('11-11-11 11:11:11.111111' as time  ));
--insert into t1 values ( cast('11-11-11 11:11:11.111111' as time(4)  ));

insert into t1 values ( cast('11-11-11 11:11:11.111111' as date)  );
insert into t1 values ( cast('11-11-11 11:11:11.111111' as datetime)  );
insert into t1 values ( cast('11-11-11 11:11:11.111111' as datetime(5))  );
insert into t1 values ( cast('11-11-11 11:11:11.111111' as timestamp)  );
insert into t1 values ( cast('11-11-11 11:11:11.111111' as timestamp(5)) );
--insert into t1 values ( cast('11-11-11 11:11:11.111111' as time)  );
--insert into t1 values ( cast('11-11-11 11:11:11.111111' as time(5))  );
select * from t1;
drop table t1;

create table t1(id timestamp);
insert into t1 values ( cast('11-11-11 11:11:11.111111' as date) );

select  cast(id  as date)         from t1; 
select  cast(id  as datetime)     from t1;
select  cast(id  as datetime(3))  from t1; 
select  cast(id  as timestamp)    from t1; 
select  cast(id  as timestamp(4)) from t1;  
select  cast(id  as time)         from t1; 
select  cast(id  as time(4))      from t1;
select  cast(id as date)          from t1;
select  cast(id as datetime)      from t1;  
select  cast(id as datetime(3))   from t1;   
select  cast(id as timestamp)     from t1;    
select  cast(id as timestamp(4))  from t1;     
select  cast(id as time)          from t1;    
select  cast(id as time(4))       from t1;  
drop table t1;

create table t1(id timestamp(5));
insert into t1 values ( cast('11-11-11 11:11:11.111111' as date) );
select  cast(id  as date)         from t1; 
select  cast(id  as datetime)     from t1;
select  cast(id  as datetime(3))  from t1; 
select  cast(id  as timestamp)    from t1; 
select  cast(id  as timestamp(4)) from t1;  
select  cast(id  as time)         from t1; 
select  cast(id  as time(4))      from t1;
select  cast(id as date)          from t1;
select  cast(id as datetime)      from t1;  
select  cast(id as datetime(3))   from t1;   
select  cast(id as timestamp)     from t1;    
select  cast(id as timestamp(4))  from t1;     
select  cast(id as time)          from t1;    
select  cast(id as time(4))       from t1;  
drop table t1;

create table timestamp_01(a1 timestamp);
set sql_mode ='';
insert into timestamp_01 values('1111111111111.1');
insert into timestamp_01 values('111111111111111.1');


set sql_mode=default;
insert into timestamp_01 values('1111111111111.1');
insert into timestamp_01 values('111111111111111.1');
select * from timestamp_01;

drop table timestamp_01;