-- Bug 8022: Full reproduction — bpchar_ope LIKE '%test%' session crash
-- Original test case from 未关闭Bug_0526_all.txt

DROP KEK IF EXISTS test_kek CASCADE;
CREATE KEK test_kek WITH (KEY_STORE = localkms, KEY_PATH = '/tmp/test_kek_bug.key');
CREATE DEK test_dek WITH VALUES(KEK = test_kek);

DROP TABLE IF EXISTS enc CASCADE;
CREATE TABLE enc (
    id serial PRIMARY KEY,
    c_text_det text_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'det,search'),
    c_text_ope text_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'ope,search'),
    c_varchar_det varchar_enc(100) ENCRYPTED WITH (dek = test_dek, support_algo = 'det,search'),
    c_varchar_ope varchar_enc(100) ENCRYPTED WITH (dek = test_dek, support_algo = 'ope,search'),
    c_bpchar_det bpchar_enc(20) ENCRYPTED WITH (dek = test_dek, support_algo = 'det,search'),
    c_bpchar_ope bpchar_enc(20) ENCRYPTED WITH (dek = test_dek, support_algo = 'ope,search'),
    c_int2_det int2_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'det'),
    c_int2_ope int2_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'ope'),
    c_int4_det int4_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'det'),
    c_int4_ope int4_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'ope'),
    c_int8_det int8_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'det'),
    c_int8_ope int8_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'ope'),
    c_float4_det float4_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'det'),
    c_float4_ope float4_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'ope'),
    c_float8_det float8_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'det'),
    c_float8_ope float8_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'ope'),
    c_numeric_det numeric_enc(10,2) ENCRYPTED WITH (dek = test_dek, support_algo = 'det'),
    c_numeric_ope numeric_enc(10,2) ENCRYPTED WITH (dek = test_dek, support_algo = 'ope')
);

-- Original insert data (14 rows, non-null values — skip NULL row)
INSERT INTO enc (c_text_det, c_text_ope, c_varchar_det, c_varchar_ope, c_bpchar_det, c_bpchar_ope,
    c_int2_det, c_int2_ope, c_int4_det, c_int4_ope, c_int8_det, c_int8_ope,
    c_float4_det, c_float4_ope, c_float8_det, c_float8_ope, c_numeric_det, c_numeric_ope) VALUES
    ('张三', '张三', '李四', '李四', '王五', '王五',
     100, 100, 20000, 20000, 3000000, 3000000,
     3.14, 3.14, 2.718281828, 2.718281828, 99999.99, 99999.99),
    ('Alice', 'Alice', 'Bob', 'Bob', 'Charlie', 'Charlie',
     1, 1, 1000, 1000, 50000, 50000,
     1.5, 1.5, 100.5, 100.5, 123.45, 123.45),
    ('赵六', '赵六', '孙七', '孙七', '周八', '周八',
     32767, 32767, 2147483647, 2147483647, 9223372036854775807, 9223372036854775807,
     1.0e10, 1.0e10, 1.7976931348623e308, 1.7976931348623e308, 0.01, 0.01),
    ('张三丰', '张三丰', 'David', 'David', 'Eve', 'Eve',
     -100, -100, -50000, -50000, -999999, -999999,
     -2.5, -2.5, -0.001, -0.001, -500.00, -500.00),
    ('hello', 'hello', 'world', 'world', 'test', 'test',
     0, 0, 0, 0, 0, 0,
     0.0, 0.0, 0.0, 0.0, 0.00, 0.00),
    ('alice', 'alice', 'ALICE', 'ALICE', 'alice', 'alice',
     1, 1, 1, 1, 1, 1,
     1.0, 1.0, 1.0, 1.0, 1.00, 1.00),
    ('张三丰', '张三丰', 'David2', 'David2', 'Eve2', 'Eve2',
     100, 100, 20000, 20000, 3000000, 3000000,
     3.14, 3.14, 2.718281828, 2.718281828, 99999.99, 99999.99),
    ('Hello', 'Hello', 'World', 'World', 'TEST', 'TEST',
     50, 50, 55555, 55555, 7777777, 7777777,
     50.5, 50.5, 55.55, 55.55, 500.00, 500.00),
    ('abc_def', 'abc_def', 'xyz_123', 'xyz_123', 'foo_bar', 'foo_bar',
     -1, -1, -1, -1, -1, -1,
     -1.0, -1.0, -1.0, -1.0, -1.00, -1.00),
    ('test%like', 'test%like', 'test_like', 'test_like', 'pct_sign', 'pct_sign',
     10, 10, 100, 100, 1000, 1000,
     10.10, 10.10, 100.10, 100.10, 10.10, 10.10),
    ('%percent', '%percent', 'under_score', 'under_score', '_start', '_start',
     20, 20, 200, 200, 2000, 2000,
     20.20, 20.20, 200.20, 200.20, 20.20, 20.20),
    ('', '', '', '', '', '',
     -32768, -32768, -2147483648, -2147483648, -9223372036854775808, -9223372036854775808,
     -3.402823e38, -3.402823e38, -1.7976931348e308, -1.7976931348e308, -99999.99, -99999.99),
    ('测试', '测试', '测试', '测试', '测试', '测试',
     42, 42, 4242, 4242, 424242, 424242,
     4.2, 4.2, 42.42, 42.42, 42.42, 42.42),
    ('test123', 'test123', 'abcXYZ', 'abcXYZ', 'mixCase', 'mixCase',
     255, 255, 65535, 65535, 16777215, 16777215,
     255.255, 255.255, 65535.65535, 65535.65535, 255.55, 255.55);

-- Bug 8022 original trigger query
SELECT 'bug8022_trigger', id, c_bpchar_ope FROM enc WHERE c_bpchar_ope LIKE '%test%';

-- ESCAPE fix verification (grammar.sql line 873 equivalent)
SELECT 'escape_test', id FROM enc WHERE c_bpchar_ope LIKE '%test%' ESCAPE '\';

SELECT 'final', 1;
