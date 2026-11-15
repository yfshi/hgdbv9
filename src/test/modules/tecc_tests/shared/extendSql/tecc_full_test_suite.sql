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

-- @param String text_enc
-- @param String varchar_enc
-- @param String bpchar_enc
-- @param String int2_enc
-- @param String int4_enc
-- @param String int8_enc
-- @param String numeric_enc
-- @param String float4_enc
-- @param String float8_enc
SELECT typname FROM pg_type WHERE typname IN (
    ?,?,?,
    ?,?,?,
    ?,?,?
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
-- @param String 张三
-- @param String 李四
-- @param String 王五
-- @param int 100
-- @param int 20000
-- @param int 3000000
-- @param BigDecimal 3.14
-- @param BigDecimal 2.718281828
-- @param BigDecimal 99999.99
-- @param String Alice
-- @param String Bob
-- @param String Charlie
-- @param int 1
-- @param int 1000
-- @param int 50000
-- @param BigDecimal 1.5
-- @param BigDecimal 100.5
-- @param BigDecimal 123.45
-- @param String 张三丰
-- @param String David
-- @param String Eve
-- @param int -100
-- @param int -50000
-- @param int -999999
-- @param BigDecimal -2.5
-- @param BigDecimal -0.001
-- @param BigDecimal -500.00
-- @param String hello
-- @param String world
-- @param String test
-- @param int 0
-- @param int 0
-- @param int 0
-- @param BigDecimal 0.0
-- @param BigDecimal 0.0
-- @param BigDecimal 0.00
-- @param String alice
-- @param String ALICE
-- @param String alice
-- @param int 1
-- @param int 1
-- @param int 1
-- @param BigDecimal 1.0
-- @param BigDecimal 1.0
-- @param BigDecimal 1.00
-- @param String 张三丰
-- @param String David2
-- @param String Eve2
-- @param int 100
-- @param int 20000
-- @param int 3000000
-- @param BigDecimal 3.14
-- @param BigDecimal 2.718281828
-- @param BigDecimal 99999.99
-- @param String Hello
-- @param String World
-- @param String TEST
-- @param int 50
-- @param int 55555
-- @param int 7777777
-- @param BigDecimal 50.5
-- @param BigDecimal 55.55
-- @param BigDecimal 500.00
-- @param String abc_def
-- @param String xyz_123
-- @param String foo_bar
-- @param int -1
-- @param int -1
-- @param int -1
-- @param BigDecimal -1.0
-- @param BigDecimal -1.0
-- @param BigDecimal -1.00
-- @param String test%like
-- @param String test_like
-- @param String pct_sign
-- @param int 10
-- @param int 100
-- @param int 1000
-- @param BigDecimal 10.10
-- @param BigDecimal 100.10
-- @param BigDecimal 10.10
-- @param String %percent
-- @param String under_score
-- @param String _start
-- @param int 20
-- @param int 200
-- @param int 2000
-- @param BigDecimal 20.20
-- @param BigDecimal 200.20
-- @param BigDecimal 20.20
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param String 
-- @param String 
-- @param String 
-- @param int -32768
-- @param int -2147483648
-- @param long -9223372036854775808
-- @param BigDecimal -3.402823e38
-- @param BigDecimal -1.7976931348e308
-- @param BigDecimal -99999.99
-- @param String 测试
-- @param String 测试
-- @param String 测试
-- @param int 42
-- @param int 4242
-- @param int 424242
-- @param BigDecimal 4.2
-- @param BigDecimal 42.42
-- @param BigDecimal 42.42
-- @param String test123
-- @param String abcXYZ
-- @param String mixCase
-- @param int 255
-- @param int 65535
-- @param int 16777215
-- @param BigDecimal 255.255
-- @param BigDecimal 65535.65535
-- @param BigDecimal 255.55
INSERT INTO tecc_test (col_text, col_varchar, col_bpchar,
                       col_int2, col_int4, col_int8,
                       col_float4, col_float8, col_numeric) VALUES
(?,      ?,     ?,      ?,     ?,     ?,        ?,        ?,   ?),
(?,     ?,      ?,   ?,       ?,      ?,          ?,         ?,         ?),
(?,    ?,    ?,       ?,    ?,    ?,        ?,        ?,        ?),
(?,     ?,    ?,      ?,       ?,         ?,              ?,         ?,           ?),
(?,     ?,    ?,     ?,       ?,         ?,              ?,         ?,           ?),
(?,    ?,   ?,      ?,     ?,     ?,        ?,        ?,   ?),
(?,     ?,    ?,      ?,      ?,     ?,        ?,        ?,         ?),
(?,   ?,  ?,   ?,      ?,        ?,             ?,        ?,          ?),
(?, ?,?,  ?,      ?,       ?,           ?,       ?,        ?),
(?,  ?,?,  ?,      ?,       ?,           ?,       ?,        ?),
(?,        ?,       ?,        ?,    ?,      ?,           ?,        ?,          ?),
(?,          ?,         ?,          ?,  ?, ?, ?, ?, ?),
(?,      ?,     ?,     ?,      ?,      ?,         ?,         ?,         ?),
(?,   ?,   ?,   ?,     ?,     ?,       ?,     ?,   ?);

SELECT 'tecc_test' AS tbl, count(*) AS row_count FROM tecc_test;

-- @param int 5
SELECT id, col_text, col_int4, col_numeric FROM tecc_test ORDER BY id LIMIT ?;

-- ============================================================================
-- 第四部分: 密态等值查询 (det: =, <>)
-- ============================================================================

-- @param String 张三
SELECT id, col_text FROM tecc_test WHERE col_text = ?;

-- @param String 王五
SELECT id, col_bpchar FROM tecc_test WHERE col_bpchar <> ? ORDER BY id;

-- @param int 20000
SELECT id, col_int4 FROM tecc_test WHERE col_int4 = ?;

-- @param int 0
SELECT id, col_int2 FROM tecc_test WHERE col_int2 <> ? ORDER BY id;

-- @param BigDecimal 1.5
SELECT id, col_float4 FROM tecc_test WHERE col_float4 = ?;

-- @param int -100
SELECT id, col_int2 FROM tecc_test WHERE col_int2 = ?;

-- @param int 0
SELECT id, col_int2 FROM tecc_test WHERE col_int2 = ?;

-- @param String 
SELECT id, col_text FROM tecc_test WHERE col_text = ?;

-- @param int 1000
SELECT id, col_int4 FROM tecc_test WHERE col_int4 > ?;

-- @param String test
SELECT id, name FROM t_rnd WHERE name = ?;

-- ============================================================================
-- 第五部分: 密态范围查询 (ope: >, >=, <, <=)
-- ============================================================================

-- @param int 0
SELECT id, col_int8 FROM tecc_test WHERE col_int8 > ? ORDER BY id;

-- @param int 50000
SELECT id, col_int8 FROM tecc_test WHERE col_int8 >= ? ORDER BY id;

-- @param int 0
SELECT id, col_int8 FROM tecc_test WHERE col_int8 < ? ORDER BY id;

-- @param BigDecimal 1.0
SELECT id, col_float8 FROM tecc_test WHERE col_float8 < ? ORDER BY id;

-- @param int 0
-- @param int 1000000
SELECT id, col_int8 FROM tecc_test WHERE col_int8 > ? AND col_int8 < ? ORDER BY id;

-- @param int 50000
SELECT id, col_int8 FROM tecc_test WHERE col_int8 = ?;

-- @param String David
SELECT id, col_varchar FROM tecc_test WHERE col_varchar > ? ORDER BY id;

-- @param String Bob
SELECT id, col_varchar FROM tecc_test WHERE col_varchar >= ? ORDER BY id;

-- ============================================================================
-- 第六部分: 密态模糊查询 (search: LIKE, NOT LIKE)
-- ============================================================================

-- @param String 张%
SELECT id, col_text FROM tecc_test WHERE col_text LIKE ? ORDER BY id;

-- @param String %b%
SELECT id, col_varchar FROM tecc_test WHERE col_varchar LIKE ? ORDER BY id;

-- @param String %张%
SELECT id, col_text FROM tecc_test WHERE col_text NOT LIKE ? ORDER BY id;

-- @param String test%
SELECT id, col_text FROM tecc_test WHERE col_text LIKE ? ORDER BY id;

-- @param String ALICE
SELECT id, col_varchar FROM tecc_test WHERE col_varchar LIKE ? ORDER BY id;

-- @param String _ob
SELECT id, col_varchar FROM tecc_test WHERE col_varchar LIKE ? ORDER BY id;

-- @param String 王%
SELECT id, col_bpchar FROM tecc_test WHERE col_bpchar LIKE ?;

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

-- @param int 100
-- @param int 20000
-- @param int 3000000
-- @param BigDecimal 3.14
-- @param BigDecimal 2.718
-- @param BigDecimal 99999.99
INSERT INTO tecc_int_test (col_int2, col_int4, col_int8, col_float4, col_float8, col_numeric)
VALUES (?, ?, ?, ?, ?, ?);

SELECT col_int2::int4_enc FROM tecc_int_test;

SELECT col_int2::int8_enc FROM tecc_int_test;

SELECT col_int4::int8_enc FROM tecc_int_test;

SELECT col_int2::float4_enc FROM tecc_int_test;

SELECT col_int2::float8_enc FROM tecc_int_test;

SELECT col_int2::numeric_enc FROM tecc_int_test;

SELECT col_float4::float8_enc FROM tecc_int_test;

SELECT col_float4::numeric_enc FROM tecc_int_test;

SELECT col_float8::numeric_enc FROM tecc_int_test;

-- @param int 3
SELECT col_text::varchar_enc FROM tecc_test ORDER BY id LIMIT ?;

-- @param int 3
SELECT col_varchar::text_enc FROM tecc_test ORDER BY id LIMIT ?;

-- @param int 3
SELECT col_varchar::bpchar_enc FROM tecc_test ORDER BY id LIMIT ?;

-- @param int 1
SELECT col_text::int4_enc FROM tecc_test LIMIT ?;

-- ============================================================================
-- 第八部分: 密态索引
-- ============================================================================

CREATE INDEX idx_tecc_int4_hash ON tecc_test USING hash(col_int4);

CREATE INDEX idx_tecc_text_btree ON tecc_test(col_text);

CREATE INDEX idx_tecc_int8_ope ON tecc_test(col_int8);

CREATE INDEX idx_t_rnd_btree ON t_rnd(name);

-- @param String tecc_test
-- @param String t_rnd
SELECT indexrelid::regclass, indrelid::regclass, amname
FROM pg_index i JOIN pg_am a ON i.indrelid = a.oid
WHERE indrelid IN (?::regclass, ?::regclass);

-- @param String test_rnd_1
INSERT INTO t_rnd (name) VALUES (?);

-- @param String test_rnd_2
INSERT INTO t_rnd (name) VALUES (?);

-- ============================================================================
-- 第九部分: RND随机加密算法
-- ============================================================================

-- 先清理RND索引避免干扰
DROP INDEX IF EXISTS idx_t_rnd_btree;

-- @param String same_text
INSERT INTO t_rnd (name) VALUES (?);

-- @param String same_text
INSERT INTO t_rnd (name) VALUES (?);

SELECT id, name FROM t_rnd ORDER BY id;

-- @param String same_text
SELECT * FROM t_rnd WHERE name = ?;

-- @param String a
SELECT * FROM t_rnd WHERE name > ?;

-- @param String %text%
SELECT * FROM t_rnd WHERE name LIKE ?;

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
-- @param String 张三
-- @param String 李四
-- @param String 王五
-- @param int 100
-- @param int 20000
-- @param int 3000000
-- @param BigDecimal 3.14
-- @param BigDecimal 2.718281828
-- @param BigDecimal 99999.99
-- @param String Alice
-- @param String Bob
-- @param String Charlie
-- @param int 1
-- @param int 1000
-- @param int 50000
-- @param BigDecimal 1.5
-- @param BigDecimal 100.5
-- @param BigDecimal 123.45
-- @param String 赵六
-- @param String 孙七
-- @param String 周八
-- @param int 32767
-- @param int 2147483647
-- @param long 9223372036854775807
-- @param BigDecimal 1.0e10
-- @param BigDecimal 1.7976931348623e308
-- @param BigDecimal 0.01
-- @param String 张三丰
-- @param String David
-- @param String Eve
-- @param int -100
-- @param int -50000
-- @param int -999999
-- @param BigDecimal -2.5
-- @param BigDecimal -0.001
-- @param BigDecimal -500.00
-- @param String hello
-- @param String world
-- @param String test
-- @param int 0
-- @param int 0
-- @param int 0
-- @param BigDecimal 0.0
-- @param BigDecimal 0.0
-- @param BigDecimal 0.00
-- @param String alice
-- @param String ALICE
-- @param String alice
-- @param int 1
-- @param int 1
-- @param int 1
-- @param BigDecimal 1.0
-- @param BigDecimal 1.0
-- @param BigDecimal 1.00
-- @param String 张三丰
-- @param String David2
-- @param String Eve2
-- @param int 100
-- @param int 20000
-- @param int 3000000
-- @param BigDecimal 3.14
-- @param BigDecimal 2.718281828
-- @param BigDecimal 99999.99
-- @param String Hello
-- @param String World
-- @param String TEST
-- @param int 50
-- @param int 55555
-- @param int 7777777
-- @param BigDecimal 50.5
-- @param BigDecimal 55.55
-- @param BigDecimal 500.00
-- @param String abc_def
-- @param String xyz_123
-- @param String foo_bar
-- @param int -1
-- @param int -1
-- @param int -1
-- @param BigDecimal -1.0
-- @param BigDecimal -1.0
-- @param BigDecimal -1.00
-- @param String test%like
-- @param String test_like
-- @param String pct_sign
-- @param int 10
-- @param int 100
-- @param int 1000
-- @param BigDecimal 10.10
-- @param BigDecimal 100.10
-- @param BigDecimal 10.10
-- @param String %percent
-- @param String under_score
-- @param String _start
-- @param int 20
-- @param int 200
-- @param int 2000
-- @param BigDecimal 20.20
-- @param BigDecimal 200.20
-- @param BigDecimal 20.20
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param String 
-- @param String 
-- @param String 
-- @param int -32768
-- @param int -2147483648
-- @param long -9223372036854775808
-- @param BigDecimal -3.402823e38
-- @param BigDecimal -1.7976931348e308
-- @param BigDecimal -99999.99
-- @param String 测试
-- @param String 测试
-- @param String 测试
-- @param int 42
-- @param int 4242
-- @param int 424242
-- @param BigDecimal 4.2
-- @param BigDecimal 42.42
-- @param BigDecimal 42.42
-- @param String test123
-- @param String abcXYZ
-- @param String mixCase
-- @param int 255
-- @param int 65535
-- @param int 16777215
-- @param BigDecimal 255.255
-- @param BigDecimal 65535.65535
-- @param BigDecimal 255.55
INSERT INTO plain (c_text, c_varchar, c_bpchar, c_int2, c_int4, c_int8, c_float4, c_float8, c_numeric) VALUES
(?,      ?,     ?,      ?,     ?,     ?,        ?,        ?,   ?),
(?,     ?,      ?,   ?,       ?,      ?,          ?,         ?,         ?),
(?,      ?,     ?,      ?,   ?, ?, ?, ?, ?),
(?,    ?,    ?,       ?,    ?,    ?,        ?,        ?,        ?),
(?,     ?,    ?,      ?,       ?,         ?,              ?,         ?,           ?),
(?,     ?,    ?,     ?,       ?,         ?,              ?,         ?,           ?),
(?,    ?,   ?,      ?,     ?,     ?,        ?,        ?,   ?),
(?,     ?,    ?,      ?,      ?,     ?,        ?,        ?,         ?),
(?,   ?,  ?,   ?,      ?,        ?,             ?,        ?,          ?),
(?, ?,?,  ?,      ?,       ?,           ?,       ?,        ?),
(?,  ?,?,  ?,      ?,       ?,           ?,       ?,        ?),
(?,        ?,       ?,        ?,    ?,      ?,           ?,        ?,          ?),
(?,          ?,         ?,          ?,  ?, ?, ?, ?, ?),
(?,      ?,     ?,     ?,      ?,      ?,         ?,         ?,         ?),
(?,   ?,   ?,   ?,     ?,     ?,       ?,     ?,   ?);

-- @param String 张三
-- @param String 张三
-- @param String 李四
-- @param String 李四
-- @param String 王五
-- @param String 王五
-- @param int 100
-- @param int 100
-- @param int 20000
-- @param int 20000
-- @param int 3000000
-- @param int 3000000
-- @param BigDecimal 3.14
-- @param BigDecimal 3.14
-- @param BigDecimal 2.718281828
-- @param BigDecimal 2.718281828
-- @param BigDecimal 99999.99
-- @param BigDecimal 99999.99
-- @param String Alice
-- @param String Alice
-- @param String Bob
-- @param String Bob
-- @param String Charlie
-- @param String Charlie
-- @param int 1
-- @param int 1
-- @param int 1000
-- @param int 1000
-- @param int 50000
-- @param int 50000
-- @param BigDecimal 1.5
-- @param BigDecimal 1.5
-- @param BigDecimal 100.5
-- @param BigDecimal 100.5
-- @param BigDecimal 123.45
-- @param BigDecimal 123.45
-- @param String 赵六
-- @param String 赵六
-- @param String 孙七
-- @param String 孙七
-- @param String 周八
-- @param String 周八
-- @param int 32767
-- @param int 32767
-- @param int 2147483647
-- @param int 2147483647
-- @param long 9223372036854775807
-- @param long 9223372036854775807
-- @param BigDecimal 1.0e10
-- @param BigDecimal 1.0e10
-- @param BigDecimal 1.7976931348623e308
-- @param BigDecimal 1.7976931348623e308
-- @param BigDecimal 0.01
-- @param BigDecimal 0.01
-- @param String 张三丰
-- @param String 张三丰
-- @param String David
-- @param String David
-- @param String Eve
-- @param String Eve
-- @param int -100
-- @param int -100
-- @param int -50000
-- @param int -50000
-- @param int -999999
-- @param int -999999
-- @param BigDecimal -2.5
-- @param BigDecimal -2.5
-- @param BigDecimal -0.001
-- @param BigDecimal -0.001
-- @param BigDecimal -500.00
-- @param BigDecimal -500.00
-- @param String hello
-- @param String hello
-- @param String world
-- @param String world
-- @param String test
-- @param String test
-- @param int 0
-- @param int 0
-- @param int 0
-- @param int 0
-- @param int 0
-- @param int 0
-- @param BigDecimal 0.0
-- @param BigDecimal 0.0
-- @param BigDecimal 0.0
-- @param BigDecimal 0.0
-- @param BigDecimal 0.00
-- @param BigDecimal 0.00
-- @param String alice
-- @param String alice
-- @param String ALICE
-- @param String ALICE
-- @param String alice
-- @param String alice
-- @param int 1
-- @param int 1
-- @param int 1
-- @param int 1
-- @param int 1
-- @param int 1
-- @param BigDecimal 1.0
-- @param BigDecimal 1.0
-- @param BigDecimal 1.0
-- @param BigDecimal 1.0
-- @param BigDecimal 1.00
-- @param BigDecimal 1.00
-- @param String 张三丰
-- @param String 张三丰
-- @param String David2
-- @param String David2
-- @param String Eve2
-- @param String Eve2
-- @param int 100
-- @param int 100
-- @param int 20000
-- @param int 20000
-- @param int 3000000
-- @param int 3000000
-- @param BigDecimal 3.14
-- @param BigDecimal 3.14
-- @param BigDecimal 2.718281828
-- @param BigDecimal 2.718281828
-- @param BigDecimal 99999.99
-- @param BigDecimal 99999.99
-- @param String Hello
-- @param String Hello
-- @param String World
-- @param String World
-- @param String TEST
-- @param String TEST
-- @param int 50
-- @param int 50
-- @param int 55555
-- @param int 55555
-- @param int 7777777
-- @param int 7777777
-- @param BigDecimal 50.5
-- @param BigDecimal 50.5
-- @param BigDecimal 55.55
-- @param BigDecimal 55.55
-- @param BigDecimal 500.00
-- @param BigDecimal 500.00
-- @param String abc_def
-- @param String abc_def
-- @param String xyz_123
-- @param String xyz_123
-- @param String foo_bar
-- @param String foo_bar
-- @param int -1
-- @param int -1
-- @param int -1
-- @param int -1
-- @param int -1
-- @param int -1
-- @param BigDecimal -1.0
-- @param BigDecimal -1.0
-- @param BigDecimal -1.0
-- @param BigDecimal -1.0
-- @param BigDecimal -1.00
-- @param BigDecimal -1.00
-- @param String test%like
-- @param String test%like
-- @param String test_like
-- @param String test_like
-- @param String pct_sign
-- @param String pct_sign
-- @param int 10
-- @param int 10
-- @param int 100
-- @param int 100
-- @param int 1000
-- @param int 1000
-- @param BigDecimal 10.10
-- @param BigDecimal 10.10
-- @param BigDecimal 100.10
-- @param BigDecimal 100.10
-- @param BigDecimal 10.10
-- @param BigDecimal 10.10
-- @param String %percent
-- @param String %percent
-- @param String under_score
-- @param String under_score
-- @param String _start
-- @param String _start
-- @param int 20
-- @param int 20
-- @param int 200
-- @param int 200
-- @param int 2000
-- @param int 2000
-- @param BigDecimal 20.20
-- @param BigDecimal 20.20
-- @param BigDecimal 200.20
-- @param BigDecimal 200.20
-- @param BigDecimal 20.20
-- @param BigDecimal 20.20
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param String 
-- @param String 
-- @param String 
-- @param String 
-- @param String 
-- @param String 
-- @param int -32768
-- @param int -32768
-- @param int -2147483648
-- @param int -2147483648
-- @param long -9223372036854775808
-- @param long -9223372036854775808
-- @param BigDecimal -3.402823e38
-- @param BigDecimal -3.402823e38
-- @param BigDecimal -1.7976931348e308
-- @param BigDecimal -1.7976931348e308
-- @param BigDecimal -99999.99
-- @param BigDecimal -99999.99
-- @param String 测试
-- @param String 测试
-- @param String 测试
-- @param String 测试
-- @param String 测试
-- @param String 测试
-- @param int 42
-- @param int 42
-- @param int 4242
-- @param int 4242
-- @param int 424242
-- @param int 424242
-- @param BigDecimal 4.2
-- @param BigDecimal 4.2
-- @param BigDecimal 42.42
-- @param BigDecimal 42.42
-- @param BigDecimal 42.42
-- @param BigDecimal 42.42
-- @param String test123
-- @param String test123
-- @param String abcXYZ
-- @param String abcXYZ
-- @param String mixCase
-- @param String mixCase
-- @param int 255
-- @param int 255
-- @param int 65535
-- @param int 65535
-- @param int 16777215
-- @param int 16777215
-- @param BigDecimal 255.255
-- @param BigDecimal 255.255
-- @param BigDecimal 65535.65535
-- @param BigDecimal 65535.65535
-- @param BigDecimal 255.55
-- @param BigDecimal 255.55
INSERT INTO enc (c_text_det, c_text_ope, c_varchar_det, c_varchar_ope, c_bpchar_det, c_bpchar_ope,
                 c_int2_det, c_int2_ope, c_int4_det, c_int4_ope, c_int8_det, c_int8_ope,
                 c_float4_det, c_float4_ope, c_float8_det, c_float8_ope, c_numeric_det, c_numeric_ope) VALUES
(?,?,?,?,?,?, ?,?, ?,?, ?,?, ?,?, ?,?, ?,?),
(?,?,?,?,?,?, ?,?, ?,?, ?,?, ?,?, ?,?, ?,?),
(?,?,?,?,?,?, ?,?, ?,?, ?,?, ?,?, ?,?, ?,?),
(?,?,?,?,?,?, ?,?, ?,?, ?,?, ?,?, ?,?, ?,?),
(?,?,?,?,?,?, ?,?, ?,?, ?,?, ?,?, ?,?, ?,?),
(?,?,?,?,?,?, ?,?, ?,?, ?,?, ?,?, ?,?, ?,?),
(?,?,?,?,?,?, ?,?, ?,?, ?,?, ?,?, ?,?, ?,?),
(?,?,?,?,?,?, ?,?, ?,?, ?,?, ?,?, ?,?, ?,?),
(?,?,?,?,?,?, ?,?, ?,?, ?,?, ?,?, ?,?, ?,?),
(?,?,?,?,?,?, ?,?, ?,?, ?,?, ?,?, ?,?, ?,?),
(?,?,?,?,?,?, ?,?, ?,?, ?,?, ?,?, ?,?, ?,?),
(?,?, ?,?, ?,?, ?,?, ?,?, ?,?, ?,?, ?,?, ?,?),
(?,?,?,?,?,?, ?,?, ?,?, ?,?, ?,?, ?,?, ?,?),
(?,?,?,?,?,?, ?,?, ?,?, ?,?, ?,?, ?,?, ?,?),
(?,?,?,?,?,?, ?,?, ?,?, ?,?, ?,?, ?,?, ?,?);

-- 对比函数: 每个查询打印 明文结果 | 密态结果


-- ============================================================================
-- 第十二部分: Bug 回归测试
-- ============================================================================

-- @param BigDecimal 99999.99
SELECT id, c_numeric_det FROM enc WHERE c_numeric_det = ?;

-- @param BigDecimal 99999.99
SELECT id, c_numeric FROM plain WHERE c_numeric = ?;

-- @param int 0
SELECT id, c_int8_ope FROM enc WHERE c_int8_ope < ? ORDER BY id;

-- @param int 0
SELECT id, c_int8 FROM plain WHERE c_int8 < ? ORDER BY id;

-- @param int 20000
SELECT id, c_int4_ope FROM enc WHERE c_int4_ope >= ? ORDER BY id;

-- @param int 0
SELECT id, c_float8_ope FROM enc WHERE c_float8_ope < ? ORDER BY id;

-- @param int -500
SELECT id, c_numeric_ope FROM enc WHERE c_numeric_ope > ? ORDER BY id;

-- @param String David
SELECT id, c_varchar_ope FROM enc WHERE c_varchar_ope > ? ORDER BY id;

-- @param String 测试
SELECT id, c_bpchar_ope FROM enc WHERE c_bpchar_ope > ? ORDER BY id;

-- @param String 测试
SELECT id, c_bpchar FROM plain WHERE c_bpchar > ? ORDER BY id;

-- @param String 张%
SELECT id, c_text_det FROM enc WHERE c_text_det LIKE ? ORDER BY id;

-- @param String 张%
SELECT id, c_text FROM plain WHERE c_text LIKE ? ORDER BY id;

-- @param int -10
-- @param int 10
SELECT id, c_int8_ope FROM enc WHERE c_int8_ope BETWEEN ? AND ? ORDER BY id;

-- @param int 0
SELECT id, c_int8_ope FROM enc WHERE c_int8_ope < ? ORDER BY id;

-- @param String test%
SELECT id, c_varchar_ope FROM enc WHERE c_varchar_ope LIKE ? ORDER BY id;

-- @param BigDecimal 1.0
SELECT id, c_float4_ope FROM enc WHERE c_float4_ope > ? ORDER BY id;

-- @param BigDecimal 1.0
SELECT id, c_float4 FROM plain WHERE c_float4 > ? ORDER BY id;

-- 正确前提: 相同DEK + 相同算法
DROP TABLE IF EXISTS b12_test;

CREATE TABLE b12_test (
    id int,
    v1 int2_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'det'),
    v2 int2_enc ENCRYPTED WITH (dek = test_dek, support_algo = 'det')
);

-- @param int 1
-- @param int 100
-- @param int 100
INSERT INTO b12_test VALUES (?, ?, ?);

-- @param int 2
-- @param int 100
-- @param int 200
INSERT INTO b12_test VALUES (?, ?, ?);

-- @param int 3
-- @param int 50
-- @param int 50
INSERT INTO b12_test VALUES (?, ?,  ?);

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

-- @param int 32767
-- @param int 32767
-- @param int -32768
-- @param int -32768
-- @param int 1
-- @param int 1
-- @param int -1
-- @param int -1
-- @param int 0
-- @param int 0
-- @param int 100
-- @param int 100
-- @param int -100
-- @param int -100
-- @param int 100
-- @param int 100
-- @param null null
-- @param null null
-- @param int -32767
-- @param int -32767
INSERT INTO b_int2(v_det,v_ope) VALUES (?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?);

-- @param int 32767
-- @param int -32768
-- @param int 0
-- @param int 100
SELECT 'max(32767)' t, count(*) FROM b_int2 WHERE v_det=?
UNION ALL SELECT 'min(-32768)', count(*) FROM b_int2 WHERE v_det=?
UNION ALL SELECT 'zero(0)', count(*) FROM b_int2 WHERE v_det=?
UNION ALL SELECT 'dup(100)', count(*) FROM b_int2 WHERE v_det=?
UNION ALL SELECT 'NULL', count(*) FROM b_int2 WHERE v_det IS NULL;

-- @param int 0
-- @param int 0
-- @param int -10
-- @param int 10
SELECT '>0' t, count(*) FROM b_int2 WHERE v_ope>?
UNION ALL SELECT '<0', count(*) FROM b_int2 WHERE v_ope<?
UNION ALL SELECT 'BETWEEN -10 AND 10', count(*) FROM b_int2 WHERE v_ope BETWEEN ? AND ?;

-- ===== 13.2 int4 边界值 =====

DROP TABLE IF EXISTS b_int4;

CREATE TABLE b_int4(id serial PRIMARY KEY, v_det int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

-- @param int 2147483647
-- @param int 2147483647
-- @param int -2147483648
-- @param int -2147483648
-- @param int 0
-- @param int 0
-- @param int 1
-- @param int 1
-- @param int -1
-- @param int -1
-- @param int 100000
-- @param int 100000
-- @param int -100000
-- @param int -100000
-- @param int 2147483646
-- @param int 2147483646
-- @param int -2147483647
-- @param int -2147483647
-- @param null null
-- @param null null
INSERT INTO b_int4(v_det,v_ope) VALUES (?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?);

-- @param int 2147483647
-- @param int -2147483648
-- @param int 2147483646
-- @param int -2147483647
-- @param int 0
SELECT 'max' t, count(*) FROM b_int4 WHERE v_det=?
UNION ALL SELECT 'min', count(*) FROM b_int4 WHERE v_det=?
UNION ALL SELECT 'max-1', count(*) FROM b_int4 WHERE v_det=?
UNION ALL SELECT 'min+1', count(*) FROM b_int4 WHERE v_det=?
UNION ALL SELECT 'zero', count(*) FROM b_int4 WHERE v_det=?;

-- @param int 0
-- @param int 0
-- @param int 0
-- @param int 1000000
SELECT '>0' t, count(*) FROM b_int4 WHERE v_ope>?
UNION ALL SELECT '<0', count(*) FROM b_int4 WHERE v_ope<?
UNION ALL SELECT '>0 AND <1000000', count(*) FROM b_int4 WHERE v_ope>? AND v_ope<?;

-- ===== 13.3 int8 边界值 =====

DROP TABLE IF EXISTS b_int8;

CREATE TABLE b_int8(id serial PRIMARY KEY, v_det int8_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope int8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

-- @param long 9223372036854775807
-- @param long 9223372036854775807
-- @param long -9223372036854775808
-- @param long -9223372036854775808
-- @param int 0
-- @param int 0
-- @param int 1
-- @param int 1
-- @param int -1
-- @param int -1
-- @param int 1000000000
-- @param int 1000000000
-- @param int -1000000000
-- @param int -1000000000
-- @param long 9223372036854775806
-- @param long 9223372036854775806
-- @param long -9223372036854775807
-- @param long -9223372036854775807
-- @param null null
-- @param null null
INSERT INTO b_int8(v_det,v_ope) VALUES (?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?);

-- @param long 9223372036854775807
-- @param long -9223372036854775808
-- @param long 9223372036854775806
-- @param long -9223372036854775807
-- @param int 0
SELECT 'max' t, count(*) FROM b_int8 WHERE v_det=?
UNION ALL SELECT 'min', count(*) FROM b_int8 WHERE v_det=?
UNION ALL SELECT 'max-1', count(*) FROM b_int8 WHERE v_det=?
UNION ALL SELECT 'min+1', count(*) FROM b_int8 WHERE v_det=?
UNION ALL SELECT 'zero', count(*) FROM b_int8 WHERE v_det=?;

-- @param int 0
-- @param int 0
SELECT '>0' t, count(*) FROM b_int8 WHERE v_ope>?
UNION ALL SELECT '<0', count(*) FROM b_int8 WHERE v_ope<?;

-- ===== 13.4 float4 边界值 =====

DROP TABLE IF EXISTS b_float4;

CREATE TABLE b_float4(id serial PRIMARY KEY, v_det float4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope float4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

-- @param BigDecimal 3.402823466E+38
-- @param BigDecimal 3.402823466E+38
-- @param BigDecimal -3.402823466E+38
-- @param BigDecimal -3.402823466E+38
-- @param BigDecimal 0.0
-- @param BigDecimal 0.0
-- @param BigDecimal 1.0
-- @param BigDecimal 1.0
-- @param BigDecimal -1.0
-- @param BigDecimal -1.0
-- @param BigDecimal 0.001
-- @param BigDecimal 0.001
-- @param BigDecimal -0.001
-- @param BigDecimal -0.001
-- @param BigDecimal 3.14
-- @param BigDecimal 3.14
-- @param null null
-- @param null null
INSERT INTO b_float4(v_det,v_ope) VALUES (?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?);

-- @param BigDecimal 0.0
-- @param BigDecimal 1.0
-- @param BigDecimal 3.14
SELECT '=0.0' t, count(*) FROM b_float4 WHERE v_det=?
UNION ALL SELECT '=1.0', count(*) FROM b_float4 WHERE v_det=?
UNION ALL SELECT '=3.14', count(*) FROM b_float4 WHERE v_det=?
UNION ALL SELECT 'IS NULL', count(*) FROM b_float4 WHERE v_det IS NULL;

-- @param int 0
-- @param int 0
SELECT '>0' t, count(*) FROM b_float4 WHERE v_ope>?
UNION ALL SELECT '<0', count(*) FROM b_float4 WHERE v_ope<?;

-- ===== 13.5 float8 边界值 =====

DROP TABLE IF EXISTS b_float8;

CREATE TABLE b_float8(id serial PRIMARY KEY, v_det float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

-- @param BigDecimal 100.5
-- @param BigDecimal 100.5
-- @param BigDecimal 0.0
-- @param BigDecimal 0.0
-- @param BigDecimal 1.0
-- @param BigDecimal 1.0
-- @param BigDecimal -1.0
-- @param BigDecimal -1.0
-- @param BigDecimal 2.718
-- @param BigDecimal 2.718
-- @param BigDecimal 0.001
-- @param BigDecimal 0.001
-- @param BigDecimal -0.001
-- @param BigDecimal -0.001
-- @param null null
-- @param null null
INSERT INTO b_float8(v_det,v_ope) VALUES (?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?);

-- @param BigDecimal 0.0
-- @param BigDecimal 1.0
SELECT '=0.0' t, count(*) FROM b_float8 WHERE v_det=?
UNION ALL SELECT '=1.0', count(*) FROM b_float8 WHERE v_det=?;

-- @param int 0
-- @param int 0
-- @param BigDecimal 1.0
SELECT '>0' t, count(*) FROM b_float8 WHERE v_ope>?
UNION ALL SELECT '<0', count(*) FROM b_float8 WHERE v_ope<?
UNION ALL SELECT '>=1.0', count(*) FROM b_float8 WHERE v_ope>=?;

-- ===== 13.6 numeric 边界值 =====

DROP TABLE IF EXISTS b_numeric;

CREATE TABLE b_numeric(id serial PRIMARY KEY, v numeric_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'));

-- @param BigDecimal 99999.99
-- @param BigDecimal -99999.99
-- @param int 0
-- @param int 1
-- @param int -1
-- @param BigDecimal 0.01
-- @param BigDecimal -0.01
-- @param BigDecimal 12345.67
-- @param BigDecimal 99999.99
-- @param BigDecimal 99999.99
-- @param null null
INSERT INTO b_numeric(v) VALUES (?),(?),(?),(?),(?),(?),(?),(?),(?),(?),(?);

-- @param int 0
-- @param BigDecimal 12345.67
-- @param BigDecimal 99999.99
SELECT '=0' t, count(*) FROM b_numeric WHERE v=?
UNION ALL SELECT '=12345.67', count(*) FROM b_numeric WHERE v=?
UNION ALL SELECT '=99999.99(dup)', count(*) FROM b_numeric WHERE v=?
UNION ALL SELECT 'IS NULL', count(*) FROM b_numeric WHERE v IS NULL;

-- @param int 0
-- @param int 0
SELECT '>0' t, count(*) FROM b_numeric WHERE v>?
UNION ALL SELECT '<0', count(*) FROM b_numeric WHERE v<?;

-- ===== 13.7 text 等价类+特殊字符 =====

DROP TABLE IF EXISTS b_text;

CREATE TABLE b_text(id serial PRIMARY KEY, v text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'));

-- @param String 
-- @param String a
-- @param String hello
-- @param String Hello
-- @param String HELLO
-- @param String 张三
-- @param String 张三丰
-- @param String abc_def
-- @param String test%like
-- @param String test_like
-- @param String a%b_c
-- @param String   空格  
-- @param String 12345
-- @param null null
INSERT INTO b_text(v) VALUES (?),(?),(?),(?),(?),(?),(?),(?),(?),(?),(?),(?),(?),(?);

-- @param String 
-- @param String hello
-- @param String Hello
-- @param String 张三
-- @param String a%b_c
-- @param String 12345
-- @param String 
SELECT '='''' (空串)' t, count(*) FROM b_text WHERE v=?
UNION ALL SELECT '=hello', count(*) FROM b_text WHERE v=?
UNION ALL SELECT '=Hello', count(*) FROM b_text WHERE v=?
UNION ALL SELECT '=张三', count(*) FROM b_text WHERE v=?
UNION ALL SELECT '=a%b_c', count(*) FROM b_text WHERE v=?
UNION ALL SELECT '=12345', count(*) FROM b_text WHERE v=?
UNION ALL SELECT '<>''''', count(*) FROM b_text WHERE v<>?;

-- ===== 13.8 NULL/空串专项 =====

DROP TABLE IF EXISTS b_null;

CREATE TABLE b_null(id serial PRIMARY KEY, c1 text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), c2 int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), c3 float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

-- @param null null
-- @param null null
-- @param null null
-- @param String 
-- @param int 0
-- @param BigDecimal 0.0
-- @param String a
-- @param int 1
-- @param BigDecimal 1.0
-- @param null null
-- @param int 2
-- @param BigDecimal 2.0
-- @param String b
-- @param null null
-- @param BigDecimal 3.0
-- @param String c
-- @param int 3
-- @param null null
INSERT INTO b_null(c1,c2,c3) VALUES (?,?,?),(?,?,?),(?,?,?),(?,?,?),(?,?,?),(?,?,?);

-- @param String 
-- @param int 0
-- @param BigDecimal 0.0
SELECT 'c1 IS NULL' t, count(*) FROM b_null WHERE c1 IS NULL
UNION ALL SELECT 'c2 IS NULL', count(*) FROM b_null WHERE c2 IS NULL
UNION ALL SELECT 'c3 IS NULL', count(*) FROM b_null WHERE c3 IS NULL
UNION ALL SELECT 'all NULL', count(*) FROM b_null WHERE c1 IS NULL AND c2 IS NULL AND c3 IS NULL
UNION ALL SELECT 'c1=''''', count(*) FROM b_null WHERE c1=?
UNION ALL SELECT 'c2=0 AND c3=0.0', count(*) FROM b_null WHERE c2=? AND c3=?;

SELECT 'c1 NOT NULL' t, count(*) FROM b_null WHERE c1 IS NOT NULL
UNION ALL SELECT 'c2 NOT NULL', count(*) FROM b_null WHERE c2 IS NOT NULL
UNION ALL SELECT 'c3 NOT NULL', count(*) FROM b_null WHERE c3 IS NOT NULL;

-- ===== 13.9 LIKE 等价类 =====

DROP TABLE IF EXISTS b_like;

CREATE TABLE b_like(id serial PRIMARY KEY, v text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'));

-- @param String abc
-- @param String abcd
-- @param String abcde
-- @param String xabc
-- @param String xabcy
-- @param String ab
-- @param String 
-- @param String 张三
-- @param String 张三丰
-- @param String abc_def
-- @param String abc%def
-- @param null null
INSERT INTO b_like(v) VALUES (?),(?),(?),(?),(?),(?),(?),(?),(?),(?),(?),(?);

-- @param String abc%
-- @param String ab%
-- @param String 张%
SELECT 'LIKE ''abc%''' t, count(*) FROM b_like WHERE v LIKE ?
UNION ALL SELECT 'LIKE ''ab%''' , count(*) FROM b_like WHERE v LIKE ?
UNION ALL SELECT 'LIKE ''张%''' , count(*) FROM b_like WHERE v LIKE ?;

-- @param String %abc%
-- @param String %bc%
SELECT 'LIKE ''%abc%''' t, count(*) FROM b_like WHERE v LIKE ?
UNION ALL SELECT 'LIKE ''%bc%''' , count(*) FROM b_like WHERE v LIKE ?;

-- @param String abc%
SELECT 'NOT LIKE ''abc%''' t, count(*) FROM b_like WHERE v NOT LIKE ? AND v IS NOT NULL;

-- @param String ab_
SELECT 'LIKE ''ab_''' t, count(*) FROM b_like WHERE v LIKE ?;

-- @param String abc
-- @param String 
SELECT 'LIKE ''abc''(精确)' t, count(*) FROM b_like WHERE v LIKE ?
UNION ALL SELECT 'LIKE ''''''' , count(*) FROM b_like WHERE v LIKE ?;

-- ===== 13.10 类型转换边界值 =====

DROP TABLE IF EXISTS b_cast;

CREATE TABLE b_cast(id serial PRIMARY KEY, c2 int2_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), c4 int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), c8 int8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'), f4 float4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), f8 float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'), num numeric_enc(10,2) ENCRYPTED WITH (dek=test_dek,support_algo='det'));

-- @param int 32767
-- @param int 2147483647
-- @param long 9223372036854775807
-- @param BigDecimal 3.4028235E38
-- @param BigDecimal 1.7976931348623157E308
-- @param BigDecimal 99999999.99
-- @param int -32768
-- @param int -2147483648
-- @param long -9223372036854775808
-- @param BigDecimal -3.4028235E38
-- @param BigDecimal -1.7976931348623157E308
-- @param BigDecimal -99999999.99
-- @param int 0
-- @param int 0
-- @param int 0
-- @param BigDecimal 0.0
-- @param BigDecimal 0.0
-- @param BigDecimal 0.00
INSERT INTO b_cast(c2,c4,c8,f4,f8,num) VALUES (?,?,?,?,?,?),(?,?,?,?,?,?),(?,?,?,?,?,?);

-- ===== 13.11 多类型混合查询 =====

DROP TABLE IF EXISTS b_mixed;

CREATE TABLE b_mixed(id serial PRIMARY KEY, name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), age int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'), salary float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

-- @param String 张三
-- @param int 25
-- @param int 5000
-- @param String 李四
-- @param int 30
-- @param int 8000
-- @param String 王五
-- @param int 28
-- @param int 6000
-- @param String 赵六
-- @param int 35
-- @param int 12000
-- @param String Alice
-- @param int 28
-- @param int 6500
-- @param String Bob
-- @param int 30
-- @param int 8500
-- @param String Charlie
-- @param int 35
-- @param int 11000
-- @param String 张三丰
-- @param int 22
-- @param int 4000
-- @param String Eve
-- @param int 40
-- @param int 15000
-- @param null null
-- @param int 25
-- @param int 5000
-- @param String 
-- @param int 0
-- @param int 0
-- @param String David
-- @param null null
-- @param int 9000
INSERT INTO b_mixed(name,age,salary) VALUES (?,?,?),(?,?,?),(?,?,?),(?,?,?),(?,?,?),(?,?,?),(?,?,?),(?,?,?),(?,?,?),(?,?,?),(?,?,?),(?,?,?);

-- @param String 张三
-- @param int 25
-- @param int 30
-- @param int 8000
-- @param int 28
-- @param int 35
-- @param String 张%
-- @param int 30
SELECT 'name=''张三'' AND age=25' t, count(*) FROM b_mixed WHERE name=? AND age=?
UNION ALL SELECT 'age=30 AND salary>8000', count(*) FROM b_mixed WHERE age=? AND salary>?
UNION ALL SELECT 'age=28 OR age=35', count(*) FROM b_mixed WHERE age=? OR age=?
UNION ALL SELECT 'LIKE+range', count(*) FROM b_mixed WHERE name LIKE ? AND age<?;

-- @param int 28
-- @param int 30
-- @param int 28
-- @param int 30
SELECT 'age IN(28,30)' t, count(*) FROM b_mixed WHERE age=? OR age=?
UNION ALL SELECT 'age NOT IN(28,30)', count(*) FROM b_mixed WHERE age<>? AND age<>? AND age IS NOT NULL;

-- @param int 25
-- @param int 30
-- @param int 5000
-- @param int 10000
SELECT 'age 25-30' t, count(*) FROM b_mixed WHERE age>=? AND age<=?
UNION ALL SELECT 'salary 5000-10000', count(*) FROM b_mixed WHERE salary>=? AND salary<=?;

-- ===== 13.12 聚合+排序 =====

SELECT 'count(*)' t, count(*) FROM b_int2
UNION ALL SELECT 'count(v_det)', count(v_det) FROM b_int2
UNION ALL SELECT 'count(DISTINCT v_det)', count(DISTINCT v_det) FROM b_int2;

-- ===== 13.13 两密态列比较 =====

DROP TABLE IF EXISTS b_colcmp;

CREATE TABLE b_colcmp(id serial PRIMARY KEY, v1 int2_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v2 int2_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'));

-- @param int 100
-- @param int 100
-- @param int 100
-- @param int 200
-- @param int 50
-- @param int 50
-- @param int -10
-- @param int -10
-- @param null null
-- @param null null
-- @param int 300
-- @param int 300
INSERT INTO b_colcmp(v1,v2) VALUES (?,?),(?,?),(?,?),(?,?),(?,?),(?,?);

SELECT 'v1=v2' t, count(*) FROM b_colcmp WHERE v1=v2
UNION ALL SELECT 'v1<>v2', count(*) FROM b_colcmp WHERE v1<>v2;

-- ============================================================================
-- 第十四部分: 密态索引专项 (9种类型 hash+btree + EXPLAIN验证)
-- ============================================================================

-- int2/int4/int8/float4/float8/numeric: 300行数据,hash(det)+btree(ope),EXPLAIN
-- ===== 14.1 int2_enc =====
DROP TABLE IF EXISTS idx_i2;

CREATE TABLE idx_i2(id serial PRIMARY KEY, v_det int2_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope int2_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

-- @param int 1
-- @param int 1
-- @param int 2
-- @param int 2
-- @param int 3
-- @param int 3
-- @param int 4
-- @param int 4
-- @param int 5
-- @param int 5
-- @param int 6
-- @param int 6
-- @param int 7
-- @param int 7
-- @param int 8
-- @param int 8
-- @param int 9
-- @param int 9
-- @param int 10
-- @param int 10
-- @param int 11
-- @param int 11
-- @param int 12
-- @param int 12
-- @param int 13
-- @param int 13
-- @param int 14
-- @param int 14
-- @param int 15
-- @param int 15
-- @param int 16
-- @param int 16
-- @param int 17
-- @param int 17
-- @param int 18
-- @param int 18
-- @param int 19
-- @param int 19
-- @param int 20
-- @param int 20
-- @param int 21
-- @param int 21
-- @param int 22
-- @param int 22
-- @param int 23
-- @param int 23
-- @param int 24
-- @param int 24
-- @param int 25
-- @param int 25
-- @param int 26
-- @param int 26
-- @param int 27
-- @param int 27
-- @param int 28
-- @param int 28
-- @param int 29
-- @param int 29
-- @param int 30
-- @param int 30
-- @param int 31
-- @param int 31
-- @param int 32
-- @param int 32
-- @param int 33
-- @param int 33
-- @param int 34
-- @param int 34
-- @param int 35
-- @param int 35
-- @param int 36
-- @param int 36
-- @param int 37
-- @param int 37
-- @param int 38
-- @param int 38
-- @param int 39
-- @param int 39
-- @param int 40
-- @param int 40
-- @param int 41
-- @param int 41
-- @param int 42
-- @param int 42
-- @param int 43
-- @param int 43
-- @param int 44
-- @param int 44
-- @param int 45
-- @param int 45
-- @param int 46
-- @param int 46
-- @param int 47
-- @param int 47
-- @param int 48
-- @param int 48
-- @param int 49
-- @param int 49
-- @param int 50
-- @param int 50
INSERT INTO idx_i2(v_det,v_ope) VALUES (?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?);

-- @param int 51
-- @param int 51
-- @param int 52
-- @param int 52
-- @param int 53
-- @param int 53
-- @param int 54
-- @param int 54
-- @param int 55
-- @param int 55
-- @param int 56
-- @param int 56
-- @param int 57
-- @param int 57
-- @param int 58
-- @param int 58
-- @param int 59
-- @param int 59
-- @param int 60
-- @param int 60
-- @param int 61
-- @param int 61
-- @param int 62
-- @param int 62
-- @param int 63
-- @param int 63
-- @param int 64
-- @param int 64
-- @param int 65
-- @param int 65
-- @param int 66
-- @param int 66
-- @param int 67
-- @param int 67
-- @param int 68
-- @param int 68
-- @param int 69
-- @param int 69
-- @param int 70
-- @param int 70
-- @param int 71
-- @param int 71
-- @param int 72
-- @param int 72
-- @param int 73
-- @param int 73
-- @param int 74
-- @param int 74
-- @param int 75
-- @param int 75
-- @param int 76
-- @param int 76
-- @param int 77
-- @param int 77
-- @param int 78
-- @param int 78
-- @param int 79
-- @param int 79
-- @param int 80
-- @param int 80
-- @param int 81
-- @param int 81
-- @param int 82
-- @param int 82
-- @param int 83
-- @param int 83
-- @param int 84
-- @param int 84
-- @param int 85
-- @param int 85
-- @param int 86
-- @param int 86
-- @param int 87
-- @param int 87
-- @param int 88
-- @param int 88
-- @param int 89
-- @param int 89
-- @param int 90
-- @param int 90
-- @param int 91
-- @param int 91
-- @param int 92
-- @param int 92
-- @param int 93
-- @param int 93
-- @param int 94
-- @param int 94
-- @param int 95
-- @param int 95
-- @param int 96
-- @param int 96
-- @param int 97
-- @param int 97
-- @param int 98
-- @param int 98
-- @param int 99
-- @param int 99
-- @param int 100
-- @param int 100
-- @param int 101
-- @param int 101
-- @param int 102
-- @param int 102
-- @param int 103
-- @param int 103
-- @param int 104
-- @param int 104
-- @param int 105
-- @param int 105
-- @param int 106
-- @param int 106
-- @param int 107
-- @param int 107
-- @param int 108
-- @param int 108
-- @param int 109
-- @param int 109
-- @param int 110
-- @param int 110
-- @param int 111
-- @param int 111
-- @param int 112
-- @param int 112
-- @param int 113
-- @param int 113
-- @param int 114
-- @param int 114
-- @param int 115
-- @param int 115
-- @param int 116
-- @param int 116
-- @param int 117
-- @param int 117
-- @param int 118
-- @param int 118
-- @param int 119
-- @param int 119
-- @param int 120
-- @param int 120
-- @param int 121
-- @param int 121
-- @param int 122
-- @param int 122
-- @param int 123
-- @param int 123
-- @param int 124
-- @param int 124
-- @param int 125
-- @param int 125
-- @param int 126
-- @param int 126
-- @param int 127
-- @param int 127
-- @param int 128
-- @param int 128
-- @param int 129
-- @param int 129
-- @param int 130
-- @param int 130
-- @param int 131
-- @param int 131
-- @param int 132
-- @param int 132
-- @param int 133
-- @param int 133
-- @param int 134
-- @param int 134
-- @param int 135
-- @param int 135
-- @param int 136
-- @param int 136
-- @param int 137
-- @param int 137
-- @param int 138
-- @param int 138
-- @param int 139
-- @param int 139
-- @param int 140
-- @param int 140
-- @param int 141
-- @param int 141
-- @param int 142
-- @param int 142
-- @param int 143
-- @param int 143
-- @param int 144
-- @param int 144
-- @param int 145
-- @param int 145
-- @param int 146
-- @param int 146
-- @param int 147
-- @param int 147
-- @param int 148
-- @param int 148
-- @param int 149
-- @param int 149
-- @param int 150
-- @param int 150
-- @param int 151
-- @param int 151
-- @param int 152
-- @param int 152
-- @param int 153
-- @param int 153
-- @param int 154
-- @param int 154
-- @param int 155
-- @param int 155
-- @param int 156
-- @param int 156
-- @param int 157
-- @param int 157
-- @param int 158
-- @param int 158
-- @param int 159
-- @param int 159
-- @param int 160
-- @param int 160
-- @param int 161
-- @param int 161
-- @param int 162
-- @param int 162
-- @param int 163
-- @param int 163
-- @param int 164
-- @param int 164
-- @param int 165
-- @param int 165
-- @param int 166
-- @param int 166
-- @param int 167
-- @param int 167
-- @param int 168
-- @param int 168
-- @param int 169
-- @param int 169
-- @param int 170
-- @param int 170
-- @param int 171
-- @param int 171
-- @param int 172
-- @param int 172
-- @param int 173
-- @param int 173
-- @param int 174
-- @param int 174
-- @param int 175
-- @param int 175
-- @param int 176
-- @param int 176
-- @param int 177
-- @param int 177
-- @param int 178
-- @param int 178
-- @param int 179
-- @param int 179
-- @param int 180
-- @param int 180
-- @param int 181
-- @param int 181
-- @param int 182
-- @param int 182
-- @param int 183
-- @param int 183
-- @param int 184
-- @param int 184
-- @param int 185
-- @param int 185
-- @param int 186
-- @param int 186
-- @param int 187
-- @param int 187
-- @param int 188
-- @param int 188
-- @param int 189
-- @param int 189
-- @param int 190
-- @param int 190
-- @param int 191
-- @param int 191
-- @param int 192
-- @param int 192
-- @param int 193
-- @param int 193
-- @param int 194
-- @param int 194
-- @param int 195
-- @param int 195
-- @param int 196
-- @param int 196
-- @param int 197
-- @param int 197
-- @param int 198
-- @param int 198
-- @param int 199
-- @param int 199
-- @param int 200
-- @param int 200
INSERT INTO idx_i2(v_det,v_ope) VALUES (?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?);

-- @param int 201
-- @param int 201
-- @param int 202
-- @param int 202
-- @param int 203
-- @param int 203
-- @param int 204
-- @param int 204
-- @param int 205
-- @param int 205
-- @param int 206
-- @param int 206
-- @param int 207
-- @param int 207
-- @param int 208
-- @param int 208
-- @param int 209
-- @param int 209
-- @param int 210
-- @param int 210
-- @param int 211
-- @param int 211
-- @param int 212
-- @param int 212
-- @param int 213
-- @param int 213
-- @param int 214
-- @param int 214
-- @param int 215
-- @param int 215
-- @param int 216
-- @param int 216
-- @param int 217
-- @param int 217
-- @param int 218
-- @param int 218
-- @param int 219
-- @param int 219
-- @param int 220
-- @param int 220
-- @param int 221
-- @param int 221
-- @param int 222
-- @param int 222
-- @param int 223
-- @param int 223
-- @param int 224
-- @param int 224
-- @param int 225
-- @param int 225
-- @param int 226
-- @param int 226
-- @param int 227
-- @param int 227
-- @param int 228
-- @param int 228
-- @param int 229
-- @param int 229
-- @param int 230
-- @param int 230
-- @param int 231
-- @param int 231
-- @param int 232
-- @param int 232
-- @param int 233
-- @param int 233
-- @param int 234
-- @param int 234
-- @param int 235
-- @param int 235
-- @param int 236
-- @param int 236
-- @param int 237
-- @param int 237
-- @param int 238
-- @param int 238
-- @param int 239
-- @param int 239
-- @param int 240
-- @param int 240
-- @param int 241
-- @param int 241
-- @param int 242
-- @param int 242
-- @param int 243
-- @param int 243
-- @param int 244
-- @param int 244
-- @param int 245
-- @param int 245
-- @param int 246
-- @param int 246
-- @param int 247
-- @param int 247
-- @param int 248
-- @param int 248
-- @param int 249
-- @param int 249
-- @param int 250
-- @param int 250
-- @param int 251
-- @param int 251
-- @param int 252
-- @param int 252
-- @param int 253
-- @param int 253
-- @param int 254
-- @param int 254
-- @param int 255
-- @param int 255
-- @param int 256
-- @param int 256
-- @param int 257
-- @param int 257
-- @param int 258
-- @param int 258
-- @param int 259
-- @param int 259
-- @param int 260
-- @param int 260
-- @param int 261
-- @param int 261
-- @param int 262
-- @param int 262
-- @param int 263
-- @param int 263
-- @param int 264
-- @param int 264
-- @param int 265
-- @param int 265
-- @param int 266
-- @param int 266
-- @param int 267
-- @param int 267
-- @param int 268
-- @param int 268
-- @param int 269
-- @param int 269
-- @param int 270
-- @param int 270
-- @param int 271
-- @param int 271
-- @param int 272
-- @param int 272
-- @param int 273
-- @param int 273
-- @param int 274
-- @param int 274
-- @param int 275
-- @param int 275
-- @param int 276
-- @param int 276
-- @param int 277
-- @param int 277
-- @param int 278
-- @param int 278
-- @param int 279
-- @param int 279
-- @param int 280
-- @param int 280
-- @param int 281
-- @param int 281
-- @param int 282
-- @param int 282
-- @param int 283
-- @param int 283
-- @param int 284
-- @param int 284
-- @param int 285
-- @param int 285
-- @param int 286
-- @param int 286
-- @param int 287
-- @param int 287
-- @param int 288
-- @param int 288
-- @param int 289
-- @param int 289
-- @param int 290
-- @param int 290
-- @param int 291
-- @param int 291
-- @param int 292
-- @param int 292
-- @param int 293
-- @param int 293
-- @param int 294
-- @param int 294
-- @param int 295
-- @param int 295
-- @param int 296
-- @param int 296
-- @param int 297
-- @param int 297
-- @param int 298
-- @param int 298
-- @param int 299
-- @param int 299
-- @param int 300
-- @param int 300
INSERT INTO idx_i2(v_det,v_ope) VALUES (?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?);

CREATE INDEX idx_i2_hash ON idx_i2 USING hash(v_det);

CREATE INDEX idx_i2_btree ON idx_i2(v_ope);

SET enable_seqscan = off;

SET enable_seqscan = on;

-- @param int 200
SELECT 'i2 det=200' t, count(*) FROM idx_i2 WHERE v_det = ?;

-- @param int 280
SELECT 'i2 ope>280' t, count(*) FROM idx_i2 WHERE v_ope > ?;

-- ===== 14.2 int4_enc =====
DROP TABLE IF EXISTS idx_i4;

CREATE TABLE idx_i4(id serial PRIMARY KEY, v_det int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

-- @param int 1
-- @param int 1
-- @param int 2
-- @param int 2
-- @param int 3
-- @param int 3
-- @param int 4
-- @param int 4
-- @param int 5
-- @param int 5
-- @param int 6
-- @param int 6
-- @param int 7
-- @param int 7
-- @param int 8
-- @param int 8
-- @param int 9
-- @param int 9
-- @param int 10
-- @param int 10
-- @param int 11
-- @param int 11
-- @param int 12
-- @param int 12
-- @param int 13
-- @param int 13
-- @param int 14
-- @param int 14
-- @param int 15
-- @param int 15
-- @param int 16
-- @param int 16
-- @param int 17
-- @param int 17
-- @param int 18
-- @param int 18
-- @param int 19
-- @param int 19
-- @param int 20
-- @param int 20
-- @param int 21
-- @param int 21
-- @param int 22
-- @param int 22
-- @param int 23
-- @param int 23
-- @param int 24
-- @param int 24
-- @param int 25
-- @param int 25
-- @param int 26
-- @param int 26
-- @param int 27
-- @param int 27
-- @param int 28
-- @param int 28
-- @param int 29
-- @param int 29
-- @param int 30
-- @param int 30
-- @param int 31
-- @param int 31
-- @param int 32
-- @param int 32
-- @param int 33
-- @param int 33
-- @param int 34
-- @param int 34
-- @param int 35
-- @param int 35
-- @param int 36
-- @param int 36
-- @param int 37
-- @param int 37
-- @param int 38
-- @param int 38
-- @param int 39
-- @param int 39
-- @param int 40
-- @param int 40
-- @param int 41
-- @param int 41
-- @param int 42
-- @param int 42
-- @param int 43
-- @param int 43
-- @param int 44
-- @param int 44
-- @param int 45
-- @param int 45
-- @param int 46
-- @param int 46
-- @param int 47
-- @param int 47
-- @param int 48
-- @param int 48
-- @param int 49
-- @param int 49
-- @param int 50
-- @param int 50
INSERT INTO idx_i4(v_det,v_ope) VALUES (?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?);

-- @param int 51
-- @param int 51
-- @param int 52
-- @param int 52
-- @param int 53
-- @param int 53
-- @param int 54
-- @param int 54
-- @param int 55
-- @param int 55
-- @param int 56
-- @param int 56
-- @param int 57
-- @param int 57
-- @param int 58
-- @param int 58
-- @param int 59
-- @param int 59
-- @param int 60
-- @param int 60
-- @param int 61
-- @param int 61
-- @param int 62
-- @param int 62
-- @param int 63
-- @param int 63
-- @param int 64
-- @param int 64
-- @param int 65
-- @param int 65
-- @param int 66
-- @param int 66
-- @param int 67
-- @param int 67
-- @param int 68
-- @param int 68
-- @param int 69
-- @param int 69
-- @param int 70
-- @param int 70
-- @param int 71
-- @param int 71
-- @param int 72
-- @param int 72
-- @param int 73
-- @param int 73
-- @param int 74
-- @param int 74
-- @param int 75
-- @param int 75
-- @param int 76
-- @param int 76
-- @param int 77
-- @param int 77
-- @param int 78
-- @param int 78
-- @param int 79
-- @param int 79
-- @param int 80
-- @param int 80
-- @param int 81
-- @param int 81
-- @param int 82
-- @param int 82
-- @param int 83
-- @param int 83
-- @param int 84
-- @param int 84
-- @param int 85
-- @param int 85
-- @param int 86
-- @param int 86
-- @param int 87
-- @param int 87
-- @param int 88
-- @param int 88
-- @param int 89
-- @param int 89
-- @param int 90
-- @param int 90
-- @param int 91
-- @param int 91
-- @param int 92
-- @param int 92
-- @param int 93
-- @param int 93
-- @param int 94
-- @param int 94
-- @param int 95
-- @param int 95
-- @param int 96
-- @param int 96
-- @param int 97
-- @param int 97
-- @param int 98
-- @param int 98
-- @param int 99
-- @param int 99
-- @param int 100
-- @param int 100
-- @param int 101
-- @param int 101
-- @param int 102
-- @param int 102
-- @param int 103
-- @param int 103
-- @param int 104
-- @param int 104
-- @param int 105
-- @param int 105
-- @param int 106
-- @param int 106
-- @param int 107
-- @param int 107
-- @param int 108
-- @param int 108
-- @param int 109
-- @param int 109
-- @param int 110
-- @param int 110
-- @param int 111
-- @param int 111
-- @param int 112
-- @param int 112
-- @param int 113
-- @param int 113
-- @param int 114
-- @param int 114
-- @param int 115
-- @param int 115
-- @param int 116
-- @param int 116
-- @param int 117
-- @param int 117
-- @param int 118
-- @param int 118
-- @param int 119
-- @param int 119
-- @param int 120
-- @param int 120
-- @param int 121
-- @param int 121
-- @param int 122
-- @param int 122
-- @param int 123
-- @param int 123
-- @param int 124
-- @param int 124
-- @param int 125
-- @param int 125
-- @param int 126
-- @param int 126
-- @param int 127
-- @param int 127
-- @param int 128
-- @param int 128
-- @param int 129
-- @param int 129
-- @param int 130
-- @param int 130
-- @param int 131
-- @param int 131
-- @param int 132
-- @param int 132
-- @param int 133
-- @param int 133
-- @param int 134
-- @param int 134
-- @param int 135
-- @param int 135
-- @param int 136
-- @param int 136
-- @param int 137
-- @param int 137
-- @param int 138
-- @param int 138
-- @param int 139
-- @param int 139
-- @param int 140
-- @param int 140
-- @param int 141
-- @param int 141
-- @param int 142
-- @param int 142
-- @param int 143
-- @param int 143
-- @param int 144
-- @param int 144
-- @param int 145
-- @param int 145
-- @param int 146
-- @param int 146
-- @param int 147
-- @param int 147
-- @param int 148
-- @param int 148
-- @param int 149
-- @param int 149
-- @param int 150
-- @param int 150
-- @param int 151
-- @param int 151
-- @param int 152
-- @param int 152
-- @param int 153
-- @param int 153
-- @param int 154
-- @param int 154
-- @param int 155
-- @param int 155
-- @param int 156
-- @param int 156
-- @param int 157
-- @param int 157
-- @param int 158
-- @param int 158
-- @param int 159
-- @param int 159
-- @param int 160
-- @param int 160
-- @param int 161
-- @param int 161
-- @param int 162
-- @param int 162
-- @param int 163
-- @param int 163
-- @param int 164
-- @param int 164
-- @param int 165
-- @param int 165
-- @param int 166
-- @param int 166
-- @param int 167
-- @param int 167
-- @param int 168
-- @param int 168
-- @param int 169
-- @param int 169
-- @param int 170
-- @param int 170
-- @param int 171
-- @param int 171
-- @param int 172
-- @param int 172
-- @param int 173
-- @param int 173
-- @param int 174
-- @param int 174
-- @param int 175
-- @param int 175
-- @param int 176
-- @param int 176
-- @param int 177
-- @param int 177
-- @param int 178
-- @param int 178
-- @param int 179
-- @param int 179
-- @param int 180
-- @param int 180
-- @param int 181
-- @param int 181
-- @param int 182
-- @param int 182
-- @param int 183
-- @param int 183
-- @param int 184
-- @param int 184
-- @param int 185
-- @param int 185
-- @param int 186
-- @param int 186
-- @param int 187
-- @param int 187
-- @param int 188
-- @param int 188
-- @param int 189
-- @param int 189
-- @param int 190
-- @param int 190
-- @param int 191
-- @param int 191
-- @param int 192
-- @param int 192
-- @param int 193
-- @param int 193
-- @param int 194
-- @param int 194
-- @param int 195
-- @param int 195
-- @param int 196
-- @param int 196
-- @param int 197
-- @param int 197
-- @param int 198
-- @param int 198
-- @param int 199
-- @param int 199
-- @param int 200
-- @param int 200
INSERT INTO idx_i4(v_det,v_ope) VALUES (?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?);

-- @param int 201
-- @param int 201
-- @param int 202
-- @param int 202
-- @param int 203
-- @param int 203
-- @param int 204
-- @param int 204
-- @param int 205
-- @param int 205
-- @param int 206
-- @param int 206
-- @param int 207
-- @param int 207
-- @param int 208
-- @param int 208
-- @param int 209
-- @param int 209
-- @param int 210
-- @param int 210
-- @param int 211
-- @param int 211
-- @param int 212
-- @param int 212
-- @param int 213
-- @param int 213
-- @param int 214
-- @param int 214
-- @param int 215
-- @param int 215
-- @param int 216
-- @param int 216
-- @param int 217
-- @param int 217
-- @param int 218
-- @param int 218
-- @param int 219
-- @param int 219
-- @param int 220
-- @param int 220
-- @param int 221
-- @param int 221
-- @param int 222
-- @param int 222
-- @param int 223
-- @param int 223
-- @param int 224
-- @param int 224
-- @param int 225
-- @param int 225
-- @param int 226
-- @param int 226
-- @param int 227
-- @param int 227
-- @param int 228
-- @param int 228
-- @param int 229
-- @param int 229
-- @param int 230
-- @param int 230
-- @param int 231
-- @param int 231
-- @param int 232
-- @param int 232
-- @param int 233
-- @param int 233
-- @param int 234
-- @param int 234
-- @param int 235
-- @param int 235
-- @param int 236
-- @param int 236
-- @param int 237
-- @param int 237
-- @param int 238
-- @param int 238
-- @param int 239
-- @param int 239
-- @param int 240
-- @param int 240
-- @param int 241
-- @param int 241
-- @param int 242
-- @param int 242
-- @param int 243
-- @param int 243
-- @param int 244
-- @param int 244
-- @param int 245
-- @param int 245
-- @param int 246
-- @param int 246
-- @param int 247
-- @param int 247
-- @param int 248
-- @param int 248
-- @param int 249
-- @param int 249
-- @param int 250
-- @param int 250
-- @param int 251
-- @param int 251
-- @param int 252
-- @param int 252
-- @param int 253
-- @param int 253
-- @param int 254
-- @param int 254
-- @param int 255
-- @param int 255
-- @param int 256
-- @param int 256
-- @param int 257
-- @param int 257
-- @param int 258
-- @param int 258
-- @param int 259
-- @param int 259
-- @param int 260
-- @param int 260
-- @param int 261
-- @param int 261
-- @param int 262
-- @param int 262
-- @param int 263
-- @param int 263
-- @param int 264
-- @param int 264
-- @param int 265
-- @param int 265
-- @param int 266
-- @param int 266
-- @param int 267
-- @param int 267
-- @param int 268
-- @param int 268
-- @param int 269
-- @param int 269
-- @param int 270
-- @param int 270
-- @param int 271
-- @param int 271
-- @param int 272
-- @param int 272
-- @param int 273
-- @param int 273
-- @param int 274
-- @param int 274
-- @param int 275
-- @param int 275
-- @param int 276
-- @param int 276
-- @param int 277
-- @param int 277
-- @param int 278
-- @param int 278
-- @param int 279
-- @param int 279
-- @param int 280
-- @param int 280
-- @param int 281
-- @param int 281
-- @param int 282
-- @param int 282
-- @param int 283
-- @param int 283
-- @param int 284
-- @param int 284
-- @param int 285
-- @param int 285
-- @param int 286
-- @param int 286
-- @param int 287
-- @param int 287
-- @param int 288
-- @param int 288
-- @param int 289
-- @param int 289
-- @param int 290
-- @param int 290
-- @param int 291
-- @param int 291
-- @param int 292
-- @param int 292
-- @param int 293
-- @param int 293
-- @param int 294
-- @param int 294
-- @param int 295
-- @param int 295
-- @param int 296
-- @param int 296
-- @param int 297
-- @param int 297
-- @param int 298
-- @param int 298
-- @param int 299
-- @param int 299
-- @param int 300
-- @param int 300
INSERT INTO idx_i4(v_det,v_ope) VALUES (?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?);

CREATE INDEX idx_i4_hash ON idx_i4 USING hash(v_det);

CREATE INDEX idx_i4_btree ON idx_i4(v_ope);

SET enable_seqscan = off;

SET enable_seqscan = on;

-- @param int 200
SELECT 'i4 det=200' t, count(*) FROM idx_i4 WHERE v_det = ?;

-- @param int 280
SELECT 'i4 ope>280' t, count(*) FROM idx_i4 WHERE v_ope > ?;

-- ===== 14.3 int8_enc =====
DROP TABLE IF EXISTS idx_i8;

CREATE TABLE idx_i8(id serial PRIMARY KEY, v_det int8_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope int8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

-- @param int 1
-- @param int 1
-- @param int 2
-- @param int 2
-- @param int 3
-- @param int 3
-- @param int 4
-- @param int 4
-- @param int 5
-- @param int 5
-- @param int 6
-- @param int 6
-- @param int 7
-- @param int 7
-- @param int 8
-- @param int 8
-- @param int 9
-- @param int 9
-- @param int 10
-- @param int 10
-- @param int 21
-- @param int 21
-- @param int 22
-- @param int 22
-- @param int 23
-- @param int 23
-- @param int 24
-- @param int 24
-- @param int 25
-- @param int 25
-- @param int 26
-- @param int 26
-- @param int 27
-- @param int 27
-- @param int 28
-- @param int 28
-- @param int 29
-- @param int 29
-- @param int 30
-- @param int 30
-- @param int 41
-- @param int 41
-- @param int 42
-- @param int 42
-- @param int 43
-- @param int 43
-- @param int 44
-- @param int 44
-- @param int 45
-- @param int 45
-- @param int 46
-- @param int 46
-- @param int 47
-- @param int 47
-- @param int 48
-- @param int 48
-- @param int 49
-- @param int 49
-- @param int 50
-- @param int 50
INSERT INTO idx_i8(v_det,v_ope) VALUES (?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?);

-- @param int 51
-- @param int 51
-- @param int 52
-- @param int 52
-- @param int 53
-- @param int 53
-- @param int 54
-- @param int 54
-- @param int 55
-- @param int 55
-- @param int 56
-- @param int 56
-- @param int 57
-- @param int 57
-- @param int 58
-- @param int 58
-- @param int 59
-- @param int 59
-- @param int 60
-- @param int 60
-- @param int 61
-- @param int 61
-- @param int 62
-- @param int 62
-- @param int 63
-- @param int 63
-- @param int 64
-- @param int 64
-- @param int 65
-- @param int 65
-- @param int 66
-- @param int 66
-- @param int 67
-- @param int 67
-- @param int 68
-- @param int 68
-- @param int 69
-- @param int 69
-- @param int 70
-- @param int 70
-- @param int 71
-- @param int 71
-- @param int 72
-- @param int 72
-- @param int 73
-- @param int 73
-- @param int 74
-- @param int 74
-- @param int 75
-- @param int 75
-- @param int 76
-- @param int 76
-- @param int 77
-- @param int 77
-- @param int 78
-- @param int 78
-- @param int 79
-- @param int 79
-- @param int 80
-- @param int 80
-- @param int 81
-- @param int 81
-- @param int 82
-- @param int 82
-- @param int 83
-- @param int 83
-- @param int 84
-- @param int 84
-- @param int 85
-- @param int 85
-- @param int 86
-- @param int 86
-- @param int 87
-- @param int 87
-- @param int 88
-- @param int 88
-- @param int 89
-- @param int 89
-- @param int 90
-- @param int 90
-- @param int 91
-- @param int 91
-- @param int 92
-- @param int 92
-- @param int 93
-- @param int 93
-- @param int 94
-- @param int 94
-- @param int 95
-- @param int 95
-- @param int 96
-- @param int 96
-- @param int 97
-- @param int 97
-- @param int 98
-- @param int 98
-- @param int 99
-- @param int 99
-- @param int 100
-- @param int 100
-- @param int 101
-- @param int 101
-- @param int 102
-- @param int 102
-- @param int 103
-- @param int 103
-- @param int 104
-- @param int 104
-- @param int 105
-- @param int 105
-- @param int 106
-- @param int 106
-- @param int 107
-- @param int 107
-- @param int 108
-- @param int 108
-- @param int 109
-- @param int 109
-- @param int 110
-- @param int 110
-- @param int 111
-- @param int 111
-- @param int 112
-- @param int 112
-- @param int 113
-- @param int 113
-- @param int 114
-- @param int 114
-- @param int 115
-- @param int 115
-- @param int 116
-- @param int 116
-- @param int 117
-- @param int 117
-- @param int 118
-- @param int 118
-- @param int 119
-- @param int 119
-- @param int 120
-- @param int 120
-- @param int 121
-- @param int 121
-- @param int 122
-- @param int 122
-- @param int 123
-- @param int 123
-- @param int 124
-- @param int 124
-- @param int 125
-- @param int 125
-- @param int 126
-- @param int 126
-- @param int 127
-- @param int 127
-- @param int 128
-- @param int 128
-- @param int 129
-- @param int 129
-- @param int 130
-- @param int 130
-- @param int 131
-- @param int 131
-- @param int 132
-- @param int 132
-- @param int 133
-- @param int 133
-- @param int 134
-- @param int 134
-- @param int 135
-- @param int 135
-- @param int 136
-- @param int 136
-- @param int 137
-- @param int 137
-- @param int 138
-- @param int 138
-- @param int 139
-- @param int 139
-- @param int 140
-- @param int 140
-- @param int 141
-- @param int 141
-- @param int 142
-- @param int 142
-- @param int 143
-- @param int 143
-- @param int 144
-- @param int 144
-- @param int 145
-- @param int 145
-- @param int 146
-- @param int 146
-- @param int 147
-- @param int 147
-- @param int 148
-- @param int 148
-- @param int 149
-- @param int 149
-- @param int 150
-- @param int 150
-- @param int 151
-- @param int 151
-- @param int 152
-- @param int 152
-- @param int 153
-- @param int 153
-- @param int 154
-- @param int 154
-- @param int 155
-- @param int 155
-- @param int 156
-- @param int 156
-- @param int 157
-- @param int 157
-- @param int 158
-- @param int 158
-- @param int 159
-- @param int 159
-- @param int 160
-- @param int 160
-- @param int 161
-- @param int 161
-- @param int 162
-- @param int 162
-- @param int 163
-- @param int 163
-- @param int 164
-- @param int 164
-- @param int 165
-- @param int 165
-- @param int 166
-- @param int 166
-- @param int 167
-- @param int 167
-- @param int 168
-- @param int 168
-- @param int 169
-- @param int 169
-- @param int 170
-- @param int 170
-- @param int 171
-- @param int 171
-- @param int 172
-- @param int 172
-- @param int 173
-- @param int 173
-- @param int 174
-- @param int 174
-- @param int 175
-- @param int 175
-- @param int 176
-- @param int 176
-- @param int 177
-- @param int 177
-- @param int 178
-- @param int 178
-- @param int 179
-- @param int 179
-- @param int 180
-- @param int 180
-- @param int 181
-- @param int 181
-- @param int 182
-- @param int 182
-- @param int 183
-- @param int 183
-- @param int 184
-- @param int 184
-- @param int 185
-- @param int 185
-- @param int 186
-- @param int 186
-- @param int 187
-- @param int 187
-- @param int 188
-- @param int 188
-- @param int 189
-- @param int 189
-- @param int 190
-- @param int 190
-- @param int 191
-- @param int 191
-- @param int 192
-- @param int 192
-- @param int 193
-- @param int 193
-- @param int 194
-- @param int 194
-- @param int 195
-- @param int 195
-- @param int 196
-- @param int 196
-- @param int 197
-- @param int 197
-- @param int 198
-- @param int 198
-- @param int 199
-- @param int 199
-- @param int 200
-- @param int 200
INSERT INTO idx_i8(v_det,v_ope) VALUES (?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?);

CREATE INDEX idx_i8_hash ON idx_i8 USING hash(v_det);

CREATE INDEX idx_i8_btree ON idx_i8(v_ope);

SET enable_seqscan = off;

SET enable_seqscan = on;

-- ===== 14.4 float4_enc =====
DROP TABLE IF EXISTS idx_f4;

CREATE TABLE idx_f4(id serial PRIMARY KEY, v_det float4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope float4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

-- @param BigDecimal 1.0
-- @param BigDecimal 1.0
-- @param BigDecimal 2.0
-- @param BigDecimal 2.0
-- @param BigDecimal 3.0
-- @param BigDecimal 3.0
-- @param BigDecimal 4.0
-- @param BigDecimal 4.0
-- @param BigDecimal 5.0
-- @param BigDecimal 5.0
-- @param BigDecimal 6.0
-- @param BigDecimal 6.0
-- @param BigDecimal 7.0
-- @param BigDecimal 7.0
-- @param BigDecimal 8.0
-- @param BigDecimal 8.0
-- @param BigDecimal 9.0
-- @param BigDecimal 9.0
-- @param BigDecimal 10.0
-- @param BigDecimal 10.0
-- @param BigDecimal 11.0
-- @param BigDecimal 11.0
-- @param BigDecimal 12.0
-- @param BigDecimal 12.0
-- @param BigDecimal 13.0
-- @param BigDecimal 13.0
-- @param BigDecimal 14.0
-- @param BigDecimal 14.0
-- @param BigDecimal 15.0
-- @param BigDecimal 15.0
-- @param BigDecimal 16.0
-- @param BigDecimal 16.0
-- @param BigDecimal 17.0
-- @param BigDecimal 17.0
-- @param BigDecimal 18.0
-- @param BigDecimal 18.0
-- @param BigDecimal 19.0
-- @param BigDecimal 19.0
-- @param BigDecimal 20.0
-- @param BigDecimal 20.0
-- @param BigDecimal 21.0
-- @param BigDecimal 21.0
-- @param BigDecimal 22.0
-- @param BigDecimal 22.0
-- @param BigDecimal 23.0
-- @param BigDecimal 23.0
-- @param BigDecimal 24.0
-- @param BigDecimal 24.0
-- @param BigDecimal 25.0
-- @param BigDecimal 25.0
-- @param BigDecimal 26.0
-- @param BigDecimal 26.0
-- @param BigDecimal 27.0
-- @param BigDecimal 27.0
-- @param BigDecimal 28.0
-- @param BigDecimal 28.0
-- @param BigDecimal 29.0
-- @param BigDecimal 29.0
-- @param BigDecimal 30.0
-- @param BigDecimal 30.0
-- @param BigDecimal 31.0
-- @param BigDecimal 31.0
-- @param BigDecimal 32.0
-- @param BigDecimal 32.0
-- @param BigDecimal 33.0
-- @param BigDecimal 33.0
-- @param BigDecimal 34.0
-- @param BigDecimal 34.0
-- @param BigDecimal 35.0
-- @param BigDecimal 35.0
-- @param BigDecimal 36.0
-- @param BigDecimal 36.0
-- @param BigDecimal 37.0
-- @param BigDecimal 37.0
-- @param BigDecimal 38.0
-- @param BigDecimal 38.0
-- @param BigDecimal 39.0
-- @param BigDecimal 39.0
-- @param BigDecimal 40.0
-- @param BigDecimal 40.0
-- @param BigDecimal 41.0
-- @param BigDecimal 41.0
-- @param BigDecimal 42.0
-- @param BigDecimal 42.0
-- @param BigDecimal 43.0
-- @param BigDecimal 43.0
-- @param BigDecimal 44.0
-- @param BigDecimal 44.0
-- @param BigDecimal 45.0
-- @param BigDecimal 45.0
-- @param BigDecimal 46.0
-- @param BigDecimal 46.0
-- @param BigDecimal 47.0
-- @param BigDecimal 47.0
-- @param BigDecimal 48.0
-- @param BigDecimal 48.0
-- @param BigDecimal 49.0
-- @param BigDecimal 49.0
-- @param BigDecimal 50.0
-- @param BigDecimal 50.0
INSERT INTO idx_f4(v_det,v_ope) VALUES (?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?);

-- @param BigDecimal 51.0
-- @param BigDecimal 51.0
-- @param BigDecimal 52.0
-- @param BigDecimal 52.0
-- @param BigDecimal 53.0
-- @param BigDecimal 53.0
-- @param BigDecimal 54.0
-- @param BigDecimal 54.0
-- @param BigDecimal 55.0
-- @param BigDecimal 55.0
-- @param BigDecimal 56.0
-- @param BigDecimal 56.0
-- @param BigDecimal 57.0
-- @param BigDecimal 57.0
-- @param BigDecimal 58.0
-- @param BigDecimal 58.0
-- @param BigDecimal 59.0
-- @param BigDecimal 59.0
-- @param BigDecimal 60.0
-- @param BigDecimal 60.0
-- @param BigDecimal 61.0
-- @param BigDecimal 61.0
-- @param BigDecimal 62.0
-- @param BigDecimal 62.0
-- @param BigDecimal 63.0
-- @param BigDecimal 63.0
-- @param BigDecimal 64.0
-- @param BigDecimal 64.0
-- @param BigDecimal 65.0
-- @param BigDecimal 65.0
-- @param BigDecimal 66.0
-- @param BigDecimal 66.0
-- @param BigDecimal 67.0
-- @param BigDecimal 67.0
-- @param BigDecimal 68.0
-- @param BigDecimal 68.0
-- @param BigDecimal 69.0
-- @param BigDecimal 69.0
-- @param BigDecimal 70.0
-- @param BigDecimal 70.0
-- @param BigDecimal 71.0
-- @param BigDecimal 71.0
-- @param BigDecimal 72.0
-- @param BigDecimal 72.0
-- @param BigDecimal 73.0
-- @param BigDecimal 73.0
-- @param BigDecimal 74.0
-- @param BigDecimal 74.0
-- @param BigDecimal 75.0
-- @param BigDecimal 75.0
-- @param BigDecimal 76.0
-- @param BigDecimal 76.0
-- @param BigDecimal 77.0
-- @param BigDecimal 77.0
-- @param BigDecimal 78.0
-- @param BigDecimal 78.0
-- @param BigDecimal 79.0
-- @param BigDecimal 79.0
-- @param BigDecimal 80.0
-- @param BigDecimal 80.0
-- @param BigDecimal 81.0
-- @param BigDecimal 81.0
-- @param BigDecimal 82.0
-- @param BigDecimal 82.0
-- @param BigDecimal 83.0
-- @param BigDecimal 83.0
-- @param BigDecimal 84.0
-- @param BigDecimal 84.0
-- @param BigDecimal 85.0
-- @param BigDecimal 85.0
-- @param BigDecimal 86.0
-- @param BigDecimal 86.0
-- @param BigDecimal 87.0
-- @param BigDecimal 87.0
-- @param BigDecimal 88.0
-- @param BigDecimal 88.0
-- @param BigDecimal 89.0
-- @param BigDecimal 89.0
-- @param BigDecimal 90.0
-- @param BigDecimal 90.0
-- @param BigDecimal 91.0
-- @param BigDecimal 91.0
-- @param BigDecimal 92.0
-- @param BigDecimal 92.0
-- @param BigDecimal 93.0
-- @param BigDecimal 93.0
-- @param BigDecimal 94.0
-- @param BigDecimal 94.0
-- @param BigDecimal 95.0
-- @param BigDecimal 95.0
-- @param BigDecimal 96.0
-- @param BigDecimal 96.0
-- @param BigDecimal 97.0
-- @param BigDecimal 97.0
-- @param BigDecimal 98.0
-- @param BigDecimal 98.0
-- @param BigDecimal 99.0
-- @param BigDecimal 99.0
-- @param BigDecimal 100.0
-- @param BigDecimal 100.0
-- @param BigDecimal 101.0
-- @param BigDecimal 101.0
-- @param BigDecimal 102.0
-- @param BigDecimal 102.0
-- @param BigDecimal 103.0
-- @param BigDecimal 103.0
-- @param BigDecimal 104.0
-- @param BigDecimal 104.0
-- @param BigDecimal 105.0
-- @param BigDecimal 105.0
-- @param BigDecimal 106.0
-- @param BigDecimal 106.0
-- @param BigDecimal 107.0
-- @param BigDecimal 107.0
-- @param BigDecimal 108.0
-- @param BigDecimal 108.0
-- @param BigDecimal 109.0
-- @param BigDecimal 109.0
-- @param BigDecimal 110.0
-- @param BigDecimal 110.0
-- @param BigDecimal 111.0
-- @param BigDecimal 111.0
-- @param BigDecimal 112.0
-- @param BigDecimal 112.0
-- @param BigDecimal 113.0
-- @param BigDecimal 113.0
-- @param BigDecimal 114.0
-- @param BigDecimal 114.0
-- @param BigDecimal 115.0
-- @param BigDecimal 115.0
-- @param BigDecimal 116.0
-- @param BigDecimal 116.0
-- @param BigDecimal 117.0
-- @param BigDecimal 117.0
-- @param BigDecimal 118.0
-- @param BigDecimal 118.0
-- @param BigDecimal 119.0
-- @param BigDecimal 119.0
-- @param BigDecimal 120.0
-- @param BigDecimal 120.0
-- @param BigDecimal 121.0
-- @param BigDecimal 121.0
-- @param BigDecimal 122.0
-- @param BigDecimal 122.0
-- @param BigDecimal 123.0
-- @param BigDecimal 123.0
-- @param BigDecimal 124.0
-- @param BigDecimal 124.0
-- @param BigDecimal 125.0
-- @param BigDecimal 125.0
-- @param BigDecimal 126.0
-- @param BigDecimal 126.0
-- @param BigDecimal 127.0
-- @param BigDecimal 127.0
-- @param BigDecimal 128.0
-- @param BigDecimal 128.0
-- @param BigDecimal 129.0
-- @param BigDecimal 129.0
-- @param BigDecimal 130.0
-- @param BigDecimal 130.0
-- @param BigDecimal 131.0
-- @param BigDecimal 131.0
-- @param BigDecimal 132.0
-- @param BigDecimal 132.0
-- @param BigDecimal 133.0
-- @param BigDecimal 133.0
-- @param BigDecimal 134.0
-- @param BigDecimal 134.0
-- @param BigDecimal 135.0
-- @param BigDecimal 135.0
-- @param BigDecimal 136.0
-- @param BigDecimal 136.0
-- @param BigDecimal 137.0
-- @param BigDecimal 137.0
-- @param BigDecimal 138.0
-- @param BigDecimal 138.0
-- @param BigDecimal 139.0
-- @param BigDecimal 139.0
-- @param BigDecimal 140.0
-- @param BigDecimal 140.0
-- @param BigDecimal 141.0
-- @param BigDecimal 141.0
-- @param BigDecimal 142.0
-- @param BigDecimal 142.0
-- @param BigDecimal 143.0
-- @param BigDecimal 143.0
-- @param BigDecimal 144.0
-- @param BigDecimal 144.0
-- @param BigDecimal 145.0
-- @param BigDecimal 145.0
-- @param BigDecimal 146.0
-- @param BigDecimal 146.0
-- @param BigDecimal 147.0
-- @param BigDecimal 147.0
-- @param BigDecimal 148.0
-- @param BigDecimal 148.0
-- @param BigDecimal 149.0
-- @param BigDecimal 149.0
-- @param BigDecimal 150.0
-- @param BigDecimal 150.0
-- @param BigDecimal 151.0
-- @param BigDecimal 151.0
-- @param BigDecimal 152.0
-- @param BigDecimal 152.0
-- @param BigDecimal 153.0
-- @param BigDecimal 153.0
-- @param BigDecimal 154.0
-- @param BigDecimal 154.0
-- @param BigDecimal 155.0
-- @param BigDecimal 155.0
-- @param BigDecimal 156.0
-- @param BigDecimal 156.0
-- @param BigDecimal 157.0
-- @param BigDecimal 157.0
-- @param BigDecimal 158.0
-- @param BigDecimal 158.0
-- @param BigDecimal 159.0
-- @param BigDecimal 159.0
-- @param BigDecimal 160.0
-- @param BigDecimal 160.0
-- @param BigDecimal 161.0
-- @param BigDecimal 161.0
-- @param BigDecimal 162.0
-- @param BigDecimal 162.0
-- @param BigDecimal 163.0
-- @param BigDecimal 163.0
-- @param BigDecimal 164.0
-- @param BigDecimal 164.0
-- @param BigDecimal 165.0
-- @param BigDecimal 165.0
-- @param BigDecimal 166.0
-- @param BigDecimal 166.0
-- @param BigDecimal 167.0
-- @param BigDecimal 167.0
-- @param BigDecimal 168.0
-- @param BigDecimal 168.0
-- @param BigDecimal 169.0
-- @param BigDecimal 169.0
-- @param BigDecimal 170.0
-- @param BigDecimal 170.0
-- @param BigDecimal 171.0
-- @param BigDecimal 171.0
-- @param BigDecimal 172.0
-- @param BigDecimal 172.0
-- @param BigDecimal 173.0
-- @param BigDecimal 173.0
-- @param BigDecimal 174.0
-- @param BigDecimal 174.0
-- @param BigDecimal 175.0
-- @param BigDecimal 175.0
-- @param BigDecimal 176.0
-- @param BigDecimal 176.0
-- @param BigDecimal 177.0
-- @param BigDecimal 177.0
-- @param BigDecimal 178.0
-- @param BigDecimal 178.0
-- @param BigDecimal 179.0
-- @param BigDecimal 179.0
-- @param BigDecimal 180.0
-- @param BigDecimal 180.0
-- @param BigDecimal 181.0
-- @param BigDecimal 181.0
-- @param BigDecimal 182.0
-- @param BigDecimal 182.0
-- @param BigDecimal 183.0
-- @param BigDecimal 183.0
-- @param BigDecimal 184.0
-- @param BigDecimal 184.0
-- @param BigDecimal 185.0
-- @param BigDecimal 185.0
-- @param BigDecimal 186.0
-- @param BigDecimal 186.0
-- @param BigDecimal 187.0
-- @param BigDecimal 187.0
-- @param BigDecimal 188.0
-- @param BigDecimal 188.0
-- @param BigDecimal 189.0
-- @param BigDecimal 189.0
-- @param BigDecimal 190.0
-- @param BigDecimal 190.0
-- @param BigDecimal 191.0
-- @param BigDecimal 191.0
-- @param BigDecimal 192.0
-- @param BigDecimal 192.0
-- @param BigDecimal 193.0
-- @param BigDecimal 193.0
-- @param BigDecimal 194.0
-- @param BigDecimal 194.0
-- @param BigDecimal 195.0
-- @param BigDecimal 195.0
-- @param BigDecimal 196.0
-- @param BigDecimal 196.0
-- @param BigDecimal 197.0
-- @param BigDecimal 197.0
-- @param BigDecimal 198.0
-- @param BigDecimal 198.0
-- @param BigDecimal 199.0
-- @param BigDecimal 199.0
-- @param BigDecimal 200.0
-- @param BigDecimal 200.0
INSERT INTO idx_f4(v_det,v_ope) VALUES (?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?);

CREATE INDEX idx_f4_hash ON idx_f4 USING hash(v_det);

CREATE INDEX idx_f4_btree ON idx_f4(v_ope);

SET enable_seqscan = off;

SET enable_seqscan = on;

-- ===== 14.5 float8_enc =====
DROP TABLE IF EXISTS idx_f8;

CREATE TABLE idx_f8(id serial PRIMARY KEY, v_det float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

-- @param BigDecimal 1.0
-- @param BigDecimal 1.0
-- @param BigDecimal 2.0
-- @param BigDecimal 2.0
-- @param BigDecimal 3.0
-- @param BigDecimal 3.0
-- @param BigDecimal 4.0
-- @param BigDecimal 4.0
-- @param BigDecimal 5.0
-- @param BigDecimal 5.0
-- @param BigDecimal 6.0
-- @param BigDecimal 6.0
-- @param BigDecimal 7.0
-- @param BigDecimal 7.0
-- @param BigDecimal 8.0
-- @param BigDecimal 8.0
-- @param BigDecimal 9.0
-- @param BigDecimal 9.0
-- @param BigDecimal 10.0
-- @param BigDecimal 10.0
-- @param BigDecimal 11.0
-- @param BigDecimal 11.0
-- @param BigDecimal 12.0
-- @param BigDecimal 12.0
-- @param BigDecimal 13.0
-- @param BigDecimal 13.0
-- @param BigDecimal 14.0
-- @param BigDecimal 14.0
-- @param BigDecimal 15.0
-- @param BigDecimal 15.0
-- @param BigDecimal 16.0
-- @param BigDecimal 16.0
-- @param BigDecimal 17.0
-- @param BigDecimal 17.0
-- @param BigDecimal 18.0
-- @param BigDecimal 18.0
-- @param BigDecimal 19.0
-- @param BigDecimal 19.0
-- @param BigDecimal 20.0
-- @param BigDecimal 20.0
-- @param BigDecimal 21.0
-- @param BigDecimal 21.0
-- @param BigDecimal 22.0
-- @param BigDecimal 22.0
-- @param BigDecimal 23.0
-- @param BigDecimal 23.0
-- @param BigDecimal 24.0
-- @param BigDecimal 24.0
-- @param BigDecimal 25.0
-- @param BigDecimal 25.0
-- @param BigDecimal 26.0
-- @param BigDecimal 26.0
-- @param BigDecimal 27.0
-- @param BigDecimal 27.0
-- @param BigDecimal 28.0
-- @param BigDecimal 28.0
-- @param BigDecimal 29.0
-- @param BigDecimal 29.0
-- @param BigDecimal 30.0
-- @param BigDecimal 30.0
-- @param BigDecimal 31.0
-- @param BigDecimal 31.0
-- @param BigDecimal 32.0
-- @param BigDecimal 32.0
-- @param BigDecimal 33.0
-- @param BigDecimal 33.0
-- @param BigDecimal 34.0
-- @param BigDecimal 34.0
-- @param BigDecimal 35.0
-- @param BigDecimal 35.0
-- @param BigDecimal 36.0
-- @param BigDecimal 36.0
-- @param BigDecimal 37.0
-- @param BigDecimal 37.0
-- @param BigDecimal 38.0
-- @param BigDecimal 38.0
-- @param BigDecimal 39.0
-- @param BigDecimal 39.0
-- @param BigDecimal 40.0
-- @param BigDecimal 40.0
-- @param BigDecimal 41.0
-- @param BigDecimal 41.0
-- @param BigDecimal 42.0
-- @param BigDecimal 42.0
-- @param BigDecimal 43.0
-- @param BigDecimal 43.0
-- @param BigDecimal 44.0
-- @param BigDecimal 44.0
-- @param BigDecimal 45.0
-- @param BigDecimal 45.0
-- @param BigDecimal 46.0
-- @param BigDecimal 46.0
-- @param BigDecimal 47.0
-- @param BigDecimal 47.0
-- @param BigDecimal 48.0
-- @param BigDecimal 48.0
-- @param BigDecimal 49.0
-- @param BigDecimal 49.0
-- @param BigDecimal 50.0
-- @param BigDecimal 50.0
INSERT INTO idx_f8(v_det,v_ope) VALUES (?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?);

-- @param BigDecimal 51.0
-- @param BigDecimal 51.0
-- @param BigDecimal 52.0
-- @param BigDecimal 52.0
-- @param BigDecimal 53.0
-- @param BigDecimal 53.0
-- @param BigDecimal 54.0
-- @param BigDecimal 54.0
-- @param BigDecimal 55.0
-- @param BigDecimal 55.0
-- @param BigDecimal 56.0
-- @param BigDecimal 56.0
-- @param BigDecimal 57.0
-- @param BigDecimal 57.0
-- @param BigDecimal 58.0
-- @param BigDecimal 58.0
-- @param BigDecimal 59.0
-- @param BigDecimal 59.0
-- @param BigDecimal 60.0
-- @param BigDecimal 60.0
-- @param BigDecimal 61.0
-- @param BigDecimal 61.0
-- @param BigDecimal 62.0
-- @param BigDecimal 62.0
-- @param BigDecimal 63.0
-- @param BigDecimal 63.0
-- @param BigDecimal 64.0
-- @param BigDecimal 64.0
-- @param BigDecimal 65.0
-- @param BigDecimal 65.0
-- @param BigDecimal 66.0
-- @param BigDecimal 66.0
-- @param BigDecimal 67.0
-- @param BigDecimal 67.0
-- @param BigDecimal 68.0
-- @param BigDecimal 68.0
-- @param BigDecimal 69.0
-- @param BigDecimal 69.0
-- @param BigDecimal 70.0
-- @param BigDecimal 70.0
-- @param BigDecimal 71.0
-- @param BigDecimal 71.0
-- @param BigDecimal 72.0
-- @param BigDecimal 72.0
-- @param BigDecimal 73.0
-- @param BigDecimal 73.0
-- @param BigDecimal 74.0
-- @param BigDecimal 74.0
-- @param BigDecimal 75.0
-- @param BigDecimal 75.0
-- @param BigDecimal 76.0
-- @param BigDecimal 76.0
-- @param BigDecimal 77.0
-- @param BigDecimal 77.0
-- @param BigDecimal 78.0
-- @param BigDecimal 78.0
-- @param BigDecimal 79.0
-- @param BigDecimal 79.0
-- @param BigDecimal 80.0
-- @param BigDecimal 80.0
-- @param BigDecimal 81.0
-- @param BigDecimal 81.0
-- @param BigDecimal 82.0
-- @param BigDecimal 82.0
-- @param BigDecimal 83.0
-- @param BigDecimal 83.0
-- @param BigDecimal 84.0
-- @param BigDecimal 84.0
-- @param BigDecimal 85.0
-- @param BigDecimal 85.0
-- @param BigDecimal 86.0
-- @param BigDecimal 86.0
-- @param BigDecimal 87.0
-- @param BigDecimal 87.0
-- @param BigDecimal 88.0
-- @param BigDecimal 88.0
-- @param BigDecimal 89.0
-- @param BigDecimal 89.0
-- @param BigDecimal 90.0
-- @param BigDecimal 90.0
-- @param BigDecimal 91.0
-- @param BigDecimal 91.0
-- @param BigDecimal 92.0
-- @param BigDecimal 92.0
-- @param BigDecimal 93.0
-- @param BigDecimal 93.0
-- @param BigDecimal 94.0
-- @param BigDecimal 94.0
-- @param BigDecimal 95.0
-- @param BigDecimal 95.0
-- @param BigDecimal 96.0
-- @param BigDecimal 96.0
-- @param BigDecimal 97.0
-- @param BigDecimal 97.0
-- @param BigDecimal 98.0
-- @param BigDecimal 98.0
-- @param BigDecimal 99.0
-- @param BigDecimal 99.0
-- @param BigDecimal 100.0
-- @param BigDecimal 100.0
-- @param BigDecimal 101.0
-- @param BigDecimal 101.0
-- @param BigDecimal 102.0
-- @param BigDecimal 102.0
-- @param BigDecimal 103.0
-- @param BigDecimal 103.0
-- @param BigDecimal 104.0
-- @param BigDecimal 104.0
-- @param BigDecimal 105.0
-- @param BigDecimal 105.0
-- @param BigDecimal 106.0
-- @param BigDecimal 106.0
-- @param BigDecimal 107.0
-- @param BigDecimal 107.0
-- @param BigDecimal 108.0
-- @param BigDecimal 108.0
-- @param BigDecimal 109.0
-- @param BigDecimal 109.0
-- @param BigDecimal 110.0
-- @param BigDecimal 110.0
-- @param BigDecimal 111.0
-- @param BigDecimal 111.0
-- @param BigDecimal 112.0
-- @param BigDecimal 112.0
-- @param BigDecimal 113.0
-- @param BigDecimal 113.0
-- @param BigDecimal 114.0
-- @param BigDecimal 114.0
-- @param BigDecimal 115.0
-- @param BigDecimal 115.0
-- @param BigDecimal 116.0
-- @param BigDecimal 116.0
-- @param BigDecimal 117.0
-- @param BigDecimal 117.0
-- @param BigDecimal 118.0
-- @param BigDecimal 118.0
-- @param BigDecimal 119.0
-- @param BigDecimal 119.0
-- @param BigDecimal 120.0
-- @param BigDecimal 120.0
-- @param BigDecimal 121.0
-- @param BigDecimal 121.0
-- @param BigDecimal 122.0
-- @param BigDecimal 122.0
-- @param BigDecimal 123.0
-- @param BigDecimal 123.0
-- @param BigDecimal 124.0
-- @param BigDecimal 124.0
-- @param BigDecimal 125.0
-- @param BigDecimal 125.0
-- @param BigDecimal 126.0
-- @param BigDecimal 126.0
-- @param BigDecimal 127.0
-- @param BigDecimal 127.0
-- @param BigDecimal 128.0
-- @param BigDecimal 128.0
-- @param BigDecimal 129.0
-- @param BigDecimal 129.0
-- @param BigDecimal 130.0
-- @param BigDecimal 130.0
-- @param BigDecimal 131.0
-- @param BigDecimal 131.0
-- @param BigDecimal 132.0
-- @param BigDecimal 132.0
-- @param BigDecimal 133.0
-- @param BigDecimal 133.0
-- @param BigDecimal 134.0
-- @param BigDecimal 134.0
-- @param BigDecimal 135.0
-- @param BigDecimal 135.0
-- @param BigDecimal 136.0
-- @param BigDecimal 136.0
-- @param BigDecimal 137.0
-- @param BigDecimal 137.0
-- @param BigDecimal 138.0
-- @param BigDecimal 138.0
-- @param BigDecimal 139.0
-- @param BigDecimal 139.0
-- @param BigDecimal 140.0
-- @param BigDecimal 140.0
-- @param BigDecimal 141.0
-- @param BigDecimal 141.0
-- @param BigDecimal 142.0
-- @param BigDecimal 142.0
-- @param BigDecimal 143.0
-- @param BigDecimal 143.0
-- @param BigDecimal 144.0
-- @param BigDecimal 144.0
-- @param BigDecimal 145.0
-- @param BigDecimal 145.0
-- @param BigDecimal 146.0
-- @param BigDecimal 146.0
-- @param BigDecimal 147.0
-- @param BigDecimal 147.0
-- @param BigDecimal 148.0
-- @param BigDecimal 148.0
-- @param BigDecimal 149.0
-- @param BigDecimal 149.0
-- @param BigDecimal 150.0
-- @param BigDecimal 150.0
-- @param BigDecimal 151.0
-- @param BigDecimal 151.0
-- @param BigDecimal 152.0
-- @param BigDecimal 152.0
-- @param BigDecimal 153.0
-- @param BigDecimal 153.0
-- @param BigDecimal 154.0
-- @param BigDecimal 154.0
-- @param BigDecimal 155.0
-- @param BigDecimal 155.0
-- @param BigDecimal 156.0
-- @param BigDecimal 156.0
-- @param BigDecimal 157.0
-- @param BigDecimal 157.0
-- @param BigDecimal 158.0
-- @param BigDecimal 158.0
-- @param BigDecimal 159.0
-- @param BigDecimal 159.0
-- @param BigDecimal 160.0
-- @param BigDecimal 160.0
-- @param BigDecimal 161.0
-- @param BigDecimal 161.0
-- @param BigDecimal 162.0
-- @param BigDecimal 162.0
-- @param BigDecimal 163.0
-- @param BigDecimal 163.0
-- @param BigDecimal 164.0
-- @param BigDecimal 164.0
-- @param BigDecimal 165.0
-- @param BigDecimal 165.0
-- @param BigDecimal 166.0
-- @param BigDecimal 166.0
-- @param BigDecimal 167.0
-- @param BigDecimal 167.0
-- @param BigDecimal 168.0
-- @param BigDecimal 168.0
-- @param BigDecimal 169.0
-- @param BigDecimal 169.0
-- @param BigDecimal 170.0
-- @param BigDecimal 170.0
-- @param BigDecimal 171.0
-- @param BigDecimal 171.0
-- @param BigDecimal 172.0
-- @param BigDecimal 172.0
-- @param BigDecimal 173.0
-- @param BigDecimal 173.0
-- @param BigDecimal 174.0
-- @param BigDecimal 174.0
-- @param BigDecimal 175.0
-- @param BigDecimal 175.0
-- @param BigDecimal 176.0
-- @param BigDecimal 176.0
-- @param BigDecimal 177.0
-- @param BigDecimal 177.0
-- @param BigDecimal 178.0
-- @param BigDecimal 178.0
-- @param BigDecimal 179.0
-- @param BigDecimal 179.0
-- @param BigDecimal 180.0
-- @param BigDecimal 180.0
-- @param BigDecimal 181.0
-- @param BigDecimal 181.0
-- @param BigDecimal 182.0
-- @param BigDecimal 182.0
-- @param BigDecimal 183.0
-- @param BigDecimal 183.0
-- @param BigDecimal 184.0
-- @param BigDecimal 184.0
-- @param BigDecimal 185.0
-- @param BigDecimal 185.0
-- @param BigDecimal 186.0
-- @param BigDecimal 186.0
-- @param BigDecimal 187.0
-- @param BigDecimal 187.0
-- @param BigDecimal 188.0
-- @param BigDecimal 188.0
-- @param BigDecimal 189.0
-- @param BigDecimal 189.0
-- @param BigDecimal 190.0
-- @param BigDecimal 190.0
-- @param BigDecimal 191.0
-- @param BigDecimal 191.0
-- @param BigDecimal 192.0
-- @param BigDecimal 192.0
-- @param BigDecimal 193.0
-- @param BigDecimal 193.0
-- @param BigDecimal 194.0
-- @param BigDecimal 194.0
-- @param BigDecimal 195.0
-- @param BigDecimal 195.0
-- @param BigDecimal 196.0
-- @param BigDecimal 196.0
-- @param BigDecimal 197.0
-- @param BigDecimal 197.0
-- @param BigDecimal 198.0
-- @param BigDecimal 198.0
-- @param BigDecimal 199.0
-- @param BigDecimal 199.0
-- @param BigDecimal 200.0
-- @param BigDecimal 200.0
INSERT INTO idx_f8(v_det,v_ope) VALUES (?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?);

CREATE INDEX idx_f8_hash ON idx_f8 USING hash(v_det);

CREATE INDEX idx_f8_btree ON idx_f8(v_ope);

SET enable_seqscan = off;

SET enable_seqscan = on;

-- ===== 14.6 numeric_enc =====
DROP TABLE IF EXISTS idx_nm;

CREATE TABLE idx_nm(id serial PRIMARY KEY, v_det numeric_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope numeric_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

-- @param int 1
-- @param int 1
-- @param int 2
-- @param int 2
-- @param int 3
-- @param int 3
-- @param int 4
-- @param int 4
-- @param int 5
-- @param int 5
-- @param int 6
-- @param int 6
-- @param int 7
-- @param int 7
-- @param int 8
-- @param int 8
-- @param int 9
-- @param int 9
-- @param int 10
-- @param int 10
-- @param int 11
-- @param int 11
-- @param int 12
-- @param int 12
-- @param int 13
-- @param int 13
-- @param int 14
-- @param int 14
-- @param int 15
-- @param int 15
-- @param int 16
-- @param int 16
-- @param int 17
-- @param int 17
-- @param int 18
-- @param int 18
-- @param int 19
-- @param int 19
-- @param int 20
-- @param int 20
-- @param int 21
-- @param int 21
-- @param int 22
-- @param int 22
-- @param int 23
-- @param int 23
-- @param int 24
-- @param int 24
-- @param int 25
-- @param int 25
-- @param int 26
-- @param int 26
-- @param int 27
-- @param int 27
-- @param int 28
-- @param int 28
-- @param int 29
-- @param int 29
-- @param int 30
-- @param int 30
-- @param int 31
-- @param int 31
-- @param int 32
-- @param int 32
-- @param int 33
-- @param int 33
-- @param int 34
-- @param int 34
-- @param int 35
-- @param int 35
-- @param int 36
-- @param int 36
-- @param int 37
-- @param int 37
-- @param int 38
-- @param int 38
-- @param int 39
-- @param int 39
-- @param int 40
-- @param int 40
-- @param int 41
-- @param int 41
-- @param int 42
-- @param int 42
-- @param int 43
-- @param int 43
-- @param int 44
-- @param int 44
-- @param int 45
-- @param int 45
-- @param int 46
-- @param int 46
-- @param int 47
-- @param int 47
-- @param int 48
-- @param int 48
-- @param int 49
-- @param int 49
-- @param int 50
-- @param int 50
INSERT INTO idx_nm(v_det,v_ope) VALUES (?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?);

-- @param int 51
-- @param int 51
-- @param int 52
-- @param int 52
-- @param int 53
-- @param int 53
-- @param int 54
-- @param int 54
-- @param int 55
-- @param int 55
-- @param int 56
-- @param int 56
-- @param int 57
-- @param int 57
-- @param int 58
-- @param int 58
-- @param int 59
-- @param int 59
-- @param int 60
-- @param int 60
-- @param int 61
-- @param int 61
-- @param int 62
-- @param int 62
-- @param int 63
-- @param int 63
-- @param int 64
-- @param int 64
-- @param int 65
-- @param int 65
-- @param int 66
-- @param int 66
-- @param int 67
-- @param int 67
-- @param int 68
-- @param int 68
-- @param int 69
-- @param int 69
-- @param int 70
-- @param int 70
-- @param int 71
-- @param int 71
-- @param int 72
-- @param int 72
-- @param int 73
-- @param int 73
-- @param int 74
-- @param int 74
-- @param int 75
-- @param int 75
-- @param int 76
-- @param int 76
-- @param int 77
-- @param int 77
-- @param int 78
-- @param int 78
-- @param int 79
-- @param int 79
-- @param int 80
-- @param int 80
-- @param int 81
-- @param int 81
-- @param int 82
-- @param int 82
-- @param int 83
-- @param int 83
-- @param int 84
-- @param int 84
-- @param int 85
-- @param int 85
-- @param int 86
-- @param int 86
-- @param int 87
-- @param int 87
-- @param int 88
-- @param int 88
-- @param int 89
-- @param int 89
-- @param int 90
-- @param int 90
-- @param int 91
-- @param int 91
-- @param int 92
-- @param int 92
-- @param int 93
-- @param int 93
-- @param int 94
-- @param int 94
-- @param int 95
-- @param int 95
-- @param int 96
-- @param int 96
-- @param int 97
-- @param int 97
-- @param int 98
-- @param int 98
-- @param int 99
-- @param int 99
-- @param int 100
-- @param int 100
-- @param int 101
-- @param int 101
-- @param int 102
-- @param int 102
-- @param int 103
-- @param int 103
-- @param int 104
-- @param int 104
-- @param int 105
-- @param int 105
-- @param int 106
-- @param int 106
-- @param int 107
-- @param int 107
-- @param int 108
-- @param int 108
-- @param int 109
-- @param int 109
-- @param int 110
-- @param int 110
-- @param int 111
-- @param int 111
-- @param int 112
-- @param int 112
-- @param int 113
-- @param int 113
-- @param int 114
-- @param int 114
-- @param int 115
-- @param int 115
-- @param int 116
-- @param int 116
-- @param int 117
-- @param int 117
-- @param int 118
-- @param int 118
-- @param int 119
-- @param int 119
-- @param int 120
-- @param int 120
-- @param int 121
-- @param int 121
-- @param int 122
-- @param int 122
-- @param int 123
-- @param int 123
-- @param int 124
-- @param int 124
-- @param int 125
-- @param int 125
-- @param int 126
-- @param int 126
-- @param int 127
-- @param int 127
-- @param int 128
-- @param int 128
-- @param int 129
-- @param int 129
-- @param int 130
-- @param int 130
-- @param int 131
-- @param int 131
-- @param int 132
-- @param int 132
-- @param int 133
-- @param int 133
-- @param int 134
-- @param int 134
-- @param int 135
-- @param int 135
-- @param int 136
-- @param int 136
-- @param int 137
-- @param int 137
-- @param int 138
-- @param int 138
-- @param int 139
-- @param int 139
-- @param int 140
-- @param int 140
-- @param int 141
-- @param int 141
-- @param int 142
-- @param int 142
-- @param int 143
-- @param int 143
-- @param int 144
-- @param int 144
-- @param int 145
-- @param int 145
-- @param int 146
-- @param int 146
-- @param int 147
-- @param int 147
-- @param int 148
-- @param int 148
-- @param int 149
-- @param int 149
-- @param int 150
-- @param int 150
-- @param int 151
-- @param int 151
-- @param int 152
-- @param int 152
-- @param int 153
-- @param int 153
-- @param int 154
-- @param int 154
-- @param int 155
-- @param int 155
-- @param int 156
-- @param int 156
-- @param int 157
-- @param int 157
-- @param int 158
-- @param int 158
-- @param int 159
-- @param int 159
-- @param int 160
-- @param int 160
-- @param int 161
-- @param int 161
-- @param int 162
-- @param int 162
-- @param int 163
-- @param int 163
-- @param int 164
-- @param int 164
-- @param int 165
-- @param int 165
-- @param int 166
-- @param int 166
-- @param int 167
-- @param int 167
-- @param int 168
-- @param int 168
-- @param int 169
-- @param int 169
-- @param int 170
-- @param int 170
-- @param int 171
-- @param int 171
-- @param int 172
-- @param int 172
-- @param int 173
-- @param int 173
-- @param int 174
-- @param int 174
-- @param int 175
-- @param int 175
-- @param int 176
-- @param int 176
-- @param int 177
-- @param int 177
-- @param int 178
-- @param int 178
-- @param int 179
-- @param int 179
-- @param int 180
-- @param int 180
-- @param int 181
-- @param int 181
-- @param int 182
-- @param int 182
-- @param int 183
-- @param int 183
-- @param int 184
-- @param int 184
-- @param int 185
-- @param int 185
-- @param int 186
-- @param int 186
-- @param int 187
-- @param int 187
-- @param int 188
-- @param int 188
-- @param int 189
-- @param int 189
-- @param int 190
-- @param int 190
-- @param int 191
-- @param int 191
-- @param int 192
-- @param int 192
-- @param int 193
-- @param int 193
-- @param int 194
-- @param int 194
-- @param int 195
-- @param int 195
-- @param int 196
-- @param int 196
-- @param int 197
-- @param int 197
-- @param int 198
-- @param int 198
-- @param int 199
-- @param int 199
-- @param int 200
-- @param int 200
INSERT INTO idx_nm(v_det,v_ope) VALUES (?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?);

CREATE INDEX idx_nm_hash ON idx_nm USING hash(v_det);

CREATE INDEX idx_nm_btree ON idx_nm(v_ope);

SET enable_seqscan = off;

SET enable_seqscan = on;

-- ===== 14.7 text_enc =====
DROP TABLE IF EXISTS idx_tx;

CREATE TABLE idx_tx(id serial PRIMARY KEY, v_det text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope text_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

-- @param String a
-- @param String a
-- @param String b
-- @param String b
-- @param String c
-- @param String c
-- @param String d
-- @param String d
-- @param String e
-- @param String e
-- @param String f
-- @param String f
-- @param String g
-- @param String g
-- @param String h
-- @param String h
-- @param String i
-- @param String i
-- @param String j
-- @param String j
-- @param String k
-- @param String k
-- @param String l
-- @param String l
-- @param String m
-- @param String m
-- @param String n
-- @param String n
-- @param String o
-- @param String o
-- @param String p
-- @param String p
-- @param String q
-- @param String q
-- @param String r
-- @param String r
-- @param String s
-- @param String s
-- @param String t
-- @param String t
-- @param String u
-- @param String u
-- @param String v
-- @param String v
-- @param String w
-- @param String w
-- @param String x
-- @param String x
-- @param String y
-- @param String y
-- @param String z
-- @param String z
-- @param String aa
-- @param String aa
-- @param String bb
-- @param String bb
-- @param String cc
-- @param String cc
-- @param String dd
-- @param String dd
-- @param String ee
-- @param String ee
-- @param String ff
-- @param String ff
-- @param String gg
-- @param String gg
-- @param String hh
-- @param String hh
-- @param String ii
-- @param String ii
-- @param String jj
-- @param String jj
-- @param String kk
-- @param String kk
-- @param String ll
-- @param String ll
-- @param String mm
-- @param String mm
-- @param String nn
-- @param String nn
-- @param String oo
-- @param String oo
-- @param String pp
-- @param String pp
-- @param String qq
-- @param String qq
-- @param String rr
-- @param String rr
-- @param String ss
-- @param String ss
-- @param String tt
-- @param String tt
-- @param String uu
-- @param String uu
-- @param String vv
-- @param String vv
-- @param String ww
-- @param String ww
-- @param String xx
-- @param String xx
INSERT INTO idx_tx(v_det,v_ope) VALUES (?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?);

-- @param String yy
-- @param String yy
-- @param String zz
-- @param String zz
-- @param String aaa
-- @param String aaa
-- @param String bbb
-- @param String bbb
-- @param String ccc
-- @param String ccc
-- @param String ddd
-- @param String ddd
-- @param String eee
-- @param String eee
-- @param String fff
-- @param String fff
-- @param String ggg
-- @param String ggg
-- @param String hhh
-- @param String hhh
-- @param String iii
-- @param String iii
-- @param String jjj
-- @param String jjj
-- @param String kkk
-- @param String kkk
-- @param String lll
-- @param String lll
-- @param String mmm
-- @param String mmm
-- @param String nnn
-- @param String nnn
-- @param String ooo
-- @param String ooo
-- @param String ppp
-- @param String ppp
-- @param String qqq
-- @param String qqq
-- @param String rrr
-- @param String rrr
-- @param String sss
-- @param String sss
-- @param String ttt
-- @param String ttt
-- @param String uuu
-- @param String uuu
-- @param String vvv
-- @param String vvv
-- @param String www
-- @param String www
-- @param String xxx
-- @param String xxx
-- @param String yyy
-- @param String yyy
-- @param String zzz
-- @param String zzz
INSERT INTO idx_tx(v_det,v_ope) VALUES (?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?);

CREATE INDEX idx_tx_hash ON idx_tx USING hash(v_det);

CREATE INDEX idx_tx_btree ON idx_tx(v_ope);

-- ===== 14.8 varchar_enc =====
DROP TABLE IF EXISTS idx_vc;

CREATE TABLE idx_vc(id serial PRIMARY KEY, v_det varchar_enc(50) ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope varchar_enc(50) ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

-- @param String a
-- @param String a
-- @param String b
-- @param String b
-- @param String c
-- @param String c
-- @param String d
-- @param String d
-- @param String e
-- @param String e
-- @param String f
-- @param String f
-- @param String g
-- @param String g
-- @param String h
-- @param String h
-- @param String i
-- @param String i
-- @param String j
-- @param String j
-- @param String k
-- @param String k
-- @param String l
-- @param String l
-- @param String m
-- @param String m
-- @param String n
-- @param String n
-- @param String o
-- @param String o
-- @param String p
-- @param String p
-- @param String q
-- @param String q
-- @param String r
-- @param String r
-- @param String s
-- @param String s
-- @param String t
-- @param String t
-- @param String u
-- @param String u
-- @param String v
-- @param String v
-- @param String w
-- @param String w
-- @param String x
-- @param String x
-- @param String y
-- @param String y
-- @param String z
-- @param String z
-- @param String aa
-- @param String aa
-- @param String bb
-- @param String bb
-- @param String cc
-- @param String cc
-- @param String dd
-- @param String dd
-- @param String ee
-- @param String ee
-- @param String ff
-- @param String ff
-- @param String gg
-- @param String gg
-- @param String hh
-- @param String hh
-- @param String ii
-- @param String ii
-- @param String jj
-- @param String jj
-- @param String kk
-- @param String kk
-- @param String ll
-- @param String ll
-- @param String mm
-- @param String mm
-- @param String nn
-- @param String nn
-- @param String oo
-- @param String oo
-- @param String pp
-- @param String pp
-- @param String qq
-- @param String qq
-- @param String rr
-- @param String rr
-- @param String ss
-- @param String ss
-- @param String tt
-- @param String tt
INSERT INTO idx_vc(v_det,v_ope) VALUES (?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?);

CREATE INDEX idx_vc_hash ON idx_vc USING hash(v_det);

CREATE INDEX idx_vc_btree ON idx_vc(v_ope);

-- ===== 14.9 bpchar_enc =====
DROP TABLE IF EXISTS idx_bp;

CREATE TABLE idx_bp(id serial PRIMARY KEY, v_det bpchar_enc(20) ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope bpchar_enc(20) ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

-- @param String a
-- @param String a
-- @param String b
-- @param String b
-- @param String c
-- @param String c
-- @param String d
-- @param String d
-- @param String e
-- @param String e
-- @param String f
-- @param String f
-- @param String g
-- @param String g
-- @param String h
-- @param String h
-- @param String i
-- @param String i
-- @param String j
-- @param String j
-- @param String k
-- @param String k
-- @param String l
-- @param String l
-- @param String m
-- @param String m
-- @param String n
-- @param String n
-- @param String o
-- @param String o
-- @param String p
-- @param String p
-- @param String q
-- @param String q
-- @param String r
-- @param String r
-- @param String s
-- @param String s
-- @param String t
-- @param String t
-- @param String u
-- @param String u
-- @param String v
-- @param String v
-- @param String w
-- @param String w
-- @param String x
-- @param String x
-- @param String y
-- @param String y
-- @param String z
-- @param String z
-- @param String aa
-- @param String aa
-- @param String bb
-- @param String bb
-- @param String cc
-- @param String cc
-- @param String dd
-- @param String dd
-- @param String ee
-- @param String ee
-- @param String ff
-- @param String ff
-- @param String gg
-- @param String gg
-- @param String hh
-- @param String hh
-- @param String ii
-- @param String ii
-- @param String jj
-- @param String jj
-- @param String kk
-- @param String kk
-- @param String ll
-- @param String ll
-- @param String mm
-- @param String mm
-- @param String nn
-- @param String nn
-- @param String oo
-- @param String oo
-- @param String pp
-- @param String pp
-- @param String qq
-- @param String qq
-- @param String rr
-- @param String rr
-- @param String ss
-- @param String ss
-- @param String tt
-- @param String tt
INSERT INTO idx_bp(v_det,v_ope) VALUES (?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?),(?,?);

CREATE INDEX idx_bp_hash ON idx_bp USING hash(v_det);

CREATE INDEX idx_bp_btree ON idx_bp(v_ope);

-- ============================================================================
-- 第十五部分: 语法/Boundary/多列/NOTLIKE/子查询补充
-- ============================================================================

-- ===== 15.1 语法覆盖: IN/NOT IN/DISTINCT/LIMIT+OFFSET/NULLS =====
DROP TABLE IF EXISTS ext_a;

CREATE TABLE ext_a(id serial, v_det int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'), v_tx text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'));

-- @param int 1
-- @param int 1
-- @param String a
-- @param int 2
-- @param int 2
-- @param String b
-- @param int 3
-- @param int 3
-- @param String c
-- @param int 4
-- @param int 4
-- @param String a
-- @param int 5
-- @param int 5
-- @param String b
-- @param null null
-- @param null null
-- @param null null
INSERT INTO ext_a(v_det,v_ope,v_tx) VALUES (?,?,?),(?,?,?),(?,?,?),(?,?,?),(?,?,?),(?,?,?);

-- ===== 15.2 边界深化 =====
DROP TABLE IF EXISTS ext_b;

CREATE TABLE ext_b(id serial, v2_d int2_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v2_o int2_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'), v4_d int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v4_o int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'), vx text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'));

-- @param int 32766
-- @param int 32766
-- @param int 2147483647
-- @param int 2147483647
-- @param String a
-- @param int -32768
-- @param int -32768
-- @param int -2147483648
-- @param int -2147483648
-- @param String bb
-- @param int 32766
-- @param int 32766
-- @param int 2147483646
-- @param int 2147483646
-- @param String 测试
-- @param int -32767
-- @param int -32767
-- @param int -2147483647
-- @param int -2147483647
-- @param String 
-- @param int 1
-- @param int 1
-- @param int 1
-- @param int 1
-- @param String  
-- @param int -1
-- @param int -1
-- @param int -1
-- @param int -1
-- @param String x
-- @param int 100
-- @param int 100
-- @param int 100
-- @param int 100
-- @param String test%_s
-- @param int 100
-- @param int 100
-- @param int 100
-- @param int 100
-- @param String test%_s
-- @param null null
-- @param null null
-- @param null null
-- @param null null
-- @param null null
INSERT INTO ext_b(v2_d,v2_o,v4_d,v4_o,vx) VALUES (?,?,?,?,?),(?,?,?,?,?),(?,?,?,?,?),(?,?,?,?,?),(?,?,?,?,?),(?,?,?,?,?),(?,?,?,?,?),(?,?,?,?,?),(?,?,?,?,?);

-- ===== 15.3 多列组合 =====

-- ===== 15.4 NOT LIKE 扩展 =====
DROP TABLE IF EXISTS ext_d;

CREATE TABLE ext_d(id serial, v text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'));

-- @param String 张三
-- @param String 张三丰
-- @param String 丰张三
-- @param String 张三李四
-- @param String 李四
-- @param String abc
-- @param String abcdxyz
-- @param String xyzabcd
-- @param String ab
-- @param String ab%cd
INSERT INTO ext_d(v) VALUES (?),(?),(?),(?),(?),(?),(?),(?),(?),(?);

-- ===== 15.5 子查询 =====

-- ============================================================================
-- 第十六部分: 反向/异常用例（不支持的操作应正常报错不崩溃）
-- ============================================================================

DROP TABLE IF EXISTS neg_test;

CREATE TABLE neg_test(id serial PRIMARY KEY, v_text text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), v_det int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), v_ope int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'), v_rnd text_enc ENCRYPTED WITH (dek=test_dek));

-- @param String abc
-- @param int 100
-- @param int 100
-- @param String xxx
-- @param String def
-- @param int 200
-- @param int 200
-- @param String yyy
INSERT INTO neg_test(v_text,v_det,v_ope,v_rnd) VALUES (?,?,?,?),(?,?,?,?);

-- @param int 100
SELECT * FROM neg_test WHERE v_det > ?;

-- @param String xxx
SELECT * FROM neg_test WHERE v_rnd = ?;

-- @param String %x%
SELECT * FROM neg_test WHERE v_rnd LIKE ?;

-- @param String %1%
SELECT * FROM neg_test WHERE v_det LIKE ?;

SELECT v_text::int4_enc FROM neg_test;

SELECT v_det::text_enc FROM neg_test;

SELECT * FROM neg_test WHERE v_det = v_ope;

-- @param String 100
SELECT * FROM neg_test WHERE v_det = ?::text;

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

-- @param int 1
-- @param String 技术部
-- @param int 100
-- @param int 2
-- @param String 市场部
-- @param int 200
-- @param int 3
-- @param String 管理部
-- @param int 300
-- @param int 4
-- @param String 技术部
-- @param int 400
INSERT INTO pt_text VALUES (?,?,?),(?,?,?),(?,?,?),(?,?,?);

SELECT 'text(ope)LIST' t, count(*) FROM pt_text;

-- ===== 14.2 varchar_enc(ope) LIST分区 =====
DROP TABLE IF EXISTS pt_vc, pt_vc_1, pt_vc_2 CASCADE;

CREATE TABLE pt_vc (id int NOT NULL, dept varchar_enc(50) ENCRYPTED WITH (dek=test_dek,support_algo='ope'), budget int) PARTITION BY LIST (dept);

CREATE TABLE pt_vc_1 (id int NOT NULL, dept varchar_enc(50) ENCRYPTED WITH (dek=test_dek,support_algo='ope'), budget int);

ALTER TABLE pt_vc ATTACH PARTITION pt_vc_1 FOR VALUES IN ('技术部','产品部');

CREATE TABLE pt_vc_2 (id int NOT NULL, dept varchar_enc(50) ENCRYPTED WITH (dek=test_dek,support_algo='ope'), budget int);

ALTER TABLE pt_vc ATTACH PARTITION pt_vc_2 FOR VALUES IN ('行政部');

-- @param int 1
-- @param String 技术部
-- @param int 500
-- @param int 2
-- @param String 产品部
-- @param int 300
-- @param int 3
-- @param String 行政部
-- @param int 100
INSERT INTO pt_vc VALUES (?,?,?),(?,?,?),(?,?,?);

SELECT 'varchar(ope)LIST' t, count(*) FROM pt_vc;

-- ===== 14.3 bpchar_enc(ope) LIST分区 =====
DROP TABLE IF EXISTS pt_bp, pt_bp_a, pt_bp_b CASCADE;

CREATE TABLE pt_bp (id int NOT NULL, code bpchar_enc(10) ENCRYPTED WITH (dek=test_dek,support_algo='ope'), descr varchar(50)) PARTITION BY LIST (code);

CREATE TABLE pt_bp_a (id int NOT NULL, code bpchar_enc(10) ENCRYPTED WITH (dek=test_dek,support_algo='ope'), descr varchar(50));

ALTER TABLE pt_bp ATTACH PARTITION pt_bp_a FOR VALUES IN ('CN','US');

CREATE TABLE pt_bp_b (id int NOT NULL, code bpchar_enc(10) ENCRYPTED WITH (dek=test_dek,support_algo='ope'), descr varchar(50));

ALTER TABLE pt_bp ATTACH PARTITION pt_bp_b FOR VALUES IN ('JP','KR');

-- @param int 1
-- @param String CN
-- @param String 中国
-- @param int 2
-- @param String US
-- @param String 美国
-- @param int 3
-- @param String JP
-- @param String 日本
-- @param int 4
-- @param String CN
-- @param String 中国备份
INSERT INTO pt_bp VALUES (?,?,?),(?,?,?),(?,?,?),(?,?,?);

SELECT 'bpchar(ope)LIST' t, count(*) FROM pt_bp;

-- ===== 14.4 int2_enc(ope) RANGE分区 + 明文对照 =====
DROP TABLE IF EXISTS pt_i2, pt_i2_young, pt_i2_old, plain_i2 CASCADE;

CREATE TABLE pt_i2 (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), age int2_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope')) PARTITION BY RANGE (age);

CREATE TABLE pt_i2_young (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), age int2_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

ALTER TABLE pt_i2 ATTACH PARTITION pt_i2_young FOR VALUES FROM (MINVALUE) TO (30);

CREATE TABLE pt_i2_old (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), age int2_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

ALTER TABLE pt_i2 ATTACH PARTITION pt_i2_old FOR VALUES FROM (30) TO (MAXVALUE);

-- @param String 张三
-- @param int 25
-- @param String 李四
-- @param int 35
-- @param String 王五
-- @param int 28
-- @param String 赵六
-- @param int 45
INSERT INTO pt_i2 VALUES (?,?),(?,?),(?,?),(?,?);

-- 明文对照
CREATE TABLE plain_i2 (name text, age int2);

-- @param String 张三
-- @param int 25
-- @param String 李四
-- @param int 35
-- @param String 王五
-- @param int 28
-- @param String 赵六
-- @param int 45
INSERT INTO plain_i2 VALUES (?,?),(?,?),(?,?),(?,?);

-- ===== 14.5 int4_enc(ope) RANGE分区 + 明文对照 =====
DROP TABLE IF EXISTS pt_i4, pt_i4_low, pt_i4_mid, pt_i4_high, plain_i4 CASCADE;

CREATE TABLE pt_i4 (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), salary int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope')) PARTITION BY RANGE (salary);

CREATE TABLE pt_i4_low (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), salary int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

ALTER TABLE pt_i4 ATTACH PARTITION pt_i4_low FOR VALUES FROM (MINVALUE) TO (10000);

CREATE TABLE pt_i4_mid (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), salary int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

ALTER TABLE pt_i4 ATTACH PARTITION pt_i4_mid FOR VALUES FROM (10000) TO (20000);

CREATE TABLE pt_i4_high (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), salary int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

ALTER TABLE pt_i4 ATTACH PARTITION pt_i4_high FOR VALUES FROM (20000) TO (MAXVALUE);

-- @param String 张三
-- @param int 8000
-- @param String 李四
-- @param int 15000
-- @param String 王五
-- @param int 25000
-- @param String 赵六
-- @param int 12000
INSERT INTO pt_i4 VALUES (?,?),(?,?),(?,?),(?,?);

CREATE TABLE plain_i4 (name text, salary int4);

-- @param String 张三
-- @param int 8000
-- @param String 李四
-- @param int 15000
-- @param String 王五
-- @param int 25000
-- @param String 赵六
-- @param int 12000
INSERT INTO plain_i4 VALUES (?,?),(?,?),(?,?),(?,?);

-- ===== 14.6 int8_enc(ope) RANGE分区 + 明文对照 =====
DROP TABLE IF EXISTS pt_i8, pt_i8_small, pt_i8_large, plain_i8 CASCADE;

CREATE TABLE pt_i8 (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), total int8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope')) PARTITION BY RANGE (total);

CREATE TABLE pt_i8_small (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), total int8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

ALTER TABLE pt_i8 ATTACH PARTITION pt_i8_small FOR VALUES FROM (MINVALUE) TO (50000);

CREATE TABLE pt_i8_large (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), total int8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

ALTER TABLE pt_i8 ATTACH PARTITION pt_i8_large FOR VALUES FROM (50000) TO (MAXVALUE);

-- @param String 张三
-- @param int 30000
-- @param String 李四
-- @param int 80000
-- @param String 王五
-- @param int 10000
-- @param String 赵六
-- @param int 50000
INSERT INTO pt_i8 VALUES (?,?),(?,?),(?,?),(?,?);

CREATE TABLE plain_i8 (name text, total int8);

-- @param String 张三
-- @param int 30000
-- @param String 李四
-- @param int 80000
-- @param String 王五
-- @param int 10000
-- @param String 赵六
-- @param int 50000
INSERT INTO plain_i8 VALUES (?,?),(?,?),(?,?),(?,?);

-- ===== 14.7 float4_enc(ope) RANGE分区 + 明文对照 =====
DROP TABLE IF EXISTS pt_f4, pt_f4_low, pt_f4_high, plain_f4 CASCADE;

CREATE TABLE pt_f4 (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), score float4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope')) PARTITION BY RANGE (score);

CREATE TABLE pt_f4_low (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), score float4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

ALTER TABLE pt_f4 ATTACH PARTITION pt_f4_low FOR VALUES FROM (MINVALUE) TO (60.0);

CREATE TABLE pt_f4_high (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), score float4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

ALTER TABLE pt_f4 ATTACH PARTITION pt_f4_high FOR VALUES FROM (60.0) TO (MAXVALUE);

-- @param String 张三
-- @param BigDecimal 55.5
-- @param String 李四
-- @param BigDecimal 85.0
-- @param String 王五
-- @param BigDecimal 59.9
-- @param String 赵六
-- @param BigDecimal 90.5
INSERT INTO pt_f4 VALUES (?,?),(?,?),(?,?),(?,?);

CREATE TABLE plain_f4 (name text, score float4);

-- @param String 张三
-- @param BigDecimal 55.5
-- @param String 李四
-- @param BigDecimal 85.0
-- @param String 王五
-- @param BigDecimal 59.9
-- @param String 赵六
-- @param BigDecimal 90.5
INSERT INTO plain_f4 VALUES (?,?),(?,?),(?,?),(?,?);

-- ===== 14.8 float8_enc(ope) RANGE分区 + 明文对照 =====
DROP TABLE IF EXISTS pt_f8, pt_f8_low, pt_f8_high, plain_f8 CASCADE;

CREATE TABLE pt_f8 (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), amount float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope')) PARTITION BY RANGE (amount);

CREATE TABLE pt_f8_low (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), amount float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

ALTER TABLE pt_f8 ATTACH PARTITION pt_f8_low FOR VALUES FROM (MINVALUE) TO (1000.0);

CREATE TABLE pt_f8_high (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), amount float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

ALTER TABLE pt_f8 ATTACH PARTITION pt_f8_high FOR VALUES FROM (1000.0) TO (MAXVALUE);

-- @param String 张三
-- @param BigDecimal 500.5
-- @param String 李四
-- @param BigDecimal 2000.0
-- @param String 王五
-- @param BigDecimal 999.9
-- @param String 赵六
-- @param BigDecimal 1500.0
INSERT INTO pt_f8 VALUES (?,?),(?,?),(?,?),(?,?);

CREATE TABLE plain_f8 (name text, amount float8);

-- @param String 张三
-- @param BigDecimal 500.5
-- @param String 李四
-- @param BigDecimal 2000.0
-- @param String 王五
-- @param BigDecimal 999.9
-- @param String 赵六
-- @param BigDecimal 1500.0
INSERT INTO plain_f8 VALUES (?,?),(?,?),(?,?),(?,?);

-- ===== 14.9 numeric_enc(ope) RANGE分区 + 明文对照 =====
DROP TABLE IF EXISTS pt_nm, pt_nm_low, pt_nm_mid, pt_nm_high, plain_nm CASCADE;

CREATE TABLE pt_nm (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), balance numeric_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope')) PARTITION BY RANGE (balance);

CREATE TABLE pt_nm_low (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), balance numeric_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

ALTER TABLE pt_nm ATTACH PARTITION pt_nm_low FOR VALUES FROM (MINVALUE) TO (50000);

CREATE TABLE pt_nm_mid (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), balance numeric_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

ALTER TABLE pt_nm ATTACH PARTITION pt_nm_mid FOR VALUES FROM (50000) TO (100000);

CREATE TABLE pt_nm_high (name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), balance numeric_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

ALTER TABLE pt_nm ATTACH PARTITION pt_nm_high FOR VALUES FROM (100000) TO (MAXVALUE);

-- @param String 张三
-- @param int 30000
-- @param String 李四
-- @param int 80000
-- @param String 王五
-- @param int 150000
-- @param String 赵六
-- @param int 50000
INSERT INTO pt_nm VALUES (?,?),(?,?),(?,?),(?,?);

CREATE TABLE plain_nm (name text, balance numeric);

-- @param String 张三
-- @param int 30000
-- @param String 李四
-- @param int 80000
-- @param String 王五
-- @param int 150000
-- @param String 赵六
-- @param int 50000
INSERT INTO plain_nm VALUES (?,?),(?,?),(?,?),(?,?);

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

-- @param int 100
-- @param int 20000
-- @param int 3000000
-- @param BigDecimal 3.14
-- @param BigDecimal 2.718
-- @param BigDecimal 99999.99
-- @param String hello
-- @param String world
-- @param String test
INSERT INTO cast_src(c2,c4,c8,f4,f8,nm,tx,vc,bp) VALUES
(?, ?, ?, ?, ?, ?, ?, ?, ?);

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

-- @param String 张三
-- @param String 110101199001011234
-- @param int 15000
-- @param String 技术部
-- @param String 2020-03-15
-- @param int 5
-- @param String 李四
-- @param String 110101199205052345
-- @param int 12000
-- @param String 市场部
-- @param String 2021-06-01
-- @param int 4
-- @param String 王五
-- @param String 110101198812013456
-- @param int 18000
-- @param String 技术部
-- @param String 2019-01-10
-- @param int 6
-- @param String 赵六
-- @param String 110101199308084567
-- @param int 25000
-- @param String 管理部
-- @param String 2018-05-20
-- @param int 7
-- @param String Alice
-- @param String 410101199510105678
-- @param int 20000
-- @param String Engineering
-- @param String 2022-01-15
-- @param int 5
-- @param String Bob
-- @param String 410101199603206789
-- @param int 16000
-- @param String Sales
-- @param String 2021-09-01
-- @param int 4
-- @param String Charlie
-- @param String 410101199707307890
-- @param int 22000
-- @param String Engineering
-- @param String 2020-07-10
-- @param int 6
-- @param String 孙七
-- @param null null
-- @param int 9000
-- @param String 市场部
-- @param null null
-- @param int 2
-- @param String 周八
-- @param String 320101199804158901
-- @param null null
-- @param String 技术部
-- @param String 2022-06-01
-- @param int 3
-- @param null null
-- @param String 320101199905209012
-- @param int 11000
-- @param String 管理部
-- @param String 2023-01-01
-- @param int 3
INSERT INTO employee(name,id_card,salary,dept,hire_date,level) VALUES
(?,   ?,  ?,  ?, ?, ?),
(?,   ?,  ?,  ?, ?, ?),
(?,   ?,  ?,  ?, ?, ?),
(?,   ?,  ?,  ?, ?, ?),
(?,  ?,  ?,  ?, ?, ?),
(?,    ?,  ?,  ?,  ?, ?),
(?,?,  ?,  ?, ?, ?),
(?,   ?,                   ?,  ?, ?,           ?),
(?,   ?,  ?,   ?, ?, ?),
(?,     ?,  ?,  ?, ?, ?);

SELECT id, name, salary, dept, hire_date, level FROM employee ORDER BY id;

-- 密态列等值
-- @param String 张三
SELECT id, name, dept FROM employee WHERE name=?;

-- 明文列等值
-- @param String 技术部
SELECT id, name, dept, hire_date FROM employee WHERE dept=? ORDER BY id;

-- 密态+明文复合条件
-- @param String 技术部
-- @param int 10000
SELECT id, name, salary, dept FROM employee WHERE dept=? AND salary>? ORDER BY id;

-- 密态列LIKE+明文条件
-- @param String 张%
-- @param int 5
SELECT id, name, level FROM employee WHERE name LIKE ? AND level>=?;

-- 明文列ORDER BY
SELECT id, name, hire_date FROM employee WHERE hire_date IS NOT NULL ORDER BY hire_date;

-- 密态列ORDER BY (ope)
SELECT id, name, salary FROM employee WHERE salary IS NOT NULL ORDER BY salary DESC;

-- 密态列范围+明文IN
-- @param int 15000
-- @param String 技术部
-- @param String 管理部
SELECT id, name, salary, dept FROM employee WHERE salary>? AND (dept=? OR dept=?) ORDER BY salary DESC;

-- COUNT聚合(混合)
SELECT dept, count(*) cnt, avg(salary::numeric)::numeric(10,2) avg_sal FROM employee WHERE dept IS NOT NULL AND salary IS NOT NULL GROUP BY dept ORDER BY cnt DESC;

-- NULL处理(密态+明文混合NULL)
SELECT id, name, id_card, hire_date FROM employee WHERE name IS NULL OR hire_date IS NULL;

-- 更新明文列(密态列不变)
-- @param int 1
-- @param String 技术部
UPDATE employee SET level=level+? WHERE dept=?;

-- @param String 技术部
SELECT id, name, dept, level FROM employee WHERE dept=? ORDER BY id;

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

-- @param String 张三
-- @param String 高血压
-- @param BigDecimal 1500.50
-- @param String 市第一医院
-- @param String 心内科
-- @param String 2025-01-10
-- @param String 李四
-- @param String 急性阑尾炎
-- @param BigDecimal 8500.00
-- @param String 市第一医院
-- @param String 普外科
-- @param String 2025-01-15
-- @param String 王五
-- @param String 糖尿病
-- @param BigDecimal 3200.00
-- @param String 省中心医院
-- @param String 内分泌科
-- @param String 2025-02-01
-- @param String 赵六
-- @param String 骨折
-- @param BigDecimal 12000.00
-- @param String 市骨科医院
-- @param String 骨科
-- @param String 2025-02-10
-- @param String Alice
-- @param String Hypertension
-- @param BigDecimal 2500.00
-- @param String General Hospital
-- @param String Cardiology
-- @param String 2025-03-01
-- @param String Bob
-- @param String Appendicitis
-- @param BigDecimal 9000.00
-- @param String General Hospital
-- @param String Surgery
-- @param String 2025-03-05
-- @param String 张三
-- @param String 常规体检
-- @param BigDecimal 800.00
-- @param String 市第一医院
-- @param String 体检中心
-- @param String 2025-03-15
-- @param String 孙七
-- @param null null
-- @param BigDecimal 1500.00
-- @param String 省中心医院
-- @param String 内科
-- @param null null
-- @param null null
-- @param String 肺炎
-- @param BigDecimal 4500.00
-- @param null null
-- @param String 呼吸科
-- @param String 2025-04-01
INSERT INTO medical(patient,diagnosis,cost,hospital,department,visit_date,is_emergency) VALUES
(?,  ?,      ?, ?, ?, ?, false),
(?,  ?,  ?, ?, ?, ?, true),
(?,  ?,      ?, ?, ?,?, false),
(?,  ?,       ?, ?, ?,   ?, true),
(?, ?,?, ?,?,?,false),
(?,   ?,?, ?,?,?,true),
(?,  ?,     ?, ?, ?,?, false),
(?,  ?,          ?, ?, ?,   ?,          false),
(?,    ?,        ?, ?,         ?, ?,  true);

SELECT id, patient, diagnosis, cost, hospital, department, visit_date FROM medical ORDER BY id;

-- 密态LIKE搜索患者
-- @param String 张%
SELECT id, patient, diagnosis, hospital FROM medical WHERE patient LIKE ? ORDER BY id;

-- 密态范围+明文布尔条件
-- @param int 5000
SELECT id, patient, cost, department FROM medical WHERE cost>? AND is_emergency=true ORDER BY cost DESC;

-- 跨表JOIN(密态列在SELECT中)
-- @param int 5
SELECT e.name, e.dept, m.diagnosis, m.cost
FROM employee e JOIN medical m ON e.id=m.id
WHERE e.name IS NOT NULL AND m.diagnosis IS NOT NULL
ORDER BY e.id LIMIT ?;

-- 多条件: 密态+明文+密态
-- @param String 市第一医院
-- @param int 5000
SELECT id, patient, diagnosis, cost FROM medical WHERE hospital=? AND cost<? ORDER BY id;

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

-- @param String 6222021234567890
-- @param String 110101199001011234
-- @param BigDecimal 50000.00
-- @param String 工商银行
-- @param String 储蓄账户
-- @param String active
-- @param String 2020-03-20
-- @param String 6228480987654321
-- @param String 110101199205052345
-- @param BigDecimal 150000.00
-- @param String 农业银行
-- @param String 理财账户
-- @param String active
-- @param String 2021-06-05
-- @param String 6227003333222244
-- @param String 110101198812013456
-- @param BigDecimal 3500.00
-- @param String 建设银行
-- @param String 储蓄账户
-- @param String active
-- @param String 2019-01-15
-- @param String 6217001111222233
-- @param String 110101199308084567
-- @param BigDecimal 850000.00
-- @param String 工商银行
-- @param String 储蓄账户
-- @param String frozen
-- @param String 2018-05-25
-- @param String 6226098888777766
-- @param String 410101199510105678
-- @param BigDecimal 25000.00
-- @param String 招商银行
-- @param String 储蓄账户
-- @param String active
-- @param String 2022-01-20
-- @param String 6225889999000011
-- @param null null
-- @param BigDecimal 1000.00
-- @param String 工商银行
-- @param String 储蓄账户
-- @param String active
-- @param String 2023-06-01
-- @param null null
-- @param String 410101199707307890
-- @param BigDecimal 750000.00
-- @param String 招商银行
-- @param String 理财账户
-- @param String active
-- @param String 2020-08-10
INSERT INTO account(account_no,id_card,balance,bank_name,acct_type,status,open_date) VALUES
(?,?,  ?, ?, ?, ?, ?),
(?,?, ?, ?, ?, ?, ?),
(?,?,   ?, ?, ?, ?, ?),
(?,?, ?, ?, ?, ?,?),
(?,?,  ?, ?, ?, ?, ?),
(?,?,                   ?, ?, ?, ?, ?),
(?,              ?,  ?, ?, ?, ?, ?);

SELECT id, account_no, balance, bank_name, acct_type, status FROM account ORDER BY id;

-- 密态范围: 高余额
-- @param int 100000
SELECT id, balance, bank_name FROM account WHERE balance>? ORDER BY balance DESC;

-- 明文+密态
-- @param String 工商银行
-- @param int 10000
SELECT id, balance, acct_type FROM account WHERE bank_name=? AND balance>? ORDER BY id;

-- 明文GROUP BY + 密态聚合
SELECT bank_name, status, count(*) cnt, sum(balance)::numeric(15,2) total_balance
FROM account WHERE bank_name IS NOT NULL AND balance IS NOT NULL
GROUP BY bank_name, status ORDER BY total_balance DESC;

-- 密态列NULL搜索
SELECT id, balance, bank_name FROM account WHERE account_no IS NULL;

-- 多密态列组合: account_no LIKE + balance范围
-- @param String 622202%
-- @param int 10000
SELECT id, account_no, balance FROM account WHERE account_no LIKE ? AND balance>?;

-- 明文IN + 密态ORDER BY
-- @param String 工商银行
-- @param String 招商银行
SELECT id, balance, bank_name FROM account WHERE bank_name IN(?,?) AND balance IS NOT NULL ORDER BY balance DESC;

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

-- @param String 张三
-- @param int 25
-- @param int 5000
-- @param String 李四
-- @param int 30
-- @param int 8000
-- @param String 王五
-- @param int 28
-- @param int 6000
INSERT INTO cp_test(name,age,sal) VALUES (?,?,?),(?,?,?),(?,?,?);

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

-- @param String 张三
-- @param int 25
-- @param BigDecimal 85.5
-- @param String 技术部
-- @param String 李四
-- @param int 30
-- @param BigDecimal 92.0
-- @param String 市场部
-- @param String 王五
-- @param int 28
-- @param BigDecimal 78.5
-- @param String 技术部
-- @param null null
-- @param int 22
-- @param BigDecimal 70.0
-- @param String 市场部
INSERT INTO r_src(name,age,score,dept) VALUES (?,?,?,?),(?,?,?,?),(?,?,?,?),(?,?,?,?);

-- COPY导出密文
-- 原 \copy 命令: COPY r_src TO 'E:/tmp/tecc-test-keys/r_src.csv' WITH (FORMAT csv, HEADER)

-- COPY导入(tecc ON, 密文文件)
DROP TABLE IF EXISTS r_copy;

CREATE TABLE r_copy(id serial, name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), age int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'), score float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'), dept varchar(50));

-- 原 \copy 命令: COPY r_copy FROM 'E:/tmp/tecc-test-keys/r_src.csv' WITH (FORMAT csv, HEADER)

SELECT id, name, age, dept FROM r_copy ORDER BY id;

-- @param String 张三
SELECT count(*) FROM r_copy WHERE name=?;

-- @param int 25
SELECT count(*) FROM r_copy WHERE age>?;

-- @param String 张%
SELECT count(*) FROM r_copy WHERE name LIKE ?;

SELECT count(*) FROM r_copy WHERE name IS NULL;

-- @param String 技术部
SELECT count(*) FROM r_copy WHERE dept=?;

-- ===== 14.15 pg_dump恢复后全面查询验证 =====
-- 替换表名为r_dump

DROP TABLE IF EXISTS r_dump;

CREATE TABLE r_dump(id serial, name text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'), age int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'), score float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'), dept varchar(50));

SET search_path = public;

SELECT id, name, age, dept FROM r_dump ORDER BY id;

-- @param String 张三
SELECT count(*) FROM r_dump WHERE name=?;

-- @param int 25
SELECT count(*) FROM r_dump WHERE age>?;

-- @param String 张%
SELECT count(*) FROM r_dump WHERE name LIKE ?;

SELECT count(*) FROM r_dump WHERE name IS NULL;

-- @param String 技术部
SELECT count(*) FROM r_dump WHERE dept=?;

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

-- @param int 100
-- @param String hello
INSERT INTO edge1(v,t) VALUES (?,?);

SELECT 'direct' t, count(*) FROM edge1;

-- @param int 1
-- @param String test
INSERT INTO edge1(v,t) VALUES (?::int2, ?);

-- @param int 1
-- @param String test
INSERT INTO edge1(v,t) VALUES (?::int8, ?);

-- @param BigDecimal 1.5
-- @param String test
INSERT INTO edge1(v,t) VALUES (?::float4, ?);

-- @param String test
INSERT INTO edge1(t) VALUES (?::varchar);

-- ===== 22.2 UPDATE + DELETE =====
DROP TABLE IF EXISTS edge2;

CREATE TABLE edge2(id serial, v int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'), t text_enc ENCRYPTED WITH (dek=test_dek,support_algo='det,search'));

-- @param int 100
-- @param String hello
-- @param int 200
-- @param String world
INSERT INTO edge2(v,t) VALUES (?,?),(?,?);

-- @param int 300
-- @param int 1
UPDATE edge2 SET v=? WHERE id=?;

-- @param int 300
SELECT 'update_ok' t, count(*) FROM edge2 WHERE v=?;

-- @param int 200
DELETE FROM edge2 WHERE v=?;

-- @param int 200
SELECT 'delete_ok' t, count(*) FROM edge2 WHERE v=?;

-- ===== 22.3 INSERT SELECT =====
DROP TABLE IF EXISTS edge3_plain;

CREATE TABLE edge3_plain(v int);

-- @param int 100
-- @param int 200
INSERT INTO edge3_plain VALUES (?),(?);

DROP TABLE IF EXISTS edge3_enc;

CREATE TABLE edge3_enc(id serial, v int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det'));

INSERT INTO edge3_enc(v) SELECT v FROM edge3_plain;

-- ===== 22.4 视图 =====
CREATE OR REPLACE VIEW edge_view AS SELECT id, v, t FROM edge2 WHERE v IS NOT NULL;

-- ===== 22.5 CTE =====
-- @param int 0
WITH t AS (SELECT id, v FROM edge2 WHERE v > ?) SELECT count(*) FROM t;

-- ===== 22.6 UNION =====
-- @param int 500
-- @param String u1
-- @param int 600
-- @param String u2
INSERT INTO edge2(v,t) VALUES (?,?),(?,?);

-- @param int 500
-- @param int 600
SELECT count(*) FROM (SELECT v FROM edge2 WHERE v=? UNION ALL SELECT v FROM edge2 WHERE v=?) t;

-- ===== 22.7 ALTER TABLE =====
ALTER TABLE edge2 ADD COLUMN new_col float8_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope');

ALTER TABLE edge2 ALTER COLUMN v TYPE int8_enc;

-- ===== 22.8 DEFAULT =====
DROP TABLE IF EXISTS edge8;

CREATE TABLE edge8(id serial, v int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='det') DEFAULT 0);

INSERT INTO edge8 DEFAULT VALUES;

-- ===== 22.9 TRUNCATE + 多行INSERT =====
TRUNCATE edge2;

-- @param int 1
-- @param String a
-- @param null null
-- @param null null
-- @param int 2
-- @param null null
-- @param null null
-- @param String b
INSERT INTO edge2(v,t) VALUES (?,?),(?,?),(?,?),(?,?);

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

-- @param int 100
INSERT INTO diff1(v) VALUES (?);

CREATE TABLE diff2(id serial, v int4_enc ENCRYPTED WITH (dek=dek2,support_algo='det'));

-- @param int 200
INSERT INTO diff2(v) VALUES (?);

INSERT INTO diff1(v) SELECT v FROM diff2;

SELECT a.id FROM diff1 a, diff2 b WHERE a.v=b.v;

-- ===== 23.2 btree+hash索引同时存在 =====
DROP TABLE IF EXISTS idx_both;

CREATE TABLE idx_both(id serial, v_ope int4_enc ENCRYPTED WITH (dek=test_dek,support_algo='ope'));

-- @param int 1
-- @param int 2
-- @param int 3
-- @param int 4
-- @param int 5
-- @param int 6
-- @param int 7
-- @param int 8
-- @param int 9
-- @param int 10
INSERT INTO idx_both(v_ope) VALUES (?),(?),(?),(?),(?),(?),(?),(?),(?),(?);

CREATE INDEX idx_both_hash ON idx_both USING hash(v_ope);

CREATE INDEX idx_both_btree ON idx_both(v_ope);

-- @param String idx_both
SELECT indexrelid::regclass idx_name, amname FROM pg_index i JOIN pg_am a ON i.indrelid=a.oid WHERE indrelid=?::regclass ORDER BY amname;

-- @param int 5
SELECT count(*) FROM idx_both WHERE v_ope=?;

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

-- @param int 100
-- @param int 100
INSERT INTO mixed_type(v_plain,v_enc) VALUES (?,?);

-- @param int 1
INSERT INTO mixed_type(v_enc) SELECT v_plain FROM mixed_type WHERE id=?;

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
