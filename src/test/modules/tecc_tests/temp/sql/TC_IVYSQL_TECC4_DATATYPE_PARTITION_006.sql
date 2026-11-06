-- 清理
DROP TABLE IF EXISTS partition_int8_main CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 测试步骤
CREATE TABLE partition_int8_main (id SERIAL, value int8_enc encrypted with (dek = dek_t1),category CHAR(1),PRIMARY KEY (id, value)) PARTITION BY RANGE (value);

CREATE TABLE partition_int8_level_a PARTITION OF partition_int8_main FOR VALUES FROM (-9223372036854775808) TO (-1000000000000000);
CREATE TABLE partition_int8_level_b PARTITION OF partition_int8_main FOR VALUES FROM (-1000000000000000) TO (0);
CREATE TABLE partition_int8_level_c PARTITION OF partition_int8_main FOR VALUES FROM (0) TO (1000000000000000);
CREATE TABLE partition_int8_level_d PARTITION OF partition_int8_main FOR VALUES FROM (1000000000000000) TO (9223372036854775807);

INSERT INTO partition_int8_main (value, category) VALUES (-2000000000000000, 'A'), (-500000, 'B'), (500000, 'C'), (2000000000000000, 'D');

SELECT int8_enc as table_type, tableoid::regclass as partition_name, COUNT(*) FROM partition_int8_main GROUP BY tableoid;

-- 清理
DROP TABLE IF EXISTS partition_int8_main CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
