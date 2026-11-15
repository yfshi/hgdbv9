-- ============================================================
-- collation.sql — 排序规则测试（COLLATE/ICU排序键）
-- 覆盖 test_ope_text_collate, test_ope_text_orderby,
--   test_ope_text_varlen, test_bug7857_select_collate,
--   test_collate_expression
-- ============================================================

-- ============================================================
-- 0. Setup
-- ============================================================
DROP KEK IF EXISTS kek_coll CASCADE;

CREATE KEK kek_coll WITH (key_store = 'localkms', key_path = '/tmp/kek_coll.key');

CREATE DEK dek_coll WITH VALUES(KEK = kek_coll);

-- ============================================================
-- 1. COLLATE in SELECT expressions (bug7857)
-- ============================================================
SELECT '=== 1 COLLATE in SELECT ===';

SELECT '1a_text_ope_collate_C', 'test'::text_enc encrypted with (dek = dek_coll, support_algo='ope') collate "C";

SELECT '1b_text_det_ope_collate_C', 'test'::text_enc encrypted with (dek = dek_coll, support_algo='det,ope') collate "C";

SELECT '1c_text_det_collate_C', 'test'::text_enc encrypted with (dek = dek_coll, support_algo='det') collate "C";

SELECT '1d_varchar_collate_C', 'hello'::varchar_enc encrypted with (dek = dek_coll) collate "C";

SELECT '1e_varchar_typmod_collate_C', 'test'::varchar_enc(20) encrypted with (dek = dek_coll, support_algo='ope') collate "C";

SELECT '1f_text_ope_collate_POSIX', 'test'::text_enc encrypted with (dek = dek_coll, support_algo='ope') collate "POSIX";

SELECT '1g_multi_collate', 'a'::text_enc encrypted with (dek = dek_coll) collate "C",
       'b'::text_enc encrypted with (dek = dek_coll) collate "POSIX";

SELECT '1h_no_collate_ctrl', 'test'::text_enc encrypted with (dek = dek_coll, support_algo='ope');

-- ============================================================
-- 2. ORDER BY with collation — locale string comparison (test_ope_text_collate)
-- ============================================================
SELECT '=== 2 ORDER BY with COLLATION ===';

DROP TABLE IF EXISTS test_ope_coll CASCADE;

CREATE TABLE test_ope_coll(
    id        int4_enc ENCRYPTED WITH (dek = dek_coll, SUPPORT_ALGO = 'ope'),
    tag       text_enc ENCRYPTED WITH (dek = dek_coll, SUPPORT_ALGO = 'ope') COLLATE "en-US-x-icu",
    desc_text text_enc ENCRYPTED WITH (dek = dek_coll, SUPPORT_ALGO = 'ope') COLLATE "en-US-x-icu",
    vc        varchar_enc(200) ENCRYPTED WITH (dek = dek_coll, SUPPORT_ALGO = 'ope') COLLATE "en-US-x-icu"
);

-- @param int 1
-- @param String Apple
-- @param String a red fruit
-- @param String hello
-- @param int 2
-- @param String apple
-- @param String a lowercase fruit
-- @param String HELLO
-- @param int 3
-- @param String Banana
-- @param String a yellow fruit
-- @param String Apfel
-- @param int 4
-- @param String banana
-- @param String another fruit
-- @param String apfel
-- @param int 5
-- @param String cafe
-- @param String coffee with accent
-- @param String Zebra
-- @param int 6
-- @param String Cafe
-- @param String coffee no accent
-- @param String zebra
-- @param int 7
-- @param String Zebra
-- @param String last in ASCII
-- @param String cafe
-- @param int 8
-- @param String zebra
-- @param String last lower
-- @param String Cafe
INSERT INTO test_ope_coll VALUES
(?, ?,   ?,       ?),
(?, ?,   ?,  ?),
(?, ?,  ?,     ?),
(?, ?,  ?,      ?),
(?, ?,    ?, ?),
(?, ?,    ?,   ?),
(?, ?,   ?,      ?),
(?, ?,   ?,         ?);

SELECT '2a_tag_asc', id, tag FROM test_ope_coll ORDER BY tag ASC;

SELECT '2b_tag_desc', id, tag FROM test_ope_coll ORDER BY tag DESC;

SELECT '2c_vc_asc', id, vc FROM test_ope_coll ORDER BY vc ASC;

SELECT '2d_vc_desc', id, vc FROM test_ope_coll ORDER BY vc DESC;

-- WHERE with collation
-- @param String apple
SELECT '2e_tag_eq_case', id, tag FROM test_ope_coll WHERE tag = ?;

-- @param String Banana
SELECT '2f_tag_gt', id, tag FROM test_ope_coll WHERE tag > ? ORDER BY tag ASC;

-- @param String Apple
-- @param String zebra
SELECT '2g_tag_between', id, tag FROM test_ope_coll WHERE tag BETWEEN ? AND ? ORDER BY tag ASC;

-- Multi-column ORDER BY
SELECT '2h_tag_vc_asc', id, tag, vc FROM test_ope_coll ORDER BY tag ASC, vc ASC;

-- NULL handling
-- @param int 11
-- @param null null
-- @param null null
-- @param null null
INSERT INTO test_ope_coll VALUES (?, ?, ?, ?);

SELECT '2i_tag_asc_nulls_first', id, tag FROM test_ope_coll ORDER BY tag ASC NULLS FIRST;

SELECT '2j_tag_asc_nulls_last', id, tag FROM test_ope_coll ORDER BY tag ASC NULLS LAST;

DROP TABLE IF EXISTS test_ope_coll CASCADE;

-- ============================================================
-- 3. COLLATE in WHERE clause (from test_collate_expression)
-- 注：以下所有 COLLATE 切换均报错，因为加密列的排序规则
-- 在创建时已固化。SELECT列表/WHERE/ORDER BY中指定不同
-- COLLATE 会改变排序列的collation属性或比较语义，但加密
-- 密文基于列定义时的collation生成，无法运行时切换。
-- ============================================================
SELECT '=== 3 COLLATE in WHERE ===';

DROP TABLE IF EXISTS test_coll CASCADE;

CREATE TABLE test_coll (
    id INT4,
    name TEXT_ENC ENCRYPTED WITH (DEK = dek_coll, SUPPORT_ALGO = 'DET,SEARCH'),
    val INT4_ENC ENCRYPTED WITH (DEK = dek_coll, SUPPORT_ALGO = 'DET')
);

-- @param int 1
-- @param String Alice
-- @param int 100
-- @param int 2
-- @param String Bob
-- @param int 200
-- @param int 3
-- @param String Charlie
-- @param int 300
INSERT INTO test_coll VALUES (?, ?, ?), (?, ?, ?), (?, ?, ?);

-- @param int 1
SELECT '3a_select_collate', name COLLATE "POSIX" FROM test_coll WHERE id = ?;

-- @param String Alice
SELECT '3b_where_collate_eq', id FROM test_coll WHERE name COLLATE "C" = ?;

-- @param String Bob
SELECT '3c_where_collate_paren', id FROM test_coll WHERE (name COLLATE "C") = ?;

-- @param String A%
SELECT '3d_collate_like', id FROM test_coll WHERE name COLLATE "C" LIKE ?;

-- @param int 3
SELECT '3e_collate_multi_col', name COLLATE "POSIX", val FROM test_coll WHERE id < ?;

-- @param String A%
-- @param String Bob
SELECT '3f_collate_or', id FROM test_coll WHERE name COLLATE "C" LIKE ? OR name COLLATE "C" = ?;

-- @param String Bob
SELECT '3g_collate_ne', id FROM test_coll WHERE name COLLATE "C" != ?;

DROP TABLE IF EXISTS test_coll CASCADE;

-- ============================================================
-- 4. zh_CN.utf8 ORDER BY with plain table comparison (bug7860)
-- ============================================================
SELECT '=== 4 zh_CN.utf8 ORDER BY ===';

-- 4a: 普通表 zh_CN.utf8 排序（验证 locale 感知排序与 "C" 排序不同）
DROP TABLE IF EXISTS tb_7860_plain CASCADE;

CREATE TABLE tb_7860_plain (id SERIAL, name text);

-- @param String 北京
-- @param String 上海
-- @param String 广州
-- @param String 成都
INSERT INTO tb_7860_plain (name) VALUES
  (?), (?), (?), (?);

SELECT '4a_plain_zh_cn', id, name FROM tb_7860_plain ORDER BY name COLLATE "zh_CN.utf8";

SELECT '4b_plain_c_collate', id, name FROM tb_7860_plain ORDER BY name COLLATE "C";

SELECT '4c_plain_default', id, name FROM tb_7860_plain ORDER BY name;

DROP TABLE IF EXISTS tb_7860_plain CASCADE;

-- 4d: 加密表 zh_CN.utf8 COLLATE（加密列应正常排序，不匹配 COLLATE 应拒绝）
DROP TABLE IF EXISTS tb_7860_enc CASCADE;

CREATE TABLE tb_7860_enc (
    id SERIAL,
    name text_enc ENCRYPTED WITH (dek = dek_coll, SUPPORT_ALGO = 'ope') COLLATE "zh_CN.utf8"
);

-- @param String 北京
-- @param String 上海
-- @param String 广州
-- @param String 成都
INSERT INTO tb_7860_enc (name) VALUES
  (?), (?), (?), (?);

SELECT '4d_enc_default', id, name FROM tb_7860_enc ORDER BY name;

SELECT '4e_enc_consistent', id, name FROM tb_7860_enc ORDER BY name COLLATE "zh_CN.utf8";

SELECT '4f_enc_inconsistent', id, name FROM tb_7860_enc ORDER BY name COLLATE "C";

DROP TABLE IF EXISTS tb_7860_enc CASCADE;

-- ============================================================
-- 5. 普通列 ORDER BY COLLATE（加密列不应影响普通列的行为）
-- ============================================================
SELECT '=== 5 Plain ORDER BY COLLATE ===';

DROP TABLE IF EXISTS tb_plain_coll CASCADE;

CREATE TABLE tb_plain_coll (
    id SERIAL,
    name text
);

-- @param String apple
-- @param String Apple
-- @param String banana
-- @param String Banana
-- @param String cherry
INSERT INTO tb_plain_coll (name) VALUES (?), (?), (?), (?), (?);

SELECT '5a_plain_collate_c', id, name FROM tb_plain_coll ORDER BY name COLLATE "C";

SELECT '5b_plain_no_collate', id, name FROM tb_plain_coll ORDER BY name;

DROP TABLE IF EXISTS tb_plain_coll CASCADE;

-- ============================================================
-- 6. 加密列COLLATE固化 + COLLATE一致性检查（Bug 8176）
-- ============================================================
SELECT '=== 6 collation fix + consistency check ===';

-- 6a: 未指定COLLATE时正常建表使用
DROP TABLE IF EXISTS tb_coll_fix CASCADE;

CREATE TABLE tb_coll_fix (
    id SERIAL,
    txt text_enc encrypted with (dek = dek_coll, support_algo = 'ope'),
    vc varchar_enc encrypted with (dek = dek_coll, support_algo = 'ope'),
    bp bpchar_enc encrypted with (dek = dek_coll, support_algo = 'ope')
);

-- @param String Alice
-- @param String Bob
-- @param String Charlie
INSERT INTO tb_coll_fix (txt, vc, bp) VALUES (?, ?, ?);

SELECT '6a_default_coll', id, txt, vc, bp FROM tb_coll_fix ORDER BY txt;

-- 6b: 一致则不报错
SELECT '6b_consistent', id, txt FROM tb_coll_fix ORDER BY txt COLLATE "en_US.utf8";

-- 6c: COLLATE不一致时报错（ORDER BY）
SELECT '6c_inconsistent_order', id, txt FROM tb_coll_fix ORDER BY txt COLLATE "C";

-- 6d: COLLATE不一致时报错（WHERE）
-- @param String Alice
SELECT '6d_inconsistent_where', id FROM tb_coll_fix WHERE txt COLLATE "C" = ?;

-- 6e: 非加密列不受影响
DROP TABLE IF EXISTS tb_plain_coll2 CASCADE;

CREATE TABLE tb_plain_coll2 (id SERIAL, name text);

-- @param String apple
-- @param String Apple
-- @param String banana
INSERT INTO tb_plain_coll2 (name) VALUES (?), (?), (?);

SELECT '6e_plain_order', id, name FROM tb_plain_coll2 ORDER BY name COLLATE "C";

-- @param String apple
SELECT '6e_plain_where', id, name FROM tb_plain_coll2 WHERE name COLLATE "C" = ?;

DROP TABLE IF EXISTS tb_plain_coll2 CASCADE;

-- 6f: 数值加密类型不受COLLATE检查影响
CREATE TABLE tb_coll_num (id SERIAL, val int4_enc encrypted with (dek = dek_coll, support_algo = 'ope'));

-- @param int 100
INSERT INTO tb_coll_num (val) VALUES (?);

SELECT '6f_numeric', id, val FROM tb_coll_num ORDER BY val;

DROP TABLE IF EXISTS tb_coll_num CASCADE;

-- 6g: ALTER TABLE ADD COLUMN固化
CREATE TABLE tb_coll_alter (id SERIAL);

ALTER TABLE tb_coll_alter ADD COLUMN txt text_enc encrypted with (dek = dek_coll, support_algo = 'ope');

-- @param String test
INSERT INTO tb_coll_alter (txt) VALUES (?);

SELECT '6g_alter', id, txt FROM tb_coll_alter ORDER BY txt;

DROP TABLE IF EXISTS tb_coll_alter CASCADE;

DROP TABLE IF EXISTS tb_coll_fix CASCADE;

-- ============================================================
-- Cleanup
-- ============================================================
SELECT '=== Cleanup ===';

DROP KEK IF EXISTS kek_coll CASCADE;
