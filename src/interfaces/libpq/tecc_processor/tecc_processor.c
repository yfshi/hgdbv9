#ifdef _MSC_VER
#define TECC_ATTR_UNUSED
#else
#define TECC_ATTR_UNUSED __attribute__((unused))
#endif

/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_processor.c
 *
 * Abstract:
 *   Main TECC processor implementation for encrypted column processing.
 *   Coordinates all submodules to provide unified SQL statement processing
 *   for transparent encryption functionality.
 *
 * Authored by <wangxp@highgo.com>, 20260403.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/tecc_processor.c
 */

/* Include order is critical to avoid conflicts */
#include "postgres_fe.h"
#include "tecc_processor.h"
#include "internal/tecc_processor_internal.h"
#include "stmt_processor.h"
#include "values_processor.h"
#include "tecc_type_output.h"
#include "where_clause_processor.h"
#include "create_stmt_processor.h"
#include "alter_stmt_processor.h"

/* Include new submodules (v4.0 additions) */
#include "record_processor.h"
#include "func_processor.h"
#include "catalog/pg_type_enc.h"  /* ReqID:TECC4.0 */

/* Include libpq integration for TeccState */
#include "../tecc_libpq_integration/fe-tecc-integration.h"

/* Include cache module */
#include "../tecc_cache/tecc_metadata_cache.h"

/* Include output formatter for decrypted binary-to-text conversion */
#include "../tecc_formatter/tecc_type_output.h"
/* ReqID:TECC4.0 - Include formatter for initialization (ICU etc.) */
#include "../tecc_formatter/tecc_formatter.h"
#include "prepared_statement.h"
#include "prepared_statements_list.h"
#include "pre_stmt_processor.h"
#include "post_stmt_processor.h"

/* Include frontend parser node definitions */
#include "../frontend_parser/tecc_nodes.h"  /* For foreach macro */

/* Standard C headers */
#include <stdlib.h>  /* For malloc, free (deprecated - use pg_malloc/pg_free) */
#include <string.h>  /* For memset */
#include <stdarg.h>  /* For va_start, va_end */
#include <stdio.h>   /* For fopen/fread */

/* TECC algorithm headers */
#include "common/tecc_algorithm/tecc_sm3_internal.h"

/* Debug function declaration */
static void tecc_debug_print_parse_tree(Node *node, int depth);

/* PostgreSQL frontend memory utilities */
#include "common/fe_memutils.h"  /* For pg_malloc, pg_free */

/* Forward declarations for external module interfaces */
#include "../frontend_parser/fe_adapter.h"  /* Frontend parser interface */
#include "../frontend_parser/tecc_nodes.h"  /* Node type definitions and macros */
#include "../tecc_cache/tecc_metadata_cache.h"  /* Cache module interface */
#include "../tecc_scheduler/tecc_crypto_scheduler.h"  /* Crypto scheduler interface */

/* PrepareStmt and ExecuteStmt are now defined in tecc_nodes.h */
/* Forward declarations for scheduler and formatter types */
typedef struct TeccCryptoScheduler TeccCryptoScheduler;
typedef struct TeccUnifiedDecryptResult TeccUnifiedDecryptResult;
typedef struct TeccFormatter TeccFormatter;

/*
 * Global processor state
 * Note: tecc_processor_initialized and tecc_processor_debug_enabled have been moved to TeccState
 * for thread safety (connection-level management)
 */

/* Begin - ReqID:TECC4.0 - 线程安全调试支持 */

/* 全局调试级别变量已移除，使用连接特定的调试级别
 * 旧代码：int tecc_processor_debug_level = TECC_LOG_DEBUG1;
 * 新代码：使用 TeccState->config.debug_level 作为连接特定的调试级别
 */

/* End - ReqID:TECC4.0 - 线程安全调试支持 */

/*
 * Forward declarations
 */
static TeccProcessorResult processor_process_statement(const Node *stmt,
                                                      TeccStatementData *statement_data);
static void tecc_copy_free_parsed_fields(char ***field_values, int **field_lengths, int field_count);
static TeccProcessorResult tecc_processor_rewrite_query(TeccStatementData *statement_data);

/* Prepared statement handling functions */
static TeccProcessorResult processor_handle_prepare_statement(const Node *stmt,
                                                             TeccStatementData *statement_data);
static TeccProcessorResult processor_handle_execute_statement(const Node *stmt,
                                                             TeccStatementData *statement_data);
static TeccProcessorResult processor_handle_declare_cursor_statement(const Node *stmt,
                                                                    TeccStatementData *statement_data);
static TeccProcessorResult processor_handle_deallocate_statement(const Node *stmt,
                                                                 TeccStatementData *statement_data);
static TeccProcessorResult processor_handle_create_table_as_statement(const Node *stmt,
                                                                     TeccStatementData *statement_data);
static void tecc_processor_encrypt_execute_params(const ExecuteStmt *execute_stmt,
    TeccPreparedStatement *ps, TeccStatementData *statement_data);

/* SQL statement type checking functions */
static bool tecc_processor_needs_parsing(const char *query);

/*
 * Initialize processor context
 * Note: This function is deprecated. Processor initialization should be done
 * through tecc_state_init() which sets processor_initialized in TeccState.
 */
void
tecc_processor_init(PGconn *conn)
{
    TeccState *state;

    if (conn == NULL)
    {
        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        /* 使用线程安全的调试宏，state为NULL时会回退到旧的调试宏 */
        tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                                 "Invalid connection for processor initialization");
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
        /* Begin - ReqID:TECC4.0 - 调试日志添加 */
        tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                 "DEBUG: tecc_processor_init returning early - conn is NULL");
        /* End - ReqID:TECC4.0 - 调试日志添加 */
        return;
    }

    /* Get TECC state from connection */
    state = tecc_get_state(conn);
    if (state == NULL)
    {
        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        /* 使用线程安全的调试宏，state为NULL时会回退到旧的调试宏 */
        tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                                 "Failed to get TECC state from connection");
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
        /* Begin - ReqID:TECC4.0 - 调试日志添加 */
        tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                 "DEBUG: tecc_processor_init returning early - tecc_get_state returned NULL");
        /* End - ReqID:TECC4.0 - 调试日志添加 */
        return;
    }

    /* Check if already initialized */
    if (state->processor_initialized)
    {
        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
        /* Begin - ReqID:TECC4.0 - 调试日志添加 */
        tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                 "DEBUG: tecc_processor_init returning early - processor already initialized");
        /* End - ReqID:TECC4.0 - 调试日志添加 */
        return;
    }

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    /* Initialize submodules */
    /* ReqID:TECC4.0 - Initialize formatter module (including ICU) before any processing.
     * tecc_formatter_initialize is idempotent — safe to call per-connection. */
    tecc_formatter_initialize();

    /* Initialize cache module - only if not already initialized by libpq integration layer */
    if (state->metadata_cache == NULL)
    {
        if (tecc_cache_init((TeccState *)state) < 0)
        {
            /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
            tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                                     "Failed to initialize metadata cache");
            /* End - ReqID:TECC4.0 - 线程安全调试支持 */
            /* Begin - ReqID:TECC4.0 - 调试日志添加 */
            tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                     "DEBUG: tecc_processor_init returning early - tecc_cache_init failed");
            /* End - ReqID:TECC4.0 - 调试日志添加 */
            return;
        }

        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
    }
    else
    {
        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
    }

    /* Initialize key manager adapter for crypto scheduler */
    if (state->key_mgr == NULL)
    {
        state->key_mgr = tecc_key_manager_adapter_create_with_state(
            (TeccState *)state);
        if (state->key_mgr == NULL)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                "Failed to create key manager adapter");
            return;
        }
    }

    /* Initialize crypto scheduler */
    if (state->crypto_scheduler == NULL)
    {
        state->crypto_scheduler = tecc_crypto_scheduler_init(
            (TeccMetadataCache *)state->metadata_cache,
            (TeccKeyManagerAdapter *)state->key_mgr);
        if (state->crypto_scheduler != NULL)
        {
            state->scheduler_initialized = true;
        }
        else
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                "Failed to initialize crypto scheduler");
            return;
        }
    }

    /* Initialize new submodules (v4.0 additions) */
    /* Note: The actual instances will be created when first needed */
    /* - record_processor: initialized on first record processing */
    /* - func_processor: initialized on first function call processing */
    /* - prepared_statement: initialized on first PREPARE statement */
    /* - prepared_statements_list: initialized on first prepared statement */
    /* - pre_stmt_processor: initialized on first preprocessing */
    /* - post_stmt_processor: initialized on first postprocessing */
    /* - guc_processor: initialized on first GUC-related processing */

    state->processor_initialized = true;
    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */
}

/*
 * Clean up statement data including parse tree
 */
static void
processor_cleanup_statement_data_internal(TeccStatementData *statement_data)
{
    if (statement_data == NULL)
        return;

    /* Free parse tree if allocated by parser */
    if (statement_data->parse_tree != NULL)
    {
        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                 "Freeing parse tree");
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
        tecc_parser_free_tree(statement_data->parse_tree);
        statement_data->parse_tree = NULL;
    }

    /* Free encrypted columns list */
    if (statement_data->encrypted_columns != NULL)
    {
        /* Note: encrypted_columns is a List* allocated by processor */
        /* We need to implement proper list cleanup */
        statement_data->encrypted_columns = NULL;
    }

    /* Free raw values list */
    if (statement_data->raw_values_list != NULL)
    {
        /* raw_values_list cleanup is handled by raw_values_list module */
        statement_data->raw_values_list = NULL;
    }

    /* Begin - ReqID:TECC4.0 - GUC处理器清理 */
    /* GUC processor module has been removed - code commented out */
    /*
    // Clean up GUC processor
    tecc_processor_cleanup_guc_processor(statement_data);
    */
    /* End - ReqID:TECC4.0 - GUC处理器清理 */
}

/*
 * Clean up processor context
 */
void
tecc_processor_cleanup(PGconn *conn)
{
    TeccState *state;

    if (conn == NULL)
        return;

    /* Get TECC state from connection */
    state = tecc_get_state(conn);
    if (state == NULL)
    {
        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                                 "Failed to get TECC state from connection");
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
        return;
    }

    /* Check if already cleaned up */
    if (!state->processor_initialized)
    {
        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
        return;
    }

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "tecc_processor_cleanup: state->crypto_scheduler=%p, state->scheduler_initialized=%d",
                        state->crypto_scheduler, state->scheduler_initialized);

    /* Clean up submodules */
    /* Cleanup cache module */
    tecc_cache_cleanup((TeccState *)state);

    /* Cleanup encryption scheduler */
    /* Use connection-specific scheduler cleanup instead of global */
    if (state->crypto_scheduler != NULL) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "tecc_processor_cleanup: Using connection-specific scheduler cleanup");
        tecc_crypto_scheduler_cleanup(state->crypto_scheduler);
        state->crypto_scheduler = NULL;
        state->scheduler_initialized = false;
    }

    /* key_mgr is owned and freed by crypto scheduler cleanup above */
    state->key_mgr = NULL;

    state->processor_initialized = false;

    /* Safety: Ensure disable_once flag is reset */
    if (state->disable_once) {
        tecc_processor_debug(TECC_LOG_WARNING,
                           "tecc_processor_cleanup: resetting disable_once flag from true to false");
        state->disable_once = false;
    }

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */
}

/*
 * Public API for cleaning up statement data
 */
void
tecc_processor_cleanup_statement_data(TeccStatementData *statement_data)
{
    processor_cleanup_statement_data_internal(statement_data);
}

/*
 * Run pre-query processing
 */
TeccProcessorResult
tecc_processor_run_pre_query(TeccStatementData *statement_data)
{
    TeccProcessorResult result;
    TeccState *state;
    const char *query;

    if (statement_data == NULL)
        return TECC_PROCESSOR_INVALID_INPUT;

    /* Get TECC state from connection */
    if (statement_data->conn == NULL)
    {
        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                                 "No connection in statement data");
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
        return TECC_PROCESSOR_INVALID_INPUT;
    }

    state = tecc_get_state(statement_data->conn);
    if (state == NULL)
    {
        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                                 "Failed to get TECC state from connection");
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
        return TECC_PROCESSOR_ERROR;
    }

    if (!state->processor_initialized)
    {
        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                                 "Processor not initialized for this connection DEBUG");
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
        return TECC_PROCESSOR_ERROR;
    }

    /* Check if TECC is enabled (considering disable_once flag) */
    if (!tecc_is_enabled(state))
    {
        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                 "TECC is disabled (possibly due to disable_once flag), skipping processing");
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
        return TECC_PROCESSOR_SUCCESS; /* Return success to allow query to proceed without TECC processing */
    }


    /* Clear previous state */
    statement_data->encrypted_columns = NIL;
    statement_data->needs_rewrite = 0;
    statement_data->stmt_type = TECC_STMT_TYPE_UNKNOWN;
    tecc_processor_clear_error(statement_data);

    /* Detect statement type from query text for statements the parser
     * does not understand (CREATE KEK, CREATE DEK), so that the
     * pre_stmt_processor can set needs_rewrite=true before parsing. */
    if (statement_data->query != NULL)
    {
        const char *q = statement_data->query;
        while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r')
            q++;
        if (strncasecmp(q, "CREATE KEK", 10) == 0)
            statement_data->stmt_type = TECC_STMT_TYPE_CREATE_KEK;
        else if (strncasecmp(q, "CREATE DEK", 10) == 0)
            statement_data->stmt_type = TECC_STMT_TYPE_CREATE_DEK;
    }

    /* Begin - ReqID:TECC4.0 - GUC参数处理初始化 */
    /* GUC processor module has been removed - code commented out */
    /*
    // Initialize GUC processor if not already initialized
    tecc_processor_init_guc_processor(statement_data);

    // Check if this is a SET statement
    if (statement_data->query != NULL) {
        statement_data->is_set_statement = tecc_processor_is_set_statement(statement_data->query);

        if (statement_data->is_set_statement) {
            // Parse SET statement
            char *param_name = NULL;
            char *param_value = NULL;

            result = tecc_processor_parse_set_statement(statement_data->query,
                                                       &param_name,
                                                       &param_value);
            if (result == TECC_PROCESSOR_SUCCESS) {
                statement_data->set_param_name = param_name;
                statement_data->set_param_value = param_value;
                
            } else {
                statement_data->is_set_statement = false;
                tecc_processor_debug(TECC_LOG_WARNING,
                                   "Failed to parse SET statement, treating as regular query");
            }
        }
    }
    */
    /* End - ReqID:TECC4.0 - GUC参数处理初始化 */

    /* Step 1: Pre-statement processing (preprocessing phase) */
    if (statement_data->conn != NULL)
    {
        TeccPreStmtProcessorResult pre_result;

        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                 "Starting pre-statement processing (preprocessing phase), statement_data=%p, statement_data->conn=%p",
                                 statement_data, statement_data ? statement_data->conn : NULL);
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */

        pre_result = tecc_pre_stmt_processor_run(statement_data->conn, statement_data);
        if (pre_result != TECC_PRE_STMT_PROCESSOR_SUCCESS)
        {
            /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
            tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                                     "Pre-statement processing failed: %d", pre_result);
            /* End - ReqID:TECC4.0 - 线程安全调试支持 */
            return TECC_PROCESSOR_ERROR;
        }

        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                 "Pre-statement processing completed successfully");
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
    }

    if (statement_data->parse_tree == NIL && statement_data->query != NULL)
    {
        /* Check if this SQL statement needs TECC parsing */
        if (tecc_processor_needs_parsing(statement_data->query))
        {
            tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                    "Calling tecc_parser_parse for '%s'",
                                    statement_data->query);
            /* Call frontend parser to parse SQL */
            statement_data->parse_tree = tecc_parser_parse(statement_data->query);
            tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                    "Parse tree=%p tag=%d",
                                    (void*)statement_data->parse_tree,
                                    statement_data->parse_tree ? nodeTag(statement_data->parse_tree) : -1);
            if (statement_data->parse_tree == NULL)
            {
                if (statement_data->stmt_type == TECC_STMT_TYPE_CREATE_KEK ||
                         statement_data->stmt_type == TECC_STMT_TYPE_CREATE_DEK ||
                         statement_data->stmt_type == TECC_STMT_TYPE_CREATE_TABLE)
                {
                    tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                            "CREATE KEK/DEK/TABLE statement, parse_tree is NULL (handled by pre_stmt_processor), needs_rewrite=%d",
                                            statement_data->needs_rewrite);
                    statement_data->parse_tree = NIL;
                    if (statement_data->needs_rewrite)
                    {
                        if (state != NULL) {
                            state->last_stmt_type = statement_data->stmt_type;
                            if (statement_data->stmt_type == TECC_STMT_TYPE_CREATE_TABLE &&
                                statement_data->encrypted_columns != NIL)
                            {
                                TeccColumnRefData *first = (TeccColumnRefData *)linitial(
                                    statement_data->encrypted_columns);
                                if (first != NULL)
                                {
                                    strncpy(state->last_table_schema, first->schema_name,
                                            sizeof(state->last_table_schema) - 1);
                                    state->last_table_schema[sizeof(state->last_table_schema) - 1] = '\0';
                                    strncpy(state->last_table_name, first->table_name,
                                            sizeof(state->last_table_name) - 1);
                                    state->last_table_name[sizeof(state->last_table_name) - 1] = '\0';
                                }
                            }
                        }
                        return TECC_PROCESSOR_NEEDS_REWRITE;
                    }
                    return TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS;
                }
                else
                {
                    tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                            "Statement type not supported by TECC parser, passing through to PostgreSQL");
                    statement_data->parse_tree = NIL;
                    statement_data->needs_rewrite = 0;
                    return TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS;
                }
            }
            /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
            if (statement_data->parse_tree != NIL)
            {
                tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                        "SQL parsing completed successfully, parse_tree=%p, nodeTag=%d",
                                        statement_data->parse_tree,
                                        nodeTag(statement_data->parse_tree));
            }
            else
            {
                tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                        "SQL parsing completed, parse_tree is NIL (empty list)");
            }

            /* Debug: Print the entire parse tree - temporarily disabled to avoid segfault */
            /* tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                    "=== DEBUG: Printing parse tree for query: %s ===",
                                    statement_data->query);
            if (statement_data->parse_tree != NULL && list_length(statement_data->parse_tree) > 0) {
                Node *first_node = (Node *)linitial(statement_data->parse_tree);
                tecc_debug_print_parse_tree(first_node, 0);
            } else {
                tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                        "Empty parse tree or NULL");
            }
            tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                    "=== DEBUG: End of parse tree ==="); */
            /* End - ReqID:TECC4.0 - 线程安全调试支持 */
        }
        else
        {
            /* This statement does not need TECC processing */
            /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
            tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                    "SQL statement does not need TECC processing: %s",
                                    statement_data->query);
            /* End - ReqID:TECC4.0 - 线程安全调试支持 */
            statement_data->parse_tree = NIL; /* Empty parse tree */
            statement_data->needs_rewrite = 0; /* No rewrite needed */
            return TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS; /* Skip further processing */
        }
    }

    /* Process parsed statements */
    if (statement_data->parse_tree != NULL)
    {
        /* Check if parse_tree is a List or a single Node */
	if (nodeTag(statement_data->parse_tree) == FE_T_List)
        {
            /* Process each statement in the list */
            /* Process each statement in the list */
            List *stmt_list = (List *) statement_data->parse_tree;
            ListCell *cell;
            int cell_count = 0;

            tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                    "List len=%d", stmt_list->length);

            /* 安全检查：如果列表长度为1，直接处理第一个节点 */
                if (stmt_list->length == 1 && stmt_list->head != NULL) {
                Node *stmt = (Node *) stmt_list->head->data.ptr_value;
                /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
                tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                        "Processing single statement in list: stmt=%p, nodeTag=%d",
                                        stmt, nodeTag(stmt));
                /* End - ReqID:TECC4.0 - 线程安全调试支持 */

                if (stmt != NULL) {
                    result = processor_process_statement(stmt, statement_data);
                    if (result != TECC_PROCESSOR_SUCCESS &&
                        result != TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS &&
                        result != TECC_PROCESSOR_NEEDS_REWRITE) {
                        /* Clean up before returning error */
                        processor_cleanup_statement_data_internal(statement_data);
                        return result;
                    }
                }
            } else {
                /* 对于长度不为1的列表，使用传统循环 */
                /* Use traditional loop instead of foreach macro */
                for (cell = stmt_list->head; cell != NULL; cell = cell->next)
                {
                    Node *stmt = (Node *) cell->data.ptr_value;
                    cell_count++;

                    /* 添加详细调试信息 */
                    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
                    tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                            "Processing list cell %d: cell=%p, cell->next=%p (expected NULL if last), stmt=%p, nodeTag=%d (0x%x)",
                                            cell_count, cell, cell->next, stmt, nodeTag(stmt), nodeTag(stmt));
                    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

                    /* 检查是否超出列表长度 */
                    if (cell_count > stmt_list->length) {
                        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
                        tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                                                "WARNING: Processing cell %d but list length is %d! Possible memory corruption.",
                                                cell_count, stmt_list->length);
                        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
                        break; /* 避免无限循环 */
                    }

                    if (stmt != NULL)
                    {
                        result = processor_process_statement(stmt, statement_data);
                        if (result != TECC_PROCESSOR_SUCCESS &&
                            result != TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS &&
                            result != TECC_PROCESSOR_NEEDS_REWRITE)
                        {
                            /* Clean up before returning error */
                            processor_cleanup_statement_data_internal(statement_data);
                            return result;
                        }
                    }
                    else
                    {
                        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
                        tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                                                "List cell %d has NULL statement pointer", cell_count);
                        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
                    }
                }
            }
        }
        else
        {
            /* Single statement (legacy support) */
            Node *stmt = (Node *) statement_data->parse_tree;
            result = processor_process_statement(stmt, statement_data);
            if (result != TECC_PROCESSOR_SUCCESS &&
                result != TECC_PROCESSOR_NEEDS_REWRITE)
            {
                /* Clean up before returning error */
                processor_cleanup_statement_data_internal(statement_data);
                return result;
            }
        }
    }
    else if (statement_data->query != NULL)
    {
        /*
         * Parse tree unavailable — soft-fail: pass original SQL through to backend.
         * TECC processing requires a parse tree; without one we cannot identify
         * encrypted columns or rewrite the query. The query is sent to the server
         * as-is, which is safe for non-encrypted statements and aligned with JDBC
         * behavior (TeccFailurePolicy.FALLBACK).
         */
        query = statement_data->query;
        tecc_processor_debug_safe(NULL, TECC_LOG_WARNING,
                                "Parse tree unavailable, passing through: %.120s",
                                query);
        return TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS;
    }


    /* Finalize processing */
    tecc_processor_debug(TECC_LOG_DEBUG1,
        "DEBUG: Finalize: encrypted_columns=%p, needs_rewrite=%d",
        statement_data->encrypted_columns, statement_data->needs_rewrite);
    if (statement_data->encrypted_columns != NULL)
    {
        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */

        /* Perform SQL rewrite if needed */
        if (statement_data->needs_rewrite || statement_data->where_needs_rewrite)
        {
            result = tecc_processor_rewrite_query(statement_data);
            if (result != TECC_PROCESSOR_SUCCESS)
            {
                /* Clean up before returning error */
                processor_cleanup_statement_data_internal(statement_data);
                return result;
            }
        }

        /* ReqID:TECC4.0 - If no rewrite is actually needed (e.g. SELECT with
         * encrypted columns in target list only, no WHERE/expression encryption),
         * return SUCCESS so original SQL is sent as-is. */
        if (!statement_data->needs_rewrite && !statement_data->where_needs_rewrite)
        {
            
            return TECC_PROCESSOR_SUCCESS;
        }

        /* Note: We don't clean up parse tree here because it might be needed
         * for further processing (e.g., SQL rewrite). The caller is responsible
         * for calling tecc_parser_free_tree() after all processing is complete.
         */
        return TECC_PROCESSOR_NEEDS_REWRITE;
    }

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    /* Check for pre-rewritten queries (e.g., CREATE DEK) */
    tecc_processor_debug(TECC_LOG_DEBUG1,
        "DEBUG: needs_rewrite=%d, rewritten_query=%p, encrypted_columns=%p",
        statement_data->needs_rewrite, statement_data->rewritten_query, statement_data->encrypted_columns);
    if (statement_data->needs_rewrite)
    {
        /* ReqID:TECC4.0 - Save statement type for post-processing cache refresh */
        if (state != NULL) {
            state->last_stmt_type = statement_data->stmt_type;
            if (statement_data->stmt_type == TECC_STMT_TYPE_DROP) {
                state->last_drop_remove_type = statement_data->drop_remove_type;
                state->last_drop_behavior = statement_data->drop_behavior;
                state->last_drop_key_oid = statement_data->drop_key_oid;
            }
            /* ReqID:TECC4.0 - Save table identification for post-DDL cache refresh */
            if (statement_data->stmt_type == TECC_STMT_TYPE_CREATE_TABLE ||
                statement_data->stmt_type == TECC_STMT_TYPE_ALTER_TABLE)
            {
                if (statement_data->encrypted_columns != NIL)
                {
                    TeccColumnRefData *first = (TeccColumnRefData *)linitial(
                        statement_data->encrypted_columns);
                    if (first != NULL)
                    {
                        strncpy(state->last_table_schema, first->schema_name,
                                sizeof(state->last_table_schema) - 1);
                        state->last_table_schema[sizeof(state->last_table_schema) - 1] = '\0';
                        strncpy(state->last_table_name, first->table_name,
                                sizeof(state->last_table_name) - 1);
                        state->last_table_name[sizeof(state->last_table_name) - 1] = '\0';
                        /* ReqID:TECC4.0 - Insert PENDING sentinel */
                        {
                            const char *db = PQdb(statement_data->conn);
                            const char *schema = first->schema_name;
                            if (db == NULL) db = "postgres";
                            if (schema == NULL || schema[0] == '\0')
                            {
                                schema = tecc_cache_get_guc_param(state, "current_schema");
                                if (schema == NULL || schema[0] == '\0')
                                    schema = "public";
                            }
                            tecc_cache_insert_pending(state, db, schema,
                                                      first->table_name);
                        }
                    }
                }
            }
        }
        return TECC_PROCESSOR_NEEDS_REWRITE;
    }
    /* Clean up parse tree since no further processing is needed */
    /* ReqID:TECC4.0 - Save statement type for post-processing cache refresh */
    if (state != NULL) {
        state->last_stmt_type = statement_data->stmt_type;
        if (statement_data->stmt_type == TECC_STMT_TYPE_DROP) {
            state->last_drop_remove_type = statement_data->drop_remove_type;
            state->last_drop_behavior = statement_data->drop_behavior;
            state->last_drop_key_oid = statement_data->drop_key_oid;
        }
    }

    processor_cleanup_statement_data_internal(statement_data);

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "DEBUG: tecc_processor_run_pre_query completed with result: %d",
                       TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS);

    return TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS;
}

/*
 * Run pre-execution processing
 */
TeccProcessorResult
tecc_processor_run_pre_exec(TeccStatementData *statement_data)
{
    TeccState *state;

    if (statement_data == NULL)
        return TECC_PROCESSOR_INVALID_INPUT;

    /* Get TECC state from connection */
    if (statement_data->conn == NULL)
    {
        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                                 "No connection in statement data");
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
        return TECC_PROCESSOR_INVALID_INPUT;
    }

    state = tecc_get_state(statement_data->conn);
    if (state == NULL)
    {
        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                                 "Failed to get TECC state from connection");
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
        return TECC_PROCESSOR_ERROR;
    }

    if (!state->processor_initialized)
    {
        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                                 "Processor not initialized for this connection DEBUG");
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
        return TECC_PROCESSOR_ERROR;
    }

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                             "Starting pre-execution processing");
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    /* Process parameters if present */
    if (statement_data->nParams > 0 && statement_data->paramValues != NULL)
    {
        TeccValuesProcessorResult result;

        /* Call values processor for parameter encryption */
        result = tecc_values_processor_process_parameters(statement_data);
        if (result != TECC_VALUES_PROCESSOR_SUCCESS)
        {
            /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
            tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                                     "Values processor failed to process parameters: %d", result);
            /* End - ReqID:TECC4.0 - 线程安全调试支持 */
            return TECC_PROCESSOR_ERROR;
        }

        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                 "Processed %zu parameters successfully", statement_data->nParams);
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
    }

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    return TECC_PROCESSOR_SUCCESS;
}

/*
 * Run post-query processing
 */
TeccProcessorResult
tecc_processor_run_post_query(PGconn *conn, PGresult *result)
{
    int nfields;
    int ntuples;
    int field;
    TeccState *state;
    /* ReqID:TECC4.0 - Track decrypt failures to detect permission errors */
    bool decrypt_failed;

    if (conn == NULL || result == NULL)
        return TECC_PROCESSOR_INVALID_INPUT;

    /* Get TECC state from connection */
    state = tecc_get_state(conn);
    if (state == NULL)
    {
        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                                 "Failed to get TECC state from connection");
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
        return TECC_PROCESSOR_ERROR;
    }

    if (!state->processor_initialized)
    {
        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                                 "Processor not initialized for this connection DEBUG");
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
        return TECC_PROCESSOR_ERROR;
    }

    /* Check if TECC is enabled (considering disable_once flag) */
    if (!tecc_is_enabled(state))
    {
        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                 "TECC is disabled (possibly due to disable_once flag), skipping post-query processing");
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
        return TECC_PROCESSOR_SUCCESS; /* Return success to allow query to proceed without TECC processing */
    }

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                            "Starting post-query processing");
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    /* Call post-statement processor if needed */
    {
        /* Create a minimal statement data structure for postprocessing */
        TeccStatementData stmt_data;
        TeccPostStmtProcessorResult post_result;

        memset(&stmt_data, 0, sizeof(TeccStatementData));
        stmt_data.conn = conn;

        /* ReqID:TECC4.0 - Restore statement type from pre-processing state */
        {
            TeccState *pstate = tecc_get_state(conn);
            if (pstate != NULL) {
                stmt_data.stmt_type = pstate->last_stmt_type;
                if (pstate->last_stmt_type == TECC_STMT_TYPE_DROP) {
                    stmt_data.drop_remove_type = pstate->last_drop_remove_type;
                    stmt_data.drop_behavior = pstate->last_drop_behavior;
                    stmt_data.drop_key_oid = pstate->last_drop_key_oid;
                }
            }
        }

        /* Call post-statement processor */
        post_result = tecc_post_stmt_processor_run(
            conn, &stmt_data, result);

        if (post_result != TECC_POST_STMT_PROCESSOR_SUCCESS)
        {
            /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
            tecc_processor_debug_safe(NULL, TECC_LOG_WARNING,
                                     "Post-statement processing returned non-success: %d",
                                     post_result);
            /* End - ReqID:TECC4.0 - 线程安全调试支持 */
            /* Continue with normal post-query processing even if post-statement processing failed */
        }

        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                 "DEBUG: Post-statement processor completed, result=%d", post_result);
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
    }

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                             "DEBUG: Starting normal post-query processing");
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    /* ReqID:TECC4.0 - Initialize decrypt failure flag */
    decrypt_failed = false;

    /* Get basic result information */
    nfields = PQnfields(result);
    ntuples = PQntuples(result);

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    /* Check if there are any fields to process */
    if (nfields <= 0)
    {
        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                 "No fields in result, skipping processing");
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
        return TECC_PROCESSOR_SUCCESS;
    }

    /* Identify encrypted columns */
    {
        int encrypted_count = 0;
        int *encrypted_fields = NULL;
        Oid *field_table_oids = NULL;
        int *field_column_attnums = NULL;

        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                 "DEBUG: Allocating encrypted fields array for %d fields", nfields);
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */

        /* Allocate arrays for field tracking */
        encrypted_fields = (int *) pg_malloc(nfields * sizeof(int));
        field_table_oids = (Oid *) pg_malloc0(nfields * sizeof(Oid));
        field_column_attnums = (int *) pg_malloc0(nfields * sizeof(int));
        if (encrypted_fields == NULL || field_table_oids == NULL || field_column_attnums == NULL)
        {
            /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
            tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                                     "Failed to allocate field tracking arrays");
            /* End - ReqID:TECC4.0 - 线程安全调试支持 */
            if (encrypted_fields) pg_free(encrypted_fields);
            if (field_table_oids) pg_free(field_table_oids);
            if (field_column_attnums) pg_free(field_column_attnums);
            return TECC_PROCESSOR_MEMORY_ERROR;
        }

        memset(encrypted_fields, 0, nfields * sizeof(int));

        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                 "DEBUG: Starting field analysis loop for %d fields", nfields);
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */

        for (field = 0; field < nfields; field++)
        {
            const char *field_name = PQfname(result, field);
            Oid field_type = PQftype(result, field);

            /* Check if this is an encrypted type */
            if (field_type >= INT2_ENCOID && field_type <= TEXT_ENCARRAYOID)
            {
                encrypted_fields[field] = 1;
                encrypted_count++;
                field_table_oids[field] = PQftable(result, field);
                field_column_attnums[field] = PQftablecol(result, field);

                /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
                tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                         "Field %d: name='%s', type=%u (ENCRYPTED) table_oid=%u attnum=%d",
                                         field, field_name ? field_name : "(null)", field_type,
                                         field_table_oids[field], field_column_attnums[field]);
                /* End - ReqID:TECC4.0 - 线程安全调试支持 */
            }
            else
            {
                /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
                tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                         "Field %d: name='%s', type=%u",
                                         field, field_name ? field_name : "(null)", field_type);
                /* End - ReqID:TECC4.0 - 线程安全调试支持 */
            }
        }

        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */

        /* If there are encrypted fields, process rows in batch chunks */
        if (encrypted_count > 0 && ntuples > 0)
        {
#define TECC_DECRYPT_BATCH_SIZE 1000
            int chunk_start;
            const char **field_tbl_names;
            const char **field_col_names;
            const char **field_schema_names;
            Oid *field_output_oids;
            int *field_type_mods;

            /* Pre-compute per-field metadata (shared across all rows) */
            field_tbl_names = pg_malloc(nfields * sizeof(char *));
            field_col_names = pg_malloc(nfields * sizeof(char *));
            field_schema_names = pg_malloc(nfields * sizeof(char *));
            field_output_oids = pg_malloc(nfields * sizeof(Oid));
            field_type_mods = pg_malloc(nfields * sizeof(int));
            if (!field_tbl_names || !field_col_names || !field_schema_names ||
                !field_output_oids || !field_type_mods)
            {
                if (field_tbl_names) pg_free(field_tbl_names);
                if (field_col_names) pg_free(field_col_names);
                if (field_schema_names) pg_free(field_schema_names);
                if (field_output_oids) pg_free(field_output_oids);
                if (field_type_mods) pg_free(field_type_mods);
                pg_free(encrypted_fields);
                pg_free(field_table_oids);
                pg_free(field_column_attnums);
                return TECC_PROCESSOR_MEMORY_ERROR;
            }

            for (field = 0; field < nfields; field++)
            {
                if (encrypted_fields[field])
                {
                    TeccColumnCacheItem *cache_item;

                    cache_item = NULL;
                    if (field_table_oids[field] != InvalidOid)
                        cache_item = tecc_cache_get_encrypted_column(state,
                            field_table_oids[field], field_column_attnums[field]);

                    if (cache_item != NULL)
                    {
                        field_schema_names[field] = cache_item->schema_name;
                        field_tbl_names[field] = cache_item->table_name;
                        field_col_names[field] = cache_item->column_name;
                        field_output_oids[field] = cache_item->original_type_oid;
                        field_type_mods[field] = cache_item->original_type_mod;
                    }
                    else
                    {
                        field_schema_names[field] = NULL;
                        field_tbl_names[field] = "unknown";
                        field_col_names[field] = PQfname(result, field);
                        field_output_oids[field] = PQftype(result, field);
                        field_type_mods[field] = -1;
                    }

                    /* Map encrypted OID to output OID once per field */
                    switch (field_output_oids[field])
                    {
                        case INT2_ENC_OID: field_output_oids[field] = TECC_OUTPUT_INT2OID; break;
                        case INT4_ENC_OID: field_output_oids[field] = TECC_OUTPUT_INT4OID; break;
                        case INT8_ENC_OID: field_output_oids[field] = TECC_OUTPUT_INT8OID; break;
                        case FLOAT4_ENC_OID: field_output_oids[field] = TECC_OUTPUT_FLOAT4OID; break;
                        case FLOAT8_ENC_OID: field_output_oids[field] = TECC_OUTPUT_FLOAT8OID; break;
                        case NUMERIC_ENC_OID: field_output_oids[field] = TECC_OUTPUT_NUMERICOID; break;
                        case VARCHAR_ENC_OID: field_output_oids[field] = TECC_OUTPUT_VARCHAROID; break;
                        case BPCHAR_ENC_OID: field_output_oids[field] = TECC_OUTPUT_BPCHAROID; break;
                        case TEXT_ENC_OID: field_output_oids[field] = TECC_OUTPUT_TEXTOID; break;
                        default: break;
                    }
                }
                else
                {
                    field_schema_names[field] = NULL;
                    field_tbl_names[field] = NULL;
                    field_col_names[field] = NULL;
                    field_output_oids[field] = InvalidOid;
                    field_type_mods[field] = 0;
                }
            }

            /* Process rows in chunks */
            for (chunk_start = 0; chunk_start < ntuples;
                 chunk_start += TECC_DECRYPT_BATCH_SIZE)
            {
                int chunk_size;
                int chunk_row;

                chunk_size = ntuples - chunk_start;
                if (chunk_size > TECC_DECRYPT_BATCH_SIZE)
                    chunk_size = TECC_DECRYPT_BATCH_SIZE;

                for (field = 0; field < nfields; field++)
                {
                    if (!encrypted_fields[field])
                        continue;

                    {
                        const char **hex_batch;
                        int *batch_rows;
                        int batch_count;
                        int row;
                        int k;

                        hex_batch = pg_malloc(sizeof(char *) * chunk_size);
                        batch_rows = pg_malloc(sizeof(int) * chunk_size);
                        if (!hex_batch || !batch_rows)
                        {
                            if (hex_batch) pg_free(hex_batch);
                            if (batch_rows) pg_free(batch_rows);
                            pg_free(field_schema_names);
                            pg_free(field_tbl_names);
                            pg_free(field_col_names);
                            pg_free(field_output_oids);
                            pg_free(field_type_mods);
                            pg_free(encrypted_fields);
                            pg_free(field_table_oids);
                            pg_free(field_column_attnums);
                            return TECC_PROCESSOR_MEMORY_ERROR;
                        }
                        batch_count = 0;

                        /* Collect non-null hex strings for this field */
                        for (chunk_row = 0; chunk_row < chunk_size; chunk_row++)
                        {
                            const char *encrypted_value;

                            row = chunk_start + chunk_row;
                            encrypted_value = PQgetvalue(result, row, field);

                            if (!PQgetisnull(result, row, field) &&
                                encrypted_value != NULL)
                            {
                                const char *hex_input;

                                hex_input = encrypted_value;
                                if (hex_input[0] == '\\' && hex_input[1] == 'x')
                                    hex_input += 2;

                                hex_batch[batch_count] = hex_input;
                                batch_rows[batch_count] = row;
                                batch_count++;
                            }
                        }

                        if (batch_count > 0)
                        {
                            TeccUnifiedDecryptResult **batch_results;

                            batch_results = tecc_unified_batch_decrypt_hex(
                                conn, hex_batch, batch_count,
                                field_schema_names[field],
                                field_tbl_names[field],
                                field_col_names[field],
                                state->crypto_ctx);

                            if (batch_results != NULL)
                            {
                                for (k = 0; k < batch_count; k++)
                                {
                                    TeccUnifiedDecryptResult *dr;
                                    int tgt_row;

                                    dr = batch_results[k];
                                    tgt_row = batch_rows[k];

                                    if (dr != NULL &&
                                        dr->error_code == TECC_CRYPTO_SUCCESS &&
                                        dr->plaintext != NULL)
                                    {
                                        if (dr->skipped_decrypt)
                                        {
                                            PQsetvalue((PGresult *)result,
                                                       tgt_row, field,
                                                       dr->plaintext,
                                                       (int)dr->plain_len);
                                        }
                                        else if (dr->plain_len == 0)
                                        {
                                            PQsetvalue((PGresult *)result,
                                                       tgt_row, field,
                                                       "", 0);
                                        }
                                        else
                                        {
                                            char *output_text;

                                            output_text = tecc_type_output_data(
                                                field_output_oids[field],
                                                field_type_mods[field],
                                                dr->plaintext,
                                                dr->plain_len);
                                            if (output_text != NULL)
                                            {
                                                PGresAttValue *attval;
                                                int text_len;

                                                attval = &result->tuples[tgt_row][field];
                                                text_len = (int)strlen(output_text);

                                                if (text_len > attval->len)
                                                {
                                                    TECC_LOG(TECC_LOG_WARNING,
                                                             "TECC: in-place buffer overflow prevented: "
                                                             "text=%d bytes > hex_buffer=%d bytes, field=%s",
                                                             text_len, attval->len,
                                                             field_col_names[field]);
                                                    PQsetvalue((PGresult *)result,
                                                               tgt_row, field,
                                                               output_text, text_len);
                                                }
                                                else
                                                {
                                                    memcpy(attval->value, output_text,
                                                           text_len);
                                                    attval->value[text_len] = '\0';
                                                    attval->len = text_len;
                                                }
                                                pg_free(output_text);
                                            }
                                        }
                                    }
                                    else
                                    {
                                        const char *err_msg;

                                        err_msg = (dr != NULL)
                                            ? dr->error_msg
                                            : "batch decrypt failed";
                                        {
                                            char err_text[256];

                                            snprintf(err_text, sizeof(err_text),
                                                     "ERROR: %s", err_msg);
                                            PQsetvalue((PGresult *)result,
                                                       tgt_row, field,
                                                       err_text,
                                                       strlen(err_text));
                                        }
                                        if (state != NULL)
                                        {
                                            strncpy(state->last_error.error_msg,
                                                    err_msg,
                                                    sizeof(state->last_error.error_msg) - 1);
                                            state->last_error.error_msg[
                                                sizeof(state->last_error.error_msg) - 1] = '\0';
                                            state->last_error.error_code = TECC_ERR_PROCESSOR;
                                            state->last_error.timestamp = time(NULL);
                                            state->last_error.recoverable = false;
                                        }
                                        decrypt_failed = true;
                                    }

                                    if (dr != NULL)
                                        tecc_unified_free_decrypt_result(dr);
                                }
                                pg_free(batch_results);
                            }
                            else
                            {
                                /* Catastrophic batch failure:
                                 * mark all items in batch as errors */
                                for (k = 0; k < batch_count; k++)
                                {
                                    int tgt_row;

                                    tgt_row = batch_rows[k];
                                    PQsetvalue((PGresult *)result,
                                               tgt_row, field,
                                               "ERROR: batch decrypt failed", 26);
                                }
                                if (state != NULL)
                                {
                                    strncpy(state->last_error.error_msg,
                                            "batch decrypt returned NULL",
                                            sizeof(state->last_error.error_msg) - 1);
                                    state->last_error.error_msg[
                                        sizeof(state->last_error.error_msg) - 1] = '\0';
                                    state->last_error.error_code = TECC_ERR_PROCESSOR;
                                    state->last_error.timestamp = time(NULL);
                                    state->last_error.recoverable = false;
                                }
                                decrypt_failed = true;
                            }
                        }

                        pg_free(hex_batch);
                        pg_free(batch_rows);
                    }
                }
            }

            pg_free(field_schema_names);
            pg_free(field_tbl_names);
            pg_free(field_col_names);
            pg_free(field_output_oids);
            pg_free(field_type_mods);
#undef TECC_DECRYPT_BATCH_SIZE
        }

        /* Free field tracking arrays */
        pg_free(encrypted_fields);
        pg_free(field_table_oids);
        pg_free(field_column_attnums);
    }

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    /* ReqID:TECC4.0 - Return error if any decryption failure occurred (e.g. permission denied) */
    if (decrypt_failed)
        return TECC_PROCESSOR_ERROR;

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                             "DEBUG: Returning TECC_PROCESSOR_SUCCESS from tecc_processor_run_post_query");
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    return TECC_PROCESSOR_SUCCESS;
}

/*
 * Process specific statement type
 */
TeccProcessorResult
tecc_processor_run_pre_statement(const Node *stmt,
                                TeccStatementData *statement_data)
{
    return processor_process_statement(stmt, statement_data);
}

/*
 * Process SELECT statement
 */
TeccProcessorResult
tecc_processor_run_pre_select_statement(const SelectStmt *select_stmt,
                                       TeccStatementData *statement_data)
{
    TeccStmtProcessorResult result;

    if (select_stmt == NULL || statement_data == NULL)
        return TECC_PROCESSOR_INVALID_INPUT;

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                            "Processing SELECT statement");
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    /* Delegate to statement processor */
    result = tecc_stmt_processor_process_select(select_stmt, statement_data);

    return (result == TECC_STMT_PROCESSOR_SUCCESS) ? TECC_PROCESSOR_SUCCESS : TECC_PROCESSOR_ERROR;
}

/*
 * Process INSERT statement
 */
TeccProcessorResult
tecc_processor_run_pre_insert_statement(const InsertStmt *insert_stmt,
                                       TeccStatementData *statement_data)
{
    TeccStmtProcessorResult result;

    if (insert_stmt == NULL || statement_data == NULL)
        return TECC_PROCESSOR_INVALID_INPUT;

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                            "Processing INSERT statement");
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    /* Delegate to statement processor */
    result = tecc_stmt_processor_process_insert(insert_stmt, statement_data);

    return (result == TECC_STMT_PROCESSOR_SUCCESS) ? TECC_PROCESSOR_SUCCESS : TECC_PROCESSOR_ERROR;
}

/*
 * Process UPDATE statement
 */
TeccProcessorResult
tecc_processor_run_pre_update_statement(const UpdateStmt *update_stmt,
                                       TeccStatementData *statement_data)
{
    TeccStmtProcessorResult result;

    if (update_stmt == NULL || statement_data == NULL)
        return TECC_PROCESSOR_INVALID_INPUT;

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                            "Processing UPDATE statement");
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    /* Delegate to statement processor */
    result = tecc_stmt_processor_process_update(update_stmt, statement_data);

    return (result == TECC_STMT_PROCESSOR_SUCCESS) ? TECC_PROCESSOR_SUCCESS : TECC_PROCESSOR_ERROR;
}

/*
 * Process DELETE statement
 */
TeccProcessorResult
tecc_processor_run_pre_delete_statement(const DeleteStmt *delete_stmt,
                                       TeccStatementData *statement_data)
{
    TeccStmtProcessorResult result;

    if (delete_stmt == NULL || statement_data == NULL)
        return TECC_PROCESSOR_INVALID_INPUT;

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                            "Processing DELETE statement");
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    /* Delegate to statement processor */
    result = tecc_stmt_processor_process_delete(delete_stmt, statement_data);

    return (result == TECC_STMT_PROCESSOR_SUCCESS) ? TECC_PROCESSOR_SUCCESS : TECC_PROCESSOR_ERROR;
}

/*
 * Process CREATE KEK statement
 */
TeccProcessorResult
tecc_processor_run_pre_create_kek_statement(const CreateKekStmt *create_kek_stmt,
                                           TeccStatementData *statement_data)
{
    TeccStmtProcessorResult result;
    const char *kek_name;

    if (create_kek_stmt == NULL || statement_data == NULL)
        return TECC_PROCESSOR_INVALID_INPUT;

    /* Extract simple KEK name from qualified name list */
    kek_name = (create_kek_stmt->names != NIL) ?
        strVal(llast(create_kek_stmt->names)) : NULL;

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    /* Delegate to statement processor */
    result = tecc_stmt_processor_process_create_kek(create_kek_stmt, statement_data);

    if (result != TECC_STMT_PROCESSOR_SUCCESS)
        return TECC_PROCESSOR_ERROR;

    /*
     * ReqID:TECC4.0 - Inject _TECC_PROCESSED and _KEK_FILE_CHECKSUM markers
     * into the WITH options.
     *
     * The parse tree already guarantees this is a CREATE KEK statement.
     * We locate the KEK name in the query text (known from the parse tree),
     * then find the WITH clause after it to inject the markers.
     *
     * _KEK_FILE_CHECKSUM is an SM3 hash of the key file contents computed
     * by the frontend, used by the backend for encrypted _KEK_META storage.
     */
    if (!tecc_stmt_needs_rewrite(statement_data) && statement_data->query != NULL &&
        kek_name != NULL)
    {
        const char *q = statement_data->query;
        const char *name_pos;
        const char *with_pos;
        const char *paren_start;
        const char *paren_end;
        int depth;
        size_t prefix_len, suffix_len, new_len;
        char *new_query;
        char marker[512];
        char checksum_hex[65];
        const char *key_path = NULL;
        ListCell *opt_cell;

        /* Extract key_path from the CREATE KEK options */
        foreach(opt_cell, create_kek_stmt->options)
        {
            DefElem *def = (DefElem *) lfirst(opt_cell);
            if (def != NULL && def->defname != NULL &&
                strcasecmp(def->defname, "key_path") == 0)
            {
                if (def->arg != NULL)
                {
                    if (nodeTag(def->arg) == FE_T_String)
                        key_path = strVal((Value *)def->arg);
                    else if (nodeTag(def->arg) == FE_T_Const)
                        key_path = ((Const *)def->arg)->constvalue.sval;
                }
                break;
            }
        }

        /* Compute SM3 checksum of the key file */
        checksum_hex[0] = '\0';
        if (key_path != NULL)
        {
            FILE *f = fopen(key_path, "rb");
            if (f != NULL)
            {
                unsigned char *buf = NULL;
                long file_len;
                unsigned char digest[32];
                int i;

                fseek(f, 0, SEEK_END);
                file_len = ftell(f);
                fseek(f, 0, SEEK_SET);

                if (file_len > 0 && file_len < (1024 * 1024))
                {
                    buf = (unsigned char *)pg_malloc(file_len);
                    if (fread(buf, 1, file_len, f) == (size_t)file_len)
                    {
                        tecc_sm3_hash(buf, file_len, digest);
                        for (i = 0; i < 32; i++)
                            sprintf(checksum_hex + i * 2, "%02x", digest[i]);
                        checksum_hex[64] = '\0';
                    }
                    pg_free(buf);
                }
                fclose(f);
            }
            if (checksum_hex[0] == '\0')
            {
                tecc_processor_debug_safe(NULL, TECC_LOG_WARNING,
                                        "CREATE KEK: failed to compute SM3 checksum for '%s'",
                                        key_path);
            }
        }

        /* Build combined marker string */
        if (checksum_hex[0] != '\0')
        {
            snprintf(marker, sizeof(marker),
                     ", _TECC_PROCESSED = 'yes', _KEK_FILE_CHECKSUM = '%s'",
                     checksum_hex);
        }
        else
        {
            snprintf(marker, sizeof(marker),
                     ", _TECC_PROCESSED = 'yes'");
        }

        /*
         * Locate the KEK name in the query text. The parse tree gives us
         * the exact name, so find that string after "CREATE KEK" to get
         * a reliable anchor point, then search for WITH from there.
         */
        name_pos = strstr(q, kek_name);
        if (name_pos == NULL)
        {
            tecc_processor_debug_safe(NULL, TECC_LOG_WARNING,
                                    "CREATE KEK: kek_name '%s' not found in query",
                                    kek_name);
        }
        else
        {
            /* Search for WITH starting from after the KEK name.
             * Must skip past the KEK name bytes first, otherwise a KEK name
             * containing "WITH" (e.g. "kekWITHhighgo") would match inside
             * the identifier rather than at the actual WITH keyword. */
            with_pos = NULL;
            {
                const char *p;
                for (p = name_pos + strlen(kek_name); *p; p++)
                {
                    if (strncasecmp(p, "WITH", 4) == 0)
                    {
                        with_pos = p;
                        break;
                    }
                }
            }

            if (with_pos != NULL)
            {
                paren_start = with_pos + 4;
                while (*paren_start == ' ' || *paren_start == '\t' ||
                       *paren_start == '\n' || *paren_start == '\r')
                    paren_start++;

                if (*paren_start == '(')
                {
                    depth = 1;
                    paren_end = paren_start + 1;
                    while (*paren_end && depth > 0)
                    {
                        if (*paren_end == '(')
                            depth++;
                        else if (*paren_end == ')')
                            depth--;
                        if (depth > 0)
                            paren_end++;
                    }

                    if (depth == 0 && *paren_end == ')')
                    {
                        prefix_len = paren_end - q;
                        suffix_len = strlen(q) - prefix_len;
                        new_len = prefix_len + strlen(marker) + suffix_len + 1;
                        new_query = (char *)pg_malloc(new_len);

                        memcpy(new_query, q, prefix_len);
                        memcpy(new_query + prefix_len, marker, strlen(marker));
                        memcpy(new_query + prefix_len + strlen(marker), paren_end, suffix_len + 1);

                        statement_data->rewritten_query = new_query;
                        tecc_stmt_set_needs_rewrite(statement_data, true);

                        tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                                "Injected markers into KEK '%s' (checksum: %s)",
                                                kek_name, checksum_hex);
                    }
                }
            }
            else
            {
                tecc_processor_debug_safe(NULL, TECC_LOG_WARNING,
                                        "CREATE KEK: WITH not found after kek_name '%s'",
                                        kek_name);
            }
        }
    }

    /* If the statement was rewritten, return NEEDS_REWRITE so caller uses rewritten_query */
    if (tecc_stmt_needs_rewrite(statement_data))
    {
        tecc_processor_debug(TECC_LOG_DEBUG1, "REWRITTEN_SQL: %s", statement_data->rewritten_query);
        return TECC_PROCESSOR_NEEDS_REWRITE;
    }

    return TECC_PROCESSOR_SUCCESS;
}

/*
 * Process CREATE DEK statement
 */
TeccProcessorResult
tecc_processor_run_pre_create_dek_statement(const CreateDekStmt *create_dek_stmt,
                                           TeccStatementData *statement_data)
{
    TeccStmtProcessorResult result;
    ListCell *lc;

    if (create_dek_stmt == NULL || statement_data == NULL)
        return TECC_PROCESSOR_INVALID_INPUT;

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                            "Processing CREATE DEK statement");
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    /*
     * If _TECC_PROCESSED is already present in the options, the SQL is
     * already fully processed (ENCRYPTED_VALUE, etc. are provided).
     * Skip frontend processing and pass through to the server as-is.
     * The stmt_processor would fail anyway because the KEK cache hasn't
     * been populated by the pass-through CREATE KEK flow.
     */
    foreach(lc, create_dek_stmt->options)
    {
        DefElem *opt = (DefElem *) lfirst(lc);
        if (pg_strcasecmp(opt->defname, "_tecc_processed") == 0)
        {
            tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                     "CREATE DEK already has _TECC_PROCESSED, passing through");
            return TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS;
        }
    }

    /* Delegate to statement processor */
    result = tecc_stmt_processor_process_create_dek(create_dek_stmt, statement_data);

    if (result != TECC_STMT_PROCESSOR_SUCCESS)
        return TECC_PROCESSOR_ERROR;

    /* If the statement was rewritten, return NEEDS_REWRITE so caller uses rewritten_query */
    if (tecc_stmt_needs_rewrite(statement_data))
    {
        tecc_processor_debug(TECC_LOG_DEBUG1, "REWRITTEN_SQL: %s", statement_data->rewritten_query);
        return TECC_PROCESSOR_NEEDS_REWRITE;
    }

    return TECC_PROCESSOR_SUCCESS;
}

/*
 * Process DROP statement
 */
TeccProcessorResult
tecc_processor_run_pre_drop_statement(const DropStmt *drop_stmt,
                                     TeccStatementData *statement_data)
{
    TeccStmtProcessorResult result;

    if (drop_stmt == NULL || statement_data == NULL)
        return TECC_PROCESSOR_INVALID_INPUT;

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                            "Processing DROP statement");
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    /* Delegate to statement processor */
    result = tecc_stmt_processor_process_drop(drop_stmt, statement_data);

    return (result == TECC_STMT_PROCESSOR_SUCCESS) ? TECC_PROCESSOR_SUCCESS : TECC_PROCESSOR_ERROR;
}

/*
 * Process COPY statement
 */
TeccProcessorResult
tecc_processor_run_pre_copy_statement(const CopyStmt *copy_stmt,
                                     TeccStatementData *statement_data)
{
    TeccStmtProcessorResult result;

    if (copy_stmt == NULL || statement_data == NULL)
        return TECC_PROCESSOR_INVALID_INPUT;

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                            "Processing COPY statement");
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    /* Delegate to statement processor */
    result = tecc_stmt_processor_process_copy(copy_stmt, statement_data);

    return (result == TECC_STMT_PROCESSOR_SUCCESS) ? TECC_PROCESSOR_SUCCESS : TECC_PROCESSOR_ERROR;
}

/*
 * Check if statement contains encrypted columns
 */
int
tecc_processor_has_encrypted_columns(TeccStatementData *statement_data)
{
    if (statement_data == NULL)
        return 0;

    return (statement_data->encrypted_columns != NULL) ? 1 : 0;
}

/*
 * Get list of encrypted column names
 */
List *
tecc_processor_get_encrypted_columns(TeccStatementData *statement_data)
{
    if (statement_data == NULL)
        return NULL;

    return statement_data->encrypted_columns;
}

/*
 * Process statement (legacy function)
 */
List *
tecc_process_statement(TeccStatementData *stmt_data)
{
    if (stmt_data == NULL)
        return NIL;

    /* Run pre-query processing */
    if (tecc_processor_run_pre_query(stmt_data) != TECC_PROCESSOR_SUCCESS)
        return NIL;

    return stmt_data->encrypted_columns;
}

/*
 * Set processor debug mode (deprecated, not thread-safe)
 * Note: This sets module-level debug level which is not thread-safe.
 * For connection-specific debug level, use tecc_state_set_debug() instead.
 *
 * WARNING: This function is not thread-safe. The debug level is shared by all connections.
 * This function is deprecated and may be removed in future versions.
 *
 * UPDATE: Now uses environment variable TECC_LOG_LEVEL for control.
 */
void
tecc_processor_set_debug(int enable)
{
    /* Begin - ReqID:TECC4.0 - 环境变量控制调试级别 */

    /* 输出废弃警告 */
    TECC_LOG(TECC_LOG_WARNING, "tecc_processor_set_debug() is deprecated. Use TECC_LOG_LEVEL env var instead");

    /* 全局调试级别变量已移除，使用环境变量控制 */
    /* 旧代码：tecc_processor_debug_level = enable ? TECC_LOG_DEBUG1 : TECC_LOG_ERROR; */

    /* End - ReqID:TECC4.0 - 环境变量控制调试级别 */
}

/*
 * Set connection-specific debug level (thread-safe version)
 * This function sets debug level in the TeccState structure associated with the connection.
 */
void
tecc_state_set_debug(PGconn *conn, int enable)
{
    TeccState *state = NULL;

    /* Validate connection parameter */
    if (!conn)
    {
        TECC_LOG(TECC_LOG_ERROR, "NULL connection passed to tecc_state_set_debug");
        return;
    }

    /* Get TECC state from connection */
    state = tecc_get_state(conn);
    if (!state)
    {
        TECC_LOG(TECC_LOG_ERROR, "Failed to get TECC state from connection");
        return;
    }

    /* Set debug level in connection-specific configuration */
    state->config.debug_level = enable ? TECC_LOG_DEBUG1 : TECC_LOG_ERROR;

    /* Log the change if debug is enabled */
    if (state->config.debug_level >= TECC_LOG_INFO)
    {
        TECC_LOG(TECC_LOG_INFO, "Connection-specific debug level set to %d",
                state->config.debug_level);
    }
}

/*
 * Get processor statistics
 * Note: This function returns module-level statistics (always 0).
 * For connection-specific statistics, use tecc_get_stats() instead.
 */
void
tecc_processor_get_stats(int *processed_statements, int *error_count)
{
    if (processed_statements != NULL)
        *processed_statements = 0;
    if (error_count != NULL)
        *error_count = 0;
}

/*
 * Interface functions for SQL rewriter
 */
bool
tecc_stmt_has_encrypted_columns(TeccStatementData *stmt_data)
{
    return (stmt_data != NULL && stmt_data->encrypted_columns != NULL);
}

int
tecc_stmt_get_encrypted_column_count(TeccStatementData *stmt_data)
{
    if (stmt_data == NULL || stmt_data->encrypted_columns == NULL)
        return 0;
    return tecc_processor_list_length(stmt_data->encrypted_columns);
}

bool
tecc_stmt_needs_rewrite(TeccStatementData *stmt_data)
{
    return (stmt_data != NULL && stmt_data->needs_rewrite != 0);
}

void
tecc_stmt_set_needs_rewrite(TeccStatementData *stmt_data, bool needs_rewrite)
{
    if (stmt_data != NULL)
        stmt_data->needs_rewrite = needs_rewrite ? 1 : 0;
}

/*
 * Forward declaration for CREATE TABLE DEFAULT handler
 */
static char *tecc_processor_handle_create_table_defaults(CreateStmt *create_stmt,
                                                         TeccStatementData *statement_data);

/*
 * Internal helper functions
 */
static TeccProcessorResult
processor_process_statement(const Node *stmt, TeccStatementData *statement_data)
{
    int result;

    if (stmt == NULL || statement_data == NULL)
        return TECC_PROCESSOR_INVALID_INPUT;

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    /* Route to appropriate statement processor */
    switch (nodeTag(stmt))
    {
        case FE_T_SelectStmt:
        {
            /* Always route SELECT to the proper SELECT processor.
             * Do NOT route to CTAS handler — it can't handle CollateClause
             * wrapping, ignores support_algo, and breaks double TypeCast chains.
             * tecc_stmt_processor_process_select calls
             * process_insert_select_typecast which handles all these correctly. */
            result = tecc_stmt_processor_process_select(
                (SelectStmt *)stmt, statement_data);
            break;
        }
            break;

        case FE_T_SetOp:
        {
            /* SetOperationStmt (UNION/INTERSECT/EXCEPT) — process recursively */
            SetOperationStmt *setop = (SetOperationStmt *) stmt;
            TeccProcessorResult lresult = TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS;
            TeccProcessorResult rresult = TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS;

            if (setop->larg != NULL)
            {
                lresult = processor_process_statement(setop->larg, statement_data);
                if (lresult != TECC_PROCESSOR_SUCCESS &&
                    lresult != TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS &&
                    lresult != TECC_PROCESSOR_NEEDS_REWRITE)
                    return lresult;
            }
            if (setop->rarg != NULL)
            {
                rresult = processor_process_statement(setop->rarg, statement_data);
                if (rresult != TECC_PROCESSOR_SUCCESS &&
                    rresult != TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS &&
                    rresult != TECC_PROCESSOR_NEEDS_REWRITE)
                    return rresult;
            }

            result = (lresult == TECC_PROCESSOR_NEEDS_REWRITE ||
                      rresult == TECC_PROCESSOR_NEEDS_REWRITE)
                     ? TECC_PROCESSOR_NEEDS_REWRITE
                     : (lresult == TECC_PROCESSOR_SUCCESS ||
                        rresult == TECC_PROCESSOR_SUCCESS)
                       ? TECC_PROCESSOR_SUCCESS
                       : TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS;
            break;
        }

        case FE_T_InsertStmt:
            
            result = tecc_stmt_processor_process_insert((InsertStmt *) stmt, statement_data);
            
            break;

        case FE_T_UpdateStmt:
            result = tecc_stmt_processor_process_update((UpdateStmt *) stmt, statement_data);
            break;

        case FE_T_DeleteStmt:
            result = tecc_stmt_processor_process_delete((DeleteStmt *) stmt, statement_data);
            break;

        case FE_T_CreateKekStmt:  /* TECC自定义标签 - 值为100 */
            statement_data->stmt_type = TECC_STMT_TYPE_CREATE_KEK;
            result = tecc_processor_run_pre_create_kek_statement((CreateKekStmt *) stmt, statement_data);
            break;

        case FE_T_CreateDekStmt:  /* TECC自定义标签 - 值为101 */
            statement_data->stmt_type = TECC_STMT_TYPE_CREATE_DEK;
            result = tecc_processor_run_pre_create_dek_statement((CreateDekStmt *) stmt, statement_data);
            break;

        case FE_T_DropStmt:  /* 102 - TECC自定义标签 */
            statement_data->stmt_type = TECC_STMT_TYPE_DROP;
            result = tecc_stmt_processor_process_drop((DropStmt *) stmt, statement_data);
            break;

        case FE_T_CreateStmt:  /* 254 - TECC前端标签 */
            {
                TeccCreateStmtProcessorResult create_result;
                create_result = tecc_create_stmt_processor_run((const CreateStmt *) stmt,
                                                               statement_data);
                statement_data->stmt_type = TECC_STMT_TYPE_CREATE_TABLE;
                switch (create_result)
                {
                    case TECC_CREATE_PROCESSOR_SUCCESS:
                        /* ReqID:TECC4.0 - DEFAULT encryption handled in
                         * tecc_create_stmt_processor_run() above. If rewritten_query
                         * is already set, use it; otherwise try legacy handler. */
                        if (statement_data->rewritten_query != NULL)
                        {
                            statement_data->needs_rewrite = 1;
                            result = TECC_PROCESSOR_NEEDS_REWRITE;
                        }
                        else
                        {
                            char *defaults_rewritten;
                            TeccState *pending_state = NULL;  /* ReqID:TECC4.0 */

                            /* ReqID:TECC4.0 - Save encrypted columns to pending_tables
                             * before they get cleared below. The encrypted_columns were
                             * extracted by tecc_create_stmt_processor_run() but the legacy
                             * handler path (handle_create_table_defaults) clears them.
                             * We save them here so same-transaction DML can find them. */
                            if (statement_data->encrypted_columns != NIL &&
                                list_length(statement_data->encrypted_columns) > 0)
                            {
                                TeccState *pstate = tecc_get_state(statement_data->conn);
                                if (pstate != NULL)
                                {
                                    TeccColumnRefData *first = (TeccColumnRefData *)linitial(
                                        statement_data->encrypted_columns);
                                    if (first != NULL)
                                    {
                                        const char *db = PQdb(statement_data->conn);
                                        const char *schema = first->schema_name;
                                        if (db == NULL) db = "postgres";
                                        if (schema == NULL || schema[0] == '\0')
                                        {
                                            schema = tecc_cache_get_guc_param(pstate, "current_schema");
                                            if (schema == NULL || schema[0] == '\0')
                                                schema = "public";
                                        }
                                        tecc_cache_insert_pending(pstate, db, schema,
                                                                  first->table_name);
                                        strncpy(pstate->last_table_schema, schema,
                                                sizeof(pstate->last_table_schema) - 1);
                                        pstate->last_table_schema[sizeof(pstate->last_table_schema) - 1] = '\0';
                                        strncpy(pstate->last_table_name, first->table_name,
                                                sizeof(pstate->last_table_name) - 1);
                                        pstate->last_table_name[sizeof(pstate->last_table_name) - 1] = '\0';
                                        pstate->last_stmt_type = TECC_STMT_TYPE_CREATE_TABLE;
                                    }
                                }
                            }

                            defaults_rewritten = tecc_processor_handle_create_table_defaults(
                                (CreateStmt *)stmt, statement_data);
                            if (defaults_rewritten != NULL)
                            {
                                statement_data->encrypted_columns = NIL;
                                statement_data->rewritten_query = defaults_rewritten;
                                statement_data->needs_rewrite = 1;
                                result = TECC_PROCESSOR_NEEDS_REWRITE;
                            }
                            else
                            {
                                statement_data->needs_rewrite = 0;
                                statement_data->encrypted_columns = NIL;
                                result = TECC_PROCESSOR_SUCCESS;
                            }
                        }
                        break;
                    default:
                        result = TECC_PROCESSOR_ERROR;
                        break;
                }
            }
            break;

        case FE_T_AlterTableStmt:  /* 244 - TECC前端标签 */
            {
                TeccAlterStmtProcessorResult alter_result;

                alter_result = tecc_alter_stmt_processor_run(
                    (const AlterTableStmt *) stmt, statement_data);
                statement_data->stmt_type = TECC_STMT_TYPE_ALTER_TABLE;

                if (alter_result == TECC_ALTER_PROCESSOR_SUCCESS)
                {
                    if (statement_data->rewritten_query != NULL)
                    {
                        statement_data->needs_rewrite = 1;
                        result = TECC_PROCESSOR_NEEDS_REWRITE;
                    }
                    else
                    {
                        statement_data->needs_rewrite = 0;
                        result = TECC_PROCESSOR_SUCCESS;
                    }
                }
                else
                {
                    result = TECC_PROCESSOR_ERROR;
                }
            }
            break;

        /* Note: DDL statements are processed by their specific handlers above. */

        case FE_T_PrepareStmt:  /* 220 - PostgreSQL原生标签 */
            /* PREPARE statement processing - handled by prepared statement module */
            return processor_handle_prepare_statement(stmt, statement_data);

        case FE_T_ExecuteStmt:  /* 221 - PostgreSQL原生标签 */
            /* EXECUTE statement processing - handled by prepared statement module */
            return processor_handle_execute_statement(stmt, statement_data);

        case FE_T_DeclareCursorStmt:  /* 222 - TECC前端节点标签 */
            /* DECLARE CURSOR statement processing - delegate to handler */
            result = processor_handle_declare_cursor_statement(stmt, statement_data);
            break;

        case FE_T_DeallocateStmt:  /* 223 - TECC前端节点标签 */
            /* DEALLOCATE statement processing - handled by prepared statement module */
            return processor_handle_deallocate_statement(stmt, statement_data);

        case FE_T_CreateTableAsStmt:  /* 1013 - TECC前端节点标签 */
            /* CREATE TABLE AS / CREATE TABLE AS EXECUTE processing */
            return processor_handle_create_table_as_statement(stmt, statement_data);

        case FE_T_CopyStmt:  /* 253 - PostgreSQL原生标签 */
            /* COPY statement processing */
            /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
            
            /* End - ReqID:TECC4.0 - 线程安全调试支持 */
            result = tecc_stmt_processor_process_copy((CopyStmt *) stmt, statement_data);
            break;

        /* ReqID:TECC4.0 - EXPLAIN statement support */
        case FE_T_ExplainStmt:  /* 260 */
        {
            ExplainStmt *explain = (ExplainStmt *)stmt;

            
            /* Recursively process the inner DML statement */
            tecc_stmt_processor_run_pre_statement(explain->query, statement_data);
            /* EXPLAIN itself doesn't add encrypted columns info */
            result = TECC_PROCESSOR_SUCCESS;
            break;
        }

        default:
            /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
            tecc_processor_debug_safe(NULL, TECC_LOG_WARNING,
                                   "Unsupported statement type: %d (node tag: 0x%x)",
                                   nodeTag(stmt), nodeTag(stmt));
            /* End - ReqID:TECC4.0 - 线程安全调试支持 */

            /* Check if it's a known but unsupported node type */
            if (nodeTag(stmt) >= 200 && nodeTag(stmt) < 300)
            {
                /* Likely a DDL or other statement type that doesn't need encryption processing */
                /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
                tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                       "Statement type %d is likely a DDL statement, skipping encryption processing",
                                       nodeTag(stmt));
                /* End - ReqID:TECC4.0 - 线程安全调试支持 */
                return TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS;
            }

            return TECC_PROCESSOR_UNSUPPORTED_STATEMENT;
    }

    /* Convert statement processor result to main result */
    /* result can be TeccStmtProcessorResult or TeccProcessorResult (e.g. NEEDS_REWRITE from CREATE DEK) */
    if (result == TECC_STMT_PROCESSOR_SUCCESS)
        return TECC_PROCESSOR_SUCCESS;
    else if (result == TECC_STMT_PROCESSOR_UNSUPPORTED_STMT)
        return TECC_PROCESSOR_UNSUPPORTED_STATEMENT;
    else if (result == TECC_STMT_PROCESSOR_NO_ENCRYPTED_COLUMNS)
        return TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS;
    else if (result == TECC_PROCESSOR_NEEDS_REWRITE)
        return TECC_PROCESSOR_NEEDS_REWRITE;
    else
        return TECC_PROCESSOR_ERROR;
}

/*
 * Find the end position of a SQL string literal starting at 'start'.
 * Returns the position after the closing quote, or -1 on error.
 * Handles escaped single quotes ('').
 */
static int
tecc_find_string_literal_end(const char *sql, int start)
{
    int i;

    if (sql == NULL || sql[start] != '\'')
        return -1;

    for (i = start + 1; sql[i] != '\0'; i++)
    {
        if (sql[i] == '\'')
        {
            if (sql[i + 1] == '\'')
            {
                i++;  /* Skip escaped quote '' */
                continue;
            }
            return i + 1;  /* End of string (exclusive) */
        }
    }
    return -1;  /* Unterminated string */
}

/*
 * Handle DEFAULT values for encrypted columns in CREATE TABLE.
 * Encrypts default values and produces a rewritten SQL string.
 * Returns the rewritten query or NULL if no DEFAULT values need encryption.
 */
static char *
tecc_processor_handle_create_table_defaults(CreateStmt *create_stmt,
                                            TeccStatementData *statement_data)
{
    const char *query;
    ListCell *lc;
    bool has_defaults;
    char *result;
    int offset_adjust;
    char *table_name;

    if (create_stmt == NULL || statement_data == NULL)
        return NULL;

    query = statement_data->query;
    if (query == NULL)
        return NULL;

    table_name = create_stmt->relation ? create_stmt->relation->relname : NULL;

    /* First check if any encrypted column has a DEFAULT value */
    has_defaults = false;
    foreach(lc, create_stmt->tableElts)
    {
        Node *element = (Node *) lfirst(lc);
        if (IsA(element, ColumnDef))
        {
            ColumnDef *coldef = (ColumnDef *) element;
            ListCell *clc;

            /* Skip non-encrypted types */
            if (coldef->typeName == NULL || coldef->typeName->names == NIL)
                continue;

            {
                Node *type_node = (Node *) llast(coldef->typeName->names);
                const char *type_name = NULL;
                if (IsA(type_node, Const))
                    type_name = ((Const *)type_node)->constvalue.sval;
                else if (IsA(type_node, String))
                    type_name = strVal((Value *)type_node);
                if (type_name == NULL)
                    continue;
                if (!tecc_create_stmt_processor_type_supports_encryption(type_name))
                    continue;
            }

            /* Check constraints for DEFAULT */
            if (coldef->constraints == NIL)
                continue;

            foreach(clc, coldef->constraints)
            {
                Node *cn = (Node *) lfirst(clc);
                if (IsA(cn, Constraint))
                {
                    Constraint *con = (Constraint *) cn;
                    if (con->contype == CONSTR_DEFAULT && con->raw_expr != NULL)
                    {
                        has_defaults = true;
                        break;
                    }
                }
            }
            if (has_defaults)
                break;
        }
    }

    if (!has_defaults)
        return NULL;

    /* Rewrite SQL: encrypt each DEFAULT literal */
    result = pg_strdup(query);
    offset_adjust = 0;

    foreach(lc, create_stmt->tableElts)
    {
        Node *element = (Node *) lfirst(lc);
        ColumnDef *coldef = NULL;
        ListCell *clc;
        char *dek_name;
        const char *column_name;
        bool is_encrypted;

        if (!IsA(element, ColumnDef))
            continue;

        coldef = (ColumnDef *) element;

        /* Check if encrypted type */
        is_encrypted = false;
        if (coldef->typeName != NULL && coldef->typeName->names != NIL)
        {
            Node *type_node = (Node *) llast(coldef->typeName->names);
            const char *type_name = NULL;
            if (IsA(type_node, Const))
                type_name = ((Const *)type_node)->constvalue.sval;
            else if (IsA(type_node, String))
                type_name = strVal((Value *)type_node);
            if (type_name != NULL)
            {
                is_encrypted = tecc_create_stmt_processor_type_supports_encryption(
                    type_name);
            }
        }
        if (!is_encrypted)
            continue;

        column_name = coldef->colname ? coldef->colname : "unknown";

        /* Get DEK name from encrypted options */
        dek_name = NULL;
        if (coldef->typeName != NULL && coldef->typeName->encryptedOpts != NIL)
        {
            ListCell *opt_lc;
            foreach(opt_lc, coldef->typeName->encryptedOpts)
            {
                DefElem *opt = (DefElem *) lfirst(opt_lc);
                if (strcmp(opt->defname, "dek") == 0 && opt->arg != NULL)
                {
                    dek_name = strVal((Value *)opt->arg);
                    break;
                }
            }
        }

        if (dek_name == NULL)
            continue;

        /* Process each DEFAULT constraint */
        foreach(clc, coldef->constraints)
        {
            Node *cn = (Node *) lfirst(clc);
            Constraint *con;

            if (!IsA(cn, Constraint))
                continue;

            con = (Constraint *) cn;
            if (con->contype != CONSTR_DEFAULT || con->raw_expr == NULL)
                continue;

            /* Extract the Const node with the value */
            if (!IsA(con->raw_expr, Const))
                continue;

            {
                Const *c = (Const *) con->raw_expr;
                TeccUnifiedEncryptResult *enc_result;
                char *plaintext;
                int literal_start;
                int literal_end;
                int orig_len;
                size_t enc_len;
                char *replacement;
                int adj_start;
                int adj_end;
                size_t new_result_len;
                char *new_result;

                if (c->constisnull || c->constvalue.sval == NULL)
                    continue;

                plaintext = c->constvalue.sval;
                literal_start = c->location;

                /* Find end of string literal in SQL */
                literal_end = tecc_find_string_literal_end(result, literal_start + offset_adjust);
                if (literal_end < 0)
                    continue;

                orig_len = literal_end - (literal_start + offset_adjust);

                /* Encrypt the default value */
                

                enc_result = tecc_unified_encrypt_string(
                    statement_data->conn,
                    plaintext,
                    table_name ? table_name : column_name,
                    column_name,
                    NULL,   /* kek_name */
                    dek_name, NULL);

                if (enc_result == NULL ||
                    enc_result->error_code != TECC_CRYPTO_SUCCESS)
                {
                    if (enc_result != NULL)
                    {
                        tecc_processor_debug(TECC_LOG_ERROR,
                                           "Failed to encrypt DEFAULT value: %s",
                                           enc_result->error_msg);
                        tecc_unified_free_encrypt_result(enc_result);
                    }
                    pg_free(result);
                    return NULL;
                }

                /* Build replacement: 'encrypted_hex' */
                enc_len = strlen(enc_result->hex_string);
                replacement = (char *)pg_malloc(enc_len + 3);
                replacement[0] = '\'';
                memcpy(replacement + 1, enc_result->hex_string, enc_len);
                replacement[enc_len + 1] = '\'';
                replacement[enc_len + 2] = '\0';

                /* Apply replacement */
                adj_start = literal_start + offset_adjust;
                adj_end = literal_start + offset_adjust + orig_len;
                new_result_len = strlen(result) - orig_len + (enc_len + 2);
                new_result = (char *)pg_malloc(new_result_len + 1);

                memcpy(new_result, result, adj_start);
                memcpy(new_result + adj_start, replacement, enc_len + 2);
                memcpy(new_result + adj_start + enc_len + 2,
                       result + adj_end,
                       strlen(result) - adj_end + 1);  /* include null terminator */

                pg_free(result);
                result = new_result;

                offset_adjust += (enc_len + 2) - orig_len;

                

                tecc_unified_free_encrypt_result(enc_result);
                pg_free(replacement);
            }
        }
    }

    return result;
}

/*
 * SQL query rewrite functionality - integrated with SQL rewriter module
 */
static TeccProcessorResult
tecc_processor_rewrite_query(TeccStatementData *statement_data)
{
    if (statement_data == NULL)
        return TECC_PROCESSOR_INVALID_INPUT;

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                            "Starting SQL query rewrite");
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    /* SQL rewrite functionality is now distributed to submodules:
     * - values_processor: parameterized query rewrite
     * - where_clause_processor: WHERE/HAVING clause rewrite
     * - stmt_processor: CREATE VIEW statement rewrite
     */
    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    /* Call distributed rewrite logic in respective submodules */

    /* 1. First call where_clause_processor for WHERE/HAVING clause rewrite.
     *    This must run BEFORE values_processor rewrite because WHERE rewrite
     *    modifies the parse tree AND adds encrypted values to raw_values_list.
     *    Values rewrite then uses the complete raw_values_list for SQL text replacement.
     *
     *    Note: For UPDATE statements, WHERE processing has already been done during
     *    the process phase (tecc_where_clause_processor_process inside stmt_processor),
     *    which encrypted WHERE values and added them to raw_values_list.
     *    Skip WHERE rewrite for UPDATE to avoid double-encryption of already-replaced Const nodes. */
    if (statement_data->where_needs_rewrite)
    {
        TeccWhereClauseProcessorResult where_result;

        where_result = tecc_where_clause_processor_rewrite_clause(statement_data);
        if (where_result != TECC_WHERE_PROCESSOR_SUCCESS)
        {
            /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
            tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                                   "WHERE clause processor rewrite failed: %d", where_result);
            /* End - ReqID:TECC4.0 - 线程安全调试支持 */
            return TECC_PROCESSOR_ERROR;
        }
    }

    /* 2. Call values_processor for parameterized query rewrite.
     *    This operates on statement_data->query (original SQL text) and replaces
     *    each value in raw_values_list at its location/data_size with the encrypted form.
     *    The raw_values_list may contain entries from both SET values (added by
     *    tecc_values_processor_process) and WHERE values (added by WHERE rewrite above). */
    if (statement_data->raw_values_list != NULL &&
        tecc_raw_values_list_count(statement_data->raw_values_list) > 0)
    {
        TeccValuesProcessorResult values_result;

        values_result = tecc_values_processor_rewrite_values(statement_data);
        if (values_result != TECC_VALUES_PROCESSOR_SUCCESS)
        {
            /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
            tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                                   "Values processor rewrite failed: %d", values_result);
            /* End - ReqID:TECC4.0 - 线程安全调试支持 */
            return TECC_PROCESSOR_ERROR;
        }
    }

    /* 3. ReqID:TECC4.0 - ENCRYPTED WITH handling for CREATE TABLE.
     *
     * IMPORTANT: The ENCRYPTED WITH clause MUST be preserved in the SQL
     * sent to the server. The server's tablecmds.c validates encrypted
     * column types and uses ENCRYPTED WITH to:
     *   a) Record metadata in hg_encrypted_columns
     *   b) Track DEK associations
     *   c) Record encryption type (support_algo)
     *
     * The frontend parser extracts DEK name and support_algo from the
     * ENCRYPTED WITH clause for its own encryption preprocessing, but
     * the original SQL is sent as-is to the server. The rewritten query
     * is set to NULL so the original SQL is used.
     *
     * Stripping ENCRYPTED WITH (the old behavior) would cause server
     * error: "encrypted data type requires ENCRYPTED WITH clause".
     *
     * Note: If rewritten_query is already set by create_stmt_processor
     * (e.g. DEFAULT encryption done), we respect it.
     */
    if (statement_data->stmt_type == TECC_STMT_TYPE_CREATE_TABLE &&
        statement_data->rewritten_query == NULL &&
        statement_data->query != NULL)
    {
        tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                "CREATE TABLE: preserving ENCRYPTED WITH clause for server-side processing");

        /* Use the original query as-is — server needs the ENCRYPTED WITH clause */
        statement_data->rewritten_query = tecc_pstrdup(statement_data->query);
    }

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */
    return TECC_PROCESSOR_SUCCESS;
}


/*
 * Unified decryption interface for external modules
 */
char *
tecc_processor_decrypt_value(PGconn *conn, const char *encrypted_value, Oid type_id)
{
    TeccStatementData statement_data;
    TeccValuesProcessorResult result;
    char *decrypted_value = NULL;
    TeccState *state;
    /* TODO: Get algorithm and key from metadata cache */
    const char *algorithm = "RND";  /* Default algorithm */
    const char *key = "default_key";  /* Default key */
    /* Create raw value structure */
    TeccRawValue raw_value;

    if (conn == NULL || encrypted_value == NULL)
    {
        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                               "Invalid parameters to decrypt_value");
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
        return NULL;
    }

    /* Get TECC state from connection */
    state = tecc_get_state(conn);
    if (state == NULL)
    {
        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                               "Failed to get TECC state from connection");
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
        return NULL;
    }

    if (!state->processor_initialized)
    {
        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                               "Processor not initialized for this connection DEBUG");
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
        return NULL;
    }

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                            "Processing decryption request for type %u", type_id);
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    /* Initialize statement data */
    memset(&statement_data, 0, sizeof(TeccStatementData));
    statement_data.conn = conn;

    /* TODO: Get table and column name from context - this requires metadata about the query */
    /* For now, use placeholder values - actual implementation should extract from query context */
    /* const char *table_name = "unknown_table"; */  /* Unused for now */
    /* const char *column_name = "unknown_column"; */  /* Unused for now */

    memset(&raw_value, 0, sizeof(TeccRawValue));
    raw_value.type = TECC_RAW_VALUE_TYPE_STRING;
    raw_value.state = TECC_RAW_VALUE_STATE_ENCRYPTED;
    raw_value.original.string_value = (char *)encrypted_value;
    raw_value.length = strlen(encrypted_value);
    raw_value.type_oid = type_id;
    raw_value.needs_decryption = true;

    /* Call values processor for decryption */
    result = tecc_values_processor_decrypt_value(&raw_value, algorithm, key, &statement_data);

    if (result != TECC_VALUES_PROCESSOR_SUCCESS)
    {
        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                               "Values processor failed to decrypt value");
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
        return NULL;
    }

    /* Extract decrypted value from raw_value */
    if (raw_value.processed.decrypted_value != NULL)
    {
        decrypted_value = tecc_processor_strdup(raw_value.processed.decrypted_value);
    }
    else if (raw_value.processed.processed_value != NULL)
    {
        /* Fallback to processed_value if decrypted_value is not set */
        decrypted_value = tecc_processor_strdup((char *)raw_value.processed.processed_value);
    }
    else
    {
        /* No decrypted value available, return copy of original as placeholder */
        decrypted_value = tecc_processor_strdup(encrypted_value);
        /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
        tecc_processor_debug_safe(NULL, TECC_LOG_WARNING,
                               "No decrypted value in raw_value, using placeholder");
        /* End - ReqID:TECC4.0 - 线程安全调试支持 */
    }

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                            "Decryption completed successfully");
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    return decrypted_value;
}

/*
 * ReqID:TECC4.0 — Encrypt EXECUTE parameter values.
 *
 * Looks up the prepared statement to find params mapped to encrypted columns,
 * extracts parameter value strings from the EXECUTE SQL text, encrypts them,
 * and constructs a rewritten EXECUTE SQL with encrypted literals.
 */
static void
tecc_processor_encrypt_execute_params(const ExecuteStmt *execute_stmt,
                                      TeccPreparedStatement *ps,
                                      TeccStatementData *statement_data)
{
    const char *query;
    const char **param_strs;
    int nparams;
    char **encrypted_hex;
    char **encrypted_type;
    int encrypted_count;
    int i;

    param_strs = NULL;
    nparams = 0;
    encrypted_hex = NULL;
    encrypted_type = NULL;
    encrypted_count = 0;

    /*
     * Step 1: extract parameter value strings from the EXECUTE SQL text.
     * execute_stmt->paramValues is not populated by the parser.
     */
    query = statement_data->query;
    if (query != NULL)
    {
        const char *p;
        int max_p;

        p = query;
        while (*p && *p != '(') p++;
        if (*p != '(') return;
        p++; /* skip '(' */

        max_p = (ps->num_params > 0) ? ps->num_params : 16;
        param_strs = (const char **)pg_malloc(max_p * sizeof(char *));
        if (param_strs == NULL) return;
        memset(param_strs, 0, max_p * sizeof(char *));

        while (*p && *p != ';' && nparams < ps->num_params)
        {
            const char *start;
            int paren_depth;
            bool in_string;

            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
            if (*p == '\0' || *p == ')' || *p == ';') break;

            start = p;
            paren_depth = 0;
            in_string = false;

            while (*p)
            {
                char c = *p;
                if (in_string) {
                    if (c == '\'') { if (p[1] == '\'') p += 2; else { in_string = false; p++; } }
                    else p++;
                }
                else if (c == '\'') { in_string = true; p++; }
                else if (c == '(') { paren_depth++; p++; }
                else if (c == ')') { if (paren_depth == 0) break; paren_depth--; p++; }
                else if (c == ',') { if (paren_depth == 0) break; p++; }
                else p++;
            }

            if (p > start && nparams < max_p)
                param_strs[nparams++] = pnstrdup(start, (Size)(p - start));

            if (*p == ',') p++;
        }
    }

    if (nparams == 0)
    {
        if (param_strs) pg_free(param_strs);
        return;
    }

    /*
     * Step 2: encrypt params that map to encrypted columns.
     */
    encrypted_hex = (char **)pg_malloc(ps->num_params * sizeof(char *));
    encrypted_type = (char **)pg_malloc(ps->num_params * sizeof(char *));
    if (encrypted_hex == NULL || encrypted_type == NULL)
    {
        for (i = 0; i < nparams; i++) if (param_strs[i]) pg_free((void *)param_strs[i]);
        pg_free(param_strs);
        if (encrypted_hex) pg_free(encrypted_hex);
        if (encrypted_type) pg_free(encrypted_type);
        return;
    }
    memset(encrypted_hex, 0, ps->num_params * sizeof(char *));
    memset(encrypted_type, 0, ps->num_params * sizeof(char *));

    for (i = 0; i < ps->num_params; i++)
    {
        TeccPreparedStatementParam *pi;
        const char *plaintext;
        const char *tname;
        const char *cname;
        char *dek_name;
        TeccUnifiedEncryptResult *enc_res;

        pi = ps->params[i];
        if (pi == NULL || !pi->needs_encryption)
            continue;

        if (i >= nparams || param_strs[i] == NULL)
            continue;

        plaintext = param_strs[i];

        /* ReqID:TECC4.0 — Skip NULL parameters, no encryption needed */
        if (pg_strcasecmp(plaintext, "NULL") == 0)
            continue;

        tname = pi->table_name;
        cname = pi->column_name;
        if (tname == NULL || cname == NULL) continue;

        dek_name = tecc_get_column_dek_name(statement_data, NULL, tname, cname);
        if (dek_name == NULL) continue;

        enc_res = tecc_unified_encrypt_string(statement_data->conn,
            plaintext, tname, cname, NULL, dek_name, NULL);
        pg_free(dek_name);

        if (enc_res == NULL || enc_res->error_code != TECC_CRYPTO_SUCCESS)
        {
            tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                "EXECUTE param $%d: encrypt failed for %s.%s: %s",
                i + 1, tname, cname,
                enc_res ? enc_res->error_msg : "unknown");
            if (enc_res) tecc_unified_free_encrypt_result(enc_res);
            continue;
        }

        encrypted_hex[i] = pg_strdup(enc_res->hex_string);
        tname = pi->column_type;
        if (tname == NULL || tname[0] == '\0')
            tname = "text";
        encrypted_type[i] = pg_strdup(tname);
        tecc_unified_free_encrypt_result(enc_res);
        encrypted_count++;

        
    }

    if (encrypted_count == 0)
        goto cleanup;

    /*
     * Step 3: construct rewritten EXECUTE SQL.
     */
    {
        size_t est;
        char *nq;
        size_t nl;
        int pi2;

        est = 128 + strlen(execute_stmt->name);
        for (pi2 = 0; pi2 < ps->num_params; pi2++)
        {
            if (encrypted_hex[pi2] != NULL)
                est += strlen(encrypted_hex[pi2]) + 32;
            else if (pi2 < nparams && param_strs[pi2] != NULL)
                est += strlen(param_strs[pi2]) + 2;
            else
                est += 8;
        }

        nq = (char *)pg_malloc(est);
        if (nq == NULL) goto cleanup;
        nl = 0;

        /* "EXECUTE <name>(" */
        memcpy(nq + nl, "EXECUTE ", 8); nl += 8;
        {
            size_t nl2 = strlen(execute_stmt->name);
            memcpy(nq + nl, execute_stmt->name, nl2); nl += nl2;
        }
        nq[nl++] = '(';

        for (pi2 = 0; pi2 < ps->num_params; pi2++)
        {
            if (pi2 > 0) nq[nl++] = ',';

            if (encrypted_hex[pi2] != NULL)
            {
                size_t hl;
                size_t tl;
                const char *tp;

                nq[nl++] = '\''; nq[nl++] = '\\'; nq[nl++] = 'x';
                /* The encrypted hex string may already start with "\x" —
                 * skip that prefix if present to avoid doubling. */
                {
                    const char *hex;
                    hex = encrypted_hex[pi2];
                    if (hex[0] == '\\' && hex[1] == 'x')
                        hex += 2;
                    hl = strlen(hex);
                    memcpy(nq + nl, hex, hl); nl += hl;
                }
                nq[nl++] = '\''; nq[nl++] = ':'; nq[nl++] = ':';
                tp = encrypted_type[pi2];
                tl = strlen(tp);
                memcpy(nq + nl, tp, tl); nl += tl;
            }
            else if (pi2 < nparams && param_strs[pi2] != NULL)
            {
                size_t pl;
                pl = strlen(param_strs[pi2]);
                memcpy(nq + nl, param_strs[pi2], pl); nl += pl;
            }
            else
            {
                nq[nl++] = 'N'; nq[nl++] = 'U'; nq[nl++] = 'L'; nq[nl++] = 'L';
            }
        }

        nq[nl++] = ')'; nq[nl++] = ';'; nq[nl] = '\0';

        if (statement_data->rewritten_query != NULL)
            pg_free(statement_data->rewritten_query);
        statement_data->rewritten_query = nq;
        statement_data->needs_rewrite = 1;

        
    }

cleanup:
    for (i = 0; i < ps->num_params; i++)
    {
        if (encrypted_hex[i]) pg_free(encrypted_hex[i]);
        if (encrypted_type[i]) pg_free(encrypted_type[i]);
    }
    pg_free(encrypted_hex);
    pg_free(encrypted_type);
    for (i = 0; i < nparams; i++)
        if (param_strs[i]) pg_free((void *)param_strs[i]);
    pg_free(param_strs);
}

/*
 * Handle PREPARE statement
 */
static TeccProcessorResult
processor_handle_prepare_statement(const Node *stmt,
                                  TeccStatementData *statement_data)
{
    const PrepareStmt *prepare_stmt;
    TeccPreparedStatementResult prepared_result;

    if (stmt == NULL || statement_data == NULL)
    {
        return TECC_PROCESSOR_INVALID_INPUT;
    }

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    prepare_stmt = (const PrepareStmt *)stmt;

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    /* Call prepared statement module to handle PREPARE */
    prepared_result = tecc_prepared_statement_handle_prepare(
        statement_data->conn,
        prepare_stmt->name,
        statement_data->query,
        statement_data);

    /*
     * ReqID:TECC4.0 — After a successful PREPARE, process the inner query
     * for encrypted Const values (e.g., INSERT INTO t VALUES('plain', 50000)).
     * Skip if the inner query only has $N params — those are encrypted at
     * EXECUTE time by processor_handle_execute_statement.
     */
    if (prepared_result == TECC_PREPARED_STATEMENT_SUCCESS &&
        prepare_stmt->query != NULL)
    {
        TeccPreparedStatementContext *ps_ctx;
        TeccPreparedStatement *ps;
        bool skip_inner;

        skip_inner = false;
        ps_ctx = tecc_prepared_statement_init(statement_data->conn);
        if (ps_ctx != NULL)
        {
            ps = tecc_prepared_statement_get(ps_ctx, prepare_stmt->name);
            if (ps != NULL && ps->num_params > 0)
            {
                
                skip_inner = true;
            }
            tecc_prepared_statement_cleanup(ps_ctx);
        }

        if (!skip_inner)
        {
            TeccProcessorResult inner_result;

            
            inner_result = processor_process_statement(
                prepare_stmt->query, statement_data);
            if (inner_result != TECC_PROCESSOR_SUCCESS &&
                inner_result != TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS &&
                inner_result != TECC_PROCESSOR_NEEDS_REWRITE)
            {
                tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                    "PREPARE inner query processing failed: %d",
                    inner_result);
                return inner_result;
            }
        }
    }

    /* Convert prepared statement result to processor result */
    switch (prepared_result)
    {
        case TECC_PREPARED_STATEMENT_SUCCESS:
            
            return TECC_PROCESSOR_SUCCESS;

        case TECC_PREPARED_STATEMENT_MEMORY_ERROR:
            return TECC_PROCESSOR_MEMORY_ERROR;

        case TECC_PREPARED_STATEMENT_ALREADY_EXISTS:
            /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
            tecc_processor_debug_safe(NULL, TECC_LOG_WARNING,
                                   "Prepared statement already exists: %s",
                                   prepare_stmt->name ? prepare_stmt->name : "(unnamed)");
            /* End - ReqID:TECC4.0 - 线程安全调试支持 */
            return TECC_PROCESSOR_SUCCESS; /* Not an error, just a warning */

        case TECC_PREPARED_STATEMENT_INVALID_INPUT:
            return TECC_PROCESSOR_INVALID_INPUT;

        case TECC_PREPARED_STATEMENT_NOT_FOUND:
            /* Should not happen for PREPARE */
            return TECC_PROCESSOR_INTERNAL_ERROR;

        default:
            /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
            tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                                   "Unexpected error from prepared statement module: %d",
                                   prepared_result);
            /* End - ReqID:TECC4.0 - 线程安全调试支持 */
            return TECC_PROCESSOR_INTERNAL_ERROR;
    }
}

/*
 * Handle EXECUTE statement
 */
static TeccProcessorResult
processor_handle_execute_statement(const Node *stmt,
                                  TeccStatementData *statement_data)
{
    const ExecuteStmt *execute_stmt;
    TeccPreparedStatementResult prepared_result;

    if (stmt == NULL || statement_data == NULL)
    {
        return TECC_PROCESSOR_INVALID_INPUT;
    }

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    execute_stmt = (const ExecuteStmt *)stmt;

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    /* Call prepared statement module to handle EXECUTE */
    prepared_result = tecc_prepared_statement_handle_execute(
        statement_data->conn,
        execute_stmt->name,
        execute_stmt->nparams,
        execute_stmt->paramtypes,
        execute_stmt->paramValues,
        execute_stmt->paramLengths,
        execute_stmt->paramFormats,
        statement_data);

    /* ReqID:TECC4.0 — EXECUTE parameter encryption */
    if (prepared_result == TECC_PREPARED_STATEMENT_SUCCESS)
    {
        TeccPreparedStatementContext *ps_ctx;
        TeccPreparedStatement *ps;

        ps_ctx = tecc_prepared_statement_init(statement_data->conn);
        if (ps_ctx != NULL)
        {
            ps = tecc_prepared_statement_get(ps_ctx, execute_stmt->name);
            tecc_prepared_statement_cleanup(ps_ctx);

            if (ps != NULL && ps->num_params > 0 &&
                ps->num_encrypted_params > 0)
            {
                tecc_processor_encrypt_execute_params(
                    execute_stmt, ps, statement_data);
            }
        }
    }

    /* Convert prepared statement result to processor result */
    switch (prepared_result)
    {
        case TECC_PREPARED_STATEMENT_SUCCESS:
            /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
            
            /* End - ReqID:TECC4.0 - 线程安全调试支持 */
            return TECC_PROCESSOR_SUCCESS;

        case TECC_PREPARED_STATEMENT_MEMORY_ERROR:
            return TECC_PROCESSOR_MEMORY_ERROR;

        case TECC_PREPARED_STATEMENT_NOT_FOUND:
            /* ReqID:TECC4.0 — prepared statement not tracked by TECC,
             * meaning no encrypted columns are involved. Pass through
             * the original EXECUTE to the backend as-is. */
            tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                   "Prepared statement not tracked by TECC, passing through: %s",
                                   execute_stmt->name ? execute_stmt->name : "(unnamed)");
            return TECC_PROCESSOR_SUCCESS;

        case TECC_PREPARED_STATEMENT_INVALID_INPUT:
            return TECC_PROCESSOR_INVALID_INPUT;

        default:
            /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
            tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                                   "Unexpected error from prepared statement module: %d",
                                   prepared_result);
            /* End - ReqID:TECC4.0 - 线程安全调试支持 */
            return TECC_PROCESSOR_INTERNAL_ERROR;
    }
}

/*
 * Handle DEALLOCATE statement
 * Routes to prepared statement module for cleanup.
 */
static TeccProcessorResult
processor_handle_deallocate_statement(const Node *stmt,
                                      TeccStatementData *statement_data)
{
    const DeallocateStmt *deallocate_stmt;
    TeccPreparedStatementResult prepared_result;

    if (stmt == NULL || statement_data == NULL)
    {
        return TECC_PROCESSOR_INVALID_INPUT;
    }

    deallocate_stmt = (const DeallocateStmt *)stmt;

    

    prepared_result = tecc_prepared_statement_handle_deallocate(
        statement_data->conn,
        deallocate_stmt->name,
        statement_data);

    switch (prepared_result)
    {
        case TECC_PREPARED_STATEMENT_SUCCESS:
            
            return TECC_PROCESSOR_SUCCESS;

        case TECC_PREPARED_STATEMENT_MEMORY_ERROR:
            return TECC_PROCESSOR_MEMORY_ERROR;

        case TECC_PREPARED_STATEMENT_NOT_FOUND:
            /* ReqID:TECC4.0 — prepared statement not tracked by TECC,
             * meaning no encrypted columns are involved. Pass through
             * the original DEALLOCATE to the backend as-is. */
            tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                                   "Prepared statement not tracked by TECC, passing through DEALLOCATE: %s",
                                   deallocate_stmt->name ? deallocate_stmt->name : "(null)");
            return TECC_PROCESSOR_SUCCESS;

        case TECC_PREPARED_STATEMENT_INVALID_INPUT:
            return TECC_PROCESSOR_INVALID_INPUT;

        default:
            tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                                   "Unexpected error from prepared statement module: %d",
                                   prepared_result);
            return TECC_PROCESSOR_INTERNAL_ERROR;
    }
}

/*
 * Handle CREATE TABLE AS / CREATE TABLE AS EXECUTE statement
 * Routes the embedded query (EXECUTE or SELECT) for encryption processing.
 * For EXECUTE variant: delegates to the EXECUTE handler for parameter encryption.
 * For SELECT variant: passes through (server handles table creation from query results).
 */
static TeccProcessorResult
processor_handle_create_table_as_statement(const Node *stmt,
                                          TeccStatementData *statement_data)
{
    const CreateTableAsStmt *ctas;

    if (stmt == NULL || statement_data == NULL)
    {
        return TECC_PROCESSOR_INVALID_INPUT;
    }

    ctas = (const CreateTableAsStmt *)stmt;

    

    if (ctas->query != NULL && nodeTag(ctas->query) == FE_T_ExecuteStmt)
    {
        TeccProcessorResult result;
        const char *rewritten;
        char ctas_prefix[512];
        char *new_query;
        size_t prefix_len;
        size_t rewritten_len;
        const char *relname;
        int prefix_written;

        /* EXECUTE variant: delegate to EXECUTE handler for parameter encryption */
        
        result = processor_handle_execute_statement(ctas->query, statement_data);

        /*
         * The EXECUTE handler rewrites statement_data->rewritten_query to
         * "EXECUTE name(...);".  For CTAS we must reconstruct the full
         * "CREATE TABLE [IF NOT EXISTS] <name> AS EXECUTE name(...);".
         */
        if (result == TECC_PROCESSOR_SUCCESS && statement_data->needs_rewrite)
        {
            rewritten = statement_data->rewritten_query;
            if (rewritten == NULL)
                return result;

            relname = (ctas->relation != NULL && ctas->relation->relname != NULL)
                      ? ctas->relation->relname : "unknown";

            prefix_written = snprintf(ctas_prefix, sizeof(ctas_prefix),
                "CREATE TABLE %s%s AS ",
                ctas->if_not_exists ? "IF NOT EXISTS " : "",
                relname);

            if (prefix_written < 0 || (size_t)prefix_written >= sizeof(ctas_prefix))
                return TECC_PROCESSOR_INTERNAL_ERROR;

            prefix_len = (size_t)prefix_written;
            rewritten_len = strlen(rewritten);
            new_query = (char *)pg_malloc(prefix_len + rewritten_len + 1);
            if (new_query == NULL)
                return TECC_PROCESSOR_MEMORY_ERROR;

            memcpy(new_query, ctas_prefix, prefix_len);
            memcpy(new_query + prefix_len, rewritten, rewritten_len + 1);
            pg_free(statement_data->rewritten_query);
            statement_data->rewritten_query = new_query;

            
        }

        return result;
    }

    /*
     * For SelectStmt or other query type: pass through.
     * The server handles CREATE TABLE ... AS SELECT ... execution,
     * including any encrypted column processing that was set up during PREPARE.
     */
    
    /* ReqID:TECC4.0 - For SelectStmt CTAS with ENCRYPTED WITH targets:
     * Encrypt constants, keep ::type ENCRYPTED WITH (...) in the query.
     * The backend parses ENCRYPTED WITH from the type cast and registers
     * column metadata via populate_ctas_encryption_metadata. */
    if (ctas->query != NULL && nodeTag(ctas->query) == FE_T_SelectStmt)
    {
        SelectStmt *select = (SelectStmt *)ctas->query;
        const char *orig_query = statement_data->query;
        ListCell *lc;
        bool has_encrypted = false;
        char *working_query;
        int offset_adjust = 0;

        // fprintf removed. Use file instead CTAS handler entered, orig_query=[%s]\n", orig_query ? orig_query : "NULL");

        /* First pass: check if any target has ENCRYPTED WITH */
        foreach(lc, select->targetList)
        {
            ResTarget *rt = (ResTarget *)lfirst(lc);
            if (rt->val != NULL && nodeTag(rt->val) == FE_T_TypeCast)
            {
                TypeCast *tc = (TypeCast *)rt->val;
                if (tc->typeName != NULL &&
                    tc->typeName->encryptedOpts != NIL)
                {
                    has_encrypted = true;
                    break;
                }
            }
        }

        if (!has_encrypted)
        {
            // fprintf removed. Use file instead no encrypted TypeCast found, returning SUCCESS\n");
            return TECC_PROCESSOR_SUCCESS;
        }
        // fprintf removed. Use file instead encrypted TypeCast found, proceeding to second pass\n");

        /* Second pass: encrypt constants, keep ENCRYPTED WITH */
        working_query = pg_strdup(orig_query);

        foreach(lc, select->targetList)
        {
            ResTarget *rt = (ResTarget *)lfirst(lc);
            if (rt->val == NULL || nodeTag(rt->val) != FE_T_TypeCast)
                continue;

            {
                TypeCast *tc = (TypeCast *)rt->val;

                if (tc->typeName == NULL ||
                    tc->typeName->encryptedOpts == NIL)
                    continue;

                {
                    ListCell *opt_lc;
                    char *dek_name = NULL;
                    char plaintext[256];
                    Const *cnst;
                    Node *arg_node;
                    TeccUnifiedEncryptResult *encrypt_result = NULL;
                    char replacement[1024];
                    char type_name[NAMEDATALEN];
                    int repl_len;
                    size_t new_len;
                    char *new_query;
                    bool is_quoted_const = false;

                    /* Extract DEK name */
                    foreach(opt_lc, tc->typeName->encryptedOpts)
                    {
                        DefElem *elem = (DefElem *)opt_lc->data.ptr_value;
                        if (elem != NULL && elem->defname != NULL &&
                            pg_strcasecmp(elem->defname, "dek") == 0)
                        {
                            if (elem->arg != NULL &&
                                nodeTag(elem->arg) == FE_T_String)
                                dek_name = strVal((const Value *)elem->arg);
                            break;
                        }
                    }

                    if (dek_name == NULL)
                    {
                        tecc_processor_debug(TECC_LOG_WARNING,
                            "No DEK found in ENCRYPTED WITH for CTAS target");
                        continue;
                    }

                    /* Extract type name */
                    {
                        Value *tn_v;
                        tn_v = (Value *)lfirst(list_head(tc->typeName->names));
                        snprintf(type_name, sizeof(type_name), "%s",
                                 tn_v ? strVal(tn_v) : "text_enc");
                    }

                    /* Extract plaintext from Const */
                    arg_node = tc->arg;
                    while (arg_node != NULL &&
                        nodeTag(arg_node) == FE_T_TypeCast)
                        arg_node = ((TypeCast *)arg_node)->arg;

                    cnst = (Const *)arg_node;
                    if (cnst == NULL || cnst->type != FE_T_Const)
                    {
                        tecc_processor_debug(TECC_LOG_WARNING,
                            "CTAS TypeCast arg is not a Const, skipping");
                        continue;
                    }

                    if (cnst->constisnull)
                        continue;

                    if (cnst->constbyval &&
                        (cnst->constlen == sizeof(int16_t) ||
                         cnst->constlen == sizeof(int32_t) ||
                         cnst->constlen == sizeof(int64_t)))
                    {
                        is_quoted_const = false;
                        snprintf(plaintext, sizeof(plaintext), "%ld",
                             (long)cnst->constvalue.ival);
                    }
                    else if (cnst->constbyval &&
                         (cnst->constlen == sizeof(float4) ||
                          cnst->constlen == sizeof(float8)))
                    {
                        is_quoted_const = false;
                        if (cnst->location >= 0 && cnst->data_size > 0 &&
                            (int)cnst->data_size < (int)sizeof(plaintext))
                        {
                            memcpy(plaintext, orig_query + cnst->location,
                                   cnst->data_size);
                            plaintext[cnst->data_size] = '\0';
                        }
                        else
                            snprintf(plaintext, sizeof(plaintext), "%g",
                             cnst->constvalue.dval);
                    }
                    else if (cnst->constvalue.sval != NULL)
                    {
                        is_quoted_const = true;
                        snprintf(plaintext, sizeof(plaintext), "%s",
                             cnst->constvalue.sval);
                    }
                    else
                    {
                        tecc_processor_debug(TECC_LOG_WARNING,
                            "Unsupported Const type in CTAS");
                        continue;
                    }

                    /* Encrypt with options - bypass cache since table doesn't
                     * exist yet. map_enc_type_name_to_base_oid gives us the
                     * base type OID from the encrypted type name. */
                    {
                        Oid base_oid;

                        base_oid = map_enc_type_name_to_base_oid(type_name);
                        if (!OidIsValid(base_oid))
                        {
                            tecc_processor_debug(TECC_LOG_WARNING,
                                "Unknown encrypted type '%s' in CTAS", type_name);
                            continue;
                        }

                        encrypt_result = tecc_unified_encrypt_string_with_options(
                            statement_data->conn,
                            plaintext,
                            "ctas", type_name,
                            NULL, dek_name,
                            base_oid, -1,
                            true, false, false, NULL);
                    }

                    if (encrypt_result == NULL ||
                        encrypt_result->error_code != TECC_CRYPTO_SUCCESS)
                    {
                        tecc_processor_debug(TECC_LOG_ERROR,
                            "CTAS encryption failed: %s",
                            encrypt_result ?
                            encrypt_result->error_msg : "unknown");
                        if (encrypt_result)
                            tecc_unified_free_encrypt_result(encrypt_result);
                        goto ctas_select_error;
                    }

                    /* Find and replace just the constant value.
                     * Search for 'plaintext'::type in the original query
                     * and replace only 'plaintext' with E'\\x<hex>',
                     * keeping ::type ENCRYPTED WITH (...) intact. */
                    {
                        char search_pattern[512];
                        const char *found;
                        int constant_start, constant_end;

                        if (is_quoted_const)
                            snprintf(search_pattern, sizeof(search_pattern),
                                "'%s'::%s", plaintext, type_name);
                        else
                            snprintf(search_pattern, sizeof(search_pattern),
                                "%s::%s", plaintext, type_name);

                        found = strstr(orig_query, search_pattern);
                        if (found == NULL)
                        {
                            tecc_unified_free_encrypt_result(
                                encrypt_result);
                            continue;
                        }

                        /* constant is 'plaintext' (with quotes) */
                        constant_start = (int)(found - orig_query);
                        if (is_quoted_const)
                            constant_end = constant_start +
                                           (int)strlen(plaintext) + 2;
                        else
                            constant_end = constant_start +
                                           (int)strlen(plaintext);
                        /* Build replacement: E'\\x<hex>' */
                        {
                            const char *hex = encrypt_result->hex_string;

                            if (hex[0] == '\\' && hex[1] == 'x')
                                hex += 2;
                            repl_len = snprintf(replacement,
                                sizeof(replacement),
                                "E'\\\\x%s'", hex);
                        }

                        tecc_unified_free_encrypt_result(encrypt_result);

                        if (repl_len >= (int)sizeof(replacement))
                            continue;

                        /* Apply offset-adjusted replacement */
                        {
                            int adj_start = constant_start + offset_adjust;
                            int adj_end = constant_end + offset_adjust;

                            if (adj_start < 0 ||
                                adj_end > (int)strlen(working_query))
                                continue;

                            new_len = strlen(working_query) -
                                  (adj_end - adj_start) + repl_len;
                            new_query = (char *)pg_malloc(new_len + 1);

                            memcpy(new_query, working_query, adj_start);
                            memcpy(new_query + adj_start, replacement,
                                   repl_len);
                            memcpy(new_query + adj_start + repl_len,
                                   working_query + adj_end,
                                   strlen(working_query) - adj_end + 1);

                            pg_free(working_query);
                            working_query = new_query;
                            offset_adjust += repl_len -
                                           (adj_end - adj_start);
                        }
                    }
                }
            }
        }

        tecc_processor_debug(TECC_LOG_DEBUG1,
            "CTAS rewrit: %s", working_query);

        if (statement_data->rewritten_query)
            pg_free(statement_data->rewritten_query);
        statement_data->rewritten_query = working_query;
        statement_data->needs_rewrite = 1;

        return TECC_PROCESSOR_SUCCESS;
    }

    return TECC_PROCESSOR_SUCCESS;

ctas_select_error:
    return TECC_PROCESSOR_ERROR;
}

/*
 * Handle DECLARE CURSOR statement
 * Processes the inner SELECT statement for encryption rewriting.
 */
static TeccProcessorResult
processor_handle_declare_cursor_statement(const Node *stmt,
                                         TeccStatementData *statement_data)
{
    const DeclareCursorStmt *declare_stmt;

    if (stmt == NULL || statement_data == NULL)
        return TECC_PROCESSOR_INVALID_INPUT;

    declare_stmt = (const DeclareCursorStmt *)stmt;

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    /* Process the inner SELECT statement recursively for encryption rewriting */
    if (declare_stmt->query != NULL)
    {
        TeccProcessorResult inner_result;

        inner_result = processor_process_statement(declare_stmt->query,
                                                   statement_data);

        /* Propagate NEEDS_REWRITE or errors from inner query processing */
        if (inner_result == TECC_PROCESSOR_NEEDS_REWRITE)
        {
            /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
            
            /* End - ReqID:TECC4.0 - 线程安全调试支持 */
            return TECC_PROCESSOR_NEEDS_REWRITE;
        }

        if (inner_result != TECC_PROCESSOR_SUCCESS &&
            inner_result != TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS)
        {
            /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
            tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                                   "DECLARE CURSOR inner SELECT processing failed: %d",
                                   inner_result);
            /* End - ReqID:TECC4.0 - 线程安全调试支持 */
            return inner_result;
        }
    }

    /* Begin - ReqID:TECC4.0 - 线程安全调试支持 */
    
    /* End - ReqID:TECC4.0 - 线程安全调试支持 */

    return TECC_PROCESSOR_SUCCESS;
}

/*
 * Handle SET statement in processor
 * GUC processor module has been removed - function removed
 */

/*
 * Apply GUC parameters to query in processor
 * GUC processor module has been removed - function removed
 */

/*
 * Sync GUC parameters from server in processor
 * GUC processor module has been removed - function removed
 */

/*
 * Check if query is SET statement
 * GUC processor module has been removed - function commented out
 */
/* Begin - ReqID:TECC4.0 - GUC processor removed
bool
tecc_processor_is_set_statement(const char *query)
{
    if (query == NULL) {
        return false;
    }

    return tecc_guc_processor_is_set_statement(query);
}
End - ReqID:TECC4.0 - GUC processor removed */

/*
 * Parse SET statement in processor
 * GUC processor module has been removed - function removed
 */

/*
 * Get GUC processor context from statement data
 * GUC processor module has been removed - function commented out
 */
/* Begin - ReqID:TECC4.0 - GUC processor removed
TeccGucProcessorContext *
tecc_processor_get_guc_context(TeccStatementData *statement_data)
{
    if (statement_data == NULL) {
        return NULL;
    }

    // Initialize GUC processor if not already initialized
    if (statement_data->guc_processor_ctx == NULL) {
        tecc_processor_init_guc_processor(statement_data);
    }

    return (TeccGucProcessorContext *)statement_data->guc_processor_ctx;
}
End - ReqID:TECC4.0 - GUC processor removed */

/*
 * Set GUC processor context in statement data
 * GUC processor module has been removed - function commented out
 */
/* Begin - ReqID:TECC4.0 - GUC processor removed
void
tecc_processor_set_guc_context(TeccStatementData *statement_data,
                              TeccGucProcessorContext *guc_ctx)
{
    if (statement_data == NULL) {
        return;
    }

    // Clean up existing context if any
    if (statement_data->guc_processor_ctx != NULL) {
        tecc_processor_cleanup_guc_processor(statement_data);
    }

    // Set new context
    statement_data->guc_processor_ctx = guc_ctx;
}
End - ReqID:TECC4.0 - GUC processor removed */

/* ==================== COPY Row Processing Functions ==================== */
/* Begin - ReqID:TECC4.0 - COPY row processing functions for import/export module */

/*
 * tecc_processor_process_copy_row - Process COPY data row for encryption (import)
 *
 * This function processes a single COPY data row for import operations.
 * It parses the row data, identifies encrypted columns, encrypts the data
 * for those columns, and returns the processed row.
 *
 * Parameters:
 *   copy_state - Enhanced COPY state structure
 *   row_data - Original row data (CSV/TEXT format)
 *   row_len - Length of row data
 *   processed_row - Output: pointer to processed row data (allocated with pg_malloc)
 *   processed_len - Output: length of processed row data
 *
 * Returns:
 *   TECC_PROCESSOR_SUCCESS on success
 *   TECC_PROCESSOR_ERROR on error
 */
TeccProcessorResult
tecc_processor_process_copy_row(
    TeccEnhancedCopyState *copy_state,
    const char *row_data,
    int row_len,
    char **processed_row,
    int *processed_len)
{
    /* Begin - ReqID:TECC4.0 */
    char **fields = NULL;
    int field_count = 0;
    int i;
    TeccProcessorResult result = TECC_PROCESSOR_SUCCESS;
    /* Key management variables */
    char *kek_name = NULL;
    char *dek_name = NULL;
    int key_ret = 0;
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    bool *column_is_encrypted = NULL;
    TeccEncryptedColumnMetadata **column_encryption_info = NULL;
    int effective_column_count = 0;

    /* Validate parameters */
    if (copy_state == NULL || row_data == NULL || processed_row == NULL || processed_len == NULL) {
        return TECC_PROCESSOR_INVALID_INPUT;
    }

    /* Validate row_len parameter */
    if (row_len <= 0) {
        return TECC_PROCESSOR_INVALID_INPUT;
    }

    /* Initialize output parameters */
    *processed_row = NULL;
    *processed_len = 0;

    /* Parse row data into fields */
    result = tecc_processor_parse_copy_row(copy_state, row_data, row_len,
                                          &field_count, &fields, NULL);
    if (result != TECC_PROCESSOR_SUCCESS) {
        /* Parse failed, go to cleanup to free any allocated resources */
        tecc_processor_debug_safe(NULL, TECC_LOG_ERROR,
                                 "Failed to parse COPY row data, result: %d", result);
        goto cleanup;
    }

    /* Check if we have encrypted columns */
    if (copy_state->encrypted_column_count == 0) {
        /* No encrypted columns, just return original row */
        /* Validate row_len before allocation */
        if (row_len < 0) {
            tecc_copy_free_parsed_fields(&fields, NULL, field_count);
            return TECC_PROCESSOR_INVALID_INPUT;
        }

        *processed_row = pg_malloc(row_len + 1);
        if (*processed_row == NULL) {
            tecc_copy_free_parsed_fields(&fields, NULL, field_count);
            return TECC_PROCESSOR_MEMORY_ERROR;
        }
        memcpy(*processed_row, row_data, row_len);
        (*processed_row)[row_len] = '\0';
        *processed_len = row_len;

        /* Free parsed fields */
        tecc_copy_free_parsed_fields(&fields, NULL, field_count);
        return TECC_PROCESSOR_SUCCESS;
    }

    /*
     * Determine effective column count for encryption loops.
     * When column_count is 0 (COPY without explicit column list),
     * use field_count since fields are in table column order.
     */
    effective_column_count = (copy_state->column_count > 0) ? copy_state->column_count : field_count;

    /* Create column encryption status map for performance optimization */
    column_is_encrypted = NULL;
    column_encryption_info = NULL;

    /* Begin - ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    {
        int j = 0;

        if (copy_state->encrypted_column_count > 0) {
            column_is_encrypted = (bool *)pg_malloc(sizeof(bool) * effective_column_count);
            column_encryption_info = (TeccEncryptedColumnMetadata **)pg_malloc(
                sizeof(TeccEncryptedColumnMetadata *) * effective_column_count);

            if (column_is_encrypted == NULL || column_encryption_info == NULL) {
                if (column_is_encrypted != NULL) pg_free(column_is_encrypted);
                if (column_encryption_info != NULL) pg_free(column_encryption_info);
                result = TECC_PROCESSOR_MEMORY_ERROR;
                goto cleanup;
            }

            /* Initialize arrays */
            for (i = 0; i < effective_column_count; i++) {
                column_is_encrypted[i] = false;
                column_encryption_info[i] = NULL;
            }

            /* Fill encryption info */
            for (j = 0; j < copy_state->encrypted_column_count; j++) {
                TeccEncryptedColumnMetadata *col_info = copy_state->encrypted_column_info[j];
                if (col_info == NULL || col_info->column_name == NULL)
                    continue;
                if (copy_state->column_count > 0 && copy_state->column_names != NULL) {
                    /* Explicit column list: match by column name */
                    int ci;
                    for (ci = 0; ci < copy_state->column_count; ci++) {
                        if (copy_state->column_names[ci] != NULL &&
                            strcmp(copy_state->column_names[ci],
                                   col_info->column_name) == 0) {
                            if (ci < effective_column_count) {
                                column_is_encrypted[ci] = true;
                                column_encryption_info[ci] = col_info;
                            }
                            break;
                        }
                    }
                } else {
                    /* No column list: use attnum-based matching */
                    if (col_info->column_attnum > 0 &&
                        col_info->column_attnum <= effective_column_count) {
                        int attnum_idx = col_info->column_attnum - 1;
                        column_is_encrypted[attnum_idx] = true;
                        column_encryption_info[attnum_idx] = col_info;
                    }
                }
            }
        }
    }
    /* End - ReqID:TECC4.0 - Fix C90 mixed declaration warning */

    /* Process each field */
    for (i = 0; i < field_count && i < effective_column_count; i++) {
        /* Check if this column is encrypted using pre-computed map */
        if (column_is_encrypted != NULL && column_is_encrypted[i]) {
            TeccEncryptedColumnMetadata *col_info = column_encryption_info[i];
            if (col_info != NULL) {
                /* This column is encrypted */

                /* ReqID:TECC4.0 — NULL value, skip encryption */
                if (fields[i] == NULL) {
                    continue;
                }

                /* Encrypt the field value */
                /* Call encryption scheduler */
                    TeccUnifiedEncryptResult *encrypt_result = NULL;

                    /* Get KEK and DEK names for this column */
                    if (copy_state->conn != NULL) {
                        key_ret = tecc_processor_get_key_names(copy_state->conn,
                                                              copy_state->table_name,
                                                              col_info->column_name,
                                                              &kek_name,
                                                              &dek_name);

                        if (key_ret != 0) {
                            /* Failed to get key names */
                            tecc_copy_state_add_error(copy_state,
                                TECC_COPY_KEY_INVALID,
                                "Failed to get key names for encrypted column",
                                copy_state->current_row,
                                i + 1);
                            result = TECC_PROCESSOR_ENCRYPTION_ERROR;
                            goto cleanup;
                        }

                        /* Validate key names */
                        if (!tecc_processor_validate_key_names(copy_state->conn, kek_name, dek_name)) {
                            tecc_copy_state_add_error(copy_state,
                                TECC_COPY_KEY_INVALID,
                                "Invalid or expired keys for encrypted column",
                                copy_state->current_row,
                                i + 1);
                            result = TECC_PROCESSOR_ENCRYPTION_ERROR;
                            goto cleanup;
                        }
                    }

                    /* Determine which encryption function to use based on connection */
                    if (copy_state->conn != NULL) {
                        /* Use thread-safe version with connection and actual key names */
                        encrypt_result = tecc_unified_encrypt_string(
                            copy_state->conn,
                            fields[i],
                            copy_state->table_name,
                            col_info->column_name,
                            kek_name,  /* Use actual KEK name */
                            dek_name,  /* Use actual DEK name */
                            NULL
                        );
                    } else {
                        /* Use non-thread-safe version (deprecated, for backward compatibility) */
                        encrypt_result = tecc_unified_encrypt_string(
                            NULL,  /* conn is NULL for non-thread-safe mode */
                            fields[i],
                            copy_state->table_name,
                            col_info->column_name,
                            NULL,  /* kek_name */
                            NULL,  /* dek_name */
                            NULL
                        );
                    }

                    if (encrypt_result != NULL && encrypt_result->error_code == TECC_CRYPTO_SUCCESS) {
                        /* Replace field with encrypted hex string.
                         * Strip \x prefix: hex_encode adds \x for
                         * SQL literal format, but COPY TEXT mode
                         * interprets \xNN as hex-escape. Plain hex
                         * (without prefix) is accepted by cipherdata_in. */
                        {
                            const char *hex_val = encrypt_result->hex_string;
                            if (hex_val[0] == '\\' && hex_val[1] == 'x')
                                hex_val += 2;
                            pg_free(fields[i]);
                            fields[i] = pstrdup(hex_val);
                        }

                        /* Free encryption result */
                        tecc_unified_free_encrypt_result(encrypt_result);
                    } else {
                        /* Encryption failed */
                        if (encrypt_result != NULL) {
                            /* Log error */
                            tecc_copy_state_add_error(copy_state,
                                TECC_COPY_ENCRYPTION_ERROR,
                                encrypt_result->error_msg,
                                copy_state->current_row,
                                i + 1);

                            tecc_unified_free_encrypt_result(encrypt_result);
                        }

                        result = TECC_PROCESSOR_ENCRYPTION_ERROR;
                        break;
                    }
                }
            }
        }

    /* Format fields back into row */
    if (result == TECC_PROCESSOR_SUCCESS) {
        int *field_lengths = NULL;
        int i;


        /* Create field lengths array */
        field_lengths = (int *)pg_malloc(sizeof(int) * field_count);
        for (i = 0; i < field_count; i++) {
            field_lengths[i] = fields[i] ? strlen(fields[i]) : 0;
        }

        /* Format the row */
        result = tecc_processor_format_copy_row(copy_state,
                                               (const char **)fields,
                                               field_lengths,
                                               field_count,
                                               processed_row,
                                               processed_len);

        pg_free(field_lengths);
    }

cleanup:

    /* Free encryption column arrays if allocated */
    if (column_is_encrypted != NULL) {
        pg_free(column_is_encrypted);
        column_is_encrypted = NULL;
    }
    if (column_encryption_info != NULL) {
        pg_free(column_encryption_info);
        column_encryption_info = NULL;
    }

    /* Free key names if allocated */
    if (kek_name != NULL) {
        pg_free(kek_name);
        kek_name = NULL;
    }
    if (dek_name != NULL) {
        pg_free(dek_name);
        dek_name = NULL;
    }

    /* Free parsed fields */
    tecc_copy_free_parsed_fields(&fields, NULL, field_count);

    /* Update row counter */
    copy_state->current_row++;

    /* Add debug log for completion */
    tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                             "COPY row processing completed, result: %d, row: %d",
                             result, copy_state->current_row);

    return result;
    /* End - ReqID:TECC4.0 */
}

/*
 * tecc_processor_process_copy_result_row - Process COPY result row for decryption (export)
 *
 * This function processes a single COPY result row for export operations.
 * It parses the encrypted row data, identifies encrypted columns, decrypts
 * the data for those columns, and returns the decrypted row.
 *
 * Parameters:
 *   copy_state - Enhanced COPY state structure
 *   encrypted_row - Encrypted row data
 *   row_len - Length of row data
 *   processed_row - Output: pointer to decrypted row data (allocated with pg_malloc)
 *   processed_len - Output: length of decrypted row data
 *
 * Returns:
 *   TECC_PROCESSOR_SUCCESS on success
 *   TECC_PROCESSOR_ERROR on error
 */
TeccProcessorResult
tecc_processor_process_copy_result_row(
    TeccEnhancedCopyState *copy_state,
    const char *encrypted_row,
    int row_len,
    char **processed_row,
    int *processed_len)
{
    /* Begin - ReqID:TECC4.0 */
    char **fields = NULL;
    int field_count = 0;
    int i;
    TeccProcessorResult result = TECC_PROCESSOR_SUCCESS;
    /* Key management variables (for consistency with process_copy_row) */
    char *kek_name = NULL;
    char *dek_name = NULL;

    /* Validate parameters */
    if (copy_state == NULL || encrypted_row == NULL || processed_row == NULL || processed_len == NULL) {
        return TECC_PROCESSOR_INVALID_INPUT;
    }

    /* Initialize output parameters */
    *processed_row = NULL;
    *processed_len = 0;

    /* Parse row data into fields */
    result = tecc_processor_parse_copy_row(copy_state, encrypted_row, row_len,
                                          &field_count, &fields, NULL);
    if (result != TECC_PROCESSOR_SUCCESS) {
        return result;
    }

    /* Check if we have encrypted columns */
    if (copy_state->encrypted_column_count == 0) {
        /* No encrypted columns, just return original row */
        *processed_row = pg_malloc(row_len + 1);
        if (*processed_row == NULL) {
            tecc_copy_free_parsed_fields(&fields, NULL, field_count);
            return TECC_PROCESSOR_MEMORY_ERROR;
        }
        memcpy(*processed_row, encrypted_row, row_len);
        (*processed_row)[row_len] = '\0';
        *processed_len = row_len;

        /* Free parsed fields */
        tecc_copy_free_parsed_fields(&fields, NULL, field_count);
        return TECC_PROCESSOR_SUCCESS;
    }

    /* Process each field */
    int max_fields_to_process = (copy_state->column_count > 0) ? copy_state->column_count : field_count;
    for (i = 0; i < field_count && i < max_fields_to_process; i++) {
        /* Check if this column is encrypted */
        int j;

        for (j = 0; j < copy_state->encrypted_column_count; j++) {
            TeccEncryptedColumnMetadata *col_info = copy_state->encrypted_column_info[j];
            if (col_info == NULL || col_info->column_name == NULL)
                continue;
            /* Match column: by name when explicit column list, by attnum otherwise */
            if (!((copy_state->column_count > 0 && copy_state->column_names != NULL &&
                   i < copy_state->column_count && copy_state->column_names[i] != NULL &&
                   strcmp(copy_state->column_names[i], col_info->column_name) == 0) ||
                  (copy_state->column_count == 0 &&
                   col_info->column_attnum == i + 1)))
                continue;
            /* This column is encrypted */

                /* Decrypt the field value */
                if (fields[i] != NULL) {
                    /* Check if this looks like a hex-encoded cipherdata.
                     * Detect and strip \x/x prefix — tecc_unified_decrypt_hex
                     * expects pure hex digits, no prefix. */
                    bool is_hex_string = false;
                    size_t field_len = strlen(fields[i]);
                    const char *hex_start = fields[i];
                    size_t hex_len = field_len;

                    /* Strip \x prefix (backslash was preserved by parser in some cases) */
                    if (field_len > 2 && fields[i][0] == '\\' &&
                        (fields[i][1] == 'x' || fields[i][1] == 'X')) {
                        hex_start = fields[i] + 2;
                        hex_len = field_len - 2;
                    }
                    /* Strip bare x prefix (backslash was consumed as escape by COPY text parser) */
                    else if (field_len > 1 &&
                             (fields[i][0] == 'x' || fields[i][0] == 'X')) {
                        hex_start = fields[i] + 1;
                        hex_len = field_len - 1;
                    }

                    /* Validate that remaining content is all hex digits */
                    if (hex_len > 0) {
                        size_t j;
                        is_hex_string = true;
                        for (j = 0; j < hex_len; j++) {
                            char c = hex_start[j];
                            if (!((c >= '0' && c <= '9') ||
                                  (c >= 'a' && c <= 'f') ||
                                  (c >= 'A' && c <= 'F'))) {
                                is_hex_string = false;
                                break;
                            }
                        }
                    }

                    if (is_hex_string) {
                        /* Call decryption scheduler */
                        TeccUnifiedDecryptResult *decrypt_result = NULL;

                        /* Use thread-safe version with connection */
                        decrypt_result = tecc_unified_decrypt_hex(
                            copy_state->conn,
                            hex_start,   /* pass pure hex without \x/x prefix */
                            copy_state->table_name,
                            col_info->column_name,
                            NULL
                        );

                        if (decrypt_result != NULL && decrypt_result->error_code == TECC_CRYPTO_SUCCESS) {
                            /* Convert decrypted binary to text using type output */
                            pg_free(fields[i]);
                            fields[i] = tecc_type_output_data(
                                col_info->data_type_original_oid,
                                col_info->data_type_original_mod,
                                decrypt_result->plaintext,
                                decrypt_result->plain_len);

                            /* ReqID:TECC4.0 — NULL decryption result, keep as NULL
                             * (tecc_processor_format_copy_row will write \N) */
                            if (fields[i] == NULL) {
                                /* fields[i] stays NULL — formatter handles \N */
                            }

                            /* Free decryption result */
                            tecc_unified_free_decrypt_result(decrypt_result);
                        } else {
                            /* Decryption failed */
                            if (decrypt_result != NULL) {
                                /* Log error */
                                tecc_copy_state_add_error(copy_state,
                                    TECC_COPY_DECRYPTION_ERROR,
                                    decrypt_result->error_msg,
                                    copy_state->current_row,
                                    i + 1);

                                tecc_unified_free_decrypt_result(decrypt_result);
                            }

                            result = TECC_PROCESSOR_DECRYPTION_ERROR;
                            break;
                        }
                    }
                    /* If not a hex string, assume it's already plaintext (e.g., NULL value) */
                }
                break;
            }
        }

    /* Format fields back into row */
    if (result == TECC_PROCESSOR_SUCCESS) {
        int *field_lengths = NULL;
        int i;

        /* Create field lengths array */
        field_lengths = (int *)pg_malloc(sizeof(int) * field_count);
        for (i = 0; i < field_count; i++) {
            field_lengths[i] = fields[i] ? strlen(fields[i]) : 0;
        }

        /* Format the row */
        result = tecc_processor_format_copy_row(copy_state,
                                               (const char **)fields,
                                               field_lengths,
                                               field_count,
                                               processed_row,
                                               processed_len);

        pg_free(field_lengths);
    }



    /* Free key names if allocated */
    if (kek_name != NULL) {
        pg_free(kek_name);
        kek_name = NULL;
    }
    if (dek_name != NULL) {
        pg_free(dek_name);
        dek_name = NULL;
    }

    /* Free parsed fields */
    tecc_copy_free_parsed_fields(&fields, NULL, field_count);

    /* Update row counter */
    copy_state->current_row++;

    /* Add debug log for completion */
    tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
                             "COPY row processing completed, result: %d, row: %d",
                             result, copy_state->current_row);

    return result;
    /* End - ReqID:TECC4.0 */
}

/*
 * tecc_processor_init_copy_context - Initialize COPY processing context
 *
 * This function initializes the processor-specific context within the
 * COPY state structure.
 *
 * Parameters:
 *   copy_state - Enhanced COPY state structure
 *
 * Returns:
 *   TECC_PROCESSOR_SUCCESS on success
 *   TECC_PROCESSOR_ERROR on error
 */
TeccProcessorResult
tecc_processor_init_copy_context(TeccEnhancedCopyState *copy_state)
{
    /* Begin - ReqID:TECC4.0 */
    if (copy_state == NULL) {
        return TECC_PROCESSOR_INVALID_INPUT;
    }

    /* Check if already initialized */
    if (copy_state->processor_ctx != NULL) {
        return TECC_PROCESSOR_SUCCESS;
    }

    /* Initialize processor context */
    /* For now, we don't need a complex context structure */
    copy_state->processor_ctx = pg_malloc0(sizeof(int)); /* Placeholder */

    /* Mark as initialized */
    copy_state->initialized = true;

    return TECC_PROCESSOR_SUCCESS;
    /* End - ReqID:TECC4.0 */
}

/*
 * tecc_processor_cleanup_copy_context - Clean up COPY processing context
 *
 * This function cleans up the processor-specific context within the
 * COPY state structure.
 *
 * Parameters:
 *   copy_state - Enhanced COPY state structure
 */
void
tecc_processor_cleanup_copy_context(TeccEnhancedCopyState *copy_state)
{
    /* Begin - ReqID:TECC4.0 */
    if (copy_state == NULL) {
        return;
    }

    /* Clean up processor context */
    if (copy_state->processor_ctx != NULL) {
        pg_free(copy_state->processor_ctx);
        copy_state->processor_ctx = NULL;
    }

    /* Mark as not initialized */
    copy_state->initialized = false;
    /* End - ReqID:TECC4.0 */
}

/* End - ReqID:TECC4.0 - COPY row processing functions for import/export module */

/* Begin - ReqID:TECC4.0 - COPY format parsing functions for import/export module */

/*
 * tecc_processor_parse_copy_row - Parse CSV/TEXT format row into fields
 *
 * This function parses a CSV or TEXT format row into individual fields,
 * handling delimiters, quotes, escapes, and NULL values according to
 * PostgreSQL COPY command specifications.
 *
 * Parameters:
 *   copy_state - Enhanced COPY state structure
 *   row_data - Input row data
 *   row_len - Length of row data
 *   field_count - Output: number of fields parsed
 *   field_values - Output: array of field values (allocated with pg_malloc)
 *   field_lengths - Output: array of field lengths
 *
 * Returns:
 *   TECC_PROCESSOR_SUCCESS on success
 *   TECC_PROCESSOR_INVALID_INPUT on invalid parameters
 */
TeccProcessorResult
tecc_processor_parse_copy_row(
    TeccEnhancedCopyState *copy_state,
    const char *row_data,
    int row_len,
    int *field_count,
    char ***field_values,
    int **field_lengths)
{
    /* Begin - ReqID:TECC4.0 */
    int i;
    char delimiter = ',';
    char quote_char = '"';
    char escape_char = '\\';
    const char *null_string = "\\N";
    bool in_quotes = false;
    bool in_escape = false;
    bool was_quoted = false;
    int field_start = 0;
    int field_len = 0;
    int max_fields = 128;
    char **fields = NULL;
    int *field_lens = NULL;
    int fields_allocated = 0;
    int fields_found = 0;

    /* Validate input parameters */
    if (copy_state == NULL || row_data == NULL || row_len <= 0 ||
        field_count == NULL || field_values == NULL) {
        return TECC_PROCESSOR_INVALID_INPUT;
    }

    /* Initialize output parameters */
    *field_count = 0;
    *field_values = NULL;
    if (field_lengths != NULL)
        *field_lengths = NULL;

    /* Get COPY format options from state */
    if (copy_state->csv_mode) {
        /* CSV format uses comma delimiter and double quotes */
        delimiter = copy_state->delimiter ? copy_state->delimiter : ',';
        quote_char = copy_state->quote ? copy_state->quote : '"';
        /* In CSV mode, quote char doubles as escape */
    } else {
        /* TEXT format uses tab delimiter and no quotes */
        delimiter = copy_state->delimiter ? copy_state->delimiter : '\t';
        quote_char = '\0';  /* No quote character in TEXT mode */
    }

    /* Allocate initial field arrays */
    fields_allocated = max_fields;
    fields = (char **)pg_malloc(sizeof(char *) * fields_allocated);
    field_lens = (int *)pg_malloc(sizeof(int) * fields_allocated);

    /* Parse the row */
    for (i = 0; i < row_len; i++) {
        char ch = row_data[i];

        /* Handle escape sequences */
        if (in_escape) {
            in_escape = false;
            field_len++;
            continue;
        }

        /* Handle quote characters */
        if (quote_char != '\0' && ch == quote_char) {
            if (!in_quotes) {
                /* Start of quoted field */
                in_quotes = true;
                field_start = i + 1;  /* Skip the opening quote */
            } else {
                /* Check if this is an escaped quote (two quotes in a row) */
                if (i + 1 < row_len && row_data[i + 1] == quote_char) {
                    /* Escaped quote - skip one and continue */
                    field_len++;
                    i++;  /* Skip the second quote */
                } else {
                    /* End of quoted field */
                    in_quotes = false;
                    was_quoted = true;
                    /* Don't save field here — let the following delimiter (or end-of-row)
                     * handle it, so we don't double-count fields when a comma follows.
                     * field_start and field_len already point to the quoted content. */
                }
            }
            continue;
        }

        /* Handle delimiter when not in quotes */
        if (!in_quotes && ch == delimiter) {
            /* End of field */
            if (field_len > 0 || was_quoted || i == 0 || (i > 0 && row_data[i - 1] == delimiter)) {
                /* Save the field (empty fields are allowed) */
                if (fields_found >= fields_allocated) {
                    /* Resize arrays */
                    fields_allocated *= 2;
                    fields = (char **)pg_realloc(fields, sizeof(char *) * fields_allocated);
                    field_lens = (int *)pg_realloc(field_lens, sizeof(int) * fields_allocated);
                }

                if (field_len > 0) {
                    int raw_len;
                    /* Use i - field_start for raw data length. If the field was quoted
                     * (ended with quote_char), exclude the closing quote from content. */
                    if (row_data[i - 1] == quote_char && in_quotes == false)
                        raw_len = i - 1 - field_start;
                    else
                        raw_len = i - field_start;
                    fields[fields_found] = (char *)pg_malloc(raw_len + 1);
                    memcpy(fields[fields_found], row_data + field_start, raw_len);
                    fields[fields_found][raw_len] = '\0';
                    field_len = raw_len;
                } else {
                    /* Empty field */
                    if (copy_state->csv_mode && !was_quoted) {
                        /* Unquoted empty field is NULL in CSV */
                        fields[fields_found] = NULL;
                    } else {
                        /* Quoted empty field (""), or TEXT mode — empty string */
                        fields[fields_found] = (char *)pg_malloc(1);
                        fields[fields_found][0] = '\0';
                    }
                    was_quoted = false;
                }
                field_lens[fields_found] = field_len;

                /* ReqID:TECC4.0 — Collapse escaped quotes "" → " in CSV mode */
                if (copy_state->csv_mode && field_len > 0)
                {
                    char *src = fields[fields_found];
                    int src_len = field_len;
                    int dst_len = 0;
                    int j;
                    for (j = 0; j < src_len; j++)
                    {
                        if (src[j] == quote_char && j + 1 < src_len && src[j + 1] == quote_char)
                        {
                            src[dst_len++] = quote_char;
                            j++; /* skip the second quote */
                        }
                        else
                        {
                            src[dst_len++] = src[j];
                        }
                    }
                    src[dst_len] = '\0';
                    field_lens[fields_found] = dst_len;
                }
                fields_found++;
            }

            /* Reset for next field */
            field_start = i + 1;
            field_len = 0;
            was_quoted = false;
            continue;
        }

        /* Handle escape character */
        if (ch == escape_char && !copy_state->csv_mode) {
            if (field_len == 0) {
                /* ReqID:TECC4.0 — Check for \N (COPY NULL marker).
                 * \N must be preserved as-is so the NULL check at end of this
                 * function can match field_len==2 && field[0]=='\\' && field[1]=='N'.
                 * Other escape sequences (like \t, \n) still consume the backslash. */
                if (i + 1 < row_len && row_data[i + 1] == 'N')
                {
                    /* \N — keep backslash as part of field content */
                    field_start = i;
                    field_len = 1;
                    in_escape = true;
                    continue;
                }
                /* Skip the backslash at start of field — it's an escape prefix */
                field_start = i + 1;
            }
            in_escape = true;
            continue;
        }

        /* Normal character */
        if (!in_quotes || ch != '\0') {
            if (field_len == 0) {
                field_start = i;
            }
            field_len++;
        }
    }

    /* Handle the last field */
    if (field_len > 0 || was_quoted || (row_len > 0 && row_data[row_len - 1] == delimiter)) {
        if (fields_found >= fields_allocated) {
            /* Resize arrays */
            fields_allocated += 1;
            fields = (char **)pg_realloc(fields, sizeof(char *) * fields_allocated);
            field_lens = (int *)pg_realloc(field_lens, sizeof(int) * fields_allocated);
        }

        if (field_len > 0) {
            int raw_len = row_len - field_start;
            fields[fields_found] = (char *)pg_malloc(raw_len + 1);
            memcpy(fields[fields_found], row_data + field_start, raw_len);
            fields[fields_found][raw_len] = '\0';
            field_len = raw_len;
        } else {
            /* Empty field */
            if (copy_state->csv_mode && !was_quoted) {
                /* Unquoted empty field is NULL in CSV */
                fields[fields_found] = NULL;
            } else {
                fields[fields_found] = (char *)pg_malloc(1);
                fields[fields_found][0] = '\0';
            }
            was_quoted = false;
        }
        field_lens[fields_found] = field_len;

        /* ReqID:TECC4.0 — Collapse escaped quotes "" → " in CSV mode (last field) */
        if (copy_state->csv_mode && field_len > 0)
        {
            char *src = fields[fields_found];
            int src_len = field_len;
            int dst_len = 0;
            int j;
            for (j = 0; j < src_len; j++)
            {
                if (src[j] == quote_char && j + 1 < src_len && src[j + 1] == quote_char)
                {
                    src[dst_len++] = quote_char;
                    j++;
                }
                else
                {
                    src[dst_len++] = src[j];
                }
            }
            src[dst_len] = '\0';
            field_lens[fields_found] = dst_len;
        }
        fields_found++;
    }

    /* Check for NULL string values */
    for (i = 0; i < fields_found; i++) {
        if (field_lens[i] == strlen(null_string) &&
            strncmp(fields[i], null_string, field_lens[i]) == 0) {
            /* This is a NULL value */
            pg_free(fields[i]);
            fields[i] = NULL;
            field_lens[i] = 0;
        }
    }

    /* Return results */
    *field_count = fields_found;
    *field_values = fields;
    if (field_lengths != NULL) *field_lengths = field_lens;

    return TECC_PROCESSOR_SUCCESS;
    /* End - ReqID:TECC4.0 */
}

/*
 * tecc_processor_format_copy_row - Format fields into CSV/TEXT row
 *
 * This function formats individual fields into a CSV or TEXT format row,
 * handling delimiters, quotes, escapes, and NULL values according to
 * PostgreSQL COPY command specifications.
 *
 * Parameters:
 *   copy_state - Enhanced COPY state structure
 *   field_values - Array of field values
 *   field_lengths - Array of field lengths
 *   field_count - Number of fields
 *   formatted_row - Output: formatted row (allocated with pg_malloc)
 *   formatted_len - Output: length of formatted row
 *
 * Returns:
 *   TECC_PROCESSOR_SUCCESS on success
 *   TECC_PROCESSOR_FORMAT_ERROR on format error
 *   TECC_PROCESSOR_INVALID_INPUT on invalid parameters
 */
TeccProcessorResult
tecc_processor_format_copy_row(
    TeccEnhancedCopyState *copy_state,
    const char **field_values,
    const int *field_lengths,
    int field_count,
    char **formatted_row,
    int *formatted_len)
{
    /* Begin - ReqID:TECC4.0 */
    int i;
    char delimiter = ',';
    char quote_char = '"';
    const char *null_string = NULL;
    int total_len = 0;
    char *result = NULL;
    int pos = 0;
    bool needs_quotes;

    /* Validate input parameters */
    if (copy_state == NULL || field_values == NULL || field_lengths == NULL ||
        formatted_row == NULL || formatted_len == NULL) {
        return TECC_PROCESSOR_INVALID_INPUT;
    }

    /* Initialize output parameters */
    *formatted_row = NULL;
    *formatted_len = 0;

    /* Get COPY format options from state */
    if (copy_state->csv_mode) {
        /* CSV format uses comma delimiter and double quotes */
        delimiter = copy_state->delimiter ? copy_state->delimiter : ',';
        quote_char = copy_state->quote ? copy_state->quote : '"';
        /* CSV NULL is empty field */
        null_string = "";
    } else {
        /* TEXT format uses tab delimiter and no quotes */
        delimiter = copy_state->delimiter ? copy_state->delimiter : '\t';
        quote_char = '\0';  /* No quote character in TEXT mode */
        /* TEXT NULL is \N */
        null_string = "\\N";
    }

    /* First pass: calculate total length needed */
    for (i = 0; i < field_count; i++) {
        const char *field = field_values[i];
        int field_len = field_lengths[i];

        if (field == NULL) {
            /* NULL value */
            total_len += strlen(null_string);
        } else {
            /* Check if field needs quoting */
            needs_quotes = false;

            if (copy_state->csv_mode) {
                /* In CSV mode, fields containing delimiter, quote, or newline need quotes */
                {
                    int j;
                    for (j = 0; j < field_len; j++) {
                        char ch = field[j];
                        if (ch == delimiter || ch == quote_char || ch == '\n' || ch == '\r') {
                            needs_quotes = true;
                            break;
                        }
                    }
                }

                /* Also quote empty fields */
                if (field_len == 0) {
                    needs_quotes = true;
                }
            }

            if (needs_quotes) {
                /* Account for opening and closing quotes */
                total_len += 2;

                /* Account for escaped quotes within the field */
                {
                    int j;
                    for (j = 0; j < field_len; j++) {
                        if (field[j] == quote_char) {
                            total_len++;  /* Each quote needs to be doubled */
                        }
                    }
                }
            }

            total_len += field_len;
        }

        /* Account for delimiter (except after last field) */
        if (i < field_count - 1) {
            total_len++;
        }
    }

    /* Allocate result buffer */
    result = (char *)pg_malloc(total_len + 1);  /* +1 for null terminator */

    /* Second pass: build the formatted row */
    for (i = 0; i < field_count; i++) {
        const char *field = field_values[i];
        int field_len = field_lengths[i];
        bool needs_quotes = false;

        if (i > 0) {
            /* Add delimiter */
            result[pos++] = delimiter;
        }

        if (field == NULL) {
            /* NULL value */
            strcpy(result + pos, null_string);
            pos += strlen(null_string);
            continue;
        }

        /* Check if field needs quoting */
        needs_quotes = false;
        if (copy_state->csv_mode) {
            /* In CSV mode, fields containing delimiter, quote, or newline need quotes */
            {
                int j;
                for (j = 0; j < field_len; j++) {
                    char ch = field[j];
                    if (ch == delimiter || ch == quote_char || ch == '\n' || ch == '\r') {
                        needs_quotes = true;
                        break;
                    }
                }
            }

            /* Also quote empty fields */
            if (field_len == 0) {
                needs_quotes = true;
            }
        }

        if (needs_quotes) {
            /* Add opening quote */
            result[pos++] = quote_char;

            /* Copy field content, doubling any quote characters */
            {
                int j;
                for (j = 0; j < field_len; j++) {
                    char ch = field[j];
                    result[pos++] = ch;
                    if (ch == quote_char) {
                        /* Double the quote character */
                        result[pos++] = quote_char;
                    }
                }
            }

            /* Add closing quote */
            result[pos++] = quote_char;
        } else {
            /* Copy field content directly */
            memcpy(result + pos, field, field_len);
            pos += field_len;
        }
    }

    /* Null-terminate the result */
    result[pos] = '\0';

    /* Return results */
    *formatted_row = result;
    *formatted_len = pos;

    return TECC_PROCESSOR_SUCCESS;
    /* End - ReqID:TECC4.0 */
}

/*
 * tecc_copy_free_parsed_fields - Free memory allocated by parse_copy_row
 *
 * This function frees the memory allocated by tecc_processor_parse_copy_row.
 *
 * Parameters:
 *   field_values - Array of field values
 *   field_lengths - Array of field lengths
 *   field_count - Number of fields
 */
static void
tecc_copy_free_parsed_fields(char ***field_values, int **field_lengths, int field_count)
{
    /* Begin - ReqID:TECC4.0 */
    int i;

    /* field_values is required; field_lengths is optional (may be NULL) */
    if (field_values == NULL || *field_values == NULL) {
        return;
    }

    /* Free individual field strings */
    for (i = 0; i < field_count; i++) {
        if ((*field_values)[i] != NULL) {
            pg_free((*field_values)[i]);
        }
    }

    /* Free the field values array */
    pg_free(*field_values);
    *field_values = NULL;

    /* Free field lengths array if provided */
    if (field_lengths != NULL && *field_lengths != NULL) {
        pg_free(*field_lengths);
        *field_lengths = NULL;
    }
    /* End - ReqID:TECC4.0 */
}

/* End - ReqID:TECC4.0 - COPY format parsing functions for import/export module */

/* Begin - ReqID:TECC4.0 - SQL statement type checking */

/*
 * is_ident_char - Check if a character is a SQL identifier character.
 *
 * SQL identifiers are [A-Za-z0-9_]. Used for word-boundary detection
 * in full-word keyword matching.
 */
static bool
is_ident_char(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') || c == '_';
}

/*
 * tecc_processor_needs_parsing - Check if SQL statement needs TECC parsing
 *
 * Uses full-word matching anywhere in the SQL text (not just prefix).
 * Whitelist approach: any statement containing one of the core TECC
 * keywords is sent to the parser. The parser/processor makes the final
 * decision on whether encryption processing is actually needed.
 *
 * Design: "宁可错抓，不可错放" (better over-catch than miss).
 * False positives are cheap (one parse attempt); false negatives
 * break the encrypted-data constraint.
 *
 * Word boundary = keyword preceded/followed by non-identifier char
 * (or start/end of string). No string literal or comment skipping
 * is attempted — edge cases are safer to let the parser handle.
 */
static bool
tecc_processor_needs_parsing(const char *query)
{
    /* ReqID:TECC4.0 */
    static const char *whitelist[] = {
        "SELECT", "INSERT", "UPDATE", "DELETE",
        "CREATE", "COPY", "EXECUTE", "DROP", "ALTER",
        "PREPARE", "DEALLOCATE"
    };
    static const int kw_len[] = { 6, 6, 6, 6, 6, 4, 7, 4, 5, 7, 10 };
    static const int num_kw = 11;

    const char *p;
    int i;
    int kw_len_i;
    char after;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "DEBUG: tecc_processor_needs_parsing: Checking query: %s",
                        query ? query : "(null)");

    if (query == NULL || *query == '\0')
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "DEBUG: tecc_processor_needs_parsing: Null/empty query");
        return false;
    }

    /*
     * Scan entire query for whitelist keywords with word boundaries.
     * Each position is checked: is it the start of a whitelist keyword,
     * with non-identifier chars before and after?
     */
    for (p = query; *p != '\0'; p++)
    {
        /* Word boundary check BEFORE: start-of-string or non-ident */
        if (p != query && is_ident_char(*(p - 1)))
            continue;

        for (i = 0; i < num_kw; i++)
        {
            kw_len_i = kw_len[i];

            if (strncasecmp(p, whitelist[i], kw_len_i) != 0)
                continue;

            /* Word boundary check AFTER: NUL or non-ident */
            after = *(p + kw_len_i);
            if (after != '\0' && is_ident_char(after))
                continue;

            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "DEBUG: tecc_processor_needs_parsing: Keyword '%s' at offset %ld, needs parsing",
                                whitelist[i], (long)(p - query));
            return true;
        }
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "DEBUG: tecc_processor_needs_parsing: No whitelist keyword found, bypassing: %s",
                        query);
    return false;
}
/* End - ReqID:TECC4.0 - SQL statement type checking */

/*
 * Debug function to print parse tree
 */
TECC_ATTR_UNUSED static void
tecc_debug_print_parse_tree(Node *node, int depth)
{
    char indent[128];
    int i;
    ListCell *cell;
    int index;
    List *list;
    InsertStmt *istmt;
    SelectStmt *sstmt;
    ResTarget *target;
    Value *value;

    if (depth >= 64) {
        tecc_processor_debug(TECC_LOG_DEBUG1, "Parse tree too deep, stopping recursion");
        return;
    }

    /* Create indentation */
    for (i = 0; i < depth && i < 63; i++) {
        indent[i] = ' ';
    }
    indent[i] = '\0';

    if (node == NULL) {
        tecc_processor_debug(TECC_LOG_DEBUG1, "%sNULL node", indent);
        return;
    }

    /* Print node type */
    tecc_processor_debug(TECC_LOG_DEBUG1, "%sNode type: %d (0x%x)", indent, node->type, node->type);

    /* Handle specific node types */
    switch (node->type) {
        case FE_T_List: {
            list = (List *)node;
            tecc_processor_debug(TECC_LOG_DEBUG1, "%sList: length=%d", indent, list->length);

            /* Recursively print list elements */
            index = 0;
            foreach(cell, list) {
                tecc_processor_debug(TECC_LOG_DEBUG1, "%s  Element %d:", indent, index++);
                tecc_debug_print_parse_tree((Node *)lfirst(cell), depth + 4);
            }
            break;
        }

        case FE_T_InsertStmt: {
            istmt = (InsertStmt *)node;
            tecc_processor_debug(TECC_LOG_DEBUG1, "%sINSERT statement:", indent);
            tecc_processor_debug(TECC_LOG_DEBUG1, "%s  Relation: %p", indent, istmt->relation);
            tecc_processor_debug(TECC_LOG_DEBUG1, "%s  Columns list: %p (length=%d)", indent,
                               istmt->cols, istmt->cols ? list_length(istmt->cols) : 0);
            tecc_processor_debug(TECC_LOG_DEBUG1, "%s  SelectStmt: %p", indent, istmt->selectStmt);

            /* Print columns if available */
            if (istmt->cols != NULL) {
                int col_index = 0;
                foreach(cell, istmt->cols) {
                    target = (ResTarget *)lfirst(cell);
                    tecc_processor_debug(TECC_LOG_DEBUG1, "%s    Column %d: name=%s, val=%p",
                                       indent, col_index++, target->name ? target->name : "NULL", target->val);
                }
            }

            /* Recursively print selectStmt if it's a SelectStmt */
            if (istmt->selectStmt != NULL && istmt->selectStmt->type == FE_T_SelectStmt) {
                tecc_processor_debug(TECC_LOG_DEBUG1, "%s  Select statement details:", indent);
                tecc_debug_print_parse_tree(istmt->selectStmt, depth + 4);
            }
            break;
        }

        case FE_T_SelectStmt: {
            sstmt = (SelectStmt *)node;
            tecc_processor_debug(TECC_LOG_DEBUG1, "%sSELECT statement:", indent);
            tecc_processor_debug(TECC_LOG_DEBUG1, "%s  TargetList: %p (length=%d)", indent,
                               sstmt->targetList, sstmt->targetList ? list_length(sstmt->targetList) : 0);
            tecc_processor_debug(TECC_LOG_DEBUG1, "%s  ValuesLists: %p (length=%d)", indent,
                               sstmt->valuesLists, sstmt->valuesLists ? list_length(sstmt->valuesLists) : 0);

            /* Print target list if available */
            if (sstmt->targetList != NULL) {
                int target_index = 0;
                foreach(cell, sstmt->targetList) {
                    target = (ResTarget *)lfirst(cell);
                    tecc_processor_debug(TECC_LOG_DEBUG1, "%s    Target %d: name=%s, val=%p, type=%d",
                                       indent, target_index++,
                                       target->name ? target->name : "NULL",
                                       target->val,
                                       target->val ? target->val->type : -1);
                }
            }
            break;
        }

        case FE_T_ResTarget: {
            target = (ResTarget *)node;
            tecc_processor_debug(TECC_LOG_DEBUG1, "%sResTarget: name=%s, val=%p",
                               indent, target->name ? target->name : "NULL", target->val);
            if (target->val != NULL) {
                tecc_debug_print_parse_tree(target->val, depth + 2);
            }
            break;
        }

        case FE_T_Integer:
        case FE_T_Float:
        case FE_T_String: {
            value = (Value *)node;
            if (node->type == FE_T_Integer) {
                tecc_processor_debug(TECC_LOG_DEBUG1, "%sInteger value: %d", indent, value->val.ival);
            } else if (node->type == FE_T_Float) {
                tecc_processor_debug(TECC_LOG_DEBUG1, "%sFloat value: %f", indent, value->val.dval);
            } else if (node->type == FE_T_String) {
                tecc_processor_debug(TECC_LOG_DEBUG1, "%sString value: %s", indent,
                                   value->val.str ? value->val.str : "NULL");
            }
            break;
        }

        default:
            tecc_processor_debug(TECC_LOG_DEBUG1, "%sUnknown node type %d", indent, node->type);
            break;
    }
}

/* End - ReqID:TECC4.0 */