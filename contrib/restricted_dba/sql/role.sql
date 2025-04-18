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

create user dba_u1;
create user dba_u2;
create user sso_u1;
create user sso_u2;
create user sao_u1;
create user sao_u2;

create database testdb;
alter database testdb set se_feature.mode to on;

-- For member of sysdba_role
\c testdb sysdba_u1
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
    -- cannot with superuser createdb createrole
create user n_u1 with superuser;
create user n_u1 with createrole;
create user n_u1 with createdb;
    --cannot be member of security roles when create;
create user n_u1 with in role sysdba_role;
create user n_u1 with in role sysdba_u1;
create user n_u1 with in role syssso_role;
create user n_u1 with in role syssso_u1;
create user n_u1 with in role syssso_role;
create user n_u1 with in role syssao_u1;
create user n_u1 with in group sysdba_role;
create user n_u1 with in group sysdba_u1;
create user n_u1 with in group syssso_role;
create user n_u1 with in group syssso_u1;
create user n_u1 with in group syssso_role;
create user n_u1 with in group syssao_u1;
    -- cannot have member of other security roles member;
create user n_u1 with role sysdba_role;
create user n_u1 with role sysdba_u1;
create user n_u1 with role syssso_role;
create user n_u1 with role syssso_u1;
create user n_u1 with role syssso_role;
create user n_u1 with role syssao_u1;
create user n_u1 with admin sysdba_role;
create user n_u1 with admin sysdba_u1;
create user n_u1 with admin syssso_role;
create user n_u1 with admin syssso_u1;
create user n_u1 with admin syssso_role;
create user n_u1 with admin syssao_u1;


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
