-- 清理
DROP TABLE IF EXISTS tb_test_tecc CASCADE;
DROP TABLE IF EXISTS tb_test_tecc2 CASCADE;
DROP TABLE IF EXISTS tb_test_tecc3 CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 测试步骤
create table tb_test_tecc(id SERIAL PRIMARY KEY, col_text text_enc encrypted with (dek = dek_t1) collate "C", col_varchar varchar_enc encrypted with (dek = dek_t1) collate "C",col_bpchar bpchar_enc encrypted with (dek = dek_t1) collate "C");

create table tb_test_tecc2(id SERIAL PRIMARY KEY, col_text text_enc encrypted with (dek = dek_t1,support_algo='det') collate "C", col_varchar varchar_enc encrypted with (dek = dek_t1,support_algo='det') collate "C",col_bpchar bpchar_enc encrypted with (dek = dek_t1,support_algo='det') collate "C");

create table tb_test_tecc3(id SERIAL PRIMARY KEY, col_text text_enc encrypted with (dek = dek_t1,support_algo='search') collate "C", col_varchar varchar_enc encrypted with (dek = dek_t1,support_algo='search') collate "C",col_bpchar bpchar_enc encrypted with (dek = dek_t1,support_algo='search') collate "C");

-- 清理
DROP TABLE IF EXISTS tb_test_tecc CASCADE;
DROP TABLE IF EXISTS tb_test_tecc2 CASCADE;
DROP TABLE IF EXISTS tb_test_tecc3 CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
