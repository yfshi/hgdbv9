-- ReqID:TECC4.0 - 分布式查询路径的密文常量反解析（W5 护栏）
-- Author: <wangxp@highgo.com>, 2026-10-05.
-- Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
--
-- 防护对象（W5，对应风险 R3「密文常量折叠/往返不稳定 → 静默写错数据」）：
--   Citus 会把用户的逻辑查询按分片**反解析**成发往 worker 的 SQL 文本
--   （router 查询走 deparser/ruleutils_14.c 的 get_const_expr）。若 `_enc` 类型的
--   常量在反解析时丢掉显式 `::<type>_enc` 强制转换，worker 会把密文当 unknown/文本
--   处理 → 语义错乱；若常量在 coordinator 侧被常量折叠（fast_path_router_planner.c
--   的 eval_const_expressions）改写成别的字节 → 静默写错数据。两者都要**字节级**钉死。
--
-- 实现现状（本用例固化，未改产品代码）：
--   1. ruleutils_14.c:7057/7141 对 `_enc` 走 default 分支（simple_quote_literal），
--      且 needlabel=true → 反解析一定带 `::int4_enc` / `::text_enc`；
--   2. FastPathPlanner 的 eval_const_expressions 只对**全常量**表达式折叠；
--      TECC 客户端注入的是解析期已成型的 Const，且比较算子里必有一侧是列（Var），
--      实际不会改写密文——本用例用"反解析文本 + 行为结果"双重钉死。
--
-- 判定手段（hgdde 变体）：hgdde.log_remote_commands=on 把**真正发往 worker 的
--   分片 SQL** 打成 NOTICE。与 Citus 版本的差异均为 hgdde 的实现形态（不是断言
--   变化，勿据此简化用例）：
--     * hgdde 把连接坐标拼进 NOTICE 消息正文（"...\nDETAIL:  on server ..."），
--       `\set VERBOSITY terse` 无法剥离随机的端口/connectionId —— 由框架统一
--       归一化丢弃该行（tecc_test_utils.pm 里的 "DETAIL:  on server ..." 规则）；
--     * hgdde 会额外记 BEGIN/COMMIT，并在 SELECT 前带 `SET search_path = ...`；
--     * 快路径（9.5）hgdde 的投影可能多带列，断言只要求加密列 `a` 未被改写、
--       常量 `::int4_enc` / `::text_enc` 强制转换保留。
--   为让分片名可复现，先 `SET hgdde.next_shard_id = 900000`，4 个分片固定为
--   ddb_qp_t_900000..900003。断言只取**单分片 router 查询**（一次只发 1 条命令，
--   NOTICE 顺序确定；跨分片查询的命令顺序不确定，只断言结果）。
--   密文 hex 由框架统一归一化为 `\x被隐藏的HEX输出`（每次运行密文不同），
--   比较的是"常量带不带类型转换、结构对不对"，不是密文字节本身。
--
-- 拓扑：hgdde 套件为 1 coordinator + 2 worker（3 节点）；分片 id 由
--   hgdde.next_shard_id 固定、worker 端口不出现在断言里，故拓扑无关。

\set ON_ERROR_STOP on
\set QUIET on
SET client_min_messages = warning;
-- hgdde 变体：分片数 GUC 为 hgdde.shard_count（与 Citus 的 citus.shard_count 同义）。
SET hgdde.shard_count = 4;
SET hgdde.next_shard_id = 900000;

DROP TABLE IF EXISTS ddb_qp_t CASCADE;
DROP DEK IF EXISTS ddb_qpdek;
DROP KEK IF EXISTS ddb_qpkek CASCADE;

\echo '=== 9.1 建钥 + 分发带 DET/OPE 密态列的加密表 ==='
CREATE KEK ddb_qpkek WITH (
    key_store = 'localkms',
    key_algo  = 'sm4',
    key_path  = '%KEKPATH%/ddb_qpkek.key'
);
CREATE DEK ddb_qpdek WITH VALUES (KEK = ddb_qpkek);
CREATE TABLE ddb_qp_t (
    id int,
    a  int4_enc ENCRYPTED WITH (DEK = 'ddb_qpdek', SUPPORT_ALGO = 'det'),
    t  text_enc ENCRYPTED WITH (DEK = 'ddb_qpdek', SUPPORT_ALGO = 'ope')
);
SELECT create_distributed_table('ddb_qp_t', 'id', colocate_with => 'none');
SELECT count(*) AS shards, min(shardid) AS first_shardid, max(shardid) AS last_shardid
FROM pg_dist_shard WHERE logicalrelid = 'ddb_qp_t'::regclass;
INSERT INTO ddb_qp_t (id, a, t) VALUES (1, 11, 'k01'), (2, 22, 'k02'), (3, 33, 'k03'), (4, 44, 'k04');

\echo '=== 9.2 行为正确性（跨分片 DET 等值 / OPE 范围）==='
SELECT id FROM ddb_qp_t WHERE a = 22 ORDER BY id;
SELECT id FROM ddb_qp_t WHERE t >= 'k03' ORDER BY id;
SELECT count(*) AS n_rows FROM ddb_qp_t;

\echo '=== 9.3 字节级：单分片 DET 等值常量的反解析文本保留 ::int4_enc ==='
\set VERBOSITY terse
SET client_min_messages = notice;
SET hgdde.log_remote_commands = on;
SELECT id FROM ddb_qp_t WHERE id = 1 AND a = 11;
SET hgdde.log_remote_commands = off;

\echo '=== 9.4 字节级：单分片 OPE 范围常量的反解析文本保留 ::text_enc ==='
SET hgdde.log_remote_commands = on;
SELECT id FROM ddb_qp_t WHERE id = 1 AND t >= 'k01';
SET hgdde.log_remote_commands = off;

\echo '=== 9.5 字节级：快速路径（分布键等值）不改写密态列投影 ==='
SET hgdde.log_remote_commands = on;
SELECT a FROM ddb_qp_t WHERE id = 1;
SET hgdde.log_remote_commands = off;
SET client_min_messages = warning;

\echo '=== 9.6 清理 ==='
DROP TABLE ddb_qp_t;
DROP DEK ddb_qpdek;
DROP KEK ddb_qpkek;
