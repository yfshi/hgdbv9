-- ============================================================
-- 010_swxa_he_cross_recheck.sql — 硬加密(HSM)×同态 跨驱动测试：对端回写复检 + 清理
-- ReqID:TECC4.0
-- ============================================================
-- 与 008/009 配对（说明见 008 头注释）。009 由对端驱动解封 DEK、解密并**回写**
-- 了一段新密文，本用例由**再下一个驱动**（即 008 的同一驱动）读回这段回写密文：
-- 于是"DEK 由 A 建 → B 解封装并用它对 A 的密文做 HE 解密、再加密回写 → A 用
-- 同一 DEK 解回"这条完整链路在一个方向上闭环，两轮反向再闭环一次。
--
-- 覆盖：对端回写后的轮询解密、SUM 聚合复检、清理。
--
-- 硬加密用例：已登记在 t/tecc_test_utils.pm 的 %HSM_CASE。
-- ============================================================

SELECT '=== 010 SWXA-HE CROSS: 对端回写后复检 ===' AS stage;
SELECT id, a, c, m, a8 FROM swxa_he_cross_t ORDER BY id;
SELECT sum(a) AS sum_a, sum(c) AS sum_c, sum(a8) AS sum_a8 FROM swxa_he_cross_t;

SELECT '=== 010 SWXA-HE CROSS: 清理 ===' AS stage;
DROP TABLE swxa_he_cross_t CASCADE;
DROP DEK swxa_he_cross_add;
DROP DEK swxa_he_cross_mul;
DROP DEK swxa_he_cross_sym;
DROP KEK swxa_he_cross_kek CASCADE;
