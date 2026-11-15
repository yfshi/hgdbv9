-- ============================================================================
-- TECC 标量函数 —— 密态列 + 裸字面量常量（全部 9 种密态类型）
-- 对应需求 4.4.1.4"内置函数适配"：greatest/least 的参数可混合密态列与常量
-- （常量按列算法加密）。
--
-- 本文件专门覆盖【裸字面量常量】形态，即不做任何显式 cast 的写法：
--     GREATEST(numeric_enc 列, 500.00)   ← 小数裸字面量
--     GREATEST(float4_enc 列,  50.5)
--     GREATEST(text_enc 列,    'banana')
--
-- 背景：前端解析器为保精度（Bug8178 高精度 numeric 的 DET 等值匹配）把
-- FLOAT 字面量原样保留为 TEXTOID 字符串常量（见 frontend_parser/tecc_grammar.y
-- 的 FLOAT 产生式）。该常量因此一度被参数组合校验与常量加密两处按"字符串"
-- 处理，导致 numeric/float 列的裸小数常量报错：
--     校验阶段：GREATEST does not support this argument combination:
--               the constant type does not match the encrypted column type
--     改写阶段：GREATEST types <enc> and numeric cannot be matched
--               GREATEST could not convert type text_enc to <enc>
-- 现由 Const.text_numeric_literal 标记区分"数值文本"与真正的字符串字面量，
-- 前者按数值族处理、按列基类型加密改写；后者（如 '123'）语义不变。
--
-- 注意：字符串字面量 '123' 不会被误判为数值 —— 它走 STRING 产生式，
-- 不带该标记（见文件末尾的回归对照）。
-- ============================================================================

-- ### 环境准备 ###############################################################
DROP TABLE IF EXISTS enc_pgfunc_bc CASCADE;
DROP DEK IF EXISTS enc_pgfunc_bc_dek CASCADE;
DROP KEK IF EXISTS enc_pgfunc_bc_kek CASCADE;

CREATE KEK enc_pgfunc_bc_kek WITH (key_store = 'localkms', key_path = '/tmp/enc_pgfunc_bc_kek.key');
CREATE DEK enc_pgfunc_bc_dek WITH VALUES(KEK = 'enc_pgfunc_bc_kek');

CREATE TABLE enc_pgfunc_bc (
    id       int,
    c_int2   int2_enc    ENCRYPTED WITH (DEK = enc_pgfunc_bc_dek, support_algo = 'ope'),
    c_int4   int4_enc    ENCRYPTED WITH (DEK = enc_pgfunc_bc_dek, support_algo = 'ope'),
    c_int8   int8_enc    ENCRYPTED WITH (DEK = enc_pgfunc_bc_dek, support_algo = 'ope'),
    c_num    numeric_enc ENCRYPTED WITH (DEK = enc_pgfunc_bc_dek, support_algo = 'ope'),
    c_f4     float4_enc  ENCRYPTED WITH (DEK = enc_pgfunc_bc_dek, support_algo = 'ope'),
    c_f8     float8_enc  ENCRYPTED WITH (DEK = enc_pgfunc_bc_dek, support_algo = 'ope'),
    c_text   text_enc    ENCRYPTED WITH (DEK = enc_pgfunc_bc_dek, support_algo = 'ope'),
    c_vchar  varchar_enc ENCRYPTED WITH (DEK = enc_pgfunc_bc_dek, support_algo = 'ope'),
    c_bpchar bpchar_enc  ENCRYPTED WITH (DEK = enc_pgfunc_bc_dek, support_algo = 'ope')
);
-- 每列两个值：常量取值居中，使两行分别命中"取列值"与"取常量"
INSERT INTO enc_pgfunc_bc VALUES
  (1,     100,        4000,        5000000,   1000.50,  6.25,        1.5, 'apple', 'apple', 'apple'),
  (2,   32000,  2000000000,  9000000000000,      99.99,  99.5,  12345.6789, 'zebra', 'zebra', 'zebra');

-- ### 1. 整数族：int2_enc / int4_enc / int8_enc + 裸整型常量 ####################
\echo '========== 1. 整数族 + 裸整型常量 =========='
SELECT 'BC-int2', GREATEST(c_int2, 2000), LEAST(c_int2, 2000),
       'BC-int4', GREATEST(c_int4, 100000), LEAST(c_int4, 100000),
       'BC-int8', GREATEST(c_int8, 6000000), LEAST(c_int8, 6000000)
FROM enc_pgfunc_bc ORDER BY id;

-- ### 2. 小数/浮点族 + 裸小数字面量（本次修复的核心场景） ######################
\echo '========== 2. 小数/浮点族 + 裸小数字面量 =========='
SELECT 'BC-numeric', GREATEST(c_num, 500.00), LEAST(c_num, 500.00),
       'BC-float4',  GREATEST(c_f4, 50.5),   LEAST(c_f4, 50.5),
       'BC-float8',  GREATEST(c_f8, 100.0),  LEAST(c_f8, 100.0)
FROM enc_pgfunc_bc ORDER BY id;

-- ### 3. 字符族 + 裸字符串常量 ###############################################
\echo '========== 3. 字符族 + 裸字符串常量 =========='
SELECT 'BC-text',   GREATEST(c_text, 'banana'),   LEAST(c_text, 'banana'),
       'BC-varchar', GREATEST(c_vchar, 'banana'),  LEAST(c_vchar, 'banana'),
       'BC-bpchar',  GREATEST(c_bpchar, 'banana'), LEAST(c_bpchar, 'banana')
FROM enc_pgfunc_bc ORDER BY id;

-- ### 4. 同族跨密态类型 + 常量（沿用拓宽格） #################################
\echo '========== 4. 同族跨密态类型 + 常量 =========='
SELECT 'BC-int2-x-int8-x-10000', GREATEST(c_int2, c_int8, 10000),
       'BC-f4-x-f8-x-100.0',     GREATEST(c_f4, c_f8, 100.0),
       'BC-text-x-varchar-x-melon', GREATEST(c_text, c_vchar, 'melon'),
       'BC-text-x-bpchar-x-melon', GREATEST(c_text, c_bpchar, 'melon')
FROM enc_pgfunc_bc ORDER BY id;

-- ### 5. 回归对照：字符串字面量语义不变 ######################################
-- '123' 走 STRING 产生式，不带 text_numeric_literal 标记，仍按字符串族处理；
-- 若被误判为数值，这条会因跨族而被拒绝（GREATEST does not support ...）。
\echo '========== 5. 字符串字面量 ''123'' 仍按字符串比较 =========='
SELECT 'BC-str-123', GREATEST(c_text, '123'), LEAST(c_text, '123'),
       'BC-str-banana', GREATEST(c_text, 'banana'), LEAST(c_text, 'banana')
FROM enc_pgfunc_bc ORDER BY id;

-- ### 6. 多常量（三参数） ####################################################
\echo '========== 6. 多常量三参数 =========='
SELECT 'BC-multi-const', GREATEST(c_num, 500.00, 5000.25), LEAST(c_num, 500.00, 5000.25),
       'BC-multi-float', GREATEST(c_f8, 100.0, 20000.5), LEAST(c_f8, 100.0, 20000.5)
FROM enc_pgfunc_bc ORDER BY id;

-- ### 清理 ###################################################################
\echo '========== 7. 清理 =========='
DROP TABLE enc_pgfunc_bc CASCADE;
DROP DEK enc_pgfunc_bc_dek CASCADE;
DROP KEK enc_pgfunc_bc_kek CASCADE;
