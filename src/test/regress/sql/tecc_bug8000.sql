-- Bug 8000: 多条件组合明文参数查询结果与明文表不一致
CREATE KEK bug8000_kek WITH (
    KEY_STORE = 'localkms',
    KEY_PATH = '/tmp/bug8000_kek'
);

CREATE DEK bug8000_dek WITH VALUES (
    KEK = 'bug8000_kek'
);

-- Create plaintext reference table
CREATE TABLE bug8000_plain (
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

-- Create encrypted table
CREATE TABLE bug8000_enc (
    id SERIAL PRIMARY KEY,
    text_col text_enc encrypted with (dek = bug8000_dek, support_algo = 'ope,search'),
    varchar_col varchar_enc encrypted with (dek = bug8000_dek, support_algo = 'ope,search'),
    bpchar_col bpchar_enc encrypted with (dek = bug8000_dek, support_algo = 'ope,search'),
    int2_col int2_enc encrypted with (dek = bug8000_dek, support_algo = 'ope'),
    int4_col int4_enc encrypted with (dek = bug8000_dek, support_algo = 'ope'),
    int8_col int8_enc encrypted with (dek = bug8000_dek, support_algo = 'ope'),
    numeric_col numeric_enc encrypted with (dek = bug8000_dek, support_algo = 'ope'),
    float4_col float4_enc encrypted with (dek = bug8000_dek, support_algo = 'ope'),
    float8_col float8_enc encrypted with (dek = bug8000_dek, support_algo = 'ope')
);

-- Insert 10 rows of test data
INSERT INTO bug8000_plain (text_col, varchar_col, bpchar_col, int2_col, int4_col, int8_col, numeric_col, float4_col, float8_col) VALUES
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

INSERT INTO bug8000_enc (text_col, varchar_col, bpchar_col, int2_col, int4_col, int8_col, numeric_col, float4_col, float8_col) VALUES
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

-- Test 1: TEXT等值 + INT4范围
SELECT 'Test 1: TEXT等值 + INT4范围' AS test_name;
SELECT id, text_col, int4_col, varchar_col, numeric_col FROM bug8000_plain WHERE text_col = 'test数据' AND int4_col >= 100 AND int4_col <= 500 ORDER BY id;
SELECT id, text_col, int4_col, varchar_col, numeric_col FROM bug8000_enc WHERE text_col = 'test数据' AND int4_col >= 100 AND int4_col <= 500 ORDER BY id;

-- Test 2: INT8范围 + VARCHAR模糊
SELECT 'Test 2: INT8范围 + VARCHAR模糊' AS test_name;
SELECT id, varchar_col, int8_col, text_col, float8_col FROM bug8000_plain WHERE int8_col >= 2000 AND int8_col <= 40000 AND varchar_col LIKE '%测试%' ORDER BY id;
SELECT id, varchar_col, int8_col, text_col, float8_col FROM bug8000_enc WHERE int8_col >= 2000 AND int8_col <= 40000 AND varchar_col LIKE '%测试%' ORDER BY id;

-- Test 3: BPCHAR模糊 + INT2等值
SELECT 'Test 3: BPCHAR模糊 + INT2等值' AS test_name;
SELECT id, bpchar_col, int2_col, varchar_col, float4_col FROM bug8000_plain WHERE bpchar_col LIKE '%test%' AND int2_col = 3 ORDER BY id;
SELECT id, bpchar_col, int2_col, varchar_col, float4_col FROM bug8000_enc WHERE bpchar_col LIKE '%test%' AND int2_col = 3 ORDER BY id;

-- Test 4: INT2等值 + NUMERIC范围 + FLOAT4等值
SELECT 'Test 4: INT2等值 + NUMERIC范围 + FLOAT4等值' AS test_name;
SELECT id, int2_col, numeric_col, float4_col, text_col FROM bug8000_plain WHERE int2_col = 1 AND numeric_col >= 100 AND numeric_col <= 500 AND float4_col = 1.23 ORDER BY id;
SELECT id, int2_col, numeric_col, float4_col, text_col FROM bug8000_enc WHERE int2_col = 1 AND numeric_col >= 100 AND numeric_col <= 500 AND float4_col = 1.23 ORDER BY id;

-- Test 5: INT4等值 + FLOAT8范围
SELECT 'Test 5: INT4等值 + FLOAT8范围' AS test_name;
SELECT id, int4_col, float8_col, varchar_col, bpchar_col FROM bug8000_plain WHERE int4_col = 200 AND float8_col >= 50 AND float8_col <= 100 ORDER BY id;
SELECT id, int4_col, float8_col, varchar_col, bpchar_col FROM bug8000_enc WHERE int4_col = 200 AND float8_col >= 50 AND float8_col <= 100 ORDER BY id;

-- Test 6: INT8范围 + TEXT模糊
SELECT 'Test 6: INT8范围 + TEXT模糊' AS test_name;
SELECT id, int8_col, text_col, numeric_col, float4_col FROM bug8000_plain WHERE int8_col >= 10000 AND int8_col <= 50000 AND text_col LIKE '%test%' ORDER BY id;
SELECT id, int8_col, text_col, numeric_col, float4_col FROM bug8000_enc WHERE int8_col >= 10000 AND int8_col <= 50000 AND text_col LIKE '%test%' ORDER BY id;

-- Test 7: NUMERIC等值 + VARCHAR模糊
SELECT 'Test 7: NUMERIC等值 + VARCHAR模糊' AS test_name;
SELECT id, numeric_col, varchar_col, int2_col, float8_col FROM bug8000_plain WHERE numeric_col = 345.67 AND varchar_col LIKE '%测试%' ORDER BY id;
SELECT id, numeric_col, varchar_col, int2_col, float8_col FROM bug8000_enc WHERE numeric_col = 345.67 AND varchar_col LIKE '%测试%' ORDER BY id;

-- Test 8: FLOAT4等值 + INT8范围 + INT2等值
SELECT 'Test 8: FLOAT4等值 + INT8范围 + INT2等值' AS test_name;
SELECT id, float4_col, int8_col, int2_col, text_col FROM bug8000_plain WHERE float4_col = 3.45 AND int8_col >= 3000 AND int8_col <= 5000 AND int2_col = 3 ORDER BY id;
SELECT id, float4_col, int8_col, int2_col, text_col FROM bug8000_enc WHERE float4_col = 3.45 AND int8_col >= 3000 AND int8_col <= 5000 AND int2_col = 3 ORDER BY id;

-- Test 9: FLOAT8范围 + BPCHAR模糊
SELECT 'Test 9: FLOAT8范围 + BPCHAR模糊' AS test_name;
SELECT id, float8_col, bpchar_col, int4_col, numeric_col FROM bug8000_plain WHERE float8_col >= 60 AND float8_col <= 100 AND bpchar_col LIKE '%测试%' ORDER BY id;
SELECT id, float8_col, bpchar_col, int4_col, numeric_col FROM bug8000_enc WHERE float8_col >= 60 AND float8_col <= 100 AND bpchar_col LIKE '%测试%' ORDER BY id;

-- Test 10: 综合查询
SELECT 'Test 10: 综合查询' AS test_name;
SELECT id, text_col, varchar_col, bpchar_col, int2_col, int4_col, int8_col, numeric_col, float4_col, float8_col FROM bug8000_plain WHERE text_col LIKE '%test%' AND varchar_col = 'test文本' AND bpchar_col LIKE '%test%' AND int2_col >= 1 AND int2_col <= 10 AND int4_col = 100 AND int8_col >= 1000 AND int8_col <= 10000 AND numeric_col >= 100 AND numeric_col <= 500 AND float4_col = 1.23 AND float8_col >= 40 AND float8_col <= 80 ORDER BY id;
SELECT id, text_col, varchar_col, bpchar_col, int2_col, int4_col, int8_col, numeric_col, float4_col, float8_col FROM bug8000_enc WHERE text_col LIKE '%test%' AND varchar_col = 'test文本' AND bpchar_col LIKE '%test%' AND int2_col >= 1 AND int2_col <= 10 AND int4_col = 100 AND int8_col >= 1000 AND int8_col <= 10000 AND numeric_col >= 100 AND numeric_col <= 500 AND float4_col = 1.23 AND float8_col >= 40 AND float8_col <= 80 ORDER BY id;

-- Cleanup
DROP TABLE bug8000_enc CASCADE;
DROP TABLE bug8000_plain CASCADE;
DROP DEK bug8000_dek;
DROP KEK bug8000_kek;
