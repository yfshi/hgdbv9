drop table if exists t1,t2;
--enable_warnings
SET sql_mode = 'NO_ENGINE_SUBSTITUTION';
SELECT 10,10.0,10.,.1e+2,100.0e-1;
SELECT 6e-16, -6e-16, --6e-16, -6e-16+1.000000;
SELECT 1e1,1.e1,1.0e1,1e+1,1.e+1,1.0e+1,1e-1,1.e-1,1.0e-1;
SELECT 0.001e+1,0.001e-1, -0.001e+01,-0.001e-01;
SELECT 123.23E+02,-123.23E-02,"123.23E+02"+0.0,"-123.23E-02"+0.0;
SELECT 2147483647E+02,21474836.47E+06;

create table t1 (f1 float(24),f2 float(52));
--# We mask out Privileges column because it differs for embedded server
--replace_column 8 #
--show full columns from t1;
insert into t1 values(10,10),(1e+5,1e+5),(1234567890,1234567890),(1e+10,1e+10),(1e+15,1e+15),(1e+20,1e+20),(1e+50,1e+50),(1e+150,1e+150);
insert into t1 values(-10,-10),(1e-5,1e-5),(1e-10,1e-10),(1e-15,1e-15),(1e-20,1e-20),(1e-50,1e-50),(1e-150,1e-150);
select * from t1;
drop table t1;

create table t1 (datum double);
insert into t1 values (0.5),(1.0),(1.5),(2.0),(2.5);
select * from t1;
select * from t1 where datum < 1.5;
select * from t1 where datum > 1.5;
select * from t1 where datum = 1.5;
drop table t1;

create table t1 (a  decimal(7,3) not null, k (a));
insert into t1 values ("0"),("-0.00"),("-0.01"),("-0.002"),("1");
select a from t1 order by a;
select min(a) from t1;
drop table t1;


create table t1 (c1 double, c2 varchar(20));
insert t1 values (121,"16");
select c1 + c1 * (c2 / 100) as col from t1;
create table t2 select c1 + c1 * (c2 / 100) as col1, round(c1, 5) as col2, round(c1, 35) as col3, sqrt(c1*1e-15) col4 from t1;

--disable_ps_protocol
select * from t2;
--enable_ps_protocol


--replace_result $DEFAULT_ENGINE ENGINE
--show create table t2;
drop table t1,t2;


create table t1 (a float);
insert into t1 values (1);
select max(a),min(a),avg(a) from t1;
drop table t1;


create table t1 (f float, f2 float(24), f3 float(6,2), d double, d2 float(53), d3 double(10,3), de decimal, de2 decimal(6), de3 decimal(5,2), n numeric, n2 numeric(8), n3 numeric(7,6));
--replace_column 8 #
--show full columns from t1;
drop table t1;

create table t1 (a  decimal(7,3) not null, k (a));
insert into t1 values ("0"),("-0.00"),("-0.01"),("-0.002"),("1");
select a from t1 order by a;
select min(a) from t1;
drop table t1;

--error 1425
create table t1 (a float(200,100), b double(200,100));

create table t1 (c20 char);
insert into t1 values (5000.0);
insert into t1 values (0.5e4);
drop table t1;


--error 1063
create table t1 (f float(54));	
--disable_warnings
drop table if exists t1;
--enable_warnings

create table t1 (d1 double, d2 double unsigned);
insert into t1 set d1 = -1.0;
update t1 set d2 = d1;
select * from t1;
drop table t1;


create table t1 (f float(4,3));
insert into t1 values (-11.0),(-11),("-11"),(11.0),(11),("11");
select * from t1;
drop table if exists t1;
create table t1 (f double(4,3));
insert into t1 values (-11.0),(-11),("-11"),(11.0),(11),("11");
select * from t1;
drop table if exists t1;


create table t1 (c char(20));
insert into t1 values (5e-28);
select * from t1;
drop table t1;
create table t1 (c char(6));
insert into t1 values (2e5),(2e6),(2e-4),(2e-5);
select * from t1;
drop table t1;


CREATE TABLE t1 (
  reckey int unsigned NOT NULL,
  recdesc varchar(50) NOT NULL,
  PRIMARY KEY  (reckey)
);

INSERT INTO t1 VALUES (108, 'Has 108 as key');
INSERT INTO t1 VALUES (109, 'Has 109 as key');
select * from t1 where reckey=108;
select * from t1 where reckey=1.08E2;
select * from t1 where reckey=109;
select * from t1 where reckey=1.09E2;
drop table t1;

create table t1 (d double(10,1));
create table t2 (d double(10,9));
insert into t1 values ("100000000.0");
insert into t2 values ("1.23456780");
create table t3 select * from t2 union select * from t1;
select * from t3;

--replace_result $DEFAULT_ENGINE ENGINE
--show create table t3;
drop table t1, t2, t3;



create table t1 select  105213674794682365.00 + 0.0 x;
--show warnings;
--desc  t1;
drop table t1;

create table t1 select 0.0 x;
--desc t1;
create table t2 select 105213674794682365.00 y;
--desc t2;
create table t3 select x+y a from t1,t2;
--show warnings;
--desc t3;
drop table t1,t2,t3;


select 1e-308, 1.00000001e-300, 100000000e-300;

select 10e307;


create table t1(a int, b double(8, 2));
insert into t1 values
(1, 28.50), (1, 121.85), (1, 157.23), (1, 1351.00), (1, -1965.35), (1, 81.75), 
(1, 217.08), (1, 7.94), (4, 96.07), (4, 6404.65), (4, -6500.72), (2, 100.00),
(5, 5.00), (5, -2104.80), (5, 2033.80), (5, 0.07), (5, 65.93),
(3, -4986.24), (3, 5.00), (3, 4857.34), (3, 123.74), (3,  0.16),
(6, -1695.31), (6, 1003.77), (6, 499.72), (6, 191.82);
--replace_result 27 26 25 26
explain select sum(b) s from t1 group by a;
select sum(b) s from t1 group by a;
select sum(b) s from t1 group by a having s <> 0;
select sum(b) s from t1 group by a having s <> 0 order by s;
select sum(b) s from t1 group by a having s <=> 0;
select sum(b) s from t1 group by a having s <=> 0 order by s;
alter table t1 add k (a, b);
--replace_result 27 26 25 26
explain select sum(b) s from t1 group by a;
select sum(b) s from t1 group by a;
select sum(b) s from t1 group by a having s <> 0;
select sum(b) s from t1 group by a having s <> 0 order by s;
select sum(b) s from t1 group by a having s <=> 0;
select sum(b) s from t1 group by a having s <=> 0 order by s;
drop table t1;

--echo End of 4.1 tests

--error 1427
create table t1 (s1 float(0,2));
--error 1427
create table t1 (s1 float(1,2));


CREATE TABLE t1 (
  f1 real zerofill,
  f2 double zerofill,
  f3 float zerofill);
INSERT INTO t1 VALUES ( 0.314152e+1, 0.314152e+1, 0.314152e+1);

--let $my_stmt= select f1, f2, f3 FROM t1;
--eval PREPARE stmt1 FROM '$my_stmt';
--select f1, f2, f3 FROM t1;
--eval $my_stmt;
--EXECUTE stmt1;

DROP TABLE t1;


create table t1 (f1 double(200, 0));
insert into t1 values (1e199), (-1e199);
insert into t1 values (1e200), (-1e200);
insert into t1 values (2e200), (-2e200);
select f1 + 0e0 from t1;
drop table t1;

create table t1 (f1 float(30, 0));
insert into t1 values (1e29), (-1e29);
insert into t1 values (1e30), (-1e30);
insert into t1 values (2e30), (-2e30);
select f1 + 0e0 from t1;
drop table t1;


create table t1 (c char(6));
insert into t1 values (2e6),(2e-5);
select * from t1;
drop table t1;


CREATE TABLE d1 (d DOUBLE);
INSERT INTO d1 VALUES (1.7976931348623157E+308);
SELECT * FROM d1;
--error ER_ILLEGAL_VALUE_FOR_TYPE
INSERT INTO d1 VALUES (1.79769313486232e+308);
SELECT * FROM d1;
DROP TABLE d1;


create table t1 (a char(20));
insert into t1 values (1.225e-05);
select a+0 from t1;
drop table t1;



create table t1(d double, u bigint unsigned);

insert into t1(d) values (9.22337203685479e18),
                         (1.84e19);

update t1 set u = d;
select u from t1;

drop table t1;


CREATE TABLE t1 (f1 DOUBLE);
INSERT INTO t1 VALUES(-1.79769313486231e+308);
SELECT f1 FROM t1;
DROP TABLE t1;


select format(-1.7976931348623157E+307,256) as foo;
/* select least(-1.1111111111111111111111111,
             - group_concat(1.7976931348623157E+308)) as foo;
eval select concat((truncate((-1.7976931348623157E+307),(0x1e))),
                   ($nine_65)) into @a; */

select format(truncate('1.7976931348623157E+308',-12),1,'fr_BE') as foo;

--echo #
--echo # Bug #13500371 63704: CONVERSION OF '1.' TO A NUMBER GIVES ERROR 1265
--echo # (WARN_DATA_TRUNCATED)
--echo #

CREATE TABLE t1 (f FLOAT);
INSERT INTO t1 VALUES ('1.');
INSERT INTO t1 VALUES ('2.0.');
INSERT INTO t1 VALUES ('.');
SELECT * FROM t1 ORDER BY f;
DROP TABLE t1;
SET sql_mode = default;

--operator
drop table if exists test,t1,t2;
create table test(id float4);
insert into test values(2.5);
select -id from test;
create table t1 as select -id from test;
\d+ t1
select +id from test;
create table t2 as select +id from test;
\d+ t2
drop table t1,t2,test;

show sql_mode;
set sql_mode = '';
show sql_mode;
create table t1(id real);
\d+ t1;

set sql_mode to default;
show sql_mode;
create table t2(id real);
\d+ t2;
drop table t1,t2;

--neg--bug uint8
select ~2.5::float8;
select ~0.0::float8;
select ~(-2.5::float8);

--logic not
select !2.5::float8;--f 
select !0::float8;--t
select !(-2.5::float8);--f

--BUG#Z5183
CREATE TABLE tb_test (f FLOAT);
INSERT INTO tb_test VALUES (3.14), (0), (-2.5);
select ~f,-f,!f from tb_test;

--<=>
select 0.9::float4 <=> 0.0::float4;--f
select 1.15::float4 <=> 0.2::float4;--f
select 2.5::float4 <=> 2.5::float4;--t
select 2.5::float4 <=> 2.1::float4;--f
select 2.5::float4 <=> 2.8::float4;--f
select -2.5::float4 <=> 2.8::float4;--f
select -5.5::float4 <=> 2.2::float4;--f
select 0.9::float4 <=> NULL;--f
select NULL <=> 0.9::float4;--f

select 0.9::float8 <=> 0.0::float8;--f
select 1.15::float8 <=> 0.2::float8;--f
select 2.5::float8 <=> 2.5::float8;--t
select 2.5::float8 <=> 2.1::float8;--f
select 2.5::float8 <=> 2.8::float8;--f
select -2.5::float8 <=> 2.8::float8;--f
select -5.5::float8 <=> 2.2::float8;--f
select 0.9::float8 <=> NULL;--f
select NULL <=> 0.9::float8;--f
select NULL <=> NULL;--t

-- &
select 0.9::float8 & 0.0::float8;--0
select 1.15::float8 & 0.2::float8;--0
select 2.5::float8 & 2.5::float8;--2
select 2.5::float8 & 2.1::float8;--2
select 2.5::float8 & 2.8::float8;--2
select -2.5::float8 & 2.8::float8;--2
select -5.5::float8 & 2.2::float8;--2
select 0.9::float8 & NULL;--NULL
select NULL & 0.9::float8;--NULL

--<<
select 0.9::float8 << 0.0::float8;--1
select 1.15::float8 << 0.2::float8;--1
select 2.5::float8 << 2.5::float8;--8
select 2.5::float8 << 2.1::float8;--8
select 2.5::float8 << 2.8::float8;--16
select -2.5::float8 << 2.8::float8;--18446744073709551600
select -5.5::float8 << 2.2::float8;--18446744073709551592
select 0.9::float8 << NULL;--NULL
select NULL << 0.9::float8;--NULL

-->>
select 0.9::float8 >> 0.0::float8;--1
select 1.15::float8 >> 0.2::float8;--1
select 2.5::float8 >> 2.5::float8;--0
select 2.5::float8 >> 2.1::float8;--0
select 2.5::float8 >> 2.8::float8;--0
select -2.5::float8 >> 2.8::float8;--2305843009213693951
select -5.5::float8 >> 2.2::float8;--4611686018427387902
select 0.9::float8 >> NULL;--NULL
select NULL >> 0.9::float8;--NULL

--|
select 0.9::float8 | 0.0::float8;--1
select 1.15::float8 | 0.2::float8;--1
select 2.5::float8 | 2.5::float8;--2
select 2.5::float8 | 2.1::float8;--2
select 2.5::float8 | 2.8::float8;--3
select -2.5::float8 | 2.8::float8;--18446744073709551615
select -5.5::float8 | 2.2::float8;--18446744073709551610
select 0.9::float8 | NULL;--NULL
select NULL | 0.9::float8;--NULL

-- ^
select 0.9::float8 ^ 0.0::float8;--1
select 1.15::float8 ^ 0.2::float8;--1
select 2.5::float8 ^ 2.5::float8;--0
select 2.5::float8 ^ 2.1::float8;--0
select 2.5::float8 ^ 2.8::float8;--1
select -2.5::float8 ^ 2.8::float8;--18446744073709551613
select -5.5::float8 ^ 2.2::float8;--18446744073709551608
select 0.9::float8 ^ NULL;--NULL
select NULL ^ 0.9::float8;--NULL

--+
select 0.9::float4 + 0.0::float4;--0.8999999761581421
select 1.15::float4 + 0.2::float4;--1.3499999791383743
select 2.5::float4 + 2.5::float4;--5
select 2.5::float4 + 2.1::float4;--4.599999904632568
select 2.5::float4 + 2.8::float4;--5.299999952316284
select -2.5::float4 + 2.8::float4;--0.2999999523162842
select -5.5::float4 + 2.2::float4;---3.299999952316284
select 0.9::float4 + NULL;--NULL
select NULL + 0.9::float4;--NULL

---
select 0.9::float4 - 0.0::float4;--0.8999999761581421
select 1.15::float4 - 0.2::float4;--0.9499999731779099
select 2.5::float4 - 2.5::float4;--0
select 2.5::float4 - 2.1::float4;--0.40000009536743164
select 2.5::float4 - 2.8::float4;---0.2999999523162842
select -2.5::float4 - 2.8::float4;---5.299999952316284
select -5.5::float4 - 2.2::float4;---7.700000047683716
select 0.9::float4 - NULL;--NULL
select NULL - 0.9::float4;--NULL

--*
select 0.9::float4 * 0.0::float4;--0
select 1.15::float4 * 0.2::float4;--0.22999999865889542
select 2.5::float4 * 2.5::float4;--6.25
select 2.5::float4 * 2.1::float4;--5.249999761581421
select 2.5::float4 * 2.8::float4;--6.9999998807907104
select -2.5::float4 * 2.8::float4;---6.9999998807907104
select -5.5::float4 * 2.2::float4;---12.100000262260437
select 0.9::float4 * NULL;--NULL
select NULL * 0.9::float4;--NULL

--div
select 0.9::float4 div 0.0::float4;--NULL warning
select 1.15::float4 div 0.2::float4;--5
select 2.5::float4 div 2.5::float4;--1
select 2.5::float4 div 2.1::float4;--1
select 2.5::float4 div 2.8::float4;--0
select -2.5::float4 div 2.8::float4;--0
select -5.5::float4 div 2.2::float4;---2
select 0.9::float4 div NULL;--NULL
select NULL div 0.9::float4;--NULL
select 0.9::float8 div 0.0::float8;--NULL warning
select 1.15::float8 div 0.2::float8;--5
select 2.5::float8 div 2.5::float8;--1
select 2.5::float8 div 2.1::float8;--1
select 2.5::float8 div 2.8::float8;--0
select -2.5::float8 div 2.8::float8;--0
select -5.5::float8 div 2.2::float8;---2
select 0.9::float8 div NULL;--NULL
select NULL div 0.9::float8;--NULL
set sql_mode = '';
select 0.9::float4 div 0.0::float4;--NULL
select 1.15::float4 div 0.2::float4;--5
select 2.5::float4 div 2.5::float4;--1
select 2.5::float4 div 2.1::float4;--1
select 2.5::float4 div 2.8::float4;--0
select -2.5::float4 div 2.8::float4;--0
select -5.5::float4 div 2.2::float4;---2
select 0.9::float4 div NULL;--NULL
select NULL div 0.9::float4;--NULL
select 0.9::float8 div 0.0::float8;--NULL
select 1.15::float8 div 0.2::float8;--5
select 2.5::float8 div 2.5::float8;--1
select 2.5::float8 div 2.1::float8;--1
select 2.5::float8 div 2.8::float8;--0
select -2.5::float8 div 2.8::float8;--0
select -5.5::float8 div 2.2::float8;---2
select 0.9::float8 div NULL;--NULL
select NULL div 0.9::float8;--NULL
set sql_mode to default;
select 0.9::float4 // 0.0::float4;--NULL warning
select 1.15::float4 // 0.2::float4;--5
select 2.5::float4 // 2.5::float4;--1
select 2.5::float4 // 2.1::float4;--1
select 2.5::float4 // 2.8::float4;--0
select -2.5::float4 // 2.8::float4;--0
select -5.5::float4 // 2.2::float4;---2
select 0.9::float4 // NULL;--NULL
select NULL // 0.9::float4;--NULL
select 0.9::float8 // 0.0::float8;--NULL warning
select 1.15::float8 // 0.2::float8;--5
select 2.5::float8 // 2.5::float8;--1
select 2.5::float8 // 2.1::float8;--1
select 2.5::float8 // 2.8::float8;--0
select -2.5::float8 // 2.8::float8;--0
select -5.5::float8 // 2.2::float8;---2
select 0.9::float8 // NULL;--NULL
select NULL // 0.9::float8;--NULL
set sql_mode = '';
select 0.9::float4 // 0.0::float4;--NULL
select 1.15::float4 // 0.2::float4;--5
select 2.5::float4 // 2.5::float4;--1
select 2.5::float4 // 2.1::float4;--1
select 2.5::float4 // 2.8::float4;--0
select -2.5::float4 // 2.8::float4;--0
select -5.5::float4 // 2.2::float4;---2
select 0.9::float4 // NULL;--NULL
select NULL // 0.9::float4;--NULL
select 0.9::float8 // 0.0::float8;--NULL
select 1.15::float8 // 0.2::float8;--5
select 2.5::float8 // 2.5::float8;--1
select 2.5::float8 // 2.1::float8;--1
select 2.5::float8 // 2.8::float8;--0
select -2.5::float8 // 2.8::float8;--0
select -5.5::float8 // 2.2::float8;---2
select 0.9::float8 // NULL;--NULL
select NULL // 0.9::float8;--NULL
set sql_mode to default;
--/
select 0.9::float4 / 0.0::float4;--NULL warning
select 1.15::float4 / 0.2::float4;--5.749999795109036
select 2.5::float4 / 2.5::float4;--1
select 2.5::float4 / 2.1::float4;--1.1904762445393628
select 2.5::float4 / 2.8::float4;--0.8928571580624096
select -2.5::float4 / 2.8::float4;---0.8928571580624096
select -5.5::float4 / 2.2::float4;---2.4999999458139603
select 0.9::float4 / NULL;--NULL
select NULL / 0.9::float4;--NULL

select 0.9::float4 / 0.0::float8;--NULL warning
select 1.15::float4 / 0.2::float8;--5.7499998807907104
select 2.5::float4 / 2.5::float8;--1
select 2.5::float4 / 2.1::float8;--1.1904761904761905
select 2.5::float4 / 2.8::float8;--0.8928571428571429
select -2.5::float4 / 2.8::float8;---0.8928571428571429
select -5.5::float4 / 2.2::float8;--2.5

select 0.9::float8 / 0.0::float8;--NULL warning
select 1.15::float8 / 0.2::float8;--5.749999999999999
select 2.5::float8 / 2.5::float8;--1
select 2.5::float8 / 2.1::float8;--1.1904761904761905
select 2.5::float8 / 2.8::float8;--0.8928571428571429
select -2.5::float8 / 2.8::float8;---0.8928571428571429
select -5.5::float8 / 2.2::float8;---2.5
select 0.9::float8 / NULL;--NULL
select NULL / 0.9::float8;--NULL

select 0.9::float8 / 0.0::float4;--NULL warning
select 1.15::float8 / 0.2::float4;--5.749999914318324
select 2.5::float8 / 2.5::float4;--1
select 2.5::float8 / 2.1::float4;--1.1904762445393628
select 2.5::float8 / 2.8::float4;--0.8928571580624096
select -2.5::float8 / 2.8::float4;---0.8928571580624096
select -5.5::float8 / 2.2::float4;---2.4999999458139603

set sql_mode = '';
select 0.9::float4 / 0.0::float4;--NULL
select 1.15::float4 / 0.2::float4;--5.749999795109036
select 2.5::float4 / 2.5::float4;--1
select 2.5::float4 / 2.1::float4;--1.1904762445393628
select 2.5::float4 / 2.8::float4;--0.8928571580624096
select -2.5::float4 / 2.8::float4;---0.8928571580624096
select -5.5::float4 / 2.2::float4;---2.4999999458139603
select 0.9::float4 / NULL;--NULL
select NULL / 0.9::float4;--NULL

select 0.9::float4 / 0.0::float8;--NULL
select 1.15::float4 / 0.2::float8;--5.7499998807907104
select 2.5::float4 / 2.5::float8;--1
select 2.5::float4 / 2.1::float8;--1.1904761904761905
select 2.5::float4 / 2.8::float8;--0.8928571428571429
select -2.5::float4 / 2.8::float8;---0.8928571428571429
select -5.5::float4 / 2.2::float8;--2.5

select 0.9::float8 / 0.0::float8;--NULL
select 1.15::float8 / 0.2::float8;--5.749999999999999
select 2.5::float8 / 2.5::float8;--1
select 2.5::float8 / 2.1::float8;--1.1904761904761905
select 2.5::float8 / 2.8::float8;--0.8928571428571429
select -2.5::float8 / 2.8::float8;---0.8928571428571429
select -5.5::float8 / 2.2::float8;---2.5
select 0.9::float8 / NULL;--NULL
select NULL / 0.9::float8;--NULL

select 0.9::float8 / 0.0::float4;--NULL
select 1.15::float8 / 0.2::float4;--5.749999914318324
select 2.5::float8 / 2.5::float4;--1
select 2.5::float8 / 2.1::float4;--1.1904762445393628
select 2.5::float8 / 2.8::float4;--0.8928571580624096
select -2.5::float8 / 2.8::float4;---0.8928571580624096
select -5.5::float8 / 2.2::float4;---2.4999999458139603
set sql_mode to default;

--%
select 0.9::float8 % 0.0::float8;--NULL warning
select 1.15::float8 % 0.2::float8;--0.14999999999999986
select 2.5::float8 % 2.5::float8;--0
select 2.5::float8 % 2.1::float8;--0.3999999999999999
select 2.5::float8 % 2.8::float8;--2.5
select -2.5::float8 % 2.8::float8;---2.5
select -5.5::float8 % 2.2::float8;---1.0999999999999996
select 0.9::float8 % NULL;--NULL
select NULL % 0.9::float8;--NULL

set sql_mode = '';
select 0.9::float8 % 0.0::float8;--NULL
select 1.15::float8 % 0.2::float8;--0.14999999999999986
select 2.5::float8 % 2.5::float8;--0
select 2.5::float8 % 2.1::float8;--0.3999999999999999
select 2.5::float8 % 2.8::float8;--2.5
select -2.5::float8 % 2.8::float8;---2.5
select -5.5::float8 % 2.2::float8;---1.0999999999999996
select 0.9::float8 % NULL;--NULL
select NULL % 0.9::float8;--NULL
set sql_mode to default;

--XOR
select 0.9::float4 XOR 0.0::float4;--1
select 1.15::float4 XOR 0.2::float4;--1
select 2.5::float4 XOR 2.5::float4;--0
select 2.5::float4 XOR 2.1::float4;--0
select 2.5::float4 XOR 2.8::float4;--0
select -2.5::float4 XOR 2.8::float4;--0
select -5.5::float4 XOR 2.2::float4;--0
select 0.9::float4 XOR NULL;--NULL
select NULL XOR 0.9::float4;--NULL

select 0.9::float8 XOR 0.0::float8;--1
select 1.15::float8 XOR 0.2::float8;--1
select 2.5::float8 XOR 2.5::float8;--0
select 2.5::float8 XOR 2.1::float8;--0
select 2.5::float8 XOR 2.8::float8;--0
select -2.5::float8 XOR 2.8::float8;--0
select -5.5::float8 XOR 2.2::float8;--0
select 0.9::float8 XOR NULL;--NULL
select NULL XOR 0.9::float8;--NULL

--BUG#Z5182
create table tb_test1 (id float(0));
create table tb_test2 (id FLOAT(8.9));
drop table tb_test1,tb_test2;

--BUG#Z5204
CREATE TABLE t_aggregate_float (
id SERIAL PRIMARY KEY,
value FLOAT
);

INSERT INTO t_aggregate_float (value) VALUES (123456.789), (-100.5), (0), (50.25), (100.75), (123456.78910111213);
SELECT MAX(value) FROM t_aggregate_float;

SELECT SUM(value) FROM t_aggregate_float;
drop table t_aggregate_float;
--BUG#Z5351
CREATE TABLE db_test (
a float,
b float,
c double
);

INSERT INTO db_test (a, b, c) VALUES
(1.5, 2.5, 3.75),
(0.0, 1.0, 2.0),
(-3.5, -1.5, -2.5),
(NULL, 2.0, 3.0),
(4.0, 0.0, 1.0),
(1e3, 2e3, 3e3);

SELECT a / b, pg_typeof(a / b) FROM db_test;

drop table db_test;

--BUG#Z5355
CREATE TABLE db_test (
a double,
b double,
c float
);

INSERT INTO db_test (a, b, c) VALUES
(10.0, 3.0, 2.0),
(0.0, 1.0, 1.0),
(-8.0, -2.0, 4.0),
(NULL, 5.0, 1.0),
(256.0, 8.0, 2.0),
(1000.0, 0.0, 0.5);
SELECT a & b, pg_typeof(a & b) FROM db_test;
SELECT a | b, pg_typeof(a & b) FROM db_test;
drop table db_test;

--BUG#Z5201
CREATE TABLE db_test (a FLOAT);

INSERT INTO db_test VALUES
(3.0),
(3.14),
(2.9999999),
(3.0000001),
(NULL);

SELECT a,a <=> 3,a <=> 2023-01-01,a <=> '3.14' FROM db_test ORDER BY a;
SELECT a,a + 3,a + '3',a + '3.14' FROM db_test ORDER BY a;
SELECT a,a - 3,a - '3',a - '3.14' FROM db_test ORDER BY a;
SELECT a,a & 3,a & '3.14',a <=> '3.14',a << 1,a >> 1,a << '1',a >> '1',a | 1,a | '1', a | '3.14',a ^ 1,a ^ '1',a ^ '3.14' FROM db_test ORDER BY a;
SELECT a,a*3,a*2023-01-01,a*'3.14' FROM db_test ORDER BY a;
SELECT a,a/3,a /2023-01-01,a/'3.14' FROM db_test ORDER BY a;
drop table db_test;
