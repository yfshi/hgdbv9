-- ============================================================
-- dek_pubfile_gate.sql — 删除 sm2 KEK 的 .pub 后禁建 DEK 回归
-- 背景（Bug 记录 L701）：CREATE KEK 之后任何路径都不再读 .pub 文件（公钥可
-- 由 .priv 派生 / 已存 hg_kek_args），故 tecc_ktool delete 掉 .pub 后 CREATE
-- DEK 仍成功——但按"密钥对文件完整性"策略（对齐 .priv 缺失禁建 DEK），.pub
-- 被删 = 密钥对不完整，应阻止继续创建绑定该 KEK 的 DEK。
-- 修复：tecc_verify_kek_pub_file() 在 CREATE DEK 时对 localkms+sm2 KEK 读
-- hg_kek_args 的 KEY_PATH，把 .priv 后缀替换成 .pub 后 stat，缺失即报
-- "KEK 'x' public key file is unavailable (localkms: ... missing)"。
-- 断言：删 .pub → 对称 DEK 绑非对称 KEK 与 HE DEK 均被拦截且不落库；
--        恢复 .pub → 两者创建成功且 DET 列可用。
-- 放 libpq/sql：仅 libpq C 前端（tecc_processor）有此校验。
-- ============================================================

-- 0. 幂等清理 + 自生成 sm2 密钥对（.pub 备份文件 .bak 用于恢复）
DROP TABLE IF EXISTS t_pubg CASCADE;
DROP DEK IF EXISTS dek_pubg_sym CASCADE;
DROP DEK IF EXISTS dek_pubg_add CASCADE;
DROP KEK IF EXISTS kek_pubg CASCADE;
\! rm -f "$TECC_KEK_PATH/kek_pubg.priv" "$TECC_KEK_PATH/kek_pubg.pub" "$TECC_KEK_PATH/kek_pubg.pub.bak" 2>/dev/null
\! tecc_ktool generate --key-name kek_pubg --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1

CREATE KEK kek_pubg WITH (
    key_store = 'localkms',
    key_algo = 'sm2',
    key_path = '%KEKPATH%/kek_pubg.priv'
);

-- 1. 删除 .pub（密钥对不完整；.priv 仍在）
\! cp "$TECC_KEK_PATH/kek_pubg.pub" "$TECC_KEK_PATH/kek_pubg.pub.bak"
\! rm -f "$TECC_KEK_PATH/kek_pubg.pub"

-- 2a. 对称 DEK 绑非对称 KEK（信封）→ 拦截（修复前成功 rc=0）
CREATE DEK dek_pubg_sym WITH VALUES (KEK = 'kek_pubg');
-- 2b. HE DEK → 同样拦截
CREATE DEK dek_pubg_add WITH VALUES (KEK = 'kek_pubg', KEY_ALGO = 'he-add');

-- 3. 两个失败 DEK 均未落库
SELECT 'not_persisted', count(*) AS n
FROM hg_dek WHERE dek_name IN ('dek_pubg_sym', 'dek_pubg_add');

-- 4. 恢复 .pub → 创建恢复成功
\! mv "$TECC_KEK_PATH/kek_pubg.pub.bak" "$TECC_KEK_PATH/kek_pubg.pub"
CREATE DEK dek_pubg_sym WITH VALUES (KEK = 'kek_pubg');
CREATE DEK dek_pubg_add WITH VALUES (KEK = 'kek_pubg', KEY_ALGO = 'he-add');

-- 5. 可用性：dek_pubg_sym 建 DET 列往返
CREATE TABLE t_pubg (
    id   INT,
    name TEXT_ENC ENCRYPTED WITH (DEK = 'dek_pubg_sym', SUPPORT_ALGO = 'det')
);
INSERT INTO t_pubg VALUES (1, 'alice'), (2, 'bob');
SELECT id, name FROM t_pubg WHERE name = 'alice';

-- 6. 清理
DROP TABLE t_pubg CASCADE;
DROP DEK dek_pubg_sym CASCADE;
DROP DEK dek_pubg_add CASCADE;
DROP KEK kek_pubg CASCADE;
