--
-- test_copy_extended.sql — libpq专有测试：\copy 扩展场景
-- 覆盖显式列列表、CSV格式、\copy TO/FROM 明文验证等
--

-- ============================================================
-- 准备
-- ============================================================
DROP TABLE IF EXISTS test_copy_ext CASCADE;
DROP DEK IF EXISTS dek_copy_ext CASCADE;
DROP KEK IF EXISTS kek_copy_ext CASCADE;

CREATE KEK kek_copy_ext WITH (
    key_store = 'localkms',
    key_path = '/tmp/kek_copy_ext.key'
);
CREATE DEK dek_copy_ext WITH VALUES(KEK = 'kek_copy_ext');

CREATE TABLE test_copy_ext (
    id      INT4_ENC    ENCRYPTED WITH (DEK = dek_copy_ext, SUPPORT_ALGO = 'DET'),
    name    TEXT_ENC    ENCRYPTED WITH (DEK = dek_copy_ext, SUPPORT_ALGO = 'DET,SEARCH')
);

INSERT INTO test_copy_ext VALUES (1, 'Alice');
INSERT INTO test_copy_ext VALUES (2, 'Bob');
INSERT INTO test_copy_ext VALUES (3, 'Charlie');

-- 验证原始数据
SELECT * FROM test_copy_ext;

-- ============================================================
-- 测试1: \copy TO 显式列列表 → 输出明文（本次修复的关键场景）
-- ============================================================
\echo '=== test1: \\copy TO with explicit column list ==='
\copy test_copy_ext (id, name) TO 'copy_ext_cols.txt'
-- 查看导出内容（应该看到明文数字/字符串，而非 \x hex）
\! head -3 copy_ext_cols.txt

-- ============================================================
-- 测试2: \copy FROM 显式列列表 + 验证导入后数据正确
-- ============================================================
\echo '=== test2: \\copy FROM with explicit column list ==='
TRUNCATE test_copy_ext;
\copy test_copy_ext (id, name) FROM 'copy_ext_cols.txt'
SELECT * FROM test_copy_ext;

-- ============================================================
-- 测试3: \copy TO 默认（无列列表） → 验证同样输出明文
-- ============================================================
\echo '=== test3: \\copy TO without column list ==='
\copy test_copy_ext TO 'copy_ext_default.txt'
\! head -3 copy_ext_default.txt

-- ============================================================
-- 测试4: \copy FROM 默认（用测试3导出的文件重新导入）
-- ============================================================
\echo '=== test4: \\copy FROM without column list ==='
TRUNCATE test_copy_ext;
\copy test_copy_ext FROM 'copy_ext_default.txt'
SELECT * FROM test_copy_ext;

-- ============================================================
-- 测试5: CSV 格式 \copy TO/FROM（修复前输出hex密文，修复后输出明文）
-- ============================================================
\echo '=== test5: \\copy CSV format (fixed: plaintext output) ==='
\copy test_copy_ext TO 'copy_ext_csv.txt' WITH (FORMAT CSV)
\! head -3 copy_ext_csv.txt
TRUNCATE test_copy_ext;
\copy test_copy_ext FROM 'copy_ext_csv.txt' WITH (FORMAT CSV)
SELECT * FROM test_copy_ext;

-- ============================================================
-- 测试6: 多行 \copy (批量行加解密，包含5行)
-- ============================================================
\echo '=== test6: multi-row \\copy ==='
INSERT INTO test_copy_ext VALUES (4, 'David');
INSERT INTO test_copy_ext VALUES (5, 'Eve');
\copy test_copy_ext TO 'copy_ext_multi.txt'
TRUNCATE test_copy_ext;
\copy test_copy_ext FROM 'copy_ext_multi.txt'
SELECT * FROM test_copy_ext;

-- ============================================================
-- 测试8: pg_dump 全9种加密类型导出明文验证
-- (TC_IVYSQL_TECC4_REL_BR_014 / Bug 8047)
-- ============================================================
\echo '=== test8: pg_dump all 9 enc types (REL_BR_014) ==='

DROP TABLE IF EXISTS tb_all CASCADE;
DROP DEK IF EXISTS dek1 CASCADE;
DROP KEK IF EXISTS kek1 CASCADE;

CREATE KEK kek1 WITH (key_store = 'localkms', key_path = '/tmp/kek_8047.key');
CREATE DEK dek1 WITH VALUES(KEK = kek1);
CREATE TABLE tb_all (
    id INT,
    col_text TEXT_ENC ENCRYPTED WITH (DEK = dek1),
    col_varchar VARCHAR_ENC ENCRYPTED WITH (DEK = dek1),
    col_bpchar BPCHAR_ENC ENCRYPTED WITH (DEK = dek1),
    col_int2 INT2_ENC ENCRYPTED WITH (DEK = dek1),
    col_int4 INT4_ENC ENCRYPTED WITH (DEK = dek1),
    col_int8 INT8_ENC ENCRYPTED WITH (DEK = dek1),
    col_numeric NUMERIC_ENC ENCRYPTED WITH (DEK = dek1),
    col_float4 FLOAT4_ENC ENCRYPTED WITH (DEK = dek1),
    col_float8 FLOAT8_ENC ENCRYPTED WITH (DEK = dek1)
);
INSERT INTO tb_all VALUES (1, 'text_value', 'varchar_value', 'bpchar_val',
    123::INT2, 456789::INT4, 9876543210::INT8, 12345.67::NUMERIC,
    3.14::FLOAT4, 2.718281828::FLOAT8);

-- 模拟pg_dump --data-only 导出（使用 COPY TO STDOUT 格式，结果与 pg_dump 一致）
\copy tb_all TO 'pg_dump_8047.sql'
\! echo '=== pg_dump-style COPY data (expect plaintext) ==='
\! cat pg_dump_8047.sql

-- 删除数据后恢复
DELETE FROM tb_all;
\copy tb_all FROM 'pg_dump_8047.sql'

-- 验证恢复结果
SELECT * FROM tb_all;

DROP TABLE IF EXISTS tb_all CASCADE;
DROP DEK IF EXISTS dek1 CASCADE;
DROP KEK IF EXISTS kek1 CASCADE;

-- ============================================================
-- 测试9: 服务端 COPY TO/FROM（不走解密，密文导入导出）
-- ============================================================
\echo '=== test9: server-side COPY TO/FROM (ciphertext, no decryption) ==='

DROP TABLE IF EXISTS test_srv CASCADE;
DROP DEK IF EXISTS dek_srv CASCADE;
DROP KEK IF EXISTS kek_srv CASCADE;

CREATE KEK kek_srv WITH (key_store = 'localkms', key_path = '/tmp/kek_srv.key');
CREATE DEK dek_srv WITH VALUES(KEK = 'kek_srv');
CREATE TABLE test_srv (
    id      INT4_ENC    ENCRYPTED WITH (DEK = dek_srv, SUPPORT_ALGO = 'DET'),
    name    TEXT_ENC    ENCRYPTED WITH (DEK = dek_srv, SUPPORT_ALGO = 'DET,SEARCH')
);
INSERT INTO test_srv VALUES (1, 'Alice');
INSERT INTO test_srv VALUES (2, 'Bob');
SELECT * FROM test_srv;

-- test9a: COPY TO 服务端文件（输出密文）— %PGDATA% 指向实例数据目录
\echo '=== test9a: COPY TO server file ==='
COPY test_srv TO '%PGDATA%/srv_copy_out.txt';
\! head -3 %PGDATA%/srv_copy_out.txt

-- test9b: COPY FROM 服务端文件导入
\echo '=== test9b: COPY FROM server file ==='
DELETE FROM test_srv;
COPY test_srv FROM '%PGDATA%/srv_copy_out.txt';
SELECT * FROM test_srv;

-- test9c: COPY TO 显式列列表
\echo '=== test9c: COPY TO with column list ==='
COPY test_srv (id, name) TO '%PGDATA%/srv_copy_cols.txt';
\! head -3 %PGDATA%/srv_copy_cols.txt

-- test9d: COPY FROM 显式列列表
\echo '=== test9d: COPY FROM with column list ==='
DELETE FROM test_srv;
COPY test_srv (id, name) FROM '%PGDATA%/srv_copy_cols.txt';
SELECT * FROM test_srv;

-- test9e: COPY TO CSV 格式（服务端密文）
\echo '=== test9e: COPY TO CSV format (ciphertext) ==='
COPY test_srv TO '%PGDATA%/srv_copy_csv.txt' WITH (FORMAT CSV);
\! head -3 %PGDATA%/srv_copy_csv.txt

-- test9f: COPY FROM CSV 格式
\echo '=== test9f: COPY FROM CSV format ==='
DELETE FROM test_srv;
COPY test_srv FROM '%PGDATA%/srv_copy_csv.txt' WITH (FORMAT CSV);
SELECT * FROM test_srv;

-- test9g: COPY TO 默认格式+WITH选项 (DELIMITER)
\echo '=== test9g: COPY TO with delimiter ==='
COPY test_srv TO '%PGDATA%/srv_copy_delim.txt' WITH (DELIMITER '|');
\! head -3 %PGDATA%/srv_copy_delim.txt

-- test9h: COPY FROM 自定义分隔符
\echo '=== test9h: COPY FROM with custom delimiter ==='
DELETE FROM test_srv;
COPY test_srv FROM '%PGDATA%/srv_copy_delim.txt' WITH (DELIMITER '|');
SELECT * FROM test_srv;

DROP TABLE IF EXISTS test_srv CASCADE;
DROP DEK IF EXISTS dek_srv CASCADE;
DROP KEK IF EXISTS kek_srv CASCADE;

-- ============================================================
-- test10: \copy TO/FROM WITH (FORMAT binary) 加密列二进制模式
-- ============================================================
\echo '=== test10a: \\copy TO binary format ==='

DROP TABLE IF EXISTS test_copy_bin CASCADE;
DROP DEK IF EXISTS dek_copy_bin CASCADE;
DROP KEK IF EXISTS kek_copy_bin CASCADE;

CREATE KEK kek_copy_bin WITH (
    key_store = 'localkms',
    key_path = '/tmp/kek_copy_bin.key'
);
CREATE DEK dek_copy_bin WITH VALUES(KEK = 'kek_copy_bin');
CREATE TABLE test_copy_bin (
    name    TEXT_ENC    ENCRYPTED WITH (DEK = dek_copy_bin, SUPPORT_ALGO = 'DET'),
    salary  INT4_ENC    ENCRYPTED WITH (DEK = dek_copy_bin, SUPPORT_ALGO = 'DET')
);
INSERT INTO test_copy_bin VALUES ('Alice', 50000);
INSERT INTO test_copy_bin VALUES ('Bob', 60000);
INSERT INTO test_copy_bin VALUES ('Charlie', 70000);

-- 二进制导出
\copy test_copy_bin TO 'copy_binary.bin' WITH (FORMAT binary)

-- 清理并二进制导入
\echo '=== test10b: \\copy FROM binary format ==='
TRUNCATE test_copy_bin;
\copy test_copy_bin FROM 'copy_binary.bin' WITH (FORMAT binary)
SELECT * FROM test_copy_bin;

-- ============================================================
-- test11: \copy binary 多行 + 混合加密/非加密列
-- ============================================================
\echo '=== test11: \\copy binary with mixed encrypted types ==='

DROP TABLE IF EXISTS test_copy_mix CASCADE;
DROP DEK IF EXISTS dek_copy_mix CASCADE;
DROP KEK IF EXISTS kek_copy_mix CASCADE;

CREATE KEK kek_copy_mix WITH (
    key_store = 'localkms',
    key_path = '/tmp/kek_copy_mix.key'
);
CREATE DEK dek_copy_mix WITH VALUES(KEK = 'kek_copy_mix');
CREATE TABLE test_copy_mix (
    id      INT4,
    name    TEXT_ENC    ENCRYPTED WITH (DEK = dek_copy_mix, SUPPORT_ALGO = 'DET'),
    bonus   INT4_ENC    ENCRYPTED WITH (DEK = dek_copy_mix, SUPPORT_ALGO = 'DET')
);
INSERT INTO test_copy_mix VALUES (1, 'Alice', 1000);
INSERT INTO test_copy_mix VALUES (2, 'Bob', 2000);
INSERT INTO test_copy_mix VALUES (3, 'Charlie', 3000);
INSERT INTO test_copy_mix VALUES (4, 'David', 4000);
INSERT INTO test_copy_mix VALUES (5, 'Eve', 5000);

\copy test_copy_mix TO 'copy_mix.bin' WITH (FORMAT binary)
TRUNCATE test_copy_mix;
\copy test_copy_mix FROM 'copy_mix.bin' WITH (FORMAT binary)
SELECT * FROM test_copy_mix;

DROP TABLE IF EXISTS test_copy_mix CASCADE;
DROP DEK IF EXISTS dek_copy_mix CASCADE;
DROP KEK IF EXISTS kek_copy_mix CASCADE;

DROP TABLE IF EXISTS test_copy_bin CASCADE;
DROP DEK IF EXISTS dek_copy_bin CASCADE;
DROP KEK IF EXISTS kek_copy_bin CASCADE;

-- ============================================================
-- test12: \copy binary 全9种加密类型 + 普通类型 + 边界值
-- ============================================================
\echo '=== test12: \\copy binary all 9 encrypted types + boundaries ==='

DROP TABLE IF EXISTS test_copy_all CASCADE;
DROP DEK IF EXISTS dek_copy_all CASCADE;
DROP KEK IF EXISTS kek_copy_all CASCADE;

CREATE KEK kek_copy_all WITH (
    key_store = 'localkms',
    key_path = '/tmp/kek_copy_all.key'
);
CREATE DEK dek_copy_all WITH VALUES(KEK = 'kek_copy_all');

CREATE TABLE test_copy_all (
    c_int2      INT2_ENC    ENCRYPTED WITH (DEK = dek_copy_all, SUPPORT_ALGO = 'DET'),
    c_int4      INT4_ENC    ENCRYPTED WITH (DEK = dek_copy_all, SUPPORT_ALGO = 'DET'),
    c_int8      INT8_ENC    ENCRYPTED WITH (DEK = dek_copy_all, SUPPORT_ALGO = 'DET'),
    c_float4    FLOAT4_ENC  ENCRYPTED WITH (DEK = dek_copy_all, SUPPORT_ALGO = 'DET'),
    c_float8    FLOAT8_ENC  ENCRYPTED WITH (DEK = dek_copy_all, SUPPORT_ALGO = 'DET'),
    c_numeric   NUMERIC_ENC ENCRYPTED WITH (DEK = dek_copy_all, SUPPORT_ALGO = 'DET'),
    c_text      TEXT_ENC    ENCRYPTED WITH (DEK = dek_copy_all, SUPPORT_ALGO = 'DET'),
    c_varchar   VARCHAR_ENC ENCRYPTED WITH (DEK = dek_copy_all, SUPPORT_ALGO = 'DET'),
    c_bpchar    BPCHAR_ENC  ENCRYPTED WITH (DEK = dek_copy_all, SUPPORT_ALGO = 'DET'),
    c_plain     INT4
);

-- 边界值 + 常规值：INT2_MIN/MAX, INT4_MIN/MAX, INT8_MIN/MAX, 0, 负数, 浮点, 字符串
INSERT INTO test_copy_all VALUES (
    -32768::INT2, -2147483648::INT4, -9223372036854775808::INT8,
    -3.402823466::FLOAT4, -1.7976931348623157e+308::FLOAT8,
    -99999999999999.99999::NUMERIC,
    'Hello', 'Varchar', 'Bpchar', 1
);
INSERT INTO test_copy_all VALUES (
    32767::INT2, 2147483647::INT4, 9223372036854775807::INT8,
    3.402823466::FLOAT4, 1.7976931348623157e+308::FLOAT8,
    99999999999999.99999::NUMERIC,
    'World', 'Varchar2', 'BpcharX', 2
);
INSERT INTO test_copy_all VALUES (
    0::INT2, 0::INT4, 0::INT8,
    0.0::FLOAT4, 0.0::FLOAT8,
    0::NUMERIC,
    '', '', '', 3
);
INSERT INTO test_copy_all VALUES (
    -1::INT2, -1::INT4, -1::INT8,
    -1.0::FLOAT4, -1.0::FLOAT8,
    -123.456::NUMERIC,
    E'\\t\\n\\\\', 'Special', 'BP', 4
);

\copy test_copy_all TO 'copy_all.bin' WITH (FORMAT binary)
TRUNCATE test_copy_all;
\copy test_copy_all FROM 'copy_all.bin' WITH (FORMAT binary)
SELECT * FROM test_copy_all ORDER BY c_plain;

-- ============================================================
-- test13: \copy binary NULL 值处理
-- ============================================================
\echo '=== test13: \\copy binary with NULL values ==='

DROP TABLE IF EXISTS test_copy_null CASCADE;

CREATE TABLE test_copy_null (
    id      INT4,
    name    TEXT_ENC    ENCRYPTED WITH (DEK = dek_copy_all, SUPPORT_ALGO = 'DET'),
    salary  INT4_ENC    ENCRYPTED WITH (DEK = dek_copy_all, SUPPORT_ALGO = 'DET'),
    bonus   INT4_ENC    ENCRYPTED WITH (DEK = dek_copy_all, SUPPORT_ALGO = 'DET')
);

-- 覆盖各种NULL组合：全NULL、部分NULL、首列为NULL、末列为NULL
INSERT INTO test_copy_null VALUES (1, 'Alice', 50000, 1000);
INSERT INTO test_copy_null VALUES (2, NULL, NULL, NULL);
INSERT INTO test_copy_null VALUES (3, 'Charlie', NULL, 3000);
INSERT INTO test_copy_null VALUES (4, NULL, 60000, NULL);
INSERT INTO test_copy_null VALUES (5, 'Eve', NULL, NULL);

\copy test_copy_null TO 'copy_null.bin' WITH (FORMAT binary)
TRUNCATE test_copy_null;
\copy test_copy_null FROM 'copy_null.bin' WITH (FORMAT binary)
SELECT * FROM test_copy_null ORDER BY id;

-- ============================================================
-- test14: \copy binary UTF-8 中文 + 长字符串 + 特殊字符
-- ============================================================
\echo '=== test14: \\copy binary UTF-8, long strings, special chars ==='

DROP TABLE IF EXISTS test_copy_utf8 CASCADE;

CREATE TABLE test_copy_utf8 (
    id      INT4,
    name    TEXT_ENC    ENCRYPTED WITH (DEK = dek_copy_all, SUPPORT_ALGO = 'DET'),
    note    TEXT_ENC    ENCRYPTED WITH (DEK = dek_copy_all, SUPPORT_ALGO = 'DET')
);

-- UTF-8 中文、日文、emoji
INSERT INTO test_copy_utf8 VALUES (1, '你好世界', '日本語テスト');
INSERT INTO test_copy_utf8 VALUES (2, '🚀🎉💯', 'Emoji test');
INSERT INTO test_copy_utf8 VALUES (3, '混合Café中文rêve', 'Special quote test');

-- 空字符串（明文非NULL）
INSERT INTO test_copy_utf8 VALUES (4, '', '');

-- 长字符串（约200字节，含中文和ASCII）
INSERT INTO test_copy_utf8 VALUES (5,
    '中国中华人民共和国国务院中国人民代表委员会办公厅文件编号NO2024',
    'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789'
    '测试混合内容Café日本語🚀');

-- 更长字符串（约300字节）
INSERT INTO test_copy_utf8 VALUES (6,
    '1234567890!@#$%^&*()_+-=[]{}|;:,.<>?/~`',
    '😀😃😄😁😅😆😂🤣😊😇🙂🙃😉😌😍🥰😘'
    'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz');

\copy test_copy_utf8 TO 'copy_utf8.bin' WITH (FORMAT binary)
TRUNCATE test_copy_utf8;
\copy test_copy_utf8 FROM 'copy_utf8.bin' WITH (FORMAT binary)
SELECT * FROM test_copy_utf8 ORDER BY id;

DROP TABLE IF EXISTS test_copy_utf8 CASCADE;
DROP TABLE IF EXISTS test_copy_null CASCADE;
DROP TABLE IF EXISTS test_copy_all CASCADE;
DROP DEK IF EXISTS dek_copy_all CASCADE;
DROP KEK IF EXISTS kek_copy_all CASCADE;

-- ============================================================
-- test15: \\copy (SELECT ...) TO 子查询（Phase 1: 单表 SELECT）
-- ============================================================

-- test15a: SELECT * 子查询
\echo '=== test15a: \\copy (SELECT * FROM tab) TO ==='
\copy (SELECT * FROM test_copy_ext) TO 'copy_subq_star.txt'
\! cat copy_subq_star.txt

-- test15b: SELECT 显式列
\echo '=== test15b: \\copy (SELECT id, name FROM tab) TO ==='
\copy (SELECT id, name FROM test_copy_ext) TO 'copy_subq_cols.txt'
\! cat copy_subq_cols.txt

-- test15c: 单列
\echo '=== test15c: \\copy (SELECT id FROM tab) TO ==='
\copy (SELECT id FROM test_copy_ext) TO 'copy_subq_single.txt'
\! cat copy_subq_single.txt

-- test15d: SELECT 单列（非加密列不处理，验证不会报错）
\echo '=== test15d: \\copy (SELECT id FROM tab) TO (plain text) ==='
\copy (SELECT id FROM test_copy_ext) TO 'copy_subq_id.txt'
\! cat copy_subq_id.txt

-- test15e: CSV 格式子查询
\echo '=== test15e: \\copy (SELECT * FROM tab) TO CSV ==='
\copy (SELECT * FROM test_copy_ext) TO 'copy_subq_csv.txt' WITH (FORMAT CSV)
\! cat copy_subq_csv.txt

-- test15f: BINARY 格式子查询（导出+导入回环验证）
\echo '=== test15f: \\copy (SELECT * FROM tab) TO BINARY (roundtrip) ==='
\copy (SELECT * FROM test_copy_ext) TO 'copy_subq_bin.bin' WITH (FORMAT BINARY)
\echo '--- BINARY export done, truncate and import back ---'
TRUNCATE test_copy_ext;
\copy test_copy_ext FROM 'copy_subq_bin.bin' WITH (FORMAT BINARY)
\echo '--- Verify imported data ---'
SELECT * FROM test_copy_ext ORDER BY id;

-- ============================================================
-- test16: \\copy (SELECT ...) TO 子查询（Phase 2: JOIN + Phase 3: 聚合）
-- ============================================================

-- Phase 3: 聚合函数子查询
-- test16a: COUNT(*) 无加密列，验证聚合不报错
\echo '=== test16a: \\copy (SELECT COUNT(*) FROM tab) TO ==='
\copy (SELECT COUNT(*) FROM test_copy_ext) TO 'copy_subq_cnt.txt'
\! cat copy_subq_cnt.txt

-- test16b: COUNT(*) + GROUP BY 加密列，验证 COUNT(*) 不被当作加密列解密
\echo '=== test16b: \\copy (SELECT COUNT(*), id FROM tab GROUP BY id) TO ==='
\copy (SELECT COUNT(*), id FROM test_copy_ext GROUP BY id) TO 'copy_subq_group.txt'
\! cat copy_subq_group.txt

-- test16c: 加密列在前、聚合在后
\echo '=== test16c: \\copy (SELECT id, COUNT(*) FROM tab GROUP BY id) TO ==='
\copy (SELECT id, COUNT(*) FROM test_copy_ext GROUP BY id) TO 'copy_subq_mix.txt'
\! cat copy_subq_mix.txt

-- Phase 2: 多表 JOIN 子查询（目前仅第一张表的加密列可解密）
-- 创建第二张加密表
DROP TABLE IF EXISTS ext_info CASCADE;
CREATE TABLE ext_info (
    ref_id  INT4_ENC    ENCRYPTED WITH (DEK = dek_copy_ext, SUPPORT_ALGO = 'DET'),
    val     TEXT_ENC    ENCRYPTED WITH (DEK = dek_copy_ext, SUPPORT_ALGO = 'DET')
);
INSERT INTO ext_info VALUES (1, 'Engineer');
INSERT INTO ext_info VALUES (2, 'Manager');
INSERT INTO ext_info VALUES (3, 'Analyst');

-- test16d: JOIN 显式列（注意：当前仅第一张表的列可解密）
\echo '=== test16d: \\copy (SELECT e.id, e.name, i.val FROM ... JOIN) TO ==='
\copy (SELECT e.id, e.name, i.val FROM test_copy_ext e, ext_info i WHERE e.id = i.ref_id) TO 'copy_subq_join.txt'
\! cat copy_subq_join.txt

-- test16e: JOIN 的 SELECT e.*
\echo '=== test16e: \\copy (SELECT e.*, i.val FROM ... JOIN) TO ==='
\copy (SELECT e.*, i.val FROM test_copy_ext e, ext_info i WHERE e.id = i.ref_id) TO 'copy_subq_join_star.txt'
\! cat copy_subq_join_star.txt

DROP TABLE IF EXISTS ext_info CASCADE;

-- ============================================================
-- 清理所有测试资源
-- ============================================================
DROP TABLE IF EXISTS test_copy_ext CASCADE;
DROP DEK IF EXISTS dek_copy_ext CASCADE;
DROP KEK IF EXISTS kek_copy_ext CASCADE;
