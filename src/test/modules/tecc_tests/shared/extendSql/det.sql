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

-- @param int 1
-- @param int 10
-- @param int 100
-- @param BigDecimal 1.5
-- @param BigDecimal 10.5
-- @param BigDecimal 100.5
-- @param String short_str
-- @param String bpchar1
-- @param String text1
-- @param int 2
-- @param int 20
-- @param int 200
-- @param BigDecimal 2.5
-- @param BigDecimal 20.5
-- @param BigDecimal 200.5
-- @param String medium_string
-- @param String bpchar2
-- @param String text2
-- @param int 3
-- @param int 30
-- @param int 300
-- @param BigDecimal 3.5
-- @param BigDecimal 30.5
-- @param BigDecimal 300.5
-- @param String a_longer_string_value
-- @param String bpchar3
-- @param String text3
-- @param int 99
-- @param int 999
-- @param int 9999
-- @param BigDecimal 99.9
-- @param BigDecimal 999.9
-- @param BigDecimal 9999.9
-- @param String zzzzzzz
-- @param String bpchar99
-- @param String text99
INSERT INTO test_all_types_det (c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric, c_varchar, c_bpchar, c_text) VALUES
(?, ?, ?, ?, ?, ?, ?, ?, ?),
(?, ?, ?, ?, ?, ?, ?, ?, ?),
(?, ?, ?, ?, ?, ?, ?, ?, ?),
(?, ?, ?, ?, ?, ?, ?, ?, ?);

SELECT '1a_all_decrypt', * FROM test_all_types_det ORDER BY id;

-- @param int 2
SELECT '1b_int2_eq', id, c_int2 FROM test_all_types_det WHERE c_int2 = ?;

-- @param int 20
SELECT '1c_int4_eq', id, c_int4 FROM test_all_types_det WHERE c_int4 = ?;

-- @param int 200
SELECT '1d_int8_eq', id, c_int8 FROM test_all_types_det WHERE c_int8 = ?;

-- @param BigDecimal 2.5
SELECT '1e_float4_eq', id, c_float4 FROM test_all_types_det WHERE c_float4 = ?;

-- @param BigDecimal 20.5
SELECT '1f_float8_eq', id, c_float8 FROM test_all_types_det WHERE c_float8 = ?;

-- @param BigDecimal 200.5
SELECT '1g_numeric_eq', id, c_numeric FROM test_all_types_det WHERE c_numeric = ?;

-- @param String medium_string
SELECT '1h_varchar_eq', id, c_varchar FROM test_all_types_det WHERE c_varchar = ?;

-- @param String bpchar2
SELECT '1i_bpchar_eq', id, c_bpchar FROM test_all_types_det WHERE c_bpchar = ?;

-- @param String text2
SELECT '1j_text_eq', id, c_text FROM test_all_types_det WHERE c_text = ?;

-- Composite conditions
-- @param int 30
-- @param int 300
SELECT '1k_int4_and_int8', id FROM test_all_types_det WHERE c_int4 = ? AND c_int8 = ?;

-- @param int 10
-- @param int 30
SELECT '1l_int4_or_int4', id, c_int4 FROM test_all_types_det WHERE c_int4 = ? OR c_int4 = ? ORDER BY id;

-- @param int 20
-- @param BigDecimal 20.5
SELECT '1m_cross_type_and', id FROM test_all_types_det WHERE c_int4 = ? AND c_float8 = ?;

-- @param BigDecimal 100.5
-- @param String short_str
SELECT '1n_numeric_and_varchar', id FROM test_all_types_det WHERE c_numeric = ? AND c_varchar = ?;

-- Not equal
-- @param int 20
SELECT '1o_int4_ne', id, c_int4 FROM test_all_types_det WHERE c_int4 <> ? ORDER BY id;

-- Mixed plain + encrypted
-- @param int 2
-- @param int 200
SELECT '1p_plain_and_enc', id, c_int8 FROM test_all_types_det WHERE id >= ? AND c_int8 = ?;

-- No match
-- @param int 999
SELECT '1q_no_match', id FROM test_all_types_det WHERE c_int4 = ?;

-- Full row exact match
-- @param int 3
-- @param int 30
-- @param int 300
-- @param BigDecimal 3.5
-- @param BigDecimal 30.5
-- @param BigDecimal 300.5
-- @param String a_longer_string_value
-- @param String bpchar3
-- @param String text3
SELECT '1r_full_row_match', id FROM test_all_types_det WHERE c_int2 = ? AND c_int4 = ? AND c_int8 = ?
    AND c_float4 = ? AND c_float8 = ? AND c_numeric = ?
    AND c_varchar = ? AND c_bpchar = ? AND c_text = ?;

-- @param BigDecimal 10.5
SELECT '1s_float8_int_input', id, c_float8 FROM test_all_types_det WHERE c_float8 = ?;

-- @param String text99
SELECT '1t_text_last_row', id, c_text FROM test_all_types_det WHERE c_text = ?;

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

-- @param String hello
-- @param String hello
-- @param String hello
-- @param String hello
-- @param String hello
-- @param String hello 
-- @param String world
-- @param String world
-- @param String world
-- @param String test
-- @param String test
-- @param String test
-- @param String Hello
-- @param String Hello
-- @param String Hello
-- @param String hello 
-- @param String hello 
-- @param String hello 
-- @param null null
-- @param null null
-- @param null null
-- @param String 
-- @param String 
-- @param String 
INSERT INTO tecc_string_type_test (txt_col, vc_col, bp_col) VALUES
(?, ?, ?), (?, ?, ?),
(?, ?, ?), (?, ?, ?),
(?, ?, ?), (?, ?, ?),
(?, ?, ?), (?, ?, ?);

-- NULL cross-type comparison
SELECT '2a_null_cross', txt_col IS NULL AS text_null, txt_col = vc_col AS te_vc
FROM tecc_string_type_test WHERE txt_col IS NULL;

-- Empty string cross-type
-- @param String 
SELECT '2b_empty_cross', txt_col = '' AS text_eq, txt_col = vc_col AS te_vc
FROM tecc_string_type_test WHERE txt_col = ?;

-- IS DISTINCT FROM (cross-type)
SELECT '2c_distinct_cross', txt_col IS DISTINCT FROM vc_col AS te_va
FROM tecc_string_type_test;

-- bpchar equality with spaces (bug7906)
CREATE TABLE string_type_test (
    id SERIAL PRIMARY KEY,
    txt_col text, vc_col varchar, bp_col bpchar
);

-- @param String hello
-- @param String hello
-- @param String hello
-- @param String hello
-- @param String hello
-- @param String hello 
-- @param String world
-- @param String world
-- @param String world
-- @param String test
-- @param String test
-- @param String test
-- @param String Hello
-- @param String Hello
-- @param String Hello
-- @param String hello 
-- @param String hello 
-- @param String hello 
-- @param null null
-- @param null null
-- @param null null
-- @param String 
-- @param String 
-- @param String 
INSERT INTO string_type_test (txt_col, vc_col, bp_col) VALUES
(?, ?, ?), (?, ?, ?),
(?, ?, ?), (?, ?, ?),
(?, ?, ?), (?, ?, ?),
(?, ?, ?), (?, ?, ?);

-- Compare bpchar equality: 'hello' vs 'hello ' (should both be true for bpchar)
-- @param String hello
SELECT '2d_bpchar_space_plain', bp_col = 'hello' AS eq_hello, bp_col = 'hello ' AS eq_hello_sp
FROM string_type_test WHERE bp_col = ?;

-- @param String hello
SELECT '2e_bpchar_space_enc', bp_col = 'hello' AS eq_hello, bp_col = 'hello ' AS eq_hello_sp
FROM tecc_string_type_test WHERE bp_col = ?;

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

-- @param int 1
-- @param String Alice
-- @param int 2
-- @param String Bob
-- @param int 3
-- @param String Charlie
-- @param int 4
-- @param String David
-- @param int 5
-- @param String Eve
INSERT INTO test_where (id, name) VALUES (?, ?), (?, ?), (?, ?), (?, ?), (?, ?);

-- @param int 2
SELECT '3a_id_gt', * FROM test_where WHERE id > ? ORDER BY id;

-- @param int 3
SELECT '3b_id_eq', * FROM test_where WHERE id = ?;

-- @param String Charlie
SELECT '3c_name_eq', * FROM test_where WHERE name = ?;

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

-- @param String apple
-- @param String apple
-- @param String apple
-- @param String application
-- @param String application
-- @param String application
-- @param String banana
-- @param String banana
-- @param String banana
-- @param String berry
-- @param String berry
-- @param String berry
-- @param String cherry
-- @param String cherry
-- @param String cherry
-- @param String Apple
-- @param String Apple
-- @param String Apple
-- @param String APPLE
-- @param String APPLE
-- @param String APPLE
-- @param String 123apple
-- @param String 123apple
-- @param String 123apple
-- @param String apple123
-- @param String apple123
-- @param String apple123
-- @param String a
-- @param String a
-- @param String a
-- @param String zoo
-- @param String zoo
-- @param String zoo
-- @param String 
-- @param String 
-- @param String 
-- @param null null
-- @param null null
-- @param null null
INSERT INTO tecc_text_case_test (text_col, varchar_col, bpchar_col) VALUES
(?, ?, ?), (?, ?, ?),
(?, ?, ?), (?, ?, ?),
(?, ?, ?), (?, ?, ?),
(?, ?, ?), (?, ?, ?),
(?, ?, ?), (?, ?, ?),
(?, ?, ?), (?, ?, ?), (?, ?, ?);

-- Case-sensitive equality
-- @param String apple
SELECT '4a_eq_apple', id, text_col FROM tecc_text_case_test WHERE text_col = ? ORDER BY text_col;

-- @param String Apple
SELECT '4b_eq_Apple', id, text_col FROM tecc_text_case_test WHERE text_col = ? ORDER BY text_col;

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

-- @param int 1
-- @param String Alice
-- @param int 100
-- @param BigDecimal 100.5
-- @param String Apple
-- @param String AA
-- @param int 2
-- @param String Bob
-- @param int 200
-- @param BigDecimal 200.5
-- @param String Banana
-- @param String BB
-- @param int 3
-- @param String Charlie
-- @param int 300
-- @param BigDecimal 300.5
-- @param String Cherry
-- @param String CC
-- @param int 4
-- @param String David
-- @param int 400
-- @param BigDecimal 400.5
-- @param String Date
-- @param String DD
-- @param int 5
-- @param String Eve
-- @param int 500
-- @param BigDecimal 500.5
-- @param String Elderberry
-- @param String EE
INSERT INTO tecc_test VALUES
(?, ?,   ?, ?, ?,     ?),
(?, ?,     ?, ?, ?,    ?),
(?, ?, ?, ?, ?,    ?),
(?, ?,   ?, ?, ?,      ?),
(?, ?,     ?, ?, ?,?);

-- DET equality
-- @param int 300
SELECT '5a_int_eq', id, name, col_int FROM tecc_test WHERE col_int = ? ORDER BY id;

-- @param BigDecimal 500.5
SELECT '5b_float_eq', id, name, col_float FROM tecc_test WHERE col_float = ? ORDER BY id;

-- OPE range
-- @param int 300
SELECT '5c_int_gt', id, name, col_int FROM tecc_test WHERE col_int > ? ORDER BY id;

-- @param int 300
SELECT '5d_int_lt', id, name, col_int FROM tecc_test WHERE col_int < ? ORDER BY id;

-- @param int 300
-- @param int 500
SELECT '5e_int_between', id, name, col_int FROM tecc_test WHERE col_int BETWEEN ? AND ? ORDER BY id;

-- SEARCH LIKE
-- @param String %ana%
SELECT '5f_like_ana', id, name, col_text FROM tecc_test WHERE col_text LIKE ? ORDER BY id;

-- @param String %erry%
SELECT '5g_like_erry', id, name, col_text FROM tecc_test WHERE col_text LIKE ? ORDER BY id;

-- @param String %e%
SELECT '5h_like_e', id, name, col_text FROM tecc_test WHERE col_text LIKE ? ORDER BY id;

-- DET equality + ORDER BY
-- @param int 500
SELECT '5i_eq_order_asc', id, name, col_int FROM tecc_test WHERE col_int = ? ORDER BY id ASC;

-- @param int 300
SELECT '5j_eq_order_desc', id, name, col_int FROM tecc_test WHERE col_int = ? ORDER BY id DESC;

-- Range + ORDER BY
-- @param int 300
SELECT '5k_range_order_asc', id, name, col_int FROM tecc_test WHERE col_int > ? ORDER BY id ASC;

-- @param int 300
SELECT '5l_range_order_desc', id, name, col_int FROM tecc_test WHERE col_int > ? ORDER BY id DESC;

-- @param int 300
SELECT '5m_range_order_name', id, name, col_int FROM tecc_test WHERE col_int <= ? ORDER BY name ASC;

-- LIKE + ORDER BY
-- @param String %e%
SELECT '5n_like_order_desc', id, name, col_text FROM tecc_test WHERE col_text LIKE ? ORDER BY id DESC;

-- UPDATE + WHERE
-- @param String BigNum
-- @param int 400
UPDATE tecc_test SET name = ? WHERE col_int > ?;

SELECT '5o_update_range', id, name, col_int FROM tecc_test ORDER BY id;

-- @param String CherryLover
-- @param String %erry%
UPDATE tecc_test SET name = ? WHERE col_text LIKE ?;

SELECT '5p_update_like', id, name, col_text FROM tecc_test ORDER BY id;

-- DELETE + WHERE
-- @param int 100
DELETE FROM tecc_test WHERE col_int <= ?;

SELECT '5q_delete_range', id, name, col_int FROM tecc_test ORDER BY id;

-- Full table
SELECT '5r_full', id, name, col_int, col_text FROM tecc_test ORDER BY id;

DROP TABLE tecc_test CASCADE;

-- ============================================================
-- Cleanup
-- ============================================================
SELECT '=== Cleanup ===';

DROP KEK IF EXISTS kek_det CASCADE;
