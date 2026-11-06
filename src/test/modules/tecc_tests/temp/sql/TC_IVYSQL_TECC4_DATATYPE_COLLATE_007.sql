-- 清理
DROP TABLE IF EXISTS tb_test_tecc CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 测试步骤
create table tb_test_tecc(id SERIAL PRIMARY KEY, col_text text_enc encrypted with (dek = dek_t1,support_algo='ope'), col_varchar varchar_enc encrypted with (dek = dek_t1,support_algo='ope'),col_bpchar bpchar_enc encrypted with (dek = dek_t1,support_algo='ope'));

INSERT INTO tb_test_tecc (col_text, col_varchar, col_bpchar) VALUES ('apple', 'apple', 'apple'),('application', 'application', 'application'),('banana', 'banana', 'banana'),('berry', 'berry', 'berry'),('cherry', 'cherry', 'cherry'),('Apple', 'Apple', 'Apple'),('APPLE', 'APPLE', 'APPLE'),('123apple', '123apple', '123apple'),('apple123', 'apple123', 'apple123'),('pineapple', 'pineapple', 'pineapple'),('grape fruit', 'grape fruit', 'grape fruit'),('red apple', 'red apple', 'red apple'),('green apple', 'green apple', 'green apple'),('', '', ''),('a', 'a', 'a'),('test data', 'test data', 'test data'),('data test', 'data test', 'data test'),('test', 'test', 'test'),('TEST', 'TEST', 'TEST'),('Test', 'Test', 'Test'),(NULL, NULL, NULL);

alter table tb_test_tecc alter column col_text set data type text_enc encrypted with (dek = dek_t1,support_algo='ope') collate "C";

alter table tb_test_tecc alter column col_varchar set data type varchar_enc encrypted with (dek = dek_t1,support_algo='ope') collate "ucs_basic";

alter table tb_test_tecc alter column col_bpchar set data type bpchar_enc encrypted with (dek = dek_t1,support_algo='ope') collate "zh_CN.utf8";

-- 清理
DROP TABLE IF EXISTS tb_test_tecc CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
