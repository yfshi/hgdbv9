-- ============================================================
-- nested_subquery.sql — 嵌套子查询密态列传播测试（Bug 9152）
-- 覆盖：SELECT * 子查询中加密列向外层传播、WHERE 条件加密常量替换
-- ============================================================

-- ============================================================
-- 0. Setup
-- ============================================================
DROP KEK IF EXISTS kek_ns CASCADE;

CREATE KEK kek_ns WITH (
    key_store = 'localkms',
    key_path = '/tmp/kek_ns.key'
);
CREATE DEK dek_det_ns WITH VALUES(KEK = kek_ns);
CREATE DEK dek_ope_ns WITH VALUES(KEK = kek_ns);

SELECT '=== Setup OK ===';

DROP TABLE IF EXISTS t_base_ns CASCADE;
CREATE TABLE t_base_ns (
    col_int int,
    col_det text_enc encrypted with(dek=dek_det_ns, support_algo='det'),
    col_ope int4_enc encrypted with(dek=dek_ope_ns, support_algo='ope')
);

INSERT INTO t_base_ns (col_int, col_det, col_ope) VALUES
(1, 'Alice', 10),
(2, 'Bob', 20),
(3, 'Alice', 30),
(4, 'Carol', 40);

-- ============================================================
-- 1. 基线测试（无子查询）
-- ============================================================
SELECT '=== 1 Baseline ===';

SELECT '1a_baseline', * FROM t_base_ns WHERE col_det = 'Alice';
SELECT '1b_baseline', * FROM t_base_ns WHERE col_det = 'Carol';

-- ============================================================
-- 2. 简单子查询（无加密列引用在外层WHERE）
-- ============================================================
SELECT '=== 2 Simple Subquery ===';

-- 2a: 子查询无 WHERE
SELECT '2a_star_subquery', * FROM (SELECT * FROM t_base_ns) AS sub;

-- 2b: 子查询有 WHERE 但外层无 WHERE
SELECT '2b_subquery_inner_where', * FROM (SELECT * FROM t_base_ns WHERE col_det = 'Alice') AS sub;

-- ============================================================
-- 3. Bug 9152 核心用例：两层嵌套子查询 WHERE 密态等值条件
-- ============================================================
SELECT '=== 3 Nested Subquery WHERE ===';

-- 3a: 子查询 WHERE 加密 + 外层 WHERE 加密
SELECT '3a_nested_where', * FROM (SELECT * FROM t_base_ns WHERE col_det = 'Alice') AS sub WHERE col_det = 'Carol';

-- 3b: 子查询 WHERE 加密 + 外层 WHERE 匹配
SELECT '3b_nested_match', * FROM (SELECT * FROM t_base_ns WHERE col_det = 'Alice') AS sub WHERE col_det = 'Alice';

-- ============================================================
-- 4. Cleanup
-- ============================================================
SELECT '=== 4 Cleanup ===';
DROP TABLE IF EXISTS t_base_ns CASCADE;
DROP KEK IF EXISTS kek_ns CASCADE;
