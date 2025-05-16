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

set SQL_MODE to default;
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