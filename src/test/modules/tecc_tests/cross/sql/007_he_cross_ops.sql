-- ============================================================
-- 007_he_cross_ops.sql — 跨驱动测试：同态运算、回写与清理（读/算侧）
-- ReqID:TECC4.0
-- ============================================================
-- 与 006 相邻，奇偶交替下必然由**另一个驱动**执行：读取上一步对端驱动写入的
-- 密文并解密、在其上做同态运算、把运算结果回写密态列、再由对端复检。
-- 覆盖：轮询解密、EC-ElGamal 加减、FF-ElGamal 乘法、标量运算（he-add 正标量、
--       he-mul 负标量）、跨类型运算、SUM 聚合、同态结果回写 he-add/he-mul 列、
--       回写后复检、清理。
--
-- 未纳入的两处驱动差异（均为前端/驱动侧，非本仓库后端问题；本套件只有一份期望、
-- 正反两轮四步共用，做不到按驱动分别期望）：
--   ① he-add 列的**标量乘回写** `UPDATE t SET a = a * k`：libpq 正常算出结果，
--      JDBC 报
--        TECC strict: encrypted column '...' lacks he-mul segment capability
--                     for homomorphic assignment to 'a' in SET clause
--      证据：共享用例 homomorphic_enc 的第 19 段两套期望（libpq 侧算出 -200，
--          JDBC 侧为上面的 ERROR 且值未变）。
--   ② he-add 列的**负标量乘 / 一元负号**（`a * (-1)`、`-a`）：JDBC 正常，
--      libpq 报 `TECC result processing error: HE result out of range for
--      column "a"`。同表 `a - 3`（结果为负）与 he-mul 列的 `m * (-2)` 两驱动
--      均正常，故非"负值"本身；已单独登记，修复后补回本用例。
-- ============================================================

SELECT '=== 007 HE CROSS: 轮询解密（对端写入的密文） ===' AS stage;
SELECT id, a, m, a8 FROM he_cross_t ORDER BY id;

SELECT '=== 007 HE CROSS: 同态加法 ===' AS stage;
SELECT id, a + a AS a_self, a + 5 AS a_scalar, a8 + a8 AS a8_self FROM he_cross_t ORDER BY id;

SELECT '=== 007 HE CROSS: 同态减法 ===' AS stage;
SELECT id, a - a AS a_zero, a - 3 AS a_scalar_sub FROM he_cross_t ORDER BY id;

SELECT '=== 007 HE CROSS: 同态乘法 ===' AS stage;
SELECT id, m * m AS m_self, m * 3 AS m_scalar FROM he_cross_t ORDER BY id;

SELECT '=== 007 HE CROSS: 标量乘（he-add 正标量 / he-mul 负标量） ===' AS stage;
SELECT id, a * 2 AS a_pos, m * (-2) AS m_neg FROM he_cross_t ORDER BY id;

SELECT '=== 007 HE CROSS: 跨类型加法 int4+int8 ===' AS stage;
SELECT id, a + a8 AS mix48 FROM he_cross_t ORDER BY id;

SELECT '=== 007 HE CROSS: SUM 聚合 ===' AS stage;
SELECT sum(a) AS sum_a FROM he_cross_t;
SELECT sum(a8) AS sum_a8 FROM he_cross_t;

SELECT '=== 007 HE CROSS: 同态结果回写 ===' AS stage;
UPDATE he_cross_t SET a = a + a WHERE id = 1;
UPDATE he_cross_t SET a = a + 100 WHERE id = 2;
UPDATE he_cross_t SET m = m * 3 WHERE id = 1;
SELECT id, a, m FROM he_cross_t WHERE id IN (1, 2) ORDER BY id;

SELECT '=== 007 HE CROSS: 回写后复检 ===' AS stage;
SELECT id, a, m, a8 FROM he_cross_t ORDER BY id;
SELECT sum(a) AS sum_a_after FROM he_cross_t;

SELECT '=== 007 HE CROSS: 清理 ===' AS stage;
DROP TABLE he_cross_t CASCADE;
DROP DEK he_cross_add;
DROP DEK he_cross_mul;
DROP KEK kek_he_cross CASCADE;
