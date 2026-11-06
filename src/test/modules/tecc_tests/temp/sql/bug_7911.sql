-- Bug 7911: 密态字符型大小写敏感比较查询结果不符
-- 前置条件: KEK kek_t1 和 DEK dek_t1 已存在

-- 清理旧表
DROP TABLE IF EXISTS tecc_text_type_test;
DROP TABLE IF EXISTS text_type_test;

-- 创建密态与普通测试表
CREATE TABLE tecc_text_type_test (
  id SERIAL PRIMARY KEY,
  text_col text_enc encrypted with (dek = dek_t1, support_algo='ope'),
  varchar_col varchar_enc encrypted with (dek = dek_t1, support_algo='ope'),
  bpchar_col bpchar_enc encrypted with (dek = dek_t1, support_algo='ope')
);

CREATE TABLE text_type_test (
  id SERIAL PRIMARY KEY,
  text_col text,
  varchar_col varchar,
  bpchar_col bpchar
);

-- 插入相同数据
INSERT INTO text_type_test (text_col, varchar_col, bpchar_col) VALUES
('apple', 'apple', 'apple'),
('application', 'application', 'application'),
('banana', 'banana', 'banana'),
('berry', 'berry', 'berry'),
('cherry', 'cherry', 'cherry'),
('Apple', 'Apple', 'Apple'),
('APPLE', 'APPLE', 'APPLE'),
('123apple', '123apple', '123apple'),
('apple123', 'apple123', 'apple123'),
('a', 'a', 'a'),
('zoo', 'zoo', 'zoo'),
('', '', ''),
(NULL, NULL, NULL);

INSERT INTO tecc_text_type_test (text_col, varchar_col, bpchar_col) VALUES
('apple', 'apple', 'apple'),
('application', 'application', 'application'),
('banana', 'banana', 'banana'),
('berry', 'berry', 'berry'),
('cherry', 'cherry', 'cherry'),
('Apple', 'Apple', 'Apple'),
('APPLE', 'APPLE', 'APPLE'),
('123apple', '123apple', '123apple'),
('apple123', 'apple123', 'apple123'),
('a', 'a', 'a'),
('zoo', 'zoo', 'zoo'),
('', '', ''),
(NULL, NULL, NULL);

-- 测试1: >= A AND < B (大小写敏感)
SELECT '=== Test1: text >= A AND < B ===' as info;
SELECT id, text_col, varchar_col, bpchar_col FROM text_type_test WHERE text_col >= 'A' AND text_col < 'B' ORDER BY text_col;
SELECT id, text_col, varchar_col, bpchar_col FROM tecc_text_type_test WHERE text_col >= 'A' AND text_col < 'B' ORDER BY text_col;

-- 测试2: >= a AND < b
SELECT '=== Test2: text >= a AND < b ===' as info;
SELECT id, text_col, varchar_col, bpchar_col FROM text_type_test WHERE text_col >= 'a' AND text_col < 'b' ORDER BY text_col;
SELECT id, text_col, varchar_col, bpchar_col FROM tecc_text_type_test WHERE text_col >= 'a' AND text_col < 'b' ORDER BY text_col;

-- 测试3: >= a AND < B (混合大小写)
SELECT '=== Test3: text >= a AND < B ===' as info;
SELECT id, text_col, varchar_col, bpchar_col FROM text_type_test WHERE text_col >= 'a' AND text_col < 'B' ORDER BY text_col;
SELECT id, text_col, varchar_col, bpchar_col FROM tecc_text_type_test WHERE text_col >= 'a' AND text_col < 'B' ORDER BY text_col;

-- 测试4: 大小写等值查询
SELECT '=== Test4: text = apple ===' as info;
SELECT id, text_col, varchar_col, bpchar_col FROM text_type_test WHERE text_col = 'apple' ORDER BY text_col;
SELECT id, text_col, varchar_col, bpchar_col FROM tecc_text_type_test WHERE text_col = 'apple' ORDER BY text_col;

SELECT '=== Test5: text = Apple ===' as info;
SELECT id, text_col, varchar_col, bpchar_col FROM text_type_test WHERE text_col = 'Apple' ORDER BY text_col;
SELECT id, text_col, varchar_col, bpchar_col FROM tecc_text_type_test WHERE text_col = 'Apple' ORDER BY text_col;
