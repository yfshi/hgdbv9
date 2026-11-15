--
-- copy_test.sql — 服务端 COPY（SQL 命令）密态数据加解密测试
-- \copy（客户端元命令）测试见 libpq/sql/test_copy_extended.sql
--
DROP TABLE IF EXISTS test_copy_enc CASCADE;

DROP DEK IF EXISTS dek_copy_enc CASCADE;

DROP KEK IF EXISTS kek_copy_enc CASCADE;

CREATE KEK kek_copy_enc WITH (
    key_store = 'localkms',
    key_path = '/tmp/kek_copy_enc.key'
);

CREATE DEK dek_copy_enc WITH VALUES(KEK = 'kek_copy_enc');

CREATE TABLE test_copy_enc (
    id INT4_ENC ENCRYPTED WITH (DEK = dek_copy_enc, SUPPORT_ALGO = 'DET'),
    name TEXT_ENC ENCRYPTED WITH (DEK = dek_copy_enc, SUPPORT_ALGO = 'DET,SEARCH')
);

-- @param int 1
-- @param String Alice
INSERT INTO test_copy_enc VALUES (?, ?);

-- @param int 2
-- @param String Bob
INSERT INTO test_copy_enc VALUES (?, ?);

-- @param int 3
-- @param String Charlie
INSERT INTO test_copy_enc VALUES (?, ?);

-- 验证原始数据
SELECT id, name FROM test_copy_enc;

-- 服务端 COPY TO: 导出到服务端文件（写密文）
COPY test_copy_enc TO '/tmp/copy_enc_server.txt';

-- 清理数据
TRUNCATE test_copy_enc;

-- 服务端 COPY FROM: 从密文文件导入
COPY test_copy_enc FROM '/tmp/copy_enc_server.txt';

SELECT id, name FROM test_copy_enc;

-- ============================================================
-- COPY (SELECT ...) TO STDOUT
-- ============================================================
SELECT '=== COPY (SELECT ...) TO STDOUT ===';

DROP TABLE IF EXISTS test_cp_select CASCADE;

CREATE TABLE test_cp_select (
    id INT4,
    name TEXT,
    salary INT4_ENC ENCRYPTED WITH (DEK = 'dek_copy_enc', SUPPORT_ALGO = 'DET')
);

-- @param int 1
-- @param String Alice
-- @param int 50000
INSERT INTO test_cp_select VALUES (?, ?, ?);

-- @param int 2
-- @param String Bob
-- @param int 60000
INSERT INTO test_cp_select VALUES (?, ?, ?);

COPY (SELECT id, name FROM test_cp_select ORDER BY id) TO STDOUT;

DROP TABLE IF EXISTS test_cp_select CASCADE;

-- 清理
DROP TABLE IF EXISTS test_copy_enc CASCADE;

DROP DEK IF EXISTS dek_copy_enc CASCADE;

DROP KEK IF EXISTS kek_copy_enc CASCADE;
