-- ============================================================
-- enc_types.sql — 密态类型与转换端到端测试
-- ============================================================

-- Setup
DROP TABLE IF EXISTS enc_types_t CASCADE;
DROP TABLE IF EXISTS enc_types_def CASCADE;
DROP DEK IF EXISTS dek_enc_types;
DROP KEK IF EXISTS kek_enc_types;

CREATE KEK kek_enc_types WITH (
    key_store = 'localkms',
    key_path = '/tmp/kek_enc_types.key'
);

CREATE DEK dek_enc_types WITH VALUES(
    KEK = kek_enc_types
);

-- ============================================================
-- 1. CREATE TABLE — 9种密态类型
-- ============================================================
SELECT '=== 1 CREATE TABLE (9 types) ===';

CREATE TABLE enc_types_t (
    id        SERIAL PRIMARY KEY,
    c_int2    INT2_ENC    ENCRYPTED WITH (DEK = 'dek_enc_types', SUPPORT_ALGO = 'det'),
    c_int4    INT4_ENC    ENCRYPTED WITH (DEK = 'dek_enc_types', SUPPORT_ALGO = 'det'),
    c_int8    INT8_ENC    ENCRYPTED WITH (DEK = 'dek_enc_types', SUPPORT_ALGO = 'det'),
    c_float4  FLOAT4_ENC  ENCRYPTED WITH (DEK = 'dek_enc_types', SUPPORT_ALGO = 'det'),
    c_float8  FLOAT8_ENC  ENCRYPTED WITH (DEK = 'dek_enc_types', SUPPORT_ALGO = 'det'),
    c_numeric NUMERIC_ENC ENCRYPTED WITH (DEK = 'dek_enc_types', SUPPORT_ALGO = 'det'),
    c_varchar VARCHAR_ENC ENCRYPTED WITH (DEK = 'dek_enc_types', SUPPORT_ALGO = 'det'),
    c_bpchar  BPCHAR_ENC  ENCRYPTED WITH (DEK = 'dek_enc_types', SUPPORT_ALGO = 'det'),
    c_text    TEXT_ENC    ENCRYPTED WITH (DEK = 'dek_enc_types', SUPPORT_ALGO = 'det')
);

-- ============================================================
-- 2. INSERT
-- ============================================================
SELECT '=== 2 INSERT row 1 ===';
INSERT INTO enc_types_t (c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric, c_varchar, c_bpchar, c_text)
VALUES (10, 100, 1000, 1.5, 3.14159, 123.45, 'hello', 'bpchar1', 'text1');

SELECT '=== 2 INSERT row 2 ===';
INSERT INTO enc_types_t (c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric, c_varchar, c_bpchar, c_text)
VALUES (20, 200, 2000, 2.5, 6.28318, 678.90, 'world', 'bpchar2', 'text2');

SELECT '=== 2 INSERT row 3 ===';
INSERT INTO enc_types_t (c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric, c_varchar, c_bpchar, c_text)
VALUES (30, 300, 3000, 3.5, 9.42477, 111.22, 'abc', 'bpchar3', 'text3');

-- ============================================================
-- 3. SELECT (解密验证)
-- ============================================================
SELECT '=== 3 SELECT all types ===';
SELECT id, c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric,
       c_varchar, c_bpchar, c_text
FROM enc_types_t ORDER BY id;

SELECT '=== 3 SELECT numeric types ===';
SELECT c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric
FROM enc_types_t ORDER BY id;

SELECT '=== 3 SELECT string types ===';
SELECT c_varchar, c_bpchar, c_text
FROM enc_types_t ORDER BY id;

-- ============================================================
-- 4. UPDATE
-- ============================================================
SELECT '=== 4 UPDATE int4_enc ===';
UPDATE enc_types_t SET c_int4 = 999 WHERE id = 1;
SELECT id, c_int4 FROM enc_types_t WHERE id = 1;

SELECT '=== 4 UPDATE text_enc ===';
UPDATE enc_types_t SET c_text = 'updated' WHERE id = 2;
SELECT id, c_text FROM enc_types_t WHERE id = 2;

-- ============================================================
-- 5. DELETE
-- ============================================================
SELECT '=== 5 DELETE ===';
INSERT INTO enc_types_t (c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric, c_varchar, c_bpchar, c_text)
VALUES (99, 999, 9999, 9.9, 99.99, 999.99, 'del', 'del', 'del_text');
DELETE FROM enc_types_t WHERE c_int2 = 99;
SELECT count(*) AS cnt FROM enc_types_t;

-- ============================================================
-- 6. DEFAULT values
-- ============================================================
SELECT '=== 6 DEFAULT int4_enc ===';
CREATE TABLE enc_types_def (
    id   INT4 DEFAULT 1,
    val  INT4_ENC ENCRYPTED WITH (DEK = 'dek_enc_types') DEFAULT 100,
    name TEXT
);
INSERT INTO enc_types_def (name) VALUES ('row1');
INSERT INTO enc_types_def (val, name) VALUES (200, 'row2');
SELECT id, val, name FROM enc_types_def ORDER BY id;
DROP TABLE enc_types_def;

SELECT '=== 6 DEFAULT text_enc ===';
CREATE TABLE enc_types_def (
    val  INT4_ENC ENCRYPTED WITH (DEK = 'dek_enc_types') DEFAULT 99,
    note TEXT_ENC ENCRYPTED WITH (DEK = 'dek_enc_types') DEFAULT 'hello'
);
INSERT INTO enc_types_def (val, note) VALUES (100, 'world');
INSERT INTO enc_types_def (note) VALUES ('only_default');
SELECT val, note FROM enc_types_def;
DROP TABLE enc_types_def;

-- ============================================================
-- 7. 支持的类型转换 (隐式转换)
-- ============================================================
SELECT '=== 7 int2 -> int4 ===';
SELECT 1::int2_enc encrypted with (dek = 'dek_enc_types')::int4_enc encrypted with (dek = 'dek_enc_types');

SELECT '=== 7 int2 -> int8 ===';
SELECT 1::int2_enc encrypted with (dek = 'dek_enc_types')::int8_enc encrypted with (dek = 'dek_enc_types');

SELECT '=== 7 text -> varchar ===';
SELECT 'hello'::text_enc encrypted with (dek = 'dek_enc_types')::varchar_enc encrypted with (dek = 'dek_enc_types');

SELECT '=== 7 text -> bpchar ===';
SELECT 'hello'::text_enc encrypted with (dek = 'dek_enc_types')::bpchar_enc encrypted with (dek = 'dek_enc_types');

SELECT '=== 7 varchar -> text ===';
SELECT 'hello'::varchar_enc encrypted with (dek = 'dek_enc_types')::text_enc encrypted with (dek = 'dek_enc_types');

SELECT '=== 7 varchar -> bpchar ===';
SELECT 'hello'::varchar_enc encrypted with (dek = 'dek_enc_types')::bpchar_enc encrypted with (dek = 'dek_enc_types');

-- ============================================================
-- 8. 非法链式TypeCast拒绝（应全部报错）
-- ============================================================
SELECT '=== 8 Illegal chain TypeCast (expected errors) ===';

-- 7893: text/varchar/bpchar → int2/int4/int8 (all should error)
SELECT '8a_text_to_int2';
SELECT '222'::text_enc encrypted with (dek = 'dek_enc_types')::int2_enc encrypted with (dek = 'dek_enc_types');
SELECT '8b_text_to_int4';
SELECT '222'::text_enc encrypted with (dek = 'dek_enc_types')::int4_enc encrypted with (dek = 'dek_enc_types');
SELECT '8c_text_to_int8';
SELECT '222'::text_enc encrypted with (dek = 'dek_enc_types')::int8_enc encrypted with (dek = 'dek_enc_types');

SELECT '8d_varchar_to_int2';
SELECT '222'::varchar_enc encrypted with (dek = 'dek_enc_types')::int2_enc encrypted with (dek = 'dek_enc_types');
SELECT '8e_varchar_to_int4';
SELECT '222'::varchar_enc encrypted with (dek = 'dek_enc_types')::int4_enc encrypted with (dek = 'dek_enc_types');
SELECT '8f_varchar_to_int8';
SELECT '222'::varchar_enc encrypted with (dek = 'dek_enc_types')::int8_enc encrypted with (dek = 'dek_enc_types');

SELECT '8g_bpchar_to_int2';
SELECT '222'::bpchar_enc encrypted with (dek = 'dek_enc_types')::int2_enc encrypted with (dek = 'dek_enc_types');
SELECT '8h_bpchar_to_int4';
SELECT '222'::bpchar_enc encrypted with (dek = 'dek_enc_types')::int4_enc encrypted with (dek = 'dek_enc_types');
SELECT '8i_bpchar_to_int8';
SELECT '222'::bpchar_enc encrypted with (dek = 'dek_enc_types')::int8_enc encrypted with (dek = 'dek_enc_types');

-- 7894: text/varchar/bpchar → float4/float8 (all should error)
SELECT '8j_text_to_float4';
SELECT '222'::text_enc encrypted with (dek = 'dek_enc_types')::float4_enc encrypted with (dek = 'dek_enc_types');
SELECT '8k_text_to_float8';
SELECT '222'::text_enc encrypted with (dek = 'dek_enc_types')::float8_enc encrypted with (dek = 'dek_enc_types');

SELECT '8l_varchar_to_float4';
SELECT '222'::varchar_enc encrypted with (dek = 'dek_enc_types')::float4_enc encrypted with (dek = 'dek_enc_types');
SELECT '8m_varchar_to_float8';
SELECT '222'::varchar_enc encrypted with (dek = 'dek_enc_types')::float8_enc encrypted with (dek = 'dek_enc_types');

SELECT '8n_bpchar_to_float4';
SELECT '222'::bpchar_enc encrypted with (dek = 'dek_enc_types')::float4_enc encrypted with (dek = 'dek_enc_types');
SELECT '8o_bpchar_to_float8';
SELECT '222'::bpchar_enc encrypted with (dek = 'dek_enc_types')::float8_enc encrypted with (dek = 'dek_enc_types');

-- 7897: encrypted → plain type rejection (all 9 types)
SELECT '8p_int2_to_plain';
SELECT 1::int2_enc encrypted with (dek = 'dek_enc_types')::int2;

SELECT '8q_int4_to_plain';
SELECT 1::int4_enc encrypted with (dek = 'dek_enc_types')::int4;

SELECT '8r_int8_to_plain';
SELECT 1::int8_enc encrypted with (dek = 'dek_enc_types')::int8;

SELECT '8s_float4_to_plain';
SELECT 1.5::float4_enc encrypted with (dek = 'dek_enc_types')::float4;

SELECT '8t_float8_to_plain';
SELECT 1.5::float8_enc encrypted with (dek = 'dek_enc_types')::float8;

SELECT '8u_numeric_to_plain';
SELECT 1.5::numeric_enc encrypted with (dek = 'dek_enc_types')::numeric;

SELECT '8v_varchar_to_plain';
SELECT 'hello'::varchar_enc encrypted with (dek = 'dek_enc_types')::varchar;

SELECT '8w_bpchar_to_plain';
SELECT 'hello'::bpchar_enc encrypted with (dek = 'dek_enc_types')::bpchar;

SELECT '8x_text_to_plain';
SELECT 'hello'::text_enc encrypted with (dek = 'dek_enc_types')::text;

-- ============================================================
-- 9. VIEW with encrypted table (plain columns only)
-- ============================================================
SELECT '=== 9 VIEW ===';

CREATE VIEW enc_types_v AS SELECT id, c_int2, c_int4 FROM enc_types_t;
SELECT id, c_int2, c_int4 FROM enc_types_v ORDER BY id;
DROP VIEW enc_types_v;

-- ============================================================
-- Cleanup
-- ============================================================
SELECT '=== cleanup ===';
DROP TABLE enc_types_t;
DROP DEK dek_enc_types;
DROP KEK kek_enc_types;
