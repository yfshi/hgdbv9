-- MySQL Character Function Test Cases in PostgreSQL-style Regression SQL Format
-- This file tests MySQL string and character-related built-in functions.

-- LOCATE
SELECT LOCATE('he','hello'),LOCATE('he','hello',2),LOCATE('lo','hello',2) ; -- 1 0 4
SELECT LOCATE('a','b',2),LOCATE('','a',1); -- 0 1
SELECT LOCATE('he','hello',-2);  -- 0

SELECT LOCATE('bar','foobarbar');       -- 4
SELECT LOCATE('bar','foobarbar', 0);    -- 0
SELECT LOCATE('bar','foobarbar', 1);    -- 4

SELECT LOCATE('lo','hello',-4294967295); -- 0
SELECT LOCATE('lo','hello',4294967295); -- 0

SELECT LOCATE('lo','hello',-4294967296); -- 0
SELECT LOCATE('lo','hello',4294967296); -- 0

SELECT LOCATE('lo','hello',-4294967297); -- 0
SELECT LOCATE('lo','hello',4294967297); -- 0

SELECT LOCATE('lo','hello',-18446744073709551615); -- 0
SELECT LOCATE('lo','hello',18446744073709551615); -- 0

SELECT LOCATE('lo','hello',-18446744073709551616); -- 0
SELECT LOCATE('lo','hello',18446744073709551616); -- 0

SELECT LOCATE('lo','hello',-18446744073709551617); -- 0
SELECT LOCATE('lo','hello',18446744073709551617); -- 0

SELECT LOCATE('foo', NULL); -- NULL
SELECT LOCATE(NULL, 'o'); -- NULL
SELECT LOCATE(NULL, NULL); -- NULL

SELECT LOCATE('foo', NULL) IS NULL; -- 1
SELECT LOCATE(NULL, 'o') IS NULL; -- 1
SELECT LOCATE(NULL, NULL) IS NULL; -- 1

SELECT ISNULL(LOCATE('foo', NULL)); -- 1
SELECT ISNULL(LOCATE(NULL, 'o')); -- 1
SELECT ISNULL(LOCATE(NULL, NULL)); -- 1

SELECT LOCATE('foo', NULL) <=> NULL; -- 1
SELECT LOCATE(NULL, 'o') <=> NULL; -- 1
SELECT LOCATE(NULL, NULL) <=> NULL; -- 1

CREATE TABLE t1 (id int NOT NULL PRIMARY KEY, a varchar(10), p varchar(10));
INSERT INTO t1 VALUES (1, 'foo', 'o');
INSERT INTO t1 VALUES (2, 'foo', NULL);
INSERT INTO t1 VALUES (3, NULL, 'o');
INSERT INTO t1 VALUES (4, NULL, NULL);
SELECT id, LOCATE(a,p) FROM t1;
SELECT id, LOCATE(a,p) IS NULL FROM t1;
SELECT id, ISNULL(LOCATE(a,p)) FROM t1;
SELECT id, LOCATE(a,p) <=> NULL FROM t1;
SELECT id FROM t1 WHERE LOCATE(a,p) IS NULL;
SELECT id FROM t1 WHERE LOCATE(a,p) <=> NULL;
drop table t1;

create table t_locate as select LOCATE('he','hello');
\d t_locate
drop table t_locate;

-- LCASE / LOWER
SELECT lcase(1);
SELECT lcase(1.1);
SELECT LCASE('Hello WORLD'); -- 'hello world'
SELECT LCASE(NULL); -- NULL
SELECT REPLACE(CONCAT(LCASE(CONCAT('THIS',' ','IS',' ','A',' ')),UCASE('false'),' ','test'),'FALSE','REAL') ;

-- UCASE / UPPER
SELECT ucase(1);
SELECT ucase(1.1);
SELECT UCASE('Hello WORLD'); -- 'HELLO WORLD'
SELECT UCASE(NULL); -- NULL

-- INSERT
SELECT INSERT('abcdefg', 2, 3, 'Z'); -- 'aZefg'
SELECT INSERT('abcdefg', '2a', 3, 'Z'); -- 'aZefg'
SELECT INSERT('abcdefg', '2a', '3a', 'Z'); -- 'aZefg'

SELECT INSERT(NULL, 2, 3, 'Z'); -- NULL
SELECT INSERT('abcdefg', -1, 3, 'Z'); -- 'abcdefg'
SELECT INSERT('abc', 1, -3, '1234'); -- '1234'
SELECT INSERT('abc', -1, -3, '1234'); -- 'abc'

SELECT INSERT('abc', 3, 3, '1234'); -- 'ab1234'
SELECT INSERT('abc', 4, 3, '1234'); -- 'abc'
SELECT INSERT('abc', 5, 3, '1234'); -- 'abc'
SELECT INSERT('abc', 6, 3, '1234'); -- 'abc'
SELECT INSERT('abc', 6, 2147483646, '1234'); -- 'abc'
SELECT INSERT('abc',  '2147483646',6, '1234'); -- 'abc'
SELECT INSERT('abc',  2147483646,'6', '1234'); -- 'abc'
SELECT INSERT('abc',  '2147483646','6', '1234'); -- 'abc'
SELECT INSERT('Quadratic', 3, 4, 'What');   -- 'QuWhattic'
SELECT INSERT('Quadratic', -1, 4, 'What');  -- 'Quadratic'
SELECT INSERT('Quadratic', 3, 100, 'What'); -- 'QuWhat'

create table t_insert as SELECT INSERT('abc', 3, 3, '1234');
\d t_insert
drop table t_insert;

-- BIN
SELECT BIN(0); -- '0'
SELECT BIN(-1);   -- '1111111111111111111111111111111111111111111111111111111111111111'
SELECT BIN(-1.2); -- '1111111111111111111111111111111111111111111111111111111111111111'
SELECT BIN(-1.6); -- '1111111111111111111111111111111111111111111111111111111111111111'
SELECT BIN(-10);  -- '1111111111111111111111111111111111111111111111111111111111110110'
SELECT BIN(1.2); -- '1'
SELECT BIN(1.6); -- '1'
SELECT BIN(10); -- '1010'
SELECT BIN(NULL); -- NULL
SELECT BIN('a'); -- '0'
SELECT bin(cast(111 as datetime)); -- '11111010000'
SELECT bin(cast(20250101 as datetime)); -- '11111101001'
SELECT bin(1,2);


-- CHAR
SELECT CHAR(65, 66, 67); -- 'ABC'
SELECT CHAR(NULL); -- NULL

-- ELT
SELECT ELT(2, 'a', 'b', 'c'); -- 'b'
SELECT ELT(0, 'a', 'b', 'c'); -- NULL
SELECT ELT(NULL, 'a', 'b'); -- NULL
SELECT ELT('1', 'a', 'b'); -- NULL
SELECT ELT('c', 'a', 'b'); -- NULL
SELECT ELT(1, 2, 3); -- 2

CREATE TABLE t_elt as SELECT ELT(2, 'a', 'b', 'c');
\d t_elt
drop table t_elt;

-- FIELD
SELECT FIELD('b', 'a', 'b', 'c'); -- 2
SELECT FIELD(NULL, 'a', 'b'); -- 0
SELECT FIELD('a', NULL, 'b'); -- 0
--SELECT FIELD('b','A','B'); -- 2
SELECT FIELD('B','A','B'); -- 2
SELECT FIELD('1','3','2',1); -- 3
SELECT FIELD(1, 3, 2, 1); -- 3
SELECT FIELD(1::int1 unsigned, 3::int2 unsigned, 2::int4 unsigned, 1::int8 unsigned);

create table t_field as SELECT FIELD('b', 'a', 'b', 'c');;
\d t_field
drop table t_field;

-- FIND_IN_SET
SELECT FIND_IN_SET('b', 'a,b,c'); -- 2
SELECT FIND_IN_SET(NULL, 'a,b,c'); -- NULL
SELECT FIND_IN_SET('a', NULL); -- NULL
SELECT FIND_IN_SET(1, 'a,2,1'); -- 3
SELECT FIND_IN_SET('ab', 'abc,ab,1'); -- 2

-- HEX
SELECT HEX('abc'); -- 616263
SELECT HEX(12356612313); -- 2E082F0D9
SELECT HEX(-12356612313); -- FFFFFFFD1F7D0F27
SELECT HEX(123.4); -- 7B
SELECT HEX(123.5); -- 7C
SELECT HEX(-123.4); -- FFFFFFFFFFFFFF85
SELECT HEX(-123.5); -- FFFFFFFFFFFFFF84
SELECT HEX(0); -- 0
SELECT HEX(NULL); -- NULL
SELECT HEX(cast('2025-12-12' as date)); -- 323032352D31322D3132
SELECT HEX(cast('111112111.1.29' as datetime)); -- 323031312D31312D31322031313A30313A3031
SELECT HEX(cast('10:57:11' as time)); -- 31303A35373A3131
SELECT HEX(true); -- 1
SELECT HEX(false); -- 0

-- SPACE
SELECT CONCAT('A', SPACE(3), 'B'); -- 'A   B'
SELECT SPACE(-1); -- ''
SELECT CONCAT('A', SPACE('4'), 'B'); -- 'A    B'
SELECT CONCAT('A', SPACE('a'), 'B'); -- 'AB'

-- SOUNDEX
SELECT SOUNDEX('hello'); -- 'H400'
SELECT SOUNDEX(NULL); -- NULL
SELECT SOUNDEX(''),SOUNDEX('he'),SOUNDEX('hello all folks'),SOUNDEX('#3556 in bugdb');
SELECT SOUNDEX(true);
SELECT SOUNDEX(123);

-- EXPORT_SET
SELECT EXPORT_SET(5, 'Y', 'N', ':'); -- Y:N:Y:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N
SELECT EXPORT_SET(5, 'Y', 'N', ':', 4); -- Y:N:Y:N
SELECT EXPORT_SET(5, 'Y', 'N', ',', 4); -- 'Y,N,Y,N'
SELECT EXPORT_SET(NULL, 'Y', 'N'); -- NULL
SELECT EXPORT_SET('5', 'Y', 'N', ',', 4); -- 'Y,N,Y,N'
SELECT EXPORT_SET('5', 'Y', 'N', ','); --'Y,N,Y,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N'
SELECT EXPORT_SET('5', 'Y', 'N'); -- 'Y,N,Y,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N'
SELECT EXPORT_SET('a', 'Y', 'N', ',', 4); -- 'N,N,N,N'


-- ORD
SELECT ORD('A'); -- 65
SELECT ORD(NULL); -- NULL

-- SUBSTRING_INDEX
SELECT SUBSTRING_INDEX('a,b,c,d', ',', 2); -- 'a,b'
SELECT SUBSTRING_INDEX('a,b,c,d', ',', -2); -- 'c,d'
SELECT SUBSTRING_INDEX(NULL, ',', 2); -- NULL
select substring_index('a..b..c', '..', 0); -- ''
select substring_index('a..b..c', '..', -2147483648); -- 'a..b..c'
select substring_index('a..b..c', '..', -2147483649); -- 'a..b..c'
select substring_index('a..b..c', '..', 2147483647); -- 'a..b..c'
select substring_index('a..b..c', '..', 2147483648); -- 'a..b..c'
select substring_index('a..b..c', '..', 9223372036854775807); -- 'c'
select substring_index('a..b..c', '..', 9223372036854775808); -- ''
select substring_index('a..b..c', '..', -9223372036854775808); -- ''
select substring_index('a..b..c', '..', -9223372036854775809); -- ''
select substring_index('a..b..c', '..', 18446744073709551615); -- 'c'
select substring_index('a..b..c', '..', 18446744073709551616); -- 'c'

create table t_sub as SELECT SUBSTRING_INDEX('a,b,c,d', ',', 2);
\d t_sub
drop table t_sub;

-- FROM_BASE64 / TO_BASE64
SELECT FROM_BASE64('aGVsbG8='); -- 'hello'
SELECT TO_BASE64('hello'); -- 'aGVsbG8='
SELECT FROM_BASE64(NULL); -- NULL
SELECT TO_BASE64(NULL); -- NULL
SELECT FROM_BASE64('Zg=='); -- 'f'
SELECT TO_BASE64('f'); -- 'Zg=='
SELECT FROM_BASE64('Zm8='); -- 'fo'
SELECT TO_BASE64('fo'); -- 'Zm8='
SELECT FROM_BASE64('Zm9v'); -- 'foo'
SELECT TO_BASE64('foo'); -- 'Zm9v'
SELECT FROM_BASE64('Zm9vYg=='); -- 'foob'
SELECT TO_BASE64('foob'); -- 'Zm9vYg=='
SELECT FROM_BASE64('Zm9vYmE='); -- 'fooba'
SELECT TO_BASE64('fooba'); -- 'Zm9vYmE='
SELECT FROM_BASE64('Zm9vYmFy'); -- 'foobar'
SELECT TO_BASE64('foobar'); -- 'Zm9vYmFy'

-- Delimiters are allowed at any position
SELECT hex(FROM_BASE64('  A B C D  ')); -- '001083'
SELECT hex(FROM_BASE64('  A A = = ')); -- '00'
SELECT hex(FROM_BASE64('  A A A = ')); -- '0000'
/* use psql return NULL; use libpq return 001083 */
SELECT hex(FROM_BASE64('  A  \n  B  \r  C  \t D  ')); -- '001083'

create table t_hex_binary (a varbinary(111));
insert into t_hex_binary values (FROM_BASE64('  A B C D  '));
select hex(a::binary) from t_hex_binary;  -- '001083'
drop table t_hex_binary;

-- UUID
--SELECT UUID(); -- (example format)

-- LENGTH / BIT_LENGTH / OCTET_LENGTH /
SELECT LENGTH('abc'); -- 3
SELECT LENGTH('你好'); -- 6 (utf8mb4)
SELECT LENGTH(NULL); -- NULL
--SELECT LENGTH(true);
SELECT LENGTH(123);
SELECT LENGTH(cast(111 as datetime));
SELECT BIT_LENGTH('abc'); -- 24
SELECT OCTET_LENGTH('abc'); -- 3
SELECT OCTET_LENGTH(12356612313); -- 11
SELECT OCTET_LENGTH(-12356612313); -- 12
SELECT OCTET_LENGTH(123.4); -- 5
SELECT OCTET_LENGTH(-123.4); -- 6
SELECT OCTET_LENGTH(0); -- 1
SELECT OCTET_LENGTH(NULL); -- NULL
SELECT OCTET_LENGTH(cast('2025-12-12' as date)); -- 10
SELECT OCTET_LENGTH(cast('111112111.1.29' as datetime)); --19
SELECT OCTET_LENGTH(cast('10:57:11' as time)); -- 8
SELECT OCTET_LENGTH(true); -- 1
SELECT OCTET_LENGTH(false); -- 1

-- CONVERT
--SELECT CONVERT('你好' USING latin1); -- likely garbled
--SELECT CONVERT(NULL USING utf8mb4); -- NULL

-- FORMAT
/*
SELECT FORMAT(12345.6789, 2); -- '12,345.68'
SELECT FORMAT(12345.6789, 2, 'de_DE'); -- '12.345,68'
SELECT FORMAT(NULL, 2); -- NULL
*/

-- LEFT / RIGHT
SELECT LEFT('abcdef', 3); -- 'abc'
SELECT LEFT('abcdef', '3'); -- 'abc'
SELECT LEFT(1234, '3'); -- 123
SELECT LEFT(1234, 3) + 0; -- 123
SELECT LEFT(12.34, 3.0) + 0; -- 12
SELECT LEFT(NULL, 3); -- NULL
SELECT LEFT(true, 1); -- 1
SELECT LEFT(false, 1); -- 0
SELECT LEFT('abcd', true); -- 'a'
SELECT LEFT('abcd', false); -- ''
SELECT RIGHT('abcdef', 3); -- 'def'
SELECT RIGHT('abcdef', '3'); -- 'def'
SELECT RIGHT(1234, '3'); -- 234
SELECT RIGHT(1234, 3) + 0; -- 234
SELECT RIGHT(12.34, 3.0) + 0; -- 0.34
SELECT RIGHT(NULL, 3); -- NULL
SELECT RIGHT(true, 1); -- 1
SELECT RIGHT(false, 1); -- 0
SELECT RIGHT('abcd', true); -- 'd'
SELECT RIGHT('abcd', false); -- ''

-- NAME_CONST
SELECT NAME_CONST('test', NOW()); --error ER_WRONG_ARGUMENTS
SELECT NAME_CONST('test', UPPER('test')); --error ER_WRONG_ARGUMENTS
SELECT NAME_CONST('test', NULL); -- NULL
SELECT NAME_CONST('test', 1); -- '1'
SELECT NAME_CONST('test', -1); -- '-1'
SELECT NAME_CONST('test', 1.0); -- '1.0'
SELECT NAME_CONST('test', -1.0); -- '-1.0'
SELECT NAME_CONST('test', 'test'); -- 'test'

CREATE TABLE t1 (a INT);
INSERT INTO t1 VALUES (1),(2),(3);
SELECT NAME_CONST('flag',1)    * MAX(a) FROM t1; -- '3'
SELECT NAME_CONST('flag',1.5)  * MAX(a) FROM t1; -- '4.5'
SELECT NAME_CONST('flag',-1)   * MAX(a) FROM t1; -- '-3'
SELECT NAME_CONST('flag',-1.5) * MAX(a) FROM t1; -- '-4.5'
SELECT NAME_CONST('flag', SQRT(4)) * MAX(a) FROM t1; --error ER_WRONG_ARGUMENTS
SELECT NAME_CONST('flag',-SQRT(4)) * MAX(a) FROM t1; --error ER_WRONG_ARGUMENTS
SELECT NAME_CONST(x,2) FROM (SELECT a x FROM t1) t; --error ER_WRONG_ARGUMENTS
SELECT NAME_CONST(a, '1') FROM t1; --error ER_WRONG_ARGUMENTS
SELECT NAME_CONST(a, '1'); --error ER_WRONG_ARGUMENTS
SELECT NAME_CONST('a', '1'); -- '1'
SELECT NAME_CONST(2, '1'); -- ok
SELECT NAME_CONST(2.345, '1'); -- ok
DROP TABLE t1;

-- POSITION
select position('bar' IN 'foobarbar'); -- 4
select position('xbar' IN  'foobar'); -- 0
