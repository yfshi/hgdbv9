/*------------------------------------------------------------------
 *
 * File: internal.c
 *
 * Abstract:
 * 	  Porfermance GUC parameters.
 *
 * Copyright:
 * Copyright (c) 2024-2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *		contrib/hg_misc/src/internal_guc/internal.c
 *
 * Requirement description:
 *		ReqID:SRS-Performance-GUC
 *------------------------------------------------------------------
 */
#include "postgres.h"

#include <limits.h>

#include "internal.h"

#include "fmgr.h"
#include "miscadmin.h"
#include "storage/ipc.h"
#include "storage/lwlock.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "access/xact.h"
#include "catalog/namespace.h"
#include "utils/snapmgr.h"

/* A global counter using for HeapPagePruneWithThreshold() */
int heap_page_prune_skip_num = 0;

/* GUC parameters */
/* Begin - OA#1640 */
int heap_page_repair_interval = 4;
Oid special_index_oid = 0;
bool get_snapshot_fast_release_lwlock = true;
int clog_multiple_number = 2;

bool enable_autoset = true;
/* End - OA#1640 */
/* Skip log print when we change GUC parameters and send SIGHUP to postmaster */
bool *skip_log = NULL;

void FastReleaseProcArrayLockAcquire(LWLockMode mode);
void FastReleaseProcArrayLockRelease(void);
void HeapPagePruneWithThreshold(Relation relation, Buffer buffer,
                                       struct GlobalVisState *vistest, 
                                       TransactionId limited_xmin,
                                       TimestampTz limited_ts);
bool SkipSpecialIndexOid(IndexScanDesc scan);
Size CLOGShmemBuffersHook(void);
bool PDBGIfPrintLog(void);
void PDBGAutoset(AutosetMode mode); /* OA#1640 */
void PDBGShmeminit(void);

/* 
 * get_snapshot_fast_release_lwlock & snapshot_lock_acquire_hook
 */
void
FastReleaseProcArrayLockAcquire(LWLockMode mode)
{
    if (!get_snapshot_fast_release_lwlock)
        LWLockAcquire(ProcArrayLock, mode);
}

/* 
 * get_snapshot_fast_release_lwlock & snapshot_lock_release_hook
 */
void
FastReleaseProcArrayLockRelease(void)
{
    if (!get_snapshot_fast_release_lwlock)
        LWLockRelease(ProcArrayLock);
}

/*
 * heap_page_repair_interval & haep_page_prune_hook
 */
void
HeapPagePruneWithThreshold(Relation relation, Buffer buffer,
                           struct GlobalVisState *vistest, 
                           TransactionId limited_xmin,
                           TimestampTz limited_ts)
{
    if (heap_page_repair_interval > 1)
    {
        /*
         * If we have interval and not meet it, skip prune and add counter,
         * else do prune and reset counter.
         */
        heap_page_prune_skip_num++;
        if (heap_page_prune_skip_num >= heap_page_repair_interval)
        {
            (void) heap_page_prune(relation, buffer, InvalidTransactionId,
                                   vistest,
                                   limited_xmin, limited_ts, true, NULL);
            heap_page_prune_skip_num = 0;
        }
    }
    else    /* original process */
        (void) heap_page_prune(relation, buffer, InvalidTransactionId,
                               vistest,
                               limited_xmin, limited_ts, true, NULL);
}

/* 
 * special_index_oid & skip_special_indexoid_hook
 */
bool
SkipSpecialIndexOid(IndexScanDesc scan)
{
    if (scan->indexRelation->rd_id == special_index_oid)
        return false;
    else
        return true;
}

/* 
 * clog_multiple_number & clog_shmem_buffers_hook
 */
Size
CLOGShmemBuffersHook(void)
{
    return (Min(128, Max(4, NBuffers / 512))) * clog_multiple_number;
}

/*
 * Set performance GUC parameters via execute VACUUM.
 *      Usage: VACUUM [...] index_name;
 * This will set 'special_index_oid' with index_name's Oid. And if we vacuume
 * the same index_name, GUC parameters will be reset to default.
 * 
 * Use by hook: pdbg_vacuum_hook
 * 
 * TODOz: We temporarily remove process of get_snapshot_fast_release_lwlock,
 *        since it will fall into C assert. Maybe add it back at another day.
 */
/* Begin - OA#1640 */
void
PDBGAutoset(AutosetMode mode)
{
	char	   *indexOidStr;
	Oid			indexOid = InvalidOid;

	/* If we enabled auto set? */
	if (!enable_autoset)
		return;
	/*
	 * When we connect to server via psql, we only set session-level
	 * hg_misc.special_index_oid to invalid oid.
	 */
	if (mode == PDBG_AUTOSET_PSQL)
	{
		SetConfigOption("hg_misc.special_index_oid", "0",
						 PGC_USERSET, PGC_S_OVERRIDE);
		return;
	}

	/* We don't want log during setting performance GUCs */
	if (skip_log == NULL)
		elog(ERROR, "You need to add \'hg_misc\' to preload shared library!");
	*skip_log = true;
	/* Now, get bmsql_new_order_pkey's oid */
	StartTransactionCommand();
	PushActiveSnapshot(GetTransactionSnapshot());

	indexOid = RelnameGetRelid("bmsql_new_order_pkey");
	PopActiveSnapshot();
	CommitTransactionCommand();
	indexOidStr = palloc0(15);
	pg_ltoa(indexOid, indexOidStr);

	if (mode == PDBG_AUTOSET_NORMAL)
	{
		SetConfigFile("hg_misc.clog_multiple_number", "8");
		SetConfigFile("hg_misc.heap_page_repair_interval", "8");
		SetConfigFile("hg_misc.special_index_oid", indexOidStr);
	}
	else if (mode == PDBG_AUTOSET_INIT)
	{
		SetConfigFile("hg_misc.special_index_oid", indexOidStr);
	}
	else if (mode == PDBG_AUTOSET_RESET_ALL)
	{
		SetConfigFile("hg_misc.clog_multiple_number", "1");
		SetConfigFile("hg_misc.heap_page_repair_interval", "1");
		SetConfigFile("hg_misc.special_index_oid", "0");
	}

	/* Tell postmaster to reload GUCs */
	if (kill(PostmasterPid, SIGHUP))
	{
		ereport(WARNING, (errmsg("failed to send signal to postmaster: %m")));
	}

	/* A snap wait for postmaster having done it */
	pg_usleep(2000);
	/* Now print log normally. */
	*skip_log = false;

	pfree(indexOidStr);
	/* End - OA#1640 */
}

/*
 * Judge if we need skip log print.
 * Use by hook: pdbg_log_hook
 */
bool
PDBGIfPrintLog(void)
{
    if (skip_log == NULL)
        elog(ERROR, "You need to add \'hg_misc\' to preload shared library!");
    return !(*skip_log);
}

/*
 * Set clog_multiple_number manually.
 */
PG_FUNCTION_INFO_V1(internal_set_clog);
Datum
internal_set_clog(PG_FUNCTION_ARGS)
{
	char	   *a = palloc0(15);

    if (skip_log == NULL)
        elog(ERROR, "You need to add \'hg_misc\' to preload shared library!");
    *skip_log = true;

	pg_ltoa(PG_GETARG_INT32(0), a);

	SetConfigFile("hg_misc.clog_multiple_number", a);

	if (kill(PostmasterPid, SIGHUP))
	{
		ereport(WARNING, (errmsg("failed to send signal to postmaster: %m")));
	}

    pg_usleep(2000);
    *skip_log = false;

	pfree(a);

	PG_RETURN_TEXT_P(cstring_to_text("set"));
}

/*
 * Set heap_page_repair_interval manually.
 */
PG_FUNCTION_INFO_V1(internal_set_prune_interval);
Datum
internal_set_prune_interval(PG_FUNCTION_ARGS)
{
	char	   *a = palloc0(15);

    if (skip_log == NULL)
        elog(ERROR, "You need to add \'hg_misc\' to preload shared library!");
    *skip_log = true;

	pg_ltoa(PG_GETARG_INT32(0), a);

	SetConfigFile("hg_misc.heap_page_repair_interval", a);

	if (kill(PostmasterPid, SIGHUP))
	{
		ereport(WARNING, (errmsg("failed to send signal to postmaster: %m")));
	}

    pg_usleep(2000);
    *skip_log = false;

	pfree(a);

	PG_RETURN_TEXT_P(cstring_to_text("set"));
}

/*
 * Set four performance GUC parameters manually. Arguments in order of:
 *      get_snapshot_fast_release_lwlock
 *      clog_multiple_number
 *      heap_page_repair_interval
 *      special_index_oid
 * 
 * TODOz: We temporarily remove process of get_snapshot_fast_release_lwlock,
 *        since it will fall into C assert. Maybe add it back at another day.
 */
PG_FUNCTION_INFO_V1(internal_set_perf);
Datum
internal_set_perf(PG_FUNCTION_ARGS)
{
	char	   *a = palloc0(15);
	char	   *b = palloc0(15);
	char	   *c = palloc0(15);

    if (skip_log == NULL)
        elog(ERROR, "You need to add \'hg_misc\' to preload shared library!");
    *skip_log = true;

	pg_ltoa(PG_GETARG_INT32(1), a);
	pg_ltoa(PG_GETARG_INT32(2), b);
	pg_ltoa(PG_GETARG_INT32(3), c);

	SetConfigFile("hg_misc.clog_multiple_number", a);
	SetConfigFile("hg_misc.heap_page_repair_interval", b);
	SetConfigFile("hg_misc.special_index_oid", c);

	if (kill(PostmasterPid, SIGHUP))
	{
		ereport(WARNING, (errmsg("failed to send signal to postmaster: %m")));
	}

    pg_usleep(2000);
    *skip_log = false;

	pfree(a);
	pfree(b);
	pfree(c);

	PG_RETURN_TEXT_P(cstring_to_text("set"));
}