-- perf_ddl.sql — TECC性能测试DDL（优化版）
-- 3张多列代表表：enc_int4 (DET+OPE), enc_float8 (DET+OPE), enc_text (DET+OPE+SEARCH)
-- int2/4/8统一用int4代表（同算法加密开销一致），float4/8/numeric用float8代表，text/varchar/bpchar用text代表

-- ============ KEK / DEK ============
DROP KEK IF EXISTS kek_perf;
DROP DEK IF EXISTS dek_perf;

CREATE KEK kek_perf WITH (
    key_store = 'localkms',
    key_path = '/tmp/kek_perf.key'
);

CREATE DEK dek_perf WITH VALUES(
    KEK = kek_perf
);

-- ============ 整数代表表 (INT4 DET + OPE) ============
DROP TABLE IF EXISTS enc_int4 CASCADE;
CREATE TABLE enc_int4 (
    id SERIAL,
    val_det INT4_ENC ENCRYPTED WITH (DEK = 'dek_perf', SUPPORT_ALGO = 'det'),
    val_ope INT4_ENC ENCRYPTED WITH (DEK = 'dek_perf', SUPPORT_ALGO = 'ope'),
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

SELECT 'perf_setup: DDL complete (3 tables, 5 indexes)';
