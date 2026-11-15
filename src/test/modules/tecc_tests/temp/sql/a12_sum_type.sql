-- A12: SUM(int4_enc) 返回类型验证
-- 验证SUM(int4_enc)是否提升到int8_enc

-- Setup: 非对称KEK
DROP KEK IF EXISTS kek_he CASCADE;
\! rm -f "$TECC_KEK_PATH/kek_he_asym.priv" "$TECC_KEK_PATH/kek_he_asym.pub" 2>/dev/null
\! tecc_ktool generate --key-name kek_he_asym --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1
CREATE KEK kek_he WITH (
    key_store = 'localkms',
    key_algo = 'sm2',
    key_path = '%KEKPATH%/kek_he_asym.priv'
);
CREATE DEK dek_he_add WITH VALUES(KEK = kek_he, KEY_ALGO = 'he-add');

-- 建表
DROP TABLE IF EXISTS t_a12 CASCADE;
CREATE TABLE t_a12(id serial, a int4_enc ENCRYPTED WITH (HE_ADD_DEK=dek_he_add, SUPPORT_ALGO='he-add'));

-- 插入足够大的数据使SUM超出int4范围
-- int4 max = 2147483647，插入3个值使SUM > int4 max
INSERT INTO t_a12(a) VALUES (1000000000), (1000000000), (500000000);
-- SUM = 2500000000，超出int4范围但在int8范围内

-- 场景1: SUM结果值
SELECT '=== A12-1 SUM value ===';
SELECT sum(a) AS total FROM t_a12;

-- 场景2: pg_typeof检查返回类型
SELECT '=== A12-2 pg_typeof(SUM) ===';
SELECT pg_typeof(sum(a)) AS sum_type FROM t_a12;

-- 场景3: 对比原生int4的SUM类型
SELECT '=== A12-3 native int4 SUM type ===';
SELECT pg_typeof(sum(1::int4)) AS native_int4_sum_type;

-- 场景4: 对比原生int8的SUM类型
SELECT '=== A12-4 native int8 SUM type ===';
SELECT pg_typeof(sum(1::int8)) AS native_int8_sum_type;

-- 场景5: SUM(int2_enc) 类型
CREATE DEK IF NOT EXISTS dek_he_add2 WITH VALUES(KEK = kek_he, KEY_ALGO = 'he-add');
DROP TABLE IF EXISTS t_a12_int2 CASCADE;
CREATE TABLE t_a12_int2(id serial, a int2_enc ENCRYPTED WITH (HE_ADD_DEK=dek_he_add2, SUPPORT_ALGO='he-add'));
INSERT INTO t_a12_int2(a) VALUES (10000), (10000), (15000);
-- SUM = 35000，超出int2范围但在int4/int8范围内

SELECT '=== A12-5 SUM(int2_enc) value ===';
SELECT sum(a) AS total FROM t_a12_int2;

SELECT '=== A12-6 pg_typeof(SUM(int2_enc)) ===';
SELECT pg_typeof(sum(a)) AS sum_type FROM t_a12_int2;

-- 清理
DROP TABLE t_a12 CASCADE;
DROP TABLE t_a12_int2 CASCADE;
DROP DEK IF EXISTS dek_he_add CASCADE;
DROP DEK IF EXISTS dek_he_add2 CASCADE;
DROP KEK IF EXISTS kek_he CASCADE;
