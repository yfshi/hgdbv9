-- ============================================================
-- he_new_column_paths_execute.sql — PREPARE + CREATE TABLE AS EXECUTE 路径的同态列支持
--
-- 与 he_new_column_paths.sql 的关系：后者覆盖 CTAS / 视图·物化视图定义 / 加密列 DEFAULT
-- 三条「新列」路径；本文件覆盖第四条——PREPARE 语句里的 ENCRYPTED WITH cast 经
-- CREATE TABLE AS EXECUTE 落成新列（PG 允许，且前端两条处理链——EXECUTE 分发与 CTAS 分发
-- ——都会消费该 cast）。
--
-- 判定方式：与主用例一致，一律用【两行同态求和】作为判据——单行 sum 是恒等返回，
-- 缺 HE 段的密文也会算出正确数值，不能作为"HE 段存在"的证据。
-- ============================================================

\! rm -f "$TECC_KEK_PATH/hencp_asym.priv" "$TECC_KEK_PATH/hencp_asym.pub" "$TECC_KEK_PATH/hencp_sym.key" 2>/dev/null
\! tecc_ktool generate --key-name hencp_asym --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1
\! tecc_ktool generate --key-name hencp_sym --key-algo sm4 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1

DROP TABLE IF EXISTS hencp_pe_mixed CASCADE;
DROP TABLE IF EXISTS hencp_pe_mixed_pure CASCADE;
DROP DEK IF EXISTS hencp_dek_add CASCADE;
DROP DEK IF EXISTS hencp_dek_sym CASCADE;
DROP KEK IF EXISTS hencp_kek_asym CASCADE;
DROP KEK IF EXISTS hencp_kek_sym CASCADE;

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
-- 1. 混合 det,he-add：PREPARE → CREATE TABLE AS EXECUTE → 追加第二行 → 两行同态求和
--    （期望 2|12；若 CTAS 写入的密文缺 HE 段，追加第二行后求和会报
--     "cannot perform homomorphic addition: operand lacks EG_ADD ciphertext"）
-- ============================================================
SELECT '=== 1 PREPARE + CTAS AS EXECUTE（混合 det,he-add）===' AS stage;
PREPARE hencp_pe_1 AS
    SELECT 5::int4_enc ENCRYPTED WITH (DEK = 'hencp_dek_sym', HE_ADD_DEK = 'hencp_dek_add',
                                       SUPPORT_ALGO = 'det,he-add') AS a;
CREATE TABLE hencp_pe_mixed AS EXECUTE hencp_pe_1;
INSERT INTO hencp_pe_mixed
    SELECT 7::int4_enc ENCRYPTED WITH (DEK = 'hencp_dek_sym', HE_ADD_DEK = 'hencp_dek_add',
                                       SUPPORT_ALGO = 'det,he-add') AS a;
SELECT count(*) AS n, (SELECT sum(a) FROM hencp_pe_mixed) AS s FROM hencp_pe_mixed;

-- ============================================================
-- 2. 纯同态（无 DEK=，只有 HE_ADD_DEK）：同一路径
--    （纯 HE 列不返回明文，故只断言聚合结果）
-- ============================================================
SELECT '=== 2 PREPARE + CTAS AS EXECUTE（纯同态 he-add）===' AS stage;
PREPARE hencp_pe_2 AS
    SELECT 5::int4_enc ENCRYPTED WITH (HE_ADD_DEK = 'hencp_dek_add', SUPPORT_ALGO = 'he-add') AS a;
CREATE TABLE hencp_pe_mixed_pure AS EXECUTE hencp_pe_2;
INSERT INTO hencp_pe_mixed_pure
    SELECT 7::int4_enc ENCRYPTED WITH (HE_ADD_DEK = 'hencp_dek_add', SUPPORT_ALGO = 'he-add') AS a;
SELECT count(*) AS n, (SELECT sum(a) FROM hencp_pe_mixed_pure) AS s FROM hencp_pe_mixed_pure;

-- 清理
DEALLOCATE hencp_pe_1;
DEALLOCATE hencp_pe_2;
DROP TABLE IF EXISTS hencp_pe_mixed CASCADE;
DROP TABLE IF EXISTS hencp_pe_mixed_pure CASCADE;
DROP DEK IF EXISTS hencp_dek_add CASCADE;
DROP DEK IF EXISTS hencp_dek_sym CASCADE;
DROP KEK IF EXISTS hencp_kek_asym CASCADE;
DROP KEK IF EXISTS hencp_kek_sym CASCADE;
