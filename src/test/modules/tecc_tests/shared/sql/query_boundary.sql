--
-- query_boundary.sql — 密态查询语法边界专项测试
--
-- 依据 plan/encrypted_query_syntax_boundary.md 评估结论补充测试：
--   1. 集合操作 UNION/INTERSECT/EXCEPT（含去重分支）
--   2. 相关子查询（IN/=ANY/NOT EXISTS/EXISTS）
--   3. JOIN 算法组合（DET 等值 / OPE 范围 / 混用 / 跨 DEK 拦截）
--
-- 背景：DET-only 等值 JOIN 依赖 cipherdata_cmp 的 DET fallback
-- （cipherdata_operators.c），默认 Merge Join 计划可用；
-- ORDER BY 对 DET-only 列返回密文序（非明文序），需 OPE 才能语义排序。
-- ============================================================

-- ============================================================
-- 0. Setup
-- ============================================================
DROP KEK IF EXISTS kek_b CASCADE;
DROP KEK IF EXISTS kek_c CASCADE;
DROP DEK IF EXISTS dek_b CASCADE;
DROP DEK IF EXISTS dek_c CASCADE;
CREATE KEK kek_b WITH (key_store='localkms', key_path='/tmp/kek_b.key');
CREATE KEK kek_c WITH (key_store='localkms', key_path='/tmp/kek_c.key');
CREATE DEK dek_b WITH VALUES(KEK=kek_b);
CREATE DEK dek_c WITH VALUES(KEK=kek_c);

DROP TABLE IF EXISTS t_a, t_d, t_e CASCADE;
CREATE TABLE t_a(id serial, v int4_enc ENCRYPTED WITH (DEK='dek_b', SUPPORT_ALGO='det'),
                 s int4_enc ENCRYPTED WITH (DEK='dek_b', SUPPORT_ALGO='ope'));
CREATE TABLE t_d(id serial, v int4_enc ENCRYPTED WITH (DEK='dek_b', SUPPORT_ALGO='det'));
CREATE TABLE t_e(id serial, v int4_enc ENCRYPTED WITH (DEK='dek_c', SUPPORT_ALGO='det'));

INSERT INTO t_a(v,s) VALUES (1,10),(2,20),(3,30),(3,30);
INSERT INTO t_d(v) VALUES (1),(3),(5);
INSERT INTO t_e(v) VALUES (3),(4);

-- ============================================================
-- 1. 集合操作：UNION（去重）/INTERSECT/EXCEPT
--    密文列两侧同 DEK 同算法，去重依赖 DET 密文哈希。
--    断言方式（确定性，不依赖行序）：
--      * total = 结果集行数 count(*)
--      * mem   = 结果集与期望值表 expect_v（同 DEK 的 DET 列）做
--                LEFT JOIN（u.v = e.v 双加密列 DET 等值），count(u.v)
--                逐值成员计数；e.id 是 plain serial，ORDER BY 稳定。
--    注意：int4_enc 列不能与 typed integer（VALUES 列、子查询/UNION
--    内字面量）比较——只有直接表列 = unknown 字面量才隐式转换；
--    DET 密文字节序还随随机 DEK 变化，故也不可对密文列 ORDER BY。
-- ============================================================
SELECT '=== 1 set ops ===';

-- 期望值表（同 dek_b DET，供逐值成员计数）
CREATE TABLE expect_v(id serial, v int4_enc ENCRYPTED WITH (DEK='dek_b', SUPPORT_ALGO='det'));
INSERT INTO expect_v(v) VALUES (1),(2),(3),(5);

-- 1a UNION（去重）: {1,2,3,5}（t_a{v1,v2,v3,v3} ∪ t_d{v1,v3,v5}）
SELECT '1a_total', count(*) FROM (SELECT v FROM t_a UNION SELECT v FROM t_d) u;
SELECT '1a_mem', e.id, count(u.v)
  FROM expect_v e
  LEFT JOIN (SELECT v FROM t_a UNION SELECT v FROM t_d) u ON u.v = e.v
  GROUP BY e.id ORDER BY 2;

-- 1b UNION ALL（不去重）: 多重集 v1×2, v2×1, v3×3, v5×1 → 共 7 行
SELECT '1b_total', count(*) FROM (SELECT v FROM t_a UNION ALL SELECT v FROM t_d) u;
SELECT '1b_mem', e.id, count(u.v)
  FROM expect_v e
  LEFT JOIN (SELECT v FROM t_a UNION ALL SELECT v FROM t_d) u ON u.v = e.v
  GROUP BY e.id ORDER BY 2;

-- 1c INTERSECT: {1,3}
SELECT '1c_total', count(*) FROM (SELECT v FROM t_a INTERSECT SELECT v FROM t_d) u;
SELECT '1c_mem', e.id, count(u.v)
  FROM expect_v e
  LEFT JOIN (SELECT v FROM t_a INTERSECT SELECT v FROM t_d) u ON u.v = e.v
  GROUP BY e.id ORDER BY 2;

-- 1d EXCEPT: {2}
SELECT '1d_total', count(*) FROM (SELECT v FROM t_a EXCEPT SELECT v FROM t_d) u;
SELECT '1d_mem', e.id, count(u.v)
  FROM expect_v e
  LEFT JOIN (SELECT v FROM t_a EXCEPT SELECT v FROM t_d) u ON u.v = e.v
  GROUP BY e.id ORDER BY 2;

-- 1e UNION 链（三段）：{1,2,3,5}
-- 注意：第三段的 WHERE 用 plain serial id 过滤（id=2 → t_d.v=3），不用
-- 加密列 = 字面量——后者在 UNION 子查询内报 int4_enc=integer（见上文说明）；
-- 加密列 WHERE 语义由场景 2 覆盖，此处测的是链式组合本身。
SELECT '1e_total', count(*) FROM (
  SELECT v FROM t_a UNION SELECT v FROM t_d
  UNION SELECT v FROM t_d WHERE id = 2) u;
SELECT '1e_mem', e.id, count(u.v)
  FROM expect_v e
  LEFT JOIN (
    SELECT v FROM t_a UNION SELECT v FROM t_d
    UNION SELECT v FROM t_d WHERE id = 2) u ON u.v = e.v
  GROUP BY e.id ORDER BY 2;

-- ============================================================
-- 2. 相关子查询（外层列在子查询 WHERE 中引用）
-- ============================================================
SELECT '=== 2 correlated subqueries ===';

-- 子查询列也用同 DEK 的 t_d（跨 DEK 的 EXISTS 语义见场景 3f 注释，
-- 此处测同 DEK 真实相关语义）；id 是 plain serial，ORDER BY 2 数值序稳定。
SELECT '2a_corr_in', id FROM t_a a WHERE v IN (SELECT v FROM t_d d WHERE d.id=a.id) ORDER BY 2;
SELECT '2b_corr_any', id FROM t_a a WHERE v = ANY (SELECT v FROM t_d d WHERE d.id=a.id) ORDER BY 2;
SELECT '2c_corr_not_exists', id FROM t_a a WHERE NOT EXISTS (SELECT 1 FROM t_d d WHERE d.v=a.v) ORDER BY 2;
SELECT '2d_corr_exists', id FROM t_a a WHERE EXISTS (SELECT 1 FROM t_d d WHERE d.v=a.v) ORDER BY 2;

-- ============================================================
-- 3. JOIN 算法组合
-- ============================================================
SELECT '=== 3 JOIN combos ===';

-- 结果集列 1 是常量 label，按列 2（a.id，plain serial）排序才稳定。
-- DET 等值 JOIN（默认 Merge Join 计划，依赖 cipherdata_cmp DET fallback）
SELECT '3a_det_join', a.id FROM t_a a JOIN t_d d ON a.v=d.v ORDER BY 2;
-- DET 等值 JOIN（逗号+WHERE 形式）
SELECT '3b_det_join_where', a.id FROM t_a a, t_d d WHERE a.v=d.v ORDER BY 2;
-- OPE 等值 JOIN（自连接）
SELECT '3c_ope_join', a.id FROM t_a a JOIN t_a b ON a.s=b.s AND a.id<>b.id ORDER BY 2;
-- OPE 范围 JOIN（a.s > b.s，仅 OPE 支持）
SELECT '3d_ope_range_join', a.id FROM t_a a JOIN t_a b ON a.s > b.s ORDER BY 2;
-- DET 与 OPE 混用 JOIN（两个条件各走各自密文）
SELECT '3e_det_ope_mix', a.id FROM t_a a JOIN t_a b ON a.v=b.v AND a.s=b.s
  WHERE a.id<>b.id ORDER BY 2;
-- 跨 DEK 等值 JOIN（不同 DEK 加密 → DET 密文逐字节不同，恒不等 → 0 行，
-- 无解密错误；同明文跨 DEK 不会匹配，属已知限制而非语法错误）
SELECT '3f_cross_dek', a.id FROM t_a a JOIN t_e e ON a.v=e.v ORDER BY 2;

-- ============================================================
-- 9. 清理
-- ============================================================
SELECT '=== 9 CLEANUP ===';

DROP TABLE t_a, t_d, t_e, expect_v CASCADE;
DROP DEK dek_b;
DROP DEK dek_c;
DROP KEK kek_b CASCADE;
DROP KEK kek_c CASCADE;
