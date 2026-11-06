-- ============================================================
-- 003_select.sql — 跨驱动测试：全量查询+算法特定查询
-- ============================================================

SELECT '=== RND ALL ===';
SELECT id, c_int2, c_int4, c_int8, c_text FROM cross_rnd ORDER BY id;
SELECT id, c_float4, c_float8, c_numeric FROM cross_rnd ORDER BY id;
SELECT id, c_varchar, c_bpchar FROM cross_rnd ORDER BY id;

SELECT '=== DET EQUAL ===';
SELECT id, c_int4, c_float8, c_text, c_varchar FROM cross_det
    WHERE c_int4 = 100 ORDER BY id;
SELECT id, c_int4, c_float8, c_text, c_varchar FROM cross_det
    WHERE c_text = 'det_alpha' ORDER BY id;

SELECT '=== OPE RANGE ===';
SELECT id, c_int4, c_numeric, c_text FROM cross_ope
    WHERE c_int4 >= 20 AND c_int4 <= 60 ORDER BY c_int4;

SELECT '=== OPE ORDER ===';
SELECT id, c_int4, c_numeric, c_text FROM cross_ope
    ORDER BY c_int4 DESC;

SELECT '=== SEARCH LIKE ===';
SELECT id, c_text, c_varchar FROM cross_search
    WHERE c_text LIKE '%app%' ORDER BY id;
SELECT id, c_text, c_varchar FROM cross_search
    WHERE c_varchar LIKE '%ana%' ORDER BY id;

-- 清理操作已移至 004_query_ciphertext.sql 末尾，确保 004 能查询到密文数据
