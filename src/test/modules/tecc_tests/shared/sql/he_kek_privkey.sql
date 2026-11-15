-- ============================================================
-- he_kek_privkey.sql — 客户端有/无 KEK 私钥场景回归测试
-- ReqID:TECC4.0
-- 覆盖：持有 KEK(sm2) 私钥的客户端可创建 DEK、加解密；
--       无私钥客户端无法创建 DEK（CREATE DEK 报错，私钥=写 DEK 前置条件），
--       但可对已存在的 HE DEK 加密写入（公钥=写），无法解密（私钥=读）
-- ============================================================

-- 0. Setup（幂等清理 + 生成 sm2 KEK）
DROP TABLE IF EXISTS he_priv_t CASCADE;
DROP TABLE IF EXISTS he_priv_nopriv CASCADE;
DROP DEK IF EXISTS dek_priv_add CASCADE;
DROP DEK IF EXISTS dek_priv_add2 CASCADE;
DROP DEK IF EXISTS dek_priv_nopriv_add CASCADE;
DROP DEK IF EXISTS dek_priv_mul CASCADE;
DROP KEK IF EXISTS kek_priv CASCADE;

\! rm -f "$TECC_KEK_PATH/kek_priv.priv" "$TECC_KEK_PATH/kek_priv.pub" "$TECC_KEK_PATH/kek_priv.priv.bak" 2>/dev/null
\! tecc_ktool generate --key-name kek_priv --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1

CREATE KEK kek_priv WITH (
    key_store = 'localkms',
    key_algo = 'sm2',
    key_path = '%KEKPATH%/kek_priv.priv'
);
CREATE DEK dek_priv_add WITH VALUES(KEK = kek_priv, KEY_ALGO = 'he-add');
CREATE DEK dek_priv_mul WITH VALUES(KEK = kek_priv, KEY_ALGO = 'he-mul');

-- 预建一个 HE DEK 供"无私钥"客户端加密写入（创建需私钥，此处 .priv 仍在）
CREATE DEK dek_priv_add2 WITH VALUES(KEK = kek_priv, KEY_ALGO = 'he-add');

-- ============================================================
-- 1. 客户端 A（有 .priv）—— 加解密正常（基线）
-- ============================================================
SELECT '=== 1 with privkey ===';
CREATE TABLE he_priv_t (
    id SERIAL,
    a INT4_ENC ENCRYPTED WITH (HE_ADD_DEK = 'dek_priv_add', SUPPORT_ALGO = 'he-add'),
    m INT4_ENC ENCRYPTED WITH (HE_MUL_DEK = 'dek_priv_mul', SUPPORT_ALGO = 'he-mul')
);
INSERT INTO he_priv_t (a, m) VALUES (10, 3), (20, 5), (0, 0), (-5, -2);

-- 有私钥：SELECT 解密正常
SELECT id, a, m FROM he_priv_t ORDER BY id;

-- 有私钥：同态加法/减法/SUM/标量运算 解密正常
SELECT SUM(a) FROM he_priv_t;
SELECT a + 1 FROM he_priv_t ORDER BY id;
SELECT a - 2 FROM he_priv_t ORDER BY id;
SELECT m * 3 FROM he_priv_t ORDER BY id;

-- ============================================================
-- 2. 客户端 B（无 .priv）—— 不能建 DEK，能写不能读
-- ============================================================
SELECT '=== 2 without privkey ===';

-- 模拟无私钥客户端：重新连接清空 KEK 私钥缓存 + 改名 .priv 文件（公钥仍在服务端 hg_kek_args）
\c postgres postgres
\! mv "$TECC_KEK_PATH/kek_priv.priv" "$TECC_KEK_PATH/kek_priv.priv.bak" 2>/dev/null

-- 2a. 无私钥 CREATE DEK he-add → 报错（.priv 缺失，私钥不可用）
CREATE DEK dek_priv_nopriv_add WITH VALUES(KEK = kek_priv, KEY_ALGO = 'he-add');

-- 2b. 无私钥建表（复用已存在的 dek_priv_add2）+ INSERT 加密 → 成功（加密仅需公钥）
CREATE TABLE he_priv_nopriv (
    id SERIAL,
    a INT4_ENC ENCRYPTED WITH (HE_ADD_DEK = 'dek_priv_add2', SUPPORT_ALGO = 'he-add')
);
INSERT INTO he_priv_nopriv (a) VALUES (100), (200);

-- 2c. 无私钥 SELECT 解密 → 失败（无 KEK 私钥，无法解 HE 私钥信封）
SELECT id, a FROM he_priv_nopriv ORDER BY id;

-- 2d. 无私钥同态运算（SUM）→ 服务端运算成功，结果解密失败
SELECT SUM(a) FROM he_priv_nopriv;

-- ============================================================
-- 3. 恢复 .priv —— 解密恢复正常
-- ============================================================
SELECT '=== 3 restore privkey ===';
\! mv "$TECC_KEK_PATH/kek_priv.priv.bak" "$TECC_KEK_PATH/kek_priv.priv" 2>/dev/null

-- 恢复后 SELECT 解密正常（原无私钥时创建的 DEK/表数据可正常解密）
SELECT id, a FROM he_priv_nopriv ORDER BY id;
SELECT SUM(a) FROM he_priv_nopriv;

-- ============================================================
-- 4. Cleanup
-- ============================================================
DROP TABLE IF EXISTS he_priv_t CASCADE;
DROP TABLE IF EXISTS he_priv_nopriv CASCADE;
DROP DEK IF EXISTS dek_priv_add CASCADE;
DROP DEK IF EXISTS dek_priv_add2 CASCADE;
DROP DEK IF EXISTS dek_priv_nopriv_add CASCADE;
DROP DEK IF EXISTS dek_priv_mul CASCADE;
DROP KEK IF EXISTS kek_priv CASCADE;
