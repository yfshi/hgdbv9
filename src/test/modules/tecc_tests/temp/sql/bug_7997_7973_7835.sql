--
-- bug_7997_7973_7835.sql — G8 精度丢失复现
-- Bug 7997: numeric_enc解密后精度丢失
-- Bug 7973: numeric_enc大数值小数进位
-- Bug 7835: float4_enc/float8_enc/numeric负数解密不一致
--
-- 根因: 前端解析器 grammar FLOAT规则用strtod转double，>15位有效数字丢失
-- 修复: values_processor.c中FLOAT常量从原始SQL提取精确文本替代double值
--

DROP TABLE IF EXISTS test_g8_prec CASCADE;
DROP DEK IF EXISTS dek_g8 CASCADE;
DROP KEK IF EXISTS kek_g8 CASCADE;

CREATE KEK kek_g8 WITH (key_store = 'localkms', key_path = '/tmp/kek_g8.key');
CREATE DEK dek_g8 WITH VALUES(KEK = 'kek_g8');

-- 高精度numeric_enc
CREATE TABLE test_g8_prec (
    id SERIAL PRIMARY KEY,
    num_col numeric_enc ENCRYPTED WITH (dek = dek_g8, support_algo = 'det'),
    f8_col float8_enc ENCRYPTED WITH (dek = dek_g8, support_algo = 'det')
);

INSERT INTO test_g8_prec (num_col, f8_col) VALUES
    (1234567890.1234567890, 1234567890.1234567890),
    (999999999999999.999999999, 999999999999999.999999999),
    (-999999999999999.999999999, -999999999999999.999999999),
    (1.12345678901234567890, 1.12345678901234567890);

SELECT 'expect: 1234567890.123456789 (trailing 0 stripped)' AS info;
SELECT id, num_col FROM test_g8_prec ORDER BY id;

DROP TABLE test_g8_prec CASCADE;
DROP DEK dek_g8 CASCADE;
DROP KEK kek_g8 CASCADE;
