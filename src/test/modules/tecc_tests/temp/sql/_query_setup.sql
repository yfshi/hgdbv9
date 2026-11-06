-- 查询模块前置建表脚本
-- 创建所有查询用例依赖的普通表和加密表

-- 清理旧表
DROP TABLE IF EXISTS string_type_test CASCADE;
DROP TABLE IF EXISTS tecc_string_type_test CASCADE;
DROP TABLE IF EXISTS int_type_test CASCADE;
DROP TABLE IF EXISTS tecc_int_type_test CASCADE;
DROP TABLE IF EXISTS float_type_test CASCADE;
DROP TABLE IF EXISTS tecc_float_type_test CASCADE;
DROP TABLE IF EXISTS text_type_test CASCADE;
DROP TABLE IF EXISTS tecc_text_type_test CASCADE;
DROP TABLE IF EXISTS text_like_test CASCADE;
DROP TABLE IF EXISTS tecc_text_like_test CASCADE;
DROP TABLE IF EXISTS text_not_like_test CASCADE;
DROP TABLE IF EXISTS tecc_text_not_like_test CASCADE;
DROP TABLE IF EXISTS data_type_test CASCADE;
DROP TABLE IF EXISTS tecc_data_type_test CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- KEK/DEK
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 1. DET字符型等值查询（SELECT_DET_001）
create table string_type_test(id SERIAL, txt_col TEXT, vc_col VARCHAR(100), bp_col BPCHAR(100));
create table tecc_string_type_test(id SERIAL, txt_col text_enc encrypted with (dek = dek_t1,support_algo='det'), vc_col varchar_enc encrypted with (dek = dek_t1,support_algo='det'), bp_col bpchar_enc encrypted with (dek = dek_t1,support_algo='det'));

-- 2. DET整型等值查询（SELECT_DET_002）
create table int_type_test(id SERIAL, int2_col INT2, int4_col INT4, int8_col INT8);
create table tecc_int_type_test(id SERIAL, int2_col int2_enc encrypted with (dek = dek_t1,support_algo='det'), int4_col int4_enc encrypted with (dek = dek_t1,support_algo='det'), int8_col int8_enc encrypted with (dek = dek_t1,support_algo='det'));

-- 3. DET浮点等值查询（SELECT_DET_003）
create table float_type_test(id SERIAL, num_col NUMERIC, f4_col FLOAT4, f8_col FLOAT8);
create table tecc_float_type_test(id SERIAL, num_col numeric_enc encrypted with (dek = dek_t1,support_algo='det'), f4_col float4_enc encrypted with (dek = dek_t1,support_algo='det'), f8_col float8_enc encrypted with (dek = dek_t1,support_algo='det'));

-- 4. OPE字符型范围查询（SELECT_OPE_001/002）
create table text_type_test(id SERIAL, text_col TEXT, varchar_col VARCHAR(100), bpchar_col BPCHAR(100));
create table tecc_text_type_test(id SERIAL, text_col text_enc encrypted with (dek = dek_t1,support_algo='ope'), varchar_col varchar_enc encrypted with (dek = dek_t1,support_algo='ope'), bpchar_col bpchar_enc encrypted with (dek = dek_t1,support_algo='ope'));

-- 5. SEARCH模糊查询（SELECT_SEARCH_009-024）
create table text_like_test(id SERIAL, text_col TEXT, varchar_col VARCHAR(100), bpchar_col BPCHAR(100));
create table tecc_text_like_test(id SERIAL, text_col text_enc encrypted with (dek = dek_t1,support_algo='search'), varchar_col varchar_enc encrypted with (dek = dek_t1,support_algo='search'), bpchar_col bpchar_enc encrypted with (dek = dek_t1,support_algo='search'));

create table text_not_like_test(id SERIAL, text_col TEXT, varchar_col VARCHAR(100), bpchar_col BPCHAR(100));
create table tecc_text_not_like_test(id SERIAL, text_col text_enc encrypted with (dek = dek_t1,support_algo='search'), varchar_col varchar_enc encrypted with (dek = dek_t1,support_algo='search'), bpchar_col bpchar_enc encrypted with (dek = dek_t1,support_algo='search'));

-- 6. 全类型混合查询（SELECT_LIMIT_001-007）
create table data_type_test(id SERIAL, text_col TEXT, varchar_col VARCHAR(100), bpchar_col BPCHAR(100), int2_col INT2, int4_col INT4, int8_col INT8, numeric_col NUMERIC, float4_col FLOAT4, float8_col FLOAT8);
create table tecc_data_type_test(id SERIAL, text_col text_enc encrypted with (dek = dek_t1,support_algo='det'), varchar_col varchar_enc encrypted with (dek = dek_t1,support_algo='det'), bpchar_col bpchar_enc encrypted with (dek = dek_t1,support_algo='det'), int2_col int2_enc encrypted with (dek = dek_t1,support_algo='det'), int4_col int4_enc encrypted with (dek = dek_t1,support_algo='det'), int8_col int8_enc encrypted with (dek = dek_t1,support_algo='det'), numeric_col numeric_enc encrypted with (dek = dek_t1,support_algo='det'), float4_col float4_enc encrypted with (dek = dek_t1,support_algo='det'), float8_col float8_enc encrypted with (dek = dek_t1,support_algo='det'));

-- 插入测试数据
-- string_type_test
INSERT INTO string_type_test (txt_col, vc_col, bp_col) VALUES ('hello','hello','hello'),('Hello','Hello','Hello'),('world','world','world'),('test','test','test'),('  hello  ','  hello  ','  hello  '),(NULL,NULL,NULL),('','',''),('apple','apple','apple'),('application','application','application');

INSERT INTO tecc_string_type_test (txt_col, vc_col, bp_col) VALUES ('hello','hello','hello'),('Hello','Hello','Hello'),('world','world','world'),('test','test','test'),('  hello  ','  hello  ','  hello  '),(NULL,NULL,NULL),('','',''),('apple','apple','apple'),('application','application','application');

-- int_type_test
INSERT INTO int_type_test (int2_col,int4_col,int8_col) VALUES (100,100,100),(200,200,200),(300,300,300),(32767,2147483647,9223372036854775807),(NULL,NULL,NULL),(10,20,30),(40,50,60),(15,25,35),(1,1,1),(2,2,2);

INSERT INTO tecc_int_type_test (int2_col,int4_col,int8_col) VALUES (100,100,100),(200,200,200),(300,300,300),(32767,2147483647,9223372036854775807),(NULL,NULL,NULL),(10,20,30),(40,50,60),(15,25,35),(1,1,1),(2,2,2);

-- float_type_test
INSERT INTO float_type_test (num_col,f4_col,f8_col) VALUES (1.0,1.0,1.0),(2.0,2.0,2.0),(3.0,3.0,3.0),(1234567890.12345678901234567890,1.23,1.23),(NULL,NULL,NULL);

INSERT INTO tecc_float_type_test (num_col,f4_col,f8_col) VALUES (1.0,1.0,1.0),(2.0,2.0,2.0),(3.0,3.0,3.0),(1234567890.12345678901234567890,1.23,1.23),(NULL,NULL,NULL);

-- text_type_test (OPE)
INSERT INTO text_type_test (text_col, varchar_col, bpchar_col) VALUES ('apple','apple','apple'),('application','application','application'),('banana','banana','banana'),('berry','berry','berry'),('cherry','cherry','cherry'),('a','a','a'),('b','b','b'),('c','c','c');

INSERT INTO tecc_text_type_test (text_col, varchar_col, bpchar_col) VALUES ('apple','apple','apple'),('application','application','application'),('banana','banana','banana'),('berry','berry','berry'),('cherry','cherry','cherry'),('a','a','a'),('b','b','b'),('c','c','c');

-- text_like_test (SEARCH)
INSERT INTO text_like_test (text_col, varchar_col, bpchar_col) VALUES ('apple','apple','apple'),('application','application','application'),('app123','app123','app123'),('banana','banana','banana'),('cherry','cherry','cherry'),('test123','test123','test123'),('123test','123test','123test');

INSERT INTO tecc_text_like_test (text_col, varchar_col, bpchar_col) VALUES ('apple','apple','apple'),('application','application','application'),('app123','app123','app123'),('banana','banana','banana'),('cherry','cherry','cherry'),('test123','test123','test123'),('123test','123test','123test');

INSERT INTO text_not_like_test (text_col, varchar_col, bpchar_col) VALUES ('apple','apple','apple'),('application','application','application'),('banana','banana','banana'),('cherry','cherry','cherry'),('test','test','test');

INSERT INTO tecc_text_not_like_test (text_col, varchar_col, bpchar_col) VALUES ('apple','apple','apple'),('application','application','application'),('banana','banana','banana'),('cherry','cherry','cherry'),('test','test','test');

-- data_type_test (全类型)
INSERT INTO data_type_test (text_col, varchar_col, bpchar_col, int2_col, int4_col, int8_col, numeric_col, float4_col, float8_col) VALUES ('test数据','test文本','test文本',1,100,1000,123.45,1.23,50.0),('test数据2','测试文本','测试文本',3,200,20000,345.67,3.45,70.0),('hello','world','test',5,300,30000,567.89,5.67,90.0),(NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);

INSERT INTO tecc_data_type_test (text_col, varchar_col, bpchar_col, int2_col, int4_col, int8_col, numeric_col, float4_col, float8_col) VALUES ('test数据','test文本','test文本',1,100,1000,123.45,1.23,50.0),('test数据2','测试文本','测试文本',3,200,20000,345.67,3.45,70.0),('hello','world','test',5,300,30000,567.89,5.67,90.0),(NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);

-- 注意：不清理，表保留供后续查询用例使用
