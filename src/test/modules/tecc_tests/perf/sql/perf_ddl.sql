-- perf_ddl.sql — TECC性能测试DDL（优化版）
-- 3张多列代表表：enc_int4 (DET+OPE+HE), enc_float8 (DET+OPE), enc_text (DET+OPE+SEARCH)
-- int2/4/8统一用int8代表（同算法加密开销一致），float4/8/numeric用float8代表，text/varchar/bpchar用text代表
-- 同态：整型代表表两列分别开启 he-add（加法/SUM）与 he-mul（乘法），
--       使全量/等值/范围查询在返回整型列时同时背负对称段+同态段解密开销（需求 4.7.1）

-- ============ KEK / DEK ============
DROP DEK IF EXISTS dek_perf CASCADE;
DROP KEK IF EXISTS kek_perf;
DROP DEK IF EXISTS dek_perf_he_add CASCADE;
DROP DEK IF EXISTS dek_perf_he_mul CASCADE;
DROP KEK IF EXISTS kek_perf_he CASCADE;

-- 对称 KEK/DEK（供 DET/OPE 对称段使用）
-- 由 004/005 测试脚本在执行 DDL 前用 tecc_ktool 预生成密钥文件，
-- key_path 用 %KEKPATH% 占位符（脚本替换为 tmp_check/keys 绝对路径），
-- 与下方非对称 KEK 一致；写死 /tmp 会在密钥文件不存在时让 CREATE KEK 失败，
-- 进而连累后续所有对称密态表建表失败。
CREATE KEK kek_perf WITH (
    key_store = 'localkms',
    key_path = '%KEKPATH%/kek_perf.key'
);

CREATE DEK dek_perf WITH VALUES(
    KEK = kek_perf
);

-- 非对称 KEK（同态 DEK 必须绑定非对称 KEK，key_algo='sm2'）
-- 由 004/005 测试脚本在执行 DDL 前用 tecc_ktool 预生成密钥对，
-- key_path 用 %KEKPATH% 占位符（脚本替换为 tmp_check/keys 绝对路径）
CREATE KEK kek_perf_he WITH (
    key_store = 'localkms',
    key_algo = 'sm2',
    key_path = '%KEKPATH%/kek_perf_he.priv'
);

-- 加法同态 DEK（EC-ElGamal，支持 + - 与 SUM 聚合）
CREATE DEK dek_perf_he_add WITH VALUES(
    KEK = kek_perf_he,
    KEY_ALGO = 'he-add'
);

-- 乘法同态 DEK（有限域 ElGamal，支持 *）
CREATE DEK dek_perf_he_mul WITH VALUES(
    KEK = kek_perf_he,
    KEY_ALGO = 'he-mul'
);

-- ============ 整数代表表 (INT4 DET / OPE，不含同态) ============
-- 同态 add/mul 独立到 he_int8 表，避免同态段干扰全量/等值/范围等
-- 对称加密场景的扫描与查询性能（需求 4.7.1）
DROP TABLE IF EXISTS enc_int4 CASCADE;
CREATE TABLE enc_int4 (
    id SERIAL,
    val_det INT4_ENC ENCRYPTED WITH (DEK = 'dek_perf', SUPPORT_ALGO = 'det'),
    val_ope INT4_ENC ENCRYPTED WITH (DEK = 'dek_perf', SUPPORT_ALGO = 'ope'),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 优化器 join/semijoin/memoize 性能基线对照表。
-- 与 enc_int4 使用相同 DEK，保证 DET/OPE 等值比较在语义上有效。
DROP TABLE IF EXISTS enc_int4_b CASCADE;
CREATE TABLE enc_int4_b (
    id SERIAL,
    val_det INT4_ENC ENCRYPTED WITH (DEK = 'dek_perf', SUPPORT_ALGO = 'det'),
    val_ope INT4_ENC ENCRYPTED WITH (DEK = 'dek_perf', SUPPORT_ALGO = 'ope'),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- ============ 同态专用表 (INT8 HE-ADD / HE-MUL) ============
-- 专测同态 add（+/-/SUM）与 mul（*/^）：两列独立开启 he-add / he-mul
DROP TABLE IF EXISTS he_int8 CASCADE;
CREATE TABLE he_int8 (
    id SERIAL,
    val_add INT8_ENC ENCRYPTED WITH (HE_ADD_DEK = 'dek_perf_he_add', SUPPORT_ALGO = 'he-add'),
    val_mul INT8_ENC ENCRYPTED WITH (HE_MUL_DEK = 'dek_perf_he_mul', SUPPORT_ALGO = 'he-mul'),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- ============ 浮点代表表 (FLOAT8 DET + OPE) ============
DROP TABLE IF EXISTS enc_float8 CASCADE;
CREATE TABLE enc_float8 (
    id SERIAL,
    val_det FLOAT8_ENC ENCRYPTED WITH (DEK = 'dek_perf', SUPPORT_ALGO = 'det'),
    val_ope FLOAT8_ENC ENCRYPTED WITH (DEK = 'dek_perf', SUPPORT_ALGO = 'ope'),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- ============ 文本代表表 (TEXT DET + OPE + SEARCH) ============
DROP TABLE IF EXISTS enc_text CASCADE;
CREATE TABLE enc_text (
    id SERIAL,
    val_det TEXT_ENC ENCRYPTED WITH (DEK = 'dek_perf', SUPPORT_ALGO = 'det'),
    val_ope TEXT_ENC ENCRYPTED WITH (DEK = 'dek_perf', SUPPORT_ALGO = 'ope'),
    val_search TEXT_ENC ENCRYPTED WITH (DEK = 'dek_perf', SUPPORT_ALGO = 'search'),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- ============ 索引（DET: hash, OPE: btree, SEARCH: 不支持）============
CREATE INDEX idx_enc_int4_det ON enc_int4 USING hash (val_det);
CREATE INDEX idx_enc_int4_ope ON enc_int4 USING btree (val_ope);
CREATE INDEX idx_enc_int4_b_det ON enc_int4_b USING hash (val_det);
CREATE INDEX idx_enc_int4_b_ope ON enc_int4_b USING btree (val_ope);
CREATE INDEX idx_enc_float8_det ON enc_float8 USING hash (val_det);
CREATE INDEX idx_enc_float8_ope ON enc_float8 USING btree (val_ope);
CREATE INDEX idx_enc_text_det ON enc_text USING hash (val_det);
CREATE INDEX idx_enc_text_ope ON enc_text USING btree (val_ope);
-- SEARCH 无索引（LIKE模糊查询走全表扫描）

-- ============ Stage 表（种子池，倍增从 stage 复制到 target）============
DROP TABLE IF EXISTS stage_int4 CASCADE;
CREATE TABLE stage_int4 (
    id SERIAL,
    val_det INT4_ENC ENCRYPTED WITH (DEK = 'dek_perf', SUPPORT_ALGO = 'det'),
    val_ope INT4_ENC ENCRYPTED WITH (DEK = 'dek_perf', SUPPORT_ALGO = 'ope')
);

DROP TABLE IF EXISTS stage_he_int8 CASCADE;
CREATE TABLE stage_he_int8 (
    id SERIAL,
    val_add INT8_ENC ENCRYPTED WITH (HE_ADD_DEK = 'dek_perf_he_add', SUPPORT_ALGO = 'he-add'),
    val_mul INT8_ENC ENCRYPTED WITH (HE_MUL_DEK = 'dek_perf_he_mul', SUPPORT_ALGO = 'he-mul')
);

DROP TABLE IF EXISTS stage_float8 CASCADE;
CREATE TABLE stage_float8 (
    id SERIAL,
    val_det FLOAT8_ENC ENCRYPTED WITH (DEK = 'dek_perf', SUPPORT_ALGO = 'det'),
    val_ope FLOAT8_ENC ENCRYPTED WITH (DEK = 'dek_perf', SUPPORT_ALGO = 'ope')
);

DROP TABLE IF EXISTS stage_text CASCADE;
CREATE TABLE stage_text (
    id SERIAL,
    val_det TEXT_ENC ENCRYPTED WITH (DEK = 'dek_perf', SUPPORT_ALGO = 'det'),
    val_ope TEXT_ENC ENCRYPTED WITH (DEK = 'dek_perf', SUPPORT_ALGO = 'ope'),
    val_search TEXT_ENC ENCRYPTED WITH (DEK = 'dek_perf', SUPPORT_ALGO = 'search')
);

SELECT 'perf_setup: DDL完成（已包含优化器对照表）';
