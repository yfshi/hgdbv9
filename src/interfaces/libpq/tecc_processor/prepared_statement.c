#include "postgres_fe.h"
#include "common/fe_memutils.h"
/* Begin - ReqID:TECC4.0 */
/*
 * File: prepared_statement.c
 *
 * Abstract:
 *   TECC prepared statement management for PREPARE/EXECUTE statement support.
 *   Handles prepared statement lifecycle, parameter caching, and state maintenance.
 *
 * Authored by <wangxp@highgo.com>, 20260412.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/prepared_statement.c
 */

#include "prepared_statement.h"
#include "internal/tecc_processor_internal.h"
#include "../tecc_libpq_integration/fe-tecc-integration.h"
#include "../tecc_cache/tecc_metadata_cache.h"
#include "../frontend_parser/tecc_nodes.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* Internal helper functions */
static TeccPreparedStatement *create_prepared_statement(const char *stmt_name, const char *query_string);
static void free_prepared_statement(TeccPreparedStatement *stmt);
static TeccPreparedStatementParam *create_param_info(int param_index, Oid param_type);
static void free_param_info(TeccPreparedStatementParam *param);
static TeccPreparedStatementResult analyze_query_for_params(TeccPreparedStatement *stmt, TeccStatementData *statement_data);
static TeccPreparedStatementResult process_param_values(TeccPreparedStatement *stmt, TeccStatementData *statement_data);

/* ReqID:TECC4.0 - Parse-tree-based parameter analysis helpers */
static Node *extract_inner_query(TeccStatementData *statement_data);
static int count_params_recursive(Node *node, bool *param_seen, int max_capacity);
static TeccPreparedStatementResult analyze_insert_params(InsertStmt *insert_stmt,
    TeccPreparedStatement *stmt, TeccStatementData *statement_data);
static TeccPreparedStatementResult analyze_update_params(UpdateStmt *update_stmt,
    TeccPreparedStatement *stmt, TeccStatementData *statement_data);
static bool set_param_column_info(TeccPreparedStatement *stmt, int param_index,
    const char *table_name, const char *column_name, TeccStatementData *statement_data);
static void map_select_params_in_expr(Node *expr, TeccPreparedStatement *stmt,
    const char *table_name, TeccStatementData *statement_data);
/* ReqID:TECC4.0 - 参数映射里把 CTE 名 / 派生表别名解析回基表 */
static const char *first_from_relname(List *from_clause);
static const char *resolve_param_table(const TeccPreparedStatement *stmt,
    const char *table_name);

/* Global prepared statements list has been moved to TeccState structure for thread safety */

/* Initialize prepared statement context */
TeccPreparedStatementContext *
tecc_prepared_statement_init(PGconn *conn)
{
    TeccPreparedStatementContext *context;

    if (conn == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid connection parameter to prepared statement init");
        return NULL;
    }

    /* Allocate context */
    context = (TeccPreparedStatementContext *)pg_malloc(sizeof(TeccPreparedStatementContext));
    if (context == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to allocate prepared statement context");
        return NULL;
    }

    /* Initialize context */
    context->conn = conn;
    context->stmt = NULL;
    context->error_code = TECC_PREPARED_STATEMENT_SUCCESS;
    context->error_msg[0] = '\0';

    

    return context;
}

/* Create a new prepared statement */
TeccPreparedStatementResult
tecc_prepared_statement_create(
    TeccPreparedStatementContext *context,
    const char *stmt_name,
    const char *query_string,
    TeccStatementData *statement_data)
{
    TeccPreparedStatement *stmt;
    TeccPreparedStatementResult result;
    TeccState *state;

    if (context == NULL || stmt_name == NULL || query_string == NULL) {
        return TECC_PREPARED_STATEMENT_INVALID_INPUT;
    }

    /* Get TECC state from connection */
    state = tecc_get_state(context->conn);
    if (state == NULL) {
        context->error_code = TECC_PREPARED_STATEMENT_ERROR;
        snprintf(context->error_msg, sizeof(context->error_msg),
                 "Failed to get TECC state from connection");
        return TECC_PREPARED_STATEMENT_ERROR;
    }

    /* Check if statement already exists */
    if (tecc_prepared_statement_exists(context, stmt_name)) {
        context->error_code = TECC_PREPARED_STATEMENT_ALREADY_EXISTS;
        snprintf(context->error_msg, sizeof(context->error_msg),
                 "Prepared statement '%s' already exists", stmt_name);
        return TECC_PREPARED_STATEMENT_ALREADY_EXISTS;
    }

    /* Create new prepared statement */
    stmt = create_prepared_statement(stmt_name, query_string);
    if (stmt == NULL) {
        context->error_code = TECC_PREPARED_STATEMENT_MEMORY_ERROR;
        snprintf(context->error_msg, sizeof(context->error_msg),
                 "Failed to create prepared statement '%s'", stmt_name);
        return TECC_PREPARED_STATEMENT_MEMORY_ERROR;
    }

    /* Analyze query for parameters */
    result = analyze_query_for_params(stmt, statement_data);
    if (result != TECC_PREPARED_STATEMENT_SUCCESS) {
        free_prepared_statement(stmt);
        context->error_code = result;
        snprintf(context->error_msg, sizeof(context->error_msg),
                 "Failed to analyze parameters for statement '%s'", stmt_name);
        return result;
    }

    /* Add to connection-specific list */
    stmt->next = state->prepared_statements_list;
    state->prepared_statements_list = stmt;
    state->prepared_statements_count++;

    context->stmt = stmt;
    stmt->is_prepared = true;
    stmt->created_time = time(NULL);
    stmt->last_used_time = stmt->created_time;

    

    return TECC_PREPARED_STATEMENT_SUCCESS;
}

/* Execute a prepared statement */
TeccPreparedStatementResult
tecc_prepared_statement_execute(
    TeccPreparedStatementContext *context,
    const char *stmt_name,
    int nParams,
    const Oid *paramTypes,
    const char * const * paramValues,
    const int *paramLengths,
    const int *paramFormats,
    TeccStatementData *statement_data)
{
    TeccPreparedStatement *stmt;
    int i;

    if (context == NULL || stmt_name == NULL) {
        return TECC_PREPARED_STATEMENT_INVALID_INPUT;
    }

    /* Get prepared statement */
    stmt = tecc_prepared_statement_get(context, stmt_name);
    if (stmt == NULL) {
        context->error_code = TECC_PREPARED_STATEMENT_NOT_FOUND;
        snprintf(context->error_msg, sizeof(context->error_msg),
                 "Prepared statement '%s' not found", stmt_name);
        return TECC_PREPARED_STATEMENT_NOT_FOUND;
    }

    /* Validate parameter count — only when the parser populated nParams.
     * The frontend parser currently sets nparams=0 and paramValues=NULL;
     * actual values are extracted from SQL text in the EXECUTE handler. */
    if (nParams > 0 && nParams != stmt->num_params) {
        context->error_code = TECC_PREPARED_STATEMENT_INVALID_INPUT;
        snprintf(context->error_msg, sizeof(context->error_msg),
                 "Parameter count mismatch for statement '%s': expected %d, got %d",
                 stmt_name, stmt->num_params, nParams);
        return TECC_PREPARED_STATEMENT_INVALID_INPUT;
    }

    /* Update parameter types if provided */
    if (paramTypes != NULL) {
        for (i = 0; i < nParams; i++) {
            if (i < stmt->num_params && stmt->params[i] != NULL) {
                stmt->params[i]->param_type = paramTypes[i];
            }
        }
    }

    /* Process parameter values */
    if (paramValues != NULL && statement_data != NULL) {
        TeccPreparedStatementResult process_result;

        /* Store parameter values in statement data for processing */
        statement_data->nParams = nParams;
        statement_data->paramTypes = paramTypes;
        /* Note: paramValues is const char *const *, we need to duplicate if we want to modify */
        /* For now, just store the pointer - processor should not modify paramValues directly */
        statement_data->paramValues = paramValues;
        statement_data->paramLengths = paramLengths;
        statement_data->paramFormats = paramFormats;

        /* Process parameter values (encryption/decryption) */
        process_result = process_param_values(stmt, statement_data);
        if (process_result != TECC_PREPARED_STATEMENT_SUCCESS) {
            context->error_code = process_result;
            snprintf(context->error_msg, sizeof(context->error_msg),
                     "Failed to process parameters for statement '%s'", stmt_name);
            return process_result;
        }
    }

    /* Update usage timestamp */
    stmt->last_used_time = time(NULL);
    stmt->is_executing = true;
    context->stmt = stmt;

    

    return TECC_PREPARED_STATEMENT_SUCCESS;
}

/* Deallocate a prepared statement */
TeccPreparedStatementResult
tecc_prepared_statement_deallocate(
    TeccPreparedStatementContext *context,
    const char *stmt_name)
{
    TeccPreparedStatement *stmt, *prev, *next;
    TeccState *state;

    if (context == NULL || stmt_name == NULL) {
        return TECC_PREPARED_STATEMENT_INVALID_INPUT;
    }

    /* Get TECC state from connection */
    state = tecc_get_state(context->conn);
    if (state == NULL) {
        context->error_code = TECC_PREPARED_STATEMENT_ERROR;
        snprintf(context->error_msg, sizeof(context->error_msg),
                 "Failed to get TECC state from connection");
        return TECC_PREPARED_STATEMENT_ERROR;
    }

    /* Find and remove from connection-specific list */
    prev = NULL;
    stmt = state->prepared_statements_list;
    while (stmt != NULL) {
        next = stmt->next;
        if (strcmp(stmt->stmt_name, stmt_name) == 0) {
            /* Remove from list */
            if (prev == NULL) {
                state->prepared_statements_list = next;
            } else {
                prev->next = next;
            }

            /* Free statement */
            free_prepared_statement(stmt);
            state->prepared_statements_count--;

            

            return TECC_PREPARED_STATEMENT_SUCCESS;
        }
        prev = stmt;
        stmt = next;
    }

    context->error_code = TECC_PREPARED_STATEMENT_NOT_FOUND;
    snprintf(context->error_msg, sizeof(context->error_msg),
             "Prepared statement '%s' not found for deallocation", stmt_name);
    return TECC_PREPARED_STATEMENT_NOT_FOUND;
}

/* Get prepared statement by name */
TeccPreparedStatement *
tecc_prepared_statement_get(
    TeccPreparedStatementContext *context,
    const char *stmt_name)
{
    TeccPreparedStatement *stmt;
    TeccState *state;

    if (context == NULL || stmt_name == NULL) {
        return NULL;
    }

    /* Get TECC state from connection */
    state = tecc_get_state(context->conn);
    if (state == NULL) {
        return NULL;
    }

    /* Search in connection-specific list */
    stmt = state->prepared_statements_list;
    while (stmt != NULL) {
        if (strcmp(stmt->stmt_name, stmt_name) == 0) {
            return stmt;
        }
        stmt = stmt->next;
    }

    return NULL;
}

/* Check if prepared statement exists */
bool
tecc_prepared_statement_exists(
    TeccPreparedStatementContext *context,
    const char *stmt_name)
{
    return (tecc_prepared_statement_get(context, stmt_name) != NULL);
}

/* Analyze prepared statement parameters */
TeccPreparedStatementResult
tecc_prepared_statement_analyze_params(
    TeccPreparedStatementContext *context,
    TeccStatementData *statement_data)
{
    if (context == NULL || context->stmt == NULL || statement_data == NULL) {
        return TECC_PREPARED_STATEMENT_INVALID_INPUT;
    }

    return analyze_query_for_params(context->stmt, statement_data);
}

/* Process prepared statement parameters (encryption/decryption) */
TeccPreparedStatementResult
tecc_prepared_statement_process_params(
    TeccPreparedStatementContext *context,
    TeccStatementData *statement_data)
{
    if (context == NULL || context->stmt == NULL || statement_data == NULL) {
        return TECC_PREPARED_STATEMENT_INVALID_INPUT;
    }

    return process_param_values(context->stmt, statement_data);
}

/* Clean up prepared statement context */
void
tecc_prepared_statement_cleanup(TeccPreparedStatementContext *context)
{
    if (context == NULL) {
        return;
    }

    /* Note: We don't free prepared statements here, they remain in global list */
    pg_free(context);
}

/* Unified prepared statement interface for external modules */
TeccPreparedStatementResult
tecc_prepared_statement_handle_prepare(
    PGconn *conn,
    const char *stmt_name,
    const char *query_string,
    TeccStatementData *statement_data)
{
    TeccPreparedStatementContext *context;
    TeccPreparedStatementResult result;

    if (conn == NULL || stmt_name == NULL || query_string == NULL) {
        return TECC_PREPARED_STATEMENT_INVALID_INPUT;
    }

    context = tecc_prepared_statement_init(conn);
    if (context == NULL) {
        return TECC_PREPARED_STATEMENT_MEMORY_ERROR;
    }

    result = tecc_prepared_statement_create(context, stmt_name, query_string, statement_data);
    tecc_prepared_statement_cleanup(context);

    return result;
}

TeccPreparedStatementResult
tecc_prepared_statement_handle_execute(
    PGconn *conn,
    const char *stmt_name,
    int nParams,
    const Oid *paramTypes,
    const char * const * paramValues,
    const int *paramLengths,
    const int *paramFormats,
    TeccStatementData *statement_data)
{
    TeccPreparedStatementContext *context;
    TeccPreparedStatementResult result;

    if (conn == NULL || stmt_name == NULL) {
        return TECC_PREPARED_STATEMENT_INVALID_INPUT;
    }

    context = tecc_prepared_statement_init(conn);
    if (context == NULL) {
        return TECC_PREPARED_STATEMENT_MEMORY_ERROR;
    }

    result = tecc_prepared_statement_execute(context, stmt_name, nParams,
                                           paramTypes, paramValues,
                                           paramLengths, paramFormats,
                                           statement_data);
    tecc_prepared_statement_cleanup(context);

    return result;
}

TeccPreparedStatementResult
tecc_prepared_statement_handle_deallocate(
    PGconn *conn,
    const char *stmt_name,
    TeccStatementData *statement_data)
{
    TeccPreparedStatementContext *context;
    TeccState *state;
    TeccPreparedStatementResult result;
    TeccPreparedStatement *stmt, *next;

    if (conn == NULL) {
        return TECC_PREPARED_STATEMENT_INVALID_INPUT;
    }

    /* DEALLOCATE ALL: name is NULL */
    if (stmt_name == NULL) {
        state = tecc_get_state(conn);
        if (state == NULL) {
            return TECC_PREPARED_STATEMENT_ERROR;
        }

        stmt = state->prepared_statements_list;
        while (stmt != NULL) {
            next = stmt->next;
            free_prepared_statement(stmt);
            stmt = next;
        }
        state->prepared_statements_list = NULL;
        state->prepared_statements_count = 0;

        
        return TECC_PREPARED_STATEMENT_SUCCESS;
    }

    context = tecc_prepared_statement_init(conn);
    if (context == NULL) {
        return TECC_PREPARED_STATEMENT_MEMORY_ERROR;
    }

    result = tecc_prepared_statement_deallocate(context, stmt_name);
    tecc_prepared_statement_cleanup(context);

    return result;
}

/* ==================== Internal Helper Functions ==================== */

/* Create a new prepared statement */
static TeccPreparedStatement *
create_prepared_statement(const char *stmt_name, const char *query_string)
{
    TeccPreparedStatement *stmt;

    stmt = (TeccPreparedStatement *)pg_malloc(sizeof(TeccPreparedStatement));
    if (stmt == NULL) {
        return NULL;
    }

    /* Initialize fields */
    stmt->stmt_name = pg_malloc(strlen(stmt_name) + 1);
    if (stmt->stmt_name == NULL) {
        pg_free(stmt);
        return NULL;
    }
    strcpy(stmt->stmt_name, stmt_name);

    stmt->query_string = pg_malloc(strlen(query_string) + 1);
    if (stmt->query_string == NULL) {
        pg_free(stmt->stmt_name);
        pg_free(stmt);
        return NULL;
    }
    strcpy(stmt->query_string, query_string);

    stmt->parse_tree = NULL;
    stmt->params = NULL;
    stmt->num_params = 0;
    stmt->num_encrypted_params = 0;
    stmt->is_prepared = false;
    stmt->is_executing = false;
    stmt->ref_count = 1;
    stmt->created_time = 0;
    stmt->last_used_time = 0;
    stmt->error_code = TECC_PREPARED_STATEMENT_SUCCESS;
    stmt->error_msg[0] = '\0';
    stmt->alias_names = NIL;
    stmt->alias_tables = NIL;
    stmt->next = NULL;

    return stmt;
}

/* Free a prepared statement */
static void
free_prepared_statement(TeccPreparedStatement *stmt)
{
    int i;

    if (stmt == NULL) {
        return;
    }

    /* Free parameter information */
    if (stmt->params != NULL) {
        for (i = 0; i < stmt->num_params; i++) {
            if (stmt->params[i] != NULL) {
                free_param_info(stmt->params[i]);
            }
        }
        pg_free(stmt->params);
    }

    /* ReqID:TECC4.0 - 释放分析期间的别名映射（元素是 AST 里的名字，不归本模块所有） */
    if (stmt->alias_names != NULL) {
        list_free(stmt->alias_names);
        stmt->alias_names = NIL;
    }
    if (stmt->alias_tables != NULL) {
        list_free(stmt->alias_tables);
        stmt->alias_tables = NIL;
    }

    /* Free strings */
    if (stmt->stmt_name != NULL) {
        pg_free(stmt->stmt_name);
    }
    if (stmt->query_string != NULL) {
        pg_free(stmt->query_string);
    }

    /* Note: parse_tree is not owned by prepared statement, don't free it */

    pg_free(stmt);
}

/* Create parameter information */
static TeccPreparedStatementParam *
create_param_info(int param_index, Oid param_type)
{
    TeccPreparedStatementParam *param;

    param = (TeccPreparedStatementParam *)pg_malloc(sizeof(TeccPreparedStatementParam));
    if (param == NULL) {
        return NULL;
    }

    param->param_index = param_index;
    param->param_type = param_type;
    param->param_name = NULL;
    param->is_encrypted = false;
    param->table_name = NULL;
    param->column_name = NULL;
    param->column_type = NULL;
    param->needs_encryption = false;
    param->needs_decryption = false;

    return param;
}

/* Free parameter information */
static void
free_param_info(TeccPreparedStatementParam *param)
{
    if (param == NULL) {
        return;
    }

    if (param->param_name != NULL) {
        pg_free(param->param_name);
    }
    if (param->table_name != NULL) {
        pg_free(param->table_name);
    }
    if (param->column_name != NULL) {
        pg_free(param->column_name);
    }
    if (param->column_type != NULL) {
        pg_free(param->column_type);
    }

    pg_free(param);
}

/*
 * Analyze query for parameters — parse-tree-based traversal.
 *
 * Replaces the old string-scanning approach which could misidentify "$N"
 * inside string literals. Uses the parsed PREPARE statement tree to:
 *   1. Walk the inner PreparableStmt and find all Param ($N) nodes.
 *   2. Map each parameter to its target column (INSERT by position,
 *      UPDATE by SET target name).
 *   3. Check the metadata cache to determine if the column is encrypted.
 *
 * Returns TECC_PREPARED_STATEMENT_PARSE_ERROR if no parse tree is available.
 */
static TeccPreparedStatementResult
analyze_query_for_params(TeccPreparedStatement *stmt, TeccStatementData *statement_data)
{
    Node *inner_query;
    TeccPreparedStatementResult result;

    if (stmt == NULL)
        return TECC_PREPARED_STATEMENT_INVALID_INPUT;

    inner_query = extract_inner_query(statement_data);

    if (inner_query == NULL)
    {
        /* ReqID:TECC4.0 - Parse tree must be available for prepared statement analysis.
         * String-scanning fallback removed: it cannot reliably identify parameters
         * (e.g. "$N" inside string literals would be misidentified). */
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Parse tree unavailable for prepared statement analysis");
        return TECC_PREPARED_STATEMENT_PARSE_ERROR;
    }

    /* Parse-tree-based analysis, dispatched by statement type */
    switch (nodeTag(inner_query))
    {
        case FE_T_InsertStmt:
            result = analyze_insert_params((InsertStmt *)inner_query, stmt, statement_data);
            break;
        case FE_T_UpdateStmt:
            result = analyze_update_params((UpdateStmt *)inner_query, stmt, statement_data);
            break;
        case FE_T_DeleteStmt:
        {
            /* DELETE: extract table name and map WHERE clause params */
            DeleteStmt *del;
            const char *table_name;
            bool *seen;
            int max_param;
            int max_params;
            int pi;
            int encrypted_count;
            int epi;

            del = (DeleteStmt *)inner_query;
            max_params = 16;
            seen = (bool *)pg_malloc(max_params * sizeof(bool));
            if (seen == NULL)
                return TECC_PREPARED_STATEMENT_MEMORY_ERROR;
            memset(seen, 0, max_params * sizeof(bool));

            max_param = count_params_recursive(
                (Node *)del->whereClause, seen, max_params);
            pg_free(seen);

            stmt->num_params = max_param;
            stmt->num_encrypted_params = 0;
            if (max_param > 0)
            {
                stmt->params = (TeccPreparedStatementParam **)pg_malloc(
                    max_param * sizeof(TeccPreparedStatementParam *));
                if (stmt->params == NULL)
                    return TECC_PREPARED_STATEMENT_MEMORY_ERROR;
                for (pi = 0; pi < max_param; pi++)
                {
                    stmt->params[pi] = create_param_info(pi + 1, 0);
                    if (stmt->params[pi] == NULL)
                    {
                        int j;
                        for (j = 0; j < pi; j++)
                            free_param_info(stmt->params[j]);
                        pg_free(stmt->params);
                        stmt->params = NULL;
                        return TECC_PREPARED_STATEMENT_MEMORY_ERROR;
                    }
                }

                /* Extract table name from DELETE relation */
                table_name = NULL;
                if (del->relation != NULL)
                    table_name = del->relation->relname;

                if (table_name != NULL && del->whereClause != NULL)
                {
                    map_select_params_in_expr(del->whereClause, stmt,
                                              table_name, statement_data);

                    /* Count encrypted params */
                    encrypted_count = 0;
                    for (epi = 0; epi < max_param; epi++)
                    {
                        if (stmt->params[epi] != NULL &&
                            stmt->params[epi]->needs_encryption)
                            encrypted_count++;
                    }
                    stmt->num_encrypted_params = encrypted_count;
                }
            }
            result = TECC_PREPARED_STATEMENT_SUCCESS;
            break;
        }
        case FE_T_SelectStmt:
        {
            /* SELECT: walk WHERE / targetList for Param nodes with column mapping */
            SelectStmt *sel = (SelectStmt *)inner_query;
            bool *seen;
            int max_param;
            int max_params;
            int pi;
            int param_where;
            int param_targets;
            const char *table_name;
            int epi;
            int encrypted_count;

            max_params = 16;
            seen = (bool *)pg_malloc(max_params * sizeof(bool));
            if (seen == NULL)
                return TECC_PREPARED_STATEMENT_MEMORY_ERROR;

            memset(seen, 0, max_params * sizeof(bool));
            param_where = count_params_recursive((Node *)sel->whereClause, seen, max_params);
            param_targets = count_params_recursive((Node *)sel->targetList, seen, max_params);
            max_param = (param_where > param_targets) ? param_where : param_targets;
            /* Also check valuesLists for Param nodes in SELECT context */
            {
                int param_values;
                param_values = count_params_recursive((Node *)sel->valuesLists, seen, max_params);
                if (param_values > max_param)
                    max_param = param_values;
            }
            pg_free(seen);

            stmt->num_params = max_param;
            stmt->num_encrypted_params = 0;
            if (max_param > 0)
            {
                stmt->params = (TeccPreparedStatementParam **)pg_malloc(
                    max_param * sizeof(TeccPreparedStatementParam *));
                if (stmt->params == NULL)
                    return TECC_PREPARED_STATEMENT_MEMORY_ERROR;
                for (pi = 0; pi < max_param; pi++)
                {
                    stmt->params[pi] = create_param_info(pi + 1, 0);
                    if (stmt->params[pi] == NULL)
                    {
                        int j;
                        for (j = 0; j < pi; j++)
                            free_param_info(stmt->params[j]);
                        pg_free(stmt->params);
                        stmt->params = NULL;
                        return TECC_PREPARED_STATEMENT_MEMORY_ERROR;
                    }
                }

                /* Extract table name from FROM clause (first RangeVar) */
                table_name = NULL;
                
                if (sel->fromClause != NULL && sel->fromClause->length > 0)
                {
                    Node *first_from;
                    first_from = (Node *)linitial(sel->fromClause);
                    
                    if (first_from != NULL && nodeTag(first_from) == FE_T_RangeVar)
                    {
                        table_name = ((RangeVar *)first_from)->relname;
                        
                    }
                }

                if (table_name != NULL || sel->cteList != NIL ||
                    sel->fromClause != NIL)
                {
                    /* Begin - ReqID:TECC4.0 - 建立「限定名 → 基表」映射 */
                    {
                        ListCell *alc;

                        /* CTE：名字 → 其定义查询的基表（链式由解析循环兜底） */
                        foreach(alc, sel->cteList)
                        {
                            CommonTableExpr *cte = (CommonTableExpr *)lfirst(alc);
                            const char *base = NULL;

                            if (cte == NULL || cte->ctename == NULL ||
                                cte->ctequery == NULL)
                                continue;
                            if (nodeTag(cte->ctequery) == FE_T_SelectStmt)
                                base = first_from_relname(
                                    ((SelectStmt *)cte->ctequery)->fromClause);
                            if (base != NULL)
                            {
                                stmt->alias_names = lappend(stmt->alias_names,
                                                            (void *)cte->ctename);
                                stmt->alias_tables = lappend(stmt->alias_tables,
                                                             (void *)base);
                            }
                        }

                        /* FROM 项：别名 → 基表（派生表取其内部的基表） */
                        foreach(alc, sel->fromClause)
                        {
                            Node *fn = (Node *)lfirst(alc);

                            if (fn == NULL)
                                continue;
                            if (nodeTag(fn) == FE_T_RangeVar)
                            {
                                RangeVar *rv = (RangeVar *)fn;
                                const char *nm;

                                if (rv->relname == NULL)
                                    continue;
                                nm = (rv->alias != NULL && rv->alias->aliasname != NULL)
                                     ? rv->alias->aliasname : rv->relname;
                                stmt->alias_names = lappend(stmt->alias_names,
                                                            (void *)nm);
                                stmt->alias_tables = lappend(stmt->alias_tables,
                                                             (void *)rv->relname);
                            }
                            else if (nodeTag(fn) == FE_T_RangeSubselect)
                            {
                                RangeSubselect *rs = (RangeSubselect *)fn;
                                const char *nm;
                                const char *base = NULL;

                                if (rs->alias == NULL || rs->alias->aliasname == NULL)
                                    continue;
                                nm = rs->alias->aliasname;
                                if (rs->subquery != NULL &&
                                    nodeTag(rs->subquery) == FE_T_SelectStmt)
                                    base = first_from_relname(
                                        ((SelectStmt *)rs->subquery)->fromClause);
                                if (base != NULL)
                                {
                                    stmt->alias_names = lappend(stmt->alias_names,
                                                                (void *)nm);
                                    stmt->alias_tables = lappend(stmt->alias_tables,
                                                                 (void *)base);
                                }
                            }
                        }
                    }
                    /* End - ReqID:TECC4.0 */

                    /* ReqID:TECC4.0 - FROM 是派生表/CTE 时上面取不到基表名，
                     * 用映射里的第一个基表兜底，让不带限定符的列也能命中 */
                    if (table_name == NULL && stmt->alias_tables != NIL)
                        table_name = (const char *)list_nth(stmt->alias_tables, 0);

                    /* Map Param nodes to encrypted columns via A_Expr walk */
                    map_select_params_in_expr((Node *)sel->whereClause, stmt,
                        table_name, statement_data);
                    map_select_params_in_expr((Node *)sel->targetList, stmt,
                        table_name, statement_data);
                    if (sel->valuesLists != NULL)
                    {
                        ListCell *vlc;
                        for (vlc = sel->valuesLists->head; vlc != NULL; vlc = vlc->next)
                            map_select_params_in_expr((Node *)vlc->data.ptr_value, stmt,
                                table_name, statement_data);
                    }

                    /* Count how many params were marked encrypted */
                    encrypted_count = 0;
                    for (epi = 0; epi < max_param; epi++)
                    {
                        if (stmt->params[epi] != NULL &&
                            stmt->params[epi]->needs_encryption)
                            encrypted_count++;
                    }
                    stmt->num_encrypted_params = encrypted_count;

                    
                }
            }
            result = TECC_PREPARED_STATEMENT_SUCCESS;
            break;
        }
        default:
            stmt->num_params = 0;
            stmt->num_encrypted_params = 0;
            result = TECC_PREPARED_STATEMENT_SUCCESS;
            break;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Analyzed query (parse tree): found %d parameters, %d encrypted",
                       stmt->num_params, stmt->num_encrypted_params);

    return result;
}

/*
 * Extract the inner PreparableStmt from the parse tree.
 * The statement_data->parse_tree is a List containing the PrepareStmt.
 * We need to reach inside and get the PreparableStmt (INSERT/UPDATE/DELETE/SELECT).
 */
static Node *
extract_inner_query(TeccStatementData *statement_data)
{
    if (statement_data == NULL)
        return NULL;

    if (statement_data->parse_tree == NULL)
        return NULL;

    if (nodeTag(statement_data->parse_tree) == FE_T_List)
    {
        List *stmt_list;
        ListCell *lc;

        stmt_list = (List *)statement_data->parse_tree;
        foreach(lc, stmt_list)
        {
            Node *node;
            PrepareStmt *prep;

            node = (Node *)lfirst(lc);
            if (node != NULL && nodeTag(node) == FE_T_PrepareStmt)
            {
                prep = (PrepareStmt *)node;
                if (prep->query != NULL)
                    return prep->query;
                return NULL;
            }
        }
        return NULL;
    }

    /* Single statement directly (not a list) */
    if (nodeTag(statement_data->parse_tree) == FE_T_PrepareStmt)
    {
        PrepareStmt *prep;

        prep = (PrepareStmt *)statement_data->parse_tree;
        if (prep->query != NULL)
            return prep->query;
        return NULL;
    }

    return NULL;
}

/*
 * Recursively count unique Param nodes in a node tree.
 * Uses a boolean array to track which parameter numbers have been seen,
 * so $1 appearing multiple times only counts once.
 * Returns the maximum paramid found (0 = no params).
 */
static int
count_params_recursive(Node *node, bool *param_seen, int max_capacity)
{
    int max_param;
    List *sub_list;
    int list_result;

    if (node == NULL)
        return 0;

    max_param = 0;

    /* Param node: record its id */
    if (nodeTag(node) == FE_T_Param)
    {
        Param *param;
        int pid;

        param = (Param *)node;
        pid = param->paramid;
        if (pid > 0)
        {
            /* Expand seen array if needed */
            if (pid >= max_capacity)
            {
                /* Don't realloc here — caller pre-allocates enough space.
                 * Just ignore params beyond capacity (shouldn't happen). */
                return pid;
            }
            if (!param_seen[pid])
            {
                param_seen[pid] = true;
                return pid;
            }
            return pid;
        }
        return 0;
    }

    /* List: iterate and recurse */
    if (nodeTag(node) == FE_T_List)
    {
        ListCell *lc;

        sub_list = (List *)node;
        max_param = 0;
        foreach(lc, sub_list)
        {
            int child_max;

            child_max = count_params_recursive((Node *)lfirst(lc), param_seen, max_capacity);
            if (child_max > max_param)
                max_param = child_max;
        }
        return max_param;
    }

    /* Recurse into known node types that contain sub-nodes */
    switch (nodeTag(node))
    {
        case FE_T_A_Expr:
        {
            A_Expr *a;

            a = (A_Expr *)node;
            max_param = count_params_recursive(a->lexpr, param_seen, max_capacity);
            list_result = count_params_recursive(a->rexpr, param_seen, max_capacity);
            if (list_result > max_param)
                max_param = list_result;
            break;
        }

        case FE_T_TypeCast:
        {
            TypeCast *tc;

            tc = (TypeCast *)node;
            max_param = count_params_recursive(tc->arg, param_seen, max_capacity);
            break;
        }

        case FE_T_SelectStmt:
        {
            SelectStmt *sel;
            int result1, result2, result3;

            sel = (SelectStmt *)node;
            max_param = count_params_recursive((Node *)sel->valuesLists, param_seen, max_capacity);
            result1 = count_params_recursive((Node *)sel->targetList, param_seen, max_capacity);
            if (result1 > max_param) max_param = result1;
            result2 = count_params_recursive(sel->whereClause, param_seen, max_capacity);
            if (result2 > max_param) max_param = result2;
            result3 = count_params_recursive((Node *)sel->fromClause, param_seen, max_capacity);
            if (result3 > max_param) max_param = result3;
            break;
        }

        case FE_T_InsertStmt:
        {
            InsertStmt *ins;

            ins = (InsertStmt *)node;
            max_param = count_params_recursive(ins->selectStmt, param_seen, max_capacity);
            list_result = count_params_recursive((Node *)ins->withClause, param_seen, max_capacity);
            if (list_result > max_param) max_param = list_result;
            break;
        }

        case FE_T_UpdateStmt:
        {
            UpdateStmt *upd;
            int result1, result2;

            upd = (UpdateStmt *)node;
            max_param = count_params_recursive((Node *)upd->targetList, param_seen, max_capacity);
            result1 = count_params_recursive(upd->whereClause, param_seen, max_capacity);
            if (result1 > max_param) max_param = result1;
            result2 = count_params_recursive((Node *)upd->fromClause, param_seen, max_capacity);
            if (result2 > max_param) max_param = result2;
            break;
        }

        case FE_T_DeleteStmt:
        {
            DeleteStmt *del;

            del = (DeleteStmt *)node;
            max_param = count_params_recursive(del->whereClause, param_seen, max_capacity);
            list_result = count_params_recursive((Node *)del->usingClause, param_seen, max_capacity);
            if (list_result > max_param) max_param = list_result;
            break;
        }

        case FE_T_BoolExpr:
        {
            BoolExpr *bexpr;
            ListCell *lc;

            bexpr = (BoolExpr *)node;
            foreach(lc, bexpr->args)
            {
                int child_max;
                child_max = count_params_recursive((Node *)lfirst(lc),
                    param_seen, max_capacity);
                if (child_max > max_param)
                    max_param = child_max;
            }
            break;
        }

        case FE_T_ResTarget:
        {
            ResTarget *rt;

            rt = (ResTarget *)node;
            max_param = count_params_recursive(rt->val, param_seen, max_capacity);
            break;
        }

        default:
            /* Other node types: don't recurse blindly, but handle common wrappers */
            break;
    }

    return max_param;
}

/*
 * Analyze INSERT parameters — traverse VALUES list, map Param nodes by
 * position to table columns, and check encryption status.
 */
static TeccPreparedStatementResult
analyze_insert_params(InsertStmt *insert_stmt,
                      TeccPreparedStatement *stmt,
                      TeccStatementData *statement_data)
{
    SelectStmt *sel;
    List *values_lists;
    int num_rows_to_check;
    ListCell *row_lc;
    int row_idx;
    bool *param_seen;
    int max_params;
    int max_param;
    int pi;
    const char *table_name;
    List *col_list;
    int col_count;

    

    if (insert_stmt == NULL || insert_stmt->selectStmt == NULL)
    {
        
        stmt->num_params = 0;
        stmt->num_encrypted_params = 0;
        return TECC_PREPARED_STATEMENT_SUCCESS;
    }

    if (nodeTag(insert_stmt->selectStmt) != FE_T_SelectStmt)
    {
        stmt->num_params = 0;
        stmt->num_encrypted_params = 0;
        return TECC_PREPARED_STATEMENT_SUCCESS;
    }

    sel = (SelectStmt *)insert_stmt->selectStmt;
    values_lists = sel->valuesLists;

    if (values_lists == NIL || values_lists == NULL)
    {
        /* INSERT...SELECT or similar, no VALUES to analyze */
        stmt->num_params = 0;
        stmt->num_encrypted_params = 0;
        return TECC_PREPARED_STATEMENT_SUCCESS;
    }

    table_name = NULL;
    if (insert_stmt->relation != NULL)
        table_name = insert_stmt->relation->relname;

    col_list = insert_stmt->cols;
    col_count = (col_list != NIL && col_list != NULL) ? list_length(col_list) : 0;

    /* Single pass: iterate VALUES, find Param nodes, map to columns.
     * We iterate directly using foreach() because inner List rows may not
     * have FE_T_List tag set, so count_params_recursive cannot detect them. */

    max_param = 0;
    max_params = 64;
    param_seen = (bool *)pg_malloc(max_params * sizeof(bool));
    if (param_seen == NULL)
        return TECC_PREPARED_STATEMENT_MEMORY_ERROR;
    memset(param_seen, 0, max_params * sizeof(bool));

    num_rows_to_check = list_length(values_lists);
    row_idx = 0;
    foreach(row_lc, values_lists)
    {
        List *row;
        ListCell *val_lc;
        int col_idx;
        int row_count;
        int cell_idx;

        

        if (row_idx >= num_rows_to_check)
            break;

        row = (List *)lfirst(row_lc);
        
        if (row == NIL || row == NULL)
        {
            row_idx++;
            continue;
        }

        col_idx = 0;
        row_count = list_length(row);
        cell_idx = 0;
        foreach(val_lc, row)
        {
            Node *val_node;

            if (cell_idx >= row_count)
                break;

            val_node = (Node *)lfirst(val_lc);

            if (val_node != NULL && nodeTag(val_node) == FE_T_Param)
            {
                Param *param;
                int pid;

                param = (Param *)val_node;
                pid = param->paramid;

                if (pid > 0)
                {
                    if (!param_seen[pid])
                    {
                        param_seen[pid] = true;
                        if (pid > max_param)
                            max_param = pid;
                    }
                }
            }
            col_idx++;
            cell_idx++;
        }
        row_idx++;
    }

    

    stmt->num_params = max_param;
    stmt->num_encrypted_params = 0;

    if (max_param > 0)
    {
        stmt->params = (TeccPreparedStatementParam **)pg_malloc(
            max_param * sizeof(TeccPreparedStatementParam *));
        if (stmt->params == NULL)
        {
            pg_free(param_seen);
            return TECC_PREPARED_STATEMENT_MEMORY_ERROR;
        }

        for (pi = 0; pi < max_param; pi++)
        {
            stmt->params[pi] = create_param_info(pi + 1, 0);
            if (stmt->params[pi] == NULL)
            {
                int j;
                for (j = 0; j < pi; j++)
                    free_param_info(stmt->params[j]);
                pg_free(stmt->params);
                stmt->params = NULL;
                pg_free(param_seen);
                return TECC_PREPARED_STATEMENT_MEMORY_ERROR;
            }
        }

        /* Second pass: map each Param by position to column */
        {
            int map_row_idx;
            ListCell *map_row_lc;

            
            map_row_idx = 0;
            foreach(map_row_lc, values_lists)
            {
                List *row;
                ListCell *val_lc;
                int col_idx;
                int row_count;
                int cell_idx;

                if (map_row_idx >= num_rows_to_check)
                    break;

                row = (List *)lfirst(map_row_lc);
                if (row == NIL || row == NULL)
                {
                    map_row_idx++;
                    continue;
                }

                col_idx = 0;
                row_count = list_length(row);
                cell_idx = 0;
                foreach(val_lc, row)
                {
                    Node *val_node;

                    if (cell_idx >= row_count)
                        break;

                    val_node = (Node *)lfirst(val_lc);

                    if (val_node != NULL && nodeTag(val_node) == FE_T_Param)
                    {
                        Param *param;
                        int pid;

                        param = (Param *)val_node;
                        pid = param->paramid;

                        if (pid > 0 && pid <= max_param)
                        {
                            const char *col_name;
                            int target_col_index;

                            col_name = NULL;
                            target_col_index = col_idx + 1;  /* attnum 1-based */

                            /* Map by position to column */
                            if (col_count > 0 && col_idx < col_count)
                            {
                                /* Explicit column list: get name from cols */
                                Node *col_node;

                                col_node = (Node *)list_nth(col_list, col_idx);
                                if (col_node != NULL)
                                {
                                    if (nodeTag(col_node) == FE_T_ResTarget)
                                    {
                                        ResTarget *rt;

                                        rt = (ResTarget *)col_node;
                                        if (rt->name != NULL)
                                            col_name = rt->name;
                                    }
                                    else if (nodeTag(col_node) == FE_T_String)
                                    {
                                        col_name = (char *)col_node;
                                    }
                                }
                            }

                            /* Look up column info and check encryption */
                            if (table_name != NULL && target_col_index > 0)
                            {
                                if (col_name != NULL)
                                {
                                    set_param_column_info(stmt, pid - 1, table_name,
                                                         col_name, statement_data);
                                }
                                else
                                {
                                    /* Need table metadata for position→name mapping */
                                    TeccState *state;
                                    const char *db_name;
                                    const char *schema_name;
                                    TeccColumnCacheItem **columns;
                                    int col_count_meta;
                                    int ci;

                                    state = tecc_get_state(statement_data->conn);
                                    db_name = (state != NULL) ? PQdb(statement_data->conn) : NULL;
                                    schema_name = (state != NULL)
                                        ? tecc_cache_get_guc_param(state, "current_schema") : NULL;

                                    if (state != NULL && db_name != NULL && schema_name != NULL)
                                    {
                                        columns = tecc_cache_get_columns_by_table_name(
                                            state, db_name, schema_name, table_name, &col_count_meta);

                                        if (columns != NULL && col_count_meta > 0)
                                        {
                                            for (ci = 0; ci < col_count_meta; ci++)
                                            {
                                                if (columns[ci] != NULL &&
                                                    columns[ci]->column_attnum == target_col_index)
                                                {
                                                    set_param_column_info(stmt, pid - 1,
                                                        table_name, columns[ci]->column_name,
                                                        statement_data);
                                                    break;
                                                }
                                            }
                                            pg_free(columns);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    col_idx++;
                    cell_idx++;
                }
                map_row_idx++;
            }
        }
    }

    pg_free(param_seen);

    /* Count encrypted params */
    if (stmt->params != NULL && stmt->num_params > 0)
    {
        int enc_count;
        int ei;

        enc_count = 0;
        for (ei = 0; ei < stmt->num_params; ei++)
        {
            if (stmt->params[ei] != NULL && stmt->params[ei]->needs_encryption)
                enc_count++;
        }
        stmt->num_encrypted_params = enc_count;
    }

    return TECC_PREPARED_STATEMENT_SUCCESS;
}

/*
 * Analyze UPDATE parameters — traverse SET targetList, map Param nodes
 * to the named SET target column, and check encryption status.
 */
static TeccPreparedStatementResult
analyze_update_params(UpdateStmt *update_stmt,
                      TeccPreparedStatement *stmt,
                      TeccStatementData *statement_data)
{
    const char *table_name;
    bool *param_seen;
    int max_params;
    int max_param;
    int pi;

    if (update_stmt == NULL)
    {
        stmt->num_params = 0;
        stmt->num_encrypted_params = 0;
        return TECC_PREPARED_STATEMENT_SUCCESS;
    }

    table_name = NULL;
    if (update_stmt->relation != NULL)
        table_name = update_stmt->relation->relname;

    /* First pass: find max param id by traversing targetList + whereClause */
    max_params = 64;
    param_seen = (bool *)pg_malloc(max_params * sizeof(bool));
    if (param_seen == NULL)
        return TECC_PREPARED_STATEMENT_MEMORY_ERROR;
    memset(param_seen, 0, max_params * sizeof(bool));

    max_param = count_params_recursive((Node *)update_stmt->targetList, param_seen, max_params);
    {
        int where_max;
        where_max = count_params_recursive(update_stmt->whereClause, param_seen, max_params);
        if (where_max > max_param)
            max_param = where_max;
    }

    stmt->num_params = max_param;
    stmt->num_encrypted_params = 0;

    if (max_param > 0)
    {
        stmt->params = (TeccPreparedStatementParam **)pg_malloc(
            max_param * sizeof(TeccPreparedStatementParam *));
        if (stmt->params == NULL)
        {
            pg_free(param_seen);
            return TECC_PREPARED_STATEMENT_MEMORY_ERROR;
        }

        for (pi = 0; pi < max_param; pi++)
        {
            stmt->params[pi] = create_param_info(pi + 1, 0);
            if (stmt->params[pi] == NULL)
            {
                int j;
                for (j = 0; j < pi; j++)
                    free_param_info(stmt->params[j]);
                pg_free(stmt->params);
                stmt->params = NULL;
                pg_free(param_seen);
                return TECC_PREPARED_STATEMENT_MEMORY_ERROR;
            }
        }

        /* Second pass: map SET targetList params to columns */
        if (table_name != NULL && update_stmt->targetList != NIL)
        {
            ListCell *lc;

            foreach(lc, update_stmt->targetList)
            {
                Node *target_node;
                ResTarget *rt;

                target_node = (Node *)lfirst(lc);
                if (target_node == NULL || nodeTag(target_node) != FE_T_ResTarget)
                    continue;

                rt = (ResTarget *)target_node;
                if (rt->name != NULL && rt->val != NULL &&
                    nodeTag(rt->val) == FE_T_Param)
                {
                    Param *param;
                    int pid;

                    param = (Param *)rt->val;
                    pid = param->paramid;
                    if (pid > 0 && pid <= max_param)
                    {
                        set_param_column_info(stmt, pid - 1, table_name,
                                             rt->name, statement_data);
                    }
                }
            }
        }

        /* ReqID:TECC4.0 — Map WHERE clause params for UPDATE */
        if (table_name != NULL && update_stmt->whereClause != NULL)
        {
            map_select_params_in_expr(update_stmt->whereClause, stmt,
                                      table_name, statement_data);
        }
    }

    pg_free(param_seen);

    /* Count encrypted params */
    if (stmt->params != NULL && stmt->num_params > 0)
    {
        int enc_count;
        int ei;

        enc_count = 0;
        for (ei = 0; ei < stmt->num_params; ei++)
        {
            if (stmt->params[ei] != NULL && stmt->params[ei]->needs_encryption)
                enc_count++;
        }
        stmt->num_encrypted_params = enc_count;
    }

    return TECC_PREPARED_STATEMENT_SUCCESS;
}

/*
 * Set column info on a parameter and check if it needs encryption.
 * Looks up the column in the metadata cache to determine encryption status.
 */
/*
 * ReqID:TECC4.0 - 取 FROM 子句里第一个基表名（派生表则递归进其子查询），
 * 用于建立「别名 → 基表」映射。
 */
static const char *
first_from_relname(List *from_clause)
{
    ListCell *lc;

    foreach(lc, from_clause)
    {
        Node *n = (Node *)lfirst(lc);

        if (n == NULL)
            continue;
        if (nodeTag(n) == FE_T_RangeVar)
            return ((RangeVar *)n)->relname;
        if (nodeTag(n) == FE_T_RangeSubselect)
        {
            RangeSubselect *rs = (RangeSubselect *)n;

            if (rs->subquery != NULL && nodeTag(rs->subquery) == FE_T_SelectStmt)
                return first_from_relname(((SelectStmt *)rs->subquery)->fromClause);
        }
    }
    return NULL;
}

/*
 * ReqID:TECC4.0 - 把限定名（CTE 名 / 派生表别名）解析成基表名。
 * PREPARE 内层里 `WITH a AS (SELECT ... FROM t) ... a.col = $1` 或
 * `FROM (SELECT ... FROM t) s WHERE s.col = $1` 的限定名都不是基表，直接拿
 * 它们查加密列缓存必然落空，参数会因此不加密并以明文外发。链式 CTE
 * （别名指向另一个别名）按映射迭代解开。
 */
static const char *
resolve_param_table(const TeccPreparedStatement *stmt, const char *table_name)
{
    const char *cur;
    int depth;

    if (stmt == NULL || table_name == NULL || stmt->alias_names == NIL)
        return table_name;

    cur = table_name;
    for (depth = 0; depth < 8; depth++)
    {
        int n;
        int i;
        const char *next;

        next = NULL;
        n = list_length(stmt->alias_names);
        for (i = 0; i < n; i++)
        {
            const char *nm = (const char *)list_nth(stmt->alias_names, i);

            if (nm != NULL && strcmp(nm, cur) == 0)
            {
                next = (const char *)list_nth(stmt->alias_tables, i);
                break;
            }
        }
        if (next == NULL || strcmp(next, cur) == 0)
            break;
        cur = next;
    }

    return cur;
}

static bool
set_param_column_info(TeccPreparedStatement *stmt, int param_index,
                      const char *table_name, const char *column_name,
                      TeccStatementData *statement_data)
{
    TeccPreparedStatementParam *param_info;
    TeccState *state;
    const char *db_name;
    const char *schema_name;
    TeccColumnCacheItem *cache_result;
    char *col_dup;
    char *table_dup;

    

    if (stmt == NULL || param_index < 0 || param_index >= stmt->num_params)
        return false;
    if (table_name == NULL || column_name == NULL)
        return false;

    /* ReqID:TECC4.0 - 限定名可能是 CTE 名或派生表别名，先解析回基表名 */
    table_name = resolve_param_table(stmt, table_name);
    if (table_name == NULL)
        return false;

    param_info = stmt->params[param_index];
    if (param_info == NULL)
        return false;

    /* Free old values if previously set */
    if (param_info->table_name != NULL)
    {
        pg_free(param_info->table_name);
        param_info->table_name = NULL;
    }
    if (param_info->column_name != NULL)
    {
        pg_free(param_info->column_name);
        param_info->column_name = NULL;
    }
    if (param_info->column_type != NULL)
    {
        pg_free(param_info->column_type);
        param_info->column_type = NULL;
    }

    col_dup = pg_strdup(column_name);
    table_dup = pg_strdup(table_name);
    if (col_dup == NULL || table_dup == NULL)
    {
        if (col_dup) pg_free(col_dup);
        if (table_dup) pg_free(table_dup);
        return false;
    }

    param_info->table_name = table_dup;
    param_info->column_name = col_dup;

    /* Check encryption via metadata cache */
    state = tecc_get_state(statement_data->conn);
    db_name = (state != NULL) ? PQdb(statement_data->conn) : NULL;
    schema_name = (state != NULL)
        ? tecc_cache_get_guc_param(state, "current_schema") : NULL;

    
    if (state != NULL && db_name != NULL && schema_name != NULL)
    {
        
        cache_result = tecc_cache_get_encrypted_column_by_name(
            state, db_name, schema_name, table_name, column_name);
        
        if (cache_result != NULL)
        {
            const char *etype;

            
            param_info->is_encrypted = true;
            param_info->needs_encryption = true;

            /* Inline OID→encrypted-type-name mapping.
             * We cannot call get_enc_type_name_from_base_oid (values_processor.c)
             * because its static lookup tables cause a segfault when accessed
             * across compilation units. */
            {
                Oid base_oid;
                static const struct { Oid base; const char *name; } type_map[] = {
                    {INT2OID,    "int2_enc"},
                    {INT4OID,    "int4_enc"},
                    {INT8OID,    "int8_enc"},
                    {FLOAT4OID,  "float4_enc"},
                    {FLOAT8OID,  "float8_enc"},
                    {NUMERICOID, "numeric_enc"},
                    {VARCHAROID, "varchar_enc"},
                    {BPCHAROID,  "bpchar_enc"},
                    {TEXTOID,    "text_enc"},
                    {InvalidOid, NULL}
                };
                int ti;
                base_oid = cache_result->original_type_oid;
                etype = NULL;
                for (ti = 0; type_map[ti].name != NULL; ti++)
                {
                    if (type_map[ti].base == base_oid)
                    {
                        etype = type_map[ti].name;
                        break;
                    }
                }
                if (etype != NULL && etype[0] != '\0')
                    param_info->column_type = pg_strdup(etype);
            }

            
            /* Do NOT call tecc_cache_free_column_item here — the cache item
             * may be shared with the 'columns' array returned by
             * tecc_cache_get_columns_by_table_name.  The caller
             * (analyze_insert_params) calls tecc_cache_free_column_items
             * to clean up all items at once. */

            
            return true;
        }
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Param $%d maps to non-encrypted column %s.%s",
                       param_info->param_index, table_name, column_name);
    return false;
}

/*
 * Walk an expression tree to find Param nodes that appear in A_Expr operators
 * alongside ColumnRef nodes. When a match is found, call set_param_column_info
 * to mark the parameter for encryption.
 *
 * This handles patterns like:
 *   WHERE salary > $1   → Param $1 maps to encrypted column "salary"
 *   WHERE $1 < id       → Param $1 maps to column "id"
 */
static void
map_select_params_in_expr(Node *expr, TeccPreparedStatement *stmt,
                          const char *table_name, TeccStatementData *statement_data)
{
    const char *colname;
    A_Expr *a;
    Node *left;
    Node *right;
    ListCell *lc;

    if (expr == NULL || stmt == NULL || table_name == NULL)
        return;

    switch (nodeTag(expr))
    {
        case FE_T_A_Expr:
            a = (A_Expr *)expr;
            left = a->lexpr;
            right = a->rexpr;

            /*
             * ReqID:TECC4.0 - 参数侧可能带显式类型声明（`$1::text_enc`），
             * 此时右操作数是 TypeCast 而非 Param。字面量路径不受影响
             * （它拿的是 AST 求值后的值），参数路径必须自己剥掉这层，
             * 否则 `col = $1::text_enc` 永远匹配不上列，参数不加密、明文
             * 直发服务端（实测报 invalid hex string）。
             */
            {
                Node *lkey = left;
                Node *rkey = right;

                if (lkey != NULL && nodeTag(lkey) == FE_T_TypeCast)
                    lkey = ((TypeCast *)lkey)->arg;
                if (rkey != NULL && nodeTag(rkey) == FE_T_TypeCast)
                    rkey = ((TypeCast *)rkey)->arg;

                if (lkey != NULL && rkey != NULL)
                {
                    /* Case: ColumnRef op Param */
                    if (nodeTag(lkey) == FE_T_ColumnRef && nodeTag(rkey) == FE_T_Param)
                    {
                        ColumnRef *col = (ColumnRef *)lkey;
                        Param *param = (Param *)rkey;
                        if (col->fields != NULL && col->fields->tail != NULL)
                        {
                            colname = strVal(col->fields->tail->data.ptr_value);
                            if (colname != NULL)
                                set_param_column_info(stmt, param->paramid - 1,
                                    table_name, colname, statement_data);
                        }
                    }
                    /* Case: Param op ColumnRef */
                    else if (nodeTag(lkey) == FE_T_Param && nodeTag(rkey) == FE_T_ColumnRef)
                    {
                        Param *param = (Param *)lkey;
                        ColumnRef *col = (ColumnRef *)rkey;
                        if (col->fields != NULL && col->fields->tail != NULL)
                        {
                            colname = strVal(col->fields->tail->data.ptr_value);
                            if (colname != NULL)
                                set_param_column_info(stmt, param->paramid - 1,
                                    table_name, colname, statement_data);
                        }
                    }
                }
            }

            /* Recurse into both sides */
            map_select_params_in_expr(left, stmt, table_name, statement_data);
            map_select_params_in_expr(right, stmt, table_name, statement_data);
            break;

        case FE_T_BoolExpr:
            foreach(lc, ((BoolExpr *)expr)->args)
                map_select_params_in_expr((Node *)lc->data.ptr_value,
                    stmt, table_name, statement_data);
            break;

        case FE_T_NullTest:
            /* NullTest: IS NULL / IS NOT NULL — no comparison with another value */
            break;

        case FE_T_BooleanTest:
            /* BooleanTest: IS TRUE/FALSE/UNKNOWN — no comparison with another value */
            break;

        case FE_T_SubLink:
            /* SubLink: subquery, params inside belong to outer context */
            break;

        case FE_T_FuncCall:
            /* Walk function arguments for Param nodes */
            foreach(lc, ((FuncCall *)expr)->args)
                map_select_params_in_expr((Node *)lc->data.ptr_value,
                    stmt, table_name, statement_data);
            break;

        case FE_T_TypeCast:
            map_select_params_in_expr(((TypeCast *)expr)->arg,
                stmt, table_name, statement_data);
            break;

        case FE_T_List:
        {
            ListCell *lc;
            foreach(lc, (List *)expr)
                map_select_params_in_expr((Node *)lfirst(lc),
                    stmt, table_name, statement_data);
            break;
        }

        default:
            /* Other node types: no Param mapping needed */
            break;
    }
}

/*
 * Process EXECUTE parameter values — encryption/decryption.
 *
 * The needs_encryption flag is set during PREPARE by analyze_query_for_params
 * (parse-tree traversal). Here we log which params need encryption.
 *
 * NOTE: Actual parameter value encryption during EXECUTE is not yet implemented.
 * The encrypted flag and column mapping are stored so a future EXECUTE handler
 * can call the encryption scheduler for each param that needs it.
 */
static TeccPreparedStatementResult
process_param_values(TeccPreparedStatement *stmt, TeccStatementData *statement_data)
{
    int i;

    if (statement_data == NULL || statement_data->nParams <= 0)
        return TECC_PREPARED_STATEMENT_SUCCESS;

    for (i = 0; i < statement_data->nParams && i < stmt->num_params; i++)
    {
        TeccPreparedStatementParam *param_info;

        param_info = stmt->params[i];
        if (param_info != NULL)
        {
            /* needs_encryption was already determined by analyze_query_for_params.
             * Do NOT override it here. */
            if (param_info->needs_encryption)
            {
                
            }
            else
            {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Param $%d does not need encryption",
                                   param_info->param_index);
            }
        }
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Processed %d parameters for statement '%s', %d need encryption",
                       stmt->num_params, stmt->stmt_name, stmt->num_encrypted_params);

    return TECC_PREPARED_STATEMENT_SUCCESS;
}
/* End - ReqID:TECC4.0 */