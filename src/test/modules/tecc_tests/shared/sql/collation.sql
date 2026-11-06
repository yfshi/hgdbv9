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

INSERT INTO test_ope_coll VALUES
(1, 'Apple',   'a red fruit',       'hello'),
(2, 'apple',   'a lowercase fruit',  'HELLO'),
(3, 'Banana',  'a yellow fruit',     'Apfel'),
(4, 'banana',  'another fruit',      'apfel'),
(5, 'cafe',    'coffee with accent', 'Zebra'),
(6, 'Cafe',    'coffee no accent',   'zebra'),
(7, 'Zebra',   'last in ASCII',      'cafe'),
(8, 'zebra',   'last lower',         'Cafe');

SELECT '2a_tag_asc', id, tag FROM test_ope_coll ORDER BY tag ASC;
SELECT '2b_tag_desc', id, tag FROM test_ope_coll ORDER BY tag DESC;
SELECT '2c_vc_asc', id, vc FROM test_ope_coll ORDER BY vc ASC;
SELECT '2d_vc_desc', id, vc FROM test_ope_coll ORDER BY vc DESC;

-- WHERE with collation
SELECT '2e_tag_eq_case', id, tag FROM test_ope_coll WHERE tag = 'apple';
SELECT '2f_tag_gt', id, tag FROM test_ope_coll WHERE tag > 'Banana' ORDER BY tag ASC;
SELECT '2g_tag_between', id, tag FROM test_ope_coll WHERE tag BETWEEN 'Apple' AND 'zebra' ORDER BY tag ASC;

-- Multi-column ORDER BY
SELECT '2h_tag_vc_asc', id, tag, vc FROM test_ope_coll ORDER BY tag ASC, vc ASC;

-- NULL handling
INSERT INTO test_ope_coll VALUES (11, NULL, NULL, NULL);
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

INSERT INTO test_coll VALUES (1, 'Alice', 100), (2, 'Bob', 200), (3, 'Charlie', 300);

SELECT '3a_select_collate', name COLLATE "POSIX" FROM test_coll WHERE id = 1;
SELECT '3b_where_collate_eq', id FROM test_coll WHERE name COLLATE "C" = 'Alice';
SELECT '3c_where_collate_paren', id FROM test_coll WHERE (name COLLATE "C") = 'Bob';
SELECT '3d_collate_like', id FROM test_coll WHERE name COLLATE "C" LIKE 'A%';
SELECT '3e_collate_multi_col', name COLLATE "POSIX", val FROM test_coll WHERE id < 3;
SELECT '3f_collate_or', id FROM test_coll WHERE name COLLATE "C" LIKE 'A%' OR name COLLATE "C" = 'Bob';
SELECT '3g_collate_ne', id FROM test_coll WHERE name COLLATE "C" != 'Bob';

DROP TABLE IF EXISTS test_coll CASCADE;

-- ============================================================
-- 4. zh_CN.utf8 ORDER BY with plain table comparison (bug7860)
-- ============================================================
SELECT '=== 4 zh_CN.utf8 ORDER BY ===';

DROP TABLE IF EXISTS tb_7860_plain CASCADE;
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
INSERT INTO tb_plain_coll (name) VALUES ('apple'), ('Apple'), ('banana'), ('Banana'), ('cherry');
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
INSERT INTO tb_coll_fix (txt, vc, bp) VALUES ('Alice', 'Bob', 'Charlie');
SELECT '6a_default_coll', id, txt, vc, bp FROM tb_coll_fix ORDER BY txt;

-- 6b: 一致则不报错
SELECT '6b_consistent', id, txt FROM tb_coll_fix ORDER BY txt COLLATE "en_US.utf8";

-- 6c: COLLATE不一致时报错（ORDER BY）
SELECT '6c_inconsistent_order', id, txt FROM tb_coll_fix ORDER BY txt COLLATE "C";

-- 6d: COLLATE不一致时报错（WHERE）
SELECT '6d_inconsistent_where', id FROM tb_coll_fix WHERE txt COLLATE "C" = 'Alice';

-- 6e: 非加密列不受影响
DROP TABLE IF EXISTS tb_plain_coll2 CASCADE;
CREATE TABLE tb_plain_coll2 (id SERIAL, name text);
INSERT INTO tb_plain_coll2 (name) VALUES ('apple'), ('Apple'), ('banana');
SELECT '6e_plain_order', id, name FROM tb_plain_coll2 ORDER BY name COLLATE "C";
SELECT '6e_plain_where', id, name FROM tb_plain_coll2 WHERE name COLLATE "C" = 'apple';
DROP TABLE IF EXISTS tb_plain_coll2 CASCADE;

-- 6f: 数值加密类型不受COLLATE检查影响
CREATE TABLE tb_coll_num (id SERIAL, val int4_enc encrypted with (dek = dek_coll, support_algo = 'ope'));
INSERT INTO tb_coll_num (val) VALUES (100);
SELECT '6f_numeric', id, val FROM tb_coll_num ORDER BY val;
DROP TABLE IF EXISTS tb_coll_num CASCADE;

-- 6g: ALTER TABLE ADD COLUMN固化
CREATE TABLE tb_coll_alter (id SERIAL);
ALTER TABLE tb_coll_alter ADD COLUMN txt text_enc encrypted with (dek = dek_coll, support_algo = 'ope');
INSERT INTO tb_coll_alter (txt) VALUES ('test');
SELECT '6g_alter', id, txt FROM tb_coll_alter ORDER BY txt;
DROP TABLE IF EXISTS tb_coll_alter CASCADE;

DROP TABLE IF EXISTS tb_coll_fix CASCADE;

-- ============================================================
-- Cleanup
-- ============================================================
SELECT '=== Cleanup ===';
DROP KEK IF EXISTS kek_coll CASCADE;
