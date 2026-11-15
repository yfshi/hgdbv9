-- ============================================================
-- swxa_key_cache.sql — 硬加密：会话密钥句柄连接级缓存（libpq 适配器内部机制）
-- ============================================================
-- 对象：libpq 客户端 src/interfaces/libpq/tecc_hsm_adapter/fe-swxa-adapter.c 的
--       swxa_sm4_encrypt / swxa_sm4_decrypt。硬加密（key_store='swxakms'）下
--       列数据的每个值都要过一次密码机；原先每值 = SDF_ImportKey + 运算 +
--       SDF_DestroyKey = 3 次设备往返（真机实测单次约 3.2 ms ⇒ 每值约 9.6 ms，
--       百万行扫描因此不可行）。现改为把 SDF_ImportKey 得到的**会话密钥句柄**
--       缓存在连接级（每个连接 16 槽，按 DEK 字节精确匹配，LRU 淘汰），每值只剩
--       1 次运算往返。
--
-- 为什么放在 libpq/（而不是 shared/）：本用例断言的是 **libpq C 适配器内部**的
--   句柄缓存行为；JDBC 侧不执行这段代码（列加解密经厂商 JCE Provider，句柄的
--   导入/销毁全在厂商 JAR 内），故无需驱动覆盖。两驱动共同行为由
--   shared/sql/swxa_*.sql 覆盖。
--
-- 断言口径：只看解密后的明文与行数，不看密文（每次随机），故 SoftSDF 与真机
--   两种模式输出一致。
--
-- 四段各对应一条缓存语义（改缓存实现前先看这里，尤其是失效路径）：
--   ① 同一 DEK 反复用 → 首值导入、其后命中，20 个值必须逐个解对
--   ② 多 DEK 交叉且总数超过 16 槽 → LRU 淘汰后仍必须按密钥字节取对句柄
--   ③ 重连（新设备会话）→ 旧句柄已随会话失效，必须重新导入后解密正确
--   ④ KEK 轮换（ALTER KEK）→ 换了 KEK 也不能命中旧句柄（DEK 重加密后仍可解）
-- ============================================================

-- 清理（DEK 随 KEK CASCADE 一起删，避免每把 DEK 一行 NOTICE）
DROP TABLE IF EXISTS kc_multi CASCADE;
DROP TABLE IF EXISTS kc_many CASCADE;
DROP KEK IF EXISTS kc_kek CASCADE;

-- 设备 KEK：预置 kek-1.key / hsm:id=1（SoftSDF 与真机同口径）
CREATE KEK kc_kek WITH (
    key_store = 'swxakms',
    key_path = 'hsm:id=1'
);

-- ------------------------------------------------------------
-- ① 同一 DEK 反复加解密：句柄复用路径
--    20 个值共享 kc_d，除首个值外都应命中已缓存的句柄。
-- ------------------------------------------------------------
SELECT '=== 1) 同一 DEK 的 20 个值 ===' AS stage;

CREATE DEK kc_d WITH VALUES (KEK = kc_kek);
CREATE TABLE kc_many (
    id INT,
    c  INT4_ENC ENCRYPTED WITH (DEK = kc_d, SUPPORT_ALGO = 'DET')
);

INSERT INTO kc_many (id, c) VALUES
    (1, 1001), (2, 1002), (3, 1003), (4, 1004), (5, 1005),
    (6, 1006), (7, 1007), (8, 1008), (9, 1009), (10, 1010),
    (11, 1011), (12, 1012), (13, 1013), (14, 1014), (15, 1015),
    (16, 1016), (17, 1017), (18, 1018), (19, 1019), (20, 1020);

-- 全表解密（每个值都要与写入值一致）
SELECT id, c FROM kc_many ORDER BY id;

-- 再读一遍：本段句柄应已全部在缓存里（纯命中路径）
SELECT id, c FROM kc_many WHERE id > 18 ORDER BY id;

-- ------------------------------------------------------------
-- ② 17 个 DEK 交叉：超过 16 槽 → 触发 LRU 淘汰后仍需取对句柄
--    17 列各绑定自己的 DEK，一次查询里交叉使用；把 16 槽缓存顶满并淘汰
--    最早使用的条目。第二遍读取时被淘汰的 DEK 需重新导入。
-- ------------------------------------------------------------
SELECT '=== 2) 17 个 DEK 交叉（超过 16 槽，触发淘汰）===' AS stage;

CREATE DEK kc_d01 WITH VALUES (KEK = kc_kek);
CREATE DEK kc_d02 WITH VALUES (KEK = kc_kek);
CREATE DEK kc_d03 WITH VALUES (KEK = kc_kek);
CREATE DEK kc_d04 WITH VALUES (KEK = kc_kek);
CREATE DEK kc_d05 WITH VALUES (KEK = kc_kek);
CREATE DEK kc_d06 WITH VALUES (KEK = kc_kek);
CREATE DEK kc_d07 WITH VALUES (KEK = kc_kek);
CREATE DEK kc_d08 WITH VALUES (KEK = kc_kek);
CREATE DEK kc_d09 WITH VALUES (KEK = kc_kek);
CREATE DEK kc_d10 WITH VALUES (KEK = kc_kek);
CREATE DEK kc_d11 WITH VALUES (KEK = kc_kek);
CREATE DEK kc_d12 WITH VALUES (KEK = kc_kek);
CREATE DEK kc_d13 WITH VALUES (KEK = kc_kek);
CREATE DEK kc_d14 WITH VALUES (KEK = kc_kek);
CREATE DEK kc_d15 WITH VALUES (KEK = kc_kek);
CREATE DEK kc_d16 WITH VALUES (KEK = kc_kek);
CREATE DEK kc_d17 WITH VALUES (KEK = kc_kek);

CREATE TABLE kc_multi (
    id  INT,
    c01 INT4_ENC ENCRYPTED WITH (DEK = kc_d01, SUPPORT_ALGO = 'DET'),
    c02 INT4_ENC ENCRYPTED WITH (DEK = kc_d02, SUPPORT_ALGO = 'DET'),
    c03 INT4_ENC ENCRYPTED WITH (DEK = kc_d03, SUPPORT_ALGO = 'DET'),
    c04 INT4_ENC ENCRYPTED WITH (DEK = kc_d04, SUPPORT_ALGO = 'DET'),
    c05 INT4_ENC ENCRYPTED WITH (DEK = kc_d05, SUPPORT_ALGO = 'DET'),
    c06 INT4_ENC ENCRYPTED WITH (DEK = kc_d06, SUPPORT_ALGO = 'DET'),
    c07 INT4_ENC ENCRYPTED WITH (DEK = kc_d07, SUPPORT_ALGO = 'DET'),
    c08 INT4_ENC ENCRYPTED WITH (DEK = kc_d08, SUPPORT_ALGO = 'DET'),
    c09 INT4_ENC ENCRYPTED WITH (DEK = kc_d09, SUPPORT_ALGO = 'DET'),
    c10 INT4_ENC ENCRYPTED WITH (DEK = kc_d10, SUPPORT_ALGO = 'DET'),
    c11 INT4_ENC ENCRYPTED WITH (DEK = kc_d11, SUPPORT_ALGO = 'DET'),
    c12 INT4_ENC ENCRYPTED WITH (DEK = kc_d12, SUPPORT_ALGO = 'DET'),
    c13 INT4_ENC ENCRYPTED WITH (DEK = kc_d13, SUPPORT_ALGO = 'DET'),
    c14 INT4_ENC ENCRYPTED WITH (DEK = kc_d14, SUPPORT_ALGO = 'DET'),
    c15 INT4_ENC ENCRYPTED WITH (DEK = kc_d15, SUPPORT_ALGO = 'DET'),
    c16 INT4_ENC ENCRYPTED WITH (DEK = kc_d16, SUPPORT_ALGO = 'DET'),
    c17 INT4_ENC ENCRYPTED WITH (DEK = kc_d17, SUPPORT_ALGO = 'DET')
);

INSERT INTO kc_multi (id, c01, c02, c03, c04, c05, c06, c07, c08, c09,
                      c10, c11, c12, c13, c14, c15, c16, c17)
VALUES (1, 201, 202, 203, 204, 205, 206, 207, 208, 209,
        210, 211, 212, 213, 214, 215, 216, 217);

-- 第一遍：17 个 DEK 全部导入，超槽淘汰
SELECT id, c01, c02, c03, c04, c05, c06, c07, c08, c09,
           c10, c11, c12, c13, c14, c15, c16, c17
FROM kc_multi;

-- 第二遍：被淘汰的 DEK 需重新导入，值仍须全部解对
SELECT id, c01, c02, c03, c04, c05, c06, c07, c08, c09,
           c10, c11, c12, c13, c14, c15, c16, c17
FROM kc_multi;

-- ------------------------------------------------------------
-- ③ 重连：新连接 = 新设备会话，旧会话的密钥句柄必须已失效，
--    重新导入后两表仍能全部解密（句柄泄漏/串用会在此暴露）
-- ------------------------------------------------------------
SELECT '=== 3) 重连后重新解密 ===' AS stage;

-- ReqID:TECC4.0 - 重连横幅会打印当前机器用户名（as user "xxx"），使期望文件依赖
-- 运行环境（作者机上是 wxp、别人机上是 wxp2）；这里用 QUIET 只静掉这条提示，
-- 重连本身仍是同库同用户的新连接，「新连接 = 新设备会话」的语义不变。
\set QUIET on
\c
\set QUIET off

SELECT id, c FROM kc_many ORDER BY id;

SELECT id, c01, c02, c03, c04, c05, c06, c07, c08, c09,
           c10, c11, c12, c13, c14, c15, c16, c17
FROM kc_multi;

-- ------------------------------------------------------------
-- ④ KEK 轮换：DEK 用新 KEK 重加密，此前缓存的句柄不能顶替新密钥
-- ------------------------------------------------------------
SELECT '=== 4) KEK 轮换到 hsm:id=2 后解密 ===' AS stage;

ALTER KEK kc_kek WITH (KEY_PATH = 'hsm:id=2');

SELECT id, c FROM kc_many ORDER BY id;

SELECT id, c01, c17 FROM kc_multi;

-- 轮换后写入新值并读回
INSERT INTO kc_many (id, c) VALUES (21, 1021);
SELECT id, c FROM kc_many WHERE id = 21;

-- 清理（库内不留残留，避免影响后续用例。逐把 DEK 显式删，避免 DROP KEK CASCADE
-- 打印一长串 "drop cascades to ..." DETAIL）
DROP TABLE kc_multi CASCADE;
DROP TABLE kc_many CASCADE;
DROP DEK kc_d CASCADE;
DROP DEK kc_d01 CASCADE;
DROP DEK kc_d02 CASCADE;
DROP DEK kc_d03 CASCADE;
DROP DEK kc_d04 CASCADE;
DROP DEK kc_d05 CASCADE;
DROP DEK kc_d06 CASCADE;
DROP DEK kc_d07 CASCADE;
DROP DEK kc_d08 CASCADE;
DROP DEK kc_d09 CASCADE;
DROP DEK kc_d10 CASCADE;
DROP DEK kc_d11 CASCADE;
DROP DEK kc_d12 CASCADE;
DROP DEK kc_d13 CASCADE;
DROP DEK kc_d14 CASCADE;
DROP DEK kc_d15 CASCADE;
DROP DEK kc_d16 CASCADE;
DROP DEK kc_d17 CASCADE;
DROP KEK kc_kek CASCADE;
