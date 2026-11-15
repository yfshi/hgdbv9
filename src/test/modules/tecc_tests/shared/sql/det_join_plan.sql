-- det_join_plan.sql — DET 等值 JOIN 规划期分流专项回归
--
-- 背景：DET 加密列（算法为 det 或 det,search，不含 ope；det 与 ope 互斥）
-- 的等值 JOIN 语义由 Hash Join 承载（等值比较 cipherdata_eq 用 DET 段
-- memcmp、哈希函数用 DET 段确定性哈希）。但密态类型注册了 btree 操作符族，
-- 优化器静态判定密态列"可排序"，可能选 Merge Join；而排序比较仅支持 OPE
-- 段，导致 DET 列等值 JOIN 运行期报错"cannot compare encrypted values for
-- ordering"。规划期分流在 check_mergejoinable 拦截"引用无 OPE 加密列的
-- 等值子句"，使 DET 等值 JOIN 稳定落 Hash Join；同时保持比较语义不变：
-- 范围比较 / 排序仍要求 OPE，DET 列不满足时明确报错，不引入密文序乱序。
--
-- 本用例覆盖：
--   1. 基础 DET 等值 JOIN（INNER / 逗号 WHERE / LEFT / RIGHT / FULL / 自连接）
--   2. 多表与嵌套 JOIN（三表串联 / 子查询 / 相关子查询 IN / =ANY / EXISTS / NOT EXISTS）
--   3. 混合场景（DET 与 OPE 混用 JOIN / 一侧 DET 一侧 OPE / JOIN+GROUP BY）
--   4. 集合操作（UNION / INTERSECT / EXCEPT 及结果 JOIN）
--   5. JOIN 相关报错场景（范围 JOIN、JOIN 条件类型不匹配等；含排序语义保持）
--   6. 跨 DEK（不同 DEK 等值 JOIN 恒不等 0 行，不报错）
--   7. 复杂多表 JOIN（内联 / 外联 / 左外 / 右外 / 多层嵌套 / 混合条件）
--
-- 说明：0 行结果用 count(*) 断言（-A -t 格式下 0 行无输出）；结果集按
-- plain 列排序（id 等）保证输出确定性。

-- ============================================================
-- 0. Setup
-- ============================================================
DROP KEK IF EXISTS djk_kek_b CASCADE;
DROP KEK IF EXISTS djk_kek_c CASCADE;
DROP DEK IF EXISTS djk_dek_b CASCADE;
DROP DEK IF EXISTS djk_dek_c CASCADE;
CREATE KEK djk_kek_b WITH (key_store='localkms', key_path='/tmp/djk_kek_b.key');
CREATE KEK djk_kek_c WITH (key_store='localkms', key_path='/tmp/djk_kek_c.key');
CREATE DEK djk_dek_b WITH VALUES(KEK=djk_kek_b);
CREATE DEK djk_dek_c WITH VALUES(KEK=djk_kek_c);

DROP TABLE IF EXISTS dj_t_a, dj_t_d, dj_t_e, dj_t_f, dj_t_o CASCADE;
-- dj_t_a: v 为 DET 列（DET JOIN 主列），s 为 OPE 列（OPE JOIN / 排序）
CREATE TABLE dj_t_a(id serial, v int4_enc ENCRYPTED WITH (DEK='djk_dek_b', SUPPORT_ALGO='det'),
                    s int4_enc ENCRYPTED WITH (DEK='djk_dek_b', SUPPORT_ALGO='ope'));
-- dj_t_d / dj_t_f: v 为 DET 列（与 dj_t_a 同 DEK，复杂多表 JOIN 用）
CREATE TABLE dj_t_d(id serial, v int4_enc ENCRYPTED WITH (DEK='djk_dek_b', SUPPORT_ALGO='det'));
CREATE TABLE dj_t_f(id serial, v int4_enc ENCRYPTED WITH (DEK='djk_dek_b', SUPPORT_ALGO='det'));
-- dj_t_e: v 为 DET 列（不同 DEK djk_dek_c，用于跨 DEK 等值场景），
--         s 为 OPE 列（不同 DEK djk_dek_c，用于跨 DEK 范围场景）
CREATE TABLE dj_t_e(id serial, v int4_enc ENCRYPTED WITH (DEK='djk_dek_c', SUPPORT_ALGO='det'),
                    s int4_enc ENCRYPTED WITH (DEK='djk_dek_c', SUPPORT_ALGO='ope'));
-- dj_t_o: v 为 OPE 列（一侧 DET 一侧 OPE 场景）
CREATE TABLE dj_t_o(id serial, v int4_enc ENCRYPTED WITH (DEK='djk_dek_b', SUPPORT_ALGO='ope'));
-- dj_t_p: 普通列（JOIN 类型不匹配场景）
CREATE TABLE dj_t_p(id serial, c int);

INSERT INTO dj_t_a(v,s) VALUES (1,10),(2,20),(3,30),(3,30),(5,50);
INSERT INTO dj_t_d(v) VALUES (1),(3),(5),(7);
INSERT INTO dj_t_f(v) VALUES (1),(3),(5);
INSERT INTO dj_t_e(v,s) VALUES (3,30),(4,40);
INSERT INTO dj_t_o(v) VALUES (1),(3),(5);
INSERT INTO dj_t_p(c) VALUES (1),(3);

-- ============================================================
-- 1. 基础 DET 等值 JOIN
-- ============================================================
SELECT '=== 1 basic DET JOIN ===';

-- 1a. EXPLAIN：DET 等值 JOIN 应走 Hash Join（规划期分流生效，不再 Merge Join 报错）
EXPLAIN (COSTS OFF) SELECT a.id FROM dj_t_a a JOIN dj_t_d d ON a.v=d.v ORDER BY a.id;
-- 1b. INNER JOIN（a.v 与 d.v 均为 DET，同 DEK）
SELECT '1b_inner', a.id FROM dj_t_a a JOIN dj_t_d d ON a.v=d.v ORDER BY 2;
-- 1c. 逗号 + WHERE 形式
SELECT '1c_where', a.id FROM dj_t_a a, dj_t_d d WHERE a.v=d.v ORDER BY 2;
-- 1d. LEFT OUTER JOIN（d 全保留，a 匹配情况）
SELECT '1d_left', d.id, a.v IS NOT NULL AS matched
FROM dj_t_d d LEFT JOIN dj_t_a a ON d.v=a.v ORDER BY 1;
-- 1e. RIGHT OUTER JOIN
SELECT '1e_right', d.id, a.v IS NOT NULL AS matched
FROM dj_t_a a RIGHT JOIN dj_t_d d ON a.v=d.v ORDER BY 1;
-- 1f. FULL OUTER JOIN（a.id 与 d.id 的匹配组合）
SELECT '1f_full', a.id, d.id
FROM dj_t_a a FULL JOIN dj_t_d d ON a.v=d.v ORDER BY 1 NULLS LAST, 2 NULLS LAST;
-- 1g. 自连接（同表 DET 等值，id 不同）
SELECT '1g_self', a.id, b.id FROM dj_t_a a JOIN dj_t_a b ON a.v=b.v AND a.id<>b.id ORDER BY 2;

-- ============================================================
-- 2. 多表与嵌套 JOIN
-- ============================================================
SELECT '=== 2 multi-table / subquery JOIN ===';

-- 2a. 三表 DET 串联 JOIN（a.v=d.v AND d.v=f.v，全 DET 同 DEK）
SELECT '2a_3way', count(*)
FROM dj_t_a a JOIN dj_t_d d ON a.v=d.v JOIN dj_t_f f ON d.v=f.v;
-- 2b. JOIN + 子查询（UNION ALL 结果作为一侧；a.v 基表 DET 列触发拦截）
SELECT '2b_subq', count(*)
FROM dj_t_a a JOIN (SELECT v FROM dj_t_d UNION ALL SELECT v FROM dj_t_e) u ON a.v=u.v;
-- 2c. 相关子查询：IN
SELECT '2c_in', a.id FROM dj_t_a a WHERE a.v IN (SELECT v FROM dj_t_d) ORDER BY 1;
-- 2d. 相关子查询：= ANY
SELECT '2d_any', a.id FROM dj_t_a a WHERE a.v = ANY (SELECT v FROM dj_t_d) ORDER BY 1;
-- 2e. EXISTS
SELECT '2e_exists', a.id FROM dj_t_a a WHERE EXISTS (SELECT 1 FROM dj_t_d d WHERE d.v=a.v) ORDER BY 1;
-- 2f. NOT EXISTS
SELECT '2f_not_exists', a.id FROM dj_t_a a WHERE NOT EXISTS (SELECT 1 FROM dj_t_d d WHERE d.v=a.v) ORDER BY 1;

-- ============================================================
-- 3. 混合场景
-- ============================================================
SELECT '=== 3 mixed scenarios ===';

-- 3a. 一侧 DET 一侧 OPE 等值 JOIN：密文段不同（DET 段 vs OPE 段）恒不等 → 0 行，
--     且规划期分流保证不报 Merge Join 排序错误（修复前会报错）
SELECT '3a_det_ope_eq', count(*)
FROM dj_t_a a JOIN dj_t_o o ON a.v=o.v;
-- 3b. DET 与 OPE 条件混用 JOIN（自连接，v DET 等值 + s OPE 等值）
SELECT '3b_mix', a.id FROM dj_t_a a JOIN dj_t_a b ON a.v=b.v AND a.s=b.s
  WHERE a.id<>b.id ORDER BY 2;
-- 3c. JOIN + GROUP BY（分组键为 plain id，JOIN 条件 DET 等值）
SELECT '3c_join_group', d.id, count(a.id)
FROM dj_t_d d LEFT JOIN dj_t_a a ON d.v=a.v GROUP BY d.id ORDER BY 2;

-- ============================================================
-- 4. 集合操作
-- ============================================================
SELECT '=== 4 set ops ===';

-- 4a. UNION（去重，依赖密文哈希）
SELECT '4a_union', count(*) FROM (SELECT v FROM dj_t_a UNION SELECT v FROM dj_t_d) u;
-- 4b. INTERSECT
SELECT '4b_intersect', count(*) FROM (SELECT v FROM dj_t_a INTERSECT SELECT v FROM dj_t_d) u;
-- 4c. EXCEPT
SELECT '4c_except', count(*) FROM (SELECT v FROM dj_t_a EXCEPT SELECT v FROM dj_t_d) u;
-- 4d. UNION 结果 JOIN DET 表（u.v 与 d.v 均为 DET 同 DEK）
SELECT '4d_union_join', count(*)
FROM (SELECT v FROM dj_t_a UNION SELECT v FROM dj_t_d) u
JOIN dj_t_d d ON u.v=d.v;

-- ============================================================
-- 5. JOIN 相关报错场景（排序语义保持，不引入密文序乱序）
-- ============================================================
SELECT '=== 5 JOIN error scenarios ===';

-- 5a. DET 列 ORDER BY → 报错（排序比较仅 OPE，DET 无 OPE 段）
SELECT v FROM dj_t_a ORDER BY v;
-- 5b. INNER JOIN 范围条件（a.v < d.v）→ 报错（范围比较需 OPE）
SELECT a.id FROM dj_t_a a JOIN dj_t_d d ON a.v < d.v;
-- 5c. WHERE 常量范围查询（v < const）→ 前端拦截（常量加密需 OPE）
SELECT v FROM dj_t_a WHERE v < 3;
-- 5d. LEFT OUTER JOIN 范围条件 → 报错（范围比较需 OPE）
SELECT d.id FROM dj_t_d d LEFT JOIN dj_t_a a ON a.v < d.v;
-- 5e. RIGHT OUTER JOIN 范围条件 → 报错
SELECT a.id FROM dj_t_a a RIGHT JOIN dj_t_d d ON a.v < d.v;
-- 5f. FULL OUTER JOIN 范围条件 → 报错（FULL JOIN 仅支持 merge/hash joinable 条件）
SELECT a.id FROM dj_t_a a FULL JOIN dj_t_d d ON a.v < d.v;
-- 5g. JOIN ON 加密列 < 常量 → 报错（无 int4_enc < integer 跨类型操作符）
SELECT a.id FROM dj_t_a a JOIN dj_t_d d ON a.v < 3;
-- 5h. JOIN 加密列 = 普通列 → 报错（无 int4_enc = integer 跨类型操作符）
SELECT a.id FROM dj_t_a a JOIN dj_t_p p ON a.v = p.c;
-- 5i. JOIN ON 加密列 LIKE → 报错（int4_enc 无 LIKE 操作符）
SELECT a.id FROM dj_t_a a JOIN dj_t_d d ON a.v LIKE '1%';
-- 5j. OPE 列 ORDER BY → 正常（不误伤）
SELECT '5j_ope_orderby', a.id FROM dj_t_a a ORDER BY a.s, a.id;

-- ============================================================
-- 6. 跨 DEK（JOIN 场景；WHERE/子查询/LIKE 跨 DEK 见 cross_dek_error.sql）
-- ============================================================
SELECT '=== 6 cross DEK ===';

-- 6a. 不同 DEK 等值 JOIN → 前端拦截报错（不同 DEK 的加密列禁止比较）
SELECT '6a_cross_dek', count(*)
FROM dj_t_a a JOIN dj_t_e e ON a.v=e.v;
-- 6b. 同 DEK 跨表匹配计数（对照 1b 结果，正常）
SELECT '6b_same_dek', count(*) FROM dj_t_a a JOIN dj_t_d d ON a.v=d.v;
-- 6c. LEFT JOIN 等值 → 报错
SELECT '6c_left_join', count(*) FROM dj_t_a a LEFT JOIN dj_t_e e ON a.v=e.v;
-- 6d. RIGHT JOIN 等值 → 报错
SELECT '6d_right_join', count(*) FROM dj_t_a a RIGHT JOIN dj_t_e e ON a.v=e.v;
-- 6e. FULL JOIN 等值 → 报错
SELECT '6e_full_join', count(*) FROM dj_t_a a FULL JOIN dj_t_e e ON a.v=e.v;
-- 6f. 范围 JOIN（OPE 跨 DEK）→ 报错
SELECT '6f_ope_range_join', count(*) FROM dj_t_a a JOIN dj_t_e e ON a.s < e.s;

-- ============================================================
-- 7. 复杂多表 JOIN（内联 / 外联 / 左外 / 右外 / 多层嵌套 / 混合条件）
-- ============================================================
SELECT '=== 7 complex multi-table JOIN ===';

-- 7a. 三表嵌套 INNER：(a JOIN d) JOIN f（括号显式嵌套）
SELECT '7a_nest_inner', count(*)
FROM (dj_t_a a JOIN dj_t_d d ON a.v=d.v) JOIN dj_t_f f ON d.v=f.v;
-- 7b. 四表嵌套（含自连接）：((a JOIN d) JOIN f) JOIN b ON f.v=b.v
SELECT '7b_nest_4way', count(*)
FROM ((dj_t_a a JOIN dj_t_d d ON a.v=d.v) JOIN dj_t_f f ON d.v=f.v)
     JOIN dj_t_a b ON f.v=b.v;
-- 7c. LEFT OUTER 后接 INNER：a LEFT JOIN d 后 INNER JOIN f（过滤 NULL 侧）
SELECT '7c_left_then_inner', count(*)
FROM dj_t_a a LEFT JOIN dj_t_d d ON a.v=d.v JOIN dj_t_f f ON d.v=f.v;
-- 7d. RIGHT OUTER 后接 INNER：a RIGHT JOIN d 后 INNER JOIN f
SELECT '7d_right_then_inner', count(*)
FROM dj_t_a a RIGHT JOIN dj_t_d d ON a.v=d.v JOIN dj_t_f f ON d.v=f.v;
-- 7e. FULL OUTER 后接 INNER：FULL JOIN 后 INNER JOIN f（未匹配行被过滤）
SELECT '7e_full_then_inner', count(*)
FROM dj_t_a a FULL JOIN dj_t_d d ON a.v=d.v JOIN dj_t_f f ON d.v=f.v;
-- 7f. 多层 LEFT OUTER：a LEFT JOIN d LEFT JOIN f（a 全保留）
SELECT '7f_multi_left', count(*)
FROM dj_t_a a LEFT JOIN dj_t_d d ON a.v=d.v LEFT JOIN dj_t_f f ON d.v=f.v;
-- 7g. LEFT OUTER 后接 RIGHT OUTER：a LEFT JOIN d RIGHT JOIN f（f 全保留）
SELECT '7g_left_then_right', count(*)
FROM dj_t_a a LEFT JOIN dj_t_d d ON a.v=d.v RIGHT JOIN dj_t_f f ON d.v=f.v;
-- 7h. OPE 范围 JOIN 正常（范围 JOIN 仅 OPE 支持，不误伤；s 为 OPE 列）
SELECT '7h_ope_range', count(*) FROM dj_t_a a JOIN dj_t_a b ON a.s > b.s;
-- 7i. 混合条件：DET 等值 + OPE 等值跨表（a.s = f.s 需 f 有 s 列，用 a 自连接）
SELECT '7i_mix_equality', a.id, b.id
FROM dj_t_a a JOIN dj_t_a b ON a.v=b.v AND a.s=b.s WHERE a.id<>b.id ORDER BY 2;

-- ============================================================
-- 9. 清理
-- ============================================================
SELECT '=== 9 CLEANUP ===';
DROP TABLE dj_t_a, dj_t_d, dj_t_e, dj_t_f, dj_t_o, dj_t_p CASCADE;
DROP DEK djk_dek_b;
DROP DEK djk_dek_c;
DROP KEK djk_kek_b CASCADE;
DROP KEK djk_kek_c CASCADE;
