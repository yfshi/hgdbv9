-- test cases for set params

-- test #1 for alter system.

---- step_1 make sure we did not turn on se_feature
\c template1 sysdba_u1
---- should show off
select setting from pg_settings where name = 'se_feature.mode';
---- should show off
select setting from pg_settings where name = 'se_feature.restricted_dba';

---- step #2 set up instance-level params and reload, it should work
alter system set se_feature.mode to on;
alter system set se_feature.restricted_dba to on;
select pg_reload_conf();
\c template1 sysdba_u1
---- should show on
select setting from pg_settings where name = 'se_feature.mode';
---- should show on
select setting from pg_settings where name = 'se_feature.restricted_dba';

---- step #3 try to reset all, it should skip secruity params
alter system reset all;
select pg_reload_conf();
\c template1 sysdba_u1
---- should show on
select setting from pg_settings where name = 'se_feature.mode';
---- should show on
select setting from pg_settings where name = 'se_feature.restricted_dba';


---- step #4 sysdba try to reset directly, should fail;
alter system reset se_feature.mode;
alter system reset se_feature.restricted_dba;

-- test #1 clean up, shut instance-level param off
\c template1 syssso_u1
alter system reset se_feature.mode;
alter system reset se_feature.restricted_dba;
select pg_reload_conf();
\c template1 sysdba_u1
---- should show off
select setting from pg_settings where name = 'se_feature.mode';
---- should show off
select setting from pg_settings where name = 'se_feature.restricted_dba';


-- test #2 for alter database
create database testdb;
\c testdb sysdba_u1;

---- step #1 set up database level security params;
alter database testdb set se_feature.mode to on;
alter database testdb set se_feature.restricted_dba to on;
\c testdb sysdba_u1
---- should show on
select setting from pg_settings where name = 'se_feature.mode';
---- should show on
select setting from pg_settings where name = 'se_feature.restricted_dba';

---- step #2 try to reset security params directly;
---- should fail
alter database testdb reset se_feature.restricted_dba;
alter database testdb reset se_feature.mode;
---- should show on
select setting from pg_settings where name = 'se_feature.mode';
---- should show on
select setting from pg_settings where name = 'se_feature.restricted_dba';

---- step #3 reset all should pass, but it won't worked on security params
alter database testdb reset all;
\c testdb sysdba_u1
---- should show on
select setting from pg_settings where name = 'se_feature.mode';
---- should show on
select setting from pg_settings where name = 'se_feature.restricted_dba';

-- test #2 clean up
\c testdb syssso_u1
set se_feature.mode to on;
alter database testdb reset se_feature.mode;
alter database testdb reset se_feature.restricted_dba;

\c template1 sysdba_u1
drop database testdb;

-- test #3 for alter user
create database testdb;
create user test_normal_user;
\c testdb sysdba_u1;
alter database testdb set se_feature.mode to on;
alter database testdb set se_feature.restricted_dba to on;
alter user test_normal_user set se_feature.mode to on;
alter user test_normal_user set se_feature.restricted_dba to on;

---- step #1 set up user level security params
---- try to do it with sysdba_role
\c testdb sysdba_u1
---- should fail
alter user test_normal_user set se_feature.mode to off;
alter user test_normal_user set se_feature.restricted_dba to off;
---- try to do it with syssso_role
\c testdb syssso_u1
---- should fail
alter user test_normal_user set se_feature.mode to off;
alter user test_normal_user set se_feature.restricted_dba to off;

---- step #2 set up user-database level security params
---- try to do it with sysdba_role
\c testdb sysdba_u1
---- should fail
alter user test_normal_user in database testdb set se_feature.mode to off;
alter user test_normal_user in database testdb set se_feature.restricted_dba to off;
---- try to do it with sysdba_role
\c testdb syssso_u1
---- should fail
alter user test_normal_user in database testdb set se_feature.mode to off;
alter user test_normal_user in database testdb set se_feature.restricted_dba to off;

---- step #3 set up all-user database level security params, this should equal alter database
---- try to do it with sysdba_role
\c testdb sysdba_u1
---- should alter but blocked by checker
alter user all in database testdb set se_feature.mode to off;
alter user all in database testdb set se_feature.restricted_dba to off;
---- try to do it with sysdba_role
\c testdb syssso_u1
---- should pass
begin;
alter user all in database testdb set se_feature.mode to off;
---- should show sefeature.mode = off
select setconfig 
from pg_db_role_setting as s
join pg_database as d 
on d.oid = s.setdatabase
where d.datname = 'testdb';
alter user all in database testdb set se_feature.restricted_dba to off;
---- should show sefeature.restricted_dba = off
select setconfig 
from pg_db_role_setting as s
join pg_database as d 
on d.oid = s.setdatabase
where d.datname = 'testdb';
rollback;

---- step #4 user level security params reset;
---- try to do it with sysdba_role
\c testdb sysdba_u1
---- should fail
alter user test_normal_user reset se_feature.mode;
alter user test_normal_user reset se_feature.restricted_dba;
---- try to do it with syssso_role
\c testdb syssso_u1
---- should fail
alter user test_normal_user reset se_feature.mode;
alter user test_normal_user reset se_feature.restricted_dba;

---- step #5 user-database level security params reset
---- try to do it with sysdba_role
\c testdb sysdba_u1
---- should fail
alter user test_normal_user in database testdb reset se_feature.mode;
alter user test_normal_user in database testdb reset se_feature.restricted_dba;
---- try to do it with syssso_role
\c testdb syssso_u1
---- should fail
alter user test_normal_user in database testdb reset se_feature.mode;
alter user test_normal_user in database testdb reset se_feature.restricted_dba;

---- step #6 all user database level security params reset
---- try to do it with sysdba_role
\c testdb sysdba_u1
---- should fail
alter user all in database testdb reset se_feature.mode;
alter user all in database testdb reset se_feature.restricted_dba;
---- try to do it with syssso_role
\c testdb syssso_u1
---- should pass
begin;
alter user all in database testdb reset se_feature.mode;
---- should show without sefeature.mode
select setconfig 
from pg_db_role_setting as s
join pg_database as d 
on d.oid = s.setdatabase
where d.datname = 'testdb';
alter user all in database testdb reset se_feature.restricted_dba;
---- should show without sefeature.restricted_dba
select setconfig 
from pg_db_role_setting as s
join pg_database as d 
on d.oid = s.setdatabase
where d.datname = 'testdb';
rollback;

---- step #7 reset all user level security params
---- try to do it with sysdba_role
\c testdb sysdba_u1
---- should pass but it skip security params
alter user test_normal_user reset all;
select setconfig 
from pg_db_role_setting as s
where s.setrole = 'test_normal_user'::regrole;
---- try to do it with syssso_role
\c testdb syssso_u1
---- should fail
begin;
alter user test_normal_user reset all;
rollback;
select setconfig 
from pg_db_role_setting as s
where s.setrole = 'test_normal_user'::regrole;

---- step #8 reset all user-database level security params
---- try to do it with sysdba_role
\c testdb sysdba_u1
---- should pass but it skip security params
alter user test_normal_user in database testdb reset all;
select setconfig 
from pg_db_role_setting as s
where s.setrole = 'test_normal_user'::regrole;
---- try to do it with syssso_role
\c testdb syssso_u1
---- should fail
begin;
alter user test_normal_user in database testdb reset all;
rollback;
select setconfig 
from pg_db_role_setting as s
where s.setrole = 'test_normal_user'::regrole;


---- step #9 reset all database level security params
---- try to do it with sysdba_role
\c testdb sysdba_u1
---- should pass but it skip security params
alter user all in database testdb reset all;
select setconfig 
from pg_db_role_setting as s
join pg_database as d 
on d.oid = s.setdatabase
where d.datname = 'testdb';
---- try to do it with syssso_role
\c testdb syssso_u1
---- should fail
begin;
alter user all in database testdb reset all;
rollback;
select setconfig 
from pg_db_role_setting as s
join pg_database as d 
on d.oid = s.setdatabase
where d.datname = 'testdb';

-- test #3 clean up
\c testdb syssso_u1
set se_feature.mode to on;
alter database testdb reset se_feature.mode;
alter database testdb reset se_feature.restricted_dba;
\c template1 sysdba_u1
alter user test_normal_user reset se_feature.mode;
alter user test_normal_user reset se_feature.restricted_dba;
drop database testdb;
drop user test_normal_user;

-- test #4 test for session level security param
create database testdb;

---- step #1 try to do it with sysdba_role
\c testdb sysdba_u1;
---- should pass;
set se_feature.restricted_dba to on;
set se_feature.mode to on;
---- should fail;
set se_feature.mode to off;
set se_feature.restricted_dba to off;
------ it works but currently we can't do anything, becaure reset skip check_hook.
reset se_featurere.mode;
reset se_feature.restricted_dba;

---- step #2 check the dependency
\c testdb sysdba_u1;
---- should pass, but restricted_dba should depend on mode;
set se_feature.restricted_dba to on;
---- should show off;
select setting from pg_settings where name = 'se_feature.restricted_dba';
set se_feature.mode to on;
---- should show on;
select setting from pg_settings where name = 'se_feature.mode';
---- should show on;
select setting from pg_settings where name = 'se_feature.restricted_dba';

---- step #3 try to reset all with sysdba_role
\c testdb sysdba_u1;
set se_feature.restricted_dba to on;
set se_feature.mode to on;
---- should work buy skip security params;
reset all;
---- should show on;
select setting from pg_settings where name = 'se_feature.mode';
---- should show on;
select setting from pg_settings where name = 'se_feature.restricted_dba';

\c template1 sysdba_u1
drop database testdb;


create user show_user;

select * from hg_security.show_secure_param();
show se_feature.mode;

set role show_user;
select * from hg_security.show_secure_param();
show se_feature.mode;
reset role;

set role syssso;
select * from hg_security.show_secure_param();
show se_feature.mode;
reset role;


drop user show_user;