-- ============================================================
-- temp_view_reject.sql — 临时视图拒绝加密列回归测试
-- 覆盖 Bug 9154: CREATE TEMP VIEW 引用加密列应明确报错
-- ============================================================

-- ============================================================
-- 0. Setup
-- ============================================================
CREATE KEK kek_tvr WITH (key_store = 'localkms', key_path = '/tmp/kek_tvr.key');
CREATE DEK dek_tvr_det WITH VALUES(KEK = kek_tvr);
CREATE DEK dek_tvr_ope WITH VALUES(KEK = kek_tvr);

CREATE TABLE t_tvr (
    id int,
    col_det text_enc encrypted with(dek=dek_tvr_det, support_algo='det'),
    col_ope int4_enc encrypted with(dek=dek_tvr_ope, support_algo='ope')
);
INSERT INTO t_tvr VALUES (1,'Alice',10),(2,'Bob',20);

-- ============================================================
-- 1. CREATE TEMP VIEW 引用加密列 → 明确拒绝
-- ============================================================
CREATE TEMP VIEW v_tvr_temp AS SELECT id,col_det FROM t_tvr WHERE col_det='Alice';

-- ============================================================
-- 2. CREATE TEMP VIEW 不引用加密列（仅普通列）→ 允许
-- ============================================================
CREATE TEMP VIEW v_tvr_temp_noenc AS SELECT id FROM t_tvr;
SELECT * FROM v_tvr_temp_noenc ORDER BY id;
DROP VIEW v_tvr_temp_noenc;

-- ============================================================
-- 3. CREATE OR REPLACE TEMP VIEW 引用加密列 → 明确拒绝
-- 先创建普通视图，再 OR REPLACE 为 TEMP
-- ============================================================
CREATE VIEW v_tvr_regular AS SELECT id,col_det FROM t_tvr WHERE col_det='Alice';
DROP VIEW v_tvr_regular;
CREATE OR REPLACE TEMP VIEW v_tvr_temp2 AS SELECT id,col_det FROM t_tvr WHERE col_det='Alice';

-- ============================================================
-- 4. 普通视图（非 TEMP）引用加密列 → 正常允许
-- ============================================================
CREATE VIEW v_tvr_normal AS SELECT id,col_det FROM t_tvr WHERE col_det='Alice';
SELECT * FROM v_tvr_normal ORDER BY id;
DROP VIEW v_tvr_normal;

-- ============================================================
-- 5. Cleanup
-- ============================================================
DROP TABLE t_tvr;
DROP DEK dek_tvr_det;
DROP DEK dek_tvr_ope;
DROP KEK kek_tvr CASCADE;
