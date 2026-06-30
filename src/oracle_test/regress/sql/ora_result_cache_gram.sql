-- I.create table base check
drop table if exists t1;
drop table if exists t2;
drop table if exists t3;
create table t1(a int) result_cache;
create table t1(a int) result_cache();
create table t1(a int) result_cache(mode);
create table t1(a int) result_cache(mode force);
create table t2(a int) result_cache(mode manual);
create table t3(a int) result_cache(mode default);
drop table if exists t1;
drop table if exists t2;
drop table if exists t3;

-- II.create table support branch
-- 1.CREATE table if not exists
drop table if exists tabifnot;
create table if not exists tabifnot (a int,b int) result_cache(mode force);

-- 2.CREATE table if not exists of anyname
drop table if exists tabifnotanyname;
drop type if exists typifnottab;
create type typifnottab as (id int, name text);
create table tabifnotanyname of typifnottab result_cache(mode force);

-- 3.CREATE table partition by
drop table if exists tabpartrange;
drop table if exists tabpartlist;
drop table if exists tabparthash;
create table tabpartrange(id int,b int) partition by range(id) (partition p1 values less than (10)) result_cache(mode force);
create table tabpartlist(id int,name varchar(10)) partition by list(name) (partition p1 values ('shandong')) result_cache(mode force);
create table tabparthash(id int,name varchar(10)) partition by hash(id) (partition p1) result_cache(mode force);

drop type if exists typorder;
drop table if exists tabpartcomrange;
drop table if exists tabpartcomlist;
drop table if exists tabpartcomhash;
create type typorder as (orderid int, ordername varchar(10));
create table tabpartcomrange(id int,b int,orderinfo typorder) partition by range(((orderinfo).orderid)) (partition p1 values less than (10)) result_cache(mode force);
create table tabpartcomlist(id int,b int,orderinfo typorder) partition by list(((orderinfo).ordername)) (partition p1 values ('shandong')) result_cache(mode force);
create table tabpartcomhash(id int,b int,orderinfo typorder) partition by hash(((orderinfo).orderid)) (partition p1) result_cache(mode force);

-- 4.CREATE table OraPreTableSpace
-- drop table if exists tabora;
-- create table tabora(a int) tablespace pg_default result_cache(mode force);

-- 5.create if not exists as select
drop table if exists tabifnotsel;
drop table if exists tabifnotassel;
create table if not exists tabifnotsel(a int,b int);
create table if not exists tabifnotassel result_cache(mode force) as select * from tabifnotsel;

-- 6.create table if not exists as execute
drop table if exists tabifnotasexe;
drop table if exists tabifnotbase;
create table tabifnotbase(id int, name varchar(10));
prepare maxid2(int) as select * from tabifnotbase where id > $1;
create table if not exists tabifnotasexe result_cache(mode force) as execute maxid2(5);

-- III.alter table base check
drop table if exists t1;
create table t1(a int) result_cache(mode force);
alter table t1 result_cache;
alter table t1 result_cache();
alter table t1 result_cache(mode);
alter table t1 result_cache(mode manual);
alter table t1 result_cache(mode default);

-- IV.alter table support branch
-- 1.alter table if exists alter_tables_cmds_opt
drop table if exists tabifalt;
create table tabifalt(id int, name varchar(10));
alter table if exists tabifalt result_cache(mode default);
alter table if exists tabifalt add b int, result_cache(mode default);
alter table if exists tabifalt result_cache(mode default), add c int;

-- V.create function
-- base data_source
drop view if exists vi_data_source01;
drop table if exists tab_data_source01;
drop table if exists tab_data_source02;
create table tab_data_source01(a int);
create table tab_data_source02(a int);
create view vi_data_source01 as select * from tab_data_source01;

-- 1.create function return xxx
drop function if exists func_result_cache;
drop function if exists func_result_cache_relies;
drop function if exists func_result_cache_relies_data;
create or replace function func_result_cache
return integer
result_cache
as
begin
return 1;
end;
/
create or replace function func_result_cache_relies
return integer
result_cache relies_on ()
as
begin
return 1;
end;
/
create or replace function func_result_cache_relies_data
return integer
result_cache relies_on (tab_data_source01, vi_data_source01)
as
begin
return 1;
end;
/
-- 2.create ora_editioned function return xxx
drop function if exists func_edit_result_cache;
drop function if exists func_edit_result_cache_relies;
drop function if exists func_edit_result_cache_relies_data;
create or replace editionable function func_edit_result_cache
return integer
result_cache
as
begin
return 1;
end;
/
create or replace editionable function func_edit_result_cache_relies
return integer
result_cache relies_on ()
as
begin
return 1;
end;
/
create or replace editionable function func_edit_result_cache_relies_data
return integer
result_cache relies_on (tab_data_source01, vi_data_source01)
as
begin
return 1;
end;
/

-- clean all objects
-- CREATE TABLE
-- drop table if exists tabora;
drop table if exists tabpartcomrange;
drop table if exists tabpartcomlist;
drop table if exists tabpartcomhash;
drop type if exists typorder;
drop table if exists tabpartrange;
drop table if exists tabpartlist;
drop table if exists tabparthash;
drop table if exists tabifnotanyname;
drop type if exists typifnottab;
drop table if exists tabifnot;
drop table if exists tabifnotsel;
drop table if exists tabifnotassel;
drop table if exists tabifnotasexe;
drop table if exists tabifnotbase;
-- ALTER TABLE
drop table if exists tabifalt;
-- FUNCTION
drop function if exists func_result_cache;
drop function if exists func_result_cache_relies;
drop function if exists func_result_cache_relies_data;
drop function if exists func_edit_result_cache;
drop function if exists func_edit_result_cache_relies;
drop function if exists func_edit_result_cache_relies_data;
drop view if exists vi_data_source01;
drop table if exists tab_data_source01;
drop table if exists tab_data_source02;