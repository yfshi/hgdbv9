--
-- scs_off_string.sql — Review 问题5b 回归测试
--
-- 验证加密前端扫描器在 standard_conforming_strings 和
-- backslash_quote 两个 GUC 参数不同取值下，正确处理
-- 字符串字面量的边界和转义语义。
--
-- 修复前：扫描器使用固定正则 ＼'([^＼']|＼'＼')*＼'，不感知 GUC，
--         scs=off 时 ＼' 中的 ' 被误判为字符串结束符。
-- 修复后：扫描器根据 scs 值选择 XQ（on）或 XE（off）起始条件，
--         XE 模式下 ＼' 是引号转义、＼＼ 是反斜杠转义。
--         GUC 值从 tecc_cache 通过线程局部变量传递到扫描器。
--

-- ============================================================
-- 0. Setup
-- ============================================================
DROP TABLE IF EXISTS t_scs CASCADE;
DROP TABLE IF EXISTS t_scs_search CASCADE;
DROP DEK IF EXISTS dek_scs CASCADE;
DROP KEK IF EXISTS kek_scs CASCADE;

CREATE KEK kek_scs WITH (key_store = 'localkms', key_path = '/tmp/kek_scs.key');
CREATE DEK dek_scs WITH VALUES(KEK = kek_scs);

CREATE TABLE t_scs (
    id serial PRIMARY KEY,
    val varchar_enc ENCRYPTED WITH (DEK = 'dek_scs', support_algo = 'det')
);

-- 支持 SEARCH 的表，用于测试 LIKE 在 scs=off 下的 round-trip
CREATE TABLE t_scs_search (
    id serial PRIMARY KEY,
    val varchar_enc ENCRYPTED WITH (DEK = 'dek_scs', support_algo = 'det,search')
);

-- ============================================================
-- 1. scs=on（默认）：正常字符串 + 含反斜杠的字符串
--    scs=on 时，＼ 是普通字符，' 结束字符串。
--    'it＼'s' 的值是 it＼（反斜杠保留），' 结束字符串。
-- ============================================================
INSERT INTO t_scs (val) VALUES ('hello');
INSERT INTO t_scs (val) VALUES ('it''s on mode');
INSERT INTO t_scs (val) VALUES ('backslash\is\literal');
SELECT id, val FROM t_scs ORDER BY id;

-- ============================================================
-- 2. scs=off：正常字符串 + 含 '' 转义的字符串
--    scs=off 时，'' 是单引号转义。
-- ============================================================
SET standard_conforming_strings = off;

INSERT INTO t_scs (val) VALUES ('scs off basic');
INSERT INTO t_scs (val) VALUES ('it''s off mode');

RESET standard_conforming_strings;

SELECT id, val FROM t_scs ORDER BY id;

-- ============================================================
-- 3. scs=off：含反斜杠的字符串
--    scs=off 时，＼＼ 是反斜杠转义（值为 ＼），
--    ＼' 是引号转义（值为 '），＼X 其他保持原样。
-- ============================================================
SET standard_conforming_strings = off;

INSERT INTO t_scs (val) VALUES ('path\C:\file');
INSERT INTO t_scs (val) VALUES ('escaped\\double');

RESET standard_conforming_strings;

SELECT id, val FROM t_scs ORDER BY id;

-- ============================================================
-- 4. backslash_quote 不同取值
--    backslash_quote 控制后端是否允许 ＼' 作为引号转义。
--    前端扫描器在 scs=off 时统一将 ＼' 解析为引号转义，
--    后端根据 backslash_quote 做验证。
-- ============================================================
SET standard_conforming_strings = off;
SET backslash_quote = on;
INSERT INTO t_scs (val) VALUES ('bq_on_test');
RESET backslash_quote;

SET backslash_quote = safe_encoding;
INSERT INTO t_scs (val) VALUES ('bq_safe_test');
RESET backslash_quote;

RESET standard_conforming_strings;

SELECT id, val FROM t_scs ORDER BY id;

-- ============================================================
-- 5. 交替切换 scs 确保缓存同步正确
-- ============================================================
SET standard_conforming_strings = off;
INSERT INTO t_scs (val) VALUES ('off mode 1');
RESET standard_conforming_strings;

INSERT INTO t_scs (val) VALUES ('on mode between');

SET standard_conforming_strings = off;
INSERT INTO t_scs (val) VALUES ('off mode 2');
RESET standard_conforming_strings;

SELECT id, val FROM t_scs ORDER BY id;

-- ============================================================
-- 6. scs=off 下 TypeCast INSERT — 验证 E'' hex 修复
--    修复前：stmt_processor.c 用普通引号 'hex' 包裹密文，
--    scs=off 时 ＼xHH 被解释为 hex escape，破坏密文首字节。
--    修复后：统一使用 E'＼＼x<hex>' 格式，scs=off 下安全。
-- ============================================================
SET standard_conforming_strings = off;

INSERT INTO t_scs (val) VALUES ('42'::varchar_enc encrypted with (dek = 'dek_scs'));
INSERT INTO t_scs (val) VALUES ('typecast_off'::varchar_enc encrypted with (dek = 'dek_scs'));

RESET standard_conforming_strings;

SELECT val FROM t_scs WHERE val = '42';
SELECT val FROM t_scs WHERE val = 'typecast_off';

-- ============================================================
-- 7. scs=off 下 CREATE TABLE DEFAULT — 验证 E'' hex 修复
--    修复前：tecc_processor.c / create_stmt_processor.c 用普通引号
--    包裹 DEFAULT 加密值，scs=off 下同样破坏密文。
-- ============================================================
CREATE TABLE t_scs_def (
    id serial PRIMARY KEY,
    val varchar_enc ENCRYPTED WITH (DEK = 'dek_scs', support_algo = 'det') DEFAULT 'default_val'
);

INSERT INTO t_scs_def (id) VALUES (1);
INSERT INTO t_scs_def (id) VALUES (2);

SELECT id, val FROM t_scs_def ORDER BY id;

-- ============================================================
-- 8. scs=off 下 PREPARE/EXECUTE — 验证 E'' hex 修复
--    修复前：tecc_processor.c EXECUTE 参数用普通引号
--    包裹加密参数，scs=off 下同样破坏密文。
-- ============================================================
SET standard_conforming_strings = off;

PREPARE p_scs_exec (varchar_enc) AS INSERT INTO t_scs (val) VALUES ($1);
EXECUTE p_scs_exec('exec_off_val');
DEALLOCATE p_scs_exec;

PREPARE p_scs_sel (varchar_enc) AS SELECT val FROM t_scs WHERE val = $1;
EXECUTE p_scs_sel('exec_off_val');
DEALLOCATE p_scs_sel;

RESET standard_conforming_strings;

-- ============================================================
-- 9. scs=off 下完整 round-trip：写入 + 读取 + LIKE 比较
--    确保加密值在 scs=off 下能正确存储和检索。
--    LIKE 走 SEARCH 算法，验证 E'' hex 修复后 SEARCH
--    密文在 scs=off 下不被 ＼xHH escape 破坏。
-- ============================================================
SET standard_conforming_strings = off;

INSERT INTO t_scs (val) VALUES ('roundtrip_a');
INSERT INTO t_scs (val) VALUES ('roundtrip_b');
INSERT INTO t_scs (val) VALUES ('roundtrip_c');

SELECT val FROM t_scs WHERE val = 'roundtrip_a';
SELECT count(*) FROM t_scs WHERE val = 'roundtrip_b';

-- LIKE 测试需要 SEARCH 算法支持
INSERT INTO t_scs_search (val) VALUES ('alpha_001');
INSERT INTO t_scs_search (val) VALUES ('alpha_002');
INSERT INTO t_scs_search (val) VALUES ('beta_001');
INSERT INTO t_scs_search (val) VALUES ('test_a_b');

SELECT val FROM t_scs_search WHERE val LIKE 'alpha%';
SELECT count(*) FROM t_scs_search WHERE val LIKE 'beta%';

-- ============================================================
-- 10. 收尾清理
-- ============================================================
DROP TABLE IF EXISTS t_scs_def CASCADE;
DROP TABLE IF EXISTS t_scs_search CASCADE;
DROP TABLE IF EXISTS t_scs CASCADE;
DROP DEK IF EXISTS dek_scs;
DROP KEK IF EXISTS kek_scs;
