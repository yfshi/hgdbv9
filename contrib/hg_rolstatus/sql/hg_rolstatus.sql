set hg_rolstatus.enable to true;
show hg_rolstatus.enable;
--==============================================================================================--
-- test rolstatus, enable/disable role. 
--     test user acl
--==============================================================================================--
create role role1;
create user user2; 

grant role1 to user2;
select pa.rolname,rolstatus from hg_security.roles_status left join pg_authid pa on pa.oid = roleoid;

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
select pa.rolname,rolstatus from hg_security.roles_status left join pg_authid pa on pa.oid = roleoid;
select hg_security.get_role_status('role1');
-- fail
set role user2;
select * from tb1;
reset role;
-- enable role1
select hg_security.set_role_status('role1', true);
select pa.rolname,rolstatus from hg_security.roles_status left join pg_authid pa on pa.oid = roleoid;
select hg_security.get_role_status('role1');
-- ok
set role user2;
select * from tb1;
reset role;
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
select pa.rolname,rolstatus from hg_security.roles_status left join pg_authid pa on pa.oid = roleoid;
-- fail
set role user2;
create table sh1.id2(id int);
reset role;
-- enable role1
select hg_security.set_role_status('role1', true);
select pa.rolname,rolstatus from hg_security.roles_status left join pg_authid pa on pa.oid = roleoid;
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
select pa.rolname,rolstatus from hg_security.roles_status left join pg_authid pa on pa.oid = roleoid;

drop table tb1;
drop user user2;
drop role role1;

--==============================================================================================--
-- test rolstatus, enable/disable role. 
--     test guc
--==============================================================================================--
set hg_rolstatus.enable to false;
create role role1;
create user user2; 

grant role1 to user2;
select pa.rolname,rolstatus from hg_security.roles_status left join pg_authid pa on pa.oid = roleoid;

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
select pa.rolname,rolstatus from hg_security.roles_status left join pg_authid pa on pa.oid = roleoid;
-- ok
set role user2;
select * from tb1;
reset role;
-- enable role1
select hg_security.set_role_status('role1', true);
select pa.rolname,rolstatus from hg_security.roles_status left join pg_authid pa on pa.oid = roleoid;
-- ok
set role user2;
select * from tb1;
reset role;

drop table tb1;
drop user user2;
drop role role1;

set hg_rolstatus.enable to true;


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
select pa.rolname,rolstatus from hg_security.roles_status left join pg_authid pa on pa.oid = roleoid;
-- ok
set role user2;
select * from tb1;
reset role;
-- enable user1
select hg_security.set_role_status('user1', true);
select pa.rolname,rolstatus from hg_security.roles_status left join pg_authid pa on pa.oid = roleoid;
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
select pa.rolname,rolstatus from hg_security.roles_status left join pg_authid pa on pa.oid = roleoid;
-- ok
grant user1 to user2;
set role user1;
select * from tb1;
reset role;
-- enable user1
select hg_security.set_role_status('user1', true);
select pa.rolname,rolstatus from hg_security.roles_status left join pg_authid pa on pa.oid = roleoid;
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

set hg_rolstatus.enable to false;
show hg_rolstatus.enable;