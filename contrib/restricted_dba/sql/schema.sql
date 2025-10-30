-- SCHEMA

-- setup before test start
create database testdb;
\c testdb -
create schema sch_syssso_u1 authorization syssso_u1;
create schema sch_syssao_u1 authorization syssao_u1;
alter database testdb set se_feature.mode to on;
set se_feature.mode to on;

-- For member of sysdba_role
\c testdb sysdba_u1;
---- CREATE SCHEMA
------ fail
create schema sch_test authorization syssso_u1;
create schema sch_test authorization syssao_u1;
------ pass
create schema sch_u1 authorization u1;
create schema sch_sysdba_u1 authorization sysdba_u1;

---- ALTER SCHEMA
------ fail
alter schema sch_sysdba_u1 owner to syssso_u1;
alter schema sch_u1 owner to syssao_u1;
alter schema sch_syssso_u1 owner  to sysdba_u1;
alter schema sch_syssao_u1 owner  to u1;
alter schema sch_syssso_u1 rename to sch_syssso_u1_new;
alter schema sch_syssao_u1 rename to sch_syssao_u1_new;
------ pass
alter schema sch_sysdba_u1 owner to u1;
alter schema sch_sysdba_u1 owner to sysdba_u1;
alter schema sch_u1 owner to sysdba_u1;
alter schema sch_u1 owner to u1;

---- GRANT/REVOKE
------ pass
grant all on schema sch_sysdba_u1 to sysdba_u2;
grant all on schema sch_u1 to u2;
grant all on schema sch_sysdba_u1 to u1;
grant all on schema sch_u1 to sysdba_u2;
grant usage on schema sch_u1 to syssso_u2;
grant usage on schema sch_sysdba_u1 to syssao_u2;
------ fail
grant all on schema sch_sysdba_u1 to syssso_u1;
grant all on schema sch_syssso_u1 to sysdba_u1;
grant all on schema sch_syssao_u1 to u1;
grant all on schema sch_u1 to syssao_u1;


---- DROP SCHEMA
------ pass
drop schema sch_u1;
drop schema sch_sysdba_u1;
------ fail
drop schema sch_syssso_u1;
drop schema sch_syssao_u1;


-- For member of syssso_role
\c testdb syssso_u1;
---- CREATE SCHEMA
------ fail
create schema sch_test authorization syssso_u2;

---- ALTER SCHEMA
------ fail
alter schema sch_syssso_u1 owner to syssso_u2;
alter schema sch_syssso_u1 rename to sch_syssso_u1_new;

---- GRANT/REVOKE
------ fail
grant all on schema sch_syssso_u1 to syssso_u2;

---- DROP SCHEMA
------ fail
drop schema sch_syssso_u1;


-- Clean test object
\c testdb syssso_u1
set se_feature.mode to on;
alter database testdb set se_feature.mode to off;
\c testdb sysdba_u1
drop schema sch_syssso_u1;
drop schema sch_syssao_u1;
\c template1 sysdba_u1
drop database testdb;

-- test for restricted_dba is on

-- setup before test start
create database testdb;
\c testdb -
create schema sch_syssso_u1 authorization syssso_u1;
create schema sch_syssao_u1 authorization syssao_u1;
alter database testdb set se_feature.mode to on;
alter database testdb set se_feature.restricted_dba to on;
set se_feature.restricted_dba to on;
set se_feature.mode to on;

-- For member of sysdba_role
\c testdb sysdba_u1;
---- CREATE SCHEMA
------ fail
create schema sch_test authorization syssso_u1;
create schema sch_test authorization syssao_u1;
------ pass
create schema sch_u1 authorization u1;
create schema sch_sysdba_u1 authorization sysdba_u1;
create schema sch_sysdba_u2 authorization sysdba_u2;

---- ALTER SCHEMA
------ fail
alter schema sch_sysdba_u1 owner to syssso_u1;
alter schema sch_sysdba_u1 owner to syssao_u1;
alter schema sch_sysdba_u1 owner to u1;
alter schema sch_syssso_u1 owner  to sysdba_u1;
alter schema sch_syssso_u1 rename to sch_syssso_u1_new;
alter schema sch_syssao_u1 owner  to u1;
alter schema sch_syssao_u1 rename to sch_syssao_u1_new;
alter schema sch_sysdba_u2 owner  to sysdba_u1;
alter schema sch_sysdba_u2 rename to sch_sysdba_u2_new;
alter schema sch_u1 owner to u1;
alter schema sch_u1 rename to sch_u1_new;
------ pass
begin;
alter schema sch_sysdba_u1 rename to sch_sysdba_u2_new;
alter schema sch_sysdba_u2_new owner to sysdba_u2;
rollback;

---- GRANT/REVOKE
------ pass
grant all on schema sch_sysdba_u1 to sysdba_u2;
grant all on schema sch_u1 to u2;
grant usage on schema sch_sysdba_u1 to syssao_u2;
------ fail
grant all on schema sch_sysdba_u2 to sysdba_u1;
grant all on schema sch_sysdba_u1 to u1;
grant all on schema sch_u1 to sysdba_u2;
grant all on schema sch_sysdba_u1 to syssso_u1;
grant all on schema sch_syssso_u1 to sysdba_u1;
grant all on schema sch_syssao_u1 to u1;
grant all on schema sch_u1 to syssao_u1;


---- DROP SCHEMA
------ pass
drop schema sch_sysdba_u1;
------ fail
drop schema sch_u1;
drop schema sch_sysdba_u2;
drop schema sch_syssso_u1;
drop schema sch_syssao_u1;


-- Clean test object
\c testdb syssso_u1
set se_feature.mode to on;
alter database testdb set se_feature.mode to off;
\c testdb sysdba_u1
drop schema sch_syssso_u1;
drop schema sch_syssao_u1;
drop schema sch_sysdba_u2;
drop schema sch_u1;
\c template1 sysdba_u1
drop database testdb;