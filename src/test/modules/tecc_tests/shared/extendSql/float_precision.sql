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

-- @param String test数据
-- @param String test文本
-- @param String test字符
-- @param int 1
-- @param int 100
-- @param int 1000
-- @param BigDecimal 123.45
-- @param BigDecimal 1.23
-- @param BigDecimal 45.678
-- @param String 测试数据
-- @param String 测试文本
-- @param String 测试字符
-- @param int 2
-- @param int 200
-- @param int 2000
-- @param BigDecimal 234.56
-- @param BigDecimal 2.34
-- @param BigDecimal 56.789
-- @param String test测试
-- @param String 测试test
-- @param String test测试
-- @param int 3
-- @param int 300
-- @param int 3000
-- @param BigDecimal 345.67
-- @param BigDecimal 3.45
-- @param BigDecimal 67.890
-- @param String test示例
-- @param String test样例
-- @param String testdemo
-- @param int 10
-- @param int 1000
-- @param int 10000
-- @param BigDecimal 456.78
-- @param BigDecimal 4.56
-- @param BigDecimal 78.901
-- @param String 测试示例
-- @param String 测试样例
-- @param String 测试demo
-- @param int 20
-- @param int 2000
-- @param int 20000
-- @param BigDecimal 567.89
-- @param BigDecimal 5.67
-- @param BigDecimal 89.012
-- @param String 这是一个test长文本数据
-- @param String 包含test的变长字符
-- @param String test
-- @param int 30
-- @param int 3000
-- @param int 30000
-- @param BigDecimal 678.90
-- @param BigDecimal 6.78
-- @param BigDecimal 90.123
-- @param String 这是一个测试长文本数据
-- @param String 包含测试的变长字符
-- @param String 测试
-- @param int 40
-- @param int 4000
-- @param int 40000
-- @param BigDecimal 789.01
-- @param BigDecimal 7.89
-- @param BigDecimal 1.234
-- @param String test test test
-- @param String test测试test
-- @param String testtest
-- @param int 50
-- @param int 5000
-- @param int 50000
-- @param BigDecimal 890.12
-- @param BigDecimal 8.90
-- @param BigDecimal 2.345
-- @param String 测试测试测试
-- @param String 测试测试文本
-- @param String 测试测
-- @param int 60
-- @param int 6000
-- @param int 60000
-- @param BigDecimal 901.23
-- @param BigDecimal 9.01
-- @param BigDecimal 3.456
-- @param String test边界值
-- @param String test最大值
-- @param String testmax
-- @param int 32767
-- @param int 2147483647
-- @param long 9223372036854775807
-- @param BigDecimal 999999.99
-- @param BigDecimal 99999.99
-- @param BigDecimal 999999.999
INSERT INTO multi_cond_plain (text_col, varchar_col, bpchar_col, int2_col, int4_col, int8_col, numeric_col, float4_col, float8_col) VALUES
    (?, ?, ?, ?, ?, ?, ?, ?, ?),
    (?, ?, ?, ?, ?, ?, ?, ?, ?),
    (?, ?, ?, ?, ?, ?, ?, ?, ?),
    (?, ?, ?, ?, ?, ?, ?, ?, ?),
    (?, ?, ?, ?, ?, ?, ?, ?, ?),
    (?, ?, ?, ?, ?, ?, ?, ?, ?),
    (?, ?, ?, ?, ?, ?, ?, ?, ?),
    (?, ?, ?, ?, ?, ?, ?, ?, ?),
    (?, ?, ?, ?, ?, ?, ?, ?, ?),
    (?, ?, ?, ?, ?, ?, ?, ?, ?);

-- @param String test数据
-- @param String test文本
-- @param String test字符
-- @param int 1
-- @param int 100
-- @param int 1000
-- @param BigDecimal 123.45
-- @param BigDecimal 1.23
-- @param BigDecimal 45.678
-- @param String 测试数据
-- @param String 测试文本
-- @param String 测试字符
-- @param int 2
-- @param int 200
-- @param int 2000
-- @param BigDecimal 234.56
-- @param BigDecimal 2.34
-- @param BigDecimal 56.789
-- @param String test测试
-- @param String 测试test
-- @param String test测试
-- @param int 3
-- @param int 300
-- @param int 3000
-- @param BigDecimal 345.67
-- @param BigDecimal 3.45
-- @param BigDecimal 67.890
-- @param String test示例
-- @param String test样例
-- @param String testdemo
-- @param int 10
-- @param int 1000
-- @param int 10000
-- @param BigDecimal 456.78
-- @param BigDecimal 4.56
-- @param BigDecimal 78.901
-- @param String 测试示例
-- @param String 测试样例
-- @param String 测试demo
-- @param int 20
-- @param int 2000
-- @param int 20000
-- @param BigDecimal 567.89
-- @param BigDecimal 5.67
-- @param BigDecimal 89.012
-- @param String 这是一个test长文本数据
-- @param String 包含test的变长字符
-- @param String test
-- @param int 30
-- @param int 3000
-- @param int 30000
-- @param BigDecimal 678.90
-- @param BigDecimal 6.78
-- @param BigDecimal 90.123
-- @param String 这是一个测试长文本数据
-- @param String 包含测试的变长字符
-- @param String 测试
-- @param int 40
-- @param int 4000
-- @param int 40000
-- @param BigDecimal 789.01
-- @param BigDecimal 7.89
-- @param BigDecimal 1.234
-- @param String test test test
-- @param String test测试test
-- @param String testtest
-- @param int 50
-- @param int 5000
-- @param int 50000
-- @param BigDecimal 890.12
-- @param BigDecimal 8.90
-- @param BigDecimal 2.345
-- @param String 测试测试测试
-- @param String 测试测试文本
-- @param String 测试测
-- @param int 60
-- @param int 6000
-- @param int 60000
-- @param BigDecimal 901.23
-- @param BigDecimal 9.01
-- @param BigDecimal 3.456
-- @param String test边界值
-- @param String test最大值
-- @param String testmax
-- @param int 32767
-- @param int 2147483647
-- @param long 9223372036854775807
-- @param BigDecimal 999999.99
-- @param BigDecimal 99999.99
-- @param BigDecimal 999999.999
INSERT INTO multi_cond_enc (text_col, varchar_col, bpchar_col, int2_col, int4_col, int8_col, numeric_col, float4_col, float8_col) VALUES
    (?, ?, ?, ?, ?, ?, ?, ?, ?),
    (?, ?, ?, ?, ?, ?, ?, ?, ?),
    (?, ?, ?, ?, ?, ?, ?, ?, ?),
    (?, ?, ?, ?, ?, ?, ?, ?, ?),
    (?, ?, ?, ?, ?, ?, ?, ?, ?),
    (?, ?, ?, ?, ?, ?, ?, ?, ?),
    (?, ?, ?, ?, ?, ?, ?, ?, ?),
    (?, ?, ?, ?, ?, ?, ?, ?, ?),
    (?, ?, ?, ?, ?, ?, ?, ?, ?),
    (?, ?, ?, ?, ?, ?, ?, ?, ?);

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

-- @param String test数据
-- @param int 100
-- @param int 500
SELECT id, text_col, int4_col, varchar_col, numeric_col FROM multi_cond_plain WHERE text_col = ? AND int4_col >= ? AND int4_col <= ? ORDER BY id;

-- @param String test数据
-- @param int 100
-- @param int 500
SELECT id, text_col, int4_col, varchar_col, numeric_col FROM multi_cond_enc WHERE text_col = ? AND int4_col >= ? AND int4_col <= ? ORDER BY id;

-- Test 2: INT8范围 + VARCHAR模糊
SELECT '3b_int8_range_varchar_like';

-- @param int 2000
-- @param int 40000
-- @param String %测试%
SELECT id, varchar_col, int8_col, text_col, float8_col FROM multi_cond_plain WHERE int8_col >= ? AND int8_col <= ? AND varchar_col LIKE ? ORDER BY id;

-- @param int 2000
-- @param int 40000
-- @param String %测试%
SELECT id, varchar_col, int8_col, text_col, float8_col FROM multi_cond_enc WHERE int8_col >= ? AND int8_col <= ? AND varchar_col LIKE ? ORDER BY id;

-- Test 3: BPCHAR模糊 + INT2等值
SELECT '3c_bpchar_like_int2_eq';

-- @param String %test%
-- @param int 3
SELECT id, bpchar_col, int2_col, varchar_col, float4_col FROM multi_cond_plain WHERE bpchar_col LIKE ? AND int2_col = ? ORDER BY id;

-- @param String %test%
-- @param int 3
SELECT id, bpchar_col, int2_col, varchar_col, float4_col FROM multi_cond_enc WHERE bpchar_col LIKE ? AND int2_col = ? ORDER BY id;

-- Test 4: INT2等值 + NUMERIC范围 + FLOAT4等值
-- 设计差异: float4_col = 1.23 因float提升精度，明文0行；密态归一化一致返回1行
SELECT '3d_float4_eq_precision_diff';

-- @param int 1
-- @param int 100
-- @param int 500
-- @param BigDecimal 1.23
SELECT id, int2_col, numeric_col, float4_col, text_col FROM multi_cond_plain WHERE int2_col = ? AND numeric_col >= ? AND numeric_col <= ? AND float4_col = ? ORDER BY id;

-- @param int 1
-- @param int 100
-- @param int 500
-- @param BigDecimal 1.23
SELECT id, int2_col, numeric_col, float4_col, text_col FROM multi_cond_enc WHERE int2_col = ? AND numeric_col >= ? AND numeric_col <= ? AND float4_col = ? ORDER BY id;

-- Test 5: INT4等值 + FLOAT8范围
SELECT '3e_int4_eq_float8_range';

-- @param int 200
-- @param int 50
-- @param int 100
SELECT id, int4_col, float8_col, varchar_col, bpchar_col FROM multi_cond_plain WHERE int4_col = ? AND float8_col >= ? AND float8_col <= ? ORDER BY id;

-- @param int 200
-- @param int 50
-- @param int 100
SELECT id, int4_col, float8_col, varchar_col, bpchar_col FROM multi_cond_enc WHERE int4_col = ? AND float8_col >= ? AND float8_col <= ? ORDER BY id;

-- Test 6: INT8范围 + TEXT模糊
SELECT '3f_int8_range_text_like';

-- @param int 10000
-- @param int 50000
-- @param String %test%
SELECT id, int8_col, text_col, numeric_col, float4_col FROM multi_cond_plain WHERE int8_col >= ? AND int8_col <= ? AND text_col LIKE ? ORDER BY id;

-- @param int 10000
-- @param int 50000
-- @param String %test%
SELECT id, int8_col, text_col, numeric_col, float4_col FROM multi_cond_enc WHERE int8_col >= ? AND int8_col <= ? AND text_col LIKE ? ORDER BY id;

-- Test 7: NUMERIC等值 + VARCHAR模糊
SELECT '3g_numeric_eq_varchar_like';

-- @param BigDecimal 345.67
-- @param String %测试%
SELECT id, numeric_col, varchar_col, int2_col, float8_col FROM multi_cond_plain WHERE numeric_col = ? AND varchar_col LIKE ? ORDER BY id;

-- @param BigDecimal 345.67
-- @param String %测试%
SELECT id, numeric_col, varchar_col, int2_col, float8_col FROM multi_cond_enc WHERE numeric_col = ? AND varchar_col LIKE ? ORDER BY id;

-- Test 8: FLOAT4等值 + INT8范围 + INT2等值
-- 设计差异: float4_col = 3.45 因float提升精度，明文0行；密态归一化一致返回1行
SELECT '3h_float4_eq_precision_diff';

-- @param BigDecimal 3.45
-- @param int 3000
-- @param int 5000
-- @param int 3
SELECT id, float4_col, int8_col, int2_col, text_col FROM multi_cond_plain WHERE float4_col = ? AND int8_col >= ? AND int8_col <= ? AND int2_col = ? ORDER BY id;

-- @param BigDecimal 3.45
-- @param int 3000
-- @param int 5000
-- @param int 3
SELECT id, float4_col, int8_col, int2_col, text_col FROM multi_cond_enc WHERE float4_col = ? AND int8_col >= ? AND int8_col <= ? AND int2_col = ? ORDER BY id;

-- Test 9: FLOAT8范围 + BPCHAR模糊
SELECT '3i_float8_range_bpchar_like';

-- @param int 60
-- @param int 100
-- @param String %测试%
SELECT id, float8_col, bpchar_col, int4_col, numeric_col FROM multi_cond_plain WHERE float8_col >= ? AND float8_col <= ? AND bpchar_col LIKE ? ORDER BY id;

-- @param int 60
-- @param int 100
-- @param String %测试%
SELECT id, float8_col, bpchar_col, int4_col, numeric_col FROM multi_cond_enc WHERE float8_col >= ? AND float8_col <= ? AND bpchar_col LIKE ? ORDER BY id;

-- Test 10: 综合查询（含float4等值）
-- 设计差异: float4_col = 1.23 因float提升精度，明文0行；密态归一化一致返回1行
SELECT '3j_combo_float4_precision_diff';

-- @param String %test%
-- @param String test文本
-- @param String %test%
-- @param int 1
-- @param int 10
-- @param int 100
-- @param int 1000
-- @param int 10000
-- @param int 100
-- @param int 500
-- @param BigDecimal 1.23
-- @param int 40
-- @param int 80
SELECT id, text_col, varchar_col, bpchar_col, int2_col, int4_col, int8_col, numeric_col, float4_col, float8_col FROM multi_cond_plain WHERE text_col LIKE ? AND varchar_col = ? AND bpchar_col LIKE ? AND int2_col >= ? AND int2_col <= ? AND int4_col = ? AND int8_col >= ? AND int8_col <= ? AND numeric_col >= ? AND numeric_col <= ? AND float4_col = ? AND float8_col >= ? AND float8_col <= ? ORDER BY id;

-- @param String %test%
-- @param String test文本
-- @param String %test%
-- @param int 1
-- @param int 10
-- @param int 100
-- @param int 1000
-- @param int 10000
-- @param int 100
-- @param int 500
-- @param BigDecimal 1.23
-- @param int 40
-- @param int 80
SELECT id, text_col, varchar_col, bpchar_col, int2_col, int4_col, int8_col, numeric_col, float4_col, float8_col FROM multi_cond_enc WHERE text_col LIKE ? AND varchar_col = ? AND bpchar_col LIKE ? AND int2_col >= ? AND int2_col <= ? AND int4_col = ? AND int8_col >= ? AND int8_col <= ? AND numeric_col >= ? AND numeric_col <= ? AND float4_col = ? AND float8_col >= ? AND float8_col <= ? ORDER BY id;

-- ============================================================
-- 4. Cleanup
-- ============================================================
SELECT '=== 4 Cleanup ===';

DROP TABLE IF EXISTS multi_cond_enc CASCADE;

DROP TABLE IF EXISTS multi_cond_plain CASCADE;

DROP DEK IF EXISTS dek_multi_cond;

DROP KEK IF EXISTS kek_multi_cond;
