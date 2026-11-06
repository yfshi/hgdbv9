-- 清理
DROP TABLE IF EXISTS float_type_test CASCADE;
DROP TABLE IF EXISTS tecc_float_type_test CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

CREATE TABLE tecc_float_type_test (id SERIAL PRIMARY KEY,num_col numeric_enc encrypted with (dek = dek_t1,support_algo='det'),f4_col float4_enc encrypted with (dek = dek_t1,support_algo='det'),f8_col float8_enc encrypted with (dek = dek_t1,support_algo='det'));
CREATE TABLE float_type_test (id SERIAL PRIMARY KEY,num_col numeric,f4_col float4,f8_col float8);

INSERT INTO tecc_float_type_test (num_col, f4_col, f8_col) VALUES (1.0, 1.0, 1.0),(0.0, 0.0, 0.0),(-1.0, -1.0, -1.0),(123.456, 123.456, 123.456),(0.1, 0.1, 0.1),(0.3, 0.3, 0.3),(1.0/3.0, 1.0/3.0, 1.0/3.0),(99999.99999, 99999.99999, 99999.99999),(0.00001, 0.00001, 0.00001),(1.0e10, 1.0e10, 1.0e10),(1.0e-10, 1.0e-10, 1.0e-10),(NULL, NULL, NULL),(1.0, NULL, 1.0),(NULL, 2.0, NULL),(1234567890.12345678901234567890, 1234567890.12345678901234567890, 1234567890.12345678901234567890),(0.00000000000000000001, 0.00000000000000000001, 0.00000000000000000001),(123456789012345678901234567890.1234567890, 123456789012345678901234567890.1234567890, 123456789012345678901234567890.1234567890),(1.0, 1.0000000000000001, 1.0),(100.0, 100.0, 100.00000000000001),(9999999999.9999999999, 10000000000.0, 9999999999.9999999999);
INSERT INTO float_type_test (num_col, f4_col, f8_col) VALUES (1.0, 1.0, 1.0),(0.0, 0.0, 0.0),(-1.0, -1.0, -1.0),(123.456, 123.456, 123.456),(0.1, 0.1, 0.1),(0.3, 0.3, 0.3),(1.0/3.0, 1.0/3.0, 1.0/3.0),(99999.99999, 99999.99999, 99999.99999),(0.00001, 0.00001, 0.00001),(1.0e10, 1.0e10, 1.0e10),(1.0e-10, 1.0e-10, 1.0e-10),(NULL, NULL, NULL),(1.0, NULL, 1.0),(NULL, 2.0, NULL),(1234567890.12345678901234567890, 1234567890.12345678901234567890, 1234567890.12345678901234567890),(0.00000000000000000001, 0.00000000000000000001, 0.00000000000000000001),(123456789012345678901234567890.1234567890, 123456789012345678901234567890.1234567890, 123456789012345678901234567890.1234567890),(1.0, 1.0000000000000001, 1.0),(100.0, 100.0, 100.00000000000001),(9999999999.9999999999, 10000000000.0, 9999999999.9999999999);

-- 测试步骤
SELECT num_col = f4_col as num_eq_f4,num_col = f8_col as num_eq_f8,f4_col = f8_col as f4_eq_f8 FROM float_type_test WHERE id = 1;
SELECT num_col = f4_col as num_eq_f4,num_col = f8_col as num_eq_f8,f4_col = f8_col as f4_eq_f8 FROM tecc_float_type_test WHERE id = 1;
SELECT num_col = f4_col as num_eq_f4,num_col = f8_col as num_eq_f8,f4_col = f8_col as f4_eq_f8,num_col as exact_value FROM float_type_test WHERE num_col = 1234567890.12345678901234567890;
SELECT num_col = f4_col as num_eq_f4,num_col = f8_col as num_eq_f8,f4_col = f8_col as f4_eq_f8,num_col as exact_value FROM tecc_float_type_test WHERE num_col = 1234567890.12345678901234567890;
SELECT num_col, f4_col, f8_col,num_col <> f4_col as num_ne_f4,num_col <> f8_col as num_ne_f8,f4_col <> f8_col as f4_ne_f8 FROM float_type_test WHERE num_col IS NOT NULL AND (num_col <> f4_col OR num_col <> f8_col OR f4_col <> f8_col);
SELECT num_col, f4_col, f8_col,num_col <> f4_col as num_ne_f4,num_col <> f8_col as num_ne_f8,f4_col <> f8_col as f4_ne_f8 FROM tecc_float_type_test WHERE num_col IS NOT NULL AND (num_col <> f4_col OR num_col <> f8_col OR f4_col <> f8_col);
SELECT num_col = f4_col as num_eq_f4,num_col = f8_col as num_eq_f8,f4_col = f8_col as f4_eq_f8,num_col = 1.0 as num_eq_1,f4_col = 2.0 as f4_eq_2,f8_col = 3.0 as f8_eq_3 FROM float_type_test WHERE num_col IS NULL OR f4_col IS NULL OR f8_col IS NULL ORDER BY num_col NULLS FIRST, f4_col NULLS FIRST, f8_col NULLS FIRST;
SELECT num_col = f4_col as num_eq_f4,num_col = f8_col as num_eq_f8,f4_col = f8_col as f4_eq_f8,num_col = 1.0 as num_eq_1,f4_col = 2.0 as f4_eq_2,f8_col = 3.0 as f8_eq_3 FROM tecc_float_type_test WHERE num_col IS NULL OR f4_col IS NULL OR f8_col IS NULL ORDER BY num_col NULLS FIRST, f4_col NULLS FIRST, f8_col NULLS FIRST;
SELECT num_col IS NOT DISTINCT FROM f4_col as num_eq_f4,num_col IS NOT DISTINCT FROM f8_col as num_eq_f8,f4_col IS NOT DISTINCT FROM f8_col as f4_eq_f8 FROM float_type_test WHERE num_col IS NULL OR f4_col IS NULL OR f8_col IS NULL;
SELECT num_col IS NOT DISTINCT FROM f4_col as num_eq_f4,num_col IS NOT DISTINCT FROM f8_col as num_eq_f8,f4_col IS NOT DISTINCT FROM f8_col as f4_eq_f8 FROM tecc_float_type_test WHERE num_col IS NULL OR f4_col IS NULL OR f8_col IS NULL;

-- 清理
DROP TABLE IF EXISTS float_type_test CASCADE;
DROP TABLE IF EXISTS tecc_float_type_test CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
