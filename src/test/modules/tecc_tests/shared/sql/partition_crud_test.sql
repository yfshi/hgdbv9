\set ON_ERROR_STOP on
\pset null '(null)'

-- ====================================================================
-- 分区表增删改查覆盖测试 (9种加密类型)
-- ====================================================================
-- 测试内容:
--   1. INSERT: 单行、多行、带列名、不带列名、DEFAULT、边界值
--   2. SELECT: 分区裁剪、WHERE等值/范围、ORDER BY、JOIN
--   3. UPDATE: 更新非分区键、更新分区键（跨分区移动）
--   4. DELETE: WHERE条件删除、全表删除
--   5. CTE/WITH: 子查询插入
-- ====================================================================

\echo '=== 1. 准备 KEK/DEK ==='
CREATE KEK IF NOT EXISTS kek_crud WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_crud.key');
CREATE DEK IF NOT EXISTS dek_crud WITH VALUES(KEK=kek_crud);

-- ====================================================================
-- text_enc LIST 分区完整测试
-- ====================================================================
\echo ''
\echo '#####################################################################'
\echo '# text_enc LIST 分区 - 完整增删改查'
\echo '#####################################################################'

DROP TABLE IF EXISTS t_text_main, t_text_a, t_text_b, t_text_other CASCADE;

CREATE TABLE t_text_main (
    id SERIAL,
    payload text_enc ENCRYPTED WITH (dek=dek_crud),
    tag text_enc ENCRYPTED WITH (dek=dek_crud, support_algo='ope'),
    PRIMARY KEY (id, tag)
) PARTITION BY LIST (tag);

CREATE TABLE t_text_a PARTITION OF t_text_main FOR VALUES IN ('A', 'B', 'C');
CREATE TABLE t_text_b PARTITION OF t_text_main FOR VALUES IN ('X', 'Y', 'Z');
CREATE TABLE t_text_other PARTITION OF t_text_main DEFAULT;

\echo '--- INSERT: 单行 ---'
INSERT INTO t_text_main (payload, tag) VALUES ('hello', 'A');
INSERT INTO t_text_main (payload, tag) VALUES ('world', 'X');
INSERT INTO t_text_main (payload, tag) VALUES ('zzzz', 'M');

\echo '--- INSERT: 多行 ---'
INSERT INTO t_text_main (payload, tag) VALUES ('data1', 'B'), ('data2', 'Y'), ('data3', 'N');

\echo '--- INSERT: 不带列名 ---'
INSERT INTO t_text_main VALUES (DEFAULT, 'no_cols', 'C');

\echo '--- SELECT: 分区数据分布 ---'
SELECT 'text' AS t, tableoid::regclass AS p, count(*) FROM t_text_main GROUP BY tableoid ORDER BY p;

\echo '--- SELECT: WHERE等值匹配 ---'
SELECT id, payload, tag FROM t_text_main WHERE tag = 'A';

\echo '--- SELECT: WHERE范围匹配 (仅OPE) ---'
SELECT id, payload, tag FROM t_text_main WHERE tag >= 'A' AND tag <= 'C' ORDER BY tag;

\echo '--- SELECT: ORDER BY ---'
SELECT id, payload, tag FROM t_text_main ORDER BY tag, id;

\echo '--- SELECT: JOIN自身 ---'
SELECT a.id, a.tag, b.tag AS peer_tag
FROM t_text_main a JOIN t_text_main b ON a.id = b.id
WHERE a.tag = 'A' AND b.tag = 'A';

\echo '--- UPDATE: 非分区键 ---'
UPDATE t_text_main SET payload = 'updated_hello' WHERE tag = 'A';
SELECT id, payload, tag FROM t_text_main WHERE tag = 'A';

\echo '--- UPDATE: 分区键 (跨分区移动) ---'
UPDATE t_text_main SET tag = 'X' WHERE tag = 'A';
SELECT 'text' AS t, tableoid::regclass AS p, count(*) FROM t_text_main GROUP BY tableoid ORDER BY p;

\echo '--- DELETE: WHERE条件 ---'
DELETE FROM t_text_main WHERE tag = 'M';
SELECT id, payload, tag FROM t_text_main WHERE tag = 'M';
\echo '(上述应为空 - 已删除)';

\echo '--- DELETE: 全表删除 ---'
DELETE FROM t_text_main WHERE tag = 'B';
SELECT 'text' AS t, tableoid::regclass AS p, count(*) FROM t_text_main GROUP BY tableoid ORDER BY p;

\echo '--- 验证剩余数据 ---'
SELECT id, payload, tag FROM t_text_main ORDER BY id;

-- ====================================================================
-- int2_enc RANGE 分区完整测试
-- ====================================================================
\echo ''
\echo '#####################################################################'
\echo '# int2_enc RANGE 分区 - 完整增删改查'
\echo '#####################################################################'

DROP TABLE IF EXISTS t_int2_main, t_int2_neg, t_int2_zero, t_int2_pos, t_int2_big CASCADE;

CREATE TABLE t_int2_main (
    id SERIAL,
    val int2_enc ENCRYPTED WITH (dek=dek_crud, support_algo='ope'),
    note CHAR(1),
    PRIMARY KEY (id, val)
) PARTITION BY RANGE (val);

CREATE TABLE t_int2_neg PARTITION OF t_int2_main FOR VALUES FROM (-32767::int2) TO (0::int2);
CREATE TABLE t_int2_pos PARTITION OF t_int2_main FOR VALUES FROM (0::int2) TO (100::int2);
CREATE TABLE t_int2_big PARTITION OF t_int2_main FOR VALUES FROM (100::int2) TO (32767::int2);

\echo '--- INSERT: 边界值 ---'
INSERT INTO t_int2_main (val, note) VALUES (-32767::int2, 'a');
INSERT INTO t_int2_main (val, note) VALUES (-1::int2, 'b');
INSERT INTO t_int2_main (val, note) VALUES (0::int2, 'c');
INSERT INTO t_int2_main (val, note) VALUES (99::int2, 'd');
INSERT INTO t_int2_main (val, note) VALUES (100::int2, 'e');
INSERT INTO t_int2_main (val, note) VALUES (32766::int2, 'f');

\echo '--- INSERT: 不带TYPECAST ---'
INSERT INTO t_int2_main (val, note) VALUES (-20000, 'g'), (-5000, 'h'), (500, 'i'), (20000, 'j');

\echo '--- SELECT: 分区数据分布 ---'
SELECT 'int2' AS t, tableoid::regclass AS p, count(*) FROM t_int2_main GROUP BY tableoid ORDER BY p;

\echo '--- SELECT: WHERE等值条件 ---'
SELECT id, val, note FROM t_int2_main WHERE val = 100;

\echo '--- SELECT: WHERE范围条件 ---'
SELECT id, val, note FROM t_int2_main WHERE val >= 0 AND val <= 200 ORDER BY val;

\echo '--- SELECT: ORDER BY ---'
SELECT id, val, note FROM t_int2_main ORDER BY val;

\echo '--- SELECT: 跨分区JOIN ---'
SELECT a.id, a.val AS v1, b.val AS v2
FROM t_int2_main a JOIN t_int2_main b ON a.id = b.id AND a.val = b.val
WHERE a.val = 100;

\echo '--- UPDATE: 跨分区move ---'
UPDATE t_int2_main SET val = 200 WHERE val = -1;
SELECT 'int2' AS t, tableoid::regclass AS p, count(*) FROM t_int2_main GROUP BY tableoid ORDER BY p;

\echo '--- DELETE: WHERE条件 ---'
DELETE FROM t_int2_main WHERE val = 0;
SELECT id, val, note FROM t_int2_main WHERE val = 0;
\echo '(上述应为空)';

\echo '--- 最终数据 ---'
SELECT id, val, note FROM t_int2_main ORDER BY val, id;

-- ====================================================================
-- numeric_enc RANGE 分区完整测试
-- ====================================================================
\echo ''
\echo '#####################################################################'
\echo '# numeric_enc RANGE 分区 - 完整增删改查'
\echo '#####################################################################'

DROP TABLE IF EXISTS t_num_main, t_num_neg, t_num_small, t_num_med, t_num_large CASCADE;

CREATE TABLE t_num_main (
    id SERIAL,
    amount numeric_enc ENCRYPTED WITH (dek=dek_crud, support_algo='ope'),
    note CHAR(1),
    PRIMARY KEY (id, amount)
) PARTITION BY RANGE (amount);

CREATE TABLE t_num_neg PARTITION OF t_num_main FOR VALUES FROM (-999999.99) TO (0.00);
CREATE TABLE t_num_small PARTITION OF t_num_main FOR VALUES FROM (0.00) TO (100.00);
CREATE TABLE t_num_med PARTITION OF t_num_main FOR VALUES FROM (100.00) TO (10000.00);
CREATE TABLE t_num_large PARTITION OF t_num_main FOR VALUES FROM (10000.00) TO (999999.99);

\echo '--- INSERT: 精确小数 ---'
INSERT INTO t_num_main (amount, note) VALUES (-500.50, 'A');
INSERT INTO t_num_main (amount, note) VALUES (0.00, 'B');
INSERT INTO t_num_main (amount, note) VALUES (99.99, 'C');
INSERT INTO t_num_main (amount, note) VALUES (100.00, 'D');
INSERT INTO t_num_main (amount, note) VALUES (5000.00, 'E');
INSERT INTO t_num_main (amount, note) VALUES (99999.99, 'F');

\echo '--- SELECT: 分区数据分布 ---'
SELECT 'num' AS t, tableoid::regclass AS p, count(*) FROM t_num_main GROUP BY tableoid ORDER BY p;

\echo '--- SELECT: 精确匹配 ---'
SELECT id, amount, note FROM t_num_main WHERE amount = 100.00;

\echo '--- SELECT: 范围匹配 ---'
SELECT id, amount, note FROM t_num_main WHERE amount > 50 AND amount < 200 ORDER BY amount;

\echo '--- SELECT: ORDER BY ---'
SELECT id, amount, note FROM t_num_main ORDER BY amount;

\echo '--- UPDATE: 跨分区 ---'
UPDATE t_num_main SET amount = -1.00 WHERE amount = 99.99;
SELECT 'num' AS t, tableoid::regclass AS p, count(*) FROM t_num_main GROUP BY tableoid ORDER BY p;

\echo '--- DELETE ---'
DELETE FROM t_num_main WHERE amount = 0.00;
SELECT 'num' AS t, tableoid::regclass AS p, count(*) FROM t_num_main GROUP BY tableoid ORDER BY p;

-- ====================================================================
-- float4_enc RANGE 分区完整测试
-- ====================================================================
\echo ''
\echo '#####################################################################'
\echo '# float4_enc RANGE 分区 - 完整增删改查'
\echo '#####################################################################'

DROP TABLE IF EXISTS t_f4_main, t_f4_cold, t_f4_cool, t_f4_warm, t_f4_hot CASCADE;

CREATE TABLE t_f4_main (
    id SERIAL,
    temp float4_enc ENCRYPTED WITH (dek=dek_crud, support_algo='ope'),
    zone CHAR(1),
    PRIMARY KEY (id, temp)
) PARTITION BY RANGE (temp);

CREATE TABLE t_f4_cold PARTITION OF t_f4_main FOR VALUES FROM ('-Infinity') TO (0.0);
CREATE TABLE t_f4_cool PARTITION OF t_f4_main FOR VALUES FROM (0.0) TO (20.0);
CREATE TABLE t_f4_warm PARTITION OF t_f4_main FOR VALUES FROM (20.0) TO (40.0);
CREATE TABLE t_f4_hot PARTITION OF t_f4_main FOR VALUES FROM (40.0) TO ('Infinity');

\echo '--- INSERT ---'
INSERT INTO t_f4_main (temp, zone) VALUES (-10.0, 'C'), (5.0, 'L'), (25.0, 'W'), (50.0, 'H');
INSERT INTO t_f4_main (temp, zone) VALUES (-0.5, 'Z'), (19.9, 'L'), (20.1, 'W'), (99.9, 'H');

\echo '--- SELECT: 分区分布 ---'
SELECT 'f4' AS t, tableoid::regclass AS p, count(*) FROM t_f4_main GROUP BY tableoid ORDER BY p;

\echo '--- SELECT: WHERE等值 ---'
SELECT id, temp, zone FROM t_f4_main WHERE temp = 25.0;

\echo '--- SELECT: ORDER BY ---'
SELECT id, temp, zone FROM t_f4_main ORDER BY temp;

\echo '--- UPDATE: 跨分区 ---'
UPDATE t_f4_main SET temp = 0.5 WHERE temp = -0.5;
SELECT 'f4' AS t, tableoid::regclass AS p, count(*) FROM t_f4_main GROUP BY tableoid ORDER BY p;

\echo '--- DELETE ---'
DELETE FROM t_f4_main WHERE temp = 99.9;
SELECT 'f4' AS t, tableoid::regclass AS p, count(*) FROM t_f4_main GROUP BY tableoid ORDER BY p;

-- ====================================================================
-- float8_enc RANGE 分区完整测试
-- ====================================================================
\echo ''
\echo '#####################################################################'
\echo '# float8_enc RANGE 分区 - 完整增删改查'
\echo '#####################################################################'

DROP TABLE IF EXISTS t_f8_main, t_f8_neg, t_f8_zero, t_f8_pos, t_f8_big CASCADE;

CREATE TABLE t_f8_main (
    id SERIAL,
    val float8_enc ENCRYPTED WITH (dek=dek_crud, support_algo='ope'),
    note CHAR(1),
    PRIMARY KEY (id, val)
) PARTITION BY RANGE (val);

CREATE TABLE t_f8_neg PARTITION OF t_f8_main FOR VALUES FROM ('-Infinity') TO (0.0);
CREATE TABLE t_f8_zero PARTITION OF t_f8_main FOR VALUES FROM (0.0) TO (1.0);
CREATE TABLE t_f8_pos PARTITION OF t_f8_main FOR VALUES FROM (1.0) TO (100.0);
CREATE TABLE t_f8_big PARTITION OF t_f8_main FOR VALUES FROM (100.0) TO ('Infinity');

\echo '--- INSERT ---'
INSERT INTO t_f8_main (val, note) VALUES (-1e10, 'A'), (-0.001, 'B'), (0.5, 'C'), (50.0, 'D'), (1e15, 'E');

\echo '--- SELECT: 分区分布 ---'
SELECT 'f8' AS t, tableoid::regclass AS p, count(*) FROM t_f8_main GROUP BY tableoid ORDER BY p;

\echo '--- SELECT: 大数匹配 ---'
SELECT id, val, note FROM t_f8_main WHERE val = 0.5;

\echo '--- SELECT: ORDER BY ---'
SELECT id, val, note FROM t_f8_main ORDER BY val;

\echo '--- UPDATE: 跨分区 ---'
UPDATE t_f8_main SET val = 0.8 WHERE val = -0.001;
SELECT 'f8' AS t, tableoid::regclass AS p, count(*) FROM t_f8_main GROUP BY tableoid ORDER BY p;

\echo '--- DELETE ---'
DELETE FROM t_f8_main WHERE val = 1e15;
SELECT id, val, note FROM t_f8_main ORDER BY val;

-- ====================================================================
-- CTE/WITH 子查询插入测试
-- ====================================================================
\echo ''
\echo '#####################################################################'
\echo '# CTE/WITH + 子查询 插入测试'
\echo '#####################################################################'

\echo '--- WITH + INSERT INTO 分区表 ---'
WITH src AS (
    SELECT 'cte_val_A' AS v, 'A' AS t UNION ALL
    SELECT 'cte_val_X', 'X' UNION ALL
    SELECT 'cte_val_M', 'M'
)
INSERT INTO t_text_main (payload, tag) SELECT v, t FROM src;
SELECT 'text_cte' AS t, tableoid::regclass AS p, count(*) FROM t_text_main GROUP BY tableoid ORDER BY p;

\echo '--- CTE: 从分区表导出 ---'
WITH t AS (
    SELECT payload, tag FROM t_text_main WHERE tag IN ('A', 'X')
)
SELECT payload, tag FROM t ORDER BY tag;

-- ====================================================================
-- 清理
-- ====================================================================
\echo ''
\echo '#####################################################################'
\echo '# 清理'
\echo '#####################################################################'

DROP TABLE IF EXISTS t_text_main, t_text_a, t_text_b, t_text_other CASCADE;
DROP TABLE IF EXISTS t_int2_main, t_int2_neg, t_int2_pos, t_int2_big CASCADE;
DROP TABLE IF EXISTS t_num_main, t_num_neg, t_num_small, t_num_med, t_num_large CASCADE;
DROP TABLE IF EXISTS t_f4_main, t_f4_cold, t_f4_cool, t_f4_warm, t_f4_hot CASCADE;
DROP TABLE IF EXISTS t_f8_main, t_f8_neg, t_f8_zero, t_f8_pos, t_f8_big CASCADE;
DROP DEK IF EXISTS dek_crud;
DROP KEK IF EXISTS kek_crud;

\echo '测试完成'
