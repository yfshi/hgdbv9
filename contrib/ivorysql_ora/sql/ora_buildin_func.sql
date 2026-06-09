/*
 * Test compatible Oracle's build-in functions
 */
--
-- REGEXP_REPLACE
--

SELECT REGEXP_REPLACE('abc123def456', '\d+') AS result FROM dual; -- abcdef

SELECT REGEXP_REPLACE('abc123def456', '\d+', '#') AS result FROM dual; -- abc#def#

SELECT REGEXP_REPLACE(
         '2024-12-31',
         '(\d{4})-(\d{2})-(\d{2})',
         '\3/\2/\1'
       ) AS result
FROM dual; -- 31/12/2024

SELECT REGEXP_REPLACE(
         'a\b\c',
         '\\',
         '\\\\'
       ) AS result
FROM dual; -- a\\b\\c

SELECT REGEXP_REPLACE(
         'abc123def456',
         '\d+',
         '#',
         7
       ) AS result
FROM dual; -- abc123def#

SELECT REGEXP_REPLACE(
         'x1y2z3',
         '\d',
         '#',
         1,
         0
       ) AS result
FROM dual; -- x#y#z#

--occurrence = 1
SELECT REGEXP_REPLACE(
         'x1y2z3',
         '\d',
         '#',
         1,
         1
       ) AS result
FROM dual; -- x#y2z3

--ccurrence = 2
SELECT REGEXP_REPLACE(
         'x1y2z3',
         '\d',
         '#',
         1,
         2
       ) AS result
FROM dual; -- x1y#z3

SELECT REGEXP_REPLACE(
         'AbCabc',
         'abc',
         '#',
         1,
         0,
         'i'
       ) AS result
FROM dual; -- ##

SELECT REGEXP_REPLACE(
         'AbCabc',
         'abc',
         '#',
         1,
         0,
         'c'
       ) AS result
FROM dual; -- AbC#

SELECT REGEXP_REPLACE(
         'a\nb\nc',
         'a.*c',
         '#',
         1,
         0,
         'n'
       ) AS result
FROM dual; -- #

SELECT REGEXP_REPLACE(
         'a\nb\nc',
         'a.*c',
         '#'
       ) AS result
FROM dual; -- #

SELECT REGEXP_REPLACE(
         'a\nb\nc',
         '^.',
         '#',
         1,
         0,
         'm'
       ) AS result
FROM dual; -- #\nb\nc

SELECT REGEXP_REPLACE(
         'ABC\nabc',
         '^abc',
         '#',
         1,
         0,
         'im'
       ) AS result
FROM dual; -- #\nabc

SELECT REGEXP_REPLACE(
         'abc',
         RPAD('a', 512, 'a'),
         '#'
       )
FROM dual; -- abc

SELECT REGEXP_REPLACE(
         'a1b2',
         '\d',
         '#',
         1,
         5
       ) AS result
FROM dual; -- a1b2

SELECT REGEXP_REPLACE(
         'abc123def456ghi',
         '(\d+)',
         '[\1]',
         4,
         2,
         'c'
       ) AS result
FROM dual; -- abc123def[456]ghi

--
-- REGEXP_SUBSTR
--

SELECT REGEXP_SUBSTR('abc123def', '\d+') AS result FROM dual; -- 123

SELECT REGEXP_SUBSTR('abcdef', '\d+') AS result FROM dual; -- (null)

SELECT REGEXP_SUBSTR(
         'abc123def456',
         '\d+',
         7
       ) AS result
FROM dual; -- 456

SELECT REGEXP_SUBSTR(
         'abc123',
         '\d+',
         100
       ) AS result
FROM dual; -- (null)

SELECT REGEXP_SUBSTR(
         'x1y2z3',
         '\d',
         1,
         1
       ) AS result
FROM dual; -- 1

SELECT REGEXP_SUBSTR(
         'x1y2z3',
         '\d',
         1,
         2
       ) AS result
FROM dual; -- 2

SELECT REGEXP_SUBSTR(
         'x1y2',
         '\d',
         1,
         5
       ) AS result
FROM dual;  -- (null)

SELECT REGEXP_SUBSTR(
         'aaaa',
         'aa',
         1,
         2
       ) AS result
FROM dual;  -- aa

SELECT REGEXP_SUBSTR(
         'AbCabc',
         'abc',
         1,
         2,
         'i'
       ) AS result
FROM dual; -- abc

SELECT REGEXP_SUBSTR(
         'AbCabc',
         'abc',
         1,
         1,
         'c'
       ) AS result
FROM dual; -- abc

SELECT REGEXP_SUBSTR(
         'a'||CHR(10)||'b'||CHR(10)||'c',
         'a.*c',
         1,
         1,
         'n'
       ) AS result
FROM dual; 

SELECT REGEXP_SUBSTR(
         'a'||CHR(10)||'b'||CHR(10)||'c',
         'a.*c'
       ) AS result
FROM dual; -- (null) 

SELECT REGEXP_SUBSTR(
         'a'||CHR(10)||'b'||CHR(10)||'c',
         '^.',
         1,
         2,
         'm'
       ) AS result
FROM dual;   -- b

SELECT REGEXP_SUBSTR(
         'ABC'||CHR(10)||'abc',
         '^abc',
         1,
         2,
         'im'
       ) AS result
FROM dual;   -- abc

SELECT REGEXP_SUBSTR(
         '2024-12-31',
         '(\d{4})-(\d{2})-(\d{2})',
         1,
         1,
         'c',
         0
       ) AS result
FROM dual; -- 2024-12-31

SELECT REGEXP_SUBSTR(
         '2024-12-31',
         '(\d{4})-(\d{2})-(\d{2})',
         1,
         1,
         'c',
         1
       ) AS result
FROM dual;  -- 2024

SELECT REGEXP_SUBSTR(
         '2024-12-31',
         '(\d{4})-(\d{2})-(\d{2})',
         1,
         1,
         'c',
         2
       ) AS result
FROM dual; -- 12

SELECT REGEXP_SUBSTR(
         '2024-12-31',
         '(\d{4})-(\d{2})-(\d{2})',
         1,
         1,
         'c',
         9
       ) AS result
FROM dual;  -- (null)

SELECT REGEXP_SUBSTR(
         'aaaaaaaaaa',
         RPAD('a', 512, 'a')
       )
FROM dual; -- (null)

SELECT REGEXP_SUBSTR(
         '',
         '.+'
       ) AS result
FROM dual;  -- (null)

SELECT REGEXP_SUBSTR(
         'abc123def456ghi',
         '(\d+)',
         4,
         2,
         'c',
         1
       ) AS result
FROM dual;  -- 456

--
-- REGEXP_INSTR
--

SELECT REGEXP_INSTR('abc123def', '\d+') AS pos FROM dual; -- 4

SELECT REGEXP_INSTR('abcdef', '\d+') AS pos FROM dual; -- 0

SELECT REGEXP_INSTR(
         'abc123def456',
         '\d+',
         7
       ) AS pos
FROM dual; -- 10

SELECT REGEXP_INSTR(
         'abc123',
         '\d+',
         100
       ) AS pos
FROM dual;  -- 0

SELECT REGEXP_INSTR(
         'x1y2z3',
         '\d',
         1,
         1
       ) AS pos
FROM dual; -- 2

SELECT REGEXP_INSTR(
         'x1y2z3',
         '\d',
         1,
         2
       ) AS pos
FROM dual; -- 4

SELECT REGEXP_INSTR(
         'x1y2',
         '\d',
         1,
         5
       ) AS pos
FROM dual;  -- 0

SELECT REGEXP_INSTR(
         'aaaa',
         'aa',
         1,
         2
       ) AS pos
FROM dual;  -- 3

SELECT REGEXP_INSTR(
         'abc123def',
         '\d+',
         1,
         1,
         0
       ) AS pos
FROM dual;  -- 4

SELECT REGEXP_INSTR(
         'abc123def',
         '\d+',
         1,
         1,
         1
       ) AS pos
FROM dual;  -- 7

SELECT REGEXP_INSTR(
         'x1y2z3',
         '\d',
         1,
         2,
         1
       ) AS pos
FROM dual;  -- 5

SELECT REGEXP_INSTR(
         'AbCabc',
         'abc',
         1,
         2,
         0,
         'i'
       ) AS pos
FROM dual;  -- 4

SELECT REGEXP_INSTR(
         'AbCabc',
         'abc',
         1,
         1,
         0,
         'c'
       ) AS pos
FROM dual;  -- 4

SELECT REGEXP_INSTR(
         'a'||CHR(10)||'b'||CHR(10)||'c',
         'a.*c',
         1,
         1,
         0,
         'n'
       ) AS pos
FROM dual;  -- 1

SELECT REGEXP_INSTR(
         'a'||CHR(10)||'b'||CHR(10)||'c',
         'a.*c'
       ) AS pos
FROM dual;  -- 0

SELECT REGEXP_INSTR(
         'a'||CHR(10)||'b'||CHR(10)||'c',
         '^.',
         1,
         2,
         0,
         'm'
       ) AS pos
FROM dual; -- 3

SELECT REGEXP_INSTR(
         'ABC'||CHR(10)||'abc',
         '^abc',
         1,
         2,
         0,
         'im'
       ) AS pos
FROM dual;  -- 5

SELECT REGEXP_INSTR(
         '2024-12-31',
         '(\d{4})-(\d{2})-(\d{2})',
         1,
         1,
         0,
         'c',
         0
       ) AS pos
FROM dual;  -- 1

SELECT REGEXP_INSTR(
         '2024-12-31',
         '(\d{4})-(\d{2})-(\d{2})',
         1,
         1,
         0,
         'c',
         1
       ) AS pos
FROM dual;  -- 1

SELECT REGEXP_INSTR(
         '2024-12-31',
         '(\d{4})-(\d{2})-(\d{2})',
         1,
         1,
         0,
         'c',
         2
       ) AS pos
FROM dual;  -- 6

SELECT REGEXP_INSTR(
         '2024-12-31',
         '(\d{4})-(\d{2})-(\d{2})',
         1,
         1,
         0,
         'c',
         3
       ) AS pos
FROM dual; -- 9

SELECT REGEXP_INSTR(
         '2024-12-31',
         '(\d{4})-(\d{2})-(\d{2})',
         1,
         1,
         0,
         'c',
         9
       ) AS pos
FROM dual;  -- 0

SELECT REGEXP_INSTR(
         '2024-12-31',
         '(\d{4})',
         1,
         1,
         0,
         'c',
         NULL
       ) AS pos
FROM dual;  -- (null)

SELECT REGEXP_INSTR(
         'abc123def456ghi',
         '(\d+)',
         4,
         2,
         1,
         'c',
         1
       ) AS pos
FROM dual;  -- 13

--
-- REGEXP_LIKE
--

SELECT
  CASE
    WHEN REGEXP_LIKE('abc123def', '\d+') THEN 'Y'
    ELSE 'N'
  END AS result
FROM dual;  -- Y

SELECT
  CASE
    WHEN REGEXP_LIKE('abcdef', '\d+') THEN 'Y'
    ELSE 'N'
  END AS result
FROM dual;  -- N

SELECT
  CASE
    WHEN REGEXP_LIKE('AbC', 'abc', 'i') THEN 'Y'
    ELSE 'N'
  END AS result
FROM dual;  -- Y

SELECT
  CASE
    WHEN REGEXP_LIKE('AbC', 'abc', 'c') THEN 'Y'
    ELSE 'N'
  END AS result
FROM dual;  -- N

SELECT
  CASE
    WHEN REGEXP_LIKE('AbC', 'abc', 'ic') THEN 'Y'
    ELSE 'N'
  END AS result
FROM dual;  -- N

SELECT
  CASE
    WHEN REGEXP_LIKE(
           'a'||CHR(10)||'b',
           'a.*b'
         )
    THEN 'Y'
    ELSE 'N'
  END AS result
FROM dual;  -- N

SELECT
  CASE
    WHEN REGEXP_LIKE(
           'a'||CHR(10)||'b',
           'a.*b',
           'n'
         )
    THEN 'Y'
    ELSE 'N'
  END AS result
FROM dual;  -- Y 

SELECT
  CASE
    WHEN REGEXP_LIKE(
           'a'||CHR(10)||'b',
           '^b'
         )
    THEN 'Y'
    ELSE 'N'
  END AS result
FROM dual;  -- N

SELECT
  CASE
    WHEN REGEXP_LIKE(
           'a'||CHR(10)||'b',
           '^b',
           'm'
         )
    THEN 'Y'
    ELSE 'N'
  END AS result
FROM dual;  -- Y

SELECT
  CASE
    WHEN REGEXP_LIKE(
           'a'||CHR(10)||'b'||CHR(10)||'c',
           '^a.*c$',
           'mn'
         )
    THEN 'Y'
    ELSE 'N'
  END AS result
FROM dual;   -- Y 

SELECT
  CASE
    WHEN REGEXP_LIKE(
           'abc123',
           'a b c \d+',
           'x'
         )
    THEN 'Y'
    ELSE 'N'
  END AS result
FROM dual;  -- Y

SELECT
  CASE
    WHEN REGEXP_LIKE(
           'abc123',
           'a b c \d+'
         )
    THEN 'Y'
    ELSE 'N'
  END AS result
FROM dual;  -- N 

SELECT
  CASE
    WHEN REGEXP_LIKE(
           'ABC'||CHR(10)||'abc',
           '^abc',
           'im'
         )
    THEN 'Y'
    ELSE 'N'
  END AS result
FROM dual;  -- Y

SELECT
  CASE
    WHEN REGEXP_LIKE(
           'ABC',
           'abc',
           'imc'
         )
    THEN 'Y'
    ELSE 'N'
  END AS result
FROM dual;  -- N

SELECT REGEXP_LIKE(
         'abc',
         RPAD('a', 513, 'a')
       )
FROM dual;

SELECT REGEXP_LIKE(
         'abc',
         'a',
         'z'
       )
FROM dual;

SELECT 1 FROM dual WHERE REGEXP_LIKE(
         'abc',
         'a',
         'z'
       );

SELECT
  CASE
    WHEN REGEXP_LIKE(
           'user.name@example.com',
           '^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$'
         )
    THEN 'VALID'
    ELSE 'INVALID'
  END AS email_check
FROM dual;  -- VALID

SELECT
  CASE
    WHEN REGEXP_LIKE(
           '001234',
           '^\d+$'
         )
    THEN 'NUMERIC'
    ELSE 'NON-NUMERIC'
  END AS result
FROM dual;  -- NUMERIC

--
-- REGEXP_COUNT
--

SELECT REGEXP_COUNT('abc123def123', '\d+') AS cnt FROM dual;  -- 2

SELECT REGEXP_COUNT('abcdef', '\d+') AS cnt FROM dual;  -- 0

SELECT REGEXP_COUNT(
         'abc123def456ghi789',
         '\d+',
         7
       ) AS cnt
FROM dual;  -- 2

SELECT REGEXP_COUNT(
         'abc123',
         '\d+',
         100
       ) AS cnt
FROM dual;  -- 0

SELECT REGEXP_COUNT(
         'aaaa',
         'aa'
       ) AS cnt
FROM dual;  -- 2

SELECT REGEXP_COUNT(
         '1234512345',
         '(123(45))'
       ) AS cnt
FROM dual;  -- 2

SELECT REGEXP_COUNT(
         'AbCabcABC',
         'abc',
         1,
         'i'
       ) AS cnt
FROM dual;  -- 3

SELECT REGEXP_COUNT(
         'AbCabcABC',
         'abc',
         1,
         'c'
       ) AS cnt
FROM dual;  -- 1

SELECT REGEXP_COUNT(
         'AbCabcABC',
         'abc',
         1,
         'ic'
       ) AS cnt
FROM dual;   -- 1

SELECT REGEXP_COUNT(
         'a'||CHR(10)||'b'||CHR(10)||'c',
         'a.*c',
         1,
         'n'
       ) AS cnt
FROM dual;  -- 1

SELECT REGEXP_COUNT(
         'a'||CHR(10)||'b'||CHR(10)||'c',
         'a.*c'
       ) AS cnt
FROM dual;  -- 0

SELECT REGEXP_COUNT(
         'a'||CHR(10)||'b'||CHR(10)||'c',
         '^.',
         1,
         'm'
       ) AS cnt
FROM dual;  -- 3

SELECT REGEXP_COUNT(
         'abc123abc',
         'a b c',
         1,
         'x'
       ) AS cnt
FROM dual;  -- 2

SELECT REGEXP_COUNT(
         'ABC'||CHR(10)||'abc',
         '^abc',
         1,
         'im'
       ) AS cnt
FROM dual;   -- 2

SELECT REGEXP_COUNT(
         'abc',
         'a',
         1,
         'z'
       )
FROM dual;

SELECT REGEXP_COUNT(
         'one two  three   four',
         '\S+'
       ) AS word_count
FROM dual;  -- 4

SELECT REGEXP_COUNT(
         'a@x.com;b@y.com;c@z.com',
         '[^;]+@[^;]+'
       ) AS cnt
FROM dual;  -- 3

--
-- REPLACE
--

SELECT REPLACE('abcabc', 'a', 'x') AS result FROM dual;  -- xbcxbc

SELECT REPLACE('2024-12-31', '-', '/') AS result FROM dual; -- 2024/12/31

SELECT REPLACE('abcdef', 'xyz', '123') AS result FROM dual; -- abcdef

SELECT REPLACE('abc123abc', 'abc') AS result FROM dual;  -- 123

SELECT REPLACE('abc123abc', 'abc', NULL) AS result FROM dual;  -- 123

SELECT REPLACE('1,234,567', ',', '') AS result FROM dual; -- 1234567

SELECT REPLACE('abcdef', NULL, 'x') AS result FROM dual;  -- abcdef

SELECT REPLACE('abcdef', NULL, NULL) AS result FROM dual; -- abcdef

SELECT REPLACE(NULL, 'a', 'b') AS result FROM dual;  -- (null)

SELECT REPLACE('AbcABCabc', 'abc', 'x') AS result FROM dual;  -- AbcABCx

SELECT REPLACE('aaaa', 'aa', 'b') AS result FROM dual;  -- bb

SELECT REPLACE('aaa', 'a', 'aa') AS result FROM dual; -- aaaaaa

SELECT REPLACE('abc123abc', 'abc', '') AS result FROM dual;  -- 123

SELECT REPLACE('abcabc', 'ab', 'X') AS replace_result FROM dual;  -- XcXc

SELECT TRANSLATE('abcabc', 'ab', 'X') AS translate_result FROM dual; -- XcXc

SELECT REPLACE(
         REPLACE('(010)-1234-5678', '-', ''),
         '(', ''
       ) AS cleaned
FROM dual;  -- 010)12345678

SELECT REPLACE(
         REPLACE('<b>hello</b>', '<b>', ''),
         '</b>', ''
       ) AS result
FROM dual;  -- hello
 
--
-- TRIM 
--

SELECT
  TRIM('  Oracle 21c  ') AS result
FROM dual; -- Oracle 21c

SELECT
  TRIM('  Oracle   21c  ') AS result
FROM dual; -- Oracle   21c

SELECT
  TRIM(LEADING FROM '   Oracle  ') AS result
FROM dual; -- Oracle  

SELECT
  TRIM(TRAILING FROM '   Oracle  ') AS result
FROM dual; --    Oracle

SELECT
  TRIM(BOTH FROM '   Oracle  ') AS result
FROM dual; -- Oracle

SELECT
  TRIM(LEADING '0' FROM '000123') AS result
FROM dual; -- 123

SELECT
  TRIM(TRAILING 'x' FROM 'dataxxx') AS result
FROM dual; -- data

SELECT
  TRIM(BOTH '*' FROM '***core***') AS result
FROM dual;  -- core

SELECT
  TRIM('#' FROM '##ABC##') AS result
FROM dual;  -- ABC

SELECT
  TRIM('#ABC#') AS result
FROM dual; -- #ABC#

SELECT
  TRIM(LEADING 'ab' FROM 'ababDATA') AS result
FROM dual;  -- ORA-30001: trim set should have only one character

SELECT
  TRIM(LEADING 'a' FROM 'aaabDATA') AS result
FROM dual; -- bDATA

SELECT
  TRIM(LEADING 'ab' FROM 'abaXDATA') AS result
FROM dual;

SELECT
  LTRIM('abaXDATA', 'ab') AS result
FROM dual; -- XDATA

SELECT
  TRIM(NULL) AS result
FROM dual;  -- (null)

SELECT
  TRIM(LEADING NULL FROM 'ABC') AS result
FROM dual;  -- (null)

SELECT
  TRIM(LEADING '0' FROM 00123) AS result
FROM dual;  -- 123

SELECT
  '"' || TRIM(CAST(' A ' AS CHAR(5))) || '"' AS trimmed_value,
  LENGTH(TRIM(CAST(' A ' AS CHAR(5)))) AS trimmed_length
FROM dual; -- "A"	1

SELECT
  TRIM(LEADING 'X' FROM TO_CLOB('XXXCLOB')) AS result
FROM dual;  -- CLOB

SELECT
  TRIM('AAA' FROM 'AAA') AS result
FROM dual;

SELECT TRIM(LEADING 'A' FROM 'AAA') AS result FROM dual; -- (null)

SELECT TRIM(TRAILING 'A' FROM 'AAA') AS result FROM dual; -- (null)

SELECT TRIM(BOTH 'A' FROM 'AAA') AS result FROM dual; -- (null)

SELECT
  TRIM('Z' FROM 'ABC') AS result
FROM dual;  -- ABC

/* Begin - ReqID: OA-2348 */
SELECT sys.empty_clob IS NULL AS result FROM DUAL;
SELECT sys.empty_blob IS NULL AS result FROM DUAL;

--
-- test TRANSLATE function, TRANSLATE(expr, from_string, to_string)
--
----test expr
SELECT TRANSLATE('Hello 123 World', '0123456789', '') FROM DUAL;
SELECT TRANSLATE('Hello 123 World', '0123456789', NULL) FROM DUAL;
SELECT TRANSLATE('Hello 123 World', '0123456789', ' ') FROM DUAL;
SELECT TRANSLATE('Hello 123 World', 'z0123456789', 'z') FROM DUAL;
SELECT TRANSLATE('abc123', 'abc123', 'xyzabc') FROM DUAL;
SELECT TRANSLATE('Hello 123 World', 'HlHl', 'hL') FROM DUAL;
SELECT TRANSLATE('SQL*Plus User''s Guide', ' */''', '___') FROM DUAL;
SELECT TRANSLATE(12345, '12', 'hL') FROM DUAL;
SELECT TRANSLATE('12345'::number, '12', 'hL') FROM DUAL;
SELECT TRANSLATE(to_date('2026-6-2', 'YYYY-MM-DD'), '26', '62') FROM DUAL;
SELECT TRANSLATE(to_char(123456789), '123', 'AB') FROM DUAL;
SELECT TRANSLATE(timestamp'1949-1-1 12:34:56', '19', '26') FROM DUAL;

CREATE TABLE t1(id int, val number(38,10));
INSERT INTO t1 VALUES (TRANSLATE(123456789::int, '123', '321'), TRANSLATE('16397.038491'::number(38,10), '391', '139'));
SELECT * FROM t1;
DROP TABLE t1;

CREATE TABLE t1(id number);
INSERT INTO t1 VALUES (4.87);
UPDATE t1 SET id = TRANSLATE(id, '478',  '874');
SELECT * FROM t1;
DROP TABLE t1;

CREATE TABLE t1(id binary_float);
INSERT INTO t1 VALUES ('1.2345678901234e-20');
UPDATE t1 SET id = TRANSLATE(id, '123', '321');
SELECT * FROM t1;
DROP TABLE t1;


CREATE TABLE t1(id binary_double);
INSERT INTO t1 VALUES ('1.2345678901234e-20');
UPDATE t1 SET id = TRANSLATE(id, '123', '321');
SELECT * FROM t1;
DROP TABLE t1;

CREATE TABLE t1(name varchar2(20));
INSERT INTO t1 VALUES('Hello 123 World');
UPDATE t1 SET name = TRANSLATE(name, 'z0123456789', 'z');
SELECT * FROM t1;
DROP TABLE t1;

CREATE TABLE t1(name varchar2(20 byte));
INSERT INTO t1 VALUES('Hello 123 World');
UPDATE t1 SET name = TRANSLATE(name, 'z0123456789', 'z');
SELECT * FROM t1;
DROP TABLE t1;

CREATE TABLE t1(name char(32 byte));
INSERT INTO t1 VALUES('Hello 123 World');
UPDATE t1 SET name = TRANSLATE(name, 'z0123456789', 'z');
SELECT * FROM t1;
DROP TABLE t1;

CREATE TABLE t1(name char(32 char));
INSERT INTO t1 VALUES('Hello 123 World');
UPDATE t1 SET name = TRANSLATE(name, 'z0123456789', 'z');
SELECT * FROM t1;
DROP TABLE t1;

CREATE TABLE t1(name nchar(20));
INSERT INTO t1 VALUES('Hello 123 World');
UPDATE t1 SET name = TRANSLATE(name, 'z0123456789', 'z');
SELECT * FROM t1;
DROP TABLE t1;

CREATE TABLE t1(name nvarchar2(20));
INSERT INTO t1 VALUES('Hello 123 World');
UPDATE t1 SET name = TRANSLATE(name, 'z0123456789', 'z');
SELECT * FROM t1;
DROP TABLE t1;

CREATE TABLE t1(name CLOB);
INSERT INTO t1 VALUES('Hello 123 World');
UPDATE t1 SET name = TRANSLATE(name, 'z0123456789', 'z');
SELECT * FROM t1;
DROP TABLE t1;

CREATE TABLE t1(name NCLOB);
INSERT INTO t1 VALUES('Hello 123 World');
UPDATE t1 SET name = TRANSLATE(name, 'z0123456789', 'z');
SELECT * FROM t1;
DROP TABLE t1;

CREATE TABLE t1(id interval year(4) to month);
INSERT INTO t1 VALUES(interval'29-10' year to month);
UPDATE t1 SET id = TRANSLATE(id, '29', '13');
SELECT * FROM t1;
DROP TABLE t1;

CREATE TABLE t1(id interval day(4) to second(6));
INSERT INTO t1 VALUES(interval '1 11:11:11' day(4) to second(6));
UPDATE t1 SET id = TRANSLATE(id, '11', '12');
SELECT * FROM t1;
DROP TABLE t1;

CREATE TABLE t1(id DATE);
INSERT INTO t1 VALUES(TRANSLATE(TO_DATE('2026-6-2', 'YYYY-MM-DD'), '16', '62'));
SELECT * FROM t1;
DROP TABLE t1;

CREATE TABLE t1(id TIMESTAMP);
INSERT INTO t1 VALUES(TRANSLATE(TIMESTAMP'2015-9-12 13:20:45.1201111', '2', '1'));
SELECT * FROM t1;
DROP TABLE t1;

CREATE TABLE t1 (data LONG);
INSERT INTO t1 VALUES ('HELLO-LONG-TEXT');
UPDATE t1 SET data = TRANSLATE(data, 'LONG', 'long');
SELECT * FROM t1;
DROP TABLE t1;

----test from_string and to_string
SELECT
    TRANSLATE('HELLO-WORLD', 'EO', 'eo') AS case1_char_expr,
    TRANSLATE(CAST('HELLO-WORLD' AS CHAR(30)), 'EO', 'eo') AS case1_char_param1,
    TRANSLATE('HELLO-WORLD', CAST('EO' AS CHAR(10)), 'eo') AS case1_char_param2,
    TRANSLATE('HELLO-WORLD', 'EO', CAST('eo' AS CHAR(10))) AS case1_char_param3
FROM DUAL;

SELECT
    TRANSLATE('HELLO-WORLD', 'EO', 'eo') AS case2_varchar2_expr,
    TRANSLATE(CAST('HELLO-WORLD' AS VARCHAR2(30)), 'EO', 'eo') AS case2_varchar2_param1,
    TRANSLATE('HELLO-WORLD', CAST('EO' AS VARCHAR2(10)), 'eo') AS case2_varchar2_param2,
    TRANSLATE('HELLO-WORLD', 'EO', CAST('eo' AS VARCHAR2(10))) AS case2_varchar2_param3
FROM DUAL;

SELECT
    TRANSLATE('HELLO-WORLD', 'EO', 'eo') AS case3_nchar_expr,
    TRANSLATE(CAST('HELLO-WORLD' AS NCHAR(30)), 'EO', 'eo') AS case3_nchar_param1,
    TRANSLATE('HELLO-WORLD', CAST('EO' AS NCHAR(10)), 'eo') AS case3_nchar_param2,
    TRANSLATE('HELLO-WORLD', 'EO', CAST('eo' AS NCHAR(10))) AS case3_nchar_param3
FROM DUAL;

SELECT
    TRANSLATE('HELLO-WORLD', 'EO', 'eo') AS case4_nvarchar2_expr,
    TRANSLATE(CAST('HELLO-WORLD' AS NVARCHAR2(30)), 'EO', 'eo') AS case4_nvarchar2_param1,
    TRANSLATE('HELLO-WORLD', CAST('EO' AS NVARCHAR2(10)), 'eo') AS case4_nvarchar2_param2,
    TRANSLATE('HELLO-WORLD', 'EO', CAST('eo' AS NVARCHAR2(10))) AS case4_nvarchar2_param3
FROM DUAL;

SELECT
    TRANSLATE(123456789, '456', 'ABC') AS case5_number_expr,
    TRANSLATE(123456789, CAST(456 AS NUMBER), 'ABC') AS case5_number_param2
FROM DUAL;

SELECT
    TRANSLATE(1.23e1F, '1e', 'Xy') AS case6_bfloat_expr,
    TRANSLATE(1.23e1F, CAST(1.23e1F AS BINARY_FLOAT), 'Xy') AS case6_bfloat_param2
FROM DUAL;

SELECT
    TRANSLATE(4.56e2F, '4e', 'Zz') AS case7_bdouble_expr,
    TRANSLATE(4.56e2F, CAST(4.56e2F AS BINARY_DOUBLE), 'Zz') AS case7_bdouble_param2
FROM DUAL;

SELECT
    TRANSLATE('2026-6-2-2026-6-2', TO_DATE('2026-6-2', 'YYYY-MM-DD'), TO_DATE('1012-2-6', 'YYYY-MM-DD')) AS case8_date_param2
FROM DUAL;

SELECT
    TRANSLATE('2015-9-20 13:20:45', TO_TIMESTAMP('2015-9-20 13:20:45', 'YYYY-MM-DD HH24:MI:SS.FF6'),
	TO_TIMESTAMP('1025-9-10', 'YYYY-MM-DD HH24:MI:SS.FF6')) AS case9_timestamp_param2
FROM DUAL;

SELECT
	 TRANSLATE('1 01:02:03, 1 01:02:03', INTERVAL '1 01:02:03' DAY TO SECOND, INTERVAL '9 23:22:23' DAY TO SECOND)
FROM DUAL;

SELECT
    TRANSLATE(TO_CLOB('HELLO-CLOB-WORLD'), 'CLO', 'clo') AS case11_clob_expr,
    TRANSLATE(TO_CLOB('HELLO-CLOB-WORLD'), CAST('CLO' AS CLOB), 'clo') AS case11_clob_param2,
    TRANSLATE(TO_CLOB('HELLO-CLOB-WORLD'), 'CLO', CAST('clo' AS CLOB)) AS case11_clob_param3
FROM DUAL;

SELECT
    TRANSLATE(CAST('HELLO-NCLOB-WORLD' AS NCLOB), 'NCLO', 'nclo') AS case12_nclob_expr,
    TRANSLATE(CAST('HELLO-NCLOB-WORLD' AS NCLOB), CAST('NCLO' AS NCLOB), 'nclo') AS case12_nclob_param2,
    TRANSLATE(CAST('HELLO-NCLOB-WORLD' AS NCLOB), 'NCLO', CAST('nclo' AS NCLOB)) AS case12_nclob_param3
FROM DUAL;

SELECT
    TRANSLATE(CAST('HELLO-LONG-WORLD' AS long), 'LONG', 'long') AS case13_long_expr,
    TRANSLATE(CAST('HELLO-LONG-WORLD' AS long), CAST('LONG' AS long), 'long') AS case13_long_param2,
    TRANSLATE(CAST('HELLO-LONG-WORLD' AS long), 'LONG', CAST('long' AS long)) AS case13_long_param3
FROM DUAL;
/* End - ReqID: OA-2348 */

/* Begin - OA-REQ-2348 */
select regexp_substr('barbcr', '(b[^b]+)') from dual;	-- bar
select regexp_substr('barbcr', '(b[^b]+)', 1.9) from dual;	-- bar
select regexp_substr('barbcr', '(b[^b]+)', 2.9, 1.9) from dual;	-- bcr
select regexp_substr('barbcr', '(b[^b]+)', 2.9, 1.9, 'i') from dual;	-- bcr
select regexp_substr('barbcr', '(b[^b]+)', 2.9, 1.9, 'i', 1.9) from dual;	-- bcr
/* End - OA-REQ-2348 */

/* Begin - OA-REQ-2451 */

CREATE TABLE TEST_REPLACE(A NUMBER);

SELECT REPLACE(A,1000,2000) + 0 AS AAA FROM TEST_REPLACE;

CREATE OR REPLACE FUNCTION FN_REGEXP_SUBSTR
RETURN VARCHAR2
AS
   V_RESULT VARCHAR2(100);
BEGIN
   SELECT 'ABB' INTO V_RESULT FROM DUAL;
   RETURN V_RESULT;
END;
/

SELECT REGEXP_SUBSTR(FN_REGEXP_SUBSTR(),'[^,]+',1, ROWNUM) FROM DUAL CONNECT BY ROWNUM <= 5;

--cleanup
DROP table TEST_REPLACE;
DROP FUNCTION FN_REGEXP_SUBSTR;
/* Begin - OA-REQ-2451 */
