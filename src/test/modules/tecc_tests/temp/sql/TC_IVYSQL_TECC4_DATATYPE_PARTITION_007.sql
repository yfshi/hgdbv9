-- 清理
DROP TABLE IF EXISTS partition_numeric_main CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 测试步骤
CREATE TABLE partition_numeric_main (id SERIAL,amount numeric_enc encrypted with (dek = dek_t1) ,range_code CHAR(1),PRIMARY KEY (id, amount)) PARTITION BY RANGE (amount);

CREATE TABLE partition_numeric_range_a PARTITION OF partition_numeric_main FOR VALUES FROM (0.00) TO (100.00);
CREATE TABLE partition_numeric_range_b PARTITION OF partition_numeric_main FOR VALUES FROM (100.00) TO (1000.00);
CREATE TABLE partition_numeric_range_c PARTITION OF partition_numeric_main FOR VALUES FROM (1000.00) TO (10000.00);
CREATE TABLE partition_numeric_range_d PARTITION OF partition_numeric_main FOR VALUES FROM (10000.00) TO (99999999.99);

INSERT INTO partition_numeric_main (amount, range_code) VALUES (50.00, 'A'), (500.00, 'B'), (5000.00, 'C'), (50000.00, 'D');

SELECT numeric_enc as table_type, tableoid::regclass as partition_name, COUNT(*) FROM partition_numeric_main GROUP BY tableoid;

-- 清理
DROP TABLE IF EXISTS partition_numeric_main CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
