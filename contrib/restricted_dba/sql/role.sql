-- ROLE/USER 

create user sysdba_u1 superuser createdb createrole;
create user sysdba_u2 superuser createdb createrole;
create user syssso_u1;
create user syssso_u2;
create user syssao_u1;
create user syssao_u2;
create user u1;
create user u2;
grant sysdba_role to sysdba_u1,sysdba_u2;
grant syssso_role to syssso_u1,syssso_u2;
grant syssao_role to syssao_u1,syssao_u2;

create user dba_u1 superuser createdb createrole;
create user dba_u2 superuser createdb createrole;
create user sso_u1;
create user sso_u2;
create user sao_u1;
create user sao_u2;

grant sysdba_role to dba_u1 with admin option;
grant sysdba_role to dba_u2;
grant syssso_role to sso_u1 with admin option;
grant syssso_role to sso_u2;
grant syssao_role to sao_u1 with admin option;
grant syssso_role to sao_u2;

create database testdb;
alter database testdb set se_feature.mode to on;

-- For member of sysdba_role
\c testdb dba_u1
---- CREATE TABLE
------ pass
create role n_r1;
create user n_user_1 with inherit login replication bypassrls;
create user n_user_2 with noinherit nologin noreplication nobypassrls;
create user n_user_3 with connection limit 10;
create user n_user_4 with in role n_r1;
create user n_user_5 with in group n_r1;
create user n_user_7 with admin n_r1;
create user n_user_8 with role n_r1;
------ fail
------ cannot with superuser createdb createrole
create user n_u1 with superuser;
create user n_u1 with createrole;
create user n_u1 with createdb;
------cannot be member of security roles when create;
create user n_u1 with in role sysdba_role;
create user n_u1 with in role dba_u1;
create user n_u1 with in role syssso_role;
create user n_u1 with in role sso_u1;
create user n_u1 with in role syssso_role;
create user n_u1 with in role sao_u1;
create user n_u1 with in group sysdba_role;
create user n_u1 with in group dba_u1;
create user n_u1 with in group syssso_role;
create user n_u1 with in group sso_u1;
create user n_u1 with in group syssso_role;
create user n_u1 with in group sao_u1;
------ cannot have member of other security roles member;
create user n_u1 with role sysdba_role;
create user n_u1 with role dba_u1;
create user n_u1 with role syssso_role;
create user n_u1 with role sso_u1;
create user n_u1 with role syssso_role;
create user n_u1 with role sao_u1;
create user n_u1 with admin sysdba_role;
create user n_u1 with admin dba_u1;
create user n_u1 with admin syssso_role;
create user n_u1 with admin sso_u1;
create user n_u1 with admin syssso_role;
create user n_u1 with admin sao_u1;

---- ALTER USER
------ pass
------ alter self pass
alter user dba_u1 with password '123456';
------ alter normal user
alter user n_user_1 with noinherit nologin noreplication nobypassrls;
alter user n_user_1 with inherit login replication bypassrls;
alter user n_user_1 with connection limit 11;
------ fail
------ alter sysdba_role member
alter user dba_u1 with password '123456' valid until '2077-01-01';
alter user dba_u1 with inherit login replication bypassrls;
alter user dba_u1 with noinherit nologin noreplication nobypassrls;
alter user dba_u1 with connection limit 11;
alter user dba_u1 with superuser createrole createdb;
alter user dba_u1 with nosuperuser nocreaterole nocreatedb;
alter user dba_u2 with password '123456' valid until '2077-01-01';
------ alter syssso_role member
alter user sso_u1 with inherit login replication bypassrls;
alter user sso_u1 with noinherit nologin noreplication nobypassrls;
alter user sso_u1 with connection limit 11;
alter user sso_u1 with superuser createrole createdb;
alter user sso_u1 with nosuperuser nocreaterole nocreatedb;
alter user sso_u1 with password '123456' valid until '2077-01-01';
------ alter syssao_role member
alter user sao_u1 with inherit login replication bypassrls;
alter user sao_u1 with noinherit nologin noreplication nobypassrls;
alter user sao_u1 with connection limit 11;
alter user sao_u1 with superuser createrole createdb;
alter user sao_u1 with nosuperuser nocreaterole nocreatedb;
alter user sao_u1 with password '123456' valid until '2077-01-01';
------ alter normal user
alter user n_user_1 with superuser;
alter user n_user_1 with createrole;
alter user n_user_1 with createdb;
alter user n_user_1 with password '123456';
alter user n_user_1 with password '123456' valid until '2077-01-01';

\c - syssso_u1
------ pass
alter user n_user_3 with password '123456';
alter user n_user_3 with valid until '2077-01-01';
------ fail
alter user n_user_3 with superuser;
alter user n_user_3 with createrole;
alter user n_user_3 with createdb;
\c - dba_u1

---- GRANT ROLE
------ pass
grant sysdba_role to n_user_1;
revoke sysdba_role from n_user_1;
------ fail
grant n_r1 to dba_u2;
grant dba_u1 to n_user_2;
grant syssso_role to n_user_2;
grant syssao_role to n_user_2;

---- DROP USER
------ pass
drop user n_user_1;
drop user n_user_2;
drop user n_user_3;
drop user n_user_4;
drop user n_user_5;
drop user n_user_7;
drop user n_user_8;
drop role n_r1;
------ fail
drop user dba_u1;
drop user dba_u2;
drop user sso_u1;
drop user sso_u2;
drop user sao_u1;
drop user sao_u2;


---- SET ROLE
------ pass
\c testdb sysdba_u1;
set role dba_u1;
------ fail
set role sso_u1;
set role sao_u1;

\c testdb syssso_u1;
------ fail
set role dba_u1;
set role sso_u1;
set role sao_u1;

\c testdb syssao_u1;
------ fail
set role dba_u1;
set role sso_u1;
set role sao_u1;

---- SET SESSION AUTHORIZATION
------ pass
\c testdb sysdba_u1;
set session authorization dba_u1;
------ fail
set session authorization sso_u1;
set session authorization sao_u1;

\c testdb syssso_u1;
------ fail
set session authorization dba_u1;
set session authorization sso_u1;
set session authorization sao_u1;

\c testdb syssao_u1;
------ fail
set session authorization dba_u1;
set session authorization sso_u1;
set session authorization sao_u1;


---- bug 6277
\c testdb sysdba_u1
create role n_r2;
create role n_u2 in role n_r2;
------ all grant should fail1
\c testdb syssso
grant syssso_role to n_r2;
grant syssso_role to n_u2;
\c testdb syssao
grant syssao_role to n_r2;
grant syssao_role to n_u2;
\c testdb sysdba_u1
drop role n_u2;
drop role n_r2;

---- bug 6276
\c testdb sysdba_u1
create role n_r2;
create role n_u2;
------ all grant should fail1
\c testdb syssso
grant n_r2 to n_u2;
\c testdb syssao
grant n_r2 to n_u2;
\c testdb sysdba_u1
drop role n_u2;
drop role n_r2;

---- bug 6273
\c template1 sysdba_u1
create user super_u1 with superuser;
create user cr_u1 with createrole;
------ should pass
\c testdb super_u1
alter user super_u1 with password '123456';
alter user super_u1 set work_mem = '6 MB';
alter user super_u1 reset work_mem;
------ should fail
\c testdb cr_u1
alter user super_u1 with inherit;
alter user super_u1 with createdb;
alter user super_u1 with createrole;
alter user super_u1 with connection limit 10;
alter user super_u1 with valid until '20300101';
alter user super_u1 set work_mem = '6 MB';
alter user super_u1 reset work_mem;
alter user super_u1 rename to super_u2;
drop user super_u1;
\c testdb sysdba_u1
drop user cr_u1;;
drop user super_u1;

-- Clean test object
\c testdb syssso_u1
set se_feature.mode to on;
alter database testdb set se_feature.mode to off;
\c testdb sysdba_u1
drop user dba_u1,dba_u2;
drop user sso_u1,sso_u2;
drop user sao_u1,sao_u2;

\c template1 sysdba_u1
drop database testdb;

---- bug z6267
------ all should fail
alter role sysdba_role with login;
alter role syssso_role with login;
alter role syssao_role with login;
alter role sysdba_role rename to sysdba_role_new;
alter role syssso_role rename to syssso_role_new;
alter role syssao_role rename to syssao_role_new;
drop role sysdba_role;
drop role syssso_role;
drop role syssao_role;