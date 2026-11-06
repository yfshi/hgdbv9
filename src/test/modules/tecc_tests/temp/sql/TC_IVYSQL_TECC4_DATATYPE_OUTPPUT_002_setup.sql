-- 清理
DROP TABLE IF EXISTS example_table CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 准备加密数据
CREATE TABLE example_table (id SERIAL PRIMARY KEY, col_text text_enc encrypted with (dek = dek_t1), col_varchar varchar_enc encrypted with (dek = dek_t1), col_bpchar bpchar_enc encrypted with (dek = dek_t1), col_int2 int2_enc encrypted with (dek = dek_t1), col_int4 int4_enc encrypted with (dek = dek_t1), col_int8 int8_enc encrypted with (dek = dek_t1), col_numeric numeric_enc encrypted with (dek = dek_t1),col_float4 float4_enc encrypted with (dek = dek_t1),col_float8 float8_enc encrypted with (dek = dek_t1));

INSERT INTO example_table (col_text, col_varchar,col_bpchar,col_int2,col_int4, col_int8,col_numeric,col_float4,col_float8) VALUES ('这是一个文本字段','可变长度字符串','anc',1234,123456,123456789012,12345678.90,123.456,123456.789012);

-- 注意：不清理，保留数据给_verify脚本
