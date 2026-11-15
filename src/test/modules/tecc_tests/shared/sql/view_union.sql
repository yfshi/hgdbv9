-- ============================================================
-- view_union.sql — 视图 UNION ALL 带密态列回归测试
-- 覆盖 Bug 9149: CREATE VIEW UNION ALL 合并两个带密态列结果集
-- ============================================================

-- ============================================================
-- 0. Setup
-- ============================================================
CREATE KEK kek_vu WITH (key_store = 'localkms', key_path = '/tmp/kek_vu.key');
CREATE DEK dek_vu_det WITH VALUES(KEK = kek_vu);

CREATE TABLE t_vu1 (id int, col_det text_enc encrypted with(dek=dek_vu_det, support_algo='det'));
CREATE TABLE t_vu2 (id int, col_det text_enc encrypted with(dek=dek_vu_det, support_algo='det'));
INSERT INTO t_vu1 VALUES (1,'Alice'),(2,'Bob');
INSERT INTO t_vu2 VALUES (3,'Carol'),(4,'Alice');

-- ============================================================
-- 1. UNION ALL WHERE 加密列 → 正常
-- ============================================================
CREATE VIEW v_vu_union AS SELECT id,col_det FROM t_vu1 WHERE col_det='Alice' UNION ALL SELECT id,col_det FROM t_vu2 WHERE col_det='Alice';
SELECT * FROM v_vu_union ORDER BY id;
DROP VIEW v_vu_union;

-- ============================================================
-- 2. UNION ALL 无 WHERE → 正常
-- ============================================================
CREATE VIEW v_vu_union_nowhere AS SELECT id,col_det FROM t_vu1 UNION ALL SELECT id,col_det FROM t_vu2;
SELECT * FROM v_vu_union_nowhere ORDER BY id;
DROP VIEW v_vu_union_nowhere;

-- ============================================================
-- 3. UNION（不含 ALL）WHERE 加密列 → 正常
-- ============================================================
CREATE VIEW v_vu_union_distinct AS SELECT id,col_det FROM t_vu1 WHERE col_det='Alice' UNION SELECT id,col_det FROM t_vu2 WHERE col_det='Alice';
SELECT * FROM v_vu_union_distinct ORDER BY id;
DROP VIEW v_vu_union_distinct;

-- ============================================================
-- 4. Cleanup
-- ============================================================
DROP TABLE t_vu1;
DROP TABLE t_vu2;
DROP DEK dek_vu_det;
DROP KEK kek_vu CASCADE;
