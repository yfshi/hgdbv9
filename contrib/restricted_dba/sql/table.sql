-- TABLE

-- setup before test start
create database testdb;
\c testdb -
create schema sch_u1 authorization u1;
create schema sch_sysdba_u1 authorization sysdba_u1;
create schema sch_syssso_u1 authorization syssso_u1;
create schema sch_syssao_u1 authorization syssao_u1;

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
------ fail
create table sch_syssso_u1.tb1_dba_u1(id int);
create table sch_syssao_u1.tb1_dba_u1(id int);

---- ALTER TABLE OWNER
------ pass
alter table tb2_dba_u1 owner to sysdba_u2;
alter table tb2_dba_u1 owner to u1;
alter table tb2_dba_u1 owner to u2;
alter table tb2_dba_u1 owner to sysdba_u1;
------ fail
alter table tb1_dba_u1 owner to syssso_u1;
alter table tb1_dba_u1 owner to syssao_u1;
alter table tb1_sso_u1 owner to sysdba_u1;
alter table tb1_sao_u1 owner to u1;

---- ALTER TABLE RENAME
------ pass
alter table tb2_dba_u1 rename to tb2_dba_u1_new;
alter table tb2_dba_u1_new owner to u1;
alter table tb2_dba_u1_new rename to tb2_dba_u1;
alter table tb2_dba_u1 owner to sysdba_u1;
------ fail
alter table tb1_sso_u1 rename to tb1_sso_u1_new;
alter table tb1_sao_u1 rename to tb1_sao_u1_new;

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

---- DROP TABLE
------ pass
drop table tb1_dba_u1;
drop table tb2_dba_u1;
drop table sch_sysdba_u1.tb1_dba_u1;
drop table sch_u1.tb1_dba_u1;
------ fail
drop table tb1_sso_u1;
drop table tb1_sao_u1;

-- Clean test object
\c testdb syssso_u1
set se_feature.mode to on;
alter database testdb set se_feature.mode to off;
\c testdb sysdba_u1
drop schema sch_syssso_u1;
drop schema sch_syssao_u1;
drop schema sch_sysdba_u1;
drop schema sch_u1;

drop table tb1_u1;
drop table tb1_sso_u1;
drop table tb1_sao_u1;

\c template1 sysdba_u1
drop database testdb;
