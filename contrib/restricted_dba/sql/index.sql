-- INDEX

-- setup before test start
create database testdb;
\c testdb -
create table tb1_dba_u1(id int);
create table tb1_dba_u2(id int);
create table tb1_sso_u1(id int, id2 int);
create table tb1_sao_u1(id int, id2 int);
create index idx_sso_u1_id on tb1_sso_u1(id);
create index idx_sao_u1_id on tb1_sao_u1(id);
create table tb1_u1(id int);

alter table tb1_dba_u1 owner to sysdba_u1;
alter table tb1_dba_u2 owner to sysdba_u2;
alter table tb1_sso_u1 owner to syssso_u1;
alter table tb1_sao_u1 owner to syssao_u1;

alter table tb1_u1 owner to u1;

alter database testdb set se_feature.mode to on;
set se_feature.mode to on;

-- For member of sysdba_role
\c testdb sysdba_u1;

---- CREATE INDEX
------ pass
create index idx_dba_u1_id on tb1_dba_u1(id);
create index idx_dba_u2_id on tb1_dba_u2(id);
create index idx_u1_id on tb1_u1(id);

------ fail
create index idx_sso_u1_id2 on tb1_sso_u1(id2);
create index idx_sao_u1_id2 on tb1_sao_u1(id2);
create index idx_non_exist on tb1_non_exist(id);

---- ALTER INDEX RENAME
------ pass
alter index idx_dba_u1_id rename to idx_dba_u1_id_new;
alter index idx_dba_u2_id rename to idx_dba_u2_id_new;
alter index idx_u1_id rename to idx_u1_id_new;
alter index if exists idx_non_exist rename to idx_non_exist_new;

------ fail
alter index idx_sso_u1_id rename to idx_sso_u1_id_new;
alter index idx_sao_u1_id rename to idx_sao_u1_id_new;
alter index idx_non_exist rename to idx_non_exist_new;

---- ALTER INDEX
------ pass
alter index idx_dba_u1_id_new set (fillfactor = 75);
alter index idx_dba_u2_id_new set (fillfactor = 75);
alter index idx_u1_id_new set (fillfactor = 75);
alter index if exists idx_non_exist set (fillfactor = 75);

------ fail
alter index idx_sso_u1_id set (fillfactor = 75);
alter index idx_sao_u1_id set (fillfactor = 75);
alter index idx_non_exist set (fillfactor = 75);

---- DROP INDEX
------ pass
drop index idx_dba_u1_id_new;
drop index idx_dba_u2_id_new;
drop index idx_u1_id_new;
drop index if exists idx_non_exist;

------ fail
drop index idx_sso_u1_id;
drop index idx_sao_u1_id;
drop index idx_non_exist;

-- Clean test object
\c testdb syssso_u1
set se_feature.mode to on;
alter database testdb set se_feature.mode to off;
\c testdb sysdba_u1
drop table tb1_dba_u1;
drop table tb1_dba_u2;
drop table tb1_sso_u1;
drop table tb1_sao_u1;
drop table tb1_u1;

\c template1 sysdba_u1
drop database testdb;

-- set restricted_dba to on;

-- setup before test start
create database testdb;
\c testdb -
create table tb1_dba_u1(id int);
create table tb1_dba_u2(id int);
create table tb1_sso_u1(id int);
create table tb1_sao_u1(id int);
create table tb1_u1(id int);

create index idx1_dba_u1_id on tb1_dba_u1(id);
create index idx1_dba_u2_id on tb1_dba_u2(id);
create index idx1_sso_u1_id on tb1_sso_u1(id);
create index idx1_sao_u1_id on tb1_sao_u1(id);
create index idx1_u1_id on tb1_u1(id);

alter table tb1_dba_u1 owner to sysdba_u1;
alter table tb1_dba_u2 owner to sysdba_u2;
alter table tb1_sso_u1 owner to syssso_u1;
alter table tb1_sao_u1 owner to syssao_u1;
alter table tb1_u1 owner to u1;

alter database testdb set se_feature.mode to on;
alter database testdb set se_feature.restricted_dba to on;
set se_feature.restricted_dba to on;
set se_feature.mode to on;

-- For member of sysdba_role
\c testdb sysdba_u1;

---- CREATE INDEX
------ pass
create index idx2_dba_u1_id on tb1_dba_u1(id);
------ fail
create index idx2_dba_u2_id on tb1_dba_u2(id);
create index idx2_sso_u1_id on tb1_sso_u1(id);
create index idx2_sao_u1_id on tb1_sao_u1(id);
create index idx2_u1_id on tb1_u1(id);
create index idx_non_exist on tb1_non_exist(id);

---- ALTER INDEX RENAME
------ pass
begin;
alter index idx2_dba_u1_id rename to idx2_dba_u1_id_new;
rollback;
alter index if exists idx_non_exist rename to idx_non_exist_new;
------ fail
alter index idx1_dba_u2_id rename to idx_dba_u2_id_new;
alter index idx1_sso_u1_id rename to idx_sso_u1_id_new;
alter index idx1_sao_u1_id rename to idx_sao_u1_id_new;
alter index idx1_u1_id rename to idx2_u1_id_new;
alter index idx_non_exist rename to idx_non_exist_new;

---- ALTER INDEX
------ pass
alter index idx1_dba_u1_id set (fillfactor = 75);
alter index if exists idx_non_exist set (fillfactor = 75);
------ fail
alter index idx1_dba_u2_id set (fillfactor = 75);
alter index idx1_sso_u1_id set (fillfactor = 75);
alter index idx1_sao_u1_id set (fillfactor = 75);
alter index idx1_u1_id set (fillfactor = 75);
alter index idx_non_exist set (fillfactor = 75);

---- DROP INDEX
------ pass
drop index idx2_dba_u1_id;
drop index if exists idx_non_exist;
------ fail
drop index idx1_dba_u2_id;
drop index idx1_sso_u1_id;
drop index idx1_sao_u1_id;
drop index idx1_u1_id;
drop index idx_non_exist;

-- Clean test object
\c testdb syssso_u1
set se_feature.mode to on;
alter database testdb set se_feature.mode to off;
alter database testdb set se_feature.restricted_dba to off;

\c template1 sysdba_u1
drop database testdb;






