-- ============================================================
-- enc_indexes.sql — 密态索引/执行计划测试
-- 覆盖 test_index, test_index_usage,
--   test_idx_plan, test_index_explain, test_explain
-- ============================================================

-- ============================================================
-- 0. Setup
-- ============================================================
DROP KEK IF EXISTS kek_idx CASCADE;

CREATE KEK kek_idx WITH (KEY_STORE = 'localkms', KEY_PATH = '/tmp/kek_idx.key');

CREATE DEK dek_idx WITH VALUES(KEK = kek_idx);

-- ============================================================
-- 1a. SEARCH算法仅限字符类型 — int4_enc + SEARCH 应报错
-- ============================================================
SELECT '=== 1a SEARCH on int4 — expect error ===';

DROP TABLE IF EXISTS test_search_int CASCADE;

CREATE TABLE test_search_int (
    id INT4_ENC ENCRYPTED WITH (DEK = dek_idx, SUPPORT_ALGO = 'DET,SEARCH')
);

SELECT '1a_search_int_error', 'error expected above' AS result;

-- ============================================================
-- 1b. Hash index on encrypted columns (修正: int列用OPE, 字符列保留SEARCH)
-- ============================================================
SELECT '=== 1b Hash Index ===';

DROP TABLE IF EXISTS test_index_all CASCADE;

CREATE TABLE test_index_all (
    id          INT4_ENC    ENCRYPTED WITH (DEK = dek_idx, SUPPORT_ALGO = 'OPE'),
    val         INT4_ENC    ENCRYPTED WITH (DEK = dek_idx, SUPPORT_ALGO = 'OPE'),
    name        TEXT_ENC    ENCRYPTED WITH (DEK = dek_idx, SUPPORT_ALGO = 'DET,SEARCH'),
    comment     VARCHAR_ENC ENCRYPTED WITH (DEK = dek_idx, SUPPORT_ALGO = 'DET,SEARCH')
);

CREATE INDEX idx_hash_id ON test_index_all USING hash (id);

CREATE INDEX idx_hash_val ON test_index_all USING hash (val);

CREATE INDEX idx_hash_name ON test_index_all USING hash (name);

CREATE INDEX idx_hash_comment ON test_index_all USING hash (comment);

-- @param String test_index_all
SELECT '1b_hash_idx_created', count(*) AS idx_cnt FROM pg_indexes
WHERE tablename = ?;

DROP TABLE IF EXISTS test_index_all CASCADE;

-- ============================================================
-- 2. B-tree index + query with encrypted columns (test_index_usage)
-- ============================================================
SELECT '=== 2 B-tree Index + Query ===';

DROP TABLE IF EXISTS test_index_usage CASCADE;

CREATE TABLE test_index_usage (
    id    INT4_ENC    ENCRYPTED WITH (DEK = dek_idx, SUPPORT_ALGO = 'OPE'),
    val   INT4_ENC    ENCRYPTED WITH (DEK = dek_idx, SUPPORT_ALGO = 'OPE'),
    name  TEXT_ENC    ENCRYPTED WITH (DEK = dek_idx, SUPPORT_ALGO = 'DET,SEARCH')
);

-- @param int 10
-- @param int 100
-- @param String Name_10
-- @param int 20
-- @param int 200
-- @param String Name_20
-- @param int 30
-- @param int 300
-- @param String Name_30
-- @param int 40
-- @param int 400
-- @param String Name_40
-- @param int 50
-- @param int 500
-- @param String Name_50
-- @param int 60
-- @param int 600
-- @param String Name_60
-- @param int 70
-- @param int 700
-- @param String Name_70
-- @param int 80
-- @param int 800
-- @param String Name_80
-- @param int 90
-- @param int 900
-- @param String Name_90
-- @param int 100
-- @param int 1000
-- @param String Name_100
INSERT INTO test_index_usage (id, val, name) VALUES
(?, ?, ?), (?, ?, ?), (?, ?, ?),
(?, ?, ?), (?, ?, ?), (?, ?, ?),
(?, ?, ?), (?, ?, ?), (?, ?, ?),
(?, ?, ?);

CREATE INDEX idx_usage_btree_id ON test_index_usage USING btree (id);

CREATE INDEX idx_usage_btree_val ON test_index_usage USING btree (val);

CREATE INDEX idx_usage_hash_id ON test_index_usage USING hash (id);

CREATE INDEX idx_usage_hash_name ON test_index_usage USING hash (name);

-- Queries
-- @param int 50
SELECT '2a_eq_id', * FROM test_index_usage WHERE id = ?;

-- @param String Name_50
SELECT '2b_eq_name', * FROM test_index_usage WHERE name = ?;

-- @param int 500
SELECT '2c_range_gt', * FROM test_index_usage WHERE val > ? ORDER BY val;

-- @param int 30
SELECT '2d_range_lt', * FROM test_index_usage WHERE id < ? ORDER BY id;

-- @param int 20
-- @param int 80
SELECT '2e_range_between', * FROM test_index_usage WHERE id > ? AND id < ? ORDER BY id;

DROP TABLE IF EXISTS test_index_usage CASCADE;

-- ============================================================
-- 3. B-tree index + EXPLAIN (test_index_explain, test_explain)
-- ============================================================
SELECT '=== 3 Index + EXPLAIN ===';

DROP TABLE IF EXISTS test_index_plan CASCADE;

CREATE TABLE test_index_plan (
    id    INT4_ENC    ENCRYPTED WITH (DEK = dek_idx, SUPPORT_ALGO = 'OPE'),
    val   INT4_ENC    ENCRYPTED WITH (DEK = dek_idx, SUPPORT_ALGO = 'OPE'),
    name  TEXT_ENC    ENCRYPTED WITH (DEK = dek_idx, SUPPORT_ALGO = 'DET,SEARCH')
);

-- @param int 10
-- @param int 100
-- @param String Name_10
-- @param int 20
-- @param int 200
-- @param String Name_20
-- @param int 30
-- @param int 300
-- @param String Name_30
-- @param int 40
-- @param int 400
-- @param String Name_40
-- @param int 50
-- @param int 500
-- @param String Name_50
-- @param int 60
-- @param int 600
-- @param String Name_60
-- @param int 70
-- @param int 700
-- @param String Name_70
-- @param int 80
-- @param int 800
-- @param String Name_80
-- @param int 90
-- @param int 900
-- @param String Name_90
-- @param int 100
-- @param int 1000
-- @param String Name_100
INSERT INTO test_index_plan (id, val, name) VALUES
(?, ?, ?), (?, ?, ?), (?, ?, ?),
(?, ?, ?), (?, ?, ?), (?, ?, ?),
(?, ?, ?), (?, ?, ?), (?, ?, ?),
(?, ?, ?);

CREATE INDEX idx_btree_id ON test_index_plan USING btree (id);

CREATE INDEX idx_btree_val ON test_index_plan USING btree (val);

CREATE INDEX idx_hash_id ON test_index_plan USING hash (id);

CREATE INDEX idx_hash_name ON test_index_plan USING hash (name);

-- Verify indexes exist
-- @param String test_index_plan
SELECT indexname, indexdef FROM pg_indexes WHERE tablename = ?;

-- Query verification with indexes
-- @param int 50
SELECT '3a_eq_id', * FROM test_index_plan WHERE id = ?;

-- @param String Name_50
SELECT '3b_eq_name', * FROM test_index_plan WHERE name = ?;

-- @param int 500
SELECT '3c_range_gt', * FROM test_index_plan WHERE val > ? ORDER BY val;

-- @param int 1000
SELECT '3d_range_ge', * FROM test_index_plan WHERE val >= ? ORDER BY val;

-- @param int 30
SELECT '3e_range_lt', * FROM test_index_plan WHERE id < ? ORDER BY id;

-- @param int 20
-- @param int 80
SELECT '3f_range_between', * FROM test_index_plan WHERE id > ? AND id < ? ORDER BY id;

-- EXPLAIN (COSTS OFF)
EXPLAIN (COSTS OFF) SELECT * FROM test_index_plan WHERE id = 50;

EXPLAIN (COSTS OFF) SELECT * FROM test_index_plan WHERE val > 300 ORDER BY val;

EXPLAIN (COSTS OFF) SELECT * FROM test_index_plan WHERE name = 'Name_30';

-- Normal SELECT unaffected
-- @param int 50
SELECT '3g_ctrl_select', * FROM test_index_plan WHERE id = ?;

DROP TABLE test_index_plan CASCADE;

-- ============================================================
-- 4. B-tree index on OPE columns + basic queries (test_idx_plan)
-- ============================================================
SELECT '=== 4 B-tree on OPE + Basic ===';

DROP TABLE IF EXISTS test_idx CASCADE;

CREATE TABLE test_idx (
    id INT4_ENC ENCRYPTED WITH (DEK = dek_idx, SUPPORT_ALGO = 'OPE'),
    val INT4_ENC ENCRYPTED WITH (DEK = dek_idx, SUPPORT_ALGO = 'OPE')
);

-- @param int 10
-- @param int 100
-- @param int 20
-- @param int 200
-- @param int 30
-- @param int 300
-- @param int 40
-- @param int 400
-- @param int 50
-- @param int 500
INSERT INTO test_idx (id, val) VALUES (?, ?), (?, ?), (?, ?), (?, ?), (?, ?);

CREATE INDEX idx_id ON test_idx USING btree (id);

CREATE INDEX idx_val ON test_idx USING btree (val);

-- @param String test_idx
SELECT '4a_idx_created', count(*) AS idx_cnt FROM pg_indexes WHERE tablename = ?;

DROP TABLE IF EXISTS test_idx CASCADE;

-- ============================================================
-- Cleanup
-- ============================================================
SELECT '=== Cleanup ===';

DROP KEK IF EXISTS kek_idx CASCADE;
