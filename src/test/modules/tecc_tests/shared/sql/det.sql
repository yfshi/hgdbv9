-- ============================================================
-- det.sql — DET等值比较测试
-- 覆盖 test_algo_regression, test_all_types_where,
--   test_bug7906/7908/7911_repro, test_select_where,
--   test_where_literal, test_tecc_20cases/basic/combination,
--   test_bool_expr, test_taskB_encrypted
-- ============================================================

-- ============================================================
-- 0. Setup — 共享KEK/DEK
-- ============================================================
DROP KEK IF EXISTS kek_det CASCADE;

CREATE KEK kek_det WITH (
    key_store = 'localkms',
    key_path = '/tmp/kek_det.key'
);
CREATE DEK dek_det WITH VALUES(KEK = kek_det);

-- ============================================================
-- 1. All 9 types WHERE DET equality (test_all_types_where)
-- ============================================================
SELECT '=== 1 All 9 Types DET Equality ===';

DROP TABLE IF EXISTS test_all_types_det CASCADE;
CREATE TABLE test_all_types_det (
    id SERIAL PRIMARY KEY,
    c_int2    INT2_ENC    ENCRYPTED WITH (DEK = 'dek_det', support_algo = 'det'),
    c_int4    INT4_ENC    ENCRYPTED WITH (DEK = 'dek_det', support_algo = 'det'),
    c_int8    INT8_ENC    ENCRYPTED WITH (DEK = 'dek_det', support_algo = 'det'),
    c_float4  FLOAT4_ENC  ENCRYPTED WITH (DEK = 'dek_det', support_algo = 'det'),
    c_float8  FLOAT8_ENC  ENCRYPTED WITH (DEK = 'dek_det', support_algo = 'det'),
    c_numeric NUMERIC_ENC ENCRYPTED WITH (DEK = 'dek_det', support_algo = 'det'),
    c_varchar VARCHAR_ENC ENCRYPTED WITH (DEK = 'dek_det', support_algo = 'det'),
    c_bpchar  BPCHAR_ENC  ENCRYPTED WITH (DEK = 'dek_det', support_algo = 'det'),
    c_text    TEXT_ENC    ENCRYPTED WITH (DEK = 'dek_det', support_algo = 'det')
);

INSERT INTO test_all_types_det (c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric, c_varchar, c_bpchar, c_text) VALUES
(1, 10, 100, 1.5, 10.5, 100.5, 'short_str', 'bpchar1', 'text1'),
(2, 20, 200, 2.5, 20.5, 200.5, 'medium_string', 'bpchar2', 'text2'),
(3, 30, 300, 3.5, 30.5, 300.5, 'a_longer_string_value', 'bpchar3', 'text3'),
(99, 999, 9999, 99.9, 999.9, 9999.9, 'zzzzzzz', 'bpchar99', 'text99');

SELECT '1a_all_decrypt', * FROM test_all_types_det ORDER BY id;
SELECT '1b_int2_eq', id, c_int2 FROM test_all_types_det WHERE c_int2 = 2;
SELECT '1c_int4_eq', id, c_int4 FROM test_all_types_det WHERE c_int4 = 20;
SELECT '1d_int8_eq', id, c_int8 FROM test_all_types_det WHERE c_int8 = 200;
SELECT '1e_float4_eq', id, c_float4 FROM test_all_types_det WHERE c_float4 = 2.5;
SELECT '1f_float8_eq', id, c_float8 FROM test_all_types_det WHERE c_float8 = 20.5;
SELECT '1g_numeric_eq', id, c_numeric FROM test_all_types_det WHERE c_numeric = 200.5;
SELECT '1h_varchar_eq', id, c_varchar FROM test_all_types_det WHERE c_varchar = 'medium_string';
SELECT '1i_bpchar_eq', id, c_bpchar FROM test_all_types_det WHERE c_bpchar = 'bpchar2';
SELECT '1j_text_eq', id, c_text FROM test_all_types_det WHERE c_text = 'text2';

-- Composite conditions
SELECT '1k_int4_and_int8', id FROM test_all_types_det WHERE c_int4 = 30 AND c_int8 = 300;
SELECT '1l_int4_or_int4', id, c_int4 FROM test_all_types_det WHERE c_int4 = 10 OR c_int4 = 30 ORDER BY id;
SELECT '1m_cross_type_and', id FROM test_all_types_det WHERE c_int4 = 20 AND c_float8 = 20.5;
SELECT '1n_numeric_and_varchar', id FROM test_all_types_det WHERE c_numeric = 100.5 AND c_varchar = 'short_str';

-- Not equal
SELECT '1o_int4_ne', id, c_int4 FROM test_all_types_det WHERE c_int4 <> 20 ORDER BY id;

-- Mixed plain + encrypted
SELECT '1p_plain_and_enc', id, c_int8 FROM test_all_types_det WHERE id >= 2 AND c_int8 = 200;

-- No match
SELECT '1q_no_match', id FROM test_all_types_det WHERE c_int4 = 999;

-- Full row exact match
SELECT '1r_full_row_match', id FROM test_all_types_det WHERE c_int2 = 3 AND c_int4 = 30 AND c_int8 = 300
    AND c_float4 = 3.5 AND c_float8 = 30.5 AND c_numeric = 300.5
    AND c_varchar = 'a_longer_string_value' AND c_bpchar = 'bpchar3' AND c_text = 'text3';

SELECT '1s_float8_int_input', id, c_float8 FROM test_all_types_det WHERE c_float8 = 10.5;
SELECT '1t_text_last_row', id, c_text FROM test_all_types_det WHERE c_text = 'text99';

DROP TABLE IF EXISTS test_all_types_det CASCADE;

-- ============================================================
-- 2. Cross-type string equality (bug7908)
-- ============================================================
SELECT '=== 2 Cross-Type String Equality ===';

DROP TABLE IF EXISTS tecc_string_type_test CASCADE;
CREATE TABLE tecc_string_type_test (
    id SERIAL PRIMARY KEY,
    txt_col text_enc encrypted with (dek = dek_det, support_algo='det'),
    vc_col varchar_enc encrypted with (dek = dek_det, support_algo='det'),
    bp_col bpchar_enc encrypted with (dek = dek_det, support_algo='det')
);

INSERT INTO tecc_string_type_test (txt_col, vc_col, bp_col) VALUES
('hello', 'hello', 'hello'), ('hello', 'hello', 'hello '),
('world', 'world', 'world'), ('test', 'test', 'test'),
('Hello', 'Hello', 'Hello'), ('hello ', 'hello ', 'hello '),
(NULL, NULL, NULL), ('', '', '');

-- NULL cross-type comparison
SELECT '2a_null_cross', txt_col IS NULL AS text_null, txt_col = vc_col AS te_vc
FROM tecc_string_type_test WHERE txt_col IS NULL;

-- Empty string cross-type
SELECT '2b_empty_cross', txt_col = '' AS text_eq, txt_col = vc_col AS te_vc
FROM tecc_string_type_test WHERE txt_col = '';

-- IS DISTINCT FROM (cross-type)
SELECT '2c_distinct_cross', txt_col IS DISTINCT FROM vc_col AS te_va
FROM tecc_string_type_test;

-- bpchar equality with spaces (bug7906)
CREATE TABLE string_type_test (
    id SERIAL PRIMARY KEY,
    txt_col text, vc_col varchar, bp_col bpchar
);
INSERT INTO string_type_test (txt_col, vc_col, bp_col) VALUES
('hello', 'hello', 'hello'), ('hello', 'hello', 'hello '),
('world', 'world', 'world'), ('test', 'test', 'test'),
('Hello', 'Hello', 'Hello'), ('hello ', 'hello ', 'hello '),
(NULL, NULL, NULL), ('', '', '');

-- Compare bpchar equality: 'hello' vs 'hello ' (should both be true for bpchar)
SELECT '2d_bpchar_space_plain', bp_col = 'hello' AS eq_hello, bp_col = 'hello ' AS eq_hello_sp
FROM string_type_test WHERE bp_col = 'hello';

SELECT '2e_bpchar_space_enc', bp_col = 'hello' AS eq_hello, bp_col = 'hello ' AS eq_hello_sp
FROM tecc_string_type_test WHERE bp_col = 'hello';

DROP TABLE IF EXISTS tecc_string_type_test CASCADE;
DROP TABLE IF EXISTS string_type_test CASCADE;

-- ============================================================
-- 3. WHERE literal comparison (where_literal)
-- ============================================================
SELECT '=== 3 WHERE Literal ===';

DROP TABLE IF EXISTS test_where CASCADE;
CREATE TABLE test_where (
    id   INT4_ENC  ENCRYPTED WITH (DEK = dek_det, SUPPORT_ALGO = 'OPE'),
    name TEXT_ENC  ENCRYPTED WITH (DEK = dek_det, SUPPORT_ALGO = 'DET,SEARCH')
);

INSERT INTO test_where (id, name) VALUES (1, 'Alice'), (2, 'Bob'), (3, 'Charlie'), (4, 'David'), (5, 'Eve');

SELECT '3a_id_gt', * FROM test_where WHERE id > 2 ORDER BY id;
SELECT '3b_id_eq', * FROM test_where WHERE id = 3;
SELECT '3c_name_eq', * FROM test_where WHERE name = 'Charlie';

DROP TABLE IF EXISTS test_where CASCADE;

-- ============================================================
-- 4. Case sensitivity + range (bug7911)
-- ============================================================
SELECT '=== 4 Case Sensitivity ===';

DROP TABLE IF EXISTS tecc_text_case_test CASCADE;
CREATE TABLE tecc_text_case_test (
    id SERIAL PRIMARY KEY,
    text_col text_enc encrypted with (dek = dek_det, support_algo='det'),
    varchar_col varchar_enc encrypted with (dek = dek_det, support_algo='det'),
    bpchar_col bpchar_enc encrypted with (dek = dek_det, support_algo='det')
);

INSERT INTO tecc_text_case_test (text_col, varchar_col, bpchar_col) VALUES
('apple', 'apple', 'apple'), ('application', 'application', 'application'),
('banana', 'banana', 'banana'), ('berry', 'berry', 'berry'),
('cherry', 'cherry', 'cherry'), ('Apple', 'Apple', 'Apple'),
('APPLE', 'APPLE', 'APPLE'), ('123apple', '123apple', '123apple'),
('apple123', 'apple123', 'apple123'), ('a', 'a', 'a'),
('zoo', 'zoo', 'zoo'), ('', '', ''), (NULL, NULL, NULL);

-- Case-sensitive equality
SELECT '4a_eq_apple', id, text_col FROM tecc_text_case_test WHERE text_col = 'apple' ORDER BY text_col;
SELECT '4b_eq_Apple', id, text_col FROM tecc_text_case_test WHERE text_col = 'Apple' ORDER BY text_col;

DROP TABLE IF EXISTS tecc_text_case_test CASCADE;

-- ============================================================
-- 5. Combined DET/OPE/LIKE/ORDER BY (tecc_combination)
-- ============================================================
SELECT '=== 5 Combined DET/OPE/LIKE/ORDER BY ===';

DROP TABLE IF EXISTS tecc_test CASCADE;
CREATE TABLE tecc_test (
    id        INT4,
    name      VARCHAR(100),
    col_int   INT4_ENC    ENCRYPTED WITH (DEK = dek_det, SUPPORT_ALGO = 'OPE'),
    col_float FLOAT8_ENC  ENCRYPTED WITH (DEK = dek_det, SUPPORT_ALGO = 'OPE'),
    col_text  TEXT_ENC    ENCRYPTED WITH (DEK = dek_det, SUPPORT_ALGO = 'DET,SEARCH'),
    col_bpchar BPCHAR_ENC ENCRYPTED WITH (DEK = dek_det, SUPPORT_ALGO = 'DET,SEARCH')
);

INSERT INTO tecc_test VALUES
(1, 'Alice',   100, 100.5, 'Apple',     'AA'),
(2, 'Bob',     200, 200.5, 'Banana',    'BB'),
(3, 'Charlie', 300, 300.5, 'Cherry',    'CC'),
(4, 'David',   400, 400.5, 'Date',      'DD'),
(5, 'Eve',     500, 500.5, 'Elderberry','EE');

-- DET equality
SELECT '5a_int_eq', id, name, col_int FROM tecc_test WHERE col_int = 300 ORDER BY id;
SELECT '5b_float_eq', id, name, col_float FROM tecc_test WHERE col_float = 500.5 ORDER BY id;

-- OPE range
SELECT '5c_int_gt', id, name, col_int FROM tecc_test WHERE col_int > 300 ORDER BY id;
SELECT '5d_int_lt', id, name, col_int FROM tecc_test WHERE col_int < 300 ORDER BY id;
SELECT '5e_int_between', id, name, col_int FROM tecc_test WHERE col_int BETWEEN 300 AND 500 ORDER BY id;

-- SEARCH LIKE
SELECT '5f_like_ana', id, name, col_text FROM tecc_test WHERE col_text LIKE '%ana%' ORDER BY id;
SELECT '5g_like_erry', id, name, col_text FROM tecc_test WHERE col_text LIKE '%erry%' ORDER BY id;
SELECT '5h_like_e', id, name, col_text FROM tecc_test WHERE col_text LIKE '%e%' ORDER BY id;

-- DET equality + ORDER BY
SELECT '5i_eq_order_asc', id, name, col_int FROM tecc_test WHERE col_int = 500 ORDER BY id ASC;
SELECT '5j_eq_order_desc', id, name, col_int FROM tecc_test WHERE col_int = 300 ORDER BY id DESC;

-- Range + ORDER BY
SELECT '5k_range_order_asc', id, name, col_int FROM tecc_test WHERE col_int > 300 ORDER BY id ASC;
SELECT '5l_range_order_desc', id, name, col_int FROM tecc_test WHERE col_int > 300 ORDER BY id DESC;
SELECT '5m_range_order_name', id, name, col_int FROM tecc_test WHERE col_int <= 300 ORDER BY name ASC;

-- LIKE + ORDER BY
SELECT '5n_like_order_desc', id, name, col_text FROM tecc_test WHERE col_text LIKE '%e%' ORDER BY id DESC;

-- UPDATE + WHERE
UPDATE tecc_test SET name = 'BigNum' WHERE col_int > 400;
SELECT '5o_update_range', id, name, col_int FROM tecc_test ORDER BY id;
UPDATE tecc_test SET name = 'CherryLover' WHERE col_text LIKE '%erry%';
SELECT '5p_update_like', id, name, col_text FROM tecc_test ORDER BY id;

-- DELETE + WHERE
DELETE FROM tecc_test WHERE col_int <= 100;
SELECT '5q_delete_range', id, name, col_int FROM tecc_test ORDER BY id;

-- Full table
SELECT '5r_full', id, name, col_int, col_text FROM tecc_test ORDER BY id;

DROP TABLE tecc_test CASCADE;

-- ============================================================
-- Cleanup
-- ============================================================
SELECT '=== Cleanup ===';
DROP KEK IF EXISTS kek_det CASCADE;
