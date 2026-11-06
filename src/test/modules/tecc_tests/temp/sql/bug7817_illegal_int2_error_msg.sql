-- Bug 7817: 非法数据插入int2_enc列时告警不够明确
-- TC_IVYSQL_TECC4_DATATYPE_INTEGER_001
-- 期望：只展示关键的报错信息，类似普通数据报错

DROP TABLE IF EXISTS tb_int2 CASCADE;
DROP DEK IF EXISTS dek_t1;
DROP KEK IF EXISTS kek_t1;

CREATE KEK kek_t1 WITH (
    key_store = 'localkms',
    key_path = '/tmp/kek_7817.key'
);

CREATE DEK dek_t1 WITH VALUES(
    KEK = kek_t1
);

-- Step 1: Create table with int2_enc column
CREATE TABLE tb_int2(
    id SERIAL PRIMARY KEY,
    smallint_col int2_enc encrypted with (dek = dek_t1),
    description TEXT
);

-- Step 2: Insert valid data (within int2 range: -32768 to 32767)
INSERT INTO tb_int2 (smallint_col, description) VALUES
    (0, '零值'),
    (1, '正值'),
    (-1, '负值'),
    (32767, '最大值'),
    (-32768, '最小值'),
    (32766, '接近最大值'),
    (-32767, '接近最小值'),
    (100, '中等正值'),
    (-100, '中等负值');

-- Step 3: Insert INVALID data — should produce clear, concise error messages
INSERT INTO tb_int2 (smallint_col, description) VALUES (32768, '超出最大值');
INSERT INTO tb_int2 (smallint_col, description) VALUES (-32769, '超出最小值');
INSERT INTO tb_int2 (smallint_col, description) VALUES (123.45, '浮点数');

-- Step 4: Verify valid data is intact
SELECT smallint_col, description, pg_typeof(smallint_col) as data_type
FROM tb_int2 ORDER BY smallint_col NULLS FIRST;
