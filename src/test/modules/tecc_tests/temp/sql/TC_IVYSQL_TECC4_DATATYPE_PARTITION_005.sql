-- 清理
DROP TABLE IF EXISTS partition_int4_main CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 测试步骤
CREATE TABLE partition_int4_main (id SERIAL, value int4_enc encrypted with (dek = dek_t1),category CHAR(1),PRIMARY KEY (id, value)) PARTITION BY RANGE (value);

CREATE TABLE partition_int4_neg_large PARTITION OF partition_int4_main FOR VALUES FROM (-2147483648) TO (-1000000);
CREATE TABLE partition_int4_neg_small PARTITION OF partition_int4_main FOR VALUES FROM (-1000000) TO (0);
CREATE TABLE partition_int4_pos_small PARTITION OF partition_int4_main FOR VALUES FROM (0) TO (1000000);
CREATE TABLE partition_int4_pos_large PARTITION OF partition_int4_main FOR VALUES FROM (1000000) TO (2147483647);

INSERT INTO partition_int4_main (value, category) VALUES (-2000000, 'N'), (-500000, 'N'), (500000, 'P'), (2000000, 'P');

SELECT int4_enc as table_type, tableoid::regclass as partition_name, COUNT(*) FROM partition_int4_main GROUP BY tableoid;

-- 清理
DROP TABLE IF EXISTS partition_int4_main CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
