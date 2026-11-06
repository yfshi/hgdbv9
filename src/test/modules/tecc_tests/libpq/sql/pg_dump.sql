--
-- pg_dump.sql — pg_dump/pg_restore 密态表导出导入验证
--

-- ============================================================
-- 准备
-- ============================================================
DROP TABLE IF EXISTS t_pgdump_no_ope CASCADE;
DROP TABLE IF EXISTS t_pgdump_ope CASCADE;
DROP TABLE IF EXISTS test_pg_dump CASCADE;
DROP TABLE IF EXISTS tb_all CASCADE;
DROP DEK IF EXISTS dek_pgdump CASCADE;
DROP KEK IF EXISTS kek_pgdump CASCADE;

CREATE KEK kek_pgdump WITH (
    key_store = 'localkms',
    key_path = '/tmp/kek_pgdump.key'
);
CREATE DEK dek_pgdump WITH VALUES(KEK = 'kek_pgdump');

-- ============================================================
-- 表1: 无OPE列 (schema-only — SUPPORT_ALGO 子句验证)
-- ============================================================
CREATE TABLE t_pgdump_no_ope (
    id   SERIAL PRIMARY KEY,
    name TEXT_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'DET,SEARCH'),
    val  INT4_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'DET'),
    note TEXT_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump')
);
INSERT INTO t_pgdump_no_ope (name, val, note) VALUES
('Alice', 100, 'first'), ('Bob', 200, 'second'), ('Charlie', 300, 'third');

-- ============================================================
-- 表2: 含OPE列 (schema-only — COLLATE 子句验证)
-- ============================================================
CREATE TABLE t_pgdump_ope (
    id     SERIAL PRIMARY KEY,
    txt    TEXT_ENC        ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'OPE'),
    vc     VARCHAR_ENC(50) ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'OPE'),
    bp     BPCHAR_ENC      ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'OPE'),
    amount INT4_ENC        ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'OPE')
);
INSERT INTO t_pgdump_ope (txt, vc, bp, amount) VALUES
('apple', 'red', 'A', 10), ('banana', 'yellow', 'B', 20), ('cherry', 'red', 'C', 30);

-- ============================================================
-- 表3: DET + SEARCH (data-only + restore, from inline test)
-- ============================================================
CREATE TABLE test_pg_dump (
    id   INT4_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'DET'),
    name TEXT_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'DET,SEARCH')
);
INSERT INTO test_pg_dump VALUES (1, 'Alice'), (2, 'Bob'), (3, 'Charlie');

-- ============================================================
-- 表4: 全9种加密类型 (data-only + restore, Bug 8047)
-- ============================================================
CREATE TABLE tb_all (
    id          INT,
    col_text    TEXT_ENC    ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_varchar VARCHAR_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_bpchar  BPCHAR_ENC  ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_int2    INT2_ENC    ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_int4    INT4_ENC    ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_int8    INT8_ENC    ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_numeric NUMERIC_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_float4  FLOAT4_ENC  ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_float8  FLOAT8_ENC  ENCRYPTED WITH (DEK = 'public.dek_pgdump')
);
INSERT INTO tb_all VALUES (1, 'text_value', 'varchar_value', 'bpchar_val',
    123::INT2, 456789::INT4, 9876543210::INT8, 12345.67::NUMERIC,
    3.14::FLOAT4, 2.718281828::FLOAT8);

-- ============================================================
-- schema-only
-- ============================================================
\echo '=== 1 schema-only no OPE ==='
\! pg_dump -t t_pgdump_no_ope --schema-only --no-owner 2>&1

\echo '=== 2 schema-only OPE ==='
\! pg_dump -t t_pgdump_ope --schema-only --no-owner 2>&1

-- ============================================================
-- data-only + restore
-- ============================================================
\echo '=== 3 data-only test_pg_dump ==='
\! pg_dump -t test_pg_dump --data-only 2>&1
\! pg_dump -t test_pg_dump --data-only -f tecc_pgdump_data.sql 2>&1
TRUNCATE test_pg_dump;
\i tecc_pgdump_data.sql
\echo 'restore test_pg_dump:'
SELECT id, name FROM test_pg_dump ORDER BY id;

\echo '=== 4 data-only tb_all (9 types) ==='
\! pg_dump -t tb_all --data-only 2>&1
\! pg_dump -t tb_all --data-only -f tecc_pgdump_9col.sql 2>&1
DELETE FROM tb_all;
\i tecc_pgdump_9col.sql
\echo 'restore tb_all:'
SELECT * FROM tb_all;

-- ============================================================
-- pg_dump -Fc (custom format) + pg_restore
-- ============================================================

-- pg_dump/pg_restore 通过 psql -At 获取数据库名（\! 中 :variable 不展开）

-- ============================================================
-- -Fc (custom format) + pg_restore 测试
-- 每个子节自包含：建表→dump→drop→restore→验证→清理
-- NOTE: \i 导入 pg_dump 输出会把 search_path 清空，需手动恢复
-- ============================================================
SET search_path TO public;

-- 5a: -Fc data-only (验证 COPY 加密走 PQputCopyData)
\echo '=== 5a pg_dump -Fc --data-only + pg_restore --data-only ==='
DROP TABLE IF EXISTS t_fc_data CASCADE;
CREATE TABLE public.t_fc_data (
    id   INT4_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'DET'),
    name TEXT_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'DET,SEARCH')
);
INSERT INTO t_fc_data VALUES (10, 'Alice'), (20, 'Bob');
\! DB=$(psql -At -c "SELECT current_database()"); pg_dump -d "$DB" -t t_fc_data --data-only -Fc -f tecc_fc_data.dump 2>&1
TRUNCATE t_fc_data;
\! DB=$(psql -At -c "SELECT current_database()"); pg_restore -d "$DB" --data-only tecc_fc_data.dump 2>&1
\echo 'restore:'
SELECT id, name FROM t_fc_data;
DROP TABLE t_fc_data CASCADE;

-- 5b: -Fc full (schema+data, 验证 search_path 空时 DEK 查找)
\echo '=== 5b pg_dump -Fc full + pg_restore full ==='
DROP TABLE IF EXISTS t_fc_full CASCADE;
CREATE TABLE public.t_fc_full (
    id   INT4_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'DET'),
    name TEXT_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'DET,SEARCH')
);
INSERT INTO t_fc_full VALUES (100, 'Alice'), (200, 'Bob');
\! DB=$(psql -At -c "SELECT current_database()"); pg_dump -d "$DB" -t t_fc_full -Fc -f tecc_fc_full.dump 2>&1
DROP TABLE t_fc_full CASCADE;
\! DB=$(psql -At -c "SELECT current_database()"); pg_restore -d "$DB" tecc_fc_full.dump 2>&1
\echo 'restore:'
SELECT id, name FROM t_fc_full;
DROP TABLE t_fc_full CASCADE;

-- 5c: -Fc full + 全9种加密类型
\echo '=== 5c pg_dump -Fc full 9 types + pg_restore ==='
DROP TABLE IF EXISTS t_fc_9col CASCADE;
CREATE TABLE public.t_fc_9col (
    id          INT,
    col_text    TEXT_ENC    ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_varchar VARCHAR_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_bpchar  BPCHAR_ENC  ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_int2    INT2_ENC    ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_int4    INT4_ENC    ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_int8    INT8_ENC    ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_numeric NUMERIC_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_float4  FLOAT4_ENC  ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_float8  FLOAT8_ENC  ENCRYPTED WITH (DEK = 'public.dek_pgdump')
);
INSERT INTO t_fc_9col VALUES (1, 'text_value', 'varchar_value', 'bpchar_val',
    123::INT2, 456789::INT4, 9876543210::INT8, 12345.67::NUMERIC,
    3.14::FLOAT4, 2.718281828::FLOAT8);
\! DB=$(psql -At -c "SELECT current_database()"); pg_dump -d "$DB" -t t_fc_9col -Fc -f tecc_fc_9col.dump 2>&1
DROP TABLE t_fc_9col CASCADE;
\! DB=$(psql -At -c "SELECT current_database()"); pg_restore -d "$DB" tecc_fc_9col.dump 2>&1
\echo 'restore:'
SELECT * FROM t_fc_9col;
DROP TABLE t_fc_9col CASCADE;

-- ============================================================
-- pg_restore 权限验证（DEK USAGE 不足时拒绝）
-- ============================================================
\echo '=== 6 pg_restore ACL: DEK permission denied ==='
DROP TABLE IF EXISTS t_acl CASCADE;
CREATE TABLE public.t_acl (
    id   INT4_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'DET'),
    name TEXT_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'DET,SEARCH')
);
INSERT INTO t_acl VALUES (1, 'Alice'), (2, 'Bob');

DROP USER IF EXISTS regress_pgdump_user;
CREATE USER regress_pgdump_user;
GRANT SELECT ON t_acl TO regress_pgdump_user;
-- 撤销 DEK 权限
REVOKE ALL ON DEK dek_pgdump FROM regress_pgdump_user;
REVOKE ALL ON KEK kek_pgdump FROM regress_pgdump_user;
-- dump
\! DB=$(psql -At -c "SELECT current_database()"); pg_dump -d "$DB" -U regress_pgdump_user -t t_acl -Fc -f tecc_acl.dump 2>&1
DROP TABLE t_acl CASCADE;
-- restore（无 DEK 权限，应报 permission denied）
\! DB=$(psql -At -c "SELECT current_database()"); pg_restore -d "$DB" -U regress_pgdump_user tecc_acl.dump 2>&1
\echo '--- grant DEK USAGE ---'
GRANT USAGE ON DEK dek_pgdump TO regress_pgdump_user;
GRANT USAGE ON KEK kek_pgdump TO regress_pgdump_user;
GRANT ALL ON SCHEMA public TO regress_pgdump_user;
\! DB=$(psql -At -c "SELECT current_database()"); pg_restore -d "$DB" -U regress_pgdump_user tecc_acl.dump 2>&1
\echo 'restore with ACL:'
SELECT id, name FROM t_acl;
DROP TABLE IF EXISTS t_acl CASCADE;
DROP USER IF EXISTS regress_pgdump_user;

-- ============================================================
-- 清理
-- ============================================================
DROP TABLE IF EXISTS t_pgdump_no_ope CASCADE;
DROP TABLE IF EXISTS t_pgdump_ope CASCADE;
DROP TABLE IF EXISTS test_pg_dump CASCADE;
DROP TABLE IF EXISTS tb_all CASCADE;
DROP DEK IF EXISTS dek_pgdump CASCADE;
DROP KEK IF EXISTS kek_pgdump CASCADE;
