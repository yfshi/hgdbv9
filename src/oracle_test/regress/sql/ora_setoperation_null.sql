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
