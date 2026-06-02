DISCARD ALL;

/*
 * test mysql mediumblob type.
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS
  my_mediumblob_t26,
  my_mediumblob_t25,
  my_mediumblob_t24,
  my_mediumblob_t23,
  my_mediumblob_t22,
  my_mediumblob_t21,
  my_mediumblob_t20,
  my_mediumblob_t19,
  my_mediumblob_t18,
  my_mediumblob_t17,
  my_mediumblob_t16,
  my_mediumblob_t15,
  my_mediumblob_t14,
  my_mediumblob_t13,
  my_mediumblob_t12,
  my_mediumblob_t11,
  my_mediumblob_t10,
  my_mediumblob_t09,
  my_mediumblob_t08,
  my_mediumblob_t07,
  my_mediumblob_t06,
  my_mediumblob_t05,
  my_mediumblob_t04,
  my_mediumblob_t03,
  my_mediumblob_t02,
  my_mediumblob_t01;

--------------------------------------------------------------------------------

-- should error
create table my_mediumblob_t01 (c1 mediumblob(10));

-- should ok
create table my_mediumblob_t01 (c1 mediumblob);

-- should ok
insert  into my_mediumblob_t01 values('123'), ('1234'), ('abcde');

-- should get {(123,3), (1234,4), (abcde,5)}
select c1, length(c1) from my_mediumblob_t01;

-- should ok
create table my_mediumblob_t02 (c1 mediumblob);

-- should ok
insert into my_mediumblob_t02 values (repeat('a', 16777215));

-- should get {16777215}
select length(c1) from my_mediumblob_t02;

-- should error for too long
insert into my_mediumblob_t02 values (repeat('a', 16777216));

SET sql_mode = '';

-- should ok and warning
insert into my_mediumblob_t02 values (repeat('a', 16777216));

-- should get {16777215, 16777215}
select length(c1) from my_mediumblob_t02;

RESET sql_mode;

-- should error
create table my_mediumblob_t03 (c1 int, c2 mediumblob default 'test');

-- TODO[TD003]: should error
select mediumblob'101010' as result;

DISCARD ALL;

/*
 * test mysql mediumblob type index.
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set enable_seqscan = off;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t01;

CREATE TABLE my_mediumblob_t01(
  c1 mediumblob,
  c2 mediumblob,
  c3 mediumblob,
  c4 mediumblob
);

INSERT INTO my_mediumblob_t01 VALUES ('abcdefghij', 'bcdefghijk', 'cdefghijkl', 'defghijklm');
INSERT INTO my_mediumblob_t01 VALUES ('abcdefghij', 'bcdefghijk', 'cdefghijkl', 'defghijklm');
INSERT INTO my_mediumblob_t01 VALUES ('abcdefghij', 'bcdefghijk', 'cdefghijkl', 'defghijklm');

--------------------------------------------------------------------------------

-- case: gin
create index my_mediumblob_t01_gin on my_mediumblob_t01 using gin(c1);
select c1 from my_mediumblob_t01 where c1 = 'abcdefghij';
explain select c1 from my_mediumblob_t01 where c1 = 'abcdefghij';

-- case: brin
create index my_mediumblob_t01_brin on my_mediumblob_t01 using brin(c2);
select c2 from my_mediumblob_t01 where c2 = 'bcdefghijk';
explain select c2 from my_mediumblob_t01 where c2 = 'bcdefghijk';

-- case: btree
create index my_mediumblob_t01_btree on my_mediumblob_t01 using btree(c3);
select c3 from my_mediumblob_t01 where c3 = 'cdefghijkl';
explain select c3 from my_mediumblob_t01 where c3 = 'cdefghijkl';

-- case: hash
create index my_mediumblob_t01_hash on my_mediumblob_t01 using hash(c4);
select c4 from my_mediumblob_t01 where c4 = 'defghijklm';
explain select c4 from my_mediumblob_t01 where c4 = 'defghijklm';

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1 mediumblob,
  c2 mediumblob,
  c3 mediumblob,
  c4 mediumblob,
  c5 mediumblob,
  c6 mediumblob,
  c7 mediumblob,
  c8 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- should get (1, 1, 1, 1, 0, 0, 0, 0, 0, 0)
select c1 AND c1, c1 AND c2, c1 AND c3, c2 AND c3, c3 AND c4, c4 AND c5, c5 AND c6, c6 AND c7, c4 AND c8, c6 AND c8 from my_mediumblob_t1;

-- should get (1, 1, 1, 1, 0, 0, 0, 0, 0, 0)
select c1 && c1, c1 && c2, c1 && c3, c2 && c3, c3 && c4, c4 && c5, c5 && c6, c6 && c7, c4 && c8, c6 && c8 from my_mediumblob_t1;

DISCARD ALL;

/*
 * test mysql mediumblob type operator between and
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1  mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES ('123'), ('12345'), ('abc');

--------------------------------------------------------------------------------

-- should get {123}
SELECT * from my_mediumblob_t1 WHERE c1 BETWEEN '12' AND '1234';

-- should get {123, 12345}
SELECT * from my_mediumblob_t1 WHERE c1 BETWEEN '12' AND '12346';

-- should get empty
SELECT * from my_mediumblob_t1 WHERE c1 BETWEEN '124' AND '999';

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1 mediumblob,
  c2 mediumblob,
  c3 mediumblob,
  c4 mediumblob,
  c5 mediumblob,
  c6 mediumblob,
  c7 mediumblob,
  c8 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- should get (123, 123, 82, 82, 0, 0, 0, 0, NULL, NULL)
select c1&c1, c1&c2, c1&c3, c2&c3, c3&c4, c4&c5, c5&c6, c6&c7, c4&c8, c6&c8 from my_mediumblob_t1;

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1 mediumblob,
  c2 mediumblob,
  c3 mediumblob,
  c4 mediumblob,
  c5 mediumblob,
  c6 mediumblob,
  c7 mediumblob,
  c8 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

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
  my_mediumblob_t1;

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1 mediumblob,
  c2 mediumblob,
  c3 mediumblob,
  c4 mediumblob,
  c5 mediumblob,
  c6 mediumblob,
  c7 mediumblob,
  c8 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- should get "(18446744073709551492, 18446744073709551492, 18446744073709550381, 18446744073709551615, 18446744073709551615, 18446744073709551615, 18446744073709551615, NULL)"
select ~c1, ~c2, ~c3, ~c4, ~c5, ~c6, ~c7, ~c8 from my_mediumblob_t1;

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1 mediumblob,
  c2 mediumblob,
  c3 mediumblob,
  c4 mediumblob,
  c5 mediumblob,
  c6 mediumblob,
  c7 mediumblob,
  c8 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

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
from my_mediumblob_t1;
DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1 mediumblob,
  c2 mediumblob,
  c3 mediumblob,
  c4 mediumblob,
  c5 mediumblob,
  c6 mediumblob,
  c7 mediumblob,
  c8 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- should get "(246, 246, 2468, 0, 0, 0, 0, NULL)"
select c1<<1, c2<<1, c3<<1, c4<<2, c5<<2, c6<<1, c7<<2, c8<<3 from my_mediumblob_t1;

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1 mediumblob,
  c2 mediumblob,
  c3 mediumblob,
  c4 mediumblob,
  c5 mediumblob,
  c6 mediumblob,
  c7 mediumblob,
  c8 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- should get (61, 61, 617, 0, 0, 0, 0, NULL)
select c1>>1, c2>>1, c3>>1, c4>>2, c5>>2, c6>>1, c7>>2, c8>>3 from my_mediumblob_t1;

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1 mediumblob,
  c2 mediumblob,
  c3 mediumblob,
  c4 mediumblob,
  c5 mediumblob,
  c6 mediumblob,
  c7 mediumblob,
  c8 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- should get "(0, 0, 1193, 1193, 1234, 0, 0, 0, NULL, NULL)"
select c1^c1, c1^c2, c1^c3, c2^c3, c3^c4, c4^c5, c5^c6, c6^c7, c4^c8, c6^c8 from my_mediumblob_t1;

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1 mediumblob,
  c2 mediumblob,
  c3 mediumblob,
  c4 mediumblob,
  c5 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES ('123', 'abc', '', ' ', null);

--------------------------------------------------------------------------------

-- should get {123, abc,  ,  , NULL}
SELECT
  COALESCE(c1, c5),
  COALESCE(c5, c2),
  COALESCE(c5, c3),
  COALESCE(c5, c4),
  COALESCE(c5, c5)
FROM
  my_mediumblob_t1;

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;
DROP TABLE IF EXISTS my_mediumblob_t2;

CREATE TABLE my_mediumblob_t1(
  c1 mediumblob,
  c2 mediumblob,
  c3 mediumblob,
  c4 mediumblob,
  c5 mediumblob,
  c6 mediumblob,
  c7 mediumblob,
  c8 mediumblob,
  c9 mediumblob
);

CREATE TABLE my_mediumblob_t2(
  c1 mediumblob,
  c2 mediumblob,
  c3 mediumblob,
  c4 mediumblob,
  c5 mediumblob,
  c6 mediumblob,
  c7 mediumblob,
  c8 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES ('2', '3', 'a', '2m', 'b2b', '4', '', ' 2', null);
INSERT INTO my_mediumblob_t2 VALUES ('4', '3', '2', '2.5', '4', '', ' ', null);

--------------------------------------------------------------------------------

-- should get (1, 1, 2, 1, 1, NULL, NULL, NULL, NULL)
select c1 div c1, c1 div c2, c1 div c3, c1 div c4, c1 div c5, c5 div c6, c5 div c7, c5 div c8, c7 div c8 from my_mediumblob_t2;

-- should get (1, 1.5, 0, 1, 0, 2, NULL, 2, 0, NULL) and warnings
select c1/c1, c2/c1, c3/c1, c4/c1, c5/c1, c6/c1, c6/c7, c6/c8, c7/c8, c6/c9 from my_mediumblob_t1;

DISCARD ALL;

/*
 * MySQL mediumblob type operator greater than test.
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1  mediumblob,
  c2  mediumblob,
  c3  mediumblob,
  c4  mediumblob,
  c5  mediumblob,
  c6  mediumblob,
  c7  mediumblob,
  c8  mediumblob,
  c9  mediumblob,
  c10 mediumblob,
  c11 mediumblob
);

INSERT INTO my_mediumblob_t1 values('123', '123', '1234', '', null, '1234', 'abcd', 'ABCD', '', null, NULL);

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
  my_mediumblob_t1;

DISCARD ALL;

/*
 * MySQL mediumblob type operator greater equal test.
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1  mediumblob,
  c2  mediumblob,
  c3  mediumblob,
  c4  mediumblob,
  c5  mediumblob,
  c6  mediumblob,
  c7  mediumblob,
  c8  mediumblob,
  c9  mediumblob,
  c10 mediumblob,
  c11 mediumblob
);

INSERT INTO my_mediumblob_t1 values('123', '123', '1234', '', null, '1234', 'abcd', 'ABCD', '', null, NULL);

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
  my_mediumblob_t1;

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1 mediumblob,
  c2 mediumblob,
  c3 mediumblob,
  c4 mediumblob,
  c5 mediumblob,
  c6 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES ('123', '123', 'abc', '', ' ', null);

--------------------------------------------------------------------------------

-- should get {123, 3, abc, abc, , NULL}
SELECT
  GREATEST(c1, c2),
  length(GREATEST(c1, c2)),
  GREATEST(c1, c3),
  GREATEST(c3, c4),
  GREATEST(c4, c5),
  GREATEST(c4, c6)
FROM
  my_mediumblob_t1;

DISCARD ALL;

/*
 * MySQL mediumblob type operator greater than test.
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t01;

CREATE TABLE my_mediumblob_t01(
  c1  mediumblob,
  c2  mediumblob,
  c3  mediumblob,
  c4  mediumblob,
  c5  mediumblob,
  c6  mediumblob,
  c7  mediumblob,
  c8  mediumblob,
  c9  mediumblob,
  c10 mediumblob,
  c11 mediumblob
);

INSERT INTO my_mediumblob_t01 values ('123', '123', '1234', '', null, '1234', 'abcd', 'ABCD', '', null, NULL);

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
  my_mediumblob_t01;

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES ('abc'), ('abc123'), ('Abc123'), ('瀚高'), (''), (' '), (null);

--------------------------------------------------------------------------------

-- TODO[TD007]: should get {abc, abc123, 瀚高}
SELECT * FROM my_mediumblob_t1 WHERE c1
  IN ('abc', 'abc123', '瀚高', '\0\0\0\0\0\0', ' \0\0\0\0\0', null);

-- TODO[TD007]: should get {abc123, Abc123, 瀚高, , }
SELECT * FROM my_mediumblob_t1 WHERE c1
  IN ('abc\0\0\0', 'abc123', 'Abc123', '瀚高', '', ' ', null);

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1 mediumblob,
  c2 mediumblob,
  c3 mediumblob,
  c4 mediumblob,
  c5 mediumblob,
  c6 mediumblob,
  c7 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES ('123', '121', 'abcd', '125', '', ' ', null);

--------------------------------------------------------------------------------

-- TODO[TD009]: should get '2', but get error, need to impl this function
SELECT interval(c1, c2, c3, c4, c5, c6, c7) FROM my_mediumblob_t1;

-- TODO[TD009]: should get '3', but get error, need to impl this function
SELECT interval(c1, c7, c6, c5, c4, c3, c2) FROM my_mediumblob_t1;

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  id int,
  c1 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES (1, '123'), (2, 'abc'), (3, ''), (4, ' '), (5, null);

--------------------------------------------------------------------------------

-- should get {(1, 123), (2, abc), (3, ), (4, )}
select * from my_mediumblob_t1 where c1 is not null;

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  id int,
  c1 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES (1, '123'), (2, 'abc'), (3, ''), (4, ' '), (5, null);

--------------------------------------------------------------------------------

-- should get {(2, abc), (3, ), (4, ), (5, NULL)}
select * from my_mediumblob_t1 where c1 is not true;

-- should get {(1, 123), (5, NULL)}
select * from my_mediumblob_t1 where c1 is not false;

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  id int,
  c1 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES (1, '123'), (2, 'abc'), (3, ''), (4, ' '), (5, null);

--------------------------------------------------------------------------------

-- should get (5, null)
select * from my_mediumblob_t1 where c1 is null;

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1 mediumblob,
  c2 mediumblob,
  c3 mediumblob,
  c4 mediumblob,
  c5 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES ('123', 'abc', '', ' ', null);

--------------------------------------------------------------------------------

-- TODO[TD010]: should get (0, 0, 0, 0, 1), but not as expected
select isnull(c1), isnull(c2), ISNULL(c3), ISNULL(c4), ISNULL(c5) from my_mediumblob_t1;

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  id int,
  c1 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES (1, '123'), (2, 'abc'), (3, ''), (4, ' '), (5, null);

--------------------------------------------------------------------------------

-- should get (1, 123)
select * from my_mediumblob_t1 where c1 is true;

-- should get {(2, abc), (3, ), (4, )}
select * from my_mediumblob_t1 where c1 is false;

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1 mediumblob,
  c2 mediumblob,
  c3 mediumblob,
  c4 mediumblob,
  c5 mediumblob,
  c6 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES ('123', '123', 'abc', '', ' ', null);

--------------------------------------------------------------------------------

-- should get (123, 3, 123, , , NULL)
select least(c1, c2), length(least(c1, c2)), least(c1, c3), least(c3, c4), least(c4, c5), least(c4, c6) from my_mediumblob_t1;

DISCARD ALL;

/*
 * MySQL mediumblob type operator greater than test.
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1  mediumblob,
  c2  mediumblob,
  c3  mediumblob,
  c4  mediumblob,
  c5  mediumblob,
  c6  mediumblob,
  c7  mediumblob,
  c8  mediumblob,
  c9  mediumblob,
  c10 mediumblob,
  c11 mediumblob
);

INSERT INTO my_mediumblob_t1 values('123', '123', '1234', '', null, '1234', 'abcd', 'ABCD', '', null, NULL);

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
  my_mediumblob_t1;

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES (' bc'), ('abc'), ('Abc'), ('abcd'), ('瀚高');

--------------------------------------------------------------------------------

-- should get {abc, abcd}
select * from my_mediumblob_t1 where c1 like 'a%';

-- should get (Abc)
select * from my_mediumblob_t1 where c1 like 'A%';

-- should get { bc, abc, Abc}
select * from my_mediumblob_t1 where c1 like '%c';

-- should get { bc, abc, Abc, abcd}
select * from my_mediumblob_t1 where c1 like '%b%';

-- should get { bc, abc, Abc, abcd, 瀚高}
-- TODO[TD007]: can not display chinese
select * from my_mediumblob_t1 where c1 like '%';

-- should get (瀚高)
-- TODO[TD007]: can not display chinese
select * from my_mediumblob_t1 where c1 like '%瀚%';

DISCARD ALL;

/*
 * MySQL mediumblob type operator greater than test.
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1  mediumblob,
  c2  mediumblob,
  c3  mediumblob,
  c4  mediumblob,
  c5  mediumblob,
  c6  mediumblob,
  c7  mediumblob,
  c8  mediumblob,
  c9  mediumblob,
  c10 mediumblob,
  c11 mediumblob
);

INSERT INTO my_mediumblob_t1 values('123', '123', '1234', '', null, '1234', 'abcd', 'ABCD', '', null, NULL);

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
  my_mediumblob_t1;

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;
DROP TABLE IF EXISTS my_mediumblob_t2;

CREATE TABLE my_mediumblob_t1(
  c1 mediumblob,
  c2 mediumblob,
  c3 mediumblob,
  c4 mediumblob,
  c5 mediumblob,
  c6 mediumblob,
  c7 mediumblob,
  c8 mediumblob,
  c9 mediumblob
);

CREATE TABLE my_mediumblob_t2(
  c1 mediumblob,
  c2 mediumblob,
  c3 mediumblob,
  c4 mediumblob,
  c5 mediumblob,
  c6 mediumblob,
  c7 mediumblob,
  c8 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES ('123', '999', 'a', '2m', 'b2b', '123', '', ' 12', null);
INSERT INTO my_mediumblob_t2 VALUES ('123', '-123', 'a', '2m', '', ' ', ' 12', null);

--------------------------------------------------------------------------------

-- TODO[TD021]: should get (0, -876, 123, 121, 123, 0, 123, 111, -12, NULL) and warnings
select c1-c1, c1-c2, c1-c3, c1-c4, c1-c5, c1-c6, c6-c7, c6-c8, c7-c8, c6-c9 from my_mediumblob_t1;

-- TODO[TD021]: should get (-123, 123, -0, -2, -0, 0, -12, NULL) and warnings
select -c1, -c2, -c3, -c4, -c5, -(-c6), -c7, -c8 from my_mediumblob_t2;

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1 mediumblob,
  c2 mediumblob,
  c3 mediumblob,
  c4 mediumblob,
  c5 mediumblob,
  c6 mediumblob,
  c7 mediumblob,
  c8 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES ('4', '3', '2', '2.5', '4', '', ' ', null);

--------------------------------------------------------------------------------

-- TODO[TD021]: should get (0, 1, 0, 1.5, 0, NULL, NULL, NULL, NULL) and warnings
select c1%c1, c1%c2, c1%c3, c1%c4, c1%c5, c5%c6, c5%c7, c5%c8, c7%c8 from my_mediumblob_t1;

-- TODO[TD021]: should get (0, 1, 0, 1.5, 0, NULL, NULL, NULL, NULL) and warnings
select c1 mod c1, c1 mod c2, c1 mod c3, c1 mod c4, c1 mod c5, c5 mod c6, c5 mod c7, c5 mod c8, c7 mod c8 from my_mediumblob_t1;

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1 mediumblob,
  c2 mediumblob,
  c3 mediumblob,
  c4 mediumblob,
  c5 mediumblob,
  c6 mediumblob,
  c7 mediumblob,
  c8 mediumblob,
  c9 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES ('2', '3', 'a', '2m', 'b2b', '4', '', ' 2', null);

--------------------------------------------------------------------------------

-- TODO[TD021]: should get (4, 6, 0, 4, 0, 8, 0, 8, 0, NULL) and warnings
select c1*c1, c1*c2, c1*c3, c1*c4, c1*c5, c1*c6, c6*c7, c6*c8, c7*c8, c6*c9 from my_mediumblob_t1;

DISCARD ALL;

/*
 * MySQL mediumblob type operator greater equal test.
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1  mediumblob,
  c2  mediumblob,
  c3  mediumblob,
  c4  mediumblob,
  c5  mediumblob,
  c6  mediumblob,
  c7  mediumblob,
  c8  mediumblob,
  c9  mediumblob,
  c10 mediumblob,
  c11 mediumblob
);

INSERT INTO my_mediumblob_t1 values('123', '123', '1234', '', null, '1234', 'abcd', 'ABCD', '', null, NULL);

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
  my_mediumblob_t1;

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
  my_mediumblob_t1;

DISCARD ALL;

/*
 * test mysql mediumblob type operator between and
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1  mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES ('123'), ('12345'), ('abc');

--------------------------------------------------------------------------------

-- should get {12345, abc}
SELECT * from my_mediumblob_t1 WHERE c1 NOT BETWEEN '12' AND '1234';

-- should get {abc}
SELECT * from my_mediumblob_t1 WHERE c1 NOT BETWEEN '12' AND '12346';

-- should get {123, 12345, abc}
SELECT * from my_mediumblob_t1 WHERE c1 NOT BETWEEN '124' AND '999';

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES ('abc'), ('abc123'), ('Abc123'), ('瀚高'), (''), (' '), (null);

--------------------------------------------------------------------------------

-- should get empty
SELECT * FROM my_mediumblob_t1 WHERE c1
  NOT IN ('abc', 'abc123', '瀚高', '\0\0\0\0\0\0', ' \0\0\0\0\0', null);

-- should get empty
SELECT * FROM my_mediumblob_t1 WHERE c1
  NOT IN ('abc\0\0\0', 'abc123', 'Abc123', '瀚高', '', ' ', null);

-- should get {Abc123, , }
select * from my_mediumblob_t1 where c1
  not in ('abc', 'abc123', '瀚高');

-- TODO[TD007]: should get {abc, abc123, 瀚高, , }
select * from my_mediumblob_t1 where c1
  not in ('abc\0\0\0', 'Abc123', '\0\0\0\0\0\0', ' \0\0\0\0\0');

-- should get empty
select * from my_mediumblob_t1 where c1
  not in ('abc', null);

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES (' bc'), ('abc'), ('Abc'), ('abcd'), ('瀚高');

--------------------------------------------------------------------------------

-- should get { bc, Abc, 瀚高}
-- TODO[TD007]: can not display chinese
select * from my_mediumblob_t1 where c1 not like 'a%';

-- shoud get { bc, abc, abcd, 瀚高}
-- TODO[TD007]: can not display chinese
select * from my_mediumblob_t1 where c1 not like 'A%';

-- shoud get {abcd, 瀚高}
-- TODO[TD007]: can not display chinese
select * from my_mediumblob_t1 where c1 not like '%c';

-- soud get {瀚高}
-- TODO[TD007]: can not display chinese
select * from my_mediumblob_t1 where c1 not like '%b%';

-- shoud get empty
select * from my_mediumblob_t1 where c1 not like '%';

-- shoud get { bc, abc, Abc, abcd}
select * from my_mediumblob_t1 where c1 not like '%瀚%';

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1 mediumblob,
  c2 mediumblob,
  c3 mediumblob,
  c4 mediumblob,
  c5 mediumblob,
  c6 mediumblob,
  c7 mediumblob,
  c8 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- TODO[TD021]: should get (0, 0, 0, 1, 1, 1, 1, NULL) and warnings
select not c1, not c2, not c3, not c4, not c5, not c6, not c7, not c8 from my_mediumblob_t1;

-- TODO[TD021]: should get (0, 0, 0, 1, 1, 1, 1, NULL) and warnings
select !c1, !c2, !c3, !c4, !c5, !c6, !c7, !c8 from my_mediumblob_t1;

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1 mediumblob,
  c2 mediumblob,
  c3 mediumblob,
  c4 mediumblob,
  c5 mediumblob,
  c6 mediumblob,
  c7 mediumblob,
  c8 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- should get (1, 1, 1, 1, 1, 0, 0, 0, NULL, NULL)
select c1 OR c1, c1 OR c2, c1 OR c3, c2 OR c3, c3 OR c4, c4 OR c5, c5 OR c6, c6 OR c7, c4 OR c8, c6 OR c8 from my_mediumblob_t1;

-- should get (1, 1, 1, 1, 1, 0, 0, 0, NULL, NULL)
select c1 || c1, c1 || c2, c1 || c3, c2 || c3, c3 || c4, c4 || c5, c5 || c6, c6 || c7, c4 || c8, c6 || c8 from my_mediumblob_t1;

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1 mediumblob,
  c2 mediumblob,
  c3 mediumblob,
  c4 mediumblob,
  c5 mediumblob,
  c6 mediumblob,
  c7 mediumblob,
  c8 mediumblob,
  c9 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES ('123', '999', 'a', '2m', 'b2b', '123', '', ' 12', null);

--------------------------------------------------------------------------------

-- TODO[TD021]: should get (246, 1122, 123, 125, 123, 246, 123, 135, 12, NULL) and warnings
select c1+c1, c1+c2, c1+c3, c1+c4, c1+c5, c1+c6, c6+c7, c6+c8, c7+c8, c6+c9 from my_mediumblob_t1;

DISCARD ALL;

/*
 * MySQL mediumblob type operator greater than test.
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1  mediumblob,
  c2  mediumblob,
  c3  mediumblob,
  c4  mediumblob,
  c5  mediumblob,
  c6  mediumblob,
  c7  mediumblob,
  c8  mediumblob,
  c9  mediumblob,
  c10 mediumblob,
  c11 mediumblob
);

INSERT INTO my_mediumblob_t1 values('123', '123', '1234', '', null, '1234', 'abcd', 'ABCD', '', null, NULL);

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
  my_mediumblob_t1;

DISCARD ALL;

/*
 * test mysql mediumblob type operator between and
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1 mediumblob,
  c2 mediumblob,
  c3 mediumblob,
  c4 mediumblob,
  c5 mediumblob,
  c6 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES ('123', '123', 'abc', '', ' ', null);

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
  my_mediumblob_t1;

DISCARD ALL;

/*
 * test mysql mediumblob type operator
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_mediumblob_t1;

CREATE TABLE my_mediumblob_t1(
  c1 mediumblob,
  c2 mediumblob,
  c3 mediumblob,
  c4 mediumblob,
  c5 mediumblob,
  c6 mediumblob,
  c7 mediumblob,
  c8 mediumblob
);

INSERT INTO my_mediumblob_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- should get (0, 0, 0, 0, 1, 0, 0, 0, NULL, NULL) and warnings
select c1 XOR c1, c1 XOR c2, c1 XOR c3, c2 XOR c3, c3 XOR c4, c4 XOR c5, c5 XOR c6, c6 XOR c7, c4 XOR c8, c6 XOR c8 from my_mediumblob_t1;

/* Begin - Bug#Z5202 */
create table tb_mediumblob(c1 mediumblob);
insert into tb_mediumblob values('1234'), ('abcd'), ('1q2w'), ('q1w2');
select cast(c1 as float) from tb_mediumblob;
drop table tb_mediumblob;
/* End - Bug#Z5202 */
