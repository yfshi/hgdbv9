--
-- subquery_alias_propagation.sql — libpq专有测试：Bug 9323
--   `TC_IVYSQL_TECC4_QUERY_SUB_002`：FROM子查询(派生表)/CTE含密态列执行报错
--
-- 场景：t_a(id int, name text_enc det, val int4_enc ope)，外层引用派生表/CTE 的
-- 密态输出列做等值匹配，例如
--   SELECT sub.id, sub.name FROM (SELECT id, name FROM t_a WHERE val > 15) sub
--   WHERE sub.name = 'Carol';
-- 修复前报错（外层常量的匹配落不到加密列上，按明文下发，服务端解不出来）。
--
-- 现有 CTE 用例（shared/sql/grammar.sql 的 cte1–cte22）全部建在明文表上：
-- "CTE ✅" 只证明了解析器认识 CTE 语法，没有一例覆盖"CTE/派生表输出列 = 密态列
-- 且外层引用它"。本用例把这个缺口锁死。
--
-- 本用例锁的是**重命名与嵌套**这一族（Bug 9323 的修复点：加密列的"暴露名"按
-- 输出位置对齐 + FROM 项统一递归）：目标列 AS 重命名、CTE 列别名表 WITH c(n)、
-- 别名链、两级限定、多层嵌套、LATERAL、子查询与基表 JOIN、子查询内集合运算、
-- CTE + DML RETURNING。同族但更偏"列映射传播"的场景见 cte_column_mapping.sql。
--
-- 断言方式：每条都必须**返回正确行**（不只是"不报错"）——密态常量若未加密下发，
-- 结果要么报错、要么恒为空。故每条都带 WHERE 常量的**正例**，另配 no_match
-- 负例（应返回 0 行），避免"匹配恒真"这种假通过。
--
-- 注：密文序不可复现，ORDER BY 一律用明文 id 列，不用密态列排序。
--

DROP TABLE IF EXISTS t_sqa CASCADE;
DROP TABLE IF EXISTS t_sqb CASCADE;
DROP DEK IF EXISTS dek_sqa_det CASCADE;
DROP DEK IF EXISTS dek_sqa_ope CASCADE;
DROP KEK IF EXISTS kek_sqa CASCADE;

CREATE KEK kek_sqa WITH (key_store = 'localkms', key_path = '%KEKPATH%/kek_sqa.key');
CREATE DEK dek_sqa_det WITH VALUES(KEK = 'kek_sqa');
CREATE DEK dek_sqa_ope WITH VALUES(KEK = 'kek_sqa');

CREATE TABLE t_sqa (
    id   INT4,
    name TEXT_ENC ENCRYPTED WITH (DEK = dek_sqa_det, SUPPORT_ALGO = 'DET'),
    val  INT4_ENC ENCRYPTED WITH (DEK = dek_sqa_ope, SUPPORT_ALGO = 'OPE')
);
CREATE TABLE t_sqb (
    id    INT4,
    name  TEXT_ENC ENCRYPTED WITH (DEK = dek_sqa_det, SUPPORT_ALGO = 'DET'),
    score INT4_ENC ENCRYPTED WITH (DEK = dek_sqa_ope, SUPPORT_ALGO = 'OPE')
);
INSERT INTO t_sqa VALUES (1, 'Alice', 10), (2, 'Bob', 20), (3, 'Carol', 30);
INSERT INTO t_sqb VALUES (1, 'Alice', 100), (2, 'Bob', 200), (4, 'Dave', 400);

SELECT '0_baseline_rows' AS tag, count(*) FROM t_sqa;

-- ============================================================
-- 1. FROM 子查询（派生表）：嵌套与别名
-- ============================================================
-- 1.1 报障原文
SELECT '1_1_reported_case' AS tag, sub.id, sub.name
FROM (SELECT id, name FROM t_sqa WHERE val > 15) sub
WHERE sub.name = 'Carol';

-- 1.2 三层嵌套，最外层用别名限定
SELECT '1_2_three_level' AS tag, x.name
FROM (SELECT y.name FROM (SELECT name FROM t_sqa WHERE val > 15) y WHERE y.name <> 'X') x
WHERE x.name = 'Carol';

-- 1.3 四层嵌套
SELECT '1_3_four_level' AS tag, w.name
FROM (SELECT x.name FROM (SELECT y.name FROM (SELECT name FROM t_sqa WHERE val > 15) y) x
      WHERE x.name <> 'Z') w
WHERE w.name = 'Carol';

-- 1.4 子查询 SELECT * 带别名
SELECT '1_4_star_alias' AS tag, s.name
FROM (SELECT * FROM t_sqa WHERE val > 15) s
WHERE s.name = 'Carol';

-- 1.5 目标列 AS 重命名（暴露名按输出位置对齐）
SELECT '1_5_target_rename' AS tag, s.n
FROM (SELECT name AS n FROM t_sqa) s
WHERE s.n = 'Carol';

-- 1.6 子查询 SELECT * 且目标列重命名
SELECT '1_6_star_target_rename' AS tag, s.n
FROM (SELECT id, name AS n FROM t_sqa) s
WHERE s.n = 'Carol';

-- 1.7 别名链（两层各自重命名）
SELECT '1_7_alias_chain' AS tag, z.n2
FROM (SELECT y.n AS n2 FROM (SELECT x.name AS n FROM t_sqa x) y) z
WHERE z.n2 = 'Carol';

-- 1.8 两级都限定 + 复合条件
SELECT '1_8_both_qualified' AS tag, s.n
FROM (SELECT name AS n FROM t_sqa) s
WHERE s.n = 'Carol' AND s.n <> 'Bob';

-- 1.9 子查询按 DET 过滤，外层 WHERE 压在 OPE 列上
SELECT '1_9_ope_outer' AS tag, s.id
FROM (SELECT id, val FROM t_sqa WHERE name = 'Carol') s
WHERE s.val > 15;

-- 1.10 外层 WHERE 压在 OPE 列上，子查询也压同一列
SELECT '1_10_ope_both' AS tag, s.name, s.val
FROM (SELECT name, val FROM t_sqa WHERE name = 'Alice') s
WHERE s.val > 5;

-- 1.11 负例：常量不存在 → 0 行
SELECT '1_11_no_match' AS tag, count(*)
FROM (SELECT name FROM t_sqa) s
WHERE s.name = 'no_such_value';

-- ============================================================
-- 2. CTE：限定名、列别名表、体内重命名
-- ============================================================
-- 2.1 CTE 名作限定
WITH cte AS (SELECT id, name FROM t_sqa WHERE val > 15)
SELECT '2_1_cte_qualified' AS tag, * FROM cte WHERE cte.name = 'Carol';

-- 2.2 CTE 体内嵌子查询
WITH cte AS (SELECT * FROM (SELECT id, name FROM t_sqa WHERE val > 15) inner1)
SELECT '2_2_cte_inner_sub' AS tag, * FROM cte WHERE name = 'Carol';

-- 2.3 WITH cte(n) 列别名表
WITH cte(n) AS (SELECT name FROM t_sqa)
SELECT '2_3_cte_column_alias_list' AS tag, * FROM cte WHERE n = 'Carol';

-- 2.4 CTE 体内 AS 重命名
WITH cte AS (SELECT name AS n FROM t_sqa)
SELECT '2_4_cte_inner_rename' AS tag, * FROM cte WHERE n = 'Carol';

-- 2.5 负例
WITH cte(n) AS (SELECT name FROM t_sqa)
SELECT '2_5_cte_alias_no_match' AS tag, count(*) FROM cte WHERE n = 'no_such_value';

-- ============================================================
-- 3. JOIN / LATERAL / 集合运算
-- ============================================================
-- 3.1 基表 JOIN 带别名
SELECT '3_1_join_alias' AS tag, a.name, b.score
FROM t_sqa a JOIN t_sqb b ON a.id = b.id
WHERE a.name = 'Alice';

-- 3.2 派生表与基表 JOIN
SELECT '3_2_sub_join_table' AS tag, s.name
FROM (SELECT id, name FROM t_sqa) s JOIN t_sqb b ON s.id = b.id
WHERE s.name = 'Alice';

-- 3.3 LATERAL 子查询
SELECT '3_3_lateral' AS tag, a.name, l.name
FROM t_sqa a, LATERAL (SELECT name FROM t_sqa b WHERE b.name = 'Alice') l
WHERE a.name = 'Carol';

-- 3.4 子查询内集合运算，外层再压常量
SELECT '3_4_union_in_sub' AS tag, s.name
FROM (SELECT name FROM t_sqa WHERE val > 15
      UNION
      SELECT name FROM t_sqb WHERE score > 150) s
WHERE s.name = 'Carol';

-- 3.5 CTE + DML RETURNING（密态列随 RETURNING 输出，外层再压常量）
WITH del AS (DELETE FROM t_sqb WHERE name = 'Dave' RETURNING id, name)
SELECT '3_5_cte_dml_returning' AS tag, * FROM del WHERE name = 'Dave';

SELECT '3_6_sqb_after_delete' AS tag, count(*) FROM t_sqb;

-- ============================================================
-- 4. 清理
-- ============================================================
DROP TABLE IF EXISTS t_sqa CASCADE;
DROP TABLE IF EXISTS t_sqb CASCADE;
DROP DEK IF EXISTS dek_sqa_det CASCADE;
DROP DEK IF EXISTS dek_sqa_ope CASCADE;
DROP KEK IF EXISTS kek_sqa CASCADE;
SELECT 'z_cleanup_done' AS tag;
