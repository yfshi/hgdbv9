-- ============================================================================
-- TECC 完整功能测试套件 — JDBC 版
-- ============================================================================
-- 原始来源: tecc_full_test_suite.sql (瀚高全密态数据库 V4.0.0)
-- 处理说明: 已移除 psql 元命令 (\echo/\set/\pset/\d/\!/\copy/\i)、
--           替换 Linux 路径为 Windows 路径 (E:/tmp/tecc-test-keys/)
--
-- 使用方法: 通过 TeccSqlTestRunner 执行
-- 版本: 1.0-jdbc
-- 处理日期: 2026-06-02
-- ============================================================================

-- 遇到错误继续执行

-- ============================================================================
-- 第一部分: 环境准备
-- ============================================================================

-- 清理旧对象
DROP TABLE IF EXISTS tecc_test CASCADE;
DROP TABLE IF EXISTS tecc_int_test CASCADE;
DROP TABLE IF EXISTS t_rnd CASCADE;
DROP TABLE IF EXISTS plain CASCADE;
DROP TABLE IF EXISTS enc CASCADE;
DROP TABLE IF EXISTS t_det CASCADE;
DROP TABLE IF EXISTS t_ope CASCADE;
DROP TABLE IF EXISTS t_search CASCADE;
DROP TABLE IF EXISTS t_det_search CASCADE;
DROP TABLE IF EXISTS t_ope_search CASCADE;
DROP TABLE IF EXISTS t_float4_test CASCADE;
DROP TABLE IF EXISTS t_rnd_idx_bug CASCADE;
DROP TABLE IF EXISTS b1_numeric_det CASCADE;
DROP TABLE IF EXISTS b1_v2 CASCADE;
DROP TABLE IF EXISTS b1_v3 CASCADE;
DROP TABLE IF EXISTS b1_test CASCADE;
DROP TABLE IF EXISTS bug1 CASCADE;
DROP TABLE IF EXISTS bug2 CASCADE;
DROP TABLE IF EXISTS bug2b CASCADE;
DROP TABLE IF EXISTS bug4 CASCADE;
DROP TABLE IF EXISTS tecc_test_all CASCADE;
DROP USER IF EXISTS tecc_test_user;
DROP DEK IF EXISTS test_dek;
DROP KEK IF EXISTS test_kek;

-- 创建密钥目录

-- 创建 KEK
CREATE KEK test_kek WITH (KEY_STORE = localkms, KEY_PATH = '/tmp/test_kek.key');

-- 创建 DEK
CREATE DEK test_dek WITH VALUES(KEK = test_kek);


-- ============================================================================
-- 第二部分: 密态数据类型和建表
-- ============================================================================

SELECT typname FROM pg_type WHERE typname IN (
    'text_enc','varchar_enc','bpchar_enc',
    'int2_enc','int4_enc','int8_enc',
    'numeric_enc','float4_enc','float8_enc'
) ORDER BY typname;


-- det only (等值)
DROP TABLE IF EXISTS t_det;
CREATE TABLE t_det (id int, name text_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'det'));

-- ope only (范围)
DROP TABLE IF EXISTS t_ope;
CREATE TABLE t_ope (id int, name text_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'ope'));

-- search only (模糊)
DROP TABLE IF EXISTS t_search;
CREATE TABLE t_search (id int, name text_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'search'));

-- det + search
DROP TABLE IF EXISTS t_det_search;
CREATE TABLE t_det_search (id int, name text_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'det,search'));

-- ope + search
DROP TABLE IF EXISTS t_ope_search;
CREATE TABLE t_ope_search (id int, name text_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'ope,search'));

-- RND (不指定 support_algo)
DROP TABLE IF EXISTS t_rnd;
CREATE TABLE t_rnd (id int, name text_enc ENCRYPTED WITH (dek = test_dek));

-- det + ope (应互斥失败)
DROP TABLE IF EXISTS t_det_ope;
CREATE TABLE t_det_ope (id int, name text_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'det,ope'));

-- 字符型密态列建表时支持设置排序规则
DROP TABLE IF EXISTS t_collate;
CREATE TABLE t_collate (id int, name varchar_enc(100) ENCRYPTED WITH (dek = test_dek, support_algo = 'det') COLLATE "C");


-- ============================================================================
-- 第三部分: 密态数据写入(INSERT)
-- ============================================================================

DROP TABLE IF EXISTS tecc_test;
CREATE TABLE tecc_test (
    id          serial PRIMARY KEY,
    col_text    text_enc    ENCRYPTED WITH (dek = test_dek, support_algo = 'det,search'),
    col_varchar varchar_enc(100) ENCRYPTED WITH (dek = test_dek, support_algo = 'ope,search'),
    col_bpchar  bpchar_enc(20)  ENCRYPTED WITH (dek = test_dek, support_algo = 'det'),
    col_int2    int2_enc    ENCRYPTED WITH (dek = test_dek, support_algo = 'det'),
    col_int4    int4_enc    ENCRYPTED WITH (dek = test_dek, support_algo = 'det'),
    col_int8    int8_enc    ENCRYPTED WITH (dek = test_dek, support_algo = 'ope'),
    col_float4  float4_enc  ENCRYPTED WITH (dek = test_dek, support_algo = 'det'),
    col_float8  float8_enc  ENCRYPTED WITH (dek = test_dek, support_algo = 'ope'),
    col_numeric numeric_enc(10,2) ENCRYPTED WITH (dek = test_dek, support_algo = 'det')
);

-- 插入数据（全部 9 种密态类型，中文+英文+数字+NULL+空串+边界值）
INSERT INTO tecc_test (col_text, col_varchar, col_bpchar,
                       col_int2, col_int4, col_int8,
                       col_float4, col_float8, col_numeric) VALUES
('张三',      '李四',     '王五',      100,     20000,     3000000,        3.14,        2.718281828,   99999.99),
('Alice',     'Bob',      'Charlie',   1,       1000,      50000,          1.5,         100.5,         123.45),
('张三丰',    'David',    'Eve',       -100,    -50000,    -999999,        -2.5,        -0.001,        -500.00),
('hello',     'world',    'test',      0,       0,         0,              0.0,         0.0,           0.00),
('alice',     'ALICE',    'alice',     1,       1,         1,              1.0,         1.0,           1.00),
('张三丰',    'David2',   'Eve2',      100,     20000,     3000000,        3.14,        2.718281828,   99999.99),
('Hello',     'World',    'TEST',      50,      55555,     7777777,        50.5,        55.55,         500.00),
('abc_def',   'xyz_123',  'foo_bar',   -1,      -1,        -1,             -1.0,        -1.0,          -1.00),
('test%like', 'test_like','pct_sign',  10,      100,       1000,           10.10,       100.10,        10.10),
('%percent',  'under_score','_start',  20,      200,       2000,           20.20,       200.20,        20.20),
(NULL,        NULL,       NULL,        NULL,    NULL,      NULL,           NULL,        NULL,          NULL),
('',          '',         '',          -32768,  -2147483648, -9223372036854775808, -3.402823e38, -1.7976931348e308, -99999.99),
('测试',      '测试',     '测试',     42,      4242,      424242,         4.2,         42.42,         42.42),
('test123',   'abcXYZ',   'mixCase',   255,     65535,     16777215,       255.255,     65535.65535,   255.55);

SELECT 'tecc_test' AS tbl, count(*) AS row_count FROM tecc_test;

SELECT id, col_text, col_int4, col_numeric FROM tecc_test ORDER BY id LIMIT 5;


-- ============================================================================
-- 第四部分: 密态等值查询 (det: =, <>)
-- ============================================================================

SELECT id, col_text FROM tecc_test WHERE col_text = '张三';

SELECT id, col_bpchar FROM tecc_test WHERE col_bpchar <> '王五' ORDER BY id;

SELECT id, col_int4 FROM tecc_test WHERE col_int4 = 20000;

SELECT id, col_int2 FROM tecc_test WHERE col_int2 <> 0 ORDER BY id;

SELECT id, col_float4 FROM tecc_test WHERE col_float4 = 1.5;

SELECT id, col_int2 FROM tecc_test WHERE col_int2 = -100;

SELECT id, col_int2 FROM tecc_test WHERE col_int2 = 0;

SELECT id, col_text FROM tecc_test WHERE col_text = '';

SELECT id, col_int4 FROM tecc_test WHERE col_int4 > 1000;

SELECT id, name FROM t_rnd WHERE name = 'test';


-- ============================================================================
-- 第五部分: 密态范围查询 (ope: >, >=, <, <=)
-- ============================================================================

SELECT id, col_int8 FROM tecc_test WHERE col_int8 > 0 ORDER BY id;

SELECT id, col_int8 FROM tecc_test WHERE col_int8 >= 50000 ORDER BY id;

SELECT id, col_int8 FROM tecc_test WHERE col_int8 < 0 ORDER BY id;

SELECT id, col_float8 FROM tecc_test WHERE col_float8 < 1.0 ORDER BY id;

SELECT id, col_int8 FROM tecc_test WHERE col_int8 > 0 AND col_int8 < 1000000 ORDER BY id;

SELECT id, col_int8 FROM tecc_test WHERE col_int8 = 50000;

SELECT id, col_varchar FROM tecc_test WHERE col_varchar > 'David' ORDER BY id;

SELECT id, col_varchar FROM tecc_test WHERE col_varchar >= 'Bob' ORDER BY id;


-- ============================================================================
-- 第六部分: 密态模糊查询 (search: LIKE, NOT LIKE)
-- ============================================================================

SELECT id, col_text FROM tecc_test WHERE col_text LIKE '张%' ORDER BY id;

SELECT id, col_varchar FROM tecc_test WHERE col_varchar LIKE '%b%' ORDER BY id;

SELECT id, col_text FROM tecc_test WHERE col_text NOT LIKE '%张%' ORDER BY id;

SELECT id, col_text FROM tecc_test WHERE col_text LIKE 'test%' ORDER BY id;

SELECT id, col_varchar FROM tecc_test WHERE col_varchar LIKE 'ALICE' ORDER BY id;

SELECT id, col_varchar FROM tecc_test WHERE col_varchar LIKE '_ob' ORDER BY id;

SELECT id, col_bpchar FROM tecc_test WHERE col_bpchar LIKE '王%';


-- ============================================================================
-- 第七部分: 密态类型转换
-- ============================================================================

DROP TABLE IF EXISTS tecc_int_test;
CREATE TABLE tecc_int_test (
    id serial PRIMARY KEY,
    col_int2   int2_enc   ENCRYPTED WITH (dek = test_dek, support_algo = 'det'),
    col_int4   int4_enc   ENCRYPTED WITH (dek = test_dek, support_algo = 'det'),
    col_int8   int8_enc   ENCRYPTED WITH (dek = test_dek, support_algo = 'ope'),
    col_float4 float4_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'det'),
    col_float8 float8_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'ope'),
    col_numeric numeric_enc(10,2) ENCRYPTED WITH (dek = test_dek, support_algo = 'det')
);
INSERT INTO tecc_int_test (col_int2, col_int4, col_int8, col_float4, col_float8, col_numeric)
VALUES (100, 20000, 3000000, 3.14, 2.718, 99999.99);

SELECT col_int2::int4_enc FROM tecc_int_test;

SELECT col_int2::int8_enc FROM tecc_int_test;

SELECT col_int4::int8_enc FROM tecc_int_test;

SELECT col_int2::float4_enc FROM tecc_int_test;

SELECT col_int2::float8_enc FROM tecc_int_test;

SELECT col_int2::numeric_enc FROM tecc_int_test;

SELECT col_float4::float8_enc FROM tecc_int_test;

SELECT col_float4::numeric_enc FROM tecc_int_test;

SELECT col_float8::numeric_enc FROM tecc_int_test;

SELECT col_text::varchar_enc FROM tecc_test ORDER BY id LIMIT 3;

SELECT col_varchar::text_enc FROM tecc_test ORDER BY id LIMIT 3;

SELECT col_varchar::bpchar_enc FROM tecc_test ORDER BY id LIMIT 3;

SELECT col_text::int4_enc FROM tecc_test LIMIT 1;


-- ============================================================================
-- 第八部分: 密态索引
-- ============================================================================

CREATE INDEX idx_tecc_int4_hash ON tecc_test USING hash(col_int4);

CREATE INDEX idx_tecc_text_btree ON tecc_test(col_text);

CREATE INDEX idx_tecc_int8_ope ON tecc_test(col_int8);

CREATE INDEX idx_t_rnd_btree ON t_rnd(name);

SELECT indexrelid::regclass, indrelid::regclass, amname
FROM pg_index i JOIN pg_am a ON i.indrelid = a.oid
WHERE indrelid IN ('tecc_test'::regclass, 't_rnd'::regclass);

INSERT INTO t_rnd (name) VALUES ('test_rnd_1');
INSERT INTO t_rnd (name) VALUES ('test_rnd_2');


-- ============================================================================
-- 第九部分: RND随机加密算法
-- ============================================================================

-- 先清理RND索引避免干扰
DROP INDEX IF EXISTS idx_t_rnd_btree;

INSERT INTO t_rnd (name) VALUES ('same_text');
INSERT INTO t_rnd (name) VALUES ('same_text');
SELECT id, name FROM t_rnd ORDER BY id;

SELECT * FROM t_rnd WHERE name = 'same_text';

SELECT * FROM t_rnd WHERE name > 'a';

SELECT * FROM t_rnd WHERE name LIKE '%text%';


-- ============================================================================
-- 第十部分: 加解密权限控制
-- ============================================================================

-- ===== 权限控制测试（GRANT/REVOKE KEK/DEK） =====
DROP USER IF EXISTS tecc_test_user;
CREATE USER tecc_test_user WITH PASSWORD 'Test@12345';
GRANT USAGE ON SCHEMA public TO tecc_test_user;
GRANT SELECT, INSERT ON tecc_test TO tecc_test_user;

GRANT USAGE ON KEK test_kek TO tecc_test_user;
GRANT USAGE ON DEK test_dek TO tecc_test_user;

REVOKE USAGE ON DEK test_dek FROM tecc_test_user;

REVOKE ALL ON SCHEMA public FROM tecc_test_user;
REVOKE USAGE ON KEK test_kek FROM tecc_test_user;
REVOKE ALL ON tecc_test FROM tecc_test_user;
DROP USER IF EXISTS tecc_test_user;


-- ============================================================================
-- 第十一部分: 查询正确性对比（明文 vs 密态）
-- ============================================================================

-- 创建明文对照表
DROP TABLE IF EXISTS plain;
CREATE TABLE plain (
    id          serial PRIMARY KEY,
    c_text      text,
    c_varchar   varchar(100),
    c_bpchar    bpchar(20),
    c_int2      int2,
    c_int4      int4,
    c_int8      int8,
    c_float4    float4,
    c_float8    float8,
    c_numeric   numeric(10,2)
);

-- 创建密态对照表
DROP TABLE IF EXISTS enc;
CREATE TABLE enc (
    id          serial PRIMARY KEY,
    c_text_det      text_enc    ENCRYPTED WITH (dek = test_dek, support_algo = 'det,search'),
    c_text_ope      text_enc    ENCRYPTED WITH (dek = test_dek, support_algo = 'ope,search'),
    c_varchar_det   varchar_enc(100) ENCRYPTED WITH (dek = test_dek, support_algo = 'det,search'),
    c_varchar_ope   varchar_enc(100) ENCRYPTED WITH (dek = test_dek, support_algo = 'ope,search'),
    c_bpchar_det    bpchar_enc(20)   ENCRYPTED WITH (dek = test_dek, support_algo = 'det,search'),
    c_bpchar_ope    bpchar_enc(20)   ENCRYPTED WITH (dek = test_dek, support_algo = 'ope,search'),
    c_int2_det     int2_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'det'),
    c_int2_ope     int2_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'ope'),
    c_int4_det     int4_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'det'),
    c_int4_ope     int4_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'ope'),
    c_int8_det     int8_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'det'),
    c_int8_ope     int8_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'ope'),
    c_float4_det   float4_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'det'),
    c_float4_ope   float4_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'ope'),
    c_float8_det   float8_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'det'),
    c_float8_ope   float8_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'ope'),
    c_numeric_det  numeric_enc(10,2) ENCRYPTED WITH (dek = test_dek, support_algo = 'det'),
    c_numeric_ope  numeric_enc(10,2) ENCRYPTED WITH (dek = test_dek, support_algo = 'ope')
);

-- 向明文表和密态表插入相同数据
INSERT INTO plain (c_text, c_varchar, c_bpchar, c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric) VALUES
('张三',      '李四',     '王五',      100,     20000,     3000000,        3.14,        2.718281828,   99999.99),
('Alice',     'Bob',      'Charlie',   1,       1000,      50000,          1.5,         100.5,         123.45),
('赵六',      '孙七',     '周八',      32767,   2147483647, 9223372036854775807, 1.0e10, 1.7976931348623e308, 0.01),
('张三丰',    'David',    'Eve',       -100,    -50000,    -999999,        -2.5,        -0.001,        -500.00),
('hello',     'world',    'test',      0,       0,         0,              0.0,         0.0,           0.00),
('alice',     'ALICE',    'alice',     1,       1,         1,              1.0,         1.0,           1.00),
('张三丰',    'David2',   'Eve2',      100,     20000,     3000000,        3.14,        2.718281828,   99999.99),
('Hello',     'World',    'TEST',      50,      55555,     7777777,        50.5,        55.55,         500.00),
('abc_def',   'xyz_123',  'foo_bar',   -1,      -1,        -1,             -1.0,        -1.0,          -1.00),
('test%like', 'test_like','pct_sign',  10,      100,       1000,           10.10,       100.10,        10.10),
('%percent',  'under_score','_start',  20,      200,       2000,           20.20,       200.20,        20.20),
(NULL,        NULL,       NULL,        NULL,    NULL,      NULL,           NULL,        NULL,          NULL),
('',          '',         '',          -32768,  -2147483648, -9223372036854775808, -3.402823e38, -1.7976931348e308, -99999.99),
('测试',      '测试',     '测试',     42,      4242,      424242,         4.2,         42.42,         42.42),
('test123',   'abcXYZ',   'mixCase',   255,     65535,     16777215,       255.255,     65535.65535,   255.55);

INSERT INTO enc (c_text_det, c_text_ope, c_varchar_det, c_varchar_ope, c_bpchar_det, c_bpchar_ope,
                 c_int2_det, c_int2_ope, c_int4_det, c_int4_ope, c_int8_det, c_int8_ope,
                 c_float4_det, c_float4_ope, c_float8_det, c_float8_ope, c_numeric_det, c_numeric_ope) VALUES
('张三','张三','李四','李四','王五','王五', 100,100, 20000,20000, 3000000,3000000, 3.14,3.14, 2.718281828,2.718281828, 99999.99,99999.99),
('Alice','Alice','Bob','Bob','Charlie','Charlie', 1,1, 1000,1000, 50000,50000, 1.5,1.5, 100.5,100.5, 123.45,123.45),
('赵六','赵六','孙七','孙七','周八','周八', 32767,32767, 2147483647,2147483647, 9223372036854775807,9223372036854775807, 1.0e10,1.0e10, 1.7976931348623e308,1.7976931348623e308, 0.01,0.01),
('张三丰','张三丰','David','David','Eve','Eve', -100,-100, -50000,-50000, -999999,-999999, -2.5,-2.5, -0.001,-0.001, -500.00,-500.00),
('hello','hello','world','world','test','test', 0,0, 0,0, 0,0, 0.0,0.0, 0.0,0.0, 0.00,0.00),
('alice','alice','ALICE','ALICE','alice','alice', 1,1, 1,1, 1,1, 1.0,1.0, 1.0,1.0, 1.00,1.00),
('张三丰','张三丰','David2','David2','Eve2','Eve2', 100,100, 20000,20000, 3000000,3000000, 3.14,3.14, 2.718281828,2.718281828, 99999.99,99999.99),
('Hello','Hello','World','World','TEST','TEST', 50,50, 55555,55555, 7777777,7777777, 50.5,50.5, 55.55,55.55, 500.00,500.00),
('abc_def','abc_def','xyz_123','xyz_123','foo_bar','foo_bar', -1,-1, -1,-1, -1,-1, -1.0,-1.0, -1.0,-1.0, -1.00,-1.00),
('test%like','test%like','test_like','test_like','pct_sign','pct_sign', 10,10, 100,100, 1000,1000, 10.10,10.10, 100.10,100.10, 10.10,10.10),
('%percent','%percent','under_score','under_score','_start','_start', 20,20, 200,200, 2000,2000, 20.20,20.20, 200.20,200.20, 20.20,20.20),
(NULL,NULL, NULL,NULL, NULL,NULL, NULL,NULL, NULL,NULL, NULL,NULL, NULL,NULL, NULL,NULL, NULL,NULL),
('','','','','','', -32768,-32768, -2147483648,-2147483648, -9223372036854775808,-9223372036854775808, -3.402823e38,-3.402823e38, -1.7976931348e308,-1.7976931348e308, -99999.99,-99999.99),
('测试','测试','测试','测试','测试','测试', 42,42, 4242,4242, 424242,424242, 4.2,4.2, 42.42,42.42, 42.42,42.42),
('test123','test123','abcXYZ','abcXYZ','mixCase','mixCase', 255,255, 65535,65535, 16777215,16777215, 255.255,255.255, 65535.65535,65535.65535, 255.55,255.55);


-- 对比函数: 每个查询打印 明文结果 | 密态结果


-- ============================================================================
-- 第十二部分: Bug 回归测试
-- ============================================================================

SELECT id, c_numeric_det FROM enc WHERE c_numeric_det = 99999.99;
SELECT id, c_numeric FROM plain WHERE c_numeric = 99999.99;

SELECT id, c_int8_ope FROM enc WHERE c_int8_ope < 0 ORDER BY id;
SELECT id, c_int8 FROM plain WHERE c_int8 < 0 ORDER BY id;


SELECT id, c_int4_ope FROM enc WHERE c_int4_ope >= 20000 ORDER BY id;

SELECT id, c_float8_ope FROM enc WHERE c_float8_ope < 0 ORDER BY id;

SELECT id, c_numeric_ope FROM enc WHERE c_numeric_ope > -500 ORDER BY id;

SELECT id, c_varchar_ope FROM enc WHERE c_varchar_ope > 'David' ORDER BY id;
SELECT id, c_bpchar_ope FROM enc WHERE c_bpchar_ope > '测试' ORDER BY id;
SELECT id, c_bpchar FROM plain WHERE c_bpchar > '测试' ORDER BY id;

SELECT id, c_text_det FROM enc WHERE c_text_det LIKE '张%' ORDER BY id;
SELECT id, c_text FROM plain WHERE c_text LIKE '张%' ORDER BY id;

SELECT id, c_int8_ope FROM enc WHERE c_int8_ope BETWEEN -10 AND 10 ORDER BY id;
SELECT id, c_int8_ope FROM enc WHERE c_int8_ope < 0 ORDER BY id;

SELECT id, c_varchar_ope FROM enc WHERE c_varchar_ope LIKE 'test%' ORDER BY id;

SELECT id, c_float4_ope FROM enc WHERE c_float4_ope > 1.0 ORDER BY id;
SELECT id, c_float4 FROM plain WHERE c_float4 > 1.0 ORDER BY id;


-- 正确前提: 相同DEK + 相同算法
DROP TABLE IF EXISTS b12_test;
CREATE TABLE b12_test (
    id int,
    v1 int2_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'det'),
    v2 int2_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'det')
);
INSERT INTO b12_test VALUES (1, 100, 100);
INSERT INTO b12_test VALUES (2, 100, 200);
INSERT INTO b12_test VALUES (3, 50,  50);

SELECT * FROM b12_test WHERE v1 = v2 ORDER BY id;

SELECT * FROM b12_test WHERE v1 <> v2 ORDER BY id;

SELECT id, c_int2_det, c_int2_ope FROM enc WHERE c_int2_det = c_int2_ope ORDER BY id;

DROP TABLE IF EXISTS b12_test;

-- B13: LIKE ESCAPE 暂不支持，跳过

-- ============================================================================
-- 第十三部分: 边界值 + 等价类补充测试
-- ============================================================================

-- ===== 13.1 int2 边界值 =====

DROP TABLE IF EXISTS b_int2;
CREATE TABLE b_int2(id serial PRIMARY KEY, v_det int2_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope int2_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
INSERT INTO b_int2(v_det,v_ope) VALUES (32767,32767),(-32768,-32768),(1,1),(-1,-1),(0,0),(100,100),(-100,-100),(100,100),(NULL,NULL),(-32767,-32767);

SELECT 'max(32767)' t, count(*) FROM b_int2 WHERE v_det=32767
UNION ALL SELECT 'min(-32768)', count(*) FROM b_int2 WHERE v_det=-32768
UNION ALL SELECT 'zero(0)', count(*) FROM b_int2 WHERE v_det=0
UNION ALL SELECT 'dup(100)', count(*) FROM b_int2 WHERE v_det=100
UNION ALL SELECT 'NULL', count(*) FROM b_int2 WHERE v_det IS NULL;

SELECT '>0' t, count(*) FROM b_int2 WHERE v_ope>0
UNION ALL SELECT '<0', count(*) FROM b_int2 WHERE v_ope<0
UNION ALL SELECT 'BETWEEN -10 AND 10', count(*) FROM b_int2 WHERE v_ope BETWEEN -10 AND 10;

-- ===== 13.2 int4 边界值 =====

DROP TABLE IF EXISTS b_int4;
CREATE TABLE b_int4(id serial PRIMARY KEY, v_det int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
INSERT INTO b_int4(v_det,v_ope) VALUES (2147483647,2147483647),(-2147483648,-2147483648),(0,0),(1,1),(-1,-1),(100000,100000),(-100000,-100000),(2147483646,2147483646),(-2147483647,-2147483647),(NULL,NULL);

SELECT 'max' t, count(*) FROM b_int4 WHERE v_det=2147483647
UNION ALL SELECT 'min', count(*) FROM b_int4 WHERE v_det=-2147483648
UNION ALL SELECT 'max-1', count(*) FROM b_int4 WHERE v_det=2147483646
UNION ALL SELECT 'min+1', count(*) FROM b_int4 WHERE v_det=-2147483647
UNION ALL SELECT 'zero', count(*) FROM b_int4 WHERE v_det=0;

SELECT '>0' t, count(*) FROM b_int4 WHERE v_ope>0
UNION ALL SELECT '<0', count(*) FROM b_int4 WHERE v_ope<0
UNION ALL SELECT '>0 AND <1000000', count(*) FROM b_int4 WHERE v_ope>0 AND v_ope<1000000;

-- ===== 13.3 int8 边界值 =====

DROP TABLE IF EXISTS b_int8;
CREATE TABLE b_int8(id serial PRIMARY KEY, v_det int8_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope int8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
INSERT INTO b_int8(v_det,v_ope) VALUES (9223372036854775807,9223372036854775807),(-9223372036854775808,-9223372036854775808),(0,0),(1,1),(-1,-1),(1000000000,1000000000),(-1000000000,-1000000000),(9223372036854775806,9223372036854775806),(-9223372036854775807,-9223372036854775807),(NULL,NULL);

SELECT 'max' t, count(*) FROM b_int8 WHERE v_det=9223372036854775807
UNION ALL SELECT 'min', count(*) FROM b_int8 WHERE v_det=-9223372036854775808
UNION ALL SELECT 'max-1', count(*) FROM b_int8 WHERE v_det=9223372036854775806
UNION ALL SELECT 'min+1', count(*) FROM b_int8 WHERE v_det=-9223372036854775807
UNION ALL SELECT 'zero', count(*) FROM b_int8 WHERE v_det=0;

SELECT '>0' t, count(*) FROM b_int8 WHERE v_ope>0
UNION ALL SELECT '<0', count(*) FROM b_int8 WHERE v_ope<0;

-- ===== 13.4 float4 边界值 =====

DROP TABLE IF EXISTS b_float4;
CREATE TABLE b_float4(id serial PRIMARY KEY, v_det float4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope float4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
INSERT INTO b_float4(v_det,v_ope) VALUES (3.402823466E+38,3.402823466E+38),(-3.402823466E+38,-3.402823466E+38),(0.0,0.0),(1.0,1.0),(-1.0,-1.0),(0.001,0.001),(-0.001,-0.001),(3.14,3.14),(NULL,NULL);

SELECT '=0.0' t, count(*) FROM b_float4 WHERE v_det=0.0
UNION ALL SELECT '=1.0', count(*) FROM b_float4 WHERE v_det=1.0
UNION ALL SELECT '=3.14', count(*) FROM b_float4 WHERE v_det=3.14
UNION ALL SELECT 'IS NULL', count(*) FROM b_float4 WHERE v_det IS NULL;

SELECT '>0' t, count(*) FROM b_float4 WHERE v_ope>0
UNION ALL SELECT '<0', count(*) FROM b_float4 WHERE v_ope<0;

-- ===== 13.5 float8 边界值 =====

DROP TABLE IF EXISTS b_float8;
CREATE TABLE b_float8(id serial PRIMARY KEY, v_det float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
INSERT INTO b_float8(v_det,v_ope) VALUES (100.5,100.5),(0.0,0.0),(1.0,1.0),(-1.0,-1.0),(2.718,2.718),(0.001,0.001),(-0.001,-0.001),(NULL,NULL);

SELECT '=0.0' t, count(*) FROM b_float8 WHERE v_det=0.0
UNION ALL SELECT '=1.0', count(*) FROM b_float8 WHERE v_det=1.0;

SELECT '>0' t, count(*) FROM b_float8 WHERE v_ope>0
UNION ALL SELECT '<0', count(*) FROM b_float8 WHERE v_ope<0
UNION ALL SELECT '>=1.0', count(*) FROM b_float8 WHERE v_ope>=1.0;

-- ===== 13.6 numeric 边界值 =====

DROP TABLE IF EXISTS b_numeric;
CREATE TABLE b_numeric(id serial PRIMARY KEY, v numeric_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'));
INSERT INTO b_numeric(v) VALUES (99999.99),(-99999.99),(0),(1),(-1),(0.01),(-0.01),(12345.67),(99999.99),(99999.99),(NULL);

SELECT '=0' t, count(*) FROM b_numeric WHERE v=0
UNION ALL SELECT '=12345.67', count(*) FROM b_numeric WHERE v=12345.67
UNION ALL SELECT '=99999.99(dup)', count(*) FROM b_numeric WHERE v=99999.99
UNION ALL SELECT 'IS NULL', count(*) FROM b_numeric WHERE v IS NULL;

SELECT '>0' t, count(*) FROM b_numeric WHERE v>0
UNION ALL SELECT '<0', count(*) FROM b_numeric WHERE v<0;

-- ===== 13.7 text 等价类+特殊字符 =====

DROP TABLE IF EXISTS b_text;
CREATE TABLE b_text(id serial PRIMARY KEY, v text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'));
INSERT INTO b_text(v) VALUES (''),('a'),('hello'),('Hello'),('HELLO'),('张三'),('张三丰'),('abc_def'),('test%like'),('test_like'),('a%b_c'),('  空格  '),('12345'),(NULL);

SELECT '='''' (空串)' t, count(*) FROM b_text WHERE v=''
UNION ALL SELECT '=hello', count(*) FROM b_text WHERE v='hello'
UNION ALL SELECT '=Hello', count(*) FROM b_text WHERE v='Hello'
UNION ALL SELECT '=张三', count(*) FROM b_text WHERE v='张三'
UNION ALL SELECT '=a%b_c', count(*) FROM b_text WHERE v='a%b_c'
UNION ALL SELECT '=12345', count(*) FROM b_text WHERE v='12345'
UNION ALL SELECT '<>''''', count(*) FROM b_text WHERE v<>'';

-- ===== 13.8 NULL/空串专项 =====

DROP TABLE IF EXISTS b_null;
CREATE TABLE b_null(id serial PRIMARY KEY, c1 text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), c2 int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), c3 float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
INSERT INTO b_null(c1,c2,c3) VALUES (NULL,NULL,NULL),('',0,0.0),('a',1,1.0),(NULL,2,2.0),('b',NULL,3.0),('c',3,NULL);

SELECT 'c1 IS NULL' t, count(*) FROM b_null WHERE c1 IS NULL
UNION ALL SELECT 'c2 IS NULL', count(*) FROM b_null WHERE c2 IS NULL
UNION ALL SELECT 'c3 IS NULL', count(*) FROM b_null WHERE c3 IS NULL
UNION ALL SELECT 'all NULL', count(*) FROM b_null WHERE c1 IS NULL AND c2 IS NULL AND c3 IS NULL
UNION ALL SELECT 'c1=''''', count(*) FROM b_null WHERE c1=''
UNION ALL SELECT 'c2=0 AND c3=0.0', count(*) FROM b_null WHERE c2=0 AND c3=0.0;

SELECT 'c1 NOT NULL' t, count(*) FROM b_null WHERE c1 IS NOT NULL
UNION ALL SELECT 'c2 NOT NULL', count(*) FROM b_null WHERE c2 IS NOT NULL
UNION ALL SELECT 'c3 NOT NULL', count(*) FROM b_null WHERE c3 IS NOT NULL;

-- ===== 13.9 LIKE 等价类 =====

DROP TABLE IF EXISTS b_like;
CREATE TABLE b_like(id serial PRIMARY KEY, v text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'));
INSERT INTO b_like(v) VALUES ('abc'),('abcd'),('abcde'),('xabc'),('xabcy'),('ab'),(''),('张三'),('张三丰'),('abc_def'),('abc%def'),(NULL);

SELECT 'LIKE ''abc%''' t, count(*) FROM b_like WHERE v LIKE 'abc%'
UNION ALL SELECT 'LIKE ''ab%''' , count(*) FROM b_like WHERE v LIKE 'ab%'
UNION ALL SELECT 'LIKE ''张%''' , count(*) FROM b_like WHERE v LIKE '张%';

SELECT 'LIKE ''%abc%''' t, count(*) FROM b_like WHERE v LIKE '%abc%'
UNION ALL SELECT 'LIKE ''%bc%''' , count(*) FROM b_like WHERE v LIKE '%bc%';

SELECT 'NOT LIKE ''abc%''' t, count(*) FROM b_like WHERE v NOT LIKE 'abc%' AND v IS NOT NULL;
SELECT 'LIKE ''ab_''' t, count(*) FROM b_like WHERE v LIKE 'ab_';
SELECT 'LIKE ''abc''(精确)' t, count(*) FROM b_like WHERE v LIKE 'abc'
UNION ALL SELECT 'LIKE ''''''' , count(*) FROM b_like WHERE v LIKE '';

-- ===== 13.10 类型转换边界值 =====

DROP TABLE IF EXISTS b_cast;
CREATE TABLE b_cast(id serial PRIMARY KEY, c2 int2_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), c4 int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), c8 int8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'), f4 float4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), f8 float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'), num numeric_enc(10,2) ENCRYPTED WITH (dek=test_dek,support_algo='det'));
INSERT INTO b_cast(c2,c4,c8,f4,f8,num) VALUES (32767,2147483647,9223372036854775807,3.4028235E38,1.7976931348623157E308,99999999.99),(-32768,-2147483648,-9223372036854775808,-3.4028235E38,-1.7976931348623157E308,-99999999.99),(0,0,0,0.0,0.0,0.00);


-- ===== 13.11 多类型混合查询 =====

DROP TABLE IF EXISTS b_mixed;
CREATE TABLE b_mixed(id serial PRIMARY KEY, name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), age int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'), salary float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
INSERT INTO b_mixed(name,age,salary) VALUES ('张三',25,5000),('李四',30,8000),('王五',28,6000),('赵六',35,12000),('Alice',28,6500),('Bob',30,8500),('Charlie',35,11000),('张三丰',22,4000),('Eve',40,15000),(NULL,25,5000),('',0,0),('David',NULL,9000);

SELECT 'name=''张三'' AND age=25' t, count(*) FROM b_mixed WHERE name='张三' AND age=25
UNION ALL SELECT 'age=30 AND salary>8000', count(*) FROM b_mixed WHERE age=30 AND salary>8000
UNION ALL SELECT 'age=28 OR age=35', count(*) FROM b_mixed WHERE age=28 OR age=35
UNION ALL SELECT 'LIKE+range', count(*) FROM b_mixed WHERE name LIKE '张%' AND age<30;

SELECT 'age IN(28,30)' t, count(*) FROM b_mixed WHERE age=28 OR age=30
UNION ALL SELECT 'age NOT IN(28,30)', count(*) FROM b_mixed WHERE age<>28 AND age<>30 AND age IS NOT NULL;

SELECT 'age 25-30' t, count(*) FROM b_mixed WHERE age>=25 AND age<=30
UNION ALL SELECT 'salary 5000-10000', count(*) FROM b_mixed WHERE salary>=5000 AND salary<=10000;

-- ===== 13.12 聚合+排序 =====

SELECT 'count(*)' t, count(*) FROM b_int2
UNION ALL SELECT 'count(v_det)', count(v_det) FROM b_int2
UNION ALL SELECT 'count(DISTINCT v_det)', count(DISTINCT v_det) FROM b_int2;


-- ===== 13.13 两密态列比较 =====

DROP TABLE IF EXISTS b_colcmp;
CREATE TABLE b_colcmp(id serial PRIMARY KEY, v1 int2_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v2 int2_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'));
INSERT INTO b_colcmp(v1,v2) VALUES (100,100),(100,200),(50,50),(-10,-10),(NULL,NULL),(300,300);

SELECT 'v1=v2' t, count(*) FROM b_colcmp WHERE v1=v2
UNION ALL SELECT 'v1<>v2', count(*) FROM b_colcmp WHERE v1<>v2;

-- ============================================================================
-- 第十四部分: 密态索引专项 (9种类型 hash+btree + EXPLAIN验证)
-- ============================================================================

-- int2/int4/int8/float4/float8/numeric: 300行数据,hash(det)+btree(ope),EXPLAIN
-- ===== 14.1 int2_enc =====
DROP TABLE IF EXISTS idx_i2;
CREATE TABLE idx_i2(id serial PRIMARY KEY, v_det int2_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope int2_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
INSERT INTO idx_i2(v_det,v_ope) VALUES (1,1),(2,2),(3,3),(4,4),(5,5),(6,6),(7,7),(8,8),(9,9),(10,10),(11,11),(12,12),(13,13),(14,14),(15,15),(16,16),(17,17),(18,18),(19,19),(20,20),(21,21),(22,22),(23,23),(24,24),(25,25),(26,26),(27,27),(28,28),(29,29),(30,30),(31,31),(32,32),(33,33),(34,34),(35,35),(36,36),(37,37),(38,38),(39,39),(40,40),(41,41),(42,42),(43,43),(44,44),(45,45),(46,46),(47,47),(48,48),(49,49),(50,50);
INSERT INTO idx_i2(v_det,v_ope) VALUES (51,51),(52,52),(53,53),(54,54),(55,55),(56,56),(57,57),(58,58),(59,59),(60,60),(61,61),(62,62),(63,63),(64,64),(65,65),(66,66),(67,67),(68,68),(69,69),(70,70),(71,71),(72,72),(73,73),(74,74),(75,75),(76,76),(77,77),(78,78),(79,79),(80,80),(81,81),(82,82),(83,83),(84,84),(85,85),(86,86),(87,87),(88,88),(89,89),(90,90),(91,91),(92,92),(93,93),(94,94),(95,95),(96,96),(97,97),(98,98),(99,99),(100,100),(101,101),(102,102),(103,103),(104,104),(105,105),(106,106),(107,107),(108,108),(109,109),(110,110),(111,111),(112,112),(113,113),(114,114),(115,115),(116,116),(117,117),(118,118),(119,119),(120,120),(121,121),(122,122),(123,123),(124,124),(125,125),(126,126),(127,127),(128,128),(129,129),(130,130),(131,131),(132,132),(133,133),(134,134),(135,135),(136,136),(137,137),(138,138),(139,139),(140,140),(141,141),(142,142),(143,143),(144,144),(145,145),(146,146),(147,147),(148,148),(149,149),(150,150),(151,151),(152,152),(153,153),(154,154),(155,155),(156,156),(157,157),(158,158),(159,159),(160,160),(161,161),(162,162),(163,163),(164,164),(165,165),(166,166),(167,167),(168,168),(169,169),(170,170),(171,171),(172,172),(173,173),(174,174),(175,175),(176,176),(177,177),(178,178),(179,179),(180,180),(181,181),(182,182),(183,183),(184,184),(185,185),(186,186),(187,187),(188,188),(189,189),(190,190),(191,191),(192,192),(193,193),(194,194),(195,195),(196,196),(197,197),(198,198),(199,199),(200,200);
INSERT INTO idx_i2(v_det,v_ope) VALUES (201,201),(202,202),(203,203),(204,204),(205,205),(206,206),(207,207),(208,208),(209,209),(210,210),(211,211),(212,212),(213,213),(214,214),(215,215),(216,216),(217,217),(218,218),(219,219),(220,220),(221,221),(222,222),(223,223),(224,224),(225,225),(226,226),(227,227),(228,228),(229,229),(230,230),(231,231),(232,232),(233,233),(234,234),(235,235),(236,236),(237,237),(238,238),(239,239),(240,240),(241,241),(242,242),(243,243),(244,244),(245,245),(246,246),(247,247),(248,248),(249,249),(250,250),(251,251),(252,252),(253,253),(254,254),(255,255),(256,256),(257,257),(258,258),(259,259),(260,260),(261,261),(262,262),(263,263),(264,264),(265,265),(266,266),(267,267),(268,268),(269,269),(270,270),(271,271),(272,272),(273,273),(274,274),(275,275),(276,276),(277,277),(278,278),(279,279),(280,280),(281,281),(282,282),(283,283),(284,284),(285,285),(286,286),(287,287),(288,288),(289,289),(290,290),(291,291),(292,292),(293,293),(294,294),(295,295),(296,296),(297,297),(298,298),(299,299),(300,300);
CREATE INDEX idx_i2_hash ON idx_i2 USING hash(v_det);
CREATE INDEX idx_i2_btree ON idx_i2(v_ope);
SET enable_seqscan = off;
SET enable_seqscan = on;
SELECT 'i2 det=200' t, count(*) FROM idx_i2 WHERE v_det = 200;
SELECT 'i2 ope>280' t, count(*) FROM idx_i2 WHERE v_ope > 280;

-- ===== 14.2 int4_enc =====
DROP TABLE IF EXISTS idx_i4;
CREATE TABLE idx_i4(id serial PRIMARY KEY, v_det int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
INSERT INTO idx_i4(v_det,v_ope) VALUES (1,1),(2,2),(3,3),(4,4),(5,5),(6,6),(7,7),(8,8),(9,9),(10,10),(11,11),(12,12),(13,13),(14,14),(15,15),(16,16),(17,17),(18,18),(19,19),(20,20),(21,21),(22,22),(23,23),(24,24),(25,25),(26,26),(27,27),(28,28),(29,29),(30,30),(31,31),(32,32),(33,33),(34,34),(35,35),(36,36),(37,37),(38,38),(39,39),(40,40),(41,41),(42,42),(43,43),(44,44),(45,45),(46,46),(47,47),(48,48),(49,49),(50,50);
INSERT INTO idx_i4(v_det,v_ope) VALUES (51,51),(52,52),(53,53),(54,54),(55,55),(56,56),(57,57),(58,58),(59,59),(60,60),(61,61),(62,62),(63,63),(64,64),(65,65),(66,66),(67,67),(68,68),(69,69),(70,70),(71,71),(72,72),(73,73),(74,74),(75,75),(76,76),(77,77),(78,78),(79,79),(80,80),(81,81),(82,82),(83,83),(84,84),(85,85),(86,86),(87,87),(88,88),(89,89),(90,90),(91,91),(92,92),(93,93),(94,94),(95,95),(96,96),(97,97),(98,98),(99,99),(100,100),(101,101),(102,102),(103,103),(104,104),(105,105),(106,106),(107,107),(108,108),(109,109),(110,110),(111,111),(112,112),(113,113),(114,114),(115,115),(116,116),(117,117),(118,118),(119,119),(120,120),(121,121),(122,122),(123,123),(124,124),(125,125),(126,126),(127,127),(128,128),(129,129),(130,130),(131,131),(132,132),(133,133),(134,134),(135,135),(136,136),(137,137),(138,138),(139,139),(140,140),(141,141),(142,142),(143,143),(144,144),(145,145),(146,146),(147,147),(148,148),(149,149),(150,150),(151,151),(152,152),(153,153),(154,154),(155,155),(156,156),(157,157),(158,158),(159,159),(160,160),(161,161),(162,162),(163,163),(164,164),(165,165),(166,166),(167,167),(168,168),(169,169),(170,170),(171,171),(172,172),(173,173),(174,174),(175,175),(176,176),(177,177),(178,178),(179,179),(180,180),(181,181),(182,182),(183,183),(184,184),(185,185),(186,186),(187,187),(188,188),(189,189),(190,190),(191,191),(192,192),(193,193),(194,194),(195,195),(196,196),(197,197),(198,198),(199,199),(200,200);
INSERT INTO idx_i4(v_det,v_ope) VALUES (201,201),(202,202),(203,203),(204,204),(205,205),(206,206),(207,207),(208,208),(209,209),(210,210),(211,211),(212,212),(213,213),(214,214),(215,215),(216,216),(217,217),(218,218),(219,219),(220,220),(221,221),(222,222),(223,223),(224,224),(225,225),(226,226),(227,227),(228,228),(229,229),(230,230),(231,231),(232,232),(233,233),(234,234),(235,235),(236,236),(237,237),(238,238),(239,239),(240,240),(241,241),(242,242),(243,243),(244,244),(245,245),(246,246),(247,247),(248,248),(249,249),(250,250),(251,251),(252,252),(253,253),(254,254),(255,255),(256,256),(257,257),(258,258),(259,259),(260,260),(261,261),(262,262),(263,263),(264,264),(265,265),(266,266),(267,267),(268,268),(269,269),(270,270),(271,271),(272,272),(273,273),(274,274),(275,275),(276,276),(277,277),(278,278),(279,279),(280,280),(281,281),(282,282),(283,283),(284,284),(285,285),(286,286),(287,287),(288,288),(289,289),(290,290),(291,291),(292,292),(293,293),(294,294),(295,295),(296,296),(297,297),(298,298),(299,299),(300,300);
CREATE INDEX idx_i4_hash ON idx_i4 USING hash(v_det);
CREATE INDEX idx_i4_btree ON idx_i4(v_ope);
SET enable_seqscan = off;
SET enable_seqscan = on;
SELECT 'i4 det=200' t, count(*) FROM idx_i4 WHERE v_det = 200;
SELECT 'i4 ope>280' t, count(*) FROM idx_i4 WHERE v_ope > 280;

-- ===== 14.3 int8_enc =====
DROP TABLE IF EXISTS idx_i8;
CREATE TABLE idx_i8(id serial PRIMARY KEY, v_det int8_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope int8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
INSERT INTO idx_i8(v_det,v_ope) VALUES (1,1),(2,2),(3,3),(4,4),(5,5),(6,6),(7,7),(8,8),(9,9),(10,10),(21,21),(22,22),(23,23),(24,24),(25,25),(26,26),(27,27),(28,28),(29,29),(30,30),(41,41),(42,42),(43,43),(44,44),(45,45),(46,46),(47,47),(48,48),(49,49),(50,50);
INSERT INTO idx_i8(v_det,v_ope) VALUES (51,51),(52,52),(53,53),(54,54),(55,55),(56,56),(57,57),(58,58),(59,59),(60,60),(61,61),(62,62),(63,63),(64,64),(65,65),(66,66),(67,67),(68,68),(69,69),(70,70),(71,71),(72,72),(73,73),(74,74),(75,75),(76,76),(77,77),(78,78),(79,79),(80,80),(81,81),(82,82),(83,83),(84,84),(85,85),(86,86),(87,87),(88,88),(89,89),(90,90),(91,91),(92,92),(93,93),(94,94),(95,95),(96,96),(97,97),(98,98),(99,99),(100,100),(101,101),(102,102),(103,103),(104,104),(105,105),(106,106),(107,107),(108,108),(109,109),(110,110),(111,111),(112,112),(113,113),(114,114),(115,115),(116,116),(117,117),(118,118),(119,119),(120,120),(121,121),(122,122),(123,123),(124,124),(125,125),(126,126),(127,127),(128,128),(129,129),(130,130),(131,131),(132,132),(133,133),(134,134),(135,135),(136,136),(137,137),(138,138),(139,139),(140,140),(141,141),(142,142),(143,143),(144,144),(145,145),(146,146),(147,147),(148,148),(149,149),(150,150),(151,151),(152,152),(153,153),(154,154),(155,155),(156,156),(157,157),(158,158),(159,159),(160,160),(161,161),(162,162),(163,163),(164,164),(165,165),(166,166),(167,167),(168,168),(169,169),(170,170),(171,171),(172,172),(173,173),(174,174),(175,175),(176,176),(177,177),(178,178),(179,179),(180,180),(181,181),(182,182),(183,183),(184,184),(185,185),(186,186),(187,187),(188,188),(189,189),(190,190),(191,191),(192,192),(193,193),(194,194),(195,195),(196,196),(197,197),(198,198),(199,199),(200,200);
CREATE INDEX idx_i8_hash ON idx_i8 USING hash(v_det);
CREATE INDEX idx_i8_btree ON idx_i8(v_ope);
SET enable_seqscan = off;
SET enable_seqscan = on;

-- ===== 14.4 float4_enc =====
DROP TABLE IF EXISTS idx_f4;
CREATE TABLE idx_f4(id serial PRIMARY KEY, v_det float4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope float4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
INSERT INTO idx_f4(v_det,v_ope) VALUES (1.0,1.0),(2.0,2.0),(3.0,3.0),(4.0,4.0),(5.0,5.0),(6.0,6.0),(7.0,7.0),(8.0,8.0),(9.0,9.0),(10.0,10.0),(11.0,11.0),(12.0,12.0),(13.0,13.0),(14.0,14.0),(15.0,15.0),(16.0,16.0),(17.0,17.0),(18.0,18.0),(19.0,19.0),(20.0,20.0),(21.0,21.0),(22.0,22.0),(23.0,23.0),(24.0,24.0),(25.0,25.0),(26.0,26.0),(27.0,27.0),(28.0,28.0),(29.0,29.0),(30.0,30.0),(31.0,31.0),(32.0,32.0),(33.0,33.0),(34.0,34.0),(35.0,35.0),(36.0,36.0),(37.0,37.0),(38.0,38.0),(39.0,39.0),(40.0,40.0),(41.0,41.0),(42.0,42.0),(43.0,43.0),(44.0,44.0),(45.0,45.0),(46.0,46.0),(47.0,47.0),(48.0,48.0),(49.0,49.0),(50.0,50.0);
INSERT INTO idx_f4(v_det,v_ope) VALUES (51.0,51.0),(52.0,52.0),(53.0,53.0),(54.0,54.0),(55.0,55.0),(56.0,56.0),(57.0,57.0),(58.0,58.0),(59.0,59.0),(60.0,60.0),(61.0,61.0),(62.0,62.0),(63.0,63.0),(64.0,64.0),(65.0,65.0),(66.0,66.0),(67.0,67.0),(68.0,68.0),(69.0,69.0),(70.0,70.0),(71.0,71.0),(72.0,72.0),(73.0,73.0),(74.0,74.0),(75.0,75.0),(76.0,76.0),(77.0,77.0),(78.0,78.0),(79.0,79.0),(80.0,80.0),(81.0,81.0),(82.0,82.0),(83.0,83.0),(84.0,84.0),(85.0,85.0),(86.0,86.0),(87.0,87.0),(88.0,88.0),(89.0,89.0),(90.0,90.0),(91.0,91.0),(92.0,92.0),(93.0,93.0),(94.0,94.0),(95.0,95.0),(96.0,96.0),(97.0,97.0),(98.0,98.0),(99.0,99.0),(100.0,100.0),(101.0,101.0),(102.0,102.0),(103.0,103.0),(104.0,104.0),(105.0,105.0),(106.0,106.0),(107.0,107.0),(108.0,108.0),(109.0,109.0),(110.0,110.0),(111.0,111.0),(112.0,112.0),(113.0,113.0),(114.0,114.0),(115.0,115.0),(116.0,116.0),(117.0,117.0),(118.0,118.0),(119.0,119.0),(120.0,120.0),(121.0,121.0),(122.0,122.0),(123.0,123.0),(124.0,124.0),(125.0,125.0),(126.0,126.0),(127.0,127.0),(128.0,128.0),(129.0,129.0),(130.0,130.0),(131.0,131.0),(132.0,132.0),(133.0,133.0),(134.0,134.0),(135.0,135.0),(136.0,136.0),(137.0,137.0),(138.0,138.0),(139.0,139.0),(140.0,140.0),(141.0,141.0),(142.0,142.0),(143.0,143.0),(144.0,144.0),(145.0,145.0),(146.0,146.0),(147.0,147.0),(148.0,148.0),(149.0,149.0),(150.0,150.0),(151.0,151.0),(152.0,152.0),(153.0,153.0),(154.0,154.0),(155.0,155.0),(156.0,156.0),(157.0,157.0),(158.0,158.0),(159.0,159.0),(160.0,160.0),(161.0,161.0),(162.0,162.0),(163.0,163.0),(164.0,164.0),(165.0,165.0),(166.0,166.0),(167.0,167.0),(168.0,168.0),(169.0,169.0),(170.0,170.0),(171.0,171.0),(172.0,172.0),(173.0,173.0),(174.0,174.0),(175.0,175.0),(176.0,176.0),(177.0,177.0),(178.0,178.0),(179.0,179.0),(180.0,180.0),(181.0,181.0),(182.0,182.0),(183.0,183.0),(184.0,184.0),(185.0,185.0),(186.0,186.0),(187.0,187.0),(188.0,188.0),(189.0,189.0),(190.0,190.0),(191.0,191.0),(192.0,192.0),(193.0,193.0),(194.0,194.0),(195.0,195.0),(196.0,196.0),(197.0,197.0),(198.0,198.0),(199.0,199.0),(200.0,200.0);
CREATE INDEX idx_f4_hash ON idx_f4 USING hash(v_det);
CREATE INDEX idx_f4_btree ON idx_f4(v_ope);
SET enable_seqscan = off;
SET enable_seqscan = on;

-- ===== 14.5 float8_enc =====
DROP TABLE IF EXISTS idx_f8;
CREATE TABLE idx_f8(id serial PRIMARY KEY, v_det float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
INSERT INTO idx_f8(v_det,v_ope) VALUES (1.0,1.0),(2.0,2.0),(3.0,3.0),(4.0,4.0),(5.0,5.0),(6.0,6.0),(7.0,7.0),(8.0,8.0),(9.0,9.0),(10.0,10.0),(11.0,11.0),(12.0,12.0),(13.0,13.0),(14.0,14.0),(15.0,15.0),(16.0,16.0),(17.0,17.0),(18.0,18.0),(19.0,19.0),(20.0,20.0),(21.0,21.0),(22.0,22.0),(23.0,23.0),(24.0,24.0),(25.0,25.0),(26.0,26.0),(27.0,27.0),(28.0,28.0),(29.0,29.0),(30.0,30.0),(31.0,31.0),(32.0,32.0),(33.0,33.0),(34.0,34.0),(35.0,35.0),(36.0,36.0),(37.0,37.0),(38.0,38.0),(39.0,39.0),(40.0,40.0),(41.0,41.0),(42.0,42.0),(43.0,43.0),(44.0,44.0),(45.0,45.0),(46.0,46.0),(47.0,47.0),(48.0,48.0),(49.0,49.0),(50.0,50.0);
INSERT INTO idx_f8(v_det,v_ope) VALUES (51.0,51.0),(52.0,52.0),(53.0,53.0),(54.0,54.0),(55.0,55.0),(56.0,56.0),(57.0,57.0),(58.0,58.0),(59.0,59.0),(60.0,60.0),(61.0,61.0),(62.0,62.0),(63.0,63.0),(64.0,64.0),(65.0,65.0),(66.0,66.0),(67.0,67.0),(68.0,68.0),(69.0,69.0),(70.0,70.0),(71.0,71.0),(72.0,72.0),(73.0,73.0),(74.0,74.0),(75.0,75.0),(76.0,76.0),(77.0,77.0),(78.0,78.0),(79.0,79.0),(80.0,80.0),(81.0,81.0),(82.0,82.0),(83.0,83.0),(84.0,84.0),(85.0,85.0),(86.0,86.0),(87.0,87.0),(88.0,88.0),(89.0,89.0),(90.0,90.0),(91.0,91.0),(92.0,92.0),(93.0,93.0),(94.0,94.0),(95.0,95.0),(96.0,96.0),(97.0,97.0),(98.0,98.0),(99.0,99.0),(100.0,100.0),(101.0,101.0),(102.0,102.0),(103.0,103.0),(104.0,104.0),(105.0,105.0),(106.0,106.0),(107.0,107.0),(108.0,108.0),(109.0,109.0),(110.0,110.0),(111.0,111.0),(112.0,112.0),(113.0,113.0),(114.0,114.0),(115.0,115.0),(116.0,116.0),(117.0,117.0),(118.0,118.0),(119.0,119.0),(120.0,120.0),(121.0,121.0),(122.0,122.0),(123.0,123.0),(124.0,124.0),(125.0,125.0),(126.0,126.0),(127.0,127.0),(128.0,128.0),(129.0,129.0),(130.0,130.0),(131.0,131.0),(132.0,132.0),(133.0,133.0),(134.0,134.0),(135.0,135.0),(136.0,136.0),(137.0,137.0),(138.0,138.0),(139.0,139.0),(140.0,140.0),(141.0,141.0),(142.0,142.0),(143.0,143.0),(144.0,144.0),(145.0,145.0),(146.0,146.0),(147.0,147.0),(148.0,148.0),(149.0,149.0),(150.0,150.0),(151.0,151.0),(152.0,152.0),(153.0,153.0),(154.0,154.0),(155.0,155.0),(156.0,156.0),(157.0,157.0),(158.0,158.0),(159.0,159.0),(160.0,160.0),(161.0,161.0),(162.0,162.0),(163.0,163.0),(164.0,164.0),(165.0,165.0),(166.0,166.0),(167.0,167.0),(168.0,168.0),(169.0,169.0),(170.0,170.0),(171.0,171.0),(172.0,172.0),(173.0,173.0),(174.0,174.0),(175.0,175.0),(176.0,176.0),(177.0,177.0),(178.0,178.0),(179.0,179.0),(180.0,180.0),(181.0,181.0),(182.0,182.0),(183.0,183.0),(184.0,184.0),(185.0,185.0),(186.0,186.0),(187.0,187.0),(188.0,188.0),(189.0,189.0),(190.0,190.0),(191.0,191.0),(192.0,192.0),(193.0,193.0),(194.0,194.0),(195.0,195.0),(196.0,196.0),(197.0,197.0),(198.0,198.0),(199.0,199.0),(200.0,200.0);
CREATE INDEX idx_f8_hash ON idx_f8 USING hash(v_det);
CREATE INDEX idx_f8_btree ON idx_f8(v_ope);
SET enable_seqscan = off;
SET enable_seqscan = on;

-- ===== 14.6 numeric_enc =====
DROP TABLE IF EXISTS idx_nm;
CREATE TABLE idx_nm(id serial PRIMARY KEY, v_det numeric_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope numeric_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
INSERT INTO idx_nm(v_det,v_ope) VALUES (1,1),(2,2),(3,3),(4,4),(5,5),(6,6),(7,7),(8,8),(9,9),(10,10),(11,11),(12,12),(13,13),(14,14),(15,15),(16,16),(17,17),(18,18),(19,19),(20,20),(21,21),(22,22),(23,23),(24,24),(25,25),(26,26),(27,27),(28,28),(29,29),(30,30),(31,31),(32,32),(33,33),(34,34),(35,35),(36,36),(37,37),(38,38),(39,39),(40,40),(41,41),(42,42),(43,43),(44,44),(45,45),(46,46),(47,47),(48,48),(49,49),(50,50);
INSERT INTO idx_nm(v_det,v_ope) VALUES (51,51),(52,52),(53,53),(54,54),(55,55),(56,56),(57,57),(58,58),(59,59),(60,60),(61,61),(62,62),(63,63),(64,64),(65,65),(66,66),(67,67),(68,68),(69,69),(70,70),(71,71),(72,72),(73,73),(74,74),(75,75),(76,76),(77,77),(78,78),(79,79),(80,80),(81,81),(82,82),(83,83),(84,84),(85,85),(86,86),(87,87),(88,88),(89,89),(90,90),(91,91),(92,92),(93,93),(94,94),(95,95),(96,96),(97,97),(98,98),(99,99),(100,100),(101,101),(102,102),(103,103),(104,104),(105,105),(106,106),(107,107),(108,108),(109,109),(110,110),(111,111),(112,112),(113,113),(114,114),(115,115),(116,116),(117,117),(118,118),(119,119),(120,120),(121,121),(122,122),(123,123),(124,124),(125,125),(126,126),(127,127),(128,128),(129,129),(130,130),(131,131),(132,132),(133,133),(134,134),(135,135),(136,136),(137,137),(138,138),(139,139),(140,140),(141,141),(142,142),(143,143),(144,144),(145,145),(146,146),(147,147),(148,148),(149,149),(150,150),(151,151),(152,152),(153,153),(154,154),(155,155),(156,156),(157,157),(158,158),(159,159),(160,160),(161,161),(162,162),(163,163),(164,164),(165,165),(166,166),(167,167),(168,168),(169,169),(170,170),(171,171),(172,172),(173,173),(174,174),(175,175),(176,176),(177,177),(178,178),(179,179),(180,180),(181,181),(182,182),(183,183),(184,184),(185,185),(186,186),(187,187),(188,188),(189,189),(190,190),(191,191),(192,192),(193,193),(194,194),(195,195),(196,196),(197,197),(198,198),(199,199),(200,200);
CREATE INDEX idx_nm_hash ON idx_nm USING hash(v_det);
CREATE INDEX idx_nm_btree ON idx_nm(v_ope);
SET enable_seqscan = off;
SET enable_seqscan = on;

-- ===== 14.7 text_enc =====
DROP TABLE IF EXISTS idx_tx;
CREATE TABLE idx_tx(id serial PRIMARY KEY, v_det text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope text_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
INSERT INTO idx_tx(v_det,v_ope) VALUES ('a','a'),('b','b'),('c','c'),('d','d'),('e','e'),('f','f'),('g','g'),('h','h'),('i','i'),('j','j'),('k','k'),('l','l'),('m','m'),('n','n'),('o','o'),('p','p'),('q','q'),('r','r'),('s','s'),('t','t'),('u','u'),('v','v'),('w','w'),('x','x'),('y','y'),('z','z'),('aa','aa'),('bb','bb'),('cc','cc'),('dd','dd'),('ee','ee'),('ff','ff'),('gg','gg'),('hh','hh'),('ii','ii'),('jj','jj'),('kk','kk'),('ll','ll'),('mm','mm'),('nn','nn'),('oo','oo'),('pp','pp'),('qq','qq'),('rr','rr'),('ss','ss'),('tt','tt'),('uu','uu'),('vv','vv'),('ww','ww'),('xx','xx');
INSERT INTO idx_tx(v_det,v_ope) VALUES ('yy','yy'),('zz','zz'),('aaa','aaa'),('bbb','bbb'),('ccc','ccc'),('ddd','ddd'),('eee','eee'),('fff','fff'),('ggg','ggg'),('hhh','hhh'),('iii','iii'),('jjj','jjj'),('kkk','kkk'),('lll','lll'),('mmm','mmm'),('nnn','nnn'),('ooo','ooo'),('ppp','ppp'),('qqq','qqq'),('rrr','rrr'),('sss','sss'),('ttt','ttt'),('uuu','uuu'),('vvv','vvv'),('www','www'),('xxx','xxx'),('yyy','yyy'),('zzz','zzz');
CREATE INDEX idx_tx_hash ON idx_tx USING hash(v_det);
CREATE INDEX idx_tx_btree ON idx_tx(v_ope);

-- ===== 14.8 varchar_enc =====
DROP TABLE IF EXISTS idx_vc;
CREATE TABLE idx_vc(id serial PRIMARY KEY, v_det varchar_enc(50) ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope varchar_enc(50) ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
INSERT INTO idx_vc(v_det,v_ope) VALUES ('a','a'),('b','b'),('c','c'),('d','d'),('e','e'),('f','f'),('g','g'),('h','h'),('i','i'),('j','j'),('k','k'),('l','l'),('m','m'),('n','n'),('o','o'),('p','p'),('q','q'),('r','r'),('s','s'),('t','t'),('u','u'),('v','v'),('w','w'),('x','x'),('y','y'),('z','z'),('aa','aa'),('bb','bb'),('cc','cc'),('dd','dd'),('ee','ee'),('ff','ff'),('gg','gg'),('hh','hh'),('ii','ii'),('jj','jj'),('kk','kk'),('ll','ll'),('mm','mm'),('nn','nn'),('oo','oo'),('pp','pp'),('qq','qq'),('rr','rr'),('ss','ss'),('tt','tt');
CREATE INDEX idx_vc_hash ON idx_vc USING hash(v_det);
CREATE INDEX idx_vc_btree ON idx_vc(v_ope);

-- ===== 14.9 bpchar_enc =====
DROP TABLE IF EXISTS idx_bp;
CREATE TABLE idx_bp(id serial PRIMARY KEY, v_det bpchar_enc(20) ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope bpchar_enc(20) ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
INSERT INTO idx_bp(v_det,v_ope) VALUES ('a','a'),('b','b'),('c','c'),('d','d'),('e','e'),('f','f'),('g','g'),('h','h'),('i','i'),('j','j'),('k','k'),('l','l'),('m','m'),('n','n'),('o','o'),('p','p'),('q','q'),('r','r'),('s','s'),('t','t'),('u','u'),('v','v'),('w','w'),('x','x'),('y','y'),('z','z'),('aa','aa'),('bb','bb'),('cc','cc'),('dd','dd'),('ee','ee'),('ff','ff'),('gg','gg'),('hh','hh'),('ii','ii'),('jj','jj'),('kk','kk'),('ll','ll'),('mm','mm'),('nn','nn'),('oo','oo'),('pp','pp'),('qq','qq'),('rr','rr'),('ss','ss'),('tt','tt');
CREATE INDEX idx_bp_hash ON idx_bp USING hash(v_det);
CREATE INDEX idx_bp_btree ON idx_bp(v_ope);

-- ============================================================================
-- 第十五部分: 语法/Boundary/多列/NOTLIKE/子查询补充
-- ============================================================================

-- ===== 15.1 语法覆盖: IN/NOT IN/DISTINCT/LIMIT+OFFSET/NULLS =====
DROP TABLE IF EXISTS ext_a;
CREATE TABLE ext_a(id serial, v_det int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'), v_tx text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'));
INSERT INTO ext_a(v_det,v_ope,v_tx) VALUES (1,1,'a'),(2,2,'b'),(3,3,'c'),(4,4,'a'),(5,5,'b'),(NULL,NULL,NULL);


-- ===== 15.2 边界深化 =====
DROP TABLE IF EXISTS ext_b;
CREATE TABLE ext_b(id serial, v2_d int2_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v2_o int2_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'), v4_d int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v4_o int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'), vx text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'));
INSERT INTO ext_b(v2_d,v2_o,v4_d,v4_o,vx) VALUES (32766,32766,2147483647,2147483647,'a'),(-32768,-32768,-2147483648,-2147483648,'bb'),(32766,32766,2147483646,2147483646,'测试'),(-32767,-32767,-2147483647,-2147483647,''),(1,1,1,1,' '),(-1,-1,-1,-1,'x'),(100,100,100,100,'test%_s'),(100,100,100,100,'test%_s'),(NULL,NULL,NULL,NULL,NULL);


-- ===== 15.3 多列组合 =====

-- ===== 15.4 NOT LIKE 扩展 =====
DROP TABLE IF EXISTS ext_d;
CREATE TABLE ext_d(id serial, v text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'));
INSERT INTO ext_d(v) VALUES ('张三'),('张三丰'),('丰张三'),('张三李四'),('李四'),('abc'),('abcdxyz'),('xyzabcd'),('ab'),('ab%cd');


-- ===== 15.5 子查询 =====

-- ============================================================================
-- 第十六部分: 反向/异常用例（不支持的操作应正常报错不崩溃）
-- ============================================================================

DROP TABLE IF EXISTS neg_test;
CREATE TABLE neg_test(id serial PRIMARY KEY, v_text text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), v_det int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'), v_rnd text_enc ENCRYPTED WITH (dek=test_dek));
INSERT INTO neg_test(v_text,v_det,v_ope,v_rnd) VALUES ('abc',100,100,'xxx'),('def',200,200,'yyy');

SELECT * FROM neg_test WHERE v_det > 100;

SELECT * FROM neg_test WHERE v_rnd = 'xxx';

SELECT * FROM neg_test WHERE v_rnd LIKE '%x%';

SELECT * FROM neg_test WHERE v_det LIKE '%1%';

SELECT v_text::int4_enc FROM neg_test;

SELECT v_det::text_enc FROM neg_test;

SELECT * FROM neg_test WHERE v_det = v_ope;

SELECT * FROM neg_test WHERE v_det = '100'::text;

SELECT 'session_alive' t, count(*) FROM neg_test;

-- ============================================================================
-- 第十七部分: 密态分区表（9种密态类型作为分区键）
-- ============================================================================
-- TECC限制: PARTITION OF/LIKE不继承ENCRYPTED WITH, 需显式定义每个分区再ATTACH
-- 字符类型(ope)做LIST分区键失败, 6种数值类型(ope)做RANGE分区键通过

-- ===== 14.1 text_enc(ope) LIST分区 =====
DROP TABLE IF EXISTS pt_text, pt_text_a, pt_text_b CASCADE;
CREATE TABLE pt_text (id int NOT NULL, name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'), val int) PARTITION BY LIST (name);
CREATE TABLE pt_text_a (id int NOT NULL, name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'), val int);
ALTER TABLE pt_text ATTACH PARTITION pt_text_a FOR VALUES IN ('技术部','管理部');
CREATE TABLE pt_text_b (id int NOT NULL, name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'), val int);
ALTER TABLE pt_text ATTACH PARTITION pt_text_b FOR VALUES IN ('市场部','销售部');
INSERT INTO pt_text VALUES (1,'技术部',100),(2,'市场部',200),(3,'管理部',300),(4,'技术部',400);
SELECT 'text(ope)LIST' t, count(*) FROM pt_text;

-- ===== 14.2 varchar_enc(ope) LIST分区 =====
DROP TABLE IF EXISTS pt_vc, pt_vc_1, pt_vc_2 CASCADE;
CREATE TABLE pt_vc (id int NOT NULL, dept varchar_enc(50) ENCRYPTED WITH (dek=test_dek,support_algo='ope'), budget int) PARTITION BY LIST (dept);
CREATE TABLE pt_vc_1 (id int NOT NULL, dept varchar_enc(50) ENCRYPTED WITH (dek=test_dek,support_algo='ope'), budget int);
ALTER TABLE pt_vc ATTACH PARTITION pt_vc_1 FOR VALUES IN ('技术部','产品部');
CREATE TABLE pt_vc_2 (id int NOT NULL, dept varchar_enc(50) ENCRYPTED WITH (dek=test_dek,support_algo='ope'), budget int);
ALTER TABLE pt_vc ATTACH PARTITION pt_vc_2 FOR VALUES IN ('行政部');
INSERT INTO pt_vc VALUES (1,'技术部',500),(2,'产品部',300),(3,'行政部',100);
SELECT 'varchar(ope)LIST' t, count(*) FROM pt_vc;

-- ===== 14.3 bpchar_enc(ope) LIST分区 =====
DROP TABLE IF EXISTS pt_bp, pt_bp_a, pt_bp_b CASCADE;
CREATE TABLE pt_bp (id int NOT NULL, code bpchar_enc(10) ENCRYPTED WITH (dek=test_dek,support_algo='ope'), descr varchar(50)) PARTITION BY LIST (code);
CREATE TABLE pt_bp_a (id int NOT NULL, code bpchar_enc(10) ENCRYPTED WITH (dek=test_dek,support_algo='ope'), descr varchar(50));
ALTER TABLE pt_bp ATTACH PARTITION pt_bp_a FOR VALUES IN ('CN','US');
CREATE TABLE pt_bp_b (id int NOT NULL, code bpchar_enc(10) ENCRYPTED WITH (dek=test_dek,support_algo='ope'), descr varchar(50));
ALTER TABLE pt_bp ATTACH PARTITION pt_bp_b FOR VALUES IN ('JP','KR');
INSERT INTO pt_bp VALUES (1,'CN','中国'),(2,'US','美国'),(3,'JP','日本'),(4,'CN','中国备份');
SELECT 'bpchar(ope)LIST' t, count(*) FROM pt_bp;

-- ===== 14.4 int2_enc(ope) RANGE分区 + 明文对照 =====
DROP TABLE IF EXISTS pt_i2, pt_i2_young, pt_i2_old, plain_i2 CASCADE;
CREATE TABLE pt_i2 (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), age int2_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope')) PARTITION BY RANGE (age);
CREATE TABLE pt_i2_young (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), age int2_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
ALTER TABLE pt_i2 ATTACH PARTITION pt_i2_young FOR VALUES FROM (MINVALUE) TO (30);
CREATE TABLE pt_i2_old (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), age int2_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
ALTER TABLE pt_i2 ATTACH PARTITION pt_i2_old FOR VALUES FROM (30) TO (MAXVALUE);
INSERT INTO pt_i2 VALUES ('张三',25),('李四',35),('王五',28),('赵六',45);
-- 明文对照
CREATE TABLE plain_i2 (name text, age int2);
INSERT INTO plain_i2 VALUES ('张三',25),('李四',35),('王五',28),('赵六',45);

-- ===== 14.5 int4_enc(ope) RANGE分区 + 明文对照 =====
DROP TABLE IF EXISTS pt_i4, pt_i4_low, pt_i4_mid, pt_i4_high, plain_i4 CASCADE;
CREATE TABLE pt_i4 (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), salary int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope')) PARTITION BY RANGE (salary);
CREATE TABLE pt_i4_low (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), salary int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
ALTER TABLE pt_i4 ATTACH PARTITION pt_i4_low FOR VALUES FROM (MINVALUE) TO (10000);
CREATE TABLE pt_i4_mid (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), salary int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
ALTER TABLE pt_i4 ATTACH PARTITION pt_i4_mid FOR VALUES FROM (10000) TO (20000);
CREATE TABLE pt_i4_high (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), salary int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
ALTER TABLE pt_i4 ATTACH PARTITION pt_i4_high FOR VALUES FROM (20000) TO (MAXVALUE);
INSERT INTO pt_i4 VALUES ('张三',8000),('李四',15000),('王五',25000),('赵六',12000);
CREATE TABLE plain_i4 (name text, salary int4);
INSERT INTO plain_i4 VALUES ('张三',8000),('李四',15000),('王五',25000),('赵六',12000);

-- ===== 14.6 int8_enc(ope) RANGE分区 + 明文对照 =====
DROP TABLE IF EXISTS pt_i8, pt_i8_small, pt_i8_large, plain_i8 CASCADE;
CREATE TABLE pt_i8 (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), total int8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope')) PARTITION BY RANGE (total);
CREATE TABLE pt_i8_small (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), total int8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
ALTER TABLE pt_i8 ATTACH PARTITION pt_i8_small FOR VALUES FROM (MINVALUE) TO (50000);
CREATE TABLE pt_i8_large (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), total int8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
ALTER TABLE pt_i8 ATTACH PARTITION pt_i8_large FOR VALUES FROM (50000) TO (MAXVALUE);
INSERT INTO pt_i8 VALUES ('张三',30000),('李四',80000),('王五',10000),('赵六',50000);
CREATE TABLE plain_i8 (name text, total int8);
INSERT INTO plain_i8 VALUES ('张三',30000),('李四',80000),('王五',10000),('赵六',50000);

-- ===== 14.7 float4_enc(ope) RANGE分区 + 明文对照 =====
DROP TABLE IF EXISTS pt_f4, pt_f4_low, pt_f4_high, plain_f4 CASCADE;
CREATE TABLE pt_f4 (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), score float4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope')) PARTITION BY RANGE (score);
CREATE TABLE pt_f4_low (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), score float4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
ALTER TABLE pt_f4 ATTACH PARTITION pt_f4_low FOR VALUES FROM (MINVALUE) TO (60.0);
CREATE TABLE pt_f4_high (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), score float4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
ALTER TABLE pt_f4 ATTACH PARTITION pt_f4_high FOR VALUES FROM (60.0) TO (MAXVALUE);
INSERT INTO pt_f4 VALUES ('张三',55.5),('李四',85.0),('王五',59.9),('赵六',90.5);
CREATE TABLE plain_f4 (name text, score float4);
INSERT INTO plain_f4 VALUES ('张三',55.5),('李四',85.0),('王五',59.9),('赵六',90.5);

-- ===== 14.8 float8_enc(ope) RANGE分区 + 明文对照 =====
DROP TABLE IF EXISTS pt_f8, pt_f8_low, pt_f8_high, plain_f8 CASCADE;
CREATE TABLE pt_f8 (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), amount float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope')) PARTITION BY RANGE (amount);
CREATE TABLE pt_f8_low (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), amount float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
ALTER TABLE pt_f8 ATTACH PARTITION pt_f8_low FOR VALUES FROM (MINVALUE) TO (1000.0);
CREATE TABLE pt_f8_high (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), amount float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
ALTER TABLE pt_f8 ATTACH PARTITION pt_f8_high FOR VALUES FROM (1000.0) TO (MAXVALUE);
INSERT INTO pt_f8 VALUES ('张三',500.5),('李四',2000.0),('王五',999.9),('赵六',1500.0);
CREATE TABLE plain_f8 (name text, amount float8);
INSERT INTO plain_f8 VALUES ('张三',500.5),('李四',2000.0),('王五',999.9),('赵六',1500.0);

-- ===== 14.9 numeric_enc(ope) RANGE分区 + 明文对照 =====
DROP TABLE IF EXISTS pt_nm, pt_nm_low, pt_nm_mid, pt_nm_high, plain_nm CASCADE;
CREATE TABLE pt_nm (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), balance numeric_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope')) PARTITION BY RANGE (balance);
CREATE TABLE pt_nm_low (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), balance numeric_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
ALTER TABLE pt_nm ATTACH PARTITION pt_nm_low FOR VALUES FROM (MINVALUE) TO (50000);
CREATE TABLE pt_nm_mid (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), balance numeric_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
ALTER TABLE pt_nm ATTACH PARTITION pt_nm_mid FOR VALUES FROM (50000) TO (100000);
CREATE TABLE pt_nm_high (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), balance numeric_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
ALTER TABLE pt_nm ATTACH PARTITION pt_nm_high FOR VALUES FROM (100000) TO (MAXVALUE);
INSERT INTO pt_nm VALUES ('张三',30000),('李四',80000),('王五',150000),('赵六',50000);
CREATE TABLE plain_nm (name text, balance numeric);
INSERT INTO plain_nm VALUES ('张三',30000),('李四',80000),('王五',150000),('赵六',50000);

-- ============================================================================
-- 第十八部分: 密态类型转换（隐式转换 + 显式强转）
-- ============================================================================

-- 准备转换测试数据
DROP TABLE IF EXISTS cast_src;
CREATE TABLE cast_src(
    id serial PRIMARY KEY,
    c2 int2_enc    ENCRYPTED WITH (dek=test_dek, support_algo='det'),
    c4 int4_enc    ENCRYPTED WITH (dek=test_dek, support_algo='det'),
    c8 int8_enc    ENCRYPTED WITH (dek=test_dek, support_algo='ope'),
    f4 float4_enc  ENCRYPTED WITH (dek=test_dek, support_algo='det'),
    f8 float8_enc  ENCRYPTED WITH (dek=test_dek, support_algo='ope'),
    nm numeric_enc(10,2) ENCRYPTED WITH (dek=test_dek, support_algo='det'),
    tx text_enc    ENCRYPTED WITH (dek=test_dek, support_algo='det,search'),
    vc varchar_enc(100) ENCRYPTED WITH (dek=test_dek, support_algo='det,search'),
    bp bpchar_enc(20)   ENCRYPTED WITH (dek=test_dek, support_algo='det,search')
);
INSERT INTO cast_src(c2,c4,c8,f4,f8,nm,tx,vc,bp) VALUES
(100, 20000, 3000000, 3.14, 2.718, 99999.99, 'hello', 'world', 'test');

-- ===== 15.1 数字类型隐式转换 =====

SELECT c2::int4_enc FROM cast_src;
SELECT c2::int8_enc FROM cast_src;
SELECT c4::int8_enc FROM cast_src;
SELECT c2::float4_enc FROM cast_src;
SELECT c2::float8_enc FROM cast_src;
SELECT c4::float8_enc FROM cast_src;
SELECT c2::numeric_enc FROM cast_src;
SELECT c4::numeric_enc FROM cast_src;
SELECT c8::numeric_enc FROM cast_src;
SELECT f4::float8_enc FROM cast_src;
SELECT f4::numeric_enc FROM cast_src;
SELECT f8::numeric_enc FROM cast_src;

-- ===== 15.2 数字类型显式强转（CAST语法） =====

SELECT CAST(c2 AS int4_enc) FROM cast_src;
SELECT CAST(c2 AS int8_enc) FROM cast_src;
SELECT CAST(c2 AS float4_enc) FROM cast_src;
SELECT CAST(c2 AS float8_enc) FROM cast_src;
SELECT CAST(c2 AS numeric_enc) FROM cast_src;
SELECT CAST(f4 AS float8_enc) FROM cast_src;
SELECT CAST(f4 AS numeric_enc) FROM cast_src;
SELECT CAST(f8 AS numeric_enc) FROM cast_src;

-- ===== 15.3 字符类型隐式转换 =====

SELECT tx::varchar_enc FROM cast_src;
SELECT vc::text_enc FROM cast_src;
SELECT tx::bpchar_enc FROM cast_src;
SELECT vc::bpchar_enc FROM cast_src;
SELECT bp::varchar_enc FROM cast_src;
SELECT bp::text_enc FROM cast_src;

-- ===== 15.4 字符类型显式强转（CAST语法） =====

SELECT CAST(tx AS varchar_enc) FROM cast_src;
SELECT CAST(vc AS text_enc) FROM cast_src;
SELECT CAST(tx AS bpchar_enc) FROM cast_src;
SELECT CAST(bp AS varchar_enc) FROM cast_src;

-- ===== 15.5 非法转换（预期失败） =====

SELECT tx::int4_enc FROM cast_src;
SELECT CAST(tx AS int4_enc) FROM cast_src;

SELECT c4::text_enc FROM cast_src;

-- 示例: 如果存在另一个DEK...

-- ===== 15.6 转换后数据正确性验证 =====

-- 验证 int2(100) 转换为各类型后数值正确
SELECT CAST(CAST(CAST(CAST(c2 AS int4_enc) AS int8_enc) AS float8_enc) AS numeric_enc) FROM cast_src;

-- 验证字符互转后内容不变
SELECT CAST(CAST(tx AS varchar_enc) AS text_enc) FROM cast_src;

SELECT CAST(CAST(bp AS varchar_enc) AS bpchar_enc) FROM cast_src;

-- ============================================================================
-- 第十九部分: 混合表真实场景测试（密态列+明文列）
-- ============================================================================

-- ===== 14.1 员工表（最常见的混合场景） =====

DROP TABLE IF EXISTS employee;
CREATE TABLE employee (
    id          serial PRIMARY KEY,
    name        text_enc    ENCRYPTED WITH (dek=test_dek, support_algo='det,search'),
    id_card     text_enc    ENCRYPTED WITH (dek=test_dek, support_algo='det'),
    salary      numeric_enc ENCRYPTED WITH (dek=test_dek, support_algo='ope'),
    dept        varchar(50),
    hire_date   date,
    level       int
);

INSERT INTO employee(name,id_card,salary,dept,hire_date,level) VALUES
('张三',   '110101199001011234',  15000,  '技术部', '2020-03-15', 5),
('李四',   '110101199205052345',  12000,  '市场部', '2021-06-01', 4),
('王五',   '110101198812013456',  18000,  '技术部', '2019-01-10', 6),
('赵六',   '110101199308084567',  25000,  '管理部', '2018-05-20', 7),
('Alice',  '410101199510105678',  20000,  'Engineering', '2022-01-15', 5),
('Bob',    '410101199603206789',  16000,  'Sales',  '2021-09-01', 4),
('Charlie','410101199707307890',  22000,  'Engineering', '2020-07-10', 6),
('孙七',   NULL,                   9000,  '市场部', NULL,           2),
('周八',   '320101199804158901',  NULL,   '技术部', '2022-06-01', 3),
(NULL,     '320101199905209012',  11000,  '管理部', '2023-01-01', 3);

SELECT id, name, salary, dept, hire_date, level FROM employee ORDER BY id;

-- 密态列等值
SELECT id, name, dept FROM employee WHERE name='张三';

-- 明文列等值
SELECT id, name, dept, hire_date FROM employee WHERE dept='技术部' ORDER BY id;

-- 密态+明文复合条件
SELECT id, name, salary, dept FROM employee WHERE dept='技术部' AND salary>10000 ORDER BY id;

-- 密态列LIKE+明文条件
SELECT id, name, level FROM employee WHERE name LIKE '张%' AND level>=5;

-- 明文列ORDER BY
SELECT id, name, hire_date FROM employee WHERE hire_date IS NOT NULL ORDER BY hire_date;

-- 密态列ORDER BY (ope)
SELECT id, name, salary FROM employee WHERE salary IS NOT NULL ORDER BY salary DESC;

-- 密态列范围+明文IN
SELECT id, name, salary, dept FROM employee WHERE salary>15000 AND (dept='技术部' OR dept='管理部') ORDER BY salary DESC;

-- COUNT聚合(混合)
SELECT dept, count(*) cnt, avg(salary::numeric)::numeric(10,2) avg_sal FROM employee WHERE dept IS NOT NULL AND salary IS NOT NULL GROUP BY dept ORDER BY cnt DESC;

-- NULL处理(密态+明文混合NULL)
SELECT id, name, id_card, hire_date FROM employee WHERE name IS NULL OR hire_date IS NULL;

-- 更新明文列(密态列不变)
UPDATE employee SET level=level+1 WHERE dept='技术部';
SELECT id, name, dept, level FROM employee WHERE dept='技术部' ORDER BY id;

-- ===== 14.2 医疗记录表 =====

DROP TABLE IF EXISTS medical;
CREATE TABLE medical (
    id          serial PRIMARY KEY,
    patient     text_enc    ENCRYPTED WITH (dek=test_dek, support_algo='det,search'),
    diagnosis   text_enc    ENCRYPTED WITH (dek=test_dek, support_algo='det'),
    cost        float8_enc  ENCRYPTED WITH (dek=test_dek, support_algo='ope'),
    hospital    varchar(100),
    department  varchar(50),
    visit_date  date,
    is_emergency boolean
);

INSERT INTO medical(patient,diagnosis,cost,hospital,department,visit_date,is_emergency) VALUES
('张三',  '高血压',      1500.50, '市第一医院', '心内科', '2025-01-10', false),
('李四',  '急性阑尾炎',  8500.00, '市第一医院', '普外科', '2025-01-15', true),
('王五',  '糖尿病',      3200.00, '省中心医院', '内分泌科','2025-02-01', false),
('赵六',  '骨折',       12000.00, '市骨科医院', '骨科',   '2025-02-10', true),
('Alice', 'Hypertension',2500.00, 'General Hospital','Cardiology','2025-03-01',false),
('Bob',   'Appendicitis',9000.00, 'General Hospital','Surgery','2025-03-05',true),
('张三',  '常规体检',     800.00, '市第一医院', '体检中心','2025-03-15', false),
('孙七',  NULL,          1500.00, '省中心医院', '内科',   NULL,          false),
(NULL,    '肺炎',        4500.00, NULL,         '呼吸科', '2025-04-01',  true);

SELECT id, patient, diagnosis, cost, hospital, department, visit_date FROM medical ORDER BY id;

-- 密态LIKE搜索患者
SELECT id, patient, diagnosis, hospital FROM medical WHERE patient LIKE '张%' ORDER BY id;

-- 密态范围+明文布尔条件
SELECT id, patient, cost, department FROM medical WHERE cost>5000 AND is_emergency=true ORDER BY cost DESC;

-- 跨表JOIN(密态列在SELECT中)
SELECT e.name, e.dept, m.diagnosis, m.cost
FROM employee e JOIN medical m ON e.id=m.id
WHERE e.name IS NOT NULL AND m.diagnosis IS NOT NULL
ORDER BY e.id LIMIT 5;

-- 多条件: 密态+明文+密态
SELECT id, patient, diagnosis, cost FROM medical WHERE hospital='市第一医院' AND cost<5000 ORDER BY id;

-- 统计: 按医院统计急诊数量+平均费用
SELECT hospital, count(*) cnt, count(CASE WHEN is_emergency THEN 1 END) emergency_cnt, avg(cost)::numeric(10,2) avg_cost
FROM medical WHERE hospital IS NOT NULL AND cost IS NOT NULL
GROUP BY hospital ORDER BY cnt DESC;

-- ===== 14.3 金融账户表（密态:账号+余额+身份证, 明文:开户行+账户类型+状态） =====

DROP TABLE IF EXISTS account;
CREATE TABLE account (
    id          serial PRIMARY KEY,
    account_no  text_enc    ENCRYPTED WITH (dek=test_dek, support_algo='det,search'),
    id_card     text_enc    ENCRYPTED WITH (dek=test_dek, support_algo='det'),
    balance     numeric_enc(15,2) ENCRYPTED WITH (dek=test_dek, support_algo='ope'),
    bank_name   varchar(100),
    acct_type   varchar(20),
    status      varchar(10),
    open_date   date
);

INSERT INTO account(account_no,id_card,balance,bank_name,acct_type,status,open_date) VALUES
('6222021234567890','110101199001011234',  50000.00, '工商银行', '储蓄账户', 'active', '2020-03-20'),
('6228480987654321','110101199205052345', 150000.00, '农业银行', '理财账户', 'active', '2021-06-05'),
('6227003333222244','110101198812013456',   3500.00, '建设银行', '储蓄账户', 'active', '2019-01-15'),
('6217001111222233','110101199308084567', 850000.00, '工商银行', '储蓄账户', 'frozen','2018-05-25'),
('6226098888777766','410101199510105678',  25000.00, '招商银行', '储蓄账户', 'active', '2022-01-20'),
('6225889999000011',NULL,                   1000.00, '工商银行', '储蓄账户', 'active', '2023-06-01'),
(NULL,              '410101199707307890',  750000.00, '招商银行', '理财账户', 'active', '2020-08-10');

SELECT id, account_no, balance, bank_name, acct_type, status FROM account ORDER BY id;

-- 密态范围: 高余额
SELECT id, balance, bank_name FROM account WHERE balance>100000 ORDER BY balance DESC;

-- 明文+密态
SELECT id, balance, acct_type FROM account WHERE bank_name='工商银行' AND balance>10000 ORDER BY id;

-- 明文GROUP BY + 密态聚合
SELECT bank_name, status, count(*) cnt, sum(balance)::numeric(15,2) total_balance
FROM account WHERE bank_name IS NOT NULL AND balance IS NOT NULL
GROUP BY bank_name, status ORDER BY total_balance DESC;

-- 密态列NULL搜索
SELECT id, balance, bank_name FROM account WHERE account_no IS NULL;

-- 多密态列组合: account_no LIKE + balance范围
SELECT id, account_no, balance FROM account WHERE account_no LIKE '622202%' AND balance>10000;

-- 明文IN + 密态ORDER BY
SELECT id, balance, bank_name FROM account WHERE bank_name IN('工商银行','招商银行') AND balance IS NOT NULL ORDER BY balance DESC;

-- 子查询(密态聚合+密态比较): 密态列不能cast到明文类型，跳过

-- ============================================================================
-- 第二十部分: COPY / \copy 导入导出测试
-- ============================================================================

-- 准备导出数据表
DROP TABLE IF EXISTS cp_test;
CREATE TABLE cp_test(
    id   serial PRIMARY KEY,
    name text_enc ENCRYPTED WITH (dek=test_dek, support_algo='det,search'),
    age  int4_enc ENCRYPTED WITH (dek=test_dek, support_algo='det'),
    sal  float8_enc ENCRYPTED WITH (dek=test_dek, support_algo='ope')
);
INSERT INTO cp_test(name,age,sal) VALUES ('张三',25,5000),('李四',30,8000),('王五',28,6000);

-- TEST 14.1: \copy TO text格式 — 导出明文
-- 原 \copy 命令: COPY cp_test TO 'E:/tmp/tecc-test-keys/tecc_cp_text.txt'

-- TEST 14.2: \copy TO csv格式 — 导出密文(已知问题)
-- 原 \copy 命令: COPY cp_test TO 'E:/tmp/tecc-test-keys/tecc_cp_csv.txt' WITH (FORMAT csv, HEADER)

-- TEST 14.3: \copy FROM text明文文件导入 — 预期失败
DROP TABLE IF EXISTS cp_restore;
CREATE TABLE cp_restore(id serial PRIMARY KEY, name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), age int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), sal float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
-- text格式明文文件导入，预期报错 invalid hex string
-- 原 \copy 命令: COPY cp_restore FROM 'E:/tmp/tecc-test-keys/tecc_cp_text.txt'

-- TEST 14.4: \copy FROM csv密文文件导入 — 成功
TRUNCATE cp_restore;
-- 原 \copy 命令: COPY cp_restore FROM 'E:/tmp/tecc-test-keys/tecc_cp_csv.txt' WITH (FORMAT csv, HEADER)
SELECT 'restored' t, count(*) FROM cp_restore;
SELECT * FROM cp_restore ORDER BY id;

-- TEST 14.5: csv密文导入后与原表数据一致
SELECT 'original' t, id, name, age, sal FROM cp_test
UNION ALL SELECT 'restored', id, name, age, sal FROM cp_restore
ORDER BY id, t;

-- TEST 14.6: pg_dump 导出 — 导出密文(与需求不符)

-- TEST 14.7: pg_dump SQL格式 round-trip 恢复
DROP TABLE IF EXISTS cp_restore2;
CREATE TABLE cp_restore2(id serial PRIMARY KEY, name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), age int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), sal float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
-- pg_dump 使用原表名 cp_test, 清空原表再恢复
TRUNCATE cp_test;
SET search_path = public;
SELECT 'pgdump+\\i' t, count(*) FROM cp_test;
SELECT * FROM public.cp_test ORDER BY id;

-- TEST 14.9: pg_dump 仅schema

-- TEST 14.10: pg_dump --exclude-table

-- TEST 14.11: pg_dumpall 全实例导出

-- TEST 14.12: KEK文件手工备份
SELECT 'kek_backup' t, count(*) FROM cp_test;

-- ===== 14.14 COPY恢复后全面查询验证 =====
DROP TABLE IF EXISTS r_src;
CREATE TABLE r_src(id serial, name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), age int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'), score float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'), dept varchar(50));
INSERT INTO r_src(name,age,score,dept) VALUES ('张三',25,85.5,'技术部'),('李四',30,92.0,'市场部'),('王五',28,78.5,'技术部'),(NULL,22,70.0,'市场部');

-- COPY导出密文
-- 原 \copy 命令: COPY r_src TO 'E:/tmp/tecc-test-keys/r_src.csv' WITH (FORMAT csv, HEADER)

-- COPY导入(tecc ON, 密文文件)
DROP TABLE IF EXISTS r_copy;
CREATE TABLE r_copy(id serial, name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), age int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'), score float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'), dept varchar(50));
-- 原 \copy 命令: COPY r_copy FROM 'E:/tmp/tecc-test-keys/r_src.csv' WITH (FORMAT csv, HEADER)

SELECT id, name, age, dept FROM r_copy ORDER BY id;
SELECT count(*) FROM r_copy WHERE name='张三';
SELECT count(*) FROM r_copy WHERE age>25;
SELECT count(*) FROM r_copy WHERE name LIKE '张%';
SELECT count(*) FROM r_copy WHERE name IS NULL;
SELECT count(*) FROM r_copy WHERE dept='技术部';

-- ===== 14.15 pg_dump恢复后全面查询验证 =====
-- 替换表名为r_dump

DROP TABLE IF EXISTS r_dump;
CREATE TABLE r_dump(id serial, name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), age int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'), score float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'), dept varchar(50));
SET search_path = public;

SELECT id, name, age, dept FROM r_dump ORDER BY id;
SELECT count(*) FROM r_dump WHERE name='张三';
SELECT count(*) FROM r_dump WHERE age>25;
SELECT count(*) FROM r_dump WHERE name LIKE '张%';
SELECT count(*) FROM r_dump WHERE name IS NULL;
SELECT count(*) FROM r_dump WHERE dept='技术部';


-- TEST 14.16: 跨库恢复(手动建表+pg_restore,需KEK/DEK存在)

-- ============================================================================
-- 第二十部分: COPY / \copy + dump/restore 汇总结论
-- ============================================================================
-- 重建目标表
DROP TABLE IF EXISTS cp_restore3;
CREATE TABLE cp_restore3(id serial PRIMARY KEY, name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), age int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), sal float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
TRUNCATE cp_test;
SELECT 'pgdump+pgrestore' t, count(*) FROM public.cp_test;
SELECT * FROM public.cp_test ORDER BY id;

-- ============================================================================
-- 第二十一部分: 深度边界场景 (INSERT强转/UPDATE/DELETE/视图/CTE/UNION等)
-- ============================================================================

-- ===== 22.1 INSERT 类型强转 =====
DROP TABLE IF EXISTS edge1;
CREATE TABLE edge1(id serial, v int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), t text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'));

INSERT INTO edge1(v,t) VALUES (100,'hello');
SELECT 'direct' t, count(*) FROM edge1;

INSERT INTO edge1(v,t) VALUES (1::int2, 'test');

INSERT INTO edge1(v,t) VALUES (1::int8, 'test');

INSERT INTO edge1(v,t) VALUES (1.5::float4, 'test');

INSERT INTO edge1(t) VALUES ('test'::varchar);

-- ===== 22.2 UPDATE + DELETE =====
DROP TABLE IF EXISTS edge2;
CREATE TABLE edge2(id serial, v int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), t text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'));
INSERT INTO edge2(v,t) VALUES (100,'hello'),(200,'world');

UPDATE edge2 SET v=300 WHERE id=1;
SELECT 'update_ok' t, count(*) FROM edge2 WHERE v=300;

DELETE FROM edge2 WHERE v=200;
SELECT 'delete_ok' t, count(*) FROM edge2 WHERE v=200;

-- ===== 22.3 INSERT SELECT =====
DROP TABLE IF EXISTS edge3_plain;
CREATE TABLE edge3_plain(v int);
INSERT INTO edge3_plain VALUES (100),(200);
DROP TABLE IF EXISTS edge3_enc;
CREATE TABLE edge3_enc(id serial, v int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'));
INSERT INTO edge3_enc(v) SELECT v FROM edge3_plain;

-- ===== 22.4 视图 =====
CREATE OR REPLACE VIEW edge_view AS SELECT id, v, t FROM edge2 WHERE v IS NOT NULL;

-- ===== 22.5 CTE =====
WITH t AS (SELECT id, v FROM edge2 WHERE v > 0) SELECT count(*) FROM t;

-- ===== 22.6 UNION =====
INSERT INTO edge2(v,t) VALUES (500,'u1'),(600,'u2');
SELECT count(*) FROM (SELECT v FROM edge2 WHERE v=500 UNION ALL SELECT v FROM edge2 WHERE v=600) t;

-- ===== 22.7 ALTER TABLE =====
ALTER TABLE edge2 ADD COLUMN new_col float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope');
ALTER TABLE edge2 ALTER COLUMN v TYPE int8_enc;

-- ===== 22.8 DEFAULT =====
DROP TABLE IF EXISTS edge8;
CREATE TABLE edge8(id serial, v int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det') DEFAULT 0);
INSERT INTO edge8 DEFAULT VALUES;

-- ===== 22.9 TRUNCATE + 多行INSERT =====
TRUNCATE edge2;
INSERT INTO edge2(v,t) VALUES (1,'a'),(NULL,NULL),(2,NULL),(NULL,'b');
SELECT 'trunc+insert' t, count(*) FROM edge2;

-- ===== 22.10 自JOIN =====
SELECT a.id aid, b.id bid FROM edge2 a, edge2 b WHERE a.v=b.v AND a.v IS NOT NULL AND a.id<>b.id;

-- ===== 22.11 会话存活验证 =====
SELECT 'session_ok' t, count(*) FROM edge2;

-- ============================================================================
-- 第二十二部分: 不同DEK/索引共存/KEK异常/明文密文转换
-- ============================================================================

-- ===== 23.1 不同DEK类型转换 =====
DROP TABLE IF EXISTS diff1, diff2;
DROP DEK IF EXISTS dek2;
DROP KEK IF EXISTS kek2;
CREATE KEK kek2 WITH (KEY_STORE=localkms, KEY_PATH='E:/tmp/tecc-test-keys/kek2.key');
CREATE DEK dek2 WITH VALUES(KEK=kek2);

CREATE TABLE diff1(id serial, v int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'));
INSERT INTO diff1(v) VALUES (100);
CREATE TABLE diff2(id serial, v int4_enc ENCRYPTED WITH (dek=dek2,support_algo='det'));
INSERT INTO diff2(v) VALUES (200);

INSERT INTO diff1(v) SELECT v FROM diff2;

SELECT a.id FROM diff1 a, diff2 b WHERE a.v=b.v;

-- ===== 23.2 btree+hash索引同时存在 =====
DROP TABLE IF EXISTS idx_both;
CREATE TABLE idx_both(id serial, v_ope int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));
INSERT INTO idx_both(v_ope) VALUES (1),(2),(3),(4),(5),(6),(7),(8),(9),(10);
CREATE INDEX idx_both_hash ON idx_both USING hash(v_ope);
CREATE INDEX idx_both_btree ON idx_both(v_ope);

SELECT indexrelid::regclass idx_name, amname FROM pg_index i JOIN pg_am a ON i.indrelid=a.oid WHERE indrelid='idx_both'::regclass ORDER BY amname;

SELECT count(*) FROM idx_both WHERE v_ope=5;

-- ===== 23.3 KEK/DEK异常场景 =====

CREATE KEK kek_bad WITH (KEY_STORE=localkms);

CREATE DEK dek_bad WITH VALUES(KEK=nonexistent_kek);

DROP KEK test_kek;

DROP DEK test_dek;

CREATE KEK kek_path WITH (KEY_STORE=localkms, KEY_PATH='E:/tmp/kek.key');

DROP TABLE IF EXISTS diff2 CASCADE;
DROP DEK IF EXISTS dek2;
DROP KEK IF EXISTS kek2 CASCADE;

-- ===== 23.4 异常dek_name参数建表 =====

CREATE TABLE bad_tbl(id int, v text_enc ENCRYPTED WITH (dek=bad_dek_name,support_algo='det'));

CREATE TABLE bad_tbl2(id int, v text_enc);

-- ===== 23.5 明文列密文列互转 =====
DROP TABLE IF EXISTS mixed_type;
CREATE TABLE mixed_type(id serial, v_plain int, v_enc int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'));
INSERT INTO mixed_type(v_plain,v_enc) VALUES (100,100);

INSERT INTO mixed_type(v_enc) SELECT v_plain FROM mixed_type WHERE id=1;

ALTER TABLE mixed_type ALTER COLUMN v_plain TYPE int4_enc;

-- ===== 23.6 DEFAULT 边界（带/不带 support_algo） =====

CREATE TABLE def_nosup(id int, v text_enc ENCRYPTED WITH (dek=test_dek) DEFAULT 'hello');

CREATE TABLE def_sup(id int, v text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search') DEFAULT 'hello');
INSERT INTO def_sup DEFAULT VALUES;

CREATE TABLE def_int(id int, v int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det') DEFAULT 100);
INSERT INTO def_int DEFAULT VALUES;

ALTER TABLE def_sup ALTER COLUMN v SET DEFAULT 'world';

CREATE TABLE def_null(id int, v text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search') DEFAULT NULL);
INSERT INTO def_null DEFAULT VALUES;

-- ===== 23.7 会话存活验证 =====
SELECT 'xdek_ok' t, count(*) FROM mixed_type;

-- ============================================================================
-- 第二十三部分: 清理
-- ============================================================================


