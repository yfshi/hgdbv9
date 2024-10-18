--
-- oracle like tests
--

alter session set timezone = '+8';

-- like

select '123456'::char(6) like '123%'  AS "true";
select '123456'::varchar(6) like '123%'  AS "true";
select '123456'::varchar2 like '123%'  AS "true";
select '123456'::text like '123%'  AS "true";

select '123456'::text like '123%'::varchar2  AS "true";
SELECT 123456::integer like '123%'::varchar2  AS "true";

SELECT 123456::integer like '123%'  AS "true";
SELECT 123456::int like '123%'  AS "true";
SELECT 1234::int2 like '123%'  AS "true";
SELECT 123456::int4 like '123%'  AS "true";
SELECT 123456::int8 like '123%'  AS "true";
SELECT 123456::bigint like '123%'  AS "true";
SELECT 1234::smallint like '123%'  AS "true";

SELECT 123456::real like '123%'  AS "true";
SELECT 123456::number like '123%'  AS "true";
SELECT 123456::numeric like '123%'  AS "true";

SELECT 123456::dec like '123%'  AS "true";
SELECT 123456::decimal like '123%'  AS "true";

SELECT 1234.56::float like '123%'  AS "true";
SELECT 1234.56::float4 like '123%'  AS "true";
SELECT 1234.56::float8 like '123%'  AS "true";
SELECT 1234.56::double precision like '123%'  AS "true";

select '2022-09-26'::date like '2022%'  AS "true";
SELECT '2022-09-26 16:39:20'::timestamp like '2022%'  AS "true";
select '2022-09-26 16:39:20'::timestamptz  like '2022%'  AS "true";
select '2022-09-26 16:39:20'::timestamp with local time zone like '2022%'  AS "true";
select '2022-09-26 16:39:20'::time like '16%'  AS "true";
select '08:55:08 GMT+2'::timetz like '08%'  AS "true";

select '2022-09-26 16:39:20'::clob like '2022%'  AS "true";
select '2022-09-26 16:39:20'::nclob like '2022%'  AS "true";

select '123456'::char(6) like null  AS "true";
select 123456::integer like null  AS "true";
SELECT null like '123%'  AS "true";
SELECT null like '123%'::varchar2  AS "true";

select '12\' like '12\'  AS "true";
select '12_12' like '12\_12' escape '\';
select '1212' like '12%' escape '';         --error

SELECT E'\\xDeAdBeE'like '\x%';




--   not like

select '123456'::char(6) not like '123%'  AS "false";
select '123456'::varchar(6) not like '123%'  AS "false";
select '123456'::varchar2 not like '123%'  AS "false";
select '123456'::text not like '123%'  AS "false";

select '123456'::text not like '123%'::varchar2  AS "false";
SELECT 123456::integer not like '123%'::varchar2  AS "false";

SELECT 123456::integer not like '123%'  AS "false";
SELECT 123456::int not like '123%'  AS "false";
SELECT 1234::int2 not like '123%'  AS "false";
SELECT 123456::int4 not like '123%'  AS "false";
SELECT 123456::int8 not like '123%'  AS "false";
SELECT 123456::bigint not like '123%'  AS "false";
SELECT 1234::smallint not like '123%'  AS "false";

SELECT 123456::real not like '123%'  AS "false";
SELECT 123456::number not like '123%'  AS "false";
SELECT 123456::numeric not like '123%'  AS "false";

SELECT 123456::dec not like '123%'  AS "false";
SELECT 123456::decimal not like '123%'  AS "false";

SELECT 1234.56::float not like '123%'  AS "false";
SELECT 1234.56::float4 not like '123%'  AS "false";
SELECT 1234.56::float8 not like '123%'  AS "false";
SELECT 1234.56::double precision not like '123%'  AS "false";

select '2022-09-26'::date not like '2022%'  AS "false";
SELECT '2022-09-26 16:39:20'::timestamp not like '2022%'  AS "false";
select '2022-09-26 16:39:20'::timestamptz  not like '2022%'  AS "false";
select '2022-09-26 16:39:20'::timestamp with local time zone not like '2022%'  AS "false";
select '2022-09-26 16:39:20'::time not like '16%'  AS "false";
select '08:55:08 GMT+2'::timetz not like '08%'  AS "false";

select '2022-09-26 16:39:20'::clob not like '2022%'  AS "false";
select '2022-09-26 16:39:20'::nclob not like '2022%'  AS "false";

select '123456'::char(6) not like null  AS "false";
select 123456::integer not like null  AS "false";
SELECT null not like '123%'  AS "false";
SELECT null not like '123%'::varchar2  AS "false";

select '12\' not like '12\'  AS "false";
select '12_12' not like '12\_12' escape '\';
select '1212' not like '12%' escape '';         --error

SELECT E'\\xDeAdBeE' not like '\x%';

-- test table 

create table t_ora_like (id int ,str1 varchar(8), date1 timestamp with time zone, date2 time with time zone, num int, str2 varchar(8));
insert into t_ora_like (id ,str1 ,date1 ,date2) values (123456,'test1','2022-09-26 16:39:20','2022-09-26 16:39:20');
insert into t_ora_like (id ,str1 ,date1 ,date2) values (123457,'test2','2022-09-26 16:40:20','2022-09-26 16:40:20');
insert into t_ora_like (id ,str1 ,date1 ,date2) values (223456,'test3','2022-09-26 16:41:20','2022-09-26 16:41:20');
insert into t_ora_like (id ,str1 ,date1 ,date2) values (123458,'atest4','2023-10-26 16:42:20','2023-10-26 16:42:20');
insert into t_ora_like (id ,str1 ,date1 ,date2) values (233459,'atest5','2023-10-26 16:45:20','2023-10-26 16:45:20');

select * from t_ora_like where str1 like 'test%';
select * from t_ora_like where date1 like '2022%';
select * from t_ora_like where date2 like '2023%';
select * from t_ora_like where id like '123%';
select * from t_ora_like where id like null;
select * from t_ora_like where num like '123%';
select * from t_ora_like where str2 like 'test%';

select * from t_ora_like where str1 not like 'test%';
select * from t_ora_like where date1 not like '2022%';
select * from t_ora_like where date2 not like '2023%';
select * from t_ora_like where id not like '123%';
select * from t_ora_like where id not like null;
select * from t_ora_like where num not like '123%';
select * from t_ora_like where str2 not like 'test%';


create table tb1(col date);
insert into tb1 values('20121212');
insert into tb1 values('1999-10-01');
insert into tb1 values('2023/10/10');
insert into tb1 values('3030\12\12');
alter session set NLS_DATE_FORMAT='YYYY-MM-DD';

select * from tb1 where to_char(tb1.col,'yyyy\MM\dd') like '1999\%';

set ivorysql.numerictonumber = on;
set num 10

create table tb2(col numeric);
insert into tb2 values(3e11);
insert into tb2 values(5.66);

select * from tb2;
select * from tb2 where col like '6%';

create table tb4(col number(5,2));
insert into tb4 values(-1.23);
insert into tb4 values(5);
insert into tb4 values(6.89);
insert into tb4 values(123.5);

select * from tb4;
select * from tb4 where col like '_';

set ivorysql.numerictonumber = off;

create table tb3(col binary_double);

insert into tb3 values(1);
insert into tb3 values(1.5);
insert into tb3 values(10.4);
insert into tb3 values(1.12345);
insert into tb3 values(1.56789);
insert into tb3 values(123456789);

select * from tb3;
select * from tb3 where col like '%E%';


drop table t_ora_like;
drop table tb1;
drop table tb2;
drop table tb3;
drop table tb4;

-- Begin - Bug#Z471 && SRS-MISC-COMPATDM-SQL
-- oracharchar like and not like operator
set ivorysql.enable_blankspace_compare to on;
create table ora_charchar_a(x char(10 char));
insert into ora_charchar_a values('aaa');
insert into ora_charchar_a values('a');
insert into ora_charchar_a values ('th is test');
insert into ora_charchar_a values ('th is st');
insert into ora_charchar_a values ('th is tst');
-- ivorysql.enable_blankspace_compare=on
select * from ora_charchar_a where x like '%a';	-- expected return null
select * from ora_charchar_a where x like '%a       '; -- expected return 1 rows 
select * from ora_charchar_a where x like '%st'; -- expected return 1 rows
select * from ora_charchar_a where x like '%st  ';	-- expected return 1 rows
select * from ora_charchar_a where x like '%a'::varchar2;	-- expected return null
select * from ora_charchar_a where x like '%st'::varchar2; -- expected return 1 rows
select * from ora_charchar_a where x like '%st  '::varchar2;	-- expected return 1 rows
select * from ora_charchar_a where x not like '%a';	-- expected return 5 rows
set ivorysql.enable_blankspace_compare to off;
-- ivorysql.enable_blankspace_compare=off
select * from ora_charchar_a where x like '%a';	-- expected return 2 rows
select * from ora_charchar_a where x like '%st'; -- expected return 3 rows
select * from ora_charchar_a where x like '%st  ';	-- expected return 1 rows
select * from ora_charchar_a where x like '%st   ';	-- expected return null
select * from ora_charchar_a where x like '%a'::varchar2;	-- expected return 2 rows
select * from ora_charchar_a where x like '%st'::varchar2; -- expected return 3 rows
select * from ora_charchar_a where x like '%st  '::varchar2;	-- expected return 1 rows
select * from ora_charchar_a where x not like '%a';  -- expected return 3 rows
drop table ora_charchar_a;

-- oracharbyte like and not like operator
set ivorysql.enable_blankspace_compare to on;
create table ora_charbyte_a(x char(10 byte));
insert into ora_charbyte_a values('aaa');
insert into ora_charbyte_a values('a');
insert into ora_charbyte_a values ('th is test');
insert into ora_charbyte_a values ('th is st');
insert into ora_charbyte_a values ('th is tst');
-- ivorysql.enable_blankspace_compare=on
select * from ora_charbyte_a where x like '%a';	-- expected return null
select * from ora_charbyte_a where x like '%a       '; -- expected return 1 rows 
select * from ora_charbyte_a where x like '%st'; -- expected return 1 rows
select * from ora_charbyte_a where x like '%st  ';	-- expected return 1 rows
select * from ora_charbyte_a where x like '%a'::varchar2;	-- expected return null
select * from ora_charbyte_a where x like '%st'::varchar2; -- expected return 1 rows
select * from ora_charbyte_a where x like '%st  '::varchar2;	-- expected return 1 rows
select * from ora_charbyte_a where x not like '%a';	-- expected return 5 rows
set ivorysql.enable_blankspace_compare to off;
-- ivorysql.enable_blankspace_compare=off
select * from ora_charbyte_a where x like '%a';	-- expected return 2 rows
select * from ora_charbyte_a where x like '%st'; -- expected return 3 rows
select * from ora_charbyte_a where x like '%st  ';	-- expected return 1 rows
select * from ora_charbyte_a where x like '%st   ';	-- expected return null
select * from ora_charbyte_a where x like '%a'::varchar2;	-- expected return 2 rows
select * from ora_charbyte_a where x like '%st'::varchar2; -- expected return 3 rows
select * from ora_charbyte_a where x like '%st  '::varchar2;	-- expected return 1 rows
select * from ora_charbyte_a where x not like '%a';  -- expected return 3 rows
drop table ora_charbyte_a;
/* has wild card */
set ivorysql.enable_blankspace_compare to off;
create table wildcard_a(x varchar(100));
insert into wildcard_a values('this is   a test');	-- none space at the end
insert into wildcard_a values('this is   a test     '); -- five space at the end
select * from wildcard_a where x like '%test';	-- 2 rows
select * from wildcard_a where x like 'this is___a_test '; -- 1 row
select * from wildcard_a where x like 'this is___a_test      '; -- none

-- no wild card
select * from wildcard_a where x like 'this is   a test';   -- two rows
select * from wildcard_a where x like 'this is   a test                                                 ';  -- two rows
select * from wildcard_a where x like 'this is   a test_';  -- 1 row
set ivorysql.enable_blankspace_compare to on;
-- End - Bug#Z471 && SRS-MISC-COMPATDM-SQL