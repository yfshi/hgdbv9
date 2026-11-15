-- ============================================================
-- he_assign_pure_col.sql — 同态结果回写「已声明对称 DEK 的纯 HE 列」
-- ============================================================
-- 背景（回归登记册第 12 项）：27447d0a493 新增的写入点跨 DEK 校验，把目标列的
-- 「配置 DEK」只解析成主 DEK（DEK = ...）。但多 DEK 体系下，纯 HE 列可以同时
-- 声明 DEK（本列未启用对称算法，该 DEK 不参与加解密）与 HE_ADD_DEK；此时列上
-- 承载的密文只有 HE 段，其主体 DEK 身份是 HE_ADD_DEK，于是合法的 HE 结果回写
-- 被误判成跨 DEK 赋值而被拒：
--     UPDATE swxa_he_t SET c = a + a;
--     ERROR: cannot store ciphertext encrypted with a different DEK into column "c"
-- 本用例用 localkms（不依赖密码机）固定同一形态，同时固定反面「异 DEK 赋值仍须
-- 拒绝」，避免修复被放得过松。
--
-- DDL 合法性依据：多 DEK 模式下 create_stmt_processor.c 仅在列启用对称算法
-- （DET/OPE/SEARCH）时才要求 DEK 的 key_algo='sm4'；纯 he-add 列 + DEK=<sm4>
-- 属合法形态（DEK 与 HE_ADD_DEK 并存即多 DEK 模式，见该文件 §一列多 DEK 校验）。
-- ============================================================

-- 0. 幂等清理
DROP TABLE IF EXISTS he_apc_t CASCADE;
DROP TABLE IF EXISTS he_apc_neg CASCADE;
DROP DEK IF EXISTS dek_apc_sym CASCADE;
DROP DEK IF EXISTS dek_apc_add CASCADE;
DROP DEK IF EXISTS dek_apc_add2 CASCADE;
DROP KEK IF EXISTS kek_apc_sym CASCADE;
DROP KEK IF EXISTS kek_apc_he CASCADE;

-- 1. 密钥：对称 KEK/DEK（列的主 DEK）+ 非对称 KEK 下两个 he-add DEK
--    两个 he-add DEK 分别为「本列可用的 HE DEK」与「异 DEK 反面用例」。
\! rm -f "$TECC_KEK_PATH/kek_apc_sym.key" 2>/dev/null
\! tecc_ktool generate --key-name kek_apc_sym --key-algo sm4 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1
\! rm -f "$TECC_KEK_PATH/kek_apc_he.priv" "$TECC_KEK_PATH/kek_apc_he.pub" 2>/dev/null
\! tecc_ktool generate --key-name kek_apc_he --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1

CREATE KEK kek_apc_sym WITH (
    key_store = 'localkms',
    key_algo  = 'sm4',
    key_path  = '%KEKPATH%/kek_apc_sym.key'
);
CREATE KEK kek_apc_he WITH (
    key_store = 'localkms',
    key_algo  = 'sm2',
    key_path  = '%KEKPATH%/kek_apc_he.priv'
);
CREATE DEK dek_apc_sym  WITH VALUES(KEK = kek_apc_sym, KEY_ALGO = 'sm4');
CREATE DEK dek_apc_add  WITH VALUES(KEK = kek_apc_he,  KEY_ALGO = 'he-add');
CREATE DEK dek_apc_add2 WITH VALUES(KEK = kek_apc_he,  KEY_ALGO = 'he-add');

-- 2. 建表：纯 he-add 列，同时声明对称 DEK（缺陷触发形态）
SELECT '=== 1 CREATE TABLE（纯 he-add + 声明对称 DEK） ===';
CREATE TABLE he_apc_t (
    id INT4,
    c  INT4_ENC ENCRYPTED WITH (DEK = dek_apc_sym, HE_ADD_DEK = dek_apc_add,
                                SUPPORT_ALGO = 'he-add')
);

-- 3. 常量写入：主 DEK 身份取列的 DEK（对称 DEK）
SELECT '=== 2 常量写入 ===';
INSERT INTO he_apc_t (id, c) VALUES (1, 7), (2, 8);
SELECT id, c FROM he_apc_t ORDER BY id;

-- 4. 同态结果回写：值主体 DEK 身份取 HE_ADD_DEK（登记册第 12 项失败点）
SELECT '=== 3 同态结果回写 c = c + c ===';
UPDATE he_apc_t SET c = c + c;
SELECT id, c FROM he_apc_t ORDER BY id;

-- 5. 常量与同态列标量运算 / 同态聚合（确认回写后的值可继续参与同态运算）
SELECT '=== 4 标量运算 c + 1 ===';
SELECT id, c + 1 AS c_plus_1 FROM he_apc_t ORDER BY id;
SELECT '=== 5 同态聚合 sum(c) ===';
SELECT sum(c) AS sum_c FROM he_apc_t;

-- 6. 混合身份共存：回写后的值（主 DEK 身份 = HE DEK）与常量新写入的值
--    （主 DEK 身份 = 列主 DEK）同列共存。同态运算按【段内】DEK 身份校验
--    （he_operators.c 的 EG_ADD/EG_MUL 段标识），故两类身份共存不影响同态
--    运算；排序/去重走主 DEK 身份校验（cipherdata_operators.c），一并固定。
SELECT '=== 6 混合身份共存：常量写入 + 同态运算 ===';
INSERT INTO he_apc_t (id, c) VALUES (3, 9);
SELECT id, c FROM he_apc_t ORDER BY id;
SELECT sum(c) AS sum_mixed FROM he_apc_t;
SELECT id, c + 1 AS c_plus_1 FROM he_apc_t ORDER BY id;
UPDATE he_apc_t SET c = c + 1;
SELECT id, c FROM he_apc_t ORDER BY id;
SELECT '=== 7 混合身份下的排序（比较路径） ===';
SELECT id, c FROM he_apc_t ORDER BY c;

-- 7. 反面：异 DEK 的 HE 密文仍不得写入本列（跨 DEK 防护不可放开）
SELECT '=== 8 异 DEK 赋值应被拒绝 ===';
CREATE TABLE he_apc_neg (
    id INT4,
    c  INT4_ENC ENCRYPTED WITH (DEK = dek_apc_sym, HE_ADD_DEK = dek_apc_add,
                                SUPPORT_ALGO = 'he-add'),
    o  INT4_ENC ENCRYPTED WITH (HE_ADD_DEK = dek_apc_add2, SUPPORT_ALGO = 'he-add')
);
INSERT INTO he_apc_neg (id, o) VALUES (1, 100);
UPDATE he_apc_neg SET c = o WHERE id = 1;
SELECT id, c FROM he_apc_neg ORDER BY id;

DROP TABLE he_apc_t;
DROP TABLE he_apc_neg;
DROP DEK dek_apc_sym;
DROP DEK dek_apc_add;
DROP DEK dek_apc_add2;
DROP KEK kek_apc_sym;
DROP KEK kek_apc_he CASCADE;
