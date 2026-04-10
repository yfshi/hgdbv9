-- Test schema
create schema s1;
select hg_security.set_schema_quota('s1', '1 MB');
set search_path to s1;

create table a(i int);
insert into a select generate_series(1,100);
-- expect insert fail
insert into a select generate_series(1,100000000);
-- expect insert fail
insert into a select generate_series(1,100);

create table a2(i int);
-- expect insert fail
insert into a2 select generate_series(1,100);

-- Test alter table set schema
create schema s2;
alter table s1.a set schema s2;
select pg_sleep(5);
-- expect insert succeed
insert into a2 select generate_series(1,20000);
-- expect insert succeed
insert into s2.a select generate_series(1,20000);

reset search_path;
drop table s1.a2, s2.a;
drop schema s1, s2;

-- Test Drop table
create schema sdrtbl;
select hg_security.set_schema_quota('sdrtbl', '1 MB');
set search_path to sdrtbl;
create table a(i int);
create table a2(i int);
insert into a select generate_series(1,100);
-- expect insert fail
insert into a select generate_series(1,100000000);
-- expect insert fail
insert into a2 select generate_series(1,100);
drop table a;
select pg_sleep(5);
insert into a2 select generate_series(1,100);

drop table a2;
reset search_path;
drop schema sdrtbl;


-- Test truncate
create schema s7;
select hg_security.set_schema_quota('s7', '1 MB');
set search_path to s7;
create table a (i int);
create table b (i int);
insert into a select generate_series(1,50000);
--select pg_sleep(5);
-- expect insert fail
insert into a select generate_series(1,30);
insert into b select generate_series(1,30);
truncate table a;
--select pg_sleep(5);
-- expect insert succeed
insert into a select generate_series(1,30);
insert into b select generate_series(1,30);

drop table a, b;
reset search_path;
drop schema s7;


-- Test Update
create schema s4;
select hg_security.set_schema_quota('s4', '1 MB');
set search_path to s4;
create table a(i int);
insert into a select generate_series(1,50000);
-- expect update fail.
update a set i = 100;
drop table a;
reset search_path;
drop schema s4;

-- Test vacuum full
create schema s6;
select hg_security.set_schema_quota('s6', '1 MB');
set search_path to s6;
create table a (i int);
create table b (i int);
insert into a select generate_series(1,50000);
-- expect insert fail
insert into a select generate_series(1,10);
-- expect insert fail
insert into b select generate_series(1,10);
delete from a where i > 10;
vacuum full a;
-- expect insert succeed
insert into a select generate_series(1,10);
insert into b select generate_series(1,10);

drop table a, b;
reset search_path;
drop schema s6;


-- Test re-set_schema_quota
create schema srE;
select hg_security.set_schema_quota('srE', '1 MB');
set search_path to srE;
create table a(i int);
-- expect insert fail
insert into a select generate_series(1,1000000);
-- expect insert fail when exceed quota limit
insert into a select generate_series(1,1000);
-- set schema quota larger
select hg_security.set_schema_quota('srE', '1 GB');
-- expect insert succeed
insert into a select generate_series(1,1000);

drop table a;
reset search_path;
drop schema srE;

