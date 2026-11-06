-- =============================================================
-- \copy TO 解密输出测试脚本
-- 覆盖 COPY 命令常见语法和边界情况
-- =============================================================

-- 清理旧数据
DROP TABLE IF EXISTS copy_test_all_types CASCADE;
DROP TABLE IF EXISTS copy_test_partial CASCADE;
DROP TABLE IF EXISTS copy_test_single CASCADE;

-- =============================================================
-- 测试1: 全类型覆盖 - 9种加密类型
-- =============================================================
CREATE TABLE copy_test_all_types (
    id   int4_enc   ENCRYPTED WITH (DEK = copy_test_dek),
    c1   int2_enc   ENCRYPTED WITH (DEK = copy_test_dek),
    c2   int4_enc   ENCRYPTED WITH (DEK = copy_test_dek),
    c3   int8_enc   ENCRYPTED WITH (DEK = copy_test_dek),
    c4   float4_enc ENCRYPTED WITH (DEK = copy_test_dek),
    c5   float8_enc ENCRYPTED WITH (DEK = copy_test_dek),
    c6   numeric_enc ENCRYPTED WITH (DEK = copy_test_dek),
    c7   text_enc   ENCRYPTED WITH (DEK = copy_test_dek),
    c8   varchar_enc ENCRYPTED WITH (DEK = copy_test_dek),
    c9   bpchar_enc ENCRYPTED WITH (DEK = copy_test_dek)
);

INSERT INTO copy_test_all_types VALUES
    (1, 42, 100000, 900000000012345,
     3.14, 2.71828, 123.456,
     'Hello 世界', 'varchar数据', '定长字符串');

INSERT INTO copy_test_all_types VALUES
    (2, -327, -214748, -9000000000123,
     -3.4, -1.79769, -999999.999,
     '特殊字符: !@#$%^&*()_+{}|:"<>?~', 'abcd''efg', NULL);

INSERT INTO copy_test_all_types (id, c1, c2, c3, c4, c5, c6, c7) VALUES
    (3, 0, 0, 0, 0.0, 0.0, 0.0, '');

INSERT INTO copy_test_all_types (id, c1, c2, c3, c7) VALUES
    (4, 1, 2, 3, '纯文本');

-- =============================================================
-- 测试2: 部分列指定
-- =============================================================
CREATE TABLE copy_test_partial (
    id   int4_enc   ENCRYPTED WITH (DEK = copy_test_dek),
    name text_enc   ENCRYPTED WITH (DEK = copy_test_dek),
    extra int4_enc  ENCRYPTED WITH (DEK = copy_test_dek)
);

INSERT INTO copy_test_partial VALUES (1, 'Alice', 100);
INSERT INTO copy_test_partial VALUES (2, 'Bob',   200);
INSERT INTO copy_test_partial VALUES (3, 'Charlie', 300);
