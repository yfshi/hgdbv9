/* Begin - ReqID:SRS-SECURITY-RESOURCEMANAGEMENT */
/*-------------------------------------------------------------------------
 *
 * resgroup_helper.c
 *	  Helper functions for resource group.
 *
 * Copyright (c) 2017-Present VMware, Inc. or its affiliates.
 *
 * Authored by zhangyilin@highgo.com
 * Portions Copyright (c) 2009-2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	    src/backend/utils/resgroup/resgroup_help.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/genam.h"
#include "access/table.h"
#include "funcapi.h"

#include "miscadmin.h"
#include "catalog/pg_resgroup.h"
#include "catalog/pg_resgroupcapability.h"
#include "commands/resgroupcmds.h"
#include "storage/procarray.h"
#include "utils/builtins.h"
#include "utils/datetime.h"
#include "utils/resgroup.h"
#include "utils/cgroup.h"
#include "utils/resource_manager.h"
#include "utils/cgroup_io_limit.h"
#include "commands/tablespace.h"

typedef struct ResGroupStat
{
	Datum groupId;

	StringInfo cpuUsage;
	StringInfo memoryUsage;
} ResGroupStat;

typedef struct ResGroupStatCtx
{
	int nGroups;
	ResGroupStat groups[1];
} ResGroupStatCtx;
static void calcCpuUsage(StringInfoData *str,
						 int64 usageBegin, TimestampTz timestampBegin,
						 int64 usageEnd, TimestampTz timestampEnd);
static void getResUsage(ResGroupStatCtx *ctx, Oid inGroupId);

static void
calcCpuUsage(StringInfoData *str,
			 int64 usageBegin, TimestampTz timestampBegin,
			 int64 usageEnd, TimestampTz timestampEnd)
{
	int64 duration;
	long secs;
	int usecs;
	int64 usage;

	usage = usageEnd - usageBegin;

	TimestampDifference(timestampBegin, timestampEnd, &secs, &usecs);

	duration = secs * 1000000 + usecs;
	
	appendStringInfo(str, "\"%d\":%.2f",
					 4,
					 cgroupOpsRoutine->convertcpuusage(usage, duration));
}

/*
 * Get resource usage.
 *
 *
 * Memory & cpu usage are returned in JSON format.
 */
static void
getResUsage(ResGroupStatCtx *ctx, Oid inGroupId)
{
	int64 *cpuUsages;
	int64 *memoryUsages;
	TimestampTz *timestamps;
	int j;

	cpuUsages = palloc(sizeof(*cpuUsages) * ctx->nGroups);
	memoryUsages = palloc(sizeof(*memoryUsages) * ctx->nGroups);
	timestamps = palloc(sizeof(*timestamps) * ctx->nGroups);

	for (j = 0; j < ctx->nGroups; j++)
	{
		ResGroupStat *row = &ctx->groups[j];
		Oid groupId = DatumGetObjectId(row->groupId);

		cpuUsages[j] = cgroupOpsRoutine->getcpuusage(groupId);
		memoryUsages[j] = cgroupOpsRoutine->getmemoryusage(groupId);
		timestamps[j] = GetCurrentTimestamp();
	}

	pg_usleep(300000);
	for (j = 0; j < ctx->nGroups; j++)
	{
		ResGroupStat *row = &ctx->groups[j];
		Oid groupId = DatumGetObjectId(row->groupId);

		/* cpu */
		appendStringInfo(row->cpuUsage, "{");
		calcCpuUsage(row->cpuUsage, cpuUsages[j], timestamps[j],
					 cgroupOpsRoutine->getcpuusage(groupId),
					 GetCurrentTimestamp());
		appendStringInfoChar(row->cpuUsage, '}');
		
		/* memory */
		appendStringInfo(row->memoryUsage, "{");
		appendStringInfo(row->memoryUsage, "\"%d\":%.2f",
						 4, memoryUsages[j] / 1024.0 / 1024.0);
		
		appendStringInfoChar(row->memoryUsage, '}');
	}
}

static int
compareRow(const void *ptr1, const void *ptr2)
{
	const ResGroupStat *row1 = (const ResGroupStat *) ptr1;
	const ResGroupStat *row2 = (const ResGroupStat *) ptr2;

	return row1->groupId - row2->groupId;
}

/*
 * Get status of resource groups
 */
Datum
pg_resgroup_get_status(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	ResGroupStatCtx *ctx;

	if (SRF_IS_FIRSTCALL())
	{
		MemoryContext oldcontext;
		TupleDesc	tupdesc;
		int			nattr = 8;

		funcctx = SRF_FIRSTCALL_INIT();

		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		tupdesc = CreateTemplateTupleDesc(nattr);
		TupleDescInitEntry(tupdesc, (AttrNumber) 1, "groupid", OIDOID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 2, "num_running", INT4OID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 3, "num_queueing", INT4OID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 4, "num_queued", INT8OID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 5, "num_executed", INT8OID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 6, "total_queue_duration", INTERVALOID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 7, "cpu_usage", JSONOID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 8, "memory_usage", JSONOID, -1, 0);

		funcctx->tuple_desc = BlessTupleDesc(tupdesc);

		if (IsResGroupActivated())
		{
			Relation	pg_resgroup_rel;
			SysScanDesc	sscan;
			HeapTuple	tuple;
			Oid			inGroupId = PG_GETARG_OID(0);

			int ctxsize = sizeof(ResGroupStatCtx) +
				sizeof(ResGroupStat) * (MaxResourceGroups - 1);

			funcctx->user_fctx = palloc(ctxsize);
			ctx = (ResGroupStatCtx *) funcctx->user_fctx;

			/*
			 * others may be creating/dropping resource group concurrently,
			 * block until creating/dropping finish to avoid inconsistent
			 * resource group metadata
			 */
			pg_resgroup_rel = table_open(ResGroupRelationId, ExclusiveLock);

			sscan = systable_beginscan(pg_resgroup_rel, InvalidOid, false,
									   NULL, 0, NULL);
			while (HeapTupleIsValid(tuple = systable_getnext(sscan)))
			{
				Oid			oid = ((Form_pg_resgroup) GETSTRUCT(tuple))->oid;

				if (inGroupId == InvalidOid || inGroupId == oid)
				{
					Assert(funcctx->max_calls < MaxResourceGroups);
					ctx->groups[funcctx->max_calls].cpuUsage = makeStringInfo();
					ctx->groups[funcctx->max_calls].memoryUsage = makeStringInfo();
					ctx->groups[funcctx->max_calls++].groupId = oid;

					if (inGroupId != InvalidOid)
						break;
				}
			}
			systable_endscan(sscan);

			ctx->nGroups = funcctx->max_calls;
			qsort(ctx->groups, ctx->nGroups, sizeof(ctx->groups[0]), compareRow);

			if (ctx->nGroups > 0)
				getResUsage(ctx, inGroupId);

			table_close(pg_resgroup_rel, ExclusiveLock);
		}

		MemoryContextSwitchTo(oldcontext);
	}

	/* stuff done on every call of the function */
	funcctx = SRF_PERCALL_SETUP();
	ctx = (ResGroupStatCtx *) funcctx->user_fctx;

	if (funcctx->call_cntr < funcctx->max_calls)
	{
		/* for each row */
		Datum		values[9];
		bool		nulls[9];
		HeapTuple	tuple;
		Oid			groupId;
		char		statVal[MAXDATELEN + 1];
		ResGroupStat *row = &ctx->groups[funcctx->call_cntr];

		MemSet(values, 0, sizeof(values));
		MemSet(nulls, 0, sizeof(nulls));
		MemSet(statVal, 0, sizeof(statVal));

		values[0] = row->groupId;
		groupId = DatumGetObjectId(values[0]);

		values[1] = ResGroupGetStat(groupId, RES_GROUP_STAT_NRUNNING);
		values[2] = ResGroupGetStat(groupId, RES_GROUP_STAT_NQUEUEING);
		values[3] = ResGroupGetStat(groupId, RES_GROUP_STAT_TOTAL_QUEUED);
		values[4] = ResGroupGetStat(groupId, RES_GROUP_STAT_TOTAL_EXECUTED);
		values[5] = ResGroupGetStat(groupId, RES_GROUP_STAT_TOTAL_QUEUE_TIME);
		values[6] = CStringGetTextDatum(row->cpuUsage->data);
		values[7] = CStringGetTextDatum(row->memoryUsage->data);

		tuple = heap_form_tuple(funcctx->tuple_desc, values, nulls);

		SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
	}
	else
	{
		/* nothing left */
		SRF_RETURN_DONE(funcctx);
	}
}
static List* getIOLimitStats(Relation rel_resgroup_caps);

PG_FUNCTION_INFO_V1(pg_resgroup_get_iostats);
Datum
pg_resgroup_get_iostats(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;

	if (SRF_IS_FIRSTCALL())
	{
		int nattr = 8;
		MemoryContext oldContext;
		TupleDesc tupdesc;

		instr_time start_time;
		instr_time end_time;
		float8 interval;

		List *stats = NIL;
		ListCell *statCell;
		List *newStats = NIL;
		ListCell *newStatCell;
		Relation rel_resgroup_caps;

		funcctx = SRF_FIRSTCALL_INIT();
		if (!IsResGroupActivated())
		{
			SRF_RETURN_DONE(funcctx);
		}

		oldContext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		tupdesc = CreateTemplateTupleDesc(nattr);
		TupleDescInitEntry(tupdesc, (AttrNumber) 1, "rsgname", TEXTOID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 2, "groupid", OIDOID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 3, "tablespace", TEXTOID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 4, "rbps", INT8OID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 5, "wbps", INT8OID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 6, "riops", INT8OID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 7, "wiops", INT8OID, -1, 0);

		funcctx->tuple_desc = BlessTupleDesc(tupdesc);

		/* collect stats */
		rel_resgroup_caps = table_open(ResGroupCapabilityRelationId, AccessShareLock);

		/* pg_usleep can be interrupted, so we use timestamp as the interval */
		INSTR_TIME_SET_CURRENT(start_time);
		stats = getIOLimitStats(rel_resgroup_caps);

		/* 1 second */
		pg_usleep(1000000L);

		INSTR_TIME_SET_CURRENT(end_time);
		newStats = getIOLimitStats(rel_resgroup_caps);

		interval = (INSTR_TIME_GET_MILLISEC(end_time) - INSTR_TIME_GET_MILLISEC(start_time)) / 1000.0;

		table_close(rel_resgroup_caps, AccessShareLock);

		if (list_length(stats) != list_length(newStats))
			ereport(ERROR, (errmsg("stats count differs between runs")));

		funcctx->max_calls = list_length(stats);
		funcctx->user_fctx = (void *) list_head(stats);

		/* oldStat and newStats maybe have different orders, so it need sort */
		list_sort(stats, compare_iostat);
		list_sort(newStats, compare_iostat);

		forboth(statCell, stats, newStatCell, newStats)
		{
			IOStat *newStat = (IOStat *) lfirst(newStatCell);
			IOStat *stat = (IOStat *) lfirst(statCell);

			if (stat->groupid != newStat->groupid || stat->tablespace != newStat->tablespace)
				ereport(ERROR, (errmsg("get different result from io.stat after little interval")));

			stat->items.rios = (newStat->items.rios - stat->items.rios) / interval;
			stat->items.wios = (newStat->items.wios - stat->items.wios) / interval;

			stat->items.rbytes = (newStat->items.rbytes - stat->items.rbytes) / interval;
			stat->items.wbytes = (newStat->items.wbytes - stat->items.wbytes) / interval;
		}

		MemoryContextSwitchTo(oldContext);
	}

	funcctx = SRF_PERCALL_SETUP();

	if (funcctx->call_cntr < funcctx->max_calls)
	{
		Datum values[8];
		bool nulls[8];
		HeapTuple tuple;
		ListCell *cell = (ListCell *) funcctx->user_fctx;
		/* funcctx->user_fctx = (void *) cell->next; */
		IOStat *stat = (IOStat *) lfirst(cell);

		MemSet(values, 0, sizeof(values));
		MemSet(nulls, 0, sizeof(nulls));

		values[0] = CStringGetTextDatum(GetResGroupNameForId(stat->groupid));
		values[1] = ObjectIdGetDatum(stat->groupid);

		if (stat->tablespace != InvalidOid)
			values[2] = CStringGetTextDatum(get_tablespace_name(stat->tablespace));
		else
			values[2] = CStringGetTextDatum("*");

		values[3] = Int64GetDatum((int64) stat->items.rbytes);
		values[4] = Int64GetDatum((int64) stat->items.wbytes);
		values[5] = Int64GetDatum((int64) stat->items.rios);
		values[6] = Int64GetDatum((int64) stat->items.wios);

		tuple = heap_form_tuple(funcctx->tuple_desc, values, nulls);

		SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
	}
	else
	{
		SRF_RETURN_DONE(funcctx);
	}
}

static List*
getIOLimitStats(Relation rel_resgroup_caps)
{
	SysScanDesc	sscan;
	HeapTuple	tuple;
	List		*io_limit_items = NIL;
	ListCell	*cell;
	List		*result = NIL;
	/* save groupid and io limits together */
	typedef struct IOLimitItem {
		Oid groupid;
		List *io_limit;
	} IOLimitItem;

	/* get io limit string from catalog */
	sscan = systable_beginscan(rel_resgroup_caps, InvalidOid, false,
							   NULL, 0, NULL);
	while (HeapTupleIsValid(tuple = systable_getnext(sscan)))
	{
		bool isNULL;
		Datum id_datum;
		Datum type_datum;
		Datum value_datum;
		Oid id;
		ResGroupLimitType type;
		char *io_limit_str;
		IOLimitItem *item;

		type_datum = heap_getattr(tuple, Anum_pg_resgroupcapability_reslimittype,
								  rel_resgroup_caps->rd_att, &isNULL);
		type = (ResGroupLimitType) DatumGetInt16(type_datum);
		if (type != RESGROUP_LIMIT_TYPE_IO_LIMIT)
			continue;

		id_datum = heap_getattr(tuple, Anum_pg_resgroupcapability_resgroupid,
								rel_resgroup_caps->rd_att, &isNULL);
		id = DatumGetObjectId(id_datum);

		value_datum = heap_getattr(tuple, Anum_pg_resgroupcapability_value,
								   rel_resgroup_caps->rd_att, &isNULL);
		io_limit_str = TextDatumGetCString(value_datum);

		if (strcmp(io_limit_str, DefaultIOLimit) == 0)
			continue;

		item = (IOLimitItem *) palloc0(sizeof(IOLimitItem));
		item->groupid = id;

		item->io_limit = io_limit_parse(io_limit_str);

		io_limit_items = lappend(io_limit_items, item);
	}
	systable_endscan(sscan);

	foreach(cell, io_limit_items)
	{
		IOLimitItem *item = (IOLimitItem *) lfirst(cell);

		List *tmp = cgroupOpsRoutine->getiostat(item->groupid, item->io_limit);

		result = list_concat(result, tmp);
	}

	list_free_deep(io_limit_items);

	return result;
}

/*
 * move a query to a resource group
 */
/*
Datum
pg_resgroup_move_query(PG_FUNCTION_ARGS)
{
	Oid groupId;
	const char *groupName;
	Oid currentGroupId;
	pid_t pid = PG_GETARG_INT32(0);
	groupName = text_to_cstring(PG_GETARG_TEXT_PP(1));

	if (!IsResGroupEnabled())
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
						(errmsg("resource group is not enabled"))));

	if (!superuser())
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
						(errmsg("must be superuser to move query"))));

	
	if (pid == MyProcPid)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						(errmsg("cannot move myself"))));
	
	groupId = get_resgroup_oid(groupName, false);
	
	if (groupId == SYSTEMRESGROUP_OID)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						(errmsg("cannot move a process to the system_group"))));
	
	if (pid == -1)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
						(errmsg("cannot find process: %d", pid))));
	
	currentGroupId = ResGroupGetGroupIdBySessionPid(pid);
	if (currentGroupId == InvalidOid)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
						(errmsg("process %d is in IDLE state", pid))));
	if (currentGroupId == groupId)
		PG_RETURN_BOOL(true);
	
	ResGroupMoveQuery(pid, groupId, groupName);

	PG_RETURN_BOOL(true);
}
*/

Datum
my_cpu_prime(PG_FUNCTION_ARGS)
{
	int32		iStart = PG_GETARG_INT32(0);
	int32		iEnd = PG_GETARG_INT32(1);
	int 		i = 0;
	int			j = 0;
	int			k = 0;
	int			iCount = 0;
	
	if(iStart > 0 && iEnd > 1 && iStart < iEnd)
	{
		for(i = 2; i <= iEnd ; i++)
		{
			iCount = 0;
			for(j = 2; j <= iEnd ; j++)
			{
				k = i/j;
				
				if(k*j == i)
				{
					iCount++;
				}
				
			}
			if(iCount == 2)
			{
				/* i is prime */
				
			}

		}
	}
	else 
	{
		PG_RETURN_BOOL(false);
	}
	
	PG_RETURN_BOOL(true);
}
/* End - ReqID:SRS-SECURITY-RESOURCEMANAGEMENT */

