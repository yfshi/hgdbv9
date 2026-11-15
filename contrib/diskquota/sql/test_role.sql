-- Test role quota
create extension diskquota;

create schema srole;
set search_path to srole;

CREATE role u1 NOLOGIN;
CREATE role u2 NOLOGIN;
CREATE TABLE b (t text);
ALTER TABLE b OWNER TO u1;
CREATE TABLE b2 (t text);
ALTER TABLE b2 OWNER TO u1;

select hg_security.set_role_quota('u1', '1 MB');

insert into b select generate_series(1,100);
-- expect insert fail
insert into b select generate_series(1,100000000);
-- expect insert fail
insert into b select generate_series(1,100);
-- expect insert fail
insert into b2 select generate_series(1,100);
alter table b owner to u2;
select pg_sleep(5);
select pg_sleep(2);
--expect insert succeed
insert into b select generate_series(1,100);
-- expect insert success
insert into b2 select generate_series(1,100);

drop table b, b2;
drop role u1, u2;
reset search_path;
drop schema srole;


/* drop table */
create schema srole;
set search_path to srole;

CREATE role u1 NOLOGIN;
CREATE role u2 NOLOGIN;
CREATE TABLE b (t text);
ALTER TABLE b OWNER TO u1;
CREATE TABLE b2 (t text);
ALTER TABLE b2 OWNER TO u1;

select hg_security.set_role_quota('u1', '1 MB');

insert into b select generate_series(1,100);
-- expect insert fail
insert into b select generate_series(1,100000000);
-- expect insert fail
insert into b select generate_series(1,100);
-- expect insert fail
insert into b2 select generate_series(1,100);
drop table b ;
select pg_sleep(5);
select pg_sleep(2);
-- expect insert success
insert into b2 select generate_series(1,100);

drop table b2;
drop role u1, u2;
reset search_path;
drop schema srole;



/* test temp table */
create schema strole;
create role u3temp nologin;
set search_path to strole;

select hg_security.set_role_quota('u3temp', '1MB');
create table a(i int);
alter table a owner to u3temp;
create temp table ta(i int);
alter table ta owner to u3temp;

-- expected failed: fill temp table
insert into ta select generate_series(1,100000000);
-- expected failed:
insert into a select generate_series(1,100);
drop table ta;
select pg_sleep(5);
insert into a select generate_series(1,100);

drop table a;
reset search_path;
drop schema strole;


-- Test truncate
CREATE role u1 NOLOGIN;
create schema s7;
select hg_security.set_role_quota('u1', '1 MB');
set search_path to s7;
create table a (i int);
create table b (i int);
ALTER TABLE a OWNER TO u1;
ALTER TABLE b OWNER TO u1;

insert into a select generate_series(1,50000);
-- expect insert fail
insert into a select generate_series(1,30);
insert into b select generate_series(1,30);
truncate table a;
-- expect insert succeed
insert into a select generate_series(1,30);
insert into b select generate_series(1,30);

drop table a, b;
reset search_path;
drop schema s7;
drop role u1;

-- Test Update
CREATE role u1 NOLOGIN;
create schema s4;
select hg_security.set_role_quota('u1', '1 MB');
set search_path to s4;
create table a(i int);
ALTER TABLE a OWNER TO u1;

insert into a select generate_series(1,50000);
-- expect update fail.
update a set i = 100;
drop table a;
reset search_path;
drop schema s4;
drop role u1;


-- Test vacuum full
CREATE role u1 NOLOGIN;

create schema s6;
select hg_security.set_role_quota('u1', '1 MB');
set search_path to s6;
create table a (i int);
create table b (i int);
ALTER TABLE a OWNER TO u1;

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
drop role u1;


