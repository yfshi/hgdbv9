/*-------------------------------------------------------------------------
 *
 * File: hg_flashback_db.c
 *
 * Abstract: extension for database level flashback.
 *
 * Copyright:
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *	  		contrib/hg_flashback_db/hg_flashback_db.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "postmaster/bgworker.h"
#include "utils/guc.h"
#include "hg_flashback_db.h"

#include <limits.h>

PG_MODULE_MAGIC;

/* GUC variables - configurable via postgresql.conf */
int   hg_flashback_db_logging_interval = 60;
char *hg_flashback_db_stanza = NULL;
char *hg_flashback_db_pgbackrest_path = "pgbackrest";
char *hg_flashback_db_config_path = NULL;
int   hg_flashback_db_dry_run = 0;
bool  hg_flashback_db_enabled = true;
char *hg_flashback_db_bgw_database = "postgres";
int   hg_flashback_db_retention_target = 10080;
bool  hg_flashback_db_auto_cleanup = true;

void		_PG_init(void);

/*
 * _PG_init - Extension initialization
 * Called when extension is loaded via shared_preload_libraries
 */
void
_PG_init(void)
{
    /* Interval between automatic restore points (minutes) */
    DefineCustomIntVariable("hg_flashback_db.db_flashback_logging_interval",
                            "Interval between automatic restore points.",
                            NULL,
                            &hg_flashback_db_logging_interval,
                            60,
                            1,
                            INT_MAX / 60000,
                            PGC_SIGHUP,
                            GUC_UNIT_MIN,
                            NULL, NULL, NULL);

    /* Retention target (minutes). Controls when to create full vs diff, and when to expire */
    DefineCustomIntVariable("hg_flashback_db.db_flashback_retention_target",
                            "Retention target for restore points in minutes.",
                            "Full backups are created when last full is older than this. "
                            "Diff restore points are expired after this time, "
                            "full restore points after 2x this time.",
                            &hg_flashback_db_retention_target,
                            10080,
                            60,
                            INT_MAX / 60000,
                            PGC_SIGHUP,
                            GUC_UNIT_MIN,
                            NULL, NULL, NULL);

    /* pgBackRest stanza name */
    DefineCustomStringVariable("hg_flashback_db.pgbackrest_stanza",
                               "pgBackRest stanza name.",
                               NULL,
                               &hg_flashback_db_stanza,
                               NULL,
                               PGC_SIGHUP,
                               0,
                               NULL, NULL, NULL);

    /* Path to pgBackRest executable */
    DefineCustomStringVariable("hg_flashback_db.pgbackrest_path",
                               "Path to pgBackRest executable.",
                               NULL,
                               &hg_flashback_db_pgbackrest_path,
                               "pgbackrest",
                               PGC_SIGHUP,
                               0,
                               NULL, NULL, NULL);

    /* Path to pgBackRest configuration file */
    DefineCustomStringVariable("hg_flashback_db.pgbackrest_config_path",
                               "Path to pgBackRest configuration file.",
                               NULL,
                               &hg_flashback_db_config_path,
                               NULL,
                               PGC_SIGHUP,
                               0,
                               NULL, NULL, NULL);

    /* Dry run mode:
     * 0 = normal mode (execute real backups)
     * 1 = dry-run mode (simulate, log commands but don't execute)
     * 2 = test mode (execute commands but don't parse JSON output)
     */
    DefineCustomIntVariable("hg_flashback_db.pgbackrest_dry_run",
                           "pgBackRest dry-run mode: 0=off, 1=dry-run, 2=test-bypass.",
                           NULL,
                           &hg_flashback_db_dry_run,
                           0,
                           0,
                           2,
                           PGC_SIGHUP,
                           0,
                           NULL, NULL, NULL);

    /* Auto cleanup orphan restore points after drop */
    DefineCustomBoolVariable("hg_flashback_db.auto_cleanup",
                             "Enable auto cleanup orphan restore points after drop.",
                             NULL,
                             &hg_flashback_db_auto_cleanup,
                             true,
                             PGC_SIGHUP,
                             0,
                             NULL, NULL, NULL);

    /* Can only set database name during preload, not via ALTER SYSTEM */
    if (!process_shared_preload_libraries_in_progress)
        return;

    /* Database for background worker to connect to */
    DefineCustomStringVariable("hg_flashback_db.db_flashback_bgw_database",
                               "Database for the background worker to connect to.",
                               NULL,
                               &hg_flashback_db_bgw_database,
                               "highgo",
                               PGC_POSTMASTER,
                               0,
                               NULL, NULL, NULL);

    /* Enable/disable background worker */
    DefineCustomBoolVariable("hg_flashback_db.db_flashback_enabled",
                             "Enable automatic flashback background worker.",
                             NULL,
                             &hg_flashback_db_enabled,
                             true,
                             PGC_POSTMASTER,
                             0,
                             NULL, NULL, NULL);

    EmitWarningsOnPlaceholders("hg_flashback_db");

    /* Register background worker if enabled */
    if (hg_flashback_db_enabled)
    {
        BackgroundWorker worker;

        MemSet(&worker, 0, sizeof(BackgroundWorker));
        worker.bgw_flags = BGWORKER_SHMEM_ACCESS |
                           BGWORKER_BACKEND_DATABASE_CONNECTION;
        worker.bgw_start_time = BgWorkerStart_RecoveryFinished;
        worker.bgw_restart_time = 60;
        snprintf(worker.bgw_library_name, BGW_MAXLEN, "hg_flashback_db");
        snprintf(worker.bgw_function_name, BGW_MAXLEN, "hg_flashback_db_bgw_main");
        snprintf(worker.bgw_name, BGW_MAXLEN, "hg_flashback_db auto restore point worker");
        snprintf(worker.bgw_type, BGW_MAXLEN, "hg_flashback_db worker");
        worker.bgw_main_arg = Int32GetDatum(0);
        worker.bgw_notify_pid = 0;

        RegisterBackgroundWorker(&worker);
    }
}
