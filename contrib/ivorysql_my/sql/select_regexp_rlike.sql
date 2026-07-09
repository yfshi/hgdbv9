
-- ============================================================
-- 1. Main test table
-- ============================================================
set sql_mode = 'REAL_AS_FLOAT,PIPES_AS_CONCAT,ANSI_QUOTES,ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION,NO_ZERO_IN_DATE';
show sql_mode;
CREATE TABLE regexp_test_text (
    id          INT PRIMARY KEY,
    txt         TEXT  collate utf8_general_ci,
    txt2        TEXT  collate utf8_general_ci,
    note        TEXT collate utf8_general_ci
);

INSERT INTO regexp_test_text (id, txt, txt2, note) VALUES
    (1,  'abc',               'ABC',              'simple lower/upper'),
    (2,  'abcd',              'xyz',              'prefix match'),
    (3,  '123',               '00123',            'digits only'),
    (4,  'abc123',            '123abc',           'alnum mixed'),
    (5,  'hello world',       'hello_world',      'space and underscore'),
    (6,  'foo.bar',           'fooXbar',          'dot special char'),
    (7,  'a+b',               'ab',               'plus char'),
    (8,  'a*b',               'aaab',             'star char and repeated a'),
    (9,  'a?b',               'axb',              'question mark char'),
    (10, '[abc]',             'a',                'bracket chars'),
    (11, '(test)',            'test',             'parentheses'),
    (12, '^anchor',           'anchor',           'caret char'),
    (13, 'end$',              'the end',          'dollar char'),
    (14, '',                  '',                 'empty string'),
    (15, '   ',               'space',            'spaces only'),
    (16, 'line1\\nline2',   'line1\\tline2',  'newline and tab'),
    (17, '中文',              '中文123',          'chinese text'),
    (18, 'ÄÖÜ',               'äöü',              'non-ascii letters'),
    (19, 'user@example.com',  'invalid@email',    'email-like text'),
    (20, '2024-01-01',        '2024/01/01',       'date-like text'),
    (21, 'aaa',               'bbb',              'for quantifier test'),
    (22, 'abcabc',            'cab',              'repeated substring'),
    (23, '0',                 '00',               'zero values'),
    (24, 'NULL',              NULL,               'literal NULL string vs SQL NULL');

-- Show base data
SELECT 'BASE DATA' AS section;
SELECT * FROM regexp_test_text ORDER BY id;

-- ============================================================
-- 2. Basic REGEXP tests
-- ============================================================
SELECT '2.1 REGEXP basic exact match' AS section;
SELECT id, txt FROM regexp_test_text WHERE txt REGEXP '^abc$' ORDER BY id;

SELECT '2.2 REGEXP prefix match' AS section;
SELECT id, txt FROM regexp_test_text WHERE txt REGEXP '^abc' ORDER BY id;

SELECT '2.3 REGEXP suffix match' AS section;
SELECT id, txt FROM regexp_test_text WHERE txt REGEXP '123$' ORDER BY id;

SELECT '2.4 REGEXP contains digits' AS section;
SELECT id, txt FROM regexp_test_text WHERE txt REGEXP '[0-9]+' ORDER BY id;

SELECT '2.5 REGEXP letters only' AS section;
SELECT id, txt FROM regexp_test_text WHERE txt REGEXP '^[A-Za-z]+$' ORDER BY id;

SELECT '2.6 REGEXP empty string' AS section;
SELECT id, txt FROM regexp_test_text WHERE txt REGEXP '^$' ORDER BY id;

SELECT '2.7 REGEXP spaces only' AS section;
SELECT id, txt FROM regexp_test_text WHERE txt REGEXP '^ +$' ORDER BY id;

SELECT '2.8 REGEXP email-like pattern' AS section;
SELECT id, txt FROM regexp_test_text WHERE txt REGEXP '^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+$' ORDER BY id;

SELECT '2.9 REGEXP date format yyyy-mm-dd' AS section;
SELECT id, txt FROM regexp_test_text WHERE txt REGEXP '^[0-9]{4}-[0-9]{2}-[0-9]{2}$' ORDER BY id;

SELECT '2.10 REGEXP chinese characters' AS section;
SELECT id, txt FROM regexp_test_text WHERE txt REGEXP '中文' ORDER BY id;

-- ============================================================
-- 3. Basic RLIKE tests
-- ============================================================
SELECT '3.1 RLIKE exact match' AS section;
SELECT id, txt FROM regexp_test_text WHERE txt RLIKE '^abc$' ORDER BY id;

SELECT '3.2 RLIKE contains abc' AS section;
SELECT id, txt FROM regexp_test_text WHERE txt RLIKE 'abc' ORDER BY id;

SELECT '3.3 RLIKE digits only' AS section;
SELECT id, txt FROM regexp_test_text WHERE txt RLIKE '^[0-9]+$' ORDER BY id;

SELECT '3.4 RLIKE special dot meta-character' AS section;
SELECT id, txt2 FROM regexp_test_text WHERE txt2 RLIKE '^foo.bar$' ORDER BY id;

SELECT '3.5 RLIKE repetition quantifier' AS section;
SELECT id, txt FROM regexp_test_text WHERE txt RLIKE '^a{3}$' ORDER BY id;

SELECT '3.6 RLIKE repeated group-like content' AS section;
SELECT id, txt FROM regexp_test_text WHERE txt RLIKE '^(abc){2}$' ORDER BY id;

-- ============================================================
-- 4. NOT REGEXP tests
-- ============================================================
SELECT '4.1 NOT REGEXP non-digit rows' AS section;
SELECT id, txt FROM regexp_test_text WHERE txt NOT REGEXP '^[0-9]+$' ORDER BY id;

SELECT '4.2 NOT REGEXP rows not starting with abc' AS section;
SELECT id, txt FROM regexp_test_text WHERE txt NOT REGEXP '^abc' ORDER BY id;

SELECT '4.3 NOT REGEXP rows not matching empty string' AS section;
SELECT id, txt FROM regexp_test_text WHERE txt NOT REGEXP '^$' ORDER BY id;


-- ============================================================
-- 6. Operator precedence / expression usage tests
-- ============================================================
SELECT '6.1 REGEXP used in SELECT target list' AS section;
SELECT id, txt, txt REGEXP '^abc' AS regexp_result FROM regexp_test_text ORDER BY id;

SELECT '6.2 RLIKE used in SELECT target list' AS section;
SELECT id, txt, txt RLIKE '123$' AS rlike_result FROM regexp_test_text ORDER BY id;

SELECT '6.3 NOT REGEXP used in SELECT target list' AS section;
SELECT id, txt, txt NOT REGEXP '^[0-9]+$' AS not_regexp_result FROM regexp_test_text ORDER BY id;

SELECT '6.4 Combined with AND/OR' AS section;
SELECT id, txt
FROM regexp_test_text
WHERE txt REGEXP 'abc' AND txt NOT REGEXP '^abc$'
ORDER BY id;

SELECT '6.5 Parenthesized boolean logic' AS section;
SELECT id, txt
FROM regexp_test_text
WHERE (txt REGEXP 'abc' OR txt RLIKE '123')
  AND txt NOT REGEXP '^$'
ORDER BY id;

-- ============================================================
-- 7. Column-to-column coverage using multiple source columns
-- ============================================================
SELECT '7.1 REGEXP on second column' AS section;
SELECT id, txt2 FROM regexp_test_text WHERE txt2 REGEXP '^[A-Z]+$' ORDER BY id;

SELECT '7.2 RLIKE on second column' AS section;
SELECT id, txt2 FROM regexp_test_text WHERE txt2 RLIKE 'abc$' ORDER BY id;

SELECT '7.3 NOT REGEXP on second column' AS section;
SELECT id, txt2 FROM regexp_test_text WHERE txt2 NOT REGEXP '^[0-9]+$' ORDER BY id;

-- ============================================================
-- 8. Edge cases with escaped/special characters
-- ============================================================

CREATE TABLE regexp_test_edge (
    id  INT PRIMARY KEY,
    txt TEXT
);

INSERT INTO regexp_test_edge (id, txt) VALUES
    (1, 'foo.bar'),
    (2, 'fooXbar'),
    (3, 'a+b'),
    (4, 'ab'),
    (5, 'a*b'),
    (6, 'aaab'),
    (7, '[abc]'),
    (8, 'abc'),
    (9, '(test)'),
    (10, 'test'),
    (11, '^start'),
    (12, 'start'),
    (13, 'end$'),
    (14, 'the end');

SELECT '8.1 Match literal dot using escape' AS section;
SELECT id, txt FROM regexp_test_edge WHERE txt REGEXP '^foo\\.bar$' ORDER BY id;

SELECT '8.2 Match any char with dot' AS section;
SELECT id, txt FROM regexp_test_edge WHERE txt REGEXP '^foo.bar$' ORDER BY id;

SELECT '8.3 Match literal plus using escape' AS section;
SELECT id, txt FROM regexp_test_edge WHERE txt REGEXP '^a\\+b$' ORDER BY id;

SELECT '8.4 Match literal star using escape' AS section;
SELECT id, txt FROM regexp_test_edge WHERE txt RLIKE '^a\\*b$' ORDER BY id;

SELECT '8.5 Match literal brackets using escape' AS section;
SELECT id, txt FROM regexp_test_edge WHERE txt REGEXP '^\\[abc\\]$' ORDER BY id;

SELECT '8.6 Match literal parentheses using escape' AS section;
SELECT id, txt FROM regexp_test_edge WHERE txt RLIKE '^\\(test\\)$' ORDER BY id;

SELECT '8.7 Match literal caret using escape' AS section;
SELECT id, txt FROM regexp_test_edge WHERE txt REGEXP '^\\^start$' ORDER BY id;

SELECT '8.8 Match literal dollar using escape' AS section;
SELECT id, txt FROM regexp_test_edge WHERE txt NOT REGEXP '^the end$' ORDER BY id;

set sql_mode = 'NO_BACKSLASH_ESCAPES,REAL_AS_FLOAT,PIPES_AS_CONCAT,ANSI_QUOTES,ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION,NO_ZERO_IN_DATE';
SELECT '8.9 NO_BACKSLASH_ESCAPES on' AS section;
SELECT id, txt FROM regexp_test_edge WHERE txt RLIKE '^\\(test\\)$' ORDER BY id;

set sql_mode = 'REAL_AS_FLOAT,PIPES_AS_CONCAT,ANSI_QUOTES,ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION,NO_ZERO_IN_DATE';
SELECT '8.10 NO_BACKSLASH_ESCAPES ff' AS section;
SELECT id, txt FROM regexp_test_edge WHERE txt RLIKE '^\\(test\\)$' ORDER BY id;

-- ============================================================
-- 9. NULL behavior tests
-- ============================================================
CREATE TABLE regexp_test_null (
    id      INT PRIMARY KEY,
    txt     TEXT,
    pattern TEXT
);

INSERT INTO regexp_test_null (id, txt, pattern) VALUES
    (1, 'abc',  'abc'),
    (2, '123',  '^[0-9]+$'),
    (3, NULL,   'abc'),
    (4, 'abc',  NULL),
    (5, NULL,   NULL),
    (6, '',     '^$');

SELECT '9.1 REGEXP with nullable input/pattern' AS section;
SELECT id, txt, pattern, txt REGEXP pattern AS regexp_result
FROM regexp_test_null
ORDER BY id;

SELECT '9.2 RLIKE with nullable input/pattern' AS section;
SELECT id, txt, pattern, txt RLIKE pattern AS rlike_result
FROM regexp_test_null
ORDER BY id;

SELECT '9.3 NOT REGEXP with nullable input/pattern' AS section;
SELECT id, txt, pattern, txt NOT REGEXP pattern AS not_regexp_result
FROM regexp_test_null
ORDER BY id;

SELECT '9.4 WHERE filter with REGEXP and NULLs' AS section;
SELECT id, txt, pattern
FROM regexp_test_null
WHERE txt REGEXP pattern
ORDER BY id;

SELECT '9.5 WHERE filter with NOT REGEXP and NULLs' AS section;
SELECT id, txt, pattern
FROM regexp_test_null
WHERE txt NOT REGEXP pattern
ORDER BY id;

-- ============================================================
-- 10. Parameterized / expression-side pattern tests
-- ============================================================
*********************************************************
SELECT '10.1 Pattern from expression concatenation' AS section;
SELECT id, txt
FROM regexp_test_text
WHERE txt REGEXP ('^' || 'abc' || '$')
ORDER BY id;


SELECT '10.2 Pattern from column value' AS section;
SELECT n.id, n.txt, n.pattern
FROM regexp_test_null n
WHERE n.txt IS NOT NULL
  AND n.pattern IS NOT NULL
  AND n.txt RLIKE n.pattern
ORDER BY n.id;

-- ============================================================
-- 11. Usage in CASE / ORDER BY / subquery
-- ============================================================
SELECT '11.1 CASE expression with REGEXP/RLIKE/NOT REGEXP' AS section;
SELECT id,
       txt,
       CASE
           WHEN txt REGEXP '^[0-9]+$' THEN 'digits'
           WHEN txt RLIKE 'abc' THEN 'contains_abc'
           WHEN txt NOT REGEXP '^$' THEN 'other_nonempty'
           ELSE 'empty_or_null'
       END AS category
FROM regexp_test_text
ORDER BY id;

SELECT '11.2 ORDER BY regex boolean result' AS section;
SELECT id, txt, (txt REGEXP 'abc') AS matched
FROM regexp_test_text
ORDER BY (txt REGEXP 'abc') DESC, id;

SELECT '11.3 Subquery with REGEXP filter' AS section;
SELECT *
FROM (
    SELECT id, txt FROM regexp_test_text WHERE txt REGEXP 'abc'
) s
ORDER BY id;

-- ============================================================
-- 12. Join usage tests
-- ============================================================
SELECT '12.1 Join + regex predicate' AS section;
SELECT a.id AS a_id, a.txt AS a_txt, b.id AS b_id, b.txt AS b_txt
FROM regexp_test_text a
JOIN regexp_test_edge b
  ON a.txt REGEXP 'abc' AND b.txt RLIKE 'abc'
WHERE a.id IN (1, 2, 22)
ORDER BY a.id, b.id;

-- ============================================================
-- 13. Negative coverage / sanity checks
-- ============================================================
SELECT '13.1 Rows that should not match exact abc via REGEXP' AS section;
SELECT id, txt
FROM regexp_test_text
WHERE txt NOT REGEXP '^abc$'
ORDER BY id;

SELECT '13.2 Rows that should not match exact abc via RLIKE inverse comparison' AS section;
SELECT id, txt
FROM regexp_test_text
WHERE NOT (txt RLIKE '^abc$')
ORDER BY id;

-- ============================================================
-- 14. Cleanup
-- ============================================================
DROP TABLE regexp_test_null;
DROP TABLE regexp_test_edge;
DROP TABLE regexp_test_text;

-- pg default collate is incasesencitive
select 'ABC' regexp 'abc';

SELECT 'abc' REGEXP 'a.c' AS regexp_ok;
SELECT 'abc' NOT REGEXP 'a.c' AS not_regexp_ok;
SELECT 'foo.bar' RLIKE ('^foo' ||  '.bar$') AS rlike_ok;
SELECT 'foo.bar' NOT REGEXP ('^foo' ||  '.bar$') AS not_regexp2_ok;

set sql_mode = 'REAL_AS_FLOAT,ANSI_QUOTES,ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION,NO_ZERO_IN_DATE';
SELECT '14.1 PIPES_AS_CONCAT = off' AS section;
SELECT 'foo.bar' REGEXP ('^foo' ||   '.bar$') AS off_regexp_failed;
SELECT 'foo.bar' RLIKE ('^foo' ||  '.bar$') AS rlike_failed;

set sql_mode = 'REAL_AS_FLOAT,PIPES_AS_CONCAT,ANSI_QUOTES,ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION,NO_ZERO_IN_DATE';
SELECT '14.2 PIPES_AS_CONCAT = on' AS section;
SELECT 'foo.bar' REGEXP ('^foo' ||   '.bar$') AS off_regexp_ok;
SELECT 'foo.bar' RLIKE ('^foo' ||  '.bar$') AS rlike_ok;


SELECT 'ALL TESTS FINISHED' AS section;


--
-- This test a bug in regexp on Alpha
--
set sql_mode = Default;
show sql_mode;

set sql_mode = 'REAL_AS_FLOAT,PIPES_AS_CONCAT,ANSI_QUOTES,ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION,NO_ZERO_IN_DATE';


create table t1 (xxx char(128));
insert into t1 (xxx) values('this is a test of some long text to see what happens');
select * from t1 where xxx regexp('is a test of some long text to');
select * from t1 where xxx regexp('is a test of some long text to ');
select * from t1 where xxx regexp('is a test of some long text to s');
select * from t1 where xxx regexp('is a test of some long text to se');
drop table t1;

create table t1 (xxx char(128));
insert into t1 (xxx) values('this is some text: to test - out.reg exp (22/45)');
select * from t1 where xxx REGEXP '^this is some text: to test - out\\.reg exp [[(][0-9]+[/\\][0-9]+[])][ ]*$';
drop table t1;

--
-- Check with different character sets and collations
--
-- select _latin1 0xFF regexp _latin1 '[[:lower:]]' COLLATE latin1_bin;
-- select _koi8r  0xFF regexp _koi8r  '[[:lower:]]' COLLATE koi8r_bin;
-- select _latin1 0xFF regexp _latin1 '[[:upper:]]' COLLATE latin1_bin;
-- select _koi8r  0xFF regexp _koi8r  '[[:upper:]]' COLLATE koi8r_bin;

-- select _latin1 0xF7 regexp _latin1 '[[:alpha:]]';
-- select _koi8r  0xF7 regexp _koi8r  '[[:alpha:]]';

-- select _latin1'a' regexp _latin1'A' collate latin1_general_ci;
-- select _latin1'a' regexp _latin1'A' collate latin1_bin;

--
-- regexp cleanup()
--
set mysql.at_or_atat = on;
create table t1 (a varchar(40));
insert into t1 values ('C1'),('C2'),('R1'),('C3'),('R2'),('R3');
prepare stmt1 as select a from t1 where a rlike $1  order by a;
set @a='^C.*';
execute stmt1 using @a;
set @a='^R.*';
execute stmt1 using @a;
deallocate prepare stmt1;
drop table t1;
set mysql.at_or_atat = off;

--echo End of 4.1 tests


--
-- Bug --31440: 'select 1 regex null' asserts debug server
--

SELECT 1 REGEXP NULL;


--
-- Bug --39021: SELECT REGEXP BINARY NULL never returns
--

SELECT '' REGEXP BINARY NULL;
SELECT NULL REGEXP BINARY NULL;
SELECT 'A' REGEXP BINARY NULL;
SELECT "ABC" REGEXP BINARY NULL;

--echo End of 5.0 tests


--
-- Bug --37337: Function returns different results
--
CREATE TABLE t1(a INT, b CHAR(4));
INSERT INTO t1 VALUES (1, '6.1'), (1, '7.0'), (1, '8.0');
PREPARE stmt1 AS SELECT a FROM t1 WHERE a=1 AND '7.0' REGEXP b LIMIT 1;
EXECUTE stmt1;
EXECUTE stmt1;
EXECUTE stmt1;
EXECUTE stmt1;
DEALLOCATE PREPARE stmt1;
DROP TABLE t1;


--echo End of 5.1 tests

--
-- Bug --54805 definitions in regex/my_regex.h conflict with /usr/include/regex.h
--
-- This test verifies that '\t' is recognized as space, but not as blank.
-- This is *not* according to the POSIX standard, but seems to have been MySQL
-- behaviour ever since regular expressions were introduced.
-- See: Bug --55427 REGEXP does not recognize '\t' as [:blank:]
--
SELECT ' '  REGEXP '[[:blank:]]';
SELECT '\t' REGEXP '[[:blank:]]';

SELECT ' '  REGEXP '[[:space:]]';
SELECT '\t' REGEXP '[[:space:]]';

----error
select 1  regexp;
----right
select 1  `regexp`;


----error
create table  regexp (id int);
----right
create table  `regexp` (id int);
drop table     `regexp`;


----error
create table t1(regexp int);
----right
create table t1(`regexp` int);
drop table t1;

--BUG#7845
select 'abc' REGEXP binary 'ABC' ;

select 'abc' REGEXP 'ABC' collate utf8_general_ci;
