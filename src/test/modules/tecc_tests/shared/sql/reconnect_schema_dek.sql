-- ============================================================
-- reconnect_schema_dek.sql — schema限定DEK + 连接重连(\c)回归
-- 对应修复: e754b061a22 (schema限定DEK名缓存查找 fallback)
-- 场景: schema限定DEK(如 s_reg.dek_reg)不在search_path时缓存加载失
--       配, 且 \c 重连(PQfinish→tecc_state_cleanup)时 DEK 缓存重复释放
--       导致 SIGSEGV(psql 段错误)。本用例验证 schema限定KEK/DEK/表
--       正常创建、\c 重连后不崩溃、元数据仍可访问。
-- 注: 不依赖 DML 字面量加密(限定表名的 INSERT 在4.0.0仍需跟进), 以
--       元数据访问 + \c 重连为验证点。
-- ============================================================

-- 0. Setup/Cleanup: 保证可重复运行
DROP TABLE IF EXISTS s_reg.t_reg CASCADE;
DROP DEK IF EXISTS s_reg.dek_reg;
DROP KEK IF EXISTS s_reg.kek_reg CASCADE;
DROP SCHEMA IF EXISTS s_reg CASCADE;
DROP ROLE IF EXISTS test_reconn;
CREATE ROLE test_reconn LOGIN SUPERUSER;
CREATE SCHEMA s_reg;
CREATE KEK s_reg.kek_reg WITH (key_store = 'localkms', key_path = '/tmp/kek_cache.key');
CREATE DEK s_reg.dek_reg WITH VALUES(KEK = s_reg.kek_reg);
SELECT '0_setup_ok';

-- 1. schema限定DEK建加密表, 触发DEK缓存加载
CREATE TABLE s_reg.t_reg (id int, v int4_enc ENCRYPTED WITH (DEK = s_reg.dek_reg, SUPPORT_ALGO = 'DET'));
SELECT '1_meta', 1 FROM hg_encrypted_columns ec JOIN pg_class c ON ec.table_oid=c.oid WHERE c.relname='t_reg';

-- 2. \c 重连: 触发旧连接 tecc_state_cleanup 清理 DEK 缓存(e754崩溃路径)
\c postgres test_reconn

-- 3. 重连后再次访问 schema限定DEK加密表元数据, 验证不崩溃
SELECT '3_count', count(*) FROM s_reg.t_reg;
SELECT '3_meta_after', 1 FROM hg_encrypted_columns ec JOIN pg_class c ON ec.table_oid=c.oid WHERE c.relname='t_reg';
SELECT '3_reconnect_ok';

-- 4. Cleanup: 切换回超户后再清理角色(当前连接是test_reconn不能删自己)
\c postgres postgres
DROP TABLE IF EXISTS s_reg.t_reg CASCADE;
DROP DEK IF EXISTS s_reg.dek_reg;
DROP KEK IF EXISTS s_reg.kek_reg CASCADE;
DROP SCHEMA IF EXISTS s_reg CASCADE;
DROP ROLE IF EXISTS test_reconn;
SELECT '4_cleanup_ok';
