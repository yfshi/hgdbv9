-- TABELSPACE

create database testdb;
\c testdb -
alter database testdb set se_feature.mode to on;
set se_feature.mode to on;

-- FOR sysdba_role
\c testdb sysdba_u1;
---- CREATE TABLESPACE
------ fail the location is invalid, but it can do create
create tablespace test_tablespace location '/a_test_tablespace/not_exist';
------ fail
create tablespace test_tablespace owner u1  location '/a_test_tablespace/not_exist';
---- ALTER TABLESPACE
------ fail
alter tablespace pg_default owner to u1;
alter tablespace tbspc_non_exist owner to sysdba_u1;
alter tablespace tbspc_non_exist rename to new_tbspc;
------ pass
alter tablespace pg_default owner to sysdba_u1;
select spcowner = 'sysdba_u1'::regrole as is_owner from pg_tablespace where spcname = 'pg_default';

---- DROP TABLESPACE
------ fail, because you cannot drop pg_default
drop tablespace pg_default;
drop tablespace tbspc_non_exist;

---- GRANT/REVOKE
------ pass
grant all on tablespace pg_default to sysdba_u2;
grant all on tablespace pg_default to u2;
revoke all on tablespace pg_default from u2;

------ fail
grant all on tablespace pg_default to syssso_u1;
grant all on tablespace pg_default to syssao_u1;
grant create on tablespace pg_default to syssso_u1;
grant create on tablespace pg_default to syssao_u1;
grant all on tablespace tbspc_non_exist to syssso_u1;
grant create on tablespace tbspc_non_exist to syssao_u1;
grant all on tablespace pg_default to u2 with grant option;

-- Clean test object
\c template1 sysdba_u1
drop database testdb
