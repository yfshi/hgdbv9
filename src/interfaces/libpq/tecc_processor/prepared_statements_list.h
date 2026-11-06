/* Begin - ReqID:TECC4.0 */
/*
 * File: prepared_statements_list.h
 *
 * Abstract:
 *   TECC prepared statements list for managing multiple named prepared statements.
 *   Handles collection management, lookup, merging, and lifecycle management.
 *
 * Authored by <wangxp@highgo.com>, 20260410.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/prepared_statements_list.h
 */

#ifndef TECC_PREPARED_STATEMENTS_LIST_H
#define TECC_PREPARED_STATEMENTS_LIST_H

#include "tecc_processor_types.h"
#include "prepared_statement.h"

/* Include PostgreSQL headers */
#include "postgres_fe.h"
#include "../libpq-fe.h"

/*
 * Prepared statements list result codes
 */
typedef enum TeccPreparedStatementsListResult {
    TECC_PREPARED_STATEMENTS_LIST_SUCCESS = 0,
    TECC_PREPARED_STATEMENTS_LIST_ERROR,
    TECC_PREPARED_STATEMENTS_LIST_NOT_FOUND,
    TECC_PREPARED_STATEMENTS_LIST_ALREADY_EXISTS,
    TECC_PREPARED_STATEMENTS_LIST_INVALID_NAME,
    TECC_PREPARED_STATEMENTS_LIST_MEMORY_ERROR,
    TECC_PREPARED_STATEMENTS_LIST_INVALID_INPUT,
    TECC_PREPARED_STATEMENTS_LIST_EMPTY
} TeccPreparedStatementsListResult;

/*
 * Prepared statements list structure
 */
typedef struct TeccPreparedStatementsList {
    TeccPreparedStatement **statements; /* Array of prepared statement pointers */
    int num_statements;        /* Number of statements in list */
    int max_statements;        /* Maximum capacity of list */
    PGconn *conn;              /* Database connection */
    int ref_count;             /* Reference count for shared usage */
    time_t created_time;       /* Creation timestamp */
    time_t last_accessed_time; /* Last access timestamp */
    int error_code;            /* Error code if processing failed */
    char error_msg[256];       /* Error message if processing failed */
} TeccPreparedStatementsList;

/*
 * Prepared statements list iterator
 */
typedef struct TeccPreparedStatementsListIterator {
    TeccPreparedStatementsList *list; /* List being iterated */
    int current_index;         /* Current index in iteration */
    bool forward;              /* Iteration direction (true=forward, false=backward) */
} TeccPreparedStatementsListIterator;

/*
 * Public interface functions
 */

/* Create a new prepared statements list */
TeccPreparedStatementsList *tecc_prepared_statements_list_create(PGconn *conn);

/* Destroy a prepared statements list */
void tecc_prepared_statements_list_destroy(TeccPreparedStatementsList *list);

/* Add a prepared statement to the list */
TeccPreparedStatementsListResult tecc_prepared_statements_list_add(
    TeccPreparedStatementsList *list,
    TeccPreparedStatement *stmt);

/* Remove a prepared statement from the list */
TeccPreparedStatementsListResult tecc_prepared_statements_list_remove(
    TeccPreparedStatementsList *list,
    const char *stmt_name);

/* Find a prepared statement by name */
TeccPreparedStatement *tecc_prepared_statements_list_find(
    TeccPreparedStatementsList *list,
    const char *stmt_name);

/* Check if a prepared statement exists in the list */
bool tecc_prepared_statements_list_exists(
    TeccPreparedStatementsList *list,
    const char *stmt_name);

/* Get the number of statements in the list */
int tecc_prepared_statements_list_count(TeccPreparedStatementsList *list);

/* Get the number of encrypted statements in the list */
int tecc_prepared_statements_list_count_encrypted(TeccPreparedStatementsList *list);

/* Merge two prepared statements lists */
TeccPreparedStatementsListResult tecc_prepared_statements_list_merge(
    TeccPreparedStatementsList *dest_list,
    TeccPreparedStatementsList *src_list,
    bool transfer_ownership);

/* Clear all statements from the list */
TeccPreparedStatementsListResult tecc_prepared_statements_list_clear(
    TeccPreparedStatementsList *list);

/* Create an iterator for the list */
TeccPreparedStatementsListIterator *tecc_prepared_statements_list_iterator_create(
    TeccPreparedStatementsList *list,
    bool forward);

/* Get next statement from iterator */
TeccPreparedStatement *tecc_prepared_statements_list_iterator_next(
    TeccPreparedStatementsListIterator *iterator);

/* Check if iterator has more statements */
bool tecc_prepared_statements_list_iterator_has_next(
    TeccPreparedStatementsListIterator *iterator);

/* Destroy an iterator */
void tecc_prepared_statements_list_iterator_destroy(
    TeccPreparedStatementsListIterator *iterator);

/* Get list statistics */
void tecc_prepared_statements_list_get_stats(
    TeccPreparedStatementsList *list,
    int *total_statements,
    int *encrypted_statements,
    int *active_statements,
    time_t *oldest_statement_time,
    time_t *newest_statement_time);

/* Unified prepared statements list interface for external modules */
TeccPreparedStatementsList *tecc_prepared_statements_list_get_default(PGconn *conn);

TeccPreparedStatementsListResult tecc_prepared_statements_list_handle_add(
    PGconn *conn,
    const char *stmt_name,
    const char *query_string,
    TeccStatementData *statement_data);

#endif /* TECC_PREPARED_STATEMENTS_LIST_H */
/* End - ReqID:TECC4.0 */