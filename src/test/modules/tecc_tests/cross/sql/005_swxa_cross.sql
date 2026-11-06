-- ============================================================
-- 005_swxa_cross.sql — SWXA硬加密跨驱动交叉验证
-- ============================================================
-- 交替执行: libpq→JDBC→libpq→JDBC
-- ============================================================

-- 清理
DROP TABLE IF EXISTS swxa_cross_t CASCADE;
DROP DEK IF EXISTS swxa_cross_dek CASCADE;
DROP KEK IF EXISTS swxa_cross_kek CASCADE;

CREATE KEK swxa_cross_kek WITH (
    key_store = 'swxakms',
    key_path = 'hsm:id=1'
);

CREATE DEK swxa_cross_dek WITH VALUES(KEK = swxa_cross_kek);

-- 建表（9种加密类型全覆盖）
CREATE TABLE swxa_cross_t (
    id       SERIAL,
    c_int2   INT2_ENC    ENCRYPTED WITH (DEK = swxa_cross_dek, SUPPORT_ALGO = 'DET'),
    c_int4   INT4_ENC    ENCRYPTED WITH (DEK = swxa_cross_dek, SUPPORT_ALGO = 'DET'),
    c_int8   INT8_ENC    ENCRYPTED WITH (DEK = swxa_cross_dek, SUPPORT_ALGO = 'DET'),
    c_float4 FLOAT4_ENC  ENCRYPTED WITH (DEK = swxa_cross_dek, SUPPORT_ALGO = 'DET'),
    c_float8 FLOAT8_ENC  ENCRYPTED WITH (DEK = swxa_cross_dek, SUPPORT_ALGO = 'DET'),
    c_numeric NUMERIC_ENC ENCRYPTED WITH (DEK = swxa_cross_dek, SUPPORT_ALGO = 'DET'),
    c_text   TEXT_ENC    ENCRYPTED WITH (DEK = swxa_cross_dek, SUPPORT_ALGO = 'DET'),
    c_varchar VARCHAR_ENC(100) ENCRYPTED WITH (DEK = swxa_cross_dek, SUPPORT_ALGO = 'DET'),
    c_bpchar BPCHAR_ENC(50)   ENCRYPTED WITH (DEK = swxa_cross_dek, SUPPORT_ALGO = 'DET'),
    tag      TEXT
);

-- INSERT 数据
INSERT INTO swxa_cross_t
    (c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric,
     c_text, c_varchar, c_bpchar, tag)
VALUES
    (10, 1000, 100000, 10.5, 100.5, 1000.55,
     'swxa cross text', 'cross_varchar', 'CROSS_BP', 'cross_r1');

INSERT INTO swxa_cross_t
    (c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric,
     c_text, c_varchar, c_bpchar, tag)
VALUES
    (20, 2000, 200000, 20.5, 200.5, 2000.55,
     'swxa cross 2nd', 'cross_vc_2', 'CROSS_2', 'cross_r2');

-- 全表解密验证
SELECT '=== SWXA CROSS: 全表解密 ===' AS stage;

SELECT id, c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric,
       c_text, c_varchar, c_bpchar, tag
FROM swxa_cross_t ORDER BY id;

-- DET等值查询
SELECT '=== SWXA CROSS: DET查询 ===' AS stage;

SELECT tag FROM swxa_cross_t WHERE c_int4 = 1000;
SELECT tag FROM swxa_cross_t WHERE c_text = 'swxa cross 2nd';

-- 清理
DROP TABLE swxa_cross_t CASCADE;
DROP DEK swxa_cross_dek CASCADE;
DROP KEK swxa_cross_kek CASCADE;
