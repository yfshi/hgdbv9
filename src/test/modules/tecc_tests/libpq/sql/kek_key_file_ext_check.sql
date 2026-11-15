-- ============================================================
-- kek_key_file_ext_check.sql — CREATE KEK KEY_ALGO 与密钥文件类型一致性回归
-- 背景（QA 报修）：CREATE KEK ... KEY_ALGO='sm4' 且 KEY_PATH 指向 tecc_ktool
-- generate 生成的 SM2 非对称密钥材料（.priv / .pub）时，预期失败（V4.0.1 概要
-- 设计 §7.1/§7.7：sm4 ↔ .key 对称密钥文件，sm2 ↔ .priv+.pub 非对称密钥对），
-- 实际却创建成功。
-- 修复：create_kek_processor.c 对称分支校验 key_path 扩展名必须为 .key。范围
-- 经产品定调：显式 KEY_ALGO='sm4' 与省略 KEY_ALGO（默认 sm4）**均强制** .key——
-- 省略却指向 .priv/.pub 属 sm2 材料，报自解释错误引导补 KEY_ALGO='sm2'，消除
-- "未标注 sm2 的 KEK 被当对称"歧义。显式 sm4 指向 .priv/.pub 报
-- ".key extension required for SM4 keys"。
-- 此处验证：sm4 + .key → 成功；sm4 + .priv / sm4 + .pub（文件真实存在，
-- 失败根因是"扩展名不匹配"而非"文件不存在"）→ 前端英文报错且不落库；
--           省略 KEY_ALGO + .priv → 引导性报错且不落库。
-- 本文件放 libpq/sql：仅 C 前端（tecc_processor）有此校验；JDBC 直发后端无。
-- 注意：key_path 用固定 /tmp 字面量（C 层经 TECC_KEK_PATH 覆盖到本回归 key
-- 目录）；密钥文件由 SQL 内 \! tecc_ktool generate 自生成到 $TECC_KEK_PATH
-- （对称 .key + 非对称 .priv/.pub 对），框架预生成逻辑见同一 key-name 即跳过。
-- ============================================================

-- 0. 幂等清理 + 自生成密钥文件（非对称对真实存在，避免"文件不存在"掩盖扩展名校验）
DROP KEK IF EXISTS kek_ext_sym CASCADE;
DROP KEK IF EXISTS kek_ext_badpriv CASCADE;
DROP KEK IF EXISTS kek_ext_badpub CASCADE;
DROP KEK IF EXISTS kek_ext_nodef CASCADE;
\! rm -f "$TECC_KEK_PATH/kek_ext_sym.key" "$TECC_KEK_PATH/kek_ext_asym.priv" "$TECC_KEK_PATH/kek_ext_asym.pub" 2>/dev/null
\! tecc_ktool generate --key-name kek_ext_sym --key-algo sm4 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1
\! tecc_ktool generate --key-name kek_ext_asym --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1

-- 1. 基线：KEY_ALGO='sm4' + .key 对称文件 → 创建成功
CREATE KEK kek_ext_sym WITH (
    key_store = 'localkms',
    key_algo = 'sm4',
    key_path = '/tmp/kek_ext_sym.key'
);

-- 2. KEY_ALGO='sm4' + .priv（真实 SM2 私钥文件）→ 前端报错，不落库（QA 步骤2）
CREATE KEK kek_ext_badpriv WITH (
    key_store = 'localkms',
    key_algo = 'sm4',
    key_path = '/tmp/kek_ext_asym.priv'
);

-- 3. KEY_ALGO='sm4' + .pub（真实 SM2 公钥文件）→ 前端报错，不落库（QA 步骤3）
CREATE KEK kek_ext_badpub WITH (
    key_store = 'localkms',
    key_algo = 'sm4',
    key_path = '/tmp/kek_ext_asym.pub'
);

-- 4. 省略 KEY_ALGO（默认 sm4）+ .priv（真实 SM2 私钥文件）→ 引导性报错，不落库（默认同强制 .key）
CREATE KEK kek_ext_nodef WITH (
    key_store = 'localkms',
    key_path = '/tmp/kek_ext_asym.priv'
);

-- 5. 校验落库结果：仅 kek_ext_sym 存在，三个失败 KEK 均未创建
SELECT kek_name FROM hg_kek
WHERE kek_name IN ('kek_ext_sym', 'kek_ext_badpriv', 'kek_ext_badpub', 'kek_ext_nodef') ORDER BY 1;

-- 6. 清理
DROP KEK kek_ext_sym CASCADE;
