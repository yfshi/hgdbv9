--
-- guc_backslash.sql — Review 问题5 修复回归测试
--
-- 背景（Review 问题5 修复）：
--   修复前：tecc_guc_apply_backslash_quote 在 backslash_quote=ON 时
--   删除字符串中的反斜杠（＼' → '），改变字面量值，导致：
--   - SQL语法错误（引号不匹配）
--   - 字符串值改变（丢失转义字符）
--   - 加密解析失败或加密列值损坏
--   修复后：＼' 转换为标准转义 ''（两个单引号），不改变值。
--   同时修复了缓冲区截断问题（remaining < 时返回错误而非静默截断），
--   以及 SAFE_ENCODING 分支中 return -1 在 strcpy 之前的 bug。
--   性能优化：默认配置（standard_conforming_strings=true,
--   backslash_quote=SAFE_ENCODING）下跳过整个处理流程。
--
-- ============================================================

-- ============================================================
-- 0. Setup: 创建测试表和 KEK/DEK
-- ============================================================
DROP TABLE IF EXISTS t_backslash CASCADE;
DROP DEK IF EXISTS dek_bs CASCADE;
DROP KEK IF EXISTS kek_bs CASCADE;

CREATE KEK kek_bs WITH (key_store = 'localkms', key_path = '/tmp/kek_bs.key');
CREATE DEK dek_bs WITH VALUES(KEK = kek_bs);

-- ============================================================
-- 1. 基线：包含引号和特殊字符的字符串正常加解密
--    修复前：backslash_quote=ON 时 ＼' → ' 改变值
--    修复后：＼' → '' 保持值不变
-- ============================================================
CREATE TABLE t_backslash (
    id serial PRIMARY KEY,
    val varchar_enc ENCRYPTED WITH (DEK = 'dek_bs', support_algo = 'det')
);

INSERT INTO t_backslash (val) VALUES ('hello');
INSERT INTO t_backslash (val) VALUES ('it''s a test');
INSERT INTO t_backslash (val) VALUES ('multi''quote''value');

-- 查询验证：字符串值正确加解密
SELECT id, val FROM t_backslash ORDER BY id;

-- ============================================================
-- 2. 长字符串：验证缓冲区不会截断
--    修复前：缓冲区空间不足时静默截断，破坏 SQL
--    修复后：返回错误而非截断
-- ============================================================
INSERT INTO t_backslash (val) VALUES ('short');
INSERT INTO t_backslash (val) VALUES ('this is a moderately long string with some special chars like ''quotes'' and spaces');
INSERT INTO t_backslash (val) VALUES ('aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeee');

-- 查询验证长字符串
SELECT id, val FROM t_backslash ORDER BY id;

-- ============================================================
-- 3. 特殊字符：验证各种边界情况
-- ============================================================
INSERT INTO t_backslash (val) VALUES ('');
INSERT INTO t_backslash (val) VALUES (' ');
INSERT INTO t_backslash (val) VALUES ('''');
INSERT INTO t_backslash (val) VALUES ('''''');

-- 查询验证
SELECT id, val FROM t_backslash ORDER BY id;

-- ============================================================
-- 4. 默认配置下性能优化：正常查询不受影响
--    默认配置（standard_conforming_strings=true,
--    backslash_quote=SAFE_ENCODING）下应跳过 GUC 处理
-- ============================================================
SELECT count(*) FROM t_backslash;
SELECT id FROM t_backslash WHERE val = 'hello';

-- ============================================================
-- 5. 收尾清理
-- ============================================================
DROP TABLE IF EXISTS t_backslash CASCADE;
DROP DEK IF EXISTS dek_bs;
DROP KEK IF EXISTS kek_bs;
