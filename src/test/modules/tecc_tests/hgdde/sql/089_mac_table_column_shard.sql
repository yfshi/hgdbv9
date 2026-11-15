-- ReqID:TECC4.0 - 标记强访：表级/列级强制访问控制在“分片执行”时按逻辑表标记生效
-- Author: <wangxp@highgo.com>, 2026-11-11.
-- Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
--
-- 来源：2026-11-11 从 citus/sql/089_mac_table_column_shard.sql 移植（目标插件已收敛为
--   hgdde，Citus 用例只作历史参考）。移植差异仅限接口层：
--     · citus.shard_count / citus.next_shard_id -> hgdde.*
--     · citus_shards 视图在 hgdde 里不存在（本用例本来也没用到）
--   断言语义逐条不变。
--
-- 防护对象：表/列安全标记只落在逻辑表（协调节点逻辑表、worker 上的 shell 父表），
--   **分片表不带标记**。分片执行（尤其是**直连分片**）时，若直接按分片 OID 查标记会
--   查不到、按“未设置”放行 —— 表级/列级强制访问控制被绕过（fail-open）。
--   修复：TECC 核心注册“分片 -> 逻辑表”解析钩子
--   （contrib/hg_sysmac/tecc_mac_distribution.c 的 TeccMacShardLogicalRelation），
--   内核表/列级检查据此回退到逻辑表标记
--   （contrib/hg_sysmac/hg_sysmac_check.c 的 hg_sysmac_label_relation），
--   而标记本身从每个节点本地的 public.hg_sysmac_labels 读（分布式一致由 TECC 自己
--   在标记变更/分发表/加节点时推送）。
--   发现与实测见 plan/2026-10-11-标记强访适配分布式需求实现一致性核对.md 第 2 条。
--
-- 断言方式：分片执行路径用 `SET ROLE <低密级角色>; SELECT … run_command_on_shards(…)`
--   触发（run_command_on_shards 的 worker 连接以当前角色建立），把每个分片上的
--   结果/报错压成 bool_and：既与拓扑规模、分片落点无关，也不打印端口与分片编号。
--   报错文本里出现的是**分片名**（证明判定发生在数据所在节点），用 LIKE 前缀匹配、
--   不打印名字本身，故无需固定分片 id（仅第 5 节的“撞名普通表”需要固定一个 id）。
--
-- 前置：表级/列级两个开关默认 off，本用例在第 1 节对**全部节点**打开、第 7 节恢复；
--   节点数不写死（run_command_on_all_nodes 会自动覆盖 085 新加入的节点）。
--   拓扑：不加入新节点，跑在既有拓扑上，断言不依赖节点数量。

\set ON_ERROR_STOP on
\set QUIET on
SET client_min_messages = warning;
SET hgdde.shard_count = 8;

DROP TABLE IF EXISTS ddb_mac_tc CASCADE;
DROP TABLE IF EXISTS ddb_mac_tc_decoy_920000 CASCADE;
DROP ROLE IF EXISTS ddb_mac_tc_lo;
DROP ROLE IF EXISTS ddb_mac_tc_hi;

\echo '=== 1 前置：全节点打开总开关 + 表级 + 列级（每实例配置，须一致）==='
SELECT count(*) = count(*) FILTER (WHERE success) AS set_on
  FROM run_command_on_all_nodes($$ ALTER SYSTEM SET hg_sysmac.global_on = on $$);
SELECT count(*) = count(*) FILTER (WHERE success) AS set_tbl
  FROM run_command_on_all_nodes($$ ALTER SYSTEM SET hg_sysmac.global_table_level = on $$);
SELECT count(*) = count(*) FILTER (WHERE success) AS set_col
  FROM run_command_on_all_nodes($$ ALTER SYSTEM SET hg_sysmac.global_column_level = on $$);
SELECT count(*) = count(*) FILTER (WHERE success) AS reload
  FROM run_command_on_all_nodes($$ SELECT pg_reload_conf() $$);
SELECT pg_sleep(2);
SELECT count(*) = count(*) FILTER (WHERE result = 'on/on/on') AS env_on_on_on
  FROM run_command_on_all_nodes($$
        SELECT current_setting('hg_sysmac.global_on') || '/' ||
               current_setting('hg_sysmac.global_table_level') || '/' ||
               current_setting('hg_sysmac.global_column_level') $$);

\echo '=== 2 角色 + 带表标记的分布表（固定分片 id，供第 5 节撞名用）==='
SET hgdde.next_shard_id = 920000;
CREATE ROLE ddb_mac_tc_lo LOGIN;
CREATE ROLE ddb_mac_tc_hi LOGIN;
SELECT set_user_seclabel('ddb_mac_tc_lo'::regrole, 's5:c1');
SELECT set_user_seclabel('ddb_mac_tc_hi'::regrole, 's10:c1');
CREATE TABLE ddb_mac_tc(id int, v text);
SELECT create_distributed_table('ddb_mac_tc', 'id', colocate_with => 'none');
SELECT set_table_seclabel('ddb_mac_tc'::regclass, 's10:c1');
GRANT SELECT ON ddb_mac_tc TO ddb_mac_tc_lo, ddb_mac_tc_hi;
INSERT INTO ddb_mac_tc VALUES (1, 'a'), (2, 'b');

\echo '=== 3 表标记只在逻辑表上、分片没有标记（所以分片上的拦截只可能来自解析回退）==='
SELECT count(*) AS shards_with_table_label
  FROM run_command_on_shards('ddb_mac_tc', $q$
        SELECT count(*) FROM pg_seclabel
         WHERE provider = 'sysmac' AND objsubid = 0 AND objoid = '%s'::regclass
      $q$) WHERE result <> '0';

\echo '=== 4 直连分片：表级按逻辑表标记生效（lo 每个分片都被拒；hi 每个分片都放行）==='
SET ROLE ddb_mac_tc_lo;
SELECT bool_and(result LIKE 'ERROR:%mandatory access control: SELECT on relation%') AS shard_table_denied
  FROM run_command_on_shards('ddb_mac_tc', $q$ SELECT count(*) FROM %s $q$);
RESET ROLE;
SET ROLE ddb_mac_tc_hi;
SELECT bool_and(result ~ '^[0-9]+$') AS shard_table_allowed
  FROM run_command_on_shards('ddb_mac_tc', $q$ SELECT count(*) FROM %s $q$);
RESET ROLE;

\echo '=== 5 误判防护：名字恰以某真实分片号结尾的普通表，用自身标记、不被当成该分片 ==='
-- ddb_mac_tc 的分片是 ddb_mac_tc_920000..；此表名以 _920000 结尾但不是分片。
-- ddb_mac_tc 的表标记是 s10:c1（lo 会被拒），本表标 s0:c1（lo 应放行）——若解析钩子
-- 不校验物理表、误把本表当成 ddb_mac_tc 的分片，lo 读它会因 s10 被拒，此处即露馅。
CREATE TABLE ddb_mac_tc_decoy_920000(id int, v text);
SELECT set_table_seclabel('ddb_mac_tc_decoy_920000'::regclass, 's0:c1');
INSERT INTO ddb_mac_tc_decoy_920000 VALUES (1, 'x');
GRANT SELECT ON ddb_mac_tc_decoy_920000 TO ddb_mac_tc_lo;
SET ROLE ddb_mac_tc_lo;
SELECT 'decoy rows=' || count(*) AS decoy_read_with_own_label
  FROM ddb_mac_tc_decoy_920000;
RESET ROLE;

\echo '=== 6 列级：表标记降为 s0:c1（lo 过表级）、列 v 标 s10:c1；分片上读 id 放行、读 v 被拒 ==='
SELECT set_table_seclabel('ddb_mac_tc'::regclass, 's0:c1');
SELECT set_column_seclabel('ddb_mac_tc'::regclass, 2, 's10:c1');
SET ROLE ddb_mac_tc_lo;
SELECT bool_and(result ~ '^[0-9]+$') AS shard_id_allowed
  FROM run_command_on_shards('ddb_mac_tc', $q$ SELECT count(id) FROM %s $q$);
SELECT bool_and(result LIKE 'ERROR:%mandatory access control: SELECT on column 2%') AS shard_column_denied
  FROM run_command_on_shards('ddb_mac_tc', $q$ SELECT count(v) FROM %s $q$);
RESET ROLE;

\echo '=== 7 收尾：清对象 + 全节点恢复表级/列级开关 off ==='
DROP TABLE ddb_mac_tc_decoy_920000;
DROP TABLE ddb_mac_tc CASCADE;
DROP ROLE ddb_mac_tc_lo;
DROP ROLE ddb_mac_tc_hi;
SELECT count(*) = count(*) FILTER (WHERE success) AS reset_tbl
  FROM run_command_on_all_nodes($$ ALTER SYSTEM SET hg_sysmac.global_table_level = off $$);
SELECT count(*) = count(*) FILTER (WHERE success) AS reset_col
  FROM run_command_on_all_nodes($$ ALTER SYSTEM SET hg_sysmac.global_column_level = off $$);
SELECT count(*) = count(*) FILTER (WHERE success) AS reload
  FROM run_command_on_all_nodes($$ SELECT pg_reload_conf() $$);
