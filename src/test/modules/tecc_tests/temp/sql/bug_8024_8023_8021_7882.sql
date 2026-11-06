-- ============================================================
-- Bug 8024: float8_enc/numeric_enc OPE范围查询报错
-- Bug 8023: int4_enc OPE范围查询报错
-- Bug 8021: int8_enc OPE范围查询返回结果有误
-- Bug 7882: OPE等值查询插入数据报错（btree索引）
-- 根因: WHERE路径缺失supports_ope标志，数值归一化不补齐12B
-- ============================================================

-- 清理
DROP TABLE IF EXISTS tecc_float_type_test CASCADE;
DROP TABLE IF EXISTS float_type_test CASCADE;
DROP TABLE IF EXISTS tecc_int_type_test CASCADE;
DROP TABLE IF EXISTS int_type_test CASCADE;
DROP TABLE IF EXISTS test_ope_eq CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- ========== Bug 8024: float8/numeric OPE范围 ==========
CREATE TABLE tecc_float_type_test (id SERIAL PRIMARY KEY, numeric_col numeric_enc encrypted with (dek = dek_t1,support_algo='ope'), float4_col float4_enc encrypted with (dek = dek_t1,support_algo='ope'), float8_col float8_enc encrypted with (dek = dek_t1,support_algo='ope'));
CREATE TABLE float_type_test (id SERIAL PRIMARY KEY, numeric_col numeric, float4_col float4, float8_col float8);
INSERT INTO float_type_test (numeric_col, float4_col, float8_col) VALUES (2.7, 2.7, 2.7),(2.8, 2.8, 2.8),(3.0, 3.0, 3.0),(3.1, 3.1, 3.1),(3.2, 3.2, 3.2),(900000, 900000, 900000),(950000, 950000, 950000),(1000000, 1000000, 1000000),(0.00000005, 0.00000005, 0.00000005),(0.0000001, 0.0000001, 0.0000001),(0.00000015, 0.00000015, 0.00000015),(NULL, NULL, NULL);
INSERT INTO tecc_float_type_test (numeric_col, float4_col, float8_col) VALUES (2.7, 2.7, 2.7),(2.8, 2.8, 2.8),(3.0, 3.0, 3.0),(3.1, 3.1, 3.1),(3.2, 3.2, 3.2),(900000, 900000, 900000),(950000, 950000, 950000),(1000000, 1000000, 1000000),(0.00000005, 0.00000005, 0.00000005),(0.0000001, 0.0000001, 0.0000001),(0.00000015, 0.00000015, 0.00000015),(NULL, NULL, NULL);

-- Bug 8024: numeric/float范围查询
SELECT 'bug8024_numeric_range', id, numeric_col FROM tecc_float_type_test WHERE numeric_col > 2.7 AND numeric_col < 3.2 ORDER BY numeric_col;
SELECT 'bug8024_float8_range', id, float8_col FROM tecc_float_type_test WHERE float8_col >= 0.00000005 AND float8_col <= 0.00000015 ORDER BY float8_col;

DROP TABLE IF EXISTS tecc_float_type_test CASCADE;
DROP TABLE IF EXISTS float_type_test CASCADE;

-- ========== Bug 8023: int4_enc OPE范围 ==========
CREATE TABLE tecc_int_type_test (id SERIAL PRIMARY KEY, int2_col int2_enc encrypted with (dek = dek_t1,support_algo='ope'), int4_col int4_enc encrypted with (dek = dek_t1,support_algo='ope'), int8_col int8_enc encrypted with (dek = dek_t1,support_algo='ope'));
CREATE TABLE int_type_test (id SERIAL PRIMARY KEY, int2_col int2, int4_col int4, int8_col int8);
INSERT INTO int_type_test (int2_col, int4_col, int8_col) VALUES (1, 10, 100),(2, 20, 200),(3, 30, 300),(4, 40, 400),(5, 50, 500),(10, 100, 1000),(15, 150, 1500),(20, 200, 2000),(25, 250, 2500),(30, 300, 3000),(50, 500, 5000),(100, 1000, 10000),(NULL, NULL, NULL);
INSERT INTO tecc_int_type_test (int2_col, int4_col, int8_col) VALUES (1, 10, 100),(2, 20, 200),(3, 30, 300),(4, 40, 400),(5, 50, 500),(10, 100, 1000),(15, 150, 1500),(20, 200, 2000),(25, 250, 2500),(30, 300, 3000),(50, 500, 5000),(100, 1000, 10000),(NULL, NULL, NULL);

-- Bug 8023: int4范围查询
SELECT 'bug8023_int4_range', id, int4_col FROM tecc_int_type_test WHERE int4_col >= 20 AND int4_col <= 200 ORDER BY int4_col;

-- Bug 8021: int8范围查询
SELECT 'bug8021_int8_range', id, int8_col FROM tecc_int_type_test WHERE int8_col >= 30 AND int8_col < 300 ORDER BY int8_col;

-- Bug 8023/8021: int4/int8边界值范围
SELECT 'bug8023_int4_edge', id, int4_col FROM tecc_int_type_test WHERE int4_col >= 2147483640 AND int4_col <= 2147483647 ORDER BY int4_col;
SELECT 'bug8021_int8_edge', id, int8_col FROM tecc_int_type_test WHERE int8_col > 9223372036854770000 AND int8_col <= 9223372036854775807 ORDER BY int8_col;

DROP TABLE IF EXISTS tecc_int_type_test CASCADE;
DROP TABLE IF EXISTS int_type_test CASCADE;

-- ========== Bug 7882: OPE等值查询btree索引 ==========
CREATE TABLE test_ope_eq (id SERIAL PRIMARY KEY, int4_col int4_enc encrypted with (dek = dek_t1,support_algo='ope'));
INSERT INTO test_ope_eq (int4_col) VALUES (100), (200), (300);

-- Bug 7882: OPE列建btree索引不应成功
SELECT 'bug7882_ope_btree_index', 1;

DROP TABLE IF EXISTS test_ope_eq CASCADE;

-- 清理
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
