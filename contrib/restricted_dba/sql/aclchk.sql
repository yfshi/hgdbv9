-- ACL privileges check for query 

-- setup before test start
create database testdb;

create user dba_u1 superuser createdb createrole;
create user dba_u2 superuser createdb createrole;
create user dba_u3 superuser createdb createrole;
create user dba_u4 superuser createdb createrole;
create user dba_u5 superuser createdb createrole;
grant sysdba_role to dba_u1,dba_u2,dba_u3,dba_u4,dba_u5;

-- FOR TABLE
\c testdb -
create table tb1_dba_u1(col1 int, col2 int, col3 int, col4 int);
create table tb1_u1(col1 int, col2 int, col3 int, col4 int);
create table tb1_sso_u1(col1 int, col2 int, col3 int, col4 int);
create table tb1_sao_u1(col1 int, col2 int, col3 int, col4 int);

alter table tb1_u1 owner to u1;
alter table tb1_sso_u1 owner to syssso_u1;
alter table tb1_sao_u1 owner to syssao_u1;

grant select on tb1_sso_u1 to dba_u1;
grant update on tb1_sso_u1 to dba_u2;
grant insert on tb1_sso_u1 to dba_u3;
grant delete on tb1_sso_u1 to dba_u4;
grant truncate on tb1_sso_u1 to dba_u5;

grant select(col1) on tb1_sao_u1 to dba_u1;
grant update(col1) on tb1_sao_u1 to dba_u2;
grant insert(col1) on tb1_sao_u1 to dba_u3;

alter database testdb set se_feature.mode to on;
set se_feature.mode to on;


-- TEST START
---- For TABLE SELECT/UPDATE/INSERT/DELETE/

\c testdb dba_u1;
------ pass
--------- table-level
select * from tb1_sso_u1;
select * from tb1_dba_u1;
select * from tb1_u1;
--------- column-level
select col1 from tb1_sao_u1;
select col1,col2,col3,col4 from tb1_dba_u1;
select col1,col2,col3,col4 from tb1_u1;
------ fail
--------- table-level
insert into tb1_sso_u1 values(1,1,1,1);
update tb1_sso_u1 set col4 = col4 + 1;
delete from tb1_sso_u1;
truncate table tb1_sso_u1;
--------- column-level
select col2 from tb1_sao_u1;
select col3 from tb1_sao_u1;
select col4 from tb1_sao_u1;
insert into tb1_sao_u1(col1) values(1);
update tb1_sao_u1 set col1 = col1 + 1;

\c testdb dba_u2;
------ pass
--------- table-level
update tb1_sso_u1 set col1 = 1;
update tb1_sso_u1 set col2 = 1;
update tb1_sso_u1 set col3 = 1;
update tb1_sso_u1 set col4 = 1;
update tb1_dba_u1 set col1 = 1;
update tb1_dba_u1 set col2 = 1;
update tb1_dba_u1 set col3 = 1;
update tb1_dba_u1 set col4 = 1;
update tb1_u1 set col1 = 1;
update tb1_u1 set col2 = 1;
update tb1_u1 set col3 = 1;
update tb1_u1 set col4 = 1;
--------- column-level
update tb1_sao_u1 set col1 = 2;
------ fail
--------- table-level
select * from tb1_sso_u1;
insert into tb1_sso_u1 values(1,1,1,1);
delete from tb1_sso_u1;
truncate table tb1_sso_u1;
--------- column-level
select col1 from tb1_sao_u1;
insert into tb1_sao_u1(col1) values(1);
update tb1_sao_u1 set col2 = 2;
update tb1_sao_u1 set col3 = 2;
update tb1_sao_u1 set col4 = 2;

\c testdb dba_u3;
------ pass
--------- table-level
insert into tb1_sso_u1 values(1,1,1,1);
insert into tb1_dba_u1 values(1,1,1,1);
insert into tb1_u1 values(1,1,1,1);
--------- column-level
insert into tb1_sao_u1(col1) values(1);
insert into tb1_dba_u1(col1) values(1);
insert into tb1_dba_u1(col2) values(1);
insert into tb1_dba_u1(col3) values(1);
insert into tb1_dba_u1(col4) values(1);
insert into tb1_u1(col1) values(1);
insert into tb1_u1(col2) values(1);
insert into tb1_u1(col3) values(1);
insert into tb1_u1(col4) values(1);
------ fail
--------- table-level
select * from tb1_sso_u1;
update tb1_sso_u1 set col4 = col4 + 1;
delete from tb1_sso_u1;
truncate table tb1_sso_u1;
--------- column-level
insert into tb1_sao_u1(col2) values(1);
insert into tb1_sao_u1(col3) values(1);
insert into tb1_sao_u1(col4) values(1);
select col1 from tb1_sao_u1;
update tb1_sao_u1 set col1 = col1 + 1;

\c testdb dba_u4;
------ pass
--------- table-level
delete from tb1_sso_u1;
delete from tb1_dba_u1;
delete from tb1_u1;
------ fail
--------- table-level
select * from tb1_sso_u1;
update tb1_sso_u1 set col4 = col4 + 1;
insert into tb1_sso_u1 values(1,1,1,1);
truncate table tb1_sso_u1;

\c testdb dba_u5;
------ pass
truncate table tb1_sso_u1;
truncate table tb1_dba_u1;
truncate table tb1_u1;
------ fail
truncate table tb1_sao_u1;




-- Clean test object
\c testdb syssso_u1
set se_feature.mode to on;
alter database testdb set se_feature.mode to off;
\c testdb sysdba_u1
drop table tb1_sso_u1;
drop table tb1_sao_u1;
drop table tb1_dba_u1;
drop table tb1_u1;

\c template1 sysdba_u1
drop user dba_u1,dba_u2,dba_u3,dba_u4,dba_u5;
drop database testdb;


-- set restricted_dba to on
create database testdb;

create user dba_u1 superuser createdb createrole;
create user dba_u2 superuser createdb createrole;
create user dba_u3 superuser createdb createrole;
create user dba_u4 superuser createdb createrole;
create user dba_u5 superuser createdb createrole;
grant sysdba_role to dba_u1,dba_u2,dba_u3,dba_u4,dba_u5;

-- \du ROLE
/* Begin - bug#z6385 */
\c testdb sysdba_u1
create user du_u1 with password '1234' superuser;
set role syssso_u1;
set se_feature.mode to on;
\du du_u1
set se_feature.mode to off;
\du du_u1

reset role;
create database testdb1;
alter database testdb1 set se_feature.mode to on;
\c testdb1 syssso_u1
alter database testdb1 set se_feature.restricted_dba to on;
\du du_u1
\c testdb1 sysdba_u1
\du du_u1
\c testdb1 syssao_u1
\du du_u1
\c testdb1 du_u1
\du du_u1
\c testdb sysdba_u1
drop user du_u1;
/* End - bug#z6385 */

-- FOR TABLE
\c testdb -
create table tb1_dba_u1(col1 int, col2 int, col3 int, col4 int);
create table tb1_u1(col1 int, col2 int, col3 int, col4 int);
create table tb1_sso_u1(col1 int, col2 int, col3 int, col4 int);
create table tb1_sao_u1(col1 int, col2 int, col3 int, col4 int);

alter table tb1_u1 owner to u1;
alter table tb1_dba_u1 owner to sysdba_u1;
alter table tb1_sso_u1 owner to syssso_u1;
alter table tb1_sao_u1 owner to syssao_u1;

grant select on tb1_dba_u1 to dba_u1;
grant update on tb1_dba_u1 to dba_u2;
grant insert on tb1_dba_u1 to dba_u3;
grant delete on tb1_dba_u1 to dba_u4;
grant truncate on tb1_dba_u1 to dba_u5;

grant select on tb1_sso_u1 to dba_u1;
grant update on tb1_sso_u1 to dba_u2;
grant insert on tb1_sso_u1 to dba_u3;
grant delete on tb1_sso_u1 to dba_u4;
grant truncate on tb1_sso_u1 to dba_u5;

grant select(col1) on tb1_sao_u1 to dba_u1;
grant update(col1) on tb1_sao_u1 to dba_u2;
grant insert(col1) on tb1_sao_u1 to dba_u3;

grant select(col1) on tb1_u1 to dba_u1;
grant update(col1) on tb1_u1 to dba_u2;
grant insert(col1) on tb1_u1 to dba_u3;

alter database testdb set se_feature.mode to on;
alter database testdb set se_feature.restricted_dba to on;
set se_feature.restricted_dba to on;
set se_feature.mode to on;


-- TEST START
---- For TABLE SELECT/UPDATE/INSERT/DELETE

\c testdb dba_u1;
------ pass
--------- table-level
select * from tb1_sso_u1;
select * from tb1_dba_u1;
--------- column-level
select col1 from tb1_sao_u1;
select col1,col2,col3,col4 from tb1_dba_u1;
select col1,col2,col3,col4 from tb1_sso_u1;
select col1 from tb1_u1;
------ fail
--------- table-level
insert into tb1_sso_u1 values(1,1,1,1);
update tb1_sso_u1 set col4 = 1;
delete from tb1_sso_u1;
insert into tb1_dba_u1 values(1,1,1,1);
update tb1_dba_u1 set col4 = 1;
delete from tb1_dba_u1;
--------- column-level
select col2 from tb1_sao_u1;
select col3 from tb1_sao_u1;
select col4 from tb1_sao_u1;
select col2 from tb1_u1;
select col3 from tb1_u1;
select col4 from tb1_u1;
insert into tb1_sao_u1(col1) values(1);
update tb1_sao_u1 set col1 = 1;
insert into tb1_u1(col1) values(1);
update tb1_u1 set col1 = 1;

-- for its owner table 
begin;
create table tb_test_dba1(col1 int, col2 int, col3 int);
insert into tb_test_dba1 values (1,2,3);
select * from tb_test_dba1;
update tb_test_dba1 set col1 = col2;
delete from tb_test_dba1;
rollback;

\c testdb dba_u2;
------ pass
--------- table-level
update tb1_dba_u1 set col1 = 1;
update tb1_dba_u1 set col2 = 1;
update tb1_dba_u1 set col3 = 1;
update tb1_dba_u1 set col4 = 1;
update tb1_sso_u1 set col1 = 1;
update tb1_sso_u1 set col2 = 1;
update tb1_sso_u1 set col3 = 1;
update tb1_sso_u1 set col4 = 1;
--------- column-level
update tb1_sao_u1 set col1 = 1;
update tb1_u1 set col1 = 1;
------ fail
--------- table-level
select * from tb1_sso_u1;
insert into tb1_sso_u1 values(1,1,1,1);
delete from tb1_sso_u1;
select * from tb1_dba_u1;
insert into tb1_dba_u1 values(1,1,1,1);
delete from tb1_dba_u1;
--------- column-level
update tb1_sao_u1 set col2 = 1;
update tb1_sao_u1 set col3 = 1;
update tb1_sao_u1 set col4 = 1;
update tb1_u1 set col2 = 1;
update tb1_u1 set col3 = 1;
update tb1_u1 set col4 = 1;
select col1 from tb1_sao_u1;
insert into tb1_sao_u1(col1) values(1);
select col1 from tb1_u1;
insert into tb1_u1(col1) values(1);

\c testdb dba_u3;
------ pass
--------- table-level
insert into tb1_sso_u1 values(1,1,1,1);
insert into tb1_dba_u1 values(1,1,1,1);
--------- column-level
insert into tb1_sao_u1(col1) values(1);
insert into tb1_u1(col1) values(1);
insert into tb1_dba_u1(col1) values(1);
insert into tb1_dba_u1(col2) values(1);
insert into tb1_dba_u1(col3) values(1);
insert into tb1_dba_u1(col4) values(1);
insert into tb1_sso_u1(col1) values(1);
insert into tb1_sso_u1(col2) values(1);
insert into tb1_sso_u1(col3) values(1);
insert into tb1_sso_u1(col4) values(1);
------ fail
--------- table-level
select * from tb1_sso_u1;
update tb1_sso_u1 set col4 = 1;
delete from tb1_sso_u1;
select * from tb1_dba_u1;
update tb1_dba_u1 set col4 = 1;
delete from tb1_dba_u1;
--------- column-level
insert into tb1_sao_u1(col2) values(1);
insert into tb1_sao_u1(col3) values(1);
insert into tb1_sao_u1(col4) values(1);
insert into tb1_u1(col2) values(1);
insert into tb1_u1(col3) values(1);
insert into tb1_u1(col4) values(1);
select col1 from tb1_sao_u1;
update tb1_sao_u1 set col1 = 1;
select col1 from tb1_u1;
update tb1_u1 set col1 = 1;

\c testdb dba_u4;
------ pass
--------- table-level
delete from tb1_sso_u1;
delete from tb1_dba_u1;
------ fail
--------- table-level
delete from tb1_u1;
delete from tb1_sao_u1;
select * from tb1_sso_u1;
update tb1_sso_u1 set col4 = 1;
insert into tb1_sso_u1 values(1,1,1,1);
select * from tb1_dba_u1;
update tb1_dba_u1 set col4 = 1;
insert into tb1_dba_u1 values(1,1,1,1);

\c testdb dba_u5;
------ pass
truncate table tb1_sso_u1;
truncate table tb1_dba_u1;
------ fail
truncate table tb1_sao_u1;
truncate table tb1_u1;

-- Clean test object
\c testdb syssso_u1
set se_feature.mode to on;
alter database testdb set se_feature.mode to off;
alter database testdb set se_feature.restricted_dba to off;
\c testdb sysdba_u1
drop table tb1_sso_u1;
drop table tb1_sao_u1;
drop table tb1_dba_u1;
drop table tb1_u1;

\c template1 sysdba_u1
drop user dba_u1,dba_u2,dba_u3,dba_u4,dba_u5;
drop database testdb;
