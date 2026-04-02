/*-------------------------------------------------------------------------
 *
 * File: hg_flashback_db.h
 *
 * Abstract: extension for database level flashback.
 *
 * Copyright:
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *	  		contrib/hg_flashback_db/hg_flashback_db.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef HG_FLASHBACK_DB_H
#define HG_FLASHBACK_DB_H

#include "postgres.h"
#include "utils/builtins.h"

/* Advisory lock constants - prevent concurrent flashback operations */
#define HG_FLASHBACK_DB_LOCK_ID1  0x4846424B   /* "HFBK" */
#define HG_FLASHBACK_DB_LOCK_ID2  0

/* GUC variables - set via postgresql.conf */
extern int   hg_flashback_db_logging_interval;
extern char *hg_flashback_db_stanza;
extern char *hg_flashback_db_pgbackrest_path;
extern char *hg_flashback_db_config_path;
extern int   hg_flashback_db_dry_run;
extern bool  hg_flashback_db_enabled;
extern char *hg_flashback_db_bgw_database;
extern int   hg_flashback_db_retention_target;
extern bool  hg_flashback_db_auto_cleanup;

static inline char *
hg_flashback_db_text_datum_copy(Datum datum, MemoryContext context)
{
	char   *str;
	char   *copy;

	str = TextDatumGetCString(datum);
	copy = MemoryContextStrdup(context, str);
	pfree(str);

	return copy;
}

/* pgbackrest.c */
extern bool  pgbackrest_execute_backup(const char *backup_type);
extern char *pgbackrest_get_latest_backup_label(MemoryContext context);
extern bool  pgbackrest_expire_backup(const char *backup_label);
extern char **pgbackrest_get_all_backup_labels(int *count,
											   MemoryContext context);
extern Datum pgbackrest_get_total_backup_size(PG_FUNCTION_ARGS);
extern Datum pgbackrest_get_avg_backup_size(PG_FUNCTION_ARGS);

/* restore_point.c */
extern void validate_restore_point_name(const char *name);

/* bgw.c */
extern void  hg_flashback_db_bgw_main(Datum main_arg);

#endif /* HG_FLASHBACK_DB_H */
