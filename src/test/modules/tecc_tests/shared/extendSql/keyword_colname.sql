\set ON_ERROR_STOP on
\pset null '(null)'

-- ====================================================================
-- 关键字列名加密测试 (keyword_colname)
-- 覆盖 TECC scanner 关键字作为加密列名的 INSERT/UPDATE/SELECT/DELETE
-- ReqID:TECC4.0
-- ====================================================================

CREATE KEK IF NOT EXISTS kek_colname WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_colname.key');

CREATE DEK IF NOT EXISTS dek_colname WITH VALUES(KEK=kek_colname);

-- ====================================================================
-- 1. 使用 TECC scanner 关键字作为列名的加密表
--    temp: TEMP 关键字
--    zone: ZONE_P 关键字
--    type_name: TYPE 关键字
--    order_no: ORDER 关键字
--    key_val: KEY 关键字
-- ====================================================================
\echo '=== Test 1: 创建含关键字列名的加密表 ==='
DROP TABLE IF EXISTS t_kw CASCADE;

CREATE TABLE t_kw (
    id SERIAL,
    temp float4_enc ENCRYPTED WITH (dek=dek_colname, support_algo='ope'),
    zone text_enc ENCRYPTED WITH (dek=dek_colname),
    type_name int4_enc ENCRYPTED WITH (dek=dek_colname, support_algo='det'),
    order_no int4_enc ENCRYPTED WITH (dek=dek_colname, support_algo='ope'),
    key_val text_enc ENCRYPTED WITH (dek=dek_colname),
    note CHAR(2),
    PRIMARY KEY (id, temp)
);

\echo '=== Test 2: INSERT 带显式列列表 ==='
INSERT INTO t_kw (temp, zone, type_name, order_no, key_val, note) VALUES (10.0, 'A区', 100, 1, 'k1', 'a1');

-- @param BigDecimal 20.5
-- @param String B区
-- @param int 200
-- @param int 2
-- @param String k2
-- @param String b1
INSERT INTO t_kw (temp, zone, type_name, order_no, key_val, note) VALUES (?, ?, ?, ?, ?, ?);

-- @param BigDecimal 30.0
-- @param String C区
-- @param int 300
-- @param int 3
-- @param String k3
-- @param String c1
INSERT INTO t_kw (temp, zone, type_name, order_no, key_val, note) VALUES (?, ?, ?, ?, ?, ?);

\echo '=== Test 3: INSERT 不带列列表 (位置映射) ==='
INSERT INTO t_kw VALUES (DEFAULT, 40.0, 'D区', 400, 4, 'k4', 'd1');

\echo '=== Test 4: SELECT: WHERE 关键字列等值 ==='
SELECT id, temp, zone, type_name, order_no, key_val, note FROM t_kw WHERE temp = 20.5;

\echo '=== Test 5: SELECT: WHERE 关键字列范围 (OPE) ==='
SELECT id, temp, order_no FROM t_kw WHERE temp >= 10.0 AND temp <= 30.0 ORDER BY temp;

\echo '=== Test 6: SELECT: ORDER BY 关键字列 (OPE) ==='
SELECT id, order_no, key_val FROM t_kw ORDER BY order_no DESC;

\echo '=== Test 7: UPDATE SET 关键字列 ==='
UPDATE t_kw SET zone = 'Z区' WHERE temp = 20.5;

-- @param BigDecimal 20.5
SELECT id, temp, zone FROM t_kw WHERE temp = ?;

\echo '=== Test 8: DELETE WHERE 关键字列条件 ==='
DELETE FROM t_kw WHERE temp = 40.0;

SELECT count(*) FROM t_kw;

\echo '=== Test 9: 验证剩余数据 ==='
SELECT id, temp, zone, type_name, order_no, key_val, note FROM t_kw ORDER BY temp;

\echo '=== 清理 ==='
DROP TABLE IF EXISTS t_kw CASCADE;

DROP DEK IF EXISTS dek_colname;

DROP KEK IF EXISTS kek_colname;

\echo '关键字列名测试完成';
