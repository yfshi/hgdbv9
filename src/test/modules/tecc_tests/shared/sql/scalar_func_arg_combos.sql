-- ============================================================================
-- TECC 标量函数参数组合校验测试（需求 4.4.1.4：不支持的内置函数固定格式错误信息）
--
-- 覆盖 coalesce / greatest / least / case when / case switch 与以下参数的组合：
--   裸 ::密态类型 cast 常量 / encrypted with (dek=...) cast 常量 / 密态列 /
--   普通常量 / 普通列 / NULL / 跨族密态列 / 数值格洞
-- 期望：所有不支持的组合统一报 F 模板（前端固定格式）：
--   <函数名> 不支持当前参数组合：<原因>
-- 成功锚点：密态列×同类型常量、同族可拓宽组合、ew cast×ew cast、ew cast×NULL。
-- (libpq 独有用例: 断言 tecc_processor 客户端预处理校验)
-- ============================================================================

-- ### 环境准备 ###############################################################
DROP TABLE IF EXISTS enc_pgfunc_test_det CASCADE;
DROP TABLE IF EXISTS enc_pgfunc_test_ope CASCADE;
DROP TABLE IF EXISTS enc_arg_combos_ope CASCADE;
DROP DEK IF EXISTS enc_pgfunc_dek;
DROP KEK IF EXISTS enc_pgfunc_kek;

CREATE KEK enc_pgfunc_kek WITH (key_store = 'localkms', key_path = '/tmp/enc_pgfunc_kek.key');
CREATE DEK enc_pgfunc_dek WITH VALUES(KEK = 'enc_pgfunc_kek');
CREATE TABLE enc_arg_combos_ope (
    id     serial PRIMARY KEY,
    c_int4 int4_enc ENCRYPTED WITH (DEK = enc_pgfunc_dek, support_algo = 'ope'),
    c_text text_enc ENCRYPTED WITH (DEK = enc_pgfunc_dek, support_algo = 'ope'),
    n_int4 int4,
    n_text text
);
INSERT INTO enc_arg_combos_ope (c_int4, c_text, n_int4, n_text) VALUES
 (10, 'abc', 10, 'abc'),
 (NULL, NULL, NULL, NULL);

-- ============ A. coalesce — 纯常量（裸 ::enc cast） ============
\echo [A1] coalesce(10::int2_enc, 10, NULL)
select coalesce(10::int2_enc, 10, NULL);
\echo [A2] coalesce(10::int2_enc, NULL)
select coalesce(10::int2_enc, NULL);
\echo [A3] coalesce(10::int2_enc, 'abc', NULL)
select coalesce(10::int2_enc, 'abc', NULL);
\echo [A4] coalesce('abc', 19::float4_enc, NULL)
select coalesce('abc', 19::float4_enc, NULL);
\echo [A5] coalesce(10::int2_enc, 20::int4_enc, NULL)
select coalesce(10::int2_enc, 20::int4_enc, NULL);
\echo [A6] coalesce(NULL, 10::int2_enc)
select coalesce(NULL, 10::int2_enc);
\echo [A7] coalesce(10::int2_enc, 10.5, NULL)
select coalesce(10::int2_enc, 10.5, NULL);
\echo [A8] coalesce('a'::text_enc, 'b', NULL)
select coalesce('a'::text_enc, 'b', NULL);
\echo [A9] coalesce(NULL, NULL, 10::int2_enc)
select coalesce(NULL, NULL, 10::int2_enc);

-- ============ B. coalesce — encrypted with ============
\echo [B1] coalesce(10::int2_enc ew(dek), 10, NULL)
select coalesce(10::int2_enc encrypted with (dek=enc_pgfunc_dek), 10, NULL);
\echo [B2] coalesce(10::int2_enc ew(dek), 'abc', NULL)
select coalesce(10::int2_enc encrypted with (dek=enc_pgfunc_dek), 'abc', NULL);
\echo [B3] coalesce('abc', 19::float4_enc ew(dek), NULL)
select coalesce('abc', 19::float4_enc encrypted with (dek=enc_pgfunc_dek), NULL);
\echo [B4] coalesce(10::int2_enc ew(dek), 20::int4_enc ew(dek), NULL) -- 成功锚点
select coalesce(10::int2_enc encrypted with (dek=enc_pgfunc_dek), 20::int4_enc encrypted with (dek=enc_pgfunc_dek), NULL);

-- ============ C. coalesce — 密态列 + 常量/列 ============
\echo [C1] coalesce(c_text, 'abc', NULL) -- 成功锚点
select coalesce(c_text, 'abc', NULL) from enc_arg_combos_ope;
\echo [C2] coalesce(c_int4, 10, NULL) -- 成功锚点
select coalesce(c_int4, 10, NULL) from enc_arg_combos_ope;
\echo [C3] coalesce(c_text, 10, NULL)
select coalesce(c_text, 10, NULL) from enc_arg_combos_ope;
\echo [C4] coalesce(c_int4, 'abc', NULL)
select coalesce(c_int4, 'abc', NULL) from enc_arg_combos_ope;
\echo [C5] coalesce(c_int4, 10::int2_enc, NULL)
select coalesce(c_int4, 10::int2_enc, NULL) from enc_arg_combos_ope;
\echo [C6] coalesce(c_text, 10::int2_enc, NULL)
select coalesce(c_text, 10::int2_enc, NULL) from enc_arg_combos_ope;
\echo [C7] coalesce(c_int4, c_text, NULL)
select coalesce(c_int4, c_text, NULL) from enc_arg_combos_ope;
\echo [C8] coalesce(c_int4, n_int4, NULL)
select coalesce(c_int4, n_int4, NULL) from enc_arg_combos_ope;

-- ============ D. greatest/least — 纯常量（裸 ::enc cast） ============
\echo [D1] greatest(10::int2_enc, 20, NULL)
select greatest(10::int2_enc, 20, NULL);
\echo [D2] least(10::int2_enc, NULL)
select least(10::int2_enc, NULL);
\echo [D3] greatest(10::int2_enc, 'abc', NULL)
select greatest(10::int2_enc, 'abc', NULL);
\echo [D4] greatest('abc', 19::float4_enc, NULL)
select greatest('abc', 19::float4_enc, NULL);
\echo [D5] greatest(10::int2_enc, 20::int4_enc, NULL)
select greatest(10::int2_enc, 20::int4_enc, NULL);
\echo [D6] least(10.5, 19::float4_enc, NULL)
select least(10.5, 19::float4_enc, NULL);

-- ============ E. greatest/least — encrypted with ============
\echo [E1] greatest(10::int2_enc ew(dek), 20, NULL)
select greatest(10::int2_enc encrypted with (dek=enc_pgfunc_dek), 20, NULL);
\echo [E2] greatest(10::int2_enc ew(dek), 'abc', NULL)
select greatest(10::int2_enc encrypted with (dek=enc_pgfunc_dek), 'abc', NULL);
\echo [E3] least(10::int2_enc ew(dek), NULL) -- 成功锚点
select least(10::int2_enc encrypted with (dek=enc_pgfunc_dek), NULL);

-- ============ F. greatest/least — 密态列 ============
\echo [F1] greatest(c_text, 'abc', NULL) -- 成功锚点
select greatest(c_text, 'abc', NULL) from enc_arg_combos_ope;
\echo [F2] least(c_int4, 10, NULL) -- 成功锚点
select least(c_int4, 10, NULL) from enc_arg_combos_ope;
\echo [F3] greatest(c_text, 10, NULL)
select greatest(c_text, 10, NULL) from enc_arg_combos_ope;
\echo [F4] greatest(c_int4, 'abc', NULL)
select greatest(c_int4, 'abc', NULL) from enc_arg_combos_ope;
\echo [F5] greatest(c_int4, 10::int2_enc, NULL)
select greatest(c_int4, 10::int2_enc, NULL) from enc_arg_combos_ope;
\echo [F6] greatest(c_int4, c_text, NULL)
select greatest(c_int4, c_text, NULL) from enc_arg_combos_ope;
\echo [F7] greatest(c_int4, n_int4, NULL)
select greatest(c_int4, n_int4, NULL) from enc_arg_combos_ope;

-- ============ G. case when — 纯常量 THEN/ELSE 含 ::enc cast ============
\echo [G1] case when true then 10::int2_enc else 10 end
select case when true then 10::int2_enc else 10 end;
\echo [G2] case when true then 10::int2_enc else NULL end
select case when true then 10::int2_enc else NULL end;
\echo [G3] case when true then 10::int2_enc else 'abc' end
select case when true then 10::int2_enc else 'abc' end;
\echo [G4] case when true then 'abc' else 19::float4_enc end
select case when true then 'abc' else 19::float4_enc end;
\echo [G5] case when true then 10::int2_enc else 20::int4_enc end
select case when true then 10::int2_enc else 20::int4_enc end;

-- ============ H. case when — 密态列 ============
\echo [H1] case when c_int4 = 10 then 10 else 20 end -- 成功锚点
select case when c_int4 = 10 then 10 else 20 end from enc_arg_combos_ope;
\echo [H2] case when c_text = 'x' then 'abc' else 'def' end -- 成功锚点
select case when c_text = 'x' then 'abc' else 'def' end from enc_arg_combos_ope;
\echo [H3] case when true then c_int4 else 10 end -- 成功锚点
select case when true then c_int4 else 10 end from enc_arg_combos_ope;
\echo [H4] case when true then c_int4 else 'abc' end -- F 模板（分支类型不兼容）
select case when true then c_int4 else 'abc' end from enc_arg_combos_ope;
\echo [H5] case when true then c_int4 else 10::int2_enc end
select case when true then c_int4 else 10::int2_enc end from enc_arg_combos_ope;

-- ============ I. case switch 形式 ============
\echo [I1] case c_int4 when 10 then 10 else 20 end
select case c_int4 when 10 then 10 else 20 end from enc_arg_combos_ope;
\echo [I2] case 10::int2_enc when 10 then 10 else 20 end
select case 10::int2_enc when 10 then 10 else 20 end;
\echo [I3] case c_int4 when n_int4 then 10 else 20 end
select case c_int4 when n_int4 then 10 else 20 end from enc_arg_combos_ope;

-- ---------- 清理 ----------
DROP TABLE IF EXISTS enc_arg_combos_ope CASCADE;
DROP DEK IF EXISTS enc_pgfunc_dek;
DROP KEK IF EXISTS enc_pgfunc_kek;
