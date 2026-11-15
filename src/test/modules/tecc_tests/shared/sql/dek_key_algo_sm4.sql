-- ============================================================
-- dek_key_algo_sm4.sql — CREATE DEK 显式 KEY_ALGO='sm4'（对称 DEK）回归测试
-- 背景（QA 报修）：CREATE DEK 前端把 key_algo 域误限为 he-add/he-mul，
-- 显式 'sm4'（V4.0.1 概要设计 §7.1：DEK key_algo 域 = sm4/he-add/he-mul，
-- sm4 为对称默认）被误报 "Invalid KEY_ALGO for DEK"，预期应创建成功。
-- 修复：stmt_processor.c CREATE DEK 校验放行 'sm4'（等价省略，is_he_dek=0），
-- 与 createdek.c 存储注释及 create_stmt_processor 列校验口径一致。
-- 本文件放 libpq/sql：仅 C 前端（tecc_processor）有此校验；JDBC 直发后端
-- createdek.c 不校验 key_algo 值，天然无此 bug。
-- 此处验证：KEY_ALGO='sm4' 建对称 DEK 成功且 hg_dek_args.key_algo='sm4'；
--           非法 key_algo（如 'rsa'）仍报 "Invalid KEY_ALGO for DEK"（不回归）。
-- 注意：key_path 用固定 /tmp 字面量（C 层经 TECC_KEK_PATH 覆盖到本回归
-- key 目录），保证框架可预生成密钥文件。
-- ============================================================

-- 0. 幂等清理
DROP DEK IF EXISTS dek_algo_sm4;
DROP KEK IF EXISTS kek_algo_sm4 CASCADE;

-- 1. 对称 KEK（sm4 .key 由框架预生成到 TECC_KEK_PATH）
CREATE KEK kek_algo_sm4 WITH (
    key_store = 'localkms',
    key_algo = 'sm4',
    key_path = '/tmp/dek_algo_sm4_regress.key'
);

-- 2. 显式 KEY_ALGO='sm4' 的对称 DEK → 成功（修复点）
CREATE DEK dek_algo_sm4 WITH VALUES(KEK = kek_algo_sm4, KEY_ALGO = 'sm4');

-- 3. 校验 hg_dek_args.key_algo 已存 'sm4'（value 十六进制 736d34）
SELECT count(*) AS n
FROM hg_dek d JOIN hg_dek_args a ON a.dek_id = d.oid
WHERE d.dek_name = 'dek_algo_sm4'
  AND a.key = 'key_algo'
  AND encode(a.value, 'hex') = '736d34';

-- 4. 非法 key_algo 仍拒绝（前端不回归）
CREATE DEK dek_algo_bad WITH VALUES(KEK = kek_algo_sm4, KEY_ALGO = 'rsa');

-- 5. 清理
DROP DEK dek_algo_sm4;
DROP KEK kek_algo_sm4 CASCADE;
