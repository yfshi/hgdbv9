-- FUNCTION

-- setup before test start
create database testdb;
\c testdb -

create schema sch_u1 authorization u1;
create schema sch_sysdba_u1 authorization sysdba_u1;
create schema sch_syssso_u1 authorization syssso_u1;
create schema sch_syssao_u1 authorization syssao_u1;

create schema sch_syssso_u2 authorization syssso_u2;
grant create on schema sch_syssso_u2 to sysdba_role;

create function func_u1(a integer, b integer) returns integer
as 'select a + b' language sql;
create function func_sysdba_u1(a integer, b integer) returns integer
as 'select a + b' language sql;
create function func_syssso_u1(a integer, b integer) returns integer
as 'select a + b' language sql;
create function func_syssao_u1(a integer, b integer) returns integer
as 'select a + b' language sql;

alter function func_u1 owner to u1;
alter function func_syssso_u1 owner to syssso_u1;
alter function func_syssao_u1 owner to syssao_u1;

alter database testdb set se_feature.mode to on;
set se_feature.mode to on;

-- For member of sysdba_role
\c testdb sysdba_u1;
---- CREATE FUNCTION
------ pass
create function public.func_sysdba_u1_f1(a integer, b integer) returns integer
as 'select a + b' language sql;
create function sch_sysdba_u1.func_sysdba_u1_f1(a integer, b integer) returns integer
as 'select a + b' language sql;
create function sch_u1.func_sysdba_u1_f1(a integer, b integer) returns integer
as 'select a + b' language sql;
create function sch_syssso_u2.func_sysdba_u1_f1(a integer, b integer) returns integer
as 'select a + b' language sql;
------ fail
create function sch_syssso_u1.func_sysdba_u1_f1(a integer, b integer) returns integer
as 'select a + b' language sql;
create function sch_syssao_u1.func_sysdba_u1_f1(a integer, b integer) returns integer
as 'select a + b' language sql;


---- ALTER FUNCTION OWNER
------ pass
alter function func_sysdba_u1_f1  owner to sysdba_u2;
alter function func_sysdba_u1_f1 owner to u1;
alter function func_sysdba_u1_f1 owner to u2;
alter function func_sysdba_u1_f1 owner to sysdba_u1;
------ fail
alter function func_sysdba_u1_f1 owner to syssso_u1;
alter function func_sysdba_u1_f1 owner to syssao_u1;
alter function func_syssao_u1 owner to sysdba_u1;
alter function func_syssao_u1 owner to u1;

---- ALTER FUNCTION RENAME
------ pass
alter function func_sysdba_u1_f1 rename to func_sysdba_u1_f1_new;
alter function func_sysdba_u1_f1_new owner to u1;
alter function func_sysdba_u1_f1_new rename to func_sysdba_u1_f1;
alter function func_sysdba_u1_f1 owner to sysdba_u1;
------ fail
alter function func_syssso_u1 rename to func_syssso_u1_new;
alter function func_syssao_u1 rename to func_syssao_u1_new;

---- ALTER FUNCTION SCHEMA
------ pass
alter function func_sysdba_u1 set schema sch_sysdba_u1;
alter function sch_sysdba_u1.func_sysdba_u1 set schema sch_u1;
alter function sch_u1.func_sysdba_u1 set schema public;
------ fail
alter function func_sysdba_u1 set schema sch_syssso_u1;
alter function func_sysdba_u1 set schema sch_syssao_u1;
alter function func_syssso_u1 set schema sch_sysdba_u1;
alter function func_syssao_u1 set schema sch_u1;

---- ALTER FUNCTION
------ pass
alter function func_sysdba_u1 strict;
alter function func_u1 strict;
------ fail
alter function func_syssso_u1 strict;
alter function func_syssao_u1 strict;

---- GRANT/REVOKE
------ pass
grant all on function func_sysdba_u1 to sysdba_u2;
grant all on function func_sysdba_u1 to u2;
grant all on function func_u1 to sysdba_u2;
grant all on function func_u1 to u2;
------ fail
grant all on function func_sysdba_u1 to syssso_u1;
grant all on function func_sysdba_u1 to syssao_u1;
grant all on function func_u1 to syssso_u1;
grant all on function func_u1 to syssao_u1;
grant all on function func_syssso_u1 to sysdba_u1;
grant all on function func_syssso_u1 to u1;
grant all on function func_syssao_u1 to sysdba_u1;
grant all on function func_syssao_u1 to u1;

---- CALL FUNCTION
------ pass
select public.func_sysdba_u1_f1(1,1);
select sch_sysdba_u1.func_sysdba_u1_f1(1,1);
select sch_u1.func_sysdba_u1_f1(1,1);
select sch_syssso_u2.func_sysdba_u1_f1(1,1);
------ fail
select func_syssso_u1(1,1);
select func_syssao_u1(1,1);

---- DROP FUNCTION
------ pass
drop function public.func_sysdba_u1_f1;
drop function sch_sysdba_u1.func_sysdba_u1_f1;
drop function sch_u1.func_sysdba_u1_f1;
drop function sch_syssso_u2.func_sysdba_u1_f1;
drop function if exists func_not_exists;
------ fail
drop function func_syssso_u1;
drop function func_syssao_u1;
drop function func_not_exists;

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

drop function func_u1;
drop function func_sysdba_u1;
drop function func_syssso_u1;
drop function func_syssao_u1;

\c template1 sysdba_u1
drop database testdb;


-- set restricted_dba to on

-- setup before test start
create database testdb;
\c testdb -
create schema sch_u1 authorization u1;
create schema sch_sysdba_u1 authorization sysdba_u1;
create schema sch_syssso_u1 authorization syssso_u1;
create schema sch_syssao_u1 authorization syssao_u1;

create schema sch_u2 authorization u2;
create schema sch_syssso_u2 authorization syssso_u2;
create schema sch1_sysdba_u2 authorization sysdba_u2;
create schema sch2_sysdba_u2 authorization sysdba_u2;

grant create on schema sch_u2 to sysdba_role;
grant create on schema sch_syssso_u2 to sysdba_role;
grant create on schema sch1_sysdba_u2 to sysdba_role;

create function func_u1(a integer, b integer) returns integer
as 'select a + b' language sql;
create function sch_u1.func_u1(a integer, b integer) returns integer
as 'select a + b' language sql;
create function func_sysdba_u1(a integer, b integer) returns integer
as 'select a + b' language sql;
create function func_sysdba_u2(a integer, b integer) returns integer
as 'select a + b' language sql;
create function sch2_sysdba_u2.func_sysdba_u2(a integer, b integer) returns integer
as 'select a + b' language sql;
create function func_sysdba_u2_f1(a integer, b integer) returns integer
as 'select a + b' language sql;
create function func_syssso_u1(a integer, b integer) returns integer
as 'select a + b' language sql;
create function func_syssao_u1(a integer, b integer) returns integer
as 'select a + b' language sql;

alter function func_u1 owner to u1;
alter function sch_u1.func_u1 owner to u1;
alter function func_syssso_u1 owner to syssso_u1;
alter function func_syssao_u1 owner to syssao_u1;
alter function func_sysdba_u2 owner to sysdba_u2;
alter function sch2_sysdba_u2.func_sysdba_u2 owner to sysdba_u2;
alter function func_sysdba_u2_f1 owner to sysdba_u2;

grant execute on function func_sysdba_u2_f1 to sysdba_u1;

alter database testdb set se_feature.mode to on;
alter database testdb set se_feature.restricted_dba to on;
set se_feature.restricted_dba to on;
set se_feature.mode to on;

-- For member of sysdba_role
\c testdb sysdba_u1;
---- CREATE FUNCTION
------ pass
create function public.func_sysdba_u1_f1(a integer, b integer) returns integer
as 'select a + b' language sql;
create function sch_sysdba_u1.func_sysdba_u1_f1(a integer, b integer) returns integer
as 'select a + b' language sql;
create function sch_u2.func_sysdba_u1_f1(a integer, b integer) returns integer
as 'select a + b' language sql;
create function sch_syssso_u2.func_sysdba_u1_f1(a integer, b integer) returns integer
as 'select a + b' language sql;
create function sch1_sysdba_u2.func_sysdba_u1_f1(a integer, b integer) returns integer
as 'select a + b' language sql;
------ fail
create function sch_syssso_u1.func_sysdba_u1_f1(a integer, b integer) returns integer
as 'select a + b' language sql;
create function sch_syssao_u1.func_sysdba_u1_f1(a integer, b integer) returns integer
as 'select a + b' language sql;
create function sch2_sysdba_u2.func_sysdba_u1_f1(a integer, b integer) returns integer
as 'select a + b' language sql;
create function sch_u1.func_sysdba_u1_f1(a integer, b integer) returns integer
as 'select a + b' language sql;

---- ALTER FUNCTION OWNER
------ pass
begin;
alter function func_sysdba_u1 owner to sysdba_u2;
rollback;
------ fail
alter function func_sysdba_u1 owner to syssso_u1;
alter function func_sysdba_u1 owner to syssao_u1;
alter function func_sysdba_u1 owner to u1;
alter function func_sysdba_u2 owner to sysdba_u1;
alter function func_sysdba_u2 owner to syssso_u1;
alter function func_sysdba_u2 owner to syssao_u1;
alter function func_sysdba_u2 owner to u1;
alter function func_syssso_u1 owner to sysdba_u1;
alter function func_syssso_u1 owner to syssso_u2;
alter function func_syssso_u1 owner to syssao_u1;
alter function func_syssso_u1 owner to u1;
alter function func_syssao_u1 owner to sysdba_u1;
alter function func_syssao_u1 owner to syssso_u1;
alter function func_syssao_u1 owner to syssao_u2;
alter function func_syssao_u1 owner to u1;
alter function func_u1 owner to sysdba_u1;
alter function func_u1 owner to syssso_u1;
alter function func_u1 owner to syssao_u1;
alter function func_u1 owner to u2;

---- ALTER FUNCTION RENAME
begin;
alter function func_sysdba_u1_f1 rename to func_sysdba_u1_f1_new;
rollback;
------ fail
alter function func_sysdba_u2 rename to func_sysdba_u2_new;
alter function func_syssso_u1 rename to func_syssso_u1_new;
alter function func_syssao_u1 rename to func_syssao_u1_new;
alter function func_u1 rename to func_u1_new;


---- ALTER FUNCTION SCHEMA
------ pass
alter function func_sysdba_u1_f1 set schema sch_sysdba_u1;
alter function sch_sysdba_u1.func_sysdba_u1_f1 set schema sch_u2;
alter function sch_u2.func_sysdba_u1_f1 set schema sch_syssso_u2;
alter function sch_syssso_u2.func_sysdba_u1_f1 set schema sch1_sysdba_u2;
alter function sch1_sysdba_u2.func_sysdba_u1_f1 set schema public;
------ fail
alter function func_sysdba_u1 set schema sch_syssso_u1;
alter function func_sysdba_u1 set schema sch_syssao_u1;
alter function func_sysdba_u1 set schema sch2_sysdba_u2;
alter function func_sysdba_u1 set schema sch_u1;
alter function func_sysdba_u2 set schema sch_syssso_u1;
alter function func_sysdba_u2 set schema sch_syssao_u1;
alter function func_sysdba_u2 set schema sch_sysdba_u1;
alter function func_sysdba_u2 set schema sch_u1;
alter function func_syssso_u1 set schema sch_sysdba_u1;
alter function func_syssso_u1 set schema sch_syssso_u1;
alter function func_syssao_u1 set schema sch_syssao_u1;
alter function func_syssao_u1 set schema sch_u1;
alter function func_u1 set schema sch_syssso_u1;
alter function func_u1 set schema sch_syssao_u1;
alter function func_u1 set schema sch_sysdba_u1;
alter function func_u1 set schema sch_u1;

---- ALTER FUNCTION
------ pass
alter function func_sysdba_u1 strict;
------ fail
alter function func_sysdba_u2 strict;
alter function func_syssso_u1 strict;
alter function func_syssao_u1 strict;
alter function func_u1 strict;

---- GRANT/REVOKE
------ pass
grant all on function func_sysdba_u1 to sysdba_u2;
------ fail
grant all on function func_sysdba_u1 to syssso_u1;
grant all on function func_sysdba_u1 to syssao_u1;
grant all on function func_sysdba_u1 to u1;
grant all on function func_sysdba_u2 to sysdba_u1;
grant all on function func_sysdba_u2 to syssso_u1;
grant all on function func_sysdba_u2 to syssao_u1;
grant all on function func_sysdba_u2 to u1;
grant all on function func_syssso_u1 to sysdba_u1;
grant all on function func_syssso_u1 to syssao_u1;
grant all on function func_syssao_u1 to syssso_u1;
grant all on function func_syssao_u1 to u1;
grant all on function func_u1 to sysdba_u1;
grant all on function func_u1 to syssao_u1;
grant all on function func_u1 to syssso_u1;
grant all on function func_u1 to u2;

---- CALL FUNCTION
------ pass
select func_sysdba_u1_f1(1,1);
select func_sysdba_u2_f1(1,1);
select func_sysdba_u2(1,1);
select func_u1(1,1);
------ fail
select sch2_sysdba_u2.func_sysdba_u2(1,1);
select func_syssao_u1(1,1);
select func_syssso_u1(1,1);
select sch_u1.func_u1(1,1);

---- DROP FUNCTION
------ pass
drop function func_sysdba_u1_f1;
------ fail
drop function func_sysdba_u2;
drop function func_syssao_u1;
drop function func_syssso_u1;
drop function func_u1;

-- Clean test object
\c testdb syssso_u1
set se_feature.mode to on;
alter database testdb set se_feature.mode to off;
alter database testdb set se_feature.restricted_dba to off;

\c template1 sysdba_u1
drop database testdb;

---- bug 6155 
\c template1 sysdba_u1
create database testdb;
\c testdb sysdba_u1
create schema sch_sysdba_u1;
create function sch_sysdba_u1.func_sysdba_u2(a integer, b integer) returns integer
as 'select a + b' language sql security definer;
create function public.func_syssso(a integer, b integer) returns integer
as 'select a + b' language sql security definer;
create function public.func_syssao(a integer, b integer) returns integer
as 'select a + b' language sql security definer;
create function sch_sysdba_u1.func_u1(a integer, b integer) returns integer
as 'select a + b' language sql security definer;

alter function sch_sysdba_u1.func_sysdba_u2(a integer, b integer) owner to sysdba_u2;
alter function public.func_syssso(a integer, b integer) owner to syssso;
alter function public.func_syssao(a integer, b integer) owner to syssao;
alter function sch_sysdba_u1.func_u1(a integer, b integer) owner to u1;

\c testdb sysdba_u1
set se_feature.mode to on;
------ should pass
select sch_sysdba_u1.func_sysdba_u2(1,1);
select sch_sysdba_u1.func_u1(1,1);
------ should fail
select func_syssso(1,1);
select func_syssao(1,1);
reset se_feature.mode;

---- restricted_dba is on;
set se_feature.restricted_dba to on;
set se_feature.mode to on;
------ should fail
select func_syssso(1,1);
select func_syssao(1,1);
select sch_sysdba_u1.func_sysdba_u2(1,1);
select sch_sysdba_u1.func_u1(1,1);
reset se_feature.mode;
reset se_feature.restricted_dba;

---- clean
drop function sch_sysdba_u1.func_sysdba_u2(a integer, b integer);
drop function public.func_syssso(a integer, b integer);
drop function public.func_syssao(a integer, b integer);
drop function sch_sysdba_u1.func_u1(a integer, b integer);
drop schema sch_sysdba_u1;
\c template1 sysdba_u1
drop database testdb;