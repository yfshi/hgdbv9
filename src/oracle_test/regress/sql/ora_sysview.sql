DROP SCHEMA IF EXISTS system;
DROP USER IF EXISTS system;

DROP SCHEMA IF EXISTS rnrol11;
DROP SCHEMA IF EXISTS rnrol22;
DROP USER IF EXISTS enrol11;
DROP USER IF EXISTS enrol22;

CREATE USER system WITH superuser;
CREATE USER enrol11 superuser;
CREATE USER enrol22;
set ivorysql.identifier_case_switch to interchange;
/****************************test xxx_tables************************************/
set ivorysql.enable_ora_default_collation = on;
set ivorysql.enable_recycle_bin = on;
purge recyclebin;

-- 1. ordinary table with fillfactor and default collation
create table sysview_all_tables_tb1(id int, name text) with (fillfactor = 75) default collation C;
insert into sysview_all_tables_tb1 values(generate_series(0, 9999), 'segfrgh');
analyze sysview_all_tables_tb1;
select table_name, tablespace_name, pct_free, logging, num_rows, blocks, avg_row_len, partitioned, temporary, default_collation from all_tables where upper(table_name) = 'SYSVIEW_ALL_TABLES_TB1';
drop table sysview_all_tables_tb1;

-- 2. temp table
start transaction;
create global temp table sysview_all_tables_tb2(id int, name text) on commit delete rows;
insert into sysview_all_tables_tb2 values(generate_series(0, 9999), 'segfrgh');
analyze sysview_all_tables_tb2;
select table_name, tablespace_name, pct_free, logging, num_rows, blocks, avg_row_len, partitioned, temporary, default_collation, duration from all_tables where upper(table_name) = 'SYSVIEW_ALL_TABLES_TB2';
drop table sysview_all_tables_tb2;

create global temp table sysview_all_tables_tb3(id int, name text) on commit preserve rows;
insert into sysview_all_tables_tb3 values(generate_series(0, 9999), 'segfrgh');
analyze sysview_all_tables_tb3;
select table_name, tablespace_name, pct_free, logging, num_rows, blocks, avg_row_len, partitioned, temporary, default_collation, duration from all_tables where upper(table_name) = 'SYSVIEW_ALL_TABLES_TB3';
commit;

-- 3. unlogged table
create unlogged table sysview_all_tables_tb4(id int, name text);
insert into sysview_all_tables_tb4 values(generate_series(0, 9999), 'segfrgh');
analyze sysview_all_tables_tb4;
select table_name, tablespace_name, pct_free, logging, num_rows, blocks, avg_row_len, partitioned, temporary, default_collation, duration from all_tables where upper(table_name) = 'SYSVIEW_ALL_TABLES_TB4';
drop table sysview_all_tables_tb4;

-- 4. partitioned table
create table sysview_all_tables_part1(id int, age int, name text) partition by hash(id);
create table sysview_all_tables_part1_1 partition of sysview_all_tables_part1 for values with (modulus 3, remainder 0) partition by list(age);
create table sysview_all_tables_part1_1_1 partition of sysview_all_tables_part1_1 for values in (0, 1);
create table sysview_all_tables_part1_1_2 partition of sysview_all_tables_part1_1 for values in (2, 3);

create table sysview_all_tables_part1_2 partition of sysview_all_tables_part1 for values with (modulus 3, remainder 1) partition by list(age);
create table sysview_all_tables_part1_2_1 partition of sysview_all_tables_part1_2 for values in (0, 1);
create table sysview_all_tables_part1_2_2 partition of sysview_all_tables_part1_2 for values in (2, 3);

create table sysview_all_tables_part1_3 partition of sysview_all_tables_part1 for values with (modulus 3, remainder 2) partition by list(age);
create table sysview_all_tables_part1_3_1 partition of sysview_all_tables_part1_3 for values in (0, 1);
create table sysview_all_tables_part1_3_2 partition of sysview_all_tables_part1_3 for values in (2, 3);

insert into sysview_all_tables_part1 values(generate_series(0, 999), 0, 'sgj78wergdihjuym,gnfddgyuj67jtbjnyyukyukjtyjytrug');
insert into sysview_all_tables_part1 values(generate_series(0, 999), 1, 'sgj78wergdihjuym,gnfddgyuj67jtbjug');
insert into sysview_all_tables_part1 values(generate_series(0, 999), 2, 'sgj78wergdihjuym,gnfddgyuj67jtbjnyyukfgjhfyjfgymnfgnyukjtyjytrug');
insert into sysview_all_tables_part1 values(generate_series(0, 999), 3, 'sjtbjnyyukyukjtyjytrug');
analyze;
select table_name, tablespace_name, pct_free, logging, num_rows, blocks, avg_row_len, partitioned, temporary, default_collation, duration from all_tables where upper(table_name) like('SYSVIEW_ALL_TABLES_PART%');

drop table sysview_all_tables_part1;

-- 5. test logical_replication
create table sysview_all_tables_tb5(id int, name text);
create publication pub1 for table sysview_all_tables_tb5;
select table_name, logical_replication from all_tables where upper(table_name) = 'SYSVIEW_ALL_TABLES_TB5';
drop publication pub1;
select table_name, logical_replication from all_tables where upper(table_name) = 'SYSVIEW_ALL_TABLES_TB5';
drop table sysview_all_tables_tb5;

-- 6. test has_identity
create table sysview_all_tables_tb6(id int generated always as identity primary key);
select table_name, has_identity from all_tables where upper(table_name) = 'SYSVIEW_ALL_TABLES_TB6';
drop table sysview_all_tables_tb6;

select table_name from all_tables where upper(table_name) like('SYSVIEW_ALL_TABLES%') or upper(table_name) like('__BIN_%');

set ivorysql.enable_ora_default_collation = off;
set ivorysql.enable_recycle_bin = off;

/****************************test xxx_tab_cols************************************/
set ivorysql.enable_recycle_bin = on;
purge recyclebin;

create table sysview_tab_cols_tb1(
    col1 int8 generated always as identity primary key,
    col2 bit,
    col3 bit(10),
    col4 boolean,
    col5 bytea,
    col6 char(11) not null,
    col7 varchar(12) default 'qwertyuio',
    col8 date,
    col9 float8,
    col10 inet,
    col11 int,
    col12 json,
    col13 jsonb,
    col14 macaddr,
    col15 money,
    col16 numeric(13, 9),
    col17 float4,
    col18 int2,
    col19 text,
    col20 timestamp,
    col21 timestamptz,
    col22 uuid,
    col23 int generated always as (col11 * 2),
    col24 int generated always as (col11 * 3) virtual,
    col25 int invisible
);

insert into sysview_tab_cols_tb1(col6, col11) values('srgrg', generate_series(0, 99));
analyze sysview_tab_cols_tb1;
select table_name, column_name, data_type, data_length, data_precision, data_scale, nullable, column_id, default_length, 
    data_default, num_nulls, character_set_name, char_col_decl_length, avg_col_len, char_length, char_used, hidden_column, virtual_column,
    internal_column_id, qualified_col_name, default_on_null, identity_column
from all_tab_cols where upper(table_name) = 'SYSVIEW_TAB_COLS_TB1' order by column_id;

create view sysview_tab_cols_v1 as select * from sysview_tab_cols_tb1;
select table_name, column_name, data_type, data_length, data_precision, data_scale, nullable, column_id, default_length, 
    data_default, num_nulls, character_set_name, char_col_decl_length, avg_col_len, char_length, char_used, hidden_column, virtual_column,
    internal_column_id, qualified_col_name, default_on_null, identity_column
from all_tab_cols where upper(table_name) = 'SYSVIEW_TAB_COLS_V1' order by column_id;

drop view sysview_tab_cols_v1;
drop table sysview_tab_cols_tb1;

select table_name, column_name, data_type from all_tab_cols where upper(table_name) like('__BIN_%');

purge recyclebin;
set ivorysql.enable_recycle_bin = off;

-- test whether string storaged by byte length or char length
-- byte length
create table sysview_tab_cols_tb(name varchar(3));
insert into sysview_tab_cols_tb values('你好呀');
drop table sysview_tab_cols_tb;

create table sysview_tab_cols_tb(name char(3));
insert into sysview_tab_cols_tb values('你好呀');
drop table sysview_tab_cols_tb;

create table sysview_tab_cols_tb(name oracharbyte(3));
insert into sysview_tab_cols_tb values('你好呀');
drop table sysview_tab_cols_tb;

create table sysview_tab_cols_tb(name oravarcharbyte(3));
insert into sysview_tab_cols_tb values('你好呀');
drop table sysview_tab_cols_tb;

-- char length
create table sysview_tab_cols_tb(name bpchar(3));
insert into sysview_tab_cols_tb values('你好呀');
drop table sysview_tab_cols_tb;

create table sysview_tab_cols_tb(name oracharchar(3));
insert into sysview_tab_cols_tb values('你好呀');
drop table sysview_tab_cols_tb;

create table sysview_tab_cols_tb(name oravarcharchar(3));
insert into sysview_tab_cols_tb values('你好呀');
drop table sysview_tab_cols_tb;
/****************************test xxx_tab_columns************************************/
set ivorysql.enable_recycle_bin = on;
purge recyclebin;

create table sysview_tab_columns_tb1(
    col1 int8 generated always as identity primary key,
    col2 bit,
    col3 bit(10),
    col4 boolean,
    col5 bytea,
    col6 char(11) not null,
    col7 varchar(12) default 'qwertyuio',
    col8 date,
    col9 float8,
    col10 inet,
    col11 int,
    col12 json,
    col13 jsonb,
    col14 macaddr,
    col15 money,
    col16 numeric(13, 9),
    col17 float4,
    col18 int2,
    col19 text,
    col20 timestamp,
    col21 timestamptz,
    col22 uuid,
    col23 int generated always as (col11 * 2),
    col24 int generated always as (col11 * 3) virtual,
    col25 int invisible
);

insert into sysview_tab_columns_tb1(col6, col11) values('srgrg', generate_series(0, 99));
analyze sysview_tab_columns_tb1;
select table_name, column_name, data_type, data_length, data_precision, data_scale, nullable, column_id, default_length, 
    data_default, num_nulls, character_set_name, char_col_decl_length, avg_col_len, char_length, char_used, 
    default_on_null, identity_column
from all_tab_columns where upper(table_name) = 'SYSVIEW_TAB_COLUMNS_TB1'  order by column_id;

create view sysview_tab_columns_v1 as select * from sysview_tab_columns_tb1;
select table_name, column_name, data_type, data_length, data_precision, data_scale, nullable, column_id, default_length, 
    data_default, num_nulls, character_set_name, char_col_decl_length, avg_col_len, char_length, char_used, 
    default_on_null, identity_column
from all_tab_columns where upper(table_name) = 'SYSVIEW_TAB_COLUMNS_V1'  order by column_id;

drop view sysview_tab_columns_v1;
drop table sysview_tab_columns_tb1;

select table_name, column_name, data_type from all_tab_columns where upper(table_name) like('__BIN_%');

set ivorysql.enable_recycle_bin = off;

/****************************test xxx_tab_comments************************************/
set ivorysql.enable_recycle_bin = on;
purge recyclebin;

create table sysview_tab_comments_tb(id int, name text);
comment on table sysview_tab_comments_tb is 'This is test table comment';

create view sysview_tab_comments_v as select * from sysview_tab_comments_tb;
comment on view sysview_tab_comments_v is 'This is test view comment';

select table_name, table_type, comments, origin_con_id from all_tab_comments where upper(table_name) like ('SYSVIEW_TAB_COMMENTS_%') order by table_name;

drop view sysview_tab_comments_v;
drop table sysview_tab_comments_tb;

select table_type, comments, origin_con_id from all_tab_comments where upper(table_name) like ('__BIN_%') order by table_name;

set ivorysql.enable_recycle_bin = off;

/****************************test xxx_indexes************************************/
set ivorysql.enable_recycle_bin = on;
purge recyclebin;

create table sysview_indexes_tb(
        col1 int primary key,
        col2 int,
        col3 jsonb,
        col4 float4,
        col5 text,
        col6 timestamp);

-- 1. create index with (deduplicate_items = yes, fillfactor = n)
create index sysview_indexes_tb_idx1 on sysview_indexes_tb(col1) with (deduplicate_items = yes, fillfactor = 65);

-- 2. create unique index
create unique index sysview_indexes_tb_idx2 on sysview_indexes_tb(col2);

-- 3. unlogged table
create unlogged table sysview_indexes_unlogged_tb(col1 int);
create index sysview_indexes_unlogged_tb_idx on sysview_indexes_unlogged_tb(col1);

-- 4. partition index
create table sysview_indexes_part(id int, name text) partition by hash(id);
create table sysview_indexes_part_1 partition of sysview_indexes_part for values with (modulus 3, remainder 0);
create table sysview_indexes_part_2 partition of sysview_indexes_part for values with (modulus 3, remainder 1);
create table sysview_indexes_part_3 partition of sysview_indexes_part for values with (modulus 3, remainder 2);
create index sysview_indexes_part_idx on sysview_indexes_part(id);

-- 5. temporary table
start transaction;
create global temp table sysview_indexes_temp_tb1(id int, name text) on commit delete rows;
create index sysview_indexes_temp_tb1_idx on sysview_indexes_temp_tb1(id);

create global temp table sysview_indexes_temp_tb2(id int, name text) on commit preserve rows;
create index sysview_indexes_temp_tb2_idx on sysview_indexes_temp_tb2(id);

select index_name, index_type, table_name, table_type, uniqueness, compression, pct_free, logging, status,
        num_rows, partitioned, temporary, duration, funcidx_status, constraint_index 
from all_indexes where upper(table_name) like('SYSVIEW_INDEXES_TEMP%') order by table_name, index_name;
commit;

-- 6. function index
create index sysview_indexes_tb_func_idx on sysview_indexes_tb(upper(col5));

-- 7. multi column index
create index sysview_indexes_tb_multi_idx on sysview_indexes_tb(col1, col2, col3);

-- 8. hash index
create index sysview_indexes_tb_hash_idx on sysview_indexes_tb using hash(col1);

-- 9. gin index
create index sysview_indexes_tb_gin_idx on sysview_indexes_tb using gin(col3);

-- 10. brin index
create index sysview_indexes_tb_brin_idx on sysview_indexes_tb using brin(col6);

select index_name, index_type, table_name, table_type, uniqueness, compression, pct_free, logging, status,
        num_rows, partitioned, temporary, duration, funcidx_status, constraint_index 
from all_indexes where upper(table_name) like('SYSVIEW_INDEXES_%') order by table_name, index_name;

drop table sysview_indexes_part cascade;
drop table sysview_indexes_tb cascade;
drop table sysview_indexes_unlogged_tb cascade;
drop table sysview_indexes_temp_tb1;
drop table sysview_indexes_temp_tb2;

select index_name, index_type, table_name from all_indexes where upper(table_name) like ('__BIN_%');

purge recyclebin;
set ivorysql.enable_recycle_bin = off;

/****************************test xxx_ind_columns************************************/
set ivorysql.enable_recycle_bin to on;
purge recyclebin;

create table sysview_ind_columns_tb(
        col1 int primary key,
        col2 int,
        col3 jsonb,
        col4 float4,
        col5 text,
        col6 timestamp);

-- 1. create index with (deduplicate_items = yes, fillfactor = n)
create index sysview_ind_columns_tb_idx1 on sysview_ind_columns_tb(col1) with (deduplicate_items = yes, fillfactor = 65);

-- 2. create unique index
create unique index sysview_ind_columns_tb_idx2 on sysview_ind_columns_tb(col2);

-- 3. unlogged table
create unlogged table sysview_ind_columns_unlogged_tb(col1 int);
create index sysview_ind_columns_unlogged_tb_idx on sysview_ind_columns_unlogged_tb(col1);

-- 4. partition index
create table sysview_ind_columns_part(id int, name text) partition by hash(id);
create table sysview_ind_columns_part_1 partition of sysview_ind_columns_part for values with (modulus 3, remainder 0);
create table sysview_ind_columns_part_2 partition of sysview_ind_columns_part for values with (modulus 3, remainder 1);
create table sysview_ind_columns_part_3 partition of sysview_ind_columns_part for values with (modulus 3, remainder 2);
create index sysview_ind_columns_part_idx on sysview_ind_columns_part(id);

-- 5. temporary table
start transaction;
create global temp table sysview_ind_columns_temp_tb1(id int, name text) on commit delete rows;
create index sysview_ind_columns_temp_tb1_idx on sysview_ind_columns_temp_tb1(id);

create global temp table sysview_ind_columns_temp_tb2(id int, name text) on commit preserve rows;
create index sysview_ind_columns_temp_tb2_idx on sysview_ind_columns_temp_tb2(id);

select index_name, table_name, column_name, column_position, column_length, char_length, descend, collated_column_id
from all_ind_columns where upper(table_name) like('SYSVIEW_IND_COLUMNS_%') order by table_name, index_name;
commit;

-- 6. function index
create index sysview_ind_columns_tb_func_idx on sysview_ind_columns_tb(upper(col5));

-- 7. multi column index
create index sysview_ind_columns_tb_multi_idx on sysview_ind_columns_tb(col1, col2, col3);

-- 8. hash index
create index sysview_ind_columns_tb_hash_idx on sysview_ind_columns_tb using hash(col1);

-- 9. gin index
create index sysview_ind_columns_tb_gin_idx on sysview_ind_columns_tb using gin(col3);

-- 10. brin index
create index sysview_ind_columns_tb_brin_idx on sysview_ind_columns_tb using brin(col6);

select index_name, table_name, column_name, column_position, column_length, char_length, descend, collated_column_id
from all_ind_columns where upper(table_name) like('SYSVIEW_IND_COLUMNS_%') order by table_name, index_name;


drop table sysview_ind_columns_part;
drop table sysview_ind_columns_tb;
drop table sysview_ind_columns_unlogged_tb;
drop table sysview_ind_columns_temp_tb1;
drop table sysview_ind_columns_temp_tb2;

select column_name, column_position, column_length, char_length, descend, collated_column_id
from all_ind_columns where upper(table_name) like('__BIN_%') or index_name like('__BIN_%') order by table_name, index_name;

set ivorysql.enable_recycle_bin to off;

/****************************test xxx_dependencies************************************/
-- 1. table and view
set ivorysql.enable_recycle_bin to on;
purge recyclebin;

create table sysview_dependencies_tb(id int primary key, name VARCHAR2(128));
insert into sysview_dependencies_tb values(generate_series(0, 99), 'asrgrghdrhg');
create view sysview_dependencies_v1 as select * from sysview_dependencies_tb;
create materialized view sysview_dependencies_v2 as select * from sysview_dependencies_tb;
create index sysview_dependencies_tb_idx on sysview_dependencies_tb(name);

select name, type, referenced_name, referenced_type, referenced_link_name, dependency_type from all_dependencies 
where upper(referenced_name) = 'SYSVIEW_DEPENDENCIES_TB' or upper(name) like('SYSVIEW_DEPENDENCIES%') order by name, referenced_name;

drop materialized view sysview_dependencies_v2;
drop view sysview_dependencies_v1;
drop table sysview_dependencies_tb;

select type, referenced_type, referenced_link_name, dependency_type from all_dependencies 
where upper(referenced_name) like('__BIN_%') or upper(name) like('__BIN_%') order by name, referenced_name;
purge recyclebin;
set ivorysql.enable_recycle_bin to off;

-- 2. proc
create table sysview_dependencies_tb1(id int, name VARCHAR2(128));

create or replace function get_name_by_id(p_id int)
returns varchar2(128)
AS $$ 
    declare v_name varchar2(128);
    begin
        select name into v_name from sysview_dependencies_tb1 where id = p_id;
        if not found then 
            return null;
        end if;
        return v_name;
    end;
$$ LANGUAGE plpgsql;
/

create or replace procedure upsert_tb_record(
    p_id int,
    p_name varchar2(128),
    out p_status varchar2(50),
    out p_message varchar2(50)
)
AS $$
    begin 
        if exists (select 1 from sysview_dependencies_tb1 where id = p_id) 
            then update sysview_dependencies_tb1 set name = p_name;
            p_status := 'updated';
            p_message := 'update success';
        else
            insert into tb values(p_id, p_name);
            p_status := 'insert';
            p_message := 'insert success';
        end if;
        commit;
    end;
$$ language plpgsql;
/

select name, type, referenced_name, referenced_type, referenced_link_name, dependency_type from all_dependencies 
where upper(name) = 'GET_NAME_BY_ID' or upper(referenced_name) = 'GET_NAME_BY_ID' order by name;
select name, type, referenced_name, referenced_type, referenced_link_name, dependency_type from all_dependencies 
where upper(name) = 'UPSERT_TB_RECORD' or upper(referenced_name) = 'UPSERT_TB_RECORD' order by name;
drop function get_name_by_id(p_id int);
drop procedure upsert_tb_record;
drop table sysview_dependencies_tb1;

-- 3. trigger and event trigger
create table sysview_dependencies_tb2(id int, name VARCHAR2(128));
create table sysview_dependencies_tb3(id int, name VARCHAR2(128));

create or replace function tri_insert_func() returns trigger as
$$
    declare
    begin
        insert into sysview_dependencies_tb2 values(new.id, new.name);
        return new;
    end;
$$ language plpgsql;
/

create trigger insert_trigger
    before insert on sysview_dependencies_tb3
    for each row
    execute procedure tri_insert_func()
;

create or replace function sysview_dependencies_cmd()
returns event_trigger
language plpgsql
as $$
    begin
        raise exception 'command % is disabled', tg_tag;
    end;
$$;
/

create event trigger event_trigger on ddl_command_start
execute function sysview_dependencies_cmd();
select name, type, referenced_name, referenced_type, referenced_link_name, dependency_type from all_dependencies 
where upper(name) = 'INSERT_TRIGGER' or upper(referenced_name) = 'INSERT_TRIGGER' order by name;
select name, type, referenced_name, referenced_type, referenced_link_name, dependency_type from all_dependencies 
where upper(name) = 'EVENT_TRIGGER' or upper(referenced_name) = 'EVENT_TRIGGER' order by name;

drop event trigger event_trigger;
drop function sysview_dependencies_cmd();

drop trigger insert_trigger on sysview_dependencies_tb3;
drop function tri_insert_func();
drop table sysview_dependencies_tb2;
drop table sysview_dependencies_tb3;

-- 4. package and package body
create or replace editionable package sysview_dependencies_pkg authid definer accessible by (function kds, package mds)
is
  var1 integer;
  function test_mds(id integer) return integer;
end sysview_dependencies_pkg;
/

create or replace editionable package body sysview_dependencies_pkg
is
    var11 integer;
    function test_mds(id integer) return integer is
        var1 integer;
        begin
            var1 := id;
        return var1;
        end;
end;
/

select name, type, referenced_name, referenced_type, referenced_link_name, dependency_type from all_dependencies 
where upper(name) = 'SYSVIEW_DEPENDENCIES_PKG' or upper(referenced_name) = 'SYSVIEW_DEPENDENCIES_PKG' order by name;
drop package sysview_dependencies_pkg;

/****************************test xxx_objects************************************/
-- 1. table, index, view, sequence
set ivory.enable_ora_default_collation = on;
set ivorysql.enable_recycle_bin to on;
purge recyclebin;

create table sysview_objects_tb(col1 int, col2 text) default collation BINARY;
create temp table sysview_objects_temp_tb(col1 int, col2 text);
create index sysview_objects_tb_idx on sysview_objects_tb(col1);
create view sysview_objects_tb_v as select * from sysview_objects_tb;
create sequence sysview_objects_seq;
create table sysview_objects_part(id int, age int) partition by hash(id);
create table sysview_objects_part1 partition of sysview_objects_part for values with(modulus 3, remainder 0) partition by hash(age);
create table sysview_objects_part2 partition of sysview_objects_part for values with(modulus 3, remainder 1) partition by hash(age);
create table sysview_objects_part3 partition of sysview_objects_part for values with(modulus 3, remainder 2) partition by hash(age);
create table sysview_objects_part1_1 partition of sysview_objects_part1 for values with(modulus 2, remainder 0);
create table sysview_objects_part1_2 partition of sysview_objects_part1 for values with(modulus 2, remainder 1);
create table sysview_objects_part2_1 partition of sysview_objects_part2 for values with(modulus 2, remainder 0);
create table sysview_objects_part2_2 partition of sysview_objects_part2 for values with(modulus 2, remainder 1);
create table sysview_objects_part3_1 partition of sysview_objects_part3 for values with(modulus 2, remainder 0);
create table sysview_objects_part3_2 partition of sysview_objects_part3 for values with(modulus 2, remainder 1);
create index sysview_objects_part_idx ON sysview_objects_part(id);
select object_name, subobject_name, object_type, temporary, editionable from all_objects
    where upper(object_name) like('SYSVIEW_OBJECTS_%') and
    object_type in ('INDEX', 'TABLE', 'VIEW', 'SEQUENCE', 'TABLE PARTITION', 'TABLE SUBPARTITION', 'INDEX PARTITION', 'INDEX SUBPARTITION')
    order by object_name, subobject_name;
drop view sysview_objects_tb_v;
drop table sysview_objects_tb;
drop sequence sysview_objects_seq;
drop table sysview_objects_temp_tb;
drop table sysview_objects_part;

select object_type, temporary, editionable from all_objects 
    where upper(object_name) like('__BIN_%') and object_type in ('INDEX', 'TABLE', 'VIEW', 'SEQUENCE') order by object_name;

set ivory.enable_ora_default_collation = off;
purge recyclebin;
set ivorysql.enable_recycle_bin to off;

-- 2. proc
create table sysview_objects_tb1(id int, name VARCHAR2(128));

create or replace function get_name_by_id(p_id int)
returns varchar2(128)
AS $$ 
    declare v_name varchar2(128);
    begin
        select name into v_name from sysview_objects_tb1 where id = p_id;
        if not found then 
            return null;
        end if;
        return v_name;
    end;
$$ LANGUAGE plpgsql;
/

create or replace procedure upsert_tb_record(
    p_id int,
    p_name varchar2(128),
    out p_status varchar2(50),
    out p_message varchar2(50)
)
AS $$
    begin 
        if exists (select 1 from sysview_objects_tb1 where id = p_id) 
            then update sysview_objects_tb1 set name = p_name;
            p_status := 'updated';
            p_message := 'update success';
        else
            insert into tb values(p_id, p_name);
            p_status := 'insert';
            p_message := 'insert success';
        end if;
        commit;
    end;
$$ language plpgsql;
/

select object_name, object_type, temporary, editionable from all_objects 
    where upper(object_name) in ('GET_NAME_BY_ID', 'UPSERT_TB_RECORD') ORDER BY object_name;

drop function get_name_by_id(p_id int);
drop procedure upsert_tb_record;
drop table sysview_objects_tb1;

-- 3. package and package body
create or replace editionable package sysview_objects_pkg authid definer accessible by (function kds, package mds)
is
  var1 integer;
  function test_mds(id integer) return integer;
end sysview_objects_pkg;

create or replace editionable package body sysview_objects_pkg
is
    var11 integer;
    function test_mds(id integer) return integer is
     var1 integer;
     begin
         var1 := id;
	 return var1;
     end;
 end;
 /

select object_name, object_type, temporary, editionable from all_objects 
    where upper(object_name) like('SYSVIEW_OBJECTS_PKG%');

drop package sysview_objects_pkg;

-- 4. type
create type sysview_objects_type AS (
    first_type int,
    second_type varchar2(128)
);
select object_name, object_type, temporary, editionable from all_objects 
    where upper(object_name) = 'SYSVIEW_OBJECTS_TYPE';
drop type sysview_objects_type;

-- 5. trigger
create table sysview_objects_tb2(id int, name VARCHAR2(128));
create table sysview_objects_tb3(id int, name VARCHAR2(128));

create or replace function tri_insert_func() returns trigger as
$$
    declare
    begin
        insert into sysview_objects_tb2 values(new.id, new.name);
        return new;
    end;
$$ language plpgsql;
/

create trigger insert_trigger
    before insert on sysview_objects_tb3
    for each row
    execute procedure tri_insert_func()
;
/

select object_name, object_type, temporary, editionable from all_objects 
    where upper(object_name) = 'INSERT_TRIGGER';

drop trigger insert_trigger on sysview_objects_tb3;
drop function tri_insert_func();
drop table sysview_objects_tb2;
drop table sysview_objects_tb3;

-- 6. constraint
create table sysview_objects_tb4(col1 int primary key);
select object_name, object_type, temporary, editionable from all_objects 
    where upper(object_name) = 'SYSVIEW_OBJECTS_TB4_PKEY' and object_type = 'CONSTRAINT';
drop table sysview_objects_tb4;

-- 7. database
create database sysview_objects_db;
select object_name, object_type, temporary, editionable from all_objects 
    where upper(object_name) = 'SYSVIEW_OBJECTS_DB';
drop database sysview_objects_db;

-- 8. schema
create schema sysview_objects_schema;
select object_name, object_type, temporary, editionable from all_objects 
    where upper(object_name) = 'SYSVIEW_OBJECTS_SCHEMA';
drop schema sysview_objects_schema;

-- 9. synonym
create table sysview_objects_tb5(col1 int);
create synonym sysview_objects_syn for sysview_objects_tb5;
select object_name, object_type, temporary, editionable from all_objects 
    where upper(object_name) = 'SYSVIEW_OBJECTS_SYN';
drop synonym sysview_objects_syn;
drop table sysview_objects_tb5;

/****************************test xxx_types************************************/
create type sysview_types_typ1 AS(
    street varchar2(128),
    city varchar2(128),
    state varchar2(128),
    num int
);

create type sysview_types_typ2 AS ENUM('active', 'inactive', 'pending');

create domain sysview_types_typ3 as varchar2(256)
    check (value ~ '^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$');

select type_name, typecode, attributes, methods, predefined, incomplete,final, instantable, persistable from all_types
where upper(type_name) like('SYSVIEW_TYPES%') order by type_name;

drop type sysview_types_typ1;
drop type sysview_types_typ2;
drop domain sysview_types_typ3;

/****************************test xxx_users************************************/
create user sysview_users_user1 password 'qwerty123456';
create user sysview_users_user2;
select username, account_status, lock_date, authentication_type from dba_users where upper(username) like ('SYSVIEW_USERS%');
drop schema if exists sysview_users_user2;
drop schema if exists sysview_users_user1;
drop user sysview_users_user2;
drop user sysview_users_user1;

/****************************test xxx_constraints************************************/
set ivorysql.enable_recycle_bin to on;
purge recyclebin;

create table sysview_constraints_ref(
    col int unique
);
create table sysview_constraints_tb(
    col1 int primary key,
    col2 int unique,
    col3 varchar2(128) not null,
    col4 int check(col4 >= 0 and col4 <= 120),
    col5 int references sysview_constraints_ref(col)
);

select constraint_name, constraint_type, table_name, search_condition, search_condition_vc, r_constraint_name, delete_rule,
        condeferrable, deferred, validated, generated, index_name, invalid
from all_constraints where upper(table_name) = 'SYSVIEW_CONSTRAINTS_TB' order by table_name, constraint_name;

drop table sysview_constraints_tb cascade;
drop table sysview_constraints_ref cascade;

create table sysview_constraints_ref1(id int);
alter table sysview_constraints_ref1 add constraint sysview_constraints_ref1_cons unique(id);
create table sysview_constraints_tb1(id int constraint sysview_constraints_tb1_cons references sysview_constraints_ref1(id));
select constraint_name,table_name,r_constraint_name from all_constraints where upper(constraint_name) IN ('SYSVIEW_CONSTRAINTS_TB1_CONS', 'SYSVIEW_CONSTRAINTS_REF1_CONS') order by constraint_name;

drop table sysview_constraints_tb1 cascade;
drop table sysview_constraints_ref1 cascade;

select count(*) from all_constraints where upper(table_name) like('__BIN_%');
purge recyclebin;
set ivorysql.enable_recycle_bin to off;

/****************************test xxx_cons_columns************************************/
set ivorysql.enable_recycle_bin to on;
purge recyclebin;

create table sysview_cons_columns_ref(
    col int unique
);
create table sysview_cons_columns_tb(
    col1 int primary key,
    col2 int unique,
    col3 varchar2(128) not null,
    col4 int check(col4 >= 0 and col4 <= 120),
    col5 int references sysview_cons_columns_ref(col)
);

select constraint_name, table_name, column_name, position from all_cons_columns
where upper(table_name) like('SYSVIEW_CONS_COLUMNS_%') order by constraint_name;

drop table sysview_cons_columns_tb cascade;
drop table sysview_cons_columns_ref cascade;

select count(*) from all_cons_columns where upper(table_name) like('__BIN_%');
purge recyclebin;
set ivorysql.enable_recycle_bin to off;

/****************************test xxx_tab_privs************************************/
create user sysview_tab_privs_user;
create table sysview_tab_privs_tb(col1 int, col2 text);
create view sysview_tab_privs_v as select * from sysview_tab_privs_tb;

grant all on sysview_tab_privs_v to public;
grant select on sysview_tab_privs_tb to public;
grant select on sysview_tab_privs_tb to sysview_tab_privs_user;
grant insert on sysview_tab_privs_tb to sysview_tab_privs_user;
grant update on sysview_tab_privs_tb to sysview_tab_privs_user;
grant select on sysview_tab_privs_v to sysview_tab_privs_user;

select table_name, privilege, grantable, hierarchy, common, type, inherited from dba_tab_privs
    where upper(table_name) like('SYSVIEW_TAB_PRIVS_%') order by table_name, privilege, grantable;
select table_schema, table_name, privilege, grantable, hierarchy, type from all_tab_privs
where upper(table_name) like('SYSVIEW_TAB_PRIVS_%') order by table_name, privilege, grantable;

drop view sysview_tab_privs_v;
drop table sysview_tab_privs_tb;
drop schema if exists sysview_tab_privs_user;
drop user sysview_tab_privs_user;

/****************************test xxx_tab_partitions************************************/
set ivorysql.enable_recycle_bin to on;
purge recyclebin;

-- 1. partitioned table
create unlogged table sysview_tab_partitions_part1(id int, age int, name text) partition by range(age);
create table sysview_tab_partitions_part1_1 partition of sysview_tab_partitions_part1 for values from (0) to (20);
create table sysview_tab_partitions_part1_2 partition of sysview_tab_partitions_part1 for values from (21) to (40) with (fillfactor = 65);
create table sysview_tab_partitions_part1_3 partition of sysview_tab_partitions_part1 for values from (41) to (60);
create table sysview_tab_partitions_part1_4 partition of sysview_tab_partitions_part1 for values from (61) to (100);
create table sysview_tab_partitions_part1_5 partition of sysview_tab_partitions_part1 default;

insert into sysview_tab_partitions_part1 values(0, generate_series(0, 120), 'sgr');
insert into sysview_tab_partitions_part1 values(0, generate_series(0, 120), 'sgfdhjertjknbdfgrhfgndfndgnmhjrerrty6r');
insert into sysview_tab_partitions_part1 values(0, generate_series(0, 120), 'sgyuk6ertyjuytr');

-- 2. subpartitioned table
create table sysview_tab_partitions_part2(id int, age int, name text) partition by hash(id);
create table sysview_tab_partitions_part2_1 partition of sysview_tab_partitions_part2 for values with (modulus 3, remainder 0) partition by list(age);
create table sysview_tab_partitions_part2_1_1 partition of sysview_tab_partitions_part2_1 for values in (0, 1);
create table sysview_tab_partitions_part2_1_2 partition of sysview_tab_partitions_part2_1 for values in (2, 3);

create table sysview_tab_partitions_part2_2 partition of sysview_tab_partitions_part2 for values with (modulus 3, remainder 1) partition by list(age);
create table sysview_tab_partitions_part2_2_1 partition of sysview_tab_partitions_part2_2 for values in (0, 1);
create table sysview_tab_partitions_part2_2_2 partition of sysview_tab_partitions_part2_2 for values in (2, 3);

create table sysview_tab_partitions_part2_3 partition of sysview_tab_partitions_part2 for values with (modulus 3, remainder 2) partition by list(age);
create table sysview_tab_partitions_part2_3_1 partition of sysview_tab_partitions_part2_3 for values in (0, 1);
create table sysview_tab_partitions_part2_3_2 partition of sysview_tab_partitions_part2_3 for values in (2, 3);

insert into sysview_tab_partitions_part2 values(generate_series(0, 999), 0, 'sgj78wergdihjuym,gnfddgyuj67jtbjnyyukyukjtyjytrug');
insert into sysview_tab_partitions_part2 values(generate_series(0, 999), 1, 'sgj78wergdihjuym,gnfddgyuj67jtbjug');
insert into sysview_tab_partitions_part2 values(generate_series(0, 999), 2, 'sgj78wergdihjuym,gnfddgyuj67jtbjnyyukfgjhfyjfgymnfgnyukjtyjytrug');
insert into sysview_tab_partitions_part2 values(generate_series(0, 999), 3, 'sjtbjnyyukyukjtyjytrug');

analyze;
select table_name, composite, partition_name, subpartition_count, high_value, high_value_length, partition_position, pct_free, logging
from all_tab_partitions where upper(table_name) like('SYSVIEW_TAB_PARTITIONS_%') order by table_name, partition_name;

drop table sysview_tab_partitions_part1;
drop table sysview_tab_partitions_part2;

-- recyclebin does not support partitioned table yet
select table_name, composite, partition_name, subpartition_count, high_value, high_value_length, partition_position, pct_free, logging,
        num_rows, blocks, avg_row_len
from all_tab_partitions where upper(table_name) like('__BIN_%') order by table_name, partition_name;

purge recyclebin;
set ivorysql.enable_recycle_bin to off;

-- partitioned table related (xxx_objects)
\c - enrol11

create table sysview_objects_part(id int, age int, name text) partition by hash(id);
create table sysview_objects_part1 partition of sysview_objects_part for values with(modulus 2, remainder 0) partition by hash(age);
create table sysview_objects_part1_1 partition of sysview_objects_part1 for values with(modulus 2, remainder 0);
create table sysview_objects_part1_2 partition of sysview_objects_part1 for values with(modulus 2, remainder 1);
create table sysview_objects_part2 partition of sysview_objects_part for values with(modulus 2, remainder 1) partition by hash(age);
create table sysview_objects_part2_1 partition of sysview_objects_part2 for values with(modulus 2, remainder 0);
create table sysview_objects_part2_2 partition of sysview_objects_part2 for values with(modulus 2, remainder 1);

grant select on sysview_objects_part1_2 to enrol22;
grant select on sysview_objects_part2 to enrol22;

\c - enrol22
set ivorysql.identifier_case_switch to interchange;
select object_name, subobject_name, object_type from all_objects where UPPER(object_name) = 'SYSVIEW_OBJECTS_PART' order by object_name, subobject_name;
select object_name, subobject_name, object_type from dba_objects where UPPER(object_name) = 'SYSVIEW_OBJECTS_PART' order by object_name, subobject_name;

\c - enrol11
set ivorysql.identifier_case_switch to interchange;
select object_name, subobject_name, object_type from all_objects where UPPER(object_name) = 'SYSVIEW_OBJECTS_PART' order by object_name, subobject_name;
select object_name, subobject_name, object_type from dba_objects where UPPER(object_name) = 'SYSVIEW_OBJECTS_PART' order by object_name, subobject_name;

revoke all on sysview_objects_part1_2 from enrol22;
revoke all on sysview_objects_part2 from enrol22;

drop table sysview_objects_part cascade;

\c - system
-- special test for user_tab_privs
create role r1;

\c - enrol11
create table tab_privs_tb(id int);
grant select on table tab_privs_tb to enrol22;

\c - enrol22
set ivorysql.identifier_case_switch to interchange;
select * from user_tab_privs;

-- special test for all_objects for CONSTRAINT 
create table objects_tb(id int);
alter table objects_tb add constraint objects_tb_con unique(id);
select object_name, object_name from all_objects where UPPER(object_name) = UPPER('objects_tb_con');
select objname from pg_object where UPPER(objname) = UPPER('objects_tb_con');

alter table objects_tb rename constraint objects_tb_con to objects_tb_con_new;
select object_name, object_name from all_objects where UPPER(object_name) = UPPER('objects_tb_con_new');
select objname from pg_object where UPPER(objname) = UPPER('objects_tb_con_new');

drop table objects_tb cascade;

\c - enrol11
drop table tab_privs_tb;

-- Additional test fof xxx_tab_privs
\c - system
-- create table, view, materialized view, sequence
create table tab_privs_tb(id int);
insert into tab_privs_tb values (generate_series(0, 9));
create view tab_privs_v1 as select * from tab_privs_tb;
create materialized view tab_privs_mv1 as select * from tab_privs_tb;
create sequence tab_privs_seq1;

grant select on tab_privs_tb to enrol11;
grant insert on tab_privs_tb to enrol22;

grant select on tab_privs_v1 to enrol11;
grant select on tab_privs_mv1 to enrol22;

grant update on tab_privs_seq1 to enrol11;

-- create function, procedure
create or replace function tab_privs_func()
returns varchar2(128)
AS $$ 
    declare v_name varchar2(128);
    begin
        return v_name;
    end;
$$ LANGUAGE plpgsql;
/

create or replace procedure tab_privs_proc
AS $$
    begin 
    end;
$$ language plpgsql;
/

grant execute on function tab_privs_func to enrol11;
grant execute on procedure tab_privs_proc to enrol22;

-- create type
create type tab_privs_typ as (id int, name text);
grant usage on type tab_privs_typ to enrol11;

-- package
CREATE OR REPLACE PACKAGE tab_privs_pkg IS
    FUNCTION GET_TIME RETURN NUMBER;
END;
/

CREATE OR REPLACE PACKAGE BODY tab_privs_pkg IS
    FUNCTION GET_TIME RETURN NUMBER IS
    BEGIN
        RETURN floor(EXTRACT(EPOCH FROM (now() - pg_postmaster_start_time())*100));
    END;
END;
/

grant execute on package tab_privs_pkg to enrol11;

-- query
\c - enrol11
set ivorysql.identifier_case_switch to interchange; 
select table_name, owner, grantor, grantee, privilege, type from dba_tab_privs where table_name like ('TAB_PRIVS_%') order by table_name, owner, grantor, grantee, privilege, type;
select table_name, grantor, grantee, privilege, type from all_tab_privs where table_name like ('TAB_PRIVS_%') order by table_name, grantor, grantee, privilege, type;
select table_name, owner, grantor, grantee, privilege, type from user_tab_privs where table_name like ('TAB_PRIVS_%') order by table_name, owner, grantor, grantee, privilege, type;

\c - enrol22
set ivorysql.identifier_case_switch to interchange; 
select table_name, owner, grantor, grantee, privilege, type from dba_tab_privs where table_name like ('TAB_PRIVS_%') order by table_name, owner, grantor, grantee, privilege, type;
select table_name, grantor, grantee, privilege, type from all_tab_privs where table_name like ('TAB_PRIVS_%') order by table_name, grantor, grantee, privilege, type;
select table_name, owner, grantor, grantee, privilege, type from user_tab_privs where table_name like ('TAB_PRIVS_%') order by table_name, owner, grantor, grantee, privilege, type;

\c - enrol11
set ivorysql.identifier_case_switch to interchange; 
select table_name, owner, grantor, grantee, privilege, type from dba_tab_privs where table_name like ('TAB_PRIVS_%') order by table_name, owner, grantor, grantee, privilege, type;
select table_name, grantor, grantee, privilege, type from all_tab_privs where table_name like ('TAB_PRIVS_%') order by table_name, grantor, grantee, privilege, type;
select table_name, owner, grantor, grantee, privilege, type from user_tab_privs where table_name like ('TAB_PRIVS_%') order by table_name, owner, grantor, grantee, privilege, type;

drop function tab_privs_func;
drop procedure tab_privs_proc;
drop type tab_privs_typ;
drop materialized view tab_privs_mv1;
drop view tab_privs_v1;
drop sequence tab_privs_seq1;
drop table tab_privs_tb;
drop PACKAGE BODY tab_privs_pkg;
drop PACKAGE tab_privs_pkg;

/* Begin - OA-REQ-2126 */
\c - system
select * from session_privs;
select * from session_privs where upper(privilege) = 'CREATE TABLE';

select comp_id, comp_name, version, version_full, status, modified, namespace, schema, procedure, startup, parent_id, other_schemas from dba_registry;

\c - enrol22
select * from session_privs;
select * from session_privs where upper(privilege) = 'CREATE TABLE';

select comp_id, comp_name, version, version_full, status, modified, namespace, schema, procedure, startup, parent_id, other_schemas from user_registry;
/* End - OA-REQ-2126 */

set ivorysql.identifier_case_switch to normal;

\c - system
drop user enrol11;
drop user enrol22;
declare
    name text;
begin
        select into name 10::regrole;
		execute format('set session authorization %s;',name);
end;
/

DROP USER system;

/* Begin - ReqID:SRS-SQL-PARTITION-TABLE */
--
-- Oracle system views
--
CREATE ROLE part_test_user_1 LOGIN;
CREATE ROLE part_test_user_2 LOGIN;

-- Grant privileges
GRANT USAGE ON SCHEMA public TO part_test_user_1, part_test_user_2;
GRANT CREATE ON SCHEMA public TO part_test_user_1, part_test_user_2;

-- Create test tables
-- Range partitioned table
CREATE TABLE test_range_part (
    id INT,
    create_date DATE,
    name VARCHAR2(50)
)
PARTITION BY RANGE (create_date) (
    PARTITION p202401 VALUES LESS THAN (TO_DATE('2024-02-01', 'YYYY-MM-DD')),
    PARTITION p202402 VALUES LESS THAN (TO_DATE('2024-03-01', 'YYYY-MM-DD')),
    PARTITION p202403 VALUES LESS THAN (MAXVALUE)
);

-- List partitioned table
CREATE TABLE test_list_part (
    id INT,
    region VARCHAR2(20),
    info TEXT
)
PARTITION BY LIST (region) (
    PARTITION p_east VALUES ('EAST', '华东'),
    PARTITION p_west VALUES ('WEST', '华西'),
    PARTITION p_other VALUES (DEFAULT)
);

-- Hash partitioned table
CREATE TABLE test_hash_part (
    id INT,
    data VARCHAR2(100)
)
PARTITION BY HASH (id)
PARTITIONS 4;

-- Composite partition table with subpartition template
CREATE TABLE test_subpart (
    id INT,
    create_date DATE,
    region VARCHAR2(20),
    status CHAR(1)
)
PARTITION BY RANGE (create_date)
SUBPARTITION BY LIST (region)
SUBPARTITION TEMPLATE (
    SUBPARTITION sp_east VALUES ('EAST'),
    SUBPARTITION sp_west VALUES ('WEST'),
    SUBPARTITION sp_other VALUES (DEFAULT)
)
(
    PARTITION p202401 VALUES LESS THAN (TO_DATE('2024-02-01', 'YYYY-MM-DD')),
    PARTITION p202402 VALUES LESS THAN (MAXVALUE)
);

-- Insert some data
INSERT INTO test_range_part VALUES (1, '2024-01-10', 'test1');
INSERT INTO test_list_part VALUES (1, 'EAST', 'east data');
INSERT INTO test_hash_part VALUES (1, 'hash test');
INSERT INTO test_subpart VALUES (1, '2024-01-05', 'EAST', '1');

-- Switch to part_test_user_1 and create a partition table
SET ROLE part_test_user_1;

CREATE TABLE user1_range_part (
    id INT,
    create_date DATE
)
PARTITION BY RANGE (create_date) (
    PARTITION p1 VALUES LESS THAN ('2024-01-01'),
    PARTITION p2 VALUES LESS THAN (MAXVALUE)
);

RESET ROLE;

-- Switch to part_test_user_2 and create a partition table
SET ROLE part_test_user_2;

CREATE TABLE user2_list_part (
    id INT,
    type VARCHAR2(10)
)
PARTITION BY LIST (type) (
    PARTITION p_a VALUES ('A'),
    PARTITION p_b VALUES ('B')
);

RESET ROLE;

-- DBA_PART_TABLES
SELECT
 table_name,
 partitioning_type,
 subpartitioning_type,
 partition_count,
 def_subpartition_count,
 partitioning_key_count,
 subpartitioning_key_count,
 interval,
 autolist,
 auto
FROM DBA_PART_TABLES
WHERE table_name ILIKE 'test_%' OR table_name ILIKE 'user1_%' OR table_name ILIKE 'user2_%'
ORDER BY owner, table_name;

-- DBA_TAB_PARTITIONS
SELECT
 table_name,
 composite,
 partition_name,
 subpartition_count,
 high_value,
 high_value_length,
 partition_position,
 interval
FROM DBA_TAB_PARTITIONS
WHERE table_name ILIKE 'test_%' OR table_name ILIKE 'user1_%' OR table_name ILIKE 'user2_%'
ORDER BY table_owner, table_name, partition_name;

-- DBA_TAB_SUBPARTITIONS
SELECT
 table_name,
 partition_name,
 subpartition_name,
 high_value,
 high_value_length,
 partition_position,
 subpartition_position
FROM DBA_TAB_SUBPARTITIONS
WHERE table_name ILIKE 'test_%' OR table_name ILIKE 'user1_%' OR table_name ILIKE 'user2_%'
ORDER BY table_owner, table_name, partition_name, subpartition_name;

-- DBA_PART_KEY_COLUMNS
SELECT
 name,
 object_type,
 column_name,
 column_position
FROM DBA_PART_KEY_COLUMNS
WHERE name ILIKE 'test_%' OR name ILIKE 'user1_%' OR name ILIKE 'user2_%'
ORDER BY owner, name, column_position;

-- DBA_SUBPART_KEY_COLUMNS
SELECT
 name,
 object_type,
 column_name,
 column_position
FROM DBA_SUBPART_KEY_COLUMNS
WHERE name ILIKE 'test_%' OR name ILIKE 'user1_%' OR name ILIKE 'user2_%'
ORDER BY owner, name, column_position;

-- DBA_SUBPARTITION_TEMPLATES
SELECT
 table_name,
 subpartition_name,
 subpartition_position,
 tablespace_name,
 high_bound
FROM DBA_SUBPARTITION_TEMPLATES
WHERE table_name ILIKE 'test_%' OR table_name ILIKE 'user1_%' OR table_name ILIKE 'user2_%'
ORDER BY user_name, table_name, subpartition_position;

-- ALL_PART_TABLES
SELECT
 table_name,
 partitioning_type,
 subpartitioning_type,
 partition_count,
 def_subpartition_count,
 partitioning_key_count,
 subpartitioning_key_count,
 interval,
 autolist,
 auto
FROM ALL_PART_TABLES
WHERE table_name ILIKE 'test_%' OR table_name ILIKE 'user1_%' OR table_name ILIKE 'user2_%'
ORDER BY owner, table_name;

-- ALL_TAB_PARTITIONS
SELECT
 table_name,
 composite,
 partition_name,
 subpartition_count,
 high_value,
 high_value_length,
 partition_position,
 interval
FROM ALL_TAB_PARTITIONS
WHERE table_name ILIKE 'test_%' OR table_name ILIKE 'user1_%' OR table_name ILIKE 'user2_%'
ORDER BY table_owner, table_name, partition_name;

-- ALL_TAB_SUBPARTITIONS
SELECT
 table_name,
 partition_name,
 subpartition_name,
 high_value,
 high_value_length,
 partition_position,
 subpartition_position
FROM ALL_TAB_SUBPARTITIONS
WHERE table_name ILIKE 'test_%' OR table_name ILIKE 'user1_%' OR table_name ILIKE 'user2_%'
ORDER BY table_owner, table_name, partition_name, subpartition_name;

-- ALL_PART_KEY_COLUMNS
SELECT
 name,
 object_type,
 column_name,
 column_position
FROM ALL_PART_KEY_COLUMNS
WHERE name ILIKE 'test_%' OR name ILIKE 'user1_%' OR name ILIKE 'user2_%'
ORDER BY owner, name, column_position;

-- ALL_SUBPART_KEY_COLUMNS
SELECT
 name,
 object_type,
 column_name,
 column_position
FROM ALL_SUBPART_KEY_COLUMNS
WHERE name ILIKE 'test_%' OR name ILIKE 'user1_%' OR name ILIKE 'user2_%'
ORDER BY owner, name, column_position;

-- ALL_SUBPARTITION_TEMPLATES
SELECT
 table_name,
 subpartition_name,
 subpartition_position,
 tablespace_name,
 high_bound
FROM ALL_SUBPARTITION_TEMPLATES
WHERE table_name ILIKE 'test_%' OR table_name ILIKE 'user1_%' OR table_name ILIKE 'user2_%'
ORDER BY user_name, table_name, subpartition_position;

-- USER_PART_TABLES
SELECT
 table_name,
 partitioning_type,
 subpartitioning_type,
 partition_count,
 def_subpartition_count,
 partitioning_key_count,
 subpartitioning_key_count,
 interval,
 autolist,
 auto
FROM USER_PART_TABLES
WHERE table_name ILIKE 'test_%' OR table_name ILIKE 'user1_%' OR table_name ILIKE 'user2_%'
ORDER BY table_name;

-- USER_TAB_PARTITIONS
SELECT
 table_name,
 composite,
 partition_name,
 subpartition_count,
 high_value,
 high_value_length,
 partition_position,
 interval
FROM USER_TAB_PARTITIONS
WHERE table_name ILIKE 'test_%' OR table_name ILIKE 'user1_%' OR table_name ILIKE 'user2_%'
ORDER BY table_name, partition_name;

-- USER_TAB_SUBPARTITIONS
SELECT
 table_name,
 partition_name,
 subpartition_name,
 high_value,
 high_value_length,
 partition_position,
 subpartition_position
FROM USER_TAB_SUBPARTITIONS
WHERE table_name ILIKE 'test_%' OR table_name ILIKE 'user1_%' OR table_name ILIKE 'user2_%'
ORDER BY table_name, partition_name, subpartition_name;

-- USER_PART_KEY_COLUMNS
SELECT
 name,
 object_type,
 column_name,
 column_position
FROM USER_PART_KEY_COLUMNS
WHERE name ILIKE 'test_%' OR name ILIKE 'user1_%' OR name ILIKE 'user2_%'
ORDER BY name, column_position;

-- USER_SUBPART_KEY_COLUMNS
SELECT
 name,
 object_type,
 column_name,
 column_position
FROM USER_SUBPART_KEY_COLUMNS
WHERE name ILIKE 'test_%' OR name ILIKE 'user1_%' OR name ILIKE 'user2_%'
ORDER BY name, column_position;

-- USER_SUBPARTITION_TEMPLATES
SELECT
 table_name,
 subpartition_name,
 subpartition_position,
 tablespace_name,
 high_bound
FROM USER_SUBPARTITION_TEMPLATES
WHERE table_name ILIKE 'test_%' OR table_name ILIKE 'user1_%' OR table_name ILIKE 'user2_%'
ORDER BY table_name, subpartition_position;

-- Cleanup
DROP TABLE IF EXISTS test_range_part;
DROP TABLE IF EXISTS test_list_part;
DROP TABLE IF EXISTS test_hash_part;
DROP TABLE IF EXISTS test_subpart;
SET ROLE part_test_user_1;
DROP TABLE IF EXISTS user1_range_part;
RESET ROLE;
SET ROLE part_test_user_2;
DROP TABLE IF EXISTS user2_list_part;
RESET ROLE;
DROP OWNED BY part_test_user_1 CASCADE;
DROP OWNED BY part_test_user_2 CASCADE;
DROP ROLE IF EXISTS part_test_user_1;
DROP ROLE IF EXISTS part_test_user_2;

--
-- Oracle syntax partition tables
--
-- Simple partition table (RANGE with multiple partition keys)
CREATE TABLE ora_simple_range(a INT, b INT, c INT)
PARTITION BY RANGE(a, b)
(
    PARTITION p1 VALUES LESS THAN(10, 100),
    PARTITION p2 VALUES LESS THAN(20, 200),
    PARTITION p3 VALUES LESS THAN(30, 300)
);

-- Composite partition table with subpartition template (RANGE-RANGE, multiple partition keys)
CREATE TABLE ora_composite_range_range(a INT, b INT, c INT, d INT)
PARTITION BY RANGE(a, b) SUBPARTITION BY RANGE(c, d)
SUBPARTITION TEMPLATE
(
    SUBPARTITION sp1 VALUES LESS THAN(100, 1000),
    SUBPARTITION sp2 VALUES LESS THAN(200, 2000)
)
(
    PARTITION p1 VALUES LESS THAN(10, 100),
    PARTITION p2 VALUES LESS THAN(20, 200)
);

-- Composite partition table with subpartition template (LIST-HASH, LIST has 1 key due to PG limit)
CREATE TABLE ora_composite_list_hash(a INT, b INT, c INT, d INT)
PARTITION BY LIST(a) SUBPARTITION BY HASH(b, c) SUBPARTITIONS 3
(
    PARTITION p1 VALUES(1, 2, 3),
    PARTITION p2 VALUES(4, 5, 6)
);

--
-- PostgreSQL syntax partition tables
--
-- Simple partition table (RANGE with multiple partition keys)
CREATE TABLE pg_simple_range(a INT, b INT, c INT) PARTITION BY RANGE(a, b);
CREATE TABLE pg_simple_range_p1 PARTITION OF pg_simple_range FOR VALUES FROM (0, 0) TO (10, 100);
CREATE TABLE pg_simple_range_p2 PARTITION OF pg_simple_range FOR VALUES FROM (10, 100) TO (20, 200);
CREATE TABLE pg_simple_range_p3 PARTITION OF pg_simple_range FOR VALUES FROM (20, 200) TO (30, 300);

-- Composite partition table with different subpartition strategies
-- p1 has RANGE subpartition, p2 has LIST subpartition
CREATE TABLE pg_composite_diff_strategy(a INT, b INT, c INT, d INT) PARTITION BY RANGE(a, b);
CREATE TABLE pg_composite_diff_strategy_p1 PARTITION OF pg_composite_diff_strategy FOR VALUES FROM (0, 0) TO (10, 100) PARTITION BY RANGE(c, d);
CREATE TABLE pg_composite_diff_strategy_p2 PARTITION OF pg_composite_diff_strategy FOR VALUES FROM (10, 100) TO (20, 200) PARTITION BY LIST(c);
CREATE TABLE pg_composite_diff_strategy_p1_sp1 PARTITION OF pg_composite_diff_strategy_p1 FOR VALUES FROM (0, 0) TO (100, 1000);
CREATE TABLE pg_composite_diff_strategy_p1_sp2 PARTITION OF pg_composite_diff_strategy_p1 FOR VALUES FROM (100, 1000) TO (200, 2000);
CREATE TABLE pg_composite_diff_strategy_p2_sp1 PARTITION OF pg_composite_diff_strategy_p2 FOR VALUES IN (1, 2, 3);
CREATE TABLE pg_composite_diff_strategy_p2_sp2 PARTITION OF pg_composite_diff_strategy_p2 FOR VALUES IN (4, 5, 6);

-- Composite partition table with same subpartition strategy
CREATE TABLE pg_composite_same_strategy(a INT, b INT, c INT, d INT) PARTITION BY RANGE(a, b);
CREATE TABLE pg_composite_same_strategy_p1 PARTITION OF pg_composite_same_strategy FOR VALUES FROM (0, 0) TO (10, 100) PARTITION BY RANGE(c, d);
CREATE TABLE pg_composite_same_strategy_p2 PARTITION OF pg_composite_same_strategy FOR VALUES FROM (10, 100) TO (20, 200) PARTITION BY RANGE(c, d);
CREATE TABLE pg_composite_same_strategy_p1_sp1 PARTITION OF pg_composite_same_strategy_p1 FOR VALUES FROM (0, 0) TO (100, 1000);
CREATE TABLE pg_composite_same_strategy_p1_sp2 PARTITION OF pg_composite_same_strategy_p1 FOR VALUES FROM (100, 1000) TO (200, 2000);
CREATE TABLE pg_composite_same_strategy_p2_sp1 PARTITION OF pg_composite_same_strategy_p2 FOR VALUES FROM (0, 0) TO (100, 1000);
CREATE TABLE pg_composite_same_strategy_p2_sp2 PARTITION OF pg_composite_same_strategy_p2 FOR VALUES FROM (100, 1000) TO (200, 2000);

-- Multi-level partition table (3 levels)
CREATE TABLE pg_multi_level(a INT, b INT, c INT, d INT) PARTITION BY RANGE(a, b);
CREATE TABLE pg_multi_level_p1 PARTITION OF pg_multi_level FOR VALUES FROM (0, 0) TO (10, 100) PARTITION BY RANGE(c, d);
CREATE TABLE pg_multi_level_p2 PARTITION OF pg_multi_level FOR VALUES FROM (10, 100) TO (20, 200) PARTITION BY RANGE(c);
CREATE TABLE pg_multi_level_p1_sp1 PARTITION OF pg_multi_level_p1 FOR VALUES FROM (0, 0) TO (50, 500) PARTITION BY RANGE(d);
CREATE TABLE pg_multi_level_p1_sp2 PARTITION OF pg_multi_level_p1 FOR VALUES FROM (50, 500) TO (100, 1000) PARTITION BY RANGE(d);
CREATE TABLE pg_multi_level_p1_sp1_ssp1 PARTITION OF pg_multi_level_p1_sp1 FOR VALUES FROM (0) TO (500);
CREATE TABLE pg_multi_level_p1_sp1_ssp2 PARTITION OF pg_multi_level_p1_sp1 FOR VALUES FROM (500) TO (1000);
CREATE TABLE pg_multi_level_p1_sp2_ssp1 PARTITION OF pg_multi_level_p1_sp2 FOR VALUES FROM (0) TO (500);
CREATE TABLE pg_multi_level_p2_sp1 PARTITION OF pg_multi_level_p2 FOR VALUES FROM (0) TO (50) PARTITION BY RANGE(d);
CREATE TABLE pg_multi_level_p2_sp1_ssp1 PARTITION OF pg_multi_level_p2_sp1 FOR VALUES FROM (0) TO (500);

-- Composite partition table with different subpartition key counts
-- p1 has 1 subpartition key, p2 has 2 subpartition keys
CREATE TABLE pg_diff_key_count(a INT, b INT, c INT, d INT) PARTITION BY RANGE(a, b);
CREATE TABLE pg_diff_key_count_p1 PARTITION OF pg_diff_key_count FOR VALUES FROM (0, 0) TO (10, 100) PARTITION BY RANGE(c);
CREATE TABLE pg_diff_key_count_p2 PARTITION OF pg_diff_key_count FOR VALUES FROM (10, 100) TO (20, 200) PARTITION BY RANGE(c, d);
CREATE TABLE pg_diff_key_count_p1_sp1 PARTITION OF pg_diff_key_count_p1 FOR VALUES FROM (0) TO (100);
CREATE TABLE pg_diff_key_count_p2_sp1 PARTITION OF pg_diff_key_count_p2 FOR VALUES FROM (0, 100) TO (100, 200);

-- Composite partition table with same key count but different key columns
-- p1 uses (c) as subpartition key, p2 uses (d) as subpartition key
CREATE TABLE pg_diff_key_column(a INT, b INT, c INT, d INT) PARTITION BY RANGE(a, b);
CREATE TABLE pg_diff_key_column_p1 PARTITION OF pg_diff_key_column FOR VALUES FROM (0, 0) TO (10, 100) PARTITION BY RANGE(c);
CREATE TABLE pg_diff_key_column_p2 PARTITION OF pg_diff_key_column FOR VALUES FROM (10, 100) TO (20, 200) PARTITION BY RANGE(d);
CREATE TABLE pg_diff_key_column_p1_sp1 PARTITION OF pg_diff_key_column_p1 FOR VALUES FROM (0) TO (100);
CREATE TABLE pg_diff_key_column_p2_sp1 PARTITION OF pg_diff_key_column_p2 FOR VALUES FROM (0) TO (100);

-- Composite partition table with same subpartition keys
-- Both p1 and p2 use (c, d) as subpartition keys with same strategy
CREATE TABLE pg_same_key(a INT, b INT, c INT, d INT) PARTITION BY RANGE(a, b);
CREATE TABLE pg_same_key_p1 PARTITION OF pg_same_key FOR VALUES FROM (0, 0) TO (10, 100) PARTITION BY RANGE(c, d);
CREATE TABLE pg_same_key_p2 PARTITION OF pg_same_key FOR VALUES FROM (10, 100) TO (20, 200) PARTITION BY RANGE(c, d);
CREATE TABLE pg_same_key_p1_sp1 PARTITION OF pg_same_key_p1 FOR VALUES FROM (0, 0) TO (100, 1000);
CREATE TABLE pg_same_key_p1_sp2 PARTITION OF pg_same_key_p1 FOR VALUES FROM (100, 1000) TO (200, 2000);
CREATE TABLE pg_same_key_p2_sp1 PARTITION OF pg_same_key_p2 FOR VALUES FROM (0, 0) TO (100, 1000);
CREATE TABLE pg_same_key_p2_sp2 PARTITION OF pg_same_key_p2 FOR VALUES FROM (100, 1000) TO (200, 2000);

-- ALL_PART_TABLES
SELECT table_name, partitioning_type, subpartitioning_type,
       partition_count, def_subpartition_count,
       partitioning_key_count, subpartitioning_key_count,
       def_tablespace_name
FROM all_part_tables
WHERE table_name ILIKE 'ora_%' OR table_name ILIKE 'pg_%'
ORDER BY table_name;

-- ALL_TAB_PARTITIONS
SELECT table_name, composite, partition_name,
       subpartition_count, high_value, high_value_length,
       partition_position, tablespace_name, pct_free, logging
FROM all_tab_partitions
WHERE table_name ILIKE 'ora_%' OR table_name ILIKE 'pg_%'
ORDER BY table_name, partition_name;

-- ALL_TAB_SUBPARTITIONS
SELECT table_name, partition_name, subpartition_name,
       high_value, high_value_length,
       partition_position, subpartition_position, tablespace_name, pct_free, logging
FROM all_tab_subpartitions
WHERE table_name ILIKE 'ora_%' OR table_name ILIKE 'pg_%'
ORDER BY table_name, partition_name, subpartition_name;

-- ALL_PART_KEY_COLUMNS
SELECT name, object_type, column_name, column_position
FROM all_part_key_columns
WHERE name ILIKE 'ora_%' OR name ILIKE 'pg_%'
ORDER BY name, column_position;

-- ALL_SUBPART_KEY_COLUMNS
SELECT name, object_type, column_name, column_position
FROM all_subpart_key_columns
WHERE name ILIKE 'ora_%' OR name ILIKE 'pg_%'
ORDER BY name, column_position;

-- ALL_SUBPARTITION_TEMPLATES
SELECT table_name, subpartition_name, subpartition_position, tablespace_name, high_bound
FROM all_subpartition_templates
WHERE table_name ILIKE 'ora_%' OR table_name ILIKE 'pg_%'
ORDER BY table_name, subpartition_name;

--
-- Clean up
--
DROP TABLE IF EXISTS ora_simple_range;
DROP TABLE IF EXISTS ora_composite_range_range;
DROP TABLE IF EXISTS ora_composite_list_hash;
DROP TABLE IF EXISTS pg_simple_range;
DROP TABLE IF EXISTS pg_composite_diff_strategy;
DROP TABLE IF EXISTS pg_composite_same_strategy;
DROP TABLE IF EXISTS pg_multi_level;
DROP TABLE IF EXISTS pg_diff_key_count;
DROP TABLE IF EXISTS pg_diff_key_column;
DROP TABLE IF EXISTS pg_same_key;
/* End - ReqID:SRS-SQL-PARTITION-TABLE */

/* Begin - bug#7786 */
SET timezone = '-08:00';
CREATE TABLE PAR_TEST(
 A FLOAT,
 B NUMERIC,
 C INT,
 D BINARY_FLOAT,
 E BINARY_DOUBLE,
 F DATE,
 G TIMESTAMP(3),
 H TIMESTAMP(4) WITH TIME ZONE,
 I TIMESTAMP(5) WITH LOCAL TIME ZONE,
 J INTERVAL YEAR TO MONTH,
 K INTERVAL DAY TO SECOND,
 L CHAR(10),
 M VARCHAR2(10))
PARTITION BY RANGE(A,B,C,D,E,F,G,H,I,J,K,L,M)
(
 PARTITION P1 VALUES LESS THAN(1E2, 1E2, 1E2, 1E2, 1E2,
							DATE '2026-1-1',
							TIMESTAMP '2026-1-1 11:11:11.123',
							TIMESTAMP '2026-1-1 11:11:11.123 +08:00',
							TIMESTAMP '2026-1-1 11:11:11.123',
							INTERVAL '1-2' YEAR TO MONTH,
							INTERVAL '1 1:1:1' DAY TO SECOND,
							'CHAR',
							'VARCHAR2')
);

select table_name, partition_name, high_value from dba_tab_partitions where table_name ilike 'PAR_TEST';

RESET timezone;
DROP TABLE PAR_TEST;
/* Begin - bug#7786 */

/* Begin - OA#2669 */
CREATE TABLE OA_2669_TB(
    COL1 LONG RAW,
    COL2 FLOAT,
    COL3 TIMESTAMP(6),
    COL4 TIMESTAMP(6) WITH TIME ZONE,
    COL5 TIMESTAMP(6) WITH LOCAL TIME ZONE,
    COL6 INTERVAL YEAR(5) TO MONTH,
    COL7 INTERVAL DAY(9) TO SECOND(6),
    COL8 DATE,
    COL9 CHAR,
    COL10 NCHAR,
    COL11 CHAR(10),
    COL12 NCHAR(10),
    COL13 NVARCHAR2,
    COL14 NVARCHAR2(128),
    COL15 VARCHAR2,
    COL16 VARCHAR2(128),
    COL17 VARCHAR2(16 CHAR),
    COL18 VARCHAR2(17 BYTE),
    COL19 BINARY_DOUBLE,
    COL20 BINARY_FLOAT,
    COL21 BINARY_INTEGER,
    COL22 NUMBER,
    COL23 TIME,
    COL24 TIME WITH TIME ZONE,
    COL26 RAW(10),
    COL27 INTEGER,
    COL28 XMLTYPE,
    COL29 LONG RAW
);

SELECT TABLE_NAME, COLUMN_NAME, DATA_TYPE FROM DBA_TAB_COLUMNS WHERE UPPER(TABLE_NAME) = UPPER('OA_2669_TB');
DROP TABLE OA_2669_TB;
/* End - OA#2669 */
