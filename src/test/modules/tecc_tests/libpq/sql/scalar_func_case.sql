-- ============================================================================
-- TECC —— CASE 表达式分支常量类型兼容性（需求 4.4.1.4）
--
-- 需求原文：CASE 的 WHEN 条件引用密态列时，条件常量按列的算法加密；THEN/ELSE
-- 分支可返回密态列（结果解密返回）或普通常量，支持密态列与普通常量混用：
-- 混用时普通常量须与密态列的原始类型兼容；不兼容时报错。
--
-- 背景：浮点字面量在前端被原样保留为 TEXTOID 字符串常量以保精度（见
-- frontend_parser/tecc_grammar.y 的 FLOAT 产生式）。CASE 分支校验原先直接
-- 用 const_expr->consttype(=TEXTOID) 判定兼容性，导致
--     CASE WHEN c_f4 > 10 THEN c_f4 ELSE 0.5 END
-- 被误判为"分支常量类型无法按密态列加密"。现带 text_numeric_literal 标记的
-- 常量按其数值语义参与判定与 enc 类型映射。
--
-- 覆盖：浮点/数值列的分支浮点常量（正向）、WHEN 条件中的浮点常量、
-- 整型列配整型常量（需求文档示例）、字符列配字符串常量、
-- 不兼容组合仍报错、switch 形式仍不支持。
-- ============================================================================

-- ### 环境准备 ###############################################################
DROP TABLE IF EXISTS enc_pgfunc_case CASCADE;
DROP DEK IF EXISTS enc_pgfunc_case_dek CASCADE;
DROP KEK IF EXISTS enc_pgfunc_case_kek CASCADE;

CREATE KEK enc_pgfunc_case_kek WITH (key_store = 'localkms', key_path = '/tmp/enc_pgfunc_case_kek.key');
CREATE DEK enc_pgfunc_case_dek WITH VALUES(KEK = 'enc_pgfunc_case_kek');

CREATE TABLE enc_pgfunc_case (
    id     int,
    c_i4   int4_enc    ENCRYPTED WITH (DEK = enc_pgfunc_case_dek, support_algo = 'ope'),
    c_num  numeric_enc ENCRYPTED WITH (DEK = enc_pgfunc_case_dek, support_algo = 'ope'),
    c_f4   float4_enc  ENCRYPTED WITH (DEK = enc_pgfunc_case_dek, support_algo = 'ope'),
    c_f8   float8_enc  ENCRYPTED WITH (DEK = enc_pgfunc_case_dek, support_algo = 'ope'),
    c_text text_enc    ENCRYPTED WITH (DEK = enc_pgfunc_case_dek, support_algo = 'ope')
);
INSERT INTO enc_pgfunc_case VALUES
  (1, 4000, 1000.50,  6.25,     1.5, 'apple'),
  (2,    0,   99.99,  99.5, 12345.6789, 'zebra');

-- ### 1. 整型列 + 整型常量（需求文档示例：CASE WHEN val > 150 THEN val ELSE 0 END）
\echo '========== 1. int 列 + int 常量（需求示例） =========='
SELECT id, CASE WHEN c_i4 > 150 THEN c_i4 ELSE 0 END AS r FROM enc_pgfunc_case ORDER BY id;

-- ### 2. 字符列 + 字符串常量
\echo '========== 2. text 列 + 字符串常量 =========='
SELECT id, CASE WHEN c_text > 'a' THEN c_text ELSE 'zzz' END AS r FROM enc_pgfunc_case ORDER BY id;

-- ### 3. 浮点/数值列 + 裸浮点分支常量（本次修复的核心场景）
\echo '========== 3. float4 列 + 浮点 ELSE 分支常量 =========='
SELECT id, CASE WHEN c_f4 > 10 THEN c_f4 ELSE 0.5 END AS r FROM enc_pgfunc_case ORDER BY id;

\echo '========== 3b. float4 列 + 浮点常量（WHEN 条件与 ELSE 分支同时） =========='
SELECT id, CASE WHEN c_f4 > 10.5 THEN c_f4 ELSE 0.25 END AS r FROM enc_pgfunc_case ORDER BY id;

\echo '========== 3c. float8 列 + 浮点常量 =========='
SELECT id, CASE WHEN c_f8 > 100.0 THEN c_f8 ELSE 0.25 END AS r FROM enc_pgfunc_case ORDER BY id;

\echo '========== 3d. numeric 列 + 浮点常量 =========='
SELECT id, CASE WHEN c_num > 500.0 THEN c_num ELSE 1.5 END AS r FROM enc_pgfunc_case ORDER BY id;

-- ### 4. WHEN 条件中的浮点常量（分支用密态列）
\echo '========== 4. WHEN 条件含浮点常量 =========='
SELECT id, CASE WHEN c_f4 > 10.5 THEN c_f4 ELSE c_f4 END AS r FROM enc_pgfunc_case ORDER BY id;

-- ### 5. 多分支
\echo '========== 5. 多 WHEN 分支 + 浮点常量 =========='
SELECT id,
       CASE WHEN c_f4 > 50 THEN c_f4
            WHEN c_f4 > 10 THEN 20.5
            ELSE 0.5 END AS r
FROM enc_pgfunc_case ORDER BY id;

-- ### 6. 不兼容组合仍应报错
\echo '========== 6. int 列 + 浮点分支常量（不兼容，应报错） =========='
SELECT id, CASE WHEN c_i4 > 150 THEN c_i4 ELSE 0.5 END AS r FROM enc_pgfunc_case ORDER BY id;

-- ### 7. switch 形式仍不支持（有意设计）
\echo '========== 7. switch 形式（应报不支持） =========='
SELECT id, CASE c_f4 WHEN 1.5 THEN c_f4 ELSE 0.5 END AS r FROM enc_pgfunc_case ORDER BY id;

-- ### 8. 无密态列时不受影响
\echo '========== 8. 纯普通 CASE =========='
SELECT id, CASE WHEN id = 1 THEN 0.5 ELSE 1.5 END AS r FROM enc_pgfunc_case ORDER BY id;

-- ### 清理 ###################################################################
\echo '========== 9. 清理 =========='
DROP TABLE enc_pgfunc_case CASCADE;
DROP DEK enc_pgfunc_case_dek CASCADE;
DROP KEK enc_pgfunc_case_kek CASCADE;
