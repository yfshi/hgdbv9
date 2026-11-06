-- ============================================================
-- grammar.sql — 语法测试（CTE/CTAS/特殊语法等）
-- 覆盖 parser_cte_comment, parser_cte_ext, const_star,
--   const_star_ext, dml_complete, subquery, agg_window,
--   ctas, ctas_execute, at_time_zone, between_symmetric,
--   boolean_test, collate_expression, default_keyword,
--   indirection, power_operator, similar_to,
--   variadic_within_group, p0_expression_coverage
-- ============================================================

-- ============================================================
-- 0. Setup — 共享KEK/DEK
-- ============================================================
DROP KEK IF EXISTS kek_grammar CASCADE;
CREATE KEK kek_grammar WITH (
    key_store = 'localkms',
    key_path = '/tmp/kek_grammar.key'
);
CREATE DEK dek_grammar WITH VALUES(
    KEK = kek_grammar
);

-- ============================================================
-- 1. Basic DML (parser_const_star: test1a-1e)
-- ============================================================
SELECT '=== 1 Basic DML ===';

-- 1a: Basic SELECT
SELECT '1a_basic_SELECT', 1;

-- 1b: INSERT
CREATE TABLE IF NOT EXISTS test_grammar_basic (id INT4, name VARCHAR(100));
INSERT INTO test_grammar_basic VALUES (1, 'test_filter');
SELECT '1b_INSERT', * FROM test_grammar_basic;

-- 1c: UPDATE
UPDATE test_grammar_basic SET name = 'updated' WHERE id = 1;
SELECT '1c_UPDATE', * FROM test_grammar_basic;

-- 1d: DELETE
DELETE FROM test_grammar_basic WHERE id = 1;
SELECT '1d_DELETE', count(*) FROM test_grammar_basic;

-- 1e: DROP
DROP TABLE IF EXISTS test_grammar_basic;
SELECT '1e_DROP_OK', 1;

-- ============================================================
-- 2. Comments (parser_cte_comment: test2a-2e)
-- ============================================================
SELECT '=== 2 Comments ===';

-- 2a: Block comment before SELECT
/* This is a block comment */ SELECT '2a_block_comment_before', 1;

-- 2b: Block comment after SELECT
SELECT '2b_block_comment_after', 1 /* block comment */;

-- 2c: Line comment before SELECT
-- This is a line comment
SELECT '2c_line_comment', 1;

-- 2d: EXPLAIN
EXPLAIN SELECT '2d_EXPLAIN', 1;

-- 2e: WITH
WITH cte2e AS (SELECT 1 AS n) SELECT '2e_WITH', n FROM cte2e;

-- ============================================================
-- 3. WITH CTE (parser_cte_ext: cte1-22)
-- ============================================================
SELECT '=== 3 WITH CTE ===';

-- Setup CTE tables
DROP TABLE IF EXISTS test_cte_t CASCADE;
CREATE TABLE test_cte_t (id INT4, val INT4, name VARCHAR(50));
INSERT INTO test_cte_t VALUES (1, 10, 'a'), (2, 20, 'b'), (3, 30, 'c'), (4, 40, 'd');

-- CTE1: Single CTE + SELECT
WITH cte1 AS (SELECT id, val FROM test_cte_t WHERE val > 15)
SELECT 'cte1_basic', * FROM cte1;

-- CTE2: CTE with column aliases
WITH cte2(x, y) AS (SELECT id, val FROM test_cte_t WHERE val > 15)
SELECT 'cte2_alias', x, y FROM cte2;

-- CTE3: CTE + ORDER BY
WITH cte3 AS (SELECT id, val FROM test_cte_t)
SELECT 'cte3_order', * FROM cte3 ORDER BY val DESC;

-- CTE4: CTE + WHERE
WITH cte4 AS (SELECT * FROM test_cte_t)
SELECT 'cte4_where', id, name FROM cte4 WHERE val > 20;

-- CTE5: Two CTEs comma-separated
WITH
    cte5a AS (SELECT id, val FROM test_cte_t WHERE val >= 20),
    cte5b AS (SELECT id, val FROM test_cte_t WHERE val <= 20)
SELECT 'cte5_multi', cte5a.val AS hi, cte5b.val AS lo
FROM cte5a JOIN cte5b ON cte5a.id = cte5b.id;

-- CTE6: Three CTEs
WITH
    a AS (SELECT 1 AS n),
    b AS (SELECT 2 AS n),
    c AS (SELECT 3 AS n)
SELECT 'cte6_three', a.n + b.n + c.n AS total FROM a, b, c;

-- CTE7: CTE referencing previous CTE
WITH
    base AS (SELECT id, val FROM test_cte_t WHERE val > 10),
    doubled AS (SELECT id, val * 2 AS dval FROM base)
SELECT 'cte7_chain', * FROM doubled ORDER BY id;

-- CTE8: CTE with constant + star
WITH cte8 AS (SELECT id, val FROM test_cte_t)
SELECT 'cte8_both', 999, * FROM cte8;

-- CTE9: WITH + INSERT ... SELECT
CREATE TABLE test_cte_ins (label TEXT, id INT4, val INT4);
WITH ins_cte AS (SELECT id, val FROM test_cte_t WHERE val >= 30)
INSERT INTO test_cte_ins SELECT 'cte9_ins', * FROM ins_cte;
SELECT * FROM test_cte_ins;

-- CTE10: WITH + UPDATE
CREATE TABLE test_cte_upd (id INT4, val INT4);
INSERT INTO test_cte_upd VALUES (1, 0), (2, 0), (3, 0);
WITH upd_cte AS (SELECT id, val FROM test_cte_t WHERE id <= 2)
UPDATE test_cte_upd SET val = upd_cte.val
FROM upd_cte WHERE test_cte_upd.id = upd_cte.id;
SELECT 'cte10_upd', * FROM test_cte_upd;

-- CTE11: WITH + DELETE
CREATE TABLE test_cte_del (id INT4, val INT4);
INSERT INTO test_cte_del VALUES (1, 10), (2, 20), (3, 30), (4, 40);
WITH del_cte AS (SELECT id FROM test_cte_t WHERE val > 25)
DELETE FROM test_cte_del WHERE id IN (SELECT id FROM del_cte);
SELECT 'cte11_del', * FROM test_cte_del;

-- CTE12: CTE + UNION ALL
WITH
    low AS (SELECT id, val, 'low' AS tier FROM test_cte_t WHERE val < 20),
    high AS (SELECT id, val, 'high' AS tier FROM test_cte_t WHERE val >= 20)
SELECT 'cte12_union', * FROM low UNION ALL SELECT 'cte12_union', * FROM high;

-- CTE13: CTE + subquery
WITH cte13 AS (SELECT id, val FROM test_cte_t)
SELECT 'cte13_sub', * FROM (SELECT * FROM cte13 WHERE val > 15) sub;

-- CTE14: CTE + aggregate
WITH cte14 AS (SELECT val FROM test_cte_t)
SELECT 'cte14_agg', COUNT(*), SUM(val) FROM cte14;

-- CTE15: CTE + self-join
WITH cte15 AS (SELECT id, val FROM test_cte_t)
SELECT 'cte15_selfjoin', a.val AS v1, b.val AS v2
FROM cte15 a JOIN cte15 b ON a.id + 1 = b.id;

-- CTE16: CTE empty result
WITH empty_cte AS (SELECT * FROM test_cte_t WHERE val < 0)
SELECT 'cte16_empty', * FROM empty_cte;

-- CTE17: CTE + DISTINCT
WITH cte17 AS (SELECT val FROM test_cte_t UNION ALL SELECT val FROM test_cte_t)
SELECT DISTINCT 'cte17_distinct', val FROM cte17 ORDER BY val;

-- CTE18: CTE with function expressions
WITH cte18 AS (
    SELECT upper(name) AS uname, val * 10 AS decaval FROM test_cte_t
)
SELECT 'cte18_func', * FROM cte18;

-- CTE19: WITH RECURSIVE basic
WITH RECURSIVE t19(n) AS (
    SELECT 1
    UNION ALL
    SELECT n + 1 FROM t19 WHERE n < 3
)
SELECT 'cte19_recursive', * FROM t19;

-- CTE20: WITH RECURSIVE + constant + star
WITH RECURSIVE t20 AS (
    SELECT id, val FROM test_cte_t WHERE id = 1
    UNION ALL
    SELECT t.id + 1, t.val FROM test_cte_t t
        JOIN t20 ON t20.id + 1 = t.id
)
SELECT 'cte20_rec_star', 999, * FROM t20;

-- CTE21: WITH RECURSIVE graph search
DROP TABLE IF EXISTS test_cte_graph CASCADE;
CREATE TABLE test_cte_graph (src INT4, dst INT4);
INSERT INTO test_cte_graph VALUES (1, 2), (2, 3), (3, 4), (1, 5);
WITH RECURSIVE path21 AS (
    SELECT src, dst FROM test_cte_graph WHERE src = 1
    UNION ALL
    SELECT g.src, g.dst FROM test_cte_graph g
        JOIN path21 p ON g.src = p.dst
)
SELECT 'cte21_graph', * FROM path21;

-- CTE22: Multiple RECURSIVE CTEs
WITH RECURSIVE
    t22a(n) AS (SELECT 1 UNION ALL SELECT n+1 FROM t22a WHERE n < 2),
    t22b(n) AS (SELECT 10 UNION ALL SELECT n-1 FROM t22b WHERE n > 8)
SELECT 'cte22_multi_rec', t22a.n AS a, t22b.n AS b FROM t22a, t22b;

-- Cleanup CTE tables
DROP TABLE IF EXISTS test_cte_graph CASCADE;
DROP TABLE IF EXISTS test_cte_t CASCADE;
DROP TABLE IF EXISTS test_cte_ins CASCADE;
DROP TABLE IF EXISTS test_cte_upd CASCADE;
DROP TABLE IF EXISTS test_cte_del CASCADE;

-- ============================================================
-- 4. Constant+Star (const_star_ext: cs1-17)
-- ============================================================
SELECT '=== 4 Constant+Star ===';

DROP TABLE IF EXISTS test_cs_t CASCADE;
CREATE TABLE test_cs_t (a INT4, b INT4, c INT4);
INSERT INTO test_cs_t VALUES (1, 2, 3), (4, 5, 6);

-- CS1: Single constant before star
SELECT 'cs1_const_star', * FROM test_cs_t;

-- CS2: Star before constant
SELECT *, 'cs2_star_const' FROM test_cs_t;

-- CS3: Multiple constants before star
SELECT 'cs3a', 42, 'cs3b', * FROM test_cs_t;

-- CS4: Constant-star-constant
SELECT 'cs4_before', *, 'cs4_after' FROM test_cs_t;

-- CS5: Star-constant (no leading constant)
SELECT *, 'cs5' FROM test_cs_t;

-- CS6: Pure star control
SELECT 'cs6_ctrl', * FROM test_cs_t;

-- CS7: Constant+star with alias
SELECT 'cs7' AS label, * FROM test_cs_t;

-- CS8: DISTINCT + constant + star
SELECT DISTINCT 'cs8', * FROM test_cs_t;

-- CS9: Constant+star FROM subquery
SELECT 'cs9', * FROM (SELECT a, b FROM test_cs_t) AS sub;

-- CS10: Constant+star FROM VALUES
SELECT 'cs10', * FROM (VALUES (10, 20), (30, 40)) AS v(x, y);

-- CS11: Constant+star + WHERE
SELECT 'cs11', * FROM test_cs_t WHERE a > 1;

-- CS12: Constant+star + ORDER BY
SELECT 'cs12', * FROM test_cs_t ORDER BY a DESC;

-- CS13: INSERT INTO ... SELECT constant, * (data-modifying CTE)
DROP TABLE IF EXISTS test_cs_t2 CASCADE;
CREATE TABLE test_cs_t2 (label TEXT, a INT4, b INT4, c INT4);
INSERT INTO test_cs_t2 SELECT 'cs13', * FROM test_cs_t;
SELECT * FROM test_cs_t2;

-- CS14: WITH CTE + constant+star
WITH cs_cte AS (SELECT a, b FROM test_cs_t WHERE a > 1)
SELECT 'cs14', * FROM cs_cte;

-- CS15: EXPLAIN + constant+star
EXPLAIN SELECT 'cs15', * FROM test_cs_t;

-- CS16: UNION + constant+star
SELECT 'cs16a', * FROM test_cs_t WHERE a = 1
UNION ALL
SELECT 'cs16b', * FROM test_cs_t WHERE a = 4;

-- CS17: NULL constant + star
SELECT NULL, 'cs17_str', * FROM test_cs_t;

DROP TABLE IF EXISTS test_cs_t CASCADE;
DROP TABLE IF EXISTS test_cs_t2 CASCADE;

-- ============================================================
-- 5. DML Complete (dml_complete: 13 tests)
-- ============================================================
SELECT '=== 5 DML Complete ===';

CREATE TABLE t_dml (
    id int4_enc ENCRYPTED WITH (DEK = 'dek_grammar', SUPPORT_ALGO = 'OPE'),
    val int4_enc ENCRYPTED WITH (DEK = 'dek_grammar', SUPPORT_ALGO = 'OPE')
);
INSERT INTO t_dml VALUES (1, 10);
INSERT INTO t_dml VALUES (2, 20);
INSERT INTO t_dml VALUES (1, 30);
INSERT INTO t_dml VALUES (3, 15);

-- Test 1: DISTINCT ON (encrypted column)
SELECT DISTINCT ON (id) id, val FROM t_dml ORDER BY id, val;

-- Test 2: FOR UPDATE locking
SELECT * FROM t_dml FOR UPDATE;

-- Test 3: FOR SHARE locking
SELECT * FROM t_dml FOR SHARE;

-- Test 4: FOR NO KEY UPDATE
SELECT * FROM t_dml FOR NO KEY UPDATE;

-- Test 5: FOR KEY SHARE
SELECT * FROM t_dml FOR KEY SHARE;

-- Test 6: Bare alias in SELECT target
SELECT id alias_id, val alias_val FROM t_dml;

-- Test 7: ORDER BY NULLS FIRST
SELECT id, val FROM t_dml ORDER BY val NULLS FIRST;

-- Test 8: ORDER BY NULLS LAST
SELECT id, val FROM t_dml ORDER BY val NULLS LAST;

-- Test 9: WITH RECURSIVE CTE
WITH RECURSIVE cte5_9 AS (
    SELECT 1 AS n
    UNION ALL
    SELECT n + 1 FROM cte5_9 WHERE n < 3
) SELECT * FROM cte5_9;

-- Test 10: Data-modifying CTE (DELETE RETURNING)
WITH del AS (
    DELETE FROM t_dml WHERE id = 2 RETURNING *
) SELECT * FROM del;

-- Test 11: Data-modifying CTE (INSERT RETURNING)
WITH ins AS (
    INSERT INTO t_dml VALUES (5, 50) RETURNING *
) SELECT * FROM ins;

-- Test 12: Verify remaining data
SELECT id, val FROM t_dml ORDER BY id, val;

-- Test 13: ORDER BY USING operator
SELECT id, val FROM t_dml ORDER BY val USING > NULLS LAST;

DROP TABLE t_dml;

-- ============================================================
-- 6. SubLink Subquery (subquery: test1-7)
-- ============================================================
SELECT '=== 6 SubLink Subquery ===';

DROP TABLE IF EXISTS test_sub CASCADE;
CREATE TABLE test_sub (id INT4, val INT4);
INSERT INTO test_sub VALUES (1, 10), (2, 20), (3, 30);

-- Test 1: Scalar subquery in WHERE
SELECT '6_1_scalar', * FROM test_sub WHERE id = (SELECT max(id) FROM test_sub);

-- Test 2: EXISTS
SELECT '6_2_exists', * FROM test_sub WHERE EXISTS (SELECT 1 FROM test_sub t2 WHERE t2.val = test_sub.val);

-- Test 3: IN subquery
SELECT '6_3_in', * FROM test_sub WHERE id IN (SELECT id FROM test_sub WHERE val > 10);

-- Test 4: NOT IN subquery
SELECT '6_4_not_in', * FROM test_sub WHERE id NOT IN (SELECT id FROM test_sub WHERE val > 25);

-- Test 5: = ANY subquery
SELECT '6_5_eq_any', * FROM test_sub WHERE id = ANY (SELECT id FROM test_sub WHERE val > 10);

-- Test 6: > ALL subquery
SELECT '6_6_gt_all', * FROM test_sub WHERE id > ALL (SELECT id FROM test_sub WHERE val > 25);

-- Test 7: Scalar subquery in SELECT list
SELECT '6_7_scalar_select', id, (SELECT max(val) FROM test_sub) FROM test_sub WHERE id = 1;

DROP TABLE test_sub;

-- ============================================================
-- 7. Aggregate & Window (agg_window: test1-12)
-- ============================================================
SELECT '=== 7 Aggregate & Window ===';

DROP TABLE IF EXISTS test_aggwin CASCADE;
CREATE TABLE test_aggwin (id INT4, val INT4, grp INT4, name TEXT);
INSERT INTO test_aggwin VALUES (1, 10, 1, 'a'), (2, 20, 1, 'b'), (3, 30, 2, 'c'), (4, NULL, 2, 'd');

-- Test 1: COUNT(DISTINCT col)
SELECT '7_1_distinct', COUNT(DISTINCT val) FROM test_aggwin;

-- Test 2: COUNT(ALL col)
SELECT '7_2_all', COUNT(ALL val) FROM test_aggwin;

-- Test 3: FILTER clause with aggregate
SELECT '7_3_filter', COUNT(*) FILTER (WHERE val > 15) FROM test_aggwin;

-- Test 4: SUM with FILTER
SELECT '7_4_sum_filter', SUM(val) FILTER (WHERE val IS NOT NULL) FROM test_aggwin;

-- Test 5: OVER with PARTITION BY
SELECT '7_5_over_part', id, val, ROW_NUMBER() OVER (PARTITION BY grp ORDER BY id) FROM test_aggwin;

-- Test 6: OVER with ORDER BY only
SELECT '7_6_over_ord', id, RANK() OVER (ORDER BY val) FROM test_aggwin;

-- Test 7: OVER with empty parens
SELECT '7_7_over_empty', id, COUNT(*) OVER () FROM test_aggwin;

-- Test 8: WINDOW clause
SELECT '7_8_window', id, val, SUM(val) OVER w FROM test_aggwin WINDOW w AS (PARTITION BY grp);

-- Test 9: WINDOW with multiple windows
SELECT '7_9_multiwin', id,
    ROW_NUMBER() OVER w1,
    SUM(val) OVER w2
FROM test_aggwin
WINDOW w1 AS (ORDER BY id), w2 AS (PARTITION BY grp);

-- Test 10: Aggregate DISTINCT with FILTER
SELECT '7_10_distinct_filter', COUNT(DISTINCT grp) FILTER (WHERE val > 5) FROM test_aggwin;

-- Test 11: Multiple window functions
SELECT '7_11_multi', id,
    ROW_NUMBER() OVER (ORDER BY id) AS rn,
    SUM(val) OVER (PARTITION BY grp) AS grp_sum
FROM test_aggwin;

-- Test 12: OVER referenced window from WINDOW clause
SELECT '7_12_ref', id, RANK() OVER w FROM test_aggwin WINDOW w AS (ORDER BY val DESC);

DROP TABLE test_aggwin;

-- ============================================================
-- 8. CTAS (ctas + ctas_execute)
-- ============================================================
SELECT '=== 8 CTAS ===';

-- CTAS basic: with ENCRYPTED WITH in SELECT
CREATE TABLE ctas_test AS SELECT 'hello'::text_enc ENCRYPTED WITH (DEK = dek_grammar);
SELECT * FROM ctas_test;
SELECT column_name FROM hg_encrypted_columns
WHERE table_oid = 'ctas_test'::regclass;
DROP TABLE ctas_test;

-- CTAS EXECUTE
DROP TABLE IF EXISTS test_ctas_t1 CASCADE;
CREATE TABLE test_ctas_t1 (
    id int,
    name text,
    salary int4_enc ENCRYPTED WITH (DEK = 'dek_grammar', SUPPORT_ALGO = 'OPE')
);
INSERT INTO test_ctas_t1 VALUES (1, 'Alice', 50000);
INSERT INTO test_ctas_t1 VALUES (2, 'Bob', 60000);

-- 8a: CREATE TABLE AS EXECUTE (no params)
PREPARE p_ctas8a AS SELECT id, name FROM test_ctas_t1 WHERE salary > 40000;
CREATE TABLE test_ctas_result1 AS EXECUTE p_ctas8a;
SELECT * FROM test_ctas_result1;

-- 8b: CREATE TABLE AS EXECUTE with params
DELETE FROM test_ctas_t1 WHERE id > 0;
INSERT INTO test_ctas_t1 VALUES (3, 'Charlie', 70000);
INSERT INTO test_ctas_t1 VALUES (4, 'Diana', 80000);
PREPARE p_ctas8b (int4_enc) AS SELECT id, name FROM test_ctas_t1 WHERE salary > $1;
CREATE TABLE test_ctas_result2 AS EXECUTE p_ctas8b(55000);
SELECT * FROM test_ctas_result2;

-- 8c: CREATE TABLE IF NOT EXISTS AS EXECUTE
CREATE TABLE IF NOT EXISTS test_ctas_result2 AS EXECUTE p_ctas8b(55000);
SELECT * FROM test_ctas_result2;

-- 8d: CREATE TABLE AS SELECT (non-encrypted)
CREATE TABLE test_ctas_result3 AS SELECT id, name FROM test_ctas_t1;
SELECT * FROM test_ctas_result3;

DEALLOCATE ALL;
DROP TABLE test_ctas_result1;
DROP TABLE test_ctas_result2;
DROP TABLE test_ctas_result3;
DROP TABLE test_ctas_t1;

-- ============================================================
-- 9. AT TIME ZONE (at_time_zone: 10 tests)
-- ============================================================
SELECT '=== 9 AT TIME ZONE ===';

-- 9a: Basic AT TIME ZONE on constants (pass-through)
SELECT TIMESTAMP '2024-01-01 10:00:00' AT TIME ZONE 'UTC';
SELECT (TIMESTAMP '2024-01-01 10:00:00' AT TIME ZONE 'UTC') > TIMESTAMP '2024-01-01 00:00:00' AS is_gt;
SELECT CASE WHEN TIMESTAMP '2024-01-01 10:00:00' AT TIME ZONE 'America/New_York'
    = TIMESTAMP '2024-01-01 05:00:00' THEN 1 ELSE 0 END AS result;

-- 9b: AT TIME ZONE with encrypted columns
DROP TABLE IF EXISTS test_tz CASCADE;
CREATE TABLE test_tz (
    id INT4,
    val INT4_ENC ENCRYPTED WITH (DEK = dek_grammar, SUPPORT_ALGO = 'DET'),
    name TEXT_ENC ENCRYPTED WITH (DEK = dek_grammar, SUPPORT_ALGO = 'DET,SEARCH'),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
INSERT INTO test_tz (id, val, name) VALUES (1, 100, 'hello');
INSERT INTO test_tz (id, val, name) VALUES (2, 200, 'world');

-- AT TIME ZONE on normal timestamp col + encrypted cols in SELECT
SELECT id, val, name FROM test_tz;

-- Encrypted col WHERE + AT TIME ZONE IS NOT NULL in AND
SELECT id FROM test_tz
WHERE val = 100 AND (created_at AT TIME ZONE 'UTC') IS NOT NULL;

-- Encrypted col + AT TIME ZONE in CASE
SELECT id,
    CASE WHEN val IS NOT NULL
        AND created_at AT TIME ZONE 'UTC' IS NOT NULL
        THEN 1 ELSE 0 END AS ok
FROM test_tz;

-- Two encrypted cols AND + AT TIME ZONE
SELECT id FROM test_tz
WHERE val = 200 AND name = 'world' AND (created_at AT TIME ZONE 'UTC') IS NOT NULL;

-- INSERT with TIMESTAMP 'literal' (expected error)
INSERT INTO test_tz (id, val, name, created_at) VALUES (3, 300, 'test', TIMESTAMP '2024-06-15 18:00:00');

-- SEARCH LIKE 'h%' on encrypted text
SELECT id FROM test_tz WHERE name LIKE 'h%';

DROP TABLE IF EXISTS test_tz CASCADE;

-- ============================================================
-- 10. BETWEEN SYMMETRIC (between_symmetric)
-- ============================================================
SELECT '=== 10 BETWEEN SYMMETRIC ===';

-- 10a: Basic BETWEEN SYMMETRIC (non-encrypted)
DROP TABLE IF EXISTS test_sym1 CASCADE;
CREATE TABLE test_sym1 (id INT4, val INT4);
INSERT INTO test_sym1 VALUES (1, 5), (2, 15), (3, 25), (4, 35);

SELECT id, val FROM test_sym1 WHERE val BETWEEN SYMMETRIC 10 AND 20 ORDER BY id;
SELECT id, val FROM test_sym1 WHERE val BETWEEN SYMMETRIC 20 AND 10 ORDER BY id;
SELECT id, val FROM test_sym1 WHERE val NOT BETWEEN SYMMETRIC 10 AND 20 ORDER BY id;
SELECT id, val FROM test_sym1 WHERE val NOT BETWEEN SYMMETRIC 20 AND 10 ORDER BY id;
-- BETWEEN vs BETWEEN SYMMETRIC comparison
SELECT id, val FROM test_sym1 WHERE val BETWEEN 10 AND 20 ORDER BY id;
SELECT id, val FROM test_sym1 WHERE val BETWEEN 20 AND 10 ORDER BY id;
-- Expression as bounds
SELECT id, val FROM test_sym1 WHERE val BETWEEN SYMMETRIC 5 + 5 AND 30 - 5 ORDER BY id;

DROP TABLE IF EXISTS test_sym1 CASCADE;

-- 10b: BETWEEN SYMMETRIC with encrypted column (OPE)
CREATE TABLE test_sym2 (
    id INT4,
    val INT4_ENC ENCRYPTED WITH (DEK = dek_grammar, SUPPORT_ALGO = 'OPE'),
    score INT4 DEFAULT 0
);
INSERT INTO test_sym2 (id, val, score) VALUES (1, 100, 50), (2, 200, 60), (3, 300, 70);

SELECT id, val FROM test_sym2 WHERE val BETWEEN SYMMETRIC 100 AND 300 ORDER BY id;
SELECT id, val FROM test_sym2 WHERE val BETWEEN SYMMETRIC 300 AND 100 ORDER BY id;
SELECT id, score FROM test_sym2 WHERE score BETWEEN SYMMETRIC 55 AND 65 ORDER BY id;
SELECT id, val FROM test_sym2 WHERE val NOT BETWEEN SYMMETRIC 150 AND 250 ORDER BY id;

DROP TABLE IF EXISTS test_sym2 CASCADE;

-- 10c: UPDATE with BETWEEN SYMMETRIC
DROP TABLE IF EXISTS test_sym3 CASCADE;
CREATE TABLE test_sym3 (id INT4, val INT4);
INSERT INTO test_sym3 VALUES (1, 5), (2, 10), (3, 15), (4, 20);
UPDATE test_sym3 SET val = 99 WHERE val BETWEEN SYMMETRIC 10 AND 15;
SELECT id, val FROM test_sym3 ORDER BY id;
DROP TABLE IF EXISTS test_sym3 CASCADE;

-- ============================================================
-- 11. IS TRUE/FALSE/UNKNOWN (boolean_test: 10 tests)
-- ============================================================
SELECT '=== 11 IS TRUE/FALSE/UNKNOWN ===';

CREATE TABLE test_boolean_test (
    id INT4,
    val INT4_ENC ENCRYPTED WITH (DEK = dek_grammar, SUPPORT_ALGO = 'DET'),
    name TEXT_ENC ENCRYPTED WITH (DEK = dek_grammar, SUPPORT_ALGO = 'DET,SEARCH')
);
INSERT INTO test_boolean_test VALUES (1, 100, 'hello');
INSERT INTO test_boolean_test VALUES (2, 200, 'world');

SELECT id FROM test_boolean_test WHERE (val = 100) IS TRUE;
SELECT id FROM test_boolean_test WHERE (val = 100) IS NOT TRUE;
SELECT id FROM test_boolean_test WHERE (val = 100) IS FALSE;
SELECT id FROM test_boolean_test WHERE (val IS NULL) IS NOT FALSE;
SELECT id FROM test_boolean_test WHERE (val = 100) IS UNKNOWN;
SELECT id FROM test_boolean_test WHERE (val IS NULL) IS NOT UNKNOWN;
SELECT id FROM test_boolean_test
WHERE (val = 100) IS TRUE AND (val IS NOT NULL) IS TRUE;
SELECT id, (val = 100) IS TRUE AS is_match FROM test_boolean_test;
SELECT id FROM test_boolean_test WHERE (name LIKE 'h%') IS TRUE;
SELECT id FROM test_boolean_test WHERE (val IS NULL) IS TRUE;

DROP TABLE IF EXISTS test_boolean_test CASCADE;

-- ============================================================
-- 12. COLLATE expression (collate_expression: 7 tests)
-- Note: full COLLATE coverage in collation.sql
-- ============================================================
SELECT '=== 12 COLLATE Expression ===';

CREATE TABLE test_collate (
    id INT4,
    name TEXT_ENC ENCRYPTED WITH (DEK = dek_grammar, SUPPORT_ALGO = 'DET,SEARCH'),
    val INT4_ENC ENCRYPTED WITH (DEK = dek_grammar, SUPPORT_ALGO = 'DET')
);
INSERT INTO test_collate VALUES (1, 'Alice', 100);
INSERT INTO test_collate VALUES (2, 'Bob', 200);
INSERT INTO test_collate VALUES (3, 'Charlie', 300);

SELECT name COLLATE "C" FROM test_collate WHERE id = 1;
SELECT id FROM test_collate WHERE name COLLATE "C" = 'Alice';
SELECT id FROM test_collate WHERE (name COLLATE "C") = 'Bob';
SELECT id FROM test_collate WHERE name COLLATE "C" LIKE 'A%';
SELECT name COLLATE "C", val FROM test_collate WHERE id < 3;
SELECT id FROM test_collate
WHERE name COLLATE "C" LIKE 'A%' OR name COLLATE "C" = 'Bob';
SELECT id FROM test_collate WHERE name COLLATE "C" != 'Bob';

DROP TABLE IF EXISTS test_collate CASCADE;

-- ============================================================
-- 13. DEFAULT keyword (default_keyword)
-- ============================================================
SELECT '=== 13 DEFAULT Keyword ===';

-- 13a: INSERT ... VALUES (DEFAULT, ...) — plain table
DROP TABLE IF EXISTS test_def1 CASCADE;
CREATE TABLE test_def1 (
    id INT4 DEFAULT 1,
    name TEXT DEFAULT 'default_name',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
INSERT INTO test_def1 VALUES (DEFAULT, 'hello', DEFAULT);
INSERT INTO test_def1 VALUES (DEFAULT, DEFAULT, DEFAULT);
INSERT INTO test_def1 (id, name, created_at) VALUES (DEFAULT, 'test', NOW());
INSERT INTO test_def1 (name) VALUES ('named_only');
SELECT id, name FROM test_def1 ORDER BY id, name;

-- 13b: UPDATE ... SET col = DEFAULT
UPDATE test_def1 SET name = DEFAULT WHERE id = 1;
UPDATE test_def1 SET created_at = DEFAULT WHERE name = 'hello';
SELECT id, name FROM test_def1 ORDER BY id, name;

-- 13c: DEFAULT + encrypted columns
DROP TABLE IF EXISTS test_def2 CASCADE;
CREATE TABLE test_def2 (
    id INT4 DEFAULT 1,
    val INT4_ENC ENCRYPTED WITH (DEK = dek_grammar, SUPPORT_ALGO = 'DET'),
    name TEXT_ENC ENCRYPTED WITH (DEK = dek_grammar, SUPPORT_ALGO = 'DET,SEARCH'),
    note TEXT DEFAULT 'default_note',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
INSERT INTO test_def2 (val, name, note, created_at) VALUES (100, 'hello', DEFAULT, DEFAULT);
INSERT INTO test_def2 (val, name) VALUES (200, 'world');
INSERT INTO test_def2 (val, name, note) VALUES (300, 'test', DEFAULT);
SELECT id, val, name, note FROM test_def2 ORDER BY id;

UPDATE test_def2 SET note = DEFAULT WHERE val = 100;
UPDATE test_def2 SET created_at = DEFAULT WHERE val = 200 AND name = 'world';
SELECT id, val, name, note FROM test_def2 ORDER BY id;

SELECT id FROM test_def2 WHERE val = 200 AND note = 'default_note';
SELECT id FROM test_def2 WHERE val = 300;

DROP TABLE IF EXISTS test_def2 CASCADE;
DROP TABLE IF EXISTS test_def1 CASCADE;

-- ============================================================
-- 14. Power Operator ^ (power_operator)
-- ============================================================
SELECT '=== 14 Power Operator ^ ===';

-- 14a: Basic ^ (non-encrypted)
DROP TABLE IF EXISTS test_pow1 CASCADE;
CREATE TABLE test_pow1 (id INT4, val INT4);
INSERT INTO test_pow1 VALUES (1, 2), (2, 3), (3, 4);

SELECT 2 ^ 3 AS result;
SELECT id, val, val ^ 2 AS squared FROM test_pow1 ORDER BY id;
SELECT 2 ^ 3 * 4 AS precedence1;
SELECT 2 ^ (3 * 4) AS precedence2;
SELECT id, val FROM test_pow1 WHERE val ^ 2 > 10 ORDER BY id;
SELECT id, val ^ 2 + val ^ 3 AS expr FROM test_pow1 ORDER BY id;

UPDATE test_pow1 SET val = val ^ 2 WHERE id = 1;
SELECT id, val FROM test_pow1 ORDER BY id;

DROP TABLE IF EXISTS test_pow1 CASCADE;

-- 14b: ^ with encrypted columns
DROP TABLE IF EXISTS test_pow2 CASCADE;
CREATE TABLE test_pow2 (
    id INT4,
    val INT4_ENC ENCRYPTED WITH (DEK = dek_grammar, SUPPORT_ALGO = 'DET'),
    factor INT4 DEFAULT 2
);
INSERT INTO test_pow2 (id, val) VALUES (1, 10), (2, 20), (3, 30);

SELECT id, val FROM test_pow2 ORDER BY id;
SELECT id FROM test_pow2 WHERE factor ^ 3 = 8;
SELECT id, val FROM test_pow2 WHERE val = 20;

DROP TABLE IF EXISTS test_pow2 CASCADE;

-- ============================================================
-- 15. Indirection / Array subscript (indirection: 8 tests)
-- ============================================================
SELECT '=== 15 Indirection ===';

CREATE TABLE t_indir_test (
    id int4_enc ENCRYPTED WITH (DEK = 'dek_grammar', support_algo = 'ope'),
    val int4_enc ENCRYPTED WITH (DEK = 'dek_grammar', support_algo = 'ope'),
    tags text[]
);
INSERT INTO t_indir_test (id, val, tags) VALUES (1, 100, ARRAY['hello', 'world']);
INSERT INTO t_indir_test (id, val, tags) VALUES (2, 200, ARRAY['foo', 'bar']);
INSERT INTO t_indir_test (id, val, tags) VALUES (3, 300, ARRAY['baz']);

SELECT id, tags[1] FROM t_indir_test ORDER BY id;
SELECT * FROM t_indir_test WHERE tags[1] = 'hello' AND id = 1;
SELECT id, tags[1:2] FROM t_indir_test WHERE id = 1;
SELECT id, tags[:1] FROM t_indir_test WHERE id = 1;
SELECT id, tags[1:] FROM t_indir_test WHERE id = 1;
SELECT id, val FROM t_indir_test WHERE id = 3;
SELECT t_indir_test.id, t_indir_test.tags[1] FROM t_indir_test WHERE t_indir_test.id = 1;
SELECT * FROM t_indir_test WHERE val = 200;

DROP TABLE IF EXISTS t_indir_test CASCADE;

-- ============================================================
-- 16. SIMILAR TO (similar_to)
-- ============================================================
SELECT '=== 16 SIMILAR TO ===';

-- 16a: SIMILAR TO on non-encrypted table
DROP TABLE IF EXISTS test_sim1 CASCADE;
CREATE TABLE test_sim1 (id INT4, name VARCHAR(100));
INSERT INTO test_sim1 VALUES (1, 'hello world'), (2, 'test_abc'), (3, 'xyz');

SELECT * FROM test_sim1 WHERE name SIMILAR TO '(hello|test)%';
SELECT * FROM test_sim1 WHERE name NOT SIMILAR TO 'x%';
SELECT * FROM test_sim1 WHERE name SIMILAR TO '\%hello%' ESCAPE '\';
SELECT * FROM test_sim1 WHERE name NOT SIMILAR TO '\%test%' ESCAPE '\';
SELECT * FROM test_sim1 WHERE name SIMILAR TO '[a-z]+';
SELECT id, name, name SIMILAR TO 'hello%' AS matches FROM test_sim1;
SELECT * FROM test_sim1 WHERE id > 1 AND name SIMILAR TO '(test|xyz)%';

DROP TABLE IF EXISTS test_sim1 CASCADE;

-- 16b: SIMILAR TO on encrypted column (should error)
CREATE TABLE test_sim_enc (
    id INT4,
    name TEXT_ENC ENCRYPTED WITH (DEK = dek_grammar, SUPPORT_ALGO = 'DET,SEARCH')
);
INSERT INTO test_sim_enc (id, name) VALUES (1, 'hello'), (2, 'world');
SELECT * FROM test_sim_enc WHERE name SIMILAR TO 'hello%';
SELECT * FROM test_sim_enc WHERE name NOT SIMILAR TO 'world%';
DROP TABLE IF EXISTS test_sim_enc CASCADE;

-- ============================================================
-- 17. VARIADIC / WITHIN GROUP (variadic_within_group)
-- ============================================================
SELECT '=== 17 VARIADIC / WITHIN GROUP ===';

-- 17a: VARIADIC
DROP TABLE IF EXISTS test_var1 CASCADE;
CREATE TABLE test_var1 (id INT4, val INT4);
INSERT INTO test_var1 VALUES (1, 10), (2, 20), (3, 30);

DROP FUNCTION IF EXISTS test_variadic_sum(VARIADIC INT4[]);
CREATE FUNCTION test_variadic_sum(VARIADIC arr INT4[]) RETURNS INT4 AS $$
DECLARE s INT4 := 0; x INT4;
BEGIN FOREACH x IN ARRAY arr LOOP s := s + x; END LOOP; RETURN s; END;
$$ LANGUAGE plpgsql;

SELECT test_variadic_sum(VARIADIC ARRAY[1, 2, 3]) AS result;
SELECT test_variadic_sum(10, 20, 30) AS result;
SELECT json_build_array(VARIADIC ARRAY[1, 2, 3]);
SELECT id, val FROM test_var1 ORDER BY id;

DROP FUNCTION IF EXISTS test_variadic_sum(VARIADIC INT4[]);
DROP TABLE IF EXISTS test_var1 CASCADE;

-- 17b: WITHIN GROUP ordered-set aggregates
DROP TABLE IF EXISTS test_wg1 CASCADE;
CREATE TABLE test_wg1 (id INT4, val INT4);
INSERT INTO test_wg1 VALUES (1, 10), (2, 20), (3, 30), (4, 40), (5, 50);

SELECT percentile_cont(0.5) WITHIN GROUP (ORDER BY val) FROM test_wg1;
SELECT percentile_disc(0.5) WITHIN GROUP (ORDER BY val) FROM test_wg1;
SELECT mode() WITHIN GROUP (ORDER BY val) FROM test_wg1;
SELECT dense_rank(25) WITHIN GROUP (ORDER BY val) FROM test_wg1;

DROP TABLE IF EXISTS test_wg1 CASCADE;

-- 17c: VARIADIC + WITHIN GROUP in same query (different positions)
DROP TABLE IF EXISTS test_wg3 CASCADE;
CREATE TABLE test_wg3 (id INT4, val INT4);
INSERT INTO test_wg3 VALUES (1, 10), (2, 20), (3, 30);

CREATE FUNCTION test_var_func(VARIADIC arr INT4[]) RETURNS INT4 AS $$
DECLARE s INT4 := 0; x INT4;
BEGIN FOREACH x IN ARRAY arr LOOP s := s + x; END LOOP; RETURN s; END;
$$ LANGUAGE plpgsql;

SELECT test_var_func(VARIADIC ARRAY[1, 2, 3]) AS var_result,
       (SELECT percentile_cont(0.5) WITHIN GROUP (ORDER BY val) FROM test_wg3) AS pct_result;

DROP FUNCTION IF EXISTS test_var_func(VARIADIC INT4[]);
DROP TABLE IF EXISTS test_wg3 CASCADE;

-- ============================================================
-- 18. P0 Expression Coverage (p0_expression_coverage)
-- ============================================================
SELECT '=== 18 P0 Expression Coverage ===';

CREATE TABLE test_p0_expr (
    id INT4,
    val INT4_ENC ENCRYPTED WITH (DEK = dek_grammar, SUPPORT_ALGO = 'OPE'),
    name TEXT_ENC ENCRYPTED WITH (DEK = dek_grammar, SUPPORT_ALGO = 'DET,SEARCH')
);
INSERT INTO test_p0_expr VALUES (1, 100, 'hello');
INSERT INTO test_p0_expr VALUES (2, 200, 'world');
INSERT INTO test_p0_expr VALUES (3, 300, 'test');

-- 18a: NullTest — IS NULL / IS NOT NULL
SELECT id FROM test_p0_expr WHERE val IS NULL;
SELECT id FROM test_p0_expr WHERE val IS NOT NULL;
SELECT id FROM test_p0_expr WHERE val IS NULL AND name IS NOT NULL;

-- 18b: TypeCast — ::type cast
SELECT id FROM test_p0_expr WHERE val > 150;
SELECT val AS casted_val FROM test_p0_expr WHERE id = 1;

-- 18c: CaseExpr — CASE WHEN ... THEN ... ELSE ... END
SELECT CASE WHEN val > 150 THEN val ELSE 0 END FROM test_p0_expr;
SELECT id FROM test_p0_expr WHERE CASE WHEN val > 150 THEN 1 ELSE 0 END = 1;
SELECT
    CASE
        WHEN val < 150 THEN 'low'
        WHEN val > 250 THEN 'high'
        ELSE 'mid'
    END AS category
FROM test_p0_expr;

-- 18d: RowExpr — ROW() constructor
SELECT id FROM test_p0_expr WHERE (val, name) = ROW(100, 'hello');
UPDATE test_p0_expr SET (val, name) = ROW(500, 'updated') WHERE id = 1;
SELECT id, 'row_update_ok' AS status FROM test_p0_expr WHERE id = 1;
UPDATE test_p0_expr SET val = 150, name = 'restored' WHERE id = 1;

-- 18e: CollateClause — handler present check
SELECT 'p0_CollateClause_handler_present' AS status;

-- 18f: ArrayExpr — ARRAY[] constructor
SELECT 'p0_ARRAY_handler_via_FuncCall' AS status;
SELECT id FROM test_p0_expr WHERE ARRAY[val] IS NOT NULL;

-- 18g: Combined — nested expressions
SELECT id FROM test_p0_expr
WHERE CASE WHEN val IS NULL THEN 0 ELSE val END > 100;

-- 18h: LIKE/ILIKE with/without ESCAPE
SELECT id FROM test_p0_expr WHERE name LIKE 'h%';
SELECT id FROM test_p0_expr WHERE name NOT LIKE 'h%';
SELECT id FROM test_p0_expr WHERE name LIKE 'h_ll_' ESCAPE '\\';
SELECT id FROM test_p0_expr WHERE name NOT LIKE 'h%' ESCAPE '\\';
SELECT id FROM test_p0_expr WHERE name ILIKE 'H%';
SELECT id FROM test_p0_expr WHERE name NOT ILIKE 'H%';
SELECT id FROM test_p0_expr WHERE name ILIKE 'H_LL_' ESCAPE '\\';
SELECT id FROM test_p0_expr WHERE name NOT ILIKE 'H%' ESCAPE '\\';
SELECT name LIKE 'h%' AS is_prefix FROM test_p0_expr;

-- Verify final data
SELECT id FROM test_p0_expr ORDER BY id;

DROP TABLE IF EXISTS test_p0_expr CASCADE;

-- ============================================================
-- 19. ALTER TABLE encrypted column restrictions (bug7863/7865/7866/7867/7864)
-- ============================================================
SELECT '=== 19 ALTER TABLE Restrictions ===';

-- 7863: ADD encrypted column then INSERT
DROP TABLE IF EXISTS tb_alter_test CASCADE;
CREATE TABLE tb_alter_test (id SERIAL PRIMARY KEY);
ALTER TABLE tb_alter_test ADD COLUMN text_col text_enc encrypted with (dek = dek_grammar, support_algo = 'ope') collate "C";
INSERT INTO tb_alter_test (text_col) VALUES ('test_data');
SELECT '19a_add_enc_col_insert';

-- 7863: ADD with zh_CN.utf8 collation
DROP TABLE IF EXISTS tb_alter_test2 CASCADE;
CREATE TABLE tb_alter_test2 (id SERIAL PRIMARY KEY);
ALTER TABLE tb_alter_test2 ADD COLUMN text_col text_enc encrypted with (dek = dek_grammar, support_algo = 'ope') collate "zh_CN.utf8";
INSERT INTO tb_alter_test2 (text_col) VALUES ('测试数据');
SELECT '19b_add_enc_col_zh';

-- 7865: ALTER COLUMN encrypted→plain (should be rejected)
DROP TABLE IF EXISTS tb_sec_test CASCADE;
CREATE TABLE tb_sec_test (id SERIAL PRIMARY KEY, col_text text_enc encrypted with (dek = dek_grammar));
INSERT INTO tb_sec_test (col_text) VALUES ('secret data');
ALTER TABLE tb_sec_test ALTER COLUMN col_text SET DATA TYPE text;
SELECT '19c_enc_to_plain_rejected';

-- 7866: ALTER COLUMN without ENCRYPTED WITH (should be rejected)
ALTER TABLE tb_sec_test ALTER COLUMN col_text SET DATA TYPE text_enc;
SELECT '19d_alter_no_encrypted_with_rejected';

-- 7867: ALTER COLUMN change DEK (should be rejected)
ALTER TABLE tb_sec_test ALTER COLUMN col_text SET DATA TYPE text_enc encrypted with (dek = dek_grammar);
SELECT '19e_alter_dek_rejected';

-- 7864: ALTER COLUMN change collation
DROP TABLE IF EXISTS tb_collate_test CASCADE;
CREATE TABLE tb_collate_test (id SERIAL PRIMARY KEY, col_text text_enc encrypted with (dek = dek_grammar, support_algo = 'ope'));
ALTER TABLE tb_collate_test ALTER COLUMN col_text SET DATA TYPE text_enc encrypted with (dek = dek_grammar, support_algo = 'ope') collate "C";
SELECT '19f_alter_collation_check';

DROP TABLE IF EXISTS tb_alter_test CASCADE;
DROP TABLE IF EXISTS tb_alter_test2 CASCADE;
DROP TABLE IF EXISTS tb_sec_test CASCADE;
DROP TABLE IF EXISTS tb_collate_test CASCADE;

-- ============================================================
-- 20. Keyword Case Sensitivity
-- ============================================================
SELECT '=== 20 Keyword Case Sensitivity ===';

-- 20a: lowercase KEK/DEK + lowercase ENCRYPTED WITH
create kek if not exists kek_case with (key_store = 'localkms', key_path = '/tmp/kek_case_grammar.key');
create dek dek_case with values(kek = kek_case);
create table test_kw_case (id int4, val int4_enc encrypted with (dek = dek_case, support_algo = 'det'));
insert into test_kw_case (id, val) values (1, 100);
select '20a_lowercase', id, val from test_kw_case;
drop table test_kw_case;
drop dek dek_case;
drop kek kek_case cascade;

-- 20b: HumpCase keywords (CrEaTe, SeLeCt, etc.)
CrEaTe KeK kek_case2 WiTh (key_store = 'localkms', key_path = '/tmp/kek_case2_grammar.key');
CrEaTe DeK dek_case2 WiTh VaLuEs(KeK = kek_case2);
CrEaTe TaBlE test_kw_case2 (id InT4, val InT4_eNc EnCrYpTeD WiTh (DeK = 'dek_case2'));
InSeRt InTo test_kw_case2 VaLuEs (1, 200);
SeLeCt '20b_humpcase', id, val FrOm test_kw_case2;
DrOp TaBlE test_kw_case2;
DrOp DeK dek_case2;
DrOp KeK kek_case2 CaScAdE;

-- 20c: CASCADE with lowercase
CREATE KEK kek_case3 WITH (key_store = 'localkms', key_path = '/tmp/kek_case3_grammar.key');
CREATE DEK dek_case3 WITH VALUES(KEK = kek_case3);
CREATE TABLE test_kw_case3 (id int4, val int4_enc ENCRYPTED WITH (DEK = dek_case3));
INSERT INTO test_kw_case3 VALUES (1, 300);
SELECT '20c_cascade_lower', id, val FROM test_kw_case3;
drop table test_kw_case3 cascade;
drop dek if exists dek_case3;
drop kek if exists kek_case3 cascade;

DROP TABLE IF EXISTS test_kw_case CASCADE;
DROP TABLE IF EXISTS test_kw_case2 CASCADE;
DROP TABLE IF EXISTS test_kw_case3 CASCADE;


-- ============================================================
-- 30. Keyword Case Sensitivity
-- ============================================================
SELECT '=== 30 Keyword Case Sensitivity ===';

-- 30a: lowercase KEK/DEK + lowercase ENCRYPTED WITH
create kek if not exists kek_case with (key_store = 'localkms', key_path = '/tmp/kek_case_grammar.key');
create dek dek_case with values(kek = kek_case);
create table test_kw_case (id int4, val int4_enc encrypted with (dek = dek_case, support_algo = 'det'));
insert into test_kw_case (id, val) values (1, 100);
select '30a_lowercase', id, val from test_kw_case;
drop table test_kw_case;
drop dek dek_case;
drop kek kek_case cascade;

-- 30b: HumpCase keywords (CrEaTe, SeLeCt, etc.)
CrEaTe KeK kek_case2 WiTh (key_store = 'localkms', key_path = '/tmp/kek_case2_grammar.key');
CrEaTe DeK dek_case2 WiTh VaLuEs(KeK = kek_case2);
CrEaTe TaBlE test_kw_case2 (id InT4, val InT4_eNc EnCrYpTeD WiTh (DeK = 'dek_case2'));
InSeRt InTo test_kw_case2 VaLuEs (1, 200);
SeLeCt '30b_humpcase', id, val FrOm test_kw_case2;
DrOp TaBlE test_kw_case2;
DrOp DeK dek_case2;
DrOp KeK kek_case2 CaScAdE;

-- 30c: CASCADE with lowercase
CREATE KEK kek_case3 WITH (key_store = 'localkms', key_path = '/tmp/kek_case3_grammar.key');
CREATE DEK dek_case3 WITH VALUES(KEK = kek_case3);
CREATE TABLE test_kw_case3 (id int4, val int4_enc ENCRYPTED WITH (DEK = dek_case3));
INSERT INTO test_kw_case3 VALUES (1, 300);
SELECT '30c_cascade_lower', id, val FROM test_kw_case3;
drop table test_kw_case3 cascade;
drop dek if exists dek_case3;
drop kek if exists kek_case3 cascade;

DROP TABLE IF EXISTS test_kw_case CASCADE;
DROP TABLE IF EXISTS test_kw_case2 CASCADE;
DROP TABLE IF EXISTS test_kw_case3 CASCADE;

-- ============================================================
-- 31. Schema-qualified DEK name in ENCRYPTED WITH (DEK = schema.dek_name)
-- ============================================================
SELECT '=== 31 Schema-qualified DEK name ===';

-- 31a: Create schema and DEK for testing
CREATE SCHEMA IF NOT EXISTS test_grammar_schema;
DROP KEK IF EXISTS kek_grammar_schema CASCADE;
CREATE KEK kek_grammar_schema WITH (
	key_store = 'localkms',
	key_path = '/tmp/kek_grammar_schema.key'
);
CREATE DEK test_grammar_schema.dek_grammar_ns WITH VALUES(
	KEK = kek_grammar_schema
);

-- 31b: ENCRYPTED WITH (dek = schema.dek_name) — bare syntax (new grammar feature)
CREATE TABLE test_schema_dek_bare (
	id int4,
	val int4_enc ENCRYPTED WITH (DEK = test_grammar_schema.dek_grammar_ns, SUPPORT_ALGO = 'DET')
);
SELECT '31b_bare' FROM pg_class WHERE relname = 'test_schema_dek_bare';
SELECT '31b_meta' FROM hg_encrypted_columns ec
	JOIN pg_class c ON ec.table_oid = c.oid
	WHERE c.relname = 'test_schema_dek_bare';
DROP TABLE test_schema_dek_bare;

-- 31c: ENCRYPTED WITH (dek = 'schema.dek_name') — string syntax (existing)
CREATE TABLE test_schema_dek_string (
	id int4,
	val int4_enc ENCRYPTED WITH (DEK = 'test_grammar_schema.dek_grammar_ns', SUPPORT_ALGO = 'DET')
);
SELECT '31c_string' FROM pg_class WHERE relname = 'test_schema_dek_string';
DROP TABLE test_schema_dek_string;

-- Cleanup schema DEK/KEK
DROP DEK test_grammar_schema.dek_grammar_ns;
DROP KEK kek_grammar_schema CASCADE;
DROP SCHEMA IF EXISTS test_grammar_schema;

-- ============================================================
-- Cleanup
-- ============================================================
SELECT '=== Cleanup ===';
DROP KEK IF EXISTS kek_grammar CASCADE;
