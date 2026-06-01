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
/* End - ReqID: OA-2348 */
