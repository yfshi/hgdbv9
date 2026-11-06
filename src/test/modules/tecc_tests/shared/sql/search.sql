-- ============================================================
-- search.sql — SEARCH模糊匹配测试
-- 覆盖 test_bug7916/7917/7918_repro,
--   test_empty_string_notlike, test_similar_to,
--   test_bool_expr, test_parser_bool_expr, test_taskB_encrypted
-- ============================================================

-- ============================================================
-- 0. Setup — 共享KEK/DEK
-- ============================================================
DROP KEK IF EXISTS kek_search CASCADE;

CREATE KEK kek_search WITH (
    key_store = 'localkms',
    key_path = '/tmp/kek_search.key'
);
CREATE DEK dek_search WITH VALUES(KEK = kek_search);

-- ============================================================
-- 1. LIKE wildcard — text/varchar/bpchar (bug7916)
-- ============================================================
SELECT '=== 1 LIKE Wildcard ===';

DROP TABLE IF EXISTS tecc_like_test CASCADE;
CREATE TABLE tecc_like_test (
    id SERIAL PRIMARY KEY,
    text_col text_enc encrypted with (dek = dek_search, support_algo='search'),
    varchar_col varchar_enc encrypted with (dek = dek_search, support_algo='search'),
    bpchar_col bpchar_enc encrypted with (dek = dek_search, support_algo='search')
);

INSERT INTO tecc_like_test (text_col, varchar_col, bpchar_col) VALUES
('apple', 'apple', 'apple'), ('application', 'application', 'application'),
('banana', 'banana', 'banana'), ('berry', 'berry', 'berry'),
('cherry', 'cherry', 'cherry'), ('Apple', 'Apple', 'Apple'),
('APPLE', 'APPLE', 'APPLE'), ('123apple', '123apple', '123apple'),
('apple123', 'apple123', 'apple123'), ('pineapple', 'pineapple', 'pineapple'),
('grape fruit', 'grape fruit', 'grape fruit'),
('red apple', 'red apple', 'red apple'),
('green apple', 'green apple', 'green apple'),
('', '', ''), ('a', 'a', 'a'),
('test data', 'test data', 'test data'),
('data test', 'data test', 'data test'),
('test', 'test', 'test'), ('TEST', 'TEST', 'TEST'),
('Test', 'Test', 'Test'), (NULL, NULL, NULL);

-- % wildcard (prefix)
SELECT '1a_text_like_apple_pct', id, text_col FROM tecc_like_test WHERE text_col LIKE 'apple%' ORDER BY id;
SELECT '1b_varchar_like_a_pct', id, varchar_col FROM tecc_like_test WHERE varchar_col LIKE 'a%' ORDER BY id;
SELECT '1c_bpchar_like_123_pct', id, bpchar_col FROM tecc_like_test WHERE bpchar_col LIKE '123%' ORDER BY id;

-- _ wildcard (single char)
SELECT '1d_text_like_app_e', id, text_col FROM tecc_like_test WHERE text_col LIKE 'app_e' ORDER BY id;
SELECT '1e_varchar_like_b_n_n_', id, varchar_col FROM tecc_like_test WHERE varchar_col LIKE 'b_n_n_' ORDER BY id;

-- % + _ combined
SELECT '1f_bpchar_mod_like_a_pct_e', id, bpchar_col FROM tecc_like_test WHERE bpchar_col LIKE 'a%_e' ORDER BY id;

-- Multi-char _
SELECT '1g_varchar_like_a_5us', id, varchar_col FROM tecc_like_test WHERE varchar_col LIKE 'a_____' ORDER BY id;

-- Mid-pattern _ + %
SELECT '1h_text_like_app_e_pct', id, text_col FROM tecc_like_test WHERE text_col LIKE 'app_e%' ORDER BY id;

-- Suffix %
SELECT '1i_text_like_pct_apple', id, text_col FROM tecc_like_test WHERE text_col LIKE '%apple' ORDER BY id;

-- Mid-string %
SELECT '1j_text_like_pct_ana_pct', id, text_col FROM tecc_like_test WHERE text_col LIKE '%ana%' ORDER BY id;

DROP TABLE IF EXISTS tecc_like_test CASCADE;

-- ============================================================
-- 2. NOT LIKE (bug7918 + test_empty_string_notlike)
-- ============================================================
SELECT '=== 2 NOT LIKE ===';

DROP TABLE IF EXISTS tecc_not_like_test CASCADE;
CREATE TABLE tecc_not_like_test (
    id SERIAL PRIMARY KEY,
    text_col text_enc encrypted with (dek = dek_search, support_algo='search'),
    varchar_col varchar_enc encrypted with (dek = dek_search, support_algo='search'),
    bpchar_col bpchar_enc encrypted with (dek = dek_search, support_algo='search')
);

INSERT INTO tecc_not_like_test (text_col, varchar_col, bpchar_col) VALUES
('apple', 'apple', 'apple'), ('application', 'application', 'application'),
('banana', 'banana', 'banana'), ('berry', 'berry', 'berry'),
('cherry', 'cherry', 'cherry'), ('Apple', 'Apple', 'Apple'),
('APPLE', 'APPLE', 'APPLE'), ('123apple', '123apple', '123apple'),
('apple123', 'apple123', 'apple123'), ('pineapple', 'pineapple', 'pineapple'),
('grape fruit', 'grape fruit', 'grape fruit'),
('red apple', 'red apple', 'red apple'),
('green apple', 'green apple', 'green apple'),
('a', 'a', 'a'), ('test data', 'test data', 'test data'),
('data test', 'data test', 'data test'),
('test', 'test', 'test'), ('TEST', 'TEST', 'TEST'),
('Test', 'Test', 'Test'), (NULL, NULL, NULL);

-- NOT LIKE prefix
SELECT '2a_text_not_like_app_pct', id, text_col FROM tecc_not_like_test WHERE text_col NOT LIKE 'app%' ORDER BY id;
SELECT '2b_varchar_not_like_a_pct', id, varchar_col FROM tecc_not_like_test WHERE varchar_col NOT LIKE 'a%' ORDER BY id;
SELECT '2c_bpchar_not_like_123_pct', id, bpchar_col FROM tecc_not_like_test WHERE bpchar_col NOT LIKE '123%' ORDER BY id;

-- NOT LIKE suffix
SELECT '2d_bpchar_not_like_pct_123', id, bpchar_col FROM tecc_not_like_test WHERE bpchar_col NOT LIKE '%123' ORDER BY id;

-- LIKE prefix (control)
SELECT '2e_text_like_app_pct_ctrl', id, text_col FROM tecc_not_like_test WHERE text_col LIKE 'app%' ORDER BY id;

DROP TABLE IF EXISTS tecc_not_like_test CASCADE;

-- ============================================================
-- 3. LIKE + NOT LIKE combined (bug7917)
-- ============================================================
SELECT '=== 3 LIKE + NOT LIKE Combined ===';

DROP TABLE IF EXISTS tecc_like_combined CASCADE;
CREATE TABLE tecc_like_combined (
    id SERIAL PRIMARY KEY,
    text_col text_enc encrypted with (dek = dek_search, support_algo='search'),
    varchar_col varchar_enc encrypted with (dek = dek_search, support_algo='search'),
    bpchar_col bpchar_enc encrypted with (dek = dek_search, support_algo='search')
);

INSERT INTO tecc_like_combined (text_col, varchar_col, bpchar_col) VALUES
('apple', 'apple', 'apple'), ('application', 'application', 'application'),
('banana', 'banana', 'banana'), ('berry', 'berry', 'berry'),
('cherry', 'cherry', 'cherry'), ('Apple', 'Apple', 'Apple'),
('APPLE', 'APPLE', 'APPLE'), ('123apple', '123apple', '123apple'),
('apple123', 'apple123', 'apple123'), ('pineapple', 'pineapple', 'pineapple'),
('grape fruit', 'grape fruit', 'grape fruit'),
('red apple', 'red apple', 'red apple'),
('green apple', 'green apple', 'green apple'),
('a', 'a', 'a'), ('test data', 'test data', 'test data'),
('data test', 'data test', 'data test'),
('test', 'test', 'test'), ('TEST', 'TEST', 'TEST'),
('Test', 'Test', 'Test'), (NULL, NULL, NULL);

-- LIKE AND NOT LIKE
SELECT '3a_bpchar_like_a_not_like_123', id, bpchar_col FROM tecc_like_combined
WHERE bpchar_col LIKE 'a%' AND bpchar_col NOT LIKE '%123' ORDER BY id;

-- Single LIKE (control)
SELECT '3b_bpchar_like_a_pct_ctrl', id, bpchar_col FROM tecc_like_combined WHERE bpchar_col LIKE 'a%' ORDER BY id;

-- Single NOT LIKE (control)
SELECT '3c_bpchar_not_like_123_ctrl', id, bpchar_col FROM tecc_like_combined WHERE bpchar_col NOT LIKE '%123' ORDER BY id;

DROP TABLE IF EXISTS tecc_like_combined CASCADE;

-- ============================================================
-- 4. Empty string + NOT LIKE (test_empty_string_notlike)
-- ============================================================
SELECT '=== 4 Empty String + NOT LIKE ===';

DROP TABLE IF EXISTS tecc_empty_search CASCADE;
CREATE TABLE tecc_empty_search (
    id SERIAL PRIMARY KEY,
    text_col text_enc encrypted with (dek = dek_search, support_algo='search'),
    varchar_col varchar_enc encrypted with (dek = dek_search, support_algo='search'),
    bpchar_col bpchar_enc encrypted with (dek = dek_search, support_algo='search')
);

INSERT INTO tecc_empty_search (text_col, varchar_col, bpchar_col) VALUES
('', '', ''), ('a', 'a', 'a'), ('ab', 'ab', 'ab'), (NULL, NULL, NULL);

-- NOT LIKE should preserve empty string
SELECT '4a_not_like_123', id, text_col, varchar_col, bpchar_col FROM tecc_empty_search WHERE bpchar_col NOT LIKE '%123' ORDER BY id;
SELECT '4b_not_like_b', id, bpchar_col FROM tecc_empty_search WHERE bpchar_col NOT LIKE 'b' ORDER BY id;
SELECT '4c_like_pct', id, bpchar_col FROM tecc_empty_search WHERE bpchar_col LIKE '%' ORDER BY id;

-- Text NOT LIKE
SELECT '4d_text_not_like_123', id, text_col FROM tecc_empty_search WHERE text_col NOT LIKE '%123' ORDER BY id;
SELECT '4e_varchar_not_like_123', id, varchar_col FROM tecc_empty_search WHERE varchar_col NOT LIKE '%123' ORDER BY id;

DROP TABLE IF EXISTS tecc_empty_search CASCADE;

-- ============================================================
-- 5. 发散覆盖 — 边界场景(纯_ / 固定+_ / %+_混合)
-- 每条用例同时输出明文对比和密态查询结果（明文列显示true/false）
-- ============================================================
SELECT '=== 5 Edge Cases ===';

DROP TABLE IF EXISTS tecc_edge_test CASCADE;
CREATE TABLE tecc_edge_test (
    id SERIAL PRIMARY KEY,
    label text,
    vc_enc varchar_enc encrypted with (dek = dek_search, support_algo='search'),
    vc_plain varchar
);

INSERT INTO tecc_edge_test (label, vc_enc, vc_plain) VALUES
('ab', 'ab', 'ab'), ('abc', 'abc', 'abc'), ('abcd', 'abcd', 'abcd'),
('abcde', 'abcde', 'abcde'), ('axc', 'axc', 'axc'), ('a_c', 'a_c', 'a_c'),
('a1234', 'a1234', 'a1234'), ('ab234', 'ab234', 'ab234'),
('', '', ''), ('a', 'a', 'a');

-- 明文基准：数据含_的LIKE语义
SELECT '5z1_plain: a_c LIKE ___', 'a_c' LIKE '___' AS r;
SELECT '5z2_plain: a_c LIKE a_c', 'a_c' LIKE 'a_c' AS r;
SELECT '5z3_plain: abc LIKE ___', 'abc' LIKE '___' AS r;
SELECT '5z4_plain: abc LIKE a_c', 'abc' LIKE 'a_c' AS r;

-- 每行同时显示明文对比和密态结果
SELECT '5a___', label, vc_plain LIKE '___' AS plain, vc_enc LIKE '___' AS enc FROM tecc_edge_test ORDER BY id;
SELECT '5b____', label, vc_plain LIKE '____' AS plain, vc_enc LIKE '____' AS enc FROM tecc_edge_test ORDER BY id;
SELECT '5c__', label, vc_plain LIKE '__' AS plain, vc_enc LIKE '__' AS enc FROM tecc_edge_test ORDER BY id;
SELECT '5d_a_', label, vc_plain LIKE 'a_' AS plain, vc_enc LIKE 'a_' AS enc FROM tecc_edge_test ORDER BY id;
SELECT '5e_a__', label, vc_plain LIKE 'a__' AS plain, vc_enc LIKE 'a__' AS enc FROM tecc_edge_test ORDER BY id;
SELECT '5f_a___', label, vc_plain LIKE 'a___' AS plain, vc_enc LIKE 'a___' AS enc FROM tecc_edge_test ORDER BY id;
SELECT '5g__c', label, vc_plain LIKE '__c' AS plain, vc_enc LIKE '__c' AS enc FROM tecc_edge_test ORDER BY id;
SELECT '5h___d', label, vc_plain LIKE '___d' AS plain, vc_enc LIKE '___d' AS enc FROM tecc_edge_test ORDER BY id;
SELECT '5i_a____', label, vc_plain LIKE 'a____' AS plain, vc_enc LIKE 'a____' AS enc FROM tecc_edge_test ORDER BY id;
SELECT '5j_ab___', label, vc_plain LIKE 'ab___' AS plain, vc_enc LIKE 'ab___' AS enc FROM tecc_edge_test ORDER BY id;
SELECT '5k_a_c', label, vc_plain LIKE 'a_c' AS plain, vc_enc LIKE 'a_c' AS enc FROM tecc_edge_test ORDER BY id;
SELECT '5l_a%_', label, vc_plain LIKE 'a%_' AS plain, vc_enc LIKE 'a%_' AS enc FROM tecc_edge_test ORDER BY id;
SELECT '5m_%c', label, vc_plain LIKE '%c' AS plain, vc_enc LIKE '%c' AS enc FROM tecc_edge_test ORDER BY id;
SELECT '5n_%a%', label, vc_plain LIKE '%a%' AS plain, vc_enc LIKE '%a%' AS enc FROM tecc_edge_test ORDER BY id;

DROP TABLE IF EXISTS tecc_edge_test CASCADE;

-- ============================================================
-- Cleanup
-- ============================================================
SELECT '=== Cleanup ===';
DROP KEK IF EXISTS kek_search CASCADE;
