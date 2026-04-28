--
-- ORA_COMPATIBLE_ORACLE_MISC
-- Tests for Oracle Compatible Features in this file.
-- Most of the tests that appear in this file are Oracle features
-- that do not have individual requirement number.
--

/* Begin - BUG#M0000217 */
create table user_info(id int, uname varchar2(64));
insert into user_info values(1, '111');
insert into user_info values(2, '222');
insert into user_info values(3, '333');
insert into user_info values(4, null);

--  ORDER BY clause supports aliases(Oracle-compatible)
-- success
select id, (case uname when null then null else uname end) as bm from user_info order by bm;
select id, (case uname when null then null else uname end) as bm from user_info order by length(bm);
select id, (case uname when null then null else uname end) as bm from user_info order by length(case uname when null then null else uname end);
select id, uname as id from user_info tbl order by tbl.id;

-- error
select id, uname as id from user_info order by id;
select id, (case uname when null then null else uname end) as id from user_info order by length(id);
select id, (case uname when null then null else uname end) as id from user_info order by id-100;

-- Aliases for sub-query in FROM are optional.
-- OK
select id, uname from (select * from user_info);
select id, uname from (select * from user_info) bm;

-- clean
drop table user_info;
/* End - BUG#M0000217 */

/* Begin - BUG#M0000390 */
create table test_m390(id int, name varchar2(24));
insert into test_m390 values(1, 'zhangsan');
insert into test_m390 values(2, 'lisi');
insert into test_m390 values(3, 'wangerma');
select * from test_m390;
delete test_m390 where id = 2;
select * from test_m390;
delete test_m390;
select * from test_m390;
drop table test_m390;
/* End - BUG#M0000390 */

/* Begin - BUG#M0000408 */
create table testm#408$#(c#408$# integer);
insert into testm#408$#(c#408$#) values(123);
select c#408$# from testm#408$#;
drop table testm#408$#;
/* End - BUG#M0000408 */

/* Begin - BUG#M0000424 */
create table test_m424(item varchar2(64), year timestamp, quantity int);
select nvl(item, 'aaa') a , nvl(year, '1990-1-1') b , sum(quantity) from test_m424 group by item, rollup(year);
drop table test_m424;
/* End - BUG#M0000424 */

/* Begin - BUG#Z1423 */
-- bug case.
SET ivorysql.identifier_case_switch=interchange;
create table test_z1423(a number,b varchar2(50));
create or replace view v_test_z1423 as select to_number(b) from test_z1423;
select pg_get_viewdef('v_test_z1423'::regclass);	-- "TO_NUMBER"
SET ivorysql.identifier_case_switch=normal;
select pg_get_viewdef('v_test_z1423'::regclass);	-- "to_number"
drop view v_test_z1423;
drop table test_z1423;

-- table name is keyword.
SET ivorysql.identifier_case_switch=interchange;
create table "UNION"(a number,b varchar2(50));
create or replace view v_test_z1423 as select to_number(b) from "UNION";
select pg_get_viewdef('v_test_z1423'::regclass);	-- "TO_NUMBER" and "UNION"
SET ivorysql.identifier_case_switch=normal;
select pg_get_viewdef('v_test_z1423'::regclass);	-- "to_number" and "union"

SET ivorysql.identifier_case_switch=interchange;
drop view v_test_z1423;
drop table "UNION";
SET ivorysql.identifier_case_switch=normal;
/* End - BUG#Z1423 */


--bug#M437
create table merge_tab(id timestamp, name varchar2(256));

SET ivorysql.insert_enable_alias TO on;

merge into merge_tab a using (select null modify_time,'test' name from dual) b on
(a.name = b.name)
when not matched then
  insert(a.id,a.name) values(b.modify_time,b.name);


 merge into merge_tab a using (select '2024-10-24' modify_time,'test1' name from dual) b on
(a.name = b.name)
when not matched then
  insert(a.id,a.name) values(b.modify_time,b.name);

create table test_a as select 'a' AS ids;

insert into test_a values('we are a varchar2 type');

select * from test_a ORDER BY ids;

create table m3_meta_schema(uuid varchar2(256),
name varchar2(256),
modify_time TIMESTAMP(2),
alias varchar2(256),
related_meta text,
schema_type text
);

MERGE INTO m3_meta_schema a using (SELECT '839724555' uuid, NULL modify_time,
'test' name,'test' alias,'test' related_meta,'1' schema_type FROM dual)
b ON (a.name = b.name)
WHEN NOT matched THEN INSERT(a.uuid,a.name,a.modify_time,a.alias,a.related_meta,
a.schema_type) VALUES(b.uuid,b.name,b.modify_time,b.alias,b.related_meta,b.schema_type);

--namedata vs varchar2
SELECT COUNT(*) FROM pg_class WHERE relname = 'm3_meta_schema';

--text vs varchar2
SELECT COUNT(*) FROM m3_meta_schema WHERE related_meta = 'test';

--clean data
DROP TABLE m3_meta_schema;
DROP TABLE test_a;
DROP TABLE merge_tab;

--end bug#M437

/* Begin - BUG#Z1488 */
select true::varchar2(1 byte) from dual;
select true::varchar2(1 char) from dual;
select true::varchar2 from dual;
select false::varchar2(1 byte) from dual;
select false::varchar2(1 char) from dual;
select false::varchar2 from dual;

select true::char(1 byte) from dual;
select true::char(1 char) from dual;
select true::char from dual;
select false::char(1 byte) from dual;
select false::char(1 char) from dual;
select false::char from dual;

/*
 * Note:
 * I think about it and I don't plan to change the conversion of boolean to text for the time being. 
 * I am worried that any other third-party programs such as extensions may not expect or know this.
 * For now, I will only handle Oracle-compatible character types.
 */
select true::text from dual;
select false::text from dual;

/* End - BUG#Z1488 */

/* Begin - BUG#Z1438 */
create table test_z1438(a int);
create or replace procedure proc_z1438
is
szSql varchar2(500);
begin
szSql := 'DROP TABLE test_z1438';
execute immediate szSql;
end;
/
call proc_z1438();
/* End - BUG#Z1438 */

/* Begin - BUG#Z1506 */
create table test_z1506(a number);
insert into test_z1506 select case when 1=1 then '1' else '2' end from dual;
select case '1' when a then '1' else '2' end from test_z1506;
drop table test_z1506;
/* End - BUG#Z1506 */

/* Begin - BUG#M0000475 */
set ivorysql.enable_pg_interval_datatype to off;
select interval(2) '1 days';
select interval '1 days';
create table test_m475(a interval);
set ivorysql.enable_pg_interval_datatype to on;
select interval(2) '1 days';
select interval '1 days';
create table test_m475(a interval);
drop table test_m475;
reset ivorysql.enable_pg_interval_datatype;
/* End - BUG#M0000475 */

--bug#Z4645
show ivorysql.identifier_case_switch;

create database test_dwdai_id;
create user enrol11 superuser;

\c test_dwdai_id enrol11
show ivorysql.identifier_case_switch;

\c regression enrol11
alter system set ivorysql.identifier_case_switch = interchange;
select pg_reload_conf();
alter database test_dwdai_id set ivorysql.identifier_case_switch = interchange;

\c test_dwdai_id enrol11
show ivorysql.identifier_case_switch;

\c regression enrol11
show ivorysql.identifier_case_switch;

set ivorysql.identifier_case_switch = lowercase;
show ivorysql.identifier_case_switch;

set ivorysql.identifier_case_switch = normal;
show ivorysql.identifier_case_switch;

reset ivorysql.identifier_case_switch;
show ivorysql.identifier_case_switch;

--recover
alter system set ivorysql.identifier_case_switch = normal;
select pg_reload_conf();
\c test_dwdai_id enrol11
\c regression enrol11
show ivorysql.identifier_case_switch;

declare
    name text;
begin
        select into name 10::regrole;
		execute format('set session authorization %s;',name);
end;
/
drop user enrol11;
drop database test_dwdai_id;

--bug#M542
create table public.test_id_bugm542(id varchar2(256));

create index test_bugm542_index on test_id_bugm542(id);

create table sys.test_idx_bugm542(id varchar2(256));
create index test_indexx_bugm542 on sys.test_idx_bugm542(id);

create sequence public.test_seq_bugm542 start with 100 scale extend maxvalue 9999;
create sequence sys.test_seq_bugm542 start with 100 scale extend maxvalue 9999;


select column_name,data_type from information_schema.columns where table_name = 'test_id_bugm542';

select schemaname,relname from pg_stat_sys_tables where relname = 'test_id_bugm542' or relname = 'test_idx_bugm542';
select schemaname,relname from pg_stat_xact_sys_tables where relname = 'test_id_bugm542' or relname = 'test_idx_bugm542';
select schemaname,relname from pg_stat_user_tables where relname = 'test_id_bugm542' or relname = 'test_idx_bugm542';
select schemaname,relname from pg_stat_xact_user_tables where relname = 'test_id_bugm542' or relname = 'test_idx_bugm542';


select schemaname,relname from pg_statio_sys_tables where relname = 'test_id_bugm542' or relname = 'test_idx_bugm542';
select schemaname,relname from pg_statio_user_tables where relname = 'test_id_bugm542' or relname = 'test_idx_bugm542';


select schemaname,relname from pg_stat_sys_indexes where indexrelname = 'test_bugm542_index' or indexrelname = 'test_indexx_bugm542';
select schemaname,relname from pg_stat_user_indexes where indexrelname = 'test_bugm542_index' or indexrelname = 'test_indexx_bugm542';


select schemaname,relname from pg_statio_sys_indexes where indexrelname = 'test_bugm542_index' or indexrelname = 'test_indexx_bugm542';
select schemaname,relname from pg_statio_user_indexes where indexrelname = 'test_bugm542_index' or indexrelname = 'test_indexx_bugm542';


select schemaname,relname from pg_statio_sys_sequences where relname = 'test_seq_bugm542';
select schemaname,relname from pg_statio_user_sequences where relname = 'test_seq_bugm542';


--clean data
drop table public.test_id_bugm542;
drop table sys.test_idx_bugm542;
drop sequence sys.test_seq_bugm542;
drop sequence public.test_seq_bugm542;


/* Begin - OA#1682 */
-- support in condition without '()'
select x from (values (1), (2), (3)) _(x) where x in 1;
select x from (values (1), (2), (3)) _(x) where x in 1 + 3 - 2;
select x from (values (1), (2), (3)) _(x) where x in 3 % 2;
select x from (values (1), (2), (3)) _(x) where x in int4(1) * int8(2);

-- support minus
select x from (values (row(1, 2)), (row(1, 3))) _(x) minus select x from (values (row(1, 2)), (row(1, 4))) _(x);
/* End - OA#1682 */

--bug#Z6324
--ok
set ivorysql.compatbile_mode = 'pg';
set ivorysql.compatible_mode = 'oracle';
--error
set ivorysql.compatible_mode = 'mysql';

/* Begin - OA-REQ-2133 */
-- =
select 1=+1;
select 1=-1;
select 1=+-1;
select 1=-+1;

-- !=
select 1!=+1;
select 1!=-1;
select 1!=+-1;
select 1!=-+1;

-- operator does not exist
select 1!-1;
select 1!==1;
select 1=!=1;
/* End - OA-REQ-2133 */

/* Begin - OA-REQ-2132 */
CREATE TABLE sales_oa_2132(channel_id NUMBER, promo_id NUMBER, amount_sold NUMBER);
INSERT INTO sales_oa_2132 VALUES(2,200, 111.1);
INSERT INTO sales_oa_2132 VALUES(2,999, 111.1);
INSERT INTO sales_oa_2132 VALUES(2,999, 222.2);
INSERT INTO sales_oa_2132 VALUES(3,300, 111.1);
INSERT INTO sales_oa_2132 VALUES(3,999, 333.3);
INSERT INTO sales_oa_2132 VALUES(3,999, 444.4);
INSERT INTO sales_oa_2132 VALUES(4,400, 111.1);
INSERT INTO sales_oa_2132 VALUES(4,999, 555.5);
INSERT INTO sales_oa_2132 VALUES(4,999, 666.6);

SELECT channel_id, promo_id, sum(amount_sold) s_sales,
       GROUPING(channel_id) gc,
       GROUPING(promo_id) gp,
       GROUPING(channel_id, promo_id) gcp,
       GROUPING(promo_id, channel_id) gpc
  FROM sales_oa_2132
  WHERE promo_id > 496
  GROUP BY CUBE(channel_id, promo_id)
  ORDER BY channel_id, promo_id, s_sales, gc;

DROP TABLE sales_oa_2132;
/* End - OA-REQ-2132 */

--bug#Z7405
create table test_id(id integer);

insert into test_id values(1);

select id year,id month,id day,id hour,id minute,id second from test_id;

drop table test_id;

/* Begin - OA-REQ-2310 */
select greatest(0,1,2) from dual;	-- 2
select greatest(0,1,null,2) from dual;	-- null
select least(0,1,2) from dual;	-- 0
select least(0,1,null,2) from dual;	-- null
/* End - OA-REQ-2310 */
