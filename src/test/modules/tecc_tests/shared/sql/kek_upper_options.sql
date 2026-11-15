-- ============================================================
-- kek_upper_options.sql — CREATE KEK 大写选项 → PUBLIC_KEY 注入回归
-- 背景（Bug 记录 L55）：前端 get_kek_option_value() 曾用 strcmp（大小写敏感）
-- 查 key_algo/key_store/key_path，按 tecc_ktool 模板写大写
-- KEY_STORE='localkms', KEY_ALGO='sm2', KEY_PATH=... 时 PUBLIC_KEY 注入被整段
-- 跳过 → hg_kek_args 无 PUBLIC_KEY → 所有 HE DEK（he-add/he-mul）创建失败报
-- "Non-asymmetric KEK missing PUBLIC_KEY for HE DEK"。
-- 修复：get_kek_option_value() 中 strcmp(def->defname,opt_name) → strcasecmp。
-- 断言：大写选项建 sm2 KEK → hg_kek_args 出现 PUBLIC_KEY（65B 未压缩点 = hex
-- 130、04 前缀）→ he-add/he-mul DEK 可创建 → HE 列同态加/标量乘可用。
-- 放 libpq/sql：仅 libpq C 前端（tecc_processor）有此选项解析。
-- ============================================================

-- 0. 幂等清理 + 自生成 sm2 密钥对
DROP TABLE IF EXISTS t_upper_he CASCADE;
DROP DEK IF EXISTS dek_upper_add CASCADE;
DROP DEK IF EXISTS dek_upper_mul CASCADE;
DROP KEK IF EXISTS kek_upper CASCADE;
\! rm -f "$TECC_KEK_PATH/kek_upper.priv" "$TECC_KEK_PATH/kek_upper.pub" 2>/dev/null
\! tecc_ktool generate --key-name kek_upper --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1

-- 1. 大写选项建 sm2 KEK（Bug 触发形态，QA 用 tecc_ktool 模板）
CREATE KEK kek_upper WITH (
    KEY_STORE = 'localkms',
    KEY_ALGO = 'sm2',
    KEY_PATH = '%KEKPATH%/kek_upper.priv'
);

-- 2. PUBLIC_KEY 已注入（修复前该行为缺行/值为空）
SELECT 'upper_pubkey_ok',
       length(encode(value, 'hex')) = 130 AND encode(value, 'hex') LIKE '04%' AS pub_ok
FROM hg_kek_args
WHERE kek_id = (SELECT oid FROM hg_kek WHERE kek_name = 'kek_upper')
  AND upper(key) = 'PUBLIC_KEY';

-- 3. HE DEK 可创建（修复前：Non-asymmetric KEK missing PUBLIC_KEY for HE DEK）
CREATE DEK dek_upper_add WITH VALUES(KEK = 'kek_upper', KEY_ALGO = 'he-add');
CREATE DEK dek_upper_mul WITH VALUES(KEK = 'kek_upper', KEY_ALGO = 'he-mul');

-- 4. HE 列端到端可用（同态加 + 标量乘 + 解密）
CREATE TABLE t_upper_he (
    id SERIAL,
    a INT4_ENC ENCRYPTED WITH (HE_ADD_DEK = 'dek_upper_add', SUPPORT_ALGO = 'he-add'),
    m INT4_ENC ENCRYPTED WITH (HE_MUL_DEK = 'dek_upper_mul', SUPPORT_ALGO = 'he-mul')
);
INSERT INTO t_upper_he (a, m) VALUES (10, 3), (20, 5);
SELECT 'he_sum', SUM(a) FROM t_upper_he;
SELECT 'he_scalar_mul', m * 2 FROM t_upper_he ORDER BY id;

-- 5. 清理
DROP TABLE t_upper_he CASCADE;
DROP DEK dek_upper_add CASCADE;
DROP DEK dek_upper_mul CASCADE;
DROP KEK kek_upper CASCADE;
