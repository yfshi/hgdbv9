-- ============================================================================
-- TECC 聚合函数测试 —— OPE / DET 密态列 + 普通列对照
-- 基于 d4fd9b3c7817663b1f72c101a16ad2742fbcfce1.test.sql 的表结构
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


-- ############################################################################
--                                   COUNT
-- ############################################################################

\echo '========== COUNT =========='

-- [COUNT-01] COUNT(*) — OPE 表
SELECT 'COUNT(*)-OPE' AS test_case, COUNT(*) AS result FROM enc_pgfunc_test_ope;

-- [COUNT-02] COUNT(*) — DET 表
SELECT 'COUNT(*)-DET' AS test_case, COUNT(*) AS result FROM enc_pgfunc_test_det;

-- [COUNT-03] COUNT(c_int4) — OPE 表 (含 NULL)
SELECT 'COUNT(c_int4)-OPE' AS test_case, COUNT(c_int4) AS result FROM enc_pgfunc_test_ope;

-- [COUNT-04] COUNT(c_int4) — DET 表 (含 NULL)
SELECT 'COUNT(c_int4)-DET' AS test_case, COUNT(c_int4) AS result FROM enc_pgfunc_test_det;

-- [COUNT-05] COUNT(c_text) — OPE 表
SELECT 'COUNT(c_text)-OPE' AS test_case, COUNT(c_text) AS result FROM enc_pgfunc_test_ope;

-- [COUNT-06] COUNT(c_text) — DET 表
SELECT 'COUNT(c_text)-DET' AS test_case, COUNT(c_text) AS result FROM enc_pgfunc_test_det;

-- [COUNT-07] COUNT(c_numeric) — OPE 表
SELECT 'COUNT(c_numeric)-OPE' AS test_case, COUNT(c_numeric) AS result FROM enc_pgfunc_test_ope;

-- [COUNT-08] COUNT(c_numeric) — DET 表
SELECT 'COUNT(c_numeric)-DET' AS test_case, COUNT(c_numeric) AS result FROM enc_pgfunc_test_det;

-- [COUNT-09] COUNT 对照: 普通列 n_int4 — OPE 表
SELECT 'COUNT(n_int4)-OPE-plain' AS test_case, COUNT(n_int4) AS result FROM enc_pgfunc_test_ope;

-- [COUNT-10] COUNT 对照: 普通列 n_int4 — DET 表
SELECT 'COUNT(n_int4)-DET-plain' AS test_case, COUNT(n_int4) AS result FROM enc_pgfunc_test_det;


-- ############################################################################
--                              COUNT(DISTINCT)
-- ############################################################################

\echo '========== COUNT(DISTINCT) =========='

-- [CD-01] COUNT(DISTINCT c_int4) — OPE
SELECT 'COUNT(DISTINCT c_int4)-OPE' AS test_case, COUNT(DISTINCT c_int4) AS result FROM enc_pgfunc_test_ope;

-- [CD-02] COUNT(DISTINCT c_int4) — DET
SELECT 'COUNT(DISTINCT c_int4)-DET' AS test_case, COUNT(DISTINCT c_int4) AS result FROM enc_pgfunc_test_det;

-- [CD-03] COUNT(DISTINCT c_int2) — OPE
SELECT 'COUNT(DISTINCT c_int2)-OPE' AS test_case, COUNT(DISTINCT c_int2) AS result FROM enc_pgfunc_test_ope;

-- [CD-04] COUNT(DISTINCT c_int2) — DET
SELECT 'COUNT(DISTINCT c_int2)-DET' AS test_case, COUNT(DISTINCT c_int2) AS result FROM enc_pgfunc_test_det;

-- [CD-05] COUNT(DISTINCT c_int8) — OPE
SELECT 'COUNT(DISTINCT c_int8)-OPE' AS test_case, COUNT(DISTINCT c_int8) AS result FROM enc_pgfunc_test_ope;

-- [CD-06] COUNT(DISTINCT c_int8) — DET
SELECT 'COUNT(DISTINCT c_int8)-DET' AS test_case, COUNT(DISTINCT c_int8) AS result FROM enc_pgfunc_test_det;

-- [CD-07] COUNT(DISTINCT c_float4) — OPE
SELECT 'COUNT(DISTINCT c_float4)-OPE' AS test_case, COUNT(DISTINCT c_float4) AS result FROM enc_pgfunc_test_ope;

-- [CD-08] COUNT(DISTINCT c_float4) — DET
SELECT 'COUNT(DISTINCT c_float4)-DET' AS test_case, COUNT(DISTINCT c_float4) AS result FROM enc_pgfunc_test_det;

-- [CD-09] COUNT(DISTINCT c_float8) — OPE
SELECT 'COUNT(DISTINCT c_float8)-OPE' AS test_case, COUNT(DISTINCT c_float8) AS result FROM enc_pgfunc_test_ope;

-- [CD-10] COUNT(DISTINCT c_float8) — DET
SELECT 'COUNT(DISTINCT c_float8)-DET' AS test_case, COUNT(DISTINCT c_float8) AS result FROM enc_pgfunc_test_det;

-- [CD-11] COUNT(DISTINCT c_numeric) — OPE
SELECT 'COUNT(DISTINCT c_numeric)-OPE' AS test_case, COUNT(DISTINCT c_numeric) AS result FROM enc_pgfunc_test_ope;

-- [CD-12] COUNT(DISTINCT c_numeric) — DET
SELECT 'COUNT(DISTINCT c_numeric)-DET' AS test_case, COUNT(DISTINCT c_numeric) AS result FROM enc_pgfunc_test_det;

-- [CD-13] COUNT(DISTINCT c_text) — OPE
SELECT 'COUNT(DISTINCT c_text)-OPE' AS test_case, COUNT(DISTINCT c_text) AS result FROM enc_pgfunc_test_ope;

-- [CD-14] COUNT(DISTINCT c_text) — DET
SELECT 'COUNT(DISTINCT c_text)-DET' AS test_case, COUNT(DISTINCT c_text) AS result FROM enc_pgfunc_test_det;

-- [CD-15] COUNT(DISTINCT c_varchar) — OPE
SELECT 'COUNT(DISTINCT c_varchar)-OPE' AS test_case, COUNT(DISTINCT c_varchar) AS result FROM enc_pgfunc_test_ope;

-- [CD-16] COUNT(DISTINCT c_varchar) — DET
SELECT 'COUNT(DISTINCT c_varchar)-DET' AS test_case, COUNT(DISTINCT c_varchar) AS result FROM enc_pgfunc_test_det;

-- [CD-17] COUNT(DISTINCT c_bpchar) — OPE
SELECT 'COUNT(DISTINCT c_bpchar)-OPE' AS test_case, COUNT(DISTINCT c_bpchar) AS result FROM enc_pgfunc_test_ope;

-- [CD-18] COUNT(DISTINCT c_bpchar) — DET
SELECT 'COUNT(DISTINCT c_bpchar)-DET' AS test_case, COUNT(DISTINCT c_bpchar) AS result FROM enc_pgfunc_test_det;

-- [CD-19] COUNT(DISTINCT) 多列 — OPE
SELECT 'COUNT(DISTINCT c_int4, c_text)-OPE' AS test_case, COUNT(DISTINCT c_int4, c_text) FROM enc_pgfunc_test_ope;

-- [CD-20] COUNT(DISTINCT) 多列 — DET
SELECT 'COUNT(DISTINCT c_int4, c_text)-DET' AS test_case, COUNT(DISTINCT c_int4, c_text) FROM enc_pgfunc_test_det;

-- [CD-21] COUNT(DISTINCT) 普通列对照
SELECT 'COUNT(DISTINCT n_int4)-OPE-plain' AS test_case, COUNT(DISTINCT n_int4) FROM enc_pgfunc_test_ope;
SELECT 'COUNT(DISTINCT n_text)-DET-plain' AS test_case, COUNT(DISTINCT n_text) FROM enc_pgfunc_test_det;


-- ############################################################################
--                              MIN / MAX (OPE)
-- ############################################################################

\echo '========== MIN — OPE (should succeed) =========='

-- [MIN-OPE-01] MIN(c_int2)
SELECT 'MIN(c_int2)-OPE' AS test_case, MIN(c_int2) AS result FROM enc_pgfunc_test_ope;

-- [MIN-OPE-02] MIN(c_int4)
SELECT 'MIN(c_int4)-OPE' AS test_case, MIN(c_int4) AS result FROM enc_pgfunc_test_ope;

-- [MIN-OPE-03] MIN(c_int8)
SELECT 'MIN(c_int8)-OPE' AS test_case, MIN(c_int8) AS result FROM enc_pgfunc_test_ope;

-- [MIN-OPE-04] MIN(c_float4)
SELECT 'MIN(c_float4)-OPE' AS test_case, MIN(c_float4) AS result FROM enc_pgfunc_test_ope;

-- [MIN-OPE-05] MIN(c_float8)
SELECT 'MIN(c_float8)-OPE' AS test_case, MIN(c_float8) AS result FROM enc_pgfunc_test_ope;

-- [MIN-OPE-06] MIN(c_numeric)
SELECT 'MIN(c_numeric)-OPE' AS test_case, MIN(c_numeric) AS result FROM enc_pgfunc_test_ope;

-- [MIN-OPE-07] MIN(c_text) — OPE 支持字符串排序
SELECT 'MIN(c_text)-OPE' AS test_case, MIN(c_text) AS result FROM enc_pgfunc_test_ope;

-- [MIN-OPE-08] MIN(c_varchar)
SELECT 'MIN(c_varchar)-OPE' AS test_case, MIN(c_varchar) AS result FROM enc_pgfunc_test_ope;

-- [MIN-OPE-09] MIN(c_bpchar)
SELECT 'MIN(c_bpchar)-OPE' AS test_case, MIN(c_bpchar) AS result FROM enc_pgfunc_test_ope;

-- [MIN-OPE-10] MIN 普通列对照
SELECT 'MIN(n_int4)-OPE-plain' AS test_case, MIN(n_int4) AS result FROM enc_pgfunc_test_ope;
SELECT 'MIN(n_text)-OPE-plain' AS test_case, MIN(n_text) AS result FROM enc_pgfunc_test_ope;


\echo '========== MAX — OPE (should succeed) =========='

-- [MAX-OPE-01] MAX(c_int2)
SELECT 'MAX(c_int2)-OPE' AS test_case, MAX(c_int2) AS result FROM enc_pgfunc_test_ope;

-- [MAX-OPE-02] MAX(c_int4)
SELECT 'MAX(c_int4)-OPE' AS test_case, MAX(c_int4) AS result FROM enc_pgfunc_test_ope;

-- [MAX-OPE-03] MAX(c_int8)
SELECT 'MAX(c_int8)-OPE' AS test_case, MAX(c_int8) AS result FROM enc_pgfunc_test_ope;

-- [MAX-OPE-04] MAX(c_float4)
SELECT 'MAX(c_float4)-OPE' AS test_case, MAX(c_float4) AS result FROM enc_pgfunc_test_ope;

-- [MAX-OPE-05] MAX(c_float8)
SELECT 'MAX(c_float8)-OPE' AS test_case, MAX(c_float8) AS result FROM enc_pgfunc_test_ope;

-- [MAX-OPE-06] MAX(c_numeric)
SELECT 'MAX(c_numeric)-OPE' AS test_case, MAX(c_numeric) AS result FROM enc_pgfunc_test_ope;

-- [MAX-OPE-07] MAX(c_text)
SELECT 'MAX(c_text)-OPE' AS test_case, MAX(c_text) AS result FROM enc_pgfunc_test_ope;

-- [MAX-OPE-08] MAX(c_varchar)
SELECT 'MAX(c_varchar)-OPE' AS test_case, MAX(c_varchar) AS result FROM enc_pgfunc_test_ope;

-- [MAX-OPE-09] MAX(c_bpchar)
SELECT 'MAX(c_bpchar)-OPE' AS test_case, MAX(c_bpchar) AS result FROM enc_pgfunc_test_ope;

-- [MAX-OPE-10] MAX 普通列对照
SELECT 'MAX(n_int4)-OPE-plain' AS test_case, MAX(n_int4) AS result FROM enc_pgfunc_test_ope;
SELECT 'MAX(n_text)-OPE-plain' AS test_case, MAX(n_text) AS result FROM enc_pgfunc_test_ope;


-- ############################################################################
--                           MIN / MAX (DET — 应报错)
-- ############################################################################

\echo '========== MIN — DET (should error: needs OPE) =========='

-- [MIN-DET-01] MIN(c_int2)
SELECT 'MIN(c_int2)-DET' AS test_case, MIN(c_int2) AS result FROM enc_pgfunc_test_det;

-- [MIN-DET-02] MIN(c_int4)
SELECT 'MIN(c_int4)-DET' AS test_case, MIN(c_int4) AS result FROM enc_pgfunc_test_det;

-- [MIN-DET-03] MIN(c_int8)
SELECT 'MIN(c_int8)-DET' AS test_case, MIN(c_int8) AS result FROM enc_pgfunc_test_det;

-- [MIN-DET-04] MIN(c_float4)
SELECT 'MIN(c_float4)-DET' AS test_case, MIN(c_float4) AS result FROM enc_pgfunc_test_det;

-- [MIN-DET-05] MIN(c_float8)
SELECT 'MIN(c_float8)-DET' AS test_case, MIN(c_float8) AS result FROM enc_pgfunc_test_det;

-- [MIN-DET-06] MIN(c_numeric)
SELECT 'MIN(c_numeric)-DET' AS test_case, MIN(c_numeric) AS result FROM enc_pgfunc_test_det;

-- [MIN-DET-07] MIN(c_text)
SELECT 'MIN(c_text)-DET' AS test_case, MIN(c_text) AS result FROM enc_pgfunc_test_det;

-- [MIN-DET-08] MIN(c_varchar)
SELECT 'MIN(c_varchar)-DET' AS test_case, MIN(c_varchar) AS result FROM enc_pgfunc_test_det;

-- [MIN-DET-09] MIN(c_bpchar)
SELECT 'MIN(c_bpchar)-DET' AS test_case, MIN(c_bpchar) AS result FROM enc_pgfunc_test_det;


\echo '========== MAX — DET (should error: needs OPE) =========='

-- [MAX-DET-01] MAX(c_int2)
SELECT 'MAX(c_int2)-DET' AS test_case, MAX(c_int2) AS result FROM enc_pgfunc_test_det;

-- [MAX-DET-02] MAX(c_int4)
SELECT 'MAX(c_int4)-DET' AS test_case, MAX(c_int4) AS result FROM enc_pgfunc_test_det;

-- [MAX-DET-03] MAX(c_int8)
SELECT 'MAX(c_int8)-DET' AS test_case, MAX(c_int8) AS result FROM enc_pgfunc_test_det;

-- [MAX-DET-04] MAX(c_float4)
SELECT 'MAX(c_float4)-DET' AS test_case, MAX(c_float4) AS result FROM enc_pgfunc_test_det;

-- [MAX-DET-05] MAX(c_float8)
SELECT 'MAX(c_float8)-DET' AS test_case, MAX(c_float8) AS result FROM enc_pgfunc_test_det;

-- [MAX-DET-06] MAX(c_numeric)
SELECT 'MAX(c_numeric)-DET' AS test_case, MAX(c_numeric) AS result FROM enc_pgfunc_test_det;

-- [MAX-DET-07] MAX(c_text)
SELECT 'MAX(c_text)-DET' AS test_case, MAX(c_text) AS result FROM enc_pgfunc_test_det;

-- [MAX-DET-08] MAX(c_varchar)
SELECT 'MAX(c_varchar)-DET' AS test_case, MAX(c_varchar) AS result FROM enc_pgfunc_test_det;

-- [MAX-DET-09] MAX(c_bpchar)
SELECT 'MAX(c_bpchar)-DET' AS test_case, MAX(c_bpchar) AS result FROM enc_pgfunc_test_det;


-- ############################################################################
--                           SUM (OPE + DET — 均应报错)
-- ############################################################################

\echo '========== SUM — OPE (should error: unsupported aggregate) =========='

-- [SUM-OPE-01] SUM(c_int2)
SELECT 'SUM(c_int2)-OPE' AS test_case, SUM(c_int2) AS result FROM enc_pgfunc_test_ope;

-- [SUM-OPE-02] SUM(c_int4)
SELECT 'SUM(c_int4)-OPE' AS test_case, SUM(c_int4) AS result FROM enc_pgfunc_test_ope;

-- [SUM-OPE-03] SUM(c_int8)
SELECT 'SUM(c_int8)-OPE' AS test_case, SUM(c_int8) AS result FROM enc_pgfunc_test_ope;

-- [SUM-OPE-04] SUM(c_float4)
SELECT 'SUM(c_float4)-OPE' AS test_case, SUM(c_float4) AS result FROM enc_pgfunc_test_ope;

-- [SUM-OPE-05] SUM(c_float8)
SELECT 'SUM(c_float8)-OPE' AS test_case, SUM(c_float8) AS result FROM enc_pgfunc_test_ope;

-- [SUM-OPE-06] SUM(c_numeric)
SELECT 'SUM(c_numeric)-OPE' AS test_case, SUM(c_numeric) AS result FROM enc_pgfunc_test_ope;

-- [SUM-OPE-07] SUM 普通列对照 (应成功)
SELECT 'SUM(n_int4)-OPE-plain' AS test_case, SUM(n_int4) AS result FROM enc_pgfunc_test_ope;
SELECT 'SUM(n_numeric)-OPE-plain' AS test_case, SUM(n_numeric) AS result FROM enc_pgfunc_test_ope;


\echo '========== SUM — DET (should error: unsupported aggregate) =========='

-- [SUM-DET-01] SUM(c_int2)
SELECT 'SUM(c_int2)-DET' AS test_case, SUM(c_int2) AS result FROM enc_pgfunc_test_det;

-- [SUM-DET-02] SUM(c_int4)
SELECT 'SUM(c_int4)-DET' AS test_case, SUM(c_int4) AS result FROM enc_pgfunc_test_det;

-- [SUM-DET-03] SUM(c_int8)
SELECT 'SUM(c_int8)-DET' AS test_case, SUM(c_int8) AS result FROM enc_pgfunc_test_det;

-- [SUM-DET-04] SUM(c_float4)
SELECT 'SUM(c_float4)-DET' AS test_case, SUM(c_float4) AS result FROM enc_pgfunc_test_det;

-- [SUM-DET-05] SUM(c_float8)
SELECT 'SUM(c_float8)-DET' AS test_case, SUM(c_float8) AS result FROM enc_pgfunc_test_det;

-- [SUM-DET-06] SUM(c_numeric)
SELECT 'SUM(c_numeric)-DET' AS test_case, SUM(c_numeric) AS result FROM enc_pgfunc_test_det;

-- [SUM-DET-07] SUM 普通列对照 (应成功)
SELECT 'SUM(n_int4)-DET-plain' AS test_case, SUM(n_int4) AS result FROM enc_pgfunc_test_det;
SELECT 'SUM(n_numeric)-DET-plain' AS test_case, SUM(n_numeric) AS result FROM enc_pgfunc_test_det;


-- ############################################################################
--                           AVG (OPE + DET — 均应报错)
-- ############################################################################

\echo '========== AVG — OPE (should error: unsupported aggregate) =========='

-- [AVG-OPE-01] AVG(c_int2)
SELECT 'AVG(c_int2)-OPE' AS test_case, AVG(c_int2) AS result FROM enc_pgfunc_test_ope;

-- [AVG-OPE-02] AVG(c_int4)
SELECT 'AVG(c_int4)-OPE' AS test_case, AVG(c_int4) AS result FROM enc_pgfunc_test_ope;

-- [AVG-OPE-03] AVG(c_int8)
SELECT 'AVG(c_int8)-OPE' AS test_case, AVG(c_int8) AS result FROM enc_pgfunc_test_ope;

-- [AVG-OPE-04] AVG(c_float4)
SELECT 'AVG(c_float4)-OPE' AS test_case, AVG(c_float4) AS result FROM enc_pgfunc_test_ope;

-- [AVG-OPE-05] AVG(c_float8)
SELECT 'AVG(c_float8)-OPE' AS test_case, AVG(c_float8) AS result FROM enc_pgfunc_test_ope;

-- [AVG-OPE-06] AVG(c_numeric)
SELECT 'AVG(c_numeric)-OPE' AS test_case, AVG(c_numeric) AS result FROM enc_pgfunc_test_ope;

-- [AVG-OPE-07] AVG 普通列对照 (应成功)
SELECT 'AVG(n_int4)-OPE-plain' AS test_case, AVG(n_int4) AS result FROM enc_pgfunc_test_ope;
SELECT 'AVG(n_numeric)-OPE-plain' AS test_case, AVG(n_numeric) AS result FROM enc_pgfunc_test_ope;


\echo '========== AVG — DET (should error: unsupported aggregate) =========='

-- [AVG-DET-01] AVG(c_int2)
SELECT 'AVG(c_int2)-DET' AS test_case, AVG(c_int2) AS result FROM enc_pgfunc_test_det;

-- [AVG-DET-02] AVG(c_int4)
SELECT 'AVG(c_int4)-DET' AS test_case, AVG(c_int4) AS result FROM enc_pgfunc_test_det;

-- [AVG-DET-03] AVG(c_int8)
SELECT 'AVG(c_int8)-DET' AS test_case, AVG(c_int8) AS result FROM enc_pgfunc_test_det;

-- [AVG-DET-04] AVG(c_float4)
SELECT 'AVG(c_float4)-DET' AS test_case, AVG(c_float4) AS result FROM enc_pgfunc_test_det;

-- [AVG-DET-05] AVG(c_float8)
SELECT 'AVG(c_float8)-DET' AS test_case, AVG(c_float8) AS result FROM enc_pgfunc_test_det;

-- [AVG-DET-06] AVG(c_numeric)
SELECT 'AVG(c_numeric)-DET' AS test_case, AVG(c_numeric) AS result FROM enc_pgfunc_test_det;

-- [AVG-DET-07] AVG 普通列对照 (应成功)
SELECT 'AVG(n_int4)-DET-plain' AS test_case, AVG(n_int4) AS result FROM enc_pgfunc_test_det;
SELECT 'AVG(n_numeric)-DET-plain' AS test_case, AVG(n_numeric) AS result FROM enc_pgfunc_test_det;


-- ############################################################################
--                     string_agg (OPE + DET — 均应报错)
-- ############################################################################

\echo '========== string_agg — OPE (should error: unsupported aggregate) =========='

-- [SA-OPE-01] string_agg(c_text, ',')
SELECT 'string_agg(c_text)-OPE' AS test_case, string_agg(c_text, ',') AS result FROM enc_pgfunc_test_ope;

-- [SA-OPE-02] string_agg(c_varchar, '|')
SELECT 'string_agg(c_varchar)-OPE' AS test_case, string_agg(c_varchar, '|') AS result FROM enc_pgfunc_test_ope;

-- [SA-OPE-03] string_agg(c_bpchar, ' - ')
SELECT 'string_agg(c_bpchar)-OPE' AS test_case, string_agg(c_bpchar, ' - ') AS result FROM enc_pgfunc_test_ope;

-- [SA-OPE-04] string_agg(c_text, ',' ORDER BY c_text) — 带 ORDER BY
SELECT 'string_agg(c_text ORDER BY c_text)-OPE' AS test_case, string_agg(c_text, ',' ORDER BY c_text) AS result FROM enc_pgfunc_test_ope;

-- [SA-OPE-05] string_agg 普通列对照 (应成功)
SELECT 'string_agg(n_text)-OPE-plain' AS test_case, string_agg(n_text, ',') AS result FROM enc_pgfunc_test_ope;


\echo '========== string_agg — DET (should error: unsupported aggregate) =========='

-- [SA-DET-01] string_agg(c_text, ',')
SELECT 'string_agg(c_text)-DET' AS test_case, string_agg(c_text, ',') AS result FROM enc_pgfunc_test_det;

-- [SA-DET-02] string_agg(c_varchar, '|')
SELECT 'string_agg(c_varchar)-DET' AS test_case, string_agg(c_varchar, '|') AS result FROM enc_pgfunc_test_det;

-- [SA-DET-03] string_agg(c_bpchar, ' - ')
SELECT 'string_agg(c_bpchar)-DET' AS test_case, string_agg(c_bpchar, ' - ') AS result FROM enc_pgfunc_test_det;

-- [SA-DET-04] string_agg 普通列对照 (应成功)
SELECT 'string_agg(n_text)-DET-plain' AS test_case, string_agg(n_text, ',') AS result FROM enc_pgfunc_test_det;


-- ############################################################################
--                    array_agg — 加密列应报错，普通列正常
-- ############################################################################

\echo '========== array_agg — OPE (encrypted columns should fail) =========='

-- [AA-OPE-01] array_agg(c_int2) — 应报错
SELECT 'array_agg(c_int2)-OPE' AS test_case, array_agg(c_int2) AS result FROM enc_pgfunc_test_ope;

-- [AA-OPE-02] array_agg(c_int4) — 应报错
SELECT 'array_agg(c_int4)-OPE' AS test_case, array_agg(c_int4) AS result FROM enc_pgfunc_test_ope;

-- [AA-OPE-07] array_agg(c_text) — 应报错
SELECT 'array_agg(c_text)-OPE' AS test_case, array_agg(c_text) AS result FROM enc_pgfunc_test_ope;

-- [AA-OPE-10] array_agg(c_text ORDER BY c_text) — 应报错
SELECT 'array_agg(c_text ORDER BY c_text)-OPE' AS test_case, array_agg(c_text ORDER BY c_text) AS result FROM enc_pgfunc_test_ope;

-- [AA-OPE-11] array_agg 普通列对照 (应成功)
SELECT 'array_agg(n_int4)-OPE-plain' AS test_case, array_agg(n_int4) AS result FROM enc_pgfunc_test_ope;
SELECT 'array_agg(n_text)-OPE-plain' AS test_case, array_agg(n_text) AS result FROM enc_pgfunc_test_ope;


\echo '========== array_agg — DET (encrypted columns should fail) =========='

-- [AA-DET-01] array_agg(c_int2) — 应报错
SELECT 'array_agg(c_int2)-DET' AS test_case, array_agg(c_int2) AS result FROM enc_pgfunc_test_det;

-- [AA-DET-02] array_agg(c_int4) — 应报错
SELECT 'array_agg(c_int4)-DET' AS test_case, array_agg(c_int4) AS result FROM enc_pgfunc_test_det;

-- [AA-DET-07] array_agg(c_text) — 应报错
SELECT 'array_agg(c_text)-DET' AS test_case, array_agg(c_text) AS result FROM enc_pgfunc_test_det;

-- [AA-DET-10] array_agg 普通列对照 (应成功)
SELECT 'array_agg(n_int4)-DET-plain' AS test_case, array_agg(n_int4) AS result FROM enc_pgfunc_test_det;
SELECT 'array_agg(n_text)-DET-plain' AS test_case, array_agg(n_text) AS result FROM enc_pgfunc_test_det;


-- ############################################################################
--                MIN/MAX with GROUP BY (OPE — should succeed)
-- ############################################################################

\echo '========== MIN/MAX with GROUP BY — OPE =========='

-- [GB-01] GROUP BY c_int4
SELECT 'GROUP BY c_int4-OPE' AS test_case, c_int4, COUNT(*) AS cnt, MIN(c_int2), MAX(c_int8)
FROM enc_pgfunc_test_ope GROUP BY c_int4 ORDER BY c_int4;

-- [GB-02] GROUP BY c_text
SELECT 'GROUP BY c_text-OPE' AS test_case, c_text, COUNT(*), MIN(c_varchar), MAX(c_bpchar)
FROM enc_pgfunc_test_ope WHERE c_text IS NOT NULL GROUP BY c_text ORDER BY c_text;


-- ############################################################################
--                   Window Function (OVER) — OPE + DET
-- ############################################################################

\echo '========== Window Functions — OPE =========='

-- [WF-01] MIN OVER — OPE
SELECT 'MIN() OVER-OPE' AS test_case, id, c_int4,
       MIN(c_int4) OVER (ORDER BY id) AS running_min
FROM enc_pgfunc_test_ope ORDER BY id;

-- [WF-02] MAX OVER — OPE
SELECT 'MAX() OVER-OPE' AS test_case, id, c_int4,
       MAX(c_int4) OVER (PARTITION BY c_text ORDER BY id) AS part_max
FROM enc_pgfunc_test_ope ORDER BY id;

-- [WF-03] COUNT OVER — OPE
SELECT 'COUNT() OVER-OPE' AS test_case, id, c_text,
       COUNT(c_int4) OVER (PARTITION BY c_text) AS part_cnt
FROM enc_pgfunc_test_ope ORDER BY id;

-- [WF-04] array_agg OVER — OPE (should error: array_agg on encrypted column)
SELECT 'array_agg() OVER-OPE' AS test_case, id, c_text,
       array_agg(c_int4) OVER (PARTITION BY c_text ORDER BY id) AS part_arr
FROM enc_pgfunc_test_ope ORDER BY id;


\echo '========== Window Functions — DET =========='

-- [WF-05] COUNT OVER — DET
SELECT 'COUNT() OVER-DET' AS test_case, id, c_text,
       COUNT(c_int4) OVER (PARTITION BY c_text) AS part_cnt
FROM enc_pgfunc_test_det ORDER BY id;

-- [WF-06] array_agg OVER — DET (should error: array_agg on encrypted column)
SELECT 'array_agg() OVER-DET' AS test_case, id, c_text,
       array_agg(c_int4) OVER (PARTITION BY c_text ORDER BY id) AS part_arr
FROM enc_pgfunc_test_det ORDER BY id;

-- [WF-07] MIN OVER — DET (should error: needs OPE)
SELECT 'MIN() OVER-DET' AS test_case, id, c_int4,
       MIN(c_int4) OVER (ORDER BY id) AS running_min
FROM enc_pgfunc_test_det ORDER BY id;

-- [WF-08] MAX OVER — DET (should error: needs OPE)
SELECT 'MAX() OVER-DET' AS test_case, id, c_int4,
       MAX(c_int4) OVER (PARTITION BY c_text ORDER BY id) AS part_max
FROM enc_pgfunc_test_det ORDER BY id;

-- [WF-09] SUM OVER — OPE (should error: unsupported aggregate)
SELECT 'SUM() OVER-OPE' AS test_case, id, c_int4,
       SUM(c_int4) OVER (ORDER BY id) AS running_sum
FROM enc_pgfunc_test_ope ORDER BY id;

-- [WF-10] SUM OVER — DET (should error: unsupported aggregate)
SELECT 'SUM() OVER-DET' AS test_case, id, c_int4,
       SUM(c_int4) OVER (ORDER BY id) AS running_sum
FROM enc_pgfunc_test_det ORDER BY id;

-- [WF-11] AVG OVER — OPE (should error: unsupported aggregate)
SELECT 'AVG() OVER-OPE' AS test_case, id, c_int4,
       AVG(c_int4) OVER (ORDER BY id) AS running_avg
FROM enc_pgfunc_test_ope ORDER BY id;

-- [WF-12] AVG OVER — DET (should error: unsupported aggregate)
SELECT 'AVG() OVER-DET' AS test_case, id, c_int4,
       AVG(c_int4) OVER (ORDER BY id) AS running_avg
FROM enc_pgfunc_test_det ORDER BY id;


-- ############################################################################
--                   HAVING with Aggregate — OPE + DET
-- ############################################################################

\echo '========== HAVING with Aggregate =========='

-- [HV-01] HAVING COUNT > N — OPE
SELECT 'HAVING COUNT-OPE' AS test_case, c_text, COUNT(*) AS cnt
FROM enc_pgfunc_test_ope
WHERE c_text IS NOT NULL
GROUP BY c_text HAVING COUNT(*) > 1
ORDER BY c_text;

-- [HV-02] HAVING COUNT > N — DET
SELECT 'HAVING COUNT-DET' AS test_case, c_text, COUNT(*) AS cnt
FROM enc_pgfunc_test_det
WHERE c_text IS NOT NULL
GROUP BY c_text HAVING COUNT(*) > 1
ORDER BY c_text;

-- [HV-03] HAVING MIN > N — OPE (should succeed)
SELECT 'HAVING MIN-OPE' AS test_case, c_int4, COUNT(*) AS cnt, MIN(c_int2)
FROM enc_pgfunc_test_ope
GROUP BY c_int4 HAVING MIN(c_int2) < 0
ORDER BY c_int4;

-- [HV-04] HAVING MIN > N — DET (should error: MIN on DET column)
SELECT 'HAVING MIN-DET' AS test_case, c_int4, COUNT(*) AS cnt, MIN(c_int2)
FROM enc_pgfunc_test_det
GROUP BY c_int4 HAVING MIN(c_int2) < 0
ORDER BY c_int4;

-- [HV-05] HAVING SUM > N — OPE (should error: SUM unsupported)
SELECT 'HAVING SUM-OPE' AS test_case, c_int4, COUNT(*), SUM(c_int8) AS total
FROM enc_pgfunc_test_ope
GROUP BY c_int4 HAVING SUM(c_int8) > 100
ORDER BY c_int4;

-- [HV-06] HAVING SUM > N — DET (should error: SUM unsupported)
SELECT 'HAVING SUM-DET' AS test_case, c_int4, COUNT(*), SUM(c_int8) AS total
FROM enc_pgfunc_test_det
GROUP BY c_int4 HAVING SUM(c_int8) > 100
ORDER BY c_int4;

-- [HV-07] HAVING with COUNT(DISTINCT) — OPE (should succeed)
SELECT 'HAVING COUNT(DISTINCT)-OPE' AS test_case, c_int4, COUNT(DISTINCT c_text) AS distinct_texts
FROM enc_pgfunc_test_ope
GROUP BY c_int4 HAVING COUNT(DISTINCT c_text) > 0
ORDER BY c_int4;

-- [HV-08] HAVING with COUNT(DISTINCT) — DET (should succeed)
SELECT 'HAVING COUNT(DISTINCT)-DET' AS test_case, c_int4, COUNT(DISTINCT c_text) AS distinct_texts
FROM enc_pgfunc_test_det
GROUP BY c_int4 HAVING COUNT(DISTINCT c_text) > 0
ORDER BY c_int4;

-- [HV-09] HAVING with array_agg — OPE (should error: array_agg on encrypted column)
SELECT 'HAVING array_agg-OPE' AS test_case, c_text, array_agg(c_int4) AS arr
FROM enc_pgfunc_test_ope
GROUP BY c_text HAVING array_length(array_agg(c_int4), 1) > 0
ORDER BY c_text;

-- [HV-10] HAVING with array_agg — DET (should error: array_agg on encrypted column)
SELECT 'HAVING array_agg-DET' AS test_case, c_text, array_agg(c_int4) AS arr
FROM enc_pgfunc_test_det
GROUP BY c_text HAVING array_length(array_agg(c_int4), 1) > 0
ORDER BY c_text;


-- ############################################################################
--                     SELECT 嵌套表达式 + 聚合
-- ############################################################################

\echo '========== Expression + Aggregate =========='

-- [EXPR-01] COUNT(DISTINCT) with CASE — OPE
SELECT 'COUNT(DISTINCT CASE)-OPE' AS test_case,
       COUNT(DISTINCT CASE WHEN c_int4 > 0 THEN c_text ELSE NULL END)
FROM enc_pgfunc_test_ope;

-- [EXPR-02] COUNT(DISTINCT) with CASE — DET
SELECT 'COUNT(DISTINCT CASE)-DET' AS test_case,
       COUNT(DISTINCT CASE WHEN c_int4 > 0 THEN c_text ELSE NULL END)
FROM enc_pgfunc_test_det;

-- [EXPR-03] COALESCE + MIN — OPE (should succeed)
SELECT 'COALESCE+MIN-OPE' AS test_case, COALESCE(MIN(c_int4), -1) AS min_val
FROM enc_pgfunc_test_ope;

-- [EXPR-04] COALESCE + MIN — DET (should error: MIN on DET)
SELECT 'COALESCE+MIN-DET' AS test_case, COALESCE(MIN(c_int4), -1) AS min_val
FROM enc_pgfunc_test_det;

-- [EXPR-05] GREATEST + MIN/MAX — OPE (should succeed)
SELECT 'GREATEST+array_agg-OPE' AS test_case,
       GREATEST(COALESCE(MIN(c_int4), 0), COALESCE(MAX(c_int4), 0)) AS range_max
FROM enc_pgfunc_test_ope;

-- [EXPR-06] SUM + arithmetic — OPE (should error: SUM unsupported)
SELECT 'SUM+expr-OPE' AS test_case, SUM(c_int4) + 100 AS adjusted_sum
FROM enc_pgfunc_test_ope;

-- [EXPR-07] SUM + arithmetic — DET (should error: SUM unsupported)
SELECT 'SUM+expr-DET' AS test_case, SUM(c_int4) + 100 AS adjusted_sum
FROM enc_pgfunc_test_det;


-- ############################################################################
--                              子查询 + 聚合
-- ############################################################################

\echo '========== Subquery + Aggregate =========='

-- [SUB-01] Subquery with COUNT — OPE
SELECT 'Subquery COUNT-OPE' AS test_case, *
FROM (SELECT c_int4, COUNT(*) AS cnt FROM enc_pgfunc_test_ope GROUP BY c_int4) sub
WHERE cnt >= 1 ORDER BY c_int4;

-- [SUB-02] Subquery with MIN — OPE (should succeed)
SELECT 'Subquery MIN-OPE' AS test_case, *
FROM (SELECT c_text, MIN(c_int4) AS min_val FROM enc_pgfunc_test_ope GROUP BY c_text) sub
ORDER BY c_text;

-- [SUB-03] Subquery with MIN — DET (should error)
SELECT 'Subquery MIN-DET' AS test_case, *
FROM (SELECT c_text, MIN(c_int4) AS min_val FROM enc_pgfunc_test_det GROUP BY c_text) sub
ORDER BY c_text;

-- [SUB-04] Subquery with array_agg — OPE (should error: array_agg on encrypted column)
SELECT 'Subquery array_agg-OPE' AS test_case, *
FROM (SELECT c_text, array_agg(c_int4 ORDER BY c_int4) AS arr FROM enc_pgfunc_test_ope GROUP BY c_text) sub
ORDER BY c_text;

-- [SUB-05] Subquery with array_agg — DET (should error: array_agg on encrypted column)
SELECT 'Subquery array_agg-DET' AS test_case, *
FROM (SELECT c_text, array_agg(c_int4 ORDER BY c_int4) AS arr FROM enc_pgfunc_test_det GROUP BY c_text) sub
ORDER BY c_text;


-- ############################################################################
--                HE (he-add / he-mul) — 同态加密列
-- ############################################################################

\echo '========== HE COUNT (should succeed) =========='

-- [HE-CNT-01] COUNT(c_int4) — he-add
SELECT 'COUNT(c_int4)-he-add' AS test_case, COUNT(c_int4) AS result FROM enc_pgfunc_test_he;

-- [HE-CNT-02] COUNT(m_int4) — he-mul
SELECT 'COUNT(m_int4)-he-mul' AS test_case, COUNT(m_int4) AS result FROM enc_pgfunc_test_he;

-- [HE-CNT-03] COUNT(c_int2) — he-add
SELECT 'COUNT(c_int2)-he-add' AS test_case, COUNT(c_int2) AS result FROM enc_pgfunc_test_he;

-- [HE-CNT-04] COUNT(m_int8) — he-mul
SELECT 'COUNT(m_int8)-he-mul' AS test_case, COUNT(m_int8) AS result FROM enc_pgfunc_test_he;

-- [HE-CNT-05] COUNT 普通列对照
SELECT 'COUNT(n_int4)-HE-plain' AS test_case, COUNT(n_int4) AS result FROM enc_pgfunc_test_he;


-- he-add/he-mul 列无 DET/OPE 密文段，服务端哈希去重不可用，
-- COUNT(DISTINCT) 报 "cannot hash encrypted value"（同 4f0db5645d6 的
-- TupleHashTable 去重路径约束）。
\echo '========== HE COUNT(DISTINCT) (should error: cannot hash) =========='

-- [HE-CD-01] COUNT(DISTINCT c_int4) — he-add (should error: cannot hash)
SELECT 'COUNT(DISTINCT c_int4)-he-add' AS test_case, COUNT(DISTINCT c_int4) AS result FROM enc_pgfunc_test_he;

-- [HE-CD-02] COUNT(DISTINCT m_int4) — he-mul (should error: cannot hash)
SELECT 'COUNT(DISTINCT m_int4)-he-mul' AS test_case, COUNT(DISTINCT m_int4) AS result FROM enc_pgfunc_test_he;

-- [HE-CD-03] COUNT(DISTINCT) 普通列对照
SELECT 'COUNT(DISTINCT n_int4)-HE-plain' AS test_case, COUNT(DISTINCT n_int4) AS result FROM enc_pgfunc_test_he;


\echo '========== HE SUM — he-add (should succeed) =========='

-- [HE-SUM-01] SUM(c_int2) — he-add
SELECT 'SUM(c_int2)-he-add' AS test_case, SUM(c_int2) AS result FROM enc_pgfunc_test_he;

-- [HE-SUM-02] SUM(c_int4) — he-add
SELECT 'SUM(c_int4)-he-add' AS test_case, SUM(c_int4) AS result FROM enc_pgfunc_test_he;

-- [HE-SUM-03] SUM(c_int8) — he-add
SELECT 'SUM(c_int8)-he-add' AS test_case, SUM(c_int8) AS result FROM enc_pgfunc_test_he;

-- [HE-SUM-04] SUM 普通列对照
SELECT 'SUM(n_int4)-HE-plain' AS test_case, SUM(n_int4) AS result FROM enc_pgfunc_test_he;


\echo '========== HE SUM — he-mul (should error: unsupported) =========='

-- [HE-SUM-05] SUM(m_int2) — he-mul (should error)
SELECT 'SUM(m_int2)-he-mul' AS test_case, SUM(m_int2) AS result FROM enc_pgfunc_test_he;

-- [HE-SUM-06] SUM(m_int4) — he-mul (should error)
SELECT 'SUM(m_int4)-he-mul' AS test_case, SUM(m_int4) AS result FROM enc_pgfunc_test_he;

-- [HE-SUM-07] SUM(m_int8) — he-mul (should error)
SELECT 'SUM(m_int8)-he-mul' AS test_case, SUM(m_int8) AS result FROM enc_pgfunc_test_he;


\echo '========== Bug 9266 / 9253 — SUM error message regression =========='

-- [BUG9266-01] SUM on he-mul column — error should mention he-add required
-- Bug 9266: he-mul列SUM聚合报错信息存在歧义
-- Bug 9253: 未指定he-add的列求和报错信息有误
-- 验证：错误消息应明确说明SUM仅支持he-add算法
SELECT 'Bug9266-sum-he-mul' AS bug, SUM(m_int4) FROM enc_pgfunc_test_he;

-- [BUG9266-02] SUM on he-mul int2 column
SELECT 'Bug9266-sum-he-mul-i2' AS bug, SUM(m_int2) FROM enc_pgfunc_test_he;

-- [BUG9266-03] SUM on he-mul int8 column
SELECT 'Bug9266-sum-he-mul-i8' AS bug, SUM(m_int8) FROM enc_pgfunc_test_he;

-- [BUG9253-01] SUM on DET-only column — error should mention he-add required
-- Bug 9253: SUM函数对未指定he-add的列求和报错信息有误
SELECT 'Bug9253-sum-det-i4' AS bug, SUM(c_int4) FROM enc_pgfunc_test_det;

-- [BUG9253-02] SUM on OPE-only column — error should mention he-add required
SELECT 'Bug9253-sum-ope-i4' AS bug, SUM(c_int4) FROM enc_pgfunc_test_ope;

-- [BUG9253-03] SUM on he-add int2 — should succeed
SELECT 'Bug9253-sum-he-add-i2-ok' AS bug, SUM(c_int2) FROM enc_pgfunc_test_he;

-- [BUG9253-04] SUM on he-add int4 — should succeed
SELECT 'Bug9253-sum-he-add-i4-ok' AS bug, SUM(c_int4) FROM enc_pgfunc_test_he;

-- [BUG9253-05] SUM on he-add int8 — should succeed
SELECT 'Bug9253-sum-he-add-i8-ok' AS bug, SUM(c_int8) FROM enc_pgfunc_test_he;


-- ############################################################################
--                              清理
-- ############################################################################

DROP TABLE IF EXISTS enc_pgfunc_test_det CASCADE;
DROP TABLE IF EXISTS enc_pgfunc_test_ope CASCADE;
DROP TABLE IF EXISTS enc_pgfunc_test_he CASCADE;
DROP DEK IF EXISTS enc_pgfunc_dek;
DROP KEK IF EXISTS enc_pgfunc_kek;
DROP DEK IF EXISTS enc_pgfunc_he_mul_dek;
DROP DEK IF EXISTS enc_pgfunc_he_add_dek;
DROP KEK IF EXISTS enc_pgfunc_he_kek;
