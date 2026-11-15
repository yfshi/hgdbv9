-- ============================================================
-- swxa_enc.sql — SWXA（三未信安）硬加密全流程测试
-- ============================================================
-- 覆盖: CREATE KEK (swxakms), CREATE DEK, RND随机加密,
--       DET确定性加密（9种类型）, INSERT/SELECT/UPDATE/DELETE,
--       NULL处理, 边界值
-- ============================================================

-- 清理
DROP TABLE IF EXISTS swxa_enc_t CASCADE;
DROP TABLE IF EXISTS swxa_rnd_t CASCADE;
DROP TABLE IF EXISTS swxa_edge_t CASCADE;
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

-- RND 随机加密（默认算法，不指定 SUPPORT_ALGO）
SELECT '=== SWXA: RND随机加密 ===' AS stage;

CREATE TABLE swxa_rnd_t (
    id    SERIAL,
    c_int INT4_ENC ENCRYPTED WITH (DEK = swxa_dek),
    c_txt TEXT_ENC ENCRYPTED WITH (DEK = swxa_dek),
    tag   TEXT
);

-- 相同明文插入两次（RND 密文不同，但解密结果一致）
INSERT INTO swxa_rnd_t (c_int, c_txt, tag) VALUES (123, 'same value', 'r1');
INSERT INTO swxa_rnd_t (c_int, c_txt, tag) VALUES (123, 'same value', 'r2');
INSERT INTO swxa_rnd_t (c_int, c_txt, tag) VALUES (456, 'other', 'r3');

SELECT id, c_int, c_txt, tag FROM swxa_rnd_t ORDER BY id;

-- UPDATE / DELETE（重新加密）
SELECT '=== SWXA: UPDATE/DELETE ===' AS stage;

UPDATE swxa_enc_t SET c_int4 = 150 WHERE tag = 'row1';
SELECT tag, c_int4 FROM swxa_enc_t WHERE tag = 'row1';

DELETE FROM swxa_enc_t WHERE c_int4 = 200;
SELECT count(*) AS remaining FROM swxa_enc_t;

-- NULL 处理
SELECT '=== SWXA: NULL处理 ===' AS stage;

INSERT INTO swxa_enc_t (c_int2, c_int4, c_text, tag) VALUES (NULL, NULL, NULL, 'null_row');
SELECT tag, c_int4, c_text FROM swxa_enc_t WHERE tag = 'null_row';

-- 边界值（int 最大最小值、空字符串、Unicode 中文）
SELECT '=== SWXA: 边界值 ===' AS stage;

CREATE TABLE swxa_edge_t (
    id    SERIAL,
    c_i4  INT4_ENC ENCRYPTED WITH (DEK = swxa_dek, SUPPORT_ALGO = 'DET'),
    c_txt TEXT_ENC ENCRYPTED WITH (DEK = swxa_dek, SUPPORT_ALGO = 'DET'),
    tag   TEXT
);

INSERT INTO swxa_edge_t (c_i4, c_txt, tag) VALUES
    (2147483647, '', 'max_int_empty'),
    (-2147483648, '中文unicode测试', 'min_int_unicode'),
    (0, 'a b c', 'zero');

SELECT id, c_i4, c_txt, tag FROM swxa_edge_t ORDER BY id;
SELECT tag FROM swxa_edge_t WHERE c_i4 = 2147483647;

-- 清理
SELECT '=== SWXA: 清理 ===' AS stage;

DROP TABLE swxa_enc_t CASCADE;
DROP TABLE swxa_rnd_t CASCADE;
DROP TABLE swxa_edge_t CASCADE;
DROP DEK swxa_dek CASCADE;
DROP KEK swxa_kek CASCADE;

-- ============================================================
-- SWXA × HE 同态加密（SM2 非对称 KEK 托管在密码机）— Bug 9267
-- ============================================================
-- 前置：密码机 hsm:id=3 处预置**无访问口令**的 SM2 密钥对
--       （tecc_kms/keys/sm2*-3.pem，由 teccsdfinit -kek 3 -key 3 生成），
--       等价真机密码机内不带访问口令的设备密钥。
-- 判定点：设备内 SM2 私钥必须先经 SDF_GetPrivateKeyAccessRight 授权，才能
--         用于解开 HE 私钥信封；未授权时 HE 列查询直接返回密文、常量与
--         同态列相乘报 "HE result out of range for column"。

DROP TABLE IF EXISTS swxa_he_t CASCADE;
DROP DEK IF EXISTS swxa_he_sym CASCADE;
DROP DEK IF EXISTS swxa_he_add CASCADE;
DROP KEK IF EXISTS swxa_he_kek CASCADE;

CREATE KEK swxa_he_kek WITH (
    key_store = 'swxakms',
    key_algo  = 'sm2',
    key_path  = 'hsm:id=3'
);

CREATE DEK swxa_he_add WITH VALUES(KEK = swxa_he_kek, KEY_ALGO = 'he-add');
CREATE DEK swxa_he_sym WITH VALUES(KEK = swxa_he_kek, KEY_ALGO = 'sm4');

CREATE TABLE swxa_he_t (
    id SERIAL,
    a  INT4_ENC ENCRYPTED WITH (DEK = swxa_he_sym, HE_ADD_DEK = swxa_he_add,
                                SUPPORT_ALGO = 'det, he-add'),
    c  INT4_ENC ENCRYPTED WITH (DEK = swxa_he_sym, HE_ADD_DEK = swxa_he_add,
                                SUPPORT_ALGO = 'he-add')
);

SELECT '=== SWXA-HE: 写入与读取 ===' AS stage;

INSERT INTO swxa_he_t (a) VALUES (10), (20), (30);
SELECT id, a, c FROM swxa_he_t ORDER BY id;

SELECT '=== SWXA-HE: 同态更新 c = a + a ===' AS stage;

UPDATE swxa_he_t SET c = a + a;
SELECT id, a, c FROM swxa_he_t ORDER BY id;

SELECT '=== SWXA-HE: 常量与同态列标量运算 ===' AS stage;

SELECT id, 2 * a AS double_a FROM swxa_he_t ORDER BY id;
SELECT id, a + c AS a_plus_c FROM swxa_he_t ORDER BY id;

SELECT '=== SWXA-HE: 同态聚合 ===' AS stage;

SELECT sum(c) AS sum_c FROM swxa_he_t;

-- Bug 9359: 硬加密 + he-add 列建视图后查询 sum——视图聚合结果列无 col_meta，
-- 需从密文结构识别 HE-only 并走设备内 SM2 私钥解密（与直接聚合同一条授权路径）。
SELECT '=== SWXA-HE: 视图聚合 sum（Bug 9359）===' AS stage;

DROP VIEW IF EXISTS v_swxa_he_sum CASCADE;
CREATE VIEW v_swxa_he_sum AS SELECT sum(c) AS s FROM swxa_he_t;
SELECT * FROM v_swxa_he_sum;
DROP VIEW v_swxa_he_sum CASCADE;

-- ============================================================
-- SWXA × HE 乘法同态（he-mul / FF-ElGamal，KEK 同为密码机托管的 SM2）
-- ============================================================
-- 判定点同 he-add 段：纯 he-mul 列的数据加解密同样依赖设备内 SM2 私钥解开
-- 同态私钥信封（he_eg_privkey），未授权时读取返回密文、同态运算报结果越界。

DROP TABLE IF EXISTS swxa_he_mul_t CASCADE;
DROP DEK IF EXISTS swxa_he_mul CASCADE;

CREATE DEK swxa_he_mul WITH VALUES(KEK = swxa_he_kek, KEY_ALGO = 'he-mul');

CREATE TABLE swxa_he_mul_t (
    id SERIAL,
    m  INT4_ENC ENCRYPTED WITH (HE_MUL_DEK = swxa_he_mul, SUPPORT_ALGO = 'he-mul')
);

SELECT '=== SWXA-HE-MUL: 写入与读取 ===' AS stage;

INSERT INTO swxa_he_mul_t (m) VALUES (2), (3), (5);
SELECT id, m FROM swxa_he_mul_t ORDER BY id;

SELECT '=== SWXA-HE-MUL: 常量标量乘 ===' AS stage;

SELECT id, m * 3 AS m_times_3 FROM swxa_he_mul_t ORDER BY id;
SELECT id, 2 * m AS two_times_m FROM swxa_he_mul_t ORDER BY id;

SELECT '=== SWXA-HE-MUL: 幂运算 ===' AS stage;

SELECT id, m ^ 2 AS m_squared FROM swxa_he_mul_t ORDER BY id;

SELECT '=== SWXA-HE-MUL: 同态更新 m = m * 2 ===' AS stage;

UPDATE swxa_he_mul_t SET m = m * 2;
SELECT id, m FROM swxa_he_mul_t ORDER BY id;

-- ============================================================
-- SWXA × HE 乘法同态 · DET + he-mul 混合列（Bug 9334）
-- ============================================================
-- 判定点：同一列同时配 DET 与 he-mul（det 段供普通读取与等值查询、he-mul 段承载
-- 乘法同态结果）时，结果解密走的是与纯同态列不同的一条兜底分支
-- （tecc_crypto_scheduler.c 里两个 fallback：纯同态列带反斜杠-x 前缀回传密文，
-- 通用分支原先不带前缀，于是被结果处理阶段当成明文做整数校验，报出与事实不符的
-- HE result out of range for column）。设备内 SM2 私钥不可用时此处应以密文原样
-- 回传、并在标准错误上给出明确原因，不得伪造范围错误。

DROP TABLE IF EXISTS swxa_he_mul_det_t CASCADE;

CREATE TABLE swxa_he_mul_det_t (
    id SERIAL,
    a  INT4_ENC ENCRYPTED WITH (DEK = swxa_he_sym, HE_MUL_DEK = swxa_he_mul,
                                SUPPORT_ALGO = 'det, he-mul'),
    b  INT4_ENC ENCRYPTED WITH (DEK = swxa_he_sym, HE_MUL_DEK = swxa_he_mul,
                                SUPPORT_ALGO = 'det, he-mul')
);

INSERT INTO swxa_he_mul_det_t (a, b) VALUES (3, 2), (4, 5), (7, 0);

SELECT '=== SWXA-HE-MUL-DET: 明文读取（DET 段）===' AS stage;

SELECT id, a, b FROM swxa_he_mul_det_t ORDER BY id;

SELECT '=== SWXA-HE-MUL-DET: 列 × 列 ===' AS stage;

SELECT id, a * b AS a_times_b, a * a AS a_squared, b * b AS b_squared
FROM swxa_he_mul_det_t ORDER BY id;

SELECT '=== SWXA-HE-MUL-DET: 常量 × 列（常量在两侧）===' AS stage;

SELECT id, a * 3 AS a_times_3, 2 * b AS two_times_b
FROM swxa_he_mul_det_t ORDER BY id;

SELECT '=== SWXA-HE: 清理 ===' AS stage;

DROP TABLE swxa_he_mul_det_t CASCADE;
DROP TABLE swxa_he_mul_t CASCADE;
DROP TABLE swxa_he_t CASCADE;
DROP DEK swxa_he_mul CASCADE;
DROP DEK swxa_he_sym CASCADE;
DROP DEK swxa_he_add CASCADE;
DROP KEK swxa_he_kek CASCADE;
