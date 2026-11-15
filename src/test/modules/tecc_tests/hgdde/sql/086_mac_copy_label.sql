-- ReqID:TECC4.0 - 标记强访：分发表上的 COPY 标记导出 + COPY FROM 与行级策略（阶段5 T5.2）
-- Author: <wangxp@highgo.com>, 2026-11-11.
-- Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
--
-- 来源：2026-11-11 从 citus/sql/086_mac_copy_label.sql 移植（目标插件已收敛为 hgdde，
--   Citus 用例只作历史参考）。移植差异仅限接口层：
--     · citus.shard_count / citus.next_shard_id -> hgdde.*
--     · citus_shards 视图在 hgdde 里不存在 -> 改查 pg_dist_shard（本用例只数分片数）
--   断言语义逐条不变。
--
-- 防护对象（详设 V9-DD-2-1 + 插件侧缺口 multi_copy 的
-- `/* TODO: Perform RLS checks once supported */`）：
--   分发表的 COPY 由分布式插件自己实现（hgdde 的 multi_copy.rs，Citus 侧是
--   multi_copy.c）：协调节点根本不调 PostgreSQL 的 DoCopy()，也就丢掉了那里的两道判定。
--   因此必须在这条路径上单独补（TECC 侧挂在核心 ProcessUtility 上、在插件之前跑，
--   见 contrib/hg_sysmac/tecc_mac_copy.c）：
--     ① 导出规则②：显式列出标记列而导出开关没打开 -> 报错退出；
--     ② COPY FROM 与行级策略：PostgreSQL 的 COPY FROM 在 RLS 对当前用户生效时一律
--        拒绝；分布式路径必须由**协调节点**直接报同样的错，不能等某个 worker 报
--        （报错会带 "CONTEXT: while executing command on …"），更不能被本地分片
--        那条路静默绕过。golden 里一旦出现 worker CONTEXT 就会 diff 出来；
--     ③ 导出规则③④：分发表未指定列清单时"开关 off 不导出标记列 / 开关 on 连标记列
--        一起导出"也要成立 —— 前者靠改写后的 COPY 列清单过滤，后者靠带 RLS 的分片
--        把 `SELECT *` 换成显式列清单（标记列是不参与星号展开的隐藏列）。
--   导入侧按决策 D9 的方案二：不动列数、不做自动判断，用户自己把文件与表列对齐；
--   因而 hg_sysmac.global_import_label 不介入（详设 V9-DD-2-1 的方案一才用它）。
--
-- 期望文件口径（重要）：分发表的 COPY TO 是**按分片流式输出**的，行序不保证，且分片
--   落点随拓扑变化。故对导出结果只断言与顺序无关的量：CSV 的表头（= 导出的列集合）、
--   行数（wc -l）、以及排序后的聚合结果；节点成功/失败、开关是否全节点一致一律压成
--   布尔，不打印 nodeid。
--
-- 拓扑：本用例跑在既有拓扑上（085 之后集群是 4 节点），断言不依赖节点数量；
--   colocate_with => 'none' 只为让分片铺满全部 worker，与规模无关。

\set ON_ERROR_STOP on
\set QUIET on
SET client_min_messages = warning;
SET hgdde.shard_count = 4;
-- 固定分片 id：分片名不打印进断言，但固定后失败信息的可复现性更好
SET hgdde.next_shard_id = 930000;

DROP TABLE IF EXISTS ddb_maccp_t CASCADE;
DROP ROLE IF EXISTS ddb_maccp_u10;

\echo '=== 1 准备：角色 + 带标记列与行级规则的分布表 ==='
CREATE ROLE ddb_maccp_u10 LOGIN;
SELECT set_user_seclabel('ddb_maccp_u10'::regrole, 's10:c1,c2');
CREATE TABLE ddb_maccp_t (id int, name text);
SELECT create_distributed_table('ddb_maccp_t', 'id', colocate_with => 'none');
SELECT apply_row_policy('ddb_maccp_t'::regclass, 'apply');
GRANT ALL ON ddb_maccp_t TO ddb_maccp_u10;
INSERT INTO ddb_maccp_t (id, name, __hg_seclabel_level__, __hg_seclabel_cats__) VALUES
 (1, 'a', 10, '{1,2}'),
 (2, 'b',  5, '{1,2}'),
 (3, 'c', 15, '{}');
SELECT count(*) AS rows_total FROM ddb_maccp_t;
SELECT count(*) AS shards FROM pg_dist_shard
 WHERE logicalrelid = 'ddb_maccp_t'::regclass;

\echo '=== 2 规则②：分发表上显式列出标记列、开关 off -> 协调节点就报错（无 worker CONTEXT）'
-- 下面这条**故意失败**：关掉 ON_ERROR_STOP 让用例继续，报错本身就是要固化的断言
-- （若判定退化成分片报错，golden 里会多出 "CONTEXT: while executing command on ..."）。
\set ON_ERROR_STOP off
\copy ddb_maccp_t (id, __hg_seclabel_level__) TO 'ddb_maccp_denied.txt'
\set ON_ERROR_STOP on

\echo '=== 3 规则③：分发表未指定列清单、开关 off -> 不导出标记列；行级策略照常过滤'
\copy ddb_maccp_t TO 'ddb_maccp_off.csv' WITH (FORMAT csv, HEADER true)
\! head -1 ddb_maccp_off.csv
\! wc -l < ddb_maccp_off.csv
SELECT string_agg(id::text, ',' ORDER BY id) AS superuser_sees FROM ddb_maccp_t;
SET ROLE ddb_maccp_u10;
\copy ddb_maccp_t TO 'ddb_maccp_off_u10.csv' WITH (FORMAT csv, HEADER true)
\! head -1 ddb_maccp_off_u10.csv
\! wc -l < ddb_maccp_off_u10.csv
SELECT string_agg(id::text, ',' ORDER BY id) AS u10_sees FROM ddb_maccp_t;
RESET ROLE;

\echo '=== 4 规则②：分片 COPY TO 上同样拒绝（角色视角，走的是远程分片 COPY）'
SET ROLE ddb_maccp_u10;
\set ON_ERROR_STOP off
\copy ddb_maccp_t (id, __hg_seclabel_level__) TO 'ddb_maccp_denied_u10.txt'
\set ON_ERROR_STOP on
RESET ROLE;

\echo '=== 5 T5.2：行级策略生效时，分发 COPY FROM 由协调节点直接拒绝（无 worker CONTEXT）'
\! printf '10|ok|10|{1,2}\n30|forge|5|{1,2}\n' > ddb_maccp_in.csv
SET ROLE ddb_maccp_u10;
\set ON_ERROR_STOP off
\copy ddb_maccp_t (id, name, __hg_seclabel_level__, __hg_seclabel_cats__) FROM 'ddb_maccp_in.csv' WITH (FORMAT csv, DELIMITER '|')
\set ON_ERROR_STOP on
RESET ROLE;
SELECT count(*) AS forged_rows FROM ddb_maccp_t WHERE id IN (10, 30);

\echo '=== 6 规则④：各节点打开导出开关 + 未指定列清单 -> 导出所有列（含标记列）'
SELECT count(*) = count(*) FILTER (WHERE result = 'on') AS all_nodes_on_before
  FROM run_command_on_all_nodes($$ SHOW hg_sysmac.global_export_label $$);
SELECT count(*) = count(*) FILTER (WHERE success) AS set_switch_ok
  FROM run_command_on_all_nodes($$ ALTER SYSTEM SET hg_sysmac.global_export_label = on $$);
SELECT count(*) = count(*) FILTER (WHERE success) AS reload_ok
  FROM run_command_on_all_nodes($$ SELECT pg_reload_conf() $$);
SELECT pg_reload_conf();
SELECT pg_sleep(2);
SELECT count(*) = count(*) FILTER (WHERE result = 'on') AS all_nodes_on_after
  FROM run_command_on_all_nodes($$ SHOW hg_sysmac.global_export_label $$);
\copy ddb_maccp_t TO 'ddb_maccp_all.csv' WITH (FORMAT csv, HEADER true)
\! head -1 ddb_maccp_all.csv
\! wc -l < ddb_maccp_all.csv
-- 开关打开时列清单仍然说了算
\copy ddb_maccp_t (id) TO 'ddb_maccp_id.csv' WITH (FORMAT csv, HEADER true)
\! head -1 ddb_maccp_id.csv
SELECT id, __hg_seclabel_level__, __hg_seclabel_cats__::text FROM ddb_maccp_t ORDER BY id;

\echo '=== 7 导入（方案二）：豁免用户（超级用户）经分发 COPY 把标记值导进去 ==='
\copy ddb_maccp_t (id, name, __hg_seclabel_level__, __hg_seclabel_cats__) FROM 'ddb_maccp_in.csv' WITH (FORMAT csv, DELIMITER '|')
SELECT id, __hg_seclabel_level__, __hg_seclabel_cats__::text FROM ddb_maccp_t
 WHERE id IN (10, 30) ORDER BY id;

\echo '=== 8 收尾：各节点把导出开关恢复成 off ==='
SELECT count(*) = count(*) FILTER (WHERE success) AS set_switch_ok
  FROM run_command_on_all_nodes($$ ALTER SYSTEM SET hg_sysmac.global_export_label = off $$);
SELECT count(*) = count(*) FILTER (WHERE success) AS reload_ok
  FROM run_command_on_all_nodes($$ SELECT pg_reload_conf() $$);
SELECT pg_reload_conf();
SELECT pg_sleep(2);
SELECT count(*) = count(*) FILTER (WHERE result = 'off') AS all_nodes_off_after
  FROM run_command_on_all_nodes($$ SHOW hg_sysmac.global_export_label $$);
