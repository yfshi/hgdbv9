-- 清理
DROP TABLE IF EXISTS test_float4 CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 测试步骤
CREATE TABLE test_float4 (id SERIAL PRIMARY KEY,float4_col float4_enc encrypted with (dek = dek_t1,support_algo='ope'),description TEXT,created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);

INSERT INTO test_float4 (float4_col, description) VALUES (0.0, '零值'),(1.0, '正一'),(-1.0, '负一'),(3.14159265, '圆周率π'),(2.71828182, '自然常数e'),(123.456, '普通小数'),(-123.456, '负小数'),(0.0000001, '极小正数'),(-0.0000001, '极小负数'),('NaN', '非数字'),('Infinity', '正无穷'),('-Infinity', '负无穷'),(1.0e10, '科学计数法大数'),(1.0e-10, '科学计数法小数');

SELECT float4_col, description, pg_typeof(float4_col) as data_type FROM test_float4 ORDER BY float4_col NULLS FIRST;

-- 清理
DROP TABLE IF EXISTS test_float4 CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
