-- ReqID:TECC4.0 - 密态列索引在分布式形态下的建立与拒绝（DDB-1.0-04）
-- Author: <wangxp@highgo.com>, 2026-10-11.
-- Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
--
-- 防护对象（DDB-1.0-04 的「索引」子项）：密态索引规则是**列级**的——
--   BTree（等值/范围/排序）要求该列启用 OPE；Hash（等值）要求 DET 或 OPE；
--   未启用相应算法时 `check_index_support_for_encrypted_column()`
--   （src/backend/utils/tecc/tecc_index.c）明确报错。分布式下索引由 Citus 下发到
--   各分片、在**分片本地**创建，因此分片上必须有该列的密态元数据（W2）才能正确判定。
--
-- 判据：
--   5A.2 合法：BTree(OPE)、Hash(DET)、Hash(OPE) 都能建；
--   5A.3 每个分片上的索引方法与个数一致（3 个索引：btree×1 + hash×2）；
--   5A.4 非法：BTree 建在 DET-only 列上被拒（feature_not_supported）；
--   5A.5 非法：Hash 建在 RND-only 列上被拒（feature_not_supported）；
--   5A.6 建索引后 DET 等值 / OPE 范围查询照常（行为不回归）。
--   拒绝用 DO + EXCEPTION 固定成稳定输出（避免把分片侧报错的 CONTEXT 里的随机端口写进期望）。

\set ON_ERROR_STOP on
\set QUIET on
SET client_min_messages = warning;
-- hgdde 变体：分片数 GUC 为 hgdde.shard_count（与 Citus 的 citus.shard_count 同义）。
SET hgdde.shard_count = 8;

DROP TABLE IF EXISTS ddb_idx CASCADE;

\echo '=== 5A.1 建表（OPE / DET / RND-only / SEARCH 各一列）并分发 ==='
CREATE TABLE ddb_idx (
    id int,
    o  int4_enc ENCRYPTED WITH (DEK = 'ddb_dek', SUPPORT_ALGO = 'ope'),
    d  int4_enc ENCRYPTED WITH (DEK = 'ddb_dek', SUPPORT_ALGO = 'det'),
    r  int4_enc ENCRYPTED WITH (DEK = 'ddb_dek'),
    s  varchar_enc(20) ENCRYPTED WITH (DEK = 'ddb_dek', SUPPORT_ALGO = 'search')
);
SELECT create_distributed_table('ddb_idx', 'id');

\echo '=== 5A.2 合法索引：BTree(OPE) / Hash(DET) / Hash(OPE) ==='
CREATE INDEX ddb_idx_o_btree ON ddb_idx USING btree (o);
CREATE INDEX ddb_idx_d_hash  ON ddb_idx USING hash (d);
CREATE INDEX ddb_idx_o_hash  ON ddb_idx USING hash (o);

\echo '=== 5A.3 每个分片上的索引结构与个数一致（3 个：btree×1 + hash×2）==='
SELECT count(*) AS placements,
       count(*) FILTER (WHERE NOT success) AS failed,
       count(DISTINCT result) AS distinct_fingerprint,
       min(result) AS shard_indexes
FROM run_command_on_placements('ddb_idx'::regclass, $ix$
    SELECT count(*)::text || '|' ||
           coalesce(string_agg(m.amname, ',' ORDER BY m.amname), '<none>')
    FROM pg_index i
    JOIN pg_class c ON c.oid = i.indexrelid
    JOIN pg_am m ON m.oid = c.relam
    WHERE i.indrelid = '%s'::regclass
$ix$);

\echo '=== 5A.4 BTree 建在 DET-only 列上必须被拒（需要 OPE）==='
DO $$
BEGIN
    EXECUTE 'CREATE INDEX ddb_idx_d_btree ON ddb_idx USING btree (d)';
    RAISE EXCEPTION 'B-tree on DET-only was ACCEPTED (bad)';
EXCEPTION WHEN feature_not_supported THEN
    NULL;
END $$;
SELECT 'btree_on_det_rejected' AS check;

\echo '=== 5A.5 Hash 建在 RND-only 列上必须被拒（需要 DET 或 OPE）==='
DO $$
BEGIN
    EXECUTE 'CREATE INDEX ddb_idx_r_hash ON ddb_idx USING hash (r)';
    RAISE EXCEPTION 'Hash on RND-only was ACCEPTED (bad)';
EXCEPTION WHEN feature_not_supported THEN
    NULL;
END $$;
SELECT 'hash_on_rnd_rejected' AS check;

\echo '=== 5A.6 建索引后跨分片查询照常 ==='
INSERT INTO ddb_idx (id, o, d, r, s) VALUES
    (1, 1, 10, 100, 'a1'), (2, 2, 20, 200, 'a2'),
    (3, 3, 30, 300, 'a3'), (4, 4, 40, 400, 'a4'),
    (5, 5, 50, 500, 'a5'), (6, 6, 60, 600, 'a6'),
    (7, 7, 70, 700, 'a7'), (8, 8, 80, 800, 'a8');
SELECT id FROM ddb_idx WHERE o >= 6 ORDER BY id;
SELECT id FROM ddb_idx WHERE d = 40 ORDER BY id;
SELECT id FROM ddb_idx WHERE s LIKE 'a6%' ORDER BY id;

DROP TABLE ddb_idx;