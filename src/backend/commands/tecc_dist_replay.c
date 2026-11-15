/*-------------------------------------------------------------------------
 *
 * tecc_dist_replay.c
 *	  Replay a SQL text on the worker nodes of a distributed cluster.
 *
 * ReqID:TECC4.0
 *
 * 为什么落在 SQL 层（而不是插件的 C 接口）：
 *	  按与 hgdde 团队的约定，hgdde 完全克隆 Citus 的 SQL 层接口（函数名与签名
 *	  一致），但不提供 Citus 的 C 层内部 API。因此凡是"要把语句送到其他节点"
 *	  的能力，都只能建立在 `run_command_on_workers()` / `citus_is_coordinator()`
 *	  这类 SQL 函数上 —— 这样同一份代码在 Citus（行为替身）与 hgdde（目标产品）
 *	  上都成立，且 `git diff contrib/citus` 保持为零。
 *
 * 失败语义（失败关闭）：
 *	  任一 worker 报错就 ereport(ERROR) 让整个语句回滚。这条底线来自"不写入错误
 *	  数据"：协调节点上有一条 KEK 而某个 worker 上没有，后续在该 worker 上建加密
 *	  分片表会以 "DEK does not exist" 失败，且错误现场离真正的原因很远。
 *
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd.
 * Author: <wangxp@highgo.com>
 *
 * IDENTIFICATION
 *	  src/backend/commands/tecc_dist_replay.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "executor/spi.h"
#include "lib/stringinfo.h"
#include "miscadmin.h"
#include "utils/builtins.h"
#include "utils/memutils.h"

#include "commands/tecc_dist_replay.h"

/*
 * 插件探测结果缓存。
 *
 * 只在"探测到了"时缓存：扩展可能在同一个会话里稍后 CREATE，缓存住"不存在"会让
 * 该会话此后一直不传播。探测本身是两次 syscache 查找，只有外围传播这条罕见路径
 * 会走到，重复探测的代价可以忽略。
 */
static bool tecc_dist_plugin_probed = false;

/*
 * ReqID:TECC4.0
 */
bool
TeccDistPluginAvailable(void)
{
	int			ret;
	bool		isnull = true;
	Datum		value;
	bool		present = false;

	if (tecc_dist_plugin_probed)
	{
		return true;
	}

	if (SPI_connect() != SPI_OK_CONNECT)
	{
		return false;
	}

	ret = SPI_execute("SELECT pg_catalog.to_regprocedure("
					  "'pg_catalog.run_command_on_workers(text, boolean)') IS NOT NULL",
					  true, 1);
	if (ret == SPI_OK_SELECT && SPI_processed == 1)
	{
		value = SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1,
							  &isnull);
		present = !isnull && DatumGetBool(value);
	}

	SPI_finish();

	if (present)
	{
		tecc_dist_plugin_probed = true;
	}

	return present;
}


/*
 * Resolve which coordinator predicate the catalog has, once per backend.
 *
 * hgdde renames the Citus-prefixed UDFs to hgdde_* (it keeps master_* aliases
 * and the pg_dist_* tables, but not citus_is_coordinator), while Citus keeps
 * the Citus name. A plain SPI_execute against a missing function raises, not
 * returns an error code, so the predicate's existence is probed first via
 * to_regprocedure and only then executed. The result is cached process-wide
 * (backends are single-threaded); a reload of the extension does not change it
 * within a session.
 */
static int
tecc_dist_coordinator_flavor(void)
{
	static int flavor = -1;	/* -1 unknown, 0 none, 1 hgdde, 2 citus */
	int			ret;
	bool		isnull = true;
	Datum		value;

	if (flavor != -1)
	{
		return flavor;
	}

	if (SPI_connect() != SPI_OK_CONNECT)
	{
		flavor = 0;	/* 暂时失败时返回未知，调用方按"不是协调节点"处理 */
		return flavor;
	}

	ret = SPI_execute("SELECT CASE "
					  "  WHEN to_regprocedure('pg_catalog.hgdde_is_coordinator()') IS NOT NULL THEN 1 "
					  "  WHEN to_regprocedure('pg_catalog.citus_is_coordinator()') IS NOT NULL THEN 2 "
					  "  ELSE 0 "
					  "END", true, 1);
	if (ret == SPI_OK_SELECT && SPI_processed == 1)
	{
		value = SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1,
							  &isnull);
		if (!isnull)
		{
			flavor = (int) DatumGetInt32(value);
		}
	}
	SPI_finish();

	if (flavor == -1)
	{
		flavor = 0;	/* 无分布式插件 */
	}
	return flavor;
}


/*
 * ReqID:TECC4.0
 */
bool
TeccDistIsCoordinator(void)
{
	int			ret;
	int			flavor = tecc_dist_coordinator_flavor();
	bool		isnull = true;
	Datum		value;
	bool		result = false;

	if (!TeccDistPluginAvailable() || flavor == 0)
	{
		return false;
	}

	if (SPI_connect() != SPI_OK_CONNECT)
	{
		return false;
	}

	ret = SPI_execute(flavor == 1
					  ? "SELECT pg_catalog.hgdde_is_coordinator()"
					  : "SELECT pg_catalog.citus_is_coordinator()", true, 1);
	if (ret == SPI_OK_SELECT && SPI_processed == 1)
	{
		value = SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1,
							  &isnull);
		result = !isnull && DatumGetBool(value);
	}

	SPI_finish();

	return result;
}


/*
 * ReqID:TECC4.0
 *
 * 给一条语句加上"关掉插件 DDL 传播"的括号。
 *
 * 用 `SET` 而不是 `SET LOCAL`：`run_command_on_workers()` 在 worker 上是一条
 * 独立的简单协议连接，不保证处于事务块中；`SET LOCAL` 在事务块外只发一个
 * WARNING 并且**不生效**，护栏反而绕不过去。多语句在同一个隐式事务里执行，
 * 连接随命令结束关闭，末尾的 `SET ... TO on` 只是恢复语义、不影响结果判定
 * （最后一条是 `SET`，命令标签 `SET` 即 `PGRES_COMMAND_OK`）。
 */
char *
TeccDistBracketDisableDDLPropagation(const char *sql)
{
	int			flavor = tecc_dist_coordinator_flavor();
	const char *prefix;
	StringInfoData buf;

	if (flavor == 2)
		prefix = "citus";
	else if (flavor == 1)
		prefix = "hgdde";
	else
		return pstrdup(sql);

	initStringInfo(&buf);
	appendStringInfo(&buf,
					 "SET %s.enable_ddl_propagation TO off; %s; "
					 "SET %s.enable_ddl_propagation TO on;",
					 prefix, sql, prefix);

	return buf.data;
}


/*
 * ReqID:TECC4.0
 */
void
TeccDistReplayOnWorkers(const char *sql, const char *what)
{
	Oid			argtypes[1] = {TEXTOID};
	Datum		values[1];
	int			ret;
	int			successColumn;
	int			nodeColumn;
	int			portColumn;
	int			resultColumn;
	char	   *failure = NULL;
	MemoryContext caller_ctx = CurrentMemoryContext;
	int			i;

	if (!TeccDistPluginAvailable())
	{
		return;
	}

	values[0] = CStringGetTextDatum(sql);

	if (SPI_connect() != SPI_OK_CONNECT)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("could not replay %s on the workers", what)));
	}

	ret = SPI_execute_with_args("SELECT * FROM pg_catalog.run_command_on_workers($1)",
								1, argtypes, values, NULL, false, 0);
	if (ret != SPI_OK_SELECT)
	{
		SPI_finish();
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("could not replay %s on the workers", what)));
	}

	successColumn = SPI_fnumber(SPI_tuptable->tupdesc, "success");
	nodeColumn = SPI_fnumber(SPI_tuptable->tupdesc, "nodename");
	portColumn = SPI_fnumber(SPI_tuptable->tupdesc, "nodeport");
	resultColumn = SPI_fnumber(SPI_tuptable->tupdesc, "result");

	for (i = 0; i < SPI_processed; i++)
	{
		HeapTuple	tuple = SPI_tuptable->vals[i];
		TupleDesc	tupdesc = SPI_tuptable->tupdesc;
		bool		ok = true;
		char	   *nodeName = NULL;
		char	   *portText = NULL;
		char	   *message = NULL;
		MemoryContext oldctx;

		if (successColumn > 0)
		{
			bool		isnull = false;
			Datum		value = SPI_getbinval(tuple, tupdesc, successColumn,
											  &isnull);

			ok = !isnull && DatumGetBool(value);
		}

		if (ok)
		{
			continue;
		}

		if (nodeColumn > 0)
		{
			nodeName = SPI_getvalue(tuple, tupdesc, nodeColumn);
		}
		if (portColumn > 0)
		{
			portText = SPI_getvalue(tuple, tupdesc, portColumn);
		}
		if (resultColumn > 0)
		{
			message = SPI_getvalue(tuple, tupdesc, resultColumn);
		}

		oldctx = MemoryContextSwitchTo(caller_ctx);
		failure = psprintf("node %s:%s: %s",
						   nodeName ? nodeName : "?",
						   portText ? portText : "?",
						   message ? message : "unknown error");
		MemoryContextSwitchTo(oldctx);
		break;
	}

	SPI_finish();

	if (failure != NULL)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("could not replay %s on the workers", what),
				 errdetail("%s", failure),
				 errhint("Check that every worker is reachable and has the TECC "
						 "extension, then retry the statement.")));
	}
}