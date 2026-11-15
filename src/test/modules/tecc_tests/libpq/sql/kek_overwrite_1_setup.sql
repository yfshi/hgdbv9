--
-- kek_overwrite.sql — KEK 文件被 tecc_ktool --force 覆盖后 DEK 失效行为回归
--
-- 覆盖场景：密钥文件被强制覆盖（绕过 ALTER KEK 正常轮换），DEK 无法再解密。
-- KEK 明文是连接级缓存（连接建立/首次使用加载一次），文件被覆盖后必须
-- 重连才能加载新密钥。因此拆成 3 个文件，各占一个独立 psql 连接：
--   kek_overwrite_1_setup.sql   创建 KEK/DEK/加密表，写入数据验证可读，
--                               备份原密钥文件，再用 --force 覆盖密钥文件
--   kek_overwrite_2_verify.sql  新连接（加载被覆盖的 KEK）：
--                               SELECT → 密文、INSERT → 报错
--   kek_overwrite_3_restore.sql 恢复原密钥文件，新连接 SELECT → 明文（数据未丢），清理
--

-- ============================================================
-- 准备：幂等清理
-- ============================================================
DROP TABLE IF EXISTS ovr_test CASCADE;
DROP DEK IF EXISTS dek_ovr CASCADE;
DROP KEK IF EXISTS kek_ovr CASCADE;
\! rm -f "$TECC_KEK_PATH/kek_ovr.key" "$TECC_KEK_PATH/kek_ovr_orig.key" 2>/dev/null
\! tecc_ktool generate --key-name kek_ovr --key-algo sm4 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1

-- ============================================================
-- 1. 创建对称 KEK + DEK + 加密表
-- ============================================================
SELECT '=== 1 CREATE KEK + DEK + TABLE ===';

CREATE KEK kek_ovr WITH (
    key_store = 'localkms',
    key_path = '%KEKPATH%/kek_ovr.key'
);
CREATE DEK dek_ovr WITH VALUES(KEK = 'kek_ovr');
CREATE TABLE ovr_test (
    id    SERIAL,
    name  TEXT_ENC ENCRYPTED WITH (DEK = 'public.dek_ovr', SUPPORT_ALGO = 'DET,SEARCH')
);

-- ============================================================
-- 2. 写入数据 + 正常读（正对照：覆盖前可解密）
-- ============================================================
SELECT '=== 2 INSERT + SELECT (baseline) ===';

INSERT INTO ovr_test (name) VALUES ('hello'), ('world');
SELECT id, name FROM ovr_test ORDER BY id;

-- ============================================================
-- 3. 备份原密钥文件，再用 tecc_ktool --force 覆盖
--    （模拟密钥文件被替换/误操作覆盖，绕过 ALTER KEK 正常轮换）
-- ============================================================
SELECT '=== 3 backup + force overwrite KEK file ===';

\! cp "$TECC_KEK_PATH/kek_ovr.key" "$TECC_KEK_PATH/kek_ovr_orig.key"
\! tecc_ktool generate --key-name kek_ovr --key-algo sm4 --key-path "$TECC_KEK_PATH" --force >/dev/null 2>&1
