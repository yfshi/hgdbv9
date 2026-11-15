-- ============================================================
-- view_order_by_ope.sql — 视图定义 ORDER BY 加密列 OPE 校验回归测试
-- 覆盖 Bug 9151: 视图定义中 ORDER BY 无 OPE 的加密列应在创建时即报错
-- ============================================================
-- 背景：加密列只有启用 OPE（保序加密，密文大小顺序与明文一致）才能正确排序。
-- 普通查询会立即执行，服务端在真正需要比较两行时才报错（数据少时放行）；而视图
-- 定义只被存储、不立即执行，服务端要到某次查询该视图时才报错——错误被推迟且可能
-- 长期潜伏。libpq 在 CREATE VIEW 时即报错，本用例验证 pgjdbc 对齐该 fail-fast 行为。
--
-- 重要：本文件所有表都必须含【≥2 行且值互异】的数据。服务端的排序校验依赖实际
-- 数据量（0/1 行无需比较则不报错），数据不足会让本用例产生假阴性——即便前端拦截
-- 失效也测不出来。
--
-- 范围说明：本文件只放【JDBC 与 libpq 行为一致】的用例。以下「pgjdbc 有意超集」
-- 场景 libpq 不拦、pgjdbc 拦（放进本文件会被 hgtest 判为 real_bug 假阳性），
-- 故只做 JUnit 覆盖（TeccSemanticAnalyzerTest）：
--   1) UNION 视图外层 ORDER BY（libpq 在 SetOp 分支跳过）；
--   2) 视图内层 GROUP BY / DISTINCT（libpq 的 tecc_semantic.c 检查模块未接线）。
--
-- 注：物化视图的 ORDER BY 曾属上述超集，现已改为「与 psql 一致地放行」并纳入本文件（见 §7）。
--    依据（实测）：物化视图的查询在**创建时就会执行**，服务端当场按实际数据量判定——
--    单行表可成功建成，pgjdbc 一律阻断属过度拒绝。而 GROUP BY / DISTINCT 即便单行表
--    也被服务端拒绝（哈希必然发生，与数据量无关），故仍维持阻断（属上述第 2 类超集）。
-- ============================================================

-- ============================================================
-- 0. Setup
-- ============================================================
CREATE KEK kek_vobo WITH (key_store = 'localkms', key_path = '/tmp/kek_vobo.key');
CREATE DEK dek_vobo_det WITH VALUES(KEK = kek_vobo);
CREATE DEK dek_vobo_ope WITH VALUES(KEK = kek_vobo);

CREATE TABLE t_vobo (
    id        int,
    col_det   text_enc encrypted with(dek=dek_vobo_det, support_algo='det'),
    col_ope   text_enc encrypted with(dek=dek_vobo_ope, support_algo='ope'),
    col_plain text
);
INSERT INTO t_vobo VALUES (1,'Alice','Alice','Alice'),(2,'Bob','Bob','Bob'),(3,'Carol','Carol','Carol');

-- ============================================================
-- 1. 普通视图：ORDER BY 无 OPE 加密列 → 报错（JDBC 前端拦 / psql 前端拦，两侧一致）
-- ============================================================
CREATE VIEW v_vobo_det AS SELECT id,col_det FROM t_vobo ORDER BY col_det;

-- ============================================================
-- 2. 普通视图：ORDER BY OPE 加密列 → 正常
-- ============================================================
CREATE VIEW v_vobo_ope AS SELECT id,col_ope FROM t_vobo ORDER BY col_ope;
SELECT * FROM v_vobo_ope ORDER BY id;
DROP VIEW v_vobo_ope;

-- ============================================================
-- 3. 普通视图：ORDER BY 非加密列 → 正常
-- ============================================================
CREATE VIEW v_vobo_plain AS SELECT id,col_plain FROM t_vobo ORDER BY col_plain;
SELECT * FROM v_vobo_plain ORDER BY id;
DROP VIEW v_vobo_plain;

-- ============================================================
-- 4. 普通视图：引用加密列但无 ORDER BY → 正常（既有行为不回归）
-- ============================================================
CREATE VIEW v_vobo_nosort AS SELECT id,col_det FROM t_vobo;
SELECT * FROM v_vobo_nosort ORDER BY id;
DROP VIEW v_vobo_nosort;

-- ============================================================
-- 5. CREATE VIEW 的其它形态（列名列表 / WITH view_option）→ 同受约束，正常通过
-- ============================================================
CREATE VIEW v_vobo_cols (a, b) AS SELECT id,col_ope FROM t_vobo;
DROP VIEW v_vobo_cols;
CREATE VIEW v_vobo_opts WITH (security_barrier=true) AS SELECT id,col_ope FROM t_vobo;
DROP VIEW v_vobo_opts;

-- ============================================================
-- 6. 对照：普通 SELECT 的 ORDER BY 无 OPE 加密列 → 由服务端报错（两侧一致）
--    JDBC 前端只告警不阻断，SQL 下发后服务端在需要比较时（≥2 行）报错
-- ============================================================
SELECT id,col_det FROM t_vobo ORDER BY col_det;

-- ============================================================
-- 7. 物化视图：ORDER BY 无 OPE 加密列 → **放行**（JDBC 与 psql 一致）
--    物化视图的查询在创建时即执行，服务端按实际数据量判定；本表 3 行且值互异，
--    故服务端会在创建时报错「cannot compare encrypted values…」——两侧同样报错即一致。
--    单行表可成功建成的情形由本文件之外的 JUnit 用例覆盖（TeccSemanticAnalyzerTest），
--    因为 psql 与 JDBC 是否报错取决于数据量，放进本文件会产生数据相关的假阳性。
-- ============================================================
CREATE MATERIALIZED VIEW mv_vobo_det AS SELECT id,col_det FROM t_vobo ORDER BY col_det;

-- ============================================================
-- 8. 清理
-- ============================================================
DROP MATERIALIZED VIEW IF EXISTS mv_vobo_det CASCADE;
DROP VIEW IF EXISTS v_vobo_det CASCADE;
DROP VIEW IF EXISTS v_vobo_ope CASCADE;
DROP VIEW IF EXISTS v_vobo_plain CASCADE;
DROP VIEW IF EXISTS v_vobo_nosort CASCADE;
DROP TABLE IF EXISTS t_vobo CASCADE;
DROP DEK IF EXISTS dek_vobo_det CASCADE;
DROP DEK IF EXISTS dek_vobo_ope CASCADE;
DROP KEK IF EXISTS kek_vobo CASCADE;
