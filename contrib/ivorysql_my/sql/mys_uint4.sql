set SQL_MODE='TRADITIONAL';
select cast(1111 as int UNSIGNED) ;
select cast(1111.8 as int UNSIGNED) ;
select cast(4294967297.8 as int UNSIGNED) ;
select cast(4294967297.8 as UNSIGNED);
select cast('1111' as int UNSIGNED) ;
select cast('4294967295' as int UNSIGNED) ;
select cast('4294967297' as int UNSIGNED) ;
select cast('a111' as int UNSIGNED) ;
select cast('111a' as int UNSIGNED) ;
select cast('.111' as int UNSIGNED) ;
select cast('0111' as int UNSIGNED) ;
select cast('#111' as int UNSIGNED) ;
select cast('111#' as int UNSIGNED) ;
select cast('111.' as int UNSIGNED) ;

select cast(1111 as int(3) UNSIGNED) ;
select cast(1111 as int4(3) UNSIGNED) ;
select cast(1111 as integer(3) UNSIGNED) ;
select cast(1111 as int(3) ZEROFILL) ;
select cast(1111 as int4(3) ZEROFILL) ;
select cast(1111 as integer(3) ZEROFILL) ;
select cast(1111 as int(3) UNSIGNED ZEROFILL) ;
select cast(1111 as int4(3) UNSIGNED ZEROFILL) ;
select cast(1111 as integer(3) UNSIGNED ZEROFILL) ;

set SQL_MODE='';
select cast(1111 as int UNSIGNED) ;
select cast(1111.8 as int UNSIGNED) ;
select cast(4294967297.8 as int UNSIGNED) ;
select cast(4294967297.8 as UNSIGNED);
select cast('1111' as int UNSIGNED) ;
select cast('4294967295' as int UNSIGNED) ;
select cast('4294967297' as int UNSIGNED) ;
select cast('a111' as int UNSIGNED) ;
select cast('111a' as int UNSIGNED) ;
select cast('.111' as int UNSIGNED) ;
select cast('0111' as int UNSIGNED) ;
select cast('#111' as int UNSIGNED) ;
select cast('111#' as int UNSIGNED) ;
select cast('111.' as int UNSIGNED) ;

select cast(1111 as int(3) UNSIGNED) ;
select cast(1111 as int4(3) UNSIGNED) ;
select cast(1111 as integer(3) UNSIGNED) ;
select cast(1111 as int(3) ZEROFILL) ;
select cast(1111 as int4(3) ZEROFILL) ;
select cast(1111 as integer(3) ZEROFILL) ;
select cast(1111 as int(3) UNSIGNED ZEROFILL) ;
select cast(1111 as int4(3) UNSIGNED ZEROFILL) ;
select cast(1111 as integer(3) UNSIGNED ZEROFILL) ;
set SQL_MODE='TRADITIONAL';
--integer
create table t1(c1 integer UNSIGNED);
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

CREATE TABLE t4 (a int UNSIGNED, b numeric);
insert into t4 values (4294967295, 11122333.8080666);
insert into t4 values (100200650, 123456.8742);
insert into t4 values (100200650, -123456.8742);
select  a / b from t4;
select  a % b from t4;
select a::numeric + 1500.123456 from t4;
select cast(a as numeric(20,8)) from t4;
select convert(a, numeric(20,8)) from t4;
drop table t4;

select cast(50 as uint4);
select cast(4294967295 as uint4);
select cast(9999999999 as uint4);
select cast(123.456 as uint4);
select cast(0 as uint4);
select cast(-7200.145 as uint4);
select cast(-2147483648 as uint4);
select cast(-9999999999 as uint4);

create table t1(c1 integer UNSIGNED);
insert into t1 values(1.4);
insert into t1 values(1.5);
insert into t1 values('1.4');
insert into t1 values('1.5');
select * from t1;
drop table t1;

--integer[(M)]
create table t1 (a integer[-1] UNSIGNED); --fail

create table t2 (a integer(0) UNSIGNED);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a integer(3) UNSIGNED);
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t3 (a integer(4) UNSIGNED);
\d t3
insert into t3 values (3);
select * from t3;
drop table t3;

create table t2(age integer(256.4) unsigned);
create table t2(id1 int4(255.9) unsigned, id2 int4(254.9) unsigned, age integer(255.4) unsigned);
\d t2
insert into t2 values (2, 5, 44);
select * from t2;
drop table t2;

--mediumint zerofill
create table t2 (a integer(-1) ZEROFILL);

create table t2 (a integer ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a integer(2) ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

--int
create table t1(c1 int UNSIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-2147483648);
insert into t1 values(2147483647);
insert into t1 values(-2147483649);
insert into t1 values(4294967295);
select * from t1;
drop table t1;

create table t1(c1 int UNSIGNED);
insert into t1 values(1.4);
insert into t1 values(1.5);
insert into t1 values('1.4');
insert into t1 values('1.5');
select * from t1;
drop table t1;

--int[(M)]
create table t1 (a int[-1] UNSIGNED); --fail

create table t2 (a int(0) UNSIGNED);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a int(3) UNSIGNED);
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t3 (a int(4) UNSIGNED);
\d t3
insert into t3 values (3);
select * from t3;
drop table t3;

--mediumint zerofill
create table t2 (a int(-1) ZEROFILL);

create table t2 (a int ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a int(2) ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

--int4
create table t1(c1 int4 UNSIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-2147483648);
insert into t1 values(2147483647);
insert into t1 values(-2147483649);
insert into t1 values(4294967295);
select * from t1;
drop table t1;

create table t1(c1 int4 UNSIGNED);
insert into t1 values(1.4);
insert into t1 values(1.5);
insert into t1 values('1.4');
insert into t1 values('1.5');
select * from t1;
drop table t1;

--int[(M)]
create table t1 (a int4[-1] UNSIGNED); --fail

create table t2 (a int4(0) UNSIGNED);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a int4(3) UNSIGNED);
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t3 (a int4(4) UNSIGNED);
\d t3
insert into t3 values (3);
select * from t3;
drop table t3;

--mediumint zerofill
create table t2 (a int4(-1) ZEROFILL);

create table t2 (a int4 ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a int4(2) ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

set SQL_MODE='';

--integer
create table t1(c1 integer UNSIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-2147483648);
insert into t1 values(2147483647);
insert into t1 values(-2147483649);
insert into t1 values(4294967295);
select * from t1;
drop table t1;

--integer[(M)]
create table t1 (a integer[-1] UNSIGNED ); --fail

create table t2 (a integer(0) UNSIGNED);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a integer(3) UNSIGNED);
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t3 (a integer(4) UNSIGNED);
\d t3
insert into t3 values (3);
select * from t3;
drop table t3;

--mediumint zerofill
create table t2 (a integer(-1) ZEROFILL);

create table t2 (a integer ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a integer(2) ZEROFILL);
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

--int
create table t1(c1 int UNSIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-2147483648);
insert into t1 values(2147483647);
insert into t1 values(-2147483649);
insert into t1 values(4294967295);
select * from t1;
drop table t1;

--int4
create table t1(c1 int4 UNSIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-2147483648);
insert into t1 values(2147483647);
insert into t1 values(-2147483649);
insert into t1 values(4294967295);
select * from t1;
drop table t1;

--numeric -> uint4
select 2000.49999::uint4;
select 2000.5::uint4;
select 4294967294.3::uint4;
select 4294967296.1::uint4;
select 999999999999999.0205::uint4;
select (-0.02)::uint4;
select (-68.753951)::uint4;
select (-2147483647.92)::uint4;
select (-2147483649.37)::uint4;
select (-999999999999999.0205)::uint4;

set SQL_MODE='TRADITIONAL';
CREATE TABLE tb4 (a numeric, b int UNSIGNED);
insert into tb4 values (11223344.556677, 99999999);
insert into tb4 values (0.62, 100);
insert into tb4 values (7.05, 1000.123);
insert into tb4 values (7.05, 4294967294.3);
insert into tb4 values (7.05, 4294967295.0);
insert into tb4 values (7.05, 4294967296.1);
insert into tb4 values (7.05, 999999999999999.0205);
insert into tb4 values (120.007, 0);
insert into tb4 values (9.36, -50000.753951);
insert into tb4 values (9.36, -2147483647.92);
insert into tb4 values (9.36, -2147483648.0);
insert into tb4 values (9.36, -2147483649.37);
insert into tb4 values (9.36, -999999999999999.0205);
select * from tb4;
select cast(a as uint4) from tb4;
select convert(a, uint4) from tb4;
select a::uint4 + b from tb4;
drop table tb4;

set SQL_MODE='';
CREATE TABLE tb4 (a numeric, b int UNSIGNED);
insert into tb4 values (11223344.556677, 99999999);
insert into tb4 values (0.62, 100);
insert into tb4 values (7.05, 1000.123);
insert into tb4 values (7.05, 4294967294.3);
insert into tb4 values (7.05, 4294967295.0);
insert into tb4 values (7.05, 4294967296.1);
insert into tb4 values (7.05, 999999999999999.0205);
insert into tb4 values (120.007, 0);
insert into tb4 values (9.36, -50000.753951);
insert into tb4 values (9.36, -2147483647.92);
insert into tb4 values (9.36, -2147483648.0);
insert into tb4 values (9.36, -2147483649.37);
insert into tb4 values (9.36, -999999999999999.0205);
select * from tb4;
select cast(a as uint4) from tb4;
select convert(a, uint4) from tb4;
select a::uint4 + b from tb4;
drop table tb4;

set SQL_MODE to default;

--uint4 -> signed/unsigned (int8/uint8)
select 4294967295::uint4::int8;
select 4294967295::uint4::uint8;
select convert(4294967295::uint4, signed);
select cast(4294967295::uint4 as unsigned);

create table test1(a uint4);
insert into test1 values(5000000);
select convert(a, signed) from test1;
select cast(a as unsigned) from test1;
drop table test1;

-- oper
---=
select cast(111 as int UNSIGNED)  = 111;
select cast(111 as int UNSIGNED)  = -111;
select cast(111 as int UNSIGNED)  = NULL;
select 111 = cast(111 as int UNSIGNED);
select -111 = cast(111 as int UNSIGNED);
select NULL = cast(111 as int UNSIGNED);
--<>
select cast(111 as int UNSIGNED)  <> 111;
select cast(111 as int UNSIGNED)  <> -111;
select cast(111 as int UNSIGNED)  <> NULL;
select 111 <> cast(111 as int UNSIGNED);
select -111 <> cast(111 as int UNSIGNED);
select NULL <> cast(111 as int UNSIGNED);
--<
select cast(111 as int UNSIGNED)  < 111;
select cast(111 as int UNSIGNED)  < -111;
select cast(111 as int UNSIGNED)  < NULL;
select 111 < cast(111 as int UNSIGNED);
select -111 < cast(111 as int UNSIGNED);
select NULL < cast(111 as int UNSIGNED);
-->
select cast(111 as int UNSIGNED)  > 111;
select cast(111 as int UNSIGNED)  > -111;
select cast(111 as int UNSIGNED)  > NULL;
select 111 > cast(111 as int UNSIGNED);
select -111 > cast(111 as int UNSIGNED);
select NULL > cast(111 as int UNSIGNED);
--<=
select cast(111 as int UNSIGNED)  <= 111;
select cast(111 as int UNSIGNED)  <= -111;
select cast(111 as int UNSIGNED)  <= NULL;
select 111 <= cast(111 as int UNSIGNED);
select -111 <= cast(111 as int UNSIGNED);
select NULL <= cast(111 as int UNSIGNED);
-->=
select cast(111 as int UNSIGNED)  >= 111;
select cast(111 as int UNSIGNED)  >= -111;
select cast(111 as int UNSIGNED)  >= NULL;
select 111 >= cast(111 as int UNSIGNED);
select -111 >= cast(111 as int UNSIGNED);
select NULL >= cast(111 as int UNSIGNED);
--<=>
select cast(111 as int UNSIGNED)  <=> 111;
select cast(111 as int UNSIGNED)  <=> -111;
select cast(111 as int UNSIGNED)  <=> NULL;
select 111 <=> cast(111 as int UNSIGNED);
select -111 <=> cast(111 as int UNSIGNED);
select NULL <=> cast(111 as int UNSIGNED);

--+
select cast(111 as int UNSIGNED) + 111;
select cast(111 as int UNSIGNED) + -111;
select cast(111 as int UNSIGNED) + NULL;
select 111 + cast(111 as int UNSIGNED);
select -111 + cast(111 as int UNSIGNED);
select NULL + cast(111 as int UNSIGNED);
---
select cast(111 as int UNSIGNED)  - 111;
select cast(111 as int UNSIGNED)  - -111;
select cast(111 as int UNSIGNED)  - NULL;
select 111 - cast(111 as int UNSIGNED);
select -111 - cast(111 as int UNSIGNED);
select NULL - cast(111 as int UNSIGNED);
-- *
select cast(111 as int UNSIGNED)  * 111;
select cast(111 as int UNSIGNED)  * -111;
select cast(111 as int UNSIGNED)  * NULL;
select 111 * cast(111 as int UNSIGNED);
select -111 * cast(111 as int UNSIGNED);
select NULL * cast(111 as int UNSIGNED);
select 4294967295 * cast(4294967295 as int UNSIGNED);

-- /
select cast(111 as int UNSIGNED)  / 111;
select cast(111 as int UNSIGNED)  / 110;
select cast(111 as int UNSIGNED)  / -111;
select cast(111 as int UNSIGNED)  / NULL;
select 111 / cast(111 as int UNSIGNED);
select 110 / cast(111 as int UNSIGNED);
select -111 / cast(111 as int UNSIGNED);
select NULL / cast(111 as int UNSIGNED);

--div
select cast(111 as int UNSIGNED) div 111;
select cast(111 as int UNSIGNED) div 110;
select cast(111 as int UNSIGNED) div -111;
select cast(111 as int UNSIGNED) div NULL;
select 111 div cast(111 as int UNSIGNED);
select 110 div cast(111 as int UNSIGNED);
select -111 div cast(111 as int UNSIGNED);
select NULL div cast(111 as int UNSIGNED);

-- %
select cast(111 as int UNSIGNED) % 111;
select cast(111 as int UNSIGNED) % -111;
select cast(111 as int UNSIGNED) % NULL;
select 111 % cast(111 as int UNSIGNED);
select -111 % cast(111 as int UNSIGNED);
select NULL % cast(111 as int UNSIGNED);

--mod
select cast(111 as int UNSIGNED) mod 111;
select cast(111 as int UNSIGNED) mod -111;
select cast(111 as int UNSIGNED) mod NULL;
select 111 mod cast(111 as int UNSIGNED);
select -111 mod cast(111 as int UNSIGNED);
select NULL mod cast(111 as int UNSIGNED);

set sql_mode to default;
select cast(111 as int UNSIGNED)  / 0;
select cast(111 as int UNSIGNED) div 0;
select cast(111 as int UNSIGNED) % 0;
select cast(111 as int UNSIGNED) mod 0;
set sql_mode= '';
select cast(111 as int UNSIGNED)  / 0;
select cast(111 as int UNSIGNED) div 0;
select cast(111 as int UNSIGNED) % 0;
select cast(111 as int UNSIGNED) mod 0;

--"&"
select cast(111 as int UNSIGNED) & 111;
select cast(111 as int UNSIGNED) & -111;
select cast(111 as int UNSIGNED) & NULL;
select 111 & cast(111 as int UNSIGNED);
select -111 & cast(111 as int UNSIGNED);
select NULL & cast(111 as int UNSIGNED);

--"|"
select cast(111 as int UNSIGNED)  | 111;
select cast(111 as int UNSIGNED)  | -111;
select cast(111 as int UNSIGNED)  | NULL;
select 111 | cast(111 as int UNSIGNED);
select -111 | cast(111 as int UNSIGNED);
select NULL | cast(111 as int UNSIGNED);

--"~"
select ~cast(111 as int UNSIGNED);

--"^"
select cast(111 as int UNSIGNED)  ^ 111;
select cast(111 as int UNSIGNED)  ^ -111;
select cast(111 as int UNSIGNED)  ^ NULL;
select 111 ^ cast(111 as int UNSIGNED);
select -111 ^ cast(111 as int UNSIGNED);
select NULL ^ cast(111 as int UNSIGNED);

--">>"
select cast(111 as int UNSIGNED) >> 111;
select cast(111 as int UNSIGNED) >> -111;
select cast(111 as int UNSIGNED) >> NULL;
select 111 >> cast(111 as int UNSIGNED);
select -111 >> cast(111 as int UNSIGNED);
select NULL >> cast(111 as int UNSIGNED);

--"<<"
select cast(111 as int UNSIGNED) << 111;
select cast(111 as int UNSIGNED) << -111;
select cast(111 as int UNSIGNED) << NULL;
select 111 << cast(111 as int UNSIGNED);
select -111 << cast(111 as int UNSIGNED);
select NULL << cast(111 as int UNSIGNED);

--"&&"
select cast(111 as int UNSIGNED) && 111;
select cast(111 as int UNSIGNED) && -111;
select cast(111 as int UNSIGNED) && NULL;
select 111 && cast(111 as int UNSIGNED);
select -111 && cast(111 as int UNSIGNED);
select NULL && cast(111 as int UNSIGNED);

--"||"
select cast(111 as int UNSIGNED) || true;
select cast(111 as int UNSIGNED) || false;
select true || cast(111 as int UNSIGNED);
select false || cast(111 as int UNSIGNED);

--"!"
select !cast(111 as int UNSIGNED);

--"and"
select cast(111 as int UNSIGNED)  and 111::boolean;
select cast(111 as int UNSIGNED)  and (-111)::boolean;
select cast(111 as int UNSIGNED)  and NULL;
select 111::boolean and cast(111 as int UNSIGNED);
select (-111)::boolean and cast(111 as int UNSIGNED);
select NULL and cast(111 as int UNSIGNED);

--"or"
select cast(111 as int UNSIGNED) or 111::boolean;
select cast(111 as int UNSIGNED) or (-111)::boolean;
select cast(111 as int UNSIGNED) or NULL;
select 111::boolean or cast(111 as int UNSIGNED);
select (-111)::boolean or cast(111 as int UNSIGNED);
select NULL or cast(111 as int UNSIGNED);

--"xor"
select cast(111 as int UNSIGNED) xor 111::boolean;
select cast(111 as int UNSIGNED) xor (-111)::boolean;
select cast(111 as int UNSIGNED) xor NULL;
select 111::boolean xor cast(111 as int UNSIGNED);
select (-111)::boolean xor cast(111 as int UNSIGNED);
select NULL xor cast(111 as int UNSIGNED);

set SQL_MODE to default;
--------------------------------------------------
CREATE TABLE t2 (a int UNSIGNED, b int UNSIGNED);
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
create table t1 (a int UNSIGNED , b int UNSIGNED);
insert into t1 values(111, 444);
insert into t1 values(222, 444);
insert into t1 values(333, 444);
insert into t1 values(444, 444);
insert into t1 values(555, 444);
select * from t1 where a between 222 and 444;
select * from t1 where b between a and 555;

drop table t1;

-- in
create table t1(a int UNSIGNED);
insert into t1 values(111),(222),(333);
select * from t1 where a in (112,222,444);
select * from t1 where a not in (112,222,444);
drop table t1;

-- is
CREATE TABLE t1 (a int UNSIGNED, b int UNSIGNED);
INSERT INTO t1 VALUES (111,1), (222,2);
SELECT * FROM t1 WHERE a IS true;
SELECT * FROM t1 WHERE a IS not true;
SELECT * FROM t1 WHERE a IS NULL;
SELECT * FROM t1 WHERE a IS not NULL;
drop table t1;

-- like
create table t1 (a int UNSIGNED);
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
create table idx_1 (idx int UNSIGNED);
create index idx1 on idx_1 using btree (idx);
insert into idx_1 values (111),(222),(333);
vacuum full analyze idx_1;
explain (COSTS OFF) select * from idx_1 where idx < 111;
explain (COSTS OFF) select * from idx_1 where idx > 222;
explain (COSTS OFF) select * from idx_1 where idx = 333;
explain (COSTS OFF) select * from idx_1 where idx <= 444;
explain (COSTS OFF) select * from idx_1 where idx >= 555;

create table idx_2 (idx int UNSIGNED);
create index idx2 on idx_2 using hash (idx);
vacuum full analyze idx_2;
insert into idx_2 values (111),(222),(333);
explain (COSTS OFF) select * from idx_2 where idx = 111;
explain (COSTS OFF) select * from idx_2 where idx > 222;

drop table idx_1,idx_2;

create table tb1 (a int UNSIGNED);
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


CREATE TABLE t1(m1 int UNSIGNED);
INSERT INTO t1 VALUES (1901),(2155),(0000);
SELECT * FROM t1;
SELECT COUNT(*) AS total_rows, MIN(m1) AS min_value, MAX(m1) AS max_value FROM t1;
SELECT AVG(m1) AS avg_value FROM t1;
SELECT SUM(m1) AS sum_value FROM t1;

DROP TABLE t1;
create table t1(id int4 unsigned);
insert into t1 values (1),(null);
select * from t1;
select avg(id) from t1;
DROP TABLE t1;

create table t_aggregate (value int4 unsigned);
insert into t_aggregate (value) values (128), (100), (0), (50), (100), (255), (32767), (65535), (4294967295), (4294967295);
select avg(value), SUM(value) from t_aggregate;
DROP TABLE t_aggregate;

create table t1 (col_uint int4 unsigned);
insert into t1 values (100);
insert into t1 values(4294967295),(4294967290),(4294967280);
select -col_uint from t1;
DROP TABLE t1;

set enable_seqscan=off;
set enable_bitmapscan =off;
create table t_index(id int4 unsigned ,description varchar(100));
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

create table t_like_operator(id int4 unsigned);
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