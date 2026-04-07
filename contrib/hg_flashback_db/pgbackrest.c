/*-------------------------------------------------------------------------
 *
 * File: pg_backrest.c
 *
 * Abstract: construct commond for pgbackrest.
 *
 * Copyright:
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *	  		contrib/hg_flashback_db/pg_backrest.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include "lib/stringinfo.h"
#include "storage/fd.h"
#include "executor/spi.h"
#include "utils/builtins.h"
#include "port.h"
#include "hg_flashback_db.h"

/*
 * build_base_command
 *	Build base pgBackRest command with stanza and config options.
 */
static void
build_base_command(StringInfo cmd)
{
    appendStringInfo(cmd, "\"%s\" --stanza=\"%s\"",
                     hg_flashback_db_pgbackrest_path,
                     hg_flashback_db_stanza);
    if (hg_flashback_db_config_path != NULL)
        appendStringInfo(cmd, " --config=\"%s\"", hg_flashback_db_config_path);
}

/*
 * pgbackrest_execute_backup
 *	Execute pgBackRest backup command.
 *
 * Parameters:
 *   backup_type - "full", "diff", or "incr"
 *
 * Returns:
 *   true - backup completed successfully
 *   false - backup failed
 */
bool
pgbackrest_execute_backup(const char *backup_type)
{
    StringInfoData cmd;
    FILE       *fp;
    char        buf[1024];
    int         pclose_rc;
    StringInfoData stderr_buf;

    if (hg_flashback_db_stanza == NULL || hg_flashback_db_stanza[0] == '\0')
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("hg_flashback_db.pgbackrest_stanza is not configured"),
                 errhint("Set hg_flashback_db.pgbackrest_stanza in postgresql.conf.")));

    initStringInfo(&cmd);
    build_base_command(&cmd);
    appendStringInfo(&cmd, " backup --type=%s 2>&1", backup_type);

    ereport(LOG,
            (errmsg("hg_flashback_db: executing backup command: %s", cmd.data)));

    /* Dry-run mode: just log the command without executing */
    if (hg_flashback_db_dry_run == 1)
    {
        ereport(LOG,
                (errmsg("hg_flashback_db: dry-run backup command: %s", cmd.data)));
        pfree(cmd.data);
        return true;
    }

    /* Test mode (value == 2): execute command but skip actual backup */
    if (hg_flashback_db_dry_run == 2)
    {
        ereport(LOG,
                (errmsg("hg_flashback_db: test-bypass backup command: %s", cmd.data)));
        pfree(cmd.data);
        return true;
    }

    /* Normal mode (value == 0): execute pgBackRest command */
    fp = OpenPipeStream(cmd.data, "r");
    if (fp == NULL)
        ereport(ERROR,
                (errcode_for_file_access(),
                 errmsg("hg_flashback_db: could not execute: %s", cmd.data)));

    /* Log output at DEBUG1 level */
    initStringInfo(&stderr_buf);
    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
        appendStringInfoString(&stderr_buf, buf);
        ereport(DEBUG1, (errmsg("pgbackrest: %s", buf)));
    }

    pclose_rc = ClosePipeStream(fp);
    pfree(cmd.data);

    if (pclose_rc != 0)
    {
        ereport(WARNING,
                (errmsg("hg_flashback_db: backup command failed: %s\n%s",
                        wait_result_to_str(pclose_rc),
                        stderr_buf.len > 0 ? stderr_buf.data : "")));
        pfree(stderr_buf.data);
        return false;
    }

    pfree(stderr_buf.data);

    ereport(LOG,
            (errmsg("hg_flashback_db: %s backup completed successfully",
                    backup_type)));
    return true;
}

/*
 * pgbackrest_get_latest_backup_label
 *	Get the latest backup label from pgBackRest info command.
 *
 * Executes "pgbackrest info" with JSON output and parses the result
 * to find the most recent backup's label. This label is used to
 * correlate restore points with their corresponding backups.
 *
 * Returns:
 *   Latest backup label (caller must pfree), or NULL if no backups exist.
 */
char *
pgbackrest_get_latest_backup_label(MemoryContext context)
{
    StringInfoData cmd;
    StringInfoData json_buf;
    FILE       *fp;
    char        buf[4096];
    int         ret;
    Oid         argtypes[1] = {TEXTOID};
    Datum       values[1];
    bool        isnull;
    char       *label = NULL;

    initStringInfo(&cmd);
    build_base_command(&cmd);
    appendStringInfoString(&cmd, " info --output=json");

    /* Dry-run mode (value == 1): return NULL without executing */
    if (hg_flashback_db_dry_run == 1)
    {
        ereport(LOG,
                (errmsg("hg_flashback_db: dry-run info command: %s", cmd.data)));
        pfree(cmd.data);
        return NULL;
    }

    /* Test mode (value == 2): execute and try to parse JSON, but handle errors gracefully */
    if (hg_flashback_db_dry_run == 2)
    {
        ereport(LOG,
                (errmsg("hg_flashback_db: test-bypass info command: %s", cmd.data)));

        fp = OpenPipeStream(cmd.data, "r");
        if (fp == NULL)
        {
            pfree(cmd.data);
            return NULL;
        }

        initStringInfo(&json_buf);
        while (fgets(buf, sizeof(buf), fp) != NULL)
            appendStringInfoString(&json_buf, buf);

        ClosePipeStream(fp);
        pfree(cmd.data);

        if (json_buf.len == 0)
        {
            pfree(json_buf.data);
            return NULL;
        }

        /* Try to parse JSON, log error but return NULL instead of throwing */
        PG_TRY();
        {
            values[0] = CStringGetTextDatum(json_buf.data);
            ret = SPI_execute_with_args(
                "SELECT b->>'label' "
                "FROM jsonb_array_elements($1::jsonb) AS s, "
                "     jsonb_array_elements(s->'backup') AS b "
                "ORDER BY b->'timestamp'->>'stop' DESC "
                "LIMIT 1",
                1, argtypes, values, NULL, true, 1);

            if (ret == SPI_OK_SELECT && SPI_processed > 0)
            {
                Datum val = SPI_getbinval(SPI_tuptable->vals[0],
                                          SPI_tuptable->tupdesc, 1, &isnull);
                if (!isnull)
                    label = hg_flashback_db_text_datum_copy(val, context);
            }
        }
        PG_CATCH();
        {
            ereport(LOG, (errmsg("invalid input syntax for type json")));
            FlushErrorState();
            pfree(json_buf.data);
            return NULL;
        }
        PG_END_TRY();

        pfree(json_buf.data);
        return label;
    }

    /* Normal mode (value == 0): execute and parse JSON (throws on error) */
    fp = OpenPipeStream(cmd.data, "r");
    if (fp == NULL)
    {
        pfree(cmd.data);
        return NULL;
    }

    /* Read JSON output */
    initStringInfo(&json_buf);
    while (fgets(buf, sizeof(buf), fp) != NULL)
        appendStringInfoString(&json_buf, buf);

    ClosePipeStream(fp);
    pfree(cmd.data);

    if (json_buf.len == 0)
    {
        pfree(json_buf.data);
        return NULL;
    }

    /* Parse JSON to get latest backup label */
    values[0] = CStringGetTextDatum(json_buf.data);
    ret = SPI_execute_with_args(
        "SELECT b->>'label' "
        "FROM jsonb_array_elements($1::jsonb) AS s, "
        "     jsonb_array_elements(s->'backup') AS b "
        "ORDER BY b->'timestamp'->>'stop' DESC "
        "LIMIT 1",
        1, argtypes, values, NULL, true, 1);

    if (ret == SPI_OK_SELECT && SPI_processed > 0)
    {
        Datum val = SPI_getbinval(SPI_tuptable->vals[0],
                                  SPI_tuptable->tupdesc, 1, &isnull);
        if (!isnull)
            label = hg_flashback_db_text_datum_copy(val, context);
    }

    pfree(json_buf.data);
    return label;
}

/*
 * pgbackrest_expire_backup
 *	Execute pgBackRest expire command to remove a specific backup set.
 *
 * Parameters:
 *   backup_label - The backup label to expire (e.g., "20260330-100917I")
 *
 * Returns:
 *   true - expire completed successfully
 *   false - expire failed
 */
bool
pgbackrest_expire_backup(const char *backup_label)
{
    StringInfoData cmd;
    FILE       *fp;
    char        buf[1024];
    int         pclose_rc;
    StringInfoData stderr_buf;

    if (hg_flashback_db_stanza == NULL || hg_flashback_db_stanza[0] == '\0')
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("hg_flashback_db.pgbackrest_stanza is not configured"),
                 errhint("Set hg_flashback_db.pgbackrest_stanza in postgresql.conf.")));

    if (backup_label == NULL || backup_label[0] == '\0')
        return true;

    initStringInfo(&cmd);
    build_base_command(&cmd);
    appendStringInfo(&cmd, " expire --set=%s 2>&1", backup_label);

    ereport(LOG,
            (errmsg("hg_flashback_db: executing expire command: %s", cmd.data)));

    /* Dry-run mode: just log the command without executing */
    if (hg_flashback_db_dry_run == 1)
    {
        ereport(LOG,
                (errmsg("hg_flashback_db: dry-run expire command: %s", cmd.data)));
        pfree(cmd.data);
        return true;
    }

    /* Test mode (value == 2): execute command but skip actual expire */
    if (hg_flashback_db_dry_run == 2)
    {
        ereport(LOG,
                (errmsg("hg_flashback_db: test-bypass expire command: %s", cmd.data)));
        pfree(cmd.data);
        return true;
    }

    /* Normal mode (value == 0): execute pgBackRest command */
    fp = OpenPipeStream(cmd.data, "r");
    if (fp == NULL)
        ereport(ERROR,
                (errcode_for_file_access(),
                 errmsg("hg_flashback_db: could not execute: %s", cmd.data)));

    /* Log output at DEBUG1 level */
    initStringInfo(&stderr_buf);
    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
        appendStringInfoString(&stderr_buf, buf);
        ereport(DEBUG1, (errmsg("pgbackrest: %s", buf)));
    }

    pclose_rc = ClosePipeStream(fp);
    pfree(cmd.data);

    if (pclose_rc != 0)
    {
        ereport(WARNING,
                (errmsg("hg_flashback_db: expire command failed: %s\n%s",
                        wait_result_to_str(pclose_rc),
                        stderr_buf.len > 0 ? stderr_buf.data : "")));
        pfree(stderr_buf.data);
        return false;
    }

    pfree(stderr_buf.data);

    ereport(LOG,
            (errmsg("hg_flashback_db: expire completed successfully for backup %s",
                    backup_label)));
    return true;
}

/*
 * pgbackrest_get_all_backup_labels
 *	Get all backup labels from pgBackRest info command.
 *
 * Executes "pgbackrest info" with JSON output and parses the result
 * to find all backup labels in the stanza.
 *
 * Parameters:
 *   count - output parameter for number of labels returned
 *
 * Returns:
 *   Array of backup labels (caller must pfree each + array), or NULL if none.
 */
char **
pgbackrest_get_all_backup_labels(int *count, MemoryContext context)
{
    StringInfoData cmd;
    StringInfoData json_buf;
    FILE       *fp;
    char        buf[4096];
    int         ret;
    Oid         argtypes[1] = {TEXTOID};
    Datum       values[1];
    char      **labels = NULL;
    int         labels_capacity = 8;
    int         labels_count = 0;

    *count = 0;

    if (hg_flashback_db_stanza == NULL || hg_flashback_db_stanza[0] == '\0')
        return NULL;

    initStringInfo(&cmd);
    build_base_command(&cmd);
    appendStringInfoString(&cmd, " info --output=json");

    /* Dry-run mode: return NULL without executing */
    if (hg_flashback_db_dry_run == 1)
    {
        ereport(LOG,
                (errmsg("hg_flashback_db: dry-run info command: %s", cmd.data)));
        pfree(cmd.data);
        return NULL;
    }

    /* Execute command */
    fp = OpenPipeStream(cmd.data, "r");
    if (fp == NULL)
    {
        pfree(cmd.data);
        return NULL;
    }

    /* Read JSON output */
    initStringInfo(&json_buf);
    while (fgets(buf, sizeof(buf), fp) != NULL)
        appendStringInfoString(&json_buf, buf);

    ClosePipeStream(fp);
    pfree(cmd.data);

    if (json_buf.len == 0)
    {
        pfree(json_buf.data);
        return NULL;
    }

    /* Allocate initial array */
    labels = (char **) MemoryContextAlloc(context,
                                          sizeof(char *) * labels_capacity);

    /* Parse JSON to get all backup labels */
    values[0] = CStringGetTextDatum(json_buf.data);
    ret = SPI_execute_with_args(
        "SELECT b->>'label' "
        "FROM jsonb_array_elements($1::jsonb) AS s, "
        "     jsonb_array_elements(s->'backup') AS b",
        1, argtypes, values, NULL, true, 0);

    if (ret == SPI_OK_SELECT)
    {
        int i;
        for (i = 0; i < SPI_processed; i++)
        {
            bool        isnull;
            Datum       val;
            val = SPI_getbinval(SPI_tuptable->vals[i],
                              SPI_tuptable->tupdesc, 1, &isnull);
            if (isnull)
                continue;

            /* Grow array if needed */
            if (labels_count >= labels_capacity)
            {
                labels_capacity *= 2;
                labels = (char **) repalloc(labels, sizeof(char *) * labels_capacity);
            }

            labels[labels_count++] = hg_flashback_db_text_datum_copy(val, context);
        }
    }

    pfree(json_buf.data);

    if (labels_count == 0)
    {
        pfree(labels);
        labels = NULL;
    }

    *count = labels_count;
    return labels;
}

/*
 * pgbackrest_get_total_backup_size
 *	Get total size of all backups from pgBackRest info command.
 *
 * Returns:
 *   Total size in bytes, or 0 if no backups or on error.
 */
PG_FUNCTION_INFO_V1(pgbackrest_get_total_backup_size);
Datum
pgbackrest_get_total_backup_size(PG_FUNCTION_ARGS)
{
    StringInfoData cmd;
    StringInfoData json_buf;
    FILE       *fp;
    char        buf[4096];
    int         ret;
    Oid         argtypes[1] = {TEXTOID};
    Datum       values[1];
    int64       total_size = 0;

    if (hg_flashback_db_stanza == NULL || hg_flashback_db_stanza[0] == '\0')
        PG_RETURN_INT64(0);

    initStringInfo(&cmd);
    build_base_command(&cmd);
    appendStringInfoString(&cmd, " info --output=json");

    /* Dry-run or test mode: return 0 */
    if (hg_flashback_db_dry_run > 0)
    {
        pfree(cmd.data);
        PG_RETURN_INT64(0);
    }

    /* Execute command */
    fp = OpenPipeStream(cmd.data, "r");
    if (fp == NULL)
    {
        pfree(cmd.data);
        PG_RETURN_INT64(0);
    }

    /* Read JSON output */
    initStringInfo(&json_buf);
    while (fgets(buf, sizeof(buf), fp) != NULL)
        appendStringInfoString(&json_buf, buf);

    ClosePipeStream(fp);
    pfree(cmd.data);

    if (json_buf.len == 0)
    {
        pfree(json_buf.data);
        PG_RETURN_INT64(0);
    }

    /* Connect to SPI for JSON parsing */
    SPI_connect();

    /* Parse JSON to sum all backup sizes */
    PG_TRY();
    {
        values[0] = CStringGetTextDatum(json_buf.data);
        ret = SPI_execute_with_args(
            "SELECT COALESCE(SUM((b->'info'->>'size')::bigint), 0)::bigint "
            "FROM jsonb_array_elements($1::jsonb) AS s, "
            "     jsonb_array_elements(s->'backup') AS b",
            1, argtypes, values, NULL, true, 1);

        if (ret == SPI_OK_SELECT && SPI_processed > 0)
        {
            bool        isnull;
            Datum       val;

            val = SPI_getbinval(SPI_tuptable->vals[0],
                              SPI_tuptable->tupdesc, 1, &isnull);
            if (!isnull)
                total_size = DatumGetInt64(val);
        }
    }
    PG_CATCH();
    {
        FlushErrorState();
        SPI_finish();
        pfree(json_buf.data);
        PG_RETURN_INT64(0);
    }
    PG_END_TRY();

    SPI_finish();
    pfree(json_buf.data);
    PG_RETURN_INT64(total_size);
}

/*
 * pgbackrest_get_avg_backup_size
 *	Get average size of backups by type from pgBackRest info command.
 *
 * Parameters:
 *   backup_type - "full", "diff", or "incr"
 *
 * Returns:
 *   Average size in bytes, or 0 if no backups of that type or on error.
 */
PG_FUNCTION_INFO_V1(pgbackrest_get_avg_backup_size);
Datum
pgbackrest_get_avg_backup_size(PG_FUNCTION_ARGS)
{
    StringInfoData cmd;
    StringInfoData json_buf;
    FILE       *fp;
    char        buf[4096];
    int         ret;
    Oid         argtypes[2] = {TEXTOID, TEXTOID};
    Datum       values[2];
    int64       avg_size = 0;
    text       *backup_type_arg = PG_GETARG_TEXT_PP(0);
    char       *backup_type = NULL;

    if (hg_flashback_db_stanza == NULL || hg_flashback_db_stanza[0] == '\0')
        PG_RETURN_INT64(0);

    if (backup_type_arg != NULL)
        backup_type = text_to_cstring(backup_type_arg);

    if (backup_type == NULL)
        PG_RETURN_INT64(0);

    initStringInfo(&cmd);
    build_base_command(&cmd);
    appendStringInfoString(&cmd, " info --output=json");

    /* Dry-run or test mode: return 0 */
    if (hg_flashback_db_dry_run > 0)
    {
        pfree(cmd.data);
        PG_RETURN_INT64(0);
    }

    /* Execute command */
    fp = OpenPipeStream(cmd.data, "r");
    if (fp == NULL)
    {
        pfree(cmd.data);
        PG_RETURN_INT64(0);
    }

    /* Read JSON output */
    initStringInfo(&json_buf);
    while (fgets(buf, sizeof(buf), fp) != NULL)
        appendStringInfoString(&json_buf, buf);

    ClosePipeStream(fp);
    pfree(cmd.data);

    if (json_buf.len == 0)
    {
        pfree(json_buf.data);
        PG_RETURN_INT64(0);
    }

    /* Connect to SPI for JSON parsing */
    SPI_connect();

    /* Parse JSON to get average size by backup type */
    PG_TRY();
    {
        values[0] = CStringGetTextDatum(json_buf.data);
        values[1] = CStringGetTextDatum(backup_type);

        ret = SPI_execute_with_args(
            "SELECT COALESCE(AVG((b->'info'->>'size')::bigint), 0)::bigint "
            "FROM jsonb_array_elements($1::jsonb) AS s, "
            "     jsonb_array_elements(s->'backup') AS b "
            "WHERE b->>'type' = $2",
            2, argtypes, values, NULL, true, 1);

        if (ret == SPI_OK_SELECT && SPI_processed > 0)
        {
            bool        isnull;
            Datum       val;

            val = SPI_getbinval(SPI_tuptable->vals[0],
                              SPI_tuptable->tupdesc, 1, &isnull);
            if (!isnull)
                avg_size = DatumGetInt64(val);
        }
    }
    PG_CATCH();
    {
        FlushErrorState();
        SPI_finish();
        pfree(json_buf.data);
        PG_RETURN_INT64(0);
    }
    PG_END_TRY();

    SPI_finish();
    pfree(json_buf.data);
    PG_RETURN_INT64(avg_size);
}
