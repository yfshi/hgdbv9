-- ============================================================
-- swxa_enc.sql — SWXA（三未信安）硬加密全流程测试
-- ============================================================
-- 覆盖: CREATE KEK (swxakms), CREATE DEK, 9种加密类型,
--       INSERT, SELECT, DROP
-- ============================================================

-- 清理
DROP TABLE IF EXISTS swxa_enc_t CASCADE;
DROP DEK IF EXISTS swxa_dek CASCADE;
DROP KEK IF EXISTS swxa_kek CASCADE;

CREATE KEK swxa_kek WITH (
    key_store = 'swxakms',
    key_path = 'hsm:id=1'
);

CREATE DEK swxa_dek WITH VALUES(KEK = swxa_kek);

-- 建表（9种加密类型全覆盖，DET用于等值查询）
CREATE TABLE swxa_enc_t (
    id       SERIAL,
    c_int2   INT2_ENC    ENCRYPTED WITH (DEK = swxa_dek, SUPPORT_ALGO = 'DET'),
    c_int4   INT4_ENC    ENCRYPTED WITH (DEK = swxa_dek, SUPPORT_ALGO = 'DET'),
    c_int8   INT8_ENC    ENCRYPTED WITH (DEK = swxa_dek, SUPPORT_ALGO = 'DET'),
    c_float4 FLOAT4_ENC  ENCRYPTED WITH (DEK = swxa_dek, SUPPORT_ALGO = 'DET'),
    c_float8 FLOAT8_ENC  ENCRYPTED WITH (DEK = swxa_dek, SUPPORT_ALGO = 'DET'),
    c_numeric NUMERIC_ENC ENCRYPTED WITH (DEK = swxa_dek, SUPPORT_ALGO = 'DET'),
    c_text   TEXT_ENC    ENCRYPTED WITH (DEK = swxa_dek, SUPPORT_ALGO = 'DET'),
    c_varchar VARCHAR_ENC(100) ENCRYPTED WITH (DEK = swxa_dek, SUPPORT_ALGO = 'DET'),
    c_bpchar BPCHAR_ENC(50)   ENCRYPTED WITH (DEK = swxa_dek, SUPPORT_ALGO = 'DET'),
    tag      TEXT
);

-- INSERT 数据
INSERT INTO swxa_enc_t
    (c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric,
     c_text, c_varchar, c_bpchar, tag)
VALUES
    (1, 100, 10000, 1.5, 3.14159, 9999.99,
     'hello swxa', 'varchar_val', 'BPCHAR_VAL', 'row1');

INSERT INTO swxa_enc_t
    (c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric,
     c_text, c_varchar, c_bpchar, tag)
VALUES
    (2, 200, 20000, 2.5, 2.71828, 500.50,
     'world swxa', 'varchar_2', 'BPCHAR_2', 'row2');

-- 全表解密验证
SELECT '=== SWXA: 全表解密 ===' AS stage;

SELECT id, c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric,
       c_text, c_varchar, c_bpchar, tag
FROM swxa_enc_t ORDER BY id;

-- DET等值查询
SELECT '=== SWXA: DET等值查询 ===' AS stage;

SELECT tag FROM swxa_enc_t WHERE c_int4 = 100;
SELECT tag FROM swxa_enc_t WHERE c_text = 'hello swxa';

-- 清理
SELECT '=== SWXA: 清理 ===' AS stage;

DROP TABLE swxa_enc_t CASCADE;
DROP DEK swxa_dek CASCADE;
DROP KEK swxa_kek CASCADE;
