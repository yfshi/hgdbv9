--
-- ctas_select_where.sql — libpq专有测试：CREATE TABLE AS 的密态常量处理
--
-- CTAS 曾只扫 targetList 上的 ENCRYPTED WITH typecast，WHERE 子句从未进入任何
-- 处理器：`CREATE TABLE t AS SELECT ... WHERE enc_col = 'x'` 的常量以明文发往
-- 服务端（报 invalid hex string），且与目标列表形态无关（空列表 / 显式列 /
-- 星号都失败）。修复后：无 ENCRYPTED WITH 目标列的 CTAS 交给 SELECT 处理器；
-- 带 ENCRYPTED WITH 的形态继续走原逻辑，由服务端派生结果表的列加密元数据。
--

DROP TABLE IF EXISTS test_ctas_src CASCADE;
DROP TABLE IF EXISTS test_ctas_a CASCADE;
DROP TABLE IF EXISTS test_ctas_b CASCADE;
DROP TABLE IF EXISTS test_ctas_c CASCADE;
DROP TABLE IF EXISTS test_ctas_d CASCADE;
DROP TABLE IF EXISTS test_ctas_enc CASCADE;
DROP DEK IF EXISTS dek_ctas CASCADE;
DROP KEK IF EXISTS kek_ctas CASCADE;

CREATE KEK kek_ctas WITH (key_store = 'localkms', key_path = '/tmp/kek_ctas.key');
CREATE DEK dek_ctas WITH VALUES(KEK = 'kek_ctas');

CREATE TABLE test_ctas_src (
    id       INT4,
    val_det  TEXT_ENC ENCRYPTED WITH (DEK = dek_ctas, SUPPORT_ALGO = 'DET'),
    val_ope  INT4_ENC ENCRYPTED WITH (DEK = dek_ctas, SUPPORT_ALGO = 'OPE'),
    plain    VARCHAR(50)
);
INSERT INTO test_ctas_src VALUES (1, 'alpha', 10, 'p1'), (2, 'beta', 20, 'p2'), (3, 'gamma', 30, 'p3');

-- ============================================================
-- 1. 空目标列表
-- ============================================================
CREATE TABLE test_ctas_a AS SELECT FROM test_ctas_src WHERE val_det = 'alpha';
SELECT '1_ctas_empty_tlist', count(*) FROM test_ctas_a;

-- ============================================================
-- 2. 显式目标列表（不含加密列）
-- ============================================================
CREATE TABLE test_ctas_b AS SELECT id, plain FROM test_ctas_src WHERE val_det = 'beta';
SELECT '2_ctas_explicit', count(*) FROM test_ctas_b;

-- ============================================================
-- 3. 星号 + OPE 条件
-- ============================================================
CREATE TABLE test_ctas_c AS SELECT * FROM test_ctas_src WHERE val_ope = 30;
SELECT '3_ctas_star_ope', count(*) FROM test_ctas_c;

-- ============================================================
-- 4. WHERE 不含密态列（对照）
-- ============================================================
CREATE TABLE test_ctas_d AS SELECT id, plain FROM test_ctas_src WHERE plain = 'p1';
SELECT '4_ctas_plain_where', count(*) FROM test_ctas_d;

-- ============================================================
-- 5. 带 ENCRYPTED WITH 目标列：原功能，服务端须派生列加密元数据
-- ============================================================
CREATE TABLE test_ctas_enc AS
    SELECT id, 'seedval'::text_enc ENCRYPTED WITH (DEK = dek_ctas, SUPPORT_ALGO = 'DET') AS c
    FROM test_ctas_src WHERE id = 1;
SELECT '5_ctas_encrypted_target', id, c FROM test_ctas_enc;
SELECT '6_ctas_meta_registered', column_name FROM hg_encrypted_columns_info
    WHERE table_oid = 'test_ctas_enc'::regclass ORDER BY column_name;

DROP TABLE IF EXISTS test_ctas_src CASCADE;
DROP TABLE IF EXISTS test_ctas_a CASCADE;
DROP TABLE IF EXISTS test_ctas_b CASCADE;
DROP TABLE IF EXISTS test_ctas_c CASCADE;
DROP TABLE IF EXISTS test_ctas_d CASCADE;
DROP TABLE IF EXISTS test_ctas_enc CASCADE;
DROP DEK IF EXISTS dek_ctas CASCADE;
DROP KEK IF EXISTS kek_ctas CASCADE;
