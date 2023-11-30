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

drop table t_ora_like;

set ivorysql.compatible_mode to pg;
