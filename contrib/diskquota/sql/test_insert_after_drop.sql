create database db_insert_after_drop;
\c db_insert_after_drop
create extension diskquota;
-- Test Drop Extension
create schema sdrtbl;
select hg_security.set_schema_quota('sdrtbl', '1 MB');
set search_path to sdrtbl;
create table a(i int);
insert into a select generate_series(1,100);
-- expect insert fail
insert into a select generate_series(1,100000000);
--select pg_sleep(5);
insert into a select generate_series(1,100);
drop extension diskquota;
select pg_sleep(5);
select pg_sleep(1);
insert into a select generate_series(1,100);

drop table a;
drop schema sdrtbl;
--\c postgres
--drop database db_insert_after_drop;

--role Test Drop Extension
CREATE role u1 NOLOGIN;
create schema sdrtbl;
create extension diskquota;
-- Test Drop Extension
select hg_security.set_role_quota('u1', '1 MB');
set search_path to sdrtbl;
create table a(i int);
ALTER TABLE a OWNER TO u1;

insert into a select generate_series(1,100);
-- expect insert fail
insert into a select generate_series(1,100000000);
--select pg_sleep(5);
insert into a select generate_series(1,100);
drop extension diskquota;
select pg_sleep(5);
select pg_sleep(1);
insert into a select generate_series(1,100);

drop table a;
\c postgres
drop database db_insert_after_drop;
drop role u1;
