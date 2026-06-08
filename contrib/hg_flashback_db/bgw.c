/*-------------------------------------------------------------------------
 *
 * File: bgw.c
 *
 * Abstract: support backgroud worker for management restore point.
 *
 * Copyright:
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *	  		contrib/hg_flashback_db/bgw.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include "access/xact.h"
#include "access/xlog.h"
#include "access/xlog_internal.h"
#include "executor/spi.h"
#include "miscadmin.h"
#include "pgstat.h"
#include "postmaster/bgworker.h"
#include "postmaster/interrupt.h"
#include "storage/ipc.h"
#include "storage/latch.h"
#include "storage/lock.h"
#include "tcop/tcopprot.h"
#include "utils/memutils.h"
#include "utils/snapmgr.h"
#include "utils/timestamp.h"
#include "utils/builtins.h"
#include "catalog/pg_type.h"
#include "hg_flashback_db.h"



/*
 * generate_auto_name
 *	Generate automatic restore point name based on current timestamp.
 *
 * Format: auto_YYYYMMDD_HHMMSS_MMM
 */
static void
generate_auto_name(char *buf, size_t buflen)
{
    TimestampTz now = GetCurrentTimestamp();
    struct pg_tm tt;
    fsec_t      fsec;
    int         tz;
    int         millis;

    timestamp2tm(now, &tz, &tt, &fsec, NULL, NULL);
    millis = (int) (fsec / 1000);

    /* Include milliseconds to avoid duplicate names in fast bgw cycles. */
    snprintf(buf, buflen, "auto_%04d%02d%02d_%02d%02d%02d_%03d",
             tt.tm_year, tt.tm_mon, tt.tm_mday,
             tt.tm_hour, tt.tm_min, tt.tm_sec, millis);
}

/*
 * restore_points_table_exists
 *	Check whether extension metadata table is available yet.
 */
static bool
restore_points_table_exists(void)
{
    int     ret;
    bool    isnull;
    bool    exists = false;
    Datum   val;

    SetCurrentStatementStartTimestamp();
    StartTransactionCommand();
    SPI_connect();
    PushActiveSnapshot(GetTransactionSnapshot());

    ret = SPI_execute(
        "SELECT to_regclass('hg_flashback_db.restore_points_base') IS NOT NULL",
        true, 1);

    if (ret == SPI_OK_SELECT && SPI_processed > 0)
    {
        val = SPI_getbinval(SPI_tuptable->vals[0],
                            SPI_tuptable->tupdesc, 1, &isnull);
        if (!isnull)
            exists = DatumGetBool(val);
    }

    PopActiveSnapshot();
    SPI_finish();
    CommitTransactionCommand();

    return exists;
}

/*
 * determine_backup_type
 *	Determine which type of pgBackRest backup to perform.
 *
 * Checks the timestamp of the last full backup. If it's older than
 * hg_flashback_db_retention_target, returns "full" to force a new
 * full backup; otherwise returns "diff" to use differential backup.
 *
 * Returns: "full" or "diff"
 */
static const char *
determine_backup_type(void)
{
    int     ret;
    bool    isnull;

    /* Query last full backup time */
    ret = SPI_execute(
        "SELECT rsp_time FROM hg_flashback_db.restore_points_base "
        "WHERE backup_type = 'full' "
        "ORDER BY rsp_time DESC LIMIT 1",
        true, 1);

    if (ret != SPI_OK_SELECT)
        elog(ERROR, "SPI_execute failed: %d", ret);

    /* No full backup yet, need full backup */
    if (SPI_processed == 0)
        return "full";

    {
        Datum       val;
        TimestampTz last_full;
        TimestampTz threshold;

        val = SPI_getbinval(SPI_tuptable->vals[0],
                            SPI_tuptable->tupdesc, 1, &isnull);
        if (isnull)
            return "full";

        last_full = DatumGetTimestampTz(val);
        /* Check if full backup interval has passed */
        threshold = TimestampTzPlusMilliseconds(
            last_full,
            (int64) hg_flashback_db_retention_target * 60 * 1000);

        if (GetCurrentTimestamp() >= threshold)
            return "full";
    }

    /* Use differential backup to save space/time */
    return "diff";
}

/*
 * drop_expired_restore_points
 *	Drop restore points that have exceeded retention target.
 *
 * Diff restore points are dropped after db_flashback_retention_target minutes.
 * Full restore points are dropped after 2 * db_flashback_retention_target minutes
 * (to allow dependent diffs to be cleaned up first).
 *
 * Expires the associated pgBackRest backup before removing the metadata.
 */
static void
drop_expired_restore_points(void)
{
    int     ret;
    int     dropped = 0;
    bool    dry_run = (hg_flashback_db_dry_run == 1);
    char   *query;

    SetCurrentStatementStartTimestamp();
    StartTransactionCommand();
    SPI_connect();
    PushActiveSnapshot(GetTransactionSnapshot());

    query = psprintf(
        "SELECT name FROM hg_flashback_db.restore_points_base "
        "WHERE (backup_type = 'diff' AND rsp_time < now() - interval '1 minute' * %d) "
        "   OR (backup_type = 'full' AND rsp_time < now() - interval '1 minute' * %d * 2) "
        "ORDER BY rsp_time ASC",
        hg_flashback_db_retention_target,
        hg_flashback_db_retention_target);

    ret = SPI_execute(query, true, 0);
    pfree(query);

    if (ret != SPI_OK_SELECT)
        elog(ERROR, "SPI_execute failed: %d", ret);

    if (SPI_processed > 0)
    {
        int     i;
        char   *names[100];
        int     count = SPI_processed;

        if (count > 100)
            count = 100;

        for (i = 0; i < count; i++)
        {
            Datum       val;
            bool        isnull;

            val = SPI_getbinval(SPI_tuptable->vals[i],
                                SPI_tuptable->tupdesc, 1, &isnull);
            if (!isnull)
                names[i] = hg_flashback_db_text_datum_copy(val,
                                                           TopMemoryContext);
            else
                names[i] = NULL;
        }

        PopActiveSnapshot();
        SPI_finish();
        CommitTransactionCommand();

        if (dry_run)
        {
            ereport(LOG,
                    (errmsg("hg_flashback_db: dry-run would drop %d expired restore points",
                            count)));

            for (i = 0; i < count; i++)
            {
                if (names[i] != NULL)
                {
                    ereport(LOG,
                            (errmsg("hg_flashback_db: dry-run would drop expired restore point \"%s\"",
                                    names[i])));
                    pfree(names[i]);
                }
            }

            return;
        }

        for (i = 0; i < count; i++)
        {
            if (names[i] != NULL)
            {
                Oid         argtypes[2] = {TEXTOID, BOOLOID};
                Datum       values[2]   = {CStringGetTextDatum(names[i]), BoolGetDatum(false)};

                SetCurrentStatementStartTimestamp();
                StartTransactionCommand();
                SPI_connect();
                PushActiveSnapshot(GetTransactionSnapshot());

                ret = SPI_execute_with_args(
                    "SELECT hg_flashback_db.drop_restore_point($1, $2)",
                    2, argtypes, values, NULL, true, 1);

                PopActiveSnapshot();
                SPI_finish();
                CommitTransactionCommand();

                if (ret == SPI_OK_SELECT && SPI_processed > 0)
                    dropped++;

                pfree(names[i]);
            }
        }
    }
    else
    {
        PopActiveSnapshot();
        SPI_finish();
        CommitTransactionCommand();
    }

    if (dropped > 0)
        ereport(LOG,
                (errmsg("hg_flashback_db: dropped %d expired restore points", dropped)));
}

/*
 * do_auto_restore_point
 *	Execute automatic restore point creation workflow.
 *
 * This is the core path for automatic restore point creation:
 * 1. Acquire advisory lock to prevent concurrent operations
 * 2. Determine backup type (full or diff) based on schedule
 * 3. Execute pgBackRest backup
 * 4. Create PostgreSQL restore point via XLogRestorePoint
 * 5. Record the restore point metadata in the database
 * 6. Release lock and cleanup
 *
 * Uses PG_TRY/PG_CATCH for error handling to ensure lock release on failure.
 */
static void
do_auto_restore_point(void)
{
    const char *backup_type;
    char        rp_name[64];
    char       *label;
    LOCKTAG     tag;
    bool        lock_held;
    int         ret;

    backup_type = NULL;
    label = NULL;
    lock_held = false;

    /* Acquire advisory lock to prevent concurrent operations */
    SET_LOCKTAG_ADVISORY(tag, MyDatabaseId,
                         HG_FLASHBACK_DB_LOCK_ID1,
                         HG_FLASHBACK_DB_LOCK_ID2, 2);
    if (LockAcquire(&tag, ExclusiveLock, true, true) == LOCKACQUIRE_NOT_AVAIL)
    {
        ereport(LOG,
                (errmsg("hg_flashback_db: skipping auto restore point, "
                        "another operation in progress")));
        return;
    }
    lock_held = true;

    PG_TRY();
    {
        /* Start transaction to query backup type */
        SetCurrentStatementStartTimestamp();
        StartTransactionCommand();
        SPI_connect();
        PushActiveSnapshot(GetTransactionSnapshot());

        backup_type = determine_backup_type();
        backup_type = MemoryContextStrdup(TopMemoryContext, backup_type);

        PopActiveSnapshot();
        SPI_finish();
        CommitTransactionCommand();

        /* Execute pgBackRest backup */
        if (!pgbackrest_execute_backup(backup_type))
        {
            ereport(WARNING,
                    (errmsg("hg_flashback_db: auto %s backup failed, "
                            "will retry next cycle", backup_type)));

            LockRelease(&tag, ExclusiveLock, true);
            lock_held = false;
            pfree((char *) backup_type);
            return;
        }

        /* Create restore point and record metadata */
        SetCurrentStatementStartTimestamp();
        StartTransactionCommand();
        SPI_connect();
        PushActiveSnapshot(GetTransactionSnapshot());

        generate_auto_name(rp_name, sizeof(rp_name));
        validate_restore_point_name(rp_name);
        label = pgbackrest_get_latest_backup_label(TopMemoryContext);

        {
            XLogRecPtr lsn = XLogRestorePoint(rp_name);
            char       lsn_str[32];
            Oid        argtypes[5] = {TEXTOID, TEXTOID, LSNOID, TEXTOID, TEXTOID};
            Datum      values[5];
            char       nulls[5] = {' ', ' ', ' ', ' ', ' '};

            snprintf(lsn_str, sizeof(lsn_str), "%X/%X",
                     (uint32) (lsn >> 32), (uint32) lsn);

            values[0] = CStringGetTextDatum(rp_name);
            values[1] = CStringGetTextDatum("auto");
            values[2] = DirectFunctionCall1(pg_lsn_in, CStringGetDatum(lsn_str));

            if (label != NULL)
                values[3] = CStringGetTextDatum(label);
            else
                nulls[3] = 'n';
            values[4] = CStringGetTextDatum(backup_type);

            ret = SPI_execute_with_args(
                "INSERT INTO hg_flashback_db.restore_points_base "
                "(name, type, lsn, backup_label, backup_type) "
                "VALUES ($1, $2, $3, $4, $5)",
                5, argtypes, values, nulls, false, 0);

            if (ret != SPI_OK_INSERT)
                elog(ERROR, "SPI_execute_with_args failed: %d", ret);
        }

        PopActiveSnapshot();
        SPI_finish();
        CommitTransactionCommand();

        ereport(LOG,
                (errmsg("hg_flashback_db: auto restore point \"%s\" created "
                        "with %s backup", rp_name, backup_type)));

        LockRelease(&tag, ExclusiveLock, true);
        lock_held = false;
        pfree((char *) backup_type);

        if (label != NULL)
            pfree(label);
    }
    PG_CATCH();
    {
        /* Ensure lock is released on error */
        if (IsTransactionState())
            AbortCurrentTransaction();

        if (lock_held)
            LockRelease(&tag, ExclusiveLock, true);

        if (backup_type != NULL)
            pfree((char *) backup_type);

        if (label != NULL)
            pfree(label);

        PG_RE_THROW();
    }
    PG_END_TRY();
}

/*
 * hg_flashback_db_bgw_main
 *	Main entry point for the flashback background worker.
 *
 * This is the entry point registered with PostgreSQL's background worker
 * framework. It runs as a long-lived process after recovery completes.
 *
 * Main loop:
 * 1. Retry every few seconds until extension metadata is available
 * 2. After metadata is ready, wait for hg_flashback_db_logging_interval
 *    (or 1 second in dry-run mode)
 * 3. Check if stanza is configured
 * 4. Execute drop_expired_restore_points() to clean up old restore points
 * 5. Execute do_auto_restore_point() to create restore point and backup
 * 6. Handle configuration reload (SIGHUP)
 * 7. Handle graceful shutdown (SIGTERM)
 */
void
hg_flashback_db_bgw_main(Datum main_arg)
{
    long    interval_ticks = (long) hg_flashback_db_logging_interval * 60;
    long    required_ticks;
    bool    metadata_ready = false;

    /* Setup signal handlers */
    pqsignal(SIGTERM, SignalHandlerForShutdownRequest);
    pqsignal(SIGHUP, SignalHandlerForConfigReload);
    BackgroundWorkerUnblockSignals();

    /* Initialize database connection */
    BackgroundWorkerInitializeConnection(hg_flashback_db_bgw_database, NULL, 0);

    ereport(LOG,
            (errmsg("hg_flashback_db: background worker started, "
                    "interval=%d min", hg_flashback_db_logging_interval)));

    /* Main worker loop */
    while (!ShutdownRequestPending)
    {

        /* Handle SIGHUP for config reload */
        if (ConfigReloadPending)
        {
            ConfigReloadPending = false;
            ProcessConfigFile(PGC_SIGHUP);
        }

        /*
         * Calculate required_ticks:
         * - dry_run mode: execute every cycle (required_ticks = 1)
         * - normal mode: execute after logging_interval minutes
         *   (required_ticks = logging_interval * 60 seconds)
         * - minimum required_ticks is 1 to ensure at least 1 second between runs
         */
        required_ticks = hg_flashback_db_dry_run ? 1 :
            (long) hg_flashback_db_logging_interval * 60;
        if (required_ticks < 1)
            required_ticks = 1;

        /*
         * Check if interval has elapsed:
         * - If counter reached threshold, execute restore point
         * - Otherwise, increment counter and wait
         */
        if (interval_ticks >= required_ticks)
        {
            /* Check if stanza is configured */
            if (hg_flashback_db_stanza == NULL ||
                hg_flashback_db_stanza[0] == '\0')
            {
                ereport(LOG,
                        (errmsg("hg_flashback_db: stanza not configured, "
                                "skipping")));
            }
            else
            {
                if (!metadata_ready)
                    metadata_ready = restore_points_table_exists();

                if (!metadata_ready)
                {
                    ereport(LOG,
                            (errmsg("hg_flashback_db: extension not found, "
                                    "bgw exiting. Please run: CREATE EXTENSION hg_flashback_db")));
                    ShutdownRequestPending = true;
                }
                else
                {
                    PG_TRY();
                    {
                        drop_expired_restore_points();
                        do_auto_restore_point();
                    }
                    PG_CATCH();
                    {
                        if (IsTransactionState())
                            AbortCurrentTransaction();

                        EmitErrorReport();
                        FlushErrorState();
                    }
                    PG_END_TRY();
                }
            }

            /* Reset counter after execution */
            interval_ticks = 0;
        }
        else
        {
            /* Increment counter and sleep for 1 second */
            interval_ticks++;

            WaitLatch(MyLatch,
                      WL_LATCH_SET | WL_TIMEOUT | WL_EXIT_ON_PM_DEATH,
                      1000L,
                      PG_WAIT_EXTENSION);
            ResetLatch(MyLatch);
        }
    }

    ereport(LOG, (errmsg("hg_flashback_db: background worker shutting down")));
}
