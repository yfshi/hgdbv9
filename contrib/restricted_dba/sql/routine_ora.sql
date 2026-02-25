-- ROUTINE

-- setup before test start
create database testdb;
\c testdb -

create schema sch_u1 authorization u1;
create schema sch_sysdba_u1 authorization sysdba_u1;
create schema sch_syssso_u1 authorization syssso_u1;
create schema sch_syssao_u1 authorization syssao_u1;

create function func_u1(a integer, b integer) returns integer
as 'select a + b' language sql;
/
create function func_sysdba_u1(a integer, b integer) returns integer
as 'select a + b' language sql;
/
create function func_syssso_u1(a integer, b integer) returns integer
as 'select a + b' language sql;
/
create function func_syssao_u1(a integer, b integer) returns integer
as 'select a + b' language sql;
/

alter function func_u1 owner to u1;
alter function func_syssso_u1 owner to syssso_u1;
alter function func_syssao_u1 owner to syssao_u1;

alter database testdb set se_feature.mode to on;
set se_feature.mode to on;

-- For member of sysdba_role
\c testdb sysdba_u1;
create function public.func_sysdba_u1_f1(a integer, b integer) returns integer
as 'select a + b' language sql;
/
create function sch_sysdba_u1.func_sysdba_u1_f1(a integer, b integer) returns integer
as 'select a + b' language sql;
/
create function sch_u1.func_sysdba_u1_f1(a integer, b integer) returns integer
as 'select a + b' language sql;
/

---- ALTER ROUTINE OWNER
------ pass
alter routine func_sysdba_u1_f1  owner to sysdba_u2;
alter routine func_sysdba_u1_f1 owner to u1;
alter routine func_sysdba_u1_f1 owner to u2;
alter routine func_sysdba_u1_f1 owner to sysdba_u1;
------ fail
alter routine func_sysdba_u1_f1 owner to syssso_u1;
alter routine func_sysdba_u1_f1 owner to syssao_u1;
alter routine func_syssao_u1 owner to sysdba_u1;
alter routine func_syssao_u1 owner to u1;

---- ALTER ROUTINE RENAME
------ pass
alter routine func_sysdba_u1_f1 rename to func_sysdba_u1_f1_new;
alter routine func_sysdba_u1_f1_new owner to u1;
alter routine func_sysdba_u1_f1_new rename to func_sysdba_u1_f1;
alter routine func_sysdba_u1_f1 owner to sysdba_u1;
------ fail
alter routine func_syssso_u1 rename to func_syssso_u1_new;
alter routine func_syssao_u1 rename to func_syssao_u1_new;

---- ALTER ROUTINE SCHEMA
------ pass
alter routine func_sysdba_u1 set schema sch_sysdba_u1;
alter routine sch_sysdba_u1.func_sysdba_u1 set schema sch_u1;
alter routine sch_u1.func_sysdba_u1 set schema public;
------ fail
alter routine func_sysdba_u1 set schema sch_syssso_u1;
alter routine func_sysdba_u1 set schema sch_syssao_u1;
alter routine func_syssso_u1 set schema sch_sysdba_u1;
alter routine func_syssao_u1 set schema sch_u1;

---- ALTER ROUTINE
------ pass
alter routine func_sysdba_u1 strict;
alter routine func_u1 strict;
------ fail
alter routine func_syssso_u1 strict;
alter routine func_syssao_u1 strict;

---- GRANT/REVOKE
------ pass
grant all on routine func_sysdba_u1 to sysdba_u2;
grant all on routine func_sysdba_u1 to u2;
grant all on routine func_u1 to sysdba_u2;
grant all on routine func_u1 to u2;
------ fail
grant all on routine func_sysdba_u1 to syssso_u1;
grant all on routine func_sysdba_u1 to syssao_u1;
grant all on routine func_u1 to syssso_u1;
grant all on routine func_u1 to syssao_u1;
grant all on routine func_syssso_u1 to sysdba_u1;
grant all on routine func_syssso_u1 to u1;
grant all on routine func_syssao_u1 to sysdba_u1;
grant all on routine func_syssao_u1 to u1;

---- DROP ROUTINE
------ pass
drop routine public.func_sysdba_u1_f1;
drop routine sch_sysdba_u1.func_sysdba_u1_f1;
drop routine sch_u1.func_sysdba_u1_f1;
drop routine if exists routine_not_exists;
------ fail
drop routine func_syssso_u1;
drop routine func_syssao_u1;
drop routine routine_not_exists;

-- Clean test object
\c testdb syssso_u1
set se_feature.mode to on;
alter database testdb set se_feature.mode to off;
\c testdb sysdba_u1
drop schema sch_syssso_u1;
drop schema sch_syssao_u1;
drop schema sch_sysdba_u1;
drop schema sch_u1;

drop routine func_u1;
drop routine func_sysdba_u1;
drop routine func_syssso_u1;
drop routine func_syssao_u1;

\c template1 sysdba_u1
drop database testdb;
