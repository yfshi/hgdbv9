-- TABLE

-- setup before test start
create database testdb;
\c testdb -
create schema sch_u1 authorization u1;
create schema sch_sysdba_u1 authorization sysdba_u1;
create schema sch_syssso_u1 authorization syssso_u1;
create schema sch_syssao_u1 authorization syssao_u1;

create schema sch_syssso_u2 authorization syssso_u2;
grant create on schema sch_syssso_u2 to sysdba_role;

create table tb1_sso_u1(id int);
create table tb1_sao_u1(id int);
alter table tb1_sso_u1 owner to syssso_u1;
alter table tb1_sao_u1 owner to syssao_u1;
create table tb1_u1(id int);
alter table tb1_u1 owner to u1;

alter database testdb set se_feature.mode to on;
set se_feature.mode to on;

-- For member of sysdba_role
\c testdb sysdba_u1;
---- CREATE TABLE
------ pass
create table public.tb1_dba_u1(id int);
create table tb2_dba_u1(id int);
create table sch_sysdba_u1.tb1_dba_u1(id int);
create table sch_u1.tb1_dba_u1(id int);
create table sch_syssso_u2.tb1_dba_u1(id int);
------ fail
create table sch_syssso_u1.tb1_dba_u1(id int);
create table sch_syssao_u1.tb1_dba_u1(id int);
create table sch_non_exist.tb1_non_exist(id int);

---- ALTER TABLE OWNER
------ pass
alter table tb2_dba_u1 owner to sysdba_u2;
alter table tb2_dba_u1 owner to u1;
alter table tb2_dba_u1 owner to u2;
alter table tb2_dba_u1 owner to sysdba_u1;
alter table if exists tb1_non_exist owner to u1;
------ fail
alter table tb1_dba_u1 owner to syssso_u1;
alter table tb1_dba_u1 owner to syssao_u1;
alter table tb1_sso_u1 owner to sysdba_u1;
alter table tb1_sao_u1 owner to u1;
alter table tb1_non_exist owner to u1;

---- ALTER TABLE RENAME
------ pass
alter table tb2_dba_u1 rename to tb2_dba_u1_new;
alter table tb2_dba_u1_new owner to u1;
alter table tb2_dba_u1_new rename to tb2_dba_u1;
alter table tb2_dba_u1 owner to sysdba_u1;
alter table if exists tb1_non_exist rename to tb1_non_exist_new;
------ fail
alter table tb1_sso_u1 rename to tb1_sso_u1_new;
alter table tb1_sao_u1 rename to tb1_sao_u1_new;
alter table tb1_non_exist rename to tb1_non_exist_new;

---- ALTER TABLE SCHEMA
------ pass
alter table tb2_dba_u1 set schema sch_sysdba_u1;
alter table sch_sysdba_u1.tb2_dba_u1 set schema sch_u1;
alter table sch_u1.tb2_dba_u1 set schema public;
------ fail
alter table tb2_dba_u1 set schema sch_syssso_u1;
alter table tb2_dba_u1 set schema sch_syssao_u1;
alter table tb1_sso_u1 set schema sch_syssso_u1;
alter table tb1_sao_u1 set schema sch_syssso_u1;
alter table tb1_non_exist set schema sch_sysdba_u1;
alter table tb2_dba_u1 set schema sch_non_exist;

---- ALTER TABLE
------ pass
alter table tb1_dba_u1 add column name text;
alter table tb2_dba_u1 owner to sysdba_u2;
alter table tb1_dba_u1 drop column name;
alter table tb2_dba_u1 owner to sysdba_u1;
alter table tb1_u1 add column name text;
alter table tb1_u1 drop column name;
------ fail
alter table tb1_sso_u1 add column name text;
alter table tb1_sao_u1 add column name text;
alter table tb1_non_exist add column name text;

---- GRANT/REVOKE
------ pass
grant all on tb1_dba_u1 to sysdba_u2;
grant all on tb1_dba_u1 to u2;
grant all on tb1_u1 to sysdba_u2;
grant all on tb1_u1 to u2;
------ fail
grant all on tb1_dba_u1 to syssso_u1;
grant all on tb1_dba_u1 to syssao_u1;
grant all on tb1_u1 to syssso_u1;
grant all on tb1_u1 to syssao_u1;
grant all on tb1_sso_u1 to sysdba_u1;
grant all on tb1_sao_u1 to u1;
grant all on tb1_sao_u1 to sysdba_u1;
grant all on tb1_sso_u1 to u1;
grant all on tb1_non_exist to sysdba_u2;

---- DROP TABLE
------ pass
drop table tb1_dba_u1;
drop table tb2_dba_u1;
drop table sch_sysdba_u1.tb1_dba_u1;
drop table sch_u1.tb1_dba_u1;
drop table sch_syssso_u2.tb1_dba_u1;
drop table if exists tb1_non_exist;
------ fail
drop table tb1_sso_u1;
drop table tb1_sao_u1;
drop table tb1_non_exist;

-- Clean test object
\c testdb syssso_u1
set se_feature.mode to on;
alter database testdb set se_feature.mode to off;
\c testdb sysdba_u1
drop schema sch_syssso_u1;
drop schema sch_syssao_u1;
drop schema sch_sysdba_u1;
drop schema sch_u1;
drop schema sch_syssso_u2;

drop table tb1_u1;
drop table tb1_sso_u1;
drop table tb1_sao_u1;

\c template1 sysdba_u1
drop database testdb;


-- set restricted_dba to on;

-- setup before test start
create database testdb;
\c testdb -
create schema sch_u1 authorization u1;
create schema sch_sysdba_u1 authorization sysdba_u1;
create schema sch_syssso_u1 authorization syssso_u1;
create schema sch_syssao_u1 authorization syssao_u1;

create schema sch_syssso_u2 authorization syssso_u2;
grant create on schema sch_syssso_u2 to sysdba_role;
create schema sch_u2 authorization u2;
grant create on schema sch_u2 to sysdba_role;
create schema sch1_sysdba_u2 authorization sysdba_u2;
create schema sch2_sysdba_u2 authorization sysdba_u2;
grant create on schema sch1_sysdba_u2 to sysdba_role;

create table tb1_sso_u1(id int);
alter table tb1_sso_u1 owner to syssso_u1;
create table tb1_sao_u1(id int);
alter table tb1_sao_u1 owner to syssao_u1;
create table tb1_u1(id int);
alter table tb1_u1 owner to u1;
create table tb1_dba_u2(id int);
alter table tb1_dba_u2 owner to sysdba_u2;



alter database testdb set se_feature.mode to on;
alter database testdb set se_feature.restricted_dba to on;
set se_feature.restricted_dba to on;
set se_feature.mode to on;

-- For member of sysdba_role
\c testdb sysdba_u1;
---- CREATE TABLE
------ pass
create table public.tb1_dba_u1(id int);
create table tb2_dba_u1(id int);
create table sch_sysdba_u1.tb1_dba_u1(id int);
create table sch1_sysdba_u2.tb1_dba_u1(id int);
create table sch_syssso_u2.tb1_dba_u1(id int);
create table sch_u2.tb1_dba_u1(id int);
------ fail
create table sch2_sysdba_u2.tb1_dba_u1(id int);
create table sch_syssso_u1.tb1_dba_u1(id int);
create table sch_syssao_u1.tb1_dba_u1(id int);
create table sch_u1.tb1_dba_u1(id int);
create table sch_non_exist.tb1_non_exist(id int);

---- ALTER TABLE OWNER
------ pass
begin;
alter table tb2_dba_u1 owner to sysdba_u2;
rollback;
alter table if exists tb1_non_exist owner to u1;
------ fail
alter table tb1_dba_u1 owner to syssso_u1;
alter table tb1_dba_u1 owner to syssao_u1;
alter table tb1_dba_u1 owner to u1;
alter table tb1_dba_u2 owner to sysdba_u1;
alter table tb1_dba_u2 owner to syssso_u1;
alter table tb1_dba_u2 owner to syssao_u1;
alter table tb1_dba_u2 owner to u1;
alter table tb1_sso_u1 owner to sysdba_u1;
alter table tb1_sso_u1 owner to syssao_u1;
alter table tb1_sso_u1 owner to syssso_u2;
alter table tb1_sso_u1 owner to u1;
alter table tb1_sao_u1 owner to sysdba_u1;
alter table tb1_sao_u1 owner to syssao_u2;
alter table tb1_sao_u1 owner to syssso_u1;
alter table tb1_sao_u1 owner to u1;
alter table tb1_u1 owner to sysdba_u1;
alter table tb1_u1 owner to syssao_u1;
alter table tb1_u1 owner to syssso_u1;
alter table tb1_u1 owner to u2;

---- ALTER TABLE RENAME
------ pass
begin;
alter table tb2_dba_u1 rename to tb2_dba_u1_new;
rollback;
alter table if exists tb1_non_exist rename to tb1_non_exist_new;
------ fail
alter table tb1_dba_u2 rename to tb1_sso_u1_new;
alter table tb1_sso_u1 rename to tb1_sso_u1_new;
alter table tb1_sao_u1 rename to tb1_sao_u1_new;
alter table tb1_u1 rename to tb1_u1_new;
alter table tb1_non_exist rename to tb1_non_exist_new;

---- ALTER TABLE SCHEMA
------ pass
alter table tb2_dba_u1 set schema sch_sysdba_u1;
alter table sch_sysdba_u1.tb2_dba_u1 set schema sch1_sysdba_u2;
alter table sch1_sysdba_u2.tb2_dba_u1 set schema sch_syssso_u2;
alter table sch_syssso_u2.tb2_dba_u1 set schema sch_u2;
alter table sch_u2.tb2_dba_u1 set schema public;
------ fail
alter table tb1_dba_u1 set schema sch_syssso_u1;
alter table tb1_dba_u1 set schema sch_syssao_u1;
alter table tb1_dba_u1 set schema sch2_sysdba_u2;
alter table tb1_dba_u1 set schema sch_u1;
alter table tb1_sso_u1 set schema sch_syssso_u1;
alter table tb1_sao_u1 set schema sch_syssao_u1;
alter table tb1_dba_u2 set schema sch1_sysdba_u2;
alter table tb1_u1 set schema sch_u1;
alter table tb1_non_exist set schema sch_sysdba_u1;
alter table tb1_dba_u1 set schema sch_non_exist;

---- ALTER TABLE
------ pass
alter table tb1_dba_u1 add column name text;
alter table if exists tb1_non_exist add column name text;
------ fail
alter table tb1_dba_u2 add column name text;
alter table tb1_sso_u1 add column name text;
alter table tb1_sao_u1 add column name text;
alter table tb1_u1 add column name text;
alter table tb1_non_exist add column name text;

---- GRANT/REVOKE
------ pass
grant all on tb1_dba_u1 to sysdba_u2;
------ fail
grant all on tb1_dba_u1 to syssso_u1;
grant all on tb1_dba_u1 to syssao_u1;
grant all on tb1_dba_u1 to u1;
grant all on tb1_u1 to sysdba_u1;
grant all on tb1_u1 to syssso_u1;
grant all on tb1_u1 to syssao_u1;
grant all on tb1_u1 to u2;
grant all on tb1_sso_u1 to sysdba_u1;
grant all on tb1_sao_u1 to u1;
grant all on tb1_sao_u1 to sysdba_u1;
grant all on tb1_sso_u1 to u1;
grant all on tb1_non_exist to sysdba_u2;

---- DROP TABLE
------ pass
drop table tb1_dba_u1;
drop table tb2_dba_u1;
drop table sch_sysdba_u1.tb1_dba_u1;
drop table sch1_sysdba_u2.tb1_dba_u1;
drop table sch_syssso_u2.tb1_dba_u1;
drop table sch_u2.tb1_dba_u1;
drop table if exists tb1_non_exist;
------ fail
drop table tb1_dba_u2;
drop table tb1_sso_u1;
drop table tb1_sao_u1;
drop table tb1_u1;
drop table tb1_non_exist;

-- Clean test object
\c testdb syssso_u1
set se_feature.mode to on;
alter database testdb set se_feature.mode to off;
alter database testdb set se_feature.restricted_dba to off;

\c template1 sysdba_u1
drop database testdb;
