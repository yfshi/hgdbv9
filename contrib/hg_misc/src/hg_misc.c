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
/* Begin - OA#1640 */
#include "miscadmin.h"
#include "pgstat.h"
#include "postmaster/bgworker.h"
#include "storage/latch.h"
#include "utils/guc.h"
#include "storage/shmem.h"
#include "utils/builtins.h"
#include "storage/ipc.h"
/* End - OA#1640 */

#ifdef USE_LICENSE
#include "license.h"
#endif

char *hg_misc_database = NULL; /* OA#1640 */

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
void PDBGWorkerMain(Datum main_arg); /* OA#1640 */

/* Begin - OA#1640 */
/* flags set by signal handlers */
static volatile sig_atomic_t got_sighup = false;
static volatile sig_atomic_t got_sigterm = false;

/*
 * Signal handler for SIGTERM
 *		Set a flag to let the main loop to terminate, and set our latch to wake
 *		it up.
 */
static void
MiscWorkerSigterm(SIGNAL_ARGS)
{
	int			save_errno = errno;

	got_sigterm = true;
	SetLatch(MyLatch);

	errno = save_errno;
}

/*
 * Signal handler for SIGHUP
 *		Set a flag to tell the main loop to reread the config file, and set
 *		our latch to wake it up.
 */
static void
MiscWorkerSighup(SIGNAL_ARGS)
{
	int			save_errno = errno;

	got_sighup = true;
	SetLatch(MyLatch);

	errno = save_errno;
}
/* End - OA#1640 */

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

/* Begin - OA#1640 */
/* when the standby machine is restored, set index_special_oid. */
void
PDBGWorkerMain(Datum main_arg)
{
	/* Establish signal handlers before unblocking signals. */
	pqsignal(SIGHUP, MiscWorkerSighup);
	pqsignal(SIGTERM, MiscWorkerSigterm);

	/* We're now ready to receive signals */
	BackgroundWorkerUnblockSignals();

	/* Connect to our database */
	BackgroundWorkerInitializeConnection(hg_misc_database, NULL, 0);

	/*
	 * Main loop: do this until the SIGTERM handler tells us to terminate
	 */
	while (!got_sigterm)
	{
		/*
		 * Background workers mustn't call usleep() or any direct equivalent:
		 * instead, they may wait on their process latch, which sleeps as
		 * necessary, but is awakened if postmaster dies.  That way the
		 * background process goes away immediately in an emergency.
		 */
		(void) WaitLatch(MyLatch,
						 WL_LATCH_SET | WL_TIMEOUT | WL_EXIT_ON_PM_DEATH,
						 1000L,
						 PG_WAIT_EXTENSION);
		ResetLatch(MyLatch);

		CHECK_FOR_INTERRUPTS();

		/*
		 * In case of a SIGHUP, just reload the configuration.
		 */
		if (got_sighup)
		{
			got_sighup = false;
			ProcessConfigFile(PGC_SIGHUP);
		}

#ifdef MISC_INTERNAL /* Begin - ReqID:Performance-GUC */
		/*
		 * Initialize GUC hg_misc.special_index_oid since we need to find out
		 * whether index bmsql_new_order_pkey exists.
		 */
		PDBGAutoset(PDBG_AUTOSET_INIT);
#endif /* End - ReqID:Performance-GUC */

		/* Currently we don't need loop */
		SetLatch(MyLatch);
		proc_exit(0);
	}

	proc_exit(1);
}
/* End OA#1640 */

/*
 * Module load callback
 */
void
_PG_init(void)
{
	BackgroundWorker worker;
/* Begin - ReqID:SRS-Performance-GUC */
#ifdef MISC_INTERNAL
    /* Shared memory */
    RequestAddinShmemSpace(MAXALIGN(sizeof(bool)));

    /* Define custom GUC variables */
    DefineCustomIntVariable("hg_misc.heap_page_repair_interval",
                            "Set the EVERY number of pages which should be prune.",
                            NULL,
                            &heap_page_repair_interval,
                            4, /* OA#1640 */
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
                            PGC_USERSET, /* OA#1640 */
                            GUC_NOT_IN_SAMPLE,
                            NULL,
                            NULL,
                            NULL);

    DefineCustomBoolVariable("hg_misc.get_snapshot_fast_release_lwlock",
							 "Wheather to fast release ProcArrayLock while getting snapshot.",
							 NULL,
							 &get_snapshot_fast_release_lwlock,
							 true,
							 PGC_SIGHUP,
							 0,
							 NULL,
							 NULL,
							 NULL);

    DefineCustomIntVariable("hg_misc.clog_multiple_number",
                            "Set clog multiple number.",
                            NULL,
                            &clog_multiple_number,
                            2, /* OA#1640 */
                            1, INT_MAX,
                            PGC_SIGHUP,
                            GUC_NOT_IN_SAMPLE,
                            NULL,
                            NULL,
                            NULL);
	/* Begin - OA#1640 */
	DefineCustomBoolVariable("hg_misc.enable_autoset",
							 "Wheather to enable auto-set at psql and vacuum to set GUCs.",
							 NULL,
							 &enable_autoset,
							 true,
							 PGC_USERSET,
							 0,
							 NULL,
							 NULL,
							 NULL);
	DefineCustomStringVariable("hg_misc.database",
							"Database to connect to.",
							NULL,
							&hg_misc_database,
							"highgo",
							PGC_USERSET,
							0,
							NULL,
							NULL,
							NULL);
	/* End - OA#1640 */

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
    pdbg_autoset_hook = PDBGAutoset;
#endif /* MISC_INTERNAL */
/* End - ReqID:SRS-Performance-GUC */
	/* set up information for hg_misc workers */
	/* Begin OA#1640 */
	memset(&worker, 0, sizeof(worker));
	worker.bgw_flags = BGWORKER_SHMEM_ACCESS |
			BGWORKER_BACKEND_DATABASE_CONNECTION;
	worker.bgw_start_time = BgWorkerStart_RecoveryFinished;
	worker.bgw_restart_time = BGW_NEVER_RESTART;
	sprintf(worker.bgw_library_name, "hg_misc");
	sprintf(worker.bgw_function_name, "PDBGWorkerMain");
	worker.bgw_notify_pid = 0;
	snprintf(worker.bgw_name, BGW_MAXLEN, "hg_misc worker");
	snprintf(worker.bgw_type, BGW_MAXLEN, "hg_misc");

	RegisterBackgroundWorker(&worker);
	/* End OA#1640 */
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
    pdbg_autoset_hook = NULL; /* OA#1640 */
#endif /* MISC_INTERNAL */
/* End - ReqID:SRS-Performance-GUC */
}
