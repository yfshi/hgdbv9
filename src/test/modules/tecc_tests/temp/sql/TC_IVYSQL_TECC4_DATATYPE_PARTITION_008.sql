-- 清理
DROP TABLE IF EXISTS partition_float4_main CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 测试步骤
CREATE TABLE partition_float4_main (id SERIAL,amount float4_enc encrypted with (dek = dek_t1) ,range_code CHAR(1),PRIMARY KEY (id, amount)) PARTITION BY RANGE (amount);

CREATE TABLE partition_float4_zone_f PARTITION OF partition_float4_main FOR VALUES FROM ('-Infinity') TO (-20.0);
CREATE TABLE partition_float4_zone_c PARTITION OF partition_float4_main FOR VALUES FROM (-20.0) TO (0.0);
CREATE TABLE partition_float4_zone_n PARTITION OF partition_float4_main FOR VALUES FROM (0.0) TO (20.0);
CREATE TABLE partition_float4_zone_w PARTITION OF partition_float4_main FOR VALUES FROM (20.0) TO ('Infinity');

INSERT INTO partition_float4_main (amount, range_code) VALUES (-30.0, 'F'), (-10.0, 'C'), (10.0, 'N'), (30.0, 'W');

SELECT float4_enc as table_type, tableoid::regclass as partition_name, COUNT(*) FROM partition_float4_main GROUP BY tableoid;

-- 清理
DROP TABLE IF EXISTS partition_float4_main CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
