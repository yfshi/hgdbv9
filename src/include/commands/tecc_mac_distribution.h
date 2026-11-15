/*-------------------------------------------------------------------------
 *
 * tecc_mac_distribution.h
 *	  Keep mandatory-access-control (label provider "sysmac") labels consistent
 *	  across a distributed cluster, without touching the plugin's source.
 *
 * ReqID:TECC4.0
 *
 * 表/列标记存在 pg_seclabel 里，**不属于**分布式插件的元数据；而"判定在数据所在
 * 节点"要求协调节点、每个 worker、以及**后加入的节点**上的标记完全一致。
 *
 * 做法（与连接访问控制的规则表同款）：把分布式表的标记记在一张 **Citus 参考表**
 * `hg_security.distributed_labels` 里 —— 参考表的内容由插件自己保证与本地事务
 * 原子地复制到全部节点，新节点加入时自动带副本。于是：
 *   - 不需要"把标记语句下发到各 worker"，也没有"表还没建好、标记先到"的顺序问题；
 *   - 判定侧优先读这张参考表，读不到再回退到 pg_seclabel（单机 / 未分发的表）。
 *
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd.
 * Author: <wangxp@highgo.com>
 *
 * IDENTIFICATION
 *	  src/include/commands/tecc_mac_distribution.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_MAC_DISTRIBUTION_H
#define TECC_MAC_DISTRIBUTION_H

#include "postgres.h"

/*
 * 后端核心（commands/tecc_dist_bootstrap.c）与标记强访实现之间的调用缝。
 *
 * 标记强访自 2026-11-09 起是预加载扩展 contrib/hg_sysmac；核心侧只在"拓扑变化后
 * 补发标记"这一处需要触发它，故走函数指针：扩展的 _PG_init 赋值，未加载扩展时
 * 保持 NULL（等价于"没有标记强访"，与单机库一致）。
 */
typedef void (*tecc_mac_sync_labels_hook_type) (void);
extern PGDLLIMPORT tecc_mac_sync_labels_hook_type tecc_mac_sync_labels_hook;

/*
 * ReqID:TECC4.0 - 安装 utility hook（把标记写进承载表）并注册"分片 → 逻辑表"的
 * 标记解析钩子。没有分布式插件的单机库上两者都是无操作。
 *
 * 安装点：扩展的 _PG_init（shared_preload_libraries）。原先是 InitPostgres()。
 */
extern void tecc_install_mac_distribution_hooks(void);

/*
 * ReqID:TECC4.0 - 读取关系的分布式标记（承载表里那一行）。
 *
 * 返回 true 表示"这个标记由承载表负责"：*label 是标记文本，没有记录时为 NULL。
 * 返回 false 表示承载表不适用于这个关系（单机、或该表根本没有分布在承载表里），
 * 调用方应回退到 pg_seclabel。
 *
 * 判据是"承载表里有没有这一行"而不是"这张表是否已分发"：前者只需一次索引查找，
 * 且与写入侧严格对称；后者要在每次检查时都去扫 pg_dist_partition。
 */
extern bool TeccMacDistributedLabel(Oid relid, int32 attnum, char **label);

/*
 * ReqID:TECC4.0 - 重算本地承载表（从 pg_seclabel 取"已分发关系的标记"）并把整份
 * 内容推给每个 worker。用在三处：标记变更、表刚被分发、新节点加入。
 * 非协调节点或没有分布式插件时是空操作。
 */
extern void TeccMacSyncLabels(void);

/*
 * ReqID:TECC4.0 - 保证每个带行标记列的关系（分片、以及 worker 上的父表副本）都有
 * 占位策略 hg_sysmac_row_all。用在两处：apply_row_policy 之后、以及 TeccMacSyncLabels
 * 里（新节点加入时补发）。插件无关、幂等：Citus 已转发过的分片不会被重复建。
 */
extern void TeccMacSyncRowPolicies(void);

/*
 * ReqID:TECC4.0 - 登记"提交后补发分片占位策略"。apply_row_policy 与 TeccMacSyncLabels
 * 都只能登记（本事务里插件的分片 DDL 锁还在，立刻下发会互相等待），真正的下发在
 * 下一个语句解析时执行。
 */
extern void TeccMacRequestRowPolicySync(void);

/*
 * ReqID:TECC4.0 - 该关系是否是分布式插件的"已分发对象"（含参考表 / citus local 表）。
 *
 * 判据是 pg_dist_partition 里有没有它 —— 与插件自己的 IsCitusTable 等价，但只用到
 * SQL 层可见的目录。插件未安装时返回 false。COPY 上的标记检查要用同一个判据，
 * 所以对外暴露。
 */
extern bool TeccMacRelationIsDistributed(Oid relid);

#endif							/* TECC_MAC_DISTRIBUTION_H */