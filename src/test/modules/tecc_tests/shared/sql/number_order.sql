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
INSERT INTO test_i2 (enc_val, plain_val, descr) VALUES
(0, 0, 'zero'), (-1, -1, 'neg one'), (32767, 32767, 'max'), (-32768, -32768, 'min'),
(100, 100, 'pos'), (-100, -100, 'neg'), (1, 1, 'one');
SELECT '1a_int2_decrypt', id, enc_val, plain_val, descr FROM test_i2 ORDER BY id;
SELECT '1b_int2_enc_order', id, enc_val FROM test_i2 ORDER BY enc_val NULLS FIRST;
SELECT '1c_int2_plain_order', id, plain_val FROM test_i2 ORDER BY plain_val NULLS FIRST;
DROP TABLE IF EXISTS test_i2 CASCADE;

-- int4
DROP TABLE IF EXISTS test_i4 CASCADE;
CREATE TABLE test_i4 (id SERIAL, enc_val int4_enc encrypted with (dek = dek_num, support_algo = 'ope'), plain_val int4, descr TEXT);
INSERT INTO test_i4 (enc_val, plain_val, descr) VALUES
(0, 0, 'zero'), (-1, -1, 'neg one'), (2147483647, 2147483647, 'max'), (-2147483648, -2147483648, 'min'),
(1000000, 1000000, 'pos'), (-1000000, -1000000, 'neg');
SELECT '1d_int4_decrypt', id, enc_val, plain_val, descr FROM test_i4 ORDER BY id;
SELECT '1e_int4_enc_order', id, enc_val FROM test_i4 ORDER BY enc_val NULLS FIRST;
SELECT '1f_int4_plain_order', id, plain_val FROM test_i4 ORDER BY plain_val NULLS FIRST;
DROP TABLE IF EXISTS test_i4 CASCADE;

-- int8
DROP TABLE IF EXISTS test_i8 CASCADE;
CREATE TABLE test_i8 (id SERIAL, enc_val int8_enc encrypted with (dek = dek_num, support_algo = 'ope'), plain_val int8, descr TEXT);
INSERT INTO test_i8 (enc_val, plain_val, descr) VALUES
(0, 0, 'zero'), (-1, -1, 'neg one'), (9223372036854775807, 9223372036854775807, 'max'),
(-9223372036854775808, -9223372036854775808, 'min'), (1000000000000, 1000000000000, 'pos'),
(-1000000000000, -1000000000000, 'neg');
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

INSERT INTO test_num_cross (c_i2, c_i4, c_i8, c_f4, c_f8, c_num) VALUES
(10, 100, 1000, 10.5, 100.5, 1000.5),
(20, 200, 2000, 20.5, 200.5, 2000.5),
(30, 300, 3000, 30.5, 300.5, 3000.5);

-- Full decrypt
SELECT '2a_full_decrypt', id, c_i2, c_i4, c_i8, c_f4, c_f8, c_num FROM test_num_cross ORDER BY id;

-- Intra-type eq and range
SELECT '2b_int4_eq', id, c_i4 FROM test_num_cross WHERE c_i4 = 100 ORDER BY id;
SELECT '2c_int4_gt', id, c_i4 FROM test_num_cross WHERE c_i4 > 100 ORDER BY id;
SELECT '2d_float4_eq', id, c_f4 FROM test_num_cross WHERE c_f4 = 10.5 ORDER BY id;
SELECT '2e_float8_gt', id, c_f8 FROM test_num_cross WHERE c_f8 > 150 ORDER BY id;
SELECT '2f_numeric_eq', id, c_num FROM test_num_cross WHERE c_num = 1000.5 ORDER BY id;
SELECT '2g_numeric_lt', id, c_num FROM test_num_cross WHERE c_num < 2000 ORDER BY id;

-- Cross-type AND
SELECT '2h_int4_and_int8', id FROM test_num_cross WHERE c_i4 = 100 AND c_i8 = 1000 ORDER BY id;
SELECT '2i_int2_and_int4', id FROM test_num_cross WHERE c_i2 = 10 AND c_i4 = 100 ORDER BY id;
SELECT '2j_int4_and_float4', id FROM test_num_cross WHERE c_i4 = 100 AND c_f4 = 10.5 ORDER BY id;
SELECT '2k_int8_and_numeric', id FROM test_num_cross WHERE c_i8 = 1000 AND c_num = 1000.5 ORDER BY id;

-- Cross-type range comparisons against constants
SELECT '2l_float4_gt_int_const', id, c_f4 FROM test_num_cross WHERE c_f4 > 15 ORDER BY id;
SELECT '2m_float8_lt_int_const', id, c_f8 FROM test_num_cross WHERE c_f8 < 150 ORDER BY id;
SELECT '2n_numeric_gt_int_const', id, c_num FROM test_num_cross WHERE c_num > 500 ORDER BY id;
SELECT '2o_int8_between', id, c_i8 FROM test_num_cross WHERE c_i8 > 500 AND c_i8 < 5000 ORDER BY id;

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
INSERT INTO test_num_cross (c_i2, c_i4, c_i8, c_f4, c_f8, c_num) VALUES (50, 50, 50, 50.0, 50.0, 50.00);

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
INSERT INTO test_num_typmod (c_num2) VALUES (100.5);
SELECT '3b_typmod_10_2_100p5', id, c_num2 FROM test_num_typmod ORDER BY id;

INSERT INTO test_num_typmod (c_num2) VALUES (100.556);  -- should round to 100.56
SELECT '3c_typmod_round', id, c_num2 FROM test_num_typmod ORDER BY id;

INSERT INTO test_num_typmod (c_num4) VALUES (12.3456);
SELECT '3d_typmod_8_4', id, c_num4 FROM test_num_typmod WHERE id = (SELECT max(id) FROM test_num_typmod);

INSERT INTO test_num_typmod (c_num2, c_int4) VALUES (50.00, 50);
SELECT '3e_typmod_cross_eq', id, c_num2, c_int4 FROM test_num_typmod WHERE c_num2 = 50.00 AND c_int4 = 50;

INSERT INTO test_num_typmod (c_num2, c_f4) VALUES (25.50, 25.5);
SELECT '3f_typmod_cross_eq_f4', id, c_num2, c_f4 FROM test_num_typmod WHERE c_num2 = 25.50 AND c_f4 = 25.5;

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
INSERT INTO test_num_typmod_ref VALUES ('3i_round', 100.556, NULL, NULL);
INSERT INTO test_num_typmod (c_num2) VALUES (100.556);
SELECT '3i_round10_2', e.c_num2 AS enc, r.c10_2 AS "plain ref"
FROM test_num_typmod e, test_num_typmod_ref r
WHERE r.descr = '3i_round' AND e.id = (SELECT max(id) FROM test_num_typmod);

-- 3j: (10,2) pad 1.5 -> both sides should show 1.50
INSERT INTO test_num_typmod_ref VALUES ('3j_pad', 1.5, NULL, NULL);
INSERT INTO test_num_typmod (c_num2) VALUES (1.5);
SELECT '3j_pad10_2', e.c_num2 AS enc, r.c10_2 AS "plain ref"
FROM test_num_typmod e, test_num_typmod_ref r
WHERE r.descr = '3j_pad' AND e.id = (SELECT max(id) FROM test_num_typmod);

-- 3k: (6,0) truncate 123.456 -> both sides should show 123
INSERT INTO test_num_typmod_ref VALUES ('3k_trunc', NULL, 123.456, NULL);
INSERT INTO test_num_typmod (c6_0) VALUES (123.456);
SELECT '3k_trunc6_0', e.c6_0 AS enc, r.c6_0 AS "plain ref"
FROM test_num_typmod e, test_num_typmod_ref r
WHERE r.descr = '3k_trunc' AND e.id = (SELECT max(id) FROM test_num_typmod);

-- 3l: (8,4) round 0.123456 -> both sides should show 0.1235
INSERT INTO test_num_typmod_ref VALUES ('3l_round4', NULL, NULL, 0.123456);
INSERT INTO test_num_typmod (c_num4) VALUES (0.123456);
SELECT '3l_round8_4', e.c_num4 AS enc, r.c8_4 AS "plain ref"
FROM test_num_typmod e, test_num_typmod_ref r
WHERE r.descr = '3l_round4' AND e.id = (SELECT max(id) FROM test_num_typmod);

-- 3n: integer for (10,2): 50 -> 50.00
INSERT INTO test_num_typmod_ref VALUES ('3n_int', 50, NULL, NULL);
INSERT INTO test_num_typmod (c_num2) VALUES (50);
SELECT '3n_int50', e.c_num2 AS enc, r.c10_2 AS "plain ref"
FROM test_num_typmod e, test_num_typmod_ref r
WHERE r.descr = '3n_int' AND e.id = (SELECT max(id) FROM test_num_typmod);

DROP TABLE IF EXISTS test_num_typmod_ref CASCADE;
DROP TABLE test_num_typmod;
-- Plain reference
SELECT '3n_plain_int50', 50::numeric(10,2) AS plain;

-- ============================================================
-- 4. Multi-type cross-type UPDATE (from test_numeric_cross_type)
-- ============================================================
SELECT '=== 4 Cross-Type UPDATE ===';

INSERT INTO test_num_cross (c_i2, c_i4, c_i8, c_f4, c_f8, c_num) VALUES
(-100, -100, -100, -100.0, -100.0, -100.00),  -- negative
(0, 0, 0, 0.0, 0.0, 0.00),                      -- zero
(32000, 2000000000, 900000000, 3.2e4, 2.0e9, 900000000),  -- large
(1, 1, 1, 1.5, 1.5, 1.23456789);               -- high precision

-- Cross-type SET
UPDATE test_num_cross SET c_f8 = c_i2 WHERE c_i2 = c_i4 AND c_i2 < 0;
SELECT '4a_update_f8_eq_i2_neg', id, c_i2, c_f8 FROM test_num_cross WHERE c_i2 = c_i4 AND c_i2 < 0 ORDER BY id;

UPDATE test_num_cross SET c_num = c_f4 WHERE c_i2 = 0;
SELECT '4b_update_num_eq_f4_zero', id, c_f4, c_num FROM test_num_cross WHERE c_i2 = 0 ORDER BY id;

DROP TABLE IF EXISTS test_num_cross CASCADE;

-- ============================================================
-- Cleanup
-- ============================================================
SELECT '=== Cleanup ===';
DROP KEK IF EXISTS kek_num CASCADE;
