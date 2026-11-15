-- A11: HE列 + 普通列混合运算验证
-- 复用 homomorphic_enc.sql 的 kek_he setup

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
CREATE DEK dek_he_mul WITH VALUES(KEK = kek_he, KEY_ALGO = 'he-mul');

-- 建表
DROP TABLE IF EXISTS t_a11_he CASCADE;
DROP TABLE IF EXISTS t_a11_plain CASCADE;
CREATE TABLE t_a11_he(id serial, a int4_enc ENCRYPTED WITH (HE_ADD_DEK=dek_he_add, SUPPORT_ALGO='he-add'));
CREATE TABLE t_a11_plain(id serial, p int4);

INSERT INTO t_a11_he(a) VALUES (100), (200), (300);
INSERT INTO t_a11_plain(p) VALUES (10), (20), (30);

-- 场景1: HE列 + 普通列（JOIN方式）
SELECT '=== A11-1 HE col + plain col via JOIN ===';
SELECT h.a, p.p, h.a + p.p AS result
FROM t_a11_he h JOIN t_a11_plain p ON h.id = p.id;

-- 场景2: HE列 + 普通字面量
SELECT '=== A11-2 HE col + literal scalar ===';
SELECT a, a + 10 AS result FROM t_a11_he;

-- 场景3: 普通列在左侧
SELECT '=== A11-3 plain col + HE col ===';
SELECT h.a, p.p, p.p + h.a AS result
FROM t_a11_he h JOIN t_a11_plain p ON h.id = p.id;

-- 场景4: HE列 - 普通列
SELECT '=== A11-4 HE col - plain col ===';
SELECT h.a, p.p, h.a - p.p AS result
FROM t_a11_he h JOIN t_a11_plain p ON h.id = p.id;

-- 场景5: HE列 * 普通字面量
SELECT '=== A11-5 HE col * literal ===';
SELECT a, a * 3 AS result FROM t_a11_he;

-- 场景6: 建立普通int4_enc(DEK)表，与普通表混合
DROP TABLE IF EXISTS t_a11_det CASCADE;
CREATE DEK IF NOT EXISTS dek_det WITH VALUES(KEK = kek_he);
CREATE TABLE t_a11_det(id serial, a int4_enc ENCRYPTED WITH (DEK=dek_det, SUPPORT_ALGO='det'));
INSERT INTO t_a11_det(a) VALUES (100), (200), (300);

SELECT '=== A11-6 DET enc col + plain col ===';
SELECT d.a, p.p, d.a + p.p AS result
FROM t_a11_det d JOIN t_a11_plain p ON d.id = p.id;

-- 清理
DROP TABLE t_a11_he CASCADE;
DROP TABLE t_a11_plain CASCADE;
DROP TABLE t_a11_det CASCADE;
DROP DEK IF EXISTS dek_he_add CASCADE;
DROP DEK IF EXISTS dek_he_mul CASCADE;
DROP DEK IF EXISTS dek_det CASCADE;
DROP KEK IF EXISTS kek_he CASCADE;
