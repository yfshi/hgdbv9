/*------------------------------------------------------------------
 *
 * File: hg_misc.c
 *
 * Abstract:
 *      Main file of contrib/hg_misc. Currently has module:
 *          Lincense
 *          internal_guc(performance GUC)
 *
 * Copyright:
 * Copyright (c) 2022-2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *		contrib/hg_misc/src/hg_misc.c
 *
 * Requirement description:
 *		ReqID:SRS-MISC-LICENSE
 *      ReqID:SRS-Performance-GUC
 *
 *------------------------------------------------------------------
 */

#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"

#ifdef USE_LICENSE
#include "license.h"
#endif

/* Begin - ReqID:SRS-Performance-GUC */
#ifdef MISC_INTERNAL
#include "internal_guc/internal.h"
static snapshot_lock_acquire_hook_type prev_snapshot_lock_acquire_hook = NULL;
static snapshot_lock_release_hook_type prev_snapshot_lock_release_hook = NULL;
static haep_page_prune_hook_type prev_haep_page_prune_hook = NULL;
static skip_special_indexoid_hook_type prev_skip_special_indexoid_hook = NULL;
static clog_shmem_buffers_hook_type prev_clog_shmem_buffers_hook = NULL;

static shmem_startup_hook_type prev_shmem_startup_hook = NULL;
#endif /* MISC_INTERNAL */
/* End - ReqID:SRS-Performance-GUC */

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif


void		_PG_init(void);
void		_PG_fini(void);


#ifdef MISC_INTERNAL
/*
 * Allocate and initialize shared memory member.
 * Use by hook: shmem_startup_hook.
 */
static void
PDBGShmeminit(void)
{
    if (prev_shmem_startup_hook)
        prev_shmem_startup_hook();

    skip_log = (bool *) ShmemAlloc(MAXALIGN(sizeof(bool)));
    
    if (skip_log != NULL)
        *skip_log = false;
}
#endif /* MISC_INTERNAL */

/*
 * Module load callback
 */
void
_PG_init(void)
{
/* Begin - ReqID:SRS-Performance-GUC */
#ifdef MISC_INTERNAL
    /* Shared memory */
    RequestAddinShmemSpace(MAXALIGN(sizeof(bool)));

    /* Define custom GUC variables */
    DefineCustomIntVariable("hg_misc.heap_page_repair_interval",
                            "Set the EVERY number of pages which should be prune.",
                            NULL,
                            &heap_page_repair_interval,
                            1,
                            1, 1000,
                            PGC_SIGHUP,
                            GUC_NOT_IN_SAMPLE,
                            NULL,
                            NULL,
                            NULL);

    DefineCustomIntVariable("hg_misc.special_index_oid",
                            "Set which index we can skip move right search.",
                            NULL,
                            (int *) &special_index_oid,
                            0,
                            0, INT_MAX,
                            PGC_SIGHUP,
                            GUC_NOT_IN_SAMPLE,
                            NULL,
                            NULL,
                            NULL);

    DefineCustomBoolVariable("hg_misc.get_snapshot_fast_release_lwlock",
							 "Wheather to fast release ProcArrayLock while getting snapshot.",
							 NULL,
							 &get_snapshot_fast_release_lwlock,
							 false,
							 PGC_SIGHUP,
							 0,
							 NULL,
							 NULL,
							 NULL);

    DefineCustomIntVariable("hg_misc.clog_multiple_number",
                            "Set clog multiple number.",
                            NULL,
                            &clog_multiple_number,
                            1,
                            1, INT_MAX,
                            PGC_SIGHUP,
                            GUC_NOT_IN_SAMPLE,
                            NULL,
                            NULL,
                            NULL);

    /* Load hooks */
    prev_snapshot_lock_acquire_hook = snapshot_lock_acquire_hook;
    snapshot_lock_acquire_hook = FastReleaseProcArrayLockAcquire;
    prev_snapshot_lock_release_hook = snapshot_lock_release_hook;
    snapshot_lock_release_hook = FastReleaseProcArrayLockRelease;

    prev_haep_page_prune_hook = haep_page_prune_hook;
    haep_page_prune_hook = HeapPagePruneWithThreshold;

    prev_skip_special_indexoid_hook = skip_special_indexoid_hook;
    skip_special_indexoid_hook = SkipSpecialIndexOid;

    prev_clog_shmem_buffers_hook = clog_shmem_buffers_hook;
    clog_shmem_buffers_hook = CLOGShmemBuffersHook;

    prev_shmem_startup_hook = shmem_startup_hook;
    shmem_startup_hook = PDBGShmeminit;

    pdbg_log_hook = PDBGIfPrintLog;
    pdbg_vacuum_hook = PDBGVacuum;
#endif /* MISC_INTERNAL */
/* End - ReqID:SRS-Performance-GUC */
}

void
_PG_fini(void)
{
/* Begin - ReqID:SRS-Performance-GUC */
#ifdef MISC_INTERNAL
    /* Unload hooks */
    snapshot_lock_acquire_hook = prev_snapshot_lock_acquire_hook;
    snapshot_lock_release_hook = prev_snapshot_lock_release_hook;
    haep_page_prune_hook = prev_haep_page_prune_hook;
    skip_special_indexoid_hook = prev_skip_special_indexoid_hook;
    clog_shmem_buffers_hook = prev_clog_shmem_buffers_hook;
    shmem_startup_hook = prev_shmem_startup_hook;
    pdbg_log_hook = NULL;
    pdbg_vacuum_hook = NULL;
#endif /* MISC_INTERNAL */
/* End - ReqID:SRS-Performance-GUC */
}