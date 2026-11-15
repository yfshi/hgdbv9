/*-------------------------------------------------------------------------
 *
 * tecc_key_recipe.h
 *	  KEK/DEK "可移植重建配方" 存储（系统表 hg_tecc_ddl）的对外接口。
 *
 * ReqID:TECC4.0
 *
 * 这一层原来放在 contrib/citus 里（因为最初的触发点是 Citus 的依赖机制）。
 * 按"外围适配"的要求搬进 PG 核心：它只读写本仓库自己的系统表，与分布式插件
 * 无关；插件侧（Citus / hgdde）只需要在"问某个对象怎么重建"时调这里。
 *
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd.
 * Author: <wangxp@highgo.com>
 *
 * IDENTIFICATION
 *	  src/include/commands/tecc_key_recipe.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_KEY_RECIPE_H
#define TECC_KEY_RECIPE_H

#include "nodes/pg_list.h"

/* ReqID:TECC4.0 */
extern bool TeccKeyObjectIdentity(Oid catalogId, Oid objectId, Oid *namespaceId,
								  char **objectName);
extern List *TeccKeyRecipeCommands(Oid catalogId, Oid objectId);
extern void TeccKeyRecipeAppend(Oid objectClass, Oid namespaceId,
								const char *objectName, const char *command,
								List **seqTracker);
extern void TeccKeyRecipeForget(Oid objectClass, Oid objectId);

#endif							/* TECC_KEY_RECIPE_H */