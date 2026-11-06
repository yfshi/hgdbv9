-- 清理
DROP TABLE IF EXISTS tb_int2 CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 测试步骤
create table tb_int2( id SERIAL PRIMARY KEY, smallint_col int2_enc encrypted with (dek = dek_t1,support_algo='ope'), description TEXT);

INSERT INTO tb_int2 (smallint_col, description) VALUES  (0, '零值'), (1, '正值'), (-1, '负值'), (32767, '最大值'),  (-32768, '最小值'),(32766, '接近最大值'),(-32767, '接近最小值'), (100, '中等正值'),(-100, '中等负值');

INSERT INTO tb_int2 (smallint_col, description) VALUES (32768, '超出最大值');
INSERT INTO tb_int2 (smallint_col, description) VALUES (-32769, '超出最小值');
INSERT INTO tb_int2 (smallint_col, description) VALUES(123.45, '浮点数');

SELECT smallint_col, description, pg_typeof(smallint_col) as data_type FROM tb_int2 ORDER BY smallint_col NULLS FIRST;

-- 清理
DROP TABLE IF EXISTS tb_int2 CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
