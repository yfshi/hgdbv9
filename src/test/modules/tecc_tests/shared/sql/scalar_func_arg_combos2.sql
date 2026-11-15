-- ============================================================================
-- TECC 标量函数参数组合校验测试 2（需求 4.4.1.4：不支持的内置函数固定格式错误信息）
--
-- 覆盖 nullif / decode / power 与以下参数的组合：
--   裸 ::密态类型 cast 常量 / encrypted with (dek=...) cast 常量 / 密态列 /
--   普通常量 / 普通列 / NULL / 跨族常量
-- 期望：不支持组合统一报 F 模板（locale 可变：TAP 环境 LC_MESSAGES=C → 英文）：
--   <函数名> does not support this argument combination: <原因>
-- power 的列相关拒绝保持其既有固定文案（函数 "power" 不支持加密列运算…）。
-- (libpq 独有用例: 断言 tecc_processor 客户端预处理校验)
-- ============================================================================

-- ### 环境准备 ###############################################################
DROP TABLE IF EXISTS enc_pgfunc_test_det CASCADE;
DROP TABLE IF EXISTS enc_pgfunc_test_ope CASCADE;
DROP TABLE IF EXISTS enc_arg_combos2_ope CASCADE;
DROP DEK IF EXISTS enc_pgfunc_dek;
DROP KEK IF EXISTS enc_pgfunc_kek;

CREATE KEK enc_pgfunc_kek WITH (key_store = 'localkms', key_path = '/tmp/enc_pgfunc_kek.key');
CREATE DEK enc_pgfunc_dek WITH VALUES(KEK = 'enc_pgfunc_kek');
CREATE TABLE enc_arg_combos2_ope (
    id     serial PRIMARY KEY,
    c_int4 int4_enc ENCRYPTED WITH (DEK = enc_pgfunc_dek, support_algo = 'ope'),
    c_text text_enc ENCRYPTED WITH (DEK = enc_pgfunc_dek, support_algo = 'ope'),
    n_int4 int4,
    n_text text
);
INSERT INTO enc_arg_combos2_ope (c_int4, c_text, n_int4, n_text) VALUES
 (10, 'abc', 10, 'abc'),
 (NULL, NULL, NULL, NULL);

-- ============ N. nullif ============
\echo [N1] nullif(10::int2_enc, 10) -- F: 裸 cast
select nullif(10::int2_enc, 10);
\echo [N2] nullif(10::int2_enc ew(dek), 10) -- F: 密态常量×普通常量
select nullif(10::int2_enc encrypted with (dek=enc_pgfunc_dek), 10);
\echo [N3] nullif(c_int4, 10) -- 成功锚点
select nullif(c_int4, 10) from enc_arg_combos2_ope;
\echo [N4] nullif(c_int4, 'abc') -- F: 常量类型不匹配
select nullif(c_int4, 'abc') from enc_arg_combos2_ope;
\echo [N5] nullif(c_text, 10) -- F: 常量类型不匹配
select nullif(c_text, 10) from enc_arg_combos2_ope;
\echo [N6] nullif(c_int4, 10::int2_enc) -- F: 裸 cast
select nullif(c_int4, 10::int2_enc) from enc_arg_combos2_ope;
\echo [N7] nullif(c_int4, n_int4) -- F: 密态列×普通列
select nullif(c_int4, n_int4) from enc_arg_combos2_ope;
\echo [N8] nullif(c_int4, NULL) -- 成功锚点
select nullif(c_int4, NULL) from enc_arg_combos2_ope;

-- ============ O. decode ============
\echo [O1] decode(10::int2_enc, 1, 'a', 2, 'b') -- F: 裸 cast
select decode(10::int2_enc, 1, 'a', 2, 'b');
\echo [O2] decode(10::int2_enc ew(dek), 1, 'a', 2, 'b') -- F: 密态常量×普通常量
select decode(10::int2_enc encrypted with (dek=enc_pgfunc_dek), 1, 'a', 2, 'b');
\echo [O5] decode(c_int4, 'abc', 'x') -- F: 搜索常量类型不匹配
select decode(c_int4, 'abc', 'x') from enc_arg_combos2_ope;
\echo [O6] decode(c_int4, 1, 'a', n_int4, 'b', 'x') -- F: 密态列×普通列
select decode(c_int4, 1, 'a', n_int4, 'b', 'x') from enc_arg_combos2_ope;
\echo [O7] decode(c_text, 1, 'a', 2, 'b') -- F: 搜索常量类型不匹配
select decode(c_text, 1, 'a', 2, 'b') from enc_arg_combos2_ope;
\echo [O8] decode(c_int4, 10::int2_enc, 'x') -- F: 裸 cast
select decode(c_int4, 10::int2_enc, 'x') from enc_arg_combos2_ope;
\echo [O4] decode(c_text, 'a', 'b', 'c', 'd', 'e') -- 已知 gap: 服务端签名不存在
select decode(c_text, 'a', 'b', 'c', 'd', 'e') from enc_arg_combos2_ope;

-- ============ P. power ============
\echo [P1] power(10::int2_enc, 2) -- F: 裸 cast
select power(10::int2_enc, 2);
\echo [P2] power(10::int2_enc ew(dek), 2) -- F: 密态常量×普通常量
select power(10::int2_enc encrypted with (dek=enc_pgfunc_dek), 2);
\echo [P3] power(c_int4, 2) -- power 既有固定文案（非HE列）
select power(c_int4, 2) from enc_arg_combos2_ope;
\echo [P4] power(c_int4, 'abc') -- power 既有固定文案（非HE列）
select power(c_int4, 'abc') from enc_arg_combos2_ope;
\echo [P5] power(c_int4, 10::int2_enc) -- F: 裸 cast
select power(c_int4, 10::int2_enc) from enc_arg_combos2_ope;
\echo [P6] power(c_int4, n_int4) -- power 既有固定文案（非HE列）
select power(c_int4, n_int4) from enc_arg_combos2_ope;
\echo [P7] power(c_text, 2) -- power 既有固定文案（非HE列）
select power(c_text, 2) from enc_arg_combos2_ope;

-- ---------- 清理 ----------
DROP TABLE IF EXISTS enc_arg_combos2_ope CASCADE;
DROP DEK IF EXISTS enc_pgfunc_dek;
DROP KEK IF EXISTS enc_pgfunc_kek;
