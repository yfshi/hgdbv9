-- ============================================================
-- express.sql — 表达式原语/布尔/子查询/聚合窗口测试
-- 覆盖 test_parser_expr_prim, test_parser_bool_expr,
--   test_bool_expr, test_enc_values_cte,
--   test_taskA/B/C/D_encrypted, test_regression_ope_coalesce
-- ============================================================

-- ============================================================
-- 0. Setup — 共享KEK/DEK
-- ============================================================
DROP KEK IF EXISTS kek_expr CASCADE;

CREATE KEK kek_expr WITH (
    key_store = 'localkms',
    key_path = '/tmp/kek_expr.key'
);
CREATE DEK dek_expr WITH VALUES(KEK = kek_expr);

-- ============================================================
-- 1. Expression primitives — non-encrypted (parser_expr_prim)
-- ============================================================
SELECT '=== 1 Expr Primitives (plain) ===';

DROP TABLE IF EXISTS test_expr CASCADE;
CREATE TABLE test_expr (id INT4, val INT4, dt TIMESTAMP);
INSERT INTO test_expr VALUES (1, 10, '2025-01-01'), (2, NULL, '2025-06-01'), (3, 30, '2025-12-01');

SELECT '1a_coalesce', id, COALESCE(val, 0) FROM test_expr;
SELECT '1b_nullif', id, NULLIF(val, 10) FROM test_expr;
SELECT '1c_greatest', GREATEST(id, val) FROM test_expr WHERE id = 3;
SELECT '1d_least', LEAST(id, val) FROM test_expr WHERE id = 3;
SELECT '1e_array', ARRAY[id, val] FROM test_expr WHERE id = 1;
SELECT '1f_current_date', id FROM test_expr WHERE id = 1;
SELECT '1g_current_ts', id FROM test_expr WHERE CURRENT_TIMESTAMP > dt;
SELECT '1h_coalesce_multi', COALESCE(NULL, NULL, 1, NULL);

DROP TABLE IF EXISTS test_expr CASCADE;

-- ============================================================
-- 2. Boolean operators — non-encrypted (parser_bool_expr)
-- ============================================================
SELECT '=== 2 Boolean Operators (plain) ===';

DROP TABLE IF EXISTS test_bool CASCADE;
CREATE TABLE test_bool (id INT4, val INT4);
INSERT INTO test_bool VALUES (1, 10), (2, 20), (3, 30);

SELECT '2a_not_like_str', * FROM test_bool WHERE 'abc' NOT LIKE 'def%';
SELECT '2b_ilike', * FROM test_bool WHERE 'ABC' ILIKE 'abc';
SELECT '2c_not_ilike', * FROM test_bool WHERE 'ABC' NOT ILIKE 'xyz%';
SELECT '2d_not_between', * FROM test_bool WHERE id NOT BETWEEN 2 AND 3;
SELECT '2e_not_in', * FROM test_bool WHERE id NOT IN (2, 3);
SELECT '2f_is_distinct', * FROM test_bool WHERE val IS DISTINCT FROM 20;
SELECT '2g_not_distinct', * FROM test_bool WHERE val IS NOT DISTINCT FROM 20;
SELECT '2h_combined', * FROM test_bool WHERE id NOT BETWEEN 1 AND 2 AND val IS DISTINCT FROM 20;

DROP TABLE IF EXISTS test_bool CASCADE;

-- ============================================================
-- 3. Bool expressions with encrypted cols (taskB)
-- ============================================================
SELECT '=== 3 Bool + Encrypted ===';

CREATE TABLE test_bool_emp (
    id INT4,
    name_enc VARCHAR_ENC(100) ENCRYPTED WITH (DEK = dek_expr, SUPPORT_ALGO = 'SEARCH'),
    salary_enc INT4_ENC ENCRYPTED WITH (DEK = dek_expr, SUPPORT_ALGO = 'OPE'),
    dept VARCHAR(50)
);

INSERT INTO test_bool_emp VALUES (1, 'Alice', 50000, 'Engineering');
INSERT INTO test_bool_emp VALUES (2, 'Bob', 60000, 'Sales');
INSERT INTO test_bool_emp VALUES (3, 'Charlie', 70000, 'Engineering');
INSERT INTO test_bool_emp VALUES (4, 'Diana', NULL, 'Marketing');

SELECT '3a_like', id, name_enc FROM test_bool_emp WHERE name_enc LIKE 'A%';
SELECT '3b_not_like', id, name_enc FROM test_bool_emp WHERE name_enc NOT LIKE 'B%';
SELECT '3c_distinct', id, name_enc FROM test_bool_emp WHERE salary_enc IS DISTINCT FROM 60000;
SELECT '3d_not_distinct', id, name_enc FROM test_bool_emp WHERE salary_enc IS NOT DISTINCT FROM 50000;
SELECT '3e_not_between', id, name_enc FROM test_bool_emp WHERE salary_enc NOT BETWEEN 55000 AND 75000;
SELECT '3f_not_in', id, name_enc FROM test_bool_emp WHERE salary_enc NOT IN (60000, 70000);
SELECT '3g_distinct_null', id, name_enc FROM test_bool_emp WHERE salary_enc IS DISTINCT FROM NULL;
SELECT '3h_combined_bool', id, name_enc FROM test_bool_emp
    WHERE salary_enc NOT BETWEEN 55000 AND 75000 AND salary_enc IS DISTINCT FROM 50000;

DROP TABLE IF EXISTS test_bool_emp CASCADE;

-- ============================================================
-- 4. SubLink with encrypted cols (taskA)
-- ============================================================
SELECT '=== 4 SubLink + Encrypted ===';

CREATE TABLE test_sublink_emp (
    id INT4, name VARCHAR(100),
    salary INT4_ENC ENCRYPTED WITH (DEK = dek_expr, SUPPORT_ALGO = 'OPE'),
    dept_id INT4
);
CREATE TABLE test_sublink_dept (dept_id INT4, budget INT4);

INSERT INTO test_sublink_emp VALUES (1, 'Alice', 50000, 1), (2, 'Bob', 60000, 1),
    (3, 'Charlie', 70000, 2), (4, 'Diana', 80000, 2);
INSERT INTO test_sublink_dept VALUES (1, 150000), (2, 200000);

SELECT '4a_exists', * FROM test_sublink_emp e
WHERE EXISTS (SELECT 1 FROM test_sublink_dept d WHERE d.dept_id = e.dept_id AND e.salary > 55000);

SELECT '4b_in', * FROM test_sublink_emp WHERE salary IN (50000, 60000, 70000);

SELECT '4c_not_in', id, name FROM test_sublink_emp
WHERE dept_id NOT IN (SELECT dept_id FROM test_sublink_dept WHERE budget < 100000);

SELECT '4d_any_eq', id, name FROM test_sublink_emp
WHERE salary = ANY (SELECT 55000 UNION SELECT 65000 UNION SELECT 75000);

SELECT '4e_all_gt', id, name FROM test_sublink_emp
WHERE salary > ALL (SELECT 50000 UNION SELECT 55000);

SELECT '4f_scalar', id, name,
    (SELECT budget FROM test_sublink_dept d WHERE d.dept_id = e.dept_id) AS dept_budget
FROM test_sublink_emp e WHERE e.salary > 55000;

SELECT '4g_exists_null', id, name FROM test_sublink_emp e
WHERE EXISTS (SELECT 1 FROM test_sublink_dept d WHERE d.dept_id = e.dept_id) AND e.salary IS NOT NULL;

DROP TABLE test_sublink_emp CASCADE;
DROP TABLE test_sublink_dept CASCADE;

-- ============================================================
-- 5. Expr primitives with encrypted cols (taskC)
-- ============================================================
SELECT '=== 5 Expr Primitives + Encrypted ===';

CREATE TABLE test_expr_emp (
    id INT4, name VARCHAR(100),
    salary INT4_ENC ENCRYPTED WITH (DEK = dek_expr, SUPPORT_ALGO = 'OPE'),
    bonus INT4_ENC ENCRYPTED WITH (DEK = dek_expr, SUPPORT_ALGO = 'OPE'),
    dept VARCHAR(50)
);

INSERT INTO test_expr_emp VALUES
(1, 'Alice', 50000, 5000, 'Engineering'),
(2, 'Bob', NULL, 8000, 'Sales'),
(3, 'Charlie', 70000, NULL, 'Engineering'),
(4, 'Diana', NULL, NULL, 'Marketing');

SELECT '5a_coalesce', id, name, COALESCE(salary, 0) FROM test_expr_emp;
SELECT '5b_coalesce_multi', id, name, COALESCE(salary, bonus, 0) FROM test_expr_emp;
SELECT '5c_nullif', id, name, NULLIF(salary, 50000) FROM test_expr_emp;
SELECT '5d_nullif_col', id, name, NULLIF(salary, bonus) FROM test_expr_emp;
SELECT '5e_greatest', id, name, GREATEST(salary, 55000) FROM test_expr_emp;
SELECT '5f_least', id, name, LEAST(salary, 65000) FROM test_expr_emp;
SELECT '5g_array', ARRAY[salary, bonus] FROM test_expr_emp WHERE salary IS NOT NULL AND bonus IS NOT NULL;
SELECT '5h_coalesce_where', id, name FROM test_expr_emp WHERE COALESCE(salary, 0) > 10000;
SELECT '5i_current_date', id, name FROM test_expr_emp WHERE salary IS NOT NULL AND CURRENT_DATE IS NOT NULL;

DROP TABLE test_expr_emp CASCADE;

-- ============================================================
-- 6. Aggregate/Window with encrypted cols (taskD)
-- ============================================================
SELECT '=== 6 Aggregate/Window + Encrypted ===';

CREATE TABLE test_aggwin_emp (
    id INT4, name VARCHAR(100),
    salary INT4_ENC ENCRYPTED WITH (DEK = dek_expr, SUPPORT_ALGO = 'ope'),
    dept VARCHAR(50)
);

INSERT INTO test_aggwin_emp VALUES
(1, 'Alice', 50000, 'Engineering'),
(2, 'Bob', 60000, 'Sales'),
(3, 'Charlie', 70000, 'Engineering'),
(4, 'Diana', 80000, 'Engineering'),
(5, 'Eve', NULL, 'Sales');

SELECT '6a_count_distinct', COUNT(DISTINCT salary) FROM test_aggwin_emp;
SELECT '6b_filter', COUNT(*) FILTER (WHERE salary > 55000) FROM test_aggwin_emp;
SELECT '6c_over_part', id, dept, salary, ROW_NUMBER() OVER (PARTITION BY dept ORDER BY salary) FROM test_aggwin_emp WHERE salary IS NOT NULL;
SELECT '6d_over_ord', id, name, salary, RANK() OVER (ORDER BY salary DESC NULLS LAST) FROM test_aggwin_emp;
SELECT '6e_over_empty', id, name, salary, COUNT(*) OVER () AS total_count FROM test_aggwin_emp WHERE salary IS NOT NULL;
SELECT '6f_distinct_filter', COUNT(DISTINCT dept) FILTER (WHERE salary > 55000) FROM test_aggwin_emp;
SELECT '6g_agg_order', STRING_AGG(name, ',' ORDER BY salary) FROM test_aggwin_emp;

DROP TABLE test_aggwin_emp CASCADE;

-- ============================================================
-- 7. VALUES in FROM + WITH RECURSIVE + encrypted (enc_values_cte)
-- ============================================================
SELECT '=== 7 VALUES/CTE + Encrypted ===';

CREATE TABLE test_ev_t (
    id INT4, name VARCHAR(100),
    salary INT4_ENC ENCRYPTED WITH (DEK = dek_expr, SUPPORT_ALGO = 'ope'),
    dept VARCHAR(50)
);
INSERT INTO test_ev_t VALUES (1, 'Alice', 50000, 'IT'), (2, 'Bob', 60000, 'HR'),
    (3, 'Charlie', 70000, 'IT'), (4, 'Diana', 80000, 'HR');

-- VALUES in FROM
SELECT '7a_const_star', * FROM (VALUES (1, 'hello'), (2, 'world')) AS v(id, msg);
SELECT '7b_where', * FROM (VALUES (10, 'a'), (20, 'b'), (30, 'c')) AS v(n, s) WHERE n > 15;

-- INSERT INTO encrypted + SELECT FROM VALUES
CREATE TABLE test_ev_t2 (label TEXT, id INT4, name VARCHAR(100),
    salary INT4_ENC ENCRYPTED WITH (DEK = dek_expr, SUPPORT_ALGO = 'ope'), dept VARCHAR(50));
INSERT INTO test_ev_t2 SELECT '7c_ins', v.id, 'from_vals', v.sal, 'IT'
FROM (VALUES (5, 55000), (6, 65000)) AS v(id, sal);
SELECT '7c_check', id, name, salary FROM test_ev_t2 ORDER BY id;

-- INSERT encrypted + SELECT const+star + FROM encrypted table
INSERT INTO test_ev_t2 SELECT '7d_star_ins', * FROM test_ev_t WHERE salary > 55000;
SELECT '7d_check', label, id, salary FROM test_ev_t2 ORDER BY id;

-- VALUES subquery in WHERE with encrypted col
SELECT '7e_sub', * FROM test_ev_t WHERE salary = (SELECT 50000);

-- VALUES + UNION ALL + encrypted SELECT
SELECT '7f_union_val', v.n FROM (VALUES (1), (2), (3)) AS v(n)
UNION ALL SELECT '7f_union_val', id FROM test_ev_t WHERE id <= 3;

-- VALUES + ORDER BY
SELECT '7g_order', * FROM (VALUES (3, 'c'), (1, 'a'), (2, 'b')) AS v(n, s) ORDER BY n;

-- VALUES nested + JOIN
SELECT '7h_nested', * FROM
    (SELECT n, s FROM (VALUES (1, 'x'), (2, 'y')) AS inner_v(n, s)) AS mid ORDER BY n;

-- EXPLAIN + FROM VALUES
EXPLAIN SELECT * FROM (VALUES (1, 'test'), (2, 'test2')) AS v(a, b) WHERE a = 1;

-- WITH RECURSIVE basic
WITH RECURSIVE r1(n) AS (
    SELECT 1 UNION ALL SELECT n + 1 FROM r1 WHERE n < 4
) SELECT '7r_basic', n FROM r1;

-- WITH RECURSIVE + const+star
WITH RECURSIVE r2(n) AS (
    SELECT 1 UNION ALL SELECT n + 1 FROM r2 WHERE n < 3
) SELECT '7r_const_star', 999, * FROM r2;

-- WITH RECURSIVE + encrypted table ref + constant WHERE
WITH RECURSIVE r3(id, n) AS (
    SELECT id, 1 FROM test_ev_t WHERE id = 1
    UNION ALL
    SELECT id, n + 1 FROM r3 WHERE n < 4
) SELECT '7r_rec_enc', * FROM r3 ORDER BY n;

-- WITH RECURSIVE + COALESCE + encrypted
WITH RECURSIVE r5(n) AS (
    SELECT 1 UNION ALL SELECT n + 1 FROM r5 WHERE n < 4
) SELECT '7r_coalesce', n, COALESCE(t.salary, 0) AS sal
FROM r5 JOIN test_ev_t t ON t.id = r5.n ORDER BY n;

-- WITH RECURSIVE + ORDER BY encrypted
WITH RECURSIVE r6 AS (
    SELECT id FROM test_ev_t WHERE id = 1
    UNION ALL
    SELECT id + 1 FROM r6 WHERE id < 4
) SELECT '7r_order_enc', t.name, t.salary
FROM r6 JOIN test_ev_t t ON t.id = r6.id ORDER BY t.salary DESC;

-- Multiple recursive CTEs
WITH RECURSIVE
    r7a(n) AS (SELECT 1 UNION ALL SELECT n+1 FROM r7a WHERE n < 2),
    r7b(m) AS (SELECT 10 UNION ALL SELECT m-1 FROM r7b WHERE m > 8)
SELECT '7r_multi', r7a.n, r7b.m FROM r7a, r7b;

-- WITH RECURSIVE + FILTER + encrypted
WITH RECURSIVE r8(id) AS (
    SELECT 1 UNION ALL SELECT id + 1 FROM r8 WHERE id < 4
) SELECT '7r_filter', COUNT(*) FILTER (WHERE r8.id > 2) AS high_cnt
FROM r8 JOIN test_ev_t t ON t.id = r8.id;

-- WITH RECURSIVE graph search + encrypted filter
CREATE TABLE test_ev_edge (from_id INT4, to_id INT4, weight INT4);
INSERT INTO test_ev_edge VALUES (1, 2, 1), (2, 3, 1), (3, 4, 1);

WITH RECURSIVE path(from_id, to_id, depth) AS (
    SELECT from_id, to_id, 1 FROM test_ev_edge WHERE from_id = 1
    UNION ALL
    SELECT e.from_id, e.to_id, p.depth + 1
    FROM test_ev_edge e JOIN path p ON e.from_id = p.to_id
) SELECT '7r_graph', p.depth, t.name
FROM path p JOIN test_ev_t t ON t.id = p.to_id ORDER BY p.depth;

DROP TABLE IF EXISTS test_ev_t CASCADE;
DROP TABLE IF EXISTS test_ev_t2 CASCADE;
DROP TABLE IF EXISTS test_ev_edge CASCADE;

-- ============================================================
-- 8. Encrypted + plain function expressions (bug7874)
-- ============================================================
SELECT '=== 8 Enc + Plain Function ===';

DROP TABLE IF EXISTS test_text_func CASCADE;
CREATE TABLE test_text_func(
    id SERIAL PRIMARY KEY,
    text_col text_enc encrypted with (dek = dek_expr, support_algo='ope'),
    category VARCHAR(50),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- INSERT...SELECT: encrypted col with constant, plain col with function expression
INSERT INTO test_text_func (text_col, category)
SELECT 'fixed_text', 'Category_' || (i % 10) FROM generate_series(1, 3) i;

SELECT '8a_func_expr', id, text_col, category FROM test_text_func ORDER BY id;

DROP TABLE IF EXISTS test_text_func CASCADE;

-- ============================================================
-- Cleanup
-- ============================================================
SELECT '=== Cleanup ===';
DROP KEK IF EXISTS kek_expr CASCADE;
