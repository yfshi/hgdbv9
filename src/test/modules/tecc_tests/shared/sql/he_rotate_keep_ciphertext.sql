--
-- he_rotate_keep_ciphertext.sql — ALTER KEK 轮换后，轮换前写入的密文
--                                 仍须能做 HE 加减/乘积运算
--
-- 背景（Bug 9263）：
--   HE 私钥以【KEK 公钥封装好的信封】形式存在 hg_dek_args（he_ec_privkey /
--   he_eg_privkey）。ALTER KEK 会用新 KEK 公钥重新封装这些信封，运行时再用
--   连接级缓存里的 KEK 明文解封。
--
--   但 crypto scheduler 里另有一份【进程级 16 项 HE 元数据缓存】，缓存的是
--   hg_dek_args 字段的原始 hex —— 对 HE 私钥而言就是那份旧信封。它既不被
--   tecc_cache_clear()（KEK/DEK/列元数据）清理，也不被 tecc_key_cache_clear()
--   （key_manager 的 DEK 明文）清理，只在 DROP KEK 时顺带清过。
--
--   于是"执行过轮换的那个进程"会继续拿【旧信封】配【新 KEK】解封：SM2 信封
--   不带完整性校验，解不出错而是静默产出一把错误的私钥，HE 运算结果变成垃圾
--   值，最终报 "HE result out of range for column ..."。另起一个 psql 进程
--   没有这份缓存，所以一切正常 —— 这正是该 Bug 只在"轮换前后同一客户端进程"
--   里出现的原因（注意：缓存是进程级的，＼connect 重连并不会清掉它）。
--
-- 判定点：
--   第 2 段（轮换前）先做一次 HE 运算，把旧信封灌进缓存；
--   第 5 段（轮换后）对同一批旧密文再做 HE 运算，必须仍得到正确值。
--   修复前此处报 "HE result out of range for column"，且第 6/7 段一并失败。
--   注意两代 KEK 必须使用【不同的密钥文件】，否则新旧 KEK 都能解开信封，
--   掩盖问题。
-- ============================================================

DROP TABLE IF EXISTS bug9263_t CASCADE;
DROP KEK IF EXISTS bug9263_k CASCADE;
\! rm -f "$TECC_KEK_PATH/bug9263_k1.priv" "$TECC_KEK_PATH/bug9263_k1.pub" 2>/dev/null
\! rm -f "$TECC_KEK_PATH/bug9263_k2.priv" "$TECC_KEK_PATH/bug9263_k2.pub" 2>/dev/null
\! tecc_ktool generate --key-name bug9263_k1 --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1
\! tecc_ktool generate --key-name bug9263_k2 --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1

SELECT '=== 1 建 KEK/DEK/表 + 写入 ===';

CREATE KEK bug9263_k WITH (
    key_store = 'localkms',
    key_algo  = 'sm2',
    key_path  = '%KEKPATH%/bug9263_k1.priv'
);
CREATE DEK bug9263_sm4 WITH VALUES(KEK = 'bug9263_k', KEY_ALGO = 'sm4');
CREATE DEK bug9263_add WITH VALUES(KEK = 'bug9263_k', KEY_ALGO = 'he-add');
CREATE DEK bug9263_mul WITH VALUES(KEK = 'bug9263_k', KEY_ALGO = 'he-mul');
CREATE TABLE bug9263_t (
    id  int,
    c1  int4_enc ENCRYPTED WITH (DEK = 'public.bug9263_sm4', HE_ADD_DEK = 'public.bug9263_add',
                                 SUPPORT_ALGO = 'det, he-add'),
    c2  int4_enc ENCRYPTED WITH (DEK = 'public.bug9263_sm4', HE_MUL_DEK = 'public.bug9263_mul',
                                 SUPPORT_ALGO = 'det, he-mul')
);
INSERT INTO bug9263_t VALUES (1, 1, 10), (2, 2, 20);
SELECT id, c1, c2 FROM bug9263_t ORDER BY id;

SELECT '=== 2 轮换前的 HE 运算（同时把旧信封灌进会话缓存） ===';

SELECT 'he-add before' AS t, bug9263_t.c1 + 5 FROM bug9263_t WHERE c2 = 20;
SELECT 'he-mul before' AS t, bug9263_t.id * bug9263_t.c2 FROM bug9263_t WHERE c2 = 20;

-- 记下轮换前 HE 密钥材料的指纹（只存指纹，不存密钥材料本身）
DROP TABLE IF EXISTS bug9263_meta;
CREATE TABLE bug9263_meta (dek_name text, key text, md5_before text);
INSERT INTO bug9263_meta
SELECT d.dek_name, a.key, md5(a.value)
FROM hg_dek_args a JOIN hg_dek d ON d.oid = a.dek_id
WHERE d.dek_name IN ('bug9263_add', 'bug9263_mul')
  AND a.key IN ('he_ec_privkey', 'he_ec_pubkey', 'he_eg_privkey', 'he_eg_pubkey');

SELECT '=== 3 ALTER KEK 轮换（换另一对密钥文件） ===';

ALTER KEK bug9263_k WITH (KEY_PATH = '%KEKPATH%/bug9263_k2.priv');

SELECT '=== 4 轮换后：私钥信封被重封，公钥保持不变 ===';

-- 私钥以【KEK 公钥封装的信封】存储，轮换必须整体换新
SELECT count(*) = 2 AS privkey_envelopes_rewrapped
FROM hg_dek_args a JOIN hg_dek d ON d.oid = a.dek_id
     JOIN bug9263_meta m ON m.dek_name = d.dek_name AND m.key = a.key
WHERE a.key IN ('he_ec_privkey', 'he_eg_privkey')
  AND md5(a.value) <> m.md5_before;

-- 公钥明文存储，HE 密钥对本身不变，故公钥一个字节都不该动
SELECT count(*) = 2 AS pubkeys_unchanged
FROM hg_dek_args a JOIN hg_dek d ON d.oid = a.dek_id
     JOIN bug9263_meta m ON m.dek_name = d.dek_name AND m.key = a.key
WHERE a.key IN ('he_ec_pubkey', 'he_eg_pubkey')
  AND md5(a.value) = m.md5_before;

SELECT '=== 5 轮换后：同一会话对【轮换前写入的密文】做 HE 运算 ===';

-- 修复前：ERROR: HE result out of range for column "c2"
SELECT 'he-mul after' AS t, bug9263_t.id * bug9263_t.c2 FROM bug9263_t WHERE c2 = 20;
SELECT 'he-add after' AS t, bug9263_t.c1 + 5 FROM bug9263_t WHERE c2 = 20;

SELECT '=== 6 轮换后：新写入的数据也正常 ===';

INSERT INTO bug9263_t VALUES (3, 3, 30);
SELECT 'he-mul new row' AS t, bug9263_t.id * bug9263_t.c2 FROM bug9263_t WHERE c2 = 30;

SELECT '=== 7 重连后读取（进程级缓存不随连接走） ===';

-- 注意：HE 元数据缓存是【进程级】的，不是连接级，所以这里的 ＼connect
-- 并不会让它失效 —— 修复前本段同样报 "HE result out of range"（换成
-- 另一个 psql 进程连接才会正常，见 bug 报告里的"新连接对照组"）。
-- QUIET 只用来压掉带用户名的连接横幅，使期望输出与运行账号无关
\set QUIET on
\c postgres postgres
\set QUIET off
SELECT 'he-mul reconnected' AS t, bug9263_t.id * bug9263_t.c2 FROM bug9263_t WHERE c2 = 20;

SELECT '=== 99 CLEANUP ===';

DROP TABLE bug9263_t;
DROP TABLE bug9263_meta;
DROP KEK bug9263_k CASCADE;
\! rm -f "$TECC_KEK_PATH/bug9263_k1.priv" "$TECC_KEK_PATH/bug9263_k1.pub" 2>/dev/null
\! rm -f "$TECC_KEK_PATH/bug9263_k2.priv" "$TECC_KEK_PATH/bug9263_k2.pub" 2>/dev/null
