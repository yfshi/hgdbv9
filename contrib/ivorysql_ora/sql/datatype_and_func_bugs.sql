--
-- Mainly contains bug-modified test cases for built-in datatypes and built-in functions.
--

/* Begin - BUG#Z040 */
create table dtos_tb1tidid004134318(dtos_clo interval day to second);
insert into dtos_tb1tidid004134318 values(interval '--2 07:16:23' day to second);
insert into dtos_tb1tidid004134318 values(interval ' 2 07:16:23' day to second);
insert into dtos_tb1tidid004134318 values(interval ' -2 07:16:23' day to second);
select * from dtos_tb1tidid004134318;
/* End - BUG#Z040 */

/* Begin - BUG#Z104 */
CREATE FUNCTION f_noparam() 
RETURN int 
IS 
BEGIN  
RETURN 1; 
END;
/
/* End - BUG#Z104 */

/* Begin - BUG#Z066 */
alter session set NLS_DATE_FORMAT='yyyy-mm-dd hh24:mi:ss';
select add_months('2022-08-23',3) from dual;
/* End - BUG#Z066 */

/* Begin - BUG#Z091 */
alter session set NLS_LENGTH_SEMANTICS='BYTE';
create table char_tb(char_clo char(3));
insert into char_tb values('测试');
alter session set NLS_LENGTH_SEMANTICS='CHAR';
create table char_tb2(char_clo char(3));
insert into char_tb2 values('测试');
/* End - BUG#Z091 */

/* Begin - BUG#Z106 */
create schema s1;
create function s1.f_alter(arg1 number)
return int
is
begin
return 1;
end;
/

create function s1.f_alter(arg1 number, arg2 number)
return int
is
begin
return 1;
end;
/

create function s1.f_alter(arg1 OUT int)
return number
is
begin
return;
end;
/

create function s1.f_alter(arg1 text)
return int
is
begin
return 1;
end;
/

create function s1.f_alter(arg1 number, arg2 number, arg3 number default 10)
return int
is
begin
return 1;
end;
/

alter function s1.f_alter compile;
alter function s1.f_alter(arg1 number) compile;
alter function s1.f_alter(arg1 number, arg2 number) compile;
alter function s1.f_alter(arg1 OUT int) compile;
alter function s1.f_alter(arg1 text) compile;
alter function s1.f_alter(arg1 number, arg2 number, arg3 number default 10) compile;
alter function s1.f_alter(arg1 number, arg2 number, arg3 number) compile;
/* End - BUG#Z106 */


-- clean
drop table dtos_tb1tidid004134318;
drop table char_tb2;
drop table char_tb;
drop function f_noparam();
drop function s1.f_alter(arg1 number);
drop function s1.f_alter(arg1 number, arg2 number);
drop function s1.f_alter(arg1 OUT int);
drop function s1.f_alter(arg1 text);
drop function s1.f_alter(arg1 number, arg2 number, arg3 number);

/* Begin - BUG:M0000085 */
CREATE TABLE hashpart_numeric (
    a NUMBER(6), 
    b BINARY_FLOAT, 
    c BINARY_DOUBLE
) PARTITION BY HASH (a, b, c);
CREATE TABLE P_ID_1 partition of hashpart_numeric for VALUES WITH (MODULUS 4,REMAINDER 0);

CREATE TABLE hashpart_datetime (
    a date, 
    b timestamp, 
    c timestamp with time zone,
	d timestamp with local time zone
) PARTITION BY HASH (a, b, c, d);
CREATE TABLE P_ID_2 partition of hashpart_datetime for VALUES WITH (MODULUS 4,REMAINDER 0);

CREATE TABLE hashpart_interval (
    a interval year to month,
	b interval day to second
) PARTITION BY HASH (a, b);
CREATE TABLE P_ID_3 partition of hashpart_interval for VALUES WITH (MODULUS 4,REMAINDER 0);

CREATE TABLE hashpart_character (
    a char(10 char),
	b char(10 byte),
	c varchar2(10 char),
	d varchar2(10 byte)
) PARTITION BY HASH (a, b, c, d);
CREATE TABLE P_ID_4 partition of hashpart_character for VALUES WITH (MODULUS 4,REMAINDER 0);

drop table hashpart_numeric;
drop table hashpart_datetime;
drop table hashpart_interval;
drop table hashpart_character;
/* End - BUG:M0000085 */


--bug#Z4327
CREATE TABLE hh_adds_static_day (
    id SERIAL PRIMARY KEY,
    work_hour INT,
    create_time TIMESTAMP
);

 INSERT INTO hh_adds_static_day (work_hour, create_time)
SELECT
    (RANDOM() * 16)::INT AS work_hour,
    '2020-09-25 08:00:00'::timestamp + (i * INTERVAL '1' day) AS create_time
FROM generate_series(0, 1999999) AS s(i);


-- encourage use of parallel plans
set parallel_setup_cost=0;
set parallel_tuple_cost=0;
set min_parallel_table_scan_size=0;
set max_parallel_workers_per_gather=4;

analyze hh_adds_static_day;

EXPLAIN (costs off) WITH result AS (
    SELECT sum(work_hour) AS work_hour_sum,
           TO_DATE('2020-10-10','YYYY-MM-DD') - INTERVAL '10' DAY AS create_time
    FROM hh_adds_static_day
    WHERE create_time <= TO_DATE('2020-10-10','YYYY-MM-DD') - INTERVAL '10' DAY
    GROUP BY TO_DATE('2020-10-10','YYYY-MM-DD') - INTERVAL '10' DAY
)
SELECT create_time, work_hour_sum FROM result;

--clean data
drop table hh_adds_static_day;

--bug#Z4345
create table tb(c1 int);
create table tb2(c1 smallint);

create table srb1(id char(10 char));
create table srb2(id char(20 byte));
create table srb3(id varchar2(10 char));
create table srb4(id varchar2(10 byte));

insert into srb1 values('100');
insert into srb2 values('100');
insert into srb3 values('100');
insert into srb4 values('100');

--ok
insert into tb(c1) select case when 1=1 then '100' else '200' end from dual;
insert into tb2(c1) select case when 1=1 then '100' else '200' end from dual;

select * from tb;
select count(*) from tb where c1 = 100;
select * from tb2;

--ok
insert into tb select * from srb1;
insert into tb select * from srb2;
insert into tb select * from srb3;
insert into tb select * from srb4;

insert into tb2 select * from srb1;
insert into tb2 select * from srb2;
insert into tb2 select * from srb3;
insert into tb2 select * from srb4;


insert into srb1 values('100a');
insert into srb2 values('100a');
insert into srb3 values('100a');
insert into srb4 values('100a');


--error
insert into tb select * from srb1;
insert into tb select * from srb2;
insert into tb select * from srb3;
insert into tb select * from srb4;

insert into tb2 select * from srb1;
insert into tb2 select * from srb2;
insert into tb2 select * from srb3;
insert into tb2 select * from srb4;

--clean data
drop table tb;
drop table tb2;
drop table srb1;
drop table srb2;
drop table srb3;
drop table srb4;

--test fk
set ivorysql.enable_ora_default_collation = off;
create table pktable(id1 integer primary key,id2 varchar2(4) unique,id3 real unique);

create table fktable(x1 integer references pktable(id1), x2 varchar2(4) references pktable(id2), x3 real references pktable(id3),
x4 varchar2(256),x5 smallint);


--ok
alter table fktable add constraint fk_2_1 foreign key (x2) references pktable(id1);

--ok
insert into pktable values(1,'23',45.5);
insert into pktable values(2,'abc',45.6);
insert into pktable values(3,'2',45.7);

--error
insert into fktable values(1,'23',45.5);

--ok
insert into fktable values(1,'2',45.5);

drop table fktable;
drop table pktable;

--req#00001426 
create table test_null(id int,name varchar2(10));
insert into test_null values(1,'12345');

--1 row
select * from test_null where substr(name,6,1) is null;

--1 row
select * from test_null where substr(name,6) is null;

--1 row
select * from test_null where substr(name,-6,1) is null;

--1 row
select * from test_null where substr(name,-6) is null;

--no row
select * from test_null where substr(name,5,1) is null;

--no row
select * from test_null where substr(name,5) is null;

--no row
select * from test_null where substr(name,-5,1) is null;

--no row
select * from test_null where substr(name,-5) is null;

--1 row
select * from test_null where substr(name,5,-1) is null;

--1 row
select * from test_null where substr(name, 2,-1) is null;

--1 row
select * from test_null where substr(name,2,0) is null;


--no row
select * from test_null where substr(name, -1,1) is null;

--no row
select * from test_null where substr(name, -5,1) is null;

--print 1
select substr(name, -5,1) from test_null;


--1 row
select * from test_null where substr(name, -6,1) is null;


--no row
select * from test_null where instr(name, '1',1) is null;

--no row
select * from test_null where instr(name,'1',2) is null;

--print 0
select instr(name, '1',0) from test_null;


--print 0
select instr(name, '1',2) from test_null;

--print 1
select instr(name, '1',1) from test_null;

--print 1
select instr(name, '1',-1) from test_null;


--print 0
select instr(name, '1',-0) from test_null;

--clean data
drop table test_null;


-- BUG#6021
set ivorysql.enable_emptystring_is_NULL to on;
create table test_isnull(id int, name varchar2(200));
insert into test_isnull values(1, 'hello');
insert into test_isnull values(2, '   ');
insert into test_isnull values(3, '');
insert into test_isnull values(4, null);
set ivorysql.enable_emptystring_to_NULL to off;
insert into test_isnull values(5, '');
reset ivorysql.enable_emptystring_to_NULL;

select *,name is null from test_isnull;
select * from test_isnull where name is null;

select trim('   ') is null;
select (trim('   ') || trim('   ')) is null;
select *,trim(name) is null from test_isnull;
select *,(trim(name) || trim(name)) is null from test_isnull;

select regexp_replace('abc', 'abc') is null;
select *,regexp_replace(name, 'hello') is null from test_isnull;

drop table test_isnull;
reset ivorysql.enable_emptystring_is_NULL;


--bug#Z6876
create table test_raw_blob(id raw, b blob, ids bytea);

insert into test_raw_blob values('\xab','\xab', '\xab');

select * from test_raw_blob;

set ivorysql.hex_bytea = on;

select * from test_raw_blob;

set ivorysql.disable_hex_bytea_for_blob = off;

select * from test_raw_blob;

drop table test_raw_blob;

/* Begin - OA-REQ-2131 */
set nls_date_format = 'YYYY-MM-DD';
set nls_timestamp_format = 'YYYY-MM-DD HH24:MI:SS.FF6';
set nls_timestamp_tz_format = 'YYYY-MM-DD HH24:MI:SS.FF6 TZH:TZM';

-- function is strict
select to_date(null);
select to_date('2026-1-1', null);
select to_date('2026-1-1', 'YYYY-MM-DD', null);
select to_timestamp(null);
select to_timestamp('2026-1-1', null);
select to_timestamp('2026-1-1', 'YYYY-MM-DD', null);
select to_timestamp_tz(null);
select to_timestamp_tz('2026-1-1', null);
select to_timestamp_tz('2026-1-1', 'YYYY-MM-DD', null);

-- ignore fmt so using nls_xxx_format
-- ok
select to_date('2026-1-2');
select to_timestamp('2026-1-2');
select to_timestamp_tz('2026-1-2');
-- error
select to_date('2026-13-2');
select to_timestamp('2026-13-2');
select to_timestamp_tz('2026-13-2');
-- ok
select to_date('2026-13-2', 'YYYY-DD-MM');
select to_timestamp('2026-13-2', 'YYYY-DD-MM');
select to_timestamp_tz('2026-13-2', 'YYYY-DD-MM');
-- nlsparam not implemented only a dummy param
select to_date('2026-13-2', 'YYYY-DD-MM', 'xxx');
select to_timestamp('2026-13-2', 'YYYY-DD-MM', 'xxx');
select to_timestamp_tz('2026-13-2', 'YYYY-DD-MM', 'xxx');

-- empty string always equal to null
select to_date('');
select to_timestamp('');
select to_timestamp_tz('');
set ivorysql.enable_emptystring_to_null to off;
select to_date('');
select to_timestamp('');
select to_timestamp_tz('');
reset ivorysql.enable_emptystring_to_null;

create table test_oa_2131(
 col1 date,
 col2 timestamp without time zone,
 col3 timestamp with time zone
);

insert into test_oa_2131 values('2026-4-1', '2026-4-1 11:12:13.123', '2026-4-1 11:12:13.123456 +08:00');

select to_date(col1) from test_oa_2131;	-- ok
select to_date(col1, 'MM-DD-YYYY') from test_oa_2131;	-- error
set nls_date_format = 'MM-DD-YYYY';
select to_date(col1, 'MM-DD-YYYY') from test_oa_2131;	-- ok

select to_timestamp(col2) from test_oa_2131;	-- ok
select to_timestamp(col2, 'MM-DD-YYYY') from test_oa_2131;	-- error
set nls_timestamp_format = 'MM-DD-YYYY';
select to_timestamp(col2, 'MM-DD-YYYY') from test_oa_2131;	-- ok

select to_timestamp_tz(col3) from test_oa_2131;	-- ok
select to_timestamp_tz(col3, 'MM-DD-YYYY') from test_oa_2131;	-- error
set nls_timestamp_tz_format = 'MM-DD-YYYY';
select to_timestamp_tz(col3, 'MM-DD-YYYY') from test_oa_2131;	-- ok

-- ok
select to_date(10012026);
select to_timestamp(10012026);
select to_timestamp_tz(10012026);

reset nls_date_format;
reset nls_timestamp_format;
reset nls_timestamp_tz_format;
drop table test_oa_2131;
/* End - OA-REQ-2131 */