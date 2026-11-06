-- 清理
DROP TABLE IF EXISTS tb_string CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 测试步骤
select 'test'::text_enc encrypted with (dek = dek_t1)::varchar_enc encrypted with (dek = dek_t1),'test'::text_enc encrypted with (dek = dek_t1)::bpchar_enc encrypted with (dek = dek_t1);
select 'test'::varchar_enc encrypted with (dek = dek_t1)::text_enc encrypted with (dek = dek_t1),'test'::varchar_enc encrypted with (dek = dek_t1)::bpchar_enc encrypted with (dek = dek_t1);
select 'test'::bpchar_enc encrypted with (dek = dek_t1)::text_enc encrypted with (dek = dek_t1),'test'::bpchar_enc encrypted with (dek = dek_t1)::varchar_enc encrypted with (dek = dek_t1);

-- 清理
DROP TABLE IF EXISTS tb_string CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
