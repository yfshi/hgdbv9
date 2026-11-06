-- 清理
DROP TABLE IF EXISTS tb_test CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件（创建KEK和DEK，但测试使用不存在的dek_t2）
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 测试步骤：9种类型全部使用错误的dek_name
create table tb_test(id SERIAL PRIMARY KEY, text_enc_col text_enc encrypted with (dek = dek_t2));
create table tb_test(id SERIAL PRIMARY KEY, varchar_enc_col bpchar_enc encrypted with (dek = dek_t2));
create table tb_test(id SERIAL PRIMARY KEY, bpchar_enc_col bpchar_enc encrypted with (dek = dek_t2));
create table tb_test(id SERIAL PRIMARY KEY, int2_enc_col int2_enc encrypted with (dek = dek_t2));
create table tb_test(id SERIAL PRIMARY KEY, int4_enc_col int4_enc encrypted with (dek = dek_t2));
create table tb_test(id SERIAL PRIMARY KEY, int8_enc_col int8_enc encrypted with (dek = dek_t2));
create table tb_test(id SERIAL PRIMARY KEY, numeric_enc_col numeric_enc encrypted with (dek = dek_t2));
create table tb_test(id SERIAL PRIMARY KEY, float4_enc_col float4_enc encrypted with (dek = dek_t2));
create table tb_test(id SERIAL PRIMARY KEY, float8_enc_col float4_enc encrypted with (dek = dek_t2));

-- 清理
DROP TABLE IF EXISTS tb_test CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
