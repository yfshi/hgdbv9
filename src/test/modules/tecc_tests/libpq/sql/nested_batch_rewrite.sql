-- ============================================================
-- nested_batch_rewrite.sql — 嵌套子查询 / CTE / 批量语句密态上下文回归
-- Bug 9152 家族修复覆盖：
--   1. 多层嵌套子查询（3/4 层）：内层加密列与改写状态向外层传播
--   2. 外层用子查询别名 / CTE 名限定加密列（sub.col_det）
--   3. CTE 内密态等值条件（含空目标列表 CTE）
--   4. 单次 PQexec 多语句批次中夹带前端未建模语句（BEGIN / DROP VIEW /
--      SHOW / COMMENT / ANALYZE / TRUNCATE）时，批内密态语句仍须加密改写；
--      否则密态常量以明文发往服务端（服务端报 invalid hex string）。
--   注：批次用例必须经 \! psql -c "<多语句>" 提交（psql -f 会按分号拆成
--       多条 PQexec，覆盖不到批量路径）。
-- ============================================================

-- ============================================================
-- 0. Setup
-- ============================================================
DROP KEK IF EXISTS kek_nbr CASCADE;

CREATE KEK kek_nbr WITH (
    key_store = 'localkms',
    key_path = '/tmp/kek_nbr.key'
);
CREATE DEK dek_det_nbr WITH VALUES(KEK = kek_nbr);
CREATE DEK dek_ope_nbr WITH VALUES(KEK = kek_nbr);

SELECT '=== Setup OK ===';

DROP TABLE IF EXISTS t_nbr CASCADE;
DROP TABLE IF EXISTS t_nbr_probe CASCADE;
DROP VIEW IF EXISTS v_nbr;

CREATE TABLE t_nbr (
    col_int int,
    col_det text_enc encrypted with(dek=dek_det_nbr, support_algo='det'),
    col_ope int4_enc encrypted with(dek=dek_ope_nbr, support_algo='ope')
);
CREATE TABLE t_nbr_probe (a int);

INSERT INTO t_nbr (col_int, col_det, col_ope) VALUES
(1, 'Alice', 10),
(2, 'Bob', 20),
(3, 'Alice', 30),
(4, 'Carol', 40);

-- ============================================================
-- 1. 多层嵌套子查询
-- ============================================================
SELECT '=== 1 Nested levels ===';

SELECT '1a_2level_nomatch', count(*) FROM (SELECT * FROM t_nbr WHERE col_det = 'Alice') AS s WHERE col_det = 'Carol';
SELECT '1b_3level_nomatch', count(*) FROM (SELECT * FROM (SELECT * FROM t_nbr WHERE col_det = 'Alice') AS a) AS b WHERE col_det = 'Carol';
SELECT '1c_3level_match', count(*) FROM (SELECT * FROM (SELECT * FROM t_nbr WHERE col_det = 'Alice') AS a) AS b WHERE col_det = 'Alice';
SELECT '1d_4level_nomatch', count(*) FROM (SELECT * FROM (SELECT * FROM (SELECT * FROM t_nbr WHERE col_det = 'Alice') AS a) AS b) AS c WHERE col_det = 'Carol';
SELECT '1e_3level_ope', count(*) FROM (SELECT * FROM (SELECT * FROM t_nbr WHERE col_ope > 15) AS a) AS b WHERE col_ope > 25;

-- ============================================================
-- 2. 子查询别名 / CTE 名限定加密列
-- ============================================================
SELECT '=== 2 Alias and CTE ===';

SELECT '2a_alias_outer_nomatch', count(*) FROM (SELECT * FROM t_nbr WHERE col_det = 'Alice') AS sub WHERE sub.col_det = 'Carol';
SELECT '2b_alias_outer_match', count(*) FROM (SELECT * FROM t_nbr WHERE col_det = 'Alice') AS sub WHERE sub.col_det = 'Alice';
SELECT '2c_alias_inner', count(*) FROM (SELECT * FROM t_nbr WHERE t_nbr.col_det = 'Alice') AS sub;
SELECT '2d_3level_alias_outer', count(*) FROM (SELECT * FROM (SELECT * FROM t_nbr WHERE col_det = 'Alice') AS a) AS b WHERE b.col_det = 'Carol';

WITH cte_nbr AS (SELECT * FROM t_nbr WHERE col_det = 'Alice')
SELECT '2e_cte_count', count(*) FROM cte_nbr;

WITH cte_nbr AS (SELECT FROM t_nbr WHERE col_det = 'Alice')
SELECT '2f_cte_empty_tlist', count(*) FROM cte_nbr;

WITH cte_nbr AS (SELECT * FROM t_nbr WHERE col_det = 'Alice')
SELECT '2g_cte_name_qualified', count(*) FROM cte_nbr WHERE cte_nbr.col_det = 'Alice';

-- ============================================================
-- 3. 批量语句（单次 PQexec）容错
-- 注：psql -c 只打印最后一条语句的结果，因此断言查询统一放在批次末尾；
--     末尾为未建模语句的用例，其断言就是"不出现 invalid hex string 报错"。
-- ============================================================
SELECT '=== 3 Batch ===';

-- 3a: 事务语句打头
\! psql -A -t -k on -c "BEGIN; COMMIT; SELECT '3a_leading_txn_batch', count(*) FROM t_nbr WHERE col_det = 'Alice';" 2>&1

-- 3b: DROP TABLE 打头
\! psql -A -t -k on -c "DROP TABLE IF EXISTS t_nbr_drop; SELECT '3b_drop_table_batch', count(*) FROM t_nbr WHERE col_det = 'Alice';" 2>&1

-- 3c: DROP VIEW + CREATE VIEW（视图 WHERE 密态常量必须被加密，否则服务端报错）
\! psql -A -t -k on -c "DROP VIEW IF EXISTS v_nbr; CREATE VIEW v_nbr AS SELECT * FROM t_nbr WHERE col_det = 'Alice'; SELECT '3c_view_batch', count(*) FROM v_nbr;" 2>&1

-- 3d: SHOW 打头
\! psql -A -t -k on -c "SHOW enable_seqscan; SELECT '3d_show_batch', count(*) FROM t_nbr WHERE col_det = 'Alice';" 2>&1

-- 3e: COMMENT 打头
\! psql -A -t -k on -c "COMMENT ON TABLE t_nbr_probe IS 'nbr'; SELECT '3e_comment_batch', count(*) FROM t_nbr WHERE col_det = 'Alice';" 2>&1

-- 3f: ANALYZE + CREATE INDEX 打头
\! psql -A -t -k on -c "ANALYZE t_nbr_probe; CREATE INDEX IF NOT EXISTS ix_nbr_probe ON t_nbr_probe(a); SELECT '3f_analyze_batch', count(*) FROM t_nbr WHERE col_det = 'Alice';" 2>&1

-- 3g: TRUNCATE + INSERT 在批次中间
\! psql -A -t -k on -c "SELECT 'x' AS pre; TRUNCATE t_nbr_probe; INSERT INTO t_nbr_probe VALUES (1); SELECT '3g_truncate_batch', count(*) FROM t_nbr WHERE col_det = 'Alice';" 2>&1

-- 3h: 未建模语句夹在中间（其后的密态语句仍须加密改写）
\! psql -A -t -k on -c "SELECT 'x' AS pre; TRUNCATE t_nbr_probe; SELECT '3h_middle_batch', count(*) FROM t_nbr WHERE col_det = 'Bob';" 2>&1

-- 3i: 未建模语句在批次末尾（只要求不报 invalid hex string）
\! psql -A -t -k on -c "SELECT 'x' AS pre; COMMIT;" 2>&1
\! psql -A -t -k on -c "SELECT 'x' AS pre; DROP VIEW IF EXISTS v_nbr;" 2>&1

-- 3j: 批次只有事务语句（无密态语句，按原文放行）
\! psql -A -t -k on -c "BEGIN;" 2>&1

-- 3k: 空语句与字符串内分号（不得被误判为语句分隔）
\! psql -A -t -k on -c "SELECT 'x' AS pre;; SELECT '3k_empty_stmt_batch', 'a;b' AS lit, count(*) FROM t_nbr WHERE col_det = 'Alice';" 2>&1

-- ============================================================
-- 4. Cleanup
-- ============================================================
SELECT '=== 4 Cleanup ===';
DROP VIEW IF EXISTS v_nbr;
DROP TABLE IF EXISTS t_nbr CASCADE;
DROP TABLE IF EXISTS t_nbr_probe CASCADE;
DROP TABLE IF EXISTS t_nbr_drop CASCADE;
DROP KEK IF EXISTS kek_nbr CASCADE;
