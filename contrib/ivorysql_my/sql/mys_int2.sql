select '0'::smallint;
select '-32768'::smallint;
select '32767'::smallint;
select '-32769'::smallint;
select '32768'::smallint;

--smallint[(M)]
create table t1 (a smallint[-1]); --fail

create table t2 (a smallint(0));
\d t2
insert into t2 values (1);
select * from t2;
drop table t2;

create table t2 (a smallint(3));
\d t2
insert into t2 values (2);
select * from t2;
drop table t2;


--int2
create table t2 (a int2);
insert into t2 values(0);
insert into t2 values(1);
insert into t2 values(-1);
insert into t2 values(-32768);
insert into t2 values(32767);
insert into t2 values(-32769);
insert into t2 values(32768);
select * from t2;
drop table t2;

create table t2 (a int2 SIGNED);
insert into t2 values(0);
insert into t2 values(1);
insert into t2 values(-1);
insert into t2 values(-32768);
insert into t2 values(32767);
insert into t2 values(-32769);
insert into t2 values(32768);
select * from t2;
drop table t2;

--smallint
create table t1 (a smallint);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-32768);
insert into t1 values(32767);
insert into t1 values(-32769);
insert into t1 values(32768);
select * from t1;
drop table t1;

create table t1 (a smallint SIGNED);
insert into t1 values(0);
insert into t1 values(1);
insert into t1 values(-1);
insert into t1 values(-32768);
insert into t1 values(32767);
insert into t1 values(-32769);
insert into t1 values(32768);
select * from t1;
drop table t1;

CREATE TABLE t1 (sint16 smallint not null);
INSERT INTO t1 VALUES ('0.1');
INSERT INTO t1 VALUES ('0.5');
INSERT INTO t1 VALUES ('32767.4');
INSERT INTO t1 VALUES ('32767.5');
INSERT INTO t1 VALUES ('-0.1');
INSERT INTO t1 VALUES ('-0.5');
INSERT INTO t1 VALUES ('-32767.4');
INSERT INTO t1 VALUES ('-32767.5');
INSERT INTO t1 VALUES ('-32768.4');
INSERT INTO t1 VALUES ('-32768.5');
SELECT * FROM t1;
DROP TABLE t1;

CREATE TABLE t1(col1 SMALLINT);
INSERT INTO t1 VALUES(-32768),(0),(32767),('-32768'),('32767'),(-32768.0),(32767.0);
--error 1264
INSERT INTO t1 (col1) VALUES(-32769);
--error 1264
INSERT INTO t1 (col1) VALUES(32768);
--error 1264
INSERT INTO t1 (col2) VALUES(-1);
--error 1264
INSERT INTO t1 (col2) VALUES(65536);
--error 1264
INSERT INTO t1 (col1) VALUES('-32769');
--error 1264
INSERT INTO t1 (col1) VALUES('32768');
--error 1264
INSERT INTO t1 (col1) VALUES(-32769.0);
--error 1264
INSERT INTO t1 (col1) VALUES(32768.0);
INSERT INTO t1 (col1) VALUES ('');
INSERT INTO t1 (col1) VALUES ('a59b');
INSERT INTO t1 (col1) VALUES ('1a');

SELECT * FROM t1;
DROP TABLE t1;

--create tables
CREATE TABLE t1(c1 SMALLINT NOT NULL, c2 SMALLINT NOT NULL);
\d t1;
CREATE TABLE t2(c1 SMALLINT NULL);
\d t2;
CREATE TABLE t3(c1 SMALLINT NOT NULL);
\d t3;
CREATE TABLE t4(c1 SMALLINT(5) NULL DEFAULT 1);
\d t4;
CREATE TABLE t5(c1 SMALLINT(1) NULL);
\d t5;
DROP TABLE t1,t2,t3,t4,t5;

--operator
create table tb_test(id int2);
insert into tb_test values (123),(-121),(0);
select !id from tb_test;
select ~id from tb_test;	--BUG#5055	
select +id from tb_test;
select -id from tb_test;
select * from tb_test;
drop table tb_test;

create table t1 (col1 int2, col2 int2);
insert into t1 values(1, 123);
insert into t1 values(1, -123);
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
select col1 || col2 from t1;
reset sql_mode;
select col1 || col2 from t1; --BUG#5051
select col1 or col2 from t1; --BUG#5051
select col1 && col2 from t1; --BUG#5050
select col1 and col2 from t1; --BUG#5050
select col1 xor col2 from t1; --BUG#5052
select col1 <=> col2 from t1;
drop table t1;


CREATE TABLE t1(c1 SMALLINT);
INSERT INTO t1 (c1) VALUES(0),(1),(2),(3),(4),(5),(6),(7),(8),(9);
SELECT * FROM t1 WHERE c1 > 5;
SELECT * FROM t1 WHERE c1 >= 5;
SELECT * FROM t1 WHERE c1 = 5;
SELECT * FROM t1 WHERE c1 != 5;
SELECT * FROM t1 WHERE c1 <> 5;
SELECT * FROM t1 WHERE c1 < 5;
SELECT * FROM t1 WHERE c1 <= 5;
DROP TABLE t1;

------------------------------
--from int2 to other pg type
------------------------------
select '123'::smallint::pg_catalog.int4;
select '123'::smallint::pg_catalog.int8;
select '123'::smallint::pg_catalog.float4;
select '123'::smallint::pg_catalog.float8;
select '123'::smallint::pg_catalog.numeric;
select '123'::smallint::pg_catalog.oid;
select '123'::smallint::pg_catalog.regclass;
select '123'::smallint::pg_catalog.char;
select '123'::smallint::pg_catalog.varchar;
select '123'::smallint::pg_catalog.bpchar;
select '123'::smallint::pg_catalog.text;
select '123'::smallint::pg_catalog.regoperator;
select '123'::smallint::pg_catalog.regcollation;
select '123'::smallint::pg_catalog.regtype;
select '123'::smallint::pg_catalog.regconfig;
select '123'::smallint::pg_catalog.regdictionary;
select '123'::smallint::pg_catalog.regnamespace;
select '123'::smallint::pg_catalog.regrole;
select '123'::smallint::pg_catalog.regproc;
select '123'::smallint::pg_catalog.regprocedure;
select '123'::smallint::pg_catalog.regoper;

----------------------------------
--from other pg type to smallint
----------------------------------
select cast('32767'::pg_catalog.int2 as smallint);
select cast('32767'::pg_catalog.int4 as smallint);
select cast('32767'::pg_catalog.int8 as smallint);
select cast('32767'::pg_catalog.float4 as smallint);
select cast('32767'::pg_catalog.float8 as smallint);
select cast('32767'::pg_catalog.numeric as smallint);

select cast('32768'::pg_catalog.int2 as smallint);
select cast('32768'::pg_catalog.int4 as smallint);
select cast('32768'::pg_catalog.int8 as smallint);
select cast('32768'::pg_catalog.float4 as smallint);
select cast('32768'::pg_catalog.float8 as smallint);
select cast('32768'::pg_catalog.numeric as smallint);

select cast('-32768'::pg_catalog.int2 as smallint);
select cast('-32768'::pg_catalog.int4 as smallint);
select cast('-32768'::pg_catalog.int8 as smallint);
select cast('-32768'::pg_catalog.float4 as smallint);
select cast('-32768'::pg_catalog.float8 as smallint);
select cast('-32768'::pg_catalog.numeric as smallint);

select cast('-32769'::pg_catalog.int2 as smallint);
select cast('-32769'::pg_catalog.int4 as smallint);
select cast('-32769'::pg_catalog.int8 as smallint);
select cast('-32769'::pg_catalog.float4 as smallint);
select cast('-32769'::pg_catalog.float8 as smallint);
select cast('-32769'::pg_catalog.numeric as smallint);

select '123'::pg_catalog.int2::smallint;
select '123'::pg_catalog.int4::smallint;
select '123'::pg_catalog.int8::smallint;
select '123'::pg_catalog.float4::smallint;
select '123'::pg_catalog.float8::smallint;
select '123'::pg_catalog.numeric::smallint;

--view
create table t1 (s1 smallint);
create view v1 as select * from t1 where 20 < (select (s1) from t1);
insert into v1 values (30);
create view v2 as select * from t1;
create view v3 as select * from t1 where 20 < (select (s1) from v2);
insert into v3 values (30);
create view v4 as select * from v2 where 20 < (select (s1) from t1);
insert into v4 values (30);
drop view v4, v3, v2, v1;
drop table t1;

-- fix bug 5015
create table tb_test(id int2);
set sql_mode = default;
insert into tb_test values ('123abc');
set sql_mode = '';
insert into tb_test values ('123abc');
select * from tb_test;
drop table tb_test;
set sql_mode = default;

-- BUG#5060
create table t(c smallint(-1) signed);
create table t(c smallint(-1) SIGNED);
create table t(c smallint(-1));
create table t(c int2(-1) signed);
create table t(c int2(-1) SIGNED);
create table t(c int2(-1));
create table t(c INT2(-1) signed);
create table t(c INT2(-1) SIGNED);
create table t(c INT2(-1));
create table t(c smallint(256) signed);
create table t(c smallint(256) SIGNED);
create table t(c smallint(256));
create table t(c int2(256) signed);
create table t(c int2(256) SIGNED);
create table t(c int2(256));
create table t(c INT2(256) signed);
create table t(c INT2(256) SIGNED);
create table t(c INT2(256));

-- BUG#5061
create table tb_test(id int2);
set sql_mode = default;
insert into tb_test values (-32769);
insert into tb_test values (32768);
set sql_mode = '';
insert into tb_test values (-32769);
insert into tb_test values (32768);
select * from tb_test;
drop table tb_test;
set sql_mode = default;

-- BUG#5048 5070
create table tb_test(id int2);
set sql_mode = default;
insert into tb_test values (0x1f);
insert into tb_test values (0x7fff);
insert into tb_test values (0x8000);
insert into tb_test values (0xffffffffffffffff);
set sql_mode = '';
insert into tb_test values (0x1f);
insert into tb_test values (0x7fff);
insert into tb_test values (0x8000);
insert into tb_test values (0xffffffffffffffff);
select * from tb_test;
drop table tb_test;
set sql_mode = default;

-- BUG#5478
set div_precision_increment=50;
create table t1 (a int1,b int2);
insert into t1 values (4,4);
select a/2 ,b/2 ,8/a, 8/b from t1;
drop table t1;