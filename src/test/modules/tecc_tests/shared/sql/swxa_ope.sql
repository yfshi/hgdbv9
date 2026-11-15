-- ============================================================
-- swxa_ope.sql — SWXA硬加密 OPE 保序加密测试
-- ============================================================
-- 覆盖: 范围查询（> < >= <= BETWEEN）、ORDER BY、聚合
-- ============================================================

-- 清理
DROP TABLE IF EXISTS swxa_ope_t CASCADE;
DROP DEK IF EXISTS swxa_ope_dek CASCADE;
DROP KEK IF EXISTS swxa_ope_kek CASCADE;

CREATE KEK swxa_ope_kek WITH (
    key_store = 'swxakms',
    key_path = 'hsm:id=1'
);
CREATE DEK swxa_ope_dek WITH VALUES(KEK = swxa_ope_kek);

-- 建表（OPE 数字类型，支持范围查询与排序）
CREATE TABLE swxa_ope_t (
    id    SERIAL,
    c_i2  INT2_ENC    ENCRYPTED WITH (DEK = swxa_ope_dek, SUPPORT_ALGO = 'OPE'),
    c_i4  INT4_ENC    ENCRYPTED WITH (DEK = swxa_ope_dek, SUPPORT_ALGO = 'OPE'),
    c_i8  INT8_ENC    ENCRYPTED WITH (DEK = swxa_ope_dek, SUPPORT_ALGO = 'OPE'),
    c_f4  FLOAT4_ENC  ENCRYPTED WITH (DEK = swxa_ope_dek, SUPPORT_ALGO = 'OPE'),
    c_f8  FLOAT8_ENC  ENCRYPTED WITH (DEK = swxa_ope_dek, SUPPORT_ALGO = 'OPE'),
    c_num NUMERIC_ENC ENCRYPTED WITH (DEK = swxa_ope_dek, SUPPORT_ALGO = 'OPE'),
    tag   TEXT
);

INSERT INTO swxa_ope_t (c_i2, c_i4, c_i8, c_f4, c_f8, c_num, tag) VALUES
    (10, 100, 1000, 10.5, 100.5, 1000.5, 'r1'),
    (20, 200, 2000, 20.5, 200.5, 2000.5, 'r2'),
    (30, 300, 3000, 30.5, 300.5, 3000.5, 'r3'),
    (-5, -50, -500, -5.5, -50.5, -500.5, 'r4');

-- 全表解密
SELECT '=== SWXA OPE: 全表解密 ===' AS stage;
SELECT id, c_i2, c_i4, c_i8, c_f4, c_f8, c_num, tag FROM swxa_ope_t ORDER BY id;

-- 范围查询
SELECT '=== SWXA OPE: 范围查询 ===' AS stage;
SELECT tag FROM swxa_ope_t WHERE c_i4 > 100 ORDER BY id;
SELECT tag FROM swxa_ope_t WHERE c_i4 >= 200 ORDER BY id;
SELECT tag FROM swxa_ope_t WHERE c_i4 < 200 ORDER BY id;
SELECT tag FROM swxa_ope_t WHERE c_i4 <= 100 ORDER BY id;
SELECT tag FROM swxa_ope_t WHERE c_i4 BETWEEN 150 AND 250 ORDER BY id;
SELECT tag FROM swxa_ope_t WHERE c_num > 1000 ORDER BY id;
SELECT tag FROM swxa_ope_t WHERE c_f8 < 0 ORDER BY id;

-- ORDER BY（保序）
SELECT '=== SWXA OPE: ORDER BY ===' AS stage;
SELECT id, c_i4 FROM swxa_ope_t ORDER BY c_i4;
SELECT id, c_i4 FROM swxa_ope_t ORDER BY c_i4 DESC;

-- 聚合（普通加密列不支持 SUM/MIN/MAX，仅 COUNT 可用）
SELECT '=== SWXA OPE: 聚合 ===' AS stage;
SELECT count(*) AS cnt FROM swxa_ope_t;

-- 清理
SELECT '=== SWXA OPE: 清理 ===' AS stage;
DROP TABLE swxa_ope_t CASCADE;
DROP DEK swxa_ope_dek CASCADE;
DROP KEK swxa_ope_kek CASCADE;
