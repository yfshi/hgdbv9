-- 清理
DROP TABLE IF EXISTS tb_test CASCADE;
DROP DEK IF EXISTS dek_t2 CASCADE;
DROP KEK IF EXISTS kek_t2 CASCADE;

-- 测试步骤：不创建任何KEK/DEK，直接使用不存在的dek_t2
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
DROP DEK IF EXISTS dek_t2 CASCADE;
DROP KEK IF EXISTS kek_t2 CASCADE;
