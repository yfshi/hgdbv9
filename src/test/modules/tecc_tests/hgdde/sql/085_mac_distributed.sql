-- ReqID:TECC4.0 - 标记强访（强制访问控制）的分布式护栏（阶段4 T4.6）
-- Author: <wangxp@highgo.com>, 2026-10-07.
-- Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
--
-- 防护对象（阶段4 T4.1~T4.5，plan/2026-10-02-citus分布式密态适配方案.md §T4.x、
-- .mimocode/plans/1791341421585-silent-river.md「阶段 4」）：
--   把标记强访搬到 Citus 分布式形态上之后，下面五件事必须同时成立：
--     ① 分片上的行级规则形态：relrowsecurity/FORCE 都是 true、两个隐藏标记列在、
--        btree+GIN 两索引在，且 pg_policy 里**只有 1 条恒真 PERMISSIVE 占位策略**
--        （四条真规则由 row_security_policy_hook_restrictive 钩子按命令类型现给，
--         阶段4 T4.5 的决策 (α)；见 shared/sql/sysmac_row.sql 的说明）；
--     ② 行级过滤在**分片执行时**真的生效，且与《标记比较规则》判定表一致
--        （SELECT 用 level <= 我 ∧ cats <@ 我；UPDATE/DELETE 用等号）；
--     ③ 表标记 / 列标记在协调节点 + 全部 worker 上一致（T4.3：改写走标准
--        SQL 路径；跨节点载体是 public.hg_sysmac_labels，见
--        src/backend/commands/tecc_mac_distribution.c）；
--     ④ **新加入的节点**也拿到表/列标记（T4.4：hgdde_add_node 成功之后由
--        tecc_dist_bootstrap.c 调 TeccMacSyncLabels() 整体补发）；
--     ⑤ 伪造行标记的 INSERT 在 worker 上被拒，报错带上占位策略名 hg_sysmac_row_insert
--        （证明强制力来自分片那一侧，不是协调节点自己过滤）。
--
-- 前置（关键）：① 行级规则由钩子提供之后受 hg_sysmac.global_on / global_row_level
--   管辖，**每个节点**都要打开 —— TAP 驱动 t/009_tecc_hgdde_test.pl 已把
--   hg_sysmac.global_on = on 写进每个实例的 postgresql.conf（含本用例要加入的新节点，
--   因为 run_command_on_all_nodes 够不到未注册的实例）。第 10.0 节先断言这一点。
--   ② 表级/列级保持默认 off：本用例给表/列打的标记只用于验证**传播**，不参与访问判定
--   （表级/列级的判定矩阵由单机用例 sysmac_label.sql 等覆盖）。
--
-- 拓扑：本用例自己把一个**待加入实例**（t/009 预先起好但未注册，端口每轮随机，经
--   %ENV:TECC_HGDDE_W3_PORT% 传入）加进集群，专门覆盖上面第 ④ 条 —— 该实例此前根本
--   没有这张表，标记只可能来自"新节点依赖重放"。加入前的节点数不写死（走 \gset 取
--   增量），故本用例既能跑全量 hgdde 套件（3 节点注册，加完变 4），也能用
--   TECC_TEST=085_mac_distributed.sql 单独跑（同样 3 节点，加完变 4）。
--   用例结束后集群多一个节点，后续用例（090+）的断言不依赖节点数量，故不受影响。
--
-- 期望文件口径：分片名用 SET hgdde.next_shard_id 固定（否则第 10.9 节那条被拒 INSERT
--   的报错里会带每轮都变的分片名）；worker 端口一律不出现在断言里
--   （`\set VERBOSITY terse` 抑制带端口的 CONTEXT/DETAIL 行）。

\set ON_ERROR_STOP on
\set QUIET on
SET client_min_messages = warning;
SET hgdde.shard_count = 8;
-- 固定本用例的分片 id，让第 10.9 节那条报错里的分片名可复现（090 用 900000）
SET hgdde.next_shard_id = 910000;

DROP TABLE IF EXISTS ddb_mac_t CASCADE;
DROP TABLE IF EXISTS ddb_mac_t2 CASCADE;
DROP ROLE IF EXISTS ddb_mac_u10;
DROP ROLE IF EXISTS ddb_mac_u5;

\echo '=== 10.0 前置：每个节点都打开了总开关（行级规则由钩子提供，受 global_on 管辖）==='
-- 四个开关一起打印，把运行环境钉死：global_on/global_row_level 必须 on、
-- global_table_level/global_column_level 保持默认 off（见文件头「前置」）。
-- 只输出"各节点是否一致 + 那一档是什么 + 是否每个节点都符合"，不输出节点数量，
-- 故与拓扑规模无关
SELECT count(DISTINCT result) AS distinct_env, min(result) AS env,
       count(*) = count(*) FILTER (WHERE result = 'on/on/off/off') AS all_nodes_expected
  FROM run_command_on_all_nodes($$
        SELECT current_setting('hg_sysmac.global_on') || '/' ||
               current_setting('hg_sysmac.global_row_level') || '/' ||
               current_setting('hg_sysmac.global_table_level') || '/' ||
               current_setting('hg_sysmac.global_column_level')
      $$);

\echo '=== 10.1 角色 + 用户标记（角色必须 LOGIN：Citus 要以该角色身份连 worker 取分片）==='
CREATE ROLE ddb_mac_u10 LOGIN;
CREATE ROLE ddb_mac_u5  LOGIN;
SELECT set_user_seclabel('ddb_mac_u10'::regrole, 's10:c1,c2');
SELECT set_user_seclabel('ddb_mac_u5'::regrole,  's5:c1,c2');

\echo '=== 10.2 用户标记在全部节点一致（T4.2：走标准 SQL 路径，由 Citus 自己转发角色标记）==='
SELECT count(DISTINCT result) AS distinct_labels, min(result) AS labels,
       count(*) = count(*) FILTER (WHERE result = 's10:c1.c2') AS all_nodes_expected
  FROM run_command_on_all_nodes($$
        SELECT coalesce((SELECT label FROM pg_shseclabel
                          WHERE provider = 'sysmac'
                            AND objoid = (SELECT oid FROM pg_authid WHERE rolname = 'ddb_mac_u10')),
                        '<none>')
      $$);

\echo '=== 10.3 建表分发 + 表标记 + 列标记 + 行级规则 ==='
CREATE TABLE ddb_mac_t (id int PRIMARY KEY, name text);
SELECT create_distributed_table('ddb_mac_t', 'id');
SELECT set_table_seclabel('ddb_mac_t'::regclass, 's7:c1');
SELECT set_column_seclabel('ddb_mac_t'::regclass, 2, 's9:c2');
SELECT apply_row_policy('ddb_mac_t'::regclass, 'apply');
GRANT ALL ON ddb_mac_t TO ddb_mac_u10, ddb_mac_u5;

\echo '=== 10.4 表/列标记在协调节点 + 全部 worker 上逐字一致（T4.3 护栏）==='
-- 跨节点载体是 public.hg_sysmac_labels（每节点一张普通表，由 TECC 在标记变更/分发/
-- 加节点时整体推送；见 src/backend/commands/tecc_mac_distribution.c）。判定侧就读它。
-- 期望 0=s7:c1,2=s9:c2（0 = 表级标记，2 = name 列；apply_row_policy 后加的两个隐藏列
-- 追加在末尾，不影响 name 的 attnum）
SELECT count(DISTINCT result) AS distinct_labels, min(result) AS labels,
       count(*) = count(*) FILTER (WHERE result = '0=s7:c1,2=s9:c2') AS all_nodes_expected
  FROM run_command_on_all_nodes($$
        SELECT coalesce(string_agg(attnum::text || '=' || label, ',' ORDER BY attnum), '<none>')
          FROM public.hg_sysmac_labels
         WHERE schema_name = 'public' AND rel_name = 'ddb_mac_t'
      $$);

-- 协调节点本地的 pg_seclabel 必须与之一致（单机语义与 pg_dump 口径仍以它为准）
SELECT coalesce(string_agg(objsubid::text || '=' || label, ',' ORDER BY objsubid), '<none>')
       AS coordinator_pg_seclabel
  FROM pg_seclabel
 WHERE provider = 'sysmac' AND objoid = 'ddb_mac_t'::regclass;

\echo '=== 10.5 每个分片的形态：RLS/FORCE + 2 个隐藏列 + 3 个索引 + 1 条恒真占位策略 ==='
-- 3 个索引 = id 的主键索引 + 隐藏标记列的 btree 与 GIN(内置 array_ops)；
-- 1 条策略 = 只允许存在恒真 PERMISSIVE 占位（PG 规定没有 PERMISSIVE 策略就整表默认全拒），
-- 四条真规则不在 pg_policy 里（见文件头 ①）
SELECT count(*) AS shards, count(DISTINCT result) AS distinct_state, min(result) AS state
  FROM run_command_on_shards('ddb_mac_t', $q$
        SELECT (relrowsecurity::text || '/' || relforcerowsecurity::text ||
                ' cols=' || (SELECT count(*)::text FROM pg_attribute a
                              WHERE a.attrelid = c.oid AND a.attname LIKE '\_\_hg\_seclabel\_%%') ||
                ' idx='  || (SELECT count(*)::text FROM pg_index i WHERE i.indrelid = c.oid) ||
                ' pol='  || (SELECT count(*)::text FROM pg_policy p WHERE p.polrelid = c.oid) ||
                ' names=' || (SELECT coalesce(string_agg(p.polname, ',' ORDER BY p.polname), '<none>')
                                FROM pg_policy p WHERE p.polrelid = c.oid))
          FROM pg_class c WHERE c.oid = '%s'::regclass
      $q$);

\echo '=== 10.6 造数据（超级用户绕过策略，显式写行标记）==='
-- 8 行覆盖判定表的四个分支：比我低且类别可被包含 / 与我相等 / 比我高 / 类别不可比，
-- 以及"未设置"（level -1 + 空类别）那一档
INSERT INTO ddb_mac_t (id, name, __hg_seclabel_level__, __hg_seclabel_cats__) VALUES
 (1, 'a', 10, '{1,2}'),
 (2, 'b',  5, '{1,2}'),
 (3, 'c', 15, '{}'),
 (4, 'd', 10, '{1}'),
 (5, 'e', -1, '{}'),
 (6, 'f',  5, '{3}'),
 (7, 'g',  3, '{1,2}'),
 (8, 'h', 12, '{1,2}');

\echo '=== 10.7 行级 SELECT：分片上的过滤与《标记比较规则》一致 ==='
-- u10(s10:c1,c2) 期望 1,2,4,5,7：level<=10 且 cats ⊆ {1,2}
--   （3 太高、6 的 {3} 不可比、8 的 12 太高）
-- u5(s5:c1,c2)  期望 2,5,7：level<=5 且 cats ⊆ {1,2}
-- 超级用户绕过的行一并打印，用来证明"行还在，是按角色在分片执行时被过滤掉的"，
-- 而不是行根本不存在（D7 已接受的前提：超级用户不受强制访问控制约束）
SET ROLE ddb_mac_u10;
SELECT 'u10(s10:c1,c2)' AS who, coalesce(string_agg(id::text, ',' ORDER BY id), '<none>') AS visible FROM ddb_mac_t;
RESET ROLE;
SET ROLE ddb_mac_u5;
SELECT 'u5(s5:c1,c2)' AS who, coalesce(string_agg(id::text, ',' ORDER BY id), '<none>') AS visible FROM ddb_mac_t;
RESET ROLE;
SELECT 'superuser(bypass)' AS who, string_agg(id::text, ',' ORDER BY id) AS visible FROM ddb_mac_t;

\echo '=== 10.8 行级 UPDATE / DELETE：等号语义（只有 level = 我 且 cats = 我 的行被改到）==='
-- u10 只改到 id=1（level=10 cats={1,2}），u5 只删到 id=2（level=5 cats={1,2}）
--
-- hgdde 变体：原 Citus 写法用 `WITH u AS (UPDATE ... RETURNING 1) SELECT count(*) FROM u`
-- 取受影响行数，但 hgdde 的规划器不支持"只在 FROM 下方才能到达分布式表的 CTE"
-- （报 "reaches a distributed table only below its own FROM"）。这里改成等价的
-- `UPDATE/DELETE ... RETURNING <标记>`：每个被改到的行输出一行标记，断言语义不变
-- （u10 恰好 1 行、u5 恰好 1 行），且不依赖 CTE 支持。
SET ROLE ddb_mac_u10;
UPDATE ddb_mac_t SET name = name RETURNING 'u10 update' AS op;
RESET ROLE;
SET ROLE ddb_mac_u5;
DELETE FROM ddb_mac_t RETURNING 'u5 delete' AS op;
RESET ROLE;
SELECT id, name FROM ddb_mac_t WHERE id IN (1, 2, 3) ORDER BY id;

\echo '=== 10.9 行级 INSERT：默认取插入者标记；伪造标记被 worker 拒绝（报错带占位策略名）；表级标记支配插入者（V9-DD-1.2）==='
-- 正例：插入者 s5:c1,c2（level 5 <= 表级 s7:c1，cats {1,2} ⊇ {1}）-> 表级支配成立，放行
SET ROLE ddb_mac_u5;
INSERT INTO ddb_mac_t (id, name) VALUES (200, 'by-u5');
RESET ROLE;
-- 默认值就是插入者的标记，故落库的标记必须等于 ddb_mac_u5 的 s5:c1,c2
SELECT id, __hg_seclabel_level__, __hg_seclabel_cats__::text FROM ddb_mac_t WHERE id = 200;
-- 负例：插入者 s10:c1,c2（level 10 > 表级 7）-> 表级标记不被插入者标得更高，行级 INSERT
--      的"表级支配"检查（ExecutorCheckPerms → hg_sysmac_check_rte INSERT 分支）必须拒绝
\set VERBOSITY terse
\set ON_ERROR_STOP off
SET ROLE ddb_mac_u10;
INSERT INTO ddb_mac_t (id, name) VALUES (201, 'by-u10');
RESET ROLE;
-- 伪造：u5 显式写更低行标记 -> 被分片上的 WITH CHECK 拒绝。下面这条**故意失败**：
-- 关掉 ON_ERROR_STOP 让用例继续，报错本身就是要固化的断言（分片名由
-- hgdde.next_shard_id 固定；CONTEXT/DETAIL 里的 worker 端口由 VERBOSITY terse 抑制）
SET ROLE ddb_mac_u5;
INSERT INTO ddb_mac_t (id, name, __hg_seclabel_level__) VALUES (202, 'forge-low', 3);
RESET ROLE;
\set ON_ERROR_STOP on
SELECT count(*) AS forged_rows FROM ddb_mac_t WHERE id = 202;

\echo '=== 10.10 加入一个待加入的新节点（该实例此前根本没有这张表）==='
-- 端口由 TAP 驱动经 %ENV:TECC_HGDDE_W3_PORT% 传入（每轮随机，不能写死在 SQL 里）；
-- 加入前的节点数用 \gset 取下来，断言"只多了 1 个"，不写死规模
SELECT count(*) AS nodes_before FROM pg_dist_node
\gset
SELECT hgdde_add_node('localhost', %ENV:TECC_HGDDE_W3_PORT%) > 0 AS added;
SELECT count(*) = :nodes_before + 1 AS one_node_added FROM pg_dist_node;
-- 新节点也必须打开总开关（它靠 postgresql.conf，不靠 run_command_on_all_nodes ——
-- 加节点之前那条路够不到它）
SELECT count(*) = count(*) FILTER (WHERE result = 'on') AS all_nodes_on
  FROM run_command_on_all_nodes($$ SHOW hg_sysmac.global_on $$);

\echo '=== 10.11 新节点也拿到 表/列标记；分片形态仍是 RLS + 隐藏列 + 占位策略（T4.4 核心护栏）==='
-- 全部节点的状态必须**逐字相同**，且都带上 0=s7:c1,2=s9:c2：新节点上此前根本没有
-- 这张表，标记只能来自 hgdde_add_node 成功之后的整体补发
-- （tecc_dist_bootstrap.c → TeccMacSyncLabels）
SELECT count(DISTINCT result) AS distinct_state,
       count(*) = count(*) FILTER (WHERE result LIKE '%labels=0=s7:c1,2=s9:c2%') AS all_nodes_have_labels,
       min(result) AS state
  FROM run_command_on_all_nodes($q$
        SELECT (relrowsecurity::text || '/' || relforcerowsecurity::text ||
                ' cols=' || (SELECT count(*)::text FROM pg_attribute a
                              WHERE a.attrelid = c.oid AND a.attname LIKE '\_\_hg\_seclabel\_%%') ||
                ' pol=' || (SELECT count(*)::text FROM pg_policy p WHERE p.polrelid = c.oid) ||
                ' labels=' || coalesce((SELECT string_agg(d.attnum::text || '=' || d.label,
                                                          ',' ORDER BY d.attnum)
                                          FROM public.hg_sysmac_labels d
                                         WHERE d.schema_name = 'public'
                                           AND d.rel_name = 'ddb_mac_t'), '<none>'))
          FROM pg_class c WHERE c.oid = 'ddb_mac_t'::regclass
      $q$);

\echo '=== 10.12 新节点能承载 MAC 分片：同构表的 8 个分片铺满全部 worker 且形态一致 ==='
-- colocate_with => 'none' 是必须的：否则新表会并进前面用例的默认 colocation
-- （那组分片只落在原来的 worker 上），一个新分片都不会落到新节点（同 080 的说明）
CREATE TABLE ddb_mac_t2 (id int, v text);
SELECT create_distributed_table('ddb_mac_t2', 'id', colocate_with => 'none');
SELECT apply_row_policy('ddb_mac_t2'::regclass, 'apply');
GRANT ALL ON ddb_mac_t2 TO ddb_mac_u10;
-- 8 个分片要铺满"所有应该持有分片的节点"（协调节点的 shouldhaveshards 是 false），
-- 并单独断言其中有一个落在刚加入的新节点上；两者都是拓扑无关的布尔
SELECT count(*) AS shards,
       count(DISTINCT p.nodeport) = (SELECT count(*) FROM pg_dist_node
                                    WHERE isactive AND shouldhaveshards) AS covers_all_workers
  FROM pg_dist_shard s JOIN pg_dist_shard_placement p USING (shardid)
 WHERE s.logicalrelid = 'ddb_mac_t2'::regclass;
SELECT count(*) > 0 AS new_node_has_shard
  FROM pg_dist_shard s JOIN pg_dist_shard_placement p USING (shardid)
 WHERE s.logicalrelid = 'ddb_mac_t2'::regclass
   AND p.nodeport = %ENV:TECC_HGDDE_W3_PORT%;
SELECT count(*) AS shards, count(DISTINCT result) AS distinct_state, min(result) AS state
  FROM run_command_on_shards('ddb_mac_t2', $q$
        SELECT (relrowsecurity::text || '/' || relforcerowsecurity::text ||
                ' cols=' || (SELECT count(*)::text FROM pg_attribute a
                              WHERE a.attrelid = c.oid AND a.attname LIKE '\_\_hg\_seclabel\_%%') ||
                ' pol='  || (SELECT count(*)::text FROM pg_policy p WHERE p.polrelid = c.oid))
          FROM pg_class c WHERE c.oid = '%s'::regclass
      $q$);
-- 新节点上的分片同样按角色过滤：u10(s10:c1,c2) 只该看到 level<=10 且 cats ⊆ {1,2} 的 id=1
INSERT INTO ddb_mac_t2 (id, v, __hg_seclabel_level__, __hg_seclabel_cats__) VALUES
 (1, 'x', 10, '{1,2}'), (2, 'y', 20, '{1,2}');
SET ROLE ddb_mac_u10;
SELECT 'u10 on t2' AS who, coalesce(string_agg(id::text, ',' ORDER BY id), '<none>') AS visible FROM ddb_mac_t2;
RESET ROLE;
