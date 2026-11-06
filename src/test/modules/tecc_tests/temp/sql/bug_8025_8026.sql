-- ============================================================
-- Bug 8025: varchar_enc OPE+search 列范围查询报错
-- Bug 8026: bpchar_enc OPE范围查询结果与明文不一致
-- 根因: hg_encrypted_columns_info视图对default排序规则(OID=100)
--       返回空串collation_locale，前端走binary路径与ICU排序不一致
-- ============================================================

-- 清理
DROP TABLE IF EXISTS tecc_text_type_test CASCADE;
DROP TABLE IF EXISTS text_type_test CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 建表（字符型ope列）
CREATE TABLE tecc_text_type_test (id SERIAL PRIMARY KEY, text_col text_enc encrypted with (dek = dek_t1,support_algo='ope'), varchar_col varchar_enc encrypted with (dek = dek_t1,support_algo='ope'), bpchar_col bpchar_enc encrypted with (dek = dek_t1,support_algo='ope'));
CREATE TABLE text_type_test (id SERIAL PRIMARY KEY, text_col text, varchar_col varchar, bpchar_col bpchar);

INSERT INTO text_type_test (text_col, varchar_col, bpchar_col) VALUES('apple', 'apple', 'apple'),('application', 'application', 'application'),('banana', 'banana', 'banana'),('berry', 'berry', 'berry'),('cherry', 'cherry', 'cherry'),('Apple', 'Apple', 'Apple'),('APPLE', 'APPLE', 'APPLE'),('123apple', '123apple', '123apple'),('apple123', 'apple123', 'apple123'),('a', 'a', 'a'),('zoo', 'zoo', 'zoo'),('', '', ''),(NULL, NULL, NULL);
INSERT INTO tecc_text_type_test (text_col, varchar_col, bpchar_col) VALUES('apple', 'apple', 'apple'),('application', 'application', 'application'),('banana', 'banana', 'banana'),('berry', 'berry', 'berry'),('cherry', 'cherry', 'cherry'),('Apple', 'Apple', 'Apple'),('APPLE', 'APPLE', 'APPLE'),('123apple', '123apple', '123apple'),('apple123', 'apple123', 'apple123'),('a', 'a', 'a'),('zoo', 'zoo', 'zoo'),('', '', ''),(NULL, NULL, NULL);

-- Bug 8025: varchar_enc OPE范围查询
SELECT 'bug8025_varchar_range', id, varchar_col FROM tecc_text_type_test WHERE varchar_col >= 'apple' AND varchar_col <= 'cherry' ORDER BY varchar_col;

-- Bug 8026: bpchar_enc OPE范围查询
SELECT 'bug8026_bpchar_range', id, bpchar_col FROM tecc_text_type_test WHERE bpchar_col >= 'banana' AND bpchar_col < 'cherry' ORDER BY bpchar_col;

-- text_enc范围查询（同路径）
SELECT 'text_range', id, text_col FROM tecc_text_type_test WHERE text_col > 'apple' AND text_col < 'cherry' ORDER BY text_col;

-- 复杂条件范围查询
SELECT 'complex_range', id, text_col FROM tecc_text_type_test WHERE (text_col > 'a' AND text_col < 'b') OR (text_col >= 'banana' AND text_col <= 'cherry') ORDER BY text_col;

-- 清理
DROP TABLE IF EXISTS tecc_text_type_test CASCADE;
DROP TABLE IF EXISTS text_type_test CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
