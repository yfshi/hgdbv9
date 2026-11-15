-- ============================================================
-- 004_query_ciphertext.sql — 跨驱动测试：查询密文数据
-- 验证libpq和JDBC对同一数据产生的密文是否一致
-- ============================================================

-- DET密文
SELECT '=== DET CIPHER ===';
SELECT id, c_int4, c_float8, c_numeric, c_text, c_varchar FROM cross_det ORDER BY id;

-- OPE密文（cross_ope仅有 int4/numeric/text 三种类型）
SELECT '=== OPE CIPHER ===';
SELECT id, c_int4, c_numeric, c_text FROM cross_ope ORDER BY id;

-- SEARCH密文（cross_search仅有 text/varchar 两种类型）
SELECT '=== SEARCH CIPHER ===';
SELECT id, c_text, c_varchar FROM cross_search ORDER BY id;

-- RND密文（cross_rnd有全部9种类型）
SELECT '=== RND CIPHER ===';
SELECT id, c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric, c_text, c_varchar, c_bpchar FROM cross_rnd ORDER BY id;

-- 清理（从 003_select.sql 移至此，确保 004 能查询到密文数据）
DROP TABLE IF EXISTS cross_rnd, cross_det, cross_ope, cross_search CASCADE;
DROP DEK IF EXISTS dek_det, dek_rnd, dek_ope, dek_search;
DROP KEK IF EXISTS kek_cross CASCADE;
