-- ============================================================
-- TECC 4.0.0.0 安装测试 - 环境准备
-- 创建测试所需的 KEK/DEK 密钥
-- 注意：使用JDBC驱动支持的语法
-- ============================================================

-- 创建测试用的KEK
CREATE KEK kek_test WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_test.key');

-- 创建测试用的DEK
CREATE DEK dek_test WITH VALUES (KEK=kek_test);

-- 确认密钥创建成功
SELECT key_name, key_store FROM hg_kek;
SELECT key_name, kek_key_name FROM hg_dek;
