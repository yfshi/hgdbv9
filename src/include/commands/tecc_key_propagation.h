/*-------------------------------------------------------------------------
 *
 * tecc_key_propagation.h
 *	  Propagate TECC key (KEK/DEK) DDL to every worker of a distributed cluster.
 *
 * ReqID:TECC4.0
 *
 * 本模块是 W3 的外围化落点：原先挂在分布式插件（contrib/citus）的 utility hook
 * 上、用插件内部的 DDL 任务机制下发；现在改由 PG 核心的 utility hook
 * postprocess 段通过 **SQL 层** `run_command_on_workers()` 下发，因而对 Citus 与
 * hgdde 通用（两者的 SQL 函数同名同签名）。
 *
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd.
 * Author: <wangxp@highgo.com>
 *
 * IDENTIFICATION
 *	  src/include/commands/tecc_key_propagation.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_KEY_PROPAGATION_H
#define TECC_KEY_PROPAGATION_H

#include "catalog/objectaddress.h"
#include "nodes/parsenodes.h"
#include "nodes/pg_list.h"

/* ReqID:TECC4.0 - install the KEK/DEK propagation utility hook */
extern void tecc_install_key_propagation_hooks(void);

/*
 * ReqID:TECC4.0 - replay the currently executing KEK/DEK statement on all workers.
 * Call from the utility hook's postprocess stage.
 */
extern void TeccKeyPropagateIfNeeded(Node *parsetree, const char *queryString,
									 int stmt_location, int stmt_len);

/*
 * ReqID:TECC4.0 - replay the reconstruction recipe of **every** KEK/DEK on every
 * worker.
 *
 * 只用于"新节点刚加入"之后的补发（见 commands/tecc_dist_bootstrap.c）：插件的
 * 依赖重放发生在 add_node **内部**、壳表创建之前，而这条路径跑在语句成功之后，
 * 因此它只能覆盖"加节点那一刻还没有壳表要建"的情形。两处并存，互为补强。
 */
extern void TeccKeyReplayAllOnWorkers(void);

/*
 * ReqID:TECC4.0 - 协调节点上是否**已把 KEK/DEK 登记为分布式对象**（pg_dist_object 有行）。
 *
 * 登记了就说明插件会在 add_node 内部重放这些密钥，此时 TECC 自己**不能**再补发一遍：
 * 两份重放同时落在新节点的同一张 hg_kek/hg_dek 上，后发的那条会被先发的（尚未提交的）
 * 事务挡住，而先发的事务要等 add_node 提交才能结束 —— 实测把 citus_add_node 挂死
 * （协调节点等待新节点、新节点等待协调节点），整个加节点动作卡在 ExecutorEnd 里。
 */
extern bool TeccKeyRegisteredAsDistributed(void);

/*
 * ReqID:TECC4.0 - resolve CREATE KEK / CREATE DEK to an ObjectAddress. Used by the
 * distributed plugin's object framework when it registers these catalogs as
 * distributed objects (dependency tracking / new-node replay).
 */
extern List * CreateKekStmtObjectAddress(Node *node, bool missing_ok,
										 bool isPostprocess);
extern List * CreateDekStmtObjectAddress(Node *node, bool missing_ok,
										 bool isPostprocess);

#endif							/* TECC_KEY_PROPAGATION_H */