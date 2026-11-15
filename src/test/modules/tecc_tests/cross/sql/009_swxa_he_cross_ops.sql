-- ============================================================
-- 009_swxa_he_cross_ops.sql — 硬加密(HSM)×同态 跨驱动测试：读/算侧
-- ReqID:TECC4.0
-- ============================================================
-- 与 008 配对（说明见 008 头注释）：读取 008 由**另一个驱动**写入的 HSM 同态
-- 密文并解密，在其上做同态运算、把结果回写密态列、再复检；清理在 010。
-- 奇偶交替下 008/009/010 必由不同驱动轮转：正向 jdbc 写 → libpq 读算回写 →
-- jdbc 复检，反向 libpq 写 → jdbc 读算回写 → libpq 复检——两个方向的
-- "一端建 DEK、另一端解封 DEK 并做 HE 加解密"都被覆盖。
--
-- 覆盖：密码机托管 HE 私钥（设备 SM2 解封信封）下的轮询解密、EC-ElGamal
--       加/减、FF-ElGamal 乘法与幂、标量运算（he-add 正标量 / he-mul 负标量）、
--       int4+int8 跨类型运算、SUM 聚合、同态结果回写纯 he-add / 纯 he-mul 列、
--       回写后复检与清理。
--
-- 未纳入的驱动差异同 007 头注释（两处均为前端/驱动侧，本套件只有一份期望，
-- 做不到按驱动分别期望）：① he-add 列标量乘回写 SET c = c * k，② he-add 列
-- 负标量乘 / 一元负号。
--
-- 硬加密用例：已登记在 t/tecc_test_utils.pm 的 %HSM_CASE。
-- ============================================================

SELECT '=== 009 SWXA-HE CROSS: 轮询解密（对端写入的密文） ===' AS stage;
SELECT id, a, c, m, a8 FROM swxa_he_cross_t ORDER BY id;

SELECT '=== 009 SWXA-HE CROSS: 同态加法 ===' AS stage;
SELECT id, a + a AS a_self, a + 5 AS a_scalar, c + c AS c_self, a8 + a8 AS a8_self
FROM swxa_he_cross_t ORDER BY id;

SELECT '=== 009 SWXA-HE CROSS: 同态减法 ===' AS stage;
SELECT id, a - a AS a_zero, a - 3 AS a_scalar_sub, c - c AS c_zero
FROM swxa_he_cross_t ORDER BY id;

SELECT '=== 009 SWXA-HE CROSS: 同态乘法 ===' AS stage;
SELECT id, m * m AS m_self, m * 3 AS m_scalar FROM swxa_he_cross_t ORDER BY id;

SELECT '=== 009 SWXA-HE CROSS: 同态幂运算 ===' AS stage;
SELECT id, m ^ 2 AS m_squared FROM swxa_he_cross_t ORDER BY id;

SELECT '=== 009 SWXA-HE CROSS: 标量乘（he-add 正标量 / he-mul 负标量） ===' AS stage;
SELECT id, a * 2 AS a_pos, m * (-2) AS m_neg FROM swxa_he_cross_t ORDER BY id;

SELECT '=== 009 SWXA-HE CROSS: 跨类型加法 int4+int8 ===' AS stage;
SELECT id, a + a8 AS mix48 FROM swxa_he_cross_t ORDER BY id;

SELECT '=== 009 SWXA-HE CROSS: SUM 聚合 ===' AS stage;
SELECT sum(a) AS sum_a, sum(c) AS sum_c, sum(a8) AS sum_a8 FROM swxa_he_cross_t;

SELECT '=== 009 SWXA-HE CROSS: 同态结果回写 ===' AS stage;
UPDATE swxa_he_cross_t SET c = c + c WHERE id = 1;
UPDATE swxa_he_cross_t SET c = c + 100 WHERE id = 2;
UPDATE swxa_he_cross_t SET m = m * 2 WHERE id = 1;
SELECT id, c, m FROM swxa_he_cross_t WHERE id IN (1, 2) ORDER BY id;

SELECT '=== 009 SWXA-HE CROSS: 回写后复检 ===' AS stage;
SELECT id, a, c, m, a8 FROM swxa_he_cross_t ORDER BY id;
SELECT sum(c) AS sum_c_after FROM swxa_he_cross_t;
