-- rename user
create user u1;
rename user u1 to u2;
select usename from pg_user where usename = 'u2';
rename user u2 to u3, u3 to u4;
select usename from pg_user where usename = 'u4';
rename user 'u4'@'localhost' to 'u5'@'localhost';
select usename from pg_user where usename = 'u5';
rename user 'u5'@'localhost' to 'u6'@'localhost', 'u6'@'localhost' to 'u7'@'localhost';
select usename from pg_user where usename = 'u7';
drop role u7;

create user ace;
RENAME USER 'ace'@'127.0.0.1'TO 'ace1'@'127.0.0.1';
select usename from pg_user where usename = 'ace1';
drop role ace1;

create user u1;
rename user 'u1'@'localhost' to 'U1'@'localhost';
select usename from pg_user where usename = 'U1';
drop role U1;

create user u1;
begin;
rename user u1 to u2;
rollback;
select usename from pg_user where usename = 'u1';
drop role u1;

create user u1;
rename user u1 to u2, u1 to u2, u2 to u3;
select usename from pg_user where usename = 'u1';
drop role u2;

-- rename table
create table t1 (c int);
rename table t1 to t2;
\d t1;
\d t2;
rename table t2 to t3, t3 to t4;
\d t2;
\d t3;
\d t4;
drop table t4;

create temporary table temp1(c int);
select * from temp1;
select * from temp2;
rename table temp1 to temp2;
select * from temp1;
select * from temp2;
drop table temp2;

create table t1 (c int);
set lower_case_table_names = 1;
alter table t1 rename to T1;
\d t1;
\d T1;
set lower_case_table_names = 0;
alter table t1 rename to T1;
\d t1;
\d T1;
drop table T1;

create table t1 (c int);
begin;
rename table t1 to t2;
rollback;
\d t1;
\d t2;
drop table t1;

/* bug#z5993 */
rename table t1 to table_new;

-- alter table rename index 
create table t(c int);
create index i1 on t(c);
alter table t rename index i1 to i2;
\d t;
alter table t rename index i2 to i3;
-- alter table rename tablename
create table t(c int);
alter table t rename to t1;
\d t1;
alter table t1 rename as t2;
\d t2;
drop index i3;
drop table t2;

--toplevel_stmt
create table t1(c int); insert into t1 values(1); rename table t1 to t2; select * from t2; drop table t2;
create user user1;create user user2;rename user user1 to user3,user2 to user4;
drop user user3;
drop user user4;

--fix bug 5992
create table test ( id int primary key, name text, email text );
create index original_index on test(name);
create index idx_02 on test(name);

select indexname from pg_indexes where tablename='test';
alter table test rename index idx_02 to ORIGINAL_INDEX; --err
select indexname from pg_indexes where tablename='test';
alter table test rename index idx_02 to IDX_02; -- succ
alter table test rename index idx_02 to ORIGINAL_INDEX;
drop table test;

create table test(id int, name text);
create index idx_temp on test(id);
create index IDX_TEMP on test(name); --err
create index idx_TeMP on test(name); --err
drop table test;

/* Begin - bug#z5994 */
create schema sch1;
create table test (id int,name text);
insert into  test values (1, 'Alice');
rename table  test to sch1.test;
select  * from  test;
select  * from  sch1.test;
drop table sch1.test;
create table test (id int,name text);
insert into  test values (1, 'Alice');
alter table test rename to sch1.test;
select  * from  test;
select  * from  sch1.test;
drop table sch1.test;
create table test (id int,name text);
rename table test to sch2.test1;
\d test;
rename table test to sch1.test1;
\d sch1.test1;
create schema sch2;
rename table sch1.test1 to sch2.test2;
\d sch2.test2;
create schema sch3;
create schema sch4;
rename table sch2.test2 to sch3.test3, sch3.test3 to sch4.test4;
\d sch4.test4;
create schema sch5;
ALTER TABLE sch4.test4 RENAME TO sch5.test5;
\d sch5.test5;
drop table sch5.test5;
drop schema sch1,sch2,sch3,sch4,sch5;
/* End - bug#z5994 */
