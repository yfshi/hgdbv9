-- ============================================================================
-- TECC 标量函数 —— greatest/least 引用"不存在的列"时的报错行为
--
-- 背景：前端只有密态列元数据（hg_encrypted_columns_info），没有普通列清单。
-- 参数组合校验原先把"查不到密态列元数据的列"一律当成普通列，于是
--     SELECT GREATEST(c_int4, no_such_col) FROM g_col;
-- 会报
--     GREATEST does not support this argument combination:
--     encrypted columns and plain columns cannot be mixed
-- 属误导——真正的问题是列不存在。
--
-- 现改为：在即将报"混用"前，经数据获取器的独立临时连接探测该列是否真实存在，
-- 确认不存在则跳过本次校验，交还服务端报 column "..." does not exist；
-- 探测不确定（表不存在/无权限/临时表等）一律回退原有行为。
--
-- 本文件同时锚定两侧行为：
--   A 段：不存在的列 → 服务端原生报错
--   B 段：真实存在的普通列 → 仍报 TECC 混用错误（不得被误伤）
-- ============================================================================

-- ### 环境准备 ###############################################################
DROP TABLE IF EXISTS enc_pgfunc_mc CASCADE;
DROP DEK IF EXISTS enc_pgfunc_mc_dek CASCADE;
DROP KEK IF EXISTS enc_pgfunc_mc_kek CASCADE;

CREATE KEK enc_pgfunc_mc_kek WITH (key_store = 'localkms', key_path = '/tmp/enc_pgfunc_mc_kek.key');
CREATE DEK enc_pgfunc_mc_dek WITH VALUES(KEK = 'enc_pgfunc_mc_kek');

CREATE TABLE enc_pgfunc_mc (
    id     int,
    c_int4 int4_enc ENCRYPTED WITH (DEK = enc_pgfunc_mc_dek, support_algo = 'ope'),
    c_text text_enc ENCRYPTED WITH (DEK = enc_pgfunc_mc_dek, support_algo = 'ope'),
    n_int4 int
);
INSERT INTO enc_pgfunc_mc VALUES (1, 4000, 'apple', 100);

-- ### A. 不存在的列：应报 column "..." does not exist ########################
\echo '========== A. 不存在的列（应报列不存在） =========='

-- A1 密态列 + 不存在的列（无限定）
SELECT 'MC-A1', GREATEST(c_int4, no_such_col) FROM enc_pgfunc_mc;

-- A2 密态列 + 表限定的不存在列
SELECT 'MC-A2', GREATEST(enc_pgfunc_mc.c_int4, enc_pgfunc_mc.no_such_col) FROM enc_pgfunc_mc;

-- A3 反序：不存在的列在前
SELECT 'MC-A3', GREATEST(no_such_col, c_int4) FROM enc_pgfunc_mc;

-- A4 字符密态列 + 不存在的列
SELECT 'MC-A4', GREATEST(c_text, no_such_col) FROM enc_pgfunc_mc;

-- A5 LEAST 同样处理
SELECT 'MC-A5', LEAST(c_int4, no_such_col) FROM enc_pgfunc_mc;

-- A6 表别名限定（走别名匹配路径）
SELECT 'MC-A6', GREATEST(t.c_int4, t.no_such_col) FROM enc_pgfunc_mc t;

-- ### B. 真实存在的普通列：仍报 TECC 混用错误（不得被误伤） ###################
\echo '========== B. 真实普通列（应报 enc/plain 混用） =========='
SELECT 'MC-B1', GREATEST(c_int4, n_int4) FROM enc_pgfunc_mc;
SELECT 'MC-B2', LEAST(c_int4, n_int4) FROM enc_pgfunc_mc;

-- ### C. 既有语义不变 ########################################################
\echo '========== C. 既有语义对照 =========='
-- C1 普通列 + 不存在的列：服务端原生报错（TECC 不介入）
SELECT 'MC-C1', GREATEST(n_int4, no_such_col) FROM enc_pgfunc_mc;
-- C2 不存在的表：服务端原生报错
SELECT 'MC-C2', GREATEST(c_int4, no_such_col) FROM no_such_table_mc;
-- C3 正常用法不受影响
SELECT 'MC-C3', GREATEST(c_int4, 100000) FROM enc_pgfunc_mc;

-- ### 清理 ###################################################################
\echo '========== 清理 =========='
DROP TABLE enc_pgfunc_mc CASCADE;
DROP DEK enc_pgfunc_mc_dek CASCADE;
DROP KEK enc_pgfunc_mc_kek CASCADE;
