-- VIEW

-- setup before test start
create database testdb;
\c testdb -
create table test_create(id int, ts timestamptz);
create schema sch_sysdba_u1 authorization sysdba_u1;
create schema sch_sysdba_u2 authorization sysdba_u2;
create schema sch_syssso_u1 authorization syssso_u1;
create schema sch_syssao_u1 authorization syssao_u1;
create schema sch_u1 authorization u1;

create table tb1_sso_u1(id int, id2 int);
create table tb1_sao_u1(id int, id2 int);

create view v1_sso_u1 as select * from test_create;
create view v1_sao_u1 as select * from test_create;
create view v1_u1 as select * from test_create;

alter view v1_sso_u1 owner to syssso_u1;
alter view v1_sao_u1 owner to syssao_u1;
alter view v1_u1 owner to u1;

alter table tb1_sso_u1 owner to syssso_u1;
alter table tb1_sao_u1 owner to syssao_u1;

alter database testdb set se_feature.mode to on;
set se_feature.mode to on;

-- For member of sysdba_role
\c testdb sysdba_u1;

---- CREATE VIEW
------ pass
create view sch_sysdba_u1.v1_sysdba_u1 as select * from test_create;
create view sch_sysdba_u2.v1_sysdba_u2 as select * from test_create;
create view v2_sysdba_u1 as select * from test_create;
create view v2_sysdba_u2 as select * from test_create;
create view sch_u1.v1_u1 as select * from test_create;
------ fail
create view sch_syssso_u1.v2_syssso_u1 as select * from test_create;
create view sch_syssao_u1.v2_syssao_u1 as select * from test_create;


---- ALTER VIEW RENAME
------ pass
alter view v2_sysdba_u1 rename to v2_sysdba_u1_new;
alter view v2_sysdba_u2 rename to v2_sysdba_u2_new;
alter view v1_u1 rename to v1_u1_new;
------ fail
alter view v1_sso_u1 rename to v1_sso_u1_new;
alter view v1_sao_u1 rename to v1_sao_u1_new;

---- ALTER VIEW
------ pass
alter view v2_sysdba_u1_new alter column ts set default now();
alter view v2_sysdba_u2_new alter column ts set default now();
alter view v1_u1_new alter column ts set default now();
------ fail
alter view v1_sso_u1 alter column ts set default now();
alter view v1_sao_u1 alter column ts set default now();

---- ALTER VIEW OWNER
------ pass
alter view v2_sysdba_u1_new owner to sysdba_u1;
alter view v2_sysdba_u2_new owner to sysdba_u2;
alter view v1_u1_new owner to u1;
------ fail
alter view v1_sso_u1 owner to sysdba_u1;
alter view v1_sao_u1 owner to u1;
alter view v2_sysdba_u1_new owner to syssso_u1;
alter view v1_u1_new owner to syssao_u1;

---- ALTER VIEW SCHEMA
------ pass
alter view v2_sysdba_u1_new set schema sch_sysdba_u1;
alter view sch_sysdba_u1.v2_sysdba_u1_new set schema sch_u1;
alter view sch_u1.v2_sysdba_u1_new set schema public;
------ fail
alter view v2_sysdba_u1_new set schema sch_syssso_u1;
alter view v2_sysdba_u1_new set schema sch_syssao_u1;
alter view v1_u1_new set schema sch_syssso_u1;
alter view v1_u1_new set schema sch_syssao_u1;

---- DROP VIEW
------ pass
drop view v2_sysdba_u1_new;
drop view v2_sysdba_u2_new;
drop view v1_u1_new;
------ fail
drop view v1_sso_u1;
drop view v1_sao_u1;

-- Clean test object
\c testdb syssso_u1
set se_feature.mode to on;
alter database testdb set se_feature.mode to off;
\c testdb sysdba_u1

drop view sch_sysdba_u1.v1_sysdba_u1;
drop view sch_sysdba_u2.v1_sysdba_u2;
drop view sch_u1.v1_u1;

drop schema sch_sysdba_u1;
drop schema sch_sysdba_u2;
drop schema sch_syssso_u1;
drop schema sch_syssao_u1;
drop schema sch_u1;

\c template1 sysdba_u1
drop database testdb;

