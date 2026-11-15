--
-- he_dek_recreate_after_drop.sql — DROP DEK 后同名重建（全新 HE 密钥对），
--                                  旧会话写入的数据须仍能被其他进程解密
--
-- 背景（Bug 9263 的第二处触发路径，同一个缓存）：
--   crypto scheduler 里有一份进程级 HE 元数据缓存 tecc_he_meta_cache，按
--   【DEK 名】索引，缓存的是 hg_dek_args 中 he_*_pubkey / he_*_privkey 的
--   原始 hex。原先只有 DROP KEK 那条路径顺带清过它，DROP DEK / CREATE DEK
--   都不清。
--
--   于是"DROP DEK → 同名重建"之后（新 DEK = 新 OID + 新 HE 密钥对），本进程
--   继续用【旧 DEK 的公钥】加密、【旧 DEK 的私钥】解密：本会话读写自洽，
--   看起来完全正常；但目录里记的是新 DEK —— 换任何其他进程来读都是
--   "HE result out of range"，写进去的数据只有那把已被删掉的旧密钥能解。
--   严重程度与 Bug 9254 同级：静默用错密钥写数据，事后不可恢复。
--
-- 判定点：
--   第 2 段（DROP + 同名重建后）的写入，发生在"缓存里还留着旧 DEK 材料"
--   的进程里；第 3 段必须用 \! 另起一个 psql 进程去读 ——
--   \connect 重连不会清进程级缓存，测不出问题。
--   修复前第 3 段报 "HE result out of range for column"，修复后返回 15。
-- ============================================================

DROP TABLE IF EXISTS bug9263b_t CASCADE;
DROP KEK IF EXISTS bug9263b_k CASCADE;
\! rm -f "$TECC_KEK_PATH/bug9263b_k.priv" "$TECC_KEK_PATH/bug9263b_k.pub" 2>/dev/null
\! tecc_ktool generate --key-name bug9263b_k --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1

SELECT '=== 1 第一代 DEK：建表写入并读一次（喂满 HE 元数据缓存） ===';

CREATE KEK bug9263b_k WITH (
    key_store = 'localkms',
    key_algo  = 'sm2',
    key_path  = '%KEKPATH%/bug9263b_k.priv'
);
CREATE DEK bug9263b_sm4 WITH VALUES(KEK = 'bug9263b_k', KEY_ALGO = 'sm4');
CREATE DEK bug9263b_add WITH VALUES(KEK = 'bug9263b_k', KEY_ALGO = 'he-add');
CREATE TABLE bug9263b_t (
    id  int,
    c   int4_enc ENCRYPTED WITH (DEK = 'public.bug9263b_sm4', HE_ADD_DEK = 'public.bug9263b_add',
                                 SUPPORT_ALGO = 'det, he-add')
);
INSERT INTO bug9263b_t VALUES (1, 10);
SELECT 'gen1 same session' AS t, c + 5 FROM bug9263b_t;

-- 记下第一代 HE 公钥指纹，用于确认重建后确实换了密钥对
DROP TABLE IF EXISTS bug9263b_pub;
CREATE TABLE bug9263b_pub (md5_before text);
INSERT INTO bug9263b_pub
SELECT md5(a.value) FROM hg_dek_args a
WHERE a.dek_id = (SELECT oid FROM hg_dek WHERE dek_name = 'bug9263b_add')
  AND a.key = 'he_ec_pubkey';

SELECT '=== 2 DROP DEK + 同名重建，仍在同一个进程里写入 ===';

DROP TABLE bug9263b_t;
DROP DEK bug9263b_add;
CREATE DEK bug9263b_add WITH VALUES(KEK = 'bug9263b_k', KEY_ALGO = 'he-add');

-- 同名重建后 HE 密钥对确实换了（公钥指纹变了）
SELECT count(*) = 1 AS he_keypair_rotated
FROM hg_dek_args a JOIN bug9263b_pub p ON true
WHERE a.dek_id = (SELECT oid FROM hg_dek WHERE dek_name = 'bug9263b_add')
  AND a.key = 'he_ec_pubkey'
  AND md5(a.value) <> p.md5_before;

CREATE TABLE bug9263b_t (
    id  int,
    c   int4_enc ENCRYPTED WITH (DEK = 'public.bug9263b_sm4', HE_ADD_DEK = 'public.bug9263b_add',
                                 SUPPORT_ALGO = 'det, he-add')
);
INSERT INTO bug9263b_t VALUES (1, 10);

-- 本会话自洽（修复前也是 15，所以这句不是判定点，只是说明"看着正常"）
SELECT 'gen2 same session' AS t, c + 5 FROM bug9263b_t;

SELECT '=== 3 另起进程读取（判定点：修复前报 HE result out of range） ===';

\! psql -A -t -k on -c "SELECT 'gen2 fresh proc' AS t, c + 5 FROM bug9263b_t;" 2>&1

SELECT '=== 99 CLEANUP ===';

DROP TABLE bug9263b_t;
DROP TABLE bug9263b_pub;
DROP KEK bug9263b_k CASCADE;
\! rm -f "$TECC_KEK_PATH/bug9263b_k.priv" "$TECC_KEK_PATH/bug9263b_k.pub" 2>/dev/null
