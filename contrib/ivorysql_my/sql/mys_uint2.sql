set SQL_MODE='TRADITIONAL';

--smallint
create table t1(c1 smallint UNSIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-2147483648);
insert into t1 values(2147483647);
insert into t1 values(-2147483649);
insert into t1 values(4294967295);
insert into t1 values('a111') ;
insert into t1 values('111a') ;
insert into t1 values('.111') ;
insert into t1 values('0111') ;
insert into t1 values('#111') ;
insert into t1 values('111#') ;
insert into t1 values('111.') ;
select * from t1;
drop table t1;

create table t1(c1 smallint UNSIGNED);
insert into t1 values(1.4);
insert into t1 values(1.5);
insert into t1 values('1.4');
insert into t1 values('1.5');
select * from t1;
drop table t1;

CREATE TABLE t2 (a smallint UNSIGNED, b numeric);
insert into t2 values (65535, 11122333.8080666);
insert into t2 values (20000, 123456.8742);
insert into t2 values (20000, -123456.8742);
select * from t2;
drop table t2;

select cast(12 as uint2);
select cast(65535 as uint2);
select cast(65536 as uint2);
select cast(99999999 as uint2);
select cast(123.456 as uint2);
select cast(0 as uint2);
select cast(-72.145 as uint2);
select cast(-32768.123 as uint2);
select cast(-99999999 as uint2);

--smallint[(M)]
create table t1 (a smallint[-1] UNSIGNED); --fail

create table t2 (a smallint(0) UNSIGNED);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a smallint(3) UNSIGNED);
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t3 (a smallint(4) UNSIGNED);
\d t3
insert into t3 values (3);
select * from t3;
drop table t3;

create table t2 (a smallint(3.4) UNSIGNED);
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t2 (a smallint(3.5) UNSIGNED);
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t2(age smallint(256.4) unsigned);
create table t2(id1 int2(255.9) unsigned, id2 int2(254.9) unsigned, age smallint(255.4) unsigned);
\d t2
insert into t2 values (2, 5, 44);
select * from t2;
drop table t2;

--smallint zerofill
create table t2 (a smallint(-1) ZEROFILL);

create table t2 (a smallint ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a smallint(2) ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

--int2
create table t1(c1 int2 UNSIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-2147483648);
insert into t1 values(2147483647);
insert into t1 values(-2147483649);
insert into t1 values(4294967295);
select * from t1;
drop table t1;

create table t1(c1 int2 UNSIGNED);
insert into t1 values(1.4);
insert into t1 values(1.5);
insert into t1 values('1.4');
insert into t1 values('1.5');
select * from t1;
drop table t1;

--smallint[(M)]
create table t1 (a int2[-1] UNSIGNED); --fail

create table t2 (a int2(0) UNSIGNED);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a int2(3) UNSIGNED);
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t3 (a int2(4) UNSIGNED);
\d t3
insert into t3 values (3);
select * from t3;
drop table t3;

--smallint zerofill
create table t2 (a int2(-1) ZEROFILL);

create table t2 (a int2 ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a int2(2) ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

set SQL_MODE='';

--smallint
create table t1(c1 smallint UNSIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-2147483648);
insert into t1 values(2147483647);
insert into t1 values(-2147483649);
insert into t1 values(4294967295);
insert into t1 values('a111') ;
insert into t1 values('111a') ;
insert into t1 values('.111') ;
insert into t1 values('0111') ;
insert into t1 values('#111') ;
insert into t1 values('111#') ;
insert into t1 values('111.') ;
select * from t1;
drop table t1;

--smallint[(M)]
create table t1 (a smallint[-1] UNSIGNED ); --fail

create table t2 (a smallint(0) UNSIGNED);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a smallint(3) UNSIGNED);
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t3 (a smallint(4) UNSIGNED);
\d t3
insert into t3 values (3);
select * from t3;
drop table t3;

--smallint zerofill
create table t2 (a smallint(-1) ZEROFILL);

create table t2 (a smallint ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a smallint(2) ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

--int2
create table t1(c1 int2 UNSIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-2147483648);
insert into t1 values(2147483647);
insert into t1 values(-2147483649);
insert into t1 values(4294967295);
select * from t1;
drop table t1;

--smallint[(M)]
create table t1 (a int2[-1] UNSIGNED); --fail

create table t2 (a int2(0) UNSIGNED);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a int2(3) UNSIGNED);
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t3 (a int2(4) UNSIGNED);
\d t3
insert into t3 values (3);
select * from t3;
drop table t3;

--smallint zerofill
create table t2 (a int2(-1) ZEROFILL);

create table t2 (a int2 ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a int2(2) ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

--numeric -> uint2
select 2000.49999::uint2;
select 2000.5::uint2;
select 65534.6::uint2;
select 65536.3::uint2;
select 999999999999999.0205::uint2;
select (-0.02)::uint2;
select (-68.753951)::uint2;
select (-32767.5)::uint2;
select (-32769.1)::uint2;
select (-999999999999999.0205)::uint2;

set SQL_MODE='TRADITIONAL';
CREATE TABLE tb2 (a numeric, b smallint UNSIGNED);
insert into tb2 values (1122.556677, 9999);
insert into tb2 values (0.62, 100);
insert into tb2 values (7.05, 1000.123);
insert into tb2 values (7.05, 65534.6);
insert into tb2 values (7.05, 65535.0);
insert into tb2 values (7.05, 65536.3);
insert into tb2 values (7.05, 999999999999999.0205);
insert into tb2 values (120.007, 0);
insert into tb2 values (9.36, -50000.753951);
insert into tb2 values (9.36, -32767.5);
insert into tb2 values (9.36, -32768.0);
insert into tb2 values (9.36, -32769.1);
insert into tb2 values (9.36, -999999999999999.0205);
select * from tb2;
select cast(a as uint2) from tb2;
select convert(a, uint2) from tb2;
select a::uint2 + b from tb2;
drop table tb2;

set SQL_MODE='';
CREATE TABLE tb2 (a numeric, b smallint UNSIGNED);
insert into tb2 values (1122.556677, 9999);
insert into tb2 values (0.62, 100);
insert into tb2 values (7.05, 1000.123);
insert into tb2 values (7.05, 65534.6);
insert into tb2 values (7.05, 65535.0);
insert into tb2 values (7.05, 65536.3);
insert into tb2 values (7.05, 999999999999999.0205);
insert into tb2 values (120.007, 0);
insert into tb2 values (9.36, -50000.753951);
insert into tb2 values (9.36, -32767.5);
insert into tb2 values (9.36, -32768.0);
insert into tb2 values (9.36, -32769.1);
insert into tb2 values (9.36, -999999999999999.0205);
select * from tb2;
select cast(a as uint2) from tb2;
select convert(a, uint2) from tb2;
select a::uint2 + b from tb2;
drop table tb2;

set SQL_MODE to default;

--uint2 -> signed/unsigned (int8/uint8)
select 1000::uint2::int8;
select 1000::uint2::uint8;
select convert(10000::uint2, signed);
select cast(10000::uint2 as unsigned);

create table test1(a smallint unsigned);
insert into test1 values(5000);
select convert(a, signed) from test1;
select cast(a as unsigned) from test1;
drop table test1;

-- oper
--------------------------------------------------
CREATE TABLE t2 (a smallint UNSIGNED, b smallint UNSIGNED);
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

-- between and
create table t1 (a smallint UNSIGNED , b smallint UNSIGNED);
insert into t1 values(111, 444);
insert into t1 values(222, 444);
insert into t1 values(333, 444);
insert into t1 values(444, 444);
insert into t1 values(555, 444);
select * from t1 where a between 222 and 444;
select * from t1 where b between a and 555;

drop table t1;

-- in
create table t1(a smallint UNSIGNED);
insert into t1 values(111),(222),(333);
select * from t1 where a in (112,222,444);
select * from t1 where a not in (112,222,444);
drop table t1;

-- is
CREATE TABLE t1 (a smallint UNSIGNED, b smallint UNSIGNED);
INSERT INTO t1 VALUES (111,1), (222,2);
SELECT * FROM t1 WHERE a IS true;
SELECT * FROM t1 WHERE a IS not true;
SELECT * FROM t1 WHERE a IS NULL;
SELECT * FROM t1 WHERE a IS not NULL;
drop table t1;

-- like
create table t1 (a smallint UNSIGNED);
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
create table idx_1 (idx smallint UNSIGNED);
create index idx1 on idx_1 using btree (idx);
insert into idx_1 values (111),(222),(333);
vacuum full analyze idx_1;
explain (COSTS OFF) select * from idx_1 where idx < 111;
explain (COSTS OFF) select * from idx_1 where idx > 222;
explain (COSTS OFF) select * from idx_1 where idx = 333;
explain (COSTS OFF) select * from idx_1 where idx <= 444;
explain (COSTS OFF) select * from idx_1 where idx >= 555;

create table idx_2 (idx smallint UNSIGNED);
create index idx2 on idx_2 using hash (idx);
vacuum full analyze idx_2;
insert into idx_2 values (111),(222),(333);
explain (COSTS OFF) select * from idx_2 where idx = 111;
explain (COSTS OFF) select * from idx_2 where idx > 222;

drop table idx_1,idx_2;

create table tb1 (a smallint UNSIGNED);
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


CREATE TABLE t1(m1 smallint UNSIGNED);
INSERT INTO t1 VALUES (1901),(2155),(0000);
SELECT * FROM t1;
SELECT COUNT(*) AS total_rows, MIN(m1) AS min_value, MAX(m1) AS max_value FROM t1;
SELECT AVG(m1) AS avg_value FROM t1;
SELECT SUM(m1) AS sum_value FROM t1;
DROP TABLE t1;

create table t1(id int2 unsigned);
insert into t1 values (1),(null);
select * from t1;
select avg(id) from t1;
DROP TABLE t1;

create table t_aggregate (value int2 unsigned);
insert into t_aggregate (value) values (128), (100), (0), (50), (100), (255), (32767), (65535);
select avg(value), SUM(value) from t_aggregate;
DROP TABLE t_aggregate;

create table t1 (col_uint int2 unsigned);
insert into t1 values (100);
insert into t1 values(65535),(65534),(65530);
select -col_uint from t1;
DROP TABLE t1;

set enable_seqscan=off;
set enable_bitmapscan =off;
create table t_index(id int2 unsigned ,description varchar(100));
insert into t_index (id, description) values(generate_series(0,1672),'aa');
create index index_id_btree on t_index (id);
explain (COSTS OFF) select * from t_index where id BETWEEN 65 and 70;
explain (COSTS OFF) select * from t_index where id BETWEEN 65::uint2 and 70::uint2;
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

create table t_like_operator(id int2 unsigned);
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

/* Begin - bug#z5166 */
create table t_div(id int1, name int2);
insert into t_div values(10, 10);
select id /3, name / 3 from t_div;
drop table t_div;
/* End - bug#z5166 */
