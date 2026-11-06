-- 清理
DROP TABLE IF EXISTS tb_string CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 测试步骤
create table tb_string (test_col text_enc encrypted with (dek = dek_t1), varchar_col varchar_enc encrypted with (dek = dek_t1), bpchar_col bpchar_enc encrypted with (dek = dek_t1));
insert into tb_string values ('test'::varchar_enc encrypted with (dek = dek_t1),'test'::bpchar_enc encrypted with (dek = dek_t1), 'test'::text_enc encrypted with (dek = dek_t1)),('bpchar'::bpchar_enc encrypted with (dek = dek_t1),'test'::text_enc encrypted with (dek = dek_t1), 'test'::varchar_enc encrypted with (dek = dek_t1));

-- 清理
DROP TABLE IF EXISTS tb_string CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
