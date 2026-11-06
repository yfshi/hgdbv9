-- ============================================================
-- 第1组复现脚本：OPE ORDER BY排序不正确 (7856, 7860, 7824)
-- 用法: psql -h localhost -p 5433 -U wxp -d postgres -C on -f group1_repro.sql
-- ============================================================

-- 清空旧表
DROP TABLE IF EXISTS tb_7856_enc, tb_7856_plain CASCADE;
DROP TABLE IF EXISTS tb_7860_enc, tb_7860_plain CASCADE;
DROP TABLE IF EXISTS tb_7824_enc, tb_7824_plain CASCADE;

\timing on

-- ============================================================
-- Bug 7856: collate "C" ORDER BY不符合预期
-- ============================================================
\echo '=== Bug 7856: collate "C" ORDER BY ==='

-- 普通表 (collate "C")
CREATE TABLE tb_7856_plain (
    id SERIAL PRIMARY KEY,
    text_col text collate "C",
    varchar_col varchar collate "C",
    bpchar_col bpchar collate "C"
);

-- 加密表 (collate "C", support_algo='ope')
CREATE TABLE tb_7856_enc (
    id SERIAL PRIMARY KEY,
    text_col text_enc encrypted with (dek = dek_t1, support_algo = 'ope') collate "C",
    varchar_col varchar_enc encrypted with (dek = dek_t1, support_algo = 'ope') collate "C",
    bpchar_col bpchar_enc encrypted with (dek = dek_t1, support_algo = 'ope') collate "C"
);

-- 插入测试数据 (含大小写、数字前缀、空串、NULL)
INSERT INTO tb_7856_plain (text_col, varchar_col, bpchar_col) VALUES
    ('apple', 'apple', 'apple'),
    ('Banana', 'Banana', 'Banana'),
    ('123abc', '123abc', '123abc'),
    ('ABC', 'ABC', 'ABC'),
    ('orange', 'orange', 'orange'),
    ('测试', '测试', '测试'),
    ('', '', ''),
    ('a', 'a', 'a'),
    ('Z', 'Z', 'Z'),
    (NULL, NULL, NULL),
    ('berry', 'berry', 'berry'),
    ('0start', '0start', '0start'),
    ('9end', '9end', '9end');

INSERT INTO tb_7856_enc (text_col, varchar_col, bpchar_col) VALUES
    ('apple', 'apple', 'apple'),
    ('Banana', 'Banana', 'Banana'),
    ('123abc', '123abc', '123abc'),
    ('ABC', 'ABC', 'ABC'),
    ('orange', 'orange', 'orange'),
    ('测试', '测试', '测试'),
    ('', '', ''),
    ('a', 'a', 'a'),
    ('Z', 'Z', 'Z'),
    (NULL, NULL, NULL),
    ('berry', 'berry', 'berry'),
    ('0start', '0start', '0start'),
    ('9end', '9end', '9end');

\echo '--- Plain table ORDER BY text_col ---'
SELECT id, text_col FROM tb_7856_plain ORDER BY text_col;

\echo '--- Enc table ORDER BY text_col ---'
SELECT id, text_col FROM tb_7856_enc ORDER BY text_col;

\echo '--- Plain table ORDER BY varchar_col ---'
SELECT id, varchar_col FROM tb_7856_plain ORDER BY varchar_col;

\echo '--- Enc table ORDER BY varchar_col ---'
SELECT id, varchar_col FROM tb_7856_enc ORDER BY varchar_col;

\echo '--- Plain table ORDER BY bpchar_col ---'
SELECT id, bpchar_col FROM tb_7856_plain ORDER BY bpchar_col;

\echo '--- Enc table ORDER BY bpchar_col ---'
SELECT id, bpchar_col FROM tb_7856_enc ORDER BY bpchar_col;

-- ============================================================
-- Bug 7860: collate "zh_CN.utf8" ORDER BY不符合预期
-- ============================================================
\echo '=== Bug 7860: collate "zh_CN.utf8" ORDER BY ==='

-- 普通表 (collate "zh_CN.utf8")
CREATE TABLE tb_7860_plain (
    id SERIAL PRIMARY KEY,
    text_col text collate "zh_CN.utf8",
    varchar_col varchar collate "zh_CN.utf8",
    bpchar_col bpchar collate "zh_CN.utf8"
);

-- 加密表 (collate "zh_CN.utf8", support_algo='ope')
CREATE TABLE tb_7860_enc (
    id SERIAL PRIMARY KEY,
    text_col text_enc encrypted with (dek = dek_t1, support_algo = 'ope') collate "zh_CN.utf8",
    varchar_col varchar_enc encrypted with (dek = dek_t1, support_algo = 'ope') collate "zh_CN.utf8",
    bpchar_col bpchar_enc encrypted with (dek = dek_t1, support_algo = 'ope') collate "zh_CN.utf8"
);

-- 插入测试数据 (含中文、字母、数字混合)
INSERT INTO tb_7860_plain (text_col, varchar_col, bpchar_col) VALUES
    ('测试', '测试', '测试'),
    ('application', 'application', 'application'),
    ('banana', 'banana', 'banana'),
    ('berry', 'berry', 'berry'),
    ('cherry', 'cherry', 'cherry'),
    ('123测试', '123测试', '123测试'),
    ('测试123', '测试123', '测试123'),
    ('pine测试', 'pine测试', 'pine测试'),
    ('grape fruit', 'grape fruit', 'grape fruit'),
    ('red 测试', 'red 测试', 'red 测试'),
    ('green 测试', 'green 测试', 'green 测试'),
    ('', '', ''),
    ('a', 'a', 'a'),
    ('排序规则测试 data', '排序规则测试 data', '排序规则测试 data'),
    ('data 排序规则测试', 'data 排序规则测试', 'data 排序规则测试'),
    ('排序规则测试', '排序规则测试', '排序规则测试'),
    (NULL, NULL, NULL);

INSERT INTO tb_7860_enc (text_col, varchar_col, bpchar_col) VALUES
    ('测试', '测试', '测试'),
    ('application', 'application', 'application'),
    ('banana', 'banana', 'banana'),
    ('berry', 'berry', 'berry'),
    ('cherry', 'cherry', 'cherry'),
    ('123测试', '123测试', '123测试'),
    ('测试123', '测试123', '测试123'),
    ('pine测试', 'pine测试', 'pine测试'),
    ('grape fruit', 'grape fruit', 'grape fruit'),
    ('red 测试', 'red 测试', 'red 测试'),
    ('green 测试', 'green 测试', 'green 测试'),
    ('', '', ''),
    ('a', 'a', 'a'),
    ('排序规则测试 data', '排序规则测试 data', '排序规则测试 data'),
    ('data 排序规则测试', 'data 排序规则测试', 'data 排序规则测试'),
    ('排序规则测试', '排序规则测试', '排序规则测试'),
    (NULL, NULL, NULL);

\echo '--- Plain table ORDER BY text_col (zh_CN.utf8) ---'
SELECT id, text_col FROM tb_7860_plain ORDER BY text_col;

\echo '--- Enc table ORDER BY text_col (zh_CN.utf8) ---'
SELECT id, text_col FROM tb_7860_enc ORDER BY text_col;

\echo '--- Plain table ORDER BY varchar_col (zh_CN.utf8) ---'
SELECT id, varchar_col FROM tb_7860_plain ORDER BY varchar_col;

\echo '--- Enc table ORDER BY varchar_col (zh_CN.utf8) ---'
SELECT id, varchar_col FROM tb_7860_enc ORDER BY varchar_col;

\echo '--- Plain table ORDER BY bpchar_col (zh_CN.utf8) ---'
SELECT id, bpchar_col FROM tb_7860_plain ORDER BY bpchar_col;

\echo '--- Enc table ORDER BY bpchar_col (zh_CN.utf8) ---'
SELECT id, bpchar_col FROM tb_7860_enc ORDER BY bpchar_col;

-- ============================================================
-- Bug 7824: 整型 OPE ORDER BY 结果有误
-- ============================================================
\echo '=== Bug 7824: 整型 OPE ORDER BY ==='

-- 普通表
CREATE TABLE tb_7824_plain (
    id SERIAL PRIMARY KEY,
    smallint_col int2,
    int_col int4,
    bigint_col int8
);

-- 加密表 (support_algo='ope')
CREATE TABLE tb_7824_enc (
    id SERIAL PRIMARY KEY,
    smallint_col int2_enc encrypted with (dek = dek_t1, support_algo = 'ope'),
    int_col int4_enc encrypted with (dek = dek_t1, support_algo = 'ope'),
    bigint_col int8_enc encrypted with (dek = dek_t1, support_algo = 'ope')
);

-- 插入测试数据
INSERT INTO tb_7824_plain (smallint_col, int_col, bigint_col) VALUES
    (0, 0, 0),
    (1, 1, 1),
    (-1, -1, -1),
    (32767, 32767, 32767),
    (-32768, -32768, -32768),
    (32766, 32766, 32766),
    (-32767, -32767, -32767),
    (100, 100, 100),
    (-100, -100, -100),
    (NULL, NULL, NULL);

INSERT INTO tb_7824_enc (smallint_col, int_col, bigint_col) VALUES
    (0, 0, 0),
    (1, 1, 1),
    (-1, -1, -1),
    (32767, 32767, 32767),
    (-32768, -32768, -32768),
    (32766, 32766, 32766),
    (-32767, -32767, -32767),
    (100, 100, 100),
    (-100, -100, -100),
    (NULL, NULL, NULL);

\echo '--- Plain table ORDER BY smallint_col NULLS FIRST ---'
SELECT id, smallint_col FROM tb_7824_plain ORDER BY smallint_col NULLS FIRST;

\echo '--- Enc table ORDER BY smallint_col NULLS FIRST ---'
SELECT id, smallint_col FROM tb_7824_enc ORDER BY smallint_col NULLS FIRST;

\echo '--- Plain table ORDER BY int_col ---'
SELECT id, int_col FROM tb_7824_plain ORDER BY int_col;

\echo '--- Enc table ORDER BY int_col ---'
SELECT id, int_col FROM tb_7824_enc ORDER BY int_col;

\echo '--- Plain table ORDER BY bigint_col ---'
SELECT id, bigint_col FROM tb_7824_plain ORDER BY bigint_col;

\echo '--- Enc table ORDER BY bigint_col ---'
SELECT id, bigint_col FROM tb_7824_enc ORDER BY bigint_col;

\echo '=== 第1组复现完成 ==='
