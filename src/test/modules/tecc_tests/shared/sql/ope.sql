-- ============================================================
-- ope.sql — OPE等值/范围/排序/COALESCE测试
-- 覆盖 test_algo_regression, test_ope_orderby,
--   test_ope_part1_int, test_ope_range_queries,
--   test_ope_text_orderby, test_ope_text_varlen,
--   test_ope_text_collate, test_ope_env_setup,
--   test_regression_ope_coalesce,
--   test_bug7856_7860_7824_ope_orderby,
--   test_bug7882/7909/7910/7912/7913/7914/7915_repro
-- ============================================================

-- ============================================================
-- 0. Setup — 共享KEK/DEK
-- ============================================================
DROP KEK IF EXISTS kek_ope CASCADE;

CREATE KEK kek_ope WITH (
    KEY_STORE = 'localkms',
    KEY_PATH = '/tmp/kek_ope.key'
);
CREATE DEK dek_ope WITH VALUES(KEK = kek_ope);

-- ============================================================
-- 1. Int2/Int4/Int8 范围查询
-- ============================================================
SELECT '=== 1 Int Range (int2/int4/int8) ===';

DROP TABLE IF EXISTS tecc_int_range_test CASCADE;
CREATE TABLE tecc_int_range_test (
    id SERIAL PRIMARY KEY,
    int2_col int2_enc encrypted with (dek = dek_ope, support_algo='ope'),
    int4_col int4_enc encrypted with (dek = dek_ope, support_algo='ope'),
    int8_col int8_enc encrypted with (dek = dek_ope, support_algo='ope')
);

INSERT INTO tecc_int_range_test (int2_col, int4_col, int8_col) VALUES
(10, 100, 1000), (20, 200, 2000), (30, 300, 3000),
(40, 400, 4000), (50, 500, 5000), (60, 600, 6000),
(70, 700, 7000), (80, 800, 8000), (90, 900, 9000),
(100, 1000, 10000);

-- 1a: int4 > < >= <= BETWEEN
SELECT '1a_int4_gt_500', id, int4_col FROM tecc_int_range_test WHERE int4_col > 500 ORDER BY id;
SELECT '1b_int4_lt_300', id, int4_col FROM tecc_int_range_test WHERE int4_col < 300 ORDER BY id;
SELECT '1c_int4_ge_500', id, int4_col FROM tecc_int_range_test WHERE int4_col >= 500 ORDER BY id;
SELECT '1d_int4_le_300', id, int4_col FROM tecc_int_range_test WHERE int4_col <= 300 ORDER BY id;
SELECT '1e_int4_between', id, int4_col FROM tecc_int_range_test WHERE int4_col BETWEEN 300 AND 700 ORDER BY id;

-- 1b: int2 + int8 range
SELECT '1f_int2_between', id, int2_col FROM tecc_int_range_test WHERE int2_col BETWEEN 30 AND 70 ORDER BY id;
SELECT '1g_int8_gt_5000', id, int8_col FROM tecc_int_range_test WHERE int8_col > 5000 ORDER BY id;

-- 1c: int4 equality (DET fallback)
SELECT '1h_int4_eq_500', id, int4_col FROM tecc_int_range_test WHERE int4_col = 500;

-- 1d: int8 small/large comparisons (bug7912/7914)
DROP TABLE IF EXISTS tecc_int_combo_test CASCADE;
CREATE TABLE tecc_int_combo_test (
    id SERIAL PRIMARY KEY,
    int2_col int2_enc encrypted with (dek = dek_ope, support_algo='ope'),
    int4_col int4_enc encrypted with (dek = dek_ope, support_algo='ope'),
    int8_col int8_enc encrypted with (dek = dek_ope, support_algo='ope')
);

INSERT INTO tecc_int_combo_test (int2_col, int4_col, int8_col) VALUES
(100, 100, 100), (0, 0, 0), (-100, -100, -100),
(32767, 32767, 32767), (-32768, -32768, -32768),
(32767, 32767, 9223372036854775807),  -- INT8_MAX
(-32768, -2147483648, -9223372036854775808),  -- INT8_MIN
(100, 200, 300), (1, 1, 2), (-5, -5, -10),
(NULL, NULL, NULL), (100, NULL, 100), (NULL, 200, NULL), (50, 50, NULL);

SELECT '1i_int8_lt_30', id, int8_col FROM tecc_int_combo_test WHERE int8_col < 30 ORDER BY int8_col;
SELECT '1j_int8_gt_0', id, int8_col FROM tecc_int_combo_test WHERE int8_col > 0 ORDER BY int8_col;
SELECT '1k_int8_eq_100', id, int8_col FROM tecc_int_combo_test WHERE int8_col = 100 ORDER BY int8_col;
SELECT '1l_int4_lt_30', id, int4_col FROM tecc_int_combo_test WHERE int4_col < 30 ORDER BY int4_col;
SELECT '1m_int2_lt_30', id, int2_col FROM tecc_int_combo_test WHERE int2_col < 30 ORDER BY int2_col;

-- int8 boundary (bug7914)
SELECT '1n_int8_gt_max_bound', id, int8_col FROM tecc_int_combo_test WHERE int8_col > 9223372036854775800 AND int8_col <= 9223372036854775807 ORDER BY int8_col;
SELECT '1o_int8_eq_max', id, int8_col FROM tecc_int_combo_test WHERE int8_col = 9223372036854775807 ORDER BY int8_col;
SELECT '1p_int8_ge_max', id, int8_col FROM tecc_int_combo_test WHERE int8_col >= 9223372036854775807 ORDER BY int8_col;

-- Combined OR/AND (bug7913)
SELECT '1q_combo_or_and', id, int2_col, int4_col, int8_col FROM tecc_int_combo_test
WHERE (int2_col < 30 OR int4_col > 40) AND int8_col >= 0 ORDER BY int2_col, int4_col;

DROP TABLE IF EXISTS tecc_int_range_test CASCADE;
DROP TABLE IF EXISTS tecc_int_combo_test CASCADE;

-- ============================================================
-- 2. Float4/Float8/Numeric 范围查询
-- ============================================================
SELECT '=== 2 Float/Numeric Range ===';

DROP TABLE IF EXISTS tecc_float_range_test CASCADE;
CREATE TABLE tecc_float_range_test (
    id SERIAL PRIMARY KEY,
    numeric_col numeric_enc encrypted with (dek = dek_ope, support_algo='ope'),
    float4_col float4_enc encrypted with (dek = dek_ope, support_algo='ope'),
    float8_col float8_enc encrypted with (dek = dek_ope, support_algo='ope')
);

INSERT INTO tecc_float_range_test (numeric_col, float4_col, float8_col) VALUES
(10.5, 10.5, 10.5), (20.25, 20.25, 20.25), (30.75, 30.75, 30.75),
(40.125, 40.125, 40.125), (50.333, 50.333, 50.333), (-10.5, -10.5, -10.5),
(100.99, 100.99, 100.99), (0.0, 0.0, 0.0), (0.0000001, 0.0000001, 0.0000001),
(999999.999999, 999999.999999, 999999.999999),
(-999999.999999, -999999.999999, -999999.999999),
(3.141592653589793, 3.141592653589793, 3.141592653589793),
(2.718281828459045, 2.718281828459045, 2.718281828459045),
(NULL, NULL, NULL);

SELECT '2a_numeric_gt_10_lt_50', id, numeric_col FROM tecc_float_range_test WHERE numeric_col > 10.0 AND numeric_col < 50.0 ORDER BY numeric_col;
SELECT '2b_float4_ge_20_le_40', id, float4_col FROM tecc_float_range_test WHERE float4_col >= 20.0 AND float4_col <= 40.0 ORDER BY float4_col;
SELECT '2c_float8_gt_30p5_lt_50', id, float8_col FROM tecc_float_range_test WHERE float8_col > 30.5 AND float8_col < 50.0 ORDER BY float8_col;
SELECT '2d_numeric_gt_30', id, numeric_col FROM tecc_float_range_test WHERE numeric_col > 30.0 ORDER BY numeric_col;
SELECT '2e_float4_ge_40', id, float4_col FROM tecc_float_range_test WHERE float4_col >= 40.0 ORDER BY float4_col;
SELECT '2f_float8_lt_30', id, float8_col FROM tecc_float_range_test WHERE float8_col < 30.0 ORDER BY float8_col;
SELECT '2g_numeric_le_20', id, numeric_col FROM tecc_float_range_test WHERE numeric_col <= 20.0 ORDER BY numeric_col;
SELECT '2h_float4_between', id, float4_col FROM tecc_float_range_test WHERE float4_col BETWEEN 3.0 AND 8.0 ORDER BY id;
SELECT '2i_float8_ge_5', id, float8_col FROM tecc_float_range_test WHERE float8_col >= 5.0 ORDER BY id;
SELECT '2j_numeric_between', id, numeric_col FROM tecc_float_range_test WHERE numeric_col BETWEEN 200 AND 800 ORDER BY id;
SELECT '2k_float8_between_precise', id, float8_col FROM tecc_float_range_test WHERE float8_col BETWEEN 2.345678 AND 7.890234 ORDER BY id;

DROP TABLE IF EXISTS tecc_float_range_test CASCADE;

-- ============================================================
-- 3. Text 范围查询 (bug7910)
-- ============================================================
SELECT '=== 3 Text Range ===';

DROP TABLE IF EXISTS tecc_text_type_test CASCADE;
CREATE TABLE tecc_text_type_test (
  id SERIAL PRIMARY KEY,
  text_col text_enc encrypted with (dek = dek_ope, support_algo='ope'),
  varchar_col varchar_enc encrypted with (dek = dek_ope, support_algo='ope'),
  bpchar_col bpchar_enc encrypted with (dek = dek_ope, support_algo='ope')
);

INSERT INTO tecc_text_type_test (text_col, varchar_col, bpchar_col) VALUES
('apple', 'apple', 'apple'), ('application', 'application', 'application'),
('banana', 'banana', 'banana'), ('berry', 'berry', 'berry'),
('cherry', 'cherry', 'cherry'), ('Apple', 'Apple', 'Apple'),
('APPLE', 'APPLE', 'APPLE'), ('123apple', '123apple', '123apple'),
('apple123', 'apple123', 'apple123'), ('a', 'a', 'a'),
('zoo', 'zoo', 'zoo'), ('', '', ''), (NULL, NULL, NULL);

SELECT '3a_text_gt_apple_lt_cherry', id, text_col FROM tecc_text_type_test WHERE text_col > 'apple' AND text_col < 'cherry' ORDER BY text_col;
SELECT '3b_varchar_ge_apple_le_cherry', id, varchar_col FROM tecc_text_type_test WHERE varchar_col >= 'apple' AND varchar_col <= 'cherry' ORDER BY varchar_col;
SELECT '3c_bpchar_gt_banana_lt_cherry', id, bpchar_col FROM tecc_text_type_test WHERE bpchar_col > 'banana' AND bpchar_col < 'cherry' ORDER BY bpchar_col;
SELECT '3d_gt_a_lt_b_or_gt_banana_lt_cherry', id, text_col FROM tecc_text_type_test WHERE (text_col > 'a' AND text_col < 'b') OR (text_col > 'banana' AND text_col < 'cherry') ORDER BY text_col;
SELECT '3e_varchar_gt_apple_and_lt_berry_or_gt_cherry', id, varchar_col FROM tecc_text_type_test WHERE varchar_col > 'apple' AND (varchar_col < 'berry' OR varchar_col > 'cherry') ORDER BY varchar_col;
SELECT '3f_text_gt_empty_lt_c', id, text_col FROM tecc_text_type_test WHERE text_col > '' AND text_col < 'c' ORDER BY text_col;
SELECT '3g_bpchar_range_ne', id, bpchar_col FROM tecc_text_type_test WHERE bpchar_col >= 'apple' AND bpchar_col <= 'cherry' AND bpchar_col != 'banana' ORDER BY bpchar_col;

DROP TABLE IF EXISTS tecc_text_type_test CASCADE;

-- ============================================================
-- 4. ORDER BY numeric (int4/float4/float8/numeric)
-- ============================================================
SELECT '=== 4 ORDER BY Numeric ===';

DROP TABLE IF EXISTS tecc_ope_ob_test CASCADE;
CREATE TABLE tecc_ope_ob_test (
    id      SERIAL PRIMARY KEY,
    ope_i4  int4_enc     ENCRYPTED WITH (dek = dek_ope, SUPPORT_ALGO = 'ope'),
    ope_f4  float4_enc   ENCRYPTED WITH (dek = dek_ope, SUPPORT_ALGO = 'ope'),
    ope_f8  float8_enc   ENCRYPTED WITH (dek = dek_ope, SUPPORT_ALGO = 'ope'),
    ope_num numeric_enc  ENCRYPTED WITH (dek = dek_ope, SUPPORT_ALGO = 'ope'),
    tag     text
);

INSERT INTO tecc_ope_ob_test (ope_i4, ope_f4, ope_f8, ope_num, tag) VALUES
(30, 5.0,  5.0,  5.0,  'r3_5'),    (10, 1.0,  1.0,  1.0,  'r1_1'),
(50, 10.0, 10.0, 10.0, 'r5_10'),   (20, 2.5,  2.5,  2.5,  'r2_2.5'),
(40, 7.5,  7.5,  7.5,  'r4_7.5'),  (15, 1.5,  1.5,  1.5,  'r1.5_1.5');

SELECT '4a_int4_asc', tag, ope_i4 FROM tecc_ope_ob_test ORDER BY ope_i4 ASC;
SELECT '4b_int4_desc', tag, ope_i4 FROM tecc_ope_ob_test ORDER BY ope_i4 DESC;
SELECT '4c_float4_asc', tag, ope_f4 FROM tecc_ope_ob_test ORDER BY ope_f4 ASC;
SELECT '4d_float4_desc', tag, ope_f4 FROM tecc_ope_ob_test ORDER BY ope_f4 DESC;
SELECT '4e_float8_asc', tag, ope_f8 FROM tecc_ope_ob_test ORDER BY ope_f8 ASC;
SELECT '4f_float8_desc', tag, ope_f8 FROM tecc_ope_ob_test ORDER BY ope_f8 DESC;
SELECT '4g_numeric_asc', tag, ope_num FROM tecc_ope_ob_test ORDER BY ope_num ASC;
SELECT '4h_numeric_desc', tag, ope_num FROM tecc_ope_ob_test ORDER BY ope_num DESC;
SELECT '4i_multi_col_order', tag, ope_i4, ope_f8 FROM tecc_ope_ob_test ORDER BY ope_i4 ASC, ope_f8 ASC;

DROP TABLE IF EXISTS tecc_ope_ob_test CASCADE;

-- ============================================================
-- 5. ORDER BY text (varchar/bpchar/text)
-- ============================================================
SELECT '=== 5 ORDER BY Text ===';

DROP TABLE IF EXISTS tecc_ope_text_test CASCADE;
CREATE TABLE tecc_ope_text_test (
    id        SERIAL PRIMARY KEY,
    ope_vc    varchar_enc(200)  ENCRYPTED WITH (dek = dek_ope, SUPPORT_ALGO = 'ope'),
    ope_bp    bpchar_enc(200)   ENCRYPTED WITH (dek = dek_ope, SUPPORT_ALGO = 'ope'),
    ope_txt   text_enc          ENCRYPTED WITH (dek = dek_ope, SUPPORT_ALGO = 'ope'),
    tag       text
);

INSERT INTO tecc_ope_text_test (ope_vc, ope_bp, ope_txt, tag) VALUES
('hello', 'hello', 'hello', 'r1_hello'), ('a', 'a', 'a', 'r2_a'),
('world', 'world', 'world', 'r3_world'), ('abcde', 'abcde', 'abcde', 'r4_abcde'),
('test', 'test', 'test', 'r5_test'), ('z', 'z', 'z', 'r6_z');

SELECT '5a_varchar_asc', tag, ope_vc FROM tecc_ope_text_test ORDER BY ope_vc ASC;
SELECT '5b_varchar_desc', tag, ope_vc FROM tecc_ope_text_test ORDER BY ope_vc DESC;
SELECT '5c_bpchar_asc', tag, ope_bp FROM tecc_ope_text_test ORDER BY ope_bp ASC;
SELECT '5d_bpchar_desc', tag, ope_bp FROM tecc_ope_text_test ORDER BY ope_bp DESC;
SELECT '5e_text_asc', tag, ope_txt FROM tecc_ope_text_test ORDER BY ope_txt ASC;
SELECT '5f_text_desc', tag, ope_txt FROM tecc_ope_text_test ORDER BY ope_txt DESC;
SELECT '5g_multi_col', tag, ope_vc, ope_txt FROM tecc_ope_text_test ORDER BY ope_vc ASC, ope_txt ASC;

DROP TABLE IF EXISTS tecc_ope_text_test CASCADE;

-- ============================================================
-- 6. ORDER BY extreme varlen
-- ============================================================
SELECT '=== 6 ORDER BY Extreme Varlen ===';

DROP TABLE IF EXISTS tecc_ope_varlen_test CASCADE;
CREATE TABLE tecc_ope_varlen_test (
    id        SERIAL PRIMARY KEY,
    ope_vc    varchar_enc(500)  ENCRYPTED WITH (dek = dek_ope, SUPPORT_ALGO = 'ope'),
    tag       text
);

INSERT INTO tecc_ope_varlen_test (ope_vc, tag) VALUES
('a', 's1_a'), ('aa', 's2_aa'), ('aaa', 's3_aaa'),
('z', 's4_z'), ('zz', 's5_zz'),
('hello world this is a long', 's6_long'),
('abcd', 's7_abcd'), ('test test test test test', 's8_xtralong');

SELECT '6a_cipher_len', tag, length(ope_vc::text) AS cipher_len FROM tecc_ope_varlen_test ORDER BY tag;
SELECT '6b_varchar_asc', tag, ope_vc FROM tecc_ope_varlen_test ORDER BY ope_vc ASC;
SELECT '6c_varchar_desc', tag, ope_vc FROM tecc_ope_varlen_test ORDER BY ope_vc DESC;

DROP TABLE IF EXISTS tecc_ope_varlen_test CASCADE;

-- ============================================================
-- 7. ORDER BY with COLLATE (bug7856/7860)
-- ============================================================
SELECT '=== 7 ORDER BY with COLLATE ===';

DROP TABLE IF EXISTS tb_7856_enc CASCADE;
CREATE TABLE tb_7856_enc (
    id SERIAL PRIMARY KEY,
    text_col text_enc encrypted with (dek = dek_ope, support_algo = 'ope') collate "C",
    varchar_col varchar_enc encrypted with (dek = dek_ope, support_algo = 'ope') collate "C",
    bpchar_col bpchar_enc encrypted with (dek = dek_ope, support_algo = 'ope') collate "C"
);

INSERT INTO tb_7856_enc (text_col, varchar_col, bpchar_col) VALUES
('apple', 'apple', 'apple'), ('Banana', 'Banana', 'Banana'),
('123abc', '123abc', '123abc'), ('ABC', 'ABC', 'ABC'),
('orange', 'orange', 'orange'), ('测试', '测试', '测试'),
('', '', ''), ('a', 'a', 'a'), ('Z', 'Z', 'Z'),
(NULL, NULL, NULL), ('berry', 'berry', 'berry'),
('0start', '0start', '0start'), ('9end', '9end', '9end');

SELECT '7a_collateC_text_asc', id, text_col FROM tb_7856_enc ORDER BY text_col;
SELECT '7b_collateC_varchar_asc', id, varchar_col FROM tb_7856_enc ORDER BY varchar_col;
SELECT '7c_collateC_bpchar_asc', id, bpchar_col FROM tb_7856_enc ORDER BY bpchar_col;

-- Integer ORDER BY with NULLS FIRST (bug7824)
DROP TABLE IF EXISTS tb_7824_enc CASCADE;
CREATE TABLE tb_7824_enc (
    id SERIAL PRIMARY KEY,
    smallint_col int2_enc encrypted with (dek = dek_ope, support_algo = 'ope'),
    int_col int4_enc encrypted with (dek = dek_ope, support_algo = 'ope'),
    bigint_col int8_enc encrypted with (dek = dek_ope, support_algo = 'ope')
);

INSERT INTO tb_7824_enc (smallint_col, int_col, bigint_col) VALUES
(0, 0, 0), (1, 1, 1), (-1, -1, -1),
(32767, 32767, 32767), (-32768, -32768, -32768),
(32766, 32766, 32766), (-32767, -32767, -32767),
(100, 100, 100), (-100, -100, -100), (NULL, NULL, NULL);

SELECT '7d_int2_asc_nullsfirst', id, smallint_col FROM tb_7824_enc ORDER BY smallint_col NULLS FIRST;
SELECT '7e_int4_asc', id, int_col FROM tb_7824_enc ORDER BY int_col;
SELECT '7f_int8_asc', id, bigint_col FROM tb_7824_enc ORDER BY bigint_col;

DROP TABLE IF EXISTS tb_7856_enc CASCADE;
DROP TABLE IF EXISTS tb_7824_enc CASCADE;

-- ============================================================
-- 8. COALESCE / Expression (regression_ope_coalesce)
-- ============================================================
SELECT '=== 8 COALESCE / Expression ===';

DROP TABLE IF EXISTS test_reg_emp CASCADE;
CREATE TABLE test_reg_emp (
    id INT4,
    name VARCHAR(100),
    salary_det INT4_ENC ENCRYPTED WITH (DEK = dek_ope, SUPPORT_ALGO = 'DET'),
    salary_ope INT4_ENC ENCRYPTED WITH (DEK = dek_ope, SUPPORT_ALGO = 'OPE'),
    bonus INT4_ENC ENCRYPTED WITH (DEK = dek_ope, SUPPORT_ALGO = 'DET'),
    dept VARCHAR(50)
);

INSERT INTO test_reg_emp VALUES
(1, 'Alice', 50000, 50000, 5000, 'IT'),
(2, 'Bob', NULL, NULL, 3000, 'HR'),
(3, 'Charlie', 80000, 80000, 8000, 'IT'),
(4, 'Diana', NULL, NULL, NULL, 'HR'),
(5, 'Eve', 0, 0, 0, 'FIN');

-- DET COALESCE equality
SELECT '8a_coalesce_det_eq', id, name FROM test_reg_emp WHERE COALESCE(salary_det, 0) = 0;
SELECT '8b_coalesce_det_ne', id, name FROM test_reg_emp WHERE COALESCE(salary_det, 0) <> 0;
SELECT '8c_nullif_det', id, name FROM test_reg_emp WHERE NULLIF(salary_det, 50000) IS NULL;

-- DET with GREATEST/LEAST (should error — DET not supported)
SELECT '8d_greatest_det_err', id, name FROM test_reg_emp WHERE GREATEST(salary_det, 55000) = 55000 AND salary_det IS NOT NULL;
SELECT '8e_least_det_err', id, name FROM test_reg_emp WHERE LEAST(salary_det, 65000) = 50000;

-- OPE COALESCE equality + range
SELECT '8f_coalesce_ope_eq', id, name FROM test_reg_emp WHERE COALESCE(salary_ope, 0) = 0;
SELECT '8g_coalesce_ope_gt', id, name FROM test_reg_emp WHERE COALESCE(salary_ope, 0) > 10000;
SELECT '8h_coalesce_ope_lt', id, name FROM test_reg_emp WHERE COALESCE(salary_ope, 999999) < 60000;
SELECT '8i_coalesce_ope_ge', id, name FROM test_reg_emp WHERE COALESCE(salary_ope, 0) >= 50000;
SELECT '8j_coalesce_ope_le', id, name FROM test_reg_emp WHERE COALESCE(salary_ope, 0) <= 0;
SELECT '8k_nullif_ope_gt', id, name FROM test_reg_emp WHERE NULLIF(salary_ope, 50000) > 60000;
SELECT '8l_greatest_ope_eq', id, name FROM test_reg_emp WHERE GREATEST(salary_ope, 55000) = 80000;
SELECT '8m_least_ope_eq', id, name FROM test_reg_emp WHERE LEAST(salary_ope, 65000) = 50000;

-- Combined
SELECT '8n_mixed_cond', id, name FROM test_reg_emp WHERE COALESCE(salary_ope, 0) > 10000 AND dept = 'IT';
SELECT '8o_is_not_null', id, name FROM test_reg_emp WHERE salary_ope IS NOT NULL AND COALESCE(bonus, 0) = 5000;
SELECT '8p_orderby_coalesce', id, name, COALESCE(salary_ope, 0) AS s FROM test_reg_emp ORDER BY s DESC;

-- Multi-arg / multi-col
SELECT '8q_coalesce3', id, name FROM test_reg_emp WHERE COALESCE(salary_det, bonus, -1) = 3000;
SELECT '8r_coalesce_self_eq', id, name FROM test_reg_emp WHERE COALESCE(salary_ope, 0) = COALESCE(salary_ope, 0);
SELECT '8s_coalesce_multi_or', id, name FROM test_reg_emp WHERE COALESCE(salary_ope, 0) = 0 OR COALESCE(salary_ope, 0) > 70000;
SELECT '8t_coalesce_neg', id, name FROM test_reg_emp WHERE COALESCE(salary_ope, -100) = -100;
SELECT '8u_coalesce_neg_gt', id, name FROM test_reg_emp WHERE COALESCE(salary_ope, -99999) > 0;

DROP TABLE IF EXISTS test_reg_emp CASCADE;

-- ============================================================
-- 9. Bug regressions
-- ============================================================
SELECT '=== 9 Bug Regressions ===';

-- 9a: Bug 7882 — OPE eq on text returns empty
SELECT '9a_bug7882';
DROP TABLE IF EXISTS test_ope_eq CASCADE;
CREATE TABLE test_ope_eq(
    id SERIAL PRIMARY KEY,
    text_col text_enc encrypted with (dek = dek_ope, support_algo='ope'),
    category VARCHAR(50)
);
INSERT INTO test_ope_eq (text_col, category) VALUES ('这是测试文本数据', 'Category_A');
INSERT INTO test_ope_eq (text_col, category) VALUES ('hello world', 'Category_B');
SELECT * FROM test_ope_eq WHERE text_col = '这是测试文本数据';
DROP TABLE IF EXISTS test_ope_eq CASCADE;

-- 9b: Bug 7909 — int2_enc OPE negative decode
SELECT '9b_bug7909_int2_neg';
DROP TABLE IF EXISTS tb_int2_enc CASCADE;
CREATE TABLE tb_int2_enc (
    id SERIAL PRIMARY KEY,
    smallint_col int2_enc encrypted with (dek = dek_ope, support_algo='ope'),
    description TEXT
);
INSERT INTO tb_int2_enc (smallint_col, description) VALUES
(0, '零值'), (1, '正值'), (-1, '负值'),
(32767, '最大值'), (-32768, '最小值'),
(32766, '接近最大值'), (-32767, '接近最小值'),
(100, '中等正值'), (-100, '中等负值'),
(2, '边界值2-1'), (-2, '边界值-2-1'),
(127, '边界值127'), (-128, '边界值-128'),
(255, '边界值255'), (-256, '边界值-256');
SELECT * FROM tb_int2_enc ORDER BY id;
DROP TABLE IF EXISTS tb_int2_enc CASCADE;

-- 9c: Algorithm regression — DET/OPE combined
SELECT '9c_algo_regression';
DROP TABLE IF EXISTS test_algo CASCADE;
CREATE TABLE test_algo (
    id    SERIAL,
    c_det INT4_ENC ENCRYPTED WITH (DEK = dek_ope, SUPPORT_ALGO = 'DET'),
    c_ope INT4_ENC ENCRYPTED WITH (DEK = dek_ope, SUPPORT_ALGO = 'OPE'),
    s_det TEXT_ENC ENCRYPTED WITH (DEK = dek_ope, SUPPORT_ALGO = 'DET'),
    s_ope TEXT_ENC ENCRYPTED WITH (DEK = dek_ope, SUPPORT_ALGO = 'OPE,SEARCH')
);
INSERT INTO test_algo (c_det, c_ope, s_det, s_ope) VALUES
(10, 100, 'AA', 'Alpha'), (20, 200, 'BB', 'Beta'), (30, 300, 'CC', 'Gamma');

-- DET/OPE eq
SELECT '9c1_det_eq', c_det, c_ope FROM test_algo WHERE c_det = 10;
SELECT '9c2_ope_eq', c_det, c_ope FROM test_algo WHERE c_ope = 200;
SELECT '9c3_det_text_eq', c_det, c_ope FROM test_algo WHERE s_det = 'BB';
SELECT '9c4_ope_text_eq', c_det, c_ope FROM test_algo WHERE s_ope = 'Gamma';

-- OPE range
SELECT '9c5_ope_gt', c_ope FROM test_algo WHERE c_ope > 100 ORDER BY c_ope;
SELECT '9c6_ope_ge', c_ope FROM test_algo WHERE c_ope >= 200 ORDER BY c_ope;
SELECT '9c7_ope_lt', c_ope FROM test_algo WHERE c_ope < 300 ORDER BY c_ope;
SELECT '9c8_ope_le', c_ope FROM test_algo WHERE c_ope <= 200 ORDER BY c_ope;
SELECT '9c9_ope_text_range', s_ope FROM test_algo WHERE s_ope >= 'Beta' ORDER BY s_ope;

-- DET range should error
SELECT '9c10_det_range_err', c_det FROM test_algo WHERE c_det > 15;

-- UPDATE/DELETE with OPE
UPDATE test_algo SET c_ope = 250 WHERE c_ope = 200;
UPDATE test_algo SET s_ope = 'Beta2' WHERE s_ope = 'Beta';
SELECT '9c11_update_eq', c_ope, s_ope FROM test_algo ORDER BY id;
UPDATE test_algo SET c_ope = 250 WHERE c_ope > 150;
SELECT '9c12_update_range', c_ope FROM test_algo ORDER BY id;

INSERT INTO test_algo (c_det, c_ope, s_det, s_ope) VALUES (99, 999, 'ZZ', 'Zeta');
DELETE FROM test_algo WHERE c_det = 99;
DELETE FROM test_algo WHERE c_ope = 999;
SELECT '9c13_delete_after', count(*) FROM test_algo;

-- DELETE with OPE range
INSERT INTO test_algo (c_det, c_ope, s_det, s_ope) VALUES (40, 400, 'DD', 'Delta');
INSERT INTO test_algo (c_det, c_ope, s_det, s_ope) VALUES (50, 500, 'EE', 'Epsilon');
DELETE FROM test_algo WHERE c_ope > 350;
DELETE FROM test_algo WHERE s_ope > 'D';
SELECT '9c14_delete_range', count(*) FROM test_algo;

DROP TABLE IF EXISTS test_algo CASCADE;

-- ============================================================
-- 10. ORDER BY collation 对比验证
--     建表和 3 区相同的表，验证加密列在各排序规则下 ORDER BY
--     与明文结果一致（C collation vs default zh_CN.UTF-8）
-- ============================================================
SELECT '=== 10 Collation ORDER BY ===';

DROP TABLE IF EXISTS tecc_collation_verify CASCADE;
CREATE TABLE tecc_collation_verify (
  id SERIAL PRIMARY KEY,
  text_col text_enc encrypted with (dek = dek_ope, support_algo='ope'),
  varchar_col varchar_enc encrypted with (dek = dek_ope, support_algo='ope'),
  bpchar_col bpchar_enc encrypted with (dek = dek_ope, support_algo='ope')
);

INSERT INTO tecc_collation_verify (text_col, varchar_col, bpchar_col) VALUES
('apple', 'apple', 'apple'), ('application', 'application', 'application'),
('banana', 'banana', 'banana'), ('berry', 'berry', 'berry'),
('cherry', 'cherry', 'cherry'), ('Apple', 'Apple', 'Apple'),
('APPLE', 'APPLE', 'APPLE'), ('123apple', '123apple', '123apple'),
('apple123', 'apple123', 'apple123'), ('a', 'a', 'a'),
('zoo', 'zoo', 'zoo'), ('', '', ''), (NULL, NULL, NULL);

-- 10a-c: C collation ORDER BY
SELECT '10a_collateC_text', id, text_col FROM tecc_collation_verify ORDER BY text_col COLLATE "C";
SELECT '10b_collateC_varchar', id, varchar_col FROM tecc_collation_verify ORDER BY varchar_col COLLATE "C";
SELECT '10c_collateC_bpchar', id, bpchar_col FROM tecc_collation_verify ORDER BY bpchar_col COLLATE "C";

-- 10d-f: default (zh_CN.UTF-8) collation ORDER BY
SELECT '10d_default_text', id, text_col FROM tecc_collation_verify ORDER BY text_col;
SELECT '10e_default_varchar', id, varchar_col FROM tecc_collation_verify ORDER BY varchar_col;
SELECT '10f_default_bpchar', id, bpchar_col FROM tecc_collation_verify ORDER BY bpchar_col;

DROP TABLE IF EXISTS tecc_collation_verify CASCADE;

-- ============================================================
-- 11. 跨数值类型 OPE 排序验证
--     验证 int2/int4/int8/float4/float8/numeric 的 ORDER BY
--     和范围查询结果正确
-- ============================================================
SELECT '=== 11 Numeric OPE ORDER BY ===';

DROP TABLE IF EXISTS tecc_num_ope_verify CASCADE;
CREATE TABLE tecc_num_ope_verify (
    id SERIAL,
    i2 int2_enc encrypted with (dek = dek_ope, support_algo='ope'),
    i4 int4_enc encrypted with (dek = dek_ope, support_algo='ope'),
    i8 int8_enc encrypted with (dek = dek_ope, support_algo='ope'),
    f4 float4_enc encrypted with (dek = dek_ope, support_algo='ope'),
    f8 float8_enc encrypted with (dek = dek_ope, support_algo='ope'),
    num numeric_enc encrypted with (dek = dek_ope, support_algo='ope'),
    tag text
);

INSERT INTO tecc_num_ope_verify (i2, i4, i8, f4, f8, num, tag) VALUES
(10, 100, 1000, 1.5, 1.5, 1.5, 'r1_mid'),
(5, 50, 500, 0.5, 0.5, 0.5, 'r2_small'),
(20, 200, 2000, 10.0, 10.0, 10.0, 'r3_large'),
(-5, -50, -500, -0.5, -0.5, -0.5, 'r4_neg'),
(NULL, NULL, NULL, NULL, NULL, NULL, 'r5_null');

SELECT '11a_int2_asc', id, tag, i2 FROM tecc_num_ope_verify ORDER BY i2;
SELECT '11b_int4_asc', id, tag, i4 FROM tecc_num_ope_verify ORDER BY i4;
SELECT '11c_int8_asc', id, tag, i8 FROM tecc_num_ope_verify ORDER BY i8;
SELECT '11d_float4_asc', id, tag, f4 FROM tecc_num_ope_verify ORDER BY f4;
SELECT '11e_float8_asc', id, tag, f8 FROM tecc_num_ope_verify ORDER BY f8;
SELECT '11f_numeric_asc', id, tag, num FROM tecc_num_ope_verify ORDER BY num;
SELECT '11g_int4_gt_0', id, tag, i4 FROM tecc_num_ope_verify WHERE i4 > 0 ORDER BY i4;
SELECT '11h_int4_lt_0', id, tag, i4 FROM tecc_num_ope_verify WHERE i4 < 0 ORDER BY i4;
SELECT '11i_float8_between', id, tag, f8 FROM tecc_num_ope_verify WHERE f8 BETWEEN 1.0 AND 20.0 ORDER BY f8;
SELECT '11j_numeric_between', id, tag, num FROM tecc_num_ope_verify WHERE num BETWEEN 0 AND 100 ORDER BY num;
SELECT '11k_null_order', id, tag, i4 FROM tecc_num_ope_verify ORDER BY i4 NULLS FIRST;

DROP TABLE IF EXISTS tecc_num_ope_verify CASCADE;

-- ============================================================
-- Cleanup
-- ============================================================
SELECT '=== Cleanup ===';
DROP KEK IF EXISTS kek_ope CASCADE;
