-- ============================================================================
-- TECC 标量函数 —— 裸字面量常量在其它函数上的覆盖（coalesce / nullif / decode）
--
-- 姊妹用例 scalar_func_bare_const.sql 覆盖 greatest/least 的裸常量；
-- 本文件补齐同根因、同修复影响到的其余函数，并专门锚定一个崩溃回归：
--
-- 崩溃回归背景：浮点字面量经前端保留为 TEXTOID 常量（text_numeric_literal），
-- 修复后不再被 char/non-char 类别检查跳过。但若列是整型（无法表示小数），
-- 加密阶段会按 int 归一化 "1.5" 失败，其失败路径会触发堆损坏：
--     TECC SCHEDULER ERROR: RND normalize failed ... (code=-1202)
--     double free or corruption (out) → psql Aborted (core dumped)
-- 现限定：数值文本形态的常量只在浮点/数值列上加密，整型/字符列维持
-- 既有的"跳过加密"行为、交由服务端报错。本文件第 4 节即为该回归锚点，
-- 断言"干净报错、不崩溃"。
--
-- 另补齐 coalesce/nullif/decode 引用不存在列的场景（与 greatest/least 同源）。
-- ============================================================================

-- ### 环境准备 ###############################################################
DROP TABLE IF EXISTS enc_pgfunc_bc2 CASCADE;
DROP DEK IF EXISTS enc_pgfunc_bc2_dek CASCADE;
DROP KEK IF EXISTS enc_pgfunc_bc2_kek CASCADE;

CREATE KEK enc_pgfunc_bc2_kek WITH (key_store = 'localkms', key_path = '/tmp/enc_pgfunc_bc2_kek.key');
CREATE DEK enc_pgfunc_bc2_dek WITH VALUES(KEK = 'enc_pgfunc_bc2_kek');

CREATE TABLE enc_pgfunc_bc2 (
    id    int,
    c_i4  int4_enc    ENCRYPTED WITH (DEK = enc_pgfunc_bc2_dek, support_algo = 'ope'),
    c_num numeric_enc ENCRYPTED WITH (DEK = enc_pgfunc_bc2_dek, support_algo = 'ope'),
    c_f4  float4_enc  ENCRYPTED WITH (DEK = enc_pgfunc_bc2_dek, support_algo = 'ope'),
    c_f8  float8_enc  ENCRYPTED WITH (DEK = enc_pgfunc_bc2_dek, support_algo = 'ope'),
    n_i4  int
);
INSERT INTO enc_pgfunc_bc2 VALUES
  (1, 4000, 1000.50,  6.25,     1.5, 100),
  (2,    0,    NULL,  NULL, 12345.6789,   0);

-- ### 1. coalesce + 裸浮点常量 ###############################################
\echo '========== 1. coalesce + 裸浮点常量 =========='
SELECT id, COALESCE(c_num, 1.5) AS num_dflt,
           COALESCE(c_f4, 0.5)  AS f4_dflt,
           COALESCE(c_f8, 100.0) AS f8_dflt
FROM enc_pgfunc_bc2 ORDER BY id;

-- ### 2. nullif + 裸浮点常量 #################################################
\echo '========== 2. nullif + 裸浮点常量 =========='
SELECT id, NULLIF(c_f4, 6.25)  AS f4_nullif,
           NULLIF(c_num, 1.5)  AS num_nullif
FROM enc_pgfunc_bc2 ORDER BY id;

-- ### 3. 混用：显式 cast 常量 + 裸常量 #######################################
\echo '========== 3. cast 常量与裸常量混用 =========='
SELECT id, GREATEST(c_f4, 0.5::float4, 50.5) AS mixed FROM enc_pgfunc_bc2 ORDER BY id;

-- ### 4. 【崩溃回归锚点】decode + 浮点结果常量 ###############################
-- 整型列无法表示小数：应干净报错，绝不允许崩溃/堆损坏。
\echo '========== 4. decode + 浮点结果常量（崩溃回归：应报错不崩溃） =========='
SELECT id, DECODE(c_i4, 4000, 1.5, 0.0) AS dec_float FROM enc_pgfunc_bc2 ORDER BY id;

-- ### 5. 引用不存在的列（coalesce / nullif / decode） ########################
\echo '========== 5. 引用不存在的列（应报列不存在） =========='
SELECT id, COALESCE(c_i4, no_such_col) FROM enc_pgfunc_bc2 ORDER BY id;
SELECT id, NULLIF(c_i4, no_such_col) FROM enc_pgfunc_bc2 ORDER BY id;
SELECT id, DECODE(c_i4, no_such_col, 7, 0) FROM enc_pgfunc_bc2 ORDER BY id;

-- ### 6. 对照：真实普通列仍报 enc/plain 混用 #################################
\echo '========== 6. 真实普通列（应报混用） =========='
SELECT id, COALESCE(c_i4, n_i4) FROM enc_pgfunc_bc2 ORDER BY id;
SELECT id, NULLIF(c_i4, n_i4) FROM enc_pgfunc_bc2 ORDER BY id;

-- ### 清理 ###################################################################
\echo '========== 7. 清理 =========='
DROP TABLE enc_pgfunc_bc2 CASCADE;
DROP DEK enc_pgfunc_bc2_dek CASCADE;
DROP KEK enc_pgfunc_bc2_kek CASCADE;
