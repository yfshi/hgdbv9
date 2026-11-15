--
-- kek_dek_recreate_after_drop.sql — DROP KEK CASCADE 后同名重建，
--                                   旧会话写入的数据须仍能被新连接解密
--
-- 背景（Bug 9254）：
--   客户端派生密钥缓存（SM4-RND 用的 derived_enc_key / derived_auth_key）
--   原先按 DEK 名字匹配。DROP KEK ... CASCADE 连带删除 DEK 后，用同名重建
--   KEK/DEK，同一个会话里仍留着的【旧 DEK】缓存条目会被再次命中：
--   写入用旧 DEK 派生出的密钥加密，而目录里记的是新 DEK；于是本会话读写
--   自洽，任何新连接解密时 HMAC-SM3 校验不过，报
--   "Batch decrypt: item N failed"，数据永久不可读。
--
-- 判定点：
--   1) 写入发生在"持有旧 DEK 的会话"里（第 2 段，未重连）；
--   2) 读取发生在"新连接"里（第 3 段 ＼connect）。
-- 修复前第 3 段必然报 Batch decrypt；修复后应正常返回 30/40。
-- 注意两代 KEK 必须使用【不同的密钥文件】，否则旧密钥文件也能解开新 DEK，
-- 掩盖问题。
-- ============================================================

DROP TABLE IF EXISTS bug9254_t CASCADE;
DROP KEK IF EXISTS bug9254_k CASCADE;
\! rm -f "$TECC_KEK_PATH/bug9254_k1.priv" "$TECC_KEK_PATH/bug9254_k1.pub" 2>/dev/null
\! rm -f "$TECC_KEK_PATH/bug9254_k2.priv" "$TECC_KEK_PATH/bug9254_k2.pub" 2>/dev/null
\! tecc_ktool generate --key-name bug9254_k1 --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1
\! tecc_ktool generate --key-name bug9254_k2 --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1

SELECT '=== 1 第一代：建 KEK/DEK/表 + 写入（喂给旧会话一份 DEK 缓存） ===';

CREATE KEK bug9254_k WITH (
    key_store = 'localkms',
    key_algo  = 'sm2',
    key_path  = '%KEKPATH%/bug9254_k1.priv'
);
CREATE DEK bug9254_d WITH VALUES(KEK = 'bug9254_k');
CREATE TABLE bug9254_t (
    id  int,
    c   int4_enc ENCRYPTED WITH (DEK = 'public.bug9254_d', SUPPORT_ALGO = 'det')
);
INSERT INTO bug9254_t VALUES (1, 10), (2, 20);
SELECT 'gen1' AS t, id, c FROM bug9254_t ORDER BY id;

SELECT '=== 2 DROP + 同名重建（换另一对密钥文件），仍在旧会话里写入 ===';

DROP TABLE bug9254_t;
DROP KEK bug9254_k CASCADE;
CREATE KEK bug9254_k WITH (
    key_store = 'localkms',
    key_algo  = 'sm2',
    key_path  = '%KEKPATH%/bug9254_k2.priv'
);
CREATE DEK bug9254_d WITH VALUES(KEK = 'bug9254_k');
CREATE TABLE bug9254_t (
    id  int,
    c   int4_enc ENCRYPTED WITH (DEK = 'public.bug9254_d', SUPPORT_ALGO = 'det')
);
INSERT INTO bug9254_t VALUES (3, 30), (4, 40);
SELECT 'gen2 same session' AS t, id, c FROM bug9254_t ORDER BY id;

SELECT '=== 3 新连接读取（修复前：Batch decrypt: item N failed） ===';

-- 重连以获得全新连接级密钥缓存；QUIET 只用来压掉带用户名的连接横幅，
-- 使期望输出与运行账号无关
\set QUIET on
\c postgres postgres
\set QUIET off
SELECT 'gen2 fresh conn' AS t, id, c FROM bug9254_t ORDER BY id;

SELECT '=== 99 CLEANUP ===';

DROP TABLE bug9254_t;
DROP KEK bug9254_k CASCADE;
\! rm -f "$TECC_KEK_PATH/bug9254_k1.priv" "$TECC_KEK_PATH/bug9254_k1.pub" 2>/dev/null
\! rm -f "$TECC_KEK_PATH/bug9254_k2.priv" "$TECC_KEK_PATH/bug9254_k2.pub" 2>/dev/null
