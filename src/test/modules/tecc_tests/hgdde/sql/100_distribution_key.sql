-- ReqID:TECC4.0 - 密态列不得作为分布键（W6 护栏）
-- Author: <wangxp@highgo.com>, 2026-10-05.
-- Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
--
-- 防护对象（W6，plan/2026-10-02-citus分布式密态适配方案.md §W6 / §6 决策 1）：
--   分布键只允许明文列。Citus 剪枝时在 coordinator 上对分布键求 hash 来决定目标分片，
--   它假定列是明文；而 TECC 的密态列在服务端只有密文，DET/OPE 提供的是**密文比较**，
--   与"按明文值定位分片"不是一回事。因此密态列做分布键必须**显式拒绝**：Citus 自己
--   会接受（§2.2 已证 `_enc` 类型有默认 btree/hash opclass），不拦就会"能建但剪枝结果错"。
--
-- 实现：Citus `EnsureDistributionColumnNotEncrypted()`（utils/distribution_column.c），
--   普通/再平衡路径经 `CreateCitusTable` 调用；并发路径经 `EnsureValidDistributionColumn`
--   调用（该函数只被 `create_distributed_table_concurrently` 使用）。
--
-- 判定：用 DO + EXCEPTION 把拒绝结果固定成 rejected（不把带文件行号的报错原文写进
--       golden）；明文分布键照常可分发。
-- 覆盖边界：并发路径 `create_distributed_table_concurrently` **不能**在事务块（含
--       DO/PL 块）内执行，无法用同一手法固化；该入口已人工验证会走
--       `EnsureValidDistributionColumn` → 同一 helper 报 feature_not_supported。

\set ON_ERROR_STOP on
\set QUIET on
SET client_min_messages = warning;
-- hgdde 变体：分片数 GUC 为 hgdde.shard_count（与 Citus 的 citus.shard_count 同义）。
SET hgdde.shard_count = 8;

DROP TABLE IF EXISTS ddb_dis_enc CASCADE;
DROP TABLE IF EXISTS ddb_dis_plain CASCADE;

\echo '=== 6.1 密态列作分布键必须被拒绝（普通路径）==='
CREATE TABLE ddb_dis_enc (
    id int4_enc ENCRYPTED WITH (DEK = 'ddb_dek', SUPPORT_ALGO = 'det'),
    v  int
);
DO $$
BEGIN
    PERFORM create_distributed_table('ddb_dis_enc', 'id');
    RAISE EXCEPTION 'encrypted distribution key was ACCEPTED (bad)';
EXCEPTION WHEN feature_not_supported THEN
    NULL;
END $$;
SELECT 'rejected' AS encrypted_distribution_key;

\echo '=== 6.2 明文列作分布键照常可分发 ==='
CREATE TABLE ddb_dis_plain (
    id int,
    v  int4_enc ENCRYPTED WITH (DEK = 'ddb_dek', SUPPORT_ALGO = 'det')
);
SELECT create_distributed_table('ddb_dis_plain', 'id');
SELECT count(*) AS shards
FROM pg_dist_shard WHERE logicalrelid = 'ddb_dis_plain'::regclass;

DROP TABLE IF EXISTS ddb_dis_plain;
DROP TABLE IF EXISTS ddb_dis_enc;
