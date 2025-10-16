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

--extra check for bug fix 4737 funciton execution;
create database testdb;

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
set role syssso;
select test_func_1_sso_u1();

-- Clean test object
\c testdb sysdba_u1
set se_feature.mode to on;
alter database testdb set se_feature.mode to off;

\c template1 sysdba_u1
drop database testdb;