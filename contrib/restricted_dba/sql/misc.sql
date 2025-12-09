-- bug fix and other odds and ends

-- setup before test start
create database testdb;
alter database testdb set se_feature.mode to on;
set se_feature.mode to on;

-- For member of sysdba_role
\c testdb sysdba_u1;

-- Clean test object
\c testdb syssso_u1
set se_feature.mode to on;
alter database testdb set se_feature.mode to off;
\c testdb sysdba_u1

\c template1 sysdba_u1
drop database testdb;

--extra check for bug fix 4737 funciton execution;
create database testdb;
\c testdb sysdba_u1

---- setup tables
create table tb1_syssso_u1(c1 int);
create table tb1_syssso_u2(c1 int);
alter table tb1_syssso_u1 owner to syssso_u1;
alter table tb1_syssso_u2 owner to syssso_u2;

---- setup function
create function test_func_1_sso_u1()
returns bigint as $$
declare
	sum1 bigint;
	sum2 bigint;
Begin
	select sum(c1) into sum1 from tb1_syssso_u1;
	select sum(c1) into sum2 from tb1_syssso_u2;

	return sum1 + sum2;
end;
$$ language plpgsql;

create function test_func_2_sso_u1()
returns bigint as $$
declare
	sum1 bigint;
	sum2 bigint;
Begin
	select sum(c1) into sum1 from tb1_syssso_u1;
	select sum(c1) into sum2 from tb1_syssso_u2;

	return sum1 + sum2;
end;
$$ language plpgsql security definer;

alter function test_func_1_sso_u1 owner to syssso;
alter function test_func_2_sso_u1 owner to syssso;

---- test start
------ should pass
set role sysdba_u1;
select test_func_1_sso_u1();
------ should fail for permission denied for tb1_syssso_u2
set role syssso_u1;
select test_func_1_sso_u1();
------should fail for permission denied for tb1_syssso_u1
set role syssso_u2;
select test_func_1_sso_u1();

---- set se_feature.mode to on;
set role sysdba_u1;
alter database testdb set se_feature.mode to on;
set se_feature.mode to on;
------ should fail for permission denied for test_func_1_sso_u1
select test_func_1_sso_u1();
------ owner should fail for permmission denied of table
\c - syssso;
select test_func_1_sso_u1();

-- Clean test object
\c testdb syssso_u1
set se_feature.mode to on;
alter database testdb set se_feature.mode to off;

\c template1 sysdba_u1
drop database testdb;

-- test for alter system
create database testdb;
alter database testdb set se_feature.mode to on;
set se_feature.mode to on;

---- test start
---- pass
\c template1 sysdba_u1
drop database testdb;

-- test for implict sysdba priv
create database testdb;

\c testdb sysdba_u1
create user test_dba_u1;
create role test_r1;
create user test_u1;
---- pass
grant sysdba_role to test_dba_u1;
select rolsuper,rolcreaterole,rolcreatedb from pg_authid where rolname = 'test_dba_u1';
revoke sysdba_role from test_dba_u1;
select rolsuper,rolcreaterole,rolcreatedb from pg_authid where rolname = 'test_dba_u1';

-- \c testdb syssso_u1;
-- ---- fail
-- grant test_r1 to test_u1;
-- ---- pass
-- set se_feature.mode to on;
-- grant test_r1 to test_u1;
-- revoke test_r1 from test_u1;
-- set se_feature.mode to off;

\c template1 sysdba_u1
drop user test_dba_u1;
drop user test_u1;
drop role test_r1;
drop database testdb;

-- test for bug z6059 should show true
select extnamespace = 'hg_security'::regnamespace as result from pg_extension where extname = 'restricted_dba';

-- bug z6352
\c template1 sysdba_u1
create user test_u1;
create database testdb;
\c testdb syssso_u1
alter database testdb set se_feature.mode to on;
---- should fail for resource group not exist
\c testdb sysdba_u1;
create user test_u2 resource group rgroup1;
alter user test_u1 resource group rgroup1;
---- should fail
\c testdb syssso_u1
create user test_u1 resource group rgroup1;
alter user test_u1 resource group rgroup1;
---- should fail
\c testdb syssao_u1
create user test_u1 resource group rgroup1;
alter user test_u1 resource group rgroup1;
---- should fail
\c testdb test_u1
create user test_u1 resource group rgroup1;
alter user test_u1 resource group rgroup1;
\c testdb syssso_u1
alter database testdb reset se_feature.mode;
\c template1 sysdba_u1
drop database testdb;
drop user test_u1;