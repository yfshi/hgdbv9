-- 清理
DROP TABLE IF EXISTS test_float8 CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 测试步骤
CREATE TABLE test_float8 (id SERIAL PRIMARY KEY,float8_col float8_enc encrypted with (dek = dek_t1,support_algo='ope'),description TEXT,created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);

INSERT INTO test_float8 (float8_col, description) VALUES (0.0, '零值'),(1.0, '正一'),(-1.0, '负一'),(3.141592653589793, '圆周率π-高精度'),(2.718281828459045, '自然常数e-高精度'),(123.456789, '普通小数'),(-123.456789, '负小数'),(0.000000000000001, '极小正数'),(-0.000000000000001, '极小负数'),('NaN', '非数字'),('Infinity', '正无穷'),('-Infinity', '负无穷'),(1.0e308, '极大科学计数法'),(1.0e-308, '极小科学计数法'),(1.7976931348623157e+308, '接近float8最大值'),(-1.7976931348623157e+308, '接近float8最小值');

SELECT float8_col, description, pg_typeof(float8_col) as data_type FROM test_float8 ORDER BY float8_col NULLS FIRST;

-- 清理
DROP TABLE IF EXISTS test_float8 CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
