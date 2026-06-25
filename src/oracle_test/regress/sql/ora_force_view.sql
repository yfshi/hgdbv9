/* Begin - ReqID:SRS-SQL-FORCEVIEW */
create view v_bug193 as select * from t_bug193;
create force view v_bug193 as select * from t_bug193;
select pg_get_viewdef('v_bug193');
create force view v_bug193 as select * from t_bug193_1;
select pg_get_viewdef('v_bug193');		-- force view defination not change
create or replace force view v_bug193 as select * from t_bug193_1;
select pg_get_viewdef('v_bug193');		-- force view defination be changed

create table test_bug193(a int);
insert into test_bug193 values(123456);
create view bug193_ok as select * from test_bug193;
select * from bug193_ok;

create force view bug193_ok as select * from t_bug193_2;
select * from bug193_ok;	-- ok
select pg_get_viewdef('bug193_ok');

create or replace force view bug193_ok as select * from t_bug193_2;
select * from bug193_ok;	-- error, view has errors
select pg_get_viewdef('bug193_ok');

create view bug193_ok_1 as select * from test_bug193;
select * from bug193_ok_1;

create table test_bug193_1(a int);
insert into test_bug193_1 values(987654);
create force view bug193_ok_1 as select * from test_bug193_1;	-- error
select pg_get_viewdef('bug193_ok_1');

create or replace force view bug193_ok_1 as select * from test_bug193_1;
select * from bug193_ok_1;
select pg_get_viewdef('bug193_ok_1');

drop table test_bug193_1 cascade;
drop table test_bug193 cascade;
drop view v_bug193;
drop view bug193_ok;

-- compile view
create force view bug193_ok_3 as select * from test_bug193_fail;
create force view bug193_ok_4 as select * from bug193_ok_3;
select * from bug193_ok_3;
select * from bug193_ok_4;

alter view bug193_ok_4 compile;		--View altered with compilation errors

create table test_bug193_fail(a int);
insert into test_bug193_fail values(1314);
alter view bug193_ok_4 compile;

select * from bug193_ok_3;
select * from bug193_ok_4;

drop table test_bug193_fail cascade;

-- the view becomes invalid after the dependent object of the view is deleted.
create table test_193(a int);
insert into test_193 values(2023);
create view v1 as select * from test_193;
create view v2 as select * from v1;
select * from v1;
select * from v2;
drop table test_193;
select * from v1;
select * from v2;
create table test_193(a int);
insert into test_193 values(2024);
select * from v1;
select * from v2;
drop table test_193;
drop view v1;
drop view v2;

-- circular dependency
create force view v1 as select * from v2;
create force view v2 as select * from v1;
select * from v1;
select * from v2;
ALTER VIEW V1 COMPILE;
ALTER VIEW V2 COMPILE;
drop view v1;
drop view v2;

-- function depend view.
create table test_193(a int);
insert into test_193 values(2023);
create view v1 as select * from test_193;
select * from v1;
create or replace function ftest193(x int) return text as
 var v1%ROWTYPE;
begin
 SELECT * INTO var FROM v1;
 RETURN 'a = ' || var.a;
end;
/
select ftest193(1);
drop table test_193;
select * from v1;		-- err
select ftest193(1);		-- err
create table test_193(a int);
insert into test_193 values(2024);
select * from v1;
select ftest193(1);
drop table test_193;
drop view v1;
drop function ftest193;
/* End - ReqID:SRS-SQL-FORCEVIEW */

/* Begin - BUG#M0000210 */
create table user_info(id int, uname varchar2(64));
insert into user_info values(1, '111');
insert into user_info values(2, '222');
insert into user_info values(3, '333');
insert into user_info values(4, null);

-- success
select id, (case uname when null then null else uname end) as bm from user_info order by bm;
select id, (case uname when null then null else uname end) as bm from user_info order by length(bm);
select id, (case uname when null then null else uname end) as bm from user_info order by length(case uname when null then null else uname end);
select id, uname as id from user_info tbl order by tbl.id;

-- error
select id, uname as id from user_info order by id;
select id, (case uname when null then null else uname end) as id from user_info order by length(id);
select id, (case uname when null then null else uname end) as id from user_info order by id-100;

drop table user_info;


create table test193(a int, b int);
insert into test193 values(1, 123);
select * from test193;
create view vtest193 as select * from test193;
select * from vtest193;
alter table test193 alter column b type varchar2(128);
select * from vtest193;
insert into test193 values(1, 'abc');
select * from vtest193;
drop view vtest193;
drop table test193;
/* End - BUG#M0000210 */

/* Begin - OA-REQ-1459 */
create schema oa_req_1459;
create force view oa_req_1459.v_req_1459(vcol1, vcol2) as select a, b from oa_req_1459.tb1;
select pg_get_viewdef('oa_req_1459.v_req_1459'); 
alter schema oa_req_1459 rename to req_1459;
select pg_get_viewdef('req_1459.v_req_1459'); 
drop view req_1459.v_req_1459;
drop schema req_1459;
/* End - OA-REQ-1459 */

/* Begin - OA-REQ-1558 */
create schema oa_req_1558;
create table oa_req_1558.test(a int);
create view oa_req_1558.vtest(va) as select a from oa_req_1558.test;
select pg_get_viewdef('oa_req_1558.vtest');	-- has schema
set search_path to oa_req_1558;
select pg_get_viewdef('oa_req_1558.vtest'); -- no schema
drop table oa_req_1558.test;
select pg_get_viewdef('oa_req_1558.vtest'); -- force view always has schema
reset search_path;
create table oa_req_1558.test(a int);
select * from oa_req_1558.vtest; -- compile ok
drop view oa_req_1558.vtest;
drop table oa_req_1558.test;
drop schema oa_req_1558;
/* End - OA-REQ-1558 */

/* Begin - BUG#5707 */
create or replace force view bug_5707_fvt(id) as select id from bug_5707_t;
create table bug_5707_t(id int);

create or replace package bug_5707_ppp as
	procedure xxx();
end;
/

create or replace package body bug_5707_ppp as
	procedure xxx as
		cursor ccc is select id from bug_5707_fvt;
		ddd ccc%ROWTYPE;
	begin
	end;
end bug_5707_ppp;
/
drop package bug_5707_ppp;
drop view bug_5707_fvt;
drop table bug_5707_t;
/* End - BUG#5707 */

/* Begin - OA-REQ-2513 */
-- test from oa2513
create table test_id(id integer,name varchar2(256));
create materialized view test_mview as select * from test_id;
create view test_view2 as select *from test_mview where id > 2;
alter table test_id alter column id type number; -- error
drop table test_id cascade;

-- test recursive invalid views
create table test_oa2513(a int, b int);
create view vtest1 as select * from test_oa2513;
create view vtest2 as select * from vtest1;
create view vtest3 as select * from vtest2;
create view vtest4 as select * from vtest3;
create view vtest5 as select * from vtest4;
alter table test_oa2513 alter column a type number; -- ok
select count(1) >= 5 from pg_force_view; -- true
select count(1) from vtest5;	-- compile all
select count(1) >=5 from pg_force_view; -- false
drop table test_oa2513 cascade;

-- materialized view current not support
create table test_oa2513(a int, b int);
create view vtest1 as select * from test_oa2513;
create view vtest2 as select * from vtest1;
create view vtest3 as select * from vtest2;
create materialized view vtest4 as select * from vtest3;
create view vtest5 as select * from vtest4;
alter table test_oa2513 alter column a type number; -- error
drop table test_oa2513 cascade;
/* End - OA-REQ-2513 */
