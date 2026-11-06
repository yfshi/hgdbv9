-- 清理
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 测试步骤
select 'test'::text_enc encrypted with (dek=dek_t1,support_algo='ope') collate "C",'test'::varchar_enc encrypted with (dek=dek_t1,support_algo='ope') collate "C",'test'::bpchar_enc encrypted with (dek=dek_t1,support_algo='ope') collate "C";

-- 清理
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
