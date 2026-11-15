-- ============================================================
-- 006_he_cross.sql — 跨驱动测试：同态(HE)密钥与基线数据（写入侧）
-- ReqID:TECC4.0
-- ============================================================
-- 本用例只做写入：建 HE KEK(SM2 非对称)/HE DEK/HE 表，并插入明文基线行。
-- 轮询解密、同态运算、结果回写与清理在 007_he_cross_ops.sql，与文件号奇偶
-- 交替配合必然由**另一个驱动**执行，形成 libpq↔JDBC 双向交叉：
--   正向 006→jdbc / 007→libpq，反向 006→libpq / 007→jdbc。
--
-- 期望输出只有一份、两轮两个驱动共用，故只覆盖两驱动行为一致的语句：
--   - 不 SELECT 布尔（libpq 输出 t/f，JDBC 输出 true/false）；
--   - 不含错误路径（两驱动前端错误文本不同）；
--   - he-add 列的标量乘回写 `SET a = a * k` 为 JDBC 前端已知缺口，归 007 侧
--     说明，不纳入。
-- ============================================================

DROP TABLE IF EXISTS he_cross_t CASCADE;
DROP DEK IF EXISTS he_cross_add CASCADE;
DROP DEK IF EXISTS he_cross_mul CASCADE;
DROP KEK IF EXISTS kek_he_cross CASCADE;

-- HE DEK 必须挂非对称（SM2）KEK：对称 KEK 会被拒绝。密钥文件由回归框架
-- pregen_localkms_kek_files 按本块预生成到 tmp_check/keys（%KEKPATH% 由
-- 执行前替换，asym KEK 建 KEK 时直接 stat key_path，不走 TECC_KEK_PATH 覆盖）。
CREATE KEK kek_he_cross WITH (
    key_store = 'localkms',
    key_algo = 'sm2',
    key_path = '%KEKPATH%/kek_he_cross.priv'
);
CREATE DEK he_cross_add WITH VALUES(KEK = kek_he_cross, KEY_ALGO = 'he-add');
CREATE DEK he_cross_mul WITH VALUES(KEK = kek_he_cross, KEY_ALGO = 'he-mul');

-- a/a8 为 EC-ElGamal 加法同态列，m 为有限域 ElGamal 乘法同态列
CREATE TABLE he_cross_t (
    id SERIAL,
    a  INT4_ENC ENCRYPTED WITH (HE_ADD_DEK = 'he_cross_add', SUPPORT_ALGO = 'he-add'),
    m  INT4_ENC ENCRYPTED WITH (HE_MUL_DEK = 'he_cross_mul', SUPPORT_ALGO = 'he-mul'),
    a8 INT8_ENC ENCRYPTED WITH (HE_ADD_DEK = 'he_cross_add', SUPPORT_ALGO = 'he-add')
);

INSERT INTO he_cross_t (a, m, a8) VALUES (10, 2, 1000);
INSERT INTO he_cross_t (a, m, a8) VALUES (100, 5, 2000);
INSERT INTO he_cross_t (a, m, a8) VALUES (0, 0, 0);
INSERT INTO he_cross_t (a, m, a8) VALUES (-5, -3, -100);

SELECT '=== 006 HE CROSS: 写入侧轮询解密 ===' AS stage;
SELECT id, a, m, a8 FROM he_cross_t ORDER BY id;
