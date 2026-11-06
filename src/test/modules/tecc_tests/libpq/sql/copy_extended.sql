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

-- test9a: COPY TO 服务端文件（输出密文）
\echo '=== test9a: COPY TO server file ==='
COPY test_srv TO '/tmp/srv_copy_out.txt';
\! head -3 /tmp/srv_copy_out.txt

-- test9b: COPY FROM 服务端文件导入
\echo '=== test9b: COPY FROM server file ==='
DELETE FROM test_srv;
COPY test_srv FROM '/tmp/srv_copy_out.txt';
SELECT * FROM test_srv;

-- test9c: COPY TO 显式列列表
\echo '=== test9c: COPY TO with column list ==='
COPY test_srv (id, name) TO '/tmp/srv_copy_cols.txt';
\! head -3 /tmp/srv_copy_cols.txt

-- test9d: COPY FROM 显式列列表
\echo '=== test9d: COPY FROM with column list ==='
DELETE FROM test_srv;
COPY test_srv (id, name) FROM '/tmp/srv_copy_cols.txt';
SELECT * FROM test_srv;

-- test9e: COPY TO CSV 格式（服务端密文）
\echo '=== test9e: COPY TO CSV format (ciphertext) ==='
COPY test_srv TO '/tmp/srv_copy_csv.txt' WITH (FORMAT CSV);
\! head -3 /tmp/srv_copy_csv.txt

-- test9f: COPY FROM CSV 格式
\echo '=== test9f: COPY FROM CSV format ==='
DELETE FROM test_srv;
COPY test_srv FROM '/tmp/srv_copy_csv.txt' WITH (FORMAT CSV);
SELECT * FROM test_srv;

-- test9g: COPY TO 默认格式+WITH选项 (DELIMITER)
\echo '=== test9g: COPY TO with delimiter ==='
COPY test_srv TO '/tmp/srv_copy_delim.txt' WITH (DELIMITER '|');
\! head -3 /tmp/srv_copy_delim.txt

-- test9h: COPY FROM 自定义分隔符
\echo '=== test9h: COPY FROM with custom delimiter ==='
DELETE FROM test_srv;
COPY test_srv FROM '/tmp/srv_copy_delim.txt' WITH (DELIMITER '|');
SELECT * FROM test_srv;

DROP TABLE IF EXISTS test_srv CASCADE;
DROP DEK IF EXISTS dek_srv CASCADE;
DROP KEK IF EXISTS kek_srv CASCADE;

-- ============================================================
-- 清理
-- ============================================================
DROP TABLE IF EXISTS test_copy_ext CASCADE;
DROP DEK IF EXISTS dek_copy_ext CASCADE;
DROP KEK IF EXISTS kek_copy_ext CASCADE;
