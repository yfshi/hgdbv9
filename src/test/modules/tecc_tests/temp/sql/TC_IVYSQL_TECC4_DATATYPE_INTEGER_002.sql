-- 清理
DROP TABLE IF EXISTS tb_int4 CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 测试步骤
create table tb_int4( id SERIAL PRIMARY KEY, int_col int4_enc encrypted with (dek = dek_t1,support_algo='ope'), description TEXT);

INSERT INTO tb_int4 (int_col, description) VALUES (0, '零值'),(1, '正值'),(-1, '负值'),(1000000, '百万'),(-1000000, '负百万'),(2147483647, 'int4最大值'),(-2147483648, 'int4最小值'),(2147483646, '接近int4最大值'),(-2147483647, '接近int4最小值');

INSERT INTO tb_int4 (int_col, description) VALUES (2147483648, '超过int4最大值');
INSERT INTO tb_int4 (int_col, description) VALUES (-2147483649, '超过int4最小值');
INSERT INTO tb_int4 (int_col, description) VALUES (4294967295, '远超过int4最大值');
INSERT INTO tb_int4 (int_col, description) VALUES (-4294967295, '远超过int4最小值');
INSERT INTO tb_int4 (int_col, description) VALUES (12.34, '浮点值');

SELECT int_col, description, pg_typeof(int_col) as data_type FROM tb_int4 ORDER BY int_col NULLS FIRST;

-- 清理
DROP TABLE IF EXISTS tb_int4 CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
