/*-------------------------------------------------------------------------
 *
 * tecc_conn_type.c
 *	  TECC connection type marking system
 *
 * The libpq frontend marks TECC internal connections (metadata fetcher
 * temp connections and key manager pool connections - connections opened
 * while a psql-to-backend connection already exists) by sending the
 * startup packet parameter "hg_tecc.conn_type" with the value "internal".
 *
 * This module defines the matching backend GUC so the value lands in
 * guc_options processing (postmaster.c routes unrecognized startup packet
 * parameters into port->guc_options; InitPostgres applies them via
 * process_startup_options()). The GUC uses PGC_BACKEND context, so it can
 * be set only at connection start, not changed later by SET.
 *
 * Backend code distinguishes internal connections from ordinary user
 * connections via tecc_is_internal_connection().
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  src/backend/utils/init/tecc_conn_type.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "utils/guc.h"
#include "utils/tecc_conn_type.h"

/*
 * Backend variable backing the hg_tecc.conn_type GUC.
 *
 * - Default "normal": ordinary user connection.
 * - "internal": connection opened by the TECC frontend for internal
 *   metadata / key management queries.
 */
static char *tecc_conn_type_var = NULL;

/* ReqID:TECC4.0 - pg_upgrade binary upgrade 模式开关。
 * pg_dump --binary-upgrade 的 dump 在 PRE_DATA 段输出
 * "SET hg_tecc.binary_upgrade = on"，pg_restore 恢复 DDL 时生效：
 * CREATE TABLE ... ENCRYPTED WITH 跳过 DEK 存在性校验与元数据记录
 * （hg_dek/hg_encrypted_columns 数据经 pg_upgrade 文件拷贝转移）。 */
static bool tecc_binary_upgrade_var = false;

/* ReqID:TECC4.0 - 元数据缺失时的失败关闭档位（见头文件里的语义说明）。
 * 默认 error：声明成加密类型却查不到 hg_encrypted_columns 行的列，
 * 一律直接报错，而不是静默当成普通列继续。 */
static int	tecc_strict_metadata_var = TECC_STRICT_METADATA_ERROR;

static const struct config_enum_entry tecc_strict_metadata_options[] =
{
	{"off", TECC_STRICT_METADATA_OFF, false},
	{"warn", TECC_STRICT_METADATA_WARN, false},
	{"error", TECC_STRICT_METADATA_ERROR, false},
	{NULL, 0, false}
};

/*
 * Initialize the hg_tecc.conn_type GUC.
 *
 * Must be called once during backend initialization, BEFORE
 * process_startup_options(), so that a startup packet parameter named
 * "hg_tecc.conn_type" is applied to a defined GUC. On a server without
 * this definition the dotted name is silently stored as a placeholder
 * and ignored, so connections never fail on name mismatch.
 */
void
tecc_init_conn_type(void)
{
	DefineCustomStringVariable("hg_tecc.conn_type",
							   "TECC connection type (normal or internal)",
							   NULL,
							   &tecc_conn_type_var,
							   "normal",
							   PGC_BACKEND,
							   GUC_NOT_IN_SAMPLE,
							   NULL, NULL, NULL);

	DefineCustomBoolVariable("hg_tecc.binary_upgrade",
							 "TECC binary upgrade mode (pg_upgrade restore)",
							 NULL,
							 &tecc_binary_upgrade_var,
							 false,
							 PGC_SUSET,
							 GUC_SUPERUSER_ONLY | GUC_NOT_IN_SAMPLE,
							 NULL, NULL, NULL);

	/*
	 * ReqID:TECC4.0 - 元数据缺失的失败关闭开关。
	 *
	 * PGC_USERSET 是刻意的：这两类会话都必须能就地切档，而它们通常不是
	 * 超级用户——(1) Citus 的内部连接（worker 上执行分片语句），(2) 运维
	 * 侧用 PGOPTIONS 临时绕过的 pg_restore/psql 会话。把"更严格"做成
	 * 需要权限反而会把用户逼回去静默降级。
	 */
	DefineCustomEnumVariable("hg_tecc.strict_metadata",
							 "TECC reaction when an encrypted-typed column has no encryption metadata",
							 "off keeps the legacy silent fallback, warn reports and continues, "
							 "error fails closed.",
							 &tecc_strict_metadata_var,
							 TECC_STRICT_METADATA_ERROR,
							 tecc_strict_metadata_options,
							 PGC_USERSET,
							 GUC_NOT_IN_SAMPLE,
							 NULL, NULL, NULL);
}

/*
 * Report whether the current backend connection is a TECC internal
 * connection.
 */
bool
tecc_is_internal_connection(void)
{
	return tecc_conn_type_var != NULL &&
		strcmp(tecc_conn_type_var, "internal") == 0;
}

/*
 * ReqID:TECC4.0 - Report whether TECC binary upgrade mode is active
 * (pg_upgrade schema restore phase). Backend code uses this to skip
 * DEK existence validation and encrypted column metadata recording.
 */
bool
tecc_is_binary_upgrade(void)
{
	return tecc_binary_upgrade_var;
}

/*
 * ReqID:TECC4.0 - Return the current hg_tecc.strict_metadata level.
 */
TeccStrictMetadata
tecc_get_strict_metadata(void)
{
	return (TeccStrictMetadata) tecc_strict_metadata_var;
}
