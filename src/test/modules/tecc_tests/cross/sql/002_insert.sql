-- ============================================================
-- 002_insert.sql — 跨驱动测试：插入全类型×全算法数据
-- ============================================================

-- RND：插入各种边界值
INSERT INTO cross_rnd (c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric, c_text, c_varchar, c_bpchar) VALUES
    (1, 100, 10000, 1.5, 1.23456789012345, 123.45, 'hello rnd', 'varchar rnd', 'bpchar rnd');
INSERT INTO cross_rnd (c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric, c_text, c_varchar, c_bpchar) VALUES
    (-1, -100, -10000, -1.5, -1.23456789012345, -123.45, 'negative vals', 'neg varchar', 'neg bpchar');
INSERT INTO cross_rnd (c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric, c_text, c_varchar, c_bpchar) VALUES
    (0, 0, 0, 0.0, 0.0, 0.0, '', '', '');
INSERT INTO cross_rnd (c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric, c_text, c_varchar, c_bpchar) VALUES
    (32767, 2147483647, 9223372036854775807, 3.14, 3.14159265358979, 999999.99, '特大值测试', 'varchar特', 'bpchar特');

-- DET：插入等值查询测试数据
INSERT INTO cross_det (c_int4, c_float8, c_numeric, c_text, c_varchar) VALUES
    (100, 1.11, 10,   'det_alpha',   'det_varchar_alpha');
INSERT INTO cross_det (c_int4, c_float8, c_numeric, c_text, c_varchar) VALUES
    (200, 2.22, 20,   'det_beta',    'det_varchar_beta');
INSERT INTO cross_det (c_int4, c_float8, c_numeric, c_text, c_varchar) VALUES
    (100, 3.33, 10,   'det_gamma',   'det_varchar_gamma');
INSERT INTO cross_det (c_int4, c_float8, c_numeric, c_text, c_varchar) VALUES
    (300, 4.44, 100,  'det_alpha',   'det_varchar_alpha');
INSERT INTO cross_det (c_int4, c_float8, c_numeric, c_text, c_varchar) VALUES
    (400, 5.55, 100,  'det_delta',   'det_varchar_delta');
INSERT INTO cross_det (c_int4, c_float8, c_numeric, c_text, c_varchar) VALUES
    (500, 6.66, 1000, 'det_epsilon', 'det_varchar_epsilon');

-- OPE：插入范围查询和排序测试数据
INSERT INTO cross_ope (c_int4, c_numeric, c_text) VALUES
    (5, 10.5, 'ope_aa');
INSERT INTO cross_ope (c_int4, c_numeric, c_text) VALUES
    (15, 20.5, 'ope_bb');
INSERT INTO cross_ope (c_int4, c_numeric, c_text) VALUES
    (25, 30.5, 'ope_cc');
INSERT INTO cross_ope (c_int4, c_numeric, c_text) VALUES
    (35, 40.5, 'ope_dd');
INSERT INTO cross_ope (c_int4, c_numeric, c_text) VALUES
    (45, 50.5, 'op_ee');
INSERT INTO cross_ope (c_int4, c_numeric, c_text) VALUES
    (55, 60.5, 'ope_ff');
INSERT INTO cross_ope (c_int4, c_numeric, c_text) VALUES
    (65, 70.5, 'opeg');

-- SEARCH：插入模糊匹配测试数据
INSERT INTO cross_search (c_text, c_varchar) VALUES
    ('apple', 'apple');
INSERT INTO cross_search (c_text, c_varchar) VALUES
    ('application', 'application');
INSERT INTO cross_search (c_text, c_varchar) VALUES
    ('pineapple', 'pineapple');
INSERT INTO cross_search (c_text, c_varchar) VALUES
    ('Apple', 'Apple');
INSERT INTO cross_search (c_text, c_varchar) VALUES
    ('APPLE', 'APPLE');
INSERT INTO cross_search (c_text, c_varchar) VALUES
    ('banana', 'banana');
INSERT INTO cross_search (c_text, c_varchar) VALUES
    ('grape fruit', 'grape fruit');
INSERT INTO cross_search (c_text, c_varchar) VALUES
    ('测试', '测试');

SELECT 'insert done' AS status;
