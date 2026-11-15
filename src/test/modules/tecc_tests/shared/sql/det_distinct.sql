--
-- det_distinct.sql — DET 列 DISTINCT / COUNT(DISTINCT) 哈希去重路径专项测试
--
-- 背景（plan/2026-08-24-服务端-DET-列-DISTINCT-排序缺口.html）：
--   DET-only 列无 OPE 保序段，COUNT(DISTINCT det_col) 原先走 executor 的
--   Tuplesort 排序去重路径，btree 比较（cipherdata_cmp_internal）直接报错
--   "cannot compare encrypted values for ordering"。
--   修复：executor 对"纯 DISTINCT（无 aggorder）且参数列无 OPE"的聚合改用
--   TupleHashTable 哈希去重（等值+哈希均基于 DET 段，正确且可用）。
--
-- 本用例覆盖：
--   1. COUNT(DISTINCT DET 列) 正确计数（text/int/varchar/numeric 多类型）
--   2. COUNT(DISTINCT DET 列) FILTER
--   3. SELECT DISTINCT DET 列 / GROUP BY DET 列（既有 HashAggregate 路径）
--   4. GROUP BY + COUNT(DISTINCT DET 列) 组合
--   5. 相关子查询 rescan 场景（每组重置哈希表）
--   6. 多 DISTINCT 聚合同查询
--   7. 对照：OPE 列 COUNT(DISTINCT) 不受影响
--   8. 语义保持：ORDER BY DET 列仍报错（仅 OPE 支持排序）
--   9. 普通类型 COUNT(DISTINCT) 不回归
--
-- 说明：DDL 与 DML 分事务（数据获取器走独立连接，需见已提交元数据）。
-- ============================================================

-- ============================================================
-- 0. Setup — 独立 KEK/DEK（避免与其它用例的 key_path 冲突）
-- ============================================================
DROP KEK IF EXISTS kek_det_dist CASCADE;

CREATE KEK kek_det_dist WITH (
    key_store = 'localkms',
    key_path = '/tmp/kek_det_dist.key'
);
CREATE DEK dek_det_dist WITH VALUES(KEK = kek_det_dist);

-- ============================================================
-- 1. COUNT(DISTINCT DET 列) 正确计数
-- ============================================================
SELECT '=== 1 COUNT(DISTINCT det) ===';

DROP TABLE IF EXISTS test_dd_t1 CASCADE;
CREATE TABLE test_dd_t1 (
    id SERIAL PRIMARY KEY,
    name  TEXT_ENC     ENCRYPTED WITH (DEK = 'dek_det_dist', support_algo = 'det'),
    code  INT4_ENC     ENCRYPTED WITH (DEK = 'dek_det_dist', support_algo = 'det'),
    tag   VARCHAR_ENC  ENCRYPTED WITH (DEK = 'dek_det_dist', support_algo = 'det'),
    price NUMERIC_ENC  ENCRYPTED WITH (DEK = 'dek_det_dist', support_algo = 'det'),
    score INT4_ENC     ENCRYPTED WITH (DEK = 'dek_det_dist', support_algo = 'ope')
);

INSERT INTO test_dd_t1 (name, code, tag, price, score) VALUES
('Alice', 10, 'a', 1.5, 90),
('Bob',   20, 'b', 2.5, 80),
('Alice', 10, 'a', 1.5, 70),
('Charlie', 30, 'c', NULL, 60),
('Bob',   20, 'b', 2.5, 50),
('Dave',  NULL, NULL, 4.5, 40);

-- 文本列去重：{Alice,Bob,Charlie,Dave} = 4
SELECT '1a_count_distinct_text', COUNT(DISTINCT name) FROM test_dd_t1;
-- int 列去重：{10,20,30,NULL} 中 NULL 不计 = 3
SELECT '1b_count_distinct_int', COUNT(DISTINCT code) FROM test_dd_t1;
-- varchar 列去重：{a,b,c,NULL} 中 NULL 不计 = 3
SELECT '1c_count_distinct_varchar', COUNT(DISTINCT tag) FROM test_dd_t1;
-- numeric 列去重：{1.5,2.5,4.5}（NULL 不计）= 3
SELECT '1d_count_distinct_numeric', COUNT(DISTINCT price) FROM test_dd_t1;
-- 对照：OPE 列 COUNT(DISTINCT) 全部 6 个值都不同 = 6
SELECT '1e_count_distinct_ope', COUNT(DISTINCT score) FROM test_dd_t1;

-- ============================================================
-- 2. COUNT(DISTINCT DET 列) FILTER
-- ============================================================
SELECT '=== 2 FILTER ===';

SELECT '2a_filter', COUNT(DISTINCT name) FILTER (WHERE id > 2) FROM test_dd_t1;
SELECT '2b_filter_all', COUNT(DISTINCT name) FILTER (WHERE id > 0) FROM test_dd_t1;
SELECT '2c_filter_none', COUNT(DISTINCT name) FILTER (WHERE id > 100) FROM test_dd_t1;

-- ============================================================
-- 3. SELECT DISTINCT / GROUP BY DET 列（既有 HashAggregate 路径）
-- ============================================================
SELECT '=== 3 DISTINCT / GROUP BY ===';

-- SELECT DISTINCT / GROUP BY 结果集本身正确；注意外层不能对 DET 列
-- ORDER BY（那需要 OPE 排序，见第 8 节），因此按 plain id 关联保证
-- 输出确定性。
SELECT '3a_select_distinct', s.name
FROM (SELECT DISTINCT name, id FROM test_dd_t1) s
ORDER BY s.id;
SELECT '3b_group_by', name, count(*) FROM test_dd_t1 GROUP BY name
ORDER BY min(id);

-- ============================================================
-- 4. GROUP BY + COUNT(DISTINCT DET 列) 组合
-- ============================================================
SELECT '=== 4 GROUP BY + COUNT(DISTINCT) ===';

-- 按 plain 表达式分组，组内对 DET 列做 DISTINCT 计数（分组键可排序，
-- 组内 DISTINCT 走哈希去重路径）
SELECT '4a_group_distinct', id % 2 AS g, COUNT(DISTINCT name) FROM test_dd_t1 GROUP BY id % 2 ORDER BY g;
-- GROUP BY DET 列（无 DISTINCT 聚合）走 HashAggregate，正常
SELECT '4b_group_det', tag, count(*) FROM test_dd_t1 GROUP BY tag
ORDER BY min(id);

-- 以下为 Bug 9324 修复后的覆盖：GROUP BY 键本身是 DET 列。
-- 规划器不再把纯 DISTINCT 聚合计入 numOrderedAggs（prepagg.c），因此可选
-- HashAggregate，避开"GroupAggregate 需要按 DET 列排序"的死路；执行器的
-- DISTINCT 去重表改为每组一份（AggStatePerGroupData.distinctHashtable），
-- 使 hashed 聚合也能去重。输出按 plain 列 min(id) 排序保证确定性
-- （DET 密文序跨运行不固定，且 ORDER BY DET 列本身仍应报错，见第 8 节）。

-- 4c. GROUP BY DET 列 + COUNT(DISTINCT 同一 DET 列)
SELECT '4c_group_det_distinct_same', name, COUNT(DISTINCT name)
FROM test_dd_t1 GROUP BY name ORDER BY min(id);
-- 4d. GROUP BY DET 列 + COUNT(DISTINCT 另一 DET 列)
SELECT '4d_group_det_distinct_other', tag, COUNT(DISTINCT name)
FROM test_dd_t1 GROUP BY tag ORDER BY min(id);
-- 4e. GROUP BY (DET, plain) 复合键 + COUNT(DISTINCT DET 列)
SELECT '4e_group_det_plain', name, id % 2 AS g, COUNT(DISTINCT tag)
FROM test_dd_t1 GROUP BY name, id % 2 ORDER BY min(id), g;
-- 4f. ROLLUP(DET 列) + COUNT(DISTINCT DET 列)：覆盖 AGG_MIXED（含空分组集）
SELECT '4f_rollup_det', tag, COUNT(DISTINCT name)
FROM test_dd_t1 GROUP BY ROLLUP(tag) ORDER BY min(id);
-- 4g. hashed 聚合聚落盘（spill）路径：强制 HashAggregate 并把 hash_mem 压到最小，
--     用连接放大组数触发分批；只断言结果正确（分批是运行时行为，不作断言）
SET work_mem = '64kB';
SET hash_mem_multiplier = 1.0;
SET enable_sort = off;
SELECT '4g_spill_hashed_distinct', count(*) FROM (
    SELECT t.name, s, COUNT(DISTINCT t.code) AS c
    FROM test_dd_t1 t, generate_series(1, 300) s
    GROUP BY t.name, s) x;
RESET enable_sort;
RESET hash_mem_multiplier;
RESET work_mem;

-- 4h. 对照：OPE 列不享受规划器的 hash 放宽
--     prepagg.c 只对"参数列不支持排序"的单列 DISTINCT 放开 numOrderedAggs，
--     带 OPE 的列 tecc_column_supports_ordering 仍返回 true，故依旧计入
--     numOrderedAggs，enable_sort=off 下规划器也只能退回 GroupAggregate+Sort
--     （分组键是 plain 列，排序去重走 OPE 密文序，合法且可用）。
--     若放宽被误扩大到 OPE 列，会选中 HashAggregate，而执行器对非
--     use_hash_distinct 的 DISTINCT 聚合在 AGG_HASHED 下仍按 grouping set
--     共用一份 Tuplesort —— 跨组去重错乱或直接崩溃（同类风险见 7c）。
--     分组切成 1:5 两组，故意让两组的去重基数不同，便于暴露跨组混算。
SET enable_sort = off;
SELECT '4h_ope_group_distinct', id = 1 AS g, COUNT(DISTINCT score)
FROM test_dd_t1 GROUP BY id = 1 ORDER BY g;
RESET enable_sort;

-- ============================================================
-- 5. 相关子查询 rescan 场景（每组重置哈希表）
-- ============================================================
SELECT '=== 5 rescan ===';

SELECT '5a_corr', a.id, (SELECT COUNT(DISTINCT b.name) FROM test_dd_t1 b WHERE b.id <= a.id)
FROM test_dd_t1 a ORDER BY a.id;

-- ============================================================
-- 6. 多 DISTINCT 聚合同查询（text DET + int DET + int OPE 混用）
-- ============================================================
SELECT '=== 6 multiple DISTINCT aggs ===';

SELECT '6a_multi', COUNT(DISTINCT name), COUNT(DISTINCT code), COUNT(DISTINCT score) FROM test_dd_t1;

-- ============================================================
-- 7. 普通类型 COUNT(DISTINCT) 不回归
-- ============================================================
SELECT '=== 7 plain types ===';

CREATE TEMP TABLE test_dd_plain(t text, n int);
INSERT INTO test_dd_plain VALUES ('a',1),('b',2),('a',1),(NULL,3),('c',4);
SELECT '7a_plain_text', COUNT(DISTINCT t) FROM test_dd_plain;
SELECT '7b_plain_int', COUNT(DISTINCT n) FROM test_dd_plain;
-- ReqID:TECC4.0 - 普通列的分组 DISTINCT 聚合仍须走 PostgreSQL 原生有序路径
SET enable_sort = off;
SELECT '7c_plain_group_distinct', n % 2 AS g, COUNT(DISTINCT t) FROM test_dd_plain GROUP BY n % 2 ORDER BY g;
RESET enable_sort;

-- ============================================================
-- 8. 语义保持：ORDER BY / 范围比较 DET 列仍报错（仅 OPE 支持排序）
-- ============================================================
SELECT '=== 8 ordering semantics preserved ===';

-- ORDER BY DET 列：排序比较需要 OPE，必须报错
SELECT '8a_order_by_det', name FROM test_dd_t1 ORDER BY name;
SELECT '8b_order_by_det_desc', name FROM test_dd_t1 ORDER BY name DESC;
-- GROUP BY DET 列 + ORDER BY DET 列：仍报错（ORDER BY 部分需要 OPE）
SELECT '8c_group_order_det', name, count(*) FROM test_dd_t1 GROUP BY name ORDER BY name;

-- ============================================================
-- 8d~8f. Bug 9349: GROUP BY + HAVING 引用分组列（DET）
--
-- HAVING 里无聚合的分组列等值谓词会被规划器下推成 WHERE，行数估计变小后
-- 成本模型偏向 Sort + GroupAggregate；DET-only 无 OPE，排序比较报
-- "cannot compare encrypted values for ordering"。修复后规划器对裸 DET
-- 分组键不再提供 sort 路径，默认与 enable_sort=off 都走 HashAggregate。
-- ============================================================
SELECT '=== 8d group by + having det ===';

-- 默认 enable_sort=on：应成功（修复前报 OPE ordering 错误）
SELECT '8d_having_det_on', name, count(*) FROM test_dd_t1
GROUP BY name HAVING name = 'Alice';

-- enable_sort=off：同样成功，结果一致
SET enable_sort = off;
SELECT '8e_having_det_off', name, count(*) FROM test_dd_t1
GROUP BY name HAVING name = 'Alice';
RESET enable_sort;

-- WHERE 下推等价形（HAVING 无聚合时的规划结果）默认也应成功
SELECT '8f_where_group_det', name, count(*) FROM test_dd_t1
WHERE name = 'Alice' GROUP BY name;

-- ============================================================
-- 9. 清理
-- ============================================================
DROP TABLE IF EXISTS test_dd_t1 CASCADE;
DROP TABLE IF EXISTS test_dd_plain CASCADE;
DROP DEK IF EXISTS dek_det_dist CASCADE;
DROP KEK IF EXISTS kek_det_dist CASCADE;
