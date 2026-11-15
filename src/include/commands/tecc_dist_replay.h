/*-------------------------------------------------------------------------
 *
 * tecc_dist_replay.h
 *	  Replay a SQL text on the worker nodes of a distributed cluster.
 *
 * ReqID:TECC4.0
 *
 * 外围适配的公共通道：TECC 的若干能力（KEK/DEK 传播、表/列标记传播、新节点补发）
 * 都需要"把一段 SQL 送到集群里其他节点上执行"。按与 hgdde 团队的约定，分布式插件
 * 的 **SQL 层**函数可以按同名同签名依赖，因此这里只用
 * `pg_catalog.run_command_on_workers(text, boolean)` 与
 * `pg_catalog.citus_is_coordinator()`，不依赖插件的任何 C 层接口。
 *
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd.
 * Author: <wangxp@highgo.com>
 *
 * IDENTIFICATION
 *	  src/include/commands/tecc_dist_replay.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_DIST_REPLAY_H
#define TECC_DIST_REPLAY_H

/*
 * ReqID:TECC4.0 - 分布式插件的 SQL 入口是否可用。
 *
 * 单机库（未安装插件）里两个函数都不存在，所有外围传播因此整体退化为空操作。
 */
extern bool TeccDistPluginAvailable(void);

/*
 * ReqID:TECC4.0 - 本节点是否为集群的协调节点。
 *
 * 插件未安装、本节点是 worker、或函数不可解析时一律返回 false —— 这是"不要在
 * worker 上二次传播"的可靠判据（worker 上重放到达的 DDL 同样是普通语句）。
 */
extern bool TeccDistIsCoordinator(void);

/*
 * ReqID:TECC4.0 - 把一段 SQL 送到**每个** worker 上执行。
 *
 * 任意节点失败即 ereport(ERROR)：协调节点上已经生效的改动会随事务一起回滚，
 * 集群不会停在"有的节点有、有的节点没有"的半途状态（失败关闭）。what 是出错
 * 文案里的对象描述（例如 "TECC key statement"）。
 *
 * 单个节点上的多条语句会落在同一个隐式事务里，因此"改一半"不会留在某个节点上。
 * 插件不可用时直接返回（单机行为不变）。
 */
extern void TeccDistReplayOnWorkers(const char *sql, const char *what);

/*
 * ReqID:TECC4.0 - 把一条语句包进"临时关掉插件 DDL 传播"的括号里。
 *
 * 为什么需要：插件的 DDL 传播在 worker 上同样有护栏。按 Citus 语义，一条
 * `SECURITY LABEL ... ON ROLE` 若带着 `enable_ddl_propagation = on` 到达
 * worker，会被 `EnsureCoordinator()` 拒绝：
 *	 ERROR: operation is not allowed on this node
 *	 HINT:  Connect to the coordinator and run it again.
 * hgdde 自 `9d7f2b6b`（补 SECURITY LABEL 传播）起与 Citus 完全对齐；插件自己的
 * `NodeDDLTaskList()` 因此把语句包成
 *	 SET <plugin>.enable_ddl_propagation TO off; <sql>; SET ... TO on;
 * 再下发。TECC 的标记回放走的是 `run_command_on_workers()`，必须做同样的包装，
 * 否则新版本插件会在 worker 上拒绝角色标记。
 *
 * 返回 palloc 的字符串；插件不可用时返回 sql 的副本（行为不变）。
 */
extern char *TeccDistBracketDisableDDLPropagation(const char *sql);

#endif							/* TECC_DIST_REPLAY_H */