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

INSERT INTO test_nan VALUES (1, 'NaN', 'NaN', 'NaN');
INSERT INTO test_nan VALUES (2, 'Infinity', 'Infinity', 'NaN');
INSERT INTO test_nan VALUES (3, '-Infinity', '-Infinity', 'NaN');
INSERT INTO test_nan VALUES (4, 1.5, 2.5, 3.5);

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

INSERT INTO test_empty (enc_text, enc_varchar, enc_bpchar, plain_text, descr)
    VALUES ('', '', '', '', 'all empty string');
INSERT INTO test_empty (enc_text, enc_varchar, enc_bpchar, plain_text, descr)
    VALUES (NULL, NULL, NULL, NULL, 'all NULL');
INSERT INTO test_empty (enc_text, enc_varchar, enc_bpchar, plain_text, descr)
    VALUES ('hello', 'hello', 'hello', 'hello', 'normal');

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
INSERT INTO test_float4_prec (f4, f4_plain, descr) VALUES
    (3.14159265, 3.14159265, 'pi 8-digit'), (2.71828182, 2.71828182, 'e 8-digit'),
    (123.456789, 123.456789, 'decimal'), (-123.456789, -123.456789, 'neg decimal');
SELECT id, f4, f4_plain, descr FROM test_float4_prec ORDER BY id;
DROP TABLE IF EXISTS test_float4_prec CASCADE;

-- 3b: float8_enc precision (bug7836) — 与普通float8对比
DROP TABLE IF EXISTS test_float8_prec CASCADE;
CREATE TABLE test_float8_prec (id SERIAL, f8 float8_enc encrypted with (dek = dek_crypto, support_algo = 'det'), f8_plain float8, descr TEXT);
INSERT INTO test_float8_prec (f8, f8_plain, descr) VALUES
    (3.141592653589793, 3.141592653589793, 'pi 15-digit'), (2.718281828459045, 2.718281828459045, 'e 15-digit'),
    (123.456789012345, 123.456789012345, 'decimal'), (-123.456789012345, -123.456789012345, 'neg decimal');
SELECT id, f8, f8_plain, descr FROM test_float8_prec ORDER BY id;
DROP TABLE IF EXISTS test_float8_prec CASCADE;

-- 3c: numeric_enc precision (bug7836) — 与普通numeric对比
DROP TABLE IF EXISTS test_numeric_prec CASCADE;
CREATE TABLE test_numeric_prec (id SERIAL, n numeric_enc encrypted with (dek = dek_crypto, support_algo = 'det'), n_plain numeric, descr TEXT);
INSERT INTO test_numeric_prec (n, n_plain, descr) VALUES
    (3.141592653589793, 3.141592653589793, 'pi 15-digit'), (2.718281828459045, 2.718281828459045, 'e 15-digit'),
    (123.456789012345, 123.456789012345, 'decimal'), (-0.0000001, -0.0000001, 'tiny neg');
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

INSERT INTO test_f8_limit (f8, f8_plain, descr) VALUES
    (1.7976931348623157e+308, 1.7976931348623157e+308, 'near float8 max'), (-1.7976931348623157e+308, -1.7976931348623157e+308, 'near float8 min'),
    (1.0e308, 1.0e308, 'big sci'), (1.0e-308, 1.0e-308, 'tiny sci'),
    (1.0e10, 1.0e10, 'sci large'), (1.0e-10, 1.0e-10, 'sci small');

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

INSERT INTO test_f4_range (enc_val, plain_val, descr) VALUES
(0.0, 0.0, 'zero'), (-1.0, -1.0, 'neg one'), (-123.456, -123.456, 'neg decimal'),
(-0.0000001, -0.0000001, 'tiny neg'), (-1.0e-10, -1.0e-10, 'tiny neg sci'),
(1.0, 1.0, 'one'), (3.14159265, 3.14159265, 'pi'), (0.0000001, 0.0000001, 'tiny pos'),
(1.0e10, 1.0e10, 'large');

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
INSERT INTO test_f8_range (enc_val, plain_val, descr) VALUES
(0.0, 0.0, 'zero'), (-1.0, -1.0, 'neg one'), (-123.456, -123.456, 'neg decimal'),
(-0.0000001, -0.0000001, 'tiny neg'), (-1.0e-10, -1.0e-10, 'tiny neg sci'),
(1.0, 1.0, 'one'), (3.14159265, 3.14159265, 'pi'), (0.0000001, 0.0000001, 'tiny pos'),
(1.0e10, 1.0e10, 'large'), (1.2345678901234567, 1.2345678901234567, 'full precision');

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
INSERT INTO test_errmsg VALUES (1, 25);
INSERT INTO test_errmsg VALUES (2, 2147483648);  -- overflow, should error
DROP TABLE IF EXISTS test_errmsg CASCADE;

-- 4b: int4 out-of-range rejection (bug7826)
CREATE TABLE tb_int4 (
    id SERIAL PRIMARY KEY,
    int_col int4_enc encrypted with (dek = dek_crypto, support_algo='ope'),
    description TEXT
);

INSERT INTO tb_int4 (int_col, description) VALUES (0, 'zero');
INSERT INTO tb_int4 (int_col, description) VALUES (1, 'positive');
INSERT INTO tb_int4 (int_col, description) VALUES (-1, 'negative');
INSERT INTO tb_int4 (int_col, description) VALUES (2147483647, 'int4 max');
INSERT INTO tb_int4 (int_col, description) VALUES (-2147483648, 'int4 min');
INSERT INTO tb_int4 (int_col, description) VALUES (2147483648, 'exceed int4 max');  -- should error
INSERT INTO tb_int4 (int_col, description) VALUES (-2147483649, 'exceed int4 min');  -- should error
INSERT INTO tb_int4 (int_col, description) VALUES (4294967295, 'way exceed int4 max');  -- should error
INSERT INTO tb_int4 (int_col, description) VALUES (12.34, 'float');  -- should error

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
INSERT INTO test_employee_select (id, name, salary) VALUES (1, 'Alice', 50000.00);
SELECT id, name, salary FROM test_employee_select WHERE id = 1;
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

INSERT INTO test_emp VALUES (1, 'Alice', 50000, 'Engineering');
INSERT INTO test_emp VALUES (2, 'Bob', 60000, 'Sales');
INSERT INTO test_emp VALUES (3, 'Charlie', 70000, 'Engineering');
INSERT INTO test_emp VALUES (4, 'Diana', NULL, 'Marketing');

SELECT '5b1_select_all', id, name FROM test_emp ORDER BY id;
SELECT '5b2_eq', id, name FROM test_emp WHERE dept = 'Engineering';
SELECT '5b3_like', id, name FROM test_emp WHERE name LIKE 'A%';
SELECT '5b4_not_like', id, name FROM test_emp WHERE name NOT LIKE 'B%';
SELECT '5b5_cmp', id, name, salary FROM test_emp WHERE salary > 50000 ORDER BY id;

UPDATE test_emp SET salary = 75000 WHERE name = 'Charlie';
SELECT '5b6_update', id, name, salary FROM test_emp WHERE id = 3;

DELETE FROM test_emp WHERE name = 'Diana';
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
INSERT INTO tecc_all_types_test (
    id, name, is_active, remark,
    col_int2, col_int4, col_int8,
    col_float4, col_float8, col_numeric,
    col_varchar, col_bpchar, col_text
) VALUES (
    1, 'Alice', true, 'Test user Alice',
    100, 100000, 9999999999,
    3.14, 3.14159265358979, 123456.78,
    'hello_varchar', 'hello_bpchar', 'This is a long text for testing encryption'
);

-- CASE 2: Second row different values
INSERT INTO tecc_all_types_test (
    id, name, is_active, remark,
    col_int2, col_int4, col_int8,
    col_float4, col_float8, col_numeric,
    col_varchar, col_bpchar, col_text
) VALUES (
    2, 'Bob', false, 'Test user Bob',
    -100, -100000, -9999999999,
    -3.14, -3.14159265358979, -123456.78,
    'world_varchar', 'world_bpchar', 'Another piece of text for testing'
);

-- CASE 3: Zero values
INSERT INTO tecc_all_types_test (
    id, name, col_int2, col_int4, col_int8,
    col_float4, col_float8, col_numeric, col_varchar, col_bpchar, col_text
) VALUES (
    3, 'Charlie', 0, 0, 0, 0.0, 0.0, 0, '', '', ''
);

-- CASE 4: Max integer values
INSERT INTO tecc_all_types_test (
    id, name, col_int2, col_int4, col_int8,
    col_float4, col_float8, col_numeric, col_varchar, col_bpchar, col_text
) VALUES (
    4, 'David', 32767, 2147483647, 9223372036854775807,
    1e10, 1e100, 999999999999.99, 'short', 'CHR', 'A'
);

-- CASE 5: Min integer values
INSERT INTO tecc_all_types_test (
    id, name, col_int2, col_int4, col_int8,
    col_float4, col_float8, col_numeric, col_varchar, col_bpchar, col_text
) VALUES (
    5, 'Eve', -32768, -2147483648, -9223372036854775808,
    -1e10, -1e100, -999999999999.99, 'a', 'Z', '!@#$%^&*()'
);

-- CASE 6: Long strings
INSERT INTO tecc_all_types_test (
    id, name, col_varchar, col_bpchar, col_text
) VALUES (
    6, 'Frank', repeat('x', 200), repeat('y', 200), repeat('longtext', 100)
);

-- CASE 7: Tiny numeric
INSERT INTO tecc_all_types_test (id, name, col_numeric) VALUES (7, 'Grace', 0.00000001);

-- CASE 8: Plaintext-only row
INSERT INTO tecc_all_types_test (id, name, is_active, remark) VALUES (8, 'Heidi', true, 'plain only');

-- CASE 9: Equality on plain id
SELECT 'c9_eq_plain_id', id, name, col_int4, col_varchar FROM tecc_all_types_test WHERE id = 1;

-- CASE 10: Equality on encrypted col_int4
SELECT 'c10_eq_enc_int4', id, name, col_int4, col_varchar FROM tecc_all_types_test WHERE col_int4 = 100000;

-- CASE 11: Equality on encrypted col_varchar
SELECT 'c11_eq_enc_varchar', id, name, col_varchar FROM tecc_all_types_test WHERE col_varchar = 'hello_varchar';

-- CASE 12: Equality on encrypted col_numeric
SELECT 'c12_eq_enc_numeric', id, name, col_numeric FROM tecc_all_types_test WHERE col_numeric = 123456.78;

-- CASE 13: Combined condition (plain + encrypted)
SELECT 'c13_combined', id, name, col_int8, col_float8 FROM tecc_all_types_test
WHERE is_active = true AND col_int8 = 9999999999;

-- CASE 14: Equality on encrypted col_text
SELECT 'c14_eq_enc_text', id, name, col_text FROM tecc_all_types_test WHERE col_text = 'This is a long text for testing encryption';

-- CASE 15: UPDATE encrypted by plain id
UPDATE tecc_all_types_test SET col_int4 = 200000, col_varchar = 'updated_varchar' WHERE id = 1;
SELECT 'c15_update_by_plain', id, name, col_int4, col_varchar FROM tecc_all_types_test WHERE id = 1;

-- CASE 16: UPDATE plain by encrypted
UPDATE tecc_all_types_test SET name = 'Alice_Updated', remark = 'Updated by CASE 16' WHERE col_int4 = 200000;
SELECT 'c16_update_by_enc', id, name, remark, col_int4 FROM tecc_all_types_test WHERE col_int4 = 200000;

-- CASE 17: UPDATE multiple encrypted by combined condition
UPDATE tecc_all_types_test SET col_int2 = 999, col_float4 = 99.99, col_numeric = 88888.88,
    col_text = 'Updated text after CASE 17'
WHERE is_active = false AND col_varchar = 'world_varchar';
SELECT 'c17_update_multi', id, name, col_int2, col_float4, col_numeric, col_text FROM tecc_all_types_test WHERE col_int2 = 999;

-- CASE 18: All rows overview
SELECT 'c18_all_rows', id, name, col_int2, col_int4, col_int8 FROM tecc_all_types_test ORDER BY id;

-- CASE 19: Single row all 9 encrypted types
SELECT 'c19_all9_single', col_int2, col_int4, col_int8, col_float4, col_float8, col_numeric,
    col_varchar, col_bpchar, col_text FROM tecc_all_types_test WHERE id = 2;

-- CASE 20: Equality on encrypted col_bpchar
SELECT 'c20_eq_bpchar', id, name, col_bpchar FROM tecc_all_types_test WHERE col_bpchar = 'hello_bpchar';

DROP TABLE IF EXISTS tecc_all_types_test CASCADE;

-- ============================================================
-- Cleanup
-- ============================================================
SELECT '=== Cleanup ===';
DROP KEK IF EXISTS kek_crypto CASCADE;
