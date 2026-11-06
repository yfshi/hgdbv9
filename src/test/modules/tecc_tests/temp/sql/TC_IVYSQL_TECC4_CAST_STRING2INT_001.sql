-- 清理
DROP TABLE IF EXISTS tb_string CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 测试步骤
select '222'::text_enc encrypted with (dek = dek_t1)::int2_enc encrypted with (dek = dek_t1);
select '222'::text_enc encrypted with (dek = dek_t1)::int4_enc encrypted with (dek = dek_t1);
select '222'::text_enc encrypted with (dek = dek_t1)::int8_enc encrypted with (dek = dek_t1);
select '222'::varchar_enc encrypted with (dek = dek_t1)::int2_enc encrypted with (dek = dek_t1);
select '222'::varchar_enc encrypted with (dek = dek_t1)::int4_enc encrypted with (dek = dek_t1);
select '222'::varchar_enc encrypted with (dek = dek_t1)::int8_enc encrypted with (dek = dek_t1);
select '222'::bpchar_enc encrypted with (dek = dek_t1)::int2_enc encrypted with (dek = dek_t1);
select '222'::bpchar_enc encrypted with (dek = dek_t1)::int4_enc encrypted with (dek = dek_t1);
select '222'::bpchar_enc encrypted with (dek = dek_t1)::int8_enc encrypted with (dek = dek_t1);

-- 清理
DROP TABLE IF EXISTS tb_string CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
