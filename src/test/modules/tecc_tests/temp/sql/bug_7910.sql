-- Bug 7910: 字符型范围查询结果与预期不符 + 多余日志
-- 前置条件: KEK kek_t1 和 DEK dek_t1 已存在

-- 密态表
DROP TABLE IF EXISTS tecc_text_type_test;
CREATE TABLE tecc_text_type_test (
  id SERIAL PRIMARY KEY,
  text_col text_enc encrypted with (dek = dek_t1, support_algo='ope'),
  varchar_col varchar_enc encrypted with (dek = dek_t1, support_algo='ope'),
  bpchar_col bpchar_enc encrypted with (dek = dek_t1, support_algo='ope')
);

-- 普通对照表
DROP TABLE IF EXISTS text_type_test;
CREATE TABLE text_type_test (
  id SERIAL PRIMARY KEY,
  text_col text,
  varchar_col varchar,
  bpchar_col bpchar
);

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

-- 测试1: 小于组合查询
SELECT '=== Test1: text > apple AND text < cherry ===' as info;
SELECT id, text_col, varchar_col, bpchar_col FROM text_type_test WHERE text_col > 'apple' AND text_col < 'cherry' ORDER BY text_col;
SELECT id, text_col, varchar_col, bpchar_col FROM tecc_text_type_test WHERE text_col > 'apple' AND text_col < 'cherry' ORDER BY text_col;

-- 测试2: 大于等于小于等于组合查询
SELECT '=== Test2: varchar >= apple AND varchar <= cherry ===' as info;
SELECT id, text_col, varchar_col, bpchar_col FROM text_type_test WHERE varchar_col >= 'apple' AND varchar_col <= 'cherry' ORDER BY varchar_col;
SELECT id, text_col, varchar_col, bpchar_col FROM tecc_text_type_test WHERE varchar_col >= 'apple' AND varchar_col <= 'cherry' ORDER BY varchar_col;

-- 测试3: 边界组合查询
SELECT '=== Test3: bpchar > banana AND bpchar < cherry ===' as info;
SELECT id, text_col, varchar_col, bpchar_col FROM text_type_test WHERE bpchar_col > 'banana' AND bpchar_col < 'cherry' ORDER BY bpchar_col;
SELECT id, text_col, varchar_col, bpchar_col FROM tecc_text_type_test WHERE bpchar_col > 'banana' AND bpchar_col < 'cherry' ORDER BY bpchar_col;

-- 测试4: 范围组合查询
SELECT '=== Test4: (text > a AND text < b) OR (text > banana AND text < cherry) ===' as info;
SELECT id, text_col, varchar_col, bpchar_col FROM text_type_test WHERE (text_col > 'a' AND text_col < 'b') OR (text_col > 'banana' AND text_col < 'cherry') ORDER BY text_col;
SELECT id, text_col, varchar_col, bpchar_col FROM tecc_text_type_test WHERE (text_col > 'a' AND text_col < 'b') OR (text_col > 'banana' AND text_col < 'cherry') ORDER BY text_col;

-- 测试5: 嵌套范围条件查询
SELECT '=== Test5: varchar > apple AND (varchar < berry OR varchar > cherry) ===' as info;
SELECT id, text_col, varchar_col, bpchar_col FROM text_type_test WHERE varchar_col > 'apple' AND (varchar_col < 'berry' OR varchar_col > 'cherry') ORDER BY varchar_col;
SELECT id, text_col, varchar_col, bpchar_col FROM tecc_text_type_test WHERE varchar_col > 'apple' AND (varchar_col < 'berry' OR varchar_col > 'cherry') ORDER BY varchar_col;

-- 测试6: 空字符串判断查询
SELECT '=== Test6: text > '' AND text < c ===' as info;
SELECT id, text_col, varchar_col, bpchar_col FROM text_type_test WHERE text_col > '' AND text_col < 'c' ORDER BY text_col;
SELECT id, text_col, varchar_col, bpchar_col FROM tecc_text_type_test WHERE text_col > '' AND text_col < 'c' ORDER BY text_col;

-- 测试7: 不等判断查询
SELECT '=== Test7: bpchar >= apple AND bpchar <= cherry AND bpchar != banana ===' as info;
SELECT id, text_col, varchar_col, bpchar_col FROM text_type_test WHERE bpchar_col >= 'apple' AND bpchar_col <= 'cherry' AND bpchar_col != 'banana' ORDER BY bpchar_col;
SELECT id, text_col, varchar_col, bpchar_col FROM tecc_text_type_test WHERE bpchar_col >= 'apple' AND bpchar_col <= 'cherry' AND bpchar_col != 'banana' ORDER BY bpchar_col;
