-- SEQUENCE

-- setup before test start
create database testdb;
\c testdb -
create schema sch_u1 authorization u1;
create schema sch_sysdba_u1 authorization sysdba_u1;
create schema sch_sysdba_u2 authorization sysdba_u2;
create schema sch_syssso_u1 authorization syssso_u1;
create schema sch_syssao_u1 authorization syssao_u1;

create schema sch_syssso_u2 authorization syssso_u2;
grant create on schema sch_syssso_u2 to sysdba_role;

create sequence seq1_syssso_u1;
create sequence seq1_syssao_u1;
create sequence seq1_u1;

alter sequence seq1_syssso_u1 owner to syssso_u1;
alter sequence seq1_syssao_u1 owner to syssao_u1;

alter database testdb set se_feature.mode to on;
set se_feature.mode to on;

-- For member of sysdba_role
\c testdb sysdba_u1;

---- CREATE SEQUENCE
------ pass
create sequence sch_sysdba_u1.seq1_sysdba_u1;
create sequence sch_sysdba_u2.seq1_sysdba_u2;
create sequence seq1_sysdba_u1;
create sequence seq1_sysdba_u2;
create sequence sch_u1.seq2_u1;
create sequence sch_syssso_u2.seq1_sysdba_u1;
------ fail
create sequence sch_syssso_u1.seq2_syssso_u1;
create sequence sch_syssao_u1.seq2_syssao_u1;
create sequence sch_non_exist.seq_non_exist;

---- ALTER SEQUENCE OWNER
------ pass
alter sequence seq1_sysdba_u1 owner to sysdba_u1;
alter sequence seq1_sysdba_u2 owner to sysdba_u2;
alter sequence seq1_u1 owner to u1;
alter sequence if exists seq_non_exist owner to sysdba_u1;
------ fail
alter sequence seq1_syssso_u1 owner to sysdba_u1;
alter sequence seq1_syssao_u1 owner to u1;
alter sequence seq1_sysdba_u1 owner to syssso_u1;
alter sequence seq1_u1 owner to syssao_u1;
alter sequence seq_non_exist owner to sysdba_u1;

---- ALTER SEQUENCE
------ pass
alter sequence seq1_sysdba_u1 restart with 100;
alter sequence seq1_sysdba_u2 restart with 100;
alter sequence seq1_u1 restart with 100;
alter sequence if exists seq_non_exist restart with 100;
------ fail
alter sequence seq1_syssso_u1 restart with 100;
alter sequence seq1_syssao_u1 restart with 100;
alter sequence seq_non_exist restart with 100;

---- ALTER SEQUENCE RENAME
------ pass
alter sequence seq1_sysdba_u1 rename to seq1_sysdba_u1_new;
alter sequence seq1_sysdba_u2 rename to seq1_sysdba_u2_new;
alter sequence seq1_u1 rename to seq1_u1_new;
alter sequence if exists seq_non_exist rename to seq_non_exist_new;
------ fail
alter sequence seq1_syssso_u1 rename to seq1_syssso_u1_new;
alter sequence seq1_syssao_u1 rename to seq1_syssao_u1_new;
alter sequence seq_non_exist rename to seq_non_exist_new;

---- ALTER SEQUENCE SCHEMA
------ pass
alter sequence seq1_sysdba_u1_new set schema sch_sysdba_u1;
alter sequence sch_sysdba_u1.seq1_sysdba_u1_new set schema sch_u1;
alter sequence sch_u1.seq1_sysdba_u1_new set schema public;
------ fail
alter sequence seq1_sysdba_u1_new set schema sch_syssso_u1;
alter sequence seq1_u1_new set schema sch_syssao_u1;
alter sequence seq1_syssso_u1 set schema sch_sysdba_u1;
alter sequence seq1_syssao_u1 set schema sch_u1;
alter sequence seq_non_exist set schema sch_sysdba_u1;
alter sequence seq1_sysdba_u1_new set schema sch_non_exist;

---- GRANT SEQUENCE
------ pass
grant all on seq1_sysdba_u1_new to sysdba_u1;
grant all on seq1_sysdba_u2_new to sysdba_u2;
grant all on seq1_sysdba_u1_new to u1;
grant all on seq1_u1_new to u1;
grant all on seq1_u1_new to u2;
grant all on seq1_u1_new to sysdba_u1;
------ fail
grant all on seq1_sysdba_u1_new to syssso_u1;
grant all on seq1_sysdba_u1_new to syssao_u1;
grant all on seq1_sysdba_u2_new to syssao_u1;
grant all on seq1_sysdba_u2_new to syssao_u1;
grant all on seq1_syssso_u1 to sysdba_u1;
grant all on seq1_syssao_u1 to sysdba_u1;
grant all on seq1_u1_new to syssso_u1;
grant all on seq1_u1_new to syssao_u1;
grant all on seq_non_exist to sysdba_u1;

---- DROP SEQUENCE
------ pass
drop sequence seq1_sysdba_u1_new;
drop sequence seq1_sysdba_u2_new;
drop sequence seq1_u1_new;
drop sequence sch_syssso_u2.seq1_sysdba_u1;
drop sequence if exists seq_non_exist;
------ fail
drop sequence seq1_syssso_u1;
drop sequence seq1_syssao_u1;
drop sequence seq_non_exist;

-- Clean test object
\c testdb syssso_u1
set se_feature.mode to on;
alter database testdb set se_feature.mode to off;
\c testdb sysdba_u1
drop sequence sch_sysdba_u1.seq1_sysdba_u1;
drop sequence sch_sysdba_u2.seq1_sysdba_u2;
drop sequence sch_u1.seq2_u1;

drop schema sch_sysdba_u1;
drop schema sch_sysdba_u2;
drop schema sch_syssso_u1;
drop schema sch_syssao_u1;
drop schema sch_u1;
drop schema sch_syssso_u2;

drop sequence seq1_syssso_u1;
drop sequence seq1_syssao_u1;

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

create sequence seq1_syssso_u1;
create sequence seq1_syssao_u1;
create sequence seq1_u1;
create sequence seq1_sysdba_u2;

alter sequence seq1_syssso_u1 owner to syssso_u1;
alter sequence seq1_syssao_u1 owner to syssao_u1;
alter sequence seq1_u1 owner to u1;
alter sequence seq1_sysdba_u2 owner to sysdba_u2;

alter database testdb set se_feature.mode to on;
alter database testdb set se_feature.restricted_dba to on;
set se_feature.restricted_dba to on;
set se_feature.mode to on;

-- For member of sysdba_role
\c testdb sysdba_u1;

---- CREATE SEQUENCE
------ pass
create sequence public.seq1_dba_u1;
create sequence seq2_dba_u1;
create sequence sch_sysdba_u1.seq1_dba_u1;
create sequence sch1_sysdba_u2.seq1_dba_u1;
create sequence sch_syssso_u2.seq1_dba_u1;
create sequence sch_u2.seq1_dba_u1;
------ fail
create sequence sch2_sysdba_u2.seq1_dba_u1;
create sequence sch_syssso_u1.seq1_dba_u1;
create sequence sch_syssao_u1.seq1_dba_u1;
create sequence sch_u1.seq1_dba_u1;
create sequence sch_non_exist.seq1_dba_u1;

---- ALTER SEQUENCE OWNER
------ pass
begin;
alter sequence seq1_dba_u1 owner to sysdba_u2;
rollback;
alter sequence if exists seq_non_exist owner to sysdba_u1;
------ fail
alter sequence seq1_dba_u1 owner to syssso_u1;
alter sequence seq1_dba_u1 owner to syssao_u1;
alter sequence seq1_dba_u1 owner to u1;
alter sequence seq1_sysdba_u2 owner to sysdba_u1;
alter sequence seq1_sysdba_u2 owner to syssso_u1;
alter sequence seq1_sysdba_u2 owner to syssao_u1;
alter sequence seq1_sysdba_u2 owner to u1;
alter sequence seq1_syssso_u1 owner to sysdba_u1;
alter sequence seq1_syssso_u1 owner to syssao_u1;
alter sequence seq1_syssso_u1 owner to syssso_u2;
alter sequence seq1_syssso_u1 owner to u1;
alter sequence seq1_syssao_u1 owner to sysdba_u1;
alter sequence seq1_syssao_u1 owner to syssao_u2;
alter sequence seq1_syssao_u1 owner to syssso_u1;
alter sequence seq1_syssao_u1 owner to u1;
alter sequence seq1_u1 owner to sysdba_u1;
alter sequence seq1_u1 owner to syssao_u1;
alter sequence seq1_u1 owner to syssso_u1;
alter sequence seq1_u1 owner to u2;

---- ALTER SEQUENCE
------ pass
alter sequence seq1_dba_u1 restart with 100;
alter sequence if exists seq_non_exist restart with 100;
------ fail
alter sequence seq1_sysdba_u2 restart with 100;
alter sequence seq1_syssso_u1 restart with 100;
alter sequence seq1_syssao_u1 restart with 100;
alter sequence seq1_u1 restart with 100;
alter sequence seq_non_exist restart with 100;

---- ALTER SEQUENCE RENAME
------ pass
begin;
alter sequence seq1_dba_u1 rename to seq1_dba_u1_new;
rollback;
alter sequence if exists seq_non_exist rename to seq_non_exist_new;
------ fail
alter sequence seq1_sysdba_u2 rename to seq1_sysdba_u2_new;
alter sequence seq1_syssso_u1 rename to seq1_syssso_u1_new;
alter sequence seq1_syssao_u1 rename to seq1_syssao_u1_new;
alter sequence seq1_u1 rename to seq1_u1_new;
alter sequence tb1_non_exist rename to seq_non_exist_new;

---- ALTER SEQUENCE SCHEMA
------ pass
alter sequence seq2_dba_u1 set schema sch_sysdba_u1;
alter sequence sch_sysdba_u1.seq2_dba_u1 set schema sch1_sysdba_u2;
alter sequence sch1_sysdba_u2.seq2_dba_u1 set schema sch_syssso_u2;
alter sequence sch_syssso_u2.seq2_dba_u1 set schema sch_u2;
alter sequence sch_u2.seq2_dba_u1 set schema public;
alter sequence if exists seq_non_exist set schema public;
------ fail
alter sequence seq1_dba_u1 set schema sch_syssso_u1;
alter sequence seq1_dba_u1 set schema sch_syssao_u1;
alter sequence seq1_dba_u1 set schema sch2_sysdba_u2;
alter sequence seq1_dba_u1 set schema sch_u1;
alter sequence seq1_syssso_u1 set schema sch_syssso_u1;
alter sequence seq1_syssao_u1 set schema sch_syssao_u1;
alter sequence seq1_sysdba_u2 set schema sch1_sysdba_u2;
alter sequence seq1_u1 set schema sch_u1;
alter sequence seq1_non_exist set schema sch_sysdba_u1;
alter sequence seq1_dba_u1 set schema sch_non_exist;

---- GRANT SEQUENCE
------ pass
grant all on seq1_dba_u1 to sysdba_u2;
------ fail
grant all on seq1_dba_u1 to syssso_u1;
grant all on seq1_dba_u1 to syssao_u1;
grant all on seq1_dba_u1 to u1;
grant all on seq1_u1 to sysdba_u1;
grant all on seq1_u1 to syssso_u1;
grant all on seq1_u1 to syssao_u1;
grant all on seq1_u1 to u2;
grant all on seq1_sysdba_u2 to sysdba_u1;
grant all on seq1_sysdba_u2 to syssso_u1;
grant all on seq1_sysdba_u2 to syssao_u1;
grant all on seq1_sysdba_u2 to u1;
grant all on seq1_syssso_u1 to sysdba_u1;
grant all on seq1_syssao_u1 to u1;
grant all on seq1_syssao_u1 to sysdba_u1;
grant all on seq1_syssso_u1 to u1;
grant all on seq1_non_exist to sysdba_u1;

---- DROP SEQUENCE
------ pass
drop sequence seq1_dba_u1;
drop sequence seq2_dba_u1;
drop sequence sch_sysdba_u1.seq1_dba_u1;
drop sequence sch1_sysdba_u2.seq1_dba_u1;
drop sequence sch_syssso_u2.seq1_dba_u1;
drop sequence sch_u2.seq1_dba_u1;
drop sequence if exists tb1_non_exist;
------ fail
drop sequence seq1_sysdba_u2;
drop sequence seq1_syssso_u1;
drop sequence seq1_syssao_u1;
drop sequence seq1_u1;
drop sequence tb1_non_exist;

-- Clean test object
\c testdb syssso_u1
set se_feature.mode to on;
alter database testdb set se_feature.mode to off;
alter database testdb set se_feature.restricted_dba to off;

\c template1 sysdba_u1
drop database testdb;

