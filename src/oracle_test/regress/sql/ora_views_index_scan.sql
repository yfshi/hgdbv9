-- view consists of columns from one relation
create view v_oct_1 as
	select
		oid::varchar2(10)::number as id,
		ora_case_trans(relname::varchar2)::varchar2(128) as name
	from pg_class
;

-- view consists of columns from two relations
create view v_oct_2 as
	select
		oid::varchar2(10)::number as id,
		ora_case_trans(relname::varchar2)::varchar2(128) as name
	from pg_class
union
	select
		oid::varchar2(10)::number as id,
		ora_case_trans(proname::varchar2)::varchar2(128)
	from pg_proc
;

-- view consists of columns from three relations
create view v_oct_3 as
	select
		oid::varchar2(10)::number as id,
		ora_case_trans(relname::varchar2)::varchar2(128) as name
	from pg_class
union
	select
		oid::varchar2(10)::number as id,
		ora_case_trans(proname::varchar2)::varchar2(128) as name
	from pg_proc
union
	select
		oid::varchar2(10)::number as id,
		ora_case_trans(typname::varchar2)::varchar2(128) as name
	from pg_type
;

create table " AAA_asc" (id oid);

create table "zzz_DESC" (id oid);

set ivorysql.ora_views_index_scan to on;

-- prefer Index Scan not Seq Scan
set seq_page_cost = 100.0;
set random_page_cost = 1.0;

--
-- ora_case_trans() does nothing
--
set ivorysql.identifier_case_switch to normal;

-- Index Cond: relname / proname / typname = 'pg_class'::name
explain (costs off)
	select id, name from v_oct_3
	where name = 'pg_class'::name;

	select id, name from v_oct_3
	where name = 'pg_class'::name;

-- relname / proname / typname = 'pg_class'::text
explain (costs off)
	select id, name from v_oct_3
	where name = 'pg_class';

	select id, name from v_oct_3
	where name = 'pg_class';

-- Index Cond: relname / proname / typname = 'pg_class'::text
explain (costs off)
	select id, name from v_oct_3
	where name = 'pg_class'::text;

	select id, name from v_oct_3
	where name = 'pg_class'::text;

-- Index Cond: relname / proname / typname = 'pg_class'::text
explain (costs off)
	select id, name from v_oct_3
	where name = 'pg_class'::varchar;

	select id, name from v_oct_3
	where name = 'pg_class'::varchar;

-- Index Cond: relname / proname / typname = 'pg_class'::text
explain (costs off)
	select id, name from v_oct_3
	where name = 'pg_class'::varchar2;

	select id, name from v_oct_3
	where name = 'pg_class'::varchar2;

-- Index Cond: relname / proname / typname = 'pg_class'::text
explain (costs off)
	select id, name from v_oct_3
	where name = 'pg_class'::bpchar;

	select id, name from v_oct_3
	where name = 'pg_class'::bpchar;

-- Index Cond: relname / proname / typname = any ('{pg_class,pg_proc}'::name[])
explain (costs off)
	select id, name from v_oct_3
	where name = any('{pg_class,pg_proc}'::name[]);

	select id, name from v_oct_3
	where name = any('{pg_class,pg_proc}'::name[]);

-- relname / proname / typname = any ('{pg_class,pg_proc}'::text[])
explain (costs off)
	select id, name from v_oct_3
	where name = any('{pg_class,pg_proc}');

	select id, name from v_oct_3
	where name = any('{pg_class,pg_proc}');

-- Index Cond: relname / proname / typname = any ('{pg_class,pg_proc}'::text[])
explain (costs off)
	select id, name from v_oct_3
	where name = any('{pg_class,pg_proc}'::text[]);

	select id, name from v_oct_3
	where name = any('{pg_class,pg_proc}'::text[]);

-- Index Cond: relname / proname / typname = any ('{pg_class,pg_proc}'::text[])
explain (costs off)
	select id, name from v_oct_3
	where name = any('{pg_class,pg_proc}'::varchar[]);

	select id, name from v_oct_3
	where name = any('{pg_class,pg_proc}'::varchar[]);

-- Index Cond: relname / proname / typname = any ('{pg_class,pg_proc}'::text[])
explain (costs off)
	select id, name from v_oct_3
	where name = any('{pg_class,pg_proc}'::varchar2[]);

	select id, name from v_oct_3
	where name = any('{pg_class,pg_proc}'::varchar2[]);

-- Index Cond: relname / proname / typname = any ('{pg_class,pg_proc}'::text[])
explain (costs off)
	select id, name from v_oct_3
	where name = any('{pg_class,pg_proc}'::bpchar[]);

	select id, name from v_oct_3
	where name = any('{pg_class,pg_proc}'::bpchar[]);

-- Index Cond: relname / proname / typname = any ('{pg_class,pg_proc}'::name[])
explain (costs off)
	select id, name from v_oct_3
	where name in ('pg_class'::name, 'pg_proc'::name);

	select id, name from v_oct_3
	where name in ('pg_class'::name, 'pg_proc'::name);

-- relname / proname / typname = any ('{pg_class,pg_proc}'::text[])
explain (costs off)
	select id, name from v_oct_3
	where name in ('pg_class', 'pg_proc');

	select id, name from v_oct_3
	where name in ('pg_class', 'pg_proc');

-- relname / proname / typname = any ('{pg_class,pg_proc}'::text[])
explain (costs off)
	select id, name from v_oct_3
	where name in ('pg_class'::text, 'pg_proc'::text);

	select id, name from v_oct_3
	where name in ('pg_class'::text, 'pg_proc'::text);

-- relname / proname / typname = any ('{pg_class,pg_proc}'::text[])
explain (costs off)
	select id, name from v_oct_3
	where name in ('pg_class'::varchar, 'pg_proc'::varchar);

	select id, name from v_oct_3
	where name in ('pg_class'::varchar, 'pg_proc'::varchar);

-- relname / proname / typname = any ('{pg_class,pg_proc}'::text[])
explain (costs off)
	select id, name from v_oct_3
	where name in ('pg_class'::varchar2, 'pg_proc'::varchar2);

	select id, name from v_oct_3
	where name in ('pg_class'::varchar2, 'pg_proc'::varchar2);

-- relname / proname / typname = any ('{pg_class,pg_proc}'::text[])
explain (costs off)
	select id, name from v_oct_3
	where name in ('pg_class'::bpchar, 'pg_proc'::bpchar);

	select id, name from v_oct_3
	where name in ('pg_class'::bpchar, 'pg_proc'::bpchar);

-- Index Cond: relname = ($0)::text
explain (costs off)
	select name from v_oct_1
	where name = (select name from v_oct_2 order by id desc limit 1);

	select name from v_oct_1
	where name = (select name from v_oct_2 order by id desc limit 1);

-- Index Cond: relname / proname = ($0)::text
explain (costs off)
	select name from v_oct_2
	where name = (select name from v_oct_1 order by id desc limit 1);

	select name from v_oct_2
	where name = (select name from v_oct_1 order by id desc limit 1);

-- Index Cond: relname / proname = ($0)::text
explain (costs off)
	select name from v_oct_2
	where name = (select name from v_oct_3 order by id desc limit 1);

	select name from v_oct_2
	where name = (select name from v_oct_3 order by id desc limit 1);

-- Index Cond: relname = ($0)::text
explain (costs off)
	select name from v_oct_3
	where name = (select name from v_oct_2 order by id desc limit 1);

	select name from v_oct_3
	where name = (select name from v_oct_2 order by id desc limit 1);

explain (costs off)
	select name from v_oct_1
	where name in (select name from v_oct_2 order by id desc limit 1);

	select name from v_oct_1
	where name in (select name from v_oct_2 order by id desc limit 1);

explain (costs off)
	select name from v_oct_2
	where name in (select name from v_oct_1 order by id desc limit 1);

	select name from v_oct_2
	where name in (select name from v_oct_1 order by id desc limit 1);

explain (costs off)
	select name from v_oct_2
	where name in (select name from v_oct_3 order by id desc limit 1);

	select name from v_oct_2
	where name in (select name from v_oct_3 order by id desc limit 1);

explain (costs off)
	select name from v_oct_3
	where name in (select name from v_oct_2 order by id desc limit 1);

	select name from v_oct_3
	where name in (select name from v_oct_2 order by id desc limit 1);

set enable_hashjoin to off;
set enable_mergejoin to off;
explain (costs off)
	select min(id) from v_oct_1 where name in (select name from v_oct_2);

	select min(id) from v_oct_1 where name in (select name from v_oct_2);
reset enable_hashjoin;
reset enable_mergejoin;

explain (costs off)
	select min(id) from v_oct_2 where name in (select name from v_oct_1);

	select min(id) from v_oct_2 where name in (select name from v_oct_1);

explain (costs off)
	select min(id) from v_oct_2 where name in (select name from v_oct_3);

	select min(id) from v_oct_2 where name in (select name from v_oct_3);

explain (costs off)
	select min(id) from v_oct_3 where name in (select name from v_oct_2);

	select min(id) from v_oct_3 where name in (select name from v_oct_2);

explain (costs off)
	select min(v_oct_1.id) from v_oct_1 join v_oct_2
		on v_oct_1.name = v_oct_2.name;

	select min(v_oct_1.id) from v_oct_1 join v_oct_2
		on v_oct_1.name = v_oct_2.name;

explain (costs off)
	select min(v_oct_2.id) from v_oct_2 join v_oct_1
		on v_oct_2.name = v_oct_1.name;

	select min(v_oct_2.id) from v_oct_2 join v_oct_1
		on v_oct_2.name = v_oct_1.name;

explain (costs off)
	select min(v_oct_2.id) from v_oct_2 join v_oct_3
		on v_oct_2.name = v_oct_3.name;

	select min(v_oct_2.id) from v_oct_2 join v_oct_3
		on v_oct_2.name = v_oct_3.name;

explain (costs off)
	select min(v_oct_3.id) from v_oct_3 join v_oct_2
		on v_oct_3.name = v_oct_2.name;

	select min(v_oct_3.id) from v_oct_3 join v_oct_2
		on v_oct_3.name = v_oct_2.name;

-- order by
explain (costs off)
	select name from v_oct_1 order by name limit 1;

	select name from v_oct_1 order by name limit 1;

-- order by desc
explain (costs off)
	select name from v_oct_1 order by name desc limit 1;

	select name from v_oct_1 order by name desc limit 1;

--
-- ora_case_trans() converts all-uppercase to all-lowercase and vice versa
--
set ivorysql.identifier_case_switch to interchange;

-- Index Cond: relname / proname / typname = 'PG_CLASS'::name
explain (costs off)
	select id, name from v_oct_3
	where name = 'PG_CLASS'::name;

	select id, name from v_oct_3
	where name = 'PG_CLASS'::name;

-- relname / proname / typname = 'PG_CLASS'::text
explain (costs off)
	select id, name from v_oct_3
	where name = 'PG_CLASS';

	select id, name from v_oct_3
	where name = 'PG_CLASS';

-- Index Cond: relname / proname / typname = 'PG_CLASS'::text
explain (costs off)
	select id, name from v_oct_3
	where name = 'PG_CLASS'::text;

	select id, name from v_oct_3
	where name = 'PG_CLASS'::text;

-- Index Cond: relname / proname / typname = 'PG_CLASS'::text
explain (costs off)
	select id, name from v_oct_3
	where name = 'PG_CLASS'::varchar;

	select id, name from v_oct_3
	where name = 'PG_CLASS'::varchar;

-- Index Cond: relname / proname / typname = 'PG_CLASS'::text
explain (costs off)
	select id, name from v_oct_3
	where name = 'PG_CLASS'::varchar2;

	select id, name from v_oct_3
	where name = 'PG_CLASS'::varchar2;

-- Index Cond: relname / proname / typname = 'PG_CLASS'::text
explain (costs off)
	select id, name from v_oct_3
	where name = 'PG_CLASS'::bpchar;

	select id, name from v_oct_3
	where name = 'PG_CLASS'::bpchar;

-- Index Cond: relname / proname / typname = any ('{PG_CLASS,PG_PROC}'::name[])
explain (costs off)
	select id, name from v_oct_3
	where name = any('{PG_CLASS,PG_PROC}'::name[]);

	select id, name from v_oct_3
	where name = any('{PG_CLASS,PG_PROC}'::name[]);

-- relname / proname / typname = any ('{PG_CLASS,PG_PROC}'::text[])
explain (costs off)
	select id, name from v_oct_3
	where name = any('{PG_CLASS,PG_PROC}');

	select id, name from v_oct_3
	where name = any('{PG_CLASS,PG_PROC}');

-- Index Cond: relname / proname / typname = any ('{PG_CLASS,PG_PROC}'::text[])
explain (costs off)
	select id, name from v_oct_3
	where name = any('{PG_CLASS,PG_PROC}'::text[]);

	select id, name from v_oct_3
	where name = any('{PG_CLASS,PG_PROC}'::text[]);

-- Index Cond: relname / proname / typname = any ('{PG_CLASS,PG_PROC}'::text[])
explain (costs off)
	select id, name from v_oct_3
	where name = any('{PG_CLASS,PG_PROC}'::varchar[]);

	select id, name from v_oct_3
	where name = any('{PG_CLASS,PG_PROC}'::varchar[]);

-- Index Cond: relname / proname / typname = any ('{PG_CLASS,PG_PROC}'::text[])
explain (costs off)
	select id, name from v_oct_3
	where name = any('{PG_CLASS,PG_PROC}'::varchar2[]);

	select id, name from v_oct_3
	where name = any('{PG_CLASS,PG_PROC}'::varchar2[]);

-- Index Cond: relname / proname / typname = any ('{PG_CLASS,PG_PROC}'::text[])
explain (costs off)
	select id, name from v_oct_3
	where name = any('{PG_CLASS,PG_PROC}'::bpchar[]);

	select id, name from v_oct_3
	where name = any('{PG_CLASS,PG_PROC}'::bpchar[]);

-- Index Cond: relname / proname / typname = any ('{PG_CLASS,PG_PROC}'::name[])
explain (costs off)
	select id, name from v_oct_3
	where name in ('PG_CLASS'::name, 'PG_PROC'::name);

	select id, name from v_oct_3
	where name in ('PG_CLASS'::name, 'PG_PROC'::name);

-- relname / proname / typname = any ('{PG_CLASS,PG_PROC}'::text[])
explain (costs off)
	select id, name from v_oct_3
	where name in ('PG_CLASS', 'PG_PROC');

	select id, name from v_oct_3
	where name in ('PG_CLASS', 'PG_PROC');

-- relname / proname / typname = any ('{PG_CLASS,PG_PROC}'::text[])
explain (costs off)
	select id, name from v_oct_3
	where name in ('PG_CLASS'::text, 'PG_PROC'::text);

	select id, name from v_oct_3
	where name in ('PG_CLASS'::text, 'PG_PROC'::text);

-- relname / proname / typname = any ('{PG_CLASS,PG_PROC}'::text[])
explain (costs off)
	select id, name from v_oct_3
	where name in ('PG_CLASS'::varchar, 'PG_PROC'::varchar);

	select id, name from v_oct_3
	where name in ('PG_CLASS'::varchar, 'PG_PROC'::varchar);

-- relname / proname / typname = any ('{PG_CLASS,PG_PROC}'::text[])
explain (costs off)
	select id, name from v_oct_3
	where name in ('PG_CLASS'::varchar2, 'PG_PROC'::varchar2);

	select id, name from v_oct_3
	where name in ('PG_CLASS'::varchar2, 'PG_PROC'::varchar2);

-- relname / proname / typname = any ('{PG_CLASS,PG_PROC}'::text[])
explain (costs off)
	select id, name from v_oct_3
	where name in ('PG_CLASS'::bpchar, 'PG_PROC'::bpchar);

	select id, name from v_oct_3
	where name in ('PG_CLASS'::bpchar, 'PG_PROC'::bpchar);

-- Index Cond: relname = ($0)::text
explain (costs off)
	select name from v_oct_1
	where name = (select name from v_oct_2 order by id desc limit 1);

	select name from v_oct_1
	where name = (select name from v_oct_2 order by id desc limit 1);

-- Index Cond: relname / proname = ($0)::text
explain (costs off)
	select name from v_oct_2
	where name = (select name from v_oct_1 order by id desc limit 1);

	select name from v_oct_2
	where name = (select name from v_oct_1 order by id desc limit 1);

-- Index Cond: relname / proname = ($0)::text
explain (costs off)
	select name from v_oct_2
	where name = (select name from v_oct_3 order by id desc limit 1);

	select name from v_oct_2
	where name = (select name from v_oct_3 order by id desc limit 1);

-- Index Cond: relname = ($0)::text
explain (costs off)
	select name from v_oct_3
	where name = (select name from v_oct_2 order by id desc limit 1);

	select name from v_oct_3
	where name = (select name from v_oct_2 order by id desc limit 1);

explain (costs off)
	select name from v_oct_1
	where name in (select name from v_oct_2 order by id desc limit 1);

	select name from v_oct_1
	where name in (select name from v_oct_2 order by id desc limit 1);

explain (costs off)
	select name from v_oct_2
	where name in (select name from v_oct_1 order by id desc limit 1);

	select name from v_oct_2
	where name in (select name from v_oct_1 order by id desc limit 1);

explain (costs off)
	select name from v_oct_2
	where name in (select name from v_oct_3 order by id desc limit 1);

	select name from v_oct_2
	where name in (select name from v_oct_3 order by id desc limit 1);

explain (costs off)
	select name from v_oct_3
	where name in (select name from v_oct_2 order by id desc limit 1);

	select name from v_oct_3
	where name in (select name from v_oct_2 order by id desc limit 1);

set enable_hashjoin to off;
set enable_mergejoin to off;
explain (costs off)
	select min(id) from v_oct_1 where name in (select name from v_oct_2);

	select min(id) from v_oct_1 where name in (select name from v_oct_2);
reset enable_hashjoin;
reset enable_mergejoin;

explain (costs off)
	select min(id) from v_oct_2 where name in (select name from v_oct_1);

	select min(id) from v_oct_2 where name in (select name from v_oct_1);

explain (costs off)
	select min(id) from v_oct_2 where name in (select name from v_oct_3);

	select min(id) from v_oct_2 where name in (select name from v_oct_3);

explain (costs off)
	select min(id) from v_oct_3 where name in (select name from v_oct_2);

	select min(id) from v_oct_3 where name in (select name from v_oct_2);

explain (costs off)
	select min(v_oct_1.id) from v_oct_1 join v_oct_2
		on v_oct_1.name = v_oct_2.name;

	select min(v_oct_1.id) from v_oct_1 join v_oct_2
		on v_oct_1.name = v_oct_2.name;

explain (costs off)
	select min(v_oct_2.id) from v_oct_2 join v_oct_1
		on v_oct_2.name = v_oct_1.name;

	select min(v_oct_2.id) from v_oct_2 join v_oct_1
		on v_oct_2.name = v_oct_1.name;

explain (costs off)
	select min(v_oct_2.id) from v_oct_2 join v_oct_3
		on v_oct_2.name = v_oct_3.name;

	select min(v_oct_2.id) from v_oct_2 join v_oct_3
		on v_oct_2.name = v_oct_3.name;

explain (costs off)
	select min(v_oct_3.id) from v_oct_3 join v_oct_2
		on v_oct_3.name = v_oct_2.name;

	select min(v_oct_3.id) from v_oct_3 join v_oct_2
		on v_oct_3.name = v_oct_2.name;

-- order by
explain (costs off)
	select name from v_oct_1 order by name limit 1;

	select name from v_oct_1 order by name limit 1;

-- order by desc
explain (costs off)
	select name from v_oct_1 order by name desc limit 1;

	select name from v_oct_1 order by name desc limit 1;

-- restore and clean
reset seq_page_cost;
reset ivorysql.identifier_case_switch;
reset ivorysql.ora_views_index_scan;
drop table if exists " AAA_asc";
drop table if exists "zzz_DESC";
drop view if exists v_oct_1, v_oct_2, v_oct_3;


--bug#Z838
SET ivorysql.identifier_case_switch = interchange;
create or replace package pkg is
 var1 integer;
 procedure test_p(id integer);
end;
/

create or replace package body pkg is
 var3 integer;
 procedure test_p(id integer) is
 begin
   var3 := 4;
 end;
begin
  var1 := 2;
end;
/

--ok
select text from all_source where name='PKG' and type='PACKAGE' ORDER BY line;
select text from dba_source where name='PKG' and type='PACKAGE' ORDER BY line;
select text from user_source where name='PKG' and type='PACKAGE' ORDER BY line;

--ok
select object_name from all_arguments where package_name='PKG';
select object_name from dba_arguments where package_name='PKG';
select object_name from user_arguments where package_name='PKG';

--ok
select object_name from all_arguments where package_name='PKG' AND package_name='PKG';
select object_name from dba_arguments where package_name='PKG' AND package_name='PKG';
select object_name from user_arguments where package_name='PKG' AND package_name='PKG';

--ok
select procedure_name,subprogram_id from dba_procedures where object_name='PKG' ORDER BY subprogram_id;
select procedure_name,subprogram_id from all_procedures where object_name='PKG' ORDER BY subprogram_id;
select procedure_name,subprogram_id from user_procedures where object_name='PKG' ORDER BY subprogram_id;

--ok
select text,type from all_source where name='PKG' ORDER BY type,line;
select text,type from dba_source where name='PKG' ORDER BY type,line;
select text,type from user_source where name='PKG' ORDER BY type,line;

CREATE OR REPLACE FUNCTION test_f(id integer,name varchar2) RETURN integer IS
BEGIN
  RETURN 1;
end;
/

--ok
select text from all_source where name='TEST_F' and type='FUNCTION' ORDER BY line;
select text from dba_source where name='TEST_F' and type='FUNCTION' ORDER BY line;
select text from user_source where name='TEST_F' and type='FUNCTION' ORDER BY line;

--ok
select object_name from all_arguments where object_name='TEST_F';
select object_name from dba_arguments where object_name='TEST_F';
select object_name from user_arguments where object_name='TEST_F';


--ok
select object_name from all_arguments where object_name='TEST_F' AND object_name='TEST_F';
select object_name from dba_arguments where object_name='TEST_F' AND object_name='TEST_F';
select object_name from user_arguments where object_name='TEST_F' AND object_name='TEST_F';

--ok
select procedure_name,subprogram_id from dba_procedures where object_name='TEST_F' ORDER BY subprogram_id;
select procedure_name,subprogram_id from all_procedures where object_name='TEST_F' ORDER BY subprogram_id;
select procedure_name,subprogram_id from user_procedures where object_name='TEST_F' ORDER BY subprogram_id;

--ok
select text,type from all_source where name='TEST_F' ORDER BY type,line;
select text,type from dba_source where name='TEST_F' ORDER BY type,line;
select text,type from user_source where name='TEST_F' ORDER BY type,line;


select object_name from all_arguments where object_name='TEST_F' OR OBJECT_NAME = 'PKG' ORDER BY object_name;
select object_name from dba_arguments where object_name='TEST_F' OR OBJECT_NAME = 'PKG' ORDER BY object_name;
select object_name from user_arguments where object_name='TEST_F'OR  OBJECT_NAME = 'PKG' ORDER BY object_name;

select object_name from all_arguments where object_name='TEST_F' OR OBJECT_NAME = 'PKG' GROUP BY OBJECT_NAME
ORDER BY object_name;
select object_name from dba_arguments where object_name='TEST_F' OR OBJECT_NAME = 'PKG' GROUP BY OBJECT_NAME
ORDER BY object_name;;
select object_name from user_arguments where object_name='TEST_F' OR OBJECT_NAME = 'PKG' GROUP BY OBJECT_NAME ORDER BY object_name;


--again test normal
set ivorysql.identifier_case_switch = normal;

--ok
select text from all_source where name='pkg' and type='PACKAGE' ORDER BY line;
select text from dba_source where name='pkg' and type='PACKAGE' ORDER BY line;
select text from user_source where name='pkg' and type='PACKAGE' ORDER BY line;

--ok
select object_name from all_arguments where package_name='pkg';
select object_name from dba_arguments where package_name='pkg';
select object_name from user_arguments where package_name='pkg';

--ok
select object_name from all_arguments where package_name='pkg' AND package_name='pkg';
select object_name from dba_arguments where package_name='pkg' AND package_name='pkg';
select object_name from user_arguments where package_name='pkg' AND package_name='pkg';

--ok
select procedure_name,subprogram_id from dba_procedures where object_name='pkg' ORDER BY subprogram_id;
select procedure_name,subprogram_id from all_procedures where object_name='pkg' ORDER BY subprogram_id;
select procedure_name,subprogram_id from user_procedures where object_name='pkg' ORDER BY subprogram_id;

--ok
select text,type from all_source where name='pkg' ORDER BY type,line;
select text,type from dba_source where name='pkg' ORDER BY type,line;
select text,type from user_source where name='pkg' ORDER BY type,line;


--ok
select text from all_source where name='test_f' and type='FUNCTION' ORDER BY line;
select text from dba_source where name='test_f' and type='FUNCTION' ORDER BY line;
select text from user_source where name='test_f' and type='FUNCTION' ORDER BY line;

--ok
select object_name from all_arguments where object_name='test_f';
select object_name from dba_arguments where object_name='test_f';
select object_name from user_arguments where object_name='test_f';


--ok
select object_name from all_arguments where object_name='test_f' OR OBJECT_NAME = 'pkg' ORDER BY object_name;
select object_name from dba_arguments where object_name='test_f' OR OBJECT_NAME = 'pkg' ORDER BY object_name;
select object_name from user_arguments where object_name='test_f' OR  OBJECT_NAME = 'pkg' ORDER BY object_name;

select object_name from all_arguments where object_name='test_f' OR OBJECT_NAME = 'pkg' GROUP BY OBJECT_NAME
ORDER BY object_name;
select object_name from dba_arguments where object_name='test_f' OR OBJECT_NAME = 'pkg' GROUP BY OBJECT_NAME
ORDER BY object_name;;
select object_name from user_arguments where object_name='test_f' OR OBJECT_NAME = 'pkg' GROUP BY OBJECT_NAME ORDER BY object_name;


--ok
select object_name from all_arguments where object_name='test_f' AND object_name='test_f';
select object_name from dba_arguments where object_name='test_f' AND object_name='test_f';
select object_name from user_arguments where object_name='test_f' AND object_name='test_f';

--ok
select procedure_name,subprogram_id from dba_procedures where object_name='test_f' ORDER BY subprogram_id;
select procedure_name,subprogram_id from all_procedures where object_name='test_f' ORDER BY subprogram_id;
select procedure_name,subprogram_id from user_procedures where object_name='test_f' ORDER BY subprogram_id;

--ok
select text,type from all_source where name='test_f' ORDER BY type,line;
select text,type from dba_source where name='test_f' ORDER BY type,line;
select text,type from user_source where name='test_f' ORDER BY type,line;


--clean data
DROP package pkg;
DROP FUNCTION test_f;
