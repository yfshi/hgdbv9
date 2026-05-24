-- Test cases for ORDER BY using SELECT-list aliases.
-- These snippets are adapted from MySQL 5.7 MTR tests and can be run
-- directly in the mysql client.

-- Case 1: GROUP BY plus ORDER BY aliases from the select list.
DROP TABLE IF EXISTS t1, t2, t3;

CREATE TABLE t1 (id INT, facility CHAR(20));
CREATE TABLE t2 (facility CHAR(20));

INSERT INTO t1 VALUES
(NULL,NULL), (-1,''), (0,''), (1,'/L'), (2,'A01'), (3,'ANC'),
(4,'F01'), (5,'FBX'), (6,'MT'), (7,'P'), (8,'RV'), (9,'SRV'), (10,'VMT');

INSERT INTO t2 SELECT DISTINCT facility FROM t1;

SET mysql.at_or_atat = on;
SET @old_sql_mode=@@sql_mode;
set  sql_mode = replace(@@sql_mode, ',ONLY_FULL_GROUP_BY', '');
--SET sql_mode=(SELECT REPLACE(@@sql_mode,'ONLY_FULL_GROUP_BY',''));

SELECT id+0 AS a, MAX(id), CONCAT(facility) AS b
FROM t1
GROUP BY a
ORDER BY b DESC, a;

SET sql_mode=@old_sql_mode;
DROP TABLE t1, t2;

-- Case 2: Aggregate aliases used in ORDER BY.
DROP TABLE IF EXISTS t1;

CREATE TABLE t1(a INTEGER);
INSERT INTO t1 VALUES (1), (2);

SELECT COUNT(*) AS c FROM t1 ORDER BY COUNT(*);
SELECT COUNT(*) AS c FROM t1 ORDER BY c;

SELECT a, COUNT(*) AS c
FROM t1
GROUP BY a
ORDER BY COUNT(*);

SELECT a, COUNT(*) AS c
FROM t1
GROUP BY a
ORDER BY c;

DROP TABLE t1;

-- Case 3: UNION result column alias used by the global ORDER BY.
DROP TABLE IF EXISTS t1;

CREATE TABLE t1(b INT) ENGINE=InnoDB;

INSERT INTO t1 VALUES (2), (1), (3);

SELECT b AS z FROM t1
UNION
SELECT b FROM t1
ORDER BY z;

-- SELECT b AS z FROM t1
-- UNION
-- SELECT b FROM t1
-- ORDER BY (SELECT z);

-- SELECT b AS z FROM t1
-- UNION
-- (SELECT b FROM t1)
-- ORDER BY (SELECT z);

DROP TABLE t1;

-- Case 4: Function expression alias used by ORDER BY ASC and DESC.
DROP TABLE IF EXISTS t1;

CREATE TABLE t1 (
  id INT(11) NOT NULL,
  `date` INT(10) DEFAULT NULL,
  `text` VARCHAR(32) NOT NULL
);

INSERT INTO t1 VALUES
(1,1110000000,'Day 1'),
(2,1111000000,'Day 2'),
(3,1112000000,'Day 3');

SELECT id,
       `date` AS date_ord,
       `text`
FROM t1
ORDER BY date_ord ASC;

SELECT id,
       `date` AS date_ord,
       `text`
FROM t1
ORDER BY date_ord DESC;

-- SELECT id,
--        IF(`date` IS NULL, '-', FROM_UNIXTIME(`date`, '%d-%m-%Y')) AS date_ord,
--        `text`
-- FROM t1
-- ORDER BY date_ord DESC;

DROP TABLE t1;

-- Case 5: Numeric function result alias used by filesort.
DROP TABLE IF EXISTS t1, t2, t3, t4;

CREATE TABLE t1(a NUMERIC, b INT);
INSERT INTO t1 VALUES (123456, 40), (123456, 40);

-- SELECT TRUNCATE(a, b) AS c FROM t1 ORDER BY c;
SELECT ROUND(a, b) AS c FROM t1 ORDER BY c;
SELECT ROUND(a, 100) AS c FROM t1 ORDER BY c;

DELETE FROM t1;
INSERT INTO t1 VALUES (1234567890, 20), (999.99, 5);

SELECT ROUND(a, b) AS c FROM t1 ORDER BY c;

DROP TABLE t1;

-- Case 6: Aggregate aliases over TEXT grouping columns.
DROP TABLE IF EXISTS t;

CREATE TABLE t(txt TEXT, i INT);
INSERT INTO t VALUES ('a', 2), ('b', 8), ('b', 0), ('c', 2);

SELECT txt, AVG(i) AS a
FROM t
GROUP BY txt
ORDER BY a, txt;

SELECT txt, VAR_POP(i) AS v
FROM t
GROUP BY txt
ORDER BY v, txt;

SELECT txt, STDDEV_POP(i) AS s
FROM t
GROUP BY txt
ORDER BY s, txt;

SELECT SQL_BUFFER_RESULT txt, AVG(i) AS a
FROM t
GROUP BY txt
ORDER BY a, txt;

DROP TABLE t;

-- Case 7: Alias in ORDER BY inside scalar subqueries and UNION.
SELECT (SELECT 0 AS a UNION SELECT 1 AS a ORDER BY a ASC LIMIT 1) AS dev;
SELECT (SELECT 0 AS a UNION SELECT 1 AS a ORDER BY a DESC LIMIT 1) AS dev;
SELECT (SELECT 1 AS a ORDER BY a) AS dev;

-- Case 8: Same-level ORDER BY name resolution prefers the SELECT-list alias.
-- The alias `a` is defined as column `b`, while table column `a` has a
-- different order. If ORDER BY uses the target-list alias, rows come out by b.
DROP TABLE IF EXISTS t_alias_same_level;

CREATE TABLE t_alias_same_level (
  id INT PRIMARY KEY,
  a INT,
  b INT
);

INSERT INTO t_alias_same_level VALUES
(1, 30, 1),
(2, 10, 3),
(3, 20, 2);

SELECT b AS a, id, t_alias_same_level.a AS table_a
FROM t_alias_same_level
ORDER BY a;

-- A qualified name forces resolution to the table column instead of the alias.
SELECT b AS a, id, t_alias_same_level.a AS table_a
FROM t_alias_same_level
ORDER BY t_alias_same_level.a;

DROP TABLE t_alias_same_level;

-- Case 9: HAVING and ORDER BY can resolve the same unqualified name differently.
-- HAVING checks the grouped result expression/alias, while ORDER BY should use
-- the SELECT-list alias when the alias name conflicts with a table column name.
DROP TABLE IF EXISTS t_having_order_alias;

CREATE TABLE t_having_order_alias (
  grp INT,
  a INT,
  b INT
);

INSERT INTO t_having_order_alias VALUES
(1, 100, 1),
(1, 200, 2),
(2, 300, 3),
(2, 400, 4),
(3, 500, 5);

SELECT grp, COUNT(*) AS a
FROM t_having_order_alias
GROUP BY grp
HAVING a > 1
ORDER BY a, grp;

-- The table column can still be named explicitly in ORDER BY.
SELECT grp, COUNT(*) AS a, MIN(t_having_order_alias.a) AS min_table_a
FROM t_having_order_alias
GROUP BY grp
HAVING a > 1
ORDER BY MIN(t_having_order_alias.a);

DROP TABLE t_having_order_alias;

-- Case 10: Alias used inside an ORDER BY expression at the same query level.
-- `alias_a + 0` should be resolved from the target list alias, not from a table
-- column, because no table column named alias_a exists.
DROP TABLE IF EXISTS t_order_alias_expr;

CREATE TABLE t_order_alias_expr (
  id INT PRIMARY KEY,
  a INT,
  b INT
);

INSERT INTO t_order_alias_expr VALUES
(1, 30, 300),
(2, 10, 100),
(3, 20, 200);

SELECT a + 1 AS alias_a, id, b
FROM t_order_alias_expr
ORDER BY alias_a + 0;

DROP TABLE t_order_alias_expr;

-- Case 11: Inner ORDER BY can use an outer table column through correlation.
-- The subquery ORDER BY references outer_t.sort_key, which is a table column in
-- the outer query block.
DROP TABLE IF EXISTS outer_t, inner_t;

CREATE TABLE outer_t (
  id INT PRIMARY KEY,
  sort_key INT
);

CREATE TABLE inner_t (
  id INT PRIMARY KEY,
  val INT
);

INSERT INTO outer_t VALUES (1, 20), (2, 10);
INSERT INTO inner_t VALUES (1, 100), (2, 200);

SELECT outer_t.id,
       (SELECT val FROM inner_t ORDER BY outer_t.sort_key LIMIT 1) AS picked_val
FROM outer_t
ORDER BY outer_t.id;


SELECT outer_t.id,
       (SELECT val FROM inner_t ORDER BY sort_key LIMIT 1) AS picked_val
FROM outer_t
ORDER BY outer_t.id;

DROP TABLE inner_t, outer_t;



-----------------------------------------
DROP TABLE IF EXISTS t1, t2, t3, t4;

create table test(name varchar(20));
insert into test values ('a'),('a'),('b');

select name, count(*) as cnt
from test
group by name
order by cnt;


-- ambiguous
CREATE TABLE test_ambiguous (
    id INT,
    table_type VARCHAR(30),
    table_name VARCHAR(30),
    flag VARCHAR(10)
);

INSERT INTO test_ambiguous VALUES
(1, 'BASE TABLE', 't1', 'Y'),
(2, 'VIEW',       't2', 'Y'),
(3, 'TEMPORARY',  't3', 'N'),
(4, 'BASE TABLE', 't4', 'N');

SELECT
    id,
    table_type,
    table_name AS table_type
FROM test_ambiguous
ORDER BY table_type;

drop table test_ambiguous;


CREATE TABLE test_table2 (
    id INT,
    col2 INT,
    table_type2 VARCHAR(30),
    table_name2 VARCHAR(30),
    flag VARCHAR(10),
    num INT
);

CREATE TABLE test_table3 (
    id INT,
    col3 INT,
    table_type3 VARCHAR(30),
    table_name3 VARCHAR(30),
    flag VARCHAR(10),
    num INT
);


INSERT INTO test_table2 VALUES
(5, 2, 'BASE TABLE', 't2', 'Y', 4),
(6, 2, 'VIEW',       't2', 'Y', 3),
(7, 2, 'TEMPORARY',  't2', 'N', 2),
(8, 2, 'BASE TABLE', 't2', 'N', 1);

INSERT INTO test_table3 VALUES
(9, 3, 'BASE TABLE', 't3', 'Y', 8),
(10, 3, 'VIEW',       't3', 'Y', 7),
(11, 3, 'TEMPORARY',  't3', 'N', 6),
(12, 3, 'BASE TABLE', 't3', 'N', 5);

-- inner table col 
select id, col2 as aaa from test_table2
having (
    select id from test_table3  order by table_name3 DESC limit 1
) = 9;

-- inner  targetList
--empty
select id, num as alias_a from test_table2
having (
    select id as alias_a from test_table3  order by alias_a DESC limit 1
) = 9;

select id, num as alias_a from test_table2
having (
    select id as alias_a from test_table3  order by alias_a DESC limit 1
) = 12;


-- outter targetlits
select id, num as aaa from test_table2
having (
    select id from test_table3  order by aaa limit 1
) = 9;

--  outter  table
--empty
select id, col2 as aaa from test_table2
having (
    select id from test_table3  order by table_name2 limit 1
) = 12;
select id, col2 as aaa from test_table2
having (
    select id from test_table3  order by table_name2 limit 1
) = 9;


drop table  test_table2, test_table3;

