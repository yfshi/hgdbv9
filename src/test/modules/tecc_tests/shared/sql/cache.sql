-- ============================================================
-- cache.sql — 缓存/密钥/权限管理测试
-- 覆盖 test_cache_refresh, test_env_setup,
--   test_ope_env_setup, test_setup_select_where,
--   test_bug7917_setup, test_bug7901/7904_repro,
--   test_dek_permission
-- ============================================================

-- ============================================================
-- 0. Setup — 多个KEK/DEK用于权限+跨DEK测试
-- ============================================================
DROP KEK IF EXISTS kek_cache CASCADE;
DROP KEK IF EXISTS kek_cache2 CASCADE;

CREATE KEK kek_cache WITH (key_store = 'localkms', key_path = '/tmp/kek_cache.key');
CREATE DEK dek_cache WITH VALUES(KEK = kek_cache);

CREATE KEK kek_cache2 WITH (key_store = 'localkms', key_path = '/tmp/kek_cache2.key');
CREATE DEK dek_cache2 WITH VALUES(KEK = kek_cache2);

-- ============================================================
-- 1. Cache refresh — CREATE TABLE then INSERT (test_cache_refresh)
-- ============================================================
SELECT '=== 1 Cache Refresh ===';

DROP TABLE IF EXISTS test_cr CASCADE;
CREATE TABLE test_cr (
    id INT4,
    val INT4_ENC ENCRYPTED WITH (DEK = dek_cache, SUPPORT_ALGO = 'DET')
);
INSERT INTO test_cr (id, val) VALUES (1, 100);
SELECT '1a_cache_refresh', id FROM test_cr;
DROP TABLE IF EXISTS test_cr CASCADE;

-- ============================================================
-- 2. Cross-DEK type cast rejection (bug7901)
-- ============================================================
SELECT '=== 2 Cross-DEK Rejection ===';

SELECT '2a_text_t2_via_dek1_dek2_err';
SELECT 'test'::text_enc encrypted with (dek = dek_cache)::varchar_enc encrypted with (dek = dek_cache2);

SELECT '2b_text_to_bpchar_diff_dek_err';
SELECT 'test'::text_enc encrypted with (dek = dek_cache)::bpchar_enc encrypted with (dek = dek_cache2);

SELECT '2c_varchar_to_text_diff_dek_err';
SELECT 'test'::varchar_enc encrypted with (dek = dek_cache)::text_enc encrypted with (dek = dek_cache2);

SELECT '2d_varchar_to_bpchar_diff_dek_err';
SELECT 'test'::varchar_enc encrypted with (dek = dek_cache)::bpchar_enc encrypted with (dek = dek_cache2);

SELECT '2e_bpchar_to_text_diff_dek_err';
SELECT 'test'::bpchar_enc encrypted with (dek = dek_cache)::text_enc encrypted with (dek = dek_cache2);

SELECT '2f_bpchar_to_varchar_diff_dek_err';
SELECT 'test'::bpchar_enc encrypted with (dek = dek_cache)::varchar_enc encrypted with (dek = dek_cache2);

-- ============================================================
-- 3. Cross-DEK INSERT/SELECT (bug7904)
-- ============================================================
SELECT '=== 3 Cross-DEK Insert Rejection ===';

DROP TABLE IF EXISTS tb_int4 CASCADE;
DROP TABLE IF EXISTS tb_int8 CASCADE;
CREATE TABLE tb_int4 (id int4_enc encrypted with (dek = dek_cache));
CREATE TABLE tb_int8 (id int8_enc encrypted with (dek = dek_cache));

-- Different DEK INSERT...SELECT should error
SELECT '3a_insert_int2_diff_dek_err';
INSERT INTO tb_int4(id) SELECT 1::int2_enc encrypted with (dek = dek_cache2);
SELECT '3b_insert_int8_diff_dek_err';
INSERT INTO tb_int8(id) SELECT 1::int2_enc encrypted with (dek = dek_cache2);

-- Same DEK INSERT should succeed
DROP TABLE IF EXISTS tb_v CASCADE;
CREATE TABLE tb_v (a int4_enc encrypted with (dek = dek_cache));
INSERT INTO tb_v(a) VALUES (2::int4_enc encrypted with (dek = dek_cache));
SELECT '3c_same_dek_insert', * FROM tb_v;

DROP TABLE IF EXISTS tb_int4 CASCADE;
DROP TABLE IF EXISTS tb_int8 CASCADE;
DROP TABLE IF EXISTS tb_v CASCADE;

-- ============================================================
-- 4. DEK Permission test (test_dek_permission)
-- ============================================================
SELECT '=== 4 DEK Permission ===';

DROP TABLE IF EXISTS test_perm_table CASCADE;
DROP USER IF EXISTS test_noperm_user;

CREATE TABLE test_perm_table (
    id INT4, name VARCHAR(100),
    salary INT4_ENC ENCRYPTED WITH (DEK = dek_cache, SUPPORT_ALGO = 'OPE'),
    bonus INT4_ENC ENCRYPTED WITH (DEK = dek_cache, SUPPORT_ALGO = 'OPE')
);

INSERT INTO test_perm_table VALUES (1, 'Alice', 50000, 5000);
INSERT INTO test_perm_table VALUES (2, 'Bob', 60000, 6000);

-- Superuser SELECT — decrypted
SELECT '4a_superuser_select', * FROM test_perm_table;

-- Create user without DEK USAGE
CREATE USER test_noperm_user WITH PASSWORD 'test123';
GRANT SELECT ON test_perm_table TO test_noperm_user;
GRANT USAGE ON SCHEMA public TO test_noperm_user;

-- 先创建已知超级用户用于切换回
DROP ROLE IF EXISTS test_postgres;
CREATE ROLE test_postgres LOGIN SUPERUSER;

-- 重新连接为无权限用户（TECC权限检查使用PQuser，不能用SET ROLE）
\c postgres test_noperm_user

-- No-permission user SELECT —只查未加密列（加密列返回hex每次不同无法精确匹配）
SELECT '4b_noperm_select', id, name FROM test_perm_table ORDER BY id;

-- No-permission INSERT — should error（错误进入stderr框架不捕获）
INSERT INTO test_perm_table VALUES (3, 'Charlie', 70000, 7000);

-- No-permission WHERE on encrypted column — should error（错误进入stderr）
SELECT '4c_noperm_where', id FROM test_perm_table WHERE salary = 50000;
SELECT '4d_noperm_range', id FROM test_perm_table WHERE salary > 40000;

-- 切回超级用户
\c postgres test_postgres

-- Superuser still works
SELECT '4e_superuser_still_works', * FROM test_perm_table;
SELECT '4f_superuser_where', * FROM test_perm_table WHERE salary > 45000 AND salary < 65000;

-- Re-connect as no-permission user for CREATE TABLE test (Bug 8041)
\c postgres test_noperm_user
-- No-permission CREATE TABLE — should ERROR
SELECT '4g_noperm_create_table_before' AS msg;
CREATE TABLE test_noperm_create_table (id INT, v INT4_ENC ENCRYPTED WITH (DEK = dek_cache, SUPPORT_ALGO = 'OPE'));
SELECT '4g_noperm_create_table_after' AS msg;
-- 切回超级用户继续
\c postgres test_postgres

-- Cleanup permissions section
DROP TABLE IF EXISTS test_perm_table CASCADE;
DROP TABLE IF EXISTS test_noperm_create_table CASCADE;
REVOKE ALL ON SCHEMA public FROM test_noperm_user;
DROP USER IF EXISTS test_noperm_user;

-- ============================================================
-- update_meta() 手动刷新元数据缓存测试（ReqID:TECC4.0）
-- ============================================================
SELECT '=== update_meta mark 置位测试 ===';
-- 先清 mark，确保初始状态为 false
SELECT hg_clear_kek_mark(), hg_clear_dek_mark(), hg_clear_column_mark();
-- 初始 mark 应为 false
SELECT '5a_mark_before', hg_is_kek_updated() AS kek, hg_is_dek_updated() AS dek, hg_is_column_updated() AS col;
-- 执行 update_meta：服务端广播 invalidation，各后端 mark 置 true
SELECT update_meta();
-- mark 应置 true
SELECT '5c_mark_after', hg_is_kek_updated() AS kek, hg_is_dek_updated() AS dek, hg_is_column_updated() AS col;
-- 清 mark，避免影响后续用例
SELECT hg_clear_kek_mark(), hg_clear_dek_mark(), hg_clear_column_mark();

-- ============================================================
-- 6. Bug 9155 — Failed CREATE TABLE stale cache sentinel
--    After a failed CREATE TABLE (wrong DEK), a subsequent
--    successful CREATE TABLE + INSERT must still encrypt.
-- ============================================================
SELECT '=== 6 Bug 9155 Stale Cache Sentinel ===';

CREATE SCHEMA IF NOT EXISTS sch_bug9155;

-- Step 1: CREATE TABLE with wrong DEK — TECC reports error but backend
-- may still create the relation. Drop to clean state.
SELECT '6a_wrong_dek_err';
CREATE TABLE sch_bug9155.t_base(
    id int,
    col_det text_enc encrypted with (dek = dek_cache2, support_algo = 'det')
);
DROP TABLE IF EXISTS sch_bug9155.t_base CASCADE;

-- Step 2: INSERT into non-existent table — should fail
SELECT '6b_insert_no_table_err';
INSERT INTO sch_bug9155.t_base(id, col_det) VALUES (1, 'test1');

-- Step 3: CREATE TABLE with correct DEK — should succeed
SELECT '6c_correct_dek_ok';
CREATE TABLE sch_bug9155.t_base(
    id int,
    col_det text_enc encrypted with (dek = dek_cache, support_algo = 'det')
);

-- Step 4: INSERT — must encrypt successfully (was 'invalid hex string' before fix)
SELECT '6d_insert_ok';
INSERT INTO sch_bug9155.t_base(id, col_det) VALUES (1, 'test1'), (2, 'test2');

-- Step 5: Verify data round-trips correctly
SELECT '6e_select', id, col_det FROM sch_bug9155.t_base ORDER BY id;

-- Cleanup
DROP TABLE IF EXISTS sch_bug9155.t_base CASCADE;
DROP SCHEMA IF EXISTS sch_bug9155 CASCADE;

-- ============================================================
-- Cleanup
-- ============================================================
SELECT '=== Cleanup ===';
DROP KEK IF EXISTS kek_cache CASCADE;
DROP KEK IF EXISTS kek_cache2 CASCADE;
