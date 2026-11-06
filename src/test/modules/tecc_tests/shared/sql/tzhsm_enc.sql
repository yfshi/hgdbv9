-- ============================================================
-- tzhsm_enc.sql — 同智伟业硬加密全流程测试
-- ============================================================
-- 覆盖: CREATE KEK (hsmkms), CREATE DEK, 9种加密类型,
--       INSERT, SELECT, DROP
-- 依赖: HSM加密机可达 + /usr/lib64/cfg/client_config.xml
-- ============================================================

-- 清理
DROP TABLE IF EXISTS tzhsm_enc_t CASCADE;
DROP DEK IF EXISTS tzhsm_dek CASCADE;
DROP KEK IF EXISTS tzhsm_kek CASCADE;

SELECT '=== TZHSM Setup ===' AS stage;
CREATE KEK tzhsm_kek WITH (
    key_store = 'hsmkms',
    key_path = 'hsm:id=1'
);

CREATE DEK tzhsm_dek WITH VALUES(KEK = tzhsm_kek);

-- 建表（9种加密类型全覆盖）
CREATE TABLE tzhsm_enc_t (
    id       SERIAL,
    c_int2   INT2_ENC    ENCRYPTED WITH (DEK = tzhsm_dek, SUPPORT_ALGO = 'DET'),
    c_int4   INT4_ENC    ENCRYPTED WITH (DEK = tzhsm_dek, SUPPORT_ALGO = 'DET'),
    c_int8   INT8_ENC    ENCRYPTED WITH (DEK = tzhsm_dek, SUPPORT_ALGO = 'DET'),
    c_float4 FLOAT4_ENC  ENCRYPTED WITH (DEK = tzhsm_dek, SUPPORT_ALGO = 'DET'),
    c_float8 FLOAT8_ENC  ENCRYPTED WITH (DEK = tzhsm_dek, SUPPORT_ALGO = 'DET'),
    c_numeric NUMERIC_ENC ENCRYPTED WITH (DEK = tzhsm_dek, SUPPORT_ALGO = 'DET'),
    c_text   TEXT_ENC    ENCRYPTED WITH (DEK = tzhsm_dek, SUPPORT_ALGO = 'DET'),
    c_varchar VARCHAR_ENC(100) ENCRYPTED WITH (DEK = tzhsm_dek, SUPPORT_ALGO = 'DET'),
    c_bpchar BPCHAR_ENC(50)   ENCRYPTED WITH (DEK = tzhsm_dek, SUPPORT_ALGO = 'DET'),
    tag      TEXT
);

-- INSERT 数据
INSERT INTO tzhsm_enc_t
    (c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric,
     c_text, c_varchar, c_bpchar, tag)
VALUES
    (1, 100, 10000, 1.5, 3.14159, 9999.99,
     'hello tzhsm', 'varchar_val', 'BPCHAR_VAL', 'row1');

INSERT INTO tzhsm_enc_t
    (c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric,
     c_text, c_varchar, c_bpchar, tag)
VALUES
    (2, 200, 20000, 2.5, 2.71828, 500.50,
     'world tzhsm', 'varchar_2', 'BPCHAR_2', 'row2');

-- 全表解密验证
SELECT '=== TZHSM: 全表解密 ===' AS stage;

SELECT id, c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric,
       c_text, c_varchar, c_bpchar, tag
FROM tzhsm_enc_t ORDER BY id;

-- DET等值查询
SELECT '=== TZHSM: DET等值查询 ===' AS stage;

SELECT tag FROM tzhsm_enc_t WHERE c_int4 = 100;
SELECT tag FROM tzhsm_enc_t WHERE c_text = 'hello tzhsm';

-- 清理
SELECT '=== TZHSM: 清理 ===' AS stage;

DROP TABLE tzhsm_enc_t CASCADE;
DROP DEK tzhsm_dek CASCADE;
DROP KEK tzhsm_kek CASCADE;
