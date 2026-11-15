-- ============================================================
-- bpchar_compare.sql — bpchar_enc OPE 比较/范围/尾部空格测试
-- 覆盖场景:
--   - bpchar_enc ORDER BY（基本排序功能）
--   - bpchar_enc >/>=/</<= 范围查询（与常量比较）
--   - bpchar_enc 与 text_enc 比较
--   - bpchar_enc 与 varchar_enc 比较
--   - bpchar 尾部空格补齐处理（'abc' vs 'abc  '）
--   - 中文字符比较
--   - 边界值（空串、NULL）
-- ============================================================

-- ============================================================
-- Setup
-- ============================================================
DROP KEK IF EXISTS kek_bpchar_cmp CASCADE;

CREATE KEK kek_bpchar_cmp WITH (
    KEY_STORE = 'localkms',
    KEY_PATH = '/tmp/kek_bpchar_cmp.key'
);

CREATE DEK dek_bpchar_cmp WITH VALUES(KEK = kek_bpchar_cmp);

SELECT '=== Setup OK ===';

-- ============================================================
-- 1. 基础 ORDER BY + 范围查询
-- ============================================================
SELECT '=== 1 ORDER BY + 范围查询 ===';

DROP TABLE IF EXISTS tecc_bpchar_cmp CASCADE;

CREATE TABLE tecc_bpchar_cmp (
    id SERIAL PRIMARY KEY,
    bpchar_col bpchar_enc encrypted with (dek = dek_bpchar_cmp, support_algo='ope'),
    text_col text_enc encrypted with (dek = dek_bpchar_cmp, support_algo='ope'),
    varchar_col varchar_enc encrypted with (dek = dek_bpchar_cmp, support_algo='ope')
);

-- @param String apple
-- @param String apple
-- @param String apple
-- @param String Apple
-- @param String Apple
-- @param String Apple
-- @param String APPLE
-- @param String APPLE
-- @param String APPLE
-- @param String banana
-- @param String banana
-- @param String banana
-- @param String cherry
-- @param String cherry
-- @param String cherry
-- @param String 测试
-- @param String 测试
-- @param String 测试
-- @param String 测试a
-- @param String 测试a
-- @param String 测试a
-- @param String 王五
-- @param String 王五
-- @param String 王五
-- @param null null
-- @param null null
-- @param null null
INSERT INTO tecc_bpchar_cmp (bpchar_col, text_col, varchar_col) VALUES
(?, ?, ?),
(?, ?, ?),
(?, ?, ?),
(?, ?, ?),
(?, ?, ?),
(?, ?, ?),
(?, ?, ?),
(?, ?, ?),
(?, ?, ?);

SELECT '1a_bpchar_asc', id, bpchar_col FROM tecc_bpchar_cmp ORDER BY bpchar_col;

SELECT '1b_text_asc', id, text_col FROM tecc_bpchar_cmp ORDER BY text_col;

SELECT '1c_varchar_asc', id, varchar_col FROM tecc_bpchar_cmp ORDER BY varchar_col;

-- 范围查询
-- @param String banana
SELECT '1d_bpchar_gt_banana', id, bpchar_col FROM tecc_bpchar_cmp WHERE bpchar_col > ? ORDER BY bpchar_col;

-- @param String banana
SELECT '1e_bpchar_ge_banana', id, bpchar_col FROM tecc_bpchar_cmp WHERE bpchar_col >= ? ORDER BY bpchar_col;

-- @param String cherry
SELECT '1f_bpchar_lt_cherry', id, bpchar_col FROM tecc_bpchar_cmp WHERE bpchar_col < ? ORDER BY bpchar_col;

-- @param String cherry
SELECT '1g_bpchar_le_cherry', id, bpchar_col FROM tecc_bpchar_cmp WHERE bpchar_col <= ? ORDER BY bpchar_col;

-- @param String banana
-- @param String cherry
SELECT '1h_bpchar_range', id, bpchar_col FROM tecc_bpchar_cmp WHERE bpchar_col > ? AND bpchar_col < ? ORDER BY bpchar_col;

-- @param String apple
-- @param String cherry
SELECT '1i_bpchar_between', id, bpchar_col FROM tecc_bpchar_cmp WHERE bpchar_col BETWEEN ? AND ? ORDER BY bpchar_col;

-- ============================================================
-- 2. 中文字符比较
-- ============================================================
SELECT '=== 2 中文字符比较 ===';

-- @param String 测试
SELECT '2a_bpchar_gt_test', id, bpchar_col FROM tecc_bpchar_cmp WHERE bpchar_col > ? ORDER BY bpchar_col;

-- @param String 测试
SELECT '2b_bpchar_ge_test', id, bpchar_col FROM tecc_bpchar_cmp WHERE bpchar_col >= ? ORDER BY bpchar_col;

-- @param String 测试
SELECT '2c_bpchar_lt_test', id, bpchar_col FROM tecc_bpchar_cmp WHERE bpchar_col < ? ORDER BY bpchar_col;

-- @param String 测试
SELECT '2d_bpchar_le_test', id, bpchar_col FROM tecc_bpchar_cmp WHERE bpchar_col <= ? ORDER BY bpchar_col;

-- ============================================================
-- 3. bpchar 与 text/varchar 一致性验证（相同数据的范围查询）
-- ============================================================
SELECT '=== 3 跨类型一致性 ===';

-- @param String banana
SELECT '3a_text_gt_banana', id, text_col FROM tecc_bpchar_cmp WHERE text_col > ? ORDER BY text_col;

-- @param String banana
SELECT '3b_varchar_gt_banana', id, varchar_col FROM tecc_bpchar_cmp WHERE varchar_col > ? ORDER BY varchar_col;

-- @param String 测试
SELECT '3c_text_gt_test', id, text_col FROM tecc_bpchar_cmp WHERE text_col > ? ORDER BY text_col;

-- @param String 测试
SELECT '3d_varchar_gt_test', id, varchar_col FROM tecc_bpchar_cmp WHERE varchar_col > ? ORDER BY varchar_col;

-- ============================================================
-- 4. 尾部空格补齐处理
-- ============================================================
SELECT '=== 4 尾部空格处理 ===';

DROP TABLE IF EXISTS tecc_bpchar_pad CASCADE;

CREATE TABLE tecc_bpchar_pad (
    id SERIAL PRIMARY KEY,
    bpchar_col bpchar_enc(10) encrypted with (dek = dek_bpchar_cmp, support_algo='ope'),
    text_col text_enc encrypted with (dek = dek_bpchar_cmp, support_algo='ope'),
    varchar_col varchar_enc(10) encrypted with (dek = dek_bpchar_cmp, support_algo='ope')
);

-- 插入相同内容的行，但尾部空格不同
-- @param String abc
-- @param String abc
-- @param String abc
-- @param String abc  
-- @param String abc  
-- @param String abc  
-- @param String abc    
-- @param String abc    
-- @param String abc    
-- @param String test
-- @param String test
-- @param String test
-- @param String test 
-- @param String test 
-- @param String test 
-- @param null null
-- @param null null
-- @param null null
INSERT INTO tecc_bpchar_pad (bpchar_col, text_col, varchar_col) VALUES
(?, ?, ?),
(?, ?, ?),
(?, ?, ?),
(?, ?, ?),
(?, ?, ?),
(?, ?, ?);

-- bpchar尾部空格应被忽略（'abc' = 'abc  ' = 'abc    '）
-- @param String abc
SELECT '4a_bpchar_eq_abc', id, bpchar_col FROM tecc_bpchar_pad WHERE bpchar_col = ? ORDER BY id;

-- @param String abc  
SELECT '4b_bpchar_eq_abc_pad', id, bpchar_col FROM tecc_bpchar_pad WHERE bpchar_col = ? ORDER BY id;

-- @param String abc
SELECT '4c_bpchar_gt_abc', id, bpchar_col FROM tecc_bpchar_pad WHERE bpchar_col > ? ORDER BY id;

-- @param String abc
SELECT '4d_bpchar_ge_abc', id, bpchar_col FROM tecc_bpchar_pad WHERE bpchar_col >= ? ORDER BY id;

-- text/varchar尾部空格保留（'abc' != 'abc  '）
-- @param String abc
SELECT '4e_text_eq_abc', id, text_col FROM tecc_bpchar_pad WHERE text_col = ? ORDER BY id;

-- @param String abc
SELECT '4f_varchar_eq_abc', id, varchar_col FROM tecc_bpchar_pad WHERE varchar_col = ? ORDER BY id;

-- text/varchar尾部空格比较
-- @param String abc
SELECT '4g_text_gt_abc', id, text_col FROM tecc_bpchar_pad WHERE text_col > ? ORDER BY id;

-- @param String abc
SELECT '4h_varchar_gt_abc', id, varchar_col FROM tecc_bpchar_pad WHERE varchar_col > ? ORDER BY id;

-- 跨类型等值比较（bpchar vs text, bpchar vs varchar）
-- @param int 1
SELECT '4i_bpchar_vs_text', a.id as id_a, b.id as id_b FROM tecc_bpchar_pad a, tecc_bpchar_pad b WHERE a.bpchar_col = b.text_col AND a.id = ? ORDER BY b.id;

-- @param int 1
SELECT '4j_bpchar_vs_varchar', a.id as id_a, b.id as id_b FROM tecc_bpchar_pad a, tecc_bpchar_pad b WHERE a.bpchar_col = b.varchar_col AND a.id = ? ORDER BY b.id;

-- ============================================================
-- 5. 多数据范围验证（含中文，完整数据集）
-- ============================================================
SELECT '=== 5 多数据范围验证 ===';

DROP TABLE IF EXISTS tecc_bpchar_full CASCADE;

CREATE TABLE tecc_bpchar_full (
    id SERIAL PRIMARY KEY,
    bpchar_col bpchar_enc(20) encrypted with (dek = dek_bpchar_cmp, support_algo='ope'),
    text_col text_enc encrypted with (dek = dek_bpchar_cmp, support_algo='ope'),
    varchar_col varchar_enc(100) encrypted with (dek = dek_bpchar_cmp, support_algo='ope')
);

-- @param String 王五
-- @param String 王五
-- @param String 王五
-- @param String Charlie
-- @param String Charlie
-- @param String Charlie
-- @param String 周八
-- @param String 周八
-- @param String 周八
-- @param String Eve
-- @param String Eve
-- @param String Eve
-- @param String test
-- @param String test
-- @param String test
-- @param String alice
-- @param String alice
-- @param String alice
-- @param String Eve2
-- @param String Eve2
-- @param String Eve2
-- @param String TEST
-- @param String TEST
-- @param String TEST
-- @param String foo_bar
-- @param String foo_bar
-- @param String foo_bar
-- @param String pct_sign
-- @param String pct_sign
-- @param String pct_sign
-- @param String _start
-- @param String _start
-- @param String _start
-- @param String 测试
-- @param String 测试
-- @param String 测试
-- @param String test123
-- @param String test123
-- @param String test123
-- @param String mixCase
-- @param String mixCase
-- @param String mixCase
-- @param String 
-- @param String 
-- @param String 
-- @param null null
-- @param null null
-- @param null null
INSERT INTO tecc_bpchar_full (bpchar_col, text_col, varchar_col) VALUES
(?, ?, ?),
(?, ?, ?),
(?, ?, ?),
(?, ?, ?),
(?, ?, ?),
(?, ?, ?),
(?, ?, ?),
(?, ?, ?),
(?, ?, ?),
(?, ?, ?),
(?, ?, ?),
(?, ?, ?),
(?, ?, ?),
(?, ?, ?),
(?, ?, ?),
(?, ?, ?);

-- ORDER BY
SELECT '5a_bpchar_order_asc', id, bpchar_col FROM tecc_bpchar_full ORDER BY bpchar_col;

SELECT '5b_text_order_asc', id, text_col FROM tecc_bpchar_full ORDER BY text_col;

SELECT '5c_varchar_order_asc', id, varchar_col FROM tecc_bpchar_full ORDER BY varchar_col;

-- 范围查询
-- @param String 测试
SELECT '5d_bpchar_gt_test', id, bpchar_col FROM tecc_bpchar_full WHERE bpchar_col > ? ORDER BY id;

-- @param String 测试
SELECT '5e_text_gt_test', id, text_col FROM tecc_bpchar_full WHERE text_col > ? ORDER BY id;

-- @param String 测试
SELECT '5f_varchar_gt_test', id, varchar_col FROM tecc_bpchar_full WHERE varchar_col > ? ORDER BY id;

-- @param String 测试
SELECT '5g_bpchar_lt_test', id, bpchar_col FROM tecc_bpchar_full WHERE bpchar_col < ? ORDER BY id;

-- @param String 测试
SELECT '5h_text_lt_test', id, text_col FROM tecc_bpchar_full WHERE text_col < ? ORDER BY id;

-- @param String 测试
SELECT '5i_varchar_lt_test', id, varchar_col FROM tecc_bpchar_full WHERE varchar_col < ? ORDER BY id;

-- @param String banana
-- @param String cherry
SELECT '5j_bpchar_banana_cherry', id, bpchar_col FROM tecc_bpchar_full WHERE bpchar_col >= ? AND bpchar_col < ? ORDER BY bpchar_col;

-- @param String banana
-- @param String cherry
SELECT '5k_text_banana_cherry', id, text_col FROM tecc_bpchar_full WHERE text_col >= ? AND text_col < ? ORDER BY text_col;

-- @param String banana
-- @param String cherry
SELECT '5l_varchar_banana_cherry', id, varchar_col FROM tecc_bpchar_full WHERE varchar_col >= ? AND varchar_col < ? ORDER BY varchar_col;

-- ============================================================
-- Cleanup
-- ============================================================
DROP TABLE IF EXISTS tecc_bpchar_cmp CASCADE;

DROP TABLE IF EXISTS tecc_bpchar_pad CASCADE;

DROP TABLE IF EXISTS tecc_bpchar_full CASCADE;

DROP DEK IF EXISTS dek_bpchar_cmp CASCADE;

DROP KEK IF EXISTS kek_bpchar_cmp CASCADE;

SELECT '=== Cleanup OK ===';
