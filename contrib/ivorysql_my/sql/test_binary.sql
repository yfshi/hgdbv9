DISCARD ALL;

/*
 * MySQL binary type test.
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS
  my_binary_t1,
  my_binary_t2,
  my_binary_t3,
  my_binary_t4,
  my_binary_t5,
  my_binary_t6,
  my_binary_t7,
  my_binary_t8
;

CREATE TABLE my_binary_t1(c1 binary(-1)); -- should error

CREATE TABLE my_binary_t2(c1 binary(0)); -- should ok
\d my_binary_t2;

INSERT INTO my_binary_t2 VALUES('a'); -- should error

CREATE TABLE my_binary_t3(c1 binary); -- should ok
\d my_binary_t3;

CREATE TABLE my_binary_t4(c1 binary(255)); -- should ok
\d my_binary_t4;

CREATE TABLE my_binary_t5(c1 binary(256)); -- should error

CREATE TABLE my_binary_t6(c1 binary(3)); -- should ok
\d my_binary_t6;
INSERT INTO my_binary_t6 VALUES('a');

-- should get (a, 3, f)
SELECT *, length(c1), c1 = 'a' FROM my_binary_t6;

-- should get (t, f)
SELECT c1 = 'a\0\0', c1 = 'b\0\0' FROM my_binary_t6;

CREATE TABLE my_binary_t7(c1 binary(3)); -- should ok
\d my_binary_t7;

INSERT INTO my_binary_t7 VALUES('abcd'); -- should error
SELECT * FROM my_binary_t7;

SET sql_mode = '';
INSERT INTO my_binary_t7 VALUES('abcd'); -- should warning
SELECT * FROM my_binary_t7;

RESET sql_mode;
INSERT INTO my_binary_t7 VALUES('abcd'); -- should error
SELECT * FROM my_binary_t7;

CREATE TABLE my_binary_t8(c1 binary('5'));  -- should error
CREATE TABLE my_binary_t8(c1 binary(a));    -- should error
CREATE TABLE my_binary_t8(c1 binary('a'));  -- should error
CREATE TABLE my_binary_t8(c1 binary(''));   -- should error
CREATE TABLE my_binary_t8(c1 binary(null)); -- should error

CREATE TABLE my_binary_t9(c1 binary(4), c2 binary(5) default 'test'); -- should ok
\d my_binary_t9;
INSERT INTO my_binary_t9 VALUES('ab'); -- should ok
SELECT * FROM my_binary_t9;

SELECT binary'101010' as result; -- should ok

SELECT binary('101010') as result; -- should ok

set sql_mode='';
set mysql.enable_character_blob to off;
CREATE TABLE my_binary_t8 (c1 binary(8));
INSERT  INTO my_binary_t8 values ('HGDB');
INSERT  INTO my_binary_t8 values ('Database');
INSERT  INTO my_binary_t8 values ('Database1');
INSERT  INTO my_binary_t8 values ('Database ');
INSERT  INTO my_binary_t8 values ('数据库');
SELECT * FROM my_binary_t8;

DISCARD ALL;

/*
 * test mysql binary type index.
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set enable_seqscan = off;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1 binary(10),
  c2 binary(10),
  c3 binary(10),
  c4 binary(10)
);

INSERT INTO my_binary_t1 VALUES ('abcdefghij', 'bcdefghijk', 'cdefghijkl', 'defghijklm');
INSERT INTO my_binary_t1 VALUES ('abcdefghij', 'bcdefghijk', 'cdefghijkl', 'defghijklm');
INSERT INTO my_binary_t1 VALUES ('abcdefghij', 'bcdefghijk', 'cdefghijkl', 'defghijklm');

-- case: gin
create index my_binary_t1_gin on my_binary_t1 using gin(c1);
select c1 from my_binary_t1 where c1 = 'abcdefghij';
select c1 from my_binary_t1 where c1 = cast('abcdefghij' as binary);
select c1 from my_binary_t1 where c1 = cast('abcdefghij' as binary(10));
explain select c1 from my_binary_t1 where c1 = cast('abcdefghij' as binary(10));

-- case: brin
create index my_binary_t1_brin on my_binary_t1 using brin(c2);
select c2 from my_binary_t1 where c2 = 'bcdefghijk';
select c2 from my_binary_t1 where c2 = cast('bcdefghijk' as binary);
select c2 from my_binary_t1 where c2 = cast('bcdefghijk' as binary(10));
explain select c2 from my_binary_t1 where c2 = cast('bcdefghijk' as binary(10));

-- case: btree
create index my_binary_t1_btree on my_binary_t1 using btree(c3);
select c3 from my_binary_t1 where c3 = 'cdefghijkl';
select c3 from my_binary_t1 where c3 = cast('cdefghijkl' as binary);
select c3 from my_binary_t1 where c3 = cast('cdefghijkl' as binary(10));
explain select c3 from my_binary_t1 where c3 = cast('cdefghijkl' as binary(10));

-- case: hash
create index my_binary_t1_hash on my_binary_t1 using hash(c4);
select c4 from my_binary_t1 where c4 = 'defghijklm';
select c4 from my_binary_t1 where c4 = cast('defghijklm' as binary);
select c4 from my_binary_t1 where c4 = cast('defghijklm' as binary(10));
explain select c4 from my_binary_t1 where c4 = cast('defghijklm' as binary(10));

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1 binary(5),
  c2 binary(10),
  c3 binary(10),
  c4 binary(10),
  c5 binary(10),
  c6 binary(10),
  c7 binary(10),
  c8 binary(10)
);

INSERT INTO my_binary_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- should get "(1, 1, 1, 1, 0, 0, 0, 0, 0, 0)"
select c1 AND c1, c1 AND c2, c1 AND c3, c2 AND c3, c3 AND c4, c4 AND c5, c5 AND c6, c6 AND c7, c4 AND c8, c6 AND c8 from my_binary_t1;

-- should get (1, 1, 1, 1, 0, 0, 0, 0, 0, 0)
select c1 && c1, c1 && c2, c1 && c3, c2 && c3, c3 && c4, c4 && c5, c5 && c6, c6 && c7, c4 && c8, c6 && c8 from my_binary_t1;

DISCARD ALL;

/*
 * test mysql binary type operator between and
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1  binary(5)
);

INSERT INTO my_binary_t1 VALUES ('123'), ('12345'), ('abc');

--------------------------------------------------------------------------------

-- should get (123)
SELECT * from my_binary_t1 WHERE c1 BETWEEN '12' AND '1234';

-- should get {(123), (12345)}
SELECT * from my_binary_t1 WHERE c1 BETWEEN '12' AND '12346';

-- should get empty
SELECT * from my_binary_t1 WHERE c1 BETWEEN '124' AND '999';

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1 binary(5),
  c2 binary(10),
  c3 binary(10),
  c4 binary(10),
  c5 binary(10),
  c6 binary(10),
  c7 binary(10),
  c8 binary(10)
);

INSERT INTO my_binary_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- should get "(123, 123, 82, 82, 0, 0, 0, 0, NULL, NULL)"
select c1&c1, c1&c2, c1&c3, c2&c3, c3&c4, c4&c5, c5&c6, c6&c7, c4&c8, c6&c8 from my_binary_t1;

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1 binary(5),
  c2 binary(10),
  c3 binary(10),
  c4 binary(10),
  c5 binary(10),
  c6 binary(10),
  c7 binary(10),
  c8 binary(10)
);

INSERT INTO my_binary_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

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
  my_binary_t1;

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1 binary(5),
  c2 binary(10),
  c3 binary(10),
  c4 binary(10),
  c5 binary(10),
  c6 binary(10),
  c7 binary(10),
  c8 binary(10)
);

INSERT INTO my_binary_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- should get "(18446744073709551492, 18446744073709551492, 18446744073709550381, 18446744073709551615, 18446744073709551615, 18446744073709551615, 18446744073709551615, NULL)"
select ~c1, ~c2, ~c3, ~c4, ~c5, ~c6, ~c7, ~c8 from my_binary_t1;

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1 binary(5),
  c2 binary(10),
  c3 binary(10),
  c4 binary(10),
  c5 binary(10),
  c6 binary(10),
  c7 binary(10),
  c8 binary(10)
);

INSERT INTO my_binary_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- should get "(123, 123, 1275, 1275, 1234, 0, 0, 0, NULL, NULL)"
select c1|c1, c1|c2, c1|c3, c2|c3, c3|c4, c4|c5, c5|c6, c6|c7, c4|c8, c6|c8 from my_binary_t1;
DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1 binary(5),
  c2 binary(10),
  c3 binary(10),
  c4 binary(10),
  c5 binary(10),
  c6 binary(10),
  c7 binary(10),
  c8 binary(10)
);

INSERT INTO my_binary_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- should get "(246, 246, 2468, 0, 0, 0, 0, NULL)"
select c1<<1, c2<<1, c3<<1, c4<<2, c5<<2, c6<<1, c7<<2, c8<<3 from my_binary_t1;

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1 binary(5),
  c2 binary(10),
  c3 binary(10),
  c4 binary(10),
  c5 binary(10),
  c6 binary(10),
  c7 binary(10),
  c8 binary(10)
);

INSERT INTO my_binary_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- should get "(61, 61, 617, 0, 0, 0, 0, NULL)"
select c1>>1, c2>>1, c3>>1, c4>>2, c5>>2, c6>>1, c7>>2, c8>>3 from my_binary_t1;

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1 binary(5),
  c2 binary(10),
  c3 binary(10),
  c4 binary(10),
  c5 binary(10),
  c6 binary(10),
  c7 binary(10),
  c8 binary(10)
);

INSERT INTO my_binary_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- should get "(0, 0, 1193, 1193, 1234, 0, 0, 0, NULL, NULL)"
select c1^c1, c1^c2, c1^c3, c2^c3, c3^c4, c4^c5, c5^c6, c6^c7, c4^c8, c6^c8 from my_binary_t1;

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set enable_seqscan to off;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1 binary(5),
  c2 binary(5),
  c3 binary(5),
  c4 binary(5),
  c5 binary(5)
);

INSERT INTO my_binary_t1 VALUES ('123', 'abc', '', ' ', null);

--------------------------------------------------------------------------------

-- should get {(123)}
SELECT COALESCE(c1, c5) FROM my_binary_t1;

-- should get {(abc)}
SELECT COALESCE(c5, c2) FROM my_binary_t1;

-- should get {()}
SELECT COALESCE(c5, c3) FROM my_binary_t1;

-- should get {( )}
SELECT COALESCE(c5, c4) FROM my_binary_t1;

-- should get {(NULL)}
SELECT COALESCE(c5, c5) FROM my_binary_t1;

-- should get {(123), (abc), (), ( ), (NULL)}
SELECT
  COALESCE(c1, c5),
  COALESCE(c5, c2),
  COALESCE(c5, c3),
  COALESCE(c5, c4),
  COALESCE(c5, c5)
FROM
  my_binary_t1;

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;
DROP TABLE IF EXISTS my_binary_t2;

CREATE TABLE my_binary_t1(
  c1 binary(3),
  c2 binary(3),
  c3 binary(3),
  c4 binary(3),
  c5 binary(3),
  c6 binary(10),
  c7 binary(10),
  c8 binary(10),
  c9 binary(10)
);

CREATE TABLE my_binary_t2(
  c1 binary(3),
  c2 binary(3),
  c3 binary(3),
  c4 binary(3),
  c5 binary(10),
  c6 binary(10),
  c7 binary(10),
  c8 binary(10)
);

INSERT INTO my_binary_t1 VALUES ('2', '3', 'a', '2m', 'b2b', '4', '', ' 2', null);
INSERT INTO my_binary_t2 VALUES ('4', '3', '2', '2.5', '4', '', ' ', null);

--------------------------------------------------------------------------------

-- should get "(1, 1.5, 0, 1, 0, 2, NULL, 2, 0, NULL)"
select c1/c1, c2/c1, c3/c1, c4/c1, c5/c1, c6/c1, c6/c7, c6/c8, c7/c8, c6/c9 from my_binary_t1;

-- should get "(1, 1, 2, 1, 1, NULL, NULL, NULL, NULL)"
select c1 div c1, c1 div c2, c1 div c3, c1 div c4, c1 div c5, c5 div c6, c5 div c7, c5 div c8, c7 div c8 from my_binary_t2;

DISCARD ALL;

/*
 * MySQL binary type operator greater than test.
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set enable_seqscan = off;

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1  binary(5),
  c2  binary(5),
  c3  binary(5),
  c4  binary(5),
  c5  binary(5),
  c6  binary(10),
  c7  binary(10),
  c8  binary(10),
  c9  binary(10),
  c10 binary(10),
  c11 binary(10)
);
INSERT INTO my_binary_t1 values('123', '123', '1234', '', null, '1234', 'abcd', 'ABCD', '', null, NULL);

--------------------------------------------------------------------------------

-- TODO[TD015]
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
  my_binary_t1;

DISCARD ALL;

/*
 * MySQL binary type operator greater equal test.
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set enable_seqscan = off;

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1  binary(5),
  c2  binary(5),
  c3  binary(5),
  c4  binary(5),
  c5  binary(5),
  c6  binary(10),
  c7  binary(10),
  c8  binary(10),
  c9  binary(10),
  c10 binary(10),
  c11 binary(10)
);
INSERT INTO my_binary_t1 values('123', '123', '1234', '', null, '1234', 'abcd', 'ABCD', '', null, NULL);

--------------------------------------------------------------------------------

-- TODO[TD015]
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
  my_binary_t1;

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1 binary(5),
  c2 binary(5),
  c3 binary(5),
  c4 binary(5),
  c5 binary(5),
  c6 binary(5)
);

INSERT INTO my_binary_t1 VALUES ('123', '123', 'abc', '', ' ', null);

--------------------------------------------------------------------------------

-- should get {(123)}
SELECT GREATEST(c1, c2) FROM my_binary_t1;

-- should get {(5)}
SELECT length(GREATEST(c1, c2)) FROM my_binary_t1;

-- should get {(abc)}
SELECT GREATEST(c1, c3) FROM my_binary_t1;

-- should get {(abc)}
SELECT GREATEST(c3, c4) FROM my_binary_t1;

-- should get {()}
SELECT GREATEST(c4, c5) FROM my_binary_t1;

-- should get {(NULL)}
SELECT GREATEST(c4, c6) FROM my_binary_t1;

-- should get {(123), (5), (abc), (abc), (     ), (NULL)}
SELECT
  GREATEST(c1, c2),
  length(GREATEST(c1, c2)),
  GREATEST(c1, c3),
  GREATEST(c3, c4),
  GREATEST(c4, c5),
  GREATEST(c4, c6)
FROM
  my_binary_t1;

-- should get {(123), (123), (abc), (), (), (NULL)}
SELECT * FROM my_binary_t1;

-- should get {(5), (5), (5), (5), (5), (NULL)}
SELECT
  length(c1),
  length(c2),
  length(c3),
  length(c4),
  length(c5),
  length(c6)
FROM my_binary_t1;

DISCARD ALL;

/*
 * MySQL binary type operator greater than test.
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set enable_seqscan = off;

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1  binary(5),
  c2  binary(5),
  c3  binary(5),
  c4  binary(5),
  c5  binary(5),
  c6  binary(10),
  c7  binary(10),
  c8  binary(10),
  c9  binary(10),
  c10 binary(10),
  c11 binary(10)
);
INSERT INTO my_binary_t1 values('123', '123', '1234', '', null, '1234', 'abcd', 'ABCD', '', null, NULL);

--------------------------------------------------------------------------------

-- TODO[TD015]
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
  my_binary_t1;

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1 binary(6)
);

INSERT INTO my_binary_t1 VALUES ('abc'), ('abc123'), ('Abc123'), ('瀚高'), (''), (' '), (null);

--------------------------------------------------------------------------------

-- should get {(abc123), (瀚高), (), ( )}
-- TODO[TD007]: can not display chinese
SELECT * FROM my_binary_t1 WHERE c1
  IN ('abc', 'abc123', '瀚高', '\0\0\0\0\0\0', ' \0\0\0\0\0', null);

-- should get {(abc), (abc123), (Abc123), (瀚高)}
-- TODO[TD007]: can not display chinese
SELECT * FROM my_binary_t1 WHERE c1
  IN ('abc\0\0\0', 'abc123', 'Abc123', '瀚高', '', ' ', null);

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1 binary(5),
  c2 binary(5),
  c3 binary(5),
  c4 binary(5),
  c5 binary(5),
  c6 binary(5),
  c7 binary(5)
);

INSERT INTO my_binary_t1 VALUES ('123', '121', 'abcd', '125', '', ' ', null);

--------------------------------------------------------------------------------

-- TODO[TD009]: should get '2', but get error, need to impl this function
SELECT interval(c1, c2, c3, c4, c5, c6, c7) FROM my_binary_t1;

-- TODO[TD009]: should get '3', but get error, need to impl this function
SELECT interval(c1, c7, c6, c5, c4, c3, c2) FROM my_binary_t1;

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  id int,
  c1 binary(10)
);

INSERT INTO my_binary_t1 VALUES (1, '123'), (2, 'abc'), (3, ''), (4, ' '), (5, null);

--------------------------------------------------------------------------------

-- should get {(1, '123'), (2, 'abc'), (3, ''), (4, ' ')}
select * from my_binary_t1 where c1 is not null;

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  id int,
  c1 binary(10)
);

INSERT INTO my_binary_t1 VALUES (1, '123'), (2, 'abc'), (3, ''), (4, ' '), (5, null);

--------------------------------------------------------------------------------

-- should get {(2, abc), (3, ), (4, ), (5, NULL)}
select * from my_binary_t1 where c1 is not true;

-- should get {(1, 123), (5, NULL)}
select * from my_binary_t1 where c1 is not false;

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  id int,
  c1 binary(10)
);

INSERT INTO my_binary_t1 VALUES (1, '123'), (2, 'abc'), (3, ''), (4, ' '), (5, null);

--------------------------------------------------------------------------------

-- should get "(5, null)"
select * from my_binary_t1 where c1 is null;

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1 binary(5),
  c2 binary(5),
  c3 binary(5),
  c4 binary(5),
  c5 binary(5)
);

INSERT INTO my_binary_t1 VALUES ('123', 'abc', '', ' ', null);

--------------------------------------------------------------------------------

-- TODO[TD010]: should get "(0, 0, 0, 0, 1)", but get error
select isnull(c1), isnull(c2), ISNULL(c3), ISNULL(c4), ISNULL(c5) from my_binary_t1;

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  id int,
  c1 binary(10)
);

INSERT INTO my_binary_t1 VALUES (1, '123'), (2, 'abc'), (3, ''), (4, ' '), (5, null);

--------------------------------------------------------------------------------

-- should get {(1, 123)}
select * from my_binary_t1 where c1 is true;

-- should get {(2, abc), (3, ), (4, )}
select * from my_binary_t1 where c1 is false;

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1 binary(5),
  c2 binary(10),
  c3 binary(5),
  c4 binary(5),
  c5 binary(5),
  c6 binary(5)
);

INSERT INTO my_binary_t1 VALUES ('123', '123', 'abc', '', ' ', null);

--------------------------------------------------------------------------------

-- should get {('123', 5, '123', '', '', NULL)}
select
  least(c1, c2),
  length(least(c1, c2)), 
  least(c1, c3), 
  least(c3, c4), 
  least(c4, c5), 
  least(c4, c6)
from my_binary_t1;

DISCARD ALL;

/*
 * MySQL binary type operator greater than test.
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set enable_seqscan = off;

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1  binary(5),
  c2  binary(5),
  c3  binary(5),
  c4  binary(5),
  c5  binary(5),
  c6  binary(10),
  c7  binary(10),
  c8  binary(10),
  c9  binary(10),
  c10 binary(10),
  c11 binary(10)
);
INSERT INTO my_binary_t1 values('123', '123', '1234', '', null, '1234', 'abcd', 'ABCD', '', null, NULL);

--------------------------------------------------------------------------------

-- TODO[TD015]
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
  my_binary_t1;

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1 binary(10)
);

INSERT INTO my_binary_t1 VALUES (' bc'), ('abc'), ('Abc'), ('abcd'), ('瀚高');

--------------------------------------------------------------------------------

-- should get "('abc', 'abcd')"
select * from my_binary_t1 where c1 like 'a%';

-- should get "('Abc')"
select * from my_binary_t1 where c1 like 'A%';

-- should get empty
select * from my_binary_t1 where c1 like '%c';

-- should get "(' bc'), ('abc'), ('Abc'), ('abcd')"
select * from my_binary_t1 where c1 like '%b%';

-- should get "(' bc'), ('abc'), ('Abc'), ('abcd'), ('瀚高')"
-- TODO[TD007]: can not display chinese
select * from my_binary_t1 where c1 like '%';

-- should get "('瀚高')"
-- TODO[TD007]: can not display chinese
select * from my_binary_t1 where c1 like '%瀚%';

DISCARD ALL;

/*
 * MySQL binary type operator greater than test.
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set enable_seqscan = off;

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1  binary(5),
  c2  binary(5),
  c3  binary(5),
  c4  binary(5),
  c5  binary(5),
  c6  binary(10),
  c7  binary(10),
  c8  binary(10),
  c9  binary(10),
  c10 binary(10),
  c11 binary(10)
);
INSERT INTO my_binary_t1 values('123', '123', '1234', '', null, '1234', 'abcd', 'ABCD', '', null, NULL);

-- TODO[TD015]
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
  my_binary_t1;

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;
DROP TABLE IF EXISTS my_binary_t2;

CREATE TABLE my_binary_t1(
  c1 binary(3),
  c2 binary(3),
  c3 binary(3),
  c4 binary(3),
  c5 binary(3),
  c6 binary(10),
  c7 binary(10),
  c8 binary(10),
  c9 binary(10)
);

CREATE TABLE my_binary_t2(
  c1 binary(5),
  c2 binary(5),
  c3 binary(5),
  c4 binary(5),
  c5 binary(5),
  c6 binary(5),
  c7 binary(5),
  c8 binary(5)
);

INSERT INTO my_binary_t1 VALUES ('123', '999', 'a', '2m', 'b2b', '123', '', ' 12', null);
INSERT INTO my_binary_t2 VALUES ('123', '-123', 'a', '2m', '', ' ', ' 12', null);

--------------------------------------------------------------------------------

-- should get "(0, -876, 123, 121, 123, 0, 123, 111, -12, NULL)"
select c1-c1, c1-c2, c1-c3, c1-c4, c1-c5, c1-c6, c6-c7, c6-c8, c7-c8, c6-c9 from my_binary_t1;

-- TODO[TD021]: should get (-123, 123, -0, -2, -0, 0, -12, NULL) and warnings
select -c1, -c2, -c3, -c4, -c5, -(-c6), -c7, -c8 from my_binary_t2;

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1 binary(3),
  c2 binary(3),
  c3 binary(3),
  c4 binary(3),
  c5 binary(10),
  c6 binary(10),
  c7 binary(10),
  c8 binary(10)
);

INSERT INTO my_binary_t1 VALUES ('4', '3', '2', '2.5', '4', '', ' ', null);

--------------------------------------------------------------------------------

-- TODO[TD021]: should get (0, 1, 0, 1.5, 0, NULL, NULL, NULL, NULL) and warnings
select c1%c1, c1%c2, c1%c3, c1%c4, c1%c5, c5%c6, c5%c7, c5%c8, c7%c8 from my_binary_t1;

-- TODO[TD021]: should get (0, 1, 0, 1.5, 0, NULL, NULL, NULL, NULL) and warnings
select c1 mod c1, c1 mod c2, c1 mod c3, c1 mod c4, c1 mod c5, c5 mod c6, c5 mod c7, c5 mod c8, c7 mod c8 from my_binary_t1;

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1 binary(3),
  c2 binary(3),
  c3 binary(3),
  c4 binary(3),
  c5 binary(3),
  c6 binary(10),
  c7 binary(10),
  c8 binary(10),
  c9 binary(10)
);

INSERT INTO my_binary_t1 VALUES ('2', '3', 'a', '2m', 'b2b', '4', '', ' 2', null);

--------------------------------------------------------------------------------

-- should get (4, 6, 0, 4, 0, 8, 0, 8, 0, NULL)
select c1*c1, c1*c2, c1*c3, c1*c4, c1*c5, c1*c6, c6*c7, c6*c8, c7*c8, c6*c9 from my_binary_t1;

DISCARD ALL;

/*
 * MySQL binary type operator greater equal test.
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set enable_seqscan = off;

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1  binary(5),
  c2  binary(5),
  c3  binary(5),
  c4  binary(5),
  c5  binary(5),
  c6  binary(10),
  c7  binary(10),
  c8  binary(10),
  c9  binary(10),
  c10 binary(10),
  c11 binary(10)
);
INSERT INTO my_binary_t1 values('123', '123', '1234', '', null, '1234', 'abcd', 'ABCD', '', null, NULL);

-- TODO[TD015]
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
  my_binary_t1;

-- TODO[TD015]
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
  my_binary_t1;

DISCARD ALL;

/*
 * test mysql binary type operator between and
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1  binary(5)
);

INSERT INTO my_binary_t1 VALUES ('123'), ('12345'), ('abc');

--------------------------------------------------------------------------------

-- should get "('12345'), ('abc')"
SELECT * from my_binary_t1 WHERE c1 NOT BETWEEN '12' AND '1234';

-- should get "('abc')"
SELECT * from my_binary_t1 WHERE c1 NOT BETWEEN '12' AND '12346';

-- should get "('123'), ('12345'), ('abc')"
SELECT * from my_binary_t1 WHERE c1 NOT BETWEEN '124' AND '999';

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1 binary(6)
);

INSERT INTO my_binary_t1 VALUES ('abc'), ('abc123'), ('Abc123'), ('瀚高'), (''), (' '), (null);

--------------------------------------------------------------------------------

-- should get empty
SELECT * FROM my_binary_t1 WHERE c1
  NOT IN ('abc', 'abc123', '瀚高', '\0\0\0\0\0\0', ' \0\0\0\0\0', null);

-- should get empty
SELECT * FROM my_binary_t1 WHERE c1
  NOT IN ('abc\0\0\0', 'abc123', 'Abc123', '瀚高', '', ' ', null);

-- should get "('abc'), ('Abc123'), (''), (' ')"
select * from my_binary_t1 where c1
  not in ('abc', 'abc123', '瀚高');

-- should get "('abc123'), ('瀚高')"
-- TODO[TD007]: can not display chinese
select * from my_binary_t1 where c1
  not in ('abc\0\0\0', 'Abc123', '\0\0\0\0\0\0', ' \0\0\0\0\0');

-- should get empty
select * from my_binary_t1 where c1
  not in ('abc', null);

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1 binary(10)
);

INSERT INTO my_binary_t1 VALUES (' bc'), ('abc'), ('Abc'), ('abcd'), ('瀚高');

--------------------------------------------------------------------------------

-- should get "(' bc'), ('Abc'), ('瀚高')"
-- TODO[TD007]: can not display chinese
select * from my_binary_t1 where c1 not like 'a%';

-- should get "(' bc'), ('abc'), ('abcd'), ('瀚高')"
-- TODO[TD007]: can not display chinese
select * from my_binary_t1 where c1 not like 'A%';

-- should get "(' bc'), ('abc'), ('Abc'), ('abcd'), ('瀚高')"
-- TODO[TD007]: can not display chinese
select * from my_binary_t1 where c1 not like '%c';

-- should get "('瀚高')"
-- TODO[TD007]: can not display chinese
select * from my_binary_t1 where c1 not like '%b%';

-- should get empty
select * from my_binary_t1 where c1 not like '%';

-- should get "(' bc'), ('abc'), ('Abc'), ('abcd')"
-- TODO[TD007]: can not display chinese
select * from my_binary_t1 where c1 not like '%瀚%';

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1 binary(5),
  c2 binary(10),
  c3 binary(10),
  c4 binary(10),
  c5 binary(10),
  c6 binary(10),
  c7 binary(10),
  c8 binary(10)
);

INSERT INTO my_binary_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- should get (0, 0, 0, 1, 1, 1, 1, NULL)
select not c1, not c2, not c3, not c4, not c5, not c6, not c7, not c8 from my_binary_t1;

-- should get (0, 0, 0, 1, 1, 1, 1, NULL)
select !c1, !c2, !c3, !c4, !c5, !c6, !c7, !c8 from my_binary_t1;

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1 binary(5),
  c2 binary(10),
  c3 binary(10),
  c4 binary(10),
  c5 binary(10),
  c6 binary(10),
  c7 binary(10),
  c8 binary(10)
);

INSERT INTO my_binary_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- should get "(1, 1, 1, 1, 1, 0, 0, 0, NULL, NULL)"
select
  c1 OR c1,
  c1 OR c2,
  c1 OR c3,
  c2 OR c3,
  c3 OR c4,
  c4 OR c5,
  c5 OR c6,
  c6 OR c7,
  c4 OR c8,
  c6 OR c8
from my_binary_t1;

-- should get "(1, 1, 1, 1, 1, 0, 0, 0, NULL, NULL)"
select
  c1 || c1,
  c1 || c2,
  c1 || c3,
  c2 || c3,
  c3 || c4,
  c4 || c5,
  c5 || c6,
  c6 || c7,
  c4 || c8,
  c6 || c8
from my_binary_t1;

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1 binary(3),
  c2 binary(3),
  c3 binary(3),
  c4 binary(3),
  c5 binary(3),
  c6 binary(10),
  c7 binary(10),
  c8 binary(10),
  c9 binary(10)
);

INSERT INTO my_binary_t1 VALUES ('123', '999', 'a', '2m', 'b2b', '123', '', ' 12', null);

--------------------------------------------------------------------------------

-- should get (246, 1122, 123, 125, 123, 246, 123, 135, 12, NULL)
select c1+c1, c1+c2, c1+c3, c1+c4, c1+c5, c1+c6, c6+c7, c6+c8, c7+c8, c6+c9 from my_binary_t1;

DISCARD ALL;

/*
 * MySQL binary type operator greater than test.
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set enable_seqscan to off;

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1  binary(5),
  c2  binary(5),
  c3  binary(5),
  c4  binary(5),
  c5  binary(5),
  c6  binary(10),
  c7  binary(10),
  c8  binary(10),
  c9  binary(10),
  c10 binary(10),
  c11 binary(10)
);
INSERT INTO my_binary_t1 values('123', '123', '1234', '', null, '1234', 'abcd', 'ABCD', '', null, NULL);

-- case: c1 <=> c2
-- TODO[TD015]: should get true
select c1 <=> c2 from my_binary_t1;

-- case: c2 <=> c3
select c2 <=> c3 from my_binary_t1; -- should get false

-- case: c3 <=> c6
select c3 <=> c6 from my_binary_t1; -- should get false

-- case: c6 <=> c7
select c6 <=> c7 from my_binary_t1; -- should get false

-- case: c7 <=> c8
select c7 <=> c8 from my_binary_t1; -- should get false

-- case: c1 <=> c4
select c1 <=> c4 from my_binary_t1; -- should get false

-- case: c1 <=> c5
-- TODO[TD015]: should get false
select c1 <=> c5 from my_binary_t1;

-- case: c4 <=> c6
select c4 <=> c6 from my_binary_t1; -- should get false

-- case: c4 <=> c9
select c4 <=> c9 from my_binary_t1; -- should get false

-- case: c4 <=> c10
-- TODO[TD015]: should get false
select c4 <=> c10 from my_binary_t1;

-- case: c5 <=> c10
-- TODO[TD015]: should get true
select c5 <=> c10 from my_binary_t1;

-- case: c10 <=> c11
-- TODO[TD015]: should get true
select c10 <=> c11 from my_binary_t1;

-- TODO[TD015]
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
  my_binary_t1;

DISCARD ALL;

/*
 * test mysql binary type operator between and
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1 binary(5),
  c2 binary(10),
  c3 binary(5),
  c4 binary(5),
  c5 binary(5),
  c6 binary(5)
);

INSERT INTO my_binary_t1 VALUES ('123', '123', 'abc', '', ' ', null);

--------------------------------------------------------------------------------

-- TODO[TD011]: should get (0, -1, -1, 1, -1, NULL), but get error
select
  strcmp(c1, c1),
  strcmp(c1, c2),
  strcmp(c1, c3),
  strcmp(c3, c4),
  strcmp(c4, c5),
  strcmp(c4, c6)
from
  my_binary_t1;

DISCARD ALL;

/*
 * test mysql binary type operator coalesce
 */

-- parameter settings
set ivorysql.compatible_mode to mysql;
set mysql.enable_character_blob to on;
\pset null 'NULL'

-- clean environment
DROP TABLE IF EXISTS my_binary_t1;

CREATE TABLE my_binary_t1(
  c1 binary(5),
  c2 binary(10),
  c3 binary(10),
  c4 binary(10),
  c5 binary(10),
  c6 binary(10),
  c7 binary(10),
  c8 binary(10)
);

INSERT INTO my_binary_t1 VALUES ('123', '123', '1234', 'abcd', 'ABCD', '', ' ', null);

--------------------------------------------------------------------------------

-- should get (0, 0, 0, 0, 1, 0, 0, 0, NULL, NULL)
select c1 XOR c1, c1 XOR c2, c1 XOR c3, c2 XOR c3, c3 XOR c4, c4 XOR c5, c5 XOR c6, c6 XOR c7, c4 XOR c8, c6 XOR c8 from my_binary_t1;

/* Begin - bug#z5465 */
RESET sql_mode;
create table tb_binary(c1 binary(5));
COPY public.tb_binary (c1) FROM stdin;
\\x313233
\.
COPY public.tb_binary (c1) FROM stdin;
\\x3132333435
\.
select * from tb_binary;
COPY public.tb_binary (c1) FROM stdin;
\\x313233343536
\.
select * from tb_binary;
SET sql_mode='';
COPY public.tb_binary (c1) FROM stdin;
\\x313233343536
\.
select * from tb_binary;
RESET sql_mode;
/* End - bug#z5465 */

-- bug 5655
create table tb1(c1 binary(10), c2 binary(10), c3 binary(10));
insert into tb1 values('abcd', '', null);
select c1 AND c3, c2 AND c3 from tb1;
select c1 && c3, c2 && c3 from tb1;
drop table tb1;

-- bug#z5800
select convert('Hello World', binary(5));
select convert(123, binary),convert(123.456, binary);

/* Begin - bug#6051 */
create table t_hex_aggregate (bcol binary(4));
insert into t_hex_aggregate values(NULL);
select  max(bcol) as max_b, min(bcol) as min_b from t_hex_aggregate;
select * from t_hex_aggregate;
select  sum(bcol) as sum_b from t_hex_aggregate;
select  avg(bcol) as avg_b from t_hex_aggregate;

insert into t_hex_aggregate values(0x31323334),(X'35363738'),(x'39303132');
select  max(bcol) as max_b, min(bcol) as min_b from t_hex_aggregate;
select * from t_hex_aggregate;
select  sum(bcol) as sum_b from t_hex_aggregate;
select  avg(bcol) as avg_b from t_hex_aggregate;
drop table t_hex_aggregate;
/* End - bug#6051 */