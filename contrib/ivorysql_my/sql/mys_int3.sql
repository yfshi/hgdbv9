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

set SQL_MODE='';
create table t1(c1 mediumint);
insert into t1 values(8388607);
insert into t1 values(8388608);
insert into t1 values(-8388608);
insert into t1 values(-8388609);
select * from t1;
drop table t1;
set SQL_MODE to default;

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

--numeric -> int3
select 2000.49999::int3;
select 2000.5::int3;
select (-8388608.2)::int3;
select 8388607.1::int3;
select 999999999999999.0205::int3;
select (-0.02)::int3;
select (-68.753951)::int3;
select (-999999999999999.0205)::int3;

set SQL_MODE='TRADITIONAL';
CREATE TABLE tb3 (a numeric, b mediumint);
insert into tb3 values (112233.556677, 9999);
insert into tb3 values (0.62, 100);
insert into tb3 values (7.05, 1000.123);
insert into tb3 values (7.05, 8388607.3);
insert into tb3 values (7.05, 8388607.0);
insert into tb3 values (7.05, 8388607.9);
insert into tb3 values (7.05, 999999999999999.0205);
insert into tb3 values (120.007, 0);
insert into tb3 values (9.36, -50000.753951);
insert into tb3 values (9.36, -8388607.8);
insert into tb3 values (9.36, -8388608.0);
insert into tb3 values (9.36, -8388609.2);
insert into tb3 values (9.36, -999999999999999.0205);
select * from tb3;
select cast(a as int3) from tb3;
select convert(a, int3) from tb3;
select a::int3 + b from tb3;
drop table tb3;

set SQL_MODE='';
CREATE TABLE tb3 (a numeric, b mediumint);
insert into tb3 values (112233.556677, 9999);
insert into tb3 values (0.62, 100);
insert into tb3 values (7.05, 1000.123);
insert into tb3 values (7.05, 8388607.3);
insert into tb3 values (7.05, 8388607.0);
insert into tb3 values (7.05, 8388607.9);
insert into tb3 values (7.05, 999999999999999.0205);
insert into tb3 values (120.007, 0);
insert into tb3 values (9.36, -50000.753951);
insert into tb3 values (9.36, -8388607.8);
insert into tb3 values (9.36, -8388608.0);
insert into tb3 values (9.36, -8388609.2);
insert into tb3 values (9.36, -999999999999999.0205);
select * from tb3;
select cast(a as int3) from tb3;
select convert(a, int3) from tb3;
select a::int3 + b from tb3;
drop table tb3;

set SQL_MODE to default;
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
