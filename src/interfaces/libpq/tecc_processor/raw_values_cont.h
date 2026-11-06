/* Begin - ReqID:TECC4.0 */
/*
 * File: raw_values_cont.h
 *
 * Abstract:
 *   Raw values extraction utilities for TECC processor module.
 *   Based on openGauss RawValues class design, provides functions to extract
 *   raw values from various SQL statements (INSERT, UPDATE, EXECUTE, etc.)
 *   and build RawValue objects for encryption/decryption processing.
 *
 * Authored by <wangxp@highgo.com>, 20260412.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/raw_values_cont.h
 */

#ifndef RAW_VALUES_CONT_H
#define RAW_VALUES_CONT_H

#include "tecc_processor_types.h"  /* Include type definitions */
#include "raw_value.h"             /* For TeccRawValue */
#include "raw_values_list.h"       /* For TeccRawValuesList */

/* Standard C headers for frontend code */
#include <stddef.h>  /* For size_t */
#include <stdbool.h> /* For bool */

/* Include frontend parser types */
#include "../frontend_parser/tecc_nodes.h"

/* Forward declarations for types that may not be fully defined yet */
typedef struct TeccExprPartsList TeccExprPartsList;
typedef struct TeccExprPart TeccExprPart;

/*
 * Advanced container structure (TECC extension)
 * Provides enhanced features beyond basic raw_values_list
 */
typedef struct TeccRawValuesCont
{
    /* Core data */
    TeccRawValue **values;          /* Array of raw value pointers */
    int num_values;                 /* Current number of values */
    int capacity;                   /* Current capacity of array */

    /* Advanced features */
    bool sorted_by_column;          /* Whether values are sorted by column name */
    int *column_index_map;          /* Map from column name to value index */
    int max_column_index;           /* Maximum column index in map */

    /* Memory management */
    size_t total_memory;            /* Total memory used */

    /* Error tracking */
    int last_error;                 /* Last error code */
    char last_error_msg[256];       /* Last error message */

} TeccRawValuesCont;

/*
 * Raw values extraction interface - based on openGauss RawValues class
 * All functions are static utility functions, no instance needed
 */

/* Extract raw values from INSERT statement */
bool tecc_raw_values_from_insert_stmt(
    const InsertStmt *insert_stmt,
    TeccStatementData *statement_data,
    TeccRawValuesList *raw_values_list);

/* Extract raw values from UPDATE statement */
bool tecc_raw_values_from_update_stmt(
    const UpdateStmt *update_stmt,
    TeccStatementData *statement_data,
    TeccRawValuesList *raw_values_list);

/* Extract raw values from EXECUTE statement */
bool tecc_raw_values_from_execute_stmt(
    const ExecuteStmt *execute_stmt,
    TeccStatementData *statement_data,
    TeccRawValuesList *raw_values_list);

/* Extract raw values from constants vector */
bool tecc_raw_values_from_consts_vec(
    const TeccExprPartsList *expr_parts_list,
    TeccStatementData *statement_data,
    int offset,
    TeccRawValuesList *raw_values_list);

/* Reverse lookup: get unprocessed data from processed data */
bool tecc_raw_values_get_unprocessed_data(
    const TeccRawValuesList *raw_values_list,
    const unsigned char *processed_data,
    unsigned char *unprocessed_data,
    size_t *size);

/*
 * Internal utility functions (not exposed in public API)
 * These correspond to private methods in openGauss RawValues class
 */

/* Count literals in string */
size_t tecc_raw_values_count_literals(const char *str, size_t size);

/* Get cached default values for table */
bool tecc_raw_values_get_cached_default_values(
    const char *db_name,
    const char *schema_name,
    const char *table_name,
    TeccRawValuesList *raw_values_list);

/* Extract raw values from INSERT...SELECT statement */
bool tecc_raw_values_from_insert_select_stmt(
    const SelectStmt *select_stmt,
    TeccStatementData *statement_data,
    TeccRawValuesList *raw_values_list);

/* Extract raw values from value lists */
bool tecc_raw_values_from_value_lists(
    List *exprList,
    TeccStatementData *statement_data,
    TeccRawValuesList *raw_values_list);

/* Extract raw values from node tag (value type) */
void tecc_raw_values_from_nodetag(
    const Value *value,
    int *end,
    int loc,
    const TeccExprPart *xpr,
    bool *is_emptyb,
    bool *is_str,
    const TeccStatementData *statement_data);

/* Extract raw values from expression parts */
TeccRawValue *tecc_raw_values_from_expr_parts(
    const TeccExprPart *expr_parts,
    TeccStatementData *statement_data,
    size_t offset);

/*
 * Advanced container operations (extended beyond openGauss)
 * These provide enhanced functionality for TECC
 */

/* Create advanced raw values container */
TeccRawValuesCont *tecc_raw_values_cont_create(int initial_capacity);

/* Destroy advanced raw values container */
void tecc_raw_values_cont_destroy(TeccRawValuesCont *cont);

/* Add value to advanced container */
bool tecc_raw_values_cont_add(TeccRawValuesCont *cont, TeccRawValue *value);

/* Get value from advanced container by index */
TeccRawValue *tecc_raw_values_cont_get(const TeccRawValuesCont *cont, int index);

/* Get value from advanced container by column name */
TeccRawValue *tecc_raw_values_cont_get_by_column(const TeccRawValuesCont *cont, const char *column_name);

/* Get number of values in advanced container */
int tecc_raw_values_cont_count(const TeccRawValuesCont *cont);

/* Check if advanced container is empty */
bool tecc_raw_values_cont_is_empty(const TeccRawValuesCont *cont);

/* Clear all values from advanced container */
void tecc_raw_values_cont_clear(TeccRawValuesCont *cont);

/* Sort advanced container by column name */
bool tecc_raw_values_cont_sort_by_column(TeccRawValuesCont *cont);

/* Merge two advanced containers */
bool tecc_raw_values_cont_merge(TeccRawValuesCont *dest, const TeccRawValuesCont *src);

/* Filter advanced container by column pattern */
TeccRawValuesCont *tecc_raw_values_cont_filter_by_column(const TeccRawValuesCont *cont, const char *column_pattern);

#endif /* RAW_VALUES_CONT_H */
/* End - ReqID:TECC4.0 */