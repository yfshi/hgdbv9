set ivorysql.compatible_mode to oracle;

set ivorysql.enable_setoperation_null=false;

--test not set ivorysql.enable_setoperation_null
select null as col from dual union select 'abc' from dual order by col;
select null as col from dual intersect select 'abc' from dual order by col;
select null as col from dual except select 'abc' from dual order by col;
select null as col from dual minus select 'abc' from dual order by col;

select 'abc' as col from dual union select null from dual order by col;
select 'abc' as col from dual intersect select null from dual order by col;
select 'abc' as col from dual except select null from dual order by col;
select 'abc' as col from dual minus select null from dual order by col;

select pg_typeof(value) from (select null from dual union select 'abc' from dual) as subquery(value);
select pg_typeof(value) from (select null from dual intersect select 'abc' from dual) as subquery(value);
select pg_typeof(value) from (select null from dual except select 'abc' from dual) as subquery(value);
select pg_typeof(value) from (select null from dual minus select 'abc' from dual) as subquery(value);

select pg_typeof(value) from (select 'abc' from dual union select null from dual) as subquery(value);
select pg_typeof(value) from (select 'abc' from dual intersect select null from dual) as subquery(value);
select pg_typeof(value) from (select 'abc' from dual except select null from dual) as subquery(value);
select pg_typeof(value) from (select 'abc' from dual minus select null from dual) as subquery(value);

select pg_typeof(value) from (select null from dual union select null from dual) as subquery(value);
select pg_typeof(value) from (select null from dual intersect select null from dual) as subquery(value);
select pg_typeof(value) from (select null from dual except select null from dual) as subquery(value);
select pg_typeof(value) from (select null from dual minus select null from dual) as subquery(value);

select null as col from dual union select 1 from dual order by col;
select null as col from dual intersect select 1 from dual order by col;
select null as col from dual except select 1 from dual order by col;
select null as col from dual minus select 1 from dual order by col;

select null as col from dual union select null from dual union select 1 from dual order by col;
select null as col from dual intersect select null from dual union select 1 from dual order by col;
select null as col from dual except select null from dual union select 1 from dual order by col;
select null as col from dual minus select null from dual union select 1 from dual order by col;

select null as col from dual union select null from dual union select 'abc' from dual order by col;
select null as col from dual intersect select null from dual union select 'abc' from dual order by col;
select null as col from dual except select null from dual union select 'abc' from dual order by col;
select null as col from dual minus select null from dual union select 'abc' from dual order by col;

select null as col from dual union select null from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select null as col from dual intersect select null from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select null as col from dual except select null from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select null as col from dual minus select null from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;

select null as col from dual union select 1 from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select null as col from dual intersect select 1 from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select null as col from dual except select 1 from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select null as col from dual minus select 1 from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;

select null as col from dual union select DATE '2026-03-24' from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select null as col from dual intersect select DATE '2026-03-24' from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select null as col from dual except select DATE '2026-03-24' from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select null as col from dual minus select DATE '2026-03-24' from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;

select null as col from dual union select 'abc' from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select null as col from dual intersect select 'abc' from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select null as col from dual except select 'abc' from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select null as col from dual minus select 'abc' from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;

select pg_typeof(value) from (select distinct null from dual) as subquery(value);

select null as col from dual union select DATE '2026-03-24' from dual union select 1 from dual order by col;
select null as col from dual intersect select DATE '2026-03-24' from dual intersect select 1 from dual order by col;
select null as col from dual except select DATE '2026-03-24' from dual except select 1 from dual order by col;
select null as col from dual minus select DATE '2026-03-24' from dual minus select 1 from dual order by col;

select distinct null as col from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select distinct null as col from dual intersect select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select distinct null as col from dual except select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select distinct null as col from dual minus select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;

select distinct null as col from dual union select 1 from dual order by col;
select distinct null as col from dual intersect select 1 from dual order by col;
select distinct null as col from dual except select 1 from dual order by col;
select distinct null as col from dual minus select 1 from dual order by col;

select distinct null as col from dual union select 'abc' from dual order by col;
select distinct null as col from dual intersect select 'abc' from dual order by col;
select distinct null as col from dual except select 'abc' from dual order by col;
select distinct null as col from dual minus select 'abc' from dual order by col;


-- set ivorysql.enable_setoperation_null
alter session set ivorysql.enable_setoperation_null to true;

select as col null from dual union select 'abc' from dual order by col;
select as col null from dual intersect select 'abc' from dual order by col;
select as col null from dual except select 'abc' from dual order by col;
select as col null from dual minus select 'abc' from dual order by col;

select 'abc' as col from dual union select null from dual order by col;
select 'abc' as col from dual intersect select null from dual order by col;
select 'abc' as col from dual except select null from dual order by col;
select 'abc' as col from dual minus select null from dual order by col;

select pg_typeof(value) from (select null from dual union select 'abc' from dual) as subquery(value);
select pg_typeof(value) from (select null from dual intersect select 'abc' from dual) as subquery(value);
select pg_typeof(value) from (select null from dual except select 'abc' from dual) as subquery(value);
select pg_typeof(value) from (select null from dual minus select 'abc' from dual) as subquery(value);

select pg_typeof(value) from (select 'abc' from dual union select null from dual) as subquery(value);
select pg_typeof(value) from (select 'abc' from dual intersect select null from dual) as subquery(value);
select pg_typeof(value) from (select 'abc' from dual except select null from dual) as subquery(value);
select pg_typeof(value) from (select 'abc' from dual minus select null from dual) as subquery(value);

select pg_typeof(value) from (select null from dual union select null from dual) as subquery(value);
select pg_typeof(value) from (select null from dual intersect select null from dual) as subquery(value);
select pg_typeof(value) from (select null from dual except select null from dual) as subquery(value);
select pg_typeof(value) from (select null from dual minus select null from dual) as subquery(value);

select null as col from dual union select 1 from dual order by col;
select null as col from dual intersect select 1 from dual order by col;
select null as col from dual except select 1 from dual order by col;
select null as col from dual minus select 1 from dual order by col;

select null as col from dual union select null from dual union select 1 from dual order by col;
select null as col from dual intersect select null from dual union select 1 from dual order by col;
select null as col from dual except select null from dual union select 1 from dual order by col;
select null as col from dual minus select null from dual union select 1 from dual order by col;

select null as col from dual union select null from dual union select 'abc' from dual order by col;
select null as col from dual intersect select null from dual union select 'abc' from dual order by col;
select null as col from dual except select null from dual union select 'abc' from dual order by col;
select null as col from dual minus select null from dual union select 'abc' from dual order by col;

select null as col from dual union select null from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select null as col from dual intersect select null from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select null as col from dual except select null from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select null as col from dual minus select null from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;

select null as col from dual union select 1 from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select null as col from dual intersect select 1 from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select null as col from dual except select 1 from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select null as col from dual minus select 1 from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;

select null as col from dual union select DATE '2026-03-24' from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select null as col from dual intersect select DATE '2026-03-24' from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select null as col from dual except select DATE '2026-03-24' from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select null as col from dual minus select DATE '2026-03-24' from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;

select null as col from dual union select 'abc' from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select null as col from dual intersect select 'abc' from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select null as col from dual except select 'abc' from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select null as col from dual minus select 'abc' from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;

select pg_typeof(value) from (select distinct null from dual) as subquery(value);

select null as col from dual union select DATE '2026-03-24' from dual union select 1 from dual order by col;
select null as col from dual intersect select DATE '2026-03-24' from dual union select 1 from dual order by col;
select null as col from dual except select DATE '2026-03-24' from dual union select 1 from dual order by col;
select null as col from dual minus select DATE '2026-03-24' from dual union select 1 from dual order by col;

select distinct null as col from dual union select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select distinct null as col from dual intersect select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select distinct null as col from dual except select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;
select distinct null as col from dual minus select TIMESTAMP '2026-03-24 00:52:08.067033 -07:00' from dual order by col;

select distinct null as col from dual union select 1 from dual order by col;
select distinct null as col from dual intersect select 1 from dual order by col;
select distinct null as col from dual except select 1 from dual order by col;
select distinct null as col from dual minus select 1 from dual order by col;

select distinct null as col from dual union select 'abc' from dual order by col;
select distinct null as col from dual intersect select 'abc' from dual order by col;
select distinct null as col from dual except select 'abc' from dual order by col;
select distinct null as col from dual minus select 'abc' from dual order by col;

-- Begin - Bug#8430 --
select * from (select null union select null) union select null;
select c1 from (select null as c1);
-- End - Bug#8430 --

-- Begin - Bug#8279 --
-- test view
create or replace view test_v1 as select null union select null;
\d test_v1
drop view test_v1;
create or replace view test_v1 as select null union select null union select null;
\d test_v1
drop view test_v1;
create or replace view test_v1 as select null union select null union select 1;
\d test_v1
drop view test_v1;
create or replace view test_v1 as select 1 union select null union select null;
\d test_v1
drop view test_v1;
create or replace view test_v1 as select null union select 1 union select null;
\d test_v1
drop view test_v1;
-- End - Bug#8279 --

--bug#Z8450
CREATE TABLE test_union(val VARCHAR2(20), status NUMBER(1));

INSERT INTO test_union VALUES ('A', 1);
INSERT INTO test_union VALUES (NULL, 1);
INSERT INTO test_union VALUES ('!', 1);
INSERT INTO test_union VALUES ('B', 0);

SELECT * FROM test_union WHERE val != (SELECT NULL FROM DUAL);

--clean data
drop table test_union;

--bug#8497
create table test_null1(id integer,name varchar2(256));

create table test_null2(id integer,rule_date date);


insert into test_null1 values(1,'wangqiang');
insert into test_null1 values(2,'limei');
insert into test_null1 values(3,'lixi');

insert into test_null2 values(1, to_date('2026-07-01', 'YYYY-MM-DD'));
insert into test_null2 values(2, to_date('2026-06-30', 'YYYY-MM-DD'));
insert into test_null2 values(3, to_date('2026-05-25', 'YYYY-MM-DD'));

--add where
select * from (select * from (select null as ids,id from test_null1 union select null,id from test_null2 ) where ids is null and ids != 'xiexie'
union select 1,2 from test_null2) order by id,ids;

--add order by
select * from (select * from (select null as ids,id from test_null1 union select null,id from test_null2 order by ids)
union select rule_date,id from test_null2) order by ids,id;

--add group by
select ids,id from (select * from (select * from (select null as ids,id from test_null1 union select null,id from test_null2) where ids is null and ids != 'xiexie' group by ids,id order by ids,id)
union select rule_date,id from test_null2) order by ids,id;

--ok
select ids from (select null as ids from test_null1) union select rule_date from test_null2 order by ids;

create view test_union_view1 as
select * from (select * from (select null as ids,id from test_null1 union select null,id from test_null2 ) where ids is null and ids != 'xiexie'
union select 1,2 from test_null2) order by id,ids;

create view test_union_view2 as
select * from (select * from (select null as ids,id from test_null1 union select null,id from test_null2 order by ids)
union select rule_date,id from test_null2) order by ids,id;

create view test_union_view3 as
select ids,id from (select * from (select * from (select null as ids,id from test_null1 union select null,id from test_null2) where ids is null and ids != 'xiexie' group by ids,id order by ids,id)
union select rule_date,id from test_null2) order by ids,id;

select * from test_union_view1;
select * from test_union_view2;
select  *from test_union_view3;


--clean data
drop table test_null1;
drop table test_null2;
drop view test_union_view1;
drop view test_union_view2;
drop view test_union_view3;
