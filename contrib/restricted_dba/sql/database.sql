-- DATABASE

set se_feature.mode to on;
---- CREATE DATABASE
------ fail
create database db1 owner u1;
------ pass
create database db1 owner sysdba_u1;

---- ALTER DATABASE
------ fail
alter database db1 owner to u1;
alter database db_non_exist owner to sysdba_u1;
alter database db_non_exist rename to db_1;
------ pass
alter database db1 owner to sysdba_u2;
alter database db1 rename to db1_new;
alter database db1_new rename to db1;

---- GRANT/REVOKE
------ pass
grant all on database db1 to sysdba_u1;
grant all on database db1 to u1;
grant connect on database db1 to syssso_u1;
------ fail
grant all on database db1 to u1 with grant option;
grant create on database db1 to syssso_u1 with grant option;
grant create on database db1 to syssso_u1;
grant all on database db1 to syssao_u1;
grant create on database db_non_exist to sysdba_u1;

---- DROP DATABASE
------ pass
drop database db1;
------ pass
drop database db_non_exist;
