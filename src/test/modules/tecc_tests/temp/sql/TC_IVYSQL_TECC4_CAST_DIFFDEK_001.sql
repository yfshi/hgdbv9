-- 清理
DROP TABLE IF EXISTS example_table CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 测试步骤
CREATE TABLE example_table (id SERIAL PRIMARY KEY, col_text text_enc encrypted with (dek = dek_t1), col_varchar varchar_enc encrypted with (dek = dek_t1), col_bpchar bpchar_enc encrypted with (dek = dek_t1), col_int2 int2_enc encrypted with (dek = dek_t1), col_int4 int4_enc encrypted with (dek = dek_t1), col_int8 int8_enc encrypted with (dek = dek_t1), col_numeric numeric_enc encrypted with (dek = dek_t1),col_float4 float4_enc encrypted with (dek = dek_t1),col_float8 float8_enc encrypted with (dek = dek_t1));
INSERT INTO example_table (col_text, col_varchar,col_bpchar,col_int2,col_int4, col_int8,col_numeric,col_float4,col_float8) VALUES ('这是一个文本字段','可变长度字符串','anc',1234,123456,123456789012,12345678.90,123.456,123456.789012);
select col_text::text from example_table;
select col_varchar::varchar from example_table;
select col_bpchar::bpchar from example_table;
select col_int2::int2 from example_table;
select col_int4::int4 from example_table;
select col_int8::int8 from example_table;
select col_float4::float4 from example_table;
select col_float8::float8 from example_table;
select col_numeric::numeric from example_table;
alter table example_table alter column col_text set data type text;
alter table example_table alter column col_varcahr set data type varchar;
alter table example_table alter column col_bpcahr set data type bpchar;
alter table example_table alter column col_int2 set data type int2;
alter table example_table alter column col_int4 set data type int4;
alter table example_table alter column col_int8 set data type int8;
alter table example_table alter column col_float4 set data type float4;
alter table example_table alter column col_float8 set data type float8;
alter table example_table alter column col_numeric set data type numeric;

-- 清理
DROP TABLE IF EXISTS example_table CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
