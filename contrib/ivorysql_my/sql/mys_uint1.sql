set SQL_MODE='TRADITIONAL';

--tinyint
create table t1(c1 tinyint UNSIGNED);
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
insert into t1 values('111.1') ;
insert into t1 values(111.1) ;
select * from t1;
drop table t1;

create table t1(c1 tinyint UNSIGNED);
insert into t1 values(1.4);
insert into t1 values(1.5);
insert into t1 values('1.4');
insert into t1 values('1.5');
select * from t1;
drop table t1;

CREATE TABLE t1 (a tinyint UNSIGNED, b numeric);
insert into t1 values (255, 11122333.8080666);
insert into t1 values (60, 123456.8742);
insert into t1 values (60, -123456.8742);
select * from t1;
drop table t1;

select cast(10 as uint1);
select cast(255 as uint1);
select cast(256 as uint1);
select cast(9999 as uint1);
select cast(123.456 as uint1);
select cast(0 as uint1);
select cast(-7200.145 as uint1);
select cast(-9999 as uint1);

--tinyint[(M)]
create table t1 (a tinyint[-1] UNSIGNED); --fail

create table t2 (a tinyint(0) UNSIGNED);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a tinyint(3) UNSIGNED);
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t3 (a tinyint(4) UNSIGNED);
\d t3
insert into t3 values (3);
select * from t3;
drop table t3;

create table t2 (a tinyint(3.4) UNSIGNED);
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t2 (a tinyint(3.5) UNSIGNED);
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t2(age tinyint(256.4) unsigned);
create table t2(id1 int1(255.9) unsigned, id2 int1(254.9) unsigned, age tinyint(255.4) unsigned);
\d t2
insert into t2 values (2, 5, 44);
select * from t2;
drop table t2;

--tinyint zerofill
create table t2 (a tinyint(-1) ZEROFILL);

create table t2 (a tinyint ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a tinyint(2) ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

--int1
create table t1(c1 int1 UNSIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-2147483648);
insert into t1 values(2147483647);
insert into t1 values(-2147483649);
insert into t1 values(4294967295);
select * from t1;
drop table t1;

create table t1(c1 int1 UNSIGNED);
insert into t1 values(1.4);
insert into t1 values(1.5);
insert into t1 values('1.4');
insert into t1 values('1.5');
select * from t1;
drop table t1;
--int1[(M)]
create table t1 (a int1[-1] UNSIGNED); --fail

create table t2 (a int1(0) UNSIGNED);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a int1(3) UNSIGNED);
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t3 (a int1(4) UNSIGNED);
\d t3
insert into t3 values (3);
select * from t3;
drop table t3;

--tinyint zerofill
create table t2 (a int1(-1) ZEROFILL);

create table t2 (a int1 ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a int1(2) ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

set SQL_MODE='';
--tinyint
create table t1(c1 tinyint UNSIGNED);
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
insert into t1 values('111.1') ;
insert into t1 values(111.1) ;
select * from t1;
drop table t1;

--tinyint[(M)]
create table t1 (a tinyint[-1] UNSIGNED ); --fail

create table t2 (a tinyint(0) UNSIGNED);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a tinyint(3) UNSIGNED);
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t3 (a tinyint(4) UNSIGNED);
\d t3
insert into t3 values (3);
select * from t3;
drop table t3;

--tinyint zerofill
create table t2 (a tinyint(-1) ZEROFILL);

create table t2 (a tinyint ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a tinyint(2) ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

--int1
create table t1(c1 int1 UNSIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-2147483648);
insert into t1 values(2147483647);
insert into t1 values(-2147483649);
insert into t1 values(4294967295);
select * from t1;
drop table t1;

--int1[(M)]
create table t1 (a int1[-1] UNSIGNED ); --fail

create table t2 (a int1(0) UNSIGNED);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a int1(3) UNSIGNED);
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t3 (a int1(4) UNSIGNED);
\d t3
insert into t3 values (3);
select * from t3;
drop table t3;

--int1 zerofill
create table t2 (a int1(-1) ZEROFILL);

create table t2 (a int1 ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a int1(2) ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

--numeric -> uint1
select 200.49999::uint1;
select 200.5::uint1;
select 254.3::uint1;
select 255.1::uint1;
select 999999999999999.0205::uint1;
select (-0.02)::uint1;
select (-68.753951)::uint1;
select (-127.9)::uint1;
select (-129.2)::uint1;
select (-999999999999999.0205)::uint1;

set SQL_MODE='TRADITIONAL';
CREATE TABLE tb1 (a numeric, b tinyint UNSIGNED);
insert into tb1 values (11.556677, 20);
insert into tb1 values (0.62, 50);
insert into tb1 values (7.05, 18.123);
insert into tb1 values (7.05, 254.3);
insert into tb1 values (7.05, 255.0);
insert into tb1 values (7.05, 256.1);
insert into tb1 values (7.05, 999999999999999.0205);
insert into tb1 values (120.007, 0);
insert into tb1 values (9.36, -50000.753951);
insert into tb1 values (9.36, -127.9);
insert into tb1 values (9.36, -128.0);
insert into tb1 values (9.36, -129.2);
insert into tb1 values (9.36, -999999999999999.0205);
select * from tb1;
select cast(a as uint1) from tb1;
select convert(a, uint1) from tb1;
select a::uint1 + b from tb1;
drop table tb1;

set SQL_MODE='';
CREATE TABLE tb1 (a numeric, b tinyint UNSIGNED);
insert into tb1 values (11.556677, 20);
insert into tb1 values (0.62, 50);
insert into tb1 values (7.05, 18.123);
insert into tb1 values (7.05, 254.3);
insert into tb1 values (7.05, 255.0);
insert into tb1 values (7.05, 256.1);
insert into tb1 values (7.05, 999999999999999.0205);
insert into tb1 values (120.007, 0);
insert into tb1 values (9.36, -50000.753951);
insert into tb1 values (9.36, -127.9);
insert into tb1 values (9.36, -128.0);
insert into tb1 values (9.36, -129.2);
insert into tb1 values (9.36, -999999999999999.0205);
select * from tb1;
select cast(a as uint1) from tb1;
select convert(a, uint1) from tb1;
select a::uint1 + b from tb1;
drop table tb1;

set SQL_MODE to default;
-- oper
--------------------------------------------------
CREATE TABLE t2 (a tinyint UNSIGNED, b tinyint UNSIGNED);
insert into t2 values(10, 99);
insert into t2 values(22, 99);
insert into t2 values(33, 99);
insert into t2 values(44, 99);
insert into t2 values(55, 10);

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
select b - 1 from t2;
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
insert into t2 values(55, 0);
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
create table t1 (a tinyint UNSIGNED , b tinyint UNSIGNED);
insert into t1 values(11, 44);
insert into t1 values(22, 44);
insert into t1 values(33, 44);
insert into t1 values(44, 44);
insert into t1 values(55, 44);
select * from t1 where a between 22 and 44;
select * from t1 where b between a and 55;

drop table t1;

-- in
create table t1(a tinyint UNSIGNED);
insert into t1 values(11),(22),(33);
select * from t1 where a in (12,22,44);
select * from t1 where a not in (12,22,44);
drop table t1;

-- is
CREATE TABLE t1 (a tinyint UNSIGNED, b tinyint UNSIGNED);
INSERT INTO t1 VALUES (11,1), (22,2);
SELECT * FROM t1 WHERE a IS true;
SELECT * FROM t1 WHERE a IS not true;
SELECT * FROM t1 WHERE a IS NULL;
SELECT * FROM t1 WHERE a IS not NULL;
drop table t1;

-- like
create table t1 (a tinyint UNSIGNED);
insert into t1 values(11);
insert into t1 values(22);
insert into t1 values(33);

set SQL_MODE='TRADITIONAL';
select a from t1 where a like '11'; --1
select a from t1 where a like 11; --0
select a from t1 where a like 011;
select a from t1 where a like 'a11'; --0
select a from t1 where a like 111; --0
select a from t1 where a like '11a'; --0
set SQL_MODE='';
select a from t1 where a like '11'; --1
select a from t1 where a like 11; --0
select a from t1 where a like 011;
select a from t1 where a like 'a11'; --0
select a from t1 where a like 111; --0
select a from t1 where a like '11a'; --0
set SQL_MODE=default;
drop table t1;

--btree and hash index
create table idx_1 (idx tinyint UNSIGNED);
create index idx1 on idx_1 using btree (idx);
insert into idx_1 values (11),(22),(33);
vacuum full analyze idx_1;
explain (COSTS OFF) select * from idx_1 where idx < 11;
explain (COSTS OFF) select * from idx_1 where idx > 22;
explain (COSTS OFF) select * from idx_1 where idx = 33;
explain (COSTS OFF) select * from idx_1 where idx <= 44;
explain (COSTS OFF) select * from idx_1 where idx >= 55;

create table idx_2 (idx tinyint UNSIGNED);
create index idx2 on idx_2 using hash (idx);
vacuum full analyze idx_2;
insert into idx_2 values (11),(22),(33);
set enable_bitmapscan =false;
set enable_seqscan =false;
explain (COSTS OFF) select * from idx_2 where idx = cast(11 as tinyint UNSIGNED);
explain (COSTS OFF) select * from idx_2 where idx > cast(11 as tinyint UNSIGNED);
set enable_seqscan =true;
set enable_bitmapscan =true;

drop table idx_1,idx_2;

create table tb1 (a tinyint UNSIGNED);
create or replace procedure proc1()
as
$$
declare
i int := 0;
begin
    while i <= 200 loop
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


CREATE TABLE t1(m1 tinyint UNSIGNED);
INSERT INTO t1 VALUES (11),(25),(00);
SELECT * FROM t1;
SELECT COUNT(*) AS total_rows, MIN(m1) AS min_value, MAX(m1) AS max_value FROM t1;
SELECT AVG(m1) AS avg_value FROM t1;
SELECT SUM(m1) AS sum_value FROM t1;
DROP TABLE t1;

create table t1(id int1 unsigned);
insert into t1 values (1),(null);
select * from t1;
select avg(id) from t1;
DROP TABLE t1;

create table t_aggregate (value INT1 unsigned);
insert into t_aggregate (value) values (128), (100), (0), (50), (100), (255);
select avg(value), SUM(value) from t_aggregate;
DROP TABLE t_aggregate;

create table t1 (col_uint int1 unsigned);
insert into t1 values (100);
insert into t1 values(254),(255),(250);
select -col_uint from t1;
insert into t1 values (256);
select -col_uint from t1;
DROP TABLE t1;

set enable_seqscan=off;
set enable_bitmapscan =off;
create table t_index(id int1 unsigned ,description varchar(100));
insert into t_index (id, description) values(generate_series(0,167),'aa');
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

create table t_like_operator(id int1 unsigned);
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