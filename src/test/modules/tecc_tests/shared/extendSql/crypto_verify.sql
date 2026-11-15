-- ============================================================
-- crypto_verify.sql — 加解密一致性/特殊值/边界值测试
-- 覆盖 test_nan_infinity, test_bug7805_empty_string,
--   test_bug7835_float4, test_bug7836_float_truncation,
--   test_bug7836_verify, test_bug7842_7843_float8_limits,
--   test_bug7914_repro, test_error_msg_propagation,
--   test_repro_bug7826, test_tecc_20cases,
--   test_tecc_basic, test_select_e2e
-- ============================================================

-- ============================================================
-- 0. Setup — 共享KEK/DEK
-- ============================================================
DROP KEK IF EXISTS kek_crypto CASCADE;

CREATE KEK kek_crypto WITH (
    KEY_STORE = 'localkms',
    KEY_PATH = '/tmp/kek_crypto.key'
);

CREATE DEK dek_crypto WITH VALUES(KEK = kek_crypto);

-- ============================================================
-- 1. NaN/Infinity 边界值 (bug7833/7831)
-- ============================================================
SELECT '=== 1 NaN/Infinity ===';

DROP TABLE IF EXISTS test_nan CASCADE;

CREATE TABLE test_nan (
    id INT4,
    f4 FLOAT4_ENC ENCRYPTED WITH (DEK = dek_crypto, SUPPORT_ALGO = 'DET'),
    f8 FLOAT8_ENC ENCRYPTED WITH (DEK = dek_crypto, SUPPORT_ALGO = 'DET'),
    nm NUMERIC_ENC ENCRYPTED WITH (DEK = dek_crypto, SUPPORT_ALGO = 'DET')
);

-- @param int 1
-- @param String NaN
-- @param String NaN
-- @param String NaN
INSERT INTO test_nan VALUES (?, ?, ?, ?);

-- @param int 2
-- @param String Infinity
-- @param String Infinity
-- @param String NaN
INSERT INTO test_nan VALUES (?, ?, ?, ?);

-- @param int 3
-- @param String -Infinity
-- @param String -Infinity
-- @param String NaN
INSERT INTO test_nan VALUES (?, ?, ?, ?);

-- @param int 4
-- @param BigDecimal 1.5
-- @param BigDecimal 2.5
-- @param BigDecimal 3.5
INSERT INTO test_nan VALUES (?, ?, ?, ?);

SELECT id, f4, f8, nm FROM test_nan ORDER BY id;

DROP TABLE IF EXISTS test_nan CASCADE;

-- ============================================================
-- 2. Empty string vs NULL (bug7805)
-- ============================================================
SELECT '=== 2 Empty String vs NULL ===';

DROP TABLE IF EXISTS test_empty CASCADE;

CREATE TABLE test_empty (
    id SERIAL,
    enc_text text_enc encrypted with (dek = dek_crypto, support_algo = 'det'),
    enc_varchar varchar_enc encrypted with (dek = dek_crypto, support_algo = 'det'),
    enc_bpchar bpchar_enc encrypted with (dek = dek_crypto, support_algo = 'det'),
    plain_text TEXT,
    descr TEXT
);

-- @param String 
-- @param String 
-- @param String 
-- @param String 
-- @param String all empty string
INSERT INTO test_empty (enc_text, enc_varchar, enc_bpchar, plain_text, descr)
    VALUES (?, ?, ?, ?, ?);

-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param String all NULL
INSERT INTO test_empty (enc_text, enc_varchar, enc_bpchar, plain_text, descr)
    VALUES (?, ?, ?, ?, ?);

-- @param String hello
-- @param String hello
-- @param String hello
-- @param String hello
-- @param String normal
INSERT INTO test_empty (enc_text, enc_varchar, enc_bpchar, plain_text, descr)
    VALUES (?, ?, ?, ?, ?);

SELECT id, COALESCE(enc_text, '<NULL>') AS enc_text,
       COALESCE(enc_varchar, '<NULL>') AS enc_varchar,
       COALESCE(enc_bpchar, '<NULL>') AS enc_bpchar, descr
FROM test_empty ORDER BY id;

SELECT 'empty_orderby', id, enc_text, descr FROM test_empty ORDER BY enc_text NULLS FIRST;

DROP TABLE IF EXISTS test_empty CASCADE;

-- ============================================================
-- 3. Float precision (bug7835/7836/7842/7843)
-- ============================================================
SELECT '=== 3 Float Precision ===';

-- 3a: float4_enc precision (bug7836) — 与普通float4对比
DROP TABLE IF EXISTS test_float4_prec CASCADE;

CREATE TABLE test_float4_prec (id SERIAL, f4 float4_enc encrypted with (dek = dek_crypto, support_algo = 'det'), f4_plain float4, descr TEXT);

-- @param BigDecimal 3.14159265
-- @param BigDecimal 3.14159265
-- @param String pi 8-digit
-- @param BigDecimal 2.71828182
-- @param BigDecimal 2.71828182
-- @param String e 8-digit
-- @param BigDecimal 123.456789
-- @param BigDecimal 123.456789
-- @param String decimal
-- @param BigDecimal -123.456789
-- @param BigDecimal -123.456789
-- @param String neg decimal
INSERT INTO test_float4_prec (f4, f4_plain, descr) VALUES
    (?, ?, ?), (?, ?, ?),
    (?, ?, ?), (?, ?, ?);

SELECT id, f4, f4_plain, descr FROM test_float4_prec ORDER BY id;

DROP TABLE IF EXISTS test_float4_prec CASCADE;

-- 3b: float8_enc precision (bug7836) — 与普通float8对比
DROP TABLE IF EXISTS test_float8_prec CASCADE;

CREATE TABLE test_float8_prec (id SERIAL, f8 float8_enc encrypted with (dek = dek_crypto, support_algo = 'det'), f8_plain float8, descr TEXT);

-- @param BigDecimal 3.141592653589793
-- @param BigDecimal 3.141592653589793
-- @param String pi 15-digit
-- @param BigDecimal 2.718281828459045
-- @param BigDecimal 2.718281828459045
-- @param String e 15-digit
-- @param BigDecimal 123.456789012345
-- @param BigDecimal 123.456789012345
-- @param String decimal
-- @param BigDecimal -123.456789012345
-- @param BigDecimal -123.456789012345
-- @param String neg decimal
INSERT INTO test_float8_prec (f8, f8_plain, descr) VALUES
    (?, ?, ?), (?, ?, ?),
    (?, ?, ?), (?, ?, ?);

SELECT id, f8, f8_plain, descr FROM test_float8_prec ORDER BY id;

DROP TABLE IF EXISTS test_float8_prec CASCADE;

-- 3c: numeric_enc precision (bug7836) — 与普通numeric对比
DROP TABLE IF EXISTS test_numeric_prec CASCADE;

CREATE TABLE test_numeric_prec (id SERIAL, n numeric_enc encrypted with (dek = dek_crypto, support_algo = 'det'), n_plain numeric, descr TEXT);

-- @param BigDecimal 3.141592653589793
-- @param BigDecimal 3.141592653589793
-- @param String pi 15-digit
-- @param BigDecimal 2.718281828459045
-- @param BigDecimal 2.718281828459045
-- @param String e 15-digit
-- @param BigDecimal 123.456789012345
-- @param BigDecimal 123.456789012345
-- @param String decimal
-- @param BigDecimal -0.0000001
-- @param BigDecimal -0.0000001
-- @param String tiny neg
INSERT INTO test_numeric_prec (n, n_plain, descr) VALUES
    (?, ?, ?), (?, ?, ?),
    (?, ?, ?), (?, ?, ?);

SELECT id, n, n_plain, descr FROM test_numeric_prec ORDER BY id;

DROP TABLE IF EXISTS test_numeric_prec CASCADE;

-- 3d: Verify native vs encrypted values match (bug7836 verify)
SELECT '3d_native_vs_enc';

SELECT 3.14159265::float4 AS float4_native;

SELECT 3.14159265::float4_enc ENCRYPTED WITH (DEK = 'dek_crypto', support_algo = 'det') AS float4_enc;

SELECT 3.141592653589793::float8 AS float8_native;

SELECT 3.141592653589793::float8_enc ENCRYPTED WITH (DEK = 'dek_crypto', support_algo = 'det') AS float8_enc;

SELECT 3.141592653589793::numeric AS numeric_native;

SELECT 3.141592653589793::numeric_enc ENCRYPTED WITH (DEK = 'dek_crypto', support_algo = 'det') AS numeric_enc;

-- 3e: float8 limits + scientific notation (bug7842/7843) — 与普通float8对比
DROP TABLE IF EXISTS test_f8_limit CASCADE;

CREATE TABLE test_f8_limit (id SERIAL, f8 float8_enc encrypted with (dek = dek_crypto, support_algo = 'det'), f8_plain float8, descr TEXT);

-- @param BigDecimal 1.7976931348623157e+308
-- @param BigDecimal 1.7976931348623157e+308
-- @param String near float8 max
-- @param BigDecimal -1.7976931348623157e+308
-- @param BigDecimal -1.7976931348623157e+308
-- @param String near float8 min
-- @param BigDecimal 1.0e308
-- @param BigDecimal 1.0e308
-- @param String big sci
-- @param BigDecimal 1.0e-308
-- @param BigDecimal 1.0e-308
-- @param String tiny sci
-- @param BigDecimal 1.0e10
-- @param BigDecimal 1.0e10
-- @param String sci large
-- @param BigDecimal 1.0e-10
-- @param BigDecimal 1.0e-10
-- @param String sci small
INSERT INTO test_f8_limit (f8, f8_plain, descr) VALUES
    (?, ?, ?), (?, ?, ?),
    (?, ?, ?), (?, ?, ?),
    (?, ?, ?), (?, ?, ?);

SELECT id, f8, f8_plain, descr FROM test_f8_limit ORDER BY id;

-- Compare with native float8
SELECT 1.7976931348623157e+308::float8 AS max_f8;

SELECT -1.7976931348623157e+308::float8 AS min_f8;

SELECT 1.0e308::float8 AS big_e308;

SELECT 1.0e-308::float8 AS small_e308;

DROP TABLE IF EXISTS test_f8_limit CASCADE;

-- 3f: float4_enc + float8_enc negative values with ORDER BY (bug7835)
DROP TABLE IF EXISTS test_f4_range CASCADE;

CREATE TABLE test_f4_range (
    id SERIAL,
    enc_val float4_enc encrypted with (dek = dek_crypto, support_algo = 'ope'),
    plain_val float4, descr TEXT
);

-- @param BigDecimal 0.0
-- @param BigDecimal 0.0
-- @param String zero
-- @param BigDecimal -1.0
-- @param BigDecimal -1.0
-- @param String neg one
-- @param BigDecimal -123.456
-- @param BigDecimal -123.456
-- @param String neg decimal
-- @param BigDecimal -0.0000001
-- @param BigDecimal -0.0000001
-- @param String tiny neg
-- @param BigDecimal -1.0e-10
-- @param BigDecimal -1.0e-10
-- @param String tiny neg sci
-- @param BigDecimal 1.0
-- @param BigDecimal 1.0
-- @param String one
-- @param BigDecimal 3.14159265
-- @param BigDecimal 3.14159265
-- @param String pi
-- @param BigDecimal 0.0000001
-- @param BigDecimal 0.0000001
-- @param String tiny pos
-- @param BigDecimal 1.0e10
-- @param BigDecimal 1.0e10
-- @param String large
INSERT INTO test_f4_range (enc_val, plain_val, descr) VALUES
(?, ?, ?), (?, ?, ?), (?, ?, ?),
(?, ?, ?), (?, ?, ?),
(?, ?, ?), (?, ?, ?), (?, ?, ?),
(?, ?, ?);

SELECT id, enc_val, plain_val, descr FROM test_f4_range ORDER BY id;

SELECT 'f4_enc_order', id, enc_val FROM test_f4_range ORDER BY enc_val NULLS FIRST;

SELECT 'f4_plain_order', id, plain_val FROM test_f4_range ORDER BY plain_val NULLS FIRST;

-- float8_enc ORDER BY
DROP TABLE IF EXISTS test_f8_range CASCADE;

CREATE TABLE test_f8_range (
    id SERIAL,
    enc_val float8_enc encrypted with (dek = dek_crypto, support_algo = 'ope'),
    plain_val float8, descr TEXT
);

-- @param BigDecimal 0.0
-- @param BigDecimal 0.0
-- @param String zero
-- @param BigDecimal -1.0
-- @param BigDecimal -1.0
-- @param String neg one
-- @param BigDecimal -123.456
-- @param BigDecimal -123.456
-- @param String neg decimal
-- @param BigDecimal -0.0000001
-- @param BigDecimal -0.0000001
-- @param String tiny neg
-- @param BigDecimal -1.0e-10
-- @param BigDecimal -1.0e-10
-- @param String tiny neg sci
-- @param BigDecimal 1.0
-- @param BigDecimal 1.0
-- @param String one
-- @param BigDecimal 3.14159265
-- @param BigDecimal 3.14159265
-- @param String pi
-- @param BigDecimal 0.0000001
-- @param BigDecimal 0.0000001
-- @param String tiny pos
-- @param BigDecimal 1.0e10
-- @param BigDecimal 1.0e10
-- @param String large
-- @param BigDecimal 1.2345678901234567
-- @param BigDecimal 1.2345678901234567
-- @param String full precision
INSERT INTO test_f8_range (enc_val, plain_val, descr) VALUES
(?, ?, ?), (?, ?, ?), (?, ?, ?),
(?, ?, ?), (?, ?, ?),
(?, ?, ?), (?, ?, ?), (?, ?, ?),
(?, ?, ?), (?, ?, ?);

SELECT id, enc_val, plain_val, descr FROM test_f8_range ORDER BY id;

SELECT 'f8_enc_order', id, enc_val FROM test_f8_range ORDER BY enc_val NULLS FIRST;

SELECT 'f8_plain_order', id, plain_val FROM test_f8_range ORDER BY plain_val NULLS FIRST;

DROP TABLE IF EXISTS test_f4_range CASCADE;

DROP TABLE IF EXISTS test_f8_range CASCADE;

-- ============================================================
-- 4. Error propagation (error_msg + bug7826)
-- ============================================================
SELECT '=== 4 Error Propagation ===';

-- 4a: int4_enc overflow (error_msg_propagation)
DROP TABLE IF EXISTS test_errmsg CASCADE;

CREATE TABLE test_errmsg (
    id INT8,
    age INT4_ENC ENCRYPTED WITH (DEK = dek_crypto, SUPPORT_ALGO = 'DET')
);

-- @param int 1
-- @param int 25
INSERT INTO test_errmsg VALUES (?, ?);

-- @param int 2
-- @param long 2147483648
INSERT INTO test_errmsg VALUES (?, ?);

-- overflow, should error
DROP TABLE IF EXISTS test_errmsg CASCADE;

-- 4b: int4 out-of-range rejection (bug7826)
CREATE TABLE tb_int4 (
    id SERIAL PRIMARY KEY,
    int_col int4_enc encrypted with (dek = dek_crypto, support_algo='ope'),
    description TEXT
);

-- @param int 0
-- @param String zero
INSERT INTO tb_int4 (int_col, description) VALUES (?, ?);

-- @param int 1
-- @param String positive
INSERT INTO tb_int4 (int_col, description) VALUES (?, ?);

-- @param int -1
-- @param String negative
INSERT INTO tb_int4 (int_col, description) VALUES (?, ?);

-- @param int 2147483647
-- @param String int4 max
INSERT INTO tb_int4 (int_col, description) VALUES (?, ?);

-- @param int -2147483648
-- @param String int4 min
INSERT INTO tb_int4 (int_col, description) VALUES (?, ?);

-- @param long 2147483648
-- @param String exceed int4 max
INSERT INTO tb_int4 (int_col, description) VALUES (?, ?);

-- should error
-- @param long -2147483649
-- @param String exceed int4 min
INSERT INTO tb_int4 (int_col, description) VALUES (?, ?);

-- should error
-- @param long 4294967295
-- @param String way exceed int4 max
INSERT INTO tb_int4 (int_col, description) VALUES (?, ?);

-- should error
-- @param BigDecimal 12.34
-- @param String float
INSERT INTO tb_int4 (int_col, description) VALUES (?, ?);

-- should error

SELECT int_col, description FROM tb_int4 ORDER BY int_col NULLS FIRST;

DROP TABLE IF EXISTS tb_int4 CASCADE;

-- ============================================================
-- 5. Basic end-to-end SELECT (select_e2e + tecc_basic)
-- ============================================================
SELECT '=== 5 Basic E2E SELECT ===';

-- 5a: Simple end-to-end (select_e2e)
DROP TABLE IF EXISTS test_employee_select CASCADE;

CREATE TABLE test_employee_select (
    id int, name varchar(50),
    salary numeric_enc ENCRYPTED WITH (DEK = dek_crypto)
);

-- @param int 1
-- @param String Alice
-- @param BigDecimal 50000.00
INSERT INTO test_employee_select (id, name, salary) VALUES (?, ?, ?);

-- @param int 1
SELECT id, name, salary FROM test_employee_select WHERE id = ?;

SELECT * FROM test_employee_select;

SELECT count(*) FROM test_employee_select;

SELECT salary FROM test_employee_select;

DROP TABLE IF EXISTS test_employee_select CASCADE;

-- 5b: Full basic test (tecc_basic)
CREATE TABLE test_emp (
    id INT4,
    name VARCHAR_ENC(100) ENCRYPTED WITH (DEK = dek_crypto, SUPPORT_ALGO = 'det,search'),
    salary INT4_ENC ENCRYPTED WITH (DEK = dek_crypto, SUPPORT_ALGO = 'OPE'),
    dept VARCHAR_ENC(50) ENCRYPTED WITH (DEK = dek_crypto, SUPPORT_ALGO = 'det')
);

-- @param int 1
-- @param String Alice
-- @param int 50000
-- @param String Engineering
INSERT INTO test_emp VALUES (?, ?, ?, ?);

-- @param int 2
-- @param String Bob
-- @param int 60000
-- @param String Sales
INSERT INTO test_emp VALUES (?, ?, ?, ?);

-- @param int 3
-- @param String Charlie
-- @param int 70000
-- @param String Engineering
INSERT INTO test_emp VALUES (?, ?, ?, ?);

-- @param int 4
-- @param String Diana
-- @param null null
-- @param String Marketing
INSERT INTO test_emp VALUES (?, ?, ?, ?);

SELECT '5b1_select_all', id, name FROM test_emp ORDER BY id;

-- @param String Engineering
SELECT '5b2_eq', id, name FROM test_emp WHERE dept = ?;

-- @param String A%
SELECT '5b3_like', id, name FROM test_emp WHERE name LIKE ?;

-- @param String B%
SELECT '5b4_not_like', id, name FROM test_emp WHERE name NOT LIKE ?;

-- @param int 50000
SELECT '5b5_cmp', id, name, salary FROM test_emp WHERE salary > ? ORDER BY id;

-- @param int 75000
-- @param String Charlie
UPDATE test_emp SET salary = ? WHERE name = ?;

-- @param int 3
SELECT '5b6_update', id, name, salary FROM test_emp WHERE id = ?;

-- @param String Diana
DELETE FROM test_emp WHERE name = ?;

SELECT '5b7_delete', count(*) FROM test_emp;

DROP TABLE IF EXISTS test_emp CASCADE;

-- ============================================================
-- 6. 20 cases — all 9 types DET round-trip (tecc_20cases)
-- ============================================================
SELECT '=== 6 All 9 Types Round-Trip (20 cases) ===';

DROP TABLE IF EXISTS tecc_all_types_test CASCADE;

CREATE TABLE tecc_all_types_test (
    id          INT4,
    name        VARCHAR(100),
    is_active   BOOLEAN DEFAULT true,
    remark      TEXT,
    created_at  TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    col_int2    INT2_ENC    ENCRYPTED WITH (DEK = dek_crypto, SUPPORT_ALGO = 'DET'),
    col_int4    INT4_ENC    ENCRYPTED WITH (DEK = dek_crypto, SUPPORT_ALGO = 'DET'),
    col_int8    INT8_ENC    ENCRYPTED WITH (DEK = dek_crypto, SUPPORT_ALGO = 'DET'),
    col_float4  FLOAT4_ENC  ENCRYPTED WITH (DEK = dek_crypto, SUPPORT_ALGO = 'DET'),
    col_float8  FLOAT8_ENC  ENCRYPTED WITH (DEK = dek_crypto, SUPPORT_ALGO = 'DET'),
    col_numeric NUMERIC_ENC ENCRYPTED WITH (DEK = dek_crypto, SUPPORT_ALGO = 'DET'),
    col_varchar VARCHAR_ENC ENCRYPTED WITH (DEK = dek_crypto, SUPPORT_ALGO = 'DET'),
    col_bpchar  BPCHAR_ENC  ENCRYPTED WITH (DEK = dek_crypto, SUPPORT_ALGO = 'DET'),
    col_text    TEXT_ENC    ENCRYPTED WITH (DEK = dek_crypto, SUPPORT_ALGO = 'DET')
);

-- CASE 1: Full row all 9 types
-- @param int 1
-- @param String Alice
-- @param String Test user Alice
-- @param int 100
-- @param int 100000
-- @param long 9999999999
-- @param BigDecimal 3.14
-- @param BigDecimal 3.14159265358979
-- @param BigDecimal 123456.78
-- @param String hello_varchar
-- @param String hello_bpchar
-- @param String This is a long text for testing encryption
INSERT INTO tecc_all_types_test (
    id, name, is_active, remark,
    col_int2, col_int4, col_int8,
    col_float4, col_float8, col_numeric,
    col_varchar, col_bpchar, col_text
) VALUES (
    ?, ?, true, ?,
    ?, ?, ?,
    ?, ?, ?,
    ?, ?, ?
);

-- CASE 2: Second row different values
-- @param int 2
-- @param String Bob
-- @param String Test user Bob
-- @param int -100
-- @param int -100000
-- @param long -9999999999
-- @param BigDecimal -3.14
-- @param BigDecimal -3.14159265358979
-- @param BigDecimal -123456.78
-- @param String world_varchar
-- @param String world_bpchar
-- @param String Another piece of text for testing
INSERT INTO tecc_all_types_test (
    id, name, is_active, remark,
    col_int2, col_int4, col_int8,
    col_float4, col_float8, col_numeric,
    col_varchar, col_bpchar, col_text
) VALUES (
    ?, ?, false, ?,
    ?, ?, ?,
    ?, ?, ?,
    ?, ?, ?
);

-- CASE 3: Zero values
-- @param int 3
-- @param String Charlie
-- @param int 0
-- @param int 0
-- @param int 0
-- @param BigDecimal 0.0
-- @param BigDecimal 0.0
-- @param int 0
-- @param String 
-- @param String 
-- @param String 
INSERT INTO tecc_all_types_test (
    id, name, col_int2, col_int4, col_int8,
    col_float4, col_float8, col_numeric, col_varchar, col_bpchar, col_text
) VALUES (
    ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?
);

-- CASE 4: Max integer values
-- @param int 4
-- @param String David
-- @param int 32767
-- @param int 2147483647
-- @param long 9223372036854775807
-- @param BigDecimal 1e10
-- @param BigDecimal 1e100
-- @param BigDecimal 999999999999.99
-- @param String short
-- @param String CHR
-- @param String A
INSERT INTO tecc_all_types_test (
    id, name, col_int2, col_int4, col_int8,
    col_float4, col_float8, col_numeric, col_varchar, col_bpchar, col_text
) VALUES (
    ?, ?, ?, ?, ?,
    ?, ?, ?, ?, ?, ?
);

-- CASE 5: Min integer values
-- @param int 5
-- @param String Eve
-- @param int -32768
-- @param int -2147483648
-- @param long -9223372036854775808
-- @param BigDecimal -1e10
-- @param BigDecimal -1e100
-- @param BigDecimal -999999999999.99
-- @param String a
-- @param String Z
-- @param String !@#$%^&*()
INSERT INTO tecc_all_types_test (
    id, name, col_int2, col_int4, col_int8,
    col_float4, col_float8, col_numeric, col_varchar, col_bpchar, col_text
) VALUES (
    ?, ?, ?, ?, ?,
    ?, ?, ?, ?, ?, ?
);

-- CASE 6: Long strings
-- @param int 6
-- @param String Frank
-- @param String x
-- @param int 200
-- @param String y
-- @param int 200
-- @param String longtext
-- @param int 100
INSERT INTO tecc_all_types_test (
    id, name, col_varchar, col_bpchar, col_text
) VALUES (
    ?, ?, repeat(?, ?), repeat(?, ?), repeat(?, ?)
);

-- CASE 7: Tiny numeric
-- @param int 7
-- @param String Grace
-- @param BigDecimal 0.00000001
INSERT INTO tecc_all_types_test (id, name, col_numeric) VALUES (?, ?, ?);

-- CASE 8: Plaintext-only row
-- @param int 8
-- @param String Heidi
-- @param String plain only
INSERT INTO tecc_all_types_test (id, name, is_active, remark) VALUES (?, ?, true, ?);

-- CASE 9: Equality on plain id
-- @param int 1
SELECT 'c9_eq_plain_id', id, name, col_int4, col_varchar FROM tecc_all_types_test WHERE id = ?;

-- CASE 10: Equality on encrypted col_int4
-- @param int 100000
SELECT 'c10_eq_enc_int4', id, name, col_int4, col_varchar FROM tecc_all_types_test WHERE col_int4 = ?;

-- CASE 11: Equality on encrypted col_varchar
-- @param String hello_varchar
SELECT 'c11_eq_enc_varchar', id, name, col_varchar FROM tecc_all_types_test WHERE col_varchar = ?;

-- CASE 12: Equality on encrypted col_numeric
-- @param BigDecimal 123456.78
SELECT 'c12_eq_enc_numeric', id, name, col_numeric FROM tecc_all_types_test WHERE col_numeric = ?;

-- CASE 13: Combined condition (plain + encrypted)
-- @param long 9999999999
SELECT 'c13_combined', id, name, col_int8, col_float8 FROM tecc_all_types_test
WHERE is_active = true AND col_int8 = ?;

-- CASE 14: Equality on encrypted col_text
-- @param String This is a long text for testing encryption
SELECT 'c14_eq_enc_text', id, name, col_text FROM tecc_all_types_test WHERE col_text = ?;

-- CASE 15: UPDATE encrypted by plain id
-- @param int 200000
-- @param String updated_varchar
-- @param int 1
UPDATE tecc_all_types_test SET col_int4 = ?, col_varchar = ? WHERE id = ?;

-- @param int 1
SELECT 'c15_update_by_plain', id, name, col_int4, col_varchar FROM tecc_all_types_test WHERE id = ?;

-- CASE 16: UPDATE plain by encrypted
-- @param String Alice_Updated
-- @param String Updated by CASE 16
-- @param int 200000
UPDATE tecc_all_types_test SET name = ?, remark = ? WHERE col_int4 = ?;

-- @param int 200000
SELECT 'c16_update_by_enc', id, name, remark, col_int4 FROM tecc_all_types_test WHERE col_int4 = ?;

-- CASE 17: UPDATE multiple encrypted by combined condition
-- @param int 999
-- @param BigDecimal 99.99
-- @param BigDecimal 88888.88
-- @param String Updated text after CASE 17
-- @param String world_varchar
UPDATE tecc_all_types_test SET col_int2 = ?, col_float4 = ?, col_numeric = ?,
    col_text = ?
WHERE is_active = false AND col_varchar = ?;

-- @param int 999
SELECT 'c17_update_multi', id, name, col_int2, col_float4, col_numeric, col_text FROM tecc_all_types_test WHERE col_int2 = ?;

-- CASE 18: All rows overview
SELECT 'c18_all_rows', id, name, col_int2, col_int4, col_int8 FROM tecc_all_types_test ORDER BY id;

-- CASE 19: Single row all 9 encrypted types
-- @param int 2
SELECT 'c19_all9_single', col_int2, col_int4, col_int8, col_float4, col_float8, col_numeric,
    col_varchar, col_bpchar, col_text FROM tecc_all_types_test WHERE id = ?;

-- CASE 20: Equality on encrypted col_bpchar
-- @param String hello_bpchar
SELECT 'c20_eq_bpchar', id, name, col_bpchar FROM tecc_all_types_test WHERE col_bpchar = ?;

DROP TABLE IF EXISTS tecc_all_types_test CASCADE;

-- ============================================================
-- Cleanup
-- ============================================================
SELECT '=== Cleanup ===';

DROP KEK IF EXISTS kek_crypto CASCADE;
