-- ============================================================
-- Bug 8026 完整复测：密文enc(18列,15行含中文) vs 明文plain
-- 字符型OPE范围查询/等值查询/ORDER BY 全部对比
-- ============================================================
DROP TABLE IF EXISTS enc CASCADE;
DROP TABLE IF EXISTS plain CASCADE;
DROP DEK IF EXISTS test_dek CASCADE;
DROP KEK IF EXISTS test_kek CASCADE;

CREATE KEK test_kek WITH (KEY_STORE = localkms, KEY_PATH = '/tmp/test_kek.key');
CREATE DEK test_dek WITH VALUES(KEK = test_kek);

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
CREATE TABLE plain (
    id serial PRIMARY KEY,
    c_text text,
    c_varchar varchar(100),
    c_bpchar bpchar(20),
    c_int2 int2,
    c_int4 int4,
    c_int8 int8,
    c_float4 float4,
    c_float8 float8,
    c_numeric numeric(10,2)
);

INSERT INTO plain (c_text, c_varchar, c_bpchar, c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric) VALUES
('张三', '李四', '王五', 100, 20000, 3000000, 3.14, 2.718281828, 99999.99),
('Alice', 'Bob', 'Charlie', 1, 1000, 50000, 1.5, 100.5, 123.45),
('赵六', '孙七', '周八', 32767, 2147483647, 9223372036854775807, 1.0e10, 1.7976931348623e308, 0.01),
('张三丰', 'David', 'Eve', -100, -50000, -999999, -2.5, -0.001, -500.00),
('hello', 'world', 'test', 0, 0, 0, 0.0, 0.0, 0.00),
('alice', 'ALICE', 'alice', 1, 1, 1, 1.0, 1.0, 1.00),
('张三丰', 'David2', 'Eve2', 100, 20000, 3000000, 3.14, 2.718281828, 99999.99),
('Hello', 'World', 'TEST', 50, 55555, 7777777, 50.5, 55.55, 500.00),
('abc_def', 'xyz_123', 'foo_bar', -1, -1, -1, -1.0, -1.0, -1.00),
('test%%like', 'test_like','pct_sign', 10, 100, 1000, 10.10, 100.10, 10.10),
('%%percent', 'under_score','_start', 20, 200, 2000, 20.20, 200.20, 20.20),
(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL),
('', '', '', -32768, -2147483648, -9223372036854775808, -3.402823e38, -1.7976931348e308, -99999.99),
('测试', '测试', '测试', 42, 4242, 424242, 4.2, 42.42, 42.42),
('test123', 'abcXYZ', 'mixCase', 255, 65535, 16777215, 255.255, 65535.65535, 255.55);

INSERT INTO enc (c_text_det, c_text_ope, c_varchar_det, c_varchar_ope, c_bpchar_det, c_bpchar_ope,
c_int2_det, c_int2_ope, c_int4_det, c_int4_ope, c_int8_det, c_int8_ope,
c_float4_det, c_float4_ope, c_float8_det, c_float8_ope, c_numeric_det, c_numeric_ope) VALUES
('张三', '张三', '李四', '李四', '王五', '王五', 100, 100, 20000, 20000, 3000000, 3000000, 3.14, 3.14, 2.718281828, 2.718281828, 99999.99, 99999.99),
('Alice', 'Alice', 'Bob', 'Bob', 'Charlie', 'Charlie', 1, 1, 1000, 1000, 50000, 50000, 1.5, 1.5, 100.5, 100.5, 123.45, 123.45),
('赵六', '赵六', '孙七', '孙七', '周八', '周八', 32767, 32767, 2147483647, 2147483647, 9223372036854775807, 9223372036854775807, 1.0e10, 1.0e10, 1.7976931348623e308, 1.7976931348623e308, 0.01, 0.01),
('张三丰', '张三丰', 'David', 'David', 'Eve', 'Eve', -100, -100, -50000, -50000, -999999, -999999, -2.5, -2.5, -0.001, -0.001, -500.00, -500.00),
('hello', 'hello', 'world', 'world', 'test', 'test', 0, 0, 0, 0, 0, 0, 0.0, 0.0, 0.0, 0.0, 0.00, 0.00),
('alice', 'alice', 'ALICE', 'ALICE', 'alice', 'alice', 1, 1, 1, 1, 1, 1, 1.0, 1.0, 1.0, 1.0, 1.00, 1.00),
('张三丰', '张三丰', 'David2', 'David2', 'Eve2', 'Eve2', 100, 100, 20000, 20000, 3000000, 3000000, 3.14, 3.14, 2.718281828, 2.718281828, 99999.99, 99999.99),
('Hello', 'Hello', 'World', 'World', 'TEST', 'TEST', 50, 50, 55555, 55555, 7777777, 7777777, 50.5, 50.5, 55.55, 55.55, 500.00, 500.00),
('abc_def', 'abc_def', 'xyz_123', 'xyz_123', 'foo_bar', 'foo_bar', -1, -1, -1, -1, -1, -1, -1.0, -1.0, -1.0, -1.0, -1.00, -1.00),
('test%%like', 'test%%like', 'test_like','test_like','pct_sign', 'pct_sign', 10, 10, 100, 100, 1000, 1000, 10.10, 10.10, 100.10, 100.10, 10.10, 10.10),
('%%percent', '%%percent', 'under_score','under_score','_start', '_start', 20, 20, 200, 200, 2000, 2000, 20.20, 20.20, 200.20, 200.20, 20.20, 20.20),
(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL),
('', '', '', '', '', '', -32768, -32768, -2147483648, -2147483648, -9223372036854775808, -9223372036854775808, -3.402823e38, -3.402823e38, -1.7976931348e308, -1.7976931348e308, -99999.99, -99999.99),
('测试', '测试', '测试', '测试', '测试', '测试', 42, 42, 4242, 4242, 424242, 424242, 4.2, 4.2, 42.42, 42.42, 42.42, 42.42),
('test123', 'test123', 'abcXYZ', 'abcXYZ', 'mixCase', 'mixCase', 255, 255, 65535, 65535, 16777215, 16777215, 255.255, 255.255, 65535.65535, 65535.65535, 255.55, 255.55);

-- ==================== OPE 查询对比 ====================

-- 1. bpchar_enc OPE
SELECT '=== 1a bpchar_ope > ''测试'' ORDER BY ===';
SELECT id, c_bpchar FROM plain WHERE c_bpchar > '测试' ORDER BY c_bpchar;
SELECT id, c_bpchar_ope FROM enc WHERE c_bpchar_ope > '测试' ORDER BY c_bpchar_ope;

SELECT '=== 1b bpchar_ope >= ''王五'' ORDER BY ===';
SELECT id, c_bpchar FROM plain WHERE c_bpchar >= '王五' ORDER BY c_bpchar;
SELECT id, c_bpchar_ope FROM enc WHERE c_bpchar_ope >= '王五' ORDER BY c_bpchar_ope;

SELECT '=== 1c bpchar_ope >= ''apple'' AND < ''cherry'' ORDER BY ===';
SELECT id, c_bpchar FROM plain WHERE c_bpchar >= 'apple' AND c_bpchar < 'cherry' ORDER BY c_bpchar;
SELECT id, c_bpchar_ope FROM enc WHERE c_bpchar_ope >= 'apple' AND c_bpchar_ope < 'cherry' ORDER BY c_bpchar_ope;

SELECT '=== 1d bpchar_ope = ''测试'' ===';
SELECT id, c_bpchar FROM plain WHERE c_bpchar = '测试';
SELECT id, c_bpchar_ope FROM enc WHERE c_bpchar_ope = '测试';

SELECT '=== 1e bpchar_ope >= ''apple'' AND <= ''cherry'' AND != ''banana'' ORDER BY ===';
SELECT id, c_bpchar FROM plain WHERE c_bpchar >= 'apple' AND c_bpchar <= 'cherry' AND c_bpchar != 'banana' ORDER BY c_bpchar;
SELECT id, c_bpchar_ope FROM enc WHERE c_bpchar_ope >= 'apple' AND c_bpchar_ope <= 'cherry' AND c_bpchar_ope != 'banana' ORDER BY c_bpchar_ope;

-- 2. text_enc OPE
SELECT '=== 2a text_ope >= ''张三'' AND < ''赵六'' ORDER BY ===';
SELECT id, c_text FROM plain WHERE c_text >= '张三' AND c_text < '赵六' ORDER BY c_text;
SELECT id, c_text_ope FROM enc WHERE c_text_ope >= '张三' AND c_text_ope < '赵六' ORDER BY c_text_ope;

SELECT '=== 2b text_ope > ''apple'' AND < ''cherry'' ORDER BY ===';
SELECT id, c_text FROM plain WHERE c_text > 'apple' AND c_text < 'cherry' ORDER BY c_text;
SELECT id, c_text_ope FROM enc WHERE c_text_ope > 'apple' AND c_text_ope < 'cherry' ORDER BY c_text_ope;

SELECT '=== 2c text_ope ''apple''~''cherry'' OR ''a''~''b'' ORDER BY ===';
SELECT id, c_text FROM plain WHERE (c_text > 'a' AND c_text < 'b') OR (c_text >= 'banana' AND c_text <= 'cherry') ORDER BY c_text;
SELECT id, c_text_ope FROM enc WHERE (c_text_ope > 'a' AND c_text_ope < 'b') OR (c_text_ope >= 'banana' AND c_text_ope <= 'cherry') ORDER BY c_text_ope;

SELECT '=== 2d text_ope = ''张三丰'' ===';
SELECT id, c_text FROM plain WHERE c_text = '张三丰';
SELECT id, c_text_ope FROM enc WHERE c_text_ope = '张三丰';

-- 3. varchar_enc OPE
SELECT '=== 3a varchar_ope >= ''apple'' AND <= ''cherry'' ORDER BY ===';
SELECT id, c_varchar FROM plain WHERE c_varchar >= 'apple' AND c_varchar <= 'cherry' ORDER BY c_varchar;
SELECT id, c_varchar_ope FROM enc WHERE c_varchar_ope >= 'apple' AND c_varchar_ope <= 'cherry' ORDER BY c_varchar_ope;

SELECT '=== 3b varchar_ope > ''David'' AND < ''World'' ORDER BY ===';
SELECT id, c_varchar FROM plain WHERE c_varchar > 'David' AND c_varchar < 'World' ORDER BY c_varchar;
SELECT id, c_varchar_ope FROM enc WHERE c_varchar_ope > 'David' AND c_varchar_ope < 'World' ORDER BY c_varchar_ope;

SELECT '=== 3c varchar_ope > ''apple'' AND ( < ''berry'' OR > ''cherry'') ORDER BY ===';
SELECT id, c_varchar FROM plain WHERE c_varchar > 'apple' AND (c_varchar < 'berry' OR c_varchar > 'cherry') ORDER BY c_varchar;
SELECT id, c_varchar_ope FROM enc WHERE c_varchar_ope > 'apple' AND (c_varchar_ope < 'berry' OR c_varchar_ope > 'cherry') ORDER BY c_varchar_ope;

SELECT '=== 3d varchar_ope = ''测试'' ===';
SELECT id, c_varchar FROM plain WHERE c_varchar = '测试';
SELECT id, c_varchar_ope FROM enc WHERE c_varchar_ope = '测试';

-- 4. ORDER BY
SELECT '=== 4a ORDER BY bpchar_ope ===';
SELECT id, c_bpchar FROM plain ORDER BY c_bpchar;
SELECT id, c_bpchar_ope FROM enc ORDER BY c_bpchar_ope;

SELECT '=== 4b ORDER BY text_ope ===';
SELECT id, c_text FROM plain ORDER BY c_text;
SELECT id, c_text_ope FROM enc ORDER BY c_text_ope;

SELECT '=== 4c ORDER BY varchar_ope ===';
SELECT id, c_varchar FROM plain ORDER BY c_varchar;
SELECT id, c_varchar_ope FROM enc ORDER BY c_varchar_ope;

-- 5. NULL和空串边界
SELECT '=== 5a bpchar_ope > '''' ORDER BY ===';
SELECT id, c_bpchar FROM plain WHERE c_bpchar > '' ORDER BY c_bpchar;
SELECT id, c_bpchar_ope FROM enc WHERE c_bpchar_ope > '' ORDER BY c_bpchar_ope;

DROP TABLE IF EXISTS enc CASCADE;
DROP TABLE IF EXISTS plain CASCADE;
DROP DEK IF EXISTS test_dek CASCADE;
DROP KEK IF EXISTS test_kek CASCADE;
