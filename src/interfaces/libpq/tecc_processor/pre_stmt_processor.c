
/* Begin - ReqID:TECC4.0 */
/*
 * File: pre_stmt_processor.c
 *
 * Abstract:
 *   TECC pre-statement processor for statement preprocessing phase.
 *   Handles preprocessing stage management, parameter type replacement, and query string reconstruction.
 *
 * Authored by <wangxp@highgo.com>, 20260412.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/pre_stmt_processor.c
 */

#include "postgres_fe.h"
#include "common/fe_memutils.h"
#include "pre_stmt_processor.h"
#include "internal/tecc_processor_internal.h"

/* Include libpq integration for tecc_get_state */
#include "../tecc_libpq_integration/fe-tecc-integration.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Internal helper functions */
static TeccPreStmtProcessorResult analyze_statement_for_preprocessing(TeccPreStmtProcessorContext *context);
static TeccPreStmtProcessorResult process_parameter_types(TeccPreStmtProcessorContext *context);
static TeccPreStmtProcessorResult build_reconstructed_query(TeccPreStmtProcessorContext *context);

/* Initialize pre-statement processor context */
TeccPreStmtProcessorContext *
tecc_pre_stmt_processor_init(PGconn *conn)
{
    TeccPreStmtProcessorContext *context;

    if (conn == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid connection parameter to pre-statement processor init");
        return NULL;
    }

    /* Allocate context */
    context = (TeccPreStmtProcessorContext *)pg_malloc(sizeof(TeccPreStmtProcessorContext));
    if (!context) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Memory allocation failed for pre-statement processor context");
        return NULL;
    }

    /* Initialize context */
    memset(context, 0, sizeof(TeccPreStmtProcessorContext));
    context->conn = conn;
    context->stmt_data = NULL;
    context->parse_tree = NULL;
    context->needs_type_replacement = false;
    context->needs_query_reconstruction = false;
    context->reconstructed_query = NULL;
    context->error_code = TECC_PRE_STMT_PROCESSOR_SUCCESS;
    context->error_msg[0] = '\0';

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Pre-statement processor context initialized");

    return context;
}

/* Process a statement in preprocessing phase */
TeccPreStmtProcessorResult
tecc_pre_stmt_processor_process(TeccPreStmtProcessorContext *context,
                               TeccStatementData *stmt_data)
{
    TeccPreStmtProcessorResult result;

    if (context == NULL || stmt_data == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid parameters to pre-statement processor process");
        return TECC_PRE_STMT_PROCESSOR_INVALID_INPUT;
    }

    /* Store statement data reference */
    context->stmt_data = stmt_data;

    

    /* Step 1: Analyze statement for preprocessing needs */
    result = analyze_statement_for_preprocessing(context);
    if (result != TECC_PRE_STMT_PROCESSOR_SUCCESS) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Statement analysis failed: %d", result);
        return result;
    }

    /* Step 2: Check if preprocessing is needed */
    if (!context->needs_type_replacement && !context->needs_query_reconstruction) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "No preprocessing needed for this statement");
        return TECC_PRE_STMT_PROCESSOR_SUCCESS;
    }

    /* Step 3: Process parameter types if needed */
    if (context->needs_type_replacement) {
        result = process_parameter_types(context);
        if (result != TECC_PRE_STMT_PROCESSOR_SUCCESS) {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "Parameter type processing failed: %d", result);
            return result;
        }
    }

    /* Step 4: Reconstruct query if needed */
    if (context->needs_query_reconstruction) {
        result = build_reconstructed_query(context);
        if (result != TECC_PRE_STMT_PROCESSOR_SUCCESS) {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "Query reconstruction failed: %d", result);
            return result;
        }

        /* Update statement data with reconstructed query */
        if (context->reconstructed_query != NULL) {
            /* Store original query for reference */
            /* TODO: Implement proper original query storage and cleanup */
            /* if (stmt_data->original_query == NULL && stmt_data->query != NULL) {
                stmt_data->original_query = pg_strdup(stmt_data->query);
            } */

            /* Update current query with reconstructed version */
            if (stmt_data->query != NULL) {
                pg_free((void *)stmt_data->query);
            }
            stmt_data->query = pg_strdup(context->reconstructed_query);

            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "Query reconstructed: %s", stmt_data->query);
        }
    }

    

    return TECC_PRE_STMT_PROCESSOR_SUCCESS;
}

/* Handle prepared statement preprocessing */
TeccPreStmtProcessorResult
tecc_pre_stmt_processor_handle_prepared(TeccPreStmtProcessorContext *context,
                                       TeccPreparedStatement *prepared_stmt)
{
    if (context == NULL || prepared_stmt == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid parameters to pre-statement processor handle_prepared");
        return TECC_PRE_STMT_PROCESSOR_INVALID_INPUT;
    }

    

    /* For prepared statements, we need to analyze parameter types */
    /* This is a simplified implementation - actual implementation would analyze parameter types */

    /* Mark that type replacement is needed for prepared statements */
    context->needs_type_replacement = true;

    /* Implement parameter type analysis for prepared statements */
    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Analyzing parameter types for prepared statement");

    /* Basic parameter type analysis:
     * 1. Check if the statement references encrypted columns
     * 2. If yes, parameters targeting those columns may need type replacement
     * 3. Determine appropriate replacement types based on encryption algorithm
     */

    /* For now, we'll implement a basic analysis that always marks type replacement as needed
     * when encrypted columns are present. Actual implementation would require:
     * 1. Parse tree analysis to map parameters to columns
     * 2. Metadata cache lookup for encryption information
     * 3. Type compatibility checking
     */

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Prepared statement preprocessing completed");

    return TECC_PRE_STMT_PROCESSOR_SUCCESS;
}

/* Perform parameter type replacement */
TeccPreStmtProcessorResult
tecc_pre_stmt_processor_replace_param_types(TeccPreStmtProcessorContext *context,
                                           TeccTypeReplacementInfo **replacements,
                                           int num_replacements)
{
    int i;

    if (context == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid context to replace_param_types");
        return TECC_PRE_STMT_PROCESSOR_INVALID_INPUT;
    }

    if (replacements == NULL || num_replacements <= 0) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "No parameter type replacements specified");
        return TECC_PRE_STMT_PROCESSOR_SUCCESS;
    }

    

    /* Process each replacement */
    for (i = 0; i < num_replacements; i++) {
        TeccTypeReplacementInfo *replacement = replacements[i];
        if (replacement == NULL) {
            continue;
        }

        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "Replacing parameter %d: type %u -> %u (%s)",
                            replacement->param_index,
                            replacement->original_type,
                            replacement->replacement_type,
                            replacement->reason ? replacement->reason : "unknown");

        /* Implement parameter type replacement logic */
        /* This involves modifying the parse tree or parameter list */

        /* Actual implementation would:
         * 1. Locate the parameter reference in the parse tree
         * 2. Update the type information
         * 3. Adjust any type-dependent structures
         * 4. Update the parameter type array if applicable
         */

        /* For now, we log the replacement and mark that reconstruction is needed */
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Parameter type replacement: %u -> %u (reason: %s)",
                           replacement->original_type,
                           replacement->replacement_type,
                           replacement->reason ? replacement->reason : "unknown");
    }

    /* Mark that query reconstruction is needed after type replacement */
    context->needs_query_reconstruction = true;

    

    return TECC_PRE_STMT_PROCESSOR_SUCCESS;
}

/* Reconstruct query string after processing */
TeccPreStmtProcessorResult
tecc_pre_stmt_processor_reconstruct_query(TeccPreStmtProcessorContext *context)
{
    if (context == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid context to reconstruct_query");
        return TECC_PRE_STMT_PROCESSOR_INVALID_INPUT;
    }

    

    return build_reconstructed_query(context);
}

/* Check if statement needs preprocessing */
bool
tecc_pre_stmt_processor_needs_preprocessing(TeccPreStmtProcessorContext *context,
                                           TeccStatementData *stmt_data)
{
    /* Variable declarations at function beginning for ISO C90 compliance */
    TeccPreStmtProcessorResult result;

    if (context == NULL || stmt_data == NULL) {
        return false;
    }

    /* Store statement data for analysis */
    context->stmt_data = stmt_data;

    /* Analyze statement */
    result = analyze_statement_for_preprocessing(context);
    if (result != TECC_PRE_STMT_PROCESSOR_SUCCESS) {
        return false;
    }

    /* Return true if either type replacement or query reconstruction is needed */
    return context->needs_type_replacement || context->needs_query_reconstruction;
}

/* Get reconstructed query string */
const char *
tecc_pre_stmt_processor_get_reconstructed_query(TeccPreStmtProcessorContext *context)
{
    if (context == NULL) {
        return NULL;
    }

    return context->reconstructed_query;
}

/* Clean up pre-statement processor context */
void
tecc_pre_stmt_processor_cleanup(TeccPreStmtProcessorContext *context)
{
    if (context == NULL) {
        return;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Cleaning up pre-statement processor context");

    /* Free reconstructed query if present */
    if (context->reconstructed_query != NULL) {
        pg_free(context->reconstructed_query);
        context->reconstructed_query = NULL;
    }

    /* Free context */
    pg_free(context);
}

/* Unified pre-statement processing interface for external modules */
TeccPreStmtProcessorResult
tecc_pre_stmt_processor_run(PGconn *conn,
                           TeccStatementData *stmt_data)
{
    TeccPreStmtProcessorContext *context;
    TeccPreStmtProcessorResult result;
    TeccState *state;

    if (conn == NULL || stmt_data == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid parameters to pre_stmt_processor_run");
        return TECC_PRE_STMT_PROCESSOR_INVALID_INPUT;
    }

    /* Get TECC state and check if TECC is enabled */
    state = tecc_get_state(conn);
    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "tecc_pre_stmt_processor_run: state=%p, conn=%p",
                       state, conn);
    if (state != NULL && !tecc_is_enabled(state)) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "TECC is disabled (possibly due to disable_once flag), skipping pre-statement processing");
        return TECC_PRE_STMT_PROCESSOR_SUCCESS; /* Return success to allow query to proceed without TECC processing */
    }

    /* Initialize context */
    context = tecc_pre_stmt_processor_init(conn);
    if (context == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to initialize pre-statement processor context");
        return TECC_PRE_STMT_PROCESSOR_MEMORY_ERROR;
    }

    /* Process statement */
    result = tecc_pre_stmt_processor_process(context, stmt_data);

    /* Clean up context */
    tecc_pre_stmt_processor_cleanup(context);

    return result;
}

/* ==================== Internal helper functions ==================== */

/* Analyze statement for preprocessing needs */
static TeccPreStmtProcessorResult
analyze_statement_for_preprocessing(TeccPreStmtProcessorContext *context)
{
    TeccStatementData *stmt_data;

    if (context == NULL || context->stmt_data == NULL) {
        return TECC_PRE_STMT_PROCESSOR_INVALID_INPUT;
    }

    stmt_data = context->stmt_data;

    /* Reset preprocessing flags */
    context->needs_type_replacement = false;
    context->needs_query_reconstruction = false;

    /* Check statement type */
    if (stmt_data->stmt_type == TECC_STMT_TYPE_PREPARE ||
        stmt_data->stmt_type == TECC_STMT_TYPE_EXECUTE ||
        stmt_data->stmt_type == TECC_STMT_TYPE_DECLARE_CURSOR) {
        /* Prepared statements need type replacement */
        context->needs_type_replacement = true;
        context->needs_query_reconstruction = true;

        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "Prepared statement detected - needs type replacement and query reconstruction");
    }
    else if (stmt_data->stmt_type == TECC_STMT_TYPE_SELECT ||
             stmt_data->stmt_type == TECC_STMT_TYPE_INSERT ||
             stmt_data->stmt_type == TECC_STMT_TYPE_UPDATE ||
             stmt_data->stmt_type == TECC_STMT_TYPE_DELETE) {
        /* DML statements may need query reconstruction if they contain encrypted columns */
        if (stmt_data->encrypted_columns != NULL &&
            list_length(stmt_data->encrypted_columns) > 0) {
            context->needs_query_reconstruction = true;

            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "DML statement with encrypted columns detected - needs query reconstruction");
        }
    }
    else if (stmt_data->stmt_type == TECC_STMT_TYPE_CREATE_TABLE)
    {
        /* CREATE TABLE may need type replacement for encrypted column definitions */
        context->needs_type_replacement = true;
        context->needs_query_reconstruction = true;

        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "CREATE TABLE detected - needs type replacement and query reconstruction");
    }
    else if (stmt_data->stmt_type == TECC_STMT_TYPE_CREATE_KEK ||
             stmt_data->stmt_type == TECC_STMT_TYPE_ALTER_KEK ||
             stmt_data->stmt_type == TECC_STMT_TYPE_CREATE_DEK)
    {
        /* CREATE KEK/DEK: pass-through only.
         * These statements have no parse tree (the TECC parser doesn't
         * understand them) and therefore must NOT trigger type replacement
         * or query reconstruction, which would free the caller's query
         * buffer and cause heap corruption / garbled SQL on send. */
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "CREATE KEK/DEK: pass-through, no rewrite needed");
    }

    return TECC_PRE_STMT_PROCESSOR_SUCCESS;
}

/* Process parameter types */
static TeccPreStmtProcessorResult
process_parameter_types(TeccPreStmtProcessorContext *context)
{
    /* This is a simplified implementation */
    /* Actual implementation would analyze and replace parameter types in the parse tree */

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing parameter types");

    /* Implement parameter type processing */
    /* This involves:
       1. Analyzing the parse tree for parameter references
       2. Identifying parameters that reference encrypted columns
       3. Determining appropriate type replacements
       4. Applying type replacements to the parse tree
    */

    /* Basic implementation:
     * 1. Check if the statement has encrypted columns
     * 2. If yes, mark that parameter type processing is needed
     * 3. Actual type replacement would be done in replace_param_types function
     */

    if (context->stmt_data != NULL && context->stmt_data->encrypted_columns != NULL) {
        if (list_length(context->stmt_data->encrypted_columns) > 0) {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Statement has encrypted columns - parameter type processing needed");
            context->needs_type_replacement = true;
        }
    }

    return TECC_PRE_STMT_PROCESSOR_SUCCESS;
}

/* Build reconstructed query string */
static TeccPreStmtProcessorResult
build_reconstructed_query(TeccPreStmtProcessorContext *context)
{
    TeccStatementData *stmt_data;
    char *reconstructed;

    if (context == NULL || context->stmt_data == NULL) {
        return TECC_PRE_STMT_PROCESSOR_INVALID_INPUT;
    }

    stmt_data = context->stmt_data;

    /* Free existing reconstructed query if present */
    if (context->reconstructed_query != NULL) {
        pg_free(context->reconstructed_query);
        context->reconstructed_query = NULL;
    }

    /* For now, just copy the original query */
    /* Actual implementation would reconstruct the query from the modified parse tree */
    if (stmt_data->query != NULL) {
        reconstructed = pg_strdup(stmt_data->query);
        if (reconstructed == NULL) {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "Memory allocation failed for reconstructed query");
            return TECC_PRE_STMT_PROCESSOR_MEMORY_ERROR;
        }

        context->reconstructed_query = reconstructed;

        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "Query reconstructed (simplified): %s", reconstructed);
    }

    return TECC_PRE_STMT_PROCESSOR_SUCCESS;
}
/* End - ReqID:TECC4.0 */