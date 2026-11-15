\set ON_ERROR_STOP off
\pset null '(null)'
\set VERBOSITY terse

\echo '=== 创建 KEK/DEK ==='
CREATE KEK IF NOT EXISTS kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');

CREATE DEK IF NOT EXISTS dek_t1 WITH VALUES(KEK=kek_t1);

DROP TABLE IF EXISTS partition_text_main, partition_text_type_aa, partition_text_type_ba, partition_text_type_ca, partition_text_type_other CASCADE;

DROP TABLE IF EXISTS partition_varchar_main, partition_varchar_type_aa, partition_varchar_type_ba, partition_varchar_type_ca, partition_varchar_type_other CASCADE;

DROP TABLE IF EXISTS partition_bpchar_main, partition_bpchar_type_aa, partition_bpchar_type_ba, partition_bpchar_type_ca, partition_bpchar_type_other CASCADE;

DROP TABLE IF EXISTS partition_int2_main, partition_int2_range1, partition_int2_range2, partition_int2_range3, partition_int2_range4 CASCADE;

DROP TABLE IF EXISTS partition_int4_main, partition_int4_neg_large, partition_int4_neg_small, partition_int4_pos_small, partition_int4_pos_large CASCADE;

DROP TABLE IF EXISTS partition_int8_main, partition_int8_level_a, partition_int8_level_b, partition_int8_level_c, partition_int8_level_d CASCADE;

DROP TABLE IF EXISTS partition_numeric_main, partition_numeric_range_neg, partition_numeric_range_a, partition_numeric_range_b, partition_numeric_range_c, partition_numeric_range_d CASCADE;

DROP TABLE IF EXISTS partition_float4_main, partition_float4_zone_f, partition_float4_zone_c, partition_float4_zone_n, partition_float4_zone_w CASCADE;

DROP TABLE IF EXISTS partition_float8_main, partition_float8_scale_n, partition_float8_scale_z, partition_float8_scale_p, partition_float8_scale_l CASCADE;

\echo ''
\echo '#####################################################################'
\echo '# 1. text_enc LIST 分区'
\echo '#####################################################################'

CREATE TABLE partition_text_main (
    id SERIAL,
    content text_enc ENCRYPTED WITH (dek=dek_t1),
    type_code text_enc ENCRYPTED WITH (dek=dek_t1, support_algo='ope'),
    PRIMARY KEY (id, type_code)
) PARTITION BY LIST (type_code);

CREATE TABLE partition_text_type_aa PARTITION OF partition_text_main FOR VALUES IN ('AA', 'AB', 'AC');

CREATE TABLE partition_text_type_ba PARTITION OF partition_text_main FOR VALUES IN ('BA', 'BB', 'BC');

CREATE TABLE partition_text_type_ca PARTITION OF partition_text_main FOR VALUES IN ('CA', 'CB', 'CC');

CREATE TABLE partition_text_type_other PARTITION OF partition_text_main DEFAULT;

-- @param String 文本内容1
-- @param String AA
-- @param String 文本内容2
-- @param String BB
-- @param String 文本内容3
-- @param String CC
-- @param String 文本内容4
-- @param String ZZ
INSERT INTO partition_text_main (content, type_code) VALUES (?, ?), (?, ?), (?, ?), (?, ?);

\echo '--- 分区数据分布 ---'
SELECT 'text_enc' AS table_type, tableoid::regclass AS partition_name, COUNT(*) FROM partition_text_main GROUP BY tableoid ORDER BY partition_name;

\echo '--- 分区裁剪验证 (WHERE type_code=AA) ---'
SELECT * FROM partition_text_main WHERE type_code = 'AA';

\echo ''
\echo '#####################################################################'
\echo '# 2. varchar_enc LIST 分区'
\echo '#####################################################################'

CREATE TABLE partition_varchar_main (
    id SERIAL,
    content varchar_enc ENCRYPTED WITH (dek=dek_t1),
    type_code varchar_enc ENCRYPTED WITH (dek=dek_t1, support_algo='ope'),
    PRIMARY KEY (id, type_code)
) PARTITION BY LIST (type_code);

CREATE TABLE partition_varchar_type_aa PARTITION OF partition_varchar_main FOR VALUES IN ('AA', 'AB', 'AC');

CREATE TABLE partition_varchar_type_ba PARTITION OF partition_varchar_main FOR VALUES IN ('BA', 'BB', 'BC');

CREATE TABLE partition_varchar_type_ca PARTITION OF partition_varchar_main FOR VALUES IN ('CA', 'CB', 'CC');

CREATE TABLE partition_varchar_type_other PARTITION OF partition_varchar_main DEFAULT;

-- @param String 文本内容1
-- @param String AA
-- @param String 文本内容2
-- @param String BB
-- @param String 文本内容3
-- @param String CC
-- @param String 文本内容4
-- @param String ZZ
INSERT INTO partition_varchar_main (content, type_code) VALUES (?, ?), (?, ?), (?, ?), (?, ?);

\echo '--- 分区数据分布 ---'
SELECT 'varchar_enc' AS table_type, tableoid::regclass AS partition_name, COUNT(*) FROM partition_varchar_main GROUP BY tableoid ORDER BY partition_name;

\echo '--- 分区裁剪验证 (WHERE type_code=BB) ---'
SELECT * FROM partition_varchar_main WHERE type_code = 'BB';

\echo ''
\echo '#####################################################################'
\echo '# 3. bpchar_enc LIST 分区'
\echo '#####################################################################'

CREATE TABLE partition_bpchar_main (
    id SERIAL,
    content bpchar_enc ENCRYPTED WITH (dek=dek_t1),
    type_code bpchar_enc ENCRYPTED WITH (dek=dek_t1, support_algo='ope'),
    PRIMARY KEY (id, type_code)
) PARTITION BY LIST (type_code);

CREATE TABLE partition_bpchar_type_aa PARTITION OF partition_bpchar_main FOR VALUES IN ('AA', 'AB', 'AC');

CREATE TABLE partition_bpchar_type_ba PARTITION OF partition_bpchar_main FOR VALUES IN ('BA', 'BB', 'BC');

CREATE TABLE partition_bpchar_type_ca PARTITION OF partition_bpchar_main FOR VALUES IN ('CA', 'CB', 'CC');

CREATE TABLE partition_bpchar_type_other PARTITION OF partition_bpchar_main DEFAULT;

-- @param String 文本内容1
-- @param String AA
-- @param String 文本内容2
-- @param String BB
-- @param String 文本内容3
-- @param String CC
-- @param String 文本内容4
-- @param String ZZ
INSERT INTO partition_bpchar_main (content, type_code) VALUES (?, ?), (?, ?), (?, ?), (?, ?);

\echo '--- 分区数据分布 ---'
SELECT 'bpchar_enc' AS table_type, tableoid::regclass AS partition_name, COUNT(*) FROM partition_bpchar_main GROUP BY tableoid ORDER BY partition_name;

\echo '--- 分区裁剪验证 (WHERE type_code=CC) ---'
SELECT * FROM partition_bpchar_main WHERE type_code = 'CC';

\echo ''
\echo '#####################################################################'
\echo '# 4. int2_enc RANGE 分区'
\echo '#####################################################################'

CREATE TABLE partition_int2_main (
    id SERIAL,
    value int2_enc ENCRYPTED WITH (dek=dek_t1, support_algo='ope'),
    category CHAR(1),
    PRIMARY KEY (id, value)
) PARTITION BY RANGE (value);

CREATE TABLE partition_int2_range1 PARTITION OF partition_int2_main FOR VALUES FROM (-32767::int2) TO (-10000::int2);

CREATE TABLE partition_int2_range2 PARTITION OF partition_int2_main FOR VALUES FROM (-10000::int2) TO (0::int2);

CREATE TABLE partition_int2_range3 PARTITION OF partition_int2_main FOR VALUES FROM (0::int2) TO (10000::int2);

CREATE TABLE partition_int2_range4 PARTITION OF partition_int2_main FOR VALUES FROM (10000::int2) TO (32767::int2);

-- @param int -20000
-- @param String A
-- @param int -5000
-- @param String B
-- @param int 5000
-- @param String C
-- @param int 20000
-- @param String D
INSERT INTO partition_int2_main (value, category) VALUES (?::int2, ?), (?::int2, ?), (?::int2, ?), (?::int2, ?);

\echo '--- 分区数据分布 ---'
SELECT 'int2_enc' AS table_type, tableoid::regclass AS partition_name, COUNT(*) FROM partition_int2_main GROUP BY tableoid ORDER BY partition_name;

\echo '--- 分区裁剪验证 (WHERE value=5000) ---'
SELECT * FROM partition_int2_main WHERE value = 5000;

\echo ''
\echo '#####################################################################'
\echo '# 5. int4_enc RANGE 分区'
\echo '#####################################################################'

CREATE TABLE partition_int4_main (
    id SERIAL,
    value int4_enc ENCRYPTED WITH (dek=dek_t1, support_algo='ope'),
    category CHAR(1),
    PRIMARY KEY (id, value)
) PARTITION BY RANGE (value);

CREATE TABLE partition_int4_neg_large PARTITION OF partition_int4_main FOR VALUES FROM (-2147483648) TO (-1000000);

CREATE TABLE partition_int4_neg_small PARTITION OF partition_int4_main FOR VALUES FROM (-1000000) TO (0);

CREATE TABLE partition_int4_pos_small PARTITION OF partition_int4_main FOR VALUES FROM (0) TO (1000000);

CREATE TABLE partition_int4_pos_large PARTITION OF partition_int4_main FOR VALUES FROM (1000000) TO (2147483647);

-- @param int -2000000
-- @param String A
-- @param int -500000
-- @param String B
-- @param int 500000
-- @param String C
-- @param int 2000000
-- @param String D
INSERT INTO partition_int4_main (value, category) VALUES (?, ?), (?, ?), (?, ?), (?, ?);

\echo '--- 分区数据分布 ---'
SELECT 'int4_enc' AS table_type, tableoid::regclass AS partition_name, COUNT(*) FROM partition_int4_main GROUP BY tableoid ORDER BY partition_name;

\echo '--- 分区裁剪验证 (WHERE value=-500000) ---'
SELECT * FROM partition_int4_main WHERE value = -500000;

\echo ''
\echo '#####################################################################'
\echo '# 6. int8_enc RANGE 分区'
\echo '#####################################################################'

CREATE TABLE partition_int8_main (
    id SERIAL,
    value int8_enc ENCRYPTED WITH (dek=dek_t1, support_algo='ope'),
    category CHAR(1),
    PRIMARY KEY (id, value)
) PARTITION BY RANGE (value);

CREATE TABLE partition_int8_level_a PARTITION OF partition_int8_main FOR VALUES FROM (-9223372036854775808) TO (-1000000000000000);

CREATE TABLE partition_int8_level_b PARTITION OF partition_int8_main FOR VALUES FROM (-1000000000000000) TO (0);

CREATE TABLE partition_int8_level_c PARTITION OF partition_int8_main FOR VALUES FROM (0) TO (1000000000000000);

CREATE TABLE partition_int8_level_d PARTITION OF partition_int8_main FOR VALUES FROM (1000000000000000) TO (9223372036854775807);

-- @param long -2000000000000000
-- @param String A
-- @param int -500000
-- @param String B
-- @param int 500000
-- @param String C
-- @param long 2000000000000000
-- @param String D
INSERT INTO partition_int8_main (value, category) VALUES (?, ?), (?, ?), (?, ?), (?, ?);

\echo '--- 分区数据分布 ---'
SELECT 'int8_enc' AS table_type, tableoid::regclass AS partition_name, COUNT(*) FROM partition_int8_main GROUP BY tableoid ORDER BY partition_name;

\echo '--- 分区裁剪验证 (WHERE value=500000) ---'
SELECT * FROM partition_int8_main WHERE value = 500000;

\echo ''
\echo '#####################################################################'
\echo '# 7. numeric_enc RANGE 分区'
\echo '#####################################################################'

CREATE TABLE partition_numeric_main (
    id SERIAL,
    amount numeric_enc ENCRYPTED WITH (dek=dek_t1, support_algo='ope'),
    range_code CHAR(1),
    PRIMARY KEY (id, amount)
) PARTITION BY RANGE (amount);

CREATE TABLE partition_numeric_range_neg PARTITION OF partition_numeric_main FOR VALUES FROM (-99999999.99) TO (0.00);

CREATE TABLE partition_numeric_range_a PARTITION OF partition_numeric_main FOR VALUES FROM (0.00) TO (100.00);

CREATE TABLE partition_numeric_range_b PARTITION OF partition_numeric_main FOR VALUES FROM (100.00) TO (1000.00);

CREATE TABLE partition_numeric_range_c PARTITION OF partition_numeric_main FOR VALUES FROM (1000.00) TO (10000.00);

CREATE TABLE partition_numeric_range_d PARTITION OF partition_numeric_main FOR VALUES FROM (10000.00) TO (99999999.99);

-- @param BigDecimal -500.00
-- @param String N
-- @param BigDecimal 50.00
-- @param String A
-- @param BigDecimal 500.00
-- @param String B
-- @param BigDecimal 5000.00
-- @param String C
-- @param BigDecimal 50000.00
-- @param String D
INSERT INTO partition_numeric_main (amount, range_code) VALUES (?, ?), (?, ?), (?, ?), (?, ?), (?, ?);

\echo '--- 分区数据分布 ---'
SELECT 'numeric_enc' AS table_type, tableoid::regclass AS partition_name, COUNT(*) FROM partition_numeric_main GROUP BY tableoid ORDER BY partition_name;

\echo '--- 分区裁剪验证 (WHERE amount=-500.00) ---'
SELECT * FROM partition_numeric_main WHERE amount = -500.00;

\echo ''
\echo '#####################################################################'
\echo '# 8. float4_enc RANGE 分区'
\echo '#####################################################################'

CREATE TABLE partition_float4_main (
    id SERIAL,
    temperature float4_enc ENCRYPTED WITH (dek=dek_t1, support_algo='ope'),
    temp_zone CHAR(1),
    PRIMARY KEY (id, temperature)
) PARTITION BY RANGE (temperature);

CREATE TABLE partition_float4_zone_f PARTITION OF partition_float4_main FOR VALUES FROM ('-Infinity') TO (-20.0);

CREATE TABLE partition_float4_zone_c PARTITION OF partition_float4_main FOR VALUES FROM (-20.0) TO (0.0);

CREATE TABLE partition_float4_zone_n PARTITION OF partition_float4_main FOR VALUES FROM (0.0) TO (20.0);

CREATE TABLE partition_float4_zone_w PARTITION OF partition_float4_main FOR VALUES FROM (20.0) TO ('Infinity');

-- @param BigDecimal -30.0
-- @param String F
-- @param BigDecimal -10.0
-- @param String C
-- @param BigDecimal 10.0
-- @param String N
-- @param BigDecimal 30.0
-- @param String W
INSERT INTO partition_float4_main (temperature, temp_zone) VALUES (?, ?), (?, ?), (?, ?), (?, ?);

\echo '--- 分区数据分布 ---'
SELECT 'float4_enc' AS table_type, tableoid::regclass AS partition_name, COUNT(*) FROM partition_float4_main GROUP BY tableoid ORDER BY partition_name;

\echo '--- 分区裁剪验证 (WHERE temperature=10.0) ---'
SELECT * FROM partition_float4_main WHERE temperature = 10.0;

\echo ''
\echo '#####################################################################'
\echo '# 9. float8_enc RANGE 分区'
\echo '#####################################################################'

CREATE TABLE partition_float8_main (
    id SERIAL,
    measurement float8_enc ENCRYPTED WITH (dek=dek_t1, support_algo='ope'),
    scale_code CHAR(1),
    PRIMARY KEY (id, measurement)
) PARTITION BY RANGE (measurement);

CREATE TABLE partition_float8_scale_n PARTITION OF partition_float8_main FOR VALUES FROM ('-Infinity') TO (-1.0);

CREATE TABLE partition_float8_scale_z PARTITION OF partition_float8_main FOR VALUES FROM (-1.0) TO (1.0);

CREATE TABLE partition_float8_scale_p PARTITION OF partition_float8_main FOR VALUES FROM (1.0) TO (10.0);

CREATE TABLE partition_float8_scale_l PARTITION OF partition_float8_main FOR VALUES FROM (10.0) TO ('Infinity');

-- @param BigDecimal -10.5
-- @param String N
-- @param BigDecimal 0.5
-- @param String Z
-- @param BigDecimal 5.5
-- @param String P
-- @param BigDecimal 50.5
-- @param String L
INSERT INTO partition_float8_main (measurement, scale_code) VALUES (?, ?), (?, ?), (?, ?), (?, ?);

\echo '--- 分区数据分布 ---'
SELECT 'float8_enc' AS table_type, tableoid::regclass AS partition_name, COUNT(*) FROM partition_float8_main GROUP BY tableoid ORDER BY partition_name;

\echo '--- 分区裁剪验证 (WHERE measurement=5.5) ---'
SELECT * FROM partition_float8_main WHERE measurement = 5.5;

\echo ''
\echo '#####################################################################'
\echo '# 分区表测试汇总'
\echo '#####################################################################'

SELECT 'text' AS t, tableoid::regclass AS p, count(*) FROM partition_text_main GROUP BY tableoid
UNION ALL
SELECT 'varchar', tableoid::regclass, count(*) FROM partition_varchar_main GROUP BY tableoid
UNION ALL
SELECT 'bpchar', tableoid::regclass, count(*) FROM partition_bpchar_main GROUP BY tableoid
UNION ALL
SELECT 'int2', tableoid::regclass, count(*) FROM partition_int2_main GROUP BY tableoid
UNION ALL
SELECT 'int4', tableoid::regclass, count(*) FROM partition_int4_main GROUP BY tableoid
UNION ALL
SELECT 'int8', tableoid::regclass, count(*) FROM partition_int8_main GROUP BY tableoid
UNION ALL
SELECT 'numeric', tableoid::regclass, count(*) FROM partition_numeric_main GROUP BY tableoid
UNION ALL
SELECT 'float4', tableoid::regclass, count(*) FROM partition_float4_main GROUP BY tableoid
UNION ALL
SELECT 'float8', tableoid::regclass, count(*) FROM partition_float8_main GROUP BY tableoid
ORDER BY t, p;

\echo ''
\echo '#####################################################################'
\echo '# 测试完成'
\echo '#####################################################################'

\echo ''
\echo '-- 清理开始 --'
DROP TABLE IF EXISTS partition_text_main, partition_text_type_aa, partition_text_type_ba, partition_text_type_ca, partition_text_type_other CASCADE;

DROP TABLE IF EXISTS partition_varchar_main, partition_varchar_type_aa, partition_varchar_type_ba, partition_varchar_type_ca, partition_varchar_type_other CASCADE;

DROP TABLE IF EXISTS partition_bpchar_main, partition_bpchar_type_aa, partition_bpchar_type_ba, partition_bpchar_type_ca, partition_bpchar_type_other CASCADE;

DROP TABLE IF EXISTS partition_int2_main, partition_int2_range1, partition_int2_range2, partition_int2_range3, partition_int2_range4 CASCADE;

DROP TABLE IF EXISTS partition_int4_main, partition_int4_neg_large, partition_int4_neg_small, partition_int4_pos_small, partition_int4_pos_large CASCADE;

DROP TABLE IF EXISTS partition_int8_main, partition_int8_level_a, partition_int8_level_b, partition_int8_level_c, partition_int8_level_d CASCADE;

DROP TABLE IF EXISTS partition_numeric_main, partition_numeric_range_neg, partition_numeric_range_a, partition_numeric_range_b, partition_numeric_range_c, partition_numeric_range_d CASCADE;

DROP TABLE IF EXISTS partition_float4_main, partition_float4_zone_f, partition_float4_zone_c, partition_float4_zone_n, partition_float4_zone_w CASCADE;

DROP TABLE IF EXISTS partition_float8_main, partition_float8_scale_n, partition_float8_scale_z, partition_float8_scale_p, partition_float8_scale_l CASCADE;

DROP DEK IF EXISTS dek_t1;

DROP KEK IF EXISTS kek_t1;
