-- ============================================================
-- multi_cond_float_precision.sql — 多条件组合查询浮点精度差异测试
--
-- 背景：
--   PostgreSQL原生float4/float8因浮点尾数精度不同，float4→float8
--   提升后比较可能不相等（如 float4(1.23)≠float8(1.23)），导致
--   某些等值条件在明文表返回0行。
--
--   密态表因TECC通过字符串归一化（strtod + Ryu最短算法）处理后，
--   INSERT和WHERE两端归一化结果一致，产生相同的OPE密文，等值匹配
--   返回1行。
--
--   这是浮点类型本身的精度特性导致的差异，非TECC bug。
--   用例保留明文/密文对比以记录和确认此设计差异。
-- ============================================================

-- ============================================================
-- 0. Setup — 共享KEK/DEK
-- ============================================================
DROP KEK IF EXISTS kek_multi_cond CASCADE;

CREATE KEK kek_multi_cond WITH (
    KEY_STORE = 'localkms',
    KEY_PATH = '/tmp/kek_multi_cond.key'
);
CREATE DEK dek_multi_cond WITH VALUES(KEK = kek_multi_cond);

-- ============================================================
-- 1. 建表：明文参考表 + 密态表（9种加密类型，支持OPE+SEARCH）
-- ============================================================
SELECT '=== 1 Setup Tables ===';

DROP TABLE IF EXISTS multi_cond_plain CASCADE;
CREATE TABLE multi_cond_plain (
    id SERIAL PRIMARY KEY,
    text_col TEXT,
    varchar_col VARCHAR(100),
    bpchar_col BPCHAR(100),
    int2_col int2,
    int4_col int4,
    int8_col int8,
    numeric_col NUMERIC,
    float4_col float4,
    float8_col float8
);

DROP TABLE IF EXISTS multi_cond_enc CASCADE;
CREATE TABLE multi_cond_enc (
    id SERIAL PRIMARY KEY,
    text_col text_enc encrypted with (dek = dek_multi_cond, support_algo = 'ope,search'),
    varchar_col varchar_enc encrypted with (dek = dek_multi_cond, support_algo = 'ope,search'),
    bpchar_col bpchar_enc encrypted with (dek = dek_multi_cond, support_algo = 'ope,search'),
    int2_col int2_enc encrypted with (dek = dek_multi_cond, support_algo = 'ope'),
    int4_col int4_enc encrypted with (dek = dek_multi_cond, support_algo = 'ope'),
    int8_col int8_enc encrypted with (dek = dek_multi_cond, support_algo = 'ope'),
    numeric_col numeric_enc encrypted with (dek = dek_multi_cond, support_algo = 'ope'),
    float4_col float4_enc encrypted with (dek = dek_multi_cond, support_algo = 'ope'),
    float8_col float8_enc encrypted with (dek = dek_multi_cond, support_algo = 'ope')
);

-- ============================================================
-- 2. 插入10行测试数据
-- ============================================================
SELECT '=== 2 Insert Data ===';

INSERT INTO multi_cond_plain (text_col, varchar_col, bpchar_col, int2_col, int4_col, int8_col, numeric_col, float4_col, float8_col) VALUES
    ('test数据', 'test文本', 'test字符', 1, 100, 1000, 123.45, 1.23, 45.678),
    ('测试数据', '测试文本', '测试字符', 2, 200, 2000, 234.56, 2.34, 56.789),
    ('test测试', '测试test', 'test测试', 3, 300, 3000, 345.67, 3.45, 67.890),
    ('test示例', 'test样例', 'testdemo', 10, 1000, 10000, 456.78, 4.56, 78.901),
    ('测试示例', '测试样例', '测试demo', 20, 2000, 20000, 567.89, 5.67, 89.012),
    ('这是一个test长文本数据', '包含test的变长字符', 'test', 30, 3000, 30000, 678.90, 6.78, 90.123),
    ('这是一个测试长文本数据', '包含测试的变长字符', '测试', 40, 4000, 40000, 789.01, 7.89, 1.234),
    ('test test test', 'test测试test', 'testtest', 50, 5000, 50000, 890.12, 8.90, 2.345),
    ('测试测试测试', '测试测试文本', '测试测', 60, 6000, 60000, 901.23, 9.01, 3.456),
    ('test边界值', 'test最大值', 'testmax', 32767, 2147483647, 9223372036854775807, 999999.99, 99999.99, 999999.999);

INSERT INTO multi_cond_enc (text_col, varchar_col, bpchar_col, int2_col, int4_col, int8_col, numeric_col, float4_col, float8_col) VALUES
    ('test数据', 'test文本', 'test字符', 1, 100, 1000, 123.45, 1.23, 45.678),
    ('测试数据', '测试文本', '测试字符', 2, 200, 2000, 234.56, 2.34, 56.789),
    ('test测试', '测试test', 'test测试', 3, 300, 3000, 345.67, 3.45, 67.890),
    ('test示例', 'test样例', 'testdemo', 10, 1000, 10000, 456.78, 4.56, 78.901),
    ('测试示例', '测试样例', '测试demo', 20, 2000, 20000, 567.89, 5.67, 89.012),
    ('这是一个test长文本数据', '包含test的变长字符', 'test', 30, 3000, 30000, 678.90, 6.78, 90.123),
    ('这是一个测试长文本数据', '包含测试的变长字符', '测试', 40, 4000, 40000, 789.01, 7.89, 1.234),
    ('test test test', 'test测试test', 'testtest', 50, 5000, 50000, 890.12, 8.90, 2.345),
    ('测试测试测试', '测试测试文本', '测试测', 60, 6000, 60000, 901.23, 9.01, 3.456),
    ('test边界值', 'test最大值', 'testmax', 32767, 2147483647, 9223372036854775807, 999999.99, 99999.99, 999999.999);

-- ============================================================
-- 3. 多条件组合查询
-- 每个Test输出明文和密态结果对比。
-- 注：float4等值条件中，float4(1.23)提升为float8后与原生
-- float8(1.23)尾数不同，明文可能返回0行，密态因归一化一致
-- 仍返回行。这是float精度设计差异，非TECC bug。
-- ============================================================
SELECT '=== 3 Multi-Condition Queries ===';

-- Test 1: TEXT等值 + INT4范围
SELECT '3a_text_eq_int4_range';
SELECT id, text_col, int4_col, varchar_col, numeric_col FROM multi_cond_plain WHERE text_col = 'test数据' AND int4_col >= 100 AND int4_col <= 500 ORDER BY id;
SELECT id, text_col, int4_col, varchar_col, numeric_col FROM multi_cond_enc WHERE text_col = 'test数据' AND int4_col >= 100 AND int4_col <= 500 ORDER BY id;

-- Test 2: INT8范围 + VARCHAR模糊
SELECT '3b_int8_range_varchar_like';
SELECT id, varchar_col, int8_col, text_col, float8_col FROM multi_cond_plain WHERE int8_col >= 2000 AND int8_col <= 40000 AND varchar_col LIKE '%测试%' ORDER BY id;
SELECT id, varchar_col, int8_col, text_col, float8_col FROM multi_cond_enc WHERE int8_col >= 2000 AND int8_col <= 40000 AND varchar_col LIKE '%测试%' ORDER BY id;

-- Test 3: BPCHAR模糊 + INT2等值
SELECT '3c_bpchar_like_int2_eq';
SELECT id, bpchar_col, int2_col, varchar_col, float4_col FROM multi_cond_plain WHERE bpchar_col LIKE '%test%' AND int2_col = 3 ORDER BY id;
SELECT id, bpchar_col, int2_col, varchar_col, float4_col FROM multi_cond_enc WHERE bpchar_col LIKE '%test%' AND int2_col = 3 ORDER BY id;

-- Test 4: INT2等值 + NUMERIC范围 + FLOAT4等值
-- 设计差异: float4_col = 1.23 因float提升精度，明文0行；密态归一化一致返回1行
SELECT '3d_float4_eq_precision_diff';
SELECT id, int2_col, numeric_col, float4_col, text_col FROM multi_cond_plain WHERE int2_col = 1 AND numeric_col >= 100 AND numeric_col <= 500 AND float4_col = 1.23 ORDER BY id;
SELECT id, int2_col, numeric_col, float4_col, text_col FROM multi_cond_enc WHERE int2_col = 1 AND numeric_col >= 100 AND numeric_col <= 500 AND float4_col = 1.23 ORDER BY id;

-- Test 5: INT4等值 + FLOAT8范围
SELECT '3e_int4_eq_float8_range';
SELECT id, int4_col, float8_col, varchar_col, bpchar_col FROM multi_cond_plain WHERE int4_col = 200 AND float8_col >= 50 AND float8_col <= 100 ORDER BY id;
SELECT id, int4_col, float8_col, varchar_col, bpchar_col FROM multi_cond_enc WHERE int4_col = 200 AND float8_col >= 50 AND float8_col <= 100 ORDER BY id;

-- Test 6: INT8范围 + TEXT模糊
SELECT '3f_int8_range_text_like';
SELECT id, int8_col, text_col, numeric_col, float4_col FROM multi_cond_plain WHERE int8_col >= 10000 AND int8_col <= 50000 AND text_col LIKE '%test%' ORDER BY id;
SELECT id, int8_col, text_col, numeric_col, float4_col FROM multi_cond_enc WHERE int8_col >= 10000 AND int8_col <= 50000 AND text_col LIKE '%test%' ORDER BY id;

-- Test 7: NUMERIC等值 + VARCHAR模糊
SELECT '3g_numeric_eq_varchar_like';
SELECT id, numeric_col, varchar_col, int2_col, float8_col FROM multi_cond_plain WHERE numeric_col = 345.67 AND varchar_col LIKE '%测试%' ORDER BY id;
SELECT id, numeric_col, varchar_col, int2_col, float8_col FROM multi_cond_enc WHERE numeric_col = 345.67 AND varchar_col LIKE '%测试%' ORDER BY id;

-- Test 8: FLOAT4等值 + INT8范围 + INT2等值
-- 设计差异: float4_col = 3.45 因float提升精度，明文0行；密态归一化一致返回1行
SELECT '3h_float4_eq_precision_diff';
SELECT id, float4_col, int8_col, int2_col, text_col FROM multi_cond_plain WHERE float4_col = 3.45 AND int8_col >= 3000 AND int8_col <= 5000 AND int2_col = 3 ORDER BY id;
SELECT id, float4_col, int8_col, int2_col, text_col FROM multi_cond_enc WHERE float4_col = 3.45 AND int8_col >= 3000 AND int8_col <= 5000 AND int2_col = 3 ORDER BY id;

-- Test 9: FLOAT8范围 + BPCHAR模糊
SELECT '3i_float8_range_bpchar_like';
SELECT id, float8_col, bpchar_col, int4_col, numeric_col FROM multi_cond_plain WHERE float8_col >= 60 AND float8_col <= 100 AND bpchar_col LIKE '%测试%' ORDER BY id;
SELECT id, float8_col, bpchar_col, int4_col, numeric_col FROM multi_cond_enc WHERE float8_col >= 60 AND float8_col <= 100 AND bpchar_col LIKE '%测试%' ORDER BY id;

-- Test 10: 综合查询（含float4等值）
-- 设计差异: float4_col = 1.23 因float提升精度，明文0行；密态归一化一致返回1行
SELECT '3j_combo_float4_precision_diff';
SELECT id, text_col, varchar_col, bpchar_col, int2_col, int4_col, int8_col, numeric_col, float4_col, float8_col FROM multi_cond_plain WHERE text_col LIKE '%test%' AND varchar_col = 'test文本' AND bpchar_col LIKE '%test%' AND int2_col >= 1 AND int2_col <= 10 AND int4_col = 100 AND int8_col >= 1000 AND int8_col <= 10000 AND numeric_col >= 100 AND numeric_col <= 500 AND float4_col = 1.23 AND float8_col >= 40 AND float8_col <= 80 ORDER BY id;
SELECT id, text_col, varchar_col, bpchar_col, int2_col, int4_col, int8_col, numeric_col, float4_col, float8_col FROM multi_cond_enc WHERE text_col LIKE '%test%' AND varchar_col = 'test文本' AND bpchar_col LIKE '%test%' AND int2_col >= 1 AND int2_col <= 10 AND int4_col = 100 AND int8_col >= 1000 AND int8_col <= 10000 AND numeric_col >= 100 AND numeric_col <= 500 AND float4_col = 1.23 AND float8_col >= 40 AND float8_col <= 80 ORDER BY id;

-- ============================================================
-- 4. Cleanup
-- ============================================================
SELECT '=== 4 Cleanup ===';
DROP TABLE IF EXISTS multi_cond_enc CASCADE;
DROP TABLE IF EXISTS multi_cond_plain CASCADE;
DROP DEK IF EXISTS dek_multi_cond;
DROP KEK IF EXISTS kek_multi_cond;
