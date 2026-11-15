-- ============================================================
-- 008_swxa_he_cross.sql — 硬加密(HSM)×同态 跨驱动测试：写入侧
-- ReqID:TECC4.0
-- ============================================================
-- 为什么单列一对（008/009）：
--   cross/sql/005_swxa_cross.sql 的硬加密列全是 DET 对称列，不含同态；
--   带同态的 006/007 走 key_store='localkms'（本地 SM2 密钥文件）。
--   于是"密码机托管的 HE 私钥信封"这条路径下（设备 SM2 解封 he_ec_privkey /
--   he_eg_privkey），libpq 经 libteccsdf.so、JDBC 经 JCE Provider 走同一设备
--   各自解出的 HE 私钥是否一致、两端 HE 密文编码是否互通，没有用例验证——
--   shared/sql/swxa_enc.sql 等 HSM×HE 用例每次只由单一驱动执行，密文从不跨
--   驱动交换。
--
-- 本用例只做写入：建 HSM(swxakms) 非对称 KEK（key_algo='sm2'，hsm:id=3）、
-- he-add / he-mul / 对称 DEK，建表并插入明文基线行。轮询解密、同态运算、结果
-- 回写在 009_swxa_he_cross_ops.sql，对端回写后的复检与清理在
-- 010_swxa_he_cross_recheck.sql，与文件号奇偶交替配合必然由**另一个驱动**执行，
-- 形成 libpq↔JDBC 双向交叉：
--   正向 008→jdbc / 009→libpq / 010→jdbc，反向 008→libpq / 009→jdbc / 010→libpq。
--
-- 关键点：HE 流程里只有 **DEK 的加解密在密码机内执行**（DEK encrypted_value 与
-- HE 私钥信封由设备 SM2/SM4 解封装），HE 值本身的加解密与同态运算都在客户端软件
-- 完成。故本对用例要证明的正是这条设备通道跨驱动一致：一端建的 DEK，另一端能
-- 解封装出同一把密钥，且用它解得出对端写下的密文、也算得出对端能解回的密文。
--
-- 期望输出只有一份、两轮两个驱动共用，故只覆盖两驱动行为一致的语句（同 006/007）：
--   - 不 SELECT 布尔（libpq 输出 t/f，JDBC 输出 true/false）；
--   - 不含错误路径（两驱动前端错误文本不同）；
--   - he-add 列的标量乘回写 `UPDATE ... SET a = a * k`（JDBC 报 lacks he-mul
--     segment capability）与 he-add 负标量乘 / 一元负号（libpq 报 HE result out
--     of range）两处驱动差异见 007 头注释，本对用例不纳入。
--
-- 硬加密用例：已登记在 t/tecc_test_utils.pm 的 %HSM_CASE（唯一名单），
-- 密码机不可用时由 exclude_hsm_cases 整体剔除。
-- ============================================================

DROP TABLE IF EXISTS swxa_he_cross_t CASCADE;
DROP DEK IF EXISTS swxa_he_cross_add CASCADE;
DROP DEK IF EXISTS swxa_he_cross_mul CASCADE;
DROP DEK IF EXISTS swxa_he_cross_sym CASCADE;
DROP KEK IF EXISTS swxa_he_cross_kek CASCADE;

-- HE DEK 必须挂非对称（SM2）KEK：对称 KEK 会被拒绝。hsm:id=3 与
-- shared/sql/swxa_enc.sql 的 SWXA×HE 段取同一设备密钥（SoftSDF 模式对应
-- tecc_kms/keys/sm2enc-3.pem，由 setup_swsds_ini 复制到 tmp_check/tmp）。
CREATE KEK swxa_he_cross_kek WITH (
    key_store = 'swxakms',
    key_algo  = 'sm2',
    key_path  = 'hsm:id=3'
);

CREATE DEK swxa_he_cross_add WITH VALUES(KEK = swxa_he_cross_kek, KEY_ALGO = 'he-add');
CREATE DEK swxa_he_cross_mul WITH VALUES(KEK = swxa_he_cross_kek, KEY_ALGO = 'he-mul');
CREATE DEK swxa_he_cross_sym WITH VALUES(KEK = swxa_he_cross_kek, KEY_ALGO = 'sm4');

-- a 为 DET + he-add 混配列（明文读取走 DET 段、同态运算走 he-add 段）
-- c 为纯 he-add 列，a8 为 int8 纯 he-add 列，m 为纯 he-mul 列
CREATE TABLE swxa_he_cross_t (
    id SERIAL,
    a  INT4_ENC ENCRYPTED WITH (DEK = swxa_he_cross_sym, HE_ADD_DEK = swxa_he_cross_add,
                                SUPPORT_ALGO = 'det, he-add'),
    c  INT4_ENC ENCRYPTED WITH (DEK = swxa_he_cross_sym, HE_ADD_DEK = swxa_he_cross_add,
                                SUPPORT_ALGO = 'he-add'),
    m  INT4_ENC ENCRYPTED WITH (HE_MUL_DEK = swxa_he_cross_mul,
                                SUPPORT_ALGO = 'he-mul'),
    a8 INT8_ENC ENCRYPTED WITH (DEK = swxa_he_cross_sym, HE_ADD_DEK = swxa_he_cross_add,
                                SUPPORT_ALGO = 'he-add')
);

INSERT INTO swxa_he_cross_t (a, c, m, a8) VALUES (10, 3, 2, 1000);
INSERT INTO swxa_he_cross_t (a, c, m, a8) VALUES (100, 7, 5, 2000);
INSERT INTO swxa_he_cross_t (a, c, m, a8) VALUES (0, 0, 0, 0);
INSERT INTO swxa_he_cross_t (a, c, m, a8) VALUES (-5, -2, -3, -100);

SELECT '=== 008 SWXA-HE CROSS: 写入侧解密 ===' AS stage;
SELECT id, a, c, m, a8 FROM swxa_he_cross_t ORDER BY id;
