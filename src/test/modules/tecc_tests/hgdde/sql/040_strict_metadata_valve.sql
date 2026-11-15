-- ReqID:TECC4.0 - 失败关闭开关在分布式形态下的默认与逃生阀（W2b 护栏）
-- Author: <wangxp@highgo.com>, 2026-10-05.
-- Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
--
-- 防护对象：hg_tecc.strict_metadata（off | warn | error，默认 error）是 W2b 引入的
--   "列声明成加密类型却查不到元数据时失败关闭"开关。分布式形态下它有两条必须固定住的
--   行为：
--     1. **每个节点都默认 error**。worker 是分片元数据的实际持有者，worker 上若默认 off，
--        "分片元数据缺失"就又退化成静默降级（W2 之前的分片表就是这状态）。
--     2. **逃生阀是逐节点的**：协调端会话里的 SET 传不到 Citus 内部连接，worker 要单独
--        设置（该节点自己的会话、ALTER DATABASE/SYSTEM SET）。这条是使用约定，
--        不写进用例的话，运维会以为在 coordinator 上 SET 一次就全集群生效了。
--
-- 判定：三节点 current_setting 相同且为 error；协调端会话改成 off 后 worker 仍为 error；
--      放宽后协调端的分布式密态读写照常。

\set ON_ERROR_STOP on
\set QUIET on
SET client_min_messages = warning;
SET hgdde.shard_count = 8;

DROP TABLE IF EXISTS ddb_valve CASCADE;

\echo '=== 4.1 三个节点默认都是 error（失败关闭）==='
SELECT count(*) AS nodes,
       count(DISTINCT result) AS distinct_values,
       count(*) FILTER (WHERE result <> 'error') AS non_error
FROM run_command_on_all_nodes($$ SELECT current_setting('hg_tecc.strict_metadata') $$);

\echo '=== 4.2 逃生阀逐节点：协调端会话内放宽 ==='
SET hg_tecc.strict_metadata = 'off';
SHOW hg_tecc.strict_metadata;

\echo '=== 4.3 worker 不受协调端会话设置影响（仍为 error）==='
SELECT count(*) AS workers,
       count(DISTINCT result) AS distinct_values,
       min(result) AS worker_value
FROM run_command_on_workers($$ SELECT current_setting('hg_tecc.strict_metadata') $$);

\echo '=== 4.4 放宽后协调端的分布式密态读写照常 ==='
CREATE TABLE ddb_valve (
    id int,
    a  int4_enc ENCRYPTED WITH (DEK = 'ddb_dek', HE_ADD_DEK = 'ddb_headd', SUPPORT_ALGO = 'det,he-add')
);
SELECT create_distributed_table('ddb_valve', 'id');
INSERT INTO ddb_valve (id, a) VALUES (1, 3), (2, 4);
SELECT count(*) AS n_rows, sum(a) AS sum_a FROM ddb_valve;
RESET hg_tecc.strict_metadata;
SHOW hg_tecc.strict_metadata;
