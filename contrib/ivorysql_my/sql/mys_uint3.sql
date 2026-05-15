set SQL_MODE='TRADITIONAL';

--mediumint
create table t1(c1 mediumint UNSIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-2147483648);
insert into t1 values(2147483647);
insert into t1 values(-2147483649);
insert into t1 values(4294967295);
insert into t1 values(16777216);
insert into t1 values(16777215);
insert into t1 values(65535);
insert into t1 values(255);
insert into t1 values('a111') ;
insert into t1 values('111a') ;
insert into t1 values('.111') ;
insert into t1 values('0111') ;
insert into t1 values('#111') ;
insert into t1 values('111#') ;
insert into t1 values('111.') ;
select * from t1;
drop table t1;

create table t1(c1 mediumint UNSIGNED);
insert into t1 values(1.4);
insert into t1 values(1.5);
insert into t1 values('1.4');
insert into t1 values('1.5');
select * from t1;
drop table t1;

--mediumint[(M)]
create table t1 (a mediumint[-1] UNSIGNED); --fail

create table t2 (a mediumint(0) UNSIGNED);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a mediumint(3) UNSIGNED);
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t3 (a mediumint(4) UNSIGNED);
\d t3
insert into t3 values (3);
select * from t3;
drop table t3;

create table t2 (a mediumint(3.4) UNSIGNED);
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t2 (a mediumint(3.5) UNSIGNED);
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t2(age mediumint(256.4) unsigned);
create table t2(id1 int3(255.9) unsigned, id2 int3(254.9) unsigned, age mediumint(255.4) unsigned);
\d t2
insert into t2 values (2, 5, 44);
select * from t2;
drop table t2;

--mediumint zerofill
create table t2 (a mediumint(-1) ZEROFILL);

create table t2 (a mediumint ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a mediumint(2) ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

--int3
create table t1(c1 int3 UNSIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-2147483648);
insert into t1 values(2147483647);
insert into t1 values(-2147483649);
insert into t1 values(4294967295);
insert into t1 values(16777216);
insert into t1 values(16777215);
insert into t1 values(65535);
insert into t1 values(255);
select * from t1;
drop table t1;

create table t1(c1 int3 UNSIGNED);
insert into t1 values(1.4);
insert into t1 values(1.5);
insert into t1 values('1.4');
insert into t1 values('1.5');
select * from t1;
drop table t1;

--int3[(M)]
create table t1 (a int3[-1] UNSIGNED); --fail

create table t2 (a int3(0) UNSIGNED);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a int3(3) UNSIGNED);
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t3 (a int3(4) UNSIGNED);
\d t3
insert into t3 values (3);
select * from t3;
drop table t3;

--mediumint zerofill
create table t2 (a int3(-1) ZEROFILL);

create table t2 (a int3 ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a int3(2) ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

set SQL_MODE='';

--mediumint
create table t1(c1 mediumint UNSIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-2147483648);
insert into t1 values(2147483647);
insert into t1 values(-2147483649);
insert into t1 values(4294967295);
insert into t1 values(16777216);
insert into t1 values(16777215);
insert into t1 values(65535);
insert into t1 values(255);
insert into t1 values('a111') ;
insert into t1 values('111a') ;
insert into t1 values('.111') ;
insert into t1 values('0111') ;
insert into t1 values('#111') ;
insert into t1 values('111#') ;
insert into t1 values('111.') ;
select * from t1;
drop table t1;

--mediumint[(M)]
create table t1 (a mediumint[-1] UNSIGNED ); --fail

create table t2 (a mediumint(0) UNSIGNED);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a mediumint(3) UNSIGNED);
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t3 (a mediumint(4) UNSIGNED);
\d t3
insert into t3 values (3);
select * from t3;
drop table t3;

--mediumint zerofill
create table t2 (a mediumint(-1) ZEROFILL);

create table t2 (a mediumint ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a mediumint(2) ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

--int3
create table t1(c1 int3 UNSIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-2147483648);
insert into t1 values(2147483647);
insert into t1 values(-2147483649);
insert into t1 values(4294967295);
insert into t1 values(16777216);
insert into t1 values(16777215);
insert into t1 values(65535);
insert into t1 values(255);
select * from t1;
drop table t1;

--int3[(M)]
create table t1 (a int3[-1] UNSIGNED); --fail

create table t2 (a int3(0) UNSIGNED);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a int3(3) UNSIGNED);
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t3 (a int3(4) UNSIGNED);
\d t3
insert into t3 values (3);
select * from t3;
drop table t3;

--mediumint zerofill
create table t2 (a int3(-1) ZEROFILL);

create table t2 (a int3 ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a int3(2) ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

--numeric -> uint3
select 2000.49999::uint3;
select 2000.5::uint3;
select 16777214.8::uint3;
select 16777216.1::uint3;
select 999999999999999.0205::uint3;
select (-0.02)::uint3;
select (-68.753951)::uint3;
select (-8388607.4)::uint3;
select (-8388609.2)::uint3;
select (-999999999999999.0205)::uint3;

set SQL_MODE='TRADITIONAL';
CREATE TABLE tb3 (a numeric, b mediumint UNSIGNED);
insert into tb3 values (112233.556677, 9999);
insert into tb3 values (0.62, 100);
insert into tb3 values (7.05, 1000.123);
insert into tb3 values (7.05, 16777214.8);
insert into tb3 values (7.05, 16777215.0);
insert into tb3 values (7.05, 16777216.1);
insert into tb3 values (7.05, 999999999999999.0205);
insert into tb3 values (120.007, 0);
insert into tb3 values (9.36, -50000.753951);
insert into tb3 values (9.36, -8388607.4);
insert into tb3 values (9.36, -8388608.0);
insert into tb3 values (9.36, -8388609.2);
insert into tb3 values (9.36, -999999999999999.0205);
select * from tb3;
select cast(a as uint3) from tb3;
select convert(a, uint3) from tb3;
select a::uint3 + b from tb3;
drop table tb3;

set SQL_MODE='';
CREATE TABLE tb3 (a numeric, b mediumint UNSIGNED);
insert into tb3 values (112233.556677, 9999);
insert into tb3 values (0.62, 100);
insert into tb3 values (7.05, 1000.123);
insert into tb3 values (7.05, 16777214.8);
insert into tb3 values (7.05, 16777215.0);
insert into tb3 values (7.05, 16777216.1);
insert into tb3 values (7.05, 999999999999999.0205);
insert into tb3 values (120.007, 0);
insert into tb3 values (9.36, -50000.753951);
insert into tb3 values (9.36, -8388607.4);
insert into tb3 values (9.36, -8388608.0);
insert into tb3 values (9.36, -8388609.2);
insert into tb3 values (9.36, -999999999999999.0205);
select * from tb3;
select cast(a as uint3) from tb3;
select convert(a, uint3) from tb3;
select a::uint3 + b from tb3;
drop table tb3;

set SQL_MODE to default;

--uint3 -> numeric
select 16777215::uint3::numeric(15,4);

create table tb_03 (a mediumint unsigned);
insert into tb_03 values(10000);
select cast(a as numeric(15,3)) from tb_03;
select convert(a, numeric) from tb_03;
drop table tb_03;

create table t_convert_3 (col_numeric numeric);
insert into t_convert_3 values (16777215::uint3);
select * from t_convert_3;
drop table t_convert_3;

--uint3 -> signed/unsigned (int8/uint8)
select 8000000::int3::int8;
select 8000000::int3::uint8;
select convert(150000::uint3, signed);
select cast(150000::uint3 as unsigned);

create table test1(a mediumint unsigned);
insert into test1 values(5000000);
select convert(a, signed) from test1;
select cast(a as unsigned) from test1;
drop table test1;

-- oper
--------------------------------------------------
CREATE TABLE t2 (a mediumint UNSIGNED, b mediumint UNSIGNED);
insert into t2 values(100, 999);
insert into t2 values(222, 999);
insert into t2 values(333, 999);
insert into t2 values(444, 999);
insert into t2 values(555, 100);

---=
select * from t2 where a = b;
select * from t2 where b = 999;
--<>
select * from t2 where a <> b;
select * from t2 where b <> 999;
--<
select * from t2 where a < b;
select * from t2 where b < 999;
-->
select * from t2 where a > b;
select * from t2 where b > 999;
--<=
select * from t2 where a <= b;
select * from t2 where b <= 999;
-->=
select * from t2 where a >= b;
select * from t2 where b >= 999;
--<=>
select * from t2 where a <=> b;
select * from t2 where b <=> 999;

--+
select a + b from t2;
select a + 11 from t2;
---
select b - a from t2;
select b - 11 from t2;
-- *
select b * a from t2;
select b * 11 from t2;

--"&"
select b & a from t2;
select b & 11 from t2;

--"|"
select b | a from t2;
select b | 11 from t2;

--"~"
select ~b from t2; --18446744073709550616

--"^"
select b ^ a from t2;
select b ^ 11 from t2;

--">>"
select b >> a from t2;
select b >> 11 from t2;
select b >> 2 from t2;

--"<<"
select b << a from t2;
select b << 11 from t2;
select b << 2 from t2;

--"&&"
select b && a from t2;
select b && 11 from t2;

--"||"
select b or a from t2;
select b or 11::boolean from t2;

--"!"
select !b from t2;

--"and"
select b and a from t2;
select b and 11::boolean from t2;

--"or"
select b or a from t2;
select b or 11::boolean from t2;

--"xor"
select b xor a from t2;
select b xor 11::boolean from t2;

set sql_mode to default;
insert into t2 values(555, 0);
-- /
select b / a from t2;
select b / 11 from t2;
select a / b from t2;
select 0 / a from t2;

--div
select b div a from t2;
select b div 11 from t2;
select a div b from t2;
select 0 div a from t2;

-- %
select b % a from t2;
select b % 11 from t2;
select a % b from t2;
select 0 % a from t2;

--mod
select b mod a from t2;
select b mod 11 from t2;
select a mod b from t2;
select 0 mod a from t2;

set sql_mode='';
-- /
select b / a from t2;
select b / 11 from t2;
select a / b from t2;
select 0 / a from t2;

--div
select b div a from t2;
select b div 11 from t2;
select a div b from t2;
select 0 div a from t2;

-- %
select b % a from t2;
select b % 11 from t2;
select a % b from t2;
select 0 % a from t2;

--mod
select b mod a from t2;
select b mod 11 from t2;
select a mod b from t2;
select 0 mod a from t2;

drop table t2;

set SQL_MODE='TRADITIONAL';

CREATE TABLE t3 (a mediumint UNSIGNED, b numeric);
insert into t3 values (16777215, 11122333.8080666);
insert into t3 values (10020065, 123456.8742);
insert into t3 values (10020065, -123456.8742);
select  a / b from t3;
select  a % b from t3;
drop table t3;

select cast(600 as uint3);
select cast(16777215 as uint3);
select cast(16777216 as uint3);
select cast(99999999 as uint3);
select cast(123.456 as uint3);
select cast(0 as uint3);
select cast(-7200.145 as uint3);
select cast(-8388609 as uint3);
select cast(-99999999 as uint3);

set sql_mode='';

select cast(16777216 as uint3);
select cast(99999999 as uint3);
select cast(-836 as uint3);
select cast(-99999999 as uint3);

-- between and
create table t1 (a mediumint UNSIGNED , b mediumint UNSIGNED);
insert into t1 values(111, 444);
insert into t1 values(222, 444);
insert into t1 values(333, 444);
insert into t1 values(444, 444);
insert into t1 values(555, 444);
select * from t1 where a between 222 and 444;
select * from t1 where b between a and 555;

drop table t1;

-- in
create table t1(a mediumint UNSIGNED);
insert into t1 values(111),(222),(333);
select * from t1 where a in (112,222,444);
select * from t1 where a not in (112,222,444);
drop table t1;

-- is
CREATE TABLE t1 (a mediumint UNSIGNED, b mediumint UNSIGNED);
INSERT INTO t1 VALUES (111,1), (222,2);
SELECT * FROM t1 WHERE a IS true;
SELECT * FROM t1 WHERE a IS not true;
SELECT * FROM t1 WHERE a IS NULL;
SELECT * FROM t1 WHERE a IS not NULL;
drop table t1;

-- like
create table t1 (a mediumint UNSIGNED);
insert into t1 values(111);
insert into t1 values(222);
insert into t1 values(333);

set SQL_MODE='TRADITIONAL';
select a from t1 where a like '111'; --1
select a from t1 where a like 111; --0
select a from t1 where a like 0111;
select a from t1 where a like 'a111'; --0
select a from t1 where a like 1111; --0
select a from t1 where a like '111a'; --0
set SQL_MODE='';
select a from t1 where a like '111'; --1
select a from t1 where a like 111; --0
select a from t1 where a like 0111;
select a from t1 where a like 'a111'; --0
select a from t1 where a like 1111; --0
select a from t1 where a like '111a'; --0
set SQL_MODE=default;
drop table t1;

--btree and hash index
create table idx_1 (idx mediumint UNSIGNED);
create index idx1 on idx_1 using btree (idx);
insert into idx_1 values (111),(222),(333);
vacuum full analyze idx_1;
explain (COSTS OFF) select * from idx_1 where idx < 111;
explain (COSTS OFF) select * from idx_1 where idx > 222;
explain (COSTS OFF) select * from idx_1 where idx = 333;
explain (COSTS OFF) select * from idx_1 where idx <= 444;
explain (COSTS OFF) select * from idx_1 where idx >= 555;

create table idx_2 (idx mediumint UNSIGNED);
create index idx2 on idx_2 using hash (idx);
vacuum full analyze idx_2;
insert into idx_2 values (111),(222),(333);
explain (COSTS OFF) select * from idx_2 where idx = 111;
explain (COSTS OFF) select * from idx_2 where idx > 222;

drop table idx_1,idx_2;

create table tb1 (a mediumint UNSIGNED);
create or replace procedure proc1()
as
$$
declare
i int := 0;
begin
    while i <= 10000 loop
        insert into tb1 (a) values (i);
        i = i+1;
    end loop;
end;
$$ language plmysql;
call proc1();
create index idx1 on tb1 using hash(a);
vacuum full analyze tb1;
explain (COSTS OFF) select * from tb1 where a = 10;
drop index idx1;

create index idx_tree on tb1(a);
vacuum full analyze tb1;
set enable_bitmapscan =false;
set enable_seqscan =false;
explain (COSTS OFF) select * from tb1 where a < 10;
explain (COSTS OFF) select * from tb1 where a <= 10;
explain (COSTS OFF) select * from tb1 where a = 10;
explain (COSTS OFF) select * from tb1 where a != 10;
explain (COSTS OFF) select * from tb1 where a >= 10;
explain (COSTS OFF) select * from tb1 where a > 10;
set enable_seqscan =true;
set enable_bitmapscan =true;
drop procedure proc1;
drop table tb1;


CREATE TABLE t1(m1 mediumint UNSIGNED);
INSERT INTO t1 VALUES (1901),(2155),(0000);
SELECT * FROM t1;
SELECT COUNT(*) AS total_rows, MIN(m1) AS min_value, MAX(m1) AS max_value FROM t1;
SELECT AVG(m1) AS avg_value FROM t1;
SELECT SUM(m1) AS sum_value FROM t1;

DROP TABLE t1;
create table t1(id int3 unsigned);
insert into t1 values (1),(null);
select * from t1;
select avg(id) from t1;
DROP TABLE t1;

create table t1 (col_uint int3 unsigned);
insert into t1 values (100);
select -col_uint from t1;
DROP TABLE t1;

set enable_seqscan=off;
set enable_bitmapscan =off;
create table t_index(id int3 unsigned ,description varchar(100));
insert into t_index (id, description) values(generate_series(0,167772),'aa');
create index index_id_btree on t_index (id);
explain (COSTS OFF) select * from t_index where id BETWEEN 65 and 70;
drop table t_index;
set enable_seqscan=on;
set enable_bitmapscan =on;

create table t_conversion (id int1 unsigned primary key ,age tinyint unsigned default 18);
insert into t_conversion (id) values (cast('127' as varchar));
select * from t_conversion;
drop table t_conversion;

create table t_conversion (id int1 unsigned primary key ,age tinyint unsigned default 18);
insert into t_conversion (id) values (cast('127' as text));
select * from t_conversion;
drop table t_conversion;

create table t_conversion (id int1 unsigned primary key ,age tinyint unsigned default 18);
insert into t_conversion (id) values (TRUE);
insert into t_conversion (id) values (0x12);
select id from t_conversion;
drop table t_conversion;

create table t_like_operator(id int3 unsigned);
insert into t_like_operator values(101),(102),(103),(104),(205),(208);
select * from t_like_operator where id like '1%';
select * from t_like_operator where id like '%4';
select * from t_like_operator where id like '%1%';
select * from t_like_operator where id like '1_3';
select * from t_like_operator where id like '_8';

select * from t_like_operator where id not like '1%';
select * from t_like_operator where id not like '%4';
select * from t_like_operator where id not like '%1%';
select * from t_like_operator where id not like '1_3';
select * from t_like_operator where id not like '_8';
drop table t_like_operator;

create table int3_test (a int3, b int3);
insert into int3_test values (10, 3);
select pg_typeof(a + b) from int3_test; -- int4
select pg_typeof(a - b) from int3_test; -- int4
select pg_typeof(+b) from int3_test;    -- int3
select pg_typeof(-b) from int3_test;    -- int4
drop table int3_test;

/* Begin - bug#z5393 */
create table t1 (
    a int3,
    b mediumint signed
);
insert into t1 values (5, 0),(10, 5);
select a / b from t1;
drop table t1;
/* End - bug#z5393 */
