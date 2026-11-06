-- 清理
DROP TABLE IF EXISTS tecc_text_type_test CASCADE;
DROP TABLE IF EXISTS text_type_test CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

CREATE TABLE tecc_text_type_test (id SERIAL PRIMARY KEY, text_col text_enc encrypted with (dek = dek_t1,support_algo='ope'), varchar_col varchar_enc encrypted with (dek = dek_t1,support_algo='ope'), bpchar_col bpchar_enc encrypted with (dek = dek_t1,support_algo='ope'));
CREATE TABLE text_type_test (id SERIAL PRIMARY KEY, text_col text, varchar_col varchar, bpchar_col bpchar);

INSERT INTO text_type_test (text_col, varchar_col, bpchar_col) VALUES('apple', 'apple', 'apple'),('application', 'application', 'application'),('banana', 'banana', 'banana'),('berry', 'berry', 'berry'),('cherry', 'cherry', 'cherry'),('Apple', 'Apple', 'Apple'),('APPLE', 'APPLE', 'APPLE'),('123apple', '123apple', '123apple'),('apple123', 'apple123', 'apple123'),('a', 'a', 'a'),('zoo', 'zoo', 'zoo'),('', '', ''),(NULL, NULL, NULL);
INSERT INTO tecc_text_type_test (text_col, varchar_col, bpchar_col) VALUES('apple', 'apple', 'apple'),('application', 'application', 'application'),('banana', 'banana', 'banana'),('berry', 'berry', 'berry'),('cherry', 'cherry', 'cherry'),('Apple', 'Apple', 'Apple'),('APPLE', 'APPLE', 'APPLE'),('123apple', '123apple', '123apple'),('apple123', 'apple123', 'apple123'),('a', 'a', 'a'),('zoo', 'zoo', 'zoo'),('', '', ''),(NULL, NULL, NULL);

-- 测试步骤（以下每个查询增加对应密文表查询以验证预期"普通表与密态表查询结果相同"）
SELECT id, text_col, varchar_col, bpchar_col FROM text_type_test WHERE bpchar_col > 'test' ORDER BY bpchar_col;
SELECT id, text_col, varchar_col, bpchar_col FROM tecc_text_type_test WHERE bpchar_col > 'test' ORDER BY bpchar_col;
SELECT id, text_col, varchar_col, bpchar_col FROM text_type_test WHERE text_col >= 'A' AND text_col < 'B' ORDER BY text_col;
SELECT id, text_col, varchar_col, bpchar_col FROM tecc_text_type_test WHERE text_col >= 'A' AND text_col < 'B' ORDER BY text_col;
SELECT id, text_col, varchar_col, bpchar_col FROM text_type_test WHERE text_col >= 'a' AND text_col < 'b' ORDER BY text_col;
SELECT id, text_col, varchar_col, bpchar_col FROM tecc_text_type_test WHERE text_col >= 'a' AND text_col < 'b' ORDER BY text_col;

-- 清理
DROP TABLE IF EXISTS tecc_text_type_test CASCADE;
DROP TABLE IF EXISTS text_type_test CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
