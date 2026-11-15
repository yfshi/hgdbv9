-- ============================================================
-- analyze_search_col.sql — SEARCH-only加密列 ANALYZE 回归
-- 对应修复: c93e22b6cef (ANALYZE 对 SEARCH-only 加密列等值比较报错——
--            eqopr 降级到 compute_trivial_stats)
-- 场景: SEARCH-only加密列(无DET/OPE段)被 autovacuum/ANALYZE 做等值统计时
--       应降级到 compute_trivial_stats(NULL/非NULL统计), 不再报 DET/OPE
--       缺失错误。本用例验证 ANALYZE SEARCH-only 列不报错。
-- ============================================================

-- 0. Setup: SEARCH-only 加密列
DROP KEK IF EXISTS kek_ana CASCADE;
CREATE KEK kek_ana WITH (key_store = 'localkms', key_path = '/tmp/kek_search.key');
CREATE DEK dek_ana WITH VALUES(KEK = kek_ana);
DROP TABLE IF EXISTS ana_search_t CASCADE;
CREATE TABLE ana_search_t (id int, txt text_enc ENCRYPTED WITH (DEK = dek_ana, SUPPORT_ALGO = 'search'));
SELECT '0_setup_ok';

-- 1. 插入后 ANALYZE, 验证 SEARCH-only 列不再报错
INSERT INTO ana_search_t VALUES (1, 'hello world');
SELECT '1_data_ok';
ANALYZE ana_search_t;
SELECT '1_analyze_ok';

-- 2. 切换 DET-only 列仍能正常 ANALYZE(对照组)
DROP TABLE IF EXISTS ana_det_t CASCADE;
CREATE TABLE ana_det_t (id int, v int4_enc ENCRYPTED WITH (DEK = dek_ana, SUPPORT_ALGO = 'DET'));
INSERT INTO ana_det_t VALUES (1, 100);
ANALYZE ana_det_t;
SELECT '2_det_analyze_ok';

-- 3. Cleanup
DROP TABLE IF EXISTS ana_search_t CASCADE;
DROP TABLE IF EXISTS ana_det_t CASCADE;
DROP DEK IF EXISTS dek_ana;
DROP KEK IF EXISTS kek_ana CASCADE;
SELECT '3_cleanup_ok';
