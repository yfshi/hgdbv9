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
