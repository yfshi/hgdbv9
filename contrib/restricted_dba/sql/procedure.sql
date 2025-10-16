-- PROCEDURE

-- setup before test start
create database testdb;
\c testdb -

create schema sch_u1 authorization u1;
create schema sch_sysdba_u1 authorization sysdba_u1;
create schema sch_syssso_u1 authorization syssso_u1;
create schema sch_syssao_u1 authorization syssao_u1;

create schema sch_syssso_u2 authorization syssso_u2;
grant create on schema sch_syssso_u2 to sysdba_role;

create procedure func_u1(a integer, b integer)
as $$select a + b $$ language sql;
create procedure func_sysdba_u1(a integer, b integer)
as $$select a + b $$ language sql;
create procedure func_syssso_u1(a integer, b integer)
as $$select a + b $$ language sql;
create procedure func_syssao_u1(a integer, b integer)
as $$select a + b $$ language sql;

alter procedure func_u1 owner to u1;
alter procedure func_syssso_u1 owner to syssso_u1;
alter procedure func_syssao_u1 owner to syssao_u1;

alter database testdb set se_feature.mode to on;
set se_feature.mode to on;

-- For member of sysdba_role
\c testdb sysdba_u1;
---- CREATE PROCEDURE
------ pass
create procedure public.func_sysdba_u1_f1(a integer, b integer)
as $$select a + b $$ language sql;
create procedure sch_sysdba_u1.func_sysdba_u1_f1(a integer, b integer)
as $$select a + b $$ language sql;
create procedure sch_u1.func_sysdba_u1_f1(a integer, b integer)
as $$select a + b $$ language sql;
create procedure sch_syssso_u2.func_sysdba_u1_f1(a integer, b integer)
as $$select a + b $$ language sql;
------ fail
create procedure sch_syssso_u1.func_sysdba_u1_f1(a integer, b integer)
as $$select a + b $$ language sql;
create procedure sch_syssao_u1.func_sysdba_u1_f1(a integer, b integer)
as $$select a + b $$ language sql;


---- ALTER PROCEDURE OWNER
------ pass
alter procedure func_sysdba_u1_f1  owner to sysdba_u2;
alter procedure func_sysdba_u1_f1 owner to u1;
alter procedure func_sysdba_u1_f1 owner to u2;
alter procedure func_sysdba_u1_f1 owner to sysdba_u1;
------ fail
alter procedure func_sysdba_u1_f1 owner to syssso_u1;
alter procedure func_sysdba_u1_f1 owner to syssao_u1;
alter procedure func_syssao_u1 owner to sysdba_u1;
alter procedure func_syssao_u1 owner to u1;

---- ALTER PROCEDURE RENAME
------ pass
alter procedure func_sysdba_u1_f1 rename to func_sysdba_u1_f1_new;
alter procedure func_sysdba_u1_f1_new owner to u1;
alter procedure func_sysdba_u1_f1_new rename to func_sysdba_u1_f1;
alter procedure func_sysdba_u1_f1 owner to sysdba_u1;
------ fail
alter procedure func_syssso_u1 rename to func_syssso_u1_new;
alter procedure func_syssao_u1 rename to func_syssao_u1_new;

---- ALTER PROCEDURE SCHEMA
------ pass
alter procedure func_sysdba_u1 set schema sch_sysdba_u1;
alter procedure sch_sysdba_u1.func_sysdba_u1 set schema sch_u1;
alter procedure sch_u1.func_sysdba_u1 set schema public;
------ fail
alter procedure func_sysdba_u1 set schema sch_syssso_u1;
alter procedure func_sysdba_u1 set schema sch_syssao_u1;
alter procedure func_syssso_u1 set schema sch_sysdba_u1;
alter procedure func_syssao_u1 set schema sch_u1;

---- ALTER PROCEDURE
------ pass
alter procedure func_sysdba_u1 security definer;
alter procedure func_u1 security definer;
------ fail
alter procedure func_syssso_u1 security definer;
alter procedure func_syssao_u1 security definer;

---- GRANT/REVOKE
------ pass
grant all on procedure func_sysdba_u1 to sysdba_u2;
grant all on procedure func_sysdba_u1 to u2;
grant all on procedure func_u1 to sysdba_u2;
grant all on procedure func_u1 to u2;
------ fail
grant all on procedure func_sysdba_u1 to syssso_u1;
grant all on procedure func_sysdba_u1 to syssao_u1;
grant all on procedure func_u1 to syssso_u1;
grant all on procedure func_u1 to syssao_u1;
grant all on procedure func_syssso_u1 to sysdba_u1;
grant all on procedure func_syssso_u1 to u1;
grant all on procedure func_syssao_u1 to sysdba_u1;
grant all on procedure func_syssao_u1 to u1;

---- CALL PROCEDURE
------ pass
call public.func_sysdba_u1_f1(1,1);
call sch_sysdba_u1.func_sysdba_u1_f1(1,1);
call sch_u1.func_sysdba_u1_f1(1,1);
call sch_syssso_u2.func_sysdba_u1_f1(1,1);
------ fail
call func_syssso_u1(1,1);
call func_syssao_u1(1,1);

---- DROP PROCEDURE
------ pass
drop procedure public.func_sysdba_u1_f1;
drop procedure sch_sysdba_u1.func_sysdba_u1_f1;
drop procedure sch_u1.func_sysdba_u1_f1;
drop procedure sch_syssso_u2.func_sysdba_u1_f1;
------ fail
drop procedure func_syssso_u1;
drop procedure func_syssao_u1;

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

drop procedure func_u1;
drop procedure func_sysdba_u1;
drop procedure func_syssso_u1;
drop procedure func_syssao_u1;

\c template1 sysdba_u1
drop database testdb;
