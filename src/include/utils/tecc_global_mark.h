/*-------------------------------------------------------------------------
 *
 * tecc_global_mark.h
 *	  TECC global metadata update mark system declarations
 *
 * This module provides three independent marks for KEK, DEK, and encrypted
 * column updates. Frontend connections can check these marks via SQL
 * functions to determine which part of their cache needs refreshing.
 *
 * Cross-backend synchronization is achieved via syscache callbacks:
 * when another backend modifies TECC system tables, the shared invalidation
 * message system triggers callbacks that set the local marks.
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 *
 * IDENTIFICATION
 *	  src/include/utils/tecc_global_mark.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_GLOBAL_MARK_H
#define TECC_GLOBAL_MARK_H

#include "postgres.h"

/*
 * Global variable declarations.
 *
 * These are per-backend process-local variables. They are NOT shared
 * memory. Cross-backend synchronization is achieved via syscache
 * callbacks registered by tecc_init_global_mark().
 *
 * Each variable corresponds to a specific TECC system table:
 *   - tecc_kek_updated_mark:       set when hg_kek table is modified
 *   - tecc_dek_updated_mark:       set when hg_dek table is modified
 *   - tecc_column_updated_mark:    set when hg_encrypted_columns modified
 */

extern volatile bool tecc_kek_updated_mark;
extern volatile bool tecc_dek_updated_mark;
extern volatile bool tecc_column_updated_mark;

/*
 * Initialization function - registers syscache callbacks for
 * cross-backend synchronization.
 *
 * Must be called once during backend initialization (e.g., from
 * InitPostgres).
 */
extern void tecc_init_global_mark(void);

/* KEK-specific mark operations */
extern void tecc_mark_kek_updated(void);
extern void tecc_clear_kek_mark(void);
extern bool tecc_is_kek_updated(void);

/* DEK-specific mark operations */
extern void tecc_mark_dek_updated(void);
extern void tecc_clear_dek_mark(void);
extern bool tecc_is_dek_updated(void);

/* Encrypted column-specific mark operations */
extern void tecc_mark_column_updated(void);
extern void tecc_clear_column_mark(void);
extern bool tecc_is_column_updated(void);

/* SQL function declarations */
extern Datum hg_is_kek_updated(PG_FUNCTION_ARGS);
extern Datum hg_clear_kek_mark(PG_FUNCTION_ARGS);

extern Datum hg_is_dek_updated(PG_FUNCTION_ARGS);
extern Datum hg_clear_dek_mark(PG_FUNCTION_ARGS);

extern Datum hg_is_column_updated(PG_FUNCTION_ARGS);
extern Datum hg_clear_column_mark(PG_FUNCTION_ARGS);

/* Fastpath function declarations */
extern int tecc_get_cache_marks(void);
extern void tecc_clear_cache_marks(int mask);
extern Datum tecc_get_cache_marks_wrapper(PG_FUNCTION_ARGS);
extern Datum tecc_clear_cache_marks_wrapper(PG_FUNCTION_ARGS);
extern Datum tecc_update_meta(PG_FUNCTION_ARGS);

#endif /* TECC_GLOBAL_MARK_H */
