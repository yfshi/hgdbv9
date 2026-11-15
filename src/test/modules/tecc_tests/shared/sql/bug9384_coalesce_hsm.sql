-- ============================================================
-- bug9384_coalesce_hsm.sql — 硬加密下 coalesce 加密常量结果解密失败
-- ============================================================
-- 报单：TC_IVYSQL_TECC4_SAFETY_TFUNC_COALESCE_002
--   硬加密（swxakms）下 coalesce(加密常量, NULL) 报
--     TECC result processing error: Batch decrypt: item 0 failed
--   同场景软加密（localkms）正常返回 10。
--
-- 本用例同时锚定根因范围：coalesce 只是触发形态之一，凡是"结果列背后
-- 没有真实加密列"（纯加密常量表达式）的句子都走同一条元数据缺失路径，
-- 故第 2 节用裸加密常量做对照。
--
-- 放 shared/：只用普通 SQL（\\echo 属 runner 静默丢弃的元命令），libpq 与 JDBC
-- 两侧都执行。文件已登记进 t/tecc_test_utils.pm 的 %HSM_CASE 名单。
-- ============================================================

DROP TABLE IF EXISTS bug9384_t CASCADE;
DROP DEK IF EXISTS dek_test9384 CASCADE;
DROP DEK IF EXISTS dek_test9384_soft CASCADE;
DROP KEK IF EXISTS kek_test9384 CASCADE;
DROP KEK IF EXISTS kek_test9384_soft CASCADE;

-- 硬加密（报单形态：swxakms + hsm:id=2）
CREATE KEK kek_test9384 WITH (
    KEY_STORE = swxakms,
    KEY_ALGO  = sm4,
    KEY_PATH  = 'hsm:id=2'
);
CREATE DEK dek_test9384 WITH VALUES(KEK = kek_test9384, KEY_ALGO = 'sm4');

-- 软加密对照
CREATE KEK kek_test9384_soft WITH (
    KEY_STORE = localkms,
    KEY_PATH  = '/tmp/bug9384_soft.key'
);
CREATE DEK dek_test9384_soft WITH VALUES(KEK = kek_test9384_soft);

-- ### 1. 报单场景：硬加密 coalesce 加密常量 ###################################
\echo '========== 1. 硬加密 coalesce 加密常量（期望 10|10|10） =========='
SELECT coalesce(10::int2_enc encrypted with (dek = dek_test9384), NULL),
       coalesce(10::int4_enc encrypted with (dek = dek_test9384), NULL),
       coalesce(NULL, 10::int8_enc encrypted with (dek = dek_test9384));

-- ### 2. 根因范围对照：硬加密裸加密常量（无 coalesce） ########################
\echo '========== 2. 硬加密 裸加密常量（期望 10） =========='
SELECT 10::int2_enc encrypted with (dek = dek_test9384);

-- ### 3. 软加密对照：同一条 coalesce ##########################################
\echo '========== 3. 软加密 coalesce 加密常量（期望 10|10|10） =========='
SELECT coalesce(10::int2_enc encrypted with (dek = dek_test9384_soft), NULL),
       coalesce(10::int4_enc encrypted with (dek = dek_test9384_soft), NULL),
       coalesce(NULL, 10::int8_enc encrypted with (dek = dek_test9384_soft));

-- ### 4. 对照：硬加密 coalesce 真实加密列（有列元数据，期望正常） ############
\echo '========== 4. 硬加密 coalesce 加密列（期望 10） =========='
CREATE TABLE bug9384_t (
    c_int2 int2_enc ENCRYPTED WITH (DEK = dek_test9384),
    c_int4 int4_enc ENCRYPTED WITH (DEK = dek_test9384)
);
INSERT INTO bug9384_t VALUES (10, 20);
SELECT coalesce(c_int2, NULL), coalesce(NULL, c_int4) FROM bug9384_t;

-- ### 5. 清理 ################################################################
\echo '========== 5. cleanup =========='
DROP TABLE bug9384_t;
DROP DEK dek_test9384;
DROP DEK dek_test9384_soft;
DROP KEK kek_test9384 CASCADE;
DROP KEK kek_test9384_soft CASCADE;
