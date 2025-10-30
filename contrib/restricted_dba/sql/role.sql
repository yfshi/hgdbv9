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
