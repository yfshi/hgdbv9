/*-------------------------------------------------------------------------
 *
 * tecc_dist_bootstrap.h
 *	  Catch up the rest of the cluster after a topology change.
 *
 * ReqID:TECC4.0
 *
 * "新节点加入"与"表刚被分发"这两件事发生时，协调节点上已经存在、但目标节点上还
 * 没有的东西（KEK/DEK、表/列标记）必须补发过去 —— 否则新节点上的加密分片 DDL 会
 * 以 "DEK does not exist" 失败，表级/列级强制访问检查会因"标记未设置"静默放行。
 *
 * 原实现靠分布式插件的对象同步（把 KEK/DEK 登记成分布式对象 + 依赖重放）；外围化
 * 之后改由本模块在**语句成功执行之后**用 SQL 层通道补发，插件侧不必认识它们。
 *
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd.
 * Author: <wangxp@highgo.com>
 *
 * IDENTIFICATION
 *	  src/include/commands/tecc_dist_bootstrap.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_DIST_BOOTSTRAP_H
#define TECC_DIST_BOOTSTRAP_H

/*
 * ReqID:TECC4.0 - 安装"拓扑变化后补发"的三个钩子（post_parse_analyze +
 * ExecutorStart/ExecutorEnd）。单机库上整套机制空转：触发函数根本解析不到 OID。
 */
extern void tecc_install_dist_bootstrap_hooks(void);

#endif							/* TECC_DIST_BOOTSTRAP_H */