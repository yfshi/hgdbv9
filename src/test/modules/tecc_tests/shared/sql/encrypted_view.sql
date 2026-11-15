--
-- encrypted_view.sql — 密态视图（普通视图 + 物化视图）测试
--
-- 覆盖：
--   1. 普通视图：定义中密态列过滤（DET）、查询解密、OR REPLACE、
--      列名列表、查询时追加过滤、边界报错（表达式/排序）
--   2. 物化视图：定义中密态列过滤（DET）、查询解密、元数据继承、
--      REFRESH 同步、WITH NO DATA、查询时追加过滤、边界报错
--

-- ============================================================
-- 0. Setup — KEK/DEK/加密基表（DET 算法用于等值过滤）
-- ============================================================
DROP VIEW IF EXISTS v_enc, v_enc2, v_plain CASCADE;
DROP MATERIALIZED VIEW IF EXISTS mv_enc, mv_enc2, mv_empty CASCADE;
DROP TABLE IF EXISTS ev_base CASCADE;
DROP DEK IF EXISTS dek_ev CASCADE;
DROP KEK IF EXISTS kek_ev CASCADE;

CREATE KEK kek_ev WITH (
    key_store = 'localkms',
    key_path = '/tmp/kek_ev.key'
);
CREATE DEK dek_ev WITH VALUES(KEK = kek_ev);

-- 一列多 DEK：HE 需要 asym KEK（he-add）+ sym KEK（对称 DEK）
\! rm -f "$TECC_KEK_PATH/ev_he.priv" "$TECC_KEK_PATH/ev_he.pub" "$TECC_KEK_PATH/ev_hm.priv" "$TECC_KEK_PATH/ev_hm.pub" 2>/dev/null
\! tecc_ktool generate --key-name ev_he --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1
\! tecc_ktool generate --key-name ev_hm --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1
CREATE KEK kek_ev_he WITH (
    key_store = 'localkms',
    key_algo = 'sm2',
    key_path = '%KEKPATH%/ev_he.priv'
);
CREATE KEK kek_ev_hm WITH (
    key_store = 'localkms',
    key_algo = 'sm2',
    key_path = '%KEKPATH%/ev_hm.priv'
);
CREATE DEK dek_ev_he WITH VALUES(KEK = kek_ev_he, KEY_ALGO = 'he-add');
CREATE DEK dek_ev_hm WITH VALUES(KEK = kek_ev_hm, KEY_ALGO = 'he-mul');

CREATE TABLE ev_base (
    id   int,
    name text_enc ENCRYPTED WITH (DEK = 'dek_ev', SUPPORT_ALGO = 'DET')
);
INSERT INTO ev_base VALUES (1, 'Alice'), (2, 'Bob'), (3, 'Charlie');

-- ============================================================
-- 1. 普通视图 — 定义中密态列过滤
-- ============================================================
SELECT '=== 1 普通视图: 定义中 WHERE 密态列 ===';

CREATE VIEW v_enc AS
    SELECT id, name FROM ev_base WHERE name = 'Alice';
SELECT id, name FROM v_enc;

-- ============================================================
-- 2. 普通视图 — 查询时追加过滤（客户端改写加密）
-- ============================================================
SELECT '=== 2 普通视图: 查询时追加过滤 ===';

SELECT id, name FROM v_enc WHERE name = 'Alice';
SELECT id, name FROM v_enc WHERE id = 2;

-- ============================================================
-- 3. 普通视图 — OR REPLACE / 列名列表 变体
-- ============================================================
SELECT '=== 3 普通视图: OR REPLACE / 列名列表 ===';

CREATE OR REPLACE VIEW v_enc AS
    SELECT id, name FROM ev_base WHERE name = 'Bob';
SELECT id, name FROM v_enc;

CREATE VIEW v_enc2 (eid, ename) AS
    SELECT id, name FROM ev_base WHERE id = 3;
SELECT eid, ename FROM v_enc2;

-- ============================================================
-- 3.1 Bug 9153 — OR REPLACE 追加密态列（新增 int4_enc 列）
-- ============================================================
SELECT '=== 3.1 Bug 9153: OR REPLACE 追加密态列 ===';

-- 基表含 int4_enc 列
CREATE TABLE ev_orep (
    col_int int,
    col_det text_enc ENCRYPTED WITH (DEK = 'dek_ev', SUPPORT_ALGO = 'DET'),
    col_ope int4_enc ENCRYPTED WITH (DEK = 'dek_ev', SUPPORT_ALGO = 'OPE')
);
INSERT INTO ev_orep VALUES (1,'Alice',10),(2,'Bob',20);

-- 无密态列 → 追加 int4_enc 列
CREATE VIEW v_orep AS SELECT col_int FROM ev_orep WHERE col_int=2;
SELECT * FROM v_orep;
CREATE OR REPLACE VIEW v_orep AS SELECT col_int,col_ope FROM ev_orep;
SELECT * FROM v_orep ORDER BY col_int;
DROP VIEW v_orep;

-- 有 text_enc → 追加 int4_enc 列
CREATE VIEW v_orep2 AS SELECT col_int,col_det FROM ev_orep WHERE col_det='Alice';
SELECT * FROM v_orep2;
CREATE OR REPLACE VIEW v_orep2 AS SELECT col_int,col_det,col_ope FROM ev_orep;
SELECT * FROM v_orep2 ORDER BY col_int;
DROP VIEW v_orep2;

DROP TABLE ev_orep;

-- ============================================================
-- 4. 普通视图 — 无加密列（pass through）
-- ============================================================
SELECT '=== 4 普通视图: 无加密列 ===';

CREATE VIEW v_plain AS SELECT 1 AS x;
SELECT * FROM v_plain;

-- ============================================================
-- 5. 普通视图 — 边界报错（与直接查询基表一致）
-- ============================================================
SELECT '=== 5 普通视图: 边界报错 ===';

-- 表达式运算
CREATE VIEW v_expr AS
    SELECT id, name || 'X' AS n FROM ev_base;

-- 排序（Bug 9151: 视图创建时前端校验 OPE，DET-only 列报错）
CREATE VIEW v_order AS
    SELECT id, name FROM ev_base ORDER BY name;

-- ============================================================
-- 6. 物化视图 — 定义中密态列过滤
-- ============================================================
SELECT '=== 6 物化视图: 定义中 WHERE 密态列 ===';

CREATE MATERIALIZED VIEW mv_enc AS
    SELECT id, name FROM ev_base WHERE name = 'Alice';
SELECT id, name FROM mv_enc;

-- ============================================================
-- 7. 物化视图 — 元数据继承
-- ============================================================
SELECT '=== 7 物化视图: 元数据继承 ===';

SELECT c.column_name, c.dek_name, c.encryption_type
FROM hg_encrypted_columns_info c
JOIN pg_class pc ON pc.oid = c.table_oid
WHERE pc.relname = 'mv_enc';

-- ============================================================
-- 7.1 一列多 DEK — 元数据读取 + DROP 清理（ReqID:TECC4.0 P0-2）
--     视图 he_add_dek/he_mul_dek 解析 + DROP 级联清理 args
-- ============================================================
SELECT '=== 7.1 一列多 DEK: 视图读取 ===';

DROP TABLE IF EXISTS ev_multi CASCADE;
CREATE TABLE ev_multi (
    id int,
    c  int4_enc ENCRYPTED WITH (DEK = 'dek_ev', HE_ADD_DEK = 'dek_ev_he', HE_MUL_DEK = 'dek_ev_hm', SUPPORT_ALGO = 'det,he-add,he-mul')
);
INSERT INTO ev_multi VALUES (1, 100);

-- 视图读取：he_add_dek/he_mul_dek 应解析出 DEK 名
SELECT c.column_name, c.dek_name, c.he_add_dek, c.he_mul_dek
FROM hg_encrypted_columns_info c
JOIN pg_class pc ON pc.oid = c.table_oid
WHERE pc.relname = 'ev_multi';

-- args 表应写入 2 条绑定
SELECT count(*) FROM hg_encrypted_column_args
WHERE column_id = (SELECT oid FROM hg_encrypted_columns
                   WHERE table_oid = 'ev_multi'::regclass AND column_name = 'c');

-- DROP 前捕获 column_id 到临时表，供 DROP 后清理校验
SELECT oid INTO TEMP ev_multi_colid FROM hg_encrypted_columns
WHERE table_oid = 'ev_multi'::regclass AND column_name = 'c';

-- DROP 后 args 应级联清理（无孤儿记录）
DROP TABLE ev_multi;
SELECT count(*) FROM hg_encrypted_column_args
WHERE column_id IN (SELECT oid FROM ev_multi_colid);

-- ============================================================
-- 8. 物化视图 — REFRESH 同步
-- ============================================================
SELECT '=== 8 物化视图: REFRESH 同步 ===';

CREATE MATERIALIZED VIEW mv_enc2 AS
    SELECT id, name FROM ev_base WHERE id > 1;
SELECT id, name FROM mv_enc2 ORDER BY id;

INSERT INTO ev_base VALUES (4, 'David');
REFRESH MATERIALIZED VIEW mv_enc2;
SELECT id, name FROM mv_enc2 ORDER BY id;

-- REFRESH 后元数据保持
SELECT c.column_name, c.dek_name
FROM hg_encrypted_columns_info c
JOIN pg_class pc ON pc.oid = c.table_oid
WHERE pc.relname = 'mv_enc2';

-- ============================================================
-- 9. 物化视图 — WITH NO DATA
-- ============================================================
SELECT '=== 9 物化视图: WITH NO DATA ===';

CREATE MATERIALIZED VIEW mv_empty AS
    SELECT id, name FROM ev_base WHERE id > 2 WITH NO DATA;
SELECT count(*) FROM mv_empty;
REFRESH MATERIALIZED VIEW mv_empty;
SELECT id, name FROM mv_empty ORDER BY id;

-- ============================================================
-- 10. 物化视图 — 查询时追加过滤
-- ============================================================
SELECT '=== 10 物化视图: 查询时追加过滤 ===';

SELECT id, name FROM mv_enc2 WHERE name = 'Bob';

-- ============================================================
-- 11. 物化视图 — 边界报错（与直接查询基表一致）
-- ============================================================
SELECT '=== 11 物化视图: 边界报错 ===';

-- 表达式运算
CREATE MATERIALIZED VIEW mv_expr AS
    SELECT id, name || 'X' AS n FROM ev_base;

-- 排序（创建失败：服务端无 OPE 密文）
CREATE MATERIALIZED VIEW mv_order AS
    SELECT id, name FROM ev_base ORDER BY name;

-- ============================================================
-- 清理
-- ============================================================
DROP VIEW v_enc, v_enc2, v_plain, v_order CASCADE;
DROP MATERIALIZED VIEW mv_enc, mv_enc2, mv_empty CASCADE;
DROP TABLE ev_base CASCADE;
DROP DEK dek_ev CASCADE;
DROP KEK kek_ev CASCADE;
DROP DEK dek_ev_he CASCADE;
DROP DEK dek_ev_hm CASCADE;
DROP KEK kek_ev_he CASCADE;
DROP KEK kek_ev_hm CASCADE;
\! rm -f "$TECC_KEK_PATH/ev_he.priv" "$TECC_KEK_PATH/ev_he.pub" "$TECC_KEK_PATH/ev_hm.priv" "$TECC_KEK_PATH/ev_hm.pub" 2>/dev/null
