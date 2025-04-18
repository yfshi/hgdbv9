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
------ pass
alter database db1 owner to sysdba_u2;

---- GRANT/REVOKE
------ pass
grant all on database db1 to sysdba_u1;
grant all on database db1 to u1;
grant connect on database db1 to syssso_u1;
------ fail
grant all on database db1 to u1 with grant option;
grant create on database db1 to syssso_u1 with grant option;
grant all on database db1 to syssao_u1;

---- DROP DATABASE
------ pass
drop database db1;
