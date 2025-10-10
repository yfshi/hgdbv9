--mediumint[(M)]
create table t1 (a mediumint(-1)); --fail
create table t1 (a mediumint(256)); --fail

create table t2 (a mediumint(0));
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a mediumint(3));
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t2 (a mediumint(3.1));
\d t2
drop table t2;

create table t2 (a mediumint(3.9));
\d t2
drop table t2;

--mediumint
create table t1(c1 mediumint);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-8388608);
insert into t1 values(8388607);
insert into t1 values(-8388609);
insert into t1 values(8388608);
insert into t1 values(true);--BUG#5392
insert into t1 values(false);--BUG#5392
select * from t1;
drop table t1;

create table t1(c1 mediumint SIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-8388608);
insert into t1 values(8388607);
insert into t1 values(-8388609);
insert into t1 values(8388608);
select * from t1;
drop table t1;

--int3
create table t1(c1 int3);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-8388608);
insert into t1 values(8388607);
insert into t1 values(-8388609);
insert into t1 values(8388608);
select * from t1;
drop table t1;

create table t1(c1 int3 SIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-8388608);
insert into t1 values(8388607);
insert into t1 values(-8388609);
insert into t1 values(8388608);
select * from t1;
drop table t1;

create table test1(b int3(0) default 3);
insert into test1 values ();
drop table test1;

--expr
create table test1(a int3, b int3 default 3);
insert into test1 values();
insert into test1 values(5, 1 + 2);
insert into test1 values(5, 5 - 2);
insert into test1 values(1, 2 * 2);
insert into test1 values(6, 6 / 2);
insert into test1 values(7, 7 % 2);
select * from test1;

select a + b from test1;
select a - b from test1;
select a * b from test1;
select a / b from test1;
select a % b from test1;
select a div b from test1; --BUG#5049

select a ^ b from test1; --BUG#5056

select not a from test1;
select !a from test1;

select a AND b from test1; --BUG#5050
select a && b from test1; --BUG#5050

select a or b from test1; --BUG#5051
set sql_mode = '';
select a || b from test1; --BUG#5051
reset sql_mode;
select a || b from test1; --BUG#5051

select a xor b from test1; --BUG#5052 

select a & b from test1; --BUG#5053
select a | b from test1; --BUG#5054
select ~b from test1;	--BUG#5055

select a >> b from test1;
select a << b from test1;

select a = b from test1;
select a <= b from test1;
select a < b from test1;
select a > b from test1;
select a >= b from test1;
--select a <=> b from test1;
drop table test1;

--'-'
create table tb_test(id int3);
insert into tb_test values (123),(-121),(0);
select -id from tb_test;
drop table tb_test;

--"+"
create table tb_test(id int3);
insert into tb_test values (123),(-121),(0);
select +id from tb_test;
drop table tb_test;

-- fix bug 5015
create table tb_test(id int3);
set sql_mode = default;
insert into tb_test values ('123abc');
set sql_mode = '';
insert into tb_test values ('123abc');
select * from tb_test;
drop table tb_test;
set sql_mode = default;
