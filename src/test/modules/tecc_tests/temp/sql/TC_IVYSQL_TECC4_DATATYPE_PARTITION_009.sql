-- 清理
DROP TABLE IF EXISTS partition_float8_main CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 测试步骤
CREATE TABLE partition_float8_main (id SERIAL,amount float8_enc encrypted with (dek = dek_t1) ,range_code CHAR(1),PRIMARY KEY (id, amount)) PARTITION BY RANGE (amount);

CREATE TABLE partition_float8_scale_n PARTITION OF partition_float8_main FOR VALUES FROM ('-Infinity') TO (-1.0);
CREATE TABLE partition_float8_scale_z PARTITION OF partition_float8_main FOR VALUES FROM (-1.0) TO (1.0);
CREATE TABLE partition_float8_scale_p PARTITION OF partition_float8_main FOR VALUES FROM (1.0) TO (10.0);
CREATE TABLE partition_float8_scale_l PARTITION OF partition_float8_main FOR VALUES FROM (10.0) TO ('Infinity');

INSERT INTO partition_float8_main (amount, range_code) VALUES (-10.5, 'N'), (0.5, 'Z'), (5.5, 'P'), (50.5, 'L');

SELECT float8_enc as table_type, tableoid::regclass as partition_name, COUNT(*) FROM partition_float8_main GROUP BY tableoid;

-- 清理
DROP TABLE IF EXISTS partition_float8_main CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
