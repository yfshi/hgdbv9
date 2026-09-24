-- Regression for mysql_force_index_select_hint:
--   1) SELECT copies index hints onto Query (previously only UPDATE/DELETE)
--   2) USE/FORCE restrict to named indexes (cheaper index must not win)
--   3) USE/FORCE skip seqscan
--   4) IGNORE skips only the named index
--   5) Multi-table FROM keeps hints for every table (JOIN and comma FROM)
--   6) enable_indexhint=off does not apply hints
--   7) missing name / mixed USE+FORCE errors
SET mysql.enable_indexhint = on;
SET enable_bitmapscan = off;

CREATE TABLE t_force_idx (id INT PRIMARY KEY, c1 INT, c2 INT,
	INDEX idx_force_c1(c1),
	UNIQUE INDEX idx_force_c2(c2)
);
INSERT INTO t_force_idx (id, c1, c2)
SELECT g, CASE WHEN g < 90 THEN 1 ELSE g END, g
  FROM generate_series(0, 99) AS g;
ANALYZE t_force_idx;

-- ------------------------------------------------------------
-- Baseline: unique idx_force_c2 is cheaper than idx_force_c1
-- ------------------------------------------------------------
EXPLAIN (COSTS OFF)
SELECT * FROM t_force_idx WHERE c1 = 1 AND c2 = 50;

-- ------------------------------------------------------------
-- (1)(2)(3) SELECT FORCE INDEX: hint reaches planner, other indexes
-- and seqscan are out; must use idx_force_c1
-- ------------------------------------------------------------
EXPLAIN (COSTS OFF)
SELECT * FROM t_force_idx FORCE INDEX (idx_force_c1) WHERE c1 = 1 AND c2 = 50;

EXPLAIN (COSTS OFF)
SELECT * FROM t_force_idx FORCE KEY (idx_force_c1) WHERE c1 = 1 AND c2 = 50;

EXPLAIN (COSTS OFF)
SELECT * FROM t_force_idx AS t FORCE INDEX (idx_force_c1) WHERE c1 = 1 AND c2 = 50;

-- FORCE with both names: cheaper unique index may still be chosen
EXPLAIN (COSTS OFF)
SELECT * FROM t_force_idx FORCE INDEX (idx_force_c1, idx_force_c2) WHERE c1 = 1 AND c2 = 50;

-- Inner SELECT (transformSelectStmt) must keep the hint
EXPLAIN (COSTS OFF)
SELECT * FROM (
	SELECT * FROM t_force_idx FORCE INDEX (idx_force_c1) WHERE c1 = 1 AND c2 = 50 OFFSET 0
) AS s;

-- ------------------------------------------------------------
-- (3) USE INDEX: named indexes only; seqscan is not considered
-- Unselective idx_force_c1 -> index; unique idx_force_c2 -> index
-- ------------------------------------------------------------
EXPLAIN (COSTS OFF)
SELECT * FROM t_force_idx USE INDEX (idx_force_c1) WHERE c1 = 1 AND c2 = 50;

EXPLAIN (COSTS OFF)
SELECT * FROM t_force_idx USE INDEX (idx_force_c2) WHERE c1 = 1 AND c2 = 50;

EXPLAIN (COSTS OFF)
SELECT * FROM t_force_idx USE KEY (idx_force_c2) WHERE c1 = 1 AND c2 = 50;

-- USE still cannot pick the cheaper idx_force_c2
SET enable_seqscan = off;
EXPLAIN (COSTS OFF)
SELECT * FROM t_force_idx USE INDEX (idx_force_c1) WHERE c1 = 1 AND c2 = 50;
RESET enable_seqscan;

-- ------------------------------------------------------------
-- (4) IGNORE INDEX: skip only the named index
-- ------------------------------------------------------------
EXPLAIN (COSTS OFF)
SELECT * FROM t_force_idx IGNORE INDEX (idx_force_c1) WHERE c1 = 1 AND c2 = 50;

SET enable_seqscan = off;
EXPLAIN (COSTS OFF)
SELECT * FROM t_force_idx IGNORE INDEX (idx_force_c2) WHERE c1 = 1 AND c2 = 50;
RESET enable_seqscan;

-- ------------------------------------------------------------
-- UPDATE / DELETE already copied hints; keep covering DML + SELECT
-- ------------------------------------------------------------
SET enable_seqscan = off;
EXPLAIN (COSTS OFF)
UPDATE t_force_idx FORCE INDEX (idx_force_c1) SET c1 = c1 WHERE c1 = 1 AND c2 = 50;

EXPLAIN (COSTS OFF)
DELETE FROM t_force_idx FORCE INDEX (idx_force_c1) WHERE c1 = 1 AND c2 = 50;
RESET enable_seqscan;

-- ------------------------------------------------------------
-- (5) Multi-table: each FROM item keeps its own FORCE INDEX
-- ------------------------------------------------------------
CREATE TABLE t_force_a ( id INT PRIMARY KEY, x INT, INDEX idx_force_a(x));
CREATE TABLE t_force_b ( id INT PRIMARY KEY, x INT, INDEX idx_force_b(x));
INSERT INTO t_force_a (id, x)
SELECT g, CASE WHEN g < 90 THEN 1 ELSE g END FROM generate_series(0, 99) AS g;
INSERT INTO t_force_b (id, x)
SELECT g, CASE WHEN g < 90 THEN 1 ELSE g END FROM generate_series(0, 99) AS g;
ANALYZE t_force_a;
ANALYZE t_force_b;

SET enable_seqscan = off;
EXPLAIN (COSTS OFF)
SELECT * FROM t_force_a FORCE INDEX (idx_force_a)
	INNER JOIN t_force_b FORCE INDEX (idx_force_b) ON t_force_a.x = t_force_b.x
 WHERE t_force_a.x = 1 AND t_force_b.x = 1;

EXPLAIN (COSTS OFF)
SELECT * FROM t_force_a a USE INDEX (idx_force_a)
	INNER JOIN t_force_b b USE INDEX (idx_force_b) ON a.x = b.x
 WHERE a.x = 1 AND b.x = 1;
RESET enable_seqscan;

-- ------------------------------------------------------------
-- list_concat: each table has a cheaper unique index.  Without concat,
-- the second FORCE INDEX would replace the first, and t_concat_a would
-- use idx_concat_a_c2.  Comma FROM matches preCheckIndexHints comment.
-- ------------------------------------------------------------
CREATE TABLE t_concat_a (
    id INT PRIMARY KEY,
    c1 INT, c2 INT,
	INDEX idx_concat_a_c1(c1),
	UNIQUE INDEX idx_concat_a_c2(c2)
);
CREATE TABLE t_concat_b (
    id INT PRIMARY KEY,
    c1 INT, c2 INT,
	INDEX idx_concat_b_c1(c1),
	UNIQUE INDEX idx_concat_b_c2(c2)
);
INSERT INTO t_concat_a (id, c1, c2)
SELECT g,
	   CASE WHEN g < 90 THEN 1 ELSE g END,
	   g
  FROM generate_series(0, 99) AS g;
INSERT INTO t_concat_b (id, c1, c2)
SELECT g,
	   CASE WHEN g < 90 THEN 1 ELSE g END,
	   g
  FROM generate_series(0, 99) AS g;
ANALYZE t_concat_a;
ANALYZE t_concat_b;

-- Keep this as a comma FROM without join quals, so plan shape is stable and
-- assertions focus on index choice (idx_*_c1 must appear on both tables).
EXPLAIN (COSTS OFF)
SELECT * FROM t_concat_a FORCE INDEX (idx_concat_a_c1),
			  t_concat_b FORCE INDEX (idx_concat_b_c1)
 WHERE t_concat_a.c1 = 1 AND t_concat_a.c2 = 50
   AND t_concat_b.c1 = 1 AND t_concat_b.c2 = 50;

-- ------------------------------------------------------------
-- (6) enable_indexhint=off: no error, hint not applied (cheaper unique wins)
-- ------------------------------------------------------------
SET mysql.enable_indexhint = off;
EXPLAIN (COSTS OFF)
SELECT * FROM t_force_idx FORCE INDEX (idx_force_c1) WHERE c1 = 1 AND c2 = 50;
SET mysql.enable_indexhint = on;

-- ------------------------------------------------------------
-- (7) Errors: missing index; mixed USE and FORCE
-- ------------------------------------------------------------
EXPLAIN (COSTS OFF)
SELECT * FROM t_force_idx FORCE INDEX (no_such_idx) WHERE c1 = 1;

EXPLAIN (COSTS OFF)
SELECT * FROM t_force_idx USE INDEX (idx_force_c1) FORCE INDEX (idx_force_c2) WHERE c1 = 1;

RESET enable_bitmapscan;
DROP TABLE t_force_a, t_force_b, t_concat_a, t_concat_b;
DROP TABLE t_force_idx;
RESET mysql.enable_indexhint;

----- bug#9228
SET mysql.enable_indexhint = on;
CREATE TABLE t_force_num (n INT PRIMARY KEY); 
INSERT INTO t_force_num VALUES (0),(1),(2),(3),(4),(5),(6),(7),(8),(9);
CREATE TABLE t_force_idx (id INT PRIMARY KEY,c1 INT,c2 INT,pad VARCHAR(200),INDEX idx_force_c1(c1),UNIQUE INDEX idx_force_c2(c2));
INSERT INTO t_force_idx(id, c1, c2, pad) SELECT num, CASE WHEN num<950 THEN 1 ELSE num END, num, REPEAT('x', 200)
  FROM (SELECT a.n*100+b.n*10+c.n AS num FROM t_force_num AS a CROSS JOIN t_force_num AS b CROSS JOIN t_force_num AS c ) AS s;

EXPLAIN (COSTS OFF)
SELECT * FROM t_force_idx WHERE c1=1 AND c2=500;
EXPLAIN (COSTS OFF)
SELECT * FROM t_force_idx FORCE INDEX (idx_force_c1) WHERE c1=1 AND c2=500;

SET enable_bitmapscan = off;
EXPLAIN (COSTS OFF)
SELECT * FROM t_force_idx WHERE c1=1 AND c2=500;
EXPLAIN (COSTS OFF)
SELECT * FROM t_force_idx FORCE INDEX (idx_force_c1) WHERE c1=1 AND c2=500;

SELECT * FROM t_force_idx FORCE INDEX (idx_force_c1) WHERE c1=1 AND c2=500;
RESET mysql.enable_indexhint;
RESET enable_bitmapscan;
DROP TABLE t_force_num, t_force_idx;

----- bug#9343
-- ------------------------------------------------------------
-- (8) Partitioned UPDATE.  The hint is on the parent; the scan is
-- the leaf.  USE/FORCE Index Scan it; IGNORE Seq Scan it.
-- ------------------------------------------------------------
SET mysql.enable_indexhint = on;
CREATE TABLE t1 (a int, b int) PARTITION BY HASH (a);
CREATE TABLE p11 PARTITION OF t1 FOR VALUES WITH (MODULUS 2, REMAINDER 0);
CREATE TABLE p12 PARTITION OF t1 FOR VALUES WITH (MODULUS 2, REMAINDER 1);
CREATE TABLE t2 (c int, d int) PARTITION BY HASH (c);
CREATE TABLE p21 PARTITION OF t2 FOR VALUES WITH (MODULUS 2, REMAINDER 0);
CREATE TABLE p22 PARTITION OF t2 FOR VALUES WITH (MODULUS 2, REMAINDER 1);
INSERT INTO t1 VALUES (1, 2), (2, 3);
INSERT INTO t2 VALUES (1, 2), (2, 3);
CREATE INDEX t1_idx ON t1(a);
CREATE INDEX t2_idx ON t2(c);

CREATE FUNCTION pg_temp.mys_plan_has(sql text, kind text, relname text)
RETURNS boolean
LANGUAGE plpgsql AS $$
DECLARE
	line text;
BEGIN
	FOR line IN EXECUTE 'EXPLAIN (COSTS OFF) ' || sql
	LOOP
		IF position(kind in line) > 0 AND position(relname in line) > 0 THEN
			RETURN true;
		END IF;
	END LOOP;
	RETURN false;
END;
$$;

-- USE INDEX: Index Scan on the hinted partition
SELECT pg_temp.mys_plan_has(
	'UPDATE t1 PARTITION (p11) USE INDEX (t1_idx), t2 PARTITION (p21) SET t1.a = t2.d WHERE t1.a = t2.c',
	'Index Scan', 'p11') AS use_index_scan;

-- IGNORE INDEX: Seq Scan on the hinted partition, not an Index Scan
SELECT pg_temp.mys_plan_has(
	'UPDATE t1 PARTITION (p11) IGNORE INDEX (t1_idx), t2 PARTITION (p21) SET t1.a = t2.d WHERE t1.a = t2.c',
	'Seq Scan', 'p11') AS ignore_seq_scan,
       pg_temp.mys_plan_has(
	'UPDATE t1 PARTITION (p11) IGNORE INDEX (t1_idx), t2 PARTITION (p21) SET t1.a = t2.d WHERE t1.a = t2.c',
	'Index Scan', 'p11') AS ignore_index_scan;

-- FORCE INDEX: Index Scan on the hinted partition
SELECT pg_temp.mys_plan_has(
	'UPDATE t1 PARTITION (p11) FORCE INDEX (t1_idx), t2 PARTITION (p21) SET t1.a = t2.d WHERE t1.a = t2.c',
	'Index Scan', 'p11') AS force_index_scan;

RESET enable_bitmapscan;
RESET mysql.enable_indexhint;
DROP FUNCTION pg_temp.mys_plan_has(text, text, text);
DROP TABLE t1, t2;
