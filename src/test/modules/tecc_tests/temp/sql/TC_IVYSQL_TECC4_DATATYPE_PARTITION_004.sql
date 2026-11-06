-- 清理
DROP TABLE IF EXISTS partition_int2_main CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 测试步骤
CREATE TABLE partition_int2_main (id SERIAL, value int2_enc encrypted with (dek = dek_t1),category CHAR(1),PRIMARY KEY (id, value)) PARTITION BY RANGE (value);

CREATE TABLE partition_int2_range1 PARTITION OF partition_int2_main FOR VALUES FROM (-32767::int2) TO (-10000::int2);
CREATE TABLE partition_int2_range2 PARTITION OF partition_int2_main FOR VALUES FROM (-10000::int2) TO (0::int2);
CREATE TABLE partition_int2_range3 PARTITION OF partition_int2_main FOR VALUES FROM (0::int2) TO (10000::int2);
CREATE TABLE partition_int2_range4 PARTITION OF partition_int2_main FOR VALUES FROM (10000::int2) TO (32767::int2);

INSERT INTO partition_int2_main (value, category) VALUES (-20000::int2, 'A'), (-5000::int2, 'B'), (5000::int2, 'C'), (20000::int2, 'D');

SELECT int2_enc as table_type, tableoid::regclass as partition_name, COUNT(*) FROM partition_int2_main GROUP BY tableoid;

-- 清理
DROP TABLE IF EXISTS partition_int2_main CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
