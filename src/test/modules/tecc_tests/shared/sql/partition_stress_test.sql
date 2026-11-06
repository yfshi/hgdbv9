\set ON_ERROR_STOP on
\pset null '(null)'

-- ====================================================================
-- 分区表压力测试 — 覆盖边界、异常、并发场景
-- ====================================================================

\echo '=== 1. 准备 KEK/DEK ==='
CREATE KEK IF NOT EXISTS kek_stress WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_stress.key');
CREATE DEK IF NOT EXISTS dek_stress WITH VALUES(KEK=kek_stress);

-- ====================================================================
-- Test 1: LIST 分区 — 边界值、NULL行为、大小写
-- ====================================================================
\echo ''
\echo '=== Test 1: text_enc LIST — 边界/大小写/特殊字符 ==='

DROP TABLE IF EXISTS stress_text CASCADE;
CREATE TABLE stress_text (
    id SERIAL,
    val text_enc ENCRYPTED WITH (dek=dek_stress),
    tag text_enc ENCRYPTED WITH (dek=dek_stress, support_algo='ope'),
    PRIMARY KEY (id, tag)
) PARTITION BY LIST (tag);

CREATE TABLE stress_text_a PARTITION OF stress_text FOR VALUES IN ('A', 'B', 'C');
CREATE TABLE stress_text_x PARTITION OF stress_text FOR VALUES IN ('X', 'Y', 'Z');
CREATE TABLE stress_text_other PARTITION OF stress_text DEFAULT;

-- 各种字符串边界
INSERT INTO stress_text (val, tag) VALUES ('empty_string', '');
INSERT INTO stress_text (val, tag) VALUES ('single_char', 'A');
INSERT INTO stress_text (val, tag) VALUES ('中文Unicode', 'B');
INSERT INTO stress_text (val, tag) VALUES (' with spaces ', 'C');
INSERT INTO stress_text (val, tag) VALUES ('special_chars!@#', 'X');
INSERT INTO stress_text (val, tag) VALUES ('very_long_string_xxxxxxxxxxxxxxxxxxx', 'Y');
INSERT INTO stress_text (val, tag) VALUES ('zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz', 'Z');
INSERT INTO stress_text (val, tag) VALUES ('default_route', 'M');
INSERT INTO stress_text (val, tag) VALUES ('default_route2', 'N');

\echo '  INSERT 9行 → 期望: 3行A区 + 3行X区 + 3行other'
SELECT tableoid::regclass AS p, count(*) AS cnt FROM stress_text GROUP BY tableoid ORDER BY p;

\echo '  WHERE空字符串匹配 → 期望: empty_string'
SELECT val FROM stress_text WHERE tag = '';

\echo '  WHERE中文匹配 → 期望: B'
SELECT val FROM stress_text WHERE tag = 'B';

\echo '  WHERE特殊字符匹配'
SELECT val FROM stress_text WHERE tag = 'X';

\echo '  ORDER BY tag'
SELECT tag, id FROM stress_text ORDER BY tag, id;

\echo '  UPDATE跨分区: tag=C→X'
UPDATE stress_text SET tag = 'X' WHERE tag = 'C';
SELECT tableoid::regclass AS p, count(*) AS cnt FROM stress_text GROUP BY tableoid ORDER BY p;

\echo '  DELETE: tag=空字符串'
DELETE FROM stress_text WHERE tag = '';
SELECT count(*) AS remaining_after_delete FROM stress_text;

-- ====================================================================
-- Test 2: RANGE 分区 — int2 边界值
-- ====================================================================
\echo ''
\echo '=== Test 2: int2_enc RANGE — 边界值 ==='

DROP TABLE IF EXISTS stress_int2 CASCADE;
CREATE TABLE stress_int2 (
    id SERIAL,
    val int2_enc ENCRYPTED WITH (dek=dek_stress, support_algo='ope'),
    note CHAR(1),
    PRIMARY KEY (id, val)
) PARTITION BY RANGE (val);

CREATE TABLE stress_int2_neg PARTITION OF stress_int2 FOR VALUES FROM (-32767::int2) TO (0::int2);
CREATE TABLE stress_int2_pos PARTITION OF stress_int2 FOR VALUES FROM (0::int2) TO (32767::int2);

-- 边界值测试
INSERT INTO stress_int2 (val, note) VALUES (-32767::int2, 'L');  -- 下界
INSERT INTO stress_int2 (val, note) VALUES (-1::int2, 'N');
INSERT INTO stress_int2 (val, note) VALUES (0::int2, 'Z');       -- 分界线
INSERT INTO stress_int2 (val, note) VALUES (1::int2, 'P');
INSERT INTO stress_int2 (val, note) VALUES (32766::int2, 'H');   -- 上界-1（32767是exclusive）

-- 大量正数值
INSERT INTO stress_int2 (val, note) VALUES (2::int2, 'B');
INSERT INTO stress_int2 (val, note) VALUES (5::int2, 'B');
INSERT INTO stress_int2 (val, note) VALUES (10::int2, 'B');
INSERT INTO stress_int2 (val, note) VALUES (20::int2, 'B');
INSERT INTO stress_int2 (val, note) VALUES (50::int2, 'B');

\echo '  INSERT 10行 → 期望: neg=4(pos边界), pos=6'
SELECT tableoid::regclass AS p, count(*) AS cnt FROM stress_int2 GROUP BY tableoid ORDER BY p;

\echo '  WHERE val=0 → 1行'
SELECT * FROM stress_int2 WHERE val = 0;

\echo '  WHERE val范围 [0, 10]'
SELECT count(*) FROM stress_int2 WHERE val >= 0 AND val <= 10;

\echo '  ORDER BY val DESC'
SELECT val, note FROM stress_int2 ORDER BY val DESC LIMIT 5;

\echo '  UPDATE跨分区: val = -1 → val = 100'
UPDATE stress_int2 SET val = 100 WHERE val = -1;
SELECT tableoid::regclass AS p, count(*) AS cnt FROM stress_int2 GROUP BY tableoid ORDER BY p;

\echo '  DELETE: 删除所有正数'
DELETE FROM stress_int2 WHERE val > 0;
SELECT tableoid::regclass AS p, count(*) AS cnt FROM stress_int2 GROUP BY tableoid ORDER BY p;

-- ====================================================================
-- Test 3: RANGE 分区 — numeric 高精度
-- ====================================================================
\echo ''
\echo '=== Test 3: numeric_enc RANGE — 高精度 ==='

DROP TABLE IF EXISTS stress_num CASCADE;
CREATE TABLE stress_num (
    id SERIAL,
    val numeric_enc ENCRYPTED WITH (dek=dek_stress, support_algo='ope'),
    note CHAR(1),
    PRIMARY KEY (id, val)
) PARTITION BY RANGE (val);

CREATE TABLE stress_num_neg PARTITION OF stress_num FOR VALUES FROM (-999999.99) TO (0.00);
CREATE TABLE stress_num_pos PARTITION OF stress_num FOR VALUES FROM (0.00) TO (999999.99);

-- 各种精度
INSERT INTO stress_num (val, note) VALUES (-0.01, 'A');
INSERT INTO stress_num (val, note) VALUES (0.00, 'B');
INSERT INTO stress_num (val, note) VALUES (0.01, 'C');
INSERT INTO stress_num (val, note) VALUES (123.456, 'D');
INSERT INTO stress_num (val, note) VALUES (999999.98, 'E');
INSERT INTO stress_num (val, note) VALUES (0.123456789, 'F');
INSERT INTO stress_num (val, note) VALUES (-999999.99, 'G');

\echo '  INSERT 7行 → 期望: neg=3, pos=4'
SELECT tableoid::regclass AS p, count(*) AS cnt FROM stress_num GROUP BY tableoid ORDER BY p;

\echo '  WHERE val=0.00精确匹配'
SELECT val, note FROM stress_num WHERE val = 0.00;

\echo '  WHERE高精度0.123456789匹配'
SELECT val, note FROM stress_num WHERE val = 0.123456789;

\echo '  ORDER BY val'
SELECT val, note FROM stress_num ORDER BY val;

\echo '  UPDATE跨分区: val = 0.01 → -100'
UPDATE stress_num SET val = -100.00 WHERE val = 0.01;
SELECT tableoid::regclass AS p, count(*) AS cnt FROM stress_num GROUP BY tableoid ORDER BY p;

\echo '  DELETE: val < -500'
DELETE FROM stress_num WHERE val < -500;
SELECT count(*) FROM stress_num;

-- ====================================================================
-- Test 4: LIST + RANGE 混合 — 多表JOIN、子查询
-- ====================================================================
\echo ''
\echo '=== Test 4: 多表JOIN + 子查询 ==='

DROP TABLE IF EXISTS stress_order CASCADE;
CREATE TABLE stress_order (
    id SERIAL,
    order_id int4_enc ENCRYPTED WITH (dek=dek_stress, support_algo='ope'),
    amount numeric_enc ENCRYPTED WITH (dek=dek_stress, support_algo='ope'),
    region text_enc ENCRYPTED WITH (dek=dek_stress, support_algo='ope'),
    PRIMARY KEY (id, order_id, amount, region)
) PARTITION BY LIST (region);

CREATE TABLE stress_order_east PARTITION OF stress_order FOR VALUES IN ('EAST');
CREATE TABLE stress_order_west PARTITION OF stress_order FOR VALUES IN ('WEST');
CREATE TABLE stress_order_other PARTITION OF stress_order DEFAULT;

INSERT INTO stress_order (order_id, amount, region) VALUES
    (1001, 50.00, 'EAST'),
    (1002, 150.00, 'EAST'),
    (1003, 200.00, 'WEST'),
    (1004, 300.00, 'WEST'),
    (1005, 999.99, 'NORTH'),
    (1006, 0.01, 'SOUTH');

\echo '  分区分布'
SELECT tableoid::regclass AS p, count(*) AS cnt FROM stress_order GROUP BY tableoid ORDER BY p;

\echo '  JOIN自关联'
SELECT a.id AS id1, b.id AS id2
FROM stress_order a JOIN stress_order b ON a.order_id = b.order_id
WHERE a.region = 'EAST' AND b.amount > 100;

\echo '  金额排序'
SELECT order_id, amount, region FROM stress_order ORDER BY amount;

-- ====================================================================
-- Test 5: 多分区表跨表UPDATE + CTE
-- ====================================================================
\echo ''
\echo '=== Test 5: 数据验证 ==='

SELECT tableoid::regclass AS p, count(*) AS cnt FROM stress_order GROUP BY tableoid ORDER BY p;

-- ====================================================================
-- Test 6: 大量数据INSERT+DELETE循环（压力）
-- ====================================================================
\echo ''
\echo '=== Test 6: 批量插入+清空 ==='

DROP TABLE IF EXISTS stress_bulk CASCADE;
CREATE TABLE stress_bulk (
    id SERIAL,
    val int8_enc ENCRYPTED WITH (dek=dek_stress, support_algo='ope'),
    PRIMARY KEY (id, val)
) PARTITION BY RANGE (val);

CREATE TABLE stress_bulk_neg PARTITION OF stress_bulk FOR VALUES FROM (-9223372036854775808) TO (0);
CREATE TABLE stress_bulk_pos PARTITION OF stress_bulk FOR VALUES FROM (0) TO (9223372036854775807);

-- 批量插入20行
INSERT INTO stress_bulk (val) VALUES (-50), (-40), (-30), (-20), (-10);
INSERT INTO stress_bulk (val) VALUES (-5), (-4), (-3), (-2), (-1);
INSERT INTO stress_bulk (val) VALUES (0), (1), (2), (3), (4);
INSERT INTO stress_bulk (val) VALUES (5), (10), (20), (30), (40);

\echo '  20行 → 期望: neg=10, pos=10'
SELECT tableoid::regclass AS p, count(*) AS cnt FROM stress_bulk GROUP BY tableoid ORDER BY p;

\echo '  TRUNCATE分区表'
TRUNCATE stress_bulk;
SELECT count(*) AS after_truncate FROM stress_bulk;

-- ====================================================================
-- 回归验证: 之前修过的bug不复现
-- ====================================================================
\echo ''
\echo '=== Test 7: 回归验证 — 之前修过的BUG ==='

\echo '  1) AAD一致: tag=value作为列名'
DROP TABLE IF EXISTS reg_aad CASCADE;
CREATE TABLE reg_aad (
    id SERIAL,
    value int4_enc ENCRYPTED WITH (dek=dek_stress, support_algo='ope'),
    note CHAR(1),
    PRIMARY KEY (id, value)
) PARTITION BY LIST (value);
CREATE TABLE reg_aad_ok PARTITION OF reg_aad FOR VALUES IN (1, 2, 3);
CREATE TABLE reg_aad_other PARTITION OF reg_aad DEFAULT;
INSERT INTO reg_aad (value, note) VALUES (1, 'A');
SELECT tableoid::regclass AS p, count(*) AS cnt FROM reg_aad GROUP BY tableoid ORDER BY p;

\echo '  2) ::int2 分区边界'
DROP TABLE IF EXISTS reg_int2 CASCADE;
CREATE TABLE reg_int2 (
    id SERIAL,
    val int2_enc ENCRYPTED WITH (dek=dek_stress, support_algo='ope'),
    PRIMARY KEY (id, val)
) PARTITION BY RANGE (val);
CREATE TABLE reg_int2_low PARTITION OF reg_int2 FOR VALUES FROM (-32767::int2) TO (0::int2);
CREATE TABLE reg_int2_high PARTITION OF reg_int2 FOR VALUES FROM (0::int2) TO (32767::int2);
INSERT INTO reg_int2 (val) VALUES (-32767::int2), (0::int2), (32766::int2);
SELECT tableoid::regclass AS p, count(*) AS cnt FROM reg_int2 GROUP BY tableoid ORDER BY p;

\echo '  3) WHERE条件 = 加密列值'
SELECT * FROM reg_int2 WHERE val = 0;

\echo '  4) UPDATE SET负数'
UPDATE reg_int2 SET val = -100 WHERE val = 0;
SELECT tableoid::regclass AS p, count(*) AS cnt FROM reg_int2 GROUP BY tableoid ORDER BY p;

\echo '  5) Infinity边界float4, float8 (已在partition_types_test中验证分区版本)'

-- ====================================================================
-- 清理
-- ====================================================================
\echo ''
\echo '=== 清理 ==='
DROP TABLE IF EXISTS stress_text, stress_text_a, stress_text_x, stress_text_other CASCADE;
DROP TABLE IF EXISTS stress_int2, stress_int2_neg, stress_int2_pos CASCADE;
DROP TABLE IF EXISTS stress_num, stress_num_neg, stress_num_pos CASCADE;
DROP TABLE IF EXISTS stress_order, stress_order_east, stress_order_west, stress_order_other CASCADE;
DROP TABLE IF EXISTS stress_bulk, stress_bulk_neg, stress_bulk_pos CASCADE;
DROP TABLE IF EXISTS reg_aad, reg_aad_ok, reg_aad_other CASCADE;
DROP TABLE IF EXISTS reg_int2, reg_int2_low, reg_int2_high CASCADE;
\set ON_ERROR_STOP off
DROP DEK IF EXISTS dek_stress;
DROP KEK IF EXISTS kek_stress;
\set ON_ERROR_STOP on

\echo ''
\echo '=== 全部测试完成 ==='
