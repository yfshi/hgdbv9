-- 清理
DROP TABLE IF EXISTS tb_int8 CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 测试步骤
create table tb_int8( id SERIAL PRIMARY KEY, bigint_col int8_enc encrypted with (dek = dek_t1,support_algo='ope'), description TEXT);

INSERT INTO tb_int8 (bigint_col, description) VALUES (0, '零值'),(1, '正值'),(-1, '负值'),(1000000000, '十亿'),(-1000000000, '负十亿'),(9223372036854775807, 'int8最大值'),(-9223372036854775808, 'int8最小值'),(9223372036854775806, '接近int8最大值'),(-9223372036854775807, '接近int8最小值');

INSERT INTO tb_int8 (bigint_col, description) VALUES (9223372036854775808, '超过int8最大值');
INSERT INTO tb_int8 (bigint_col, description) VALUES (-9223372036854775809, '超过int8最小值');
INSERT INTO tb_int8 (bigint_col, description) VALUES (12.24, '浮点值');

SELECT bigint_col, description, pg_typeof(bigint_col) as data_type FROM tb_int8 ORDER BY bigint_col NULLS FIRST;

-- 清理
DROP TABLE IF EXISTS tb_int8 CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
