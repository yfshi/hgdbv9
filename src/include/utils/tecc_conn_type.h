/*-------------------------------------------------------------------------
 *
 * tecc_conn_type.h
 *	  TECC connection type marking system declarations
 *
 * This module defines the backend-side GUC hg_tecc.conn_type, which is set
 * from the connection's startup packet. TECC internal connections (metadata
 * fetcher temp connections, key manager pool connections) are marked with
 * the value "internal" by the libpq frontend; ordinary user connections
 * carry the default value "normal". Backend code can distinguish them via
 * tecc_is_internal_connection().
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  src/include/utils/tecc_conn_type.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_CONN_TYPE_H
#define TECC_CONN_TYPE_H

#include "postgres.h"

/*
 * Initialize the hg_tecc.conn_type GUC.
 *
 * Must be called once during backend initialization (from InitPostgres),
 * BEFORE process_startup_options() is invoked, so that the startup packet
 * parameter "hg_tecc.conn_type" is applied to a defined GUC.
 */
extern void tecc_init_conn_type(void);

/*
 * Return true if the current backend connection is a TECC internal
 * connection (hg_tecc.conn_type = 'internal'), as opposed to a normal
 * user connection.
 */
extern bool tecc_is_internal_connection(void);

/*
 * ReqID:TECC4.0 - Return true if TECC binary upgrade mode is active
 * (hg_tecc.binary_upgrade = on, set by pg_dump --binary-upgrade dump
 * during pg_upgrade schema restore). Backend code uses this to skip
 * DEK existence validation and encrypted column metadata recording.
 */
extern bool tecc_is_binary_upgrade(void);

/*
 * ReqID:TECC4.0 - hg_tecc.strict_metadata 的三档取值。
 *
 * 语义：当某列**声明成加密类型**（atttypid 属 50000–50008）却在
 * hg_encrypted_columns 里查不到对应元数据行时，后端各消费点原先把该列
 * 静默当作普通列继续。分布式形态下（Citus 分片表在 W2 之前就是这样）
 * 这会让服务端按错误语义处理密文，且不报错——属"能跑但结果可能错"的
 * 最坏形态。
 *
 *	off   —— 保持旧的静默降级行为（仅用于临时绕过 / 兼容旧数据）。
 *	warn  —— 发出 WARNING 后按普通列继续，便于排查而不阻断业务。
 *	error —— 直接 ERROR 失败关闭（默认值）。
 */
typedef enum TeccStrictMetadata
{
	TECC_STRICT_METADATA_OFF = 0,
	TECC_STRICT_METADATA_WARN,
	TECC_STRICT_METADATA_ERROR
} TeccStrictMetadata;

/*
 * Return the current hg_tecc.strict_metadata level.
 */
extern TeccStrictMetadata tecc_get_strict_metadata(void);

#endif							/* TECC_CONN_TYPE_H */
