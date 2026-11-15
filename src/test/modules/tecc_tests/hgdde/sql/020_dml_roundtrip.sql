-- ReqID:TECC4.0 - 分布式密态读写往返与内部连接隔离（W1/W2 护栏）
-- Author: <wangxp@highgo.com>, 2026-10-05.
-- Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
--
-- 防护对象：
--   W1（内部连接与密态处理隔离）：Citus 在 coordinator 后端里开出的、发往 worker 的
--     libpq 连接**只能做 DDL 子句合成，不得跑数据面**（TECC_MODE_DDL_ONLY：不解密
--     结果、不加密常量）。否则分片结果会在 coordinator 后端里被"提前解密"，密文分发
--     路径就断了。该判定现在**由 libpq 自己完成**（弱符号探测服务端进程），
--     `contrib/citus` 里那个 `citus.internal_connection_enable_tecc` 开关已按新策略
--     移除——本用例顺带钉住"插件侧不再有这个开关"。
--   W2（分片元数据）：分片能按密态列正常读写，说明分片的 hg_encrypted_columns 与
--     DEK 绑定都对得上（元数据缺失时写入的跨 DEK/算法段校验整列不发生，会静默写错）。
--
-- 判定口径：
--   1. 用户连接（psql → coordinator）读回**明文**：客户端解密链路通；
--   2. 同一批数据在分片上用类型的 out 函数直读是**十六进制密文**且不含明文标记：
--      客户端确实加密后才发出、落盘是密文；
--   3. DET 等值、OPE 范围、跨分片同态 sum 全部正确 —— 这是"内部分发没有把密文当
--      明文处理"的行为级判据（内部连接若解密，跨分片聚合必然错）。
--   4. 插件侧的 W1 开关已不存在（避免又悄悄回到"改 Citus"的老路）。

\set ON_ERROR_STOP on
\set QUIET on
SET client_min_messages = warning;
SET hgdde.shard_count = 8;

DROP TABLE IF EXISTS ddb_dml CASCADE;

\echo '=== 2.1 W1 前提：插件侧不再有 TECC 开关（判定改由 libpq 承担）==='
SELECT count(*) AS plugin_side_tecc_gucs
FROM pg_settings WHERE name = 'citus.internal_connection_enable_tecc';

\echo '=== 2.2 建表并分发 ==='
CREATE TABLE ddb_dml (
    id int,
    a  int4_enc  ENCRYPTED WITH (DEK = 'ddb_dek', HE_ADD_DEK = 'ddb_headd', SUPPORT_ALGO = 'det,he-add'),
    d  varchar_enc(20) ENCRYPTED WITH (DEK = 'ddb_dek', SUPPORT_ALGO = 'search'),
    t  text_enc  ENCRYPTED WITH (DEK = 'ddb_dek', SUPPORT_ALGO = 'ope')
);
SELECT create_distributed_table('ddb_dml', 'id');
SELECT count(*) AS shards, count(DISTINCT p.nodeport) AS nodes
FROM pg_dist_shard s JOIN pg_dist_shard_placement p USING (shardid)
WHERE s.logicalrelid = 'ddb_dml'::regclass;

\echo '=== 2.3 客户端写入（明文 SQL，加密在 libpq 侧完成）==='
INSERT INTO ddb_dml (id, a, d, t) VALUES
    (1, 11, 'DDBMARK-01', 'k01'),
    (2, 22, 'DDBMARK-02', 'k02'),
    (3, 33, 'DDBMARK-03', 'k03'),
    (4, 44, 'DDBMARK-04', 'k04'),
    (5, 55, 'DDBMARK-05', 'k05'),
    (6, 66, 'DDBMARK-06', 'k06'),
    (7, 77, 'DDBMARK-07', 'k07'),
    (8, 88, 'DDBMARK-08', 'k08');

\echo '=== 2.4 跨分片回读解密 + 同态聚合 ==='
SELECT count(*) AS n_rows, sum(a) AS sum_a FROM ddb_dml;
SELECT id, a, d, t FROM ddb_dml ORDER BY id;

\echo '=== 2.5 密态条件查询：DET 等值 / OPE 范围 ==='
SELECT count(*) AS det_eq FROM ddb_dml WHERE a = 55;
SELECT count(*) AS ope_range FROM ddb_dml WHERE t >= 'k05';
SELECT id, t FROM ddb_dml WHERE t >= 'k05' ORDER BY id;

\echo '=== 2.6 分片落盘仍是密文（明文标记不得出现在任何分片）==='
-- 用类型的 out 函数直读：走内部连接（TECC off）拿到的是原始十六进制密文。
-- 标记 'DDBMARK' 含非十六进制字符（M/K/R），因此"命中 0 次"是可判定的。
SELECT sum(split_part(result, '/', 1)::int) AS rows_on_shards,
       sum(split_part(result, '/', 2)::int) AS plaintext_leaks,
       sum(split_part(result, '/', 3)::int) AS non_ciphertext_varchar,
       sum(split_part(result, '/', 4)::int) AS non_ciphertext_int4
FROM run_command_on_placements('ddb_dml'::regclass, $raw$
    SELECT count(*)::text || '/' ||
           count(*) FILTER (WHERE position('DDBMARK' in varchar_enc_out(d)::text) > 0)::text || '/' ||
           count(*) FILTER (WHERE left(varchar_enc_out(d)::text, 2) IS DISTINCT FROM '\x')::text || '/' ||
           count(*) FILTER (WHERE left(int4_enc_out(a)::text, 2) IS DISTINCT FROM '\x')::text
    FROM %s
$raw$);
