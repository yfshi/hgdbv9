-- ============================================================
-- number_order.sql — 数字排序/跨类型比较测试
-- 覆盖 test_ope_orderby, test_ope_part1_int,
--   test_ope_range_queries, test_ope_text_orderby,
--   test_int_types, test_numeric_cross_type,
--   test_bug7856_7860_7824_ope_orderby
-- ============================================================

-- ============================================================
-- 0. Setup
-- ============================================================
DROP KEK IF EXISTS kek_num CASCADE;

CREATE KEK kek_num WITH (KEY_STORE = 'localkms', KEY_PATH = '/tmp/kek_num.key');

CREATE DEK dek_num WITH VALUES(KEK = kek_num);

-- ============================================================
-- 1. Int2/Int4/Int8 ORDER BY + plain comparison (test_int_types + bug7824)
-- ============================================================
SELECT '=== 1 Int ORDER BY ===';

-- int2
DROP TABLE IF EXISTS test_i2 CASCADE;

CREATE TABLE test_i2 (id SERIAL, enc_val int2_enc encrypted with (dek = dek_num, support_algo = 'ope'), plain_val int2, descr TEXT);

-- @param int 0
-- @param int 0
-- @param String zero
-- @param int -1
-- @param int -1
-- @param String neg one
-- @param int 32767
-- @param int 32767
-- @param String max
-- @param int -32768
-- @param int -32768
-- @param String min
-- @param int 100
-- @param int 100
-- @param String pos
-- @param int -100
-- @param int -100
-- @param String neg
-- @param int 1
-- @param int 1
-- @param String one
INSERT INTO test_i2 (enc_val, plain_val, descr) VALUES
(?, ?, ?), (?, ?, ?), (?, ?, ?), (?, ?, ?),
(?, ?, ?), (?, ?, ?), (?, ?, ?);

SELECT '1a_int2_decrypt', id, enc_val, plain_val, descr FROM test_i2 ORDER BY id;

SELECT '1b_int2_enc_order', id, enc_val FROM test_i2 ORDER BY enc_val NULLS FIRST;

SELECT '1c_int2_plain_order', id, plain_val FROM test_i2 ORDER BY plain_val NULLS FIRST;

DROP TABLE IF EXISTS test_i2 CASCADE;

-- int4
DROP TABLE IF EXISTS test_i4 CASCADE;

CREATE TABLE test_i4 (id SERIAL, enc_val int4_enc encrypted with (dek = dek_num, support_algo = 'ope'), plain_val int4, descr TEXT);

-- @param int 0
-- @param int 0
-- @param String zero
-- @param int -1
-- @param int -1
-- @param String neg one
-- @param int 2147483647
-- @param int 2147483647
-- @param String max
-- @param int -2147483648
-- @param int -2147483648
-- @param String min
-- @param int 1000000
-- @param int 1000000
-- @param String pos
-- @param int -1000000
-- @param int -1000000
-- @param String neg
INSERT INTO test_i4 (enc_val, plain_val, descr) VALUES
(?, ?, ?), (?, ?, ?), (?, ?, ?), (?, ?, ?),
(?, ?, ?), (?, ?, ?);

SELECT '1d_int4_decrypt', id, enc_val, plain_val, descr FROM test_i4 ORDER BY id;

SELECT '1e_int4_enc_order', id, enc_val FROM test_i4 ORDER BY enc_val NULLS FIRST;

SELECT '1f_int4_plain_order', id, plain_val FROM test_i4 ORDER BY plain_val NULLS FIRST;

DROP TABLE IF EXISTS test_i4 CASCADE;

-- int8
DROP TABLE IF EXISTS test_i8 CASCADE;

CREATE TABLE test_i8 (id SERIAL, enc_val int8_enc encrypted with (dek = dek_num, support_algo = 'ope'), plain_val int8, descr TEXT);

-- @param int 0
-- @param int 0
-- @param String zero
-- @param int -1
-- @param int -1
-- @param String neg one
-- @param long 9223372036854775807
-- @param long 9223372036854775807
-- @param String max
-- @param long -9223372036854775808
-- @param long -9223372036854775808
-- @param String min
-- @param long 1000000000000
-- @param long 1000000000000
-- @param String pos
-- @param long -1000000000000
-- @param long -1000000000000
-- @param String neg
INSERT INTO test_i8 (enc_val, plain_val, descr) VALUES
(?, ?, ?), (?, ?, ?), (?, ?, ?),
(?, ?, ?), (?, ?, ?),
(?, ?, ?);

SELECT '1g_int8_decrypt', id, enc_val, plain_val, descr FROM test_i8 ORDER BY id;

SELECT '1h_int8_enc_order', id, enc_val FROM test_i8 ORDER BY enc_val NULLS FIRST;

SELECT '1i_int8_plain_order', id, plain_val FROM test_i8 ORDER BY plain_val NULLS FIRST;

DROP TABLE IF EXISTS test_i8 CASCADE;

-- ============================================================
-- 2. Cross-type comparison (test_numeric_cross_type)
-- ============================================================
SELECT '=== 2 Cross-Type Comparison ===';

DROP TABLE IF EXISTS test_num_cross CASCADE;

CREATE TABLE test_num_cross (
    id      SERIAL PRIMARY KEY,
    c_i2    INT2_ENC    ENCRYPTED WITH (DEK = 'dek_num', SUPPORT_ALGO = 'OPE'),
    c_i4    INT4_ENC    ENCRYPTED WITH (DEK = 'dek_num', SUPPORT_ALGO = 'OPE'),
    c_i8    INT8_ENC    ENCRYPTED WITH (DEK = 'dek_num', SUPPORT_ALGO = 'OPE'),
    c_f4    FLOAT4_ENC  ENCRYPTED WITH (DEK = 'dek_num', SUPPORT_ALGO = 'OPE'),
    c_f8    FLOAT8_ENC  ENCRYPTED WITH (DEK = 'dek_num', SUPPORT_ALGO = 'OPE'),
    c_num   NUMERIC_ENC ENCRYPTED WITH (DEK = 'dek_num', SUPPORT_ALGO = 'OPE')
);

-- @param int 10
-- @param int 100
-- @param int 1000
-- @param BigDecimal 10.5
-- @param BigDecimal 100.5
-- @param BigDecimal 1000.5
-- @param int 20
-- @param int 200
-- @param int 2000
-- @param BigDecimal 20.5
-- @param BigDecimal 200.5
-- @param BigDecimal 2000.5
-- @param int 30
-- @param int 300
-- @param int 3000
-- @param BigDecimal 30.5
-- @param BigDecimal 300.5
-- @param BigDecimal 3000.5
INSERT INTO test_num_cross (c_i2, c_i4, c_i8, c_f4, c_f8, c_num) VALUES
(?, ?, ?, ?, ?, ?),
(?, ?, ?, ?, ?, ?),
(?, ?, ?, ?, ?, ?);

-- Full decrypt
SELECT '2a_full_decrypt', id, c_i2, c_i4, c_i8, c_f4, c_f8, c_num FROM test_num_cross ORDER BY id;

-- Intra-type eq and range
-- @param int 100
SELECT '2b_int4_eq', id, c_i4 FROM test_num_cross WHERE c_i4 = ? ORDER BY id;

-- @param int 100
SELECT '2c_int4_gt', id, c_i4 FROM test_num_cross WHERE c_i4 > ? ORDER BY id;

-- @param BigDecimal 10.5
SELECT '2d_float4_eq', id, c_f4 FROM test_num_cross WHERE c_f4 = ? ORDER BY id;

-- @param int 150
SELECT '2e_float8_gt', id, c_f8 FROM test_num_cross WHERE c_f8 > ? ORDER BY id;

-- @param BigDecimal 1000.5
SELECT '2f_numeric_eq', id, c_num FROM test_num_cross WHERE c_num = ? ORDER BY id;

-- @param int 2000
SELECT '2g_numeric_lt', id, c_num FROM test_num_cross WHERE c_num < ? ORDER BY id;

-- Cross-type AND
-- @param int 100
-- @param int 1000
SELECT '2h_int4_and_int8', id FROM test_num_cross WHERE c_i4 = ? AND c_i8 = ? ORDER BY id;

-- @param int 10
-- @param int 100
SELECT '2i_int2_and_int4', id FROM test_num_cross WHERE c_i2 = ? AND c_i4 = ? ORDER BY id;

-- @param int 100
-- @param BigDecimal 10.5
SELECT '2j_int4_and_float4', id FROM test_num_cross WHERE c_i4 = ? AND c_f4 = ? ORDER BY id;

-- @param int 1000
-- @param BigDecimal 1000.5
SELECT '2k_int8_and_numeric', id FROM test_num_cross WHERE c_i8 = ? AND c_num = ? ORDER BY id;

-- Cross-type range comparisons against constants
-- @param int 15
SELECT '2l_float4_gt_int_const', id, c_f4 FROM test_num_cross WHERE c_f4 > ? ORDER BY id;

-- @param int 150
SELECT '2m_float8_lt_int_const', id, c_f8 FROM test_num_cross WHERE c_f8 < ? ORDER BY id;

-- @param int 500
SELECT '2n_numeric_gt_int_const', id, c_num FROM test_num_cross WHERE c_num > ? ORDER BY id;

-- @param int 500
-- @param int 5000
SELECT '2o_int8_between', id, c_i8 FROM test_num_cross WHERE c_i8 > ? AND c_i8 < ? ORDER BY id;

-- Cross-type column-to-column comparison
SELECT '2p_i4_gt_i2', id, c_i2, c_i4 FROM test_num_cross WHERE c_i4 > c_i2 ORDER BY id;

SELECT '2q_i8_gt_i4', id, c_i4, c_i8 FROM test_num_cross WHERE c_i8 > c_i4 ORDER BY id;

SELECT '2r_f4_gt_i2', id, c_i2, c_f4 FROM test_num_cross WHERE c_f4 > c_i2 ORDER BY id;

SELECT '2s_f8_gt_i4', id, c_i4, c_f8 FROM test_num_cross WHERE c_f8 > c_i4 ORDER BY id;

SELECT '2t_num_gt_i8', id, c_i8, c_num FROM test_num_cross WHERE c_num > c_i8 ORDER BY id;

SELECT '2u_i8_gt_f4', id, c_f4, c_i8 FROM test_num_cross WHERE c_i8 > c_f4 ORDER BY id;

SELECT '2v_num_gt_f4', id, c_f4, c_num FROM test_num_cross WHERE c_num > c_f4 ORDER BY id;

SELECT '2w_combo_i4_gt_i2_and_i8_gt_i4', id, c_i2, c_i4, c_i8 FROM test_num_cross WHERE c_i4 > c_i2 AND c_i8 > c_i4 ORDER BY id;

-- Equal value cross-type
-- @param int 50
-- @param int 50
-- @param int 50
-- @param BigDecimal 50.0
-- @param BigDecimal 50.0
-- @param BigDecimal 50.00
INSERT INTO test_num_cross (c_i2, c_i4, c_i8, c_f4, c_f8, c_num) VALUES (?, ?, ?, ?, ?, ?);

SELECT '2x_eq_i4_eq_i2', id, c_i2, c_i4 FROM test_num_cross WHERE c_i4 = c_i2 ORDER BY id;

SELECT '2y_eq_f4_eq_i2', id, c_i2, c_f4 FROM test_num_cross WHERE c_f4 = c_i2 ORDER BY id;

SELECT '2z_eq_num_eq_i8', id, c_i8, c_num FROM test_num_cross WHERE c_num = c_i8 ORDER BY id;

-- ============================================================
-- 3. Numeric typmod tests
-- ============================================================
SELECT '=== 3 Numeric Typmod ===';

DROP TABLE IF EXISTS test_num_typmod CASCADE;

CREATE TABLE test_num_typmod (
    id      SERIAL PRIMARY KEY,
    c_num2  NUMERIC_ENC(10,2) ENCRYPTED WITH (DEK = 'dek_num', SUPPORT_ALGO = 'OPE'),
    c_num4  NUMERIC_ENC(8,4)  ENCRYPTED WITH (DEK = 'dek_num', SUPPORT_ALGO = 'OPE'),
    c_int4  INT4_ENC          ENCRYPTED WITH (DEK = 'dek_num', SUPPORT_ALGO = 'OPE'),
    c_f4    FLOAT4_ENC        ENCRYPTED WITH (DEK = 'dek_num', SUPPORT_ALGO = 'OPE'),
    c6_0    NUMERIC_ENC(6,0)  ENCRYPTED WITH (DEK = 'dek_num', SUPPORT_ALGO = 'OPE')
);

SELECT '3a_typmod_10_2_100p5', id, c_num2 FROM test_num_typmod;

-- @param BigDecimal 100.5
INSERT INTO test_num_typmod (c_num2) VALUES (?);

SELECT '3b_typmod_10_2_100p5', id, c_num2 FROM test_num_typmod ORDER BY id;

-- @param BigDecimal 100.556
INSERT INTO test_num_typmod (c_num2) VALUES (?);

-- should round to 100.56
SELECT '3c_typmod_round', id, c_num2 FROM test_num_typmod ORDER BY id;

-- @param BigDecimal 12.3456
INSERT INTO test_num_typmod (c_num4) VALUES (?);

SELECT '3d_typmod_8_4', id, c_num4 FROM test_num_typmod WHERE id = (SELECT max(id) FROM test_num_typmod);

-- @param BigDecimal 50.00
-- @param int 50
INSERT INTO test_num_typmod (c_num2, c_int4) VALUES (?, ?);

-- @param BigDecimal 50.00
-- @param int 50
SELECT '3e_typmod_cross_eq', id, c_num2, c_int4 FROM test_num_typmod WHERE c_num2 = ? AND c_int4 = ?;

-- @param BigDecimal 25.50
-- @param BigDecimal 25.5
INSERT INTO test_num_typmod (c_num2, c_f4) VALUES (?, ?);

-- @param BigDecimal 25.50
-- @param BigDecimal 25.5
SELECT '3f_typmod_cross_eq_f4', id, c_num2, c_f4 FROM test_num_typmod WHERE c_num2 = ? AND c_f4 = ?;

SELECT '3g_typmod_col_eq', id, c_num2, c_int4 FROM test_num_typmod WHERE c_num2 = c_int4;

SELECT '3h_typmod_col_gt', id, c_num2, c_f4 FROM test_num_typmod WHERE c_num2 > c_f4;

-- ============================================================
-- 3i-3n: Typmod output format â encrypted vs plain consistency
-- Encrypted output should display same text as plain numeric for same typmod
-- ============================================================
SELECT '=== 3i-3n: Typmod output format ===';

DROP TABLE IF EXISTS test_num_typmod_ref CASCADE;

CREATE TABLE test_num_typmod_ref (
    descr   text,
    c10_2   numeric(10,2),
    c6_0    numeric(6,0),
    c8_4    numeric(8,4)
);

-- 3i: (10,2) round 100.556 -> both sides should show 100.56
-- @param String 3i_round
-- @param BigDecimal 100.556
-- @param null null
-- @param null null
INSERT INTO test_num_typmod_ref VALUES (?, ?, ?, ?);

-- @param BigDecimal 100.556
INSERT INTO test_num_typmod (c_num2) VALUES (?);

-- @param String 3i_round
SELECT '3i_round10_2', e.c_num2 AS enc, r.c10_2 AS "plain ref"
FROM test_num_typmod e, test_num_typmod_ref r
WHERE r.descr = ? AND e.id = (SELECT max(id) FROM test_num_typmod);

-- 3j: (10,2) pad 1.5 -> both sides should show 1.50
-- @param String 3j_pad
-- @param BigDecimal 1.5
-- @param null null
-- @param null null
INSERT INTO test_num_typmod_ref VALUES (?, ?, ?, ?);

-- @param BigDecimal 1.5
INSERT INTO test_num_typmod (c_num2) VALUES (?);

-- @param String 3j_pad
SELECT '3j_pad10_2', e.c_num2 AS enc, r.c10_2 AS "plain ref"
FROM test_num_typmod e, test_num_typmod_ref r
WHERE r.descr = ? AND e.id = (SELECT max(id) FROM test_num_typmod);

-- 3k: (6,0) truncate 123.456 -> both sides should show 123
-- @param String 3k_trunc
-- @param null null
-- @param BigDecimal 123.456
-- @param null null
INSERT INTO test_num_typmod_ref VALUES (?, ?, ?, ?);

-- @param BigDecimal 123.456
INSERT INTO test_num_typmod (c6_0) VALUES (?);

-- @param String 3k_trunc
SELECT '3k_trunc6_0', e.c6_0 AS enc, r.c6_0 AS "plain ref"
FROM test_num_typmod e, test_num_typmod_ref r
WHERE r.descr = ? AND e.id = (SELECT max(id) FROM test_num_typmod);

-- 3l: (8,4) round 0.123456 -> both sides should show 0.1235
-- @param String 3l_round4
-- @param null null
-- @param null null
-- @param BigDecimal 0.123456
INSERT INTO test_num_typmod_ref VALUES (?, ?, ?, ?);

-- @param BigDecimal 0.123456
INSERT INTO test_num_typmod (c_num4) VALUES (?);

-- @param String 3l_round4
SELECT '3l_round8_4', e.c_num4 AS enc, r.c8_4 AS "plain ref"
FROM test_num_typmod e, test_num_typmod_ref r
WHERE r.descr = ? AND e.id = (SELECT max(id) FROM test_num_typmod);

-- 3n: integer for (10,2): 50 -> 50.00
-- @param String 3n_int
-- @param int 50
-- @param null null
-- @param null null
INSERT INTO test_num_typmod_ref VALUES (?, ?, ?, ?);

-- @param int 50
INSERT INTO test_num_typmod (c_num2) VALUES (?);

-- @param String 3n_int
SELECT '3n_int50', e.c_num2 AS enc, r.c10_2 AS "plain ref"
FROM test_num_typmod e, test_num_typmod_ref r
WHERE r.descr = ? AND e.id = (SELECT max(id) FROM test_num_typmod);

DROP TABLE IF EXISTS test_num_typmod_ref CASCADE;

DROP TABLE test_num_typmod;

-- Plain reference
SELECT '3n_plain_int50', 50::numeric(10,2) AS plain;

-- ============================================================
-- 4. Multi-type cross-type UPDATE (from test_numeric_cross_type)
-- ============================================================
SELECT '=== 4 Cross-Type UPDATE ===';

-- @param int -100
-- @param int -100
-- @param int -100
-- @param BigDecimal -100.0
-- @param BigDecimal -100.0
-- @param BigDecimal -100.00
-- @param int 0
-- @param int 0
-- @param int 0
-- @param BigDecimal 0.0
-- @param BigDecimal 0.0
-- @param BigDecimal 0.00
-- @param int 32000
-- @param int 2000000000
-- @param int 900000000
-- @param BigDecimal 3.2e4
-- @param BigDecimal 2.0e9
-- @param int 900000000
-- @param int 1
-- @param int 1
-- @param int 1
-- @param BigDecimal 1.5
-- @param BigDecimal 1.5
-- @param BigDecimal 1.23456789
INSERT INTO test_num_cross (c_i2, c_i4, c_i8, c_f4, c_f8, c_num) VALUES
(?, ?, ?, ?, ?, ?),  -- negative
(?, ?, ?, ?, ?, ?),                      -- zero
(?, ?, ?, ?, ?, ?),  -- large
(?, ?, ?, ?, ?, ?);

-- high precision

-- Cross-type SET
-- @param int 0
UPDATE test_num_cross SET c_f8 = c_i2 WHERE c_i2 = c_i4 AND c_i2 < ?;

-- @param int 0
SELECT '4a_update_f8_eq_i2_neg', id, c_i2, c_f8 FROM test_num_cross WHERE c_i2 = c_i4 AND c_i2 < ? ORDER BY id;

-- @param int 0
UPDATE test_num_cross SET c_num = c_f4 WHERE c_i2 = ?;

-- @param int 0
SELECT '4b_update_num_eq_f4_zero', id, c_f4, c_num FROM test_num_cross WHERE c_i2 = ? ORDER BY id;

DROP TABLE IF EXISTS test_num_cross CASCADE;

-- ============================================================
-- Cleanup
-- ============================================================
SELECT '=== Cleanup ===';

DROP KEK IF EXISTS kek_num CASCADE;
