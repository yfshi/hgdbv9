-- ReqID:TECC4.0 - 集群密钥就位与跨节点身份标识一致（W3 前提护栏）
-- Author: <wangxp@highgo.com>, 2026-10-05.
-- Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
--
-- 归属：citus/ 是**多节点**用例目录，与按客户端划分的 shared/、libpq/、cross/ 并列。
--       入口 make check-tecc-citus（t/008_tecc_citus_test.pl）。
--
-- 本用例只做断言，不建密钥：KEK/DEK 由 TAP 驱动在起完集群后**只在 coordinator 上**
-- 建一份（见 t/008_tecc_citus_test.pl 的 note_node_keys），依赖 W3 的自动传播到 worker。
-- 因此本用例通过 = 传播生效（三节点都就位且身份标识一致）。
--
-- 背景（W3 实现约束，改传播方式时勿踩）：CREATE KEK / CREATE DEK 在服务端要求语句
-- 携带 `_TECC_PROCESSED` 标记，该标记由 libpq 侧的 TECC 处理器注入（psql 默认
-- enable_tecc=true）。Citus 的内部连接被**故意**关掉了 TECC（W1：libpq 对"在服务端
-- 进程内打开的连接"强制 enable_tecc=false，Citus 侧再显式传 enable_tecc=false），
-- 所以传播只能重放**已处理过的语句文本**，不能只发用户原文：
--     ERROR: CREATE KEK requires TECC encryption mode to be enabled
-- 实现见 src/backend/commands/tecc_key_propagation.c（2026-10-29 从 Citus 外围化：
-- PG 核心 utility hook 的 postprocess 段，用 SQL 层 run_command_on_workers 下发）。
--
-- 固定住的两条 W3 前提（缺了它们「传播无需额外映射」就不成立）：
--   1. 三节点上同名 KEK/DEK 都在；
--   2. hg_dek.column_key_distributed_id（按 schema.dekname 哈希生成，createdek.c）
--      三节点完全相同 —— 因此分片元数据可以用它做跨节点指纹。

\set ON_ERROR_STOP on
\set QUIET on
SET client_min_messages = warning;

\echo '=== 0.1 集群拓扑（1 coordinator + 2 worker）==='
-- 只断言节点数与角色/状态，不打印 nodename/nodeport：端口每次运行随机分配。
SELECT nodeid, isactive, hasmetadata FROM pg_dist_node ORDER BY nodeid;
SELECT count(*) AS active_workers FROM master_get_active_worker_nodes();

\echo '=== 0.2 三节点 KEK 就位（0 = 无缺失）==='
SELECT count(*) AS nodes, count(*) FILTER (WHERE result <> '2') AS kek_mismatch
FROM run_command_on_all_nodes($$
    SELECT count(*)::text FROM hg_kek WHERE kek_name IN ('ddb_kek', 'ddb_hekek')
$$);

\echo '=== 0.3 三节点 DEK 就位（0 = 无缺失）==='
SELECT count(*) AS nodes, count(*) FILTER (WHERE result <> '3') AS dek_mismatch
FROM run_command_on_all_nodes($$
    SELECT count(*)::text FROM hg_dek
    WHERE dek_name IN ('ddb_dek', 'ddb_headd', 'ddb_hemul')
$$);

\echo '=== 0.4 DEK 分布式身份标识三节点一致（1 = 一致）==='
SELECT count(DISTINCT result) AS distinct_ids, min(result) AS distributed_ids
FROM run_command_on_all_nodes($$
    SELECT string_agg(dek_name || ':' || column_key_distributed_id::text, ',' ORDER BY dek_name)
    FROM hg_dek WHERE dek_name IN ('ddb_dek', 'ddb_headd', 'ddb_hemul')
$$);
