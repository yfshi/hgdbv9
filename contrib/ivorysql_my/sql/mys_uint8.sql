set SQL_MODE='TRADITIONAL';
select cast(1111 as UNSIGNED) ; -- 1111
select cast(1111.4 as UNSIGNED) ; -- 1111
select cast(1111.8 as UNSIGNED) ; --1112
select cast('1111.4' as UNSIGNED) ; -- 1111
select cast('1111.8' as UNSIGNED) ; -- 1111
select cast(4294967297.8 as UNSIGNED) ; --4294967298
select cast('1111' as UNSIGNED) ; --1111
select cast('4294967295' as UNSIGNED) ; --4294967295
select cast('4294967297' as UNSIGNED) ; --4294967297
select cast('a111' as UNSIGNED) ; --0
select cast('111a' as UNSIGNED) ; --111
select cast('.111' as UNSIGNED) ; --0
select cast('0111' as UNSIGNED) ; --111
select cast('#111' as UNSIGNED) ; --0
select cast('111#' as UNSIGNED) ; --111
select cast('111.' as UNSIGNED) ; --111
select cast(42222222222222294967297.8 as UNSIGNED) ; --18446744073709551615
select cast(42222222222222294967297 as UNSIGNED) ; --18446744073709551615
select cast(9223372036854775808 as UNSIGNED) ; --9223372036854775808

set SQL_MODE='';
select cast(1111 as UNSIGNED) ; --1111
select cast(1111.4 as UNSIGNED) ; -- 1111
select cast(1111.8 as UNSIGNED) ; --1112
select cast('1111.4' as UNSIGNED) ; -- 1111
select cast('1111.8' as UNSIGNED) ; -- 1111
select cast(4294967297.8 as UNSIGNED) ; --4294967298
select cast('1111' as UNSIGNED) ; --1111
select cast('4294967295' as UNSIGNED) ; --4294967295
select cast('4294967297' as UNSIGNED) ; --4294967297
select cast('a111' as UNSIGNED) ; --0
select cast('111a' as UNSIGNED) ; --111
select cast('.111' as UNSIGNED) ; --0
select cast('0111' as UNSIGNED) ; --111
select cast('#111' as UNSIGNED) ; --0
select cast('111#' as UNSIGNED) ; --111
select cast('111.' as UNSIGNED) ; --111
select cast(42222222222222294967297.8 as UNSIGNED) ; --18446744073709551615
select cast(42222222222222294967297 as UNSIGNED) ; --18446744073709551615
select cast(9223372036854775808 as UNSIGNED) ; --9223372036854775808

set SQL_MODE='TRADITIONAL';
--bigint
create table t1(c1 bigint UNSIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(42222222222222294967297);
insert into t1 values(9223372036854775808);
insert into t1 values('100');
insert into t1 values('9223372036854775808');
insert into t1 values('42222222222222294967297');
select * from t1;
drop table t1;
--bigint[(M)]
create table t1 (a bigint[-1] UNSIGNED); --fail

create table t2 (a bigint(0) UNSIGNED);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a bigint(3) UNSIGNED);
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t3 (a bigint(4) UNSIGNED);
\d t3
insert into t3 values (3);
select * from t3;
drop table t3;

create table t2(age bigint(256.4) unsigned);
create table t2(id1 int8(255.9) unsigned, id2 int8(254.9) unsigned, age bigint(255.4) unsigned);
\d t2
insert into t2 values (2, 5, 44);
select * from t2;
drop table t2;

--mediumint zerofill
create table t2 (a bigint(-1) ZEROFILL);

create table t2 (a bigint ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a bigint(2) ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a bigint(2) UNSIGNED ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

--int8
create table t1(c1 int8 UNSIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(42222222222222294967297);
insert into t1 values(9223372036854775808);
insert into t1 values('100');
insert into t1 values('9223372036854775808');
insert into t1 values('42222222222222294967297');
insert into t1 values('a111') ;
insert into t1 values('111a') ;
insert into t1 values('.111') ;
insert into t1 values('0111') ;
insert into t1 values('#111') ;
insert into t1 values('111#') ;
insert into t1 values('111.') ;
select * from t1;
drop table t1;

create table t1(c1 int8 UNSIGNED);
insert into t1 values(1.4);
insert into t1 values(1.5);
insert into t1 values('1.4');
insert into t1 values('1.5');
select * from t1;
drop table t1;

--int8[(M)]
create table t1 (a int8[-1] UNSIGNED); --fail

create table t2 (a int8(0) UNSIGNED);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a int8(3) UNSIGNED);
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t3 (a int8(4) UNSIGNED);
\d t3
insert into t3 values (3);
select * from t3;
drop table t3;

--mediumint zerofill
create table t2 (a int8(-1) ZEROFILL);

create table t2 (a int8 ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a int8(2) ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a int8(2) UNSIGNED ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

set SQL_MODE='';
--bigint
create table t1(c1 bigint UNSIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(42222222222222294967297);
insert into t1 values(9223372036854775808);
insert into t1 values('100');
insert into t1 values('9223372036854775808');
insert into t1 values('42222222222222294967297');
select * from t1;
drop table t1;

create table t1(c1 bigint UNSIGNED);
insert into t1 values(1.4);
insert into t1 values(1.5);
insert into t1 values('1.4');
insert into t1 values('1.5');
select * from t1;
drop table t1;
--int8
create table t1(c1 int8 UNSIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(42222222222222294967297);
insert into t1 values(9223372036854775808);
insert into t1 values('100');
insert into t1 values('9223372036854775808');
insert into t1 values('42222222222222294967297');
select * from t1;
drop table t1;

create table t1(c1 int8 UNSIGNED);
insert into t1 values(1.4);
insert into t1 values(1.5);
insert into t1 values('1.4');
insert into t1 values('1.5');
select * from t1;
drop table t1;


set SQL_MODE to default;
-- oper
---=
select cast(111 as UNSIGNED)  = 111; --1
select cast(111 as UNSIGNED)  = -111; --0
select cast(111 as UNSIGNED)  = NULL; --NULL
select 111 = cast(111 as UNSIGNED); --1
select -111 = cast(111 as UNSIGNED); --0
select NULL = cast(111 as UNSIGNED); --NULL
--<> 
select cast(111 as UNSIGNED)  <> 111; --0
select cast(111 as UNSIGNED)  <> -111; --1
select cast(111 as UNSIGNED)  <> NULL; --NULL
select 111 <> cast(111 as UNSIGNED); --0
select -111 <> cast(111 as UNSIGNED); --1
select NULL <> cast(111 as UNSIGNED); --NULL
--<
select cast(111 as UNSIGNED)  < 111; --0
select cast(111 as UNSIGNED)  < -111; --0
select cast(111 as UNSIGNED)  < NULL;  --NULL
select 111 < cast(111 as UNSIGNED); --0
select -111 < cast(111 as UNSIGNED); --1
select NULL < cast(111 as UNSIGNED); --NULL
-->
select cast(111 as UNSIGNED)  > 111; --0
select cast(111 as UNSIGNED)  > -111;  --1
select cast(111 as UNSIGNED)  > NULL; --NULL
select 111 > cast(111 as UNSIGNED); --0
select -111 > cast(111 as UNSIGNED); --0
select NULL > cast(111 as UNSIGNED); --NULL
--<=
select cast(111 as UNSIGNED)  <= 111; --1
select cast(111 as UNSIGNED)  <= -111; --0
select cast(111 as UNSIGNED)  <= NULL; --NULL
select 111 <= cast(111 as UNSIGNED); --1
select -111 <= cast(111 as UNSIGNED); --1
select NULL <= cast(111 as UNSIGNED); --NULL
-->=
select cast(111 as UNSIGNED)  >= 111; --1
select cast(111 as UNSIGNED)  >= -111; --1
select cast(111 as UNSIGNED)  >= NULL; --NULL
select 111 >= cast(111 as UNSIGNED); --1
select -111 >= cast(111 as UNSIGNED); --0
select NULL >= cast(111 as UNSIGNED); --NULL
--<=>
select cast(111 as UNSIGNED)  <=> 111; --1
select cast(111 as UNSIGNED)  <=> -111; --0
select cast(111 as UNSIGNED)  <=> NULL; --0
select 111 <=> cast(111 as UNSIGNED); --1
select -111 <=> cast(111 as UNSIGNED); --0
select NULL <=> cast(111 as UNSIGNED); --0

--+
select cast(111 as UNSIGNED) + 111; --222
select cast(111 as UNSIGNED) + -111; --0
select cast(111 as UNSIGNED) + NULL; --NULL
select 111 + cast(111 as UNSIGNED); --222
select -111 + cast(111 as UNSIGNED); --0
select NULL + cast(111 as UNSIGNED); --NULL
---
select cast(111 as UNSIGNED)  - 111; --0
select cast(111 as UNSIGNED)  - -111; --222
select cast(111 as UNSIGNED)  - NULL; --NULL
select 111 - cast(111 as UNSIGNED); --0
select -111 - cast(111 as UNSIGNED); --err
select NULL - cast(111 as UNSIGNED); --NULL
-- *
select cast(111 as UNSIGNED)  * 111; --12321
select cast(111 as UNSIGNED)  * -111; --err
select cast(111 as UNSIGNED)  * NULL; --NULL
select 111 * cast(111 as UNSIGNED);--12321
select -111 * cast(111 as UNSIGNED); --err
select NULL * cast(111 as UNSIGNED); --NULL
select 4294967295 * cast(4294967295 as UNSIGNED);--18446744065119617025

-- /
select cast(111 as UNSIGNED)  / 111; --1.0000
select cast(111 as UNSIGNED)  / 110; --1.0091
select cast(111 as UNSIGNED)  / -111; -- -1.0000
select cast(111 as UNSIGNED)  / NULL; --NULL
select 111 / cast(111 as UNSIGNED); --1.0000
select 110 / cast(111 as UNSIGNED); --0.9910
select -111 / cast(111 as UNSIGNED); ---1.0000
select NULL / cast(111 as UNSIGNED); --NULL

--div
select cast(111 as UNSIGNED) div 111; --1
select cast(111 as UNSIGNED) div 110; --1
select cast(111 as UNSIGNED) div -111; --err
select cast(111 as UNSIGNED) div NULL; --NULL
select 111 div cast(111 as UNSIGNED); --1
select 110 div cast(111 as UNSIGNED); --0
select -111 div cast(111 as UNSIGNED); --err
select NULL div cast(111 as UNSIGNED); --NULL

-- %
select cast(111 as UNSIGNED) % 111; --0
select cast(111 as UNSIGNED) % -111; --0
select cast(111 as UNSIGNED) % NULL; --NULL
select 111 % cast(111 as UNSIGNED); --0
select -111 % cast(111 as UNSIGNED); --0
select NULL % cast(111 as UNSIGNED); --NULL

--mod
select cast(111 as UNSIGNED) mod 111; --0
select cast(111 as UNSIGNED) mod -111; --0
select cast(111 as UNSIGNED) mod NULL; --NULL
select 111 mod cast(111 as UNSIGNED); --0
select -111 mod cast(111 as UNSIGNED); --0
select NULL mod cast(111 as UNSIGNED); --NULL

set sql_mode to default;
select cast(111 as UNSIGNED)  / 0; --NULL warning
select cast(111 as UNSIGNED) div 0; --NULL warning
select cast(111 as UNSIGNED) % 0; --NULL warning
select cast(111 as UNSIGNED) mod 0; --NULL warning
set sql_mode= '';
select cast(111 as UNSIGNED)  / 0; --NULL
select cast(111 as UNSIGNED) div 0; --NULL
select cast(111 as UNSIGNED) % 0; --NULL
select cast(111 as UNSIGNED) mod 0; --NULL

--"&"
select cast(111 as UNSIGNED) & 111; --111
select cast(111 as UNSIGNED) & -111; --1
select cast(111 as UNSIGNED) & NULL; --NULL
select 111 & cast(111 as UNSIGNED); --111
select -111 & cast(111 as UNSIGNED); --1
select NULL & cast(111 as UNSIGNED);--NULL

--"|"
select cast(111 as UNSIGNED)  | 111; --111
select cast(111 as UNSIGNED)  | -111; --18446744073709551615
select cast(111 as UNSIGNED)  | NULL; --NULL
select 111 | cast(111 as UNSIGNED); --111
select -111 | cast(111 as UNSIGNED); --18446744073709551615
select NULL | cast(111 as UNSIGNED); --NULL

--"~"
select ~cast(111 as UNSIGNED); --18446744073709551504

--"^"
select cast(111 as UNSIGNED)  ^ 111; --0
select cast(111 as UNSIGNED)  ^ -111; --18446744073709551614
select cast(111 as UNSIGNED)  ^ NULL; --NULL
select 111 ^ cast(111 as UNSIGNED); --0
select -111 ^ cast(111 as UNSIGNED);  --18446744073709551614
select NULL ^ cast(111 as UNSIGNED); --NULL

--">>"
select cast(111 as UNSIGNED) >> 111;--0
select cast(111 as UNSIGNED) >> -111;--0
select cast(111 as UNSIGNED) >> NULL; --NULL
select 111 >> cast(111 as UNSIGNED);--0
select -111 >> cast(111 as UNSIGNED);--0
select NULL >> cast(111 as UNSIGNED); --NULL

--"<<"
select cast(111 as UNSIGNED) << 111;--0
select cast(111 as UNSIGNED) << -111;--0
select cast(111 as UNSIGNED) << NULL;--NULL
select 111 << cast(111 as UNSIGNED);--0
select -111 << cast(111 as UNSIGNED);--0
select NULL << cast(111 as UNSIGNED);--NULL

--"&&"
select cast(111 as UNSIGNED) && 111;--1
select cast(111 as UNSIGNED) && -111;--1
select cast(111 as UNSIGNED) && NULL;--NULL
select 111 && cast(111 as UNSIGNED);--1
select -111 && cast(111 as UNSIGNED);--1
select NULL && cast(111 as UNSIGNED);--NULL

--"||"
select cast(111 as UNSIGNED) || true;--1
select cast(111 as UNSIGNED) || false;--1
select true || cast(111 as UNSIGNED);--1
select false || cast(111 as UNSIGNED);--1

--"!"
select !cast(111 as UNSIGNED); --0

--"and"
select cast(111 as UNSIGNED)  and 111::boolean;--1
select cast(111 as UNSIGNED)  and (-111)::boolean;--1
select cast(111 as UNSIGNED)  and NULL;--NULL
select 111::boolean and cast(111 as UNSIGNED);--1
select (-111)::boolean and cast(111 as UNSIGNED);--1
select NULL and cast(111 as UNSIGNED);--NULL

--"or"
select cast(111 as UNSIGNED) or 111::boolean;--1
select cast(111 as UNSIGNED) or (-111)::boolean;--1
select cast(111 as UNSIGNED) or NULL;--1
select 111::boolean or cast(111 as UNSIGNED);--1
select (-111)::boolean or cast(111 as UNSIGNED);--1
select NULL or cast(111 as UNSIGNED);--1

--"xor"
select cast(111 as UNSIGNED) xor 111::boolean;--0
select cast(111 as UNSIGNED) xor (-111)::boolean;--0
select cast(111 as UNSIGNED) xor NULL;--NULL
select 111::boolean xor cast(111 as UNSIGNED);--0
select (-111)::boolean xor cast(111 as UNSIGNED);--0
select NULL xor cast(111 as UNSIGNED);--NULL

set SQL_MODE to default;
--------------------------------------------------
CREATE TABLE t2 (a bigint UNSIGNED, b bigint UNSIGNED);
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
create table t1 (a bigint UNSIGNED , b bigint UNSIGNED);
insert into t1 values(111, 444);
insert into t1 values(222, 444);
insert into t1 values(333, 444);
insert into t1 values(444, 444);
insert into t1 values(555, 444);
select * from t1 where a between 222 and 444;
select * from t1 where b between a and 555;

drop table t1;

-- in
create table t1(a bigint UNSIGNED);
insert into t1 values(111),(222),(333);
select * from t1 where a in (112,222,444);
select * from t1 where a not in (112,222,444);
drop table t1;

-- is
CREATE TABLE t1 (a bigint UNSIGNED, b bigint UNSIGNED);
INSERT INTO t1 VALUES (111,1), (222,2);
SELECT * FROM t1 WHERE a IS true;
SELECT * FROM t1 WHERE a IS not true;
SELECT * FROM t1 WHERE a IS NULL;
SELECT * FROM t1 WHERE a IS not NULL;
drop table t1;

-- like
create table t1 (a bigint UNSIGNED);
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
create table idx_1 (idx bigint UNSIGNED);
create index idx1 on idx_1 using btree (idx);
insert into idx_1 values (111),(222),(333);
vacuum full analyze idx_1;
explain (COSTS OFF) select * from idx_1 where idx < 111;
explain (COSTS OFF) select * from idx_1 where idx > 222;
explain (COSTS OFF) select * from idx_1 where idx = 333;
explain (COSTS OFF) select * from idx_1 where idx <= 444;
explain (COSTS OFF) select * from idx_1 where idx >= 555;

create table idx_2 (idx bigint UNSIGNED);
create index idx2 on idx_2 using hash (idx);
vacuum full analyze idx_2;
insert into idx_2 values (111),(222),(333);
explain (COSTS OFF) select * from idx_2 where idx = 111;
explain (COSTS OFF) select * from idx_2 where idx > 222;

drop table idx_1,idx_2;

create table tb1 (a bigint UNSIGNED);
create or replace procedure proc1()
as
$$
declare
i bigint := 0;
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


CREATE TABLE t1(m1 bigint UNSIGNED);
INSERT INTO t1 VALUES (1901),(2155),(0000);
SELECT * FROM t1;
SELECT COUNT(*) AS total_rows, MIN(m1) AS min_value, MAX(m1) AS max_value FROM t1;
SELECT AVG(m1) AS avg_value FROM t1;
SELECT SUM(m1) AS sum_value FROM t1;

DROP TABLE t1;
create table t1(id int8 unsigned);
insert into t1 values (1),(null);
select * from t1;
select avg(id) from t1;
DROP TABLE t1;

create table t_aggregate (value int8 unsigned);
insert into t_aggregate (value) values (128), (100), (0), (50), (100), (255), (32767), (65535), (4294967295), (18446744073709551615);
select avg(value), SUM(value) from t_aggregate;
DROP TABLE t_aggregate;

set sql_mode=default;
create table t1 (col_uint int8 unsigned);
insert into t1 values (100);
select -col_uint from t1;
insert into t1 values (4294967295);
select -col_uint from t1;
insert into t1 values (4294967296);
select -col_uint from t1;
insert into t1 values(9223372036854775808);
select -col_uint from t1;
insert into t1 values(9223372036854775809);
select -col_uint from t1;
insert into t1 values(18446744073709551615),(18446744073709551610),(1844674407370955150);
select -col_uint from t1;
DROP TABLE t1;
set sql_mode='';
create table t1 (col_uint int8 unsigned);
insert into t1 values (100);
select -col_uint from t1;
insert into t1 values (4294967295);
select -col_uint from t1;
insert into t1 values (4294967296);
select -col_uint from t1;
insert into t1 values(9223372036854775808);
select -col_uint from t1;
insert into t1 values(9223372036854775809);
select -col_uint from t1;
insert into t1 values(18446744073709551615),(18446744073709551610),(1844674407370955150);
select -col_uint from t1;
DROP TABLE t1;
set sql_mode=default;

create table t1(id in1 signed);
create table t2(id intt unsigned);

set enable_seqscan=off;
set enable_bitmapscan =off;
create table t_index(id int8 unsigned ,description varchar(100));
insert into t_index (id, description) values(generate_series(0,167772),'aa');
create index index_id_btree on t_index (id);
explain (COSTS OFF)  select * from t_index where id BETWEEN 65 and 70;
drop table t_index;
set enable_seqscan=on;
set enable_bitmapscan =on;

create table tt(id1 uint1, id2 uint8);
insert into tt values(255, 257);
select id1<id2 from tt;
select id1=id2 from tt;
select id1>id2 from tt;
drop table tt;
create table tt(id1 uint1, id2 uint2);
insert into tt values(255, 65535);
select id1<id2 from tt;
select id1=id2 from tt;
select id1>id2 from tt;
drop table tt;

create table tt(id1 uint1, id2 int4);
insert into tt values(255, -128);
select id1<id2 from tt;
select id1=id2 from tt;
select id1>id2 from tt;
drop table tt;

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

create table t_like_operator(id int8 unsigned);
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

create table t_binary_operators (col_uint3 int3 unsigned,col_int8 int8);
insert into t_binary_operators values (65535,65535),(500000,3),(1,5),(0,0);
select col_int8 AND col_uint3 from t_binary_operators;
select col_int8 OR col_uint3 from t_binary_operators;
select col_int8 XOR col_uint3 from t_binary_operators;

/* Begin - bug#z6043 */
create table t_char_arith_hex(c char(10), v varchar(255),t text);
insert into t_char_arith_hex values (X'41', X'42', X'43'), (x'31', x'32', x'33'), (0x39, 0x30, 0x35);
select c, v, t,
c + 10 as c_add10,
c + 0x10 as c_add_hex,
v + 10 as v_add10,
v + 0x10 as v_add_hex,
t + 10 as t_add10,
t + 0x10 as t_add_hex
from t_char_arith_hex;
drop table t_char_arith_hex;
/* End - bug#z6043 */