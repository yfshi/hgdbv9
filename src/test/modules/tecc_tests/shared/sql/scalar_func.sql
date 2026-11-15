-- ============================================================================
-- TECC 标量函数测试 —— coalesce / nullif / greatest / least / case / decode / power
-- OPE / DET 密态列 + 普通列对照
-- 基于 aggregate_func.sql 的表结构
-- (libpq 独有用例: 断言 tecc_processor 客户端预处理校验)
-- ============================================================================

-- ### 环境准备 ###############################################################
DROP TABLE IF EXISTS enc_pgfunc_test_det CASCADE;
DROP TABLE IF EXISTS enc_pgfunc_test_ope CASCADE;
DROP DEK IF EXISTS enc_pgfunc_dek;
DROP KEK IF EXISTS enc_pgfunc_kek;

CREATE KEK enc_pgfunc_kek WITH (key_store = 'localkms', key_path = '/tmp/enc_pgfunc_kek.key');
CREATE DEK enc_pgfunc_dek WITH VALUES(KEK = 'enc_pgfunc_kek');

-- HE 测试环境 (asym KEK + he-add/he-mul DEK, 同 homomorphic_enc.sql 模式)
DROP DEK IF EXISTS enc_pgfunc_he_mul_dek CASCADE;
DROP DEK IF EXISTS enc_pgfunc_he_add_dek CASCADE;
DROP KEK IF EXISTS enc_pgfunc_he_kek CASCADE;
\! rm -f "$TECC_KEK_PATH/enc_pgfunc_he_asym.priv" "$TECC_KEK_PATH/enc_pgfunc_he_asym.pub" 2>/dev/null
\! tecc_ktool generate --key-name enc_pgfunc_he_asym --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1
CREATE KEK enc_pgfunc_he_kek WITH (
    key_store = 'localkms',
    key_algo = 'sm2',
    key_path = '%KEKPATH%/enc_pgfunc_he_asym.priv'
);
-- he-add 仅 EC 密钥对，he-mul 需 FF 密钥对，故拆两个 DEK（同 homomorphic_enc.sql）
CREATE DEK enc_pgfunc_he_add_dek WITH VALUES(
    KEK = enc_pgfunc_he_kek,
    KEY_ALGO = 'he-add'
);
CREATE DEK enc_pgfunc_he_mul_dek WITH VALUES(
    KEK = enc_pgfunc_he_kek,
    KEY_ALGO = 'he-mul'
);

-- DET 测试表 (deterministic only — 等值比较，无排序)
CREATE TABLE enc_pgfunc_test_det (
    id          serial PRIMARY KEY,
    c_int2      int2_enc    ENCRYPTED WITH (DEK = enc_pgfunc_dek, support_algo = 'det'),
    c_int4      int4_enc    ENCRYPTED WITH (DEK = enc_pgfunc_dek, support_algo = 'det'),
    c_int8      int8_enc    ENCRYPTED WITH (DEK = enc_pgfunc_dek, support_algo = 'det'),
    c_float4    float4_enc  ENCRYPTED WITH (DEK = enc_pgfunc_dek, support_algo = 'det'),
    c_float8    float8_enc  ENCRYPTED WITH (DEK = enc_pgfunc_dek, support_algo = 'det'),
    c_numeric   numeric_enc ENCRYPTED WITH (DEK = enc_pgfunc_dek, support_algo = 'det'),
    c_text      text_enc    ENCRYPTED WITH (DEK = enc_pgfunc_dek, support_algo = 'det,search'),
    c_varchar   varchar_enc ENCRYPTED WITH (DEK = enc_pgfunc_dek, support_algo = 'det,search'),
    c_bpchar    bpchar_enc  ENCRYPTED WITH (DEK = enc_pgfunc_dek, support_algo = 'det,search'),
    n_int4      int4,
    n_text      text,
    n_numeric   numeric
);

-- OPE 测试表 (order-preserving — 保序)
CREATE TABLE enc_pgfunc_test_ope (
    id          serial PRIMARY KEY,
    c_int2      int2_enc    ENCRYPTED WITH (DEK = enc_pgfunc_dek, support_algo = 'ope'),
    c_int4      int4_enc    ENCRYPTED WITH (DEK = enc_pgfunc_dek, support_algo = 'ope'),
    c_int8      int8_enc    ENCRYPTED WITH (DEK = enc_pgfunc_dek, support_algo = 'ope'),
    c_float4    float4_enc  ENCRYPTED WITH (DEK = enc_pgfunc_dek, support_algo = 'ope'),
    c_float8    float8_enc  ENCRYPTED WITH (DEK = enc_pgfunc_dek, support_algo = 'ope'),
    c_numeric   numeric_enc ENCRYPTED WITH (DEK = enc_pgfunc_dek, support_algo = 'ope'),
    c_text      text_enc    ENCRYPTED WITH (DEK = enc_pgfunc_dek, support_algo = 'ope,search'),
    c_varchar   varchar_enc ENCRYPTED WITH (DEK = enc_pgfunc_dek, support_algo = 'ope,search'),
    c_bpchar    bpchar_enc  ENCRYPTED WITH (DEK = enc_pgfunc_dek, support_algo = 'ope,search'),
    n_int4      int4,
    n_text      text,
    n_numeric   numeric
);

-- HE 测试表 (he-add / he-mul — 同态加密)
CREATE TABLE enc_pgfunc_test_he (
    id          serial PRIMARY KEY,
    c_int2      int2_enc    ENCRYPTED WITH (HE_ADD_DEK = 'enc_pgfunc_he_add_dek', SUPPORT_ALGO = 'he-add'),
    c_int4      int4_enc    ENCRYPTED WITH (HE_ADD_DEK = 'enc_pgfunc_he_add_dek', SUPPORT_ALGO = 'he-add'),
    c_int8      int8_enc    ENCRYPTED WITH (HE_ADD_DEK = 'enc_pgfunc_he_add_dek', SUPPORT_ALGO = 'he-add'),
    m_int2      int2_enc    ENCRYPTED WITH (HE_MUL_DEK = 'enc_pgfunc_he_mul_dek', SUPPORT_ALGO = 'he-mul'),
    m_int4      int4_enc    ENCRYPTED WITH (HE_MUL_DEK = 'enc_pgfunc_he_mul_dek', SUPPORT_ALGO = 'he-mul'),
    m_int8      int8_enc    ENCRYPTED WITH (HE_MUL_DEK = 'enc_pgfunc_he_mul_dek', SUPPORT_ALGO = 'he-mul'),
    n_int4      int4
);

-- 测试数据 (5 行: 正/负/零/NULL/特殊值)
INSERT INTO enc_pgfunc_test_det (c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric,
                              c_text, c_varchar, c_bpchar,
                              n_int4, n_text, n_numeric)
VALUES (100, 9999, 1234567890123, 3.14, 2.718281828, 123.456,
        'Hello World', 'VARCHAR值', 'BPCHAR值',
        9999, 'Hello World', 123.456);

INSERT INTO enc_pgfunc_test_det (c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric,
                              c_text, c_varchar, c_bpchar,
                              n_int4, n_text, n_numeric)
VALUES (-50, -1000, -999999999, -1.5, -3.14159, -500.0,
        '中文字符串🎉', 'Mixed混合', '定长char',
        -1000, '中文字符串🎉', -500.0);

INSERT INTO enc_pgfunc_test_det (c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric,
                              c_text, c_varchar, c_bpchar,
                              n_int4, n_text, n_numeric)
VALUES (0, 0, 0, 0.0, 0.0, 0.0,
        '', '', '',
        0, '', 0.0);

INSERT INTO enc_pgfunc_test_det (c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric,
                              c_text, c_varchar, c_bpchar,
                              n_int4, n_text, n_numeric)
VALUES (NULL, NULL, 500, NULL, 1.0, NULL,
        '仅部分列为NULL', NULL, '有值bpchar',
        NULL, '仅部分列为NULL', NULL);

INSERT INTO enc_pgfunc_test_det (c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric,
                              c_text, c_varchar, c_bpchar,
                              n_int4, n_text, n_numeric)
VALUES (200, 8888, 999999999999, 99.99, 12345.6789, 99999.9999,
        'Special: !@#$%^&*()_+-=[]{}|;:''",.<>?/~`',
        'varchar special chars',
        'bpchar_special',
        8888, 'Special: !@#$%^&*()_+-=[]{}|;:''",.<>?/~`', 99999.9999);

INSERT INTO enc_pgfunc_test_ope (c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric,
                              c_text, c_varchar, c_bpchar,
                              n_int4, n_text, n_numeric)
VALUES (100, 9999, 1234567890123, 3.14, 2.718281828, 123.456,
        'Hello World', 'VARCHAR值', 'BPCHAR值',
        9999, 'Hello World', 123.456);

INSERT INTO enc_pgfunc_test_ope (c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric,
                              c_text, c_varchar, c_bpchar,
                              n_int4, n_text, n_numeric)
VALUES (-50, -1000, -999999999, -1.5, -3.14159, -500.0,
        '中文字符串🎉', 'Mixed混合', '定长char',
        -1000, '中文字符串🎉', -500.0);

INSERT INTO enc_pgfunc_test_ope (c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric,
                              c_text, c_varchar, c_bpchar,
                              n_int4, n_text, n_numeric)
VALUES (0, 0, 0, 0.0, 0.0, 0.0,
        '', '', '',
        0, '', 0.0);

INSERT INTO enc_pgfunc_test_ope (c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric,
                              c_text, c_varchar, c_bpchar,
                              n_int4, n_text, n_numeric)
VALUES (NULL, NULL, 500, NULL, 1.0, NULL,
        '仅部分列为NULL', NULL, '有值bpchar',
        NULL, '仅部分列为NULL', NULL);

INSERT INTO enc_pgfunc_test_ope (c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric,
                              c_text, c_varchar, c_bpchar,
                              n_int4, n_text, n_numeric)
VALUES (200, 8888, 999999999999, 99.99, 12345.6789, 99999.9999,
        'Special: !@#$%^&*()_+-=[]{}|;:''",.<>?/~`',
        'varchar special chars',
        'bpchar_special',
        8888, 'Special: !@#$%^&*()_+-=[]{}|;:''",.<>?/~`', 99999.9999);

-- HE 测试数据 (5 行: 小数值 + NULL, 幂运算防溢出)
INSERT INTO enc_pgfunc_test_he (c_int2, c_int4, c_int8, m_int2, m_int4, m_int8, n_int4)
VALUES (10, 100, 1000, 2, 3, 4, 100);

INSERT INTO enc_pgfunc_test_he (c_int2, c_int4, c_int8, m_int2, m_int4, m_int8, n_int4)
VALUES (-5, -50, -500, -2, -3, -4, -50);

INSERT INTO enc_pgfunc_test_he (c_int2, c_int4, c_int8, m_int2, m_int4, m_int8, n_int4)
VALUES (0, 0, 0, 0, 0, 0, 0);

INSERT INTO enc_pgfunc_test_he (c_int2, c_int4, c_int8, m_int2, m_int4, m_int8, n_int4)
VALUES (7, 700, 7000, 3, 5, 6, 700);

INSERT INTO enc_pgfunc_test_he (c_int2, c_int4, c_int8, m_int2, m_int4, m_int8, n_int4)
VALUES (NULL, NULL, NULL, NULL, NULL, NULL, NULL);


-- ============================================================================
-- decode 函数实现 (Oracle 兼容 —— CASE WHEN 等价)
-- 为密态类型提供 int4_enc / text_enc 两个签名的重载
-- ============================================================================

CREATE OR REPLACE FUNCTION decode(v int4_enc, s1 int4_enc, r1 text,
                                        s2 int4_enc, r2 text, d text)
RETURNS text AS $$
  SELECT CASE WHEN $1 = $2 THEN $3
              WHEN $1 = $4 THEN $5
              ELSE $6 END;
$$ LANGUAGE sql IMMUTABLE;

CREATE OR REPLACE FUNCTION decode(v text_enc, s1 text_enc, r1 text, d text)
RETURNS text AS $$
  SELECT CASE WHEN $1 = $2 THEN $3
              ELSE $4 END;
$$ LANGUAGE sql IMMUTABLE;


-- ############################################################################
--                              COALESCE
-- ############################################################################

\echo '========== COALESCE =========='

-- [COAL-OPE-01] COALESCE(c_int4, 0) — OPE
SELECT 'COALESCE(c_int4,0)-OPE' AS test_case, COALESCE(c_int4, 0) AS result FROM enc_pgfunc_test_ope;

-- [COAL-OPE-02] COALESCE(c_text, 'default') — OPE
SELECT 'COALESCE(c_text,''def'')-OPE' AS test_case, COALESCE(c_text, 'default') AS result FROM enc_pgfunc_test_ope;

-- [COAL-OPE-03] COALESCE(c_numeric, 0) — OPE
SELECT 'COALESCE(c_numeric,0)-OPE' AS test_case, COALESCE(c_numeric, 0) AS result FROM enc_pgfunc_test_ope;

-- [COAL-DET-01] COALESCE(c_int4, 0) — DET
SELECT 'COALESCE(c_int4,0)-DET' AS test_case, COALESCE(c_int4, 0) AS result FROM enc_pgfunc_test_det;

-- [COAL-DET-02] COALESCE(c_text, 'default') — DET
SELECT 'COALESCE(c_text,''def'')-DET' AS test_case, COALESCE(c_text, 'default') AS result FROM enc_pgfunc_test_det;

-- [COAL-DET-03] COALESCE(c_numeric, 0) — DET
SELECT 'COALESCE(c_numeric,0)-DET' AS test_case, COALESCE(c_numeric, 0) AS result FROM enc_pgfunc_test_det;

-- [COAL-PLAIN] 普通列对照
SELECT 'COALESCE(n_int4,0)-OPE-plain' AS test_case, COALESCE(n_int4, 0) AS result FROM enc_pgfunc_test_ope;
SELECT 'COALESCE(n_text,''def'')-DET-plain' AS test_case, COALESCE(n_text, 'default') AS result FROM enc_pgfunc_test_det;


-- ############################################################################
--                              NULLIF
-- ############################################################################

\echo '========== NULLIF =========='

-- [NULLIF-OPE-01] NULLIF(c_int4, 0) — OPE
SELECT 'NULLIF(c_int4,0)-OPE' AS test_case, NULLIF(c_int4, 0) AS result FROM enc_pgfunc_test_ope;

-- [NULLIF-OPE-02] NULLIF(c_text, '') — OPE
SELECT 'NULLIF(c_text,'''')-OPE' AS test_case, NULLIF(c_text, '') AS result FROM enc_pgfunc_test_ope;

-- [NULLIF-DET-01] NULLIF(c_int4, 0) — DET
SELECT 'NULLIF(c_int4,0)-DET' AS test_case, NULLIF(c_int4, 0) AS result FROM enc_pgfunc_test_det;

-- [NULLIF-DET-02] NULLIF(c_text, '') — DET
SELECT 'NULLIF(c_text,'''')-DET' AS test_case, NULLIF(c_text, '') AS result FROM enc_pgfunc_test_det;

-- [NULLIF-PLAIN] 普通列对照
SELECT 'NULLIF(n_int4,0)-OPE-plain' AS test_case, NULLIF(n_int4, 0) AS result FROM enc_pgfunc_test_ope;
SELECT 'NULLIF(n_text,'''')-DET-plain' AS test_case, NULLIF(n_text, '') AS result FROM enc_pgfunc_test_det;


-- ############################################################################
--                              GREATEST
-- ############################################################################

\echo '========== GREATEST — OPE (should succeed) =========='

-- [GRT-OPE-01] GREATEST(c_int4, 0) — OPE
SELECT 'GREATEST(c_int4,0)-OPE' AS test_case, GREATEST(c_int4, 0) AS result FROM enc_pgfunc_test_ope;

-- [GRT-OPE-02] GREATEST(c_int8, 100) — OPE
SELECT 'GREATEST(c_int8,100)-OPE' AS test_case, GREATEST(c_int8, 100) AS result FROM enc_pgfunc_test_ope;

-- [GRT-OPE-03] GREATEST(c_numeric, 0) — OPE
SELECT 'GREATEST(c_numeric,0)-OPE' AS test_case, GREATEST(c_numeric, 0) AS result FROM enc_pgfunc_test_ope;

-- [GRT-PLAIN-01] 普通列对照
SELECT 'GREATEST(n_int4,0)-OPE-plain' AS test_case, GREATEST(n_int4, 0) AS result FROM enc_pgfunc_test_ope;
SELECT 'GREATEST(n_numeric,0)-OPE-plain' AS test_case, GREATEST(n_numeric, 0) AS result FROM enc_pgfunc_test_ope;

\echo '========== GREATEST — DET (should error: needs OPE) =========='

-- [GRT-DET-01] GREATEST(c_int4, 0) — DET (should error)
SELECT 'GREATEST(c_int4,0)-DET' AS test_case, GREATEST(c_int4, 0) AS result FROM enc_pgfunc_test_det;

-- [GRT-DET-02] GREATEST(c_int8, 100) — DET (should error)
SELECT 'GREATEST(c_int8,100)-DET' AS test_case, GREATEST(c_int8, 100) AS result FROM enc_pgfunc_test_det;

-- [GRT-DET-03] GREATEST(c_numeric, 0) — DET (should error)
SELECT 'GREATEST(c_numeric,0)-DET' AS test_case, GREATEST(c_numeric, 0) AS result FROM enc_pgfunc_test_det;

-- [GRT-PLAIN-02] DET 表普通列 (should succeed)
SELECT 'GREATEST(n_int4,0)-DET-plain' AS test_case, GREATEST(n_int4, 0) AS result FROM enc_pgfunc_test_det;


-- ############################################################################
--                              LEAST
-- ############################################################################

\echo '========== LEAST — OPE (should succeed) =========='

-- [LST-OPE-01] LEAST(c_int4, 9999) — OPE
SELECT 'LEAST(c_int4,9999)-OPE' AS test_case, LEAST(c_int4, 9999) AS result FROM enc_pgfunc_test_ope;

-- [LST-OPE-02] LEAST(c_int8, 0) — OPE
SELECT 'LEAST(c_int8,0)-OPE' AS test_case, LEAST(c_int8, 0) AS result FROM enc_pgfunc_test_ope;

-- [LST-OPE-03] LEAST(c_numeric, 99999) — OPE
SELECT 'LEAST(c_numeric,99999)-OPE' AS test_case, LEAST(c_numeric, 99999) AS result FROM enc_pgfunc_test_ope;

-- [LST-PLAIN-01] 普通列对照
SELECT 'LEAST(n_int4,9999)-OPE-plain' AS test_case, LEAST(n_int4, 9999) AS result FROM enc_pgfunc_test_ope;
SELECT 'LEAST(n_numeric,99999)-OPE-plain' AS test_case, LEAST(n_numeric, 99999) AS result FROM enc_pgfunc_test_ope;

\echo '========== LEAST — DET (should error: needs OPE) =========='

-- [LST-DET-01] LEAST(c_int4, 9999) — DET (should error)
SELECT 'LEAST(c_int4,9999)-DET' AS test_case, LEAST(c_int4, 9999) AS result FROM enc_pgfunc_test_det;

-- [LST-DET-02] LEAST(c_int8, 0) — DET (should error)
SELECT 'LEAST(c_int8,0)-DET' AS test_case, LEAST(c_int8, 0) AS result FROM enc_pgfunc_test_det;

-- [LST-DET-03] LEAST(c_numeric, 99999) — DET (should error)
SELECT 'LEAST(c_numeric,99999)-DET' AS test_case, LEAST(c_numeric, 99999) AS result FROM enc_pgfunc_test_det;

-- [LST-PLAIN-02] DET 表普通列 (should succeed)
SELECT 'LEAST(n_int4,9999)-DET-plain' AS test_case, LEAST(n_int4, 9999) AS result FROM enc_pgfunc_test_det;


-- ############################################################################
--                              CASE
-- ############################################################################

\echo '========== CASE =========='

-- [CASE-OPE-01] CASE WHEN c_int4 > 0 THEN 'positive' ELSE 'non-positive' END — OPE
SELECT 'CASE-WHEN-OPE' AS test_case,
    CASE WHEN c_int4 > 0 THEN 'positive' ELSE 'non-positive' END
FROM enc_pgfunc_test_ope;

-- [CASE-OPE-02] CASE c_int4 WHEN 9999 THEN 'max' WHEN -1000 THEN 'min' ELSE 'other' END — OPE
SELECT 'CASE-SWITCH-OPE' AS test_case,
    CASE c_int4 WHEN 9999 THEN 'max' WHEN -1000 THEN 'min' ELSE 'other' END
FROM enc_pgfunc_test_ope;

-- [CASE-OPE-03] CASE WHEN c_text = 'Hello World' THEN 'match' ELSE 'no' END — OPE
SELECT 'CASE-text-OPE' AS test_case,
    CASE WHEN c_text = 'Hello World' THEN 'match' ELSE 'no' END
FROM enc_pgfunc_test_ope;

-- [CASE-DET-01] CASE WHEN c_int4 = 9999 THEN 'max' ELSE 'other' END — DET (= only)
SELECT 'CASE-eq-DET' AS test_case,
    CASE WHEN c_int4 = 9999 THEN 'max' ELSE 'other' END
FROM enc_pgfunc_test_det;

-- [CASE-DET-02] CASE WHEN c_text = 'Hello World' THEN 'match' ELSE 'no' END — DET
SELECT 'CASE-text-DET' AS test_case,
    CASE WHEN c_text = 'Hello World' THEN 'match' ELSE 'no' END
FROM enc_pgfunc_test_det;

-- [CASE-DET-03] CASE c_int4 WHEN 0 THEN 'zero' ELSE 'non-zero' END — DET
SELECT 'CASE-SWITCH-DET' AS test_case,
    CASE c_int4 WHEN 0 THEN 'zero' ELSE 'non-zero' END
FROM enc_pgfunc_test_det;

-- [CASE-PLAIN] 普通列对照
SELECT 'CASE-plain-OPE' AS test_case,
    CASE WHEN n_int4 > 0 THEN 'positive' ELSE 'non-positive' END
FROM enc_pgfunc_test_ope;
SELECT 'CASE-plain-DET' AS test_case,
    CASE WHEN n_text = 'Hello World' THEN 'match' ELSE 'no' END
FROM enc_pgfunc_test_det;


-- ############################################################################
--                              DECODE
-- ############################################################################

\echo '========== DECODE =========='

-- [DEC-OPE-01] decode(c_int4, 9999, 'max', -1000, 'min', 'other') — OPE
SELECT 'decode(c_int4)-OPE' AS test_case,
    decode(c_int4, 9999, 'max', -1000, 'min', 'other') AS result
FROM enc_pgfunc_test_ope;

-- [DEC-OPE-02] decode(c_text, 'Hello World', 'matched', 'not found') — OPE
SELECT 'decode(c_text)-OPE' AS test_case,
    decode(c_text, 'Hello World', 'matched', 'not found') AS result
FROM enc_pgfunc_test_ope;

-- [DEC-DET-01] decode(c_int4, 9999, 'max', -1000, 'min', 'other') — DET
SELECT 'decode(c_int4)-DET' AS test_case,
    decode(c_int4, 9999, 'max', -1000, 'min', 'other') AS result
FROM enc_pgfunc_test_det;

-- [DEC-DET-02] decode(c_text, 'Hello World', 'matched', 'not found') — DET
SELECT 'decode(c_text)-DET' AS test_case,
    decode(c_text, 'Hello World', 'matched', 'not found') AS result
FROM enc_pgfunc_test_det;

-- [DEC-PLAIN] 普通列对照
SELECT 'decode(n_int4)-OPE-plain' AS test_case,
    decode(n_int4, 9999, 'max', -1000, 'min', 'other') AS result
FROM enc_pgfunc_test_ope;
SELECT 'decode(n_text)-DET-plain' AS test_case,
    decode(n_text, 'Hello World', 'matched', 'not found') AS result
FROM enc_pgfunc_test_det;


-- ############################################################################
--                              POWER
-- ############################################################################

\echo '========== POWER — OPE (should error: unsupported scalar) =========='

-- [PWR-OPE-01] power(c_int4, 2) — OPE (should error)
SELECT 'power(c_int4,2)-OPE' AS test_case, power(c_int4, 2) AS result FROM enc_pgfunc_test_ope;

-- [PWR-OPE-02] power(c_int8, 3) — OPE (should error)
SELECT 'power(c_int8,3)-OPE' AS test_case, power(c_int8, 3) AS result FROM enc_pgfunc_test_ope;

-- [PWR-OPE-03] power(c_float8, 2) — OPE (should error)
SELECT 'power(c_float8,2)-OPE' AS test_case, power(c_float8, 2) AS result FROM enc_pgfunc_test_ope;

\echo '========== POWER — DET (should error: unsupported scalar) =========='

-- [PWR-DET-01] power(c_int4, 2) — DET (should error)
SELECT 'power(c_int4,2)-DET' AS test_case, power(c_int4, 2) AS result FROM enc_pgfunc_test_det;

-- [PWR-DET-02] power(c_int8, 3) — DET (should error)
SELECT 'power(c_int8,3)-DET' AS test_case, power(c_int8, 3) AS result FROM enc_pgfunc_test_det;

-- [PWR-DET-03] power(c_float8, 2) — DET (should error)
SELECT 'power(c_float8,2)-DET' AS test_case, power(c_float8, 2) AS result FROM enc_pgfunc_test_det;

-- [PWR-PLAIN] 普通列对照 (should succeed)
SELECT 'power(n_int4,2)-OPE-plain' AS test_case, power(n_int4, 2) AS result FROM enc_pgfunc_test_ope;
SELECT 'power(n_int4,2)-DET-plain' AS test_case, power(n_int4, 2) AS result FROM enc_pgfunc_test_det;
SELECT 'power(n_numeric,2)-OPE-plain' AS test_case, power(n_numeric, 2) AS result FROM enc_pgfunc_test_ope;


-- ############################################################################
--                              EXPRESSION + SCALAR
-- ############################################################################

\echo '========== Expression + Scalar =========='

-- Scalar functions nested in expressions
SELECT 'coalesce+expr-OPE' AS test_case, COALESCE(c_int4, 0) + 1 AS result FROM enc_pgfunc_test_ope;
SELECT 'coalesce+expr-DET' AS test_case, COALESCE(c_int4, 0) + 1 AS result FROM enc_pgfunc_test_det;

-- GREATEST in WHERE clause — OPE should succeed
SELECT 'GREATEST-in-WHERE-OPE' AS test_case, c_int4, c_text
FROM enc_pgfunc_test_ope
WHERE GREATEST(c_int4, 0) > 100;

-- LEAST in WHERE clause — DET should error
SELECT 'LEAST-in-WHERE-DET' AS test_case, c_int4
FROM enc_pgfunc_test_det
WHERE LEAST(c_int4, 9999) < 500;

-- power in WHERE clause — should error
SELECT 'power-in-WHERE-OPE' AS test_case, c_int4
FROM enc_pgfunc_test_ope
WHERE power(c_int4, 2) > 100;

-- CASE in WHERE — OPE with >
SELECT 'CASE-in-WHERE-OPE' AS test_case, c_int4
FROM enc_pgfunc_test_ope
WHERE CASE WHEN c_int4 > 0 THEN 1 ELSE 0 END = 1;

-- CASE in WHERE — DET with =
SELECT 'CASE-in-WHERE-DET' AS test_case, c_int4
FROM enc_pgfunc_test_det
WHERE CASE WHEN c_int4 = 9999 THEN 1 ELSE 0 END = 1;


-- ############################################################################
--                HE (he-add / he-mul) — 同态加密列
-- ############################################################################

-- ReqID:TECC4.0 - 需求 4.4.1.4：coalesce 覆盖全部密态类型，常量按列
-- 算法加密——he-add/he-mul 列常量按 HE 算法（公钥）加密后与列密文
-- 同类型，COALESCE 正常执行。
\echo '========== COALESCE — HE (should succeed) =========='

-- [COAL-HE-01] COALESCE(c_int4, 0) — he-add
SELECT 'COALESCE(c_int4,0)-he-add' AS test_case, COALESCE(c_int4, 0) AS result FROM enc_pgfunc_test_he;

-- [COAL-HE-02] COALESCE(m_int4, 0) — he-mul
SELECT 'COALESCE(m_int4,0)-he-mul' AS test_case, COALESCE(m_int4, 0) AS result FROM enc_pgfunc_test_he;

-- [COAL-HE-03] COALESCE(c_int2, 0) — he-add
SELECT 'COALESCE(c_int2,0)-he-add' AS test_case, COALESCE(c_int2, 0) AS result FROM enc_pgfunc_test_he;

-- [COAL-HE-04] COALESCE(m_int8, 0) — he-mul
SELECT 'COALESCE(m_int8,0)-he-mul' AS test_case, COALESCE(m_int8, 0) AS result FROM enc_pgfunc_test_he;

-- [COAL-HE-05] 普通列对照
SELECT 'COALESCE(n_int4,0)-HE-plain' AS test_case, COALESCE(n_int4, 0) AS result FROM enc_pgfunc_test_he;


\echo '========== POWER — he-mul (should succeed) =========='

-- [PWR-HE-01] power(m_int2, 3) — he-mul
SELECT 'power(m_int2,3)-he-mul' AS test_case, power(m_int2, 3) AS result FROM enc_pgfunc_test_he;

-- [PWR-HE-02] power(m_int4, 3) — he-mul
SELECT 'power(m_int4,3)-he-mul' AS test_case, power(m_int4, 3) AS result FROM enc_pgfunc_test_he;

-- [PWR-HE-03] power(m_int8, 3) — he-mul
SELECT 'power(m_int8,3)-he-mul' AS test_case, power(m_int8, 3) AS result FROM enc_pgfunc_test_he;


\echo '========== POWER — he-add (should error: unsupported) =========='

-- [PWR-HE-04] power(c_int2, 3) — he-add (should error)
SELECT 'power(c_int2,3)-he-add' AS test_case, power(c_int2, 3) AS result FROM enc_pgfunc_test_he;

-- [PWR-HE-05] power(c_int4, 2) — he-add (should error)
SELECT 'power(c_int4,2)-he-add' AS test_case, power(c_int4, 2) AS result FROM enc_pgfunc_test_he;

-- [PWR-HE-06] power(c_int8, 3) — he-add (should error)
SELECT 'power(c_int8,3)-he-add' AS test_case, power(c_int8, 3) AS result FROM enc_pgfunc_test_he;

-- [PWR-HE-07] 普通列对照
SELECT 'power(n_int4,2)-HE-plain' AS test_case, power(n_int4, 2) AS result FROM enc_pgfunc_test_he;


\echo '========== Bug 9259 — power error message regression =========='

-- [BUG9259-01] power on he-add column — should error, message should mention he-mul required
-- Bug 9259: power函数不指定he-mul做运算时，报错信息有误
-- 验证：错误消息应明确说明power需要he-mul算法
SELECT 'Bug9259-pwr-he-add-i2' AS bug, power(c_int2, 2) FROM enc_pgfunc_test_he;

-- [BUG9259-02] power on he-add int4
SELECT 'Bug9259-pwr-he-add-i4' AS bug, power(c_int4, 2) FROM enc_pgfunc_test_he;

-- [BUG9259-03] power on he-add int8
SELECT 'Bug9259-pwr-he-add-i8' AS bug, power(c_int8, 3) FROM enc_pgfunc_test_he;

-- [BUG9259-04] power on DET column — should also error with he-mul message
SELECT 'Bug9259-pwr-det-i4' AS bug, power(c_int4, 2) FROM enc_pgfunc_test_det;

-- [BUG9259-05] power on he-mul int4 — should succeed
SELECT 'Bug9259-pwr-he-mul-i4-ok' AS bug, power(m_int4, 2) FROM enc_pgfunc_test_he;

-- [BUG9259-06] power on he-mul int2 — should succeed
SELECT 'Bug9259-pwr-he-mul-i2-ok' AS bug, power(m_int2, 3) FROM enc_pgfunc_test_he;

-- [BUG9259-07] power on he-mul int8 — should succeed
SELECT 'Bug9259-pwr-he-mul-i8-ok' AS bug, power(m_int8, 2) FROM enc_pgfunc_test_he;


-- ### 清理 ###################################################################
DROP TABLE IF EXISTS enc_pgfunc_test_det CASCADE;
DROP TABLE IF EXISTS enc_pgfunc_test_ope CASCADE;
DROP TABLE IF EXISTS enc_pgfunc_test_he CASCADE;
DROP FUNCTION IF EXISTS decode(int4_enc, int4_enc, text, int4_enc, text, text);
DROP FUNCTION IF EXISTS decode(text_enc, text_enc, text, text);
DROP DEK IF EXISTS enc_pgfunc_dek;
DROP KEK IF EXISTS enc_pgfunc_kek;
DROP DEK IF EXISTS enc_pgfunc_he_mul_dek;
DROP DEK IF EXISTS enc_pgfunc_he_add_dek;
DROP KEK IF EXISTS enc_pgfunc_he_kek;
