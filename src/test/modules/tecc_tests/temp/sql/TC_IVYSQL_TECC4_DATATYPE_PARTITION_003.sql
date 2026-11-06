-- 清理
DROP TABLE IF EXISTS partition_bpchar_main CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 测试步骤
CREATE TABLE partition_bpchar_main (id SERIAL, content bpchar_enc encrypted with (dek = dek_t1), type_code bpchar_enc encrypted with (dek = dek_t1), PRIMARY KEY (id, type_code)) PARTITION BY LIST (type_code);

CREATE TABLE partition_bpchar_type_aa PARTITION OF partition_bpchar_main FOR VALUES IN ('AA', 'AB', 'AC');
CREATE TABLE partition_bpchar_type_ba PARTITION OF partition_bpchar_main FOR VALUES IN ('BA', 'BB', 'BC');
CREATE TABLE partition_bpchar_type_ca PARTITION OF partition_bpchar_main FOR VALUES IN ('CA', 'CB', 'CC');
CREATE TABLE partition_bpchar_type_other PARTITION OF partition_bpchar_main DEFAULT;

INSERT INTO partition_bpchar_main (content, type_code) VALUES ('文本内容1', 'AA'), ('文本内容2', 'BB'), ('文本内容3', 'CC'), ('文本内容4', 'ZZ');

SELECT bpchar_enc as table_type, tableoid::regclass as partition_name, COUNT(*) FROM partition_bpchar_main GROUP BY tableoid;

-- 清理
DROP TABLE IF EXISTS partition_bpchar_main CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
