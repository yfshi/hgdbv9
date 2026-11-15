--
-- cte_column_mapping.sql — libpq专有测试：CTE / 派生表的加密列映射传播
--
-- 两处缺口曾让 CTE / 派生表里的密态常量不被加密（明文外发，服务端报
-- invalid hex string）：
--   1) 子语句向父语句传播加密列时只按 (表,列) 判重、不带别名——同一张基表
--      被两条 CTE 各自引用时，后一条 CTE 的记录被当成重复丢弃，外层
--      `b.col` 解析不到，于是多 CTE 时只有第一个 CTE 生效；
--   2) CTE 的 FROM 引用上游 CTE 时，按 CTE 名查表元数据必然落空，上游的
--      列映射在此断裂，链式 CTE 整体失效（单跳 CTE 与派生表一直正常）。
--

DROP TABLE IF EXISTS test_cte_map CASCADE;
DROP DEK IF EXISTS dek_cte_map CASCADE;
DROP KEK IF EXISTS kek_cte_map CASCADE;

CREATE KEK kek_cte_map WITH (key_store = 'localkms', key_path = '/tmp/kek_cte_map.key');
CREATE DEK dek_cte_map WITH VALUES(KEK = 'kek_cte_map');

CREATE TABLE test_cte_map (
    id       INT4,
    val_det  TEXT_ENC ENCRYPTED WITH (DEK = dek_cte_map, SUPPORT_ALGO = 'DET'),
    val_ope  INT4_ENC ENCRYPTED WITH (DEK = dek_cte_map, SUPPORT_ALGO = 'OPE')
);
INSERT INTO test_cte_map VALUES (1, 'alpha', 10), (2, 'beta', 20), (3, 'gamma', 30);

SELECT '1_baseline', id, val_det, val_ope FROM test_cte_map ORDER BY id;

-- ============================================================
-- 2. 单跳 CTE（对照：本来就正常）
-- ============================================================
WITH a AS (SELECT id, val_det FROM test_cte_map) SELECT '2_single_cte', id FROM a WHERE a.val_det = 'alpha';
WITH a AS (SELECT * FROM test_cte_map) SELECT '3_single_cte_star', id FROM a WHERE a.val_det = 'beta';

-- ============================================================
-- 3. 两个 CTE 各自引用基表：引用第二个也必须生效
-- ============================================================
WITH a AS (SELECT id, val_det FROM test_cte_map), b AS (SELECT id, val_det FROM test_cte_map)
SELECT '4_second_of_two_cte', id FROM b WHERE b.val_det = 'alpha';
WITH a AS (SELECT id, val_det FROM test_cte_map), b AS (SELECT id, val_det FROM test_cte_map)
SELECT '5_first_of_two_cte', id FROM a WHERE a.val_det = 'gamma';

-- ============================================================
-- 4. 链式 CTE：CTE 的 FROM 引用上游 CTE
-- ============================================================
WITH a AS (SELECT id, val_det FROM test_cte_map), b AS (SELECT id, val_det FROM a)
SELECT '6_two_level_chain', id FROM b WHERE b.val_det = 'alpha';

WITH a AS (SELECT id, val_det FROM test_cte_map), b AS (SELECT id, val_det FROM a),
     c AS (SELECT id, val_det FROM b)
SELECT '7_three_level_chain', id FROM c WHERE c.val_det = 'beta';

WITH a AS (SELECT id, val_det FROM test_cte_map), b AS (SELECT id, val_det FROM a),
     c AS (SELECT id, val_det FROM b), d AS (SELECT id, val_det FROM c)
SELECT '8_four_level_chain', id FROM d WHERE d.val_det = 'gamma';

WITH a AS (SELECT id, val_det FROM test_cte_map), b AS (SELECT id, val_det FROM a),
     c AS (SELECT id, val_det FROM b)
SELECT '9_reference_middle', id FROM b WHERE b.val_det = 'alpha';

WITH a AS (SELECT * FROM test_cte_map), b AS (SELECT * FROM a)
SELECT '10_chain_star', id FROM b WHERE b.val_det = 'beta';

WITH a AS (SELECT id, val_ope FROM test_cte_map), b AS (SELECT id, val_ope FROM a)
SELECT '11_chain_ope', id FROM b WHERE b.val_ope = 20;

WITH a AS (SELECT id, val_det FROM test_cte_map), b AS (SELECT id, val_det FROM a)
SELECT '12_chain_no_match', id FROM b WHERE b.val_det = 'no_such_value';

-- ============================================================
-- 5. 派生表（对照：本来就正常）
-- ============================================================
SELECT '13_derived', id FROM (SELECT id, val_det FROM test_cte_map) s WHERE s.val_det = 'alpha';
SELECT '14_nested_derived', id FROM (SELECT id, val_det FROM (SELECT id, val_det FROM test_cte_map) x) y WHERE y.val_det = 'beta';

DROP TABLE IF EXISTS test_cte_map CASCADE;
DROP DEK IF EXISTS dek_cte_map CASCADE;
DROP KEK IF EXISTS kek_cte_map CASCADE;
