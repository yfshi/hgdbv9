/*-------------------------------------------------------------------------
 *
 * tecc_shard_ddl.h
 *	  Rebuild ENCRYPTED WITH clauses on shard DDL that arrives without them.
 *
 * ReqID:TECC4.0
 *
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd.
 * Author: <wangxp@highgo.com>
 *
 * IDENTIFICATION
 *	  src/include/commands/tecc_shard_ddl.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_SHARD_DDL_H
#define TECC_SHARD_DDL_H

/* ReqID:TECC4.0 - install the shard DDL clause-rebuild hook */
extern void tecc_install_shard_ddl_hooks(void);

#endif							/* TECC_SHARD_DDL_H */
