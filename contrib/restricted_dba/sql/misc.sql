-- bug fix and other odds and ends

-- setup before test start
create database testdb;
alter database testdb set se_feature.mode to on;
set se_feature.mode to on;

-- For member of sysdba_role
\c testdb sysdba_u1;

-- Should fail for resource group not exist
alter role u1 resource group rg_non_exist;

-- Clean test object
\c testdb syssso_u1
set se_feature.mode to on;
alter database testdb set se_feature.mode to off;
\c testdb sysdba_u1

\c template1 sysdba_u1
drop database testdb;

