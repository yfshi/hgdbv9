ALTER SYSTEM SET hg_rolstatus.enable TO ON;
SELECT PG_RELOAD_CONF();
select pg_sleep(2);
--==============================================================================================--
-- test rolstatus, enable/disable role. 
--     test user acl
--==============================================================================================--
create role role1;
create user user2; 

grant role1 to user2;
select rolname,rolstatus from pg_role_status;

------------------- table -------------------------------
create table tb1(id int);
insert into tb1 values(111);

grant select on table tb1 to role1;
-- ok
set role user2;
select * from tb1;
reset role;

-- disable role1
select hg_security.set_role_status('role1', false);
select rolname,rolstatus from pg_role_status;
select hg_security.get_role_status('role1');
-- fail
set role user2;
select * from tb1;
reset role;
-- enable role1
select hg_security.set_role_status('role1', true);
select rolname,rolstatus from pg_role_status;
select hg_security.get_role_status('role1');
-- enable/disable not exist role
select hg_security.set_role_status('role12', true);
select hg_security.set_role_status('role12', false);
select hg_security.get_role_status('role12');
-- ok
set role user2;
select * from tb1;
reset role;

-- repeat set
select hg_security.set_role_status('role1', false);
select hg_security.set_role_status('role1', false);
select hg_security.set_role_status('role1', false);
select hg_security.set_role_status('role1', true);
select hg_security.set_role_status('role1', true);
select hg_security.set_role_status('role1', true);

drop table tb1;
------------------- schema -------------------------------
create schema sh1;
grant all on schema sh1 to role1;
-- ok
set role user2;
create table sh1.id1(id int);
reset role;
-- disable role1
select hg_security.set_role_status('role1', false);
select rolname,rolstatus from pg_role_status;
-- fail
set role user2;
create table sh1.id2(id int);
reset role;
-- enable role1
select hg_security.set_role_status('role1', true);
select rolname,rolstatus from pg_role_status;
-- ok
set role user2;
create table sh1.id3(id int);
reset role;

drop table sh1.id1;
drop table sh1.id3;
drop schema sh1;

drop user user2;
drop role role1;

--==============================================================================================--
-- test rolstatus, enable/disable role. 
--     test role grant
--==============================================================================================--
create role role1;
create user user2; 
create user user3;

grant role1 to user2;
-- disable role1
select hg_security.set_role_status('role1', false);
grant role1 to user3;
revoke role1 from user2;

select hg_security.set_role_status('role1', true);
grant role1 to user3;
revoke role1 from user2;

drop user user2;
drop user user3;
drop role role1;
--==============================================================================================--
-- test rolstatus, enable/disable role. 
--     test function peri
--==============================================================================================--
create role role1;
create user user2; 
create table tb1(id int);
insert into tb1 values(111);

grant role1 to user2;
grant select on table tb1 to role1;

set role user2;
select hg_security.set_role_status('role1', false);
select * from tb1;
select hg_security.set_role_status('role1', true);
select * from tb1;
reset role;
select rolname,rolstatus from pg_role_status;

drop table tb1;
drop user user2;
drop role role1;

--==============================================================================================--
-- test rolstatus, enable/disable role. 
--     test guc
--==============================================================================================--
ALTER SYSTEM SET hg_rolstatus.enable TO OFF;
SELECT PG_RELOAD_CONF();
select pg_sleep(2);
create role role1;
create user user2; 

grant role1 to user2;
select rolname,rolstatus from pg_role_status;

------------------- table -------------------------------
create table tb1(id int);
insert into tb1 values(111);

grant select on table tb1 to role1;
-- ok
set role user2;
select * from tb1;
reset role;
-- disable role1
select hg_security.set_role_status('role1', false);
select rolname,rolstatus from pg_role_status;
-- ok
set role user2;
select * from tb1;
reset role;
-- enable role1
select hg_security.set_role_status('role1', true);
select rolname,rolstatus from pg_role_status;
-- ok
set role user2;
select * from tb1;
reset role;

-- enable/disable not exist role
select hg_security.set_role_status('role12', true);
select hg_security.set_role_status('role12', false);
select hg_security.get_role_status('role12');

-- repeat set
select hg_security.set_role_status('role1', false);
select hg_security.set_role_status('role1', false);
select hg_security.set_role_status('role1', false);
select hg_security.set_role_status('role1', true);
select hg_security.set_role_status('role1', true);
select hg_security.set_role_status('role1', true);

drop table tb1;
drop user user2;
drop role role1;

ALTER SYSTEM SET hg_rolstatus.enable TO ON;
SELECT PG_RELOAD_CONF();
select pg_sleep(2);
--==============================================================================================--
-- test rolstatus, enable/disable role. 
--     test set user
--==============================================================================================--
create user user1; 
create user user2; 
grant user1 to user2;
create table tb1(id int);
insert into tb1 values(111);

grant select on table tb1 to user1;
-- ok
set role user2;
select * from tb1;
reset role;
-- disable user1
select hg_security.set_role_status('user1', false);
select hg_security.get_role_status('user1');
select rolname,rolstatus from pg_role_status;
-- ok
set role user2;
select * from tb1;
reset role;
-- enable user1
select hg_security.set_role_status('user1', true);
select rolname,rolstatus from pg_role_status;
-- ok
set role user2;
select * from tb1;
reset role;

drop table tb1;
drop user user2;
drop role user1;
----------------------------------------------------
create user user1; 
create user user2; 
create table tb1(id int);
insert into tb1 values(111);

grant select on table tb1 to user1;
-- ok
set role user1;
select * from tb1;
reset role;
-- disable user1
select hg_security.set_role_status('user1', false);
select rolname,rolstatus from pg_role_status;
-- ok
grant user1 to user2;
set role user1;
select * from tb1;
reset role;
-- enable user1
select hg_security.set_role_status('user1', true);
select rolname,rolstatus from pg_role_status;
-- ok
set role user1;
select * from tb1;
reset role;

drop table tb1;
drop role user1;
drop user user2;

--==============================================================================================--
-- test rolstatus, enable/disable role. 
--     test set sso sao dba user
--==============================================================================================--
select hg_security.set_role_status('sysdba_role', false);
select hg_security.set_role_status('syssso_role', false);
select hg_security.set_role_status('syssao_role', false);
select hg_security.get_role_status('sysdba_role');
select hg_security.get_role_status('syssso_role');
select hg_security.get_role_status('syssao_role');

--==============================================================================================--
-- test seesion
--==============================================================================================--
create role role1;
create user user1;
grant role1 to user1;

create table tb1(id int, info text);
insert into tb1 values(1,'1111');
grant select on table tb1 to role1;
grant insert on table tb1 to user1;

set role user1;
insert into tb1 values(2,'2222');
update tb1 set id=22 where id=2;
select * from tb1;
reset role;
select hg_security.set_role_status('role1',False);

set role user1;
insert into tb1 values(3,'3333');
update tb1 set id=22 where id=2;
select * from tb1;
reset role;
-- test database
select hg_security.set_role_status('role1',true);

drop database db1;
drop table tb1;
drop role role1;
drop role user1;

--==============================================================================================--
-- test view
--==============================================================================================--
create role test_role1;
create role test_role2;
create role test_role3;
create role test_role4;
create role test_role5;
create role test_role6;
create role test_role7;

select rolname, rolstatus from hg_security.role_status where rolname like '%test_role%' order by rolname;
select hg_security.set_role_status('test_role1',false);
select hg_security.set_role_status('test_role2',true);
select hg_security.set_role_status('test_role3',false);
select hg_security.set_role_status('test_role4',true);
select hg_security.set_role_status('test_role5',false);
select rolname, rolstatus from hg_security.role_status where rolname like '%test_role%' order by rolname;
drop role test_role1;
drop role test_role2;
drop role test_role3;
drop role test_role4;
drop role test_role5;
drop role test_role6;
drop role test_role7;
select rolname, rolstatus from hg_security.role_status where rolname like '%test_role%' order by rolname;

--==============================================================================================--
-- test se_feature.mode =on
--==============================================================================================--
create role role1;
create user user1;
create user user2;
grant syssso_role to user1; 

-- test se_feature.mode =off
set role syssso;
select hg_security.set_role_status('role1', false);
select hg_security.set_role_status('role1', true);
select hg_security.get_role_status('role1');
alter system set hg_rolstatus.enable=off;
alter system set hg_rolstatus.enable=on;
select pg_reload_conf();
reset role;

set role syssao;
select hg_security.set_role_status('role1', false);
select hg_security.set_role_status('role1', true);
select hg_security.get_role_status('role1');
alter system set hg_rolstatus.enable=off;
alter system set hg_rolstatus.enable=on;
select pg_reload_conf();
reset role;

set role user1;
select hg_security.set_role_status('role1', false);
select hg_security.set_role_status('role1', true);
select hg_security.get_role_status('role1');
alter system set hg_rolstatus.enable=off;
alter system set hg_rolstatus.enable=on;
select pg_reload_conf();
reset role;

set role user2;
select hg_security.set_role_status('role1', false);
select hg_security.set_role_status('role1', true);
select hg_security.get_role_status('role1');
alter system set hg_rolstatus.enable=off;
alter system set hg_rolstatus.enable=on;
select pg_reload_conf();
reset role;

drop role user1;
drop role user2;