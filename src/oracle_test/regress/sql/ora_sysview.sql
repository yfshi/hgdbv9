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

select table_name from all_tables where upper(table_name) like('SYSVIEW_ALL_TABLES%') or upper(table_name) like('BIN%');

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

select table_name, column_name, data_type from all_tab_cols where upper(table_name) like('BIN$%');

purge recyclebin;
set ivorysql.enable_recycle_bin = off;

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

select table_name, column_name, data_type from all_tab_columns where upper(table_name) like('BIN$%');

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

select table_type, comments, origin_con_id from all_tab_comments where upper(table_name) like ('BIN$%') order by table_name;

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

select index_name, index_type, table_name from all_indexes where upper(table_name) like ('BIN$%');

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
from all_ind_columns where upper(table_name) like('BIN%') or index_name like('BIN%') order by table_name, index_name;

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
where upper(referenced_name) like('BIN$%') or upper(name) like('BIN$%') order by name, referenced_name;
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
    where upper(object_name) like('BIN$%') and object_type in ('INDEX', 'TABLE', 'VIEW', 'SEQUENCE') order by object_name;

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

select count(*) from all_constraints where upper(table_name) like('BIN%');
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

select count(*) from all_cons_columns where upper(table_name) like('BIN%');
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
select table_name, composite, partition_name, subpartition_count, high_value, high_value_length, partition_position, pct_free, logging,
        num_rows, blocks, avg_row_len
from all_tab_partitions where upper(table_name) like('SYSVIEW_TAB_PARTITIONS_%') order by table_name, partition_name;

drop table sysview_tab_partitions_part1;
drop table sysview_tab_partitions_part2;

-- recyclebin does not support partitioned table yet
select table_name, composite, partition_name, subpartition_count, high_value, high_value_length, partition_position, pct_free, logging,
        num_rows, blocks, avg_row_len
from all_tab_partitions where upper(table_name) like('BIN$%') order by table_name, partition_name;

purge recyclebin;
set ivorysql.enable_recycle_bin to off;
