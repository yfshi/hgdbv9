-- 清理
DROP TABLE IF EXISTS test_numeric CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 测试步骤
CREATE TABLE test_numeric (id SERIAL PRIMARY KEY,numeric_col numeric_enc encrypted with (dek = dek_t1,support_algo='ope'),description TEXT,created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);

INSERT INTO test_numeric (numeric_col, description) VALUES (0, '零值'),(1, '正整数'),(-1, '负整数'),(123.456, '正小数'),(-123.456, '负小数'),(0.000000001, '极小正小数'),(-0.000000001, '极小负小数'),(999999999999999.999999999, '大数值小数'),(-999999999999999.999999999, '大数值负小数'),('NaN', '非数字'),('0', '字符串零'),('123.45', '字符串数值');

SELECT numeric_col, description, pg_typeof(numeric_col) as data_type FROM test_numeric ORDER BY numeric_col NULLS FIRST;

-- 清理
DROP TABLE IF EXISTS test_numeric CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
