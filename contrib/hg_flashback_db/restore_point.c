/*-------------------------------------------------------------------------
 *
 * File: restore_point.c
 *
 * Abstract: support SQL functions for management restore point.
 *
 * Copyright:
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *	  		contrib/hg_flashback_db/restore_point.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"
#include "access/xact.h"
#include "access/xlog.h"
#include "access/xlog_internal.h"
#include "catalog/pg_type.h"
#include "executor/spi.h"
#include "miscadmin.h"
#include "storage/lock.h"
#include "utils/builtins.h"
#include "utils/pg_lsn.h"
#include "utils/timestamp.h"
#include "hg_flashback_db.h"

PG_FUNCTION_INFO_V1(hg_flashback_db_create_restore_point);
PG_FUNCTION_INFO_V1(hg_flashback_db_drop_restore_point);
PG_FUNCTION_INFO_V1(hg_flashback_db_cleanup_orphan_restore_points);

/* Forward declaration */
static int cleanup_orphan_restore_points_internal(void);

/*
 * validate_restore_point_name
 *	Validate restore point name requirements.
 *
 * Checks:
 * 1. Not in recovery mode (WAL control functions require normal operation)
 * 2. WAL level is sufficient (replica or logical required)
 * 3. Name length is within limits
 */
void
validate_restore_point_name(const char *name)
{
    /* Cannot create restore point during recovery */
    if (RecoveryInProgress())
        ereport(ERROR,
                (errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
                 errmsg("recovery is in progress"),
                 errhint("WAL control functions cannot be executed during recovery.")));

    /* WAL level must be replica or logical */
    if (!XLogIsNeeded())
        ereport(ERROR,
                (errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
                 errmsg("WAL level not sufficient for creating a restore point"),
                 errhint("wal_level must be set to \"replica\" or \"logical\" at server start.")));

    /* Check name length */
    if (strlen(name) >= MAXFNAMELEN)
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("value too long for restore point (maximum %d characters)",
                        MAXFNAMELEN - 1)));
}

/*
 * acquire_flashback_lock
 *	Acquire advisory lock for flashback operations.
 *
 * Uses PostgreSQL advisory locks to prevent concurrent flashback operations
 * which could lead to inconsistent state.
 *
 * Parameters:
 *   dontWait - if true, return immediately if lock unavailable
 *
 * Returns:
 *   true - lock acquired
 *   false - lock not available (if dontWait=true)
 */
static bool
acquire_flashback_lock(bool dontWait)
{
    LOCKTAG     tag;

    /* Acquire advisory lock to prevent concurrent operations */
    SET_LOCKTAG_ADVISORY(tag, MyDatabaseId,
                         HG_FLASHBACK_DB_LOCK_ID1,
                         HG_FLASHBACK_DB_LOCK_ID2, 2);

    return LockAcquire(&tag, ExclusiveLock,
                       false,
                       dontWait) != LOCKACQUIRE_NOT_AVAIL;
}

/*
 * hg_flashback_db_create_restore_point
 *	SQL-callable function to create a restore point.
 *
 * This is the user-facing API that can be called via:
 *   SELECT hg_flashback_db.create_restore_point('name')
 *   SELECT hg_flashback_db.create_restore_point('name', 'full')
 *
 * Workflow:
 * 1. Validate input name
 * 2. Check for duplicate name
 * 3. Optionally execute pgBackRest backup
 * 4. Create PostgreSQL restore point via XLogRestorePoint
 * 5. Store metadata in restore_points_base table
 *
 * Parameters:
 *   arg[0] - restore point name (text)
 *   arg[1] - optional backup type: 'full', 'diff', 'incr' (text)
 *
 * Returns:
 *   LSN of the restore point
 */
Datum
hg_flashback_db_create_restore_point(PG_FUNCTION_ARGS)
{
    text       *name_text;
    char       *name;
    char       *backup_type = NULL;
    char       *label = NULL;
    char       *rp_type = "manual";
    XLogRecPtr  lsn;
    char        lsn_str[32];
    int         ret;

    /* Get restore point name from argument */
    if (PG_ARGISNULL(0))
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("restore point name must not be NULL")));

    name_text = PG_GETARG_TEXT_PP(0);
    name = text_to_cstring(name_text);
    validate_restore_point_name(name);

    /* Optional backup type argument */
    if (!PG_ARGISNULL(1))
    {
        backup_type = text_to_cstring(PG_GETARG_TEXT_PP(1));
        if (strcmp(backup_type, "full") != 0 &&
            strcmp(backup_type, "diff") != 0 &&
            strcmp(backup_type, "incr") != 0)
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                     errmsg("invalid backup type: \"%s\"", backup_type),
                     errhint("Valid types: full, diff, incr.")));
    }

    /* Acquire lock to prevent concurrent flashback operations */
    if (!acquire_flashback_lock(true))
        ereport(ERROR,
                (errcode(ERRCODE_LOCK_NOT_AVAILABLE),
                 errmsg("another flashback operation is in progress")));

    SPI_connect();

    /* Check if restore point already exists */
    {
        Oid      argtypes[1] = {TEXTOID};
        Datum    values[1]   = {CStringGetTextDatum(name)};

        ret = SPI_execute_with_args(
            "SELECT 1 FROM hg_flashback_db.restore_points_base WHERE name = $1",
            1, argtypes, values, NULL, true, 1);

        if (ret != SPI_OK_SELECT)
            elog(ERROR, "SPI_execute_with_args failed: %d", ret);

        if (SPI_processed > 0)
            ereport(ERROR,
                    (errcode(ERRCODE_UNIQUE_VIOLATION),
                     errmsg("restore point \"%s\" already exists", name)));
    }

    /* Execute backup if requested */
    if (backup_type != NULL)
    {
        if (!pgbackrest_execute_backup(backup_type))
            ereport(ERROR,
                    (errcode(ERRCODE_EXTERNAL_ROUTINE_EXCEPTION),
                     errmsg("backup failed, restore point not created"),
                     errhint("Check server logs for pgBackRest error details.")));

        label = pgbackrest_get_latest_backup_label(CurrentMemoryContext);
    }

    /* Create PostgreSQL restore point */
    lsn = XLogRestorePoint(name);
    snprintf(lsn_str, sizeof(lsn_str), "%X/%X",
             (uint32) (lsn >> 32), (uint32) lsn);

    /* Insert restore point metadata into table */
    {
        Oid      argtypes[5] = {TEXTOID, TEXTOID, LSNOID, TEXTOID, TEXTOID};
        Datum    values[5];
        char     nulls[5] = {' ', ' ', ' ', ' ', ' '};

        values[0] = CStringGetTextDatum(name);
        values[1] = CStringGetTextDatum(rp_type);
        values[2] = DirectFunctionCall1(pg_lsn_in, CStringGetDatum(lsn_str));

        if (backup_type != NULL)
        {
            if (label != NULL)
                values[3] = CStringGetTextDatum(label);
            else
                nulls[3] = 'n';
            values[4] = CStringGetTextDatum(backup_type);
        }
        else
        {
            nulls[3] = 'n';
            nulls[4] = 'n';
        }

        ret = SPI_execute_with_args(
            "INSERT INTO hg_flashback_db.restore_points_base "
            "(name, type, lsn, backup_label, backup_type) "
            "VALUES ($1, $2, $3, $4, $5)",
            5, argtypes, values, nulls, false, 0);

        if (ret != SPI_OK_INSERT)
            elog(ERROR, "SPI_execute_with_args failed: %d", ret);
    }

    SPI_finish();

    if (label != NULL)
        pfree(label);

    PG_RETURN_LSN(lsn);
}

/*
 * hg_flashback_db_drop_restore_point
 *	SQL-callable function to drop a restore point.
 *
 * Removes the restore point from the metadata table and optionally
 * executes pgBackRest expire to remove the backup data.
 *
 * Parameters:
 *   arg[0] - restore point name (text)
 *   arg[1] - skip_backup (boolean, optional, default false)
 *            if true, skip pgBackRest expire command
 *
 * Returns:
 *   true on success; throws error if not found or lock unavailable
 */
Datum
hg_flashback_db_drop_restore_point(PG_FUNCTION_ARGS)
{
    text   *name_text = PG_GETARG_TEXT_PP(0);
    char   *name = text_to_cstring(name_text);
    bool    skip_backup = false;
    char   *backup_label = NULL;
    int     ret;

    /* Get optional skip_backup parameter */
    if (!PG_ARGISNULL(1))
        skip_backup = PG_GETARG_BOOL(1);

    /* Acquire lock to prevent concurrent operations */
    if (!acquire_flashback_lock(true))
        ereport(ERROR,
                (errcode(ERRCODE_LOCK_NOT_AVAILABLE),
                 errmsg("another flashback operation is in progress")));

    SPI_connect();

    /* First, get the backup_label before deleting */
    {
        Oid         argtypes[1] = {TEXTOID};
        Datum       values[1]   = {CStringGetTextDatum(name)};
        bool        isnull;

        ret = SPI_execute_with_args(
            "SELECT backup_label FROM hg_flashback_db.restore_points_base WHERE name = $1",
            1, argtypes, values, NULL, true, 1);

        if (ret != SPI_OK_SELECT)
            elog(ERROR, "SPI_execute_with_args failed: %d", ret);

        if (SPI_processed > 0)
        {
            Datum val = SPI_getbinval(SPI_tuptable->vals[0],
                                      SPI_tuptable->tupdesc, 1, &isnull);
            if (!isnull)
                backup_label = hg_flashback_db_text_datum_copy(val,
                                                               CurrentMemoryContext);
        }
    }

    /* Execute pgBackRest expire if backup_label exists and skip_backup is false */
    if (backup_label != NULL && !skip_backup)
    {
        if (!pgbackrest_expire_backup(backup_label))
            ereport(ERROR,
                    (errcode(ERRCODE_EXTERNAL_ROUTINE_EXCEPTION),
                     errmsg("pgbackrest expire failed, restore point not dropped"),
                     errhint("Check server logs for pgBackRest error details.")));

        pfree(backup_label);
    }
    else if (backup_label != NULL)
        pfree(backup_label);

    /* Delete restore point from metadata table */
    {
        Oid      argtypes[1] = {TEXTOID};
        Datum    values[1]   = {CStringGetTextDatum(name)};

        ret = SPI_execute_with_args(
            "DELETE FROM hg_flashback_db.restore_points_base WHERE name = $1",
            1, argtypes, values, NULL, false, 0);

        if (ret != SPI_OK_DELETE)
            elog(ERROR, "SPI_execute_with_args failed: %d", ret);

        if (SPI_processed == 0)
            ereport(ERROR,
                    (errcode(ERRCODE_UNDEFINED_OBJECT),
                     errmsg("restore point \"%s\" does not exist", name)));
    }

    SPI_finish();

    /* Auto cleanup orphan restore points if enabled */
    if (hg_flashback_db_auto_cleanup)
    {
        SPI_connect();
        cleanup_orphan_restore_points_internal();
        SPI_finish();
    }

    PG_RETURN_BOOL(true);
}

/*
 * cleanup_orphan_restore_points_internal
 *	Cleanup restore points whose backups no longer exist in pgBackRest.
 *
 * This is called after drop_restore_point to clean up any orphaned
 * restore points (e.g., when a full backup is deleted and its
 * dependent diff/incr backups are also deleted by pgBackRest).
 *
 * Returns:
 *   Number of orphaned restore points cleaned up, or -1 on error.
 */
static int
cleanup_orphan_restore_points_internal(void)
{
    char      **existing_labels = NULL;
    int         existing_count = 0;
    int         cleaned = 0;
    int         ret;
    int         i;
    char      **orphan_names = NULL;
    int         orphan_count = 0;
    int         orphan_capacity = 8;

    existing_labels = pgbackrest_get_all_backup_labels(&existing_count,
                                                       CurrentMemoryContext);
    if (existing_labels == NULL)
    {
        ereport(LOG,
                (errmsg("hg_flashback_db: no backups found in pgBackRest, skipping cleanup")));
        return 0;
    }

    if (existing_count == 0)
    {
        ereport(LOG,
                (errmsg("hg_flashback_db: no backups found in pgBackRest, skipping cleanup")));
        return 0;
    }

    /* Query all restore points with backup_label */
    ret = SPI_execute(
        "SELECT name, backup_label FROM hg_flashback_db.restore_points_base "
        "WHERE backup_label IS NOT NULL",
        true, 0);

    if (ret != SPI_OK_SELECT)
    {
        for (i = 0; i < existing_count; i++)
            pfree(existing_labels[i]);
        pfree(existing_labels);

        ereport(WARNING,
                (errmsg("hg_flashback_db: failed to query restore points: %d", ret)));
        return -1;
    }

    ereport(LOG,
            (errmsg("hg_flashback_db: checking %d restore points against %d pgBackRest backups",
                    (int) SPI_processed, existing_count)));

    /* Allocate array for orphan names */
    orphan_names = (char **) palloc(sizeof(char *) * orphan_capacity);

    /* First pass: collect all orphan names */
    for (i = 0; i < SPI_processed; i++)
    {
        bool        isnull;
        Datum       name_val;
        Datum       label_val;
        char       *name = NULL;
        char       *label = NULL;
        bool        found = false;
        int         j;

        name_val = SPI_getbinval(SPI_tuptable->vals[i],
                                 SPI_tuptable->tupdesc, 1, &isnull);
        if (isnull)
            continue;

        name = hg_flashback_db_text_datum_copy(name_val, CurrentMemoryContext);

        label_val = SPI_getbinval(SPI_tuptable->vals[i],
                                 SPI_tuptable->tupdesc, 2, &isnull);
        if (isnull)
        {
            pfree(name);
            continue;
        }

        label = hg_flashback_db_text_datum_copy(label_val, CurrentMemoryContext);

        /* Check if label exists in pgBackRest backups */
        for (j = 0; j < existing_count; j++)
        {
            if (strcmp(label, existing_labels[j]) == 0)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            /* Collect orphan name for later deletion */
            if (orphan_count >= orphan_capacity)
            {
                orphan_capacity *= 2;
                orphan_names = (char **) repalloc(orphan_names, sizeof(char *) * orphan_capacity);
            }
            orphan_names[orphan_count++] = name;
            name = NULL;
        }

        if (name != NULL)
            pfree(name);
        pfree(label);
    }

    /* Free existing_labels array */
    for (i = 0; i < existing_count; i++)
        pfree(existing_labels[i]);
    pfree(existing_labels);

    /* Second pass: delete all orphans */
    for (i = 0; i < orphan_count; i++)
    {
        Oid         argtypes[1] = {TEXTOID};
        Datum       values[1]   = {CStringGetTextDatum(orphan_names[i])};

        ret = SPI_execute_with_args(
            "DELETE FROM hg_flashback_db.restore_points_base WHERE name = $1",
            1, argtypes, values, NULL, false, 0);

        if (ret == SPI_OK_DELETE && SPI_processed > 0)
        {
            ereport(LOG,
                    (errmsg("hg_flashback_db: cleaned up orphan restore point \"%s\"",
                            orphan_names[i])));
            cleaned++;
        }
        pfree(orphan_names[i]);
    }
    pfree(orphan_names);

    if (cleaned > 0)
        ereport(LOG,
                (errmsg("hg_flashback_db: cleaned up %d orphan restore point(s)", cleaned)));

    return cleaned;
}

/*
 * hg_flashback_db_cleanup_orphan_restore_points
 *	SQL-callable function to cleanup orphan restore points.
 *
 * Removes restore point metadata for backups that no longer exist
 * in pgBackRest (e.g., due to cascade deletion when expire removes
 * a full backup and its dependent backups).
 *
 * Returns:
 *   Number of orphan restore points cleaned up, or -1 on error.
 */
Datum
hg_flashback_db_cleanup_orphan_restore_points(PG_FUNCTION_ARGS)
{
    int     cleaned;

    /* Acquire lock to prevent concurrent operations */
    if (!acquire_flashback_lock(true))
        ereport(ERROR,
                (errcode(ERRCODE_LOCK_NOT_AVAILABLE),
                 errmsg("another flashback operation is in progress")));

    SPI_connect();

    cleaned = cleanup_orphan_restore_points_internal();

    SPI_finish();

    PG_RETURN_INT32(cleaned);
}
