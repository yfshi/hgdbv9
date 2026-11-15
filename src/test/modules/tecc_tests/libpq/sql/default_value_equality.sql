--
-- default_value_equality.sql — libpq专有测试：加密列 DEFAULT 插入的行查不到
--
-- 报障形态：加密列带 DEFAULT，用 DEFAULT 插入的行用**等值条件查不到**（静默少行）
--   CREATE TABLE t (id int, s text_enc ... SUPPORT_ALGO='det' DEFAULT 'mx');
--   INSERT INTO t (id) VALUES (1);          -- 走 DEFAULT
--   INSERT INTO t (id, s) VALUES (2, 'mx'); -- 显式同值
--   SELECT count(*) FROM t WHERE s = 'mx';  -- 实际 1（只命中 id=2），预期 2
--   SELECT (SELECT s FROM t WHERE id=1) = (SELECT s FROM t WHERE id=2);  -- 实际 f
-- 根因：DET/OPE 归一化要用 collation locale 生成 ICU sort key，而 collation
--   locale 是由前端从 hg_encrypted_columns_info 取来的（该视图把后端的
--   collation_oid 映射成 collcollate）。建表那一刻列还没入库、元数据缓存必然
--   miss，前端加密 DEFAULT 值时拿到的 locale 是空 —— 归一化退化成"无 collation
--   的二进制拷贝"，与 INSERT/WHERE 路径（缓存里 locale 齐全）生成两套互不相等
--   的（但各自确定性的）密文。于是 DEFAULT 行在 DET 域里"另一套密文"，等值查询
--   静默漏掉；int4_enc 无 collation 不受影响，LIKE(SEARCH/HMAC) 也不受影响，
--   所以既有用例（只断 int 等值 + text LIKE）恰好绕过了它。
--
-- 本用例锁：DEFAULT 行必须能被等值查询命中，且与显式插入的同一明文**等值**，
--   覆盖 text/varchar/bpchar（无 COLLATE，走"库默认排序规则"分支）、
--   COLLATE "C"+OPE（走"COLLATE 子句"分支）、int4_enc（无 collation）、
--   同表多列 DEFAULT、空串/带尾空格 DEFAULT、以及负例（不同明文不得命中）。
-- 断言方式：断"命中行数与行号"，不只断"不报错"——两套密文时查询不报错，
--   只是安静地返回 0 行/少行。
--
-- 敏不敏感看排序规则：本例的灵敏度来自"该列有效排序规则不是 raw-byte"
--   （tecc_is_raw_byte_collation() 为假才会走 ICU sort key）。库里默认排序规则
--   是 C/POSIX 时两条路径本来都不做 ICU 归一化，缺陷压根不成立，本例会
--   带不带修复都通过——不是用例失灵，是场景不满足。第 1/2/5/6 段在当前
--   zh_CN.UTF-8 库上正是靠这一点抓到了修复前的回归（1a 由 1 变 0、
--   1d 由 "2|1,2" 变 "1|2"、1e 由 t 变 f）。
--
-- 注：密文序不可复现，ORDER BY 一律用明文列 id；本用例不依赖密文列排序。
--

DROP TABLE IF EXISTS t_dve_txt, t_dve_var, t_dve_bpc, t_dve_int CASCADE;
DROP TABLE IF EXISTS t_dve_col, t_dve_multi, t_dve_edge CASCADE;
DROP DEK IF EXISTS dek_dve CASCADE;
DROP KEK IF EXISTS kek_dve CASCADE;

CREATE KEK kek_dve WITH (key_store = 'localkms', key_path = '%KEKPATH%/kek_dve.key');
CREATE DEK dek_dve WITH VALUES(KEK = 'kek_dve');

-- ============================================================
-- 1. 报障原文：text_enc DET，只有 DEFAULT 行时也要能等值命中
-- ============================================================
CREATE TABLE t_dve_txt (
    id int,
    s  text_enc ENCRYPTED WITH (DEK = dek_dve, SUPPORT_ALGO = 'det') DEFAULT 'mx'
);
INSERT INTO t_dve_txt (id) VALUES (1);

SELECT '1a_only_default_hit' AS tag, count(*) FROM t_dve_txt WHERE s = 'mx';
SELECT '1b_default_hit_id'   AS tag, id FROM t_dve_txt WHERE s = 'mx' ORDER BY id;
SELECT '1c_other_value'      AS tag, count(*) FROM t_dve_txt WHERE s = 'no_such_value';

-- 再显式插入同一明文：两行都要命中，且两行密文**等值**
INSERT INTO t_dve_txt (id, s) VALUES (2, 'mx');
SELECT '1d_both_hit'   AS tag, count(*), coalesce(string_agg(id::text, ',' ORDER BY id), '-')
  FROM t_dve_txt WHERE s = 'mx';
SELECT '1e_default_eq_explicit' AS tag,
       (SELECT s FROM t_dve_txt WHERE id = 1) = (SELECT s FROM t_dve_txt WHERE id = 2);
SELECT '1f_readback'   AS tag, s FROM t_dve_txt WHERE id = 1;

-- ============================================================
-- 2. 其他文本密态类型：varchar_enc / bpchar_enc
-- ============================================================
CREATE TABLE t_dve_var (
    id int,
    s  varchar_enc ENCRYPTED WITH (DEK = dek_dve, SUPPORT_ALGO = 'det') DEFAULT 'vv'
);
CREATE TABLE t_dve_bpc (
    id int,
    s  bpchar_enc ENCRYPTED WITH (DEK = dek_dve, SUPPORT_ALGO = 'det') DEFAULT 'bb'
);
INSERT INTO t_dve_var (id) VALUES (1);
INSERT INTO t_dve_var (id, s) VALUES (2, 'vv');
INSERT INTO t_dve_bpc (id) VALUES (1);
INSERT INTO t_dve_bpc (id, s) VALUES (2, 'bb');

SELECT '2a_varchar_hit' AS tag, count(*), coalesce(string_agg(id::text, ',' ORDER BY id), '-')
  FROM t_dve_var WHERE s = 'vv';
SELECT '2b_bpchar_hit'  AS tag, count(*), coalesce(string_agg(id::text, ',' ORDER BY id), '-')
  FROM t_dve_bpc WHERE s = 'bb';

-- ============================================================
-- 3. 非文本密态类型（无 collation，本就不该受影响）——防回归
-- ============================================================
CREATE TABLE t_dve_int (
    id int,
    s  int4_enc ENCRYPTED WITH (DEK = dek_dve, SUPPORT_ALGO = 'det') DEFAULT 100
);
INSERT INTO t_dve_int (id) VALUES (1);
INSERT INTO t_dve_int (id, s) VALUES (2, 100);
SELECT '3a_int4_hit' AS tag, count(*), coalesce(string_agg(id::text, ',' ORDER BY id), '-')
  FROM t_dve_int WHERE s = 100;

-- ============================================================
-- 4. 列上写了 COLLATE（走"COLLATE 子句"分支，本环境需要 OPE）
-- ============================================================
CREATE TABLE t_dve_col (
    id int,
    s  text_enc ENCRYPTED WITH (DEK = dek_dve, SUPPORT_ALGO = 'ope') COLLATE "C" DEFAULT 'cc'
);
INSERT INTO t_dve_col (id) VALUES (1);
INSERT INTO t_dve_col (id, s) VALUES (2, 'cc');
SELECT '4a_collate_hit' AS tag, count(*), coalesce(string_agg(id::text, ',' ORDER BY id), '-')
  FROM t_dve_col WHERE s = 'cc';

-- ============================================================
-- 5. 同一条 CREATE TABLE 里多列 DEFAULT（逐列各自解析 locale）
-- ============================================================
CREATE TABLE t_dve_multi (
    id int,
    a text_enc ENCRYPTED WITH (DEK = dek_dve, SUPPORT_ALGO = 'det') DEFAULT 'm_a',
    b text_enc ENCRYPTED WITH (DEK = dek_dve, SUPPORT_ALGO = 'det') DEFAULT 'm_b',
    c int4_enc ENCRYPTED WITH (DEK = dek_dve, SUPPORT_ALGO = 'det') DEFAULT 7
);
INSERT INTO t_dve_multi (id) VALUES (1);
INSERT INTO t_dve_multi (id, a, b, c) VALUES (2, 'm_a', 'm_b', 7);
SELECT '5a_multi_a' AS tag, count(*), coalesce(string_agg(id::text, ',' ORDER BY id), '-')
  FROM t_dve_multi WHERE a = 'm_a';
SELECT '5b_multi_b' AS tag, count(*), coalesce(string_agg(id::text, ',' ORDER BY id), '-')
  FROM t_dve_multi WHERE b = 'm_b';
SELECT '5c_multi_c' AS tag, count(*), coalesce(string_agg(id::text, ',' ORDER BY id), '-')
  FROM t_dve_multi WHERE c = 7;

-- ============================================================
-- 6. 边界默认值：空串、带尾空格
-- ============================================================
CREATE TABLE t_dve_edge (
    id int,
    e  text_enc ENCRYPTED WITH (DEK = dek_dve, SUPPORT_ALGO = 'det') DEFAULT '',
    p  text_enc ENCRYPTED WITH (DEK = dek_dve, SUPPORT_ALGO = 'det') DEFAULT 'pad   '
);
INSERT INTO t_dve_edge DEFAULT VALUES;
SELECT '6a_empty_default_hit' AS tag, count(*) FROM t_dve_edge WHERE e = '';
SELECT '6b_padded_default_hit' AS tag, count(*) FROM t_dve_edge WHERE p = 'pad   ';
SELECT '6c_padded_not_trimmed' AS tag, count(*) FROM t_dve_edge WHERE p = 'pad';

-- ============================================================
-- 7. 显式值覆盖默认值仍正常（默认值本身已按正确密文存储）
-- ============================================================
INSERT INTO t_dve_txt (id, s) VALUES (3, 'other');
SELECT '7a_override_hit' AS tag, count(*), coalesce(string_agg(id::text, ',' ORDER BY id), '-')
  FROM t_dve_txt WHERE s = 'other';
SELECT '7b_rows_total' AS tag, count(*) FROM t_dve_txt;

-- ============================================================
-- 8. 清理
-- ============================================================
DROP TABLE IF EXISTS t_dve_txt, t_dve_var, t_dve_bpc, t_dve_int CASCADE;
DROP TABLE IF EXISTS t_dve_col, t_dve_multi, t_dve_edge CASCADE;
DROP DEK IF EXISTS dek_dve CASCADE;
DROP KEK IF EXISTS kek_dve CASCADE;
SELECT 'z_cleanup_done' AS tag;
