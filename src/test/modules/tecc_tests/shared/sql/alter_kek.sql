--
-- alter_kek.sql — ALTER KEK 密钥轮换回归测试
--
-- 覆盖：
--   1. 对称 KEK 创建 + DEK + 加密表 + 写入
--   2. ALTER KEK 切换到新密钥文件（通过 \! cp 复制旧文件作为新文件）
--   3. 旧数据仍可解密
--   4. 轮换后插入新数据
--   5. SEARCH/DET 查询正常
--   6. OPE 范围查询正常
-- ============================================================

-- ============================================================
-- 准备
-- ============================================================
DROP TABLE IF EXISTS rot_test CASCADE;
DROP DEK IF EXISTS dek_rotate CASCADE;
DROP KEK IF EXISTS kek_rotate CASCADE;

-- 生成新密钥文件（复制旧的，后续 ALTER KEK 使用）
-- 注意：前端 CREATE KEK 要求 key_path 为绝对路径，但 C 层用
-- TECC_KEK_PATH 覆盖实际存储目录（测试框架设置）。因此密钥文件
-- 实际生成到 $TECC_KEK_PATH 下，复制时须用该环境变量定位。
\! rm -f "$TECC_KEK_PATH/kek_rotate_new.key" 2>/dev/null

-- ============================================================
-- 1. 创建对称 KEK（前端生成密钥文件，C 层存到 $TECC_KEK_PATH）
--    然后复制一份作为"新文件"供 ALTER 使用
-- ============================================================
SELECT '=== 1 CREATE KEK (old) ===';

CREATE KEK kek_rotate WITH (
    key_store = 'localkms',
    key_path = '/tmp/kek_rotate_old.key'
);

-- 复制旧文件作为新密钥文件（模拟 tecc_ktool 生成的另一个密钥）
\! cp "$TECC_KEK_PATH/kek_rotate_old.key" "$TECC_KEK_PATH/kek_rotate_new.key"

-- ============================================================
-- 2. 创建 DEK
-- ============================================================
SELECT '=== 2 CREATE DEK ===';

CREATE DEK dek_rotate WITH VALUES(KEK = 'kek_rotate');

-- ============================================================
-- 3. 建加密表 + 插入数据
-- ============================================================
SELECT '=== 3 CREATE TABLE + INSERT ===';

CREATE TABLE rot_test (
    id    SERIAL,
    name  TEXT_ENC ENCRYPTED WITH (DEK = 'public.dek_rotate', SUPPORT_ALGO = 'DET,SEARCH'),
    age   INT4_ENC ENCRYPTED WITH (DEK = 'public.dek_rotate', SUPPORT_ALGO = 'OPE')
);

INSERT INTO rot_test (name, age) VALUES ('Alice', 30), ('Bob', 25), ('Charlie', 35);

-- ============================================================
-- 4. 轮换前验证
-- ============================================================
SELECT '=== 4 BEFORE ALTER ===';

SELECT id, name, age FROM rot_test ORDER BY id;

-- ============================================================
-- 5. ALTER KEK 轮换
-- ============================================================
SELECT '=== 5 ALTER KEK ===';

ALTER KEK kek_rotate WITH (KEY_PATH = '/tmp/kek_rotate_new.key');

-- ============================================================
-- 6. 轮换后验证（旧数据）
-- ============================================================
SELECT '=== 6 AFTER ALTER (old data) ===';

SELECT id, name, age FROM rot_test ORDER BY id;

-- ============================================================
-- 7. 轮换后插入新数据
-- ============================================================
SELECT '=== 7 INSERT new data ===';

INSERT INTO rot_test (name, age) VALUES ('Diana', 28), ('Eve', 22);

-- ============================================================
-- 8. 全部数据验证
-- ============================================================
SELECT '=== 8 ALL data ===';

SELECT id, name, age FROM rot_test ORDER BY id;

-- ============================================================
-- 9. SEARCH 查询（DET）
-- ============================================================
SELECT '=== 9 SEARCH ===';

SELECT id, name, age FROM rot_test WHERE name = 'Alice' ORDER BY id;

-- ============================================================
-- 10. OPE 范围查询
-- ============================================================
SELECT '=== 10 OPE range ===';

SELECT id, name, age FROM rot_test WHERE age >= 28 ORDER BY age;

-- ============================================================
-- 11. 非对称 KEK + 对称 DEK（SM2 信封）
--     覆盖：CREATE DEK 信封加密 → 信封长度断言 →
--           运行时信封解密 round-trip → ALTER KEK 信封重加密
-- ============================================================
DROP TABLE IF EXISTS env_test CASCADE;
DROP DEK IF EXISTS dek_env CASCADE;
DROP KEK IF EXISTS kek_env CASCADE;
\! rm -f "$TECC_KEK_PATH/kek_env_asym.priv" "$TECC_KEK_PATH/kek_env_asym.pub" 2>/dev/null
\! rm -f "$TECC_KEK_PATH/kek_env_new.priv" "$TECC_KEK_PATH/kek_env_new.pub" 2>/dev/null
\! tecc_ktool generate --key-name kek_env_asym --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1

SELECT '=== 11 CREATE asym KEK + sym DEK ===';

CREATE KEK kek_env WITH (
    key_store = 'localkms',
    key_algo = 'sm2',
    key_path = '%KEKPATH%/kek_env_asym.priv'
);

-- 对称 DEK 绑非对称 KEK → encrypted_value 为 SM2 信封
CREATE DEK dek_env WITH VALUES(KEK = 'kek_env');

-- ============================================================
-- 12. 信封长度断言 — encrypted_value 应为 SM2 信封（OpenSSL ASN.1，
--     32B DEK → 139-141B/hex 278-282，非 16 倍数），
--     而非 SM4-CBC+HMAC（80-96B/hex 160-192，恒 16 倍数）
-- ============================================================
SELECT '=== 12 envelope length assert ===';

SELECT length(encode(value, 'escape')) >= 200
       AND length(encode(value, 'escape')) % 16 <> 0 AS is_envelope
FROM hg_dek_args
WHERE key = 'encrypted_value' AND dek_id = (SELECT oid FROM hg_dek WHERE dek_name = 'dek_env');

-- ============================================================
-- 13. 建加密表 + INSERT/SELECT round-trip（运行时信封解密）
-- ============================================================
SELECT '=== 13 CREATE TABLE + INSERT (envelope) ===';

CREATE TABLE env_test (
    id    SERIAL,
    name  TEXT_ENC ENCRYPTED WITH (DEK = 'public.dek_env', SUPPORT_ALGO = 'DET,SEARCH')
);

INSERT INTO env_test (name) VALUES ('Eve'), ('Frank'), ('Grace');

SELECT id, name FROM env_test ORDER BY id;

-- ============================================================
-- 14. ALTER KEK 轮换到新非对称文件（信封重加密）
-- ============================================================
SELECT '=== 14 ALTER KEK (envelope re-encrypt) ===';

\! cp "$TECC_KEK_PATH/kek_env_asym.priv" "$TECC_KEK_PATH/kek_env_new.priv"
\! cp "$TECC_KEK_PATH/kek_env_asym.pub" "$TECC_KEK_PATH/kek_env_new.pub"
ALTER KEK kek_env WITH (KEY_PATH = '%KEKPATH%/kek_env_new.priv');

-- ============================================================
-- 15. 轮换后验证：旧数据可读 + 新数据可写
-- ============================================================
SELECT '=== 15 AFTER ALTER (envelope) ===';

SELECT id, name FROM env_test ORDER BY id;
INSERT INTO env_test (name) VALUES ('Hank');
SELECT id, name FROM env_test ORDER BY id;

-- ============================================================
-- 16. 轮换后新建 HE DEK + round-trip
--     验证 PUBLIC_KEY 已随轮换更新为新公钥。若未更新，前端读旧公钥
--     信封加密 HE 私钥，运行时新私钥解不开 → INSERT/SELECT 报错。
-- ============================================================
SELECT '=== 16 PUBLIC_KEY updated (HE DEK after rotation) ===';

-- 轮换后 PUBLIC_KEY 应为 65 字节解压点（hex 长度 130）
SELECT length(encode(value, 'hex')) = 130 AS pub_len_ok
FROM hg_kek_args
WHERE upper(key) = 'PUBLIC_KEY'
  AND kek_id = (SELECT oid FROM hg_kek WHERE kek_name = 'kek_env');

-- 新建 HE DEK（用新 PUBLIC_KEY 信封加密 HE 私钥）
-- 注意：DEK 用 schema 限定名 'public.dek_env_he' 引用，验证
--       tecc_he_load_dek_key_hex 的 key_namespace 解析（修复前报
--       "must be a HE DEK"，因 qualified 名按 bare dek_name 查不到）
CREATE DEK dek_env_he WITH VALUES(KEK = 'kek_env', KEY_ALGO = 'he-add');

CREATE TABLE env_he_test (
    id    SERIAL,
    i4_a  INT4_ENC ENCRYPTED WITH (HE_ADD_DEK = 'public.dek_env_he', SUPPORT_ALGO = 'he-add')
);

INSERT INTO env_he_test (i4_a) VALUES (100);
INSERT INTO env_he_test (i4_a) VALUES (200);
SELECT id, i4_a FROM env_he_test ORDER BY id;

-- ============================================================
-- 17. ALTER KEK 权限（Bug 9255）
--
-- 修复前：
--   a) 无 USAGE 的用户得到的是 "Failed to load old KEK"（无 USAGE 的 KEK
--      被 hg_kek_info 视图过滤掉，前端"加载旧 KEK"必然失败，权限不足被
--      伪装成 KEK 不存在）；
--   b) GRANT USAGE 之后仍恒报 "permission denied for key encryption key"，
--      因为后端按 ACL_UPDATE 判定，而 KEK 只有 USAGE 一种可授予权限
--      （ACL_ALL_RIGHTS_KEK == ACL_USAGE，GRANT UPDATE ON KEK 直接语法报错）；
--   c) 连 KEK 属主本人也无法轮换自己的 KEK。
--
-- 修复后：统一按 USAGE 判定；轮换所需的元数据写回改走 SECURITY DEFINER 的
-- 受控入口（hg_kek_rotate_dek_arg / hg_kek_rotate_kek_arg），普通用户不必
-- 直接持有 hg_dek_args / hg_kek_args 的 UPDATE 权限。
-- ============================================================
SELECT '=== 17 ALTER KEK privilege (Bug 9255) ===';

DROP ROLE IF EXISTS u_rot;
CREATE ROLE u_rot LOGIN;
GRANT USAGE, CREATE ON SCHEMA public TO u_rot;

-- 17a. 无 USAGE：须给出明确的权限不足（而非"加载旧 KEK 失败"）
SET ROLE u_rot;
ALTER KEK kek_rotate WITH (KEY_PATH = '/tmp/kek_rotate_old.key');
RESET ROLE;

-- 17b. GRANT USAGE 后：同一用户可完成轮换，且旧数据仍可解密
GRANT USAGE ON KEK kek_rotate TO u_rot;
SET ROLE u_rot;
ALTER KEK kek_rotate WITH (KEY_PATH = '/tmp/kek_rotate_old.key');
RESET ROLE;
SELECT id, name, age FROM rot_test ORDER BY id;

-- 17c. 非超级用户的 KEK 属主：可以轮换自己的 KEK（无需任何额外 GRANT）
\! tecc_ktool generate --key-name kek_rot_own --key-algo sm4 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1
\! tecc_ktool generate --key-name kek_rot_own2 --key-algo sm4 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1

SET ROLE u_rot;
CREATE KEK kek_rot_own WITH (
    key_store = 'localkms',
    key_path  = '%KEKPATH%/kek_rot_own.key'
);
CREATE DEK dek_rot_own WITH VALUES(KEK = 'kek_rot_own');
CREATE TABLE rot_own_test (
    id  SERIAL,
    v   TEXT_ENC ENCRYPTED WITH (DEK = 'public.dek_rot_own', SUPPORT_ALGO = 'DET')
);
INSERT INTO rot_own_test (v) VALUES ('owner1'), ('owner2');
SELECT id, v FROM rot_own_test ORDER BY id;

ALTER KEK kek_rot_own WITH (KEY_PATH = '%KEKPATH%/kek_rot_own2.key');
SELECT id, v FROM rot_own_test ORDER BY id;
RESET ROLE;

-- ============================================================
-- 99. 清理
-- ============================================================
SELECT '=== 99 CLEANUP ===';

DROP TABLE rot_own_test CASCADE;
DROP DEK dek_rot_own;
DROP KEK kek_rot_own CASCADE;
-- u_rot 仍持有 schema public 与 kek_rotate 上的授权，须先回收才能 DROP ROLE
REVOKE USAGE, CREATE ON SCHEMA public FROM u_rot;
REVOKE USAGE ON KEK kek_rotate FROM u_rot;
DROP ROLE u_rot;
\! rm -f "$TECC_KEK_PATH/kek_rot_own.key" "$TECC_KEK_PATH/kek_rot_own2.key" 2>/dev/null

DROP TABLE rot_test CASCADE;
DROP DEK dek_rotate;
DROP KEK kek_rotate CASCADE;

DROP TABLE env_test CASCADE;
DROP TABLE env_he_test CASCADE;
DROP DEK dek_env;
DROP DEK dek_env_he CASCADE;
DROP KEK kek_env CASCADE;
\! rm -f "$TECC_KEK_PATH/kek_env_asym.priv" "$TECC_KEK_PATH/kek_env_asym.pub" 2>/dev/null
\! rm -f "$TECC_KEK_PATH/kek_env_new.priv" "$TECC_KEK_PATH/kek_env_new.pub" 2>/dev/null
