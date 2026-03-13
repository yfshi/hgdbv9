----------------- modify
-------- test case: modify col
-- if there has one, modify the same one
-- mysql succ, v9 fail
create table test_m(a int, b float auto_increment unique key, c varchar(20));
insert into test_m(a,c) values(10,'111'),(20,'222');
select * from test_m;
show create table test_m;
alter table test_m modify b float auto_increment;
drop table test_m;

-------- test case: add, modify
create table test_m_a(a int, b varchar(20), c int);
-- mysql, error
alter table test_m_a add d int, modify d varchar(20);
-- diff col, succ
alter table test_m_a add d int primary key , modify c float;
alter table test_m_a add e float unique key, modify b varchar(20);
show create table test_m_a;
drop table test_m_a;

-------- test case: alter, modify
-- same column
create table test_m_a(a int, b varchar(20), c int);
insert into test_m_a values(1,1,1),(2,2,2);
-- mysql report error
alter table test_m_a alter column b set default 111,modify b int primary key;
insert into test_m_a values(3,3,3);
insert into test_m_a(a,c) values(4,4);
select * from test_m_a;
drop table test_m_a;
-- mysql success
create table test_m_a(a int, b varchar(20), c int);
alter table test_m_a modify b int, add primary key(b);
show create table test_m_a;
drop table test_m_a;

-- diff column
create table test_m_a(a int, b varchar(20), c int);
insert into test_m_a values(1,1,1),(2,2,2);
-- mysql report succ
alter table test_m_a alter column b set default 111,modify c varchar(20) default 'ssss';
insert into test_m_a values(3,3,3);
insert into test_m_a(a) values(4);
select * from test_m_a;
drop table test_m_a;

-------- test case: modify b, modify c, diff col
create table test_m_m(a int, b int, c varchar(20));
alter table test_m_m modify b int default '11', modify c varchar(20) default 'aaaa';
show create table test_m_m;
insert into test_m_m values();
select * from test_m_m;
alter table test_m_m modify b int, modify c varchar(20);
show create table test_m_m;
insert into test_m_m values();
select * from test_m_m;
drop table test_m_m;

-------- test case: modify,modify auto
-- modify one columns to auto_increment twice in one SQL
-- mysql error
create table test_auto(a int, b varchar(20), c float);
alter table test_auto modify b int auto_increment unique key, modify b float auto_increment;
show create table test_auto;
drop table test_auto;

create table test_auto(a int, b varchar(20), c float);
alter table test_auto modify b int auto_increment unique key, modify b float auto_increment unique key;
show create table test_auto;
drop table test_auto;

-- modify two columns to auto_increment in one SQL
-- should fail
create table test_auto(a int, b varchar(20), c float);
alter table test_auto modify b int auto_increment unique key, modify c int auto_increment unique key;

-- modify one column to two difference datatype and other attribute
alter table test_auto modify b int auto_increment unique key, modify c varchar(20);
alter table test_auto modify b int, modify c int auto_increment unique key;
show create table test_auto;
drop table test_auto;

-- b is a auto
create table test_auto(a int, b float auto_increment unique key, c int);
alter table test_auto modify b int auto_increment unique key, modify b float auto_increment;
show create table test_auto;
drop table test_auto;

create table test_auto(a int, b float auto_increment unique key, c int);
alter table test_auto modify b int auto_increment unique key, modify b float auto_increment unique key;
show create table test_auto;
drop table test_auto;

create table test_auto(a int, b varchar(20), c float);
alter table test_auto modify b int auto_increment unique key, modify c int auto_increment unique key;

alter table test_auto modify b int auto_increment unique key, modify c varchar(20);
alter table test_auto modify b int, modify c int auto_increment unique key;
show create table test_auto;
drop table test_auto;

-- modify, add
create table test_auto(a int, b varchar(20), c float);
alter table test_auto modify b float auto_increment unique key, add d int auto_increment unique key;
drop table test_auto;
-- modify, change
create table test_auto(a int, b varchar(20), c float);
alter table test_auto modify b float auto_increment unique key, change b b_new float auto_increment unique key;
show create table test_auto;
drop table test_auto;


------------------------- change
-- if there has one, modify the same one
-- mysql succ, v9 fail
create table test_m(a int, b float auto_increment unique key, c varchar(20));
insert into test_m(a,c) values(10,'111'),(20,'222');
select * from test_m;
show create table test_m;
alter table test_m change b b_new float auto_increment;
drop table test_m;

------- test case: change a, change a
-- mysql report idd does not exit
create table test_c2(a int, id int, name varchar(20));
alter table test_c2 change id idd int default '10', change idd iddd varchar(20) default 'ssss';
show create table test_c2;
insert into test_c2 values();
select * from test_c2;

-- succ
alter table test_c2 change id idd int default '10', change id iddd varchar(20) default 'ssss';
show create table test_c2;
insert into test_c2 values();
select * from test_c2;

-- succ
alter table test_c2 change id id int default '10', change id iddd varchar(20) default 'ssss';
show create table test_c2;
insert into test_c2 values();
select * from test_c2;

alter table test_c2 change iddd id int default '10';
insert into test_c2 values();
alter table test_c2 change id iddd varchar(20) default 'xxx';
insert into test_c2 values();
select * from test_c2;
drop table test_c2;

------- test case: change a, change b
create table test_c2(a int, id int, name varchar(20));
alter table test_c2 change id idd int default '10', change name namename varchar(20) default 'ssss';
show create table test_c2;
insert into test_c2 values();
select * from test_c2;
drop table test_c2;

------ test case: change a auto, change b auto
create table test_c2(a int, id int, name varchar(20));
alter table test_c2 change id idd int auto_increment unique key, change id idd int auto_increment unique key;
alter table test_c2 change id idd int auto_increment unique key, change name name_new int auto_increment unique key;
show create table test_c2;
insert into test_c2 values();
select * from test_c2;
drop table test_c2;

-- has one
create table test_c2(a int, id int auto_increment unique key, name varchar(20));
alter table test_c2 change id idd int auto_increment unique key, change id idd int auto_increment unique key;
alter table test_c2 change id idd int auto_increment unique key, change name name_new int auto_increment unique key;
show create table test_c2;
drop table test_c2;


