--
-- default_keyword_case.sql — libpq专有测试：Bug 7972（含 2026-09-21 补充修复）
--   加密列 DEFAULT 值因 SQL 关键字大小写导致 "invalid hex string" 报错
--
-- 报障形态：CREATE TABLE t(v text_enc ... default 'defaultvalue')
--   → ERROR: invalid hex string: "defaultvalue"
-- 根因：建表阶段前端要做"把 DEFAULT 明文替换成密文"的**文本重写**，用 strstr
--   硬编码大写 "DEFAULT" 定位；用户写小写 default 时找不到 → 重写被静默跳过
--   （仅 TECC_LOG_WARNING）→ 明文原样下发后端 cipherdata_in → 报错。
-- 首次修复：先试大写 "DEFAULT"，找不到再试全小写 "default"。
--   但 SQL 关键字本应大小写不敏感，**混合大小写（DeFaUlT/Default/dEfAuLt）
--   两种都不匹配**，即修复只做了一半——2026-09-21 补成对关键字部分一次
--   大小写不敏感扫描（create_stmt_processor.c: tecc_find_pattern_keyword_ci）。
--   该残留缺陷此前无任何在跑的用例兜着（唯一用例在 temp/sql/，不进 make check），
--   故本用例连同补充修复一起落地。
--
-- 本用例锁：五种写法（全小写/全大写/首字母大写/两种混排）× 字符串列与整数列
--   + 同一条 CREATE TABLE 里多个 DEFAULT（多列重写路径，逐列替换的位置/长度
--   计算最易出错）+ 明文列 DEFAULT 不被误伤 + 默认值确实按密文存储（可用
--   WHERE 命中、可用显式 INSERT 覆盖）。
-- 断言方式：每种写法都**建表后 INSERT ... DEFAULT VALUES 再读回明文**，不只是
--   "建表不报错"——重写被跳过会报错，但替换长度算错会导致读回错值或截断。
--
-- 注：密文序不可复现，ORDER BY 一律用明文列；本用例不依赖密文列排序。
--

DROP TABLE IF EXISTS t_dkc_low, t_dkc_up, t_dkc_cap, t_dkc_mix1, t_dkc_mix2 CASCADE;
DROP TABLE IF EXISTS t_dkc_int, t_dkc_multi, t_dkc_plain CASCADE;
DROP DEK IF EXISTS dek_dkc CASCADE;
DROP KEK IF EXISTS kek_dkc CASCADE;

CREATE KEK kek_dkc WITH (key_store = 'localkms', key_path = '%KEKPATH%/kek_dkc.key');
CREATE DEK dek_dkc WITH VALUES(KEK = 'kek_dkc');

-- ============================================================
-- 1. 五种写法各建一张表（一表一列，隔离单列重写路径）
-- ============================================================
CREATE TABLE t_dkc_low  (v text_enc ENCRYPTED WITH (DEK = dek_dkc) default 'low_val');
CREATE TABLE t_dkc_up   (v text_enc ENCRYPTED WITH (DEK = dek_dkc) DEFAULT 'up_val');
CREATE TABLE t_dkc_cap  (v text_enc ENCRYPTED WITH (DEK = dek_dkc) Default 'cap_val');
CREATE TABLE t_dkc_mix1 (v text_enc ENCRYPTED WITH (DEK = dek_dkc) DeFaUlT 'mix1_val');
CREATE TABLE t_dkc_mix2 (v text_enc ENCRYPTED WITH (DEK = dek_dkc) dEfAuLt 'mix2_val');
SELECT '1_tables_created' AS tag, count(*) FROM pg_class
 WHERE relname IN ('t_dkc_low','t_dkc_up','t_dkc_cap','t_dkc_mix1','t_dkc_mix2');

INSERT INTO t_dkc_low  DEFAULT VALUES;
INSERT INTO t_dkc_up   DEFAULT VALUES;
INSERT INTO t_dkc_cap  DEFAULT VALUES;
INSERT INTO t_dkc_mix1 DEFAULT VALUES;
INSERT INTO t_dkc_mix2 DEFAULT VALUES;

SELECT '1a_lower'   AS tag, v FROM t_dkc_low;
SELECT '1b_UPPER'   AS tag, v FROM t_dkc_up;
SELECT '1c_Default' AS tag, v FROM t_dkc_cap;
SELECT '1d_DeFaUlT' AS tag, v FROM t_dkc_mix1;
SELECT '1e_dEfAuLt' AS tag, v FROM t_dkc_mix2;

-- ============================================================
-- 2. 整数列默认值 + 同表多列默认值（多列重写路径）
-- ============================================================
CREATE TABLE t_dkc_int (v int4_enc ENCRYPTED WITH (DEK = dek_dkc, SUPPORT_ALGO = 'det') DeFaUlT 100);

CREATE TABLE t_dkc_multi (
    a text_enc ENCRYPTED WITH (DEK = dek_dkc, SUPPORT_ALGO = 'det') default 'm_low',
    b text_enc ENCRYPTED WITH (DEK = dek_dkc, SUPPORT_ALGO = 'det') DEFAULT 'm_up',
    c text_enc ENCRYPTED WITH (DEK = dek_dkc, SUPPORT_ALGO = 'det') DeFaUlT 'm_mix',
    d int4_enc ENCRYPTED WITH (DEK = dek_dkc, SUPPORT_ALGO = 'det') dEfAuLt 42
);

INSERT INTO t_dkc_int   DEFAULT VALUES;
INSERT INTO t_dkc_multi DEFAULT VALUES;

SELECT '2a_int_mixed' AS tag, v FROM t_dkc_int;
SELECT '2b_multi_cols' AS tag, a, b, c, d FROM t_dkc_multi;

-- ============================================================
-- 3. 明文列 DEFAULT 不被误伤
-- ============================================================
CREATE TABLE t_dkc_plain (
    id int DeFaUlT 7,
    v  text_enc ENCRYPTED WITH (DEK = dek_dkc) DeFaUlT 'p_mix'
);
INSERT INTO t_dkc_plain DEFAULT VALUES;
SELECT '3a_plain_int_default' AS tag, id FROM t_dkc_plain;
SELECT '3b_enc_default'       AS tag, v  FROM t_dkc_plain;

-- ============================================================
-- 4. 显式值覆盖默认值仍然正常（默认值本身已按密文存储）
--
-- 注：本段同时**正向断言**"DEFAULT 插入的行能被等值查询命中"。这一点曾经做不到：
--   建表时前端加密默认值得到的密文，与同会话同 DEK 下字面量 'm_mix' 加密的密文
--   不相等（DET 却不等值），于是 `WHERE c = 'm_mix'` 静默找不到 DEFAULT 行。
--   根因（建表路径拿不到 collation locale → 归一化没走 ICU sort key）已修，
--   见王好燕_0921_bug.md §12.3；专门用例见 default_value_equality.sql，
--   这里补一条轻量断言防回归（只断行数，专门用例里断得更细）。
--   注：t_dkc_plain.v 未指定 SUPPORT_ALGO，不含 DET/OPE，等值运算符本就不支持
--   （报 "Operator requires DET or OPE algorithm"），故那列不作为本断言的载体。
-- ============================================================
SELECT '4a_default_not_null' AS tag, count(*) FROM t_dkc_multi WHERE c IS NOT NULL;
SELECT '4a_default_eq_hit'   AS tag, count(*) FROM t_dkc_multi WHERE c = 'm_mix';

INSERT INTO t_dkc_multi (a, b, c, d) VALUES ('x1', 'y1', 'z1', 9);
SELECT '4b_rows_after_explicit' AS tag, count(*) FROM t_dkc_multi;
SELECT '4c_explicit_readback'   AS tag, a, b, c, d FROM t_dkc_multi
 WHERE a = 'x1' ORDER BY a;

-- ============================================================
-- 5. 清理
-- ============================================================
DROP TABLE IF EXISTS t_dkc_low, t_dkc_up, t_dkc_cap, t_dkc_mix1, t_dkc_mix2 CASCADE;
DROP TABLE IF EXISTS t_dkc_int, t_dkc_multi, t_dkc_plain CASCADE;
DROP DEK IF EXISTS dek_dkc CASCADE;
DROP KEK IF EXISTS kek_dkc CASCADE;
SELECT 'z_cleanup_done' AS tag;
