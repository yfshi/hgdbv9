-- ReqID:TECC4.0 - 跨分片密态聚合与排序语义（W6 护栏）
-- Author: <wangxp@highgo.com>, 2026-10-05.
-- Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
--
-- 防护对象（W6，plan/2026-10-02-citus分布式密态适配方案.md §W6）：
--   分布后同一列的数据被切到多个分片，聚合/排序要在"每分片局部密文 → coordinator
--   合并 → 客户端解密"的两阶段语义下仍然正确。具体覆盖：
--     - COUNT(*)：与明文一致；
--     - SUM(he-add)：worker 出局部密文，coordinator 在密文上做同态加法后由客户端解密，
--       结果必须等于明文总和（内部连接若把密文当明文处理，这条必然错）；
--     - MIN/MAX(ope)：OPE 密文保序，跨分片比较后由客户端解密；
--     - DET 等值谓词跨分片命中；
--     - ORDER BY ope LIMIT/DESC LIMIT：coordinator 在 OPE 密文上归并 + 客户端按密文序
--       返回，等价于明文排序。
--
-- 说明：密态列 AVG 按设计不支持——客户端对密态列 AVG 直接 fail-closed 报错
-- （`aggregate function "avg" is not supported on encrypted column`），符合"密态列不做 AVG"
-- 的产品口径，故不在本用例内断言其成功。
--
-- 数据为 8 行、分片数 8，聚合/排序结果与分片落点无关，故输出确定。

\set ON_ERROR_STOP on
\set QUIET on
SET client_min_messages = warning;
-- hgdde 变体：分片数 GUC 为 hgdde.shard_count（与 Citus 的 citus.shard_count 同义）。
SET hgdde.shard_count = 8;

DROP TABLE IF EXISTS ddb_agg CASCADE;

\echo '=== 6.3 建表并分发（明文分布键，密态列含 det/ope/he-add）==='
CREATE TABLE ddb_agg (
    id  int,
    he  int4_enc ENCRYPTED WITH (HE_ADD_DEK = 'ddb_headd', SUPPORT_ALGO = 'he-add'),
    ope int4_enc ENCRYPTED WITH (DEK = 'ddb_dek', SUPPORT_ALGO = 'ope'),
    det int4_enc ENCRYPTED WITH (DEK = 'ddb_dek', SUPPORT_ALGO = 'det')
);
SELECT create_distributed_table('ddb_agg', 'id');
INSERT INTO ddb_agg (id, he, ope, det) VALUES
    (1, 10, 1, 10), (2, 20, 2, 20), (3, 30, 3, 30), (4, 40, 4, 40),
    (5, 50, 5, 50), (6, 60, 6, 60), (7, 70, 7, 70), (8, 80, 8, 80);

\echo '=== 6.4 跨分片标量聚合：COUNT / SUM(he-add) / MIN,MAX(ope) ==='
SELECT count(*) AS n, sum(he) AS sum_he FROM ddb_agg;
SELECT min(ope) AS min_ope, max(ope) AS max_ope FROM ddb_agg;

\echo '=== 6.5 DET 等值跨分片命中 ==='
SELECT id FROM ddb_agg WHERE det = 20;

\echo '=== 6.6 OPE 跨分片排序与分页 ==='
SELECT id, ope FROM ddb_agg ORDER BY ope LIMIT 3;
SELECT id, ope FROM ddb_agg ORDER BY ope DESC LIMIT 2;

DROP TABLE IF EXISTS ddb_agg;
