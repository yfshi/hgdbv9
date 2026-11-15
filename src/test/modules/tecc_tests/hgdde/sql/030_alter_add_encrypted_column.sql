-- ReqID:TECC4.0 - 分发后新增加密列（W2 的 ALTER 路径护栏）
-- Author: <wangxp@highgo.com>, 2026-10-05（2026-10-30 按 libpq 侧新机制改写）.
-- Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
--
-- 防护对象（W2，plan/2026-10-28-hgdde分布式适配可行性分析.md §0.3/§0.4）：
--   `ALTER TABLE ... ADD COLUMN <密态列>` 在分布式下会走
--   PreprocessAlterTableStmt → 反解析 → 对每个分片调 worker_apply_shard_ddl_command。
--   反解析文本里没有 ENCRYPTED WITH（子句只存在 hg_encrypted_columns），现在的补法在
--   **libpq 侧**（TECC_MODE_DDL_ONLY）：TECC 的 DDL 处理在下发前把子句合成进文本。
--   这一步退化的话，分片会建出"有加密类型、查不到元数据"的列 —— 写入校验与查询
--   规划都会静默走错路径。
--
-- 为什么 ADD COLUMN 能成立（这条比建表更苛刻）：
--   该列的声明是在**同一个事务里**刚写进 hg_encrypted_columns 的，另开会话的取数器
--   看不到未提交行。所以子句取自**同进程**目录查询
--   （hg_get_encrypted_clause_by_name()，PG 核心、由 libpq 以弱符号调用），
--   本事务未提交的行对它可见。
--
-- 判定：ALTER 后全部分片的元数据指纹必须一致地多出 `n:2:<dekdist>`（2 = ope），
--       且新列可写、可用 OPE 范围条件查；worker 上的父表副本也要一致。

\set ON_ERROR_STOP on
\set QUIET on
SET client_min_messages = warning;
SET hgdde.shard_count = 8;

DROP TABLE IF EXISTS ddb_alt CASCADE;

\echo '=== 3.1 建表并分发 ==='
CREATE TABLE ddb_alt (
    id int,
    a  int4_enc ENCRYPTED WITH (DEK = 'ddb_dek', SUPPORT_ALGO = 'det')
);
SELECT create_distributed_table('ddb_alt', 'id');

\echo '=== 3.2 分发之后给已有表加一个加密列 ==='
ALTER TABLE ddb_alt ADD COLUMN n text_enc ENCRYPTED WITH (DEK = 'ddb_dek', SUPPORT_ALGO = 'ope');

\echo '=== 3.3 全部 8 个分片都拿到新列元数据（指纹必须唯一）==='
SELECT count(*) AS placements,
       count(DISTINCT result) AS distinct_fingerprint,
       min(result) AS shard_fingerprint
FROM run_command_on_placements('ddb_alt'::regclass, $fp$
    WITH s AS (SELECT '%s'::regclass::oid AS oid)
    SELECT (SELECT count(*) FROM hg_encrypted_columns WHERE table_oid = s.oid)::text || '|' ||
           coalesce((SELECT string_agg(a.attname || ':' || ec.encryption_type || ':' ||
                              coalesce(k.column_key_distributed_id::text, '0'), ',' ORDER BY a.attnum)
                     FROM pg_attribute a
                     JOIN hg_encrypted_columns ec
                       ON ec.table_oid = a.attrelid AND ec.column_attnum = a.attnum
                     LEFT JOIN hg_dek k ON k.oid = ec.column_key_id
                     WHERE a.attrelid = s.oid AND a.attnum > 0 AND NOT a.attisdropped), '(none)')
    FROM s
$fp$);

\echo '=== 3.4 worker 上的父表副本同步拿到新列 ==='
SELECT count(*) AS nodes,
       count(DISTINCT result) AS distinct_fingerprint,
       min(result) AS parent_fingerprint
FROM run_command_on_workers($wp$
    WITH s AS (SELECT 'public.ddb_alt'::regclass::oid AS oid)
    SELECT (SELECT count(*) FROM hg_encrypted_columns WHERE table_oid = s.oid)::text || '|' ||
           coalesce((SELECT string_agg(a.attname || ':' || ec.encryption_type || ':' ||
                              coalesce(k.column_key_distributed_id::text, '0'), ',' ORDER BY a.attnum)
                     FROM pg_attribute a
                     JOIN hg_encrypted_columns ec
                       ON ec.table_oid = a.attrelid AND ec.column_attnum = a.attnum
                     LEFT JOIN hg_dek k ON k.oid = ec.column_key_id
                     WHERE a.attrelid = s.oid AND a.attnum > 0 AND NOT a.attisdropped), '(none)')
    FROM s
$wp$);

\echo '=== 3.5 换一条连接后写新列并回读 ==='
-- 这里必须重连：ALTER TABLE ADD COLUMN 之后**同一条连接**内引用新列时，客户端不
-- 加密该列的明文常量（列元数据缓存没有随 DDL 失效），实测把原文直接发给服务端：
--     ERROR: invalid hex string: "vvv"      （服务端输入函数拒绝，明文已出客户端）
-- 该缺口与 Citus 无关：单机本地表同样复现（已登记为客户端缓存缺口，W7 范畴）。
-- 重连一次得到干净的元数据缓存，用来验证分发链路上新列确实可写可查。
\c -
SET client_min_messages = warning;
INSERT INTO ddb_alt (id, a) VALUES (1, 10), (2, 20);
UPDATE ddb_alt SET n = 'vvv';
SELECT count(*) AS n_rows, count(*) FILTER (WHERE n > 'a') AS above_a FROM ddb_alt;