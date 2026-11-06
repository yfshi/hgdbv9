-- 清理
DROP TABLE IF EXISTS tecc_int_type_test CASCADE;
DROP TABLE IF EXISTS int_type_test CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

CREATE TABLE tecc_int_type_test (id SERIAL PRIMARY KEY, int2_col int2_enc encrypted with (dek = dek_t1,support_algo='ope'), int4_col int4_enc encrypted with (dek = dek_t1,support_algo='ope'), int8_col int8_enc encrypted with (dek = dek_t1,support_algo='ope'));
CREATE TABLE int_type_test (id SERIAL PRIMARY KEY, int2_col int2, int4_col int4, int8_col int8);

INSERT INTO int_type_test (int2_col, int4_col, int8_col) VALUES (1, 10, 100),(2, 20, 200),(3, 30, 300),(4, 40, 400),(5, 50, 500),(10, 100, 1000),(15, 150, 1500),(20, 200, 2000),(25, 250, 2500),(30, 300, 3000),(50, 500, 5000),(100, 1000, 10000),(NULL, NULL, NULL);
INSERT INTO tecc_int_type_test (int2_col, int4_col, int8_col) VALUES (1, 10, 100),(2, 20, 200),(3, 30, 300),(4, 40, 400),(5, 50, 500),(10, 100, 1000),(15, 150, 1500),(20, 200, 2000),(25, 250, 2500),(30, 300, 3000),(50, 500, 5000),(100, 1000, 10000),(NULL, NULL, NULL);

-- 测试步骤
SELECT id, int2_col, int4_col, int8_col FROM int_type_test WHERE int2_col > 10 AND int2_col < 100 ORDER BY int2_col;
SELECT id, int2_col, int4_col, int8_col FROM tecc_int_type_test WHERE int2_col > 10 AND int2_col < 100 ORDER BY int2_col;
SELECT id, int2_col, int4_col, int8_col FROM int_type_test WHERE int4_col >= 20 AND int4_col <= 200 ORDER BY int4_col;
SELECT id, int2_col, int4_col, int8_col FROM tecc_int_type_test WHERE int4_col >= 20 AND int4_col <= 200 ORDER BY int4_col;
SELECT id, int2_col, int4_col, int8_col FROM int_type_test WHERE int8_col >= 30 AND int8_col < 300 ORDER BY int8_col;
SELECT id, int2_col, int4_col, int8_col FROM tecc_int_type_test WHERE int8_col >= 30 AND int8_col < 300 ORDER BY int8_col;
SELECT id, int2_col, int4_col, int8_col FROM int_type_test WHERE int2_col > 30 ORDER BY int2_col;
SELECT id, int2_col, int4_col, int8_col FROM tecc_int_type_test WHERE int2_col > 30 ORDER BY int2_col;
SELECT id, int2_col, int4_col, int8_col FROM int_type_test WHERE int4_col >= 40 ORDER BY int4_col;
SELECT id, int2_col, int4_col, int8_col FROM tecc_int_type_test WHERE int4_col >= 40 ORDER BY int4_col;
SELECT id, int2_col, int4_col, int8_col FROM int_type_test WHERE int8_col < 30 ORDER BY int8_col;
SELECT id, int2_col, int4_col, int8_col FROM tecc_int_type_test WHERE int8_col < 30 ORDER BY int8_col;
SELECT id, int2_col, int4_col, int8_col FROM int_type_test WHERE int2_col <= 20 ORDER BY int2_col;
SELECT id, int2_col, int4_col, int8_col FROM tecc_int_type_test WHERE int2_col <= 20 ORDER BY int2_col;

-- 清理
DROP TABLE IF EXISTS tecc_int_type_test CASCADE;
DROP TABLE IF EXISTS int_type_test CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
