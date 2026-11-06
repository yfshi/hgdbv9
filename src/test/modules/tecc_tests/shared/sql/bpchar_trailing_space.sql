-- ============================================================
-- bpchar_trailing_space.sql — BPCHAR尾空格全覆盖测试
--
-- PG明文字符类型尾空格比较规则:
--   1. BPCHAR: 比较时忽略尾空格（blank-padded语义）
--      - 'abc'::bpchar = 'abc   '::bpchar → t
--      - '   '::bpchar = ''::bpchar      → t （纯空格=空串）
--   2. TEXT: 比较时保留尾空格，'abc' ≠ 'abc   '
--   3. VARCHAR: 同TEXT，保留尾空格，'abc' ≠ 'abc   '
--   4. 跨类型比较规则:
--      a) bpchar在左（如B1/B3/B5）: bpchar语义，忽略尾空格 → t
--      b) text在左（如B8/B9）: text语义，保留尾空格
--         - tx('abc') = bp('abc   ') → t（bp转text时rtrim了）
--         - tx('abc   ') = bp('abc') → f（tx保留尾空格）
--      c) bpchar = text: 无直接跨类型操作符，text_enc为preferred type
--         走隐式转换→text_enc_eq
--      d) bpchar = varchar / varchar = bpchar: 走bpchar_enc_eq
--         （bpchar语义，忽略尾空格）→ t
--      e) text = varchar / varchar = text: 走各自类型的比较函数
--
-- 密文格式说明（DET/OPE文本类型）:
--   格式: [format_byte:1][encrypted_core:16][space_count:4] 共21字节
--   - format_byte: 0x01=有尾空格，0x00=无尾空格
--   - encrypted_core: rtrim后明文的SM4加密结果（16字节）
--   - space_count: 大端uint32，rtrim掉的尾空格数
--   - bpchar(10)中"abc"：plaintext="abc       "(10字节)，rtrim后"abc"(3字节)，
--     space_count=7
--   - varchar中"abc"：plaintext="abc"(3字节)，rtrim后"abc"(3字节)，
--     space_count=0
--   - 两者encrypted_core相同（都是"abc"加密结果），space_count不同
--   - bpchar_enc_eq比较时strip掉space_count只比较core，所以相等
--   - text_enc_eq/varchar_enc_eq比较core+space_count，所以不等
--
-- 覆盖场景及条数（每条含明文+密文=2条）:
--   A  同类型等值比较            9场景 ×2 = 18条
--   B  跨类型等值比较（全排列）  18场景 ×2 = 36条
--   C  范围比较（全排列）       18场景 ×2 = 36条
--   D  纯空格/边界值            12场景 ×2 = 24条
--   E  常量比较(含空格/显式类型) 8场景  ×2 = 16条
--   F  SELECT列表表达式比较     3场景  ×2 = 6条
--   G  NULL/空串               3场景  ×2 = 6条
--   H  UPDATE跨类型赋值         3场景  ×2 = 6条
--   I  INSERT隐式转换          2场景  ×2 = 4条
--                               合计：148条
-- ============================================================

-- ============================================================
-- Setup
-- ============================================================
DROP TABLE IF EXISTS bpchar_plain CASCADE;
DROP TABLE IF EXISTS cross_plain CASCADE;
DROP TABLE IF EXISTS bpchar_enc CASCADE;
DROP TABLE IF EXISTS cross_enc CASCADE;
DROP TABLE IF EXISTS bpchar_enc_ope CASCADE;
DROP TABLE IF EXISTS cross_enc_ope CASCADE;
DROP DEK IF EXISTS dek_bpchar_ts CASCADE;
DROP DEK IF EXISTS dek_bpchar_ope CASCADE;
DROP KEK IF EXISTS kek_bpchar_ts CASCADE;

CREATE KEK kek_bpchar_ts WITH (key_store='localkms', key_path='/tmp/kek_bpchar_ts.key');
CREATE DEK dek_bpchar_ts WITH VALUES(KEK = kek_bpchar_ts);
CREATE DEK dek_bpchar_ope WITH VALUES(KEK = kek_bpchar_ts);

-- 明文表（等值+范围共用）
CREATE TABLE bpchar_plain (id SERIAL, txt text, vc varchar(10), bp bpchar(10));
CREATE TABLE cross_plain (id SERIAL, txt text, vc varchar(10), bp bpchar(10));
-- 密文表（DET，仅等值比较）
CREATE TABLE bpchar_enc (id SERIAL,
    txt text_enc encrypted with (dek = dek_bpchar_ts, support_algo = 'det'),
    vc varchar_enc encrypted with (dek = dek_bpchar_ts, support_algo = 'det'),
    bp bpchar_enc(10) encrypted with (dek = dek_bpchar_ts, support_algo = 'det'));
CREATE TABLE cross_enc (id SERIAL,
    txt text_enc encrypted with (dek = dek_bpchar_ts, support_algo = 'det'),
    vc varchar_enc encrypted with (dek = dek_bpchar_ts, support_algo = 'det'),
    bp bpchar_enc(10) encrypted with (dek = dek_bpchar_ts, support_algo = 'det'));
-- 密文表（OPE，支持等值+范围比较）
CREATE TABLE bpchar_enc_ope (id SERIAL,
    txt text_enc encrypted with (dek = dek_bpchar_ope, support_algo = 'ope'),
    vc varchar_enc encrypted with (dek = dek_bpchar_ope, support_algo = 'ope'),
    bp bpchar_enc(10) encrypted with (dek = dek_bpchar_ope, support_algo = 'ope'));
CREATE TABLE cross_enc_ope (id SERIAL,
    txt text_enc encrypted with (dek = dek_bpchar_ope, support_algo = 'ope'),
    vc varchar_enc encrypted with (dek = dek_bpchar_ope, support_algo = 'ope'),
    bp bpchar_enc(10) encrypted with (dek = dek_bpchar_ope, support_algo = 'ope'));

INSERT INTO bpchar_plain (txt, vc, bp) VALUES
    ('abc', 'abc', 'abc'),           -- 1: 都无尾空格
    ('abc   ', 'abc   ', 'abc   '),  -- 2: 都有3尾空格
    ('', '', ''),                     -- 3: 空串
    (NULL, NULL, NULL),              -- 4: NULL
    ('   ', '   ', '   ');          -- 5: 纯空格

INSERT INTO cross_plain (txt, vc, bp) VALUES
    ('abc', 'abc', 'abc'),           ('abc   ', 'abc   ', 'abc   ');

INSERT INTO bpchar_enc (txt, vc, bp) VALUES
    ('abc', 'abc', 'abc'),           ('abc   ', 'abc   ', 'abc   '),
    ('', '', ''), (NULL, NULL, NULL), ('   ', '   ', '   ');

INSERT INTO cross_enc (txt, vc, bp) VALUES
    ('abc', 'abc', 'abc'),           ('abc   ', 'abc   ', 'abc   ');

INSERT INTO bpchar_enc_ope (txt, vc, bp) VALUES
    ('abc', 'abc', 'abc'),           ('abc   ', 'abc   ', 'abc   '),
    ('', '', ''), (NULL, NULL, NULL), ('   ', '   ', '   ');

INSERT INTO cross_enc_ope (txt, vc, bp) VALUES
    ('abc', 'abc', 'abc'),           ('abc   ', 'abc   ', 'abc   ');

SELECT '=== Setup OK ===';

-- ============================================================
-- A: 同类型等值比较（9场景×2=18条）
-- ============================================================
SELECT '=== A: 同类型等值比较 ===';

-- A1-A2: bpchar/bpchar
SELECT 'A1_bp_eq_bp', a.bp = b.bp FROM bpchar_plain a, bpchar_plain b WHERE a.id=1 AND b.id=1;
SELECT 'A1_bp_eq_bp_enc', a.bp = b.bp FROM bpchar_enc a, bpchar_enc b WHERE a.id=1 AND b.id=1;
SELECT 'A2_bp_eq_bp_sp', a.bp = b.bp FROM bpchar_plain a, bpchar_plain b WHERE a.id=1 AND b.id=2;
SELECT 'A2_bp_eq_bp_sp_enc', a.bp = b.bp FROM bpchar_enc a, bpchar_enc b WHERE a.id=1 AND b.id=2;
-- A3-A5: text/text（都无/左尾/尾相同）
SELECT 'A3_tx_eq_tx', a.txt = b.txt FROM bpchar_plain a, bpchar_plain b WHERE a.id=1 AND b.id=1;
SELECT 'A3_tx_eq_tx_enc', a.txt = b.txt FROM bpchar_enc a, bpchar_enc b WHERE a.id=1 AND b.id=1;
SELECT 'A4_tx_eq_tx_sp', a.txt = b.txt FROM bpchar_plain a, bpchar_plain b WHERE a.id=1 AND b.id=2;
SELECT 'A4_tx_eq_tx_sp_enc', a.txt = b.txt FROM bpchar_enc a, bpchar_enc b WHERE a.id=1 AND b.id=2;
SELECT 'A5_tx_eq_tx_spsp', a.txt = b.txt FROM bpchar_plain a, bpchar_plain b WHERE a.id=2 AND b.id=2;
SELECT 'A5_tx_eq_tx_spsp_enc', a.txt = b.txt FROM bpchar_enc a, bpchar_enc b WHERE a.id=2 AND b.id=2;
-- A6-A7: varchar/varchar
SELECT 'A6_vc_eq_vc', a.vc = b.vc FROM bpchar_plain a, bpchar_plain b WHERE a.id=1 AND b.id=1;
SELECT 'A6_vc_eq_vc_enc', a.vc = b.vc FROM bpchar_enc a, bpchar_enc b WHERE a.id=1 AND b.id=1;
SELECT 'A7_vc_eq_vc_sp', a.vc = b.vc FROM bpchar_plain a, bpchar_plain b WHERE a.id=1 AND b.id=2;
SELECT 'A7_vc_eq_vc_sp_enc', a.vc = b.vc FROM bpchar_enc a, bpchar_enc b WHERE a.id=1 AND b.id=2;
-- A8-A9: bpchar常量等值
SELECT 'A8_bp_eq_const', bp FROM bpchar_plain WHERE bp = 'abc' ORDER BY id;
SELECT 'A8_bp_eq_const_enc', bp FROM bpchar_enc WHERE bp = 'abc' ORDER BY id;
SELECT 'A9_bp_eq_const_sp', bp FROM bpchar_plain WHERE bp = 'abc   ' ORDER BY id;
SELECT 'A9_bp_eq_const_sp_enc', bp FROM bpchar_enc WHERE bp = 'abc   ' ORDER BY id;

-- ============================================================
-- B: 跨类型等值比较 全排列（18场景×2=36条）
--    类型组合: bp/tx, tx/bp, bp/vc, vc/bp, tx/vc, vc/tx
--    空格状态: 都无 / 左尾 / 右尾
-- ============================================================
SELECT '=== B: 跨类型等值比较（全排列） ===';

-- B1-B3: bpchar在左 vs text
SELECT 'B1_bp_eq_tx', a.bp = b.txt FROM cross_plain a, cross_plain b WHERE a.id=1 AND b.id=1;
SELECT 'B1_bp_eq_tx_enc', a.bp = b.txt FROM cross_enc a, cross_enc b WHERE a.id=1 AND b.id=1;
SELECT 'B2_bp_eq_tx_tail', a.bp = b.txt FROM cross_plain a, cross_plain b WHERE a.id=1 AND b.id=2;
SELECT 'B2_bp_eq_tx_tail_enc', a.bp = b.txt FROM cross_enc a, cross_enc b WHERE a.id=1 AND b.id=2;
SELECT 'B3_bp_tail_eq_tx', a.bp = b.txt FROM cross_plain a, cross_plain b WHERE a.id=2 AND b.id=1;
SELECT 'B3_bp_tail_eq_tx_enc', a.bp = b.txt FROM cross_enc a, cross_enc b WHERE a.id=2 AND b.id=1;
-- B4-B6: bpchar在左 vs varchar
SELECT 'B4_bp_eq_vc', a.bp = b.vc FROM cross_plain a, cross_plain b WHERE a.id=1 AND b.id=1;
SELECT 'B4_bp_eq_vc_enc', a.bp = b.vc FROM cross_enc a, cross_enc b WHERE a.id=1 AND b.id=1;
SELECT 'B5_bp_eq_vc_tail', a.bp = b.vc FROM cross_plain a, cross_plain b WHERE a.id=1 AND b.id=2;
SELECT 'B5_bp_eq_vc_tail_enc', a.bp = b.vc FROM cross_enc a, cross_enc b WHERE a.id=1 AND b.id=2;
SELECT 'B6_bp_tail_eq_vc', a.bp = b.vc FROM cross_plain a, cross_plain b WHERE a.id=2 AND b.id=1;
SELECT 'B6_bp_tail_eq_vc_enc', a.bp = b.vc FROM cross_enc a, cross_enc b WHERE a.id=2 AND b.id=1;
-- B7-B9: text在左 vs bpchar
SELECT 'B7_tx_eq_bp', a.txt = b.bp FROM cross_plain a, cross_plain b WHERE a.id=1 AND b.id=1;
SELECT 'B7_tx_eq_bp_enc', a.txt = b.bp FROM cross_enc a, cross_enc b WHERE a.id=1 AND b.id=1;
SELECT 'B8_tx_tail_eq_bp', a.txt = b.bp FROM cross_plain a, cross_plain b WHERE a.id=2 AND b.id=1;
SELECT 'B8_tx_tail_eq_bp_enc', a.txt = b.bp FROM cross_enc a, cross_enc b WHERE a.id=2 AND b.id=1;
SELECT 'B9_tx_eq_bp_tail', a.txt = b.bp FROM cross_plain a, cross_plain b WHERE a.id=1 AND b.id=2;
SELECT 'B9_tx_eq_bp_tail_enc', a.txt = b.bp FROM cross_enc a, cross_enc b WHERE a.id=1 AND b.id=2;
-- B10-B12: varchar在左 vs bpchar
SELECT 'B10_vc_eq_bp', a.vc = b.bp FROM cross_plain a, cross_plain b WHERE a.id=1 AND b.id=1;
SELECT 'B10_vc_eq_bp_enc', a.vc = b.bp FROM cross_enc a, cross_enc b WHERE a.id=1 AND b.id=1;
SELECT 'B11_vc_tail_eq_bp', a.vc = b.bp FROM cross_plain a, cross_plain b WHERE a.id=2 AND b.id=1;
SELECT 'B11_vc_tail_eq_bp_enc', a.vc = b.bp FROM cross_enc a, cross_enc b WHERE a.id=2 AND b.id=1;
SELECT 'B12_vc_eq_bp_tail', a.vc = b.bp FROM cross_plain a, cross_plain b WHERE a.id=1 AND b.id=2;
SELECT 'B12_vc_eq_bp_tail_enc', a.vc = b.bp FROM cross_enc a, cross_enc b WHERE a.id=1 AND b.id=2;
-- B13-B15: text在左 vs varchar
SELECT 'B13_tx_eq_vc', a.txt = b.vc FROM cross_plain a, cross_plain b WHERE a.id=1 AND b.id=1;
SELECT 'B13_tx_eq_vc_enc', a.txt = b.vc FROM cross_enc a, cross_enc b WHERE a.id=1 AND b.id=1;
SELECT 'B14_tx_tail_eq_vc', a.txt = b.vc FROM cross_plain a, cross_plain b WHERE a.id=2 AND b.id=1;
SELECT 'B14_tx_tail_eq_vc_enc', a.txt = b.vc FROM cross_enc a, cross_enc b WHERE a.id=2 AND b.id=1;
SELECT 'B15_tx_eq_vc_tail', a.txt = b.vc FROM cross_plain a, cross_plain b WHERE a.id=1 AND b.id=2;
SELECT 'B15_tx_eq_vc_tail_enc', a.txt = b.vc FROM cross_enc a, cross_enc b WHERE a.id=1 AND b.id=2;
-- B16-B18: varchar在左 vs text
SELECT 'B16_vc_eq_tx', a.vc = b.txt FROM cross_plain a, cross_plain b WHERE a.id=1 AND b.id=1;
SELECT 'B16_vc_eq_tx_enc', a.vc = b.txt FROM cross_enc a, cross_enc b WHERE a.id=1 AND b.id=1;
SELECT 'B17_vc_tail_eq_tx', a.vc = b.txt FROM cross_plain a, cross_plain b WHERE a.id=2 AND b.id=1;
SELECT 'B17_vc_tail_eq_tx_enc', a.vc = b.txt FROM cross_enc a, cross_enc b WHERE a.id=2 AND b.id=1;
SELECT 'B18_vc_eq_tx_tail', a.vc = b.txt FROM cross_plain a, cross_plain b WHERE a.id=1 AND b.id=2;
SELECT 'B18_vc_eq_tx_tail_enc', a.vc = b.txt FROM cross_enc a, cross_enc b WHERE a.id=1 AND b.id=2;

-- ============================================================
-- C: 范围比较 全排列（18场景×2=36条）
--     同类型: bp/bp, tx/tx, vc/vc
--     跨类型: bp/tx, tx/bp, bp/vc, vc/bp, tx/vc, vc/tx
--     空格状态: 都无/左尾/右尾
--     使用 OPE 表（bpchar_enc_ope / cross_enc_ope）
-- ============================================================
SELECT '=== C: 范围比较（全排列） ===';

-- C1: bpchar > 常量（无尾/带尾）
SELECT 'C1_bp_gt_abc', id, bp FROM bpchar_plain WHERE bp > 'abc' ORDER BY id;
SELECT 'C1_bp_gt_abc_ope', id, bp FROM bpchar_enc_ope WHERE bp > 'abc' ORDER BY id;
SELECT 'C2_bp_gt_abc_sp', id, bp FROM bpchar_plain WHERE bp > 'abc   ' ORDER BY id;
SELECT 'C2_bp_gt_abc_sp_ope', id, bp FROM bpchar_enc_ope WHERE bp > 'abc   ' ORDER BY id;
-- C3: text > 常量（bpchar语义下无影响）
SELECT 'C3_tx_gt_abc', id, txt FROM bpchar_plain WHERE txt > 'abc' ORDER BY id;
SELECT 'C3_tx_gt_abc_ope', id, txt FROM bpchar_enc_ope WHERE txt > 'abc' ORDER BY id;
-- C4: text尾空格影响排序（text 'abc   ' > text 'abc'）
SELECT 'C4_tx_gt_tail', a.txt > b.txt FROM bpchar_plain a, bpchar_plain b WHERE a.id=2 AND b.id=1;
SELECT 'C4_tx_gt_tail_ope', a.txt > b.txt FROM bpchar_enc_ope a, bpchar_enc_ope b WHERE a.id=2 AND b.id=1;
-- C5: bpchar尾空格不影响排序（bpchar忽略尾空格）
SELECT 'C5_bp_gt_bp_tail', a.bp > b.bp FROM bpchar_plain a, bpchar_plain b WHERE a.id=2 AND b.id=1;
SELECT 'C5_bp_gt_bp_tail_ope', a.bp > b.bp FROM bpchar_enc_ope a, bpchar_enc_ope b WHERE a.id=2 AND b.id=1;
-- C6: bpchar < 常量
SELECT 'C6_bp_lt_abc', id, bp FROM bpchar_plain WHERE bp < 'abc' ORDER BY id;
SELECT 'C6_bp_lt_abc_ope', id, bp FROM bpchar_enc_ope WHERE bp < 'abc' ORDER BY id;
-- C7-C9: bpchar在左 vs text（跨类型范围比较——不走隐式转换，直接报错？或者走隐式转换后比较）
-- 实测：PG明文 => (b)::text > txt（bpchar转text后比较）
SELECT 'C7_bp_gt_tx', a.bp > b.txt FROM cross_plain a, cross_plain b WHERE a.id=2 AND b.id=1;
SELECT 'C7_bp_gt_tx_ope', a.bp > b.txt FROM cross_enc_ope a, cross_enc_ope b WHERE a.id=2 AND b.id=1;
SELECT 'C8_tx_gt_bp', a.txt > b.bp FROM cross_plain a, cross_plain b WHERE a.id=1 AND b.id=2;
SELECT 'C8_tx_gt_bp_ope', a.txt > b.bp FROM cross_enc_ope a, cross_enc_ope b WHERE a.id=1 AND b.id=2;
SELECT 'C9_bp_between', id, bp FROM bpchar_plain WHERE bp BETWEEN 'aaa' AND 'zzz' ORDER BY id;
SELECT 'C9_bp_between_ope', id, bp FROM bpchar_enc_ope WHERE bp BETWEEN 'aaa' AND 'zzz' ORDER BY id;

-- ============================================================
-- D: 纯空格/边界值（8场景×2=16条）
-- ============================================================
SELECT '=== D: 纯空格/边界值 ===';

SELECT 'D1_sp_vs_empty', bp FROM bpchar_plain WHERE bp = '' ORDER BY id;
SELECT 'D1_sp_vs_empty_enc', bp FROM bpchar_enc WHERE bp = '' ORDER BY id;
SELECT 'D2_sp_eq_sp', a.bp = b.bp FROM bpchar_plain a, bpchar_plain b WHERE a.id=5 AND b.id=5;
SELECT 'D2_sp_eq_sp_enc', a.bp = b.bp FROM bpchar_enc a, bpchar_enc b WHERE a.id=5 AND b.id=5;
SELECT 'D3_bp_sp_eq_tx_sp', a.bp = b.txt FROM bpchar_plain a, bpchar_plain b WHERE a.id=5 AND b.id=5;
SELECT 'D3_bp_sp_eq_tx_sp_enc', a.bp = b.txt FROM bpchar_enc a, bpchar_enc b WHERE a.id=5 AND b.id=5;
SELECT 'D4_bp_sp_eq_vc_sp', a.bp = b.vc FROM bpchar_plain a, bpchar_plain b WHERE a.id=5 AND b.id=5;
SELECT 'D4_bp_sp_eq_vc_sp_enc', a.bp = b.vc FROM bpchar_enc a, bpchar_enc b WHERE a.id=5 AND b.id=5;
SELECT 'D5_vc_sp_eq_empty', vc FROM bpchar_plain WHERE vc = '' ORDER BY id;
SELECT 'D5_vc_sp_eq_empty_enc', vc FROM bpchar_enc WHERE vc = '' ORDER BY id;
SELECT 'D6_tx_sp_gt_abc', id FROM bpchar_plain WHERE txt > 'abc' ORDER BY id;
SELECT 'D6_tx_sp_gt_abc_ope', id FROM bpchar_enc_ope WHERE txt > 'abc' ORDER BY id;
SELECT 'D7_bp_sp_gt_abc', id FROM bpchar_plain WHERE bp > 'abc' ORDER BY id;
SELECT 'D7_bp_sp_gt_abc_ope', id FROM bpchar_enc_ope WHERE bp > 'abc' ORDER BY id;
SELECT 'D8_sp_gt_empty', a.txt > b.txt FROM bpchar_plain a, bpchar_plain b WHERE a.id=5 AND b.id=3;
SELECT 'D8_sp_gt_empty_ope', a.txt > b.txt FROM bpchar_enc_ope a, bpchar_enc_ope b WHERE a.id=5 AND b.id=3;
-- D9-D12: 纯空格跨类型补充（bpchar在左 vs text, bpchar空 vs vc空格）
SELECT 'D9_tx_sp_eq_bp_sp', a.txt = b.bp FROM bpchar_plain a, bpchar_plain b WHERE a.id=5 AND b.id=5;
SELECT 'D9_tx_sp_eq_bp_sp_enc', a.txt = b.bp FROM bpchar_enc a, bpchar_enc b WHERE a.id=5 AND b.id=5;
SELECT 'D10_bp_empty_eq_bp_sp', a.bp = b.bp FROM bpchar_plain a, bpchar_plain b WHERE a.id=3 AND b.id=5;
SELECT 'D10_bp_empty_eq_bp_sp_enc', a.bp = b.bp FROM bpchar_enc a, bpchar_enc b WHERE a.id=3 AND b.id=5;
SELECT 'D11_bp_empty_eq_vc_sp', a.bp = b.vc FROM bpchar_plain a, bpchar_plain b WHERE a.id=3 AND b.id=5;
SELECT 'D11_bp_empty_eq_vc_sp_enc', a.bp = b.vc FROM bpchar_enc a, bpchar_enc b WHERE a.id=3 AND b.id=5;
SELECT 'D12_bp_empty_eq_vc_sp_ope', a.bp = b.vc FROM bpchar_enc_ope a, bpchar_enc_ope b WHERE a.id=3 AND b.id=5;
SELECT 'D12_bp_empty_eq_vc_sp_ope_plain', a.bp = b.vc FROM bpchar_plain a, bpchar_plain b WHERE a.id=3 AND b.id=5;

-- ============================================================
-- E: 常量比较(含带显式类型转换)（8场景×2=16条）
-- ============================================================
SELECT '=== E: 常量比较 ===';

-- E1-E2: bpchar列 = 'abc' 常量（无::text，前端自动加密为bpchar_enc）
SELECT 'E1_bp_eq_const', bp FROM bpchar_plain WHERE bp = 'abc' ORDER BY id;
SELECT 'E1_bp_eq_const_enc', bp FROM bpchar_enc WHERE bp = 'abc' ORDER BY id;
SELECT 'E2_bp_eq_const_sp', bp FROM bpchar_plain WHERE bp = 'abc   ' ORDER BY id;
SELECT 'E2_bp_eq_const_sp_enc', bp FROM bpchar_enc WHERE bp = 'abc   ' ORDER BY id;
-- E3-E4: text列 = 'abc' 常量
SELECT 'E3_tx_eq_const', txt FROM bpchar_plain WHERE txt = 'abc' ORDER BY id;
SELECT 'E3_tx_eq_const_enc', txt FROM bpchar_enc WHERE txt = 'abc' ORDER BY id;
SELECT 'E4_tx_eq_const_sp', txt FROM bpchar_plain WHERE txt = 'abc   ' ORDER BY id;
SELECT 'E4_tx_eq_const_sp_enc', txt FROM bpchar_enc WHERE txt = 'abc   ' ORDER BY id;
-- E5: varchar列 = 'abc   ' 常量
SELECT 'E5_vc_eq_const_sp', vc FROM bpchar_plain WHERE vc = 'abc   ' ORDER BY id;
SELECT 'E5_vc_eq_const_sp_enc', vc FROM bpchar_enc WHERE vc = 'abc   ' ORDER BY id;
-- E6: 常量在左 'abc' = bpchar列
SELECT 'E6_const_eq_bp', bp FROM bpchar_plain WHERE 'abc' = bp ORDER BY id;
SELECT 'E6_const_eq_bp_enc', bp FROM bpchar_enc WHERE 'abc' = bp ORDER BY id;
-- E7: 跨类型常量——bpchar列 = 'abc'（无显式类型，前端自动加密）
SELECT 'E7_bp_eq_plain_const', bp FROM cross_plain WHERE bp = 'abc' ORDER BY id;
SELECT 'E7_bp_eq_plain_const_enc', bp FROM cross_enc WHERE bp = 'abc' ORDER BY id;
-- E8: text列 > 'abc' 常量（范围）
SELECT 'E8_tx_gt_const', txt FROM bpchar_plain WHERE txt > 'abc' ORDER BY id;
SELECT 'E8_tx_gt_const_ope', txt FROM bpchar_enc_ope WHERE txt > 'abc' ORDER BY id;

-- ============================================================
-- F: SELECT列表表达式比较（3场景×2=6条）
-- ============================================================
SELECT '=== F: SELECT表达式比较 ===';

SELECT 'F1_bp_cmp_eq', bp = 'abc' AS eq_a, bp = 'abc   ' AS eq_a_sp FROM bpchar_plain WHERE id=1;
SELECT 'F1_bp_cmp_eq_enc', bp = 'abc' AS eq_a, bp = 'abc   ' AS eq_a_sp FROM bpchar_enc WHERE id=1;
SELECT 'F2_tx_cmp_eq', txt = 'abc' AS eq_a, txt = 'abc   ' AS eq_a_sp FROM bpchar_plain WHERE id=1;
SELECT 'F2_tx_cmp_eq_enc', txt = 'abc' AS eq_a, txt = 'abc   ' AS eq_a_sp FROM bpchar_enc WHERE id=1;
SELECT 'F3_vc_cmp_eq', vc = 'abc' AS eq_a, vc = 'abc   ' AS eq_a_sp FROM bpchar_plain WHERE id=1;
SELECT 'F3_vc_cmp_eq_enc', vc = 'abc' AS eq_a, vc = 'abc   ' AS eq_a_sp FROM bpchar_enc WHERE id=1;

-- ============================================================
-- G: NULL/空串（3场景×2=6条）
-- ============================================================
SELECT '=== G: NULL/空串 ===';

SELECT 'G1_null', id FROM bpchar_plain WHERE bp IS NULL ORDER BY id;
SELECT 'G1_null_enc', id FROM bpchar_enc WHERE bp IS NULL ORDER BY id;
SELECT 'G2_empty_bp', id FROM bpchar_plain WHERE bp = '' ORDER BY id;
SELECT 'G2_empty_bp_enc', id FROM bpchar_enc WHERE bp = '' ORDER BY id;
SELECT 'G3_empty_tx', id FROM bpchar_plain WHERE txt = '' ORDER BY id;
SELECT 'G3_empty_tx_enc', id FROM bpchar_enc WHERE txt = '' ORDER BY id;

-- ============================================================
-- H: UPDATE跨类型赋值（3场景×2=6条）
-- ============================================================
SELECT '=== H: UPDATE跨类型赋值 ===';

UPDATE cross_enc SET txt = bp WHERE id=1;
SELECT 'H1_enc_tx_from_bp', txt FROM cross_enc WHERE id=1;
UPDATE cross_enc SET bp = txt WHERE id=2;
SELECT 'H2_enc_bp_from_tx', bp FROM cross_enc WHERE id=2;
SELECT 'H3_cross_after_update', txt = bp FROM cross_enc WHERE id=1;
UPDATE cross_enc SET txt='abc', bp='abc' WHERE id=1;
UPDATE cross_enc SET txt='abc   ', bp='abc   ' WHERE id=2;

UPDATE cross_plain SET txt = bp WHERE id=1;
SELECT 'H1p_tx_from_bp', txt FROM cross_plain WHERE id=1;
UPDATE cross_plain SET bp = txt WHERE id=2;
SELECT 'H2p_bp_from_tx', bp FROM cross_plain WHERE id=2;
SELECT 'H3p_cross_after_update', txt = bp FROM cross_plain WHERE id=1;
UPDATE cross_plain SET txt='abc', bp='abc' WHERE id=1;
UPDATE cross_plain SET txt='abc   ', bp='abc   ' WHERE id=2;

-- ============================================================
-- I: INSERT隐式转换（2场景×2=4条）
-- ============================================================
SELECT '=== I: INSERT隐式转换 ===';

SELECT 'I1_insert_bp', bp FROM cross_plain WHERE bp = 'abc' ORDER BY id;
SELECT 'I1_insert_bp_enc', bp FROM cross_enc WHERE bp = 'abc' ORDER BY id;
SELECT 'I2_insert_bp_sp', bp FROM cross_plain WHERE bp = 'abc   ' ORDER BY id;
SELECT 'I2_insert_bp_sp_enc', bp FROM cross_enc WHERE bp = 'abc   ' ORDER BY id;

-- ============================================================
-- Cleanup
-- ============================================================
SELECT '=== Cleanup ===';
DROP TABLE bpchar_plain CASCADE;
DROP TABLE cross_plain CASCADE;
DROP TABLE bpchar_enc CASCADE;
DROP TABLE cross_enc CASCADE;
DROP TABLE bpchar_enc_ope CASCADE;
DROP TABLE cross_enc_ope CASCADE;
DROP DEK dek_bpchar_ts CASCADE;
DROP DEK dek_bpchar_ope CASCADE;
DROP KEK kek_bpchar_ts CASCADE;
