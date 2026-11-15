-- ============================================================
-- he_new_column_paths.sql — 同态列在"目标列还不存在"的三条写入路径上的支持
--
-- 覆盖面（三条路径 + 两种声明）：
--   1. CTAS 的结果表列：CREATE TABLE t AS SELECT 常量::密态类型 ENCRYPTED WITH (...)
--   2. 视图 / 物化视图定义里的列：CREATE [MATERIALIZED] VIEW ... 同上
--   3. 加密列的 DEFAULT 值：CREATE TABLE t (a 密态类型 ENCRYPTED WITH (...) DEFAULT 常量)
--   声明形态：混合（DEK + HE_ADD_DEK + SUPPORT_ALGO='det,he-add'）与纯同态
--   （只写 HE_ADD_DEK + SUPPORT_ALGO='he-add'）。
--
-- 为什么放 libpq/（不共享给 JDBC）：这三条路径的同态支持是 libpq 客户端这一侧
-- 的实现——把 ENCRYPTED WITH 里的 HE_ADD_DEK/HE_MUL_DEK 解析出来传给加密入口
-- （这些列此刻还没有元数据，没有别的来源）。Java 驱动的同态实现由 Java 侧同事
-- 同步，未纳入本例，故不做跨驱动共享。
--
-- 判定方式：回读明文 + 同态聚合（sum）。密文缺 HE 段时 sum 取不到值，
-- 而纯 he-add 列本来就不返回明文，因此两种列各自看可判定的那一个。
-- 注意事项：本用例的密文不进期望文件（只打印明文与聚合结果），与随机段无关。
-- ============================================================

\! rm -f "$TECC_KEK_PATH/hencp_asym.priv" "$TECC_KEK_PATH/hencp_asym.pub" "$TECC_KEK_PATH/hencp_sym.key" 2>/dev/null
\! tecc_ktool generate --key-name hencp_asym --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1
\! tecc_ktool generate --key-name hencp_sym --key-algo sm4 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1

DROP TABLE IF EXISTS hencp_plain CASCADE;
DROP TABLE IF EXISTS hencp_ctas CASCADE;
DROP TABLE IF EXISTS hencp_ctas_pure CASCADE;
DROP TABLE IF EXISTS hencp_dflt CASCADE;
DROP TABLE IF EXISTS hencp_dflt_pure CASCADE;
DROP VIEW IF EXISTS hencp_view;
DROP MATERIALIZED VIEW IF EXISTS hencp_mview;
DROP DEK IF EXISTS hencp_dek_add CASCADE;
DROP DEK IF EXISTS hencp_dek_sym CASCADE;
DROP KEK IF EXISTS hencp_kek_asym CASCADE;
DROP KEK IF EXISTS hencp_kek_sym CASCADE;

-- 对称 DEK（DET）与同态 DEK（he-add）分开：一列多 DEK，与普通密态列的写法一致。
CREATE KEK hencp_kek_sym WITH (
    key_store = 'localkms',
    key_algo  = 'sm4',
    key_path  = '%KEKPATH%/hencp_sym.key'
);
CREATE DEK hencp_dek_sym WITH VALUES(KEK = hencp_kek_sym);
CREATE KEK hencp_kek_asym WITH (
    key_store = 'localkms',
    key_algo  = 'sm2',
    key_path  = '%KEKPATH%/hencp_asym.priv'
);
CREATE DEK hencp_dek_add WITH VALUES(KEK = hencp_kek_asym, KEY_ALGO = 'he-add');

-- ============================================================
-- 0. 对照：普通建表 + INSERT（列元数据可用，HE DEK 本来就解析得到）
-- ============================================================
SELECT '=== 0 对照：普通列 + INSERT ===' AS stage;
CREATE TABLE hencp_plain (
    a int4_enc ENCRYPTED WITH (DEK = 'hencp_dek_sym', HE_ADD_DEK = 'hencp_dek_add', SUPPORT_ALGO = 'det,he-add')
);
INSERT INTO hencp_plain VALUES (5);
SELECT a, a = 5 AS eq, (SELECT sum(a) FROM hencp_plain) AS sum FROM hencp_plain;

-- ============================================================
-- 1. 路径一：CTAS 的结果表列
-- ============================================================
SELECT '=== 1a CTAS：混合 det,he-add ===' AS stage;
CREATE TABLE hencp_ctas AS
    SELECT 5::int4_enc ENCRYPTED WITH (DEK = 'hencp_dek_sym', HE_ADD_DEK = 'hencp_dek_add', SUPPORT_ALGO = 'det,he-add') AS a;
SELECT a, a = 5 AS eq, (SELECT sum(a) FROM hencp_ctas) AS sum FROM hencp_ctas;

SELECT '=== 1b CTAS：纯同态（无 DEK=，只有 HE_ADD_DEK）===' AS stage;
CREATE TABLE hencp_ctas_pure AS
    SELECT 5::int4_enc ENCRYPTED WITH (HE_ADD_DEK = 'hencp_dek_add', SUPPORT_ALGO = 'he-add') AS a;
SELECT (SELECT sum(a) FROM hencp_ctas_pure) AS sum FROM hencp_ctas_pure;

-- ============================================================
-- 2. 路径二：视图 / 物化视图定义里的列
-- ============================================================
-- 视图/物化视图的派生列没有加密元数据（前端因此不把它当密态列），按该列过滤的比较
-- 常量不会加密、服务端报 operator does not exist —— 这是另一条已登记的缺口（见 plan
-- 文档"视图/物化视图的派生化密态列没有加密元数据"）。本节只验证写入与回读：
-- 回读靠密文自身结构，同态聚合靠 CTAS 建表登记的同态元数据。
SELECT '=== 2a 视图定义（混合）===' AS stage;
CREATE VIEW hencp_view AS
    SELECT 5::int4_enc ENCRYPTED WITH (DEK = 'hencp_dek_sym', HE_ADD_DEK = 'hencp_dek_add', SUPPORT_ALGO = 'det,he-add') AS a;
SELECT a FROM hencp_view;

SELECT '=== 2b 物化视图定义（混合）===' AS stage;
CREATE MATERIALIZED VIEW hencp_mview AS
    SELECT 5::int4_enc ENCRYPTED WITH (DEK = 'hencp_dek_sym', HE_ADD_DEK = 'hencp_dek_add', SUPPORT_ALGO = 'det,he-add') AS a;
SELECT a, (SELECT sum(a) FROM hencp_mview) AS sum FROM hencp_mview;

-- ============================================================
-- 3. 路径三：加密列的 DEFAULT 值
-- ============================================================
SELECT '=== 3a DEFAULT：混合 det,he-add ===' AS stage;
CREATE TABLE hencp_dflt (
    a int4_enc ENCRYPTED WITH (DEK = 'hencp_dek_sym', HE_ADD_DEK = 'hencp_dek_add', SUPPORT_ALGO = 'det,he-add')
        DEFAULT 5
);
INSERT INTO hencp_dflt DEFAULT VALUES;
SELECT a, a = 5 AS eq, (SELECT sum(a) FROM hencp_dflt) AS sum FROM hencp_dflt;

SELECT '=== 3b DEFAULT：纯同态 ===' AS stage;
CREATE TABLE hencp_dflt_pure (
    a int4_enc ENCRYPTED WITH (HE_ADD_DEK = 'hencp_dek_add', SUPPORT_ALGO = 'he-add')
        DEFAULT 7
);
INSERT INTO hencp_dflt_pure DEFAULT VALUES;
SELECT (SELECT sum(a) FROM hencp_dflt_pure) AS sum FROM hencp_dflt_pure;

-- 清理
DROP TABLE IF EXISTS hencp_plain CASCADE;
DROP TABLE IF EXISTS hencp_ctas CASCADE;
DROP TABLE IF EXISTS hencp_ctas_pure CASCADE;
DROP TABLE IF EXISTS hencp_dflt CASCADE;
DROP TABLE IF EXISTS hencp_dflt_pure CASCADE;
DROP VIEW IF EXISTS hencp_view;
DROP MATERIALIZED VIEW IF EXISTS hencp_mview;
DROP DEK IF EXISTS hencp_dek_add CASCADE;
DROP DEK IF EXISTS hencp_dek_sym CASCADE;
DROP KEK IF EXISTS hencp_kek_asym CASCADE;
DROP KEK IF EXISTS hencp_kek_sym CASCADE;
\! rm -f "$TECC_KEK_PATH/hencp_asym.priv" "$TECC_KEK_PATH/hencp_asym.pub" "$TECC_KEK_PATH/hencp_sym.key" 2>/dev/null
