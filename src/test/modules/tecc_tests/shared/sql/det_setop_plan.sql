-- det_setop_plan.sql — DET 加密列集合操作规划期分流专项回归
--
-- 背景：集合操作去重有两条实现路径 —— hash（UNION 用 AGG_HASHED，INTERSECT/EXCEPT
-- 用 SETOP_HASHED）与 sort（Sort + Unique/SetOp），规划期在 choose_hashed_setop()
-- 里按成本和 GUC 选择：enable_hashagg=off 或 hashentrysize*dNumGroups 超过 hash_mem
-- （默认 work_mem=4MB）时改走排序。密态类型注册了 btree 操作符族，
-- grouping_is_sortable() 于是静态判定密态列"可排序"，但对没有 OPE 密文段的列
-- （DET-only / RND-only）排序比较 cipherdata_cmp_internal() 会直接报
-- "cannot compare encrypted values for ordering"。而集合去重其实只需要等值
-- （Unique/SetOp 走 cipherdata_eq()，DET 段就够），排序只是把重复行聚到一起的手段。
--
-- 修复：choose_hashed_setop() 增加规划期分流 —— 分组列中只要含"无 OPE 密态列"
-- （tecc_column_supports_ordering() 判否），就强制 hash 去重；OPE 列不受影响，
-- 仍按成本模型在 hash / sort 之间选。
--
-- 本用例覆盖：
--   1. 复现用例：表里同时存在 det+he-add / ope / 纯 he-add / det+he-add+he-mul 四列，
--      SELECT 只取两个 DET 列（含纯 he-add 的 c 列不参与）做 UNION
--   2. enable_hashagg=off 强制走原排序分支 vs 默认 hash 分支，结果必须一致
--   3. UNION / UNION ALL / INTERSECT / INTERSECT ALL / EXCEPT / EXCEPT ALL
--      及单列 / 多列 / 三分支 / 混合列类型
--   4. OPE 列集合操作不受分流影响（对照，无回归）
--   5. 设计边界保持：DET 列 ORDER BY 仍报排序错误；纯 HE 列仍无法参与去重
--   6. 两个触发条件分别覆盖：① enable_hashagg=off（第 1 节）；②
--      hashentrysize*dNumGroups > hash_mem，用 work_mem=64kB + 连接放大组数等价
--      构造，避免回归里塞几十万行（第 1.5 节；UNION 与 INTERSECT/EXCEPT 的
--      dNumGroups 口径不同，故两者分别构造）
--
-- 说明：结果用 count(*) 断言（-A -t 格式下 0 行无输出）；需要看具体值时按
-- plain 列 id 取单行，避免对 DET 列做排序。
--
-- 注意：前端常量加密不递归"嵌在 FROM 子查询/CTE 里的集合操作"
-- （tecc_stmt_processor_process_select 只处理普通 SelectStmt，不看
-- select_stmt->op；顶层集合操作另有 FE_T_SetOp 分支会递归），所以
-- `SELECT count(*) FROM (SELECT ... WHERE enc_col = 10 UNION ...) u`
-- 会漏掉常量改写、报 "operator does not exist: int4_enc = integer"。
-- 因此凡 WHERE 里带密态常量的用例一律写成顶层集合操作 + plain 列 id 作
-- ORDER BY（id 不参与加密比较），既能真实改写常量又保证输出确定。

-- ============================================================
-- 0. Setup
-- ============================================================
DROP TABLE IF EXISTS ds_t CASCADE;
DROP DEK IF EXISTS ds_dek CASCADE;
DROP DEK IF EXISTS ds_he_add_dek CASCADE;
DROP DEK IF EXISTS ds_he_mul_dek CASCADE;
DROP KEK IF EXISTS ds_kek CASCADE;
DROP KEK IF EXISTS ds_he_kek CASCADE;

CREATE KEK ds_kek WITH (key_store = 'localkms', key_path = '/tmp/ds_kek.key');
CREATE DEK ds_dek WITH VALUES(KEK = ds_kek);
-- 非对称 KEK 用 %KEKPATH% + tecc_ktool 预生成（.pub 需与 .priv 同目录，
-- 裸 /tmp 路径下 C 层按原路径找 .pub 会失败）；he-add 用 EC 密钥对、
-- he-mul 用 FF 密钥对，故拆两个 DEK
\! rm -f "$TECC_KEK_PATH/ds_he_asym.priv" "$TECC_KEK_PATH/ds_he_asym.pub" 2>/dev/null
\! tecc_ktool generate --key-name ds_he_asym --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1
CREATE KEK ds_he_kek WITH (
    key_store = 'localkms',
    key_algo = 'sm2',
    key_path = '%KEKPATH%/ds_he_asym.priv'
);
CREATE DEK ds_he_add_dek WITH VALUES(KEK = ds_he_kek, KEY_ALGO = 'he-add');
CREATE DEK ds_he_mul_dek WITH VALUES(KEK = ds_he_kek, KEY_ALGO = 'he-mul');

-- 表结构与问题现场一致：a=DET+he-add / b=OPE / c=纯 he-add / d=DET+he-add+he-mul
-- （额外补一个 plain 列 id，仅用于让输出确定性排序）
CREATE TABLE ds_t (
    id int4,
    a int4_enc ENCRYPTED WITH (DEK = ds_dek, HE_ADD_DEK = ds_he_add_dek, SUPPORT_ALGO = 'det,he-add'),
    b int4_enc ENCRYPTED WITH (DEK = ds_dek, SUPPORT_ALGO = 'ope'),
    c int4_enc ENCRYPTED WITH (HE_ADD_DEK = ds_he_add_dek, SUPPORT_ALGO = 'he-add'),
    d int4_enc ENCRYPTED WITH (DEK = ds_dek, HE_ADD_DEK = ds_he_add_dek, HE_MUL_DEK = ds_he_mul_dek, SUPPORT_ALGO = 'det,he-add,he-mul')
);

-- a: 10,20,20,30（有重复，用于验证去重确实生效）
-- b: 10,20,30,40   c: 1,2,3,4   d: 20,30,30,40
INSERT INTO ds_t(id,a,b,c,d) VALUES
    (1, 10, 10, 1, 20),
    (2, 20, 20, 2, 30),
    (3, 20, 30, 3, 30),
    (4, 30, 40, 4, 40);

-- 单行取值（证明 DET 列可正常解密回明文）
SELECT '0a_det_value', a, d FROM ds_t WHERE id = 1;
SELECT '0b_ope_value', b FROM ds_t WHERE id = 1;

-- ============================================================
-- 1. 复现：enable_hashagg=off 强制原排序分支
-- ============================================================
SELECT '=== 1 enable_hashagg=off ===';

SET enable_hashagg = off;

-- 1a. EXPLAIN：DET-only 列的分流应强制 HashAggregate（修复前为 Sort + Unique）
EXPLAIN (COSTS OFF) SELECT a, d FROM ds_t WHERE a = 10 UNION SELECT a, d FROM ds_t WHERE d = 20;
-- 1b. 问题现场原句（加 plain 列 id 作确定性排序键）：a、d 均为 DET（无 OPE），
--     修复前报 ordering 错
SELECT '1b_union_ad', id, a, d FROM ds_t WHERE a = 10
UNION
SELECT '1b_union_ad', id, a, d FROM ds_t WHERE d = 20
ORDER BY 2;
-- 1c. 单列 UNION（4 行输入去重成 3 个值）
SELECT '1c_union_a', count(*) FROM (SELECT a FROM ds_t UNION SELECT a FROM ds_t) u;
-- 1d. 跨列 UNION（a 与 d 均为 DET 同 DEK）
SELECT '1d_union_cross', count(*) FROM (SELECT a FROM ds_t UNION SELECT d FROM ds_t) u;
-- 1e. 三分支 UNION
SELECT '1e_union_3way', count(*)
FROM (SELECT a, d FROM ds_t WHERE id = 1 UNION SELECT a, d FROM ds_t WHERE id = 2
      UNION SELECT a, d FROM ds_t WHERE id = 3) u;
-- 1f. INTERSECT（等值常量，两侧都命中 id=1 行 → 1 行）
SELECT '1f_intersect', id, a, d FROM ds_t WHERE a = 10
INTERSECT
SELECT '1f_intersect', id, a, d FROM ds_t WHERE d = 20
ORDER BY 2;
-- 1g. EXCEPT（4 行减去 2 行 a=20 的行 → id 1、4 共 2 行）
SELECT '1g_except', id, a FROM ds_t
EXCEPT
SELECT '1g_except', id, a FROM ds_t WHERE a = 20
ORDER BY 2;
-- 1h. INTERSECT ALL（按出现次数取交集）
SELECT '1h_intersect_all', count(*) FROM (SELECT a FROM ds_t INTERSECT ALL SELECT a FROM ds_t) u;
-- 1i. EXCEPT ALL（同集合相减 → 0 行）
SELECT '1i_except_all', count(*) FROM (SELECT a FROM ds_t EXCEPT ALL SELECT a FROM ds_t) u;
-- 1j. 多列混合：DET 列 + OPE 列 + plain 列同时参与去重
SELECT '1j_union_mixed', count(*)
FROM (SELECT id, a, b, d FROM ds_t UNION SELECT id, a, b, d FROM ds_t) u;
-- 1k. INTERSECT/EXCEPT 的 EXPLAIN（应为 HashSetOp，修复前可能为 Sort + SetOp）
EXPLAIN (COSTS OFF) SELECT a FROM ds_t INTERSECT SELECT a FROM ds_t;
EXPLAIN (COSTS OFF) SELECT a FROM ds_t EXCEPT SELECT a FROM ds_t;
-- 1k2. OPE 列对照（关键无回归断言）：分流不介入，enable_hashagg=off 下
--      仍按原成本模型走 Sort + Unique，不被强制 hash
EXPLAIN (COSTS OFF) SELECT b FROM ds_t UNION SELECT b FROM ds_t;
-- 1l. OPE 列对照：分流不介入，结果不受影响
SELECT '1l_union_ope', count(*) FROM (SELECT b FROM ds_t UNION SELECT b FROM ds_t) u;
-- 1m. DET 列 JOIN 结果参与集合操作（与 det_join_plan.sql 的分流叠加）
SELECT '1m_union_join', count(*)
FROM (SELECT a FROM ds_t UNION SELECT d FROM ds_t) u JOIN ds_t t ON u.a = t.a;

RESET enable_hashagg;

-- ============================================================
-- 1.5 触发条件②：hash_mem 放不下（大数据量的等价构造）
-- ============================================================
SELECT '=== 1.5 hash_mem 不足（大数据量等价） ===';

-- choose_hashed_setop 还有一条选排序的分支：hashentrysize * dNumGroups 超过
-- hash_mem（= work_mem * hash_mem_multiplier，默认 4MB）。表一大就会命中，
-- 与 GUC 无关，正是"小数据能过、几十万行就报错"的现场。这里把 work_mem 压到
-- 最小值 64kB、再用连接把去重输入的组数放大，命中同一条分支、同一个判定，
-- 避免往回归里塞几十万行数据（实测 20 万行/10 万组时行为完全一致）。
SET work_mem = '64kB';
-- 1n. UNION（单列，分组列 = DET 列 a）：UNION 的 dNumGroups = path->rows（输入行数），
--     小表加连接后即可超过 hash_mem，修复前规划为 Sort + Unique 并在执行期报
--     ordering 错
EXPLAIN (COSTS OFF)
SELECT t.a FROM ds_t t, generate_series(1, 5000) g
UNION
SELECT t.a FROM ds_t t, generate_series(1, 5000) g;
SELECT '1n_union_big', count(*) FROM (
    SELECT t.a FROM ds_t t, generate_series(1, 5000) g
    UNION
    SELECT t.a FROM ds_t t, generate_series(1, 5000) g) u;
-- 1n2. INTERSECT / EXCEPT（单列）：它们的 dNumGroups 取"分组列的去重个数估计"
--      （prepunion.c:806-812），只有 a 时估计值很小（3 个值），即使 work_mem=64kB
--      也仍走 hash —— 这里只作低 work_mem 下的正确性断言，触发条件②见 1p
SELECT '1n_intersect_big', count(*) FROM (
    SELECT t.a FROM ds_t t, generate_series(1, 5000) g
    INTERSECT
    SELECT t.a FROM ds_t t, generate_series(1, 5000) g) u;
SELECT '1n_except_big', count(*) FROM (
    SELECT t.a FROM ds_t t, generate_series(1, 5000) g
    EXCEPT
    SELECT t.a FROM ds_t t, generate_series(1, 5000) g) u;
-- 1o. OPE 列对照：同样条件仍按成本模型走 Sort（分流不介入）
EXPLAIN (COSTS OFF)
SELECT t.b FROM ds_t t, generate_series(1, 5000) g
UNION
SELECT t.b FROM ds_t t, generate_series(1, 5000) g;
-- 1p. 触发条件②在 INTERSECT/EXCEPT 上的构造：分组列 = (DET 列 a, plain 列 g)，
--     去重个数估计被放大到超过 hash_mem，修复前规划为 SetOp + Sort、执行期报
--     ordering 错（与 20 万行/10 万组实测同一分支）
EXPLAIN (COSTS OFF)
SELECT t.a, g FROM ds_t t, generate_series(1, 5000) g
INTERSECT
SELECT t.a, g FROM ds_t t, generate_series(1, 5000) g;
EXPLAIN (COSTS OFF)
SELECT t.a, g FROM ds_t t, generate_series(1, 5000) g
EXCEPT
SELECT t.a, g FROM ds_t t, generate_series(1, 5000) g;
SELECT '1p_intersect_big', count(*) FROM (
    SELECT t.a, g FROM ds_t t, generate_series(1, 5000) g
    INTERSECT
    SELECT t.a, g FROM ds_t t, generate_series(1, 5000) g) u;
SELECT '1p_except_big', count(*) FROM (
    SELECT t.a, g FROM ds_t t, generate_series(1, 5000) g
    EXCEPT
    SELECT t.a, g FROM ds_t t, generate_series(1, 5000) g) u;
-- 1p2. 同一构造下的 OPE 列对照：不介入分流，仍走 SetOp + Sort
EXPLAIN (COSTS OFF)
SELECT t.b, g FROM ds_t t, generate_series(1, 5000) g
INTERSECT
SELECT t.b, g FROM ds_t t, generate_series(1, 5000) g;
RESET work_mem;

-- ============================================================
-- 2. 对照：默认（hash）分支，结果必须与第 1 节逐条一致
-- ============================================================
SELECT '=== 2 default ===';

SELECT '2b_union_ad', id, a, d FROM ds_t WHERE a = 10
UNION
SELECT '2b_union_ad', id, a, d FROM ds_t WHERE d = 20
ORDER BY 2;
SELECT '2c_union_a', count(*) FROM (SELECT a FROM ds_t UNION SELECT a FROM ds_t) u;
SELECT '2d_union_cross', count(*) FROM (SELECT a FROM ds_t UNION SELECT d FROM ds_t) u;
SELECT '2e_union_3way', count(*)
FROM (SELECT a, d FROM ds_t WHERE id = 1 UNION SELECT a, d FROM ds_t WHERE id = 2
      UNION SELECT a, d FROM ds_t WHERE id = 3) u;
SELECT '2f_intersect', id, a, d FROM ds_t WHERE a = 10
INTERSECT
SELECT '2f_intersect', id, a, d FROM ds_t WHERE d = 20
ORDER BY 2;
SELECT '2g_except', id, a FROM ds_t
EXCEPT
SELECT '2g_except', id, a FROM ds_t WHERE a = 20
ORDER BY 2;
SELECT '2h_intersect_all', count(*) FROM (SELECT a FROM ds_t INTERSECT ALL SELECT a FROM ds_t) u;
SELECT '2i_except_all', count(*) FROM (SELECT a FROM ds_t EXCEPT ALL SELECT a FROM ds_t) u;
SELECT '2j_union_mixed', count(*)
FROM (SELECT id, a, b, d FROM ds_t UNION SELECT id, a, b, d FROM ds_t) u;
SELECT '2l_union_ope', count(*) FROM (SELECT b FROM ds_t UNION SELECT b FROM ds_t) u;
SELECT '2m_union_join', count(*)
FROM (SELECT a FROM ds_t UNION SELECT d FROM ds_t) u JOIN ds_t t ON u.a = t.a;
-- 2n. UNION ALL 不去重（对照：8 = 4 + 4）
SELECT '2n_union_all', count(*) FROM (SELECT a FROM ds_t UNION ALL SELECT a FROM ds_t) u;

-- ============================================================
-- 3. 设计边界保持（本次不放开）
-- ============================================================
SELECT '=== 3 design boundaries ===';

-- 3a. DET 列 ORDER BY → 仍报排序错误（排序比较仅支持 OPE 段）
SELECT a FROM ds_t ORDER BY a;
-- 3b. OPE 列 ORDER BY → 正常，不误伤
SELECT '3b_ope_orderby', id FROM ds_t ORDER BY b, id;
-- 3c. 纯 HE 列（c，无 DET/OPE 密文段）参与去重 → 报错（无论走 hash 还是 sort 都无可比段）
SELECT c FROM ds_t UNION SELECT c FROM ds_t;

-- ============================================================
-- 4. 清理
-- ============================================================
SELECT '=== 4 CLEANUP ===';
DROP TABLE ds_t CASCADE;
DROP DEK ds_dek;
DROP DEK ds_he_add_dek;
DROP DEK ds_he_mul_dek;
DROP KEK ds_kek CASCADE;
DROP KEK ds_he_kek CASCADE;
