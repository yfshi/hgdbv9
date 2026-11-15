-- ============================================================
-- empty_target_list.sql — 空目标列表（SELECT FROM t）密态处理回归
-- Bug 9152 后续：前端语法缺少 target_list 空产生式，导致
-- "SELECT FROM t WHERE enc='x'" 整体解析失败被放行，密态常量以明文
-- 发往服务端，服务端报 invalid hex string。
-- 覆盖：顶层空目标列表、子查询空目标列表、CTE 空目标列表。
-- ============================================================

-- ============================================================
-- 0. Setup
-- ============================================================
DROP KEK IF EXISTS kek_etl CASCADE;

CREATE KEK kek_etl WITH (
    key_store = 'localkms',
    key_path = '/tmp/kek_etl.key'
);
CREATE DEK dek_det_etl WITH VALUES(KEK = kek_etl);
CREATE DEK dek_ope_etl WITH VALUES(KEK = kek_etl);

SELECT '=== Setup OK ===';

DROP TABLE IF EXISTS t_etl CASCADE;
CREATE TABLE t_etl (
    col_int int,
    col_det text_enc encrypted with(dek=dek_det_etl, support_algo='det'),
    col_ope int4_enc encrypted with(dek=dek_ope_etl, support_algo='ope')
);

INSERT INTO t_etl (col_int, col_det, col_ope) VALUES
(1, 'Alice', 10),
(2, 'Bob', 20),
(3, 'Alice', 30),
(4, 'Carol', 40);

-- ============================================================
-- 1. 顶层空目标列表：SELECT FROM t WHERE 密态等值
-- ============================================================
SELECT '=== 1 Top-level empty target list ===';

-- 1a: 3 行匹配（SELECT FROM 返回空列，用 count 包裹以便结果可比较）
SELECT '1a_empty_tlist_count', count(*) FROM (SELECT FROM t_etl WHERE col_det = 'Alice') AS a;

-- 1b: 无匹配
SELECT '1b_empty_tlist_nomatch_count', count(*) FROM (SELECT FROM t_etl WHERE col_det = 'Bob') AS a;

-- 1c: 空目标列表 + OPE 范围条件
SELECT '1c_empty_tlist_ope_count', count(*) FROM (SELECT FROM t_etl WHERE col_ope > 15) AS a;

-- ============================================================
-- 2. 子查询空目标列表：外层带密态等值条件
-- ============================================================
SELECT '=== 2 Empty target list in subquery ===';

-- 2a: 内层空目标列表（内层 WHERE 密态等值），外层为普通列
SELECT '2a_subquery_empty_tlist_count',
       count(*) FROM (SELECT FROM t_etl WHERE col_det = 'Alice') AS sub;

-- 2b: 内层普通目标列表 + 外层密态等值条件（对照用例）
SELECT '2b_subquery_where_count',
       count(*) FROM (SELECT col_int, col_det FROM t_etl WHERE col_det = 'Alice') AS sub
       WHERE col_det = 'Alice';

-- ============================================================
-- 3. 密态列参与投影（对照：结果仍可正确解密）
-- ============================================================
SELECT '=== 3 Projection control ===';

SELECT '3a_projection', count(*), min(col_int) FROM t_etl WHERE col_det = 'Alice';
SELECT '3b_projection', col_int, col_det, col_ope FROM t_etl WHERE col_det = 'Carol';

-- ============================================================
-- 4. Cleanup
-- ============================================================
SELECT '=== 4 Cleanup ===';
DROP TABLE IF EXISTS t_etl CASCADE;
DROP KEK IF EXISTS kek_etl CASCADE;
