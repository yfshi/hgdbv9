-- int1
create table t1 (a int1);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-128);
insert into t1 values(-129);
insert into t1 values(127);
insert into t1 values(128);
select * from t1;
drop table t1;

create table t1 (a int1 SIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-128);
insert into t1 values(-129);
insert into t1 values(127);
insert into t1 values(128);
select * from t1;
drop table t1;

--tinyint
create table t1 (a tinyint);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-128);
insert into t1 values(-129);
insert into t1 values(127);
insert into t1 values(128);
select * from t1;
drop table t1;

create table t1 (a tinyint SIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-128);
insert into t1 values(-129);
insert into t1 values(127);
insert into t1 values(128);
select * from t1;
drop table t1;

create table t1 (a int1 UNSIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-128);
insert into t1 values(-129);
insert into t1 values(127);
insert into t1 values(128);
select * from t1;
drop table t1;

create table t1 (a tinyint UNSIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-128);
insert into t1 values(-129);
insert into t1 values(127);
insert into t1 values(128);
select * from t1;
drop table t1;

--int1[(M)]
create table t1 (a int1(-1));

create table t2 (a int1(0));
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a int1(3));
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t2 (a int1(3.1));
\d t2
drop table t2;

create table t2 (a int1(3.9));
\d t2
drop table t2;

--tinyint[(M)]
create table t1 (a tinyint(-1));
create table t1 (a tinyint(256));

create table t2 (a tinyint(0));
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a tinyint(3));
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table test1(b int1(0) default 3);
insert into test1 values ();
drop table test1;

--intx -> int1
select 32700::smallint::tinyint, (-32700)::smallint::tinyint;
select 20000000::tinyint, (-20000000)::tinyint;
select 6000000000::tinyint, (-6000000000)::tinyint;
create table t_1(a1 smallint);
create table t_2(a2 int);
create table t_3(a3 bigint);
insert into t_1 values(32700), (-32700);
insert into t_2 values(20000000), (-20000000);
insert into t_3 values(6000000000), (-6000000000);
select cast(a1 as tinyint) from t_1;
select cast(a2 as tinyint) from t_2;
select cast(a3 as tinyint) from t_3;
drop table t_1,t_2,t_3;

create table test1(a tinyint);
insert into test1 values(32700::smallint);
insert into test1 values(20000000);
insert into test1 values(6000000000);
insert into test1 values(127);
insert into test1 values(128);
insert into test1 values(0);
insert into test1 values(-128);
insert into test1 values(-129);
insert into test1 values(-32700::smallint);
insert into test1 values(-20000000);
insert into test1 values(-6000000000);
select * from test1;
drop table test1;

set SQL_MODE='';
create table test1(a tinyint);
insert into test1 values(32700::smallint);
insert into test1 values(20000000);
insert into test1 values(6000000000);
insert into test1 values(127);
insert into test1 values(128);
insert into test1 values(0);
insert into test1 values(-128);
insert into test1 values(-129);
insert into test1 values(-32700::smallint);
insert into test1 values(-20000000);
insert into test1 values(-6000000000);
select * from test1;
drop table test1;
set SQL_MODE to default;

--int1 -> numeric
select 20::int1::numeric;
select 0::int1::numeric;
select 127::int1::numeric;
select (-128)::int1::numeric;

CREATE TABLE tb1 (a tinyint);
insert into tb1 values (10);
insert into tb1 values (0);
insert into tb1 values (127);
insert into tb1 values (-128);
select * from tb1;
select cast(a as numeric) from tb1;
select convert(a, numeric) from tb1;
create table tb2 as select convert(a, decimal(20,8)) from tb1;
desc tb2;
select * from tb2;
drop table tb1,tb2;

set SQL_MODE='';
CREATE TABLE tb1 (a tinyint);
insert into tb1 values (10);
insert into tb1 values (0);
insert into tb1 values (127);
insert into tb1 values (-128);
select * from tb1;
select cast(a as numeric) from tb1;
select convert(a, numeric) from tb1;
drop table tb1;
set SQL_MODE to default;

--numeric -> int1
select 20.49999::int1;
select 20.5::int1;
select 127.3::int1;
select 999999999999999.0205::int1;
select (-0.02)::int1;
select (-68.753951)::int1;
select (-128.2)::int1;
select (-999999999999999.0205)::int1;

CREATE TABLE tb1 (a numeric, b tinyint);
insert into tb1 values (112.556677, 99);
insert into tb1 values (0.62, 100);
insert into tb1 values (7.05, 10.123);
insert into tb1 values (7.05, 127.3);
insert into tb1 values (7.05, 127.0);
insert into tb1 values (7.05, 127.9);
insert into tb1 values (7.05, 999999999999999.0205);
insert into tb1 values (12.007, 0);
insert into tb1 values (9.36, -50.753951);
insert into tb1 values (9.36, -127.8);
insert into tb1 values (9.36, -128.0);
insert into tb1 values (9.36, -129.2);
insert into tb1 values (9.36, -999999999999999.0205);
select * from tb1;
select cast(a as int1) from tb1;
select convert(a, int1) from tb1;
select a::int1 + b from tb1;
drop table tb1;

set SQL_MODE='';
CREATE TABLE tb1 (a numeric, b tinyint);
insert into tb1 values (112.556677, 99);
insert into tb1 values (0.62, 100);
insert into tb1 values (7.05, 10.123);
insert into tb1 values (7.05, 127.3);
insert into tb1 values (7.05, 127.0);
insert into tb1 values (7.05, 127.9);
insert into tb1 values (7.05, 999999999999999.0205);
insert into tb1 values (12.007, 0);
insert into tb1 values (9.36, -50.753951);
insert into tb1 values (9.36, -127.8);
insert into tb1 values (9.36, -128.0);
insert into tb1 values (9.36, -129.2);
insert into tb1 values (9.36, -999999999999999.0205);
select * from tb1;
select cast(a as int1) from tb1;
select convert(a, int1) from tb1;
select a::int1 + b from tb1;
drop table tb1;
set SQL_MODE to default;

--expr
create table test1(a int1, b int1 default 3);
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

select not a from test1;
select !a from test1;

select a AND b from test1; --BUG#5050
select a && b from test1; --BUG#5050

select a or b from test1; --BUG#5051
set sql_mode = '';
select a || b from test1; --BUG#5051
reset sql_mode;
select a || b from test1; --BUG#5051

select a ^ b from test1; --BUG#5056
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
select a <=> b from test1;
drop table test1;

--'-'
create table tb_test(id int1);
insert into tb_test values (123),(-121),(0);
select -id from tb_test;
select * from tb_test;
drop table tb_test;

--'+'
create table tb_test(id int1);
insert into tb_test values (123),(-121),(0);
select +id from tb_test;
drop table tb_test;

-- fix bug 5015
create table tb_test(id int1);
set sql_mode = default;
insert into tb_test values ('123abc');
set sql_mode = '';
insert into tb_test values ('123abc');
select * from tb_test;
drop table tb_test;
set sql_mode = default;

--btree and hash index
set enable_bitmapscan = false;
set enable_seqscan = false;

create table idx_1 (idx tinyint);
create index idx1 on idx_1 using btree (idx);
insert into idx_1 values (11),(22),(33);
vacuum full analyze idx_1;
explain (COSTS OFF) select * from idx_1 where idx < 11;
explain (COSTS OFF) select * from idx_1 where idx > 22;
explain (COSTS OFF) select * from idx_1 where idx = 33;
explain (COSTS OFF) select * from idx_1 where idx <= 44;
explain (COSTS OFF) select * from idx_1 where idx >= 55;

create table idx_2 (idx tinyint);
create index idx2 on idx_2 using hash (idx);
vacuum full analyze idx_2;
insert into idx_2 values (11),(22),(33);
explain (COSTS OFF) select * from idx_2 where idx = cast(11 as tinyint);
explain (COSTS OFF) select * from idx_2 where idx > cast(11 as tinyint);

drop table idx_1,idx_2;

--Cross data type comparison for index
CREATE TABLE index_conversion(
    tinyint_col             tinyint,
    smallint_col            smallint,
    mediumint_col           mediumint,
    int_col                 int,
    bigint_col              bigint,
    tinyint_unsigned_col    tinyint unsigned,
    smallint_unsigned_col   smallint unsigned,
    mediumint_unsigned_col  mediumint unsigned,
    int_unsigned_col        int unsigned,
    bigint_unsigned_col     bigint unsigned
);

create index idx1 on index_conversion using btree (tinyint_col);
create index idx2 on index_conversion using btree (smallint_col);
create index idx3 on index_conversion using btree (mediumint_col);
create index idx4 on index_conversion using btree (int_col);
create index idx5 on index_conversion using btree (bigint_col);
create index idx6 on index_conversion using btree (tinyint_unsigned_col);
create index idx7 on index_conversion using btree (smallint_unsigned_col);
create index idx8 on index_conversion using btree (mediumint_unsigned_col);
create index idx9 on index_conversion using btree (int_unsigned_col);
create index idx10 on index_conversion using btree (bigint_unsigned_col);

insert into index_conversion values (10, 3000, 8388600, 2147483647, 1000000000000, 252, 65530, 16777210, 4294967290, 2000000000000);
vacuum full analyze index_conversion;

explain select * from index_conversion where tinyint_col = 10;
explain select * from index_conversion where tinyint_col = 1000;
explain select * from index_conversion where tinyint_col = CAST(10 AS UNSIGNED);
explain select * from index_conversion where tinyint_col > 1000000000000;
explain select * from index_conversion where tinyint_col < -5000;
explain select * from index_conversion where tinyint_col <= CAST(65530 AS UNSIGNED);

reset enable_seqscan;
reset enable_bitmapscan;
drop table index_conversion;