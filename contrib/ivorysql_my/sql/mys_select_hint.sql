--
-- MySQL SELECT modifiers accepted around ALL/DISTINCT
--

DROP TABLE IF EXISTS mysql_select_modifiers_test;
DROP TABLE IF EXISTS mysql_select_modifier_names;

CREATE TABLE mysql_select_modifiers_test (
  a integer
);


INSERT INTO mysql_select_modifiers_test VALUES (1);
INSERT INTO mysql_select_modifiers_test VALUES (1);
INSERT INTO mysql_select_modifiers_test VALUES (2);

SELECT SQL_SMALL_RESULT SQL_BIG_RESULT SQL_BUFFER_RESULT
       SQL_CACHE SQL_CALC_FOUND_ROWS ALL a
FROM mysql_select_modifiers_test
ORDER BY a;

SELECT ALL SQL_SMALL_RESULT SQL_NO_CACHE SQL_BUFFER_RESULT a
FROM mysql_select_modifiers_test
ORDER BY a;

SELECT SQL_SMALL_RESULT DISTINCT SQL_BIG_RESULT SQL_BUFFER_RESULT
       SQL_NO_CACHE SQL_CALC_FOUND_ROWS a
FROM mysql_select_modifiers_test
ORDER BY a;

SELECT DISTINCT SQL_SMALL_RESULT SQL_BUFFER_RESULT SQL_CACHE a
FROM mysql_select_modifiers_test
ORDER BY a;

SELECT SQL_CACHE a
FROM mysql_select_modifiers_test
ORDER BY a;

SELECT a AS sql_cache
FROM mysql_select_modifiers_test
ORDER BY a;

--
-- Reserved keyword cases (MySQL 5.7: reserved words need quoting, so bare
-- table/column/alias forms should fail)
--

--mysql e1064 (42000): You have an error in your SQL syntax;
SELECT SQL_BIG_RESULT;

--mysql e1064 (42000): You have an error in your SQL syntax;
SELECT a AS SQL_BIG_RESULT
FROM mysql_select_modifiers_test
ORDER BY a;

--mysql e1064 (42000): You have an error in your SQL syntax;
SELECT a SQL_BIG_RESULT
FROM mysql_select_modifiers_test
ORDER BY a;
CREATE TABLE SQL_BIG_RESULT (a integer);
SELECT a
FROM SQL_BIG_RESULT;


DROP TABLE mysql_select_modifiers_test;
