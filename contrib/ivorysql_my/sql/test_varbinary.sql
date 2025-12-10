DISCARD ALL;

/*
 * test mysql varbinary type.
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS
  my_varbinary_t18,
  my_varbinary_t17,
  my_varbinary_t16,
  my_varbinary_t15,
  my_varbinary_t14,
  my_varbinary_t13,
  my_varbinary_t12,
  my_varbinary_t11,
  my_varbinary_t10,
  my_varbinary_t09,
  my_varbinary_t08,
  my_varbinary_t07,
  my_varbinary_t06,
  my_varbinary_t05,
  my_varbinary_t04,
  my_varbinary_t03,
  my_varbinary_t02,
  my_varbinary_t01
;

--------------------------------------------------------------------------------

-- should ok
create table my_varbinary_t01 (c1 varbinary(6));

-- should ok
insert into my_varbinary_t01 values ('hgdb'), ('hgdb ');

-- should ok
insert into my_varbinary_t01 values ('hgdb\0\0');

-- should get {'hgdb', 'hgdb ', 'hgdb'}
select c1 from my_varbinary_t01;

-- should get {4,5,6}
select length(c1) from my_varbinary_t01;

-- should get {1,0,0}
select c1='hgdb' from my_varbinary_t01;

-- should get {0,1,0}
select c1='hgdb ' from my_varbinary_t01;

-- should get {0,0,1}
select c1='hgdb\0\0' from my_varbinary_t01;

-- should error BUG#5401
create table my_varbinary_t02(c1 varbinary);

-- should ok
create table my_varbinary_t02(c1 varbinary(10));

-- should ok
\d my_varbinary_t02;

-- should error
create table my_varbinary_t03(c1 varbinary(-1));

-- should ok
create table my_varbinary_t04(c1 varbinary(0));
\d my_varbinary_t04

-- should ok
create table my_varbinary_t05(c1 varbinary(1));
-- should ok and show varbinary(1)
\d my_varbinary_t05

-- should ok
create table my_varbinary_t06(c1 varbinary(655.32));
-- should ok and show varbinary(655)
\d my_varbinary_t06

-- should ok
create table my_varbinary_t07(c1 varbinary(65532));

-- should error
create table my_varbinary_t08(c1 varbinary(65533));
-- should error
create table my_varbinary_t08(c1 varbinary(65534));
-- should error
create table my_varbinary_t08(c1 varbinary(65535));

-- TODO[TD023]: should ok
create table my_varbinary_t09(c1 varbinary(65533) not null);

-- should error
create table my_varbinary_t10(c1 varbinary(65530), c2 varbinary(2));

SET sql_mode = '';

-- should ok and warnings
create table my_varbinary_t11(c1 varbinary(65536), c2 varbinary(16777215));
-- should show mediumblob
\d my_varbinary_t11

-- should ok and warnings
create table my_varbinary_t12(c1 varbinary(16777216), c2 varbinary(4294967295));
-- should show longblob
\d my_varbinary_t12

-- TODO[TD012]: should syntax error
create table my_varbinary_t13(c1 varbinary(4294967296));

RESET sql_mode;

-- should syntax error
create table my_varbinary_t14(c1 varbinary('5'));
-- should syntax error
create table my_varbinary_t14(c1 varbinary('a'));
-- should syntax error
create table my_varbinary_t14(c1 varbinary(''));
-- should syntax error
create table my_varbinary_t14(c1 varbinary(null));
-- should syntax error
create table my_varbinary_t14(c1 varbinary(a));

-- should ok
create table my_varbinary_t15 (c1 varbinary(8));
-- should ok
insert into  my_varbinary_t15 values ('HGDB');
-- should ok
insert into  my_varbinary_t15 values ('Database');
-- should error
insert into  my_varbinary_t15 values ('Database1');
-- should error
insert into  my_varbinary_t15 values ('Database ');
-- should error
insert into  my_varbinary_t15 values ('数据库');

-- should ok
create table my_varbinary_t16 (c1 varbinary(0));
-- should ok
insert into  my_varbinary_t16 values (NULL);
-- should ok
insert into  my_varbinary_t16 values ('');
-- should error
insert into  my_varbinary_t16 values (' ');
-- should error
insert into  my_varbinary_t16 values ('1');
-- should error
insert into  my_varbinary_t16 values ('a');

-- should ok
create table my_varbinary_t17 (c1 varbinary(5.6));
-- should ok
insert into  my_varbinary_t17 values ('HGDB');
-- should error
insert into  my_varbinary_t17 values ('HGDB  ');
-- should error
insert into  my_varbinary_t17 values ('HGDB   ');

-- should ok
create table my_varbinary_t18 (c1 int, c2 varbinary(4) default 'test');
-- should ok
insert into my_varbinary_t18 (c1) values (1), (2);
-- should get {1,'test'}, {2,'test'}
select * from my_varbinary_t18;

-- should syntax error
select varbinary'101010' as result;

SET sql_mode = '';

-- should warning
insert into  my_varbinary_t15 values ('Database1');
-- should warning
insert into  my_varbinary_t15 values ('Database ');
-- should warning
insert into  my_varbinary_t15 values ('数据库');

-- should warning
insert into  my_varbinary_t16 values (' ');
-- should warning
insert into  my_varbinary_t16 values ('1');
-- should warning
insert into  my_varbinary_t16 values ('a');

-- should warning
insert into  my_varbinary_t17 values ('HGDB  ');
-- should warning
insert into  my_varbinary_t17 values ('HGDB   ');

RESET sql_mode;

DISCARD ALL;

/*
 * test mysql varbinary type index.
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set enable_seqscan = off;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1 varbinary(10),
  c2 varbinary(10),
  c3 varbinary(10),
  c4 varbinary(10)
);

INSERT INTO my_varbinary_t1 VALUES ('abcdefghij', 'bcdefghijk', 'cdefghijkl', 'defghijklm');
INSERT INTO my_varbinary_t1 VALUES ('abcdefghij', 'bcdefghijk', 'cdefghijkl', 'defghijklm');
INSERT INTO my_varbinary_t1 VALUES ('abcdefghij', 'bcdefghijk', 'cdefghijkl', 'defghijklm');

-- case: gin
create index my_varbinary_t1_gin on my_varbinary_t1 using gin(c1);
select c1 from my_varbinary_t1 where c1 = 'abcdefghij';
select c1 from my_varbinary_t1 where c1 = cast('abcdefghij' as binary);
select c1 from my_varbinary_t1 where c1 = cast('abcdefghij' as binary(10));
explain select c1 from my_varbinary_t1 where c1 = 'abcdefghij';

-- case: brin
create index my_varbinary_t1_brin on my_varbinary_t1 using brin(c2);
select c2 from my_varbinary_t1 where c2 = 'bcdefghijk';
select c2 from my_varbinary_t1 where c2 = cast('bcdefghijk' as binary);
select c2 from my_varbinary_t1 where c2 = cast('bcdefghijk' as binary(10));
explain select c2 from my_varbinary_t1 where c2 = 'bcdefghijk';

-- case: btree
create index my_varbinary_t1_btree on my_varbinary_t1 using btree(c3);
select c3 from my_varbinary_t1 where c3 = 'cdefghijkl';
select c3 from my_varbinary_t1 where c3 = cast('cdefghijkl' as binary);
select c3 from my_varbinary_t1 where c3 = cast('cdefghijkl' as binary(10));
explain select c3 from my_varbinary_t1 where c3 = 'cdefghijkl';

-- case: hash
create index my_varbinary_t1_hash on my_varbinary_t1 using hash(c4);
select c4 from my_varbinary_t1 where c4 = 'defghijklm';
select c4 from my_varbinary_t1 where c4 = cast('defghijklm' as binary);
select c4 from my_varbinary_t1 where c4 = cast('defghijklm' as binary(10));
explain select c4 from my_varbinary_t1 where c4 = 'defghijklm';

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1 varbinary(5),
  c2 varbinary(10),
  c3 varbinary(10),
  c4 varbinary(10),
  c5 varbinary(10),
  c6 varbinary(10),
  c7 varbinary(10),
  c8 varbinary(10)
);

INSERT INTO my_varbinary_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- TODO[TD015]: should get (1, 1, 1, 1, 0, 0, 0, 0, 0, 0)
select c1 AND c1, c1 AND c2, c1 AND c3, c2 AND c3, c3 AND c4, c4 AND c5, c5 AND c6, c6 AND c7, c4 AND c8, c6 AND c8 from my_varbinary_t1;

-- TODO[TD015]: should get (1, 1, 1, 1, 0, 0, 0, 0, 0, 0)
select c1 && c1, c1 && c2, c1 && c3, c2 && c3, c3 && c4, c4 && c5, c5 && c6, c6 && c7, c4 && c8, c6 && c8 from my_varbinary_t1;

DISCARD ALL;

/*
 * test mysql varbinary type operator between and
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1  varbinary(5)
);

INSERT INTO my_varbinary_t1 VALUES ('123'), ('12345'), ('abc');

--------------------------------------------------------------------------------

-- should get {123}
SELECT * from my_varbinary_t1 WHERE c1 BETWEEN '12' AND '1234';

-- should get {123, 12345}
SELECT * from my_varbinary_t1 WHERE c1 BETWEEN '12' AND '12346';

-- should get empty
SELECT * from my_varbinary_t1 WHERE c1 BETWEEN '124' AND '999';

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1 varbinary(5),
  c2 varbinary(10),
  c3 varbinary(10),
  c4 varbinary(10),
  c5 varbinary(10),
  c6 varbinary(10),
  c7 varbinary(10),
  c8 varbinary(10)
);

INSERT INTO my_varbinary_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- TODO[TD021]: should get (123, 123, 82, 82, 0, 0, 0, 0, NULL, NULL) and warnings
select c1&c1, c1&c2, c1&c3, c2&c3, c3&c4, c4&c5, c5&c6, c6&c7, c4&c8, c6&c8 from my_varbinary_t1;

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1 varbinary(5),
  c2 varbinary(10),
  c3 varbinary(10),
  c4 varbinary(10),
  c5 varbinary(10),
  c6 varbinary(10),
  c7 varbinary(10),
  c8 varbinary(10)
);

INSERT INTO my_varbinary_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- should get (6, 6, 5, 0, 0, 0, 0, NULL) and warning
select
  bit_count(c1),
  bit_count(c2),
  bit_count(c3),
  bit_count(c4),
  bit_count(c5),
  bit_count(c6),
  bit_count(c7),
  bit_count(c8)
from
  my_varbinary_t1;

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1 varbinary(5),
  c2 varbinary(10),
  c3 varbinary(10),
  c4 varbinary(10),
  c5 varbinary(10),
  c6 varbinary(10),
  c7 varbinary(10),
  c8 varbinary(10)
);

INSERT INTO my_varbinary_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- should get "(18446744073709551492, 18446744073709551492, 18446744073709550381, 18446744073709551615, 18446744073709551615, 18446744073709551615, 18446744073709551615, NULL)"
select ~c1, ~c2, ~c3, ~c4, ~c5, ~c6, ~c7, ~c8 from my_varbinary_t1;

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1 varbinary(5),
  c2 varbinary(10),
  c3 varbinary(10),
  c4 varbinary(10),
  c5 varbinary(10),
  c6 varbinary(10),
  c7 varbinary(10),
  c8 varbinary(10)
);

INSERT INTO my_varbinary_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- TODO[TD021]: should get (123, 123, 1275, 1275, 1234, 0, 0, 0, NULL, NULL) and warnings
select
  c1|c1,
  c1|c2,
  c1|c3,
  c2|c3,
  c3|c4,
  c4|c5,
  c5|c6,
  c6|c7,
  c4|c8,
  c6|c8
from my_varbinary_t1;
DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1 varbinary(5),
  c2 varbinary(10),
  c3 varbinary(10),
  c4 varbinary(10),
  c5 varbinary(10),
  c6 varbinary(10),
  c7 varbinary(10),
  c8 varbinary(10)
);

INSERT INTO my_varbinary_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- should get {246, 246, 2468, 0, 0, 0, 0, NULL}
select c1<<1, c2<<1, c3<<1, c4<<2, c5<<2, c6<<1, c7<<2, c8<<3 from my_varbinary_t1;

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1 varbinary(5),
  c2 varbinary(10),
  c3 varbinary(10),
  c4 varbinary(10),
  c5 varbinary(10),
  c6 varbinary(10),
  c7 varbinary(10),
  c8 varbinary(10)
);

INSERT INTO my_varbinary_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- should get (61, 61, 617, 0, 0, 0, 0, NULL)
select c1>>1, c2>>1, c3>>1, c4>>2, c5>>2, c6>>1, c7>>2, c8>>3 from my_varbinary_t1;

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1 varbinary(5),
  c2 varbinary(10),
  c3 varbinary(10),
  c4 varbinary(10),
  c5 varbinary(10),
  c6 varbinary(10),
  c7 varbinary(10),
  c8 varbinary(10)
);

INSERT INTO my_varbinary_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- TODO[TD021]: should get "(0, 0, 1193, 1193, 1234, 0, 0, 0, NULL, NULL)" and warnings
select c1^c1, c1^c2, c1^c3, c2^c3, c3^c4, c4^c5, c5^c6, c6^c7, c4^c8, c6^c8 from my_varbinary_t1;

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1 varbinary(5),
  c2 varbinary(5),
  c3 varbinary(5),
  c4 varbinary(5),
  c5 varbinary(5)
);

INSERT INTO my_varbinary_t1 VALUES ('123', 'abc', '', ' ', null);

--------------------------------------------------------------------------------

-- should get {123, abc,  ,  , NULL}
SELECT
  COALESCE(c1, c5),
  COALESCE(c5, c2),
  COALESCE(c5, c3),
  COALESCE(c5, c4),
  COALESCE(c5, c5)
FROM
  my_varbinary_t1;

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;
DROP TABLE IF EXISTS my_varbinary_t2;

CREATE TABLE my_varbinary_t1(
  c1 varbinary(3),
  c2 varbinary(3),
  c3 varbinary(3),
  c4 varbinary(3),
  c5 varbinary(3),
  c6 varbinary(10),
  c7 varbinary(10),
  c8 varbinary(10),
  c9 varbinary(10)
);

CREATE TABLE my_varbinary_t2(
  c1 varbinary(3),
  c2 varbinary(3),
  c3 varbinary(3),
  c4 varbinary(3),
  c5 varbinary(10),
  c6 varbinary(10),
  c7 varbinary(10),
  c8 varbinary(10)
);

INSERT INTO my_varbinary_t1 VALUES ('2', '3', 'a', '2m', 'b2b', '4', '', ' 2', null);
INSERT INTO my_varbinary_t2 VALUES ('4', '3', '2', '2.5', '4', '', ' ', null);

--------------------------------------------------------------------------------

-- should get (1, 1, 2, 1, 1, NULL, NULL, NULL, NULL)
select c1 div c1, c1 div c2, c1 div c3, c1 div c4, c1 div c5, c5 div c6, c5 div c7, c5 div c8, c7 div c8 from my_varbinary_t2;

-- should get (1, 1.5, 0, 1, 0, 2, NULL, 2, 0, NULL) and warnings
select c1/c1, c2/c1, c3/c1, c4/c1, c5/c1, c6/c1, c6/c7, c6/c8, c7/c8, c6/c9 from my_varbinary_t1;

DISCARD ALL;

/*
 * MySQL varbinary type operator greater than test.
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1  varbinary(5),
  c2  varbinary(5),
  c3  varbinary(5),
  c4  varbinary(5),
  c5  varbinary(5),
  c6  varbinary(10),
  c7  varbinary(10),
  c8  varbinary(10),
  c9  varbinary(10),
  c10 varbinary(10),
  c11 varbinary(10)
);

INSERT INTO my_varbinary_t1 values('123', '123', '1234', '', null, '1234', 'abcd', 'ABCD', '', null, NULL);

--------------------------------------------------------------------------------

-- TODO[TD015]: should get (1, 0, 1, 0, 0, 0, 0, 0, 1, NULL, NULL, NULL), but get (t, f, t, f, f, f, f, f, t, , , )
select
  c1 = c2,
  c2 = c3,
  c3 = c6,
  c6 = c7,
  c7 = c8,
  c1 = c4,
  c1 = c5,
  c4 = c6,
  c4 = c9,
  c4 = c10,
  c5 = c10,
  c10 = c11
from
  my_varbinary_t1;

DISCARD ALL;

/*
 * MySQL varbinary type operator greater equal test.
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set enable_seqscan = off;

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1  varbinary(5),
  c2  varbinary(5),
  c3  varbinary(5),
  c4  varbinary(5),
  c5  varbinary(5),
  c6  varbinary(10),
  c7  varbinary(10),
  c8  varbinary(10),
  c9  varbinary(10),
  c10 varbinary(10),
  c11 varbinary(10)
);

INSERT INTO my_varbinary_t1 values('123', '123', '1234', '', null, '1234', 'abcd', 'ABCD', '', null, NULL);

--------------------------------------------------------------------------------

-- TODO[TD015]: should get (1, 0, 1, 0, 1, 1, NULL, 0, 1, NULL, NULL, NULL), but get (t, f, t, f, t, t, , f, t, , , )
select
  c1 >= c2,
  c2 >= c3,
  c3 >= c6,
  c6 >= c7,
  c7 >= c8,
  c1 >= c4,
  c1 >= c5,
  c4 >= c6,
  c4 >= c9,
  c4 >= c10,
  c5 >= c10,
  c10 >= c11
from
  my_varbinary_t1;

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1 varbinary(5),
  c2 varbinary(5),
  c3 varbinary(5),
  c4 varbinary(5),
  c5 varbinary(5),
  c6 varbinary(5)
);

INSERT INTO my_varbinary_t1 VALUES ('123', '123', 'abc', '', ' ', null);

--------------------------------------------------------------------------------

-- should get {123}
SELECT GREATEST(c1, c2) FROM my_varbinary_t1;

-- should get (3)
SELECT length(GREATEST(c1, c2)) FROM my_varbinary_t1;

-- should get {abc}
SELECT GREATEST(c1, c3) FROM my_varbinary_t1;

-- should get {abc}
SELECT GREATEST(c3, c4) FROM my_varbinary_t1;

-- should get {}
SELECT GREATEST(c4, c5) FROM my_varbinary_t1;

-- should get {NULL}
SELECT GREATEST(c4, c6) FROM my_varbinary_t1;

-- should get (123, 3, abc, abc, , NULL)
SELECT
  GREATEST(c1, c2),
  length(GREATEST(c1, c2)),
  GREATEST(c1, c3),
  GREATEST(c3, c4),
  GREATEST(c4, c5),
  GREATEST(c4, c6)
FROM
  my_varbinary_t1;

DISCARD ALL;

/*
 * MySQL varbinary type operator greater than test.
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1  varbinary(5),
  c2  varbinary(5),
  c3  varbinary(5),
  c4  varbinary(5),
  c5  varbinary(5),
  c6  varbinary(10),
  c7  varbinary(10),
  c8  varbinary(10),
  c9  varbinary(10),
  c10 varbinary(10),
  c11 varbinary(10)
);

INSERT INTO my_varbinary_t1 values ('123', '123', '1234', '', null, '1234', 'abcd', 'ABCD', '', null, NULL);

--------------------------------------------------------------------------------

-- TODO[TD015]: should get (0, 0, 0, 0, 1, 1, NULL, 0, 0, NULL, NULL, NULL), but get (f, f, f, f, t, t, , f, f, , , )
select
  c1 > c2,
  c2 > c3,
  c3 > c6,
  c6 > c7,
  c7 > c8,
  c1 > c4,
  c1 > c5,
  c4 > c6,
  c4 > c9,
  c4 > c10,
  c5 > c10,
  c10 > c11
from
  my_varbinary_t1;

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1 varbinary(6)
);

INSERT INTO my_varbinary_t1 VALUES ('abc'), ('abc123'), ('Abc123'), ('瀚高'), (''), (' '), (null);

--------------------------------------------------------------------------------

-- TODO[TD007]: should get {abc, abc123, 瀚高}
SELECT * FROM my_varbinary_t1 WHERE c1
  IN ('abc', 'abc123', '瀚高', '\0\0\0\0\0\0', ' \0\0\0\0\0', null);

-- TODO[TD007]: should get {abc123, Abc123, 瀚高, , }
SELECT * FROM my_varbinary_t1 WHERE c1
  IN ('abc\0\0\0', 'abc123', 'Abc123', '瀚高', '', ' ', null);

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1 varbinary(5),
  c2 varbinary(5),
  c3 varbinary(5),
  c4 varbinary(5),
  c5 varbinary(5),
  c6 varbinary(5),
  c7 varbinary(5)
);

INSERT INTO my_varbinary_t1 VALUES ('123', '121', 'abcd', '125', '', ' ', null);

--------------------------------------------------------------------------------

-- TODO[TD009]: should get '2', but get error, need to impl this function
SELECT interval(c1, c2, c3, c4, c5, c6, c7) FROM my_varbinary_t1;

-- TODO[TD009]: should get '3', but get error, need to impl this function
SELECT interval(c1, c7, c6, c5, c4, c3, c2) FROM my_varbinary_t1;

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  id int,
  c1 varbinary(10)
);

INSERT INTO my_varbinary_t1 VALUES (1, '123'), (2, 'abc'), (3, ''), (4, ' '), (5, null);

--------------------------------------------------------------------------------

-- should get {(1, 123), (2, abc), (3, ), (4, )}
select * from my_varbinary_t1 where c1 is not null;

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  id int,
  c1 varbinary(10)
);

INSERT INTO my_varbinary_t1 VALUES (1, '123'), (2, 'abc'), (3, ''), (4, ' '), (5, null);

--------------------------------------------------------------------------------

-- should get {(2, abc), (3, ), (4, ), (5, NULL)}
select * from my_varbinary_t1 where c1 is not true;

-- should get {(1, 123), (5, NULL)}
select * from my_varbinary_t1 where c1 is not false;

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  id int,
  c1 varbinary(10)
);

INSERT INTO my_varbinary_t1 VALUES (1, '123'), (2, 'abc'), (3, ''), (4, ' '), (5, null);

--------------------------------------------------------------------------------

-- should get (5, null)
select * from my_varbinary_t1 where c1 is null;

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1 varbinary(5),
  c2 varbinary(5),
  c3 varbinary(5),
  c4 varbinary(5),
  c5 varbinary(5)
);

INSERT INTO my_varbinary_t1 VALUES ('123', 'abc', '', ' ', null);

--------------------------------------------------------------------------------

-- TODO[TD010]: should get (0, 0, 0, 0, 1), but not as expected
select isnull(c1), isnull(c2), ISNULL(c3), ISNULL(c4), ISNULL(c5) from my_varbinary_t1;

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  id int,
  c1 varbinary(10)
);

INSERT INTO my_varbinary_t1 VALUES (1, '123'), (2, 'abc'), (3, ''), (4, ' '), (5, null);

--------------------------------------------------------------------------------

-- should get (1, 123)
select * from my_varbinary_t1 where c1 is true;

-- should get {(2, abc), (3, ), (4, )}
select * from my_varbinary_t1 where c1 is false;

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1 varbinary(5),
  c2 varbinary(10),
  c3 varbinary(5),
  c4 varbinary(5),
  c5 varbinary(5),
  c6 varbinary(5)
);

INSERT INTO my_varbinary_t1 VALUES ('123', '123', 'abc', '', ' ', null);

--------------------------------------------------------------------------------

-- should get (123, 3, 123, , , NULL)
select least(c1, c2), length(least(c1, c2)), least(c1, c3), least(c3, c4), least(c4, c5), least(c4, c6) from my_varbinary_t1;

DISCARD ALL;

/*
 * MySQL varbinary type operator greater than test.
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1  varbinary(5),
  c2  varbinary(5),
  c3  varbinary(5),
  c4  varbinary(5),
  c5  varbinary(5),
  c6  varbinary(10),
  c7  varbinary(10),
  c8  varbinary(10),
  c9  varbinary(10),
  c10 varbinary(10),
  c11 varbinary(10)
);

INSERT INTO my_varbinary_t1 values('123', '123', '1234', '', null, '1234', 'abcd', 'ABCD', '', null, NULL);

--------------------------------------------------------------------------------

-- TODO[TD015]: should get (1, 1, 1, 1, 0, 0, NULL, 1, 1, NULL, NULL, NULL), but get (t, t, t, t, f, f, , t, t, , , )
select
  c1 <= c2,
  c2 <= c3,
  c3 <= c6,
  c6 <= c7,
  c7 <= c8,
  c1 <= c4,
  c1 <= c5,
  c4 <= c6,
  c4 <= c9,
  c4 <= c10,
  c5 <= c10,
  c10 <= c11
from
  my_varbinary_t1;

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1 varbinary(10)
);

INSERT INTO my_varbinary_t1 VALUES (' bc'), ('abc'), ('Abc'), ('abcd'), ('瀚高');

--------------------------------------------------------------------------------

-- should get {abc, abcd}
select * from my_varbinary_t1 where c1 like 'a%';

-- should get (Abc)
select * from my_varbinary_t1 where c1 like 'A%';

-- should get { bc, abc, Abc}
select * from my_varbinary_t1 where c1 like '%c';

-- should get { bc, abc, Abc, abcd}
select * from my_varbinary_t1 where c1 like '%b%';

-- should get { bc, abc, Abc, abcd, 瀚高}
-- TODO[TD007]: can not display chinese
select * from my_varbinary_t1 where c1 like '%';

-- should get (瀚高)
-- TODO[TD007]: can not display chinese
select * from my_varbinary_t1 where c1 like '%瀚%';

DISCARD ALL;

/*
 * MySQL varbinary type operator greater than test.
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1  varbinary(5),
  c2  varbinary(5),
  c3  varbinary(5),
  c4  varbinary(5),
  c5  varbinary(5),
  c6  varbinary(10),
  c7  varbinary(10),
  c8  varbinary(10),
  c9  varbinary(10),
  c10 varbinary(10),
  c11 varbinary(10)
);

INSERT INTO my_varbinary_t1 values('123', '123', '1234', '', null, '1234', 'abcd', 'ABCD', '', null, NULL);

-- TODO[TD015]: should get (0, 1, 0, 1, 0, 0, NULL, 1, 0, NULL, NULL, NULL), but get (f, t, f, t, f, f, , t, f, , , )
select
  c1 < c2,
  c2 < c3,
  c3 < c6,
  c6 < c7,
  c7 < c8,
  c1 < c4,
  c1 < c5,
  c4 < c6,
  c4 < c9,
  c4 < c10,
  c5 < c10,
  c10 < c11
from
  my_varbinary_t1;

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;
DROP TABLE IF EXISTS my_varbinary_t2;

CREATE TABLE my_varbinary_t1(
  c1 varbinary(3),
  c2 varbinary(3),
  c3 varbinary(3),
  c4 varbinary(3),
  c5 varbinary(3),
  c6 varbinary(10),
  c7 varbinary(10),
  c8 varbinary(10),
  c9 varbinary(10)
);

CREATE TABLE my_varbinary_t2(
  c1 varbinary(5),
  c2 varbinary(5),
  c3 varbinary(5),
  c4 varbinary(5),
  c5 varbinary(5),
  c6 varbinary(5),
  c7 varbinary(5),
  c8 varbinary(5)
);

INSERT INTO my_varbinary_t1 VALUES ('123', '999', 'a', '2m', 'b2b', '123', '', ' 12', null);
INSERT INTO my_varbinary_t2 VALUES ('123', '-123', 'a', '2m', '', ' ', ' 12', null);

--------------------------------------------------------------------------------

-- TODO[TD021]: should get (0, -876, 123, 121, 123, 0, 123, 111, -12, NULL) and warnings
select c1-c1, c1-c2, c1-c3, c1-c4, c1-c5, c1-c6, c6-c7, c6-c8, c7-c8, c6-c9 from my_varbinary_t1;

-- TODO[TD021]: should get (-123, 123, -0, -2, -0, 0, -12, NULL) and warnings
select -c1, -c2, -c3, -c4, -c5, -(-c6), -c7, -c8 from my_varbinary_t2;

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1 varbinary(3),
  c2 varbinary(3),
  c3 varbinary(3),
  c4 varbinary(3),
  c5 varbinary(10),
  c6 varbinary(10),
  c7 varbinary(10),
  c8 varbinary(10)
);

INSERT INTO my_varbinary_t1 VALUES ('4', '3', '2', '2.5', '4', '', ' ', null);

--------------------------------------------------------------------------------

-- TODO[TD021]: should get (0, 1, 0, 1.5, 0, NULL, NULL, NULL, NULL) and warnings
select c1%c1, c1%c2, c1%c3, c1%c4, c1%c5, c5%c6, c5%c7, c5%c8, c7%c8 from my_varbinary_t1;

-- TODO[TD021]: should get (0, 1, 0, 1.5, 0, NULL, NULL, NULL, NULL) and warnings
select c1 mod c1, c1 mod c2, c1 mod c3, c1 mod c4, c1 mod c5, c5 mod c6, c5 mod c7, c5 mod c8, c7 mod c8 from my_varbinary_t1;

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1 varbinary(3),
  c2 varbinary(3),
  c3 varbinary(3),
  c4 varbinary(3),
  c5 varbinary(3),
  c6 varbinary(10),
  c7 varbinary(10),
  c8 varbinary(10),
  c9 varbinary(10)
);

INSERT INTO my_varbinary_t1 VALUES ('2', '3', 'a', '2m', 'b2b', '4', '', ' 2', null);

--------------------------------------------------------------------------------

-- TODO[TD021]: should get (4, 6, 0, 4, 0, 8, 0, 8, 0, NULL) and warnings
select c1*c1, c1*c2, c1*c3, c1*c4, c1*c5, c1*c6, c6*c7, c6*c8, c7*c8, c6*c9 from my_varbinary_t1;

DISCARD ALL;

/*
 * MySQL varbinary type operator greater equal test.
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1  varbinary(5),
  c2  varbinary(5),
  c3  varbinary(5),
  c4  varbinary(5),
  c5  varbinary(5),
  c6  varbinary(10),
  c7  varbinary(10),
  c8  varbinary(10),
  c9  varbinary(10),
  c10 varbinary(10),
  c11 varbinary(10)
);

INSERT INTO my_varbinary_t1 values('123', '123', '1234', '', null, '1234', 'abcd', 'ABCD', '', null, NULL);

--------------------------------------------------------------------------------

-- TODO[TD015]: should get (0, 1, 0, 1, 1, 1, NULL, 1, 0, NULL, NULL, NULL), but get (f, t, f, t, t, t, , t, f, , , )
select
  c1 <> c2,
  c2 <> c3,
  c3 <> c6,
  c6 <> c7,
  c7 <> c8,
  c1 <> c4,
  c1 <> c5,
  c4 <> c6,
  c4 <> c9,
  c4 <> c10,
  c5 <> c10,
  c10 <> c11
from
  my_varbinary_t1;

-- TODO[TD015]: should get (0, 1, 0, 1, 1, 1, NULL, 1, 0, NULL, NULL, NULL), but get (f, t, f, t, t, t, , t, f, , , )
select
  c1 != c2,
  c2 != c3,
  c3 != c6,
  c6 != c7,
  c7 != c8,
  c1 != c4,
  c1 != c5,
  c4 != c6,
  c4 != c9,
  c4 != c10,
  c5 != c10,
  c10 != c11
from
  my_varbinary_t1;

DISCARD ALL;

/*
 * test mysql varbinary type operator between and
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1  varbinary(5)
);

INSERT INTO my_varbinary_t1 VALUES ('123'), ('12345'), ('abc');

--------------------------------------------------------------------------------

-- should get {12345, abc}
SELECT * from my_varbinary_t1 WHERE c1 NOT BETWEEN '12' AND '1234';

-- should get {abc}
SELECT * from my_varbinary_t1 WHERE c1 NOT BETWEEN '12' AND '12346';

-- should get {123, 12345, abc}
SELECT * from my_varbinary_t1 WHERE c1 NOT BETWEEN '124' AND '999';

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1 varbinary(6)
);

INSERT INTO my_varbinary_t1 VALUES ('abc'), ('abc123'), ('Abc123'), ('瀚高'), (''), (' '), (null);

--------------------------------------------------------------------------------

-- should get empty
SELECT * FROM my_varbinary_t1 WHERE c1
  NOT IN ('abc', 'abc123', '瀚高', '\0\0\0\0\0\0', ' \0\0\0\0\0', null);

-- should get empty
SELECT * FROM my_varbinary_t1 WHERE c1
  NOT IN ('abc\0\0\0', 'abc123', 'Abc123', '瀚高', '', ' ', null);

-- should get {Abc123, , }
select * from my_varbinary_t1 where c1
  not in ('abc', 'abc123', '瀚高');

-- TODO[TD007]: should get {abc, abc123, 瀚高, , }
select * from my_varbinary_t1 where c1
  not in ('abc\0\0\0', 'Abc123', '\0\0\0\0\0\0', ' \0\0\0\0\0');

-- should get empty
select * from my_varbinary_t1 where c1
  not in ('abc', null);

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1 varbinary(10)
);

INSERT INTO my_varbinary_t1 VALUES (' bc'), ('abc'), ('Abc'), ('abcd'), ('瀚高');

--------------------------------------------------------------------------------

-- should get { bc, Abc, 瀚高}
-- TODO[TD007]: can not display chinese
select * from my_varbinary_t1 where c1 not like 'a%';

-- shoud get { bc, abc, abcd, 瀚高}
-- TODO[TD007]: can not display chinese
select * from my_varbinary_t1 where c1 not like 'A%';

-- shoud get {abcd, 瀚高}
-- TODO[TD007]: can not display chinese
select * from my_varbinary_t1 where c1 not like '%c';

-- soud get {瀚高}
-- TODO[TD007]: can not display chinese
select * from my_varbinary_t1 where c1 not like '%b%';

-- shoud get empty
select * from my_varbinary_t1 where c1 not like '%';

-- shoud get { bc, abc, Abc, abcd}
select * from my_varbinary_t1 where c1 not like '%瀚%';

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1 varbinary(5),
  c2 varbinary(10),
  c3 varbinary(10),
  c4 varbinary(10),
  c5 varbinary(10),
  c6 varbinary(10),
  c7 varbinary(10),
  c8 varbinary(10)
);

INSERT INTO my_varbinary_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- TODO[TD015]: should get (0, 0, 0, 1, 1, 1, 1, NULL) and warnings
select not c1, not c2, not c3, not c4, not c5, not c6, not c7, not c8 from my_varbinary_t1;

-- TODO[TD015]: should get (0, 0, 0, 1, 1, 1, 1, NULL) and warnings
select !c1, !c2, !c3, !c4, !c5, !c6, !c7, !c8 from my_varbinary_t1;

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1 varbinary(5),
  c2 varbinary(10),
  c3 varbinary(10),
  c4 varbinary(10),
  c5 varbinary(10),
  c6 varbinary(10),
  c7 varbinary(10),
  c8 varbinary(10)
);

INSERT INTO my_varbinary_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- TODO[TD015]: should get (1, 1, 1, 1, 1, 0, 0, 0, NULL, NULL)
select c1 OR c1, c1 OR c2, c1 OR c3, c2 OR c3, c3 OR c4, c4 OR c5, c5 OR c6, c6 OR c7, c4 OR c8, c6 OR c8 from my_varbinary_t1;

-- TODO[TD015]: should get (1, 1, 1, 1, 1, 0, 0, 0, NULL, NULL)
select c1 || c1, c1 || c2, c1 || c3, c2 || c3, c3 || c4, c4 || c5, c5 || c6, c6 || c7, c4 || c8, c6 || c8 from my_varbinary_t1;

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1 varbinary(3),
  c2 varbinary(3),
  c3 varbinary(3),
  c4 varbinary(3),
  c5 varbinary(3),
  c6 varbinary(10),
  c7 varbinary(10),
  c8 varbinary(10),
  c9 varbinary(10)
);

INSERT INTO my_varbinary_t1 VALUES ('123', '999', 'a', '2m', 'b2b', '123', '', ' 12', null);

--------------------------------------------------------------------------------

-- TODO[TD021]: should get (246, 1122, 123, 125, 123, 246, 123, 135, 12, NULL) and warnings
select c1+c1, c1+c2, c1+c3, c1+c4, c1+c5, c1+c6, c6+c7, c6+c8, c7+c8, c6+c9 from my_varbinary_t1;

DISCARD ALL;

/*
 * MySQL varbinary type operator greater than test.
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1  varbinary(5),
  c2  varbinary(5),
  c3  varbinary(5),
  c4  varbinary(5),
  c5  varbinary(5),
  c6  varbinary(10),
  c7  varbinary(10),
  c8  varbinary(10),
  c9  varbinary(10),
  c10 varbinary(10),
  c11 varbinary(10)
);

INSERT INTO my_varbinary_t1 values('123', '123', '1234', '', null, '1234', 'abcd', 'ABCD', '', null, NULL);

--------------------------------------------------------------------------------

-- TODO[TD015]: should get (1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1)
select
  c1 <=> c2,
  c2 <=> c3,
  c3 <=> c6,
  c6 <=> c7,
  c7 <=> c8,
  c1 <=> c4,
  c1 <=> c5,
  c4 <=> c6,
  c4 <=> c9,
  c4 <=> c10,
  c5 <=> c10,
  c10 <=> c11
from
  my_varbinary_t1;

DISCARD ALL;

/*
 * test mysql varbinary type operator between and
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1 varbinary(5),
  c2 varbinary(10),
  c3 varbinary(5),
  c4 varbinary(5),
  c5 varbinary(5),
  c6 varbinary(5)
);

INSERT INTO my_varbinary_t1 VALUES ('123', '123', 'abc', '', ' ', null);

--------------------------------------------------------------------------------

-- TODO[TD011]: should get (0, -1, -1, 1, -1, NULL), but not as expected
select
  strcmp(c1, c1),
  strcmp(c1, c2),
  strcmp(c1, c3),
  strcmp(c3, c4),
  strcmp(c4, c5),
  strcmp(c4, c6)
from
  my_varbinary_t1;

DISCARD ALL;

/*
 * test mysql varbinary type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_varbinary_t1;

CREATE TABLE my_varbinary_t1(
  c1 varbinary(5),
  c2 varbinary(10),
  c3 varbinary(10),
  c4 varbinary(10),
  c5 varbinary(10),
  c6 varbinary(10),
  c7 varbinary(10),
  c8 varbinary(10)
);

INSERT INTO my_varbinary_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- should get (0, 0, 0, 0, 1, 0, 0, 0, NULL, NULL) and warnings
select c1 XOR c1, c1 XOR c2, c1 XOR c3, c2 XOR c3, c3 XOR c4, c4 XOR c5, c5 XOR c6, c6 XOR c7, c4 XOR c8, c6 XOR c8 from my_varbinary_t1;

/* Begin - bug#z5411 */
create table tb_varbinary(c1 varbinary(10));
insert into tb_varbinary values('1234'), ('abcd'), ('1q2w'), ('q1w2');
select cast(c1 as unsigned) from tb_varbinary;
select convert(c1, unsigned) from tb_varbinary;
drop table tb_varbinary;
/* End - bug#z5411 */

/* Begin - bug#z5465 */
RESET sql_mode;
create table tb_varbinary(c1 varbinary(5));
COPY public.tb_varbinary (c1) FROM stdin;
\\x313233
\.
COPY public.tb_varbinary (c1) FROM stdin;
\\x3132333435
\.
select * from tb_varbinary;
COPY public.tb_varbinary (c1) FROM stdin;
\\x313233343536
\.
select * from tb_varbinary;
SET sql_mode='';
COPY public.tb_varbinary (c1) FROM stdin;
\\x313233343536
\.
select * from tb_varbinary;
RESET sql_mode;
/* End - bug#z5465 */

/* Begin - bug#5184 */
drop table tb_varbinary;
create table tb_varbinary(c1 binary(10));
insert into tb_varbinary values('1234'), ('abcd'), ('1q2w'), ('q1w2');
select cast(c1 as date) from tb_varbinary;
select cast(c1 as time) from tb_varbinary;
select cast(c1 as datetime) from tb_varbinary;
select cast(c1 as decimal(4, 0)) from tb_varbinary;
select cast(c1 as signed) from tb_varbinary;
select cast(c1 as unsigned) from tb_varbinary;
drop table tb_varbinary;
/* End - bug#5184 */