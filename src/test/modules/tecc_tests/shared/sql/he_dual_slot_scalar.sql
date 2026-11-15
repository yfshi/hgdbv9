-- ============================================================
-- he_dual_slot_scalar.sql — 双开 he-add,he-mul 列的两项行为锁定
-- ============================================================
-- 覆盖两个点：
--
-- A. 密文段覆盖校验（写入点，服务端）：
--    同态加/乘运算结果只携带参与运算的那一个 HE 段（build_he_add_result /
--    build_he_mul_result 只填 EG_ADD 或 EG_MUL）。因此「单段 HE 结果」不得写入
--    同时声明 he-add+he-mul 的列（该列要求两段都在）——否则列声明的算法能力与
--    实际数据背离，后续运算/解密才暴露。写入点 tecc_check_cipher_segments
--    必须在写入当时拒绝。（对应 QA 反馈的「双开列可接收单段结果」。）
--
-- B. 双开列的「明文标量乘」结果段与常量路径一致（修复点）：
--    前端对 `enc * <常量>` 走"用 he-mul 公钥加密常量 → he_mul_generic"，
--    结果只含 EG_MUL 段。服务端混合参数算子（enc × 明文 int，如 enc * 明文列）
--    原先对双槽密文优先 EC-ElGamal 标量倍点，结果只含 EG_ADD 段 —— 同一语义
--    的两种写法产出不同段，赋值目标与可参与的后续运算都不同。
--    现统一为：双槽优先 EG_MUL（与常量路径一致）；仅 he-add 列才走 EC 倍点。
--
-- 判定方式：把结果赋给「纯 he-mul 列 m」（能收 EG_MUL）与「纯 he-add 列 a」
-- （能收 EG_ADD），用赋值成败反推结果段。
--
-- 驱动差异（见 shared/jdbc_vs_libpq_diff.md）：B4「he-add 列 * 明文列」在 libpq
-- 侧成功（服务端 EC 倍点路径，结果 EG_ADD 与目标列匹配），JDBC 侧被前端严格规则
-- 拦下（"lacks he-mul segment capability for homomorphic assignment"）。同一现象
-- 已存在于既有用例 homomorphic_enc.sql 的 "19 UPDATE mul neg scalar (he-add)"。
-- 该点用于锁定「仅 he-add 列仍可标量乘」，故保留并分别固化两侧期望。
-- 归属：服务端执行器/算子行为，两驱动一致，故放 shared。
-- ============================================================

-- ============================================================
-- 0. 密钥与表
-- ============================================================
DROP TABLE IF EXISTS he_dss_t CASCADE;
DROP TABLE IF EXISTS he_dss_sc CASCADE;
DROP TABLE IF EXISTS he_dss_src CASCADE;
DROP DEK IF EXISTS he_dss_add CASCADE;
DROP DEK IF EXISTS he_dss_mul CASCADE;
DROP KEK IF EXISTS he_dss_kek CASCADE;

\! rm -f "$TECC_KEK_PATH/he_dss_kek.priv" "$TECC_KEK_PATH/he_dss_kek.pub" 2>/dev/null
\! tecc_ktool generate --key-name he_dss_kek --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1

CREATE KEK he_dss_kek WITH (
    key_store = 'localkms',
    key_algo  = 'sm2',
    key_path  = '%KEKPATH%/he_dss_kek.priv'
);
CREATE DEK he_dss_add WITH VALUES(KEK = he_dss_kek, KEY_ALGO = 'he-add');
CREATE DEK he_dss_mul WITH VALUES(KEK = he_dss_kek, KEY_ALGO = 'he-mul');

CREATE TABLE he_dss_t (
    id int,
    a int4_enc ENCRYPTED WITH (HE_ADD_DEK = 'he_dss_add', SUPPORT_ALGO = 'he-add'),
    m int4_enc ENCRYPTED WITH (HE_MUL_DEK = 'he_dss_mul', SUPPORT_ALGO = 'he-mul'),
    x int4_enc ENCRYPTED WITH (HE_ADD_DEK = 'he_dss_add', HE_MUL_DEK = 'he_dss_mul',
                               SUPPORT_ALGO = 'he-add,he-mul')
);
INSERT INTO he_dss_t (id, a, m, x) VALUES (1, 10, 2, 3);

SELECT '=== 0 baseline ===' AS stage;
SELECT id, a, m, x FROM he_dss_t;

-- ============================================================
-- A. 单段 HE 结果 -> 双开列：必须被拒
-- ============================================================
SELECT '=== A1 he-add 结果（仅 EG_ADD）-> 双开列 x：期望失败 ===' AS stage;
UPDATE he_dss_t SET x = a + a;

SELECT '=== A2 he-mul 结果（仅 EG_MUL）-> 双开列 x：期望失败 ===' AS stage;
UPDATE he_dss_t SET x = m * m;

SELECT '=== A3 常量 -> 双开列 x：期望成功（客户端双段加密）===' AS stage;
UPDATE he_dss_t SET x = 7;

SELECT '=== A4 UPDATE ... FROM 单段结果 -> 双开列 x：期望失败 ===' AS stage;
CREATE TABLE he_dss_src (
    id int,
    a int4_enc ENCRYPTED WITH (HE_ADD_DEK = 'he_dss_add', SUPPORT_ALGO = 'he-add')
);
INSERT INTO he_dss_src (id, a) VALUES (1, 100);
UPDATE he_dss_t SET x = s.a + s.a FROM he_dss_src s WHERE s.id = he_dss_t.id;

SELECT '=== A5 被拒后列值完好（x 仍为常量写入的 7）===' AS stage;
SELECT id, a, m, x FROM he_dss_t;

-- ============================================================
-- B. 双开列明文标量乘：结果段与常量路径一致
-- ============================================================
SELECT '=== B1 create ===' AS stage;
CREATE TABLE he_dss_sc (
    id int,
    x int4_enc ENCRYPTED WITH (HE_ADD_DEK = 'he_dss_add', HE_MUL_DEK = 'he_dss_mul',
                               SUPPORT_ALGO = 'he-add,he-mul'),
    k int4,
    m int4_enc ENCRYPTED WITH (HE_MUL_DEK = 'he_dss_mul', SUPPORT_ALGO = 'he-mul'),
    a int4_enc ENCRYPTED WITH (HE_ADD_DEK = 'he_dss_add', SUPPORT_ALGO = 'he-add')
);
INSERT INTO he_dss_sc (id, x, k, m, a) VALUES (1, 6, 4, 0, 0);

SELECT '=== B2 常量路径 x * 3 -> 纯 he-mul 列：期望成功（结果 EG_MUL）===' AS stage;
UPDATE he_dss_sc SET m = x * 3;

SELECT '=== B3 明文列路径 x * k -> 纯 he-mul 列：期望成功（修复后结果 EG_MUL）===' AS stage;
UPDATE he_dss_sc SET m = x * k;

SELECT '=== B4 仅 he-add 列 a * k（EC 倍点路径保留；目标列无 he-mul，JDBC 前端拦截）===' AS stage;
UPDATE he_dss_sc SET a = a * k;

SELECT '=== B5 仅 he-mul 列 m * k：期望成功（FF 同态乘路径保留）===' AS stage;
UPDATE he_dss_sc SET m = m * k;

SELECT '=== B6 明文正确性（x=6 k=4：m=6*3=18 → *4=24 → *4=96；a=0*4=0）===' AS stage;
SELECT id, x, k, m, a FROM he_dss_sc;

-- ============================================================
-- 9. 清理
-- ============================================================
DROP TABLE he_dss_sc CASCADE;
DROP TABLE he_dss_t CASCADE;
DROP TABLE he_dss_src CASCADE;
DROP DEK he_dss_add;
DROP DEK he_dss_mul;
DROP KEK he_dss_kek CASCADE;
