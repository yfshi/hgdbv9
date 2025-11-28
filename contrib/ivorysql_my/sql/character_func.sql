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
SELECT BIN(cast(111 as datetime)); -- '11111010000'
SELECT BIN(cast(20250101 as datetime)); -- '11111101001'
SELECT BIN(1,2); -- error
SELECT BIN('-9223372036854775809'); -- '111111111111111111111111111111111111111111111111111111111111111'
SELECT BIN(-9223372036854775809); -- '111111111111111111111111111111111111111111111111111111111111111'
SELECT BIN('-9223372036854775809123111111111111'); -- '1111111111111111111111111111111111111111111111111111111111111111'
SELECT BIN(-9223372036854775809123111111111111); -- '1111111111111111111111111111111111111111111111111111111111111111'
SELECT BIN('-123'); -- '1111111111111111111111111111111111111111111111111111111110000101'
SELECT BIN(-123); -- '1111111111111111111111111111111111111111111111111111111110000101'
SELECT BIN(null); -- NULL
SELECT BIN(true); -- '1'
SELECT BIN(false); -- '0'

-- CHAR
SELECT CHAR(65, 66, 67); -- '\x414243'
SELECT CHAR(NULL); -- '\x'
SELECT CHAR(72, 101, 108, 108, 111); -- '\x48656C6C6F'
SELECT CHAR(72, NULL, 108, 111); -- '\x486C6F'
SELECT CHAR(65, 999999, 66); -- '0x410F423F42'
SELECT HEX(CHAR(-65)); -- 'FFFFFFBF'
SELECT CHAR(0), CHAR(65535); --'\x00 \xFFFF'
SELECT CHAR(-1),CHAR(65536); -- '\xFFFFFFFF \x010000'
SELECT CHAR(0),CHAR(127); -- '\x00 \x7F'
SELECT CHAR(-1),CHAR(128); -- '\xFFFFFFFF \x80'
SELECT CHAR(1114111),CHAR(1114112),CHAR(4294967295); -- '\x10FFFF \x110000 \xFFFFFFFF'
SELECT CHAR(128); -- '\x80'
SELECT CHAR(255); -- '\xff'
SELECT CHAR(65536); -- '\x010000'
SELECT CHAR(11111111); -- '\xa98ac7'
SELECT CHAR(111111111111); -- '\xdebd01c7'
SELECT CHAR(1111111111111111); -- '\xb71571c7'
SELECT CHAR(-2); -- '\xfffffffe'
SELECT CHAR(-3, 95, -6); -- '\xfffffffd5ffffffffa'
SELECT CHAR(6.1); -- '\x06'
SELECT CHAR(-4.5); -- '\xfffffffb'
SELECT CHAR(100, -27, 3.82); -- '\x64ffffffe504'
select char('100'); -- '\x64'
SELECT CHAR('10.7'); -- '\x0a'

-- CONCAT
SELECT CONCAT(20, 30, 40); -- '203040'
SELECT CONCAT('A', 'B', 'C'); -- 'ABC'
SELECT CONCAT(100, 'a', 200); -- '100a200'
SELECT CONCAT(20, NULL, 30); -- NULL
SELECT CONCAT('a', 50, NULL, true); -- NULL

-- ELT
SELECT ELT(2, 'a', 'b', 'c'); -- 'b'
SELECT ELT(0, 'a', 'b', 'c'); -- NULL
SELECT ELT(NULL, 'a', 'b'); -- NULL
SELECT ELT('1', 'a', 'b'); -- NULL
SELECT ELT('c', 'a', 'b'); -- NULL
SELECT ELT(1, 2, 3); -- 2
SELECT ELT(3, 'Apple', 'Banana', 'Cherry'); -- 'Cherry'
SELECT ELT(-1, 'Apple', 'Banana', 'Cherry'); -- NULL
SELECT ELT(2.5, 'Apple', 'Banana', 'Cherry'); -- 'Cherry'
SELECT ELT(1); -- error
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
/*Begin BUG#Z5516 */
SELECT FIELD(2-2, 1, TRUE, 0);
SELECT FIELD(TRUE, '1', 'TRUE', '0');
SELECT FIELD(2-2, 1, 2, 0);
SELECT FIELD(2-1, '1', 'TRUE', '0');
SELECT FIELD(2-2, NULL, 'TRUE', '0');
SELECT FIELD('banana', 'Apple', 'Banana', 'Cherry');
SELECT FIELD('Apple', NULL, 'Apple', 'Banana');

SELECT FIELD(2-1, 'TRUE', '0', '1');
SELECT FIELD('1', 'TRUE', '0', '1');
SELECT FIELD('0', 'TRUE', '0', '1');
SELECT FIELD(0, 'TRUE', '0', '1');

SELECT FIELD('0', 'FALSE', '0', '1');
SELECT FIELD(0, 'FALSE', '0', '1');
SELECT FIELD('0', '12a','FALSE', 0, '23a');
SELECT FIELD(23, '12a','FALSE', 0, '23a');
SELECT FIELD(NULL, NULL, '1', '1.1');
SELECT FIELD(1.1, NULL, '1', '1.1');
SELECT FIELD('a'::char, NULL, '1', 'a');
SELECT FIELD(' ', 'a',' ', 0, '23a');

create table t1 (id int,str text);
insert into t1 values (1, 'ab');
SELECT FIELD(id, 1, 2, 0) from t1;
SELECT FIELD(str, '1', 'abc', 'ab') from t1;
drop table t1;
/*End BUG#Z5516 */

create table t_field as SELECT FIELD('b', 'a', 'b', 'c');;
\d t_field
drop table t_field;

-- FIND_IN_SET
SELECT FIND_IN_SET('b', 'a,b,c'); -- 2
SELECT FIND_IN_SET(NULL, 'a,b,c'); -- NULL
SELECT FIND_IN_SET('a', NULL); -- NULL
SELECT FIND_IN_SET(1, 'a,2,1'); -- 3
SELECT FIND_IN_SET('ab', 'abc,ab,1'); -- 2
SELECT FIND_IN_SET('a,b','a,b,c'); -- 0
SELECT FIND_IN_SET('a,b','"a,b",c,d'); -- 0
SELECT FIND_IN_SET('a\,b','a\,b,c,d'); -- 0
SELECT FIND_IN_SET(',',',a,b'); -- 0
SELECT FIND_IN_SET('Orange', 'Apple,Banana,Cherry'); -- 0
SELECT FIND_IN_SET('banana', 'Apple,Banana,Cherry'); -- 2
SELECT FIND_IN_SET('Apple', ''); -- 0
SELECT FIND_IN_SET('Banana', 'Apple, Banana ,Cherry'); -- 0

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
select HEX(X'4D7953516C'); -- 4D7953516C
select HEX(FROM_BASE64(TO_BASE64(0xF8))); -- F8

-- SPACE
SELECT CONCAT('A', SPACE(3), 'B'); -- 'A   B'
SELECT SPACE(-1); -- ''
SELECT CONCAT('A', SPACE('4'), 'B'); -- 'A    B'
SELECT CONCAT('A', SPACE('a'), 'B'); -- 'AB'
SELECT CONCAT('A', SPACE(NULL), 'B'); -- NULL

-- SOUNDEX
SELECT SOUNDEX('hello'); -- 'H400'
SELECT SOUNDEX(NULL); -- NULL
SELECT SOUNDEX(''),SOUNDEX('he'),SOUNDEX('hello all folks'),SOUNDEX('#3556 in bugdb');
SELECT SOUNDEX(true);
SELECT SOUNDEX(123);
SELECT SOUNDEX('中文'); -- '中000'

-- EXPORT_SET
SELECT EXPORT_SET(5, 'Y', 'N', ':'); -- Y:N:Y:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N:N
SELECT EXPORT_SET(5, 'Y', 'N', ':', 4); -- Y:N:Y:N
SELECT EXPORT_SET(5, 'Y', 'N', ',', 4); -- 'Y,N,Y,N'
SELECT EXPORT_SET(NULL, 'Y', 'N'); -- NULL
SELECT EXPORT_SET('5', 'Y', 'N', ',', 4); -- 'Y,N,Y,N'
SELECT EXPORT_SET('5', 'Y', 'N', ','); --'Y,N,Y,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N'
SELECT EXPORT_SET('5', 'Y', 'N'); -- 'Y,N,Y,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N'
SELECT EXPORT_SET('a', 'Y', 'N', ',', 4); -- 'N,N,N,N'
SELECT EXPORT_SET(8, 'Y', 'N', 9, 8); -- 'N9N9N9Y9N9N9N9N'
SELECT EXPORT_SET(8, 'Y', 'N', -1, 4); -- 'N-1N-1N-1Y'
SELECT EXPORT_SET(255, 'Y', 'N', ',', 4.9); -- 'Y,Y,Y,Y,Y'
SELECT EXPORT_SET(8, 'Y', 'N', ',', 12.36); -- 'N,N,N,Y,N,N,N,N,N,N,N,N'

-- ORD
SELECT ORD('A'); -- 65
SELECT ORD(NULL); -- NULL
SELECT ORD(char(10)); -- 10
SELECT ORD(char(255)); -- 255
SELECT ORD(char(-2)); -- 255
SELECT ORD(char(6.66)); -- 7

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
SELECT OCTET_LENGTH(binary 'Hello'); -- 5
SET standard_conforming_strings to OFF;
SELECT OCTET_LENGTH('Hello\tWorld\n'); -- 12
SET standard_conforming_strings to ON;
SELECT OCTET_LENGTH('Hello\tWorld\n'); -- 14

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
select LEFT('zxcvbnm',1.4); -- 'z'
select LEFT('zxcvbnm',1.5); -- 'zx'
select LEFT('zxcvbnm', -6); -- NULL
select LEFT('zxcvbnm', '-6'); -- NULL
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
select RIGHT('zxcvbnm',1.4); -- 'm'
select RIGHT('zxcvbnm',1.5); -- 'nm'
select RIGHT('zxcvbnm', -6); -- NULL
select RIGHT('zxcvbnm', '-6'); -- NULL

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


-- CONV
SELECT conv(NULL    , 10,  2) AS result;  -- NULL
SELECT conv('123'   , NULL, 2) AS result;  -- NULL
SELECT conv('123'   , 10,  NULL) AS result; -- NULL
SELECT conv('123', 1,  10) AS result;  -- NULL  (from_base<2)
SELECT conv('123', 37, 10) AS result;  -- NULL  (from_base>36)
SELECT conv('123', 10, 1) AS result;   -- NULL  (to_base<2)
SELECT conv('123', 10, 37) AS result;  -- NULL  (to_base>36)
SELECT conv('0', 10, 2)    AS result;  -- '0'
SELECT conv('1', 10, 2)    AS result;  -- '1'
SELECT conv('10', 10, 2)   AS result;  -- '1010'
SELECT conv('FF', 16, 10)  AS result;  -- '255'
SELECT conv('255', 10, 16) AS result;  -- 'FF'
SELECT conv('aBc', 16, 10) AS result;  -- '2748'
SELECT conv('  15  ', 10, 2) AS result; -- '1111'
SELECT conv('+15', 10, 2) AS result;    -- '1111'
SELECT conv('+0F',16,2)  AS result;    -- '1111'
SELECT conv('12X3', 10, 2) AS result;   -- '1100'
SELECT conv('D2X3', 10, 2) AS result;   -- '0'
SELECT conv('18446744073709551615', 10, 10) AS result;  -- '18446744073709551615'
SELECT conv('18446744073709551616', 10, 10) AS result;  -- '18446744073709551615'
SELECT conv('-18446744073709551616', 10, 10) AS result; -- '18446744073709551615'
SELECT conv('1844674407370955161511', 10, 10) AS result;  -- '18446744073709551615'
SELECT conv('-1844674407370955161511', 10, 10) AS result;  -- '18446744073709551615'
SELECT conv('-18446744073709551615', 10, 10) AS result; -- '1'
SELECT conv('-18446744073709551614', 10, 10) AS result; -- '2'
SELECT conv('-18446744073709551613', 10, 10) AS result; -- '3'
SELECT conv('-1',   10, 2)   AS result;  -- '1111111111111111111111111111111111111111111111111111111111111111'
SELECT conv('-1',   10, -2)  AS result;  -- '-1'
SELECT conv('-17', -10, 2)   AS result;  -- '1111111111111111111111111111111111111111111111111111111111101111'
SELECT right(conv('-17', -10, 2), 8) AS result; -- '11101111' (-17 mod 256 = 239)
SELECT conv('-17', 10, -18)  AS result;  -- '-H'
SELECT conv('17', 10, -18)   AS result;  -- 'H'
SELECT conv('ZZZZZZZZ', 36, 10) AS result;  -- '2821109907455'
SELECT conv((10 + '10'::int + '10'::int + x'0a'::int)::text, 10, 10) AS result;  -- '40'
SELECT conv('101010', 2, 3) AS result;  -- '1120'
SELECT conv('Z', 36, 10) AS result;   -- '35'
SELECT conv('123ABCxyz', 36, 16) AS result; -- '2B6FF6839FB'
SELECT conv('-0', 10, 2)  AS result;  -- '0'
SELECT conv('+0', 10, 2)  AS result;  -- '0'
SELECT conv('',   10, 2) AS result;  -- NULL
SELECT conv(2,    '', 2) AS result;  -- NULL
SELECT conv(2,   10, '') AS result;  -- NULL
SELECT conv('AB',16.5,32) AS result; -- '5L'
SELECT conv('AB',16.4,32) AS result; -- '5B'
SELECT conv('AB',32,16.5) AS result; -- '128'
SELECT conv('AB',32,16.4) AS result; -- '14B'

-- unhex

SET bytea_output = 'hex';
SELECT unhex('');       -- '\x'
SELECT unhex('123');    -- '\x0123'
SELECT unhex(123);      -- '\x0123'
SELECT unhex(+123);     -- '\x0123'
SELECT unhex(-123);     -- '\x'
SELECT unhex(123.1);    -- '\x'
SELECT unhex('GG');     -- '\x'
SELECT unhex('4D7953514C'); -- '\x4D7953514C'
SELECT unhex(BINARY '414243'); -- '\x414243'

SET bytea_output = 'escape';
SELECT unhex('');       -- ''
SELECT unhex('123');    -- '\001#'
SELECT unhex(123);      -- '\001#'
SELECT unhex(+123);     -- '\001#'
SELECT unhex(-123);     -- 'NULL'
SELECT unhex(123.1);    -- 'NULL'
SELECT unhex('GG');     -- 'NULL'
SELECT unhex('4D7953514C'); -- 'MySQL'
SELECT unhex(BINARY '414243'); -- 'ABC'

-- strmcp
select strcmp(null, null);
select strcmp(null, 'other');
select strcmp('other', null);
select strcmp('something','other');
select strcmp('something','something');
select strcmp(null, 321);
select strcmp(321, null);
select strcmp(321, 321);
select strcmp(321, 123);
select strcmp(null, 321::int2);
select strcmp(321::int2, null);
select strcmp(321::int2, 321::int2);
select strcmp(321::int2, 123::int2);
select strcmp(null, 321::int4);
select strcmp(321::int4, null);
select strcmp(321::int4, 321::int4);
select strcmp(321::int4, 123::int4);
select strcmp(null, 321::int8);
select strcmp(321::int8, null);
select strcmp(321::int8, 321::int8);
select strcmp(321::int8, 123::int8);
select strcmp(null, 321::float4);
select strcmp(321::float4, null);
select strcmp(321::float4, 321::float4);
select strcmp(321::float4, 123::float4);
select strcmp(null, 321::float8);
select strcmp(321::float8, null);
select strcmp(321::float8, 321::float8);
select strcmp(321::float8, 123::float8);

select strcmp(null,'something'::text);
select strcmp('something'::text,null);
select strcmp('something'::text,'something'::text);
select strcmp('something'::text,'other'::text);

select strcmp(null, '2001-01-01 01:01:01.3654'::date);
select strcmp('2001-01-01 01:01:01.3654'::date, null);
select strcmp('2001-01-01 01:01:01.3654'::date, '2001-01-01 01:01:01.3654'::date);
select strcmp('2001-01-01 01:01:01.3654'::date, '1999-01-01 01:01:01.3654'::date);

-- instr

select instr(null,null);
select instr(null,'bc');
select instr('abcd12abc',null);
select instr('abcd12abc','bc');
select instr('abcd12abc','BC');
select instr('abcd12abc' collate utf8_general_ci,'BC' collate utf8_general_ci);
select instr('abcd12abc','ff');
select instr(1234, 23);
select instr(1234.5, 34.5);

-- if
set mysql.enable_mysql_information_schema TO on;
set mysql.use_mysql_bool TO true;

CREATE TABLE type_analysis AS
SELECT
	-- 1. Numeric types vs String types
	IF(TRUE, 123, 'abc') as int_to_string,
	IF(TRUE, 'abc', 123) as string_to_int,
	IF(TRUE, 45.67, 'text') as float_to_string,
	IF(TRUE, 'text', 45.67) as string_to_float,

	-- 2. Mixing different numeric types
	IF(TRUE, 123, 45.67) as int_to_float,
	IF(TRUE, 45.67, 123) as float_to_int,

	-- 3. Large numeric type testing
	IF(TRUE, 123456789012345, 'big_number') as bigint_to_string,
	IF(TRUE, 999999999.9999, 'decimal_text') as decimal_to_string,
	IF(TRUE, 'text', 123456789012345) as string_to_bigint,

	-- 4. Numeric types vs Date types
	IF(TRUE, 100, '2023-01-01'::mysdate) as int_to_date,
	IF(TRUE, '2023-01-01'::mysdate, 100) as date_to_int,
	IF(TRUE, 99.99, '2023-01-01'::mysdate) as float_to_date,
	IF(TRUE, '2023-01-01'::mysdate, 99.99) as date_to_float,

	-- 5. String types vs Date types
	IF(TRUE, 'hello world', '2023-01-01'::mysdate) as string_to_date,
	IF(TRUE, '2023-01-01'::mysdate, 'hello world') as date_to_string,
	IF(TRUE, 'text', '2025-11-25 15:20:27'::mysdatetime) as string_to_datetime,
	IF(TRUE, '2025-11-25 15:20:27'::mysdatetime, 'text') as datetime_to_string,

	-- 6. Mixing different datetime types
	IF(TRUE, '2025-11-25'::mysdate, '2025-11-25 15:20:27'::mysdatetime) as date_to_datetime,
	IF(TRUE, '2025-11-25 15:20:27'::mysdatetime, '2025-11-25'::mysdate) as datetime_to_date,

	-- 7. FALSE condition testing
	IF(FALSE, 123, 'abc') as int_to_string_false,
	IF(FALSE, 'abc', 123) as string_to_int_false,
	IF(FALSE, 45.67, 'text') as float_to_string_false,
	IF(FALSE, '2023-01-01', 100) as date_to_int_false,
	IF(FALSE, TRUE, 'false_text') as bool_to_string_false,

	-- 8. Boolean types vs Other types
	IF(TRUE, TRUE, 'false_text') as bool_to_string,
	IF(TRUE, 'true_text', FALSE) as string_to_bool,
	IF(TRUE, TRUE, 0) as bool_to_int,
	IF(TRUE, 1, FALSE) as int_to_bool,

	-- 9. NULL values with other types
	IF(TRUE, NULL, 'not_null') as null_to_string,
	IF(TRUE, 'not_null', NULL) as string_to_null,
	IF(TRUE, NULL, 123) as null_to_int,
	IF(TRUE, 456, NULL) as int_to_null,
	IF(TRUE, NULL, 45.67) as null_to_float,
	IF(TRUE, 89.12, NULL) as float_to_null,
	IF(TRUE, NULL, TRUE) as null_to_bool,
	IF(TRUE, FALSE, NULL) as bool_to_null,

	-- 10. Time types vs Other types
	IF(TRUE, '11:00:00'::time, '12:00:00') as time_to_string,
	IF(TRUE, '12:00:00', '11:00:00'::time) as string_to_time,
	IF(TRUE, '2025-11-25 15:04:55'::mysdatetime, 12345) as datetime_to_int,
	IF(TRUE, 67890, '2025-11-25 15:04:55'::mysdatetime) as int_to_datetime,
	IF(TRUE, '11:00:00'::mystime, 123) as time_to_int,
	IF(TRUE, 456, '11:00:00'::mystime) as int_to_time,

	-- 11. Binary data vs Other types
	IF(TRUE, BINARY 'binary_data', 'text_data') as binary_to_string,
	IF(TRUE, 'text_data', BINARY 'binary_data') as string_to_binary,
	IF(TRUE, BINARY 'data', 123) as binary_to_int,
	IF(TRUE, 456, BINARY 'data') as int_to_binary,

	-- 12. Hexadecimal and bit types
	IF(TRUE, X'4D7953514C', 'hex_text') as hex_to_string,
	IF(TRUE, 'hex_text', X'4D7953514C') as string_to_hex,
	IF(TRUE, b'1010', 'bit_text') as bit_to_string,
	IF(TRUE, 'bit_text', b'1010') as string_to_bit,

	-- 13. Scientific notation numbers
	IF(TRUE, 1.23e5, 'scientific') as scientific_to_string,
	IF(TRUE, '1.23e5', 123000) as string_to_scientific,

	-- 14. Complex expression type inference
	IF(TRUE, 1 + 2, 'calculated') as expression_to_string,
	IF(TRUE, 'prefix_' || 'suffix', 123) as concat_to_int,
	IF(TRUE, LENGTH('test'), 'length_text') as function_to_string,

	-- 15. Boundary values and special values
	IF(TRUE, 0, 'zero') as zero_to_string,
	IF(TRUE, '', 123) as empty_string_to_int,
	IF(TRUE, '0', 0) as string_zero_to_int,
	IF(TRUE, NULL, 'default') as null_default,
	IF(TRUE, -123, 'negative') as negative_to_string,
	IF(TRUE, 999999999999999999, 'very_large') as very_large_int_to_string,

	-- 16. Spatial data types (if supported)
	IF(TRUE, POINT(1, 1), 'point_text') as spatial_to_string,
	IF(TRUE, 'point_text', POINT(1, 1)) as string_to_spatial;

-- Display detailed data type information for all test cases
SELECT
	COLUMN_NAME as test_case,
	DATA_TYPE,
	COLUMN_TYPE,
	CHARACTER_MAXIMUM_LENGTH,
	NUMERIC_PRECISION,
	NUMERIC_SCALE,
	IS_NULLABLE,
	CHARACTER_SET_NAME,
	COLLATION_NAME
FROM INFORMATION_SCHEMA.COLUMNS 
WHERE TABLE_SCHEMA = DATABASE() 
AND TABLE_NAME = 'type_analysis'
ORDER BY
	CASE
		WHEN COLUMN_NAME LIKE '%int%' THEN 1
		WHEN COLUMN_NAME LIKE '%float%' THEN 2
		WHEN COLUMN_NAME LIKE '%string%' THEN 3
		WHEN COLUMN_NAME LIKE '%date%' THEN 4
		WHEN COLUMN_NAME LIKE '%time%' THEN 5
		WHEN COLUMN_NAME LIKE '%bool%' THEN 6
		WHEN COLUMN_NAME LIKE '%null%' THEN 7
		WHEN COLUMN_NAME LIKE '%binary%' THEN 8
		WHEN COLUMN_NAME LIKE '%json%' THEN 9
		ELSE 10
	END,
	ORDINAL_POSITION;
set mysql.enable_mysql_information_schema TO off;
set mysql.use_mysql_bool TO false;

-- ifnull

select ifnull(null, null);
select ifnull(null, 'other');
select ifnull('something','other');
select ifnull(null,321);
select ifnull(123,'321');
select ifnull(123::int2,321::int2);
select ifnull(123::int4,321::int4);
select ifnull(123::int8,321::int8);
select ifnull(123::float4,321::float4);
select ifnull(123::float8,321::float8);
select ifnull('2001-01-01 01:01:01.3654'::date,'2012-08-02 15:57:54.6365'::date);

--ifnull(unknown, int)
SELECT ifnull('j', 1);
--ifnull(int, unknown)
SELECT ifnull(1, 'k');
SELECT ifnull(1, '6');
--ifnull(char, int)
SELECT ifnull('o'::char, 1);
--ifnull(int, char)
SELECT ifnull(1, 'o'::char);
--ifnull(text, int2)
SELECT ifnull('hello!'::text, '100'::int2);
SELECT ifnull('100'::int2, 'hello!'::text);
--ifnull(text, int4)
SELECT ifnull('hello!'::text, '100'::int4);
SELECT ifnull('100'::int4, 'hello!'::text);
--ifnull(text, int8)
SELECT ifnull('hello!'::text, '100'::int8);
SELECT ifnull('100'::int8, 'hello!'::text);
--ifnull(text, float4)
SELECT ifnull('hello!'::text, '100.001'::float4);
SELECT ifnull('100.001'::float4, 'hello!'::text);
--ifnull(text, float8)
SELECT ifnull('hello!'::text, '100.001'::float8);
SELECT ifnull('100.001'::float8, 'hello!'::text);
--ifnull(text, number)
SELECT ifnull('hello!'::text, '100.001'::numeric);
SELECT ifnull('100.001'::numeric, 'hello!'::text);

-- ifnull in where
CREATE TABLE test_table (id INT, column1 TEXT, column2 TEXT);
INSERT INTO test_table (id, column1, column2) VALUES
(1, 'value', 'other'),
(2, NULL, 'value'),
(3, 'other', 'other'),
(4, NULL, 'other');
SELECT * FROM test_table WHERE IFNULL(column1, column2) = 'value';
SELECT * FROM test_table WHERE IFNULL(column2, column1) = 'value';
DROP TABLE test_table;

-- replace

SELECT replace (123,3,'highgo') as test1; -- '12highgo'
SELECT replace (123.5,3,'highgo') as test1; -- '12highgo.5'
SELECT replace ('helloworld','world',1) as test1; -- 'hello1'
SELECT replace ('helloworld','world',1.55) as test1; -- 'hello1.55'
SELECT replace ('helloworld','world','1970-01-01 10:10:10') as test1; -- 'hello1970-01-01 10:10:10'
SELECT replace ('1970-01-01 10:10:10',7,'highgo') as test1; -- '19highgo0-01-01 10:10:10'

/* Begin - bug#z6050 */
select char_length(X'41'), char_length(X'4142'), char_length(x'410041'), char_length(x'414243'), char_length(0x41), char_length(0x4142);
/* End - bug#z6050 */