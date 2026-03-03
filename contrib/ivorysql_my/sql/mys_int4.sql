--int[(M)]
create table t1 (a int[-1]); --fail

create table t2 (a int(0));
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a int(3));
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t2 (a int4(0));
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a int4(3));
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

create table t2 (a integer(0));
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a integer(3));
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;

--int4
create table t1(c1 int);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(1.5);
insert into t1 values('1.5');
insert into t1 values(-1);
insert into t1 values(-2147483648);
insert into t1 values(2147483647);
insert into t1 values(-2147483649);
insert into t1 values(4294967295);
insert into t1 values(true);--BUG#5392
insert into t1 values(false);--BUG#5392
select * from t1;
drop table t1;

create table t1(c1 int SIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(1.5);
insert into t1 values('1.5');
insert into t1 values(-1);
insert into t1 values(-2147483648);
insert into t1 values(2147483647);
insert into t1 values(-2147483649);
insert into t1 values(4294967295);
select * from t1;
drop table t1;

create table t1(c1 int4);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-2147483648);
insert into t1 values(2147483647);
insert into t1 values(-2147483649);
insert into t1 values(4294967295);
select * from t1;
drop table t1;

create table t1(c1 int4 SIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-2147483648);
insert into t1 values(2147483647);
insert into t1 values(-2147483649);
insert into t1 values(4294967295);
select * from t1;
drop table t1;

create table t1(c1 integer);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-2147483648);
insert into t1 values(2147483647);
insert into t1 values(-2147483649);
insert into t1 values(4294967295);
select * from t1;
drop table t1;

create table t1(c1 integer SIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-2147483648);
insert into t1 values(2147483647);
insert into t1 values(-2147483649);
insert into t1 values(4294967295);
select * from t1;
drop table t1;


CREATE TABLE t1 (col1 INT);
INSERT INTO t1 VALUES(-2147483648),(0),(2147483647),('-2147483648'),('2147483647'),(-2147483648.0),(2147483647.0);
INSERT INTO t1 (col1) VALUES(-2147483649);
INSERT INTO t1 (col1) VALUES(2147643648);
INSERT INTO t1 (col1) VALUES(-1);
INSERT INTO t1 (col1) VALUES(4294967296);
INSERT INTO t1 (col1) VALUES('-2147483649');
INSERT INTO t1 (col1) VALUES('2147643648');
INSERT INTO t1 (col1) VALUES('-1');
INSERT INTO t1 (col1) VALUES('4294967296');
INSERT INTO t1 (col1) VALUES(-2147483649.0);
INSERT INTO t1 (col1) VALUES(2147643648.0);
INSERT INTO t1 (col1) VALUES(-1.0);
INSERT INTO t1 (col1) VALUES(4294967296.0);
UPDATE t1 SET col1 = col1 - 50 WHERE col1 < 0;
UPDATE t1 SET col1 =col1 + 50 WHERE col1 > 0;
UPDATE t1 SET col1 =col1 / 0 WHERE col1 > 0;
UPDATE t1 SET col1= MOD(col1,0) WHERE col1 > 0;
INSERT INTO t1 (col1) VALUES ('');
INSERT INTO t1 (col1) VALUES ('a59b');
INSERT INTO t1 (col1) VALUES ('1a');
INSERT INTO t1 (col1) VALUES ('2a');
INSERT INTO t1 values (1/0);
INSERT INTO t1 values (-2147483649),(2147643648);
INSERT INTO t1 values ('-2147483649'),('2147643648');
INSERT INTO t1 values (-2147483649.0),(2147643648.0);
UPDATE t1 SET col1=1/NULL where col1=0;
SELECT * FROM t1;
DROP TABLE t1;

--create tables
CREATE TABLE t1(c1 INT NOT NULL, c2 INT NOT NULL);
\d t1;
CREATE TABLE t2(c1 INT NULL);
\d t2;
CREATE TABLE t3(c1 INT NOT NULL);
\d t3;
CREATE TABLE t4(c1 INT(5) NULL DEFAULT 1);
\d t4;
CREATE TABLE t5(c1 INT(1) NULL);
\d t5;
DROP TABLE t1,t2,t3,t4,t5;

--int4 -> signed/unsigned (int8/uint8)
select 500::int4::int8;
select 500::int4::uint8;
select (-6000)::int4::int8;
select (-6000)::int4::uint8;
select convert((-7000)::int, signed);
select cast((-7000)::int as unsigned);

create table test1(a int);
insert into test1 values(-5000);
select convert(a, signed) from test1;
select cast(a as unsigned) from test1;
drop table test1;

--operator
create table tb_test(id int);
insert into tb_test values (2147483647),(-2147483648),(0),(1);
select !id from tb_test;
select ~id from tb_test;	--BUG#5055	
select +id from tb_test;
select -id from tb_test;
select * from tb_test;
drop table tb_test;

create table t1 (col1 int, col2 int);
insert into t1 values(1, 2147483647);
insert into t1 values(1, -2147483648);
select col1 + col2 from t1;
select col1 - col2 from t1;
select col1 * col2 from t1;
select col1 / col2 from t1;
select col1 div col2 from t1; --BUG#5049
select col1 % col2 from t1;
select col1 & col2 from t1; --BUG#5053
select col1 | col2 from t1; --BUG#5054
select col1 ^ col2 from t1; --BUG#5056
set sql_mode = '';
select col1 || col2 from t1; --BUG#5051
reset sql_mode;
select col1 || col2 from t1; --BUG#5051
select col1 or col2 from t1; --BUG#5051
select col1 && col2 from t1; --BUG#5050
select col1 and col2 from t1; --BUG#5050
select col1 xor col2 from t1; --BUG#5052
select col1 <=> col2 from t1;
drop table t1;

CREATE TABLE t1(c1 INT);
INSERT INTO t1 (c1) VALUES(0),(1),(2),(3),(4),(5),(6),(7),(8),(9);
SELECT * FROM t1 WHERE c1 > 5;
SELECT * FROM t1 WHERE c1 >= 5;
SELECT * FROM t1 WHERE c1 = 5;
SELECT * FROM t1 WHERE c1 != 5;
SELECT * FROM t1 WHERE c1 <> 5;
SELECT * FROM t1 WHERE c1 < 5;
SELECT * FROM t1 WHERE c1 <= 5;
DROP TABLE t1;

create table test1(a int, b int default 3);
insert into test1 values();
insert into test1 values(5, a + 2);
insert into test1 values(5, a - 2);
insert into test1 values(1, a * 2);
insert into test1 values(6, a / 2);
insert into test1 values(7, a % 2);

select * from test1;
drop table test1;

create table tb_test(id int);
insert into tb_test values (123),(-456),(0);
select -id from tb_test;
select * from tb_test;
drop table tb_test;

create table t1 (col1 int, col2 int);
insert into t1 values(1, 123);
insert into t1 values(1, -123);
select col1 + col2 from t1;
insert into t1 values(1, col1 + 1);
select * from t1;
drop table t1;

create table t1 (col1 int, col2 int);
insert into t1 values(1, 123);
insert into t1 values(1, -123);
select col1 - col2 from t1;
insert into t1 values(2, col1 - 1);
select * from t1;
drop table t1;

create table t1 (col1 int, col2 int);
insert into t1 values(1, 123);
insert into t1 values(1, -123);
select col1 * col2 from t1;
insert into t1 values(2, col1 * 2);
select * from t1;
drop table t1;


create table t1 (col1 int, col2 int);
insert into t1 values(100, 10);
insert into t1 values(100, -10);
select col1 / col2 from t1;
insert into t1 values(2, col1 / 2);
select * from t1;
drop table t1;

create table t1 (col1 float8, col2 float8);
insert into t1 values(2.1, col1 / 2);
select * from t1;
drop table t1;


create table t1 (i int, j int);
insert into t1 values (1,1), (1,2), (2,3), (2,4);
select i, count(distinct j) from t1 group by i;
select i+0.0 as i2, count(distinct j) from t1 group by i2;
drop table t1;

CREATE TABLE t1 (a int DEFAULT NULL, b int DEFAULT NULL);
INSERT INTO t1 VALUES (3,30), (1,10), (2,10);
SELECT a+CAST(1 AS decimal(65,30)) AS aa, SUM(b) FROM t1 GROUP BY aa;
SELECT a+CAST(1 AS decimal(65,31)) AS aa, SUM(b) FROM t1 GROUP BY aa;
DROP TABLE t1;

-- cast
select '0'::int;
select '-2147483648'::int;
select '2147483647'::int;
select '-2147483649'::int;
select '2147483648'::int;

------------------------------
--to integer
------------------------------
------------------------------
--from int4 to other pg type
------------------------------
select '2147483647'::int::pg_catalog.int4;
select '2147483647'::int::pg_catalog.int8;
select '2147483647'::int::pg_catalog.float4;
select '2147483647'::int::pg_catalog.float8;
select '2147483647'::int::pg_catalog.numeric;
select '2147483647'::int::pg_catalog.oid;
select '2147483647'::int::pg_catalog.regclass;
select '2147483647'::int::pg_catalog.varchar;
select '2147483647'::int::pg_catalog.bpchar;
select '2147483647'::int::pg_catalog.text;
select '2147483647'::int::pg_catalog.regoperator;
select '2147483647'::int::pg_catalog.regcollation;
select '2147483647'::int::pg_catalog.regtype;
select '2147483647'::int::pg_catalog.regconfig;
select '2147483647'::int::pg_catalog.regdictionary;
select '2147483647'::int::pg_catalog.regnamespace;
select '2147483647'::int::pg_catalog.regrole;
select '2147483647'::int::pg_catalog.regproc;
select '2147483647'::int::pg_catalog.regprocedure;
select '2147483647'::int::pg_catalog.regoper;

----------------------------------
--from other pg type to int
----------------------------------
select cast('32767'::pg_catalog.int2 as int);
select cast('2147483647'::pg_catalog.int4 as int);
select cast('2147483647'::pg_catalog.int8 as int);
select cast('2000000000'::pg_catalog.float4 as int);
select cast('2147483647'::pg_catalog.float8 as int);
select cast('2147483647'::pg_catalog.numeric as int);
select cast('-32768'::pg_catalog.int2 as int);
select cast('-2147483648'::pg_catalog.int4 as int);
select cast('-2147483648'::pg_catalog.int8 as int);
select cast('-2147483648'::pg_catalog.float4 as int);
select cast('-2147483648'::pg_catalog.float8 as int);
select cast('-2147483648'::pg_catalog.numeric as int);
select cast('-2147483648'::pg_catalog.int4 as int);
select cast('-2147483648'::pg_catalog.int8 as int);
select cast('-2147483648'::pg_catalog.float4 as int);
select cast('-2147483648'::pg_catalog.float8 as int);
select cast('-2147483648'::pg_catalog.numeric as int);
select '123'::pg_catalog.int2::int;
select '123'::pg_catalog.int4::int;
select '123'::pg_catalog.int8::int;
select '123'::pg_catalog.float4::int;
select '123'::pg_catalog.float8::int;
select '123'::pg_catalog.numeric::int;

-- fix bug 5015
create table tb_test(id int);
set sql_mode = default;
insert into tb_test values ('123abc');
set sql_mode = '';
insert into tb_test values ('123abc');
select * from tb_test;
drop table tb_test;
set sql_mode = default;

-- fix bug 5060
create table t(c integer(-1) signed);
create table t(c integer(-1) SIGNED);
create table t(c integer(-1));
create table t(c int4(-1) signed);
create table t(c int4(-1) SIGNED);
create table t(c int4(-1));
create table t(c INT4(-1) signed);
create table t(c INT4(-1) SIGNED);
create table t(c INT4(-1));
create table t(c integer(256) signed);
create table t(c integer(256) SIGNED);
create table t(c integer(256));
create table t(c int4(256) signed);
create table t(c int4(256) SIGNED);
create table t(c int4(256));
create table t(c INT4(256) signed);
create table t(c INT4(256) SIGNED);
create table t(c INT4(256));
create table t(c int(256) signed);
create table t(c int(256) SIGNED);
create table t(c int(256));
create table t(c INT(256) signed);
create table t(c INT(256) SIGNED);
create table t(c INT(256));
create table t(c int(-1) signed);
create table t(c int(-1) SIGNED);
create table t(c int(-1));
create table t(c INT(-1) signed);
create table t(c INT(-1) SIGNED);
create table t(c INT(-1));

-- BUG#5103
create table t1(c int(10) signed);
drop table t1;
create table t2(c integer(10) signed);
drop table t2;

-- BUG#5164
set sql_mode = default;
set div_precision_increment = 99;
show div_precision_increment;
set div_precision_increment = -1;
show div_precision_increment;
set sql_mode = '';
set div_precision_increment = 99;
show div_precision_increment;
set div_precision_increment = -1;
show div_precision_increment;

-- BUG#5091
create table test_MEDIUMINT (a int3,b mediumint signed);
insert into test_MEDIUMINT (a, b) values(8388607, -8388608);
select a * b AS mul_result from test_MEDIUMINT;
drop table test_MEDIUMINT;
create table test_INT (a int4,b int signed);
insert into test_INT (a, b) values(8388607, -8388608);
select a * b AS mul_result from test_INT;
drop table test_INT;

-- BUG#5048 5070
create table tb_test(id int4);
set sql_mode = default;
insert into tb_test values (0x1f2a);
insert into tb_test values (0x7fffffff);
insert into tb_test values (0x80000000);
insert into tb_test values (0xffffffffffffffff);
set sql_mode = '';
insert into tb_test values (0x1f2a);
insert into tb_test values (0x7fffffff);
insert into tb_test values (0x80000000);
insert into tb_test values (0xffffffffffffffff);
select * from tb_test;
drop table tb_test;
set sql_mode = default;