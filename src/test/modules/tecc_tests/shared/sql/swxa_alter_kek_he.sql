-- ============================================================
-- swxa_alter_kek_he.sql — 硬加密【非对称 KEK】轮换（sm2 / 同态设备路径）
-- ============================================================
-- 为什么单列一条：shared/sql/swxa_alter_kek.sql 只覆盖**对称** HSM KEK，
--   shared/sql/swxa_enc.sql 覆盖 HSM×同态但**没有 ALTER KEK**（grep 0 命中）。
--   于是 ALTER KEK 里两条只在非对称 KEK 上才走得到的设备通道长期无回归覆盖：
--     a) 新公钥由设备导出（SDF_ExportEncPublicKey，新索引）并回写 PUBLIC_KEY；
--     b) HE 私钥信封（he_ec_privkey / he_eg_privkey）用设备 SM2 解封后重封装。
--
-- 覆盖：HSM 托管的非对称 KEK（key_store='swxakms' + key_algo='sm2'）
--   从设备密钥 hsm:id=3 轮换到 hsm:id=4，三条设备侧通道都要走通：
--     1) 对称 DEK 的 encrypted_value：设备 SM4 解包（旧索引）→ 设备 SM4 重封装（新索引）
--        （HSM KEK 下 DEK 一律由设备 SM4 封装，非对称 KEK 也一样，
--          见 encrypt_dek_based_on_kek_type 的 is_hsm_placeholder 分支）
--     2) HE 私钥信封：设备 SM2 解封（旧索引）→ 新公钥重封装
--     3) 新公钥设备导出 + hg_kek_args.PUBLIC_KEY / KEY_PATH 写回
--
-- 两个驱动都执行（放 shared/sql 的原因）：libpq 经 SDF 动态库调用设备，JDBC 经
--   JCE Provider 调用设备，两条通道互相独立——本用例是二者在"非对称 KEK 轮换"
--   上的唯一覆盖。（归类原则：只要两个驱动都能跑就放 shared/ —— 原 jdbc/ 目录因用例
--   实由 psql 执行、名实不符，已于 2026-09-20 解散，见 archive/jdbc_dir_retired_20260920/）
--
-- 判定点：
--   轮换后旧数据可解、同态加/乘运算正常、轮换后新数据可写；
--   **轮换后新建的 HE DEK** 也能 round-trip —— 这一条才真正证明 PUBLIC_KEY 已换成
--   新设备的公钥（若沿用旧公钥，新 HE 私钥会用旧公钥信封、运行时用新索引的设备
--   私钥解不开）；KEY_PATH 元数据确已切到 hsm:id=4。
--
-- 前置：设备内预置两把**无访问口令**的 SM2 设备密钥，各带一把对称设备密钥：
--   hsm:id=3 —— tecc_kms/keys/{kek-3.key, sm2*-3.pem}（swxa_enc.sql 亦用）
--   hsm:id=4 —— tecc_kms/keys/{kek-4.key, sm2*-4.pem}（本用例的"新"密钥）
--   回归框架（tecc_test_utils.pm 的 setup_swsds_ini）已把这两组复制到两个驱动的
--   工作目录（CWD = tmp_check/tmp），SDF 与 JCE Provider 都按相对 CWD 取密钥。
-- ============================================================

-- ============================================================
-- 0. 清理 + 建密钥
-- ============================================================
DROP TABLE IF EXISTS swxa_hr_t CASCADE;
DROP TABLE IF EXISTS swxa_hr_mul_t CASCADE;
DROP TABLE IF EXISTS swxa_hr_new_t CASCADE;
DROP DEK IF EXISTS swxa_hr_sym CASCADE;
DROP DEK IF EXISTS swxa_hr_add CASCADE;
DROP DEK IF EXISTS swxa_hr_mul CASCADE;
DROP DEK IF EXISTS swxa_hr_add2 CASCADE;
DROP KEK IF EXISTS swxa_hr_kek CASCADE;

-- 1. 非对称 KEK：SM2 设备密钥托管在密码机 hsm:id=3
CREATE KEK swxa_hr_kek WITH (
    key_store = 'swxakms',
    key_algo  = 'sm2',
    key_path  = 'hsm:id=3'
);

-- 2. 一对称 DEK + 两个同态 DEK（he-add / he-mul）
CREATE DEK swxa_hr_sym WITH VALUES(KEK = swxa_hr_kek, KEY_ALGO = 'sm4');
CREATE DEK swxa_hr_add WITH VALUES(KEK = swxa_hr_kek, KEY_ALGO = 'he-add');
CREATE DEK swxa_hr_mul WITH VALUES(KEK = swxa_hr_kek, KEY_ALGO = 'he-mul');

-- ============================================================
-- 1. 建表 + 轮换前写入/读取
-- ============================================================
SELECT '=== 1.1 建表 ===' AS stage;

CREATE TABLE swxa_hr_t (
    id SERIAL,
    a  INT4_ENC ENCRYPTED WITH (DEK = swxa_hr_sym, HE_ADD_DEK = swxa_hr_add,
                                SUPPORT_ALGO = 'det, he-add'),
    c  INT4_ENC ENCRYPTED WITH (DEK = swxa_hr_sym, HE_ADD_DEK = swxa_hr_add,
                                SUPPORT_ALGO = 'he-add')
);

CREATE TABLE swxa_hr_mul_t (
    id SERIAL,
    m  INT4_ENC ENCRYPTED WITH (HE_MUL_DEK = swxa_hr_mul, SUPPORT_ALGO = 'he-mul')
);

SELECT '=== 1.2 轮换前写入 + 读取 ===' AS stage;

INSERT INTO swxa_hr_t (a) VALUES (10), (20), (30);
INSERT INTO swxa_hr_mul_t (m) VALUES (2), (3), (5);

SELECT id, a, c FROM swxa_hr_t ORDER BY id;
SELECT id, m FROM swxa_hr_mul_t ORDER BY id;

SELECT '=== 1.3 轮换前同态运算 ===' AS stage;

UPDATE swxa_hr_t SET c = a + a;
SELECT id, a, c FROM swxa_hr_t ORDER BY id;
SELECT sum(c) AS sum_c FROM swxa_hr_t;

SELECT '=== 1.4 轮换前 KEK 元数据（设备密钥 hsm:id=3 的公钥，65 字节 = hex 130） ===' AS stage;

SELECT 'kek path before',
       encode(value, 'escape') = 'hsm:id=3' AS key_path_is_id3
FROM hg_kek_args
WHERE upper(key) = 'KEY_PATH'
  AND kek_id = (SELECT oid FROM hg_kek WHERE kek_name = 'swxa_hr_kek');

SELECT 'pubkey before', length(encode(value, 'hex')) = 130 AS pub_is_65b
FROM hg_kek_args
WHERE upper(key) = 'PUBLIC_KEY'
  AND kek_id = (SELECT oid FROM hg_kek WHERE kek_name = 'swxa_hr_kek');

-- ============================================================
-- 2. ALTER KEK：hsm:id=3 → hsm:id=4
--    设备解包（旧索引）→ 新公钥导出（新索引）→ 重封装 → 元数据写回
-- ============================================================
SELECT '=== 2.1 ALTER KEK hsm:id=3 → hsm:id=4 ===' AS stage;

ALTER KEK swxa_hr_kek WITH (KEY_PATH = 'hsm:id=4');

SELECT '=== 2.2 轮换后 KEK 元数据 ===' AS stage;

SELECT 'kek path after',
       encode(value, 'escape') = 'hsm:id=4' AS key_path_is_id4
FROM hg_kek_args
WHERE upper(key) = 'KEY_PATH'
  AND kek_id = (SELECT oid FROM hg_kek WHERE kek_name = 'swxa_hr_kek');

SELECT 'pubkey after', length(encode(value, 'hex')) = 130 AS pub_is_65b
FROM hg_kek_args
WHERE upper(key) = 'PUBLIC_KEY'
  AND kek_id = (SELECT oid FROM hg_kek WHERE kek_name = 'swxa_hr_kek');

-- ============================================================
-- 3. 轮换后：旧数据可解（对称 DEK 已重封装 + HE 私钥信封已用新公钥重封装）
--    读取路径要用设备 hsm:id=4 的 SM2 私钥解封 HE 信封，用设备 SM4 解包 DEK
-- ============================================================
SELECT '=== 3.1 轮换后旧数据（须为明文，非原始密文） ===' AS stage;

SELECT id, a, c FROM swxa_hr_t ORDER BY id;
SELECT id, m FROM swxa_hr_mul_t ORDER BY id;

SELECT '=== 3.2 轮换后同态运算 ===' AS stage;

UPDATE swxa_hr_t SET c = a + a;
SELECT id, a, c FROM swxa_hr_t ORDER BY id;
SELECT sum(c) AS sum_c FROM swxa_hr_t;
SELECT id, m * 3 AS m_times_3 FROM swxa_hr_mul_t ORDER BY id;

SELECT '=== 3.3 轮换后新数据可写 ===' AS stage;

INSERT INTO swxa_hr_t (a) VALUES (40);
INSERT INTO swxa_hr_mul_t (m) VALUES (7);
SELECT id, a, c FROM swxa_hr_t ORDER BY id;
SELECT id, m FROM swxa_hr_mul_t ORDER BY id;

-- ============================================================
-- 4. 轮换后新建 HE DEK：证明 PUBLIC_KEY 已是设备 hsm:id=4 的公钥
--    （若 PUBLIC_KEY 未更新，新 HE 私钥会用旧公钥信封，
--      运行时用 hsm:id=4 的设备私钥解不开 → 读回的是原始密文或直接报错）
-- ============================================================
SELECT '=== 4.1 轮换后新建 HE DEK + round-trip ===' AS stage;

CREATE DEK swxa_hr_add2 WITH VALUES(KEK = swxa_hr_kek, KEY_ALGO = 'he-add');

CREATE TABLE swxa_hr_new_t (
    id SERIAL,
    n  INT4_ENC ENCRYPTED WITH (HE_ADD_DEK = swxa_hr_add2, SUPPORT_ALGO = 'he-add')
);

INSERT INTO swxa_hr_new_t (n) VALUES (100), (200);
SELECT id, n FROM swxa_hr_new_t ORDER BY id;
UPDATE swxa_hr_new_t SET n = n + n;
SELECT id, n FROM swxa_hr_new_t ORDER BY id;

-- ============================================================
-- 5. 清理
-- ============================================================
DROP TABLE swxa_hr_new_t CASCADE;
DROP TABLE swxa_hr_mul_t CASCADE;
DROP TABLE swxa_hr_t CASCADE;
DROP DEK swxa_hr_add2 CASCADE;
DROP DEK swxa_hr_mul CASCADE;
DROP DEK swxa_hr_add CASCADE;
DROP DEK swxa_hr_sym CASCADE;
DROP KEK swxa_hr_kek CASCADE;
