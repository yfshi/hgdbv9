/* Begin - ReqID:TECC4.0 */
/*
 * File: stmt_processor.c
 *
 * Abstract:
 *   Statement processor implementation for TECC processor module.
 *   Implements SQL statement type identification, routing logic,
 *   and coordination with specialized processors.
 *
 * Authored by <wangxp@highgo.com>, 20260403.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/stmt_processor.c
 */

#include "postgres_fe.h"
#include "common/fe_memutils.h"
#include <unistd.h>
#include "libpq-fe.h"  /* For PQdb() function */
#include "stmt_processor.h"
#include "values_processor.h"
#include "where_clause_processor.h"
#include "create_stmt_processor.h"
#include "alter_stmt_processor.h"
#include "../tecc_scheduler/tecc_crypto_scheduler.h"  /* tecc_unified_encrypt_string */
#include "create_kek_processor.h"

/* Include expression processor interface for TeccColumnRefData */
#include "column_ref_data.h"
#include "../frontend_parser/tecc_nodes.h"
#include "expr_processor.h"

/* Include frontend parser definitions for CreateStmt and ColumnDef */
#include "../frontend_parser/fe_adapter.h"  /* For PostgreSQL types */
#include "../frontend_parser/tecc_nodes.h"  /* For node type definitions */

/* Forward declarations for external module interfaces */
/* Cache module interface */
#include "../tecc_cache/tecc_metadata_cache.h"
#include "catalog/pg_type_enc.h"  /* ReqID:TECC4.0 - TEXT_ENCOID etc. for type check */

/* TECC state management */
#include "../tecc_libpq_integration/fe-tecc-integration.h"  /* For tecc_get_state() */

/* Security and key management interfaces */
#include "../../../common/tecc_key/security.h"
#include "../../../common/tecc_key/high_level_api.h"
#include "../tecc_hsm_adapter/fe-hsm-adapter.h"
#include "tecc_expression_walker.h"

/* ReqID:TECC4.0 - Encryption type bitmask flags (from tecc_processor_internal.h) */
#ifndef TECC_ENC_TYPE_DET
#define TECC_ENC_TYPE_DET    0x01
#define TECC_ENC_TYPE_OPE    0x02
#define TECC_ENC_TYPE_SEARCH 0x04
#endif

/*
 * Forward declarations for internal functions
 */
static TeccStmtProcessorResult stmt_process_parse_tree(TeccStatementData *statement_data);
static TeccStmtProcessorResult stmt_collect_encrypted_columns(TeccStatementData *statement_data);
static TeccStmtProcessorResult stmt_extract_and_check_columns(const Node *stmt,
                                                             TeccStatementData *statement_data,
                                                             List **encrypted_columns);
static TeccStmtProcessorResult stmt_extract_select_columns(const SelectStmt *select_stmt,
                                                          List **columns,
                                                          const TeccStatementData *statement_data);
static TeccStmtProcessorResult stmt_extract_insert_columns(const InsertStmt *insert_stmt,
                                                          List **columns,
                                                          const TeccStatementData *statement_data);
static TeccStmtProcessorResult stmt_extract_update_columns(const UpdateStmt *update_stmt,
                                                          List **columns,
                                                          const TeccStatementData *statement_data);
static TeccStmtProcessorResult stmt_extract_delete_columns(const DeleteStmt *delete_stmt,
                                                          List **columns,
                                                          const TeccStatementData *statement_data);
static TeccStmtProcessorResult stmt_extract_create_columns(const CreateStmt *create_stmt,
                                                          List **columns);
static TeccStmtProcessorResult stmt_extract_copy_columns(const CopyStmt *copy_stmt,
                                                        List **columns,
                                                        const TeccStatementData *statement_data);
static void stmt_extract_columns_from_expression(const Node *expr, List **columns,
                                                const TeccStatementData *statement_data);

/* Helper functions for metadata loading and checking */
static bool stmt_load_table_metadata(const RangeVar *range_var, const TeccStatementData *statement_data, TeccColumnCacheItem ***encrypted_columns_ptr, int *column_count_ptr);
static bool stmt_check_column_encrypted(const TeccStatementData *statement_data, const char *table_name, const char *column_name);
static bool stmt_query_metadata_cache_simple(const TeccStatementData *statement_data, const char *table_name, const char *column_name);
static bool stmt_is_column_in_encrypted_list(const char *column_name, TeccColumnCacheItem **encrypted_columns, int column_count);

/*
 * Main statement processing entry point
 */
TeccStmtProcessorResult
tecc_stmt_processor_run_pre_query(TeccStatementData *statement_data)
{
    TeccStmtProcessorResult result;

    if (statement_data == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Starting statement pre-query processing");

    /* Initialize context */
    statement_data->encrypted_columns = NIL;
    statement_data->needs_rewrite = 0;

    /* Process parse tree if available */
    result = stmt_process_parse_tree(statement_data);
    if (result != TECC_STMT_PROCESSOR_SUCCESS)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                            "Parse tree processing failed");
        return result;
    }

    /* Collect encrypted columns */
    result = stmt_collect_encrypted_columns(statement_data);
    if (result != TECC_STMT_PROCESSOR_SUCCESS)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                            "Encrypted column collection failed");
        return result;
    }

    

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Process specific statement type
 */
TeccStmtProcessorResult
tecc_stmt_processor_run_pre_statement(const Node *stmt,
                                     TeccStatementData *statement_data)
{
    TeccStatementType stmt_type;
    TeccStmtProcessorResult result;

    if (stmt == NULL || statement_data == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    /* Identify statement type and store in statement_data for downstream use */
    stmt_type = tecc_stmt_processor_identify_statement(stmt);
    statement_data->stmt_type = stmt_type;
    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing statement type: %d", stmt_type);

    /* Check for COPY statement first (special handling) */
    if (nodeTag(stmt) == FE_T_CopyStmt) {
        result = tecc_stmt_processor_process_copy((CopyStmt *) stmt,
                                                 statement_data);
    }
    else {
        /* Route to appropriate processor */
        switch (stmt_type)
        {
            case TECC_STMT_TYPE_SELECT:
                result = tecc_stmt_processor_process_select((SelectStmt *) stmt,
                                                           statement_data);
                break;

            case TECC_STMT_TYPE_INSERT:
                result = tecc_stmt_processor_process_insert((InsertStmt *) stmt,
                                                           statement_data);
                break;

            case TECC_STMT_TYPE_UPDATE:
                result = tecc_stmt_processor_process_update((UpdateStmt *) stmt,
                                                           statement_data);
                break;

            case TECC_STMT_TYPE_DELETE:
                result = tecc_stmt_processor_process_delete((DeleteStmt *) stmt,
                                                           statement_data);
                break;

            case TECC_STMT_TYPE_CREATE_KEK:
                result = tecc_stmt_processor_process_create_kek((CreateKekStmt *) stmt,
                                                               statement_data);
                break;

            case TECC_STMT_TYPE_CREATE_DEK:
                result = tecc_stmt_processor_process_create_dek((CreateDekStmt *) stmt,
                                                               statement_data);
                break;

            case TECC_STMT_TYPE_CREATE_TABLE:
                result = tecc_stmt_processor_process_create_table((CreateStmt *) stmt,
                                                                 statement_data);
                break;

            case TECC_STMT_TYPE_DROP:
                result = tecc_stmt_processor_process_drop((DropStmt *) stmt,
                                                         statement_data);
                break;

            case TECC_STMT_TYPE_EXPLAIN:
                /* EXPLAIN - recursively process inner DML statement.
                 * The actual recursive call is in processor_process_statement
                 * (tecc_processor.c), so this just returns success. */
                result = TECC_STMT_PROCESSOR_SUCCESS;
                break;

            case TECC_STMT_TYPE_ALTER_TABLE:
                result = tecc_stmt_processor_process_alter_table(
                    (const AlterTableStmt *) stmt, statement_data);
                break;

            default:
                
                result = TECC_STMT_PROCESSOR_UNSUPPORTED_STMT;
                break;
        }
    }

    return result;
}

/* Forward declaration */
static TeccStmtProcessorResult
process_insert_select_typecast(SelectStmt *select_stmt,
                              TeccStatementData *statement_data);

/*
 * Process constants in FuncCall arguments for encryption.
 *
 * These functions can mix encrypted columns with plaintext constants.
 * Constants that may be returned to an outer expression (COALESCE/GREATEST/
 * LEAST) are encrypted with maximal algorithms (RND+OPE+SEARCH for char types)
 * so they work in any outer context. NULLIF constants are only compared,
 * not returned, so DET is sufficient.
 *
 * Multi-column DEK consistency is checked: all encrypted columns in the
 * same function call must share the same DEK. ReqID:TECC4.0
 */
void
stmt_process_funcall_constants(FuncCall *fc, TeccStatementData *statement_data)
{
    const char *func_name;
    ListCell *lc;
    bool has_encrypted_col = false;
    TeccColumnCacheItem *enc_cache = NULL;  /* first encrypted column's cache */
    const char *common_dek_name = NULL;
    bool may_return_const;                  /* true=constant may be returned to outer expr */
    bool is_char_type = false;              /* any encrypted column is character type */
    int union_encryption_type = 0;          /* union of algo bits across all columns */
    int i;

    if (fc == NULL || statement_data == NULL)
        return;
    if (fc->funcname == NIL || fc->args == NIL)
        return;

    /* Get function name */
    {
        Node *name_node = (Node *) linitial(fc->funcname);
        Value *v;
        if (name_node == NULL)
            return;
        v = (Value *) name_node;
        func_name = strVal(v);
    }

    /* Only process specific functions that mix encrypted cols with constants */
    if (strcasecmp(func_name, "coalesce") != 0 &&
        strcasecmp(func_name, "nullif") != 0 &&
        strcasecmp(func_name, "greatest") != 0 &&
        strcasecmp(func_name, "least") != 0)
        return;

    /* ReqID:TECC4.0 - NULLIF only compares, never returns the constant.
     * COALESCE/GREATEST/LEAST may return the constant to outer expressions. */
    may_return_const = (strcasecmp(func_name, "nullif") != 0);

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "stmt_process_funcall_constants: processing %s(), may_return_const=%d",
                       func_name, may_return_const);

    /*
     * Step 1: Collect ALL encrypted columns from arguments.
     * Check DEK consistency — all must share the same DEK.
     * Track whether any column is character type for SEARCH decision.
     */
    i = 0;
    foreach(lc, fc->args)
    {
        Node *arg = (Node *) lc->data.ptr_value;
        if (arg != NULL && nodeTag(arg) == FE_T_ColumnRef)
        {
            struct ColumnRef *col_ref = (struct ColumnRef *) arg;
            char *col_name = NULL;
            if (col_ref->fields != NIL)
            {
                int num_fields = list_length(col_ref->fields);
                Node *field = (Node *) list_nth(col_ref->fields, num_fields - 1);
                col_name = strVal((Value *) field);
            }
            if (col_name != NULL)
            {
                /* Check in statement_data->encrypted_columns */
                ListCell *elc;
                foreach(elc, statement_data->encrypted_columns)
                {
                    TeccColumnRefData *ec = (TeccColumnRefData *) elc->data.ptr_value;
                    if (ec != NULL && ec->column_name[0] != '\0' &&
                        strcmp(ec->column_name, col_name) == 0)
                    {
                        TeccColumnCacheItem *col_cache = NULL;

                        /* Look up cache for full metadata */
                        if (statement_data->conn != NULL)
                        {
                            TeccState *s = tecc_get_state(statement_data->conn);
                            const char *db = PQdb(statement_data->conn);
                            const char *schema = (s != NULL) ?
                                tecc_cache_get_guc_param(s, "current_schema") : NULL;
                            if (s != NULL && db != NULL && schema != NULL &&
                                ec->table_name[0] != '\0')
                            {
                                col_cache = tecc_cache_get_encrypted_column_by_name(
                                    s, db, schema, ec->table_name, col_name);
                            }
                        }

                        if (col_cache == NULL || col_cache->dek_name == NULL)
                            break;

                        if (!has_encrypted_col)
                        {
                            /* First encrypted column — use as reference */
                            has_encrypted_col = true;
                            enc_cache = col_cache;
                            common_dek_name = col_cache->dek_name;
                            union_encryption_type = col_cache->encryption_type;
                            if (col_cache->original_type_oid == VARCHAR_ENCOID ||
                                col_cache->original_type_oid == BPCHAR_ENCOID ||
                                col_cache->original_type_oid == TEXT_ENCOID)
                                is_char_type = true;
                        }
                        else
                        {
                            /* Additional encrypted column — check DEK consistency */
                            union_encryption_type |= col_cache->encryption_type;
                            if (!is_char_type &&
                                (col_cache->original_type_oid == VARCHAR_ENCOID ||
                                 col_cache->original_type_oid == BPCHAR_ENCOID ||
                                 col_cache->original_type_oid == TEXT_ENCOID))
                                is_char_type = true;
                            if (strcmp(common_dek_name, col_cache->dek_name) != 0)
                            {
                                tecc_processor_set_error(statement_data,
                                    TECC_PROCESSOR_ERROR,
                                    "function \"%s\" has encrypted columns with "
                                    "different DEKs: \"%s\" and \"%s\"",
                                    func_name, common_dek_name,
                                    col_cache->dek_name);
                                return;
                            }
                        }
                        break;
                    }
                }
            }
        }
        i++;
    }

    if (!has_encrypted_col)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "stmt_process_funcall_constants: no encrypted column in %s()",
                           func_name);
        return;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "stmt_process_funcall_constants: found encrypted col in %s(), "
                       "DEK=%s, char_type=%d, union_algo=0x%02x",
                       func_name, common_dek_name, is_char_type,
                       union_encryption_type);

    /* Step 2: Encrypt constant arguments with appropriate strategy */
    foreach(lc, fc->args)
    {
        Node *arg = (Node *) lc->data.ptr_value;
        Oid const_oid = InvalidOid;
        size_t const_loc = 0, const_dsize = 0;
        char *const_text = NULL;

        if (arg == NULL)
            continue;

        /* ReqID:TECC4.0 - Handle both plain Const and UMINUS+Const.
         * Negative integers like -1 are parsed as A_Expr(UMINUS,Const(1)). */
        if (nodeTag(arg) == FE_T_Const)
        {
            Const *cnode = (Const *) arg;
            if (cnode->constisnull || cnode->location < 0 || cnode->data_size <= 0)
                continue;
            const_loc = (size_t)cnode->location;
            const_dsize = (size_t)cnode->data_size;
            const_oid = cnode->consttype;
        }
        else if (nodeTag(arg) == FE_T_A_Expr)
        {
            A_Expr *aexpr = (A_Expr *) arg;
            Node *inner;
            if (aexpr->kind != AEXPR_OP)
                continue;
            if (aexpr->lexpr == NULL && aexpr->rexpr != NULL)
                inner = aexpr->rexpr;
            else if (aexpr->rexpr == NULL && aexpr->lexpr != NULL)
                inner = aexpr->lexpr;
            else
                continue;
            if (nodeTag(inner) != FE_T_Const)
                continue;
            {
                Const *ic = (Const *) inner;
                if (ic->constisnull || ic->location < 0 || ic->data_size <= 0)
                    continue;
                if (aexpr->location < 0)
                    continue;
                const_loc = (size_t)aexpr->location;
                const_dsize = (size_t)(ic->location + ic->data_size) -
                              (size_t)aexpr->location;
                const_oid = ic->consttype;
            }
        }
        else
            continue;

        {
            const char *query = statement_data->query;
            TeccUnifiedEncryptResult *encrypt_result;
            TeccRawValue *raw_value;
            const char *enc_type_name;
            char *table_name;
            char *column_name;
            char *dek_name;
            const char *schema_name;

            const_text = (char *) pg_malloc(const_dsize + 1);
            if (const_text == NULL)
                continue;
            memcpy(const_text, query + const_loc, const_dsize);
            const_text[const_dsize] = '\0';

            /* Determine table/column/dek for encryption */
            if (enc_cache != NULL && enc_cache->dek_name != NULL)
            {
                table_name = enc_cache->table_name;
                column_name = enc_cache->column_name;
                dek_name = enc_cache->dek_name;
                schema_name = enc_cache->schema_name;
            }
            else
            {
                tecc_processor_debug(TECC_LOG_WARNING,
                                   "stmt_process_funcall_constants: "
                                   "enc_cache or DEK name not available, "
                                   "skipping constant encryption for %s()",
                                   func_name);
                pg_free(const_text);
                continue;
            }

            /*
             * ReqID:TECC4.0 - Encrypt with strategy based on function semantics.
             * COALESCE/GREATEST/LEAST: max encryption — constant may be returned
             * to an outer expression and face any operator (=, >, LIKE, ...).
             * Use RND + DET + OPE + SEARCH(for char types) to ensure the
             * encrypted constant can match regardless of outer context.
             * NULLIF: constant is only compared for equality, never returned.
             * Uses column's actual algorithm support (DET preferred, OPE fallback).
             */
            if (may_return_const)
            {
                encrypt_result = tecc_unified_encrypt_string_with_options(
                    statement_data->conn,
                    const_text,
                    table_name,
                    column_name,
                    NULL, /* default KEK */
                    dek_name,
                    enc_cache->original_type_oid,
                    enc_cache->original_type_mod,
                    true,           /* supports_det — for equality */
                    true,           /* supports_ope — for ordering */
                    is_char_type,    /* supports_search — for pattern matching */
                    NULL
                );
            }
            else
            {
                /*
                 * NULLIF: equality comparison only.
                 * Must match the column's actual algorithm support:
                 * prefer DET if available, fall back to OPE (covers equality),
                 * error if neither is available.
                 */
                bool has_det = (enc_cache->encryption_type & 0x01) != 0;
                bool has_ope = (enc_cache->encryption_type & 0x02) != 0;

                if (!has_det && !has_ope)
                {
                    tecc_processor_set_error(statement_data,
                        TECC_PROCESSOR_ERROR,
                        "function \"%s\" requires DET or OPE encryption "
                        "on column \"%s\", but neither is available",
                        func_name, column_name);
                    pg_free(const_text);
                    continue;
                }

                encrypt_result = tecc_unified_encrypt_string_with_options(
                    statement_data->conn,
                    const_text,
                    table_name,
                    column_name,
                    NULL, /* default KEK */
                    dek_name,
                    enc_cache->original_type_oid,
                    enc_cache->original_type_mod,
                    has_det,        /* supports_det — prefer if available */
                    !has_det,       /* supports_ope — fallback when DET missing */
                    false,           /* supports_search — not needed */
                    NULL
                );
            }

            pg_free(const_text);
            const_text = NULL;

            if (encrypt_result == NULL ||
                encrypt_result->error_code != TECC_CRYPTO_SUCCESS)
            {
                tecc_processor_debug(TECC_LOG_WARNING,
                                   "stmt_process_funcall_constants: encryption failed for const in %s()",
                                   func_name);
                continue;
            }

            /* Create raw value for SQL rewriting */
            raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_STRING);
            if (raw_value != NULL)
            {
                raw_value->location = const_loc;
                raw_value->data_size = const_dsize;
                raw_value->state = TECC_RAW_VALUE_STATE_ENCRYPTED;

                /* Set column type for ::typename cast */
                enc_type_name = get_enc_type_name_from_base_oid(const_oid);
                tecc_raw_value_set_column_info(raw_value, column_name,
                                               enc_type_name, InvalidOid);
                if (table_name != NULL)
                {
                    tecc_raw_value_set_table_info(raw_value, schema_name, table_name);
                }

                /* Store encrypted hex value */
                raw_value->processed.encrypted_value =
                    tecc_processor_strdup(encrypt_result->hex_string);

                /* Add to rewrite list */
                if (statement_data->raw_values_list == NULL)
                    statement_data->raw_values_list =
                        tecc_raw_values_list_create(16);
                if (statement_data->raw_values_list != NULL)
                {
                    if (tecc_raw_values_list_add(
                            statement_data->raw_values_list, raw_value) < 0)
                        tecc_raw_value_destroy(raw_value);
                }
                statement_data->needs_rewrite = 1;

                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "stmt_process_funcall_constants: encrypted const in %s() at loc=%zu",
                                   func_name, const_loc);
            }
        }
    }
}

/*
 * Process SELECT statement
 */
TeccStmtProcessorResult
tecc_stmt_processor_process_select(const SelectStmt *select_stmt,
                                  TeccStatementData *statement_data)
{
    ListCell *lc;
    int target_count = 0;
    int from_count = 0;
    TeccWhereClauseProcessorResult where_result;
    ResTarget *res_target = NULL;

    if (select_stmt == NULL || statement_data == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    /* ReqID:TECC4.0 - Set stmt_type for downstream safety checks */
    statement_data->stmt_type = TECC_STMT_TYPE_SELECT;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing SELECT statement");

    /* ReqID:TECC4.0 - Traverse CTE definitions first */
    if (select_stmt->cteList != NIL)
    {
        ListCell *cte_lc;
        foreach(cte_lc, select_stmt->cteList)
        {
            CommonTableExpr *cte = (CommonTableExpr *) lfirst(cte_lc);
            if (cte->ctequery != NULL)
            {
                TeccStatementData cte_data;
                memset(&cte_data, 0, sizeof(TeccStatementData));
                cte_data.conn = statement_data->conn;  /* propagate connection for CTE encryption */
                if (nodeTag(cte->ctequery) == FE_T_SelectStmt)
                {
                    tecc_stmt_processor_process_select((SelectStmt *)cte->ctequery, &cte_data);
                }
                /* ReqID:TECC4.0 - DML CTE: INSERT/UPDATE/DELETE in WITH clause */
                else if (nodeTag(cte->ctequery) == FE_T_InsertStmt)
                {
                    tecc_stmt_processor_process_insert((InsertStmt *)cte->ctequery, &cte_data);
                }
                else if (nodeTag(cte->ctequery) == FE_T_UpdateStmt)
                {
                    tecc_stmt_processor_process_update((UpdateStmt *)cte->ctequery, &cte_data);
                }
                else if (nodeTag(cte->ctequery) == FE_T_DeleteStmt)
                {
                    tecc_stmt_processor_process_delete((DeleteStmt *)cte->ctequery, &cte_data);
                }
                if (cte_data.needs_rewrite)
                {
                    statement_data->needs_rewrite = 1;
                }
            }
        }
    }

    /* Process target list */
    if (select_stmt->targetList != NIL) {
        for (lc = select_stmt->targetList->head; lc != NULL; lc = lc->next) {
            Node *target_node = (Node *) lc->data.ptr_value;
            if (target_node == NULL) {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "target_node is NULL, skipping");
                continue;
            }

            /* FIX: Check if this is a Const node (type FE_T_Const = 211) and skip it */
            if (target_node->type == FE_T_Const) {
                target_count++;
                continue;
            }

            /* Debug: Log the node type */
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "target_node type: %d", target_node->type);

            /* Check if this is a ColumnRef node (type 7) */
            if (target_node->type == FE_T_ColumnRef) {
                /* Handle ColumnRef node directly in target list */
                ColumnRef *col_ref = (ColumnRef *) target_node;
                TeccColumnRefData *tecc_col = NULL;

                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing ColumnRef node directly in target list");

                /* Expand column reference with state context */
                if (statement_data->conn != NULL)
                {
                    /* Get TECC state from connection */
                    TeccState *state = tecc_get_state(statement_data->conn);
                    if (state != NULL)
                    {
                        if (!tecc_expand_column_ref_with_state(col_ref, &tecc_col, state))
                        {
                            tecc_col = NULL;
                        }
                    }
                }

                target_count++;
                continue;
            }

            /* Only cast to ResTarget if it's not a Const or ColumnRef node */
            res_target = (ResTarget *) target_node;
            if (res_target->val == NULL)
                continue;

            target_count++;

            /* Process each target expression */
            if (nodeTag(res_target->val) == FE_T_ColumnRef)
            {
                ColumnRef *col_ref = (ColumnRef *) res_target->val;
                TeccColumnRefData *tecc_col = NULL;

                /* Expand column reference with state context */
                if (statement_data->conn != NULL)
                {
                    /* Get TECC state from connection */
                    TeccState *state = tecc_get_state(statement_data->conn);
                    if (state != NULL)
                    {
                        if (!tecc_expand_column_ref_with_state(col_ref, &tecc_col, state))
                        {
                            tecc_col = NULL;
                        }
                    }
                    else
                    {
                        tecc_processor_debug(TECC_LOG_ERROR,
                                           "TECC state not available for column reference expansion");
                        tecc_col = NULL;
                    }
                }
                else
                {
                    tecc_processor_debug(TECC_LOG_ERROR,
                                       "No connection available for column reference expansion");
                    tecc_col = NULL;
                }
                {
                    if (tecc_col != NULL)
                    {
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                           "Processed target column: %s (encrypted=%s)",
                                           tecc_col->alias_fqdn,
                                           tecc_col->is_encrypted ? "true" : "false");

                        /* ReqID:TECC4.0 - Encrypted column in SELECT target list:
                         * needs post-query result decryption, but no SQL rewrite.
                         * WHERE clause and expression encryption sets needs_rewrite separately. */
                        if (tecc_col->is_encrypted)
                        {
                            
                        }

                        tecc_column_ref_data_destroy(tecc_col);
                    }
                }
            }
            else if (nodeTag(res_target->val) == FE_T_FuncCall)
            {
                FuncCall *func_call = (FuncCall *) res_target->val;
                TeccFuncNameData *func_data = NULL;

                /* Expand function name */
                if (tecc_expand_function_name(func_call, &func_data))
                {
                    if (func_data != NULL)
                    {
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                           "Processed target function: %s.%s",
                                           func_data->schema_name,
                                           func_data->function_name);

                        /* Process function arguments for encrypted columns */
                        if (func_call->args != NULL)
                        {
                            ListCell *arg_lc;
                            for (arg_lc = func_call->args->head; arg_lc != NULL; arg_lc = arg_lc->next)
                            {
                                Node *arg = (Node *) arg_lc->data.ptr_value;
                                if (arg != NULL && nodeTag(arg) == FE_T_ColumnRef)
                                {
                                    ColumnRef *col_ref = (ColumnRef *) arg;
                                    TeccColumnRefData *tecc_col = NULL;

                                    if (tecc_expand_column_ref_with_stmt_data(col_ref, &tecc_col, statement_data))
                                    {
                                        if (tecc_col != NULL && tecc_col->is_encrypted)
                                        {
                                            if (statement_data->stmt_type != TECC_STMT_TYPE_SELECT)
                                            {
                                                statement_data->needs_rewrite = 1;
                                            }
                                            
                                        }
                                        tecc_column_ref_data_destroy(tecc_col);
                                    }
                                }
                            }
                        }

                        pg_free(func_data);
                    }
                }
            }
            else
            {
                /* ReqID:TECC4.0 - Other expression types in target list.
                 * Constant encryption is deferred to the second pass
                 * (after FROM clause populates encrypted_columns). */
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Deferring target expression type: %d to second pass",
                                   nodeTag(res_target->val));
            }
        }
    }

    /* Process FROM clause */
    if (select_stmt->fromClause != NULL) {
        ListCell *lc;
        for (lc = select_stmt->fromClause->head; lc != NULL; lc = lc->next) {
            Node *from_item = (Node *) lc->data.ptr_value;

            from_count++;

            /* Process each from item */
            if (nodeTag(from_item) == FE_T_RangeVar)
            {
                RangeVar *range_var = (RangeVar *) from_item;
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing FROM table: %s.%s",
                                   range_var->schemaname ? range_var->schemaname : "(null)",
                                   range_var->relname ? range_var->relname : "(null)");

                /* Load table metadata and identify encrypted columns */
                {
                    TeccColumnCacheItem **table_cols = NULL;
                    int table_col_count = 0;
                    stmt_load_table_metadata(range_var, statement_data, &table_cols, &table_col_count);
                    if (table_cols != NULL && table_col_count > 0)
                    {
                        int col_i;
                        for (col_i = 0; col_i < table_col_count; col_i++)
                        {
                            if (table_cols[col_i] != NULL)
                            {
                                TeccColumnRefData *col_data = tecc_column_ref_data_create();
                                if (col_data != NULL)
                                {
                                    /* ReqID:TECC4.0 - Use init function to set all fields including table_alias */
                                    tecc_column_ref_data_init(col_data,
                                                             NULL,
                                                             range_var->schemaname,
                                                             range_var->relname,
                                                             table_cols[col_i]->column_name);
                                    /* Set table alias if present on RangeVar */
                                    if (range_var->alias != NULL
                                        && range_var->alias->aliasname != NULL)
                                        tecc_column_ref_data_set_alias(col_data,
                                            range_var->alias->aliasname);
                                    col_data->is_encrypted = true;
                                    statement_data->encrypted_columns = lappend(
                                        statement_data->encrypted_columns, col_data);
                                }
                            }
                        }
                        pg_free(table_cols);
                    }
                }
            }
            else if (nodeTag(from_item) == FE_T_SelectStmt)
            {
                /* Subselect in FROM clause */
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing FROM subselect");

                /* Process subselect recursively */
                if (nodeTag(from_item) == FE_T_SelectStmt)
                {
                    SelectStmt *subselect = (SelectStmt *) from_item;
                    TeccStmtProcessorResult subselect_result;

                    /* 创建临时的statement_data用于子查询处理 */
                    TeccStatementData subselect_data;
                    memset(&subselect_data, 0, sizeof(TeccStatementData));
                    subselect_data.conn = statement_data->conn;  /* propagate connection for subquery encryption */

                    subselect_result = tecc_stmt_processor_process_select(subselect, &subselect_data);
                    if (subselect_result != TECC_STMT_PROCESSOR_SUCCESS)
                    {
                        tecc_processor_debug(TECC_LOG_ERROR,
                                           "Failed to process FROM subselect: %d", subselect_result);
                        return subselect_result;
                    }

                    /* 如果子查询需要改写，则主查询也需要改写 */
                    if (subselect_data.needs_rewrite)
                    {
                        statement_data->needs_rewrite = 1;
                        
                    }

                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                       "Successfully processed FROM subselect");
                }
            }
            else if (nodeTag(from_item) == FE_T_RangeSubselect)
            {
                /* ReqID:TECC4.0 - RangeSubselect in FROM clause
                 * Unwrap subquery and process recursively so encrypted
                 * columns/constants in subquery are visible to the main query. */
                RangeSubselect *rss = (RangeSubselect *) from_item;
                if (rss->subquery != NULL && nodeTag(rss->subquery) == FE_T_SelectStmt)
                {
                    SelectStmt *subselect = (SelectStmt *) rss->subquery;
                    TeccStmtProcessorResult subselect_result;
                    TeccStatementData subselect_data;
                    memset(&subselect_data, 0, sizeof(TeccStatementData));
                    subselect_data.conn = statement_data->conn;  /* propagate connection for subquery encryption */
                    subselect_result = tecc_stmt_processor_process_select(subselect, &subselect_data);
                    if (subselect_result != TECC_STMT_PROCESSOR_SUCCESS)
                    {
                        tecc_processor_debug(TECC_LOG_ERROR,
                                           "Failed to process FROM RangeSubselect: %d", subselect_result);
                    }
                    if (subselect_data.needs_rewrite)
                    {
                        statement_data->needs_rewrite = 1;
                    }
                }
            }
            else if (nodeTag(from_item) == FE_T_JoinExpr)
            {
                /* ReqID:TECC4.0 - Recursively process JOIN components
                 * so encrypted columns from joined tables are discovered. */
                JoinExpr *join = (JoinExpr *) from_item;
                Node *join_items[2] = { join->larg, join->rarg };
                int ji;
                for (ji = 0; ji < 2; ji++)
                {
                    Node *ji_node = join_items[ji];
                    if (ji_node == NULL) continue;
                    if (nodeTag(ji_node) == FE_T_RangeVar)
                    {
                        RangeVar *rv = (RangeVar *) ji_node;
                        TeccColumnCacheItem **table_cols = NULL;
                        int table_col_count = 0;
                        stmt_load_table_metadata(rv, statement_data, &table_cols, &table_col_count);
                        if (table_cols != NULL && table_col_count > 0)
                        {
                            int ci;
                            for (ci = 0; ci < table_col_count; ci++)
                            {
                                if (table_cols[ci] != NULL)
                                {
                                    TeccColumnRefData *cd = tecc_column_ref_data_create();
                                    if (cd != NULL)
                                    {
                                        /* ReqID:TECC4.0 - Use init function to set fields including table_alias */
                                        tecc_column_ref_data_init(cd, NULL,
                                                                 rv->schemaname,
                                                                 rv->relname,
                                                                 table_cols[ci]->column_name);
                                        if (rv->alias != NULL && rv->alias->aliasname != NULL)
                                            tecc_column_ref_data_set_alias(cd, rv->alias->aliasname);
                                        cd->is_encrypted = true;
                                        statement_data->encrypted_columns = lappend(
                                            statement_data->encrypted_columns, cd);
                                    }
                                }
                            }
                            pg_free(table_cols);
}
                    }
                    else if (nodeTag(ji_node) == FE_T_JoinExpr)
                    {
                        /* Recurse: nested JOIN. Wrap in a single-item list for processing.
                         * Fall through to recursive type dispatch.
                         * Simply process larg/rarg of the nested join. */
                        JoinExpr *nested = (JoinExpr *) ji_node;
                        Node *nested_items[2] = { nested->larg, nested->rarg };
                        int ni;
                        for (ni = 0; ni < 2; ni++)
                        {
                            Node *n_node = nested_items[ni];
                            if (n_node != NULL && nodeTag(n_node) == FE_T_RangeVar)
                            {
                                RangeVar *nrv = (RangeVar *) n_node;
                                TeccColumnCacheItem **tcols = NULL;
                                int tcnt = 0;
                                stmt_load_table_metadata(nrv, statement_data, &tcols, &tcnt);
                                if (tcols != NULL && tcnt > 0)
                                {
                                    int cj;
                                    for (cj = 0; cj < tcnt; cj++)
                                    {
                                        if (tcols[cj] != NULL)
                                        {
                                            TeccColumnRefData *cd2 = tecc_column_ref_data_create();
                                            if (cd2 != NULL)
                                            {
                                                strncpy(cd2->column_name, tcols[cj]->column_name, NAMEDATALEN - 1);
                                                cd2->column_name[NAMEDATALEN - 1] = '\0';
                                                if (nrv->relname)
                                                {
                                                    strncpy(cd2->table_name, nrv->relname, NAMEDATALEN - 1);
                                                    cd2->table_name[NAMEDATALEN - 1] = '\0';
                                                }
                                                cd2->is_encrypted = true;
                                                statement_data->encrypted_columns = lappend(
                                                    statement_data->encrypted_columns, cd2);
                                            }
                                        }
                                    }
                                    pg_free(tcols);
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing FROM item type: %d", nodeTag(from_item));
            }
        }
    }

    /* Process WHERE clause if present */
    if (select_stmt->whereClause != NULL)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Processing WHERE clause");

        /* Delegate to WHERE clause processor */
        where_result = tecc_where_clause_processor_process(
            select_stmt->whereClause, statement_data);

        if (where_result != TECC_WHERE_PROCESSOR_SUCCESS)
        {
            switch (where_result)
            {
                case TECC_WHERE_PROCESSOR_MISSING_DET:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "操作符需要DET（确定性加密）算法，但该列未配置DET。");
                    break;
                case TECC_WHERE_PROCESSOR_MISSING_OPE:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "操作符需要OPE（保序加密）算法，但该列未配置OPE。");
                    break;
                case TECC_WHERE_PROCESSOR_MISSING_SEARCH:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "操作符需要SEARCH（可搜索加密）算法，但该列未配置SEARCH。");
                    break;
                case TECC_WHERE_PROCESSOR_MISSING_DET_OPE:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "操作符需要DET或OPE算法，但该列既未配置DET也未配置OPE。");
                    break;
                case TECC_WHERE_PROCESSOR_INCOMPATIBLE_TYPES:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "该列上的操作符与加密算法不兼容。");
                    break;
                default:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "WHERE clause processing failed");
                    break;
            }
            /* ReqID:TECC4.0 - Clear rewrite flags to prevent SAFETY CHECK
             * false positive. When WHERE processing fails (e.g. missing OPE),
             * needs_rewrite may have been set by earlier FuncCall constant
             * processing (COALESCE/GREATEST/LEAST). Reset so user gets a
             * clean error message instead of "no rewritten query". */
            statement_data->needs_rewrite = 0;
            statement_data->where_needs_rewrite = 0;
            return TECC_STMT_PROCESSOR_ERROR;
        }

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "WHERE clause processing completed successfully");
    }

    /* ReqID:TECC4.0 - Process FILTER/OVER clauses in target list FuncCall nodes */
    if (select_stmt->targetList != NIL)
    {
        ListCell *tlc2;
        for (tlc2 = select_stmt->targetList->head; tlc2 != NULL; tlc2 = tlc2->next)
        {
            Node *target_node = (Node *)tlc2->data.ptr_value;
            ResTarget *rt;

            if (target_node == NULL)
                continue;
            if (target_node->type == FE_T_Const || target_node->type == FE_T_ColumnRef)
                continue;

            rt = (ResTarget *)target_node;
            if (rt->val == NULL)
                continue;

            if (nodeTag(rt->val) == FE_T_FuncCall)
            {
                FuncCall *fc = (FuncCall *)rt->val;

                /* Process FILTER (WHERE) clause - encrypt constants */
                if (fc->agg_filter != NULL)
                {
                    tecc_processor_debug(TECC_LOG_DEBUG1, "Processing FILTER clause in target list");
                    where_result = tecc_where_clause_processor_process(fc->agg_filter, statement_data);
                    if (where_result != TECC_WHERE_PROCESSOR_SUCCESS)
                    {
                        tecc_processor_debug(TECC_LOG_ERROR, "FILTER clause processing failed");
                        return TECC_STMT_PROCESSOR_ERROR;
                    }
                }
                /* Process OVER clause (WindowDef) */
                if (fc->over != NULL)
                {
                    WindowDef *wd = (WindowDef *)fc->over;
                    ListCell *plc;

                    if (wd->partitionClause != NULL)
                    {
                        for (plc = wd->partitionClause->head; plc != NULL; plc = plc->next)
                        {
                            where_result = tecc_where_clause_processor_process((Node *)plc->data.ptr_value, statement_data);
                            if (where_result != TECC_WHERE_PROCESSOR_SUCCESS)
                                return TECC_STMT_PROCESSOR_ERROR;
                        }
                    }
                    if (wd->orderClause != NULL)
                    {
                        for (plc = wd->orderClause->head; plc != NULL; plc = plc->next)
                        {
                            SortBy *sortby = (SortBy *)plc->data.ptr_value;
                            if (sortby != NULL && sortby->node != NULL)
                            {
                                where_result = tecc_where_clause_processor_process(sortby->node, statement_data);
                                if (where_result != TECC_WHERE_PROCESSOR_SUCCESS)
                                    return TECC_STMT_PROCESSOR_ERROR;
                            }
                        }
                    }
                }
                /* ReqID:TECC4.0 - Encrypt constants in FuncCall arguments
                 * for functions like COALESCE, NULLIF, GREATEST, LEAST */
                stmt_process_funcall_constants(fc, statement_data);
            }

            /* ReqID:TECC4.0 - Process SubLink in target list */
            if (nodeTag(rt->val) == FE_T_SubLink)
            {
                tecc_processor_debug(TECC_LOG_DEBUG1, "Processing SubLink in target list");
                where_result = tecc_where_clause_processor_process(rt->val, statement_data);
                if (where_result != TECC_WHERE_PROCESSOR_SUCCESS)
                {
                    tecc_processor_debug(TECC_LOG_ERROR, "SubLink target processing failed");
                    return TECC_STMT_PROCESSOR_ERROR;
                }
            }
            /* ReqID:TECC4.0 - Process other expression types for constant
             * encryption (CaseExpr, A_Expr, TypeCast, RowExpr, NullTest).
             * This runs in the second pass after FROM clause has populated
             * encrypted_columns, so where_is_encrypted_column() works. */
            else if (nodeTag(rt->val) != FE_T_FuncCall &&
                     nodeTag(rt->val) != FE_T_ColumnRef)
            {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing target expression type %d for constant encryption",
                                   nodeTag(rt->val));
                /* ReqID:TECC4.0 - Check return value. Operators like || are
                 * not compatible with encrypted columns — must reject here
                 * instead of silently passing garbage to the server. */
                where_result = tecc_where_clause_processor_process_expr(
                    rt->val, statement_data);
                if (where_result != TECC_WHERE_PROCESSOR_SUCCESS)
                {
                    tecc_processor_set_error(statement_data,
                        TECC_PROCESSOR_ERROR,
                        "Operator in SELECT expression is not supported "
                        "with encrypted columns");
                    return TECC_STMT_PROCESSOR_ERROR;
                }
            }
        }
    }

    /* Process HAVING clause if present */
    if (select_stmt->havingClause != NULL)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1, "Processing HAVING clause");
        where_result = tecc_where_clause_processor_process(select_stmt->havingClause, statement_data);
        if (where_result != TECC_WHERE_PROCESSOR_SUCCESS)
        {
            tecc_processor_debug(TECC_LOG_ERROR, "HAVING clause processing failed");
            return TECC_STMT_PROCESSOR_ERROR;
        }
    }

    

    /* Process TypeCast expressions with encrypted options in SELECT target list */
    if (select_stmt != NULL)
    {
        TeccStmtProcessorResult typecast_result;
        typecast_result = process_insert_select_typecast(
            (SelectStmt *)select_stmt, statement_data);
        if (typecast_result != TECC_STMT_PROCESSOR_SUCCESS)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "SELECT TypeCast processing failed: %d",
                               typecast_result);
            return TECC_STMT_PROCESSOR_ERROR;
        }
    }

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Helper: find end position of a TypeCast expression in the original SQL text.
 * The TypeCast expression has the form: <arg>::<type_name> [ENCRYPTED WITH (...)]
 * Returns the byte offset just past the end of the expression.
 */
size_t
find_typecast_expr_end(const char *query, const TypeCast *type_cast)
{
    size_t pos;
    size_t start;
    int depth;

    if (query == NULL || type_cast == NULL)
        return 0;

    /* Start from the TypeName location (the type name token) */
    if (type_cast->typeName == NULL)
        return type_cast->location;

    start = (size_t)type_cast->typeName->location;

    /* If encryptedOpts present, scan for ENCRYPTED WITH (...) */
    if (type_cast->typeName->encryptedOpts == NIL)
    {
        /* Plain type cast without encrypted options - use data_size heuristic */
        /* The type name starts at start. Since we don't have a stored length,
         * just return start + some reasonable guess. For plain type casts
         * (no ENCRYPTED WITH), the expression extends past the type name.
         * Find the end by looking for non-identifier characters. */
        pos = start;
        while (query[pos] != '\0' &&
               (isalnum((unsigned char)query[pos]) || query[pos] == '_'))
            pos++;
        return pos;
    }

    /* With ENCRYPTED WITH, scan from the type name to find encrypted with (...) */
    pos = start;
    while (query[pos] != '\0')
    {
        /* Look for 'encrypted' (case-insensitive) */
        if ((query[pos] == 'e' || query[pos] == 'E') &&
            strncasecmp(query + pos, "encrypted", 9) == 0)
        {
            size_t p = pos + 9;

            /* Skip whitespace and find '(' */
            while (query[p] != '\0' && query[p] != '(' && query[p] != ')')
            {
                if (query[p] == ';' || query[p] == ',')
                    break;
                p++;
            }

            if (query[p] == '(')
            {
                /* Track parenthesis depth to find matching ')' */
                depth = 1;
                p++;
                while (depth > 0 && query[p] != '\0')
                {
                    if (query[p] == '(') depth++;
                    else if (query[p] == ')') depth--;
                    p++;
                }
                return p;  /* Past the closing ')' */
            }
            break;
        }

        /* Stop at delimiters that can't be part of a type name */
        if (query[pos] == ';' || query[pos] == ',')
            break;
        pos++;
    }

    return start; /* fallback */
}

/*
 * Recursively walk an expression tree and collect all TypeCast nodes that have
 * encrypted options. Only collects the outermost TypeCast in each chain — inner
 * TypeCast nodes with encryptedOpts are absorbed into the outer encryption.
 */
static void
collect_typecast_nodes(Node *expr, List **nodes)
{
    if (expr == NULL)
        return;

    switch (expr->type)
    {
        case FE_T_TypeCast:
        {
            TypeCast *tc = (TypeCast *)expr;
            if (tc->typeName != NULL && tc->typeName->encryptedOpts != NIL)
            {
                /* Found a TypeCast with encrypted options — collect it.
                 * Don't recurse into arg since nested TypeCast with
                 * encryptedOpts is absorbed by the outer encryption. */
                *nodes = lappend(*nodes, tc);
            }
            else if (tc->arg != NULL)
            {
                /* No encrypted opts here, keep walking */
                collect_typecast_nodes(tc->arg, nodes);
            }
            break;
        }
        case FE_T_FuncCall:
        {
            FuncCall *fc = (FuncCall *)expr;
            if (fc->args != NIL)
            {
                ListCell *lc;
                foreach (lc, fc->args)
                    collect_typecast_nodes((Node *)lc->data.ptr_value, nodes);
            }
            break;
        }
        /* Begin - ReqID:TECC4.0 - Bug 7857: COLLATE wrapping TypeCast with encryptedOpts */
        case FE_T_CollateClause:
        {
            CollateClause *cc = (CollateClause *)expr;
            if (cc->arg != NULL)
                collect_typecast_nodes(cc->arg, nodes);
            break;
        }
        /* End - ReqID:TECC4.0 */
        /* ReqID:TECC4.0 - Bug 8001: walk A_Expr (WHERE comparison) to find TypeCast */
        case FE_T_A_Expr:
        {
            A_Expr *ae = (A_Expr *)expr;
            if (ae->lexpr != NULL)
                collect_typecast_nodes(ae->lexpr, nodes);
            if (ae->rexpr != NULL)
                collect_typecast_nodes(ae->rexpr, nodes);
            break;
        }
        /* ReqID:TECC4.0 - Bug 8001: walk BoolExpr (AND/OR/NOT) in WHERE */
        case FE_T_BoolExpr:
        {
            struct BoolExpr *be = (struct BoolExpr *)expr;
            ListCell *lc;
            foreach (lc, be->args)
                collect_typecast_nodes((Node *)lc->data.ptr_value, nodes);
            break;
        }
        /* ReqID:TECC4.0 - Bug 8001: walk NullTest/BooleanTest for TypeCast inside */
        case FE_T_NullTest:
        {
            NullTest *nt = (NullTest *)expr;
            if (nt->arg != NULL)
                collect_typecast_nodes(nt->arg, nodes);
            break;
        }
        case FE_T_BooleanTest:
        {
            struct BooleanTest *bt = (struct BooleanTest *)expr;
            if (bt->arg != NULL)
                collect_typecast_nodes(bt->arg, nodes);
            break;
        }
        default:
            break;
    }
}

/*
 * Get the type name string from a TypeCast node as it appears in the original SQL.
 * Returns a palloc'd string (caller must pg_free), or NULL on failure.
 * ReqID:TECC4.0
 */
static char *
get_typecast_typename(const TypeCast *type_cast, const char *orig_query)
{
    const char *start;
    const char *p;
    size_t len;
    char *result;

    if (type_cast == NULL || type_cast->typeName == NULL || orig_query == NULL)
        return NULL;

    start = orig_query + type_cast->typeName->location;
    p = start;

    /* Scan forward through identifier chars (alphanum, _, ., " for qualified names) */
    while (*p != '\0' && (isalnum((unsigned char)*p) || *p == '_' || *p == '.' || *p == '"'))
        p++;

    len = p - start;
    if (len == 0)
        return NULL;

    result = (char *)pg_malloc(len + 1);
    memcpy(result, start, len);
    result[len] = '\0';
    return result;
}

/*
 * Determine encryption algorithm flags for a TypeCast expression based on the
 * encrypted type name and optional support_algo in the encrypted options.
 * ReqID:TECC4.0
 */
static void
get_typecast_algo_flags(const TypeCast *type_cast,
                        const char *typename_str,
                        bool *supports_det,
                        bool *supports_ope,
                        bool *supports_search)
{
    ListCell *lc;

    /* Defaults based on encrypted type */
    *supports_det = false;
    *supports_ope = false;
    *supports_search = false;

    if (typename_str != NULL)
    {
        if (strcasecmp(typename_str, "int2_enc") == 0 ||
            strcasecmp(typename_str, "int4_enc") == 0 ||
            strcasecmp(typename_str, "int8_enc") == 0)
            *supports_det = true;
        else if (strcasecmp(typename_str, "float4_enc") == 0 ||
                 strcasecmp(typename_str, "float8_enc") == 0 ||
                 strcasecmp(typename_str, "numeric_enc") == 0)
            *supports_ope = true;
        else if (strcasecmp(typename_str, "text_enc") == 0 ||
                 strcasecmp(typename_str, "varchar_enc") == 0 ||
                 strcasecmp(typename_str, "bpchar_enc") == 0)
            *supports_det = true;
    }

    /* Check for support_algo override in encrypted options */
    if (type_cast == NULL || type_cast->typeName == NULL)
        return;

    foreach(lc, type_cast->typeName->encryptedOpts)
    {
        DefElem *elem = (DefElem *) lc->data.ptr_value;
        if (elem != NULL && elem->defname != NULL &&
            pg_strcasecmp(elem->defname, "support_algo") == 0)
        {
            char *algo_value;

            if (elem->arg != NULL && nodeTag(elem->arg) == FE_T_String)
            {
                algo_value = strVal((const Value *)elem->arg);
                *supports_det = (strstr(algo_value, "det") != NULL);
                *supports_ope = (strstr(algo_value, "ope") != NULL);
                *supports_search = (strstr(algo_value, "search") != NULL);
            }
            break;
        }
    }
}

/* Begin - ReqID:TECC4.0 - Bug 7893/7901 */

/* Type family classification for encrypted type conversions */
typedef enum
{
    TECC_TYPE_FAMILY_NONE = 0,
    TECC_TYPE_FAMILY_INT,
    TECC_TYPE_FAMILY_FLOAT,
    TECC_TYPE_FAMILY_NUMERIC,
    TECC_TYPE_FAMILY_STRING
} TeccTypeFamily;

/*
 * Classify an encrypted type name into a type family.
 * Conversions within the same family (e.g., text_enc ↔ varchar_enc) are
 * allowed; cross-family conversions (e.g., text_enc → int2_enc) are not.
 */
static TeccTypeFamily
get_enc_type_family(const char *typename_str)
{
    if (typename_str == NULL)
        return TECC_TYPE_FAMILY_NONE;
    if (strcasecmp(typename_str, "int2_enc") == 0 ||
        strcasecmp(typename_str, "int4_enc") == 0 ||
        strcasecmp(typename_str, "int8_enc") == 0)
        return TECC_TYPE_FAMILY_INT;
    if (strcasecmp(typename_str, "float4_enc") == 0 ||
        strcasecmp(typename_str, "float8_enc") == 0)
        return TECC_TYPE_FAMILY_FLOAT;
    if (strcasecmp(typename_str, "numeric_enc") == 0)
        return TECC_TYPE_FAMILY_NUMERIC;
    if (strcasecmp(typename_str, "text_enc") == 0 ||
        strcasecmp(typename_str, "varchar_enc") == 0 ||
        strcasecmp(typename_str, "bpchar_enc") == 0)
        return TECC_TYPE_FAMILY_STRING;
    return TECC_TYPE_FAMILY_NONE;
}

/*
 * Rank an encrypted type by storage width (larger rank = wider type).
 * Used to reject narrowing casts within the same type family
 * (e.g., int8_enc -> int4_enc).
 */
static int
get_enc_type_rank(const char *typename_str)
{
    if (typename_str == NULL)
        return 0;
    if (strcasecmp(typename_str, "int2_enc") == 0)
        return 1;
    if (strcasecmp(typename_str, "int4_enc") == 0 ||
        strcasecmp(typename_str, "float4_enc") == 0)
        return 2;
    if (strcasecmp(typename_str, "int8_enc") == 0 ||
        strcasecmp(typename_str, "float8_enc") == 0)
        return 3;
    if (strcasecmp(typename_str, "numeric_enc") == 0)
        return 4;
    /* String types all treated equally */
    return 0;
}

/*
 * Extract DEK name from a TypeCast's encrypted options.
 * Returns a pointer into the parsed tree (do not free).
 */
static const char *
get_typecast_dek_name(const TypeCast *type_cast)
{
    ListCell *lc;

    if (type_cast == NULL || type_cast->typeName == NULL)
        return NULL;

    foreach (lc, type_cast->typeName->encryptedOpts)
    {
        DefElem *elem = (DefElem *) lc->data.ptr_value;
        if (elem != NULL && elem->defname != NULL &&
            pg_strcasecmp(elem->defname, "dek") == 0)
        {
            if (elem->arg != NULL && nodeTag(elem->arg) == FE_T_String)
                return strVal((const Value *)elem->arg);
            break;
        }
    }
    return NULL;
}

/*
 * Check that nested TypeCast nodes with encryptedOpts don't have conflicting
 * encrypted type families or DEK names. Cross-family casts (text_enc→int2_enc)
 * are rejected per Bug 7893/7894. Same-family casts with different DEKs
 * (text_enc→varchar_enc with dek_t1→dek_t2) are rejected per Bug 7901.
 * ReqID:TECC4.0 - Bug 7893/7901
 */
static bool
check_typecast_chain_nested_types(const TypeCast *type_cast,
                                  const char *orig_query,
                                  TeccStatementData *statement_data)
{
    char *outer_type;
    Node *arg_node;

    if (type_cast == NULL || type_cast->arg == NULL)
        return true;

    outer_type = get_typecast_typename(type_cast, orig_query);
    if (outer_type == NULL)
        return true;

    arg_node = type_cast->arg;

    while (arg_node != NULL && arg_node->type == FE_T_TypeCast)
    {
        TypeCast *inner = (TypeCast *)arg_node;

        if (inner->typeName != NULL &&
            inner->typeName->encryptedOpts != NIL)
        {
            char *inner_type = get_typecast_typename(inner, orig_query);

            if (inner_type != NULL)
            {
                TeccTypeFamily inner_family;
                TeccTypeFamily outer_family;

                inner_family = get_enc_type_family(inner_type);
                outer_family = get_enc_type_family(outer_type);

                                /*
                 * Cross-family conversion check.
                 * Numeric families (INTEGER, FLOAT, NUMERIC) are inter-convertible
                 * with rank-based narrowing protection (e.g., int4_enc -> float8_enc
                 * is widening → allowed; float8_enc -> int4_enc is narrowing → rejected).
                 * Other cross-family casts (e.g., text_enc -> int2_enc) are rejected.
                 * ReqID:TECC4.0 - Bug 7893/7993
                 */
                if (inner_family != TECC_TYPE_FAMILY_NONE &&
                    outer_family != TECC_TYPE_FAMILY_NONE &&
                    inner_family != outer_family)
                {
                    bool inner_is_numeric = (inner_family == TECC_TYPE_FAMILY_INT ||
                                             inner_family == TECC_TYPE_FAMILY_FLOAT ||
                                             inner_family == TECC_TYPE_FAMILY_NUMERIC);
                    bool outer_is_numeric = (outer_family == TECC_TYPE_FAMILY_INT ||
                                             outer_family == TECC_TYPE_FAMILY_FLOAT ||
                                             outer_family == TECC_TYPE_FAMILY_NUMERIC);

                    if (!inner_is_numeric || !outer_is_numeric)
                    {
                        tecc_processor_set_error(statement_data,
                            TECC_PROCESSOR_ERROR,
                            "Cannot cast encrypted type %s to %s: "
                            "incompatible encrypted types",
                            inner_type, outer_type);
                        if (inner_type != NULL)
                            pg_free(inner_type);
                        if (outer_type != NULL)
                            pg_free(outer_type);
                        return false;
                    }

                    /* Cross-numeric-family narrowing check: reject if inner type is
                     * wider than outer type (e.g., float8_enc -> int4_enc). */
                    {
                        int inner_rank = get_enc_type_rank(inner_type);
                        int outer_rank = get_enc_type_rank(outer_type);
                        if (inner_rank > 0 && outer_rank > 0 &&
                            inner_rank > outer_rank)
                        {
                            tecc_processor_set_error(statement_data,
                                TECC_PROCESSOR_ERROR,
                                "Cannot cast encrypted type %s to %s: "
                                "narrowing conversion would lose data",
                                inner_type, outer_type);
                            if (inner_type != NULL)
                                pg_free(inner_type);
                            if (outer_type != NULL)
                                pg_free(outer_type);
                            return false;
                        }
                    }
                }

                /* Same-family with different DEK → reject */
                if (inner_family != TECC_TYPE_FAMILY_NONE &&
                    outer_family != TECC_TYPE_FAMILY_NONE &&
                    inner_family == outer_family)
                {
                    const char *inner_dek;
                    const char *outer_dek;

                    inner_dek = get_typecast_dek_name(inner);
                    outer_dek = get_typecast_dek_name(type_cast);

                    if (inner_dek != NULL && outer_dek != NULL &&
                        strcasecmp(inner_dek, outer_dek) != 0)
                    {
                        tecc_processor_set_error(statement_data,
                            TECC_PROCESSOR_ERROR,
                            "Cannot cast encrypted type %s to %s "
                            "with different DEKs (%s vs %s)",
                            inner_type, outer_type,
                            inner_dek, outer_dek);
                        if (inner_type != NULL)
                            pg_free(inner_type);
                        if (outer_type != NULL)
                            pg_free(outer_type);
                        return false;
                    }

                    /* Same-family narrowing check: reject if inner type is
                     * wider than outer type (e.g., int4_enc -> int2_enc).
                     * ReqID:TECC4.0 - Bug 7992 */
                    {
                        int inner_rank = get_enc_type_rank(inner_type);
                        int outer_rank = get_enc_type_rank(outer_type);
                        if (inner_rank > 0 && outer_rank > 0 &&
                            inner_rank > outer_rank)
                        {
                            tecc_processor_set_error(statement_data,
                                TECC_PROCESSOR_ERROR,
                                "Cannot cast encrypted type %s to %s: "
                                "narrowing conversion would lose data",
                                inner_type, outer_type);
                            if (inner_type != NULL)
                                pg_free(inner_type);
                            if (outer_type != NULL)
                                pg_free(outer_type);
                            return false;
                        }
                    }
                }
            }
            if (inner_type != NULL)
                pg_free(inner_type);
        }
        arg_node = inner->arg;
    }

    if (outer_type != NULL)
        pg_free(outer_type);
    return true;
}
/* End - ReqID:TECC4.0 */

/*
 * Find the comparison context (column reference and operator) for a TypeCast
 * node within a WHERE clause tree. Walks the expression tree looking for the
 * parent A_Expr that contains this TypeCast on one side and a ColumnRef on
 * the other. Returns true and sets col_ref/operator_name if found.
 * ReqID:TECC4.0 - Bug 8001
 */
static bool
find_where_comparison_context(Node *where_clause, TypeCast *type_cast,
                              ColumnRef **col_ref, const char **operator_name)
{
    if (where_clause == NULL || type_cast == NULL || col_ref == NULL || operator_name == NULL)
        return false;

    switch (nodeTag(where_clause))
    {
        case FE_T_A_Expr:
        {
            A_Expr *ae = (A_Expr *)where_clause;

            if (ae->lexpr == (Node *)type_cast || ae->rexpr == (Node *)type_cast)
            {
                Node *other = (ae->lexpr == (Node *)type_cast)
                    ? ae->rexpr : ae->lexpr;

                if (other == NULL)
                    return false;

                /* Unwrap CollateClause (e.g., name COLLATE "C") */
                if (nodeTag(other) == FE_T_CollateClause)
                    other = ((CollateClause *)other)->arg;

                if (nodeTag(other) != FE_T_ColumnRef)
                    return false;

                *col_ref = (ColumnRef *)other;
                *operator_name = (ae->name != NIL)
                    ? strVal((Value *)llast(ae->name)) : NULL;
                return true;
            }

            /* Recurse */
            if (ae->lexpr != NULL &&
                find_where_comparison_context(ae->lexpr, type_cast, col_ref, operator_name))
                return true;
            if (ae->rexpr != NULL &&
                find_where_comparison_context(ae->rexpr, type_cast, col_ref, operator_name))
                return true;
            break;
        }
        case FE_T_BoolExpr:
        {
            struct BoolExpr *be = (struct BoolExpr *)where_clause;
            ListCell *lc;
            foreach (lc, be->args)
            {
                if (find_where_comparison_context((Node *)lc->data.ptr_value,
                                                  type_cast, col_ref, operator_name))
                    return true;
            }
            break;
        }
        case FE_T_NullTest:
        {
            NullTest *nt = (NullTest *)where_clause;
            if (nt->arg != NULL &&
                find_where_comparison_context(nt->arg, type_cast, col_ref, operator_name))
                return true;
            break;
        }
        case FE_T_BooleanTest:
        {
            struct BooleanTest *bt = (struct BooleanTest *)where_clause;
            if (bt->arg != NULL &&
                find_where_comparison_context(bt->arg, type_cast, col_ref, operator_name))
                return true;
            break;
        }
        default:
            break;
    }
    return false;
}

/*
 * Map a SQL operator name string to TeccOperatorType enum.
 * ReqID:TECC4.0 - Bug 8001
 */
static TeccOperatorType
map_operator_name_to_type(const char *operator_name)
{
    if (operator_name == NULL) return TECC_OP_UNKNOWN;
    if (strcmp(operator_name, "=") == 0) return TECC_OP_EQUAL;
    if (strcmp(operator_name, "<>") == 0 || strcmp(operator_name, "!=") == 0)
        return TECC_OP_NOT_EQUAL;
    if (strcmp(operator_name, "<") == 0) return TECC_OP_LESS_THAN;
    if (strcmp(operator_name, ">") == 0) return TECC_OP_GREATER_THAN;
    if (strcmp(operator_name, "<=") == 0) return TECC_OP_LESS_EQUAL;
    if (strcmp(operator_name, ">=") == 0) return TECC_OP_GREATER_EQUAL;
    if (pg_strcasecmp(operator_name, "like") == 0) return TECC_OP_LIKE;
    if (pg_strcasecmp(operator_name, "not like") == 0) return TECC_OP_NOT_LIKE;
    if (pg_strcasecmp(operator_name, "ilike") == 0) return TECC_OP_ILIKE;
    if (pg_strcasecmp(operator_name, "not ilike") == 0) return TECC_OP_NOT_ILIKE;
    return TECC_OP_UNKNOWN;
}

/*
 * Process a single TypeCast node with encryptedOpts: extract the constant,
 * encrypt it, and append the encrypted value to the rewritten query.
 */
static TeccStmtProcessorResult
process_one_typecast(TeccStatementData *statement_data,
                     TypeCast *type_cast,
                     const char *orig_query,
                     size_t orig_len,
                     size_t *orig_pos,
                     char **new_query,
                     size_t *new_query_len,
                     size_t *new_query_capacity,
                     int *typecast_processed)
{
    size_t expr_start;
    size_t expr_end;
    char *dek_name = NULL;
    char plaintext[256];
    ListCell *opt_lc;
    TeccUnifiedEncryptResult *encrypt_result = NULL;

    /* Find DEK name from encrypted options */
    foreach (opt_lc, type_cast->typeName->encryptedOpts)
    {
        DefElem *elem = (DefElem *)opt_lc->data.ptr_value;
        if (elem != NULL &&
            (elem->defname != NULL && pg_strcasecmp(elem->defname, "dek") == 0))
        {
            if (elem->arg != NULL && nodeTag(elem->arg) == FE_T_String)
                dek_name = strVal((const Value *)elem->arg);
            break;
        }
    }

    if (dek_name == NULL)
    {
        tecc_processor_debug(TECC_LOG_WARNING,
                           "No DEK found in encrypted options for TypeCast");
        return TECC_STMT_PROCESSOR_SUCCESS;
    }

    /* Begin - ReqID:TECC4.0 - Bug 7893: reject nested TypeCasts with conflicting encrypted types */
    if (!check_typecast_chain_nested_types(type_cast, orig_query, statement_data))
        return TECC_STMT_PROCESSOR_ERROR;
    /* End - ReqID:TECC4.0 */

    /* Extract plaintext from the Const argument */
    {
        /* Walk through nested TypeCast chain to find innermost Const */
        Node *arg_node = type_cast->arg;
        Const *cnst;

        while (arg_node != NULL && arg_node->type == FE_T_TypeCast)
            arg_node = ((TypeCast *)arg_node)->arg;

        cnst = (Const *)arg_node;
        if (cnst == NULL || cnst->type != FE_T_Const)
        {
            tecc_processor_debug(TECC_LOG_WARNING,
                               "TypeCast arg is not a Const, skipping");
            return TECC_STMT_PROCESSOR_SUCCESS;
        }

        /* Convert Const value to string based on type */
        if (cnst->constbyval && (cnst->constlen == sizeof(int32_t) || cnst->constlen == 2))
        {
            /* Integer constant (int2 or int4) */
            snprintf(plaintext, sizeof(plaintext), "%d",
                     (int)cnst->constvalue.ival);
        }
        else if (cnst->constbyval && cnst->constlen == sizeof(float8))
        {
            /* Float constant */
            snprintf(plaintext, sizeof(plaintext), "%g",
                     cnst->constvalue.dval);
        }
        else if (cnst->constvalue.sval != NULL && !cnst->constisnull)
        {
            /* String constant */
            snprintf(plaintext, sizeof(plaintext), "%s",
                     cnst->constvalue.sval);
        }
        else if (cnst->constisnull)
        {
            /* NULL constant - just pass through */
            return TECC_STMT_PROCESSOR_SUCCESS;
        }
        else
        {
            tecc_processor_debug(TECC_LOG_WARNING,
                               "Unsupported Const type for encryption");
            return TECC_STMT_PROCESSOR_SUCCESS;
        }
    }

    /* Encrypt the value — use with_options to bypass column metadata cache,
     * since TypeCast expressions have no real table/column in the cache.
     * Determine algorithm flags from the declared encrypted type and optional
     * support_algo in the encrypted options. */
    {
        char *typename_str;
        Oid base_oid;
        bool supports_det, supports_ope, supports_search;

        typename_str = get_typecast_typename(type_cast, orig_query);
        base_oid = (typename_str != NULL) ?
            map_enc_type_name_to_base_oid(typename_str) : InvalidOid;
        get_typecast_algo_flags(type_cast, typename_str,
                                &supports_det, &supports_ope, &supports_search);

        if (OidIsValid(base_oid))
        {
            encrypt_result = tecc_unified_encrypt_string_with_options(
                statement_data->conn,
                plaintext,
                "insert_select", typename_str ? typename_str : "typecast",
                NULL, dek_name,
                base_oid, -1,
                supports_det, supports_ope, supports_search, NULL);
        }
        else
        {
            encrypt_result = tecc_unified_encrypt_string(
                statement_data->conn, plaintext,
                "insert_select", "typecast",
                NULL, dek_name, NULL);
        }

        if (typename_str != NULL)
            pg_free(typename_str);
    }

    if (encrypt_result == NULL ||
        encrypt_result->error_code != TECC_CRYPTO_SUCCESS)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Encryption failed for TypeCast value: %s",
                           encrypt_result ? encrypt_result->error_msg : "unknown error");
        if (encrypt_result)
            tecc_unified_free_encrypt_result(encrypt_result);
        return TECC_STMT_PROCESSOR_ERROR;
    }

    /* Find the TypeCast expression position in the original SQL */
    expr_start = (size_t)type_cast->location;
    expr_end = find_typecast_expr_end(orig_query, type_cast);

    

    /* Ensure valid positions */
    if (expr_start >= orig_len || expr_end > orig_len || expr_end <= expr_start)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid TypeCast expression positions: start=%zu end=%zu",
                           expr_start, expr_end);
        tecc_unified_free_encrypt_result(encrypt_result);
        return TECC_STMT_PROCESSOR_ERROR;
    }

    /* Copy original text from orig_pos to expr_start */
    if (expr_start > *orig_pos)
    {
        size_t copy_len = expr_start - *orig_pos;
        while (*new_query_len + copy_len + 1 > *new_query_capacity)
        {
            *new_query_capacity *= 2;
            *new_query = (char *)pg_realloc(*new_query, *new_query_capacity);
            if (*new_query == NULL)
            {
                tecc_unified_free_encrypt_result(encrypt_result);
                return TECC_STMT_PROCESSOR_ERROR;
            }
        }
        memcpy(*new_query + *new_query_len, orig_query + *orig_pos, copy_len);
        *new_query_len += copy_len;
    }

    /* Append encrypted hex string: '\x<hex>'::typename */
    {
        size_t hex_len = encrypt_result->hex_len;
        char *typename_str = get_typecast_typename(type_cast, orig_query);
        size_t typelen = (typename_str != NULL) ? strlen(typename_str) : 0;
        size_t needed = *new_query_len + hex_len + 3 +
                        (typelen > 0 ? typelen + 2 : 0);
        while (needed > *new_query_capacity)
        {
            *new_query_capacity *= 2;
            *new_query = (char *)pg_realloc(*new_query, *new_query_capacity);
            if (*new_query == NULL)
            {
                if (typename_str)
                    pg_free(typename_str);
                tecc_unified_free_encrypt_result(encrypt_result);
                return TECC_STMT_PROCESSOR_ERROR;
            }
        }

        /* Build: 'hex_string' (hex_string already has \x prefix) */
        (*new_query)[*new_query_len] = '\'';
        memcpy(*new_query + *new_query_len + 1,
               encrypt_result->hex_string, hex_len);
        *new_query_len += 1 + hex_len;
        (*new_query)[*new_query_len] = '\'';
        *new_query_len += 1;

        /* Append ::typename to preserve type info for backend cast checking */
        if (typename_str != NULL && typelen > 0)
        {
            (*new_query)[*new_query_len] = ':';
            (*new_query)[*new_query_len + 1] = ':';
            memcpy(*new_query + *new_query_len + 2, typename_str, typelen);
            *new_query_len += 2 + typelen;
            pg_free(typename_str);
        }
    }

    tecc_unified_free_encrypt_result(encrypt_result);

    /* Advance past the TypeCast expression in the original text */
    if (expr_end > *orig_pos)
        *orig_pos = expr_end;

    (*typecast_processed)++;
    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Process INSERT...SELECT encrypted TypeCast expressions.
 * For statements like: INSERT INTO enc_table SELECT 1::int2_enc ENCRYPTED WITH (dek=...)
 * This function encrypts the constant values and rewrites the query.
 */
static TeccStmtProcessorResult
process_insert_select_typecast(SelectStmt *select_stmt,
                              TeccStatementData *statement_data)
{
    ListCell *lc;
    const char *orig_query;
    char *new_query = NULL;
    size_t new_query_len = 0;
    size_t new_query_capacity = 0;
    size_t orig_pos = 0;
    size_t orig_len;
    int typecast_processed = 0;

    if (select_stmt == NULL ||
        (select_stmt->targetList == NIL && select_stmt->valuesLists == NIL))
        return TECC_STMT_PROCESSOR_SUCCESS;

    orig_query = statement_data->query;
    if (orig_query == NULL)
    {
        /* ReqID:TECC4.0 - Subquery (e.g., RangeSubselect in FROM) has no original query
         * string; skip TypeCast rewriting since there is nothing to rewrite. */
        return TECC_STMT_PROCESSOR_SUCCESS;
    }

    orig_len = strlen(orig_query);

    /* First pass: count TypeCast nodes and allocate initial buffer */
    {
        size_t extra_estimate = 256; /* extra space for encrypted values */

        new_query_capacity = orig_len + extra_estimate + 1024;
        new_query = (char *)pg_malloc(new_query_capacity);
        if (new_query == NULL)
            return TECC_STMT_PROCESSOR_ERROR;
        new_query[0] = '\0';
    }

    /* Build rewritten query by scanning from left to right */
    if (select_stmt->targetList != NIL)
    {
    for (lc = select_stmt->targetList->head; lc != NULL; lc = lc->next)
    {
        Node *target_node = (Node *)lc->data.ptr_value;
        ResTarget *res_target;

        if (target_node == NULL || target_node->type != FE_T_ResTarget)
            continue;

        res_target = (ResTarget *)target_node;
        if (res_target->val == NULL)
            continue;

        /* Collect all TypeCast nodes with encryptedOpts from this target,
         * including those nested inside function arguments, subexpressions, etc. */
        {
            List *typecast_nodes = NIL;
            ListCell *tc_lc;

            collect_typecast_nodes(res_target->val, &typecast_nodes);

            if (typecast_nodes == NIL)
                continue;

            foreach (tc_lc, typecast_nodes)
            {
                TypeCast *type_cast = (TypeCast *)tc_lc->data.ptr_value;

                /* Found a TypeCast with encrypted options */
                {
                size_t expr_start;
                size_t expr_end;
                char *dek_name = NULL;
                char plaintext[256];
                ListCell *opt_lc;
                TeccUnifiedEncryptResult *encrypt_result = NULL;

                /* Find DEK name from encrypted options */
            foreach (opt_lc, type_cast->typeName->encryptedOpts)
            {
                DefElem *elem = (DefElem *)opt_lc->data.ptr_value;
                if (elem != NULL &&
                    (elem->defname != NULL && pg_strcasecmp(elem->defname, "dek") == 0))
                {
                    if (elem->arg != NULL && nodeTag(elem->arg) == FE_T_String)
                        dek_name = strVal((const Value *)elem->arg);
                    break;
                }
            }

            if (dek_name == NULL)
            {
                tecc_processor_debug(TECC_LOG_WARNING,
                                   "No DEK found in encrypted options for TypeCast");
                continue;
            }

            /* Begin - ReqID:TECC4.0 - Bug 7893: reject nested TypeCasts with conflicting encrypted types */
            if (!check_typecast_chain_nested_types(type_cast, orig_query, statement_data))
            {
                if (new_query)
                    pg_free(new_query);
                return TECC_STMT_PROCESSOR_ERROR;
            }
            /* End - ReqID:TECC4.0 */

            /* Extract plaintext from the Const argument */
            {
                /* Walk through nested TypeCast chain to find innermost Const */
                Node *arg_node = type_cast->arg;
                Const *cnst;

                while (arg_node != NULL && arg_node->type == FE_T_TypeCast)
                    arg_node = ((TypeCast *)arg_node)->arg;

                cnst = (Const *)arg_node;
                if (cnst == NULL || cnst->type != FE_T_Const)
                {
                    tecc_processor_debug(TECC_LOG_WARNING,
                                       "TypeCast arg is not a Const, skipping");
                    continue;
                }

                /* Convert Const value to string based on type */
                if (cnst->constbyval && (cnst->constlen == sizeof(int32_t) || cnst->constlen == 2))
                {
                    /* Integer constant */
                    snprintf(plaintext, sizeof(plaintext), "%d",
                             (int)cnst->constvalue.ival);
                }
                else if (cnst->constbyval && cnst->constlen == sizeof(float8))
                {
                    /* Float constant */
                    snprintf(plaintext, sizeof(plaintext), "%g",
                             cnst->constvalue.dval);
                }
                else if (cnst->constvalue.sval != NULL && !cnst->constisnull)
                {
                    /* String constant */
                    snprintf(plaintext, sizeof(plaintext), "%s",
                             cnst->constvalue.sval);
                }
                else if (cnst->constisnull)
                {
                    /* NULL constant - just pass through */
                    continue;
                }
                else
                {
                    tecc_processor_debug(TECC_LOG_WARNING,
                                       "Unsupported Const type for encryption");
                    continue;
                }
            }

            /* Encrypt the value — use with_options to bypass column metadata cache */
            {
                char *typename_str = get_typecast_typename(type_cast, orig_query);
                Oid base_oid = (typename_str != NULL) ?
                    map_enc_type_name_to_base_oid(typename_str) : InvalidOid;
                bool supports_det, supports_ope, supports_search;

                get_typecast_algo_flags(type_cast, typename_str,
                                        &supports_det, &supports_ope, &supports_search);

                if (OidIsValid(base_oid))
                {
                    encrypt_result = tecc_unified_encrypt_string_with_options(
                        statement_data->conn, plaintext,
                        "insert_select", typename_str ? typename_str : "typecast",
                        NULL, dek_name,
                        base_oid, -1,
                        supports_det, supports_ope, supports_search, NULL);
                }
                else
                {
                    encrypt_result = tecc_unified_encrypt_string(
                        statement_data->conn, plaintext,
                        "insert_select", "typecast",
                        NULL, dek_name, NULL);
                }

                if (typename_str != NULL)
                    pg_free(typename_str);
            }

            if (encrypt_result == NULL ||
                encrypt_result->error_code != TECC_CRYPTO_SUCCESS)
            {
                tecc_processor_debug(TECC_LOG_ERROR,
                                   "Encryption failed for TypeCast value: %s",
                                   encrypt_result ? encrypt_result->error_msg : "unknown error");
                if (encrypt_result)
                    tecc_unified_free_encrypt_result(encrypt_result);
                if (new_query)
                    pg_free(new_query);
                return TECC_STMT_PROCESSOR_ERROR;
            }

            /* Find the TypeCast expression position in the original SQL */
            expr_start = (size_t)type_cast->location;
            expr_end = find_typecast_expr_end(orig_query, type_cast);

            

            /* Ensure valid positions */
            if (expr_start >= orig_len || expr_end > orig_len || expr_end <= expr_start)
            {
                tecc_processor_debug(TECC_LOG_ERROR,
                                   "Invalid TypeCast expression positions: start=%zu end=%zu",
                                   expr_start, expr_end);
                tecc_unified_free_encrypt_result(encrypt_result);
                if (new_query)
                    pg_free(new_query);
                return TECC_STMT_PROCESSOR_ERROR;
            }

            /* Copy original text from orig_pos to expr_start */
            if (expr_start > orig_pos)
            {
                size_t copy_len = expr_start - orig_pos;
                while (new_query_len + copy_len + 1 > new_query_capacity)
                {
                    new_query_capacity *= 2;
                    new_query = (char *)pg_realloc(new_query, new_query_capacity);
                    if (new_query == NULL)
                    {
                        tecc_unified_free_encrypt_result(encrypt_result);
                        return TECC_STMT_PROCESSOR_ERROR;
                    }
                }
                memcpy(new_query + new_query_len, orig_query + orig_pos, copy_len);
                new_query_len += copy_len;
            }

            /* Append encrypted hex string: '\x<hex>'::typename */
            {
                size_t hex_len = encrypt_result->hex_len;
                char *typename_str = get_typecast_typename(type_cast, orig_query);
                size_t typelen = (typename_str != NULL) ? strlen(typename_str) : 0;
                /* hex_string already includes \x prefix, just wrap in quotes */
                size_t needed = new_query_len + hex_len + 3 +
                                (typelen > 0 ? typelen + 2 : 0);
                while (needed > new_query_capacity)
                {
                    new_query_capacity *= 2;
                    new_query = (char *)pg_realloc(new_query, new_query_capacity);
                    if (new_query == NULL)
                    {
                        if (typename_str)
                            pg_free(typename_str);
                        tecc_unified_free_encrypt_result(encrypt_result);
                        return TECC_STMT_PROCESSOR_ERROR;
                    }
                }

                /* Build: 'hex_string' (hex_string already has \x prefix) */
                new_query[new_query_len] = '\'';
                memcpy(new_query + new_query_len + 1,
                       encrypt_result->hex_string, hex_len);
                new_query_len += 1 + hex_len;
                new_query[new_query_len] = '\'';
                new_query_len += 1;

                /* Append ::typename to preserve type info for backend cast checking */
                if (typename_str != NULL && typelen > 0)
                {
                    new_query[new_query_len] = ':';
                    new_query[new_query_len + 1] = ':';
                    memcpy(new_query + new_query_len + 2, typename_str, typelen);
                    new_query_len += 2 + typelen;
                    pg_free(typename_str);
                }
            }

            tecc_unified_free_encrypt_result(encrypt_result);

            /* Advance past the TypeCast expression in the original text */
            if (expr_end > orig_pos)
                orig_pos = expr_end;

            typecast_processed++;
            }
        }
    }
    }
    }

    /* Process valuesLists (INSERT...VALUES) — each row has a list of value expressions */
    if (select_stmt->valuesLists != NIL)
    {
        ListCell *row_lc;
        foreach (row_lc, select_stmt->valuesLists)
        {
            List *row = (List *)row_lc->data.ptr_value;
            ListCell *val_lc;
            if (row == NIL) continue;

            foreach (val_lc, row)
            {
                Node *val = (Node *)val_lc->data.ptr_value;
                if (val == NULL) continue;

                /* Values in INSERT...VALUES are direct expression nodes,
                 * not wrapped in ResTarget like targetList entries */
                {
                    List *typecast_nodes = NIL;
                    ListCell *tc_lc;

                    collect_typecast_nodes(val, &typecast_nodes);

                    if (typecast_nodes == NIL)
                        continue;

                    foreach (tc_lc, typecast_nodes)
                    {
                        TypeCast *type_cast = (TypeCast *)tc_lc->data.ptr_value;

                        TeccStmtProcessorResult res = process_one_typecast(
                            statement_data, type_cast, orig_query, orig_len,
                            &orig_pos, &new_query, &new_query_len,
                            &new_query_capacity, &typecast_processed);
                        if (res != TECC_STMT_PROCESSOR_SUCCESS)
                        {
                            if (new_query)
                                pg_free(new_query);
                            return res;
                        }
                    }
                }
            }
        }
    }

    /* Process whereClause — TypeCast expressions with encrypted options
     * in WHERE/Having clause (e.g., col = 'val'::text_enc encrypted with (dek=...)).
     *
     * For WHERE clause comparisons, we must use tecc_unified_encrypt_for_where()
     * instead of the generic process_one_typecast() encryption, because:
     *   1. The operator determines which algorithm to use (DET for =, OPE for < >)
     *   2. The column's DEK/KEK from the cache must be used, not the TypeCast's DEK
     *   3. The TypeCast's support_algo may not include the algorithm needed
     * ReqID:TECC4.0 - Bug 8001 */
    if (select_stmt->whereClause != NULL)
    {
        List *typecast_nodes = NIL;
        collect_typecast_nodes(select_stmt->whereClause, &typecast_nodes);
        if (typecast_nodes != NIL)
        {
            ListCell *tc_lc;
            foreach (tc_lc, typecast_nodes)
            {
                TypeCast *type_cast = (TypeCast *)tc_lc->data.ptr_value;
                ColumnRef *col_ref = NULL;
                const char *op_name = NULL;
                const char *column_name = NULL;
                const char *table_name = NULL;
                const char *schema_name = NULL;
                const char *dek_name = NULL;
                const char *kek_name = NULL;
                TeccOperatorType op_type;
                Node *arg_node;
                Const *cnst;
                char plaintext[256];
                TeccUnifiedEncryptResult *encrypt_result = NULL;
                bool where_encrypted = false;

                /* Find the comparison column and operator for this TypeCast */
                if (find_where_comparison_context(select_stmt->whereClause,
                                                  type_cast, &col_ref, &op_name))
                {
                    /* Extract column name from ColumnRef */
                    if (col_ref->fields != NIL)
                    {
                        int nf = list_length(col_ref->fields);
                        column_name = strVal((Value *)llast(col_ref->fields));
                        if (nf >= 2)
                            table_name = strVal((Value *)list_nth(col_ref->fields, nf - 2));
                        if (nf >= 3)
                            schema_name = strVal((Value *)linitial(col_ref->fields));
                    }

                    if (column_name != NULL)
                    {
                        /* Resolve real table name from encrypted_columns */
                        ListCell *elc;
                        foreach (elc, statement_data->encrypted_columns)
                        {
                            TeccProcessorColumnRef *ed = (TeccProcessorColumnRef *)lfirst(elc);
                            if (ed == NULL || ed->column_name[0] == '\0')
                                continue;
                            if (strcmp(ed->column_name, column_name) != 0)
                                continue;
                            if (table_name != NULL)
                            {
                                if (strcmp(ed->table_alias, table_name) == 0 ||
                                    strcmp(ed->table_name, table_name) == 0)
                                {
                                    table_name = ed->table_name;
                                    if (ed->schema_name[0] != '\0')
                                        schema_name = ed->schema_name;
                                    where_encrypted = true;
                                    break;
                                }
                            }
                            else
                            {
                                /* No table qualifier — single table, use it */
                                table_name = ed->table_name;
                                if (ed->schema_name[0] != '\0')
                                    schema_name = ed->schema_name;
                                where_encrypted = true;
                                break;
                            }
                        }
                    }
                }

                if (where_encrypted && column_name != NULL && table_name != NULL)
                {
                    /* Look up DEK from cache */
                    dek_name = tecc_get_column_dek_name(statement_data,
                                                         schema_name, table_name, column_name);
                    if (dek_name != NULL)
                    {
                        /* Look up KEK from DEK cache */
                        if (statement_data->conn != NULL)
                        {
                            TeccState *state = tecc_get_state(statement_data->conn);
                            if (state != NULL)
                            {
                                TeccDekCacheItem *dek_item =
                                    tecc_cache_get_dek_by_name_search_path(
                                        state, dek_name);
                                if (dek_item != NULL)
                                    kek_name = dek_item->kek_name;
                            }
                        }
                    }

                    if (dek_name != NULL && kek_name != NULL)
                    {
                        /* Map operator string to TeccOperatorType */
                        op_type = map_operator_name_to_type(op_name);

                        if (op_type != TECC_OP_UNKNOWN)
                        {
                            /* Extract plaintext from innermost Const */
                            arg_node = type_cast->arg;
                            while (arg_node != NULL && arg_node->type == FE_T_TypeCast)
                                arg_node = ((TypeCast *)arg_node)->arg;

                            cnst = (Const *)arg_node;
                            if (cnst != NULL && cnst->type == FE_T_Const && !cnst->constisnull)
                            {
                                if (cnst->constbyval &&
                                    (cnst->constlen == sizeof(int32_t) || cnst->constlen == 2))
                                {
                                    snprintf(plaintext, sizeof(plaintext), "%d",
                                             (int)cnst->constvalue.ival);
                                }
                                else if (cnst->constbyval && cnst->constlen == sizeof(float8))
                                {
                                    snprintf(plaintext, sizeof(plaintext), "%g",
                                             cnst->constvalue.dval);
                                }
                                else if (cnst->constvalue.sval != NULL)
                                {
                                    snprintf(plaintext, sizeof(plaintext), "%s",
                                             cnst->constvalue.sval);
                                }
                                else
                                {
                                    where_encrypted = false; /* Unsupported Const type */
                                }
                            }
                            else
                            {
                                where_encrypted = false; /* Not a Const or IS NULL */
                            }
                        }
                        else
                        {
                            where_encrypted = false; /* Unknown operator */
                        }
                    }
                    else
                    {
                        where_encrypted = false; /* Could not resolve DEK/KEK */
                    }
                }

                if (where_encrypted)
                {
                    /* Encrypt using WHERE-specific function */
                    encrypt_result = tecc_unified_encrypt_for_where(
                        statement_data->conn, plaintext,
                        table_name, column_name,
                        kek_name, dek_name, op_type, NULL);

                    if (encrypt_result != NULL &&
                        encrypt_result->error_code == TECC_CRYPTO_SUCCESS)
                    {
                        /* Replace the TypeCast expression in the SQL buffer */
                        size_t expr_start = (size_t)type_cast->location;
                        size_t expr_end = find_typecast_expr_end(orig_query, type_cast);
                        size_t hex_len = encrypt_result->hex_len;
                        char *typename_str = get_typecast_typename(type_cast, orig_query);
                        size_t typelen = (typename_str != NULL) ? strlen(typename_str) : 0;

                        if (expr_start < orig_len && expr_end <= orig_len && expr_end > expr_start)
                        {
                            /* Copy text before TypeCast */
                            if (expr_start > orig_pos)
                            {
                                size_t copy_len = expr_start - orig_pos;
                                while (new_query_len + copy_len + 1 > new_query_capacity)
                                {
                                    new_query_capacity *= 2;
                                    new_query = (char *)pg_realloc(new_query, new_query_capacity);
                                    if (new_query == NULL)
                                    {
                                        if (typename_str) pg_free(typename_str);
                                        tecc_unified_free_encrypt_result(encrypt_result);
                                        return TECC_STMT_PROCESSOR_ERROR;
                                    }
                                }
                                memcpy(new_query + new_query_len, orig_query + orig_pos, copy_len);
                                new_query_len += copy_len;
                            }

                            /* Append encrypted hex string with type cast */
                            {
                                size_t needed = new_query_len + hex_len + 3 +
                                                (typelen > 0 ? typelen + 2 : 0);
                                while (needed > new_query_capacity)
                                {
                                    new_query_capacity *= 2;
                                    new_query = (char *)pg_realloc(new_query, new_query_capacity);
                                    if (new_query == NULL)
                                    {
                                        if (typename_str) pg_free(typename_str);
                                        tecc_unified_free_encrypt_result(encrypt_result);
                                        return TECC_STMT_PROCESSOR_ERROR;
                                    }
                                }
                                new_query[new_query_len] = '\'';
                                memcpy(new_query + new_query_len + 1,
                                       encrypt_result->hex_string, hex_len);
                                new_query_len += 1 + hex_len;
                                new_query[new_query_len] = '\'';
                                new_query_len += 1;

                                if (typename_str != NULL && typelen > 0)
                                {
                                    new_query[new_query_len] = ':';
                                    new_query[new_query_len + 1] = ':';
                                    memcpy(new_query + new_query_len + 2, typename_str, typelen);
                                    new_query_len += 2 + typelen;
                                }
                            }

                            if (typename_str != NULL)
                                pg_free(typename_str);

                            /* Advance past TypeCast */
                            if (expr_end > orig_pos)
                                orig_pos = expr_end;

                            typecast_processed++;
                        }

                        tecc_unified_free_encrypt_result(encrypt_result);
                        encrypt_result = NULL;
                        continue; /* Skip to next TypeCast */
                    }

                    /* Encryption failed — fall through to fallback */
                    if (encrypt_result)
                    {
                        tecc_unified_free_encrypt_result(encrypt_result);
                        encrypt_result = NULL;
                    }
                }

                /* Fallback: use generic process_one_typecast */
                {
                    TeccStmtProcessorResult res = process_one_typecast(
                        statement_data, type_cast, orig_query, orig_len,
                        &orig_pos, &new_query, &new_query_len,
                        &new_query_capacity, &typecast_processed);
                    if (res != TECC_STMT_PROCESSOR_SUCCESS)
                    {
                        if (new_query)
                            pg_free(new_query);
                        return res;
                    }
                }
            }
        }
    }

    /* Copy any remaining text after the last processed expression */
    if (orig_pos < orig_len)
    {
        size_t remaining = orig_len - orig_pos;
        while (new_query_len + remaining + 1 > new_query_capacity)
        {
            new_query_capacity *= 2;
            new_query = (char *)pg_realloc(new_query, new_query_capacity);
            if (new_query == NULL)
                return TECC_STMT_PROCESSOR_ERROR;
        }
        memcpy(new_query + new_query_len, orig_query + orig_pos, remaining);
        new_query_len += remaining;
    }

    /* Null-terminate */
    new_query[new_query_len] = '\0';

    if (typecast_processed > 0)
    {
        /* Mark rewrite needed and set the rewritten query */
        statement_data->needs_rewrite = 1;
        if (statement_data->rewritten_query != NULL)
            pg_free(statement_data->rewritten_query);
        statement_data->rewritten_query = new_query;

        

        /* Debug: log rewritten query */

    }
    else
    {
        /* No TypeCast processed, free the buffer */
        if (new_query)
            pg_free(new_query);
    }

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Process INSERT statement
 */
TeccStmtProcessorResult
tecc_stmt_processor_process_insert(const InsertStmt *insert_stmt,
                                  TeccStatementData *statement_data)
{
    int encrypted_column_count = 0;
    int values_column_count = 0;
    TeccStmtProcessorResult select_result;
    TeccValuesProcessorResult values_result;
    TeccColumnCacheItem **table_encrypted_columns = NULL;
    int table_encrypted_column_count = 0;
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    TeccColumnRefData *col_ref = NULL;
    char *column_name = NULL;

    if (insert_stmt == NULL || statement_data == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    /* ReqID:TECC4.0 - Set stmt_type */
    statement_data->stmt_type = TECC_STMT_TYPE_INSERT;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing INSERT statement");

    /* 添加详细调试日志 */
    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "INSERT statement details: relation=%s, cols=%p, selectStmt=%p",
                        insert_stmt->relation ? insert_stmt->relation->relname : "NULL",
                        insert_stmt->cols,
                        insert_stmt->selectStmt);

    /* Process target table */
    if (insert_stmt->relation != NULL)
    {
        RangeVar *range_var = insert_stmt->relation;
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "INSERT target table: %s.%s",
                           range_var->schemaname ? range_var->schemaname : "(null)",
                           range_var->relname ? range_var->relname : "(null)");

        /* Load table metadata and identify encrypted columns */
        stmt_load_table_metadata(range_var, statement_data, &table_encrypted_columns, &table_encrypted_column_count);
        

        /* Check if column list is explicitly specified */
        if (insert_stmt->cols != NULL)
        {
            ListCell *lc;
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "INSERT column list exists, checking %d columns",
                               insert_stmt->cols->length);

            for (lc = insert_stmt->cols->head; lc != NULL; lc = lc->next)
            {
                Node *target_node = (Node *) lc->data.ptr_value;
                bool is_encrypted;

                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing INSERT column target, node=%p, type=%d (0x%x)",
                                   target_node, target_node ? target_node->type : -1, target_node ? target_node->type : 0);

                /* Check if this is a Const node */
                if (target_node == NULL || target_node->type == FE_T_Const) {
                    /* It's a constant value or NULL, not a column reference */
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                       "Skipping Const node or NULL");
                    continue;
                }

                /* 检查是否是ResTarget节点 */
                if (target_node->type == FE_T_ResTarget) {
                    ResTarget *res_target = (ResTarget *) target_node;
                    if (res_target->name != NULL)
                    {
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                           "INSERT target column: %s", res_target->name);
                        values_column_count++;

                        /* Check if this column is encrypted using cached column list */
                        is_encrypted = stmt_is_column_in_encrypted_list(res_target->name, table_encrypted_columns, table_encrypted_column_count);
                        if (is_encrypted) {
                            statement_data->needs_rewrite = 1;
                            encrypted_column_count++;
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                               "Column %s is encrypted, marking statement for rewrite",
                                               res_target->name);

                            /* Create column reference and add to encrypted columns list */
                            col_ref = tecc_column_ref_data_create();
                            if (col_ref != NULL) {
                                tecc_column_ref_data_init(col_ref,
                                                         NULL,
                                                         range_var->schemaname,
                                                         range_var->relname,
                                                         res_target->name);
                                statement_data->encrypted_columns = lappend(statement_data->encrypted_columns, col_ref);
                                tecc_processor_debug(TECC_LOG_DEBUG1,
                                                   "Added encrypted column %s.%s to encrypted_columns list",
                                                   range_var->relname ? range_var->relname : "(null)",
                                                   res_target->name);
                            }
                        }
                    } else {
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                           "INSERT target column is NULL");
                    }
                }
                /* 检查是否是字符串节点（直接列名） */
                else if (target_node->type == FE_T_String) {
                    column_name = (char *)target_node;
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                       "INSERT target column (String): %s", column_name);
                    values_column_count++;

                    /* Check if this column is encrypted using cached column list */
                    is_encrypted = stmt_is_column_in_encrypted_list(column_name, table_encrypted_columns, table_encrypted_column_count);
                    if (is_encrypted) {
                        statement_data->needs_rewrite = 1;
                        encrypted_column_count++;
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                           "Column %s is encrypted, marking statement for rewrite",
                                           column_name);

                        /* Create column reference and add to encrypted columns list */
                        col_ref = tecc_column_ref_data_create();
                        if (col_ref != NULL) {
                            tecc_column_ref_data_init(col_ref,
                                                     NULL,
                                                     range_var->schemaname,
                                                     range_var->relname,
                                                     column_name);
                            statement_data->encrypted_columns = lappend(statement_data->encrypted_columns, col_ref);
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                               "Added encrypted column %s.%s to encrypted_columns list",
                                               range_var->relname ? range_var->relname : "(null)",
                                               column_name);
                        }
                    }
                }
                /* 特殊处理：如果type字段看起来像字符串内容（检查前4个字节是否是ASCII字符） */
                else if (target_node->type > 0x20 && target_node->type < 0x7F000000) {
                    /* 这可能是一个字符串指针被错误地解释为Node */
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                       "INSERT target appears to be string: type=0x%x, as string: '%c%c%c%c'",
                                       target_node->type,
                                       (target_node->type >> 0) & 0xFF,
                                       (target_node->type >> 8) & 0xFF,
                                       (target_node->type >> 16) & 0xFF,
                                       (target_node->type >> 24) & 0xFF);

                    /* 尝试从ListCell数据中直接获取字符串 */
                    column_name = (char *)lc->data.ptr_value;
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                       "INSERT target column from ListCell data: %s", column_name);
                    values_column_count++;

                    /* Check if this column is encrypted using cached column list */
                    is_encrypted = stmt_is_column_in_encrypted_list(column_name, table_encrypted_columns, table_encrypted_column_count);
                    if (is_encrypted) {
                        statement_data->needs_rewrite = 1;
                        encrypted_column_count++;
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                           "Column %s is encrypted, marking statement for rewrite",
                                           column_name);
                    }
                }
                else {
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                       "INSERT target is not ResTarget or String, type=%d (0x%x)",
                                       target_node->type, target_node->type);
                    continue;  /* 跳过非ResTarget节点 */
                }
            }
        }
        else
        {
            /* No explicit column list - use positional mapping from table metadata */
            if (table_encrypted_columns != NULL && table_encrypted_column_count > 0)
            {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "INSERT has no explicit column list, using table metadata for %d encrypted columns",
                                   table_encrypted_column_count);

                for (int i = 0; i < table_encrypted_column_count; i++)
                {
                    TeccColumnCacheItem *col_cache = table_encrypted_columns[i];
                    if (col_cache != NULL && col_cache->column_name != NULL)
                    {
                        TeccColumnRefData *col_ref = tecc_column_ref_data_create();
                        if (col_ref != NULL)
                        {
                            tecc_column_ref_data_init(col_ref,
                                                     NULL,
                                                     NULL,
                                                     insert_stmt->relation->relname,
                                                     col_cache->column_name);
                            statement_data->encrypted_columns = lappend(statement_data->encrypted_columns, col_ref);
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                               "Added encrypted column %s to encrypted_columns list (positional)",
                                               col_cache->column_name);
                        }
                    }
                }

                statement_data->needs_rewrite = 1;
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Table has encrypted columns, marking for rewrite");
            }
        }
    }

    /* Process VALUES clause or SELECT subquery */
    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "INSERT statement selectStmt pointer: %p", insert_stmt->selectStmt);

    if (insert_stmt->selectStmt != NULL)
    {
        Node *select_node = insert_stmt->selectStmt;
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "INSERT select node type: %d", nodeTag(select_node));

        if (nodeTag(select_node) == FE_T_SelectStmt)
        {
            SelectStmt *select_stmt = (SelectStmt *) select_node;

            /* Check if this is a VALUES clause or SELECT statement */
            /* In PostgreSQL, VALUES clause is represented as a SelectStmt with valuesLists != NULL */
            if (select_stmt->defaultValues)
            {
                /* This is a DEFAULT VALUES clause — no values to encrypt, send SQL as-is. */
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing INSERT DEFAULT VALUES clause (no values to process)");
                /* ReqID:TECC4.0 - Bug 7972: DEFAULT VALUES needs no rewrite.
                 * Set rewritten_query to original query to pass the safety check. */
                if (statement_data->rewritten_query != NULL)
                    pg_free(statement_data->rewritten_query);
                if (statement_data->query != NULL)
                    statement_data->rewritten_query = pg_strdup(statement_data->query);
                statement_data->needs_rewrite = false;
                statement_data->where_needs_rewrite = false;
                /* Skip values processing — no values to encrypt. */
                return TECC_STMT_PROCESSOR_SUCCESS;
            }
            else if (select_stmt->valuesLists != NULL)
            {
                /* This is a VALUES clause */
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing INSERT VALUES clause");

                /* First, process TypeCast expressions with encrypted options
                 * (e.g., 'test'::varchar_enc encrypted with (dek = dek_t1)) */
                {
                    TeccStmtProcessorResult typecast_result;
                    typecast_result = process_insert_select_typecast(
                        select_stmt, statement_data);
                    if (typecast_result != TECC_STMT_PROCESSOR_SUCCESS)
                    {
                        tecc_processor_debug(TECC_LOG_ERROR,
                                           "INSERT...VALUES TypeCast processing failed: %d",
                                           typecast_result);
                        return TECC_STMT_PROCESSOR_ERROR;
                    }
                }

                /* Begin - ReqID:TECC4.0 - Bug 7904: Check DEK consistency for VALUES TypeCasts */
                if (select_stmt->valuesLists != NIL
                    && table_encrypted_columns != NULL
                    && table_encrypted_column_count > 0)
                {
                    List *first_row;
                    ListCell *col_lc;
                    int col_idx;

                    first_row = (List *) list_nth(select_stmt->valuesLists, 0);
                    col_idx = 0;

                    foreach (col_lc, first_row)
                    {
                        Node *val;
                        TypeCast *tc;
                        const char *ins_col_name;
                        int tci;
                        TeccColumnCacheItem *col_item;
                        ListCell *opt_lc;
                        const char *type_dek;

                        val = (Node *) col_lc->data.ptr_value;
                        tc = NULL;
                        ins_col_name = NULL;
                        col_item = NULL;
                        type_dek = NULL;

                        if (val == NULL || val->type != FE_T_TypeCast)
                        {
                            col_idx++;
                            continue;
                        }
                        tc = (TypeCast *) val;

                        if (tc->typeName == NULL
                            || tc->typeName->encryptedOpts == NIL)
                        {
                            col_idx++;
                            continue;
                        }

                        /* Map position to INSERT column */
                        if (insert_stmt->cols != NIL
                            && col_idx < list_length(insert_stmt->cols))
                        {
                            Node *col_node = (Node *) list_nth(
                                insert_stmt->cols, col_idx);
                            if (col_node->type == FE_T_ResTarget)
                                ins_col_name = ((ResTarget *) col_node)->name;
                            else if (col_node->type == FE_T_String)
                                ins_col_name = (const char *) col_node;
                        }
                        else
                        {
                            for (tci = 0; tci < table_encrypted_column_count; tci++)
                            {
                                if (table_encrypted_columns[tci] != NULL
                                    && table_encrypted_columns[tci]->column_attnum
                                       == col_idx + 1)
                                {
                                    ins_col_name =
                                        table_encrypted_columns[tci]->column_name;
                                    col_item = table_encrypted_columns[tci];
                                    break;
                                }
                            }
                        }

                        if (ins_col_name == NULL)
                        {
                            col_idx++;
                            continue;
                        }

                        /* Find column metadata if not set above */
                        if (col_item == NULL)
                        {
                            for (tci = 0; tci < table_encrypted_column_count; tci++)
                            {
                                if (table_encrypted_columns[tci] != NULL
                                    && table_encrypted_columns[tci]->column_name != NULL
                                    && strcmp(table_encrypted_columns[tci]->column_name,
                                              ins_col_name) == 0)
                                {
                                    col_item = table_encrypted_columns[tci];
                                    break;
                                }
                            }
                        }

                        if (col_item == NULL)
                        {
                            col_idx++;
                            continue;
                        }

                        /* Extract DEK name from TypeCast */
                        foreach (opt_lc, tc->typeName->encryptedOpts)
                        {
                            DefElem *elem = (DefElem *) opt_lc->data.ptr_value;
                            if (elem != NULL && elem->defname != NULL
                                && pg_strcasecmp(elem->defname, "dek") == 0)
                            {
                                if (elem->arg != NULL
                                    && nodeTag(elem->arg) == FE_T_String)
                                    type_dek = strVal((const Value *) elem->arg);
                                break;
                            }
                        }

                        if (type_dek != NULL
                            && col_item->dek_name != NULL
                            && pg_strcasecmp(type_dek, col_item->dek_name) != 0)
                        {
                            tecc_processor_set_error(statement_data,
                                TECC_PROCESSOR_ERROR,
                                "cannot assign encrypted value with DEK %s "
                                "to column %s which uses DEK %s",
                                type_dek, ins_col_name, col_item->dek_name);
                            return TECC_STMT_PROCESSOR_ERROR;
                        }

                        col_idx++;
                    }
                }
                /* End - ReqID:TECC4.0 */

                /* If TypeCast processor rewrote the query, skip the values processor.
                 * Otherwise delegate to the values processor for automatic encryption. */
                if (statement_data->rewritten_query == NULL)
                {
                    /* Delegate to values processor */
                    values_result = tecc_values_processor_process(statement_data);

                    if (values_result != TECC_VALUES_PROCESSOR_SUCCESS)
                    {
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                           "VALUES clause processing failed: %d", values_result);
                        return TECC_STMT_PROCESSOR_ERROR;
                    }

                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                       "VALUES clause processing completed successfully");
                }
                else
                {
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                       "TypeCast expressions handled, skipped values processor");
                }
            }
            else
            {
                /* This is a SELECT statement */
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing INSERT...SELECT statement");

                /* Process the SELECT statement recursively */
                select_result = tecc_stmt_processor_process_select(
                    select_stmt, statement_data);

                if (select_result != TECC_STMT_PROCESSOR_SUCCESS)
                {
                    tecc_processor_debug(TECC_LOG_ERROR,
                                       "INSERT...SELECT processing failed: %d", select_result);
                    return TECC_STMT_PROCESSOR_ERROR;
                }

                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "INSERT...SELECT processing completed successfully");

                /* ReqID:TECC4.0 - Handle bare constants in SELECT target list that map
                 * to encrypted INSERT columns. When a SELECT target at a position that
                 * corresponds to an encrypted INSERT column is a bare Const (no explicit
                 * TypeCast with encryptedOpts), encrypt it and add to raw_values_list
                 * for subsequent SQL text rewrite via tecc_processor_rewrite_query(). */
                if (statement_data->needs_rewrite
                    && select_stmt->targetList != NIL
                    && table_encrypted_columns != NULL
                    && table_encrypted_column_count > 0)
                {
                    int num_targets = list_length(select_stmt->targetList);
                    int tp;

                    for (tp = 0; tp < num_targets; tp++)
                    {
                        Node *target_node;
                        ResTarget *res_target;
                        Node *val;
                        Const *cnst;
                        const char *ins_col_name;
                        int tci;
                        TeccColumnCacheItem *col_item;

                        ins_col_name = NULL;
                        col_item = NULL;

                        /* Get INSERT column name at this position */
                        if (insert_stmt->cols != NIL
                            && tp < list_length(insert_stmt->cols))
                        {
                            Node *col_node = (Node *) list_nth(
                                insert_stmt->cols, tp);
                            if (col_node->type == FE_T_ResTarget)
                                ins_col_name = ((ResTarget *) col_node)->name;
                            else if (col_node->type == FE_T_String)
                                ins_col_name = (const char *) col_node;
                        }
                        else
                        {
                            /* No explicit column list — use column_attnum mapping */
                            for (tci = 0; tci < table_encrypted_column_count; tci++)
                            {
                                if (table_encrypted_columns[tci] != NULL
                                    && table_encrypted_columns[tci]->column_attnum
                                       == tp + 1)
                                {
                                    ins_col_name =
                                        table_encrypted_columns[tci]->column_name;
                                    col_item = table_encrypted_columns[tci];
                                    break;
                                }
                            }
                        }
                        if (ins_col_name == NULL)
                            continue;

                        /* Find encrypted column metadata if not set above */
                        if (col_item == NULL)
                        {
                            for (tci = 0; tci < table_encrypted_column_count; tci++)
                            {
                                if (table_encrypted_columns[tci] != NULL
                                    && table_encrypted_columns[tci]->column_name != NULL
                                    && strcmp(table_encrypted_columns[tci]->column_name,
                                              ins_col_name) == 0)
                                {
                                    col_item = table_encrypted_columns[tci];
                                    break;
                                }
                            }
                        }
                        if (col_item == NULL)
                            continue;   /* Not an encrypted column */

                        /* Check SELECT target at this position */
                        target_node = (Node *) list_nth(
                            select_stmt->targetList, tp);
                        if (target_node == NULL
                            || target_node->type != FE_T_ResTarget)
                            continue;
                        res_target = (ResTarget *) target_node;
                        val = res_target->val;
                        if (val == NULL || val->type != FE_T_Const)
                            continue;   /* Not a bare constant */
                        cnst = (Const *) val;

                        if (cnst->constisnull)
                            continue;

                        /* Extract plaintext from Const */
                        {
                            char plaintext[256];
                            bool has_value;

                            has_value = false;
                            /* ReqID:TECC4.0 - Extract plaintext by consttype.
                             * INT2OID(21)/INT4OID(23)/INT8OID(20): constbyval,
                             *   extract from ival.
                             * FLOAT4OID(700)/FLOAT8OID(701): constbyval,
                             *   extract from dval.
                             * VARCHAR/BPCHAR/TEXT: !constbyval, sval pointer. */
                            if (cnst->consttype == 20 ||  /* INT8OID */
                                cnst->consttype == 21 ||  /* INT2OID */
                                cnst->consttype == 23)    /* INT4OID */
                            {
                                snprintf(plaintext, sizeof(plaintext), "%lld",
                                         (long long) cnst->constvalue.ival);
                                has_value = true;
                            }
                            else if (cnst->consttype == 700 ||  /* FLOAT4OID */
                                     cnst->consttype == 701)    /* FLOAT8OID */
                            {
                                snprintf(plaintext, sizeof(plaintext), "%g",
                                         cnst->constvalue.dval);
                                has_value = true;
                            }
                            else if (cnst->constvalue.sval != NULL)
                            {
                                snprintf(plaintext, sizeof(plaintext), "%s",
                                         cnst->constvalue.sval);
                                has_value = true;
                            }

                            if (!has_value)
                                continue;

                            /* Encrypt using column metadata */
                            {
                                TeccUnifiedEncryptResult *encrypt_result;
                                bool supports_det;
                                bool supports_ope;
                                bool supports_search;
                                TeccRawValue *rv;
                                const char *type_name;

                                supports_det =
                                    (col_item->encryption_type
                                     & TECC_ENC_TYPE_DET) != 0;
                                supports_ope =
                                    (col_item->encryption_type
                                     & TECC_ENC_TYPE_OPE) != 0;
                                supports_search =
                                    (col_item->encryption_type
                                     & TECC_ENC_TYPE_SEARCH) != 0;

                                encrypt_result =
                                    tecc_unified_encrypt_string_with_options(
                                        statement_data->conn,
                                        plaintext,
                                        col_item->table_name,
                                        col_item->column_name,
                                        NULL,  /* kek_name */
                                        col_item->dek_name,
                                        col_item->original_type_oid,
                                        col_item->original_type_mod,
                                        supports_det,
                                        supports_ope,
                                        supports_search,
                                        NULL);

                                if (encrypt_result == NULL
                                    || encrypt_result->error_code
                                       != TECC_CRYPTO_SUCCESS)
                                {
                                    tecc_processor_debug(TECC_LOG_WARNING,
                                        "Failed to encrypt bare constant "
                                        "for column %s: %s",
                                        col_item->column_name,
                                        encrypt_result
                                        ? encrypt_result->error_msg
                                        : "unknown error");
                                    if (encrypt_result)
                                        tecc_unified_free_encrypt_result(
                                            encrypt_result);
                                    continue;
                                }

                                if (cnst->location < 0
                                    || cnst->data_size == 0)
                                {
                                    tecc_unified_free_encrypt_result(
                                        encrypt_result);
                                    continue;
                                }

                                /* Create raw value, add to raw_values_list */
                                if (statement_data->raw_values_list == NULL)
                                {
                                    statement_data->raw_values_list =
                                        tecc_raw_values_list_create(
                                            num_targets);
                                }

                                rv = tecc_raw_value_create_from_string(
                                    plaintext, strlen(plaintext));
                                if (rv != NULL)
                                {
                                    rv->location = (size_t) cnst->location;
                                    rv->data_size = (size_t) cnst->data_size;

                                    /* Set type name for ::typename suffix */
                                    type_name =
                                        get_enc_type_name_from_base_oid(
                                            col_item->original_type_oid);
                                    if (type_name != NULL)
                                        rv->column_type = pg_strdup(type_name);

                                    rv->processed.encrypted_value =
                                        pg_strdup(
                                            encrypt_result->hex_string);
                                    rv->state =
                                        TECC_RAW_VALUE_STATE_ENCRYPTED;

                                    tecc_raw_values_list_add(
                                        statement_data->raw_values_list, rv);

                                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                        "Encrypted bare constant "
                                        "for column %s at position %d",
                                        col_item->column_name,
                                        (int) rv->location);
                                }

                                tecc_unified_free_encrypt_result(
                                    encrypt_result);
                            }
                        }
                    }
                }

                /* Begin - ReqID:TECC4.0 - Bug 7904: Check DEK consistency for TypeCast targets */
                {
                    int num_tp;
                    int tp2;

                    num_tp = list_length(select_stmt->targetList);

                    for (tp2 = 0; tp2 < num_tp; tp2++)
                    {
                        Node *target_node;
                        ResTarget *res_target;
                        Node *val;
                        const char *ins_col_name;
                        int tci;
                        TeccColumnCacheItem *col_item;
                        TypeCast *tc;
                        ListCell *opt_lc;
                        const char *type_dek;

                        ins_col_name = NULL;
                        col_item = NULL;
                        tc = NULL;
                        type_dek = NULL;

                        /* Get INSERT column name at this position */
                        if (insert_stmt->cols != NIL
                            && tp2 < list_length(insert_stmt->cols))
                        {
                            Node *col_node = (Node *) list_nth(
                                insert_stmt->cols, tp2);
                            if (col_node->type == FE_T_ResTarget)
                                ins_col_name = ((ResTarget *) col_node)->name;
                            else if (col_node->type == FE_T_String)
                                ins_col_name = (const char *) col_node;
                        }
                        else
                        {
                            for (tci = 0; tci < table_encrypted_column_count; tci++)
                            {
                                if (table_encrypted_columns[tci] != NULL
                                    && table_encrypted_columns[tci]->column_attnum
                                       == tp2 + 1)
                                {
                                    ins_col_name =
                                        table_encrypted_columns[tci]->column_name;
                                    col_item = table_encrypted_columns[tci];
                                    break;
                                }
                            }
                        }
                        if (ins_col_name == NULL)
                            continue;

                        /* Find encrypted column metadata if not set above */
                        if (col_item == NULL)
                        {
                            for (tci = 0; tci < table_encrypted_column_count; tci++)
                            {
                                if (table_encrypted_columns[tci] != NULL
                                    && table_encrypted_columns[tci]->column_name != NULL
                                    && strcmp(table_encrypted_columns[tci]->column_name,
                                              ins_col_name) == 0)
                                {
                                    col_item = table_encrypted_columns[tci];
                                    break;
                                }
                            }
                        }
                        if (col_item == NULL)
                            continue;

                        /* Check SELECT target at this position */
                        target_node = (Node *) list_nth(
                            select_stmt->targetList, tp2);
                        if (target_node == NULL
                            || target_node->type != FE_T_ResTarget)
                            continue;
                        res_target = (ResTarget *) target_node;
                        val = res_target->val;
                        if (val == NULL || val->type != FE_T_TypeCast)
                            continue;
                        tc = (TypeCast *) val;

                        /* Only check TypeCasts with encrypted options */
                        if (tc->typeName == NULL
                            || tc->typeName->encryptedOpts == NIL)
                            continue;

                        /* Extract DEK name from encrypted options */
                        foreach (opt_lc, tc->typeName->encryptedOpts)
                        {
                            DefElem *elem = (DefElem *) opt_lc->data.ptr_value;
                            if (elem != NULL && elem->defname != NULL
                                && pg_strcasecmp(elem->defname, "dek") == 0)
                            {
                                if (elem->arg != NULL
                                    && nodeTag(elem->arg) == FE_T_String)
                                    type_dek = strVal((const Value *) elem->arg);
                                break;
                            }
                        }

                        if (type_dek != NULL
                            && col_item->dek_name != NULL
                            && pg_strcasecmp(type_dek, col_item->dek_name) != 0)
                        {
                            tecc_processor_set_error(statement_data,
                                TECC_PROCESSOR_ERROR,
                                "cannot assign encrypted value with DEK %s "
                                "to column %s which uses DEK %s",
                                type_dek, ins_col_name, col_item->dek_name);
                            return TECC_STMT_PROCESSOR_ERROR;
                        }
                    }
                }
                /* End - ReqID:TECC4.0 */

                /* ReqID:TECC4.0 - Handle VALUES inside RangeSubselect in INSERT...SELECT.
                 * Walk the SELECT's FROM clause for RangeSubselect nodes containing VALUES.
                 * For each, map VALUES columns to encrypted INSERT target columns and encrypt. */
                if (select_stmt->fromClause != NIL
                    && statement_data->encrypted_columns != NIL)
                {
                    int num_target_positions = list_length(select_stmt->targetList);
                    bool *target_is_encrypted;
                    char **target_enc_col_name;
                    int tp;

                    target_is_encrypted = (bool *) palloc(num_target_positions * sizeof(bool));
                    target_enc_col_name = (char **) palloc(num_target_positions * sizeof(char *));
                    memset(target_is_encrypted, 0, num_target_positions * sizeof(bool));
                    memset(target_enc_col_name, 0, num_target_positions * sizeof(char *));

                    /* Build INSERT position → encrypted column name mapping */
                    for (tp = 0; tp < num_target_positions; tp++)
                    {
                        const char *ins_col_name = NULL;
                        bool is_enc = false;

                        if (insert_stmt->cols != NIL && tp < list_length(insert_stmt->cols))
                        {
                            ResTarget *ins_rt = (ResTarget *) list_nth(insert_stmt->cols, tp);
                            ins_col_name = ins_rt->name;
                        }
                        else if (table_encrypted_column_count > 0 && table_encrypted_columns != NULL)
                        {
                            int tci;
                            for (tci = 0; tci < table_encrypted_column_count; tci++)
                            {
                                TeccColumnCacheItem *ci = table_encrypted_columns[tci];
                                if (ci != NULL && ci->column_attnum == tp + 1)
                                {
                                    ins_col_name = ci->column_name;
                                    break;
                                }
                            }
                        }

                        if (ins_col_name != NULL)
                        {
                            ListCell *eclc;
                            foreach(eclc, statement_data->encrypted_columns)
                            {
                                TeccColumnRefData *cd = (TeccColumnRefData *) lfirst(eclc);
                                if (strcmp(ins_col_name, cd->column_name) == 0)
                                {
                                    is_enc = true;
                                    break;
                                }
                            }
                        }
                        target_is_encrypted[tp] = is_enc;
                        if (is_enc)
                            target_enc_col_name[tp] = (char *) ins_col_name;
                    }

                    /* Walk FROM clause for RangeSubselect with VALUES */
                    {
                        ListCell *frm_lc;
                        foreach(frm_lc, select_stmt->fromClause)
                        {
                            Node *from_item = (Node *) lfirst(frm_lc);
                            if (nodeTag(from_item) != FE_T_RangeSubselect)
                                continue;
                            {
                                RangeSubselect *rss = (RangeSubselect *) from_item;
                                if (rss->subquery == NULL
                                    || nodeTag(rss->subquery) != FE_T_SelectStmt)
                                    continue;
                                {
                                    SelectStmt *subselect = (SelectStmt *) rss->subquery;
                                    if (subselect->valuesLists == NIL || rss->alias == NULL
                                        || rss->alias->colnames == NIL)
                                        continue;
                                    {
                                        Alias *alias = rss->alias;
                                        List *rss_colnames = alias->colnames;
                                        int num_rss_cols = list_length(rss_colnames);
                                        List *target_columns = NIL;
                                        int vcol;

                                        for (vcol = 0; vcol < num_rss_cols; vcol++)
                                        {
                                            char *rss_col_name = (char *) list_nth(rss_colnames, vcol);
                                            bool found_encrypted = false;
                                            char *insert_col_name = NULL;
                                            ListCell *slc;
                                            int sel_pos = 0;

                                            if (rss_col_name == NULL) continue;

                                            /* Search SELECT targetList for ColumnRef
                                             * matching this RangeSubselect alias + column */
                                            foreach(slc, select_stmt->targetList)
                                            {
                                                ResTarget *sel_rt = (ResTarget *) lfirst(slc);
                                                if (sel_rt->val != NULL
                                                    && nodeTag(sel_rt->val) == FE_T_ColumnRef)
                                                {
                                                    ColumnRef *cref = (ColumnRef *) sel_rt->val;
                                                    if (list_length(cref->fields) == 2)
                                                    {
                                                        Node *f1 = (Node *) list_nth(cref->fields, 0);
                                                        Node *f2 = (Node *) list_nth(cref->fields, 1);
                                                        const char *f1_str = NULL;
                                                        const char *f2_str = NULL;

                                                        if (nodeTag(f1) == FE_T_Const)
                                                            f1_str = ((struct Const *)f1)->constvalue.sval;
                                                        if (nodeTag(f2) == FE_T_Const)
                                                            f2_str = ((struct Const *)f2)->constvalue.sval;

                                                        if (f1_str != NULL && f2_str != NULL
                                                            && alias->aliasname != NULL
                                                            && strcmp(f1_str, alias->aliasname) == 0
                                                            && strcmp(f2_str, rss_col_name) == 0)
                                                        {
                                                            if (sel_pos < num_target_positions
                                                                && target_is_encrypted[sel_pos])
                                                            {
                                                                insert_col_name = target_enc_col_name[sel_pos];
                                                                found_encrypted = true;
                                                            }
                                                            break;
                                                        }
                                                    }
                                                }
                                                sel_pos++;
                                            }

                                            {
                                                ResTarget *rt = makeResTarget(
                                                    found_encrypted ? insert_col_name : NULL,
                                                    NULL, 0);
                                                target_columns = lappend(target_columns, (Node *) rt);
                                            }
                                        }

                                        /* Process VALUES encryption */
                                        {
                                            TeccValuesProcessorResult vp_result;
                                            vp_result = tecc_values_processor_process_insert_values(
                                                subselect->valuesLists,
                                                target_columns,
                                                insert_stmt->relation ? insert_stmt->relation->schemaname : NULL,
                                                insert_stmt->relation ? insert_stmt->relation->relname : NULL,
                                                statement_data);

                                            if (vp_result == TECC_VALUES_PROCESSOR_SUCCESS
                                                && statement_data->raw_values_list != NULL)
                                            {
                                                /* Fix column_type for subquery VALUES:
                                                 * the explicit target_columns path sets it NULL,
                                                 * but subquery VALUES need ::type_name cast. */
                                                TeccRawValuesList *rvlist = statement_data->raw_values_list;
                                                int rvcount = tecc_raw_values_list_count(rvlist);
                                                int rvi;
                                                for (rvi = 0; rvi < rvcount; rvi++)
                                                {
                                                    TeccRawValue *rv = tecc_raw_values_list_get(rvlist, rvi);
                                                    if (rv != NULL && rv->needs_encryption
                                                        && (rv->column_type == NULL || rv->column_type[0] == '\0')
                                                        && rv->column_name != NULL)
                                                    {
                                                        int tci;
                                                        for (tci = 0; tci < table_encrypted_column_count; tci++)
                                                        {
                                                            TeccColumnCacheItem *ci = table_encrypted_columns[tci];
                                                            if (ci != NULL && ci->column_name != NULL
                                                                && strcmp(ci->column_name, rv->column_name) == 0)
                                                            {
                                                                const char *enc_type = get_enc_type_name_from_base_oid(
                                                                    ci->original_type_oid);
                                                                if (enc_type != NULL)
                                                                    tecc_raw_value_set_column_info(rv, NULL, enc_type, InvalidOid);
                                                                break;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }

                                        /* Free target_columns */
                                        {
                                            ListCell *tc_lc;
                                            foreach(tc_lc, target_columns)
                                            {
                                                ResTarget *rt = (ResTarget *) lfirst(tc_lc);
                                                if (rt != NULL) pfree(rt);
                                            }
                                            list_free(target_columns);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    pfree(target_is_encrypted);
                    pfree(target_enc_col_name);
                }

                /* Process TypeCast expressions with encrypted options in the SELECT list */
                {
                    TeccStmtProcessorResult typecast_result;
                    typecast_result = process_insert_select_typecast(
                        select_stmt, statement_data);
                    if (typecast_result != TECC_STMT_PROCESSOR_SUCCESS)
                    {
                        tecc_processor_debug(TECC_LOG_ERROR,
                                           "INSERT...SELECT TypeCast processing failed: %d",
                                           typecast_result);
                        return TECC_STMT_PROCESSOR_ERROR;
                    }
                }
            }
        }
        else
        {
            /* Handle other node types that might represent VALUES clause */
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Processing INSERT VALUES clause (node type: %d)",
                               nodeTag(select_node));

            /* Delegate to values processor */
            values_result = tecc_values_processor_process(statement_data);

            if (values_result != TECC_VALUES_PROCESSOR_SUCCESS)
            {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "VALUES clause processing failed: %d", values_result);
                return TECC_STMT_PROCESSOR_ERROR;
            }

            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "VALUES clause processing completed successfully");
        }
    }
    else
    {
        tecc_processor_debug(TECC_LOG_WARNING,
                           "INSERT statement has no VALUES or SELECT clause");
    }

    /* ReqID:TECC4.0 - Process ON CONFLICT DO UPDATE SET clause.
     * Traverse SET assignments in ON CONFLICT DO UPDATE to identify and
     * encrypt values for encrypted columns.
     * Must run BEFORE table_encrypted_columns is freed and AFTER
     * raw_values_list is populated by INSERT values processing. */
    if (insert_stmt->onConflictClause != NIL)
    {
        ListCell *occ_lc;
        foreach(occ_lc, insert_stmt->onConflictClause)
        {
            OnConflictClause *occ = (OnConflictClause *) lfirst(occ_lc);
            if (occ != NULL && occ->action == ONCONFLICT_UPDATE
                && occ->targetList != NIL)
            {
                ListCell *set_lc;
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing ON CONFLICT DO UPDATE SET clause");
                for (set_lc = occ->targetList->head; set_lc != NULL; set_lc = set_lc->next)
                {
                    Node *set_node = (Node *) set_lc->data.ptr_value;
                    if (set_node == NULL || nodeTag(set_node) != FE_T_ResTarget)
                        continue;
                    {
                        ResTarget *set_target = (ResTarget *) set_node;
                        bool is_enc = false;
                        if (set_target->name == NULL)
                            continue;

                        /* Check if column is encrypted */
                        if (table_encrypted_columns != NULL)
                            is_enc = stmt_is_column_in_encrypted_list(
                                set_target->name, table_encrypted_columns,
                                table_encrypted_column_count);
                        else if (insert_stmt->relation != NULL)
                            is_enc = stmt_check_column_encrypted(
                                statement_data,
                                insert_stmt->relation->relname,
                                set_target->name);

                        if (is_enc)
                        {
                            statement_data->needs_rewrite = 1;
                            col_ref = tecc_column_ref_data_create();
                            if (col_ref != NULL && insert_stmt->relation != NULL)
                            {
                                tecc_column_ref_data_init(col_ref,
                                                         NULL,
                                                         insert_stmt->relation->schemaname,
                                                         insert_stmt->relation->relname,
                                                         set_target->name);
                                statement_data->encrypted_columns = lappend(
                                    statement_data->encrypted_columns, col_ref);
                            }
                            encrypted_column_count++;
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                               "ON CONFLICT DO UPDATE SET column %s is encrypted",
                                               set_target->name);
                        }
                    }
                }

                /* ReqID:TECC4.0 - Encrypt ON CONFLICT DO UPDATE SET values.
                 * Build a separate raw_values_list for ON CONFLICT SET entries,
                 * process and encrypt, then merge into main raw_values_list.
                 * This avoids re-processing already-encrypted INSERT values. */
                if (statement_data->needs_rewrite)
                {
                    ListCell *enc_lc;
                    int occ_added = 0;
                    TeccRawValuesList *occ_list = NULL;

                    for (enc_lc = occ->targetList->head; enc_lc != NULL; enc_lc = enc_lc->next)
                    {
                        Node *n = (Node *) enc_lc->data.ptr_value;
                        ResTarget *rt;
                        bool is_enc;
                        TeccRawValue *rv;
                        Const *cv;

                        if (n == NULL || nodeTag(n) != FE_T_ResTarget)
                            continue;
                        rt = (ResTarget *) n;
                        if (rt->name == NULL || rt->val == NULL)
                            continue;

                        /* Check if this SET column is encrypted */
                        is_enc = false;
                        if (table_encrypted_columns != NULL)
                            is_enc = stmt_is_column_in_encrypted_list(
                                rt->name, table_encrypted_columns,
                                table_encrypted_column_count);
                        else if (insert_stmt->relation != NULL)
                            is_enc = stmt_check_column_encrypted(statement_data,
                                insert_stmt->relation->relname, rt->name);
                        if (!is_enc)
                            continue;

                        /* Only process Const values that scanner tagged */
                        if (nodeTag(rt->val) != FE_T_Const)
                        {
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "ON CONFLICT SET column %s value is not Const, skipping",
                                rt->name);
                            continue;
                        }
                        cv = (Const *) rt->val;

                        /* Lazy-init the ON CONFLICT raw_values_list */
                        if (occ_list == NULL)
                        {
                            occ_list = tecc_raw_values_list_create(
                                list_length(occ->targetList));
                            if (occ_list == NULL)
                                break;
                        }

                        rv = tecc_create_raw_value_from_node(rt->val);
                        if (rv == NULL)
                            continue;

                        rv->location = (size_t) cv->location;
                        rv->data_size = (size_t) cv->data_size;
                        rv->needs_encryption = true;
                        rv->table_name = insert_stmt->relation->relname
                            ? pg_strdup(insert_stmt->relation->relname) : NULL;
                        rv->column_name = pg_strdup(rt->name);

                        tecc_raw_values_list_add(occ_list, rv);
                        occ_added++;
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "ON CONFLICT SET: raw_value for %s.%s (loc=%zu, size=%zu)",
                            rv->table_name ? rv->table_name : "?",
                            rv->column_name ? rv->column_name : "?",
                            rv->location, rv->data_size);
                    }

                    if (occ_added > 0 && occ_list != NULL)
                    {
                        TeccValuesProcessorResult occ_proc_result;
                        int k;

                        occ_proc_result = tecc_process_values_list(
                            occ_list, statement_data);
                        if (occ_proc_result != TECC_VALUES_PROCESSOR_SUCCESS)
                        {
                            tecc_processor_debug(TECC_LOG_ERROR,
                                "ON CONFLICT SET value encryption failed: %d",
                                occ_proc_result);
                            tecc_raw_values_list_destroy(occ_list);
                        }
                        else
                        {
                            /* Merge encrypted ON CONFLICT values into main list */
                            int occ_count = tecc_raw_values_list_count(occ_list);
                            for (k = 0; k < occ_count; k++)
                            {
                                TeccRawValue *rv = tecc_raw_values_list_get(occ_list, k);
                                if (rv != NULL
                                    && statement_data->raw_values_list != NULL)
                                {
                                    tecc_raw_values_list_add(
                                        statement_data->raw_values_list, rv);
                                }
                            }
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "ON CONFLICT SET: encrypted+merged %d values",
                                occ_count);
                            /* Free list container (values now owned by main list) */
                            if (occ_list->values != NULL)
                                pg_free(occ_list->values);
                            pg_free(occ_list);
                        }
                    }
                }
            }
        }
    }

    /* Free encrypted columns list — must come after ON CONFLICT block
     * which needs table_encrypted_columns for column encryption check. */
    if (table_encrypted_columns != NULL && table_encrypted_column_count > 0)
    {
        pg_free(table_encrypted_columns);
    }

    

    /* ReqID:TECC4.0 - Detect INSERT...SELECT with WITH clause (CTE) into encrypted table.
     * When the INSERT source is a SELECT from a CTE, the CTE values are computed
     * server-side and cannot be encrypted by the client at SQL parse time.
     * Report a clear error so the user knows this feature combination is unsupported. */
    if (statement_data->needs_rewrite
        && statement_data->rewritten_query == NULL
        && insert_stmt != NULL && insert_stmt->selectStmt != NULL
        && nodeTag(insert_stmt->selectStmt) == FE_T_SelectStmt
        && insert_stmt->withClause != NIL
        && statement_data->encrypted_columns != NIL)
    {
        SelectStmt *sel_stmt = (SelectStmt *) insert_stmt->selectStmt;

        /* Only when the source is a SELECT (not direct VALUES) */
        if (sel_stmt->valuesLists == NIL && sel_stmt->fromClause != NIL)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "WITH clause (CTE) combined with INSERT...SELECT into encrypted table "
                               "is not supported in TECC mode. "
                               "The CTE values are computed on the server side and cannot be "
                               "pre-encrypted by the client. "
                               "Consider: 1) rewriting as INSERT...VALUES with pre-computed values, "
                               "or 2) avoiding the WITH clause for INSERTs into encrypted tables.");
            tecc_record_error(statement_data->conn
                              ? tecc_get_state(statement_data->conn) : NULL,
                            TECC_ERR_PROCESSOR,
                            "WITH ... INSERT ... SELECT into encrypted table is not supported "
                            "because CTE values are server-side computed and cannot be "
                            "client-side encrypted",
                            __func__, __LINE__, false);
            return TECC_STMT_PROCESSOR_ERROR;
        }
    }

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Process UPDATE statement
 */
TeccStmtProcessorResult
tecc_stmt_processor_process_update(const UpdateStmt *update_stmt,
                                  TeccStatementData *statement_data)
{
    ListCell *lc;
    int set_clause_count = 0;
    int encrypted_column_count = 0;
    TeccValuesProcessorResult values_result;
    TeccWhereClauseProcessorResult where_result;
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    Node *target_node = NULL;
    ResTarget *res_target = NULL;
    TeccColumnCacheItem **table_encrypted_columns = NULL;
    int table_encrypted_column_count = 0;
    TeccColumnRefData *col_ref = NULL;

    if (update_stmt == NULL || statement_data == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    /* ReqID:TECC4.0 - Set stmt_type */
    statement_data->stmt_type = TECC_STMT_TYPE_UPDATE;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing UPDATE statement");

    /* Process target table */
    if (update_stmt->relation != NULL)
    {
        RangeVar *range_var = update_stmt->relation;
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "UPDATE target table: %s.%s",
                           range_var->schemaname ? range_var->schemaname : "(null)",
                           range_var->relname ? range_var->relname : "(null)");

        /* Load table metadata and identify encrypted columns */
        stmt_load_table_metadata(range_var, statement_data, &table_encrypted_columns, &table_encrypted_column_count);
    }

    /* Process SET clause */
    if (update_stmt->targetList != NIL) {
        for (lc = update_stmt->targetList->head; lc != NULL; lc = lc->next)
        {
            /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
            bool is_encrypted = false;

            target_node = (Node *) lc->data.ptr_value;

            /* Check if this is a Const node */
            if (target_node == NULL || target_node->type == FE_T_Const) {
                /* It's a constant value or NULL, not a column reference */
                continue;
            }

            /* Otherwise, assume it's a ResTarget */
            res_target = (ResTarget *) target_node;
            if (res_target->name == NULL)
                continue;

            set_clause_count++;
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "UPDATE SET clause: %s", res_target->name);

            /* Check if this column is encrypted using cached column list */
            if (table_encrypted_columns != NULL)
                is_encrypted = stmt_is_column_in_encrypted_list(res_target->name, table_encrypted_columns, table_encrypted_column_count);
            else
                is_encrypted = stmt_check_column_encrypted(statement_data,
                    update_stmt->relation ? update_stmt->relation->relname : NULL,
                    res_target->name);

            if (is_encrypted) {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "UPDATE SET column %s is encrypted",
                                   res_target->name);

                /* ReqID:TECC4.0 - For encrypted SET columns, only encrypt constant
                 * values client-side. ColumnRef/A_Expr values (e.g. SET c_f4 = c_i2)
                 * pass through unchanged — the server handles ciphertext copy
                 * between compatible encrypted numeric types directly. */
                if (res_target->val != NULL)
                {
                    int val_type = nodeTag(res_target->val);
                    bool is_const_val = (val_type == FE_T_Const
                                         || val_type == FE_T_Integer
                                         || val_type == FE_T_Float
                                         || val_type == FE_T_String
                                         || val_type == FE_T_RowExpr);
                    bool is_unary_minus = false;

                    if (val_type == FE_T_A_Expr)
                    {
                        A_Expr *aexpr = (A_Expr *)res_target->val;
                        is_unary_minus = (aexpr->kind == AEXPR_OP
                                          && aexpr->lexpr == NULL
                                          && aexpr->name != NIL);
                    }

                    if (is_const_val || is_unary_minus)
                    {
                        /* Constant value — client can encrypt */
                        statement_data->needs_rewrite = 1;
                        encrypted_column_count++;
                        col_ref = tecc_column_ref_data_create();
                        if (col_ref != NULL && update_stmt->relation != NULL)
                        {
                            tecc_column_ref_data_init(col_ref,
                                                     NULL,
                                                     update_stmt->relation->schemaname,
                                                     update_stmt->relation->relname,
                                                     res_target->name);
                            if (update_stmt->relation->alias != NULL
                                && update_stmt->relation->alias->aliasname != NULL)
                                tecc_column_ref_data_set_alias(col_ref,
                                    update_stmt->relation->alias->aliasname);
                            statement_data->encrypted_columns = lappend(
                                statement_data->encrypted_columns, col_ref);
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "Added encrypted column %s.%s to encrypted_columns list",
                                update_stmt->relation->relname,
                                res_target->name);
                        }
                    }
                    else
                    {
                        /* ColumnRef or expression — server handles ciphertext copy.
                         * Don't set needs_rewrite, don't add to encrypted_columns. */
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "UPDATE SET column %s: non-Const value, passing through for server-side copy",
                            res_target->name);
                    }
                }
            }

            /* Process the value expression if present */
            if (res_target->val != NULL)
            {
                /* Extract column references from the value expression */
                List *columns = NIL;
                stmt_extract_columns_from_expression(res_target->val, &columns, statement_data);

                /* Check if any referenced columns are encrypted */
                if (columns != NIL)
                {
                    ListCell *col_lc;
                    for (col_lc = columns->head; col_lc != NULL; col_lc = col_lc->next)
                    {
                        TeccColumnRefData *tecc_col = (TeccColumnRefData *) col_lc->data.ptr_value;
                        if (tecc_col != NULL && tecc_col->is_encrypted)
                        {
                            
                        }
                        tecc_column_ref_data_destroy(tecc_col);
                    }
                    list_free(columns);
                }
            }
        }

        /* ReqID:TECC4.0 - Expand RowExpr in multi-column SET.
         * When (col1, col2) = ROW(val1, val2), the grammar creates
         * a ResTarget per column with the same RowExpr val.
         * Expand: assign n'th RowExpr arg to n'th ResTarget. */
        {
            ListCell *mlc;
            for (mlc = update_stmt->targetList->head; mlc != NULL; mlc = mlc->next)
            {
                ResTarget *rt = (ResTarget *) mlc->data.ptr_value;
                Node *shared_val;
                if (rt == NULL || rt->val == NULL) continue;
                if (nodeTag(rt->val) != FE_T_RowExpr) continue;

                shared_val = rt->val;
                {
                    RowExpr *rx = (RowExpr *) shared_val;
                    int nargs = list_length(rx->args);
                    ListCell *mc2;
                    int idx;

                    /* Count consecutive targets sharing this RowExpr */
                    idx = 0;
                    for (mc2 = mlc; mc2 != NULL; mc2 = mc2->next)
                    {
                        ResTarget *rt2 = (ResTarget *) mc2->data.ptr_value;
                        if (rt2 == NULL || rt2->val != shared_val) break;
                        idx++;
                    }

                    if (idx > 1 && idx <= nargs)
                    {
                        /* Assign n'th arg to each target */
                        ListCell *assign_lc;
                        int ai;
                        for (ai = 0, assign_lc = mlc;
                             assign_lc != NULL && ai < nargs;
                             assign_lc = assign_lc->next, ai++)
                        {
                            ResTarget *rt2 = (ResTarget *) assign_lc->data.ptr_value;
                            if (rt2 == NULL || rt2->val != shared_val) break;
                            rt2->val = (Node *) list_nth(rx->args, ai);
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "Multi-col SET: column %s <- RowExpr arg[%d]",
                                rt2->name ? rt2->name : "?", ai);
                        }
                        /* Skip processed targets (minus 1 for loop incr) */
                        {
                            int skip;
                            for (skip = 0; skip < idx - 1 && mlc != NULL; skip++)
                                mlc = mlc->next;
                        }
                    }
                }
            }
        }

        /* Delegate to values processor for actual value processing */
        values_result = tecc_values_processor_process(statement_data);

        if (values_result != TECC_VALUES_PROCESSOR_SUCCESS)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "SET clause processing failed: %d", values_result);
            return TECC_STMT_PROCESSOR_ERROR;
        }

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "SET clause processing completed: %d columns", set_clause_count);
    }

    /* ReqID:TECC4.0 - Process UPDATE FROM clause for multi-table UPDATE.
     * Load metadata for FROM tables so encrypted columns in WHERE conditions
     * referencing FROM tables are properly recognized and encrypted. */
    if (update_stmt->fromClause != NIL)
    {
        ListCell *frm_lc;
        statement_data->multi_table_context = 1;
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Processing UPDATE FROM clause (%d items)",
                           list_length(update_stmt->fromClause));
        for (frm_lc = update_stmt->fromClause->head; frm_lc != NULL; frm_lc = frm_lc->next)
        {
            Node *from_item = (Node *) frm_lc->data.ptr_value;
            if (from_item == NULL)
                continue;

            if (nodeTag(from_item) == FE_T_RangeVar)
            {
                RangeVar *rv = (RangeVar *) from_item;
                TeccColumnCacheItem **from_cols = NULL;
                int from_col_count = 0;
                const char *alias_name = NULL;

                /* Extract alias if present */
                if (rv->alias != NULL && rv->alias->aliasname != NULL)
                    alias_name = rv->alias->aliasname;

                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "UPDATE FROM table: %s.%s (alias=%s)",
                                   rv->schemaname ? rv->schemaname : "public",
                                   rv->relname ? rv->relname : "(null)",
                                   alias_name ? alias_name : "(none)");

                stmt_load_table_metadata(rv, statement_data, &from_cols, &from_col_count);
                if (from_cols != NULL && from_col_count > 0)
                {
                    int ci;
                    for (ci = 0; ci < from_col_count; ci++)
                    {
                        if (from_cols[ci] == NULL) continue;
                        col_ref = tecc_column_ref_data_create();
                        if (col_ref != NULL)
                        {
                            tecc_column_ref_data_init(col_ref,
                                                     NULL,
                                                     rv->schemaname,
                                                     rv->relname,
                                                     from_cols[ci]->column_name);
                            if (alias_name != NULL)
                                tecc_column_ref_data_set_alias(col_ref, alias_name);
                            statement_data->encrypted_columns = lappend(
                                statement_data->encrypted_columns, col_ref);
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                               "Added FROM encrypted column: %s.%s (alias=%s)",
                                               rv->relname, from_cols[ci]->column_name,
                                               alias_name ? alias_name : rv->relname);
                        }
                    }
                    pg_free(from_cols);
                }
            }
            else if (nodeTag(from_item) == FE_T_JoinExpr)
            {
                /* ReqID:TECC4.0 - Recursively traverse JOIN arg tables */
                JoinExpr *j = (JoinExpr *) from_item;
                ListCell *jlc;
                List *join_items = NIL;
                join_items = lappend(join_items, j->larg);
                join_items = lappend(join_items, j->rarg);
                for (jlc = join_items->head; jlc != NULL; jlc = jlc->next)
                {
                    Node *ji = (Node *) jlc->data.ptr_value;
                    if (ji != NULL && nodeTag(ji) == FE_T_RangeVar)
                    {
                        RangeVar *jrv = (RangeVar *) ji;
                        TeccColumnCacheItem **jcols = NULL;
                        int jcol_count = 0;
                        const char *jalias = NULL;
                        if (jrv->alias != NULL && jrv->alias->aliasname != NULL)
                            jalias = jrv->alias->aliasname;
                        stmt_load_table_metadata(jrv, statement_data, &jcols, &jcol_count);
                        if (jcols != NULL && jcol_count > 0)
                        {
                            int ji2;
                            for (ji2 = 0; ji2 < jcol_count; ji2++)
                            {
                                if (jcols[ji2] == NULL) continue;
                                col_ref = tecc_column_ref_data_create();
                                if (col_ref != NULL)
                                {
                                    tecc_column_ref_data_init(col_ref, NULL,
                                                             jrv->schemaname,
                                                             jrv->relname,
                                                             jcols[ji2]->column_name);
                                    if (jalias != NULL)
                                        tecc_column_ref_data_set_alias(col_ref, jalias);
                                    statement_data->encrypted_columns = lappend(
                                        statement_data->encrypted_columns, col_ref);
                                }
                            }
                            pg_free(jcols);
                        }
                    }
                }
                list_free(join_items);
            }
            else if (nodeTag(from_item) == FE_T_RangeSubselect)
            {
                /* ReqID:TECC4.0 - RangeSubselect in FROM clause */
                RangeSubselect *rss = (RangeSubselect *) from_item;
                if (rss->subquery != NULL && nodeTag(rss->subquery) == FE_T_SelectStmt)
                {
                    SelectStmt *subq = (SelectStmt *) rss->subquery;
                    TeccStatementData sub_data;
                    memset(&sub_data, 0, sizeof(TeccStatementData));
                    sub_data.conn = statement_data->conn;
                    tecc_stmt_processor_process_select(subq, &sub_data);
                    if (sub_data.needs_rewrite)
                        statement_data->needs_rewrite = 1;
                }
            }
        }
    }

    /* Ensure all encrypted columns from table are in encrypted_columns list
     * BEFORE WHERE clause processing. WHERE columns (e.g. 'c_int2') may not
     * be in SET clause and need to be in the list for where_is_encrypted_column()
     * to find them during the process phase. ReqID:TECC4.0 */
    if (table_encrypted_columns != NULL && update_stmt->relation != NULL)
    {
        int kk;
        for (kk = 0; kk < table_encrypted_column_count; kk++)
        {
            bool found_already = false;
            ListCell *lc2;
            if (table_encrypted_columns[kk] == NULL) continue;
            foreach(lc2, statement_data->encrypted_columns)
            {
                TeccColumnRefData *ed = (TeccColumnRefData *) lfirst(lc2);
                if (ed != NULL)
                {
                    const char *cn = ed->column_name;
                    if (cn[0] == '\0') cn = ed->table_name;
                    if (strcmp(cn, table_encrypted_columns[kk]->column_name) == 0)
                    { found_already = true; break; }
                }
            }
            if (!found_already)
            {
                col_ref = tecc_column_ref_data_create();
                if (col_ref != NULL) {
                    tecc_column_ref_data_init(col_ref,
                        NULL,
                        update_stmt->relation->schemaname,
                        update_stmt->relation->relname,
                        table_encrypted_columns[kk]->column_name);
                    /* ReqID:TECC4.0 - Set table alias if target table has one */
                    if (update_stmt->relation->alias != NULL
                        && update_stmt->relation->alias->aliasname != NULL)
                        tecc_column_ref_data_set_alias(col_ref,
                            update_stmt->relation->alias->aliasname);
                    statement_data->encrypted_columns = lappend(
                        statement_data->encrypted_columns, col_ref);
                }
            }
        }
    }

    /* Process WHERE clause if present */
    if (update_stmt->whereClause != NULL)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Processing UPDATE WHERE clause");

        /* Delegate to WHERE clause processor */
        where_result = tecc_where_clause_processor_process(
            update_stmt->whereClause, statement_data);

        if (where_result != TECC_WHERE_PROCESSOR_SUCCESS)
        {
            switch (where_result)
            {
                case TECC_WHERE_PROCESSOR_MISSING_DET:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "操作符需要DET（确定性加密）算法，但该列未配置DET。");
                    break;
                case TECC_WHERE_PROCESSOR_MISSING_OPE:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "操作符需要OPE（保序加密）算法，但该列未配置OPE。");
                    break;
                case TECC_WHERE_PROCESSOR_MISSING_SEARCH:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "操作符需要SEARCH（可搜索加密）算法，但该列未配置SEARCH。");
                    break;
                case TECC_WHERE_PROCESSOR_MISSING_DET_OPE:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "操作符需要DET或OPE算法，但该列既未配置DET也未配置OPE。");
                    break;
                case TECC_WHERE_PROCESSOR_INCOMPATIBLE_TYPES:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "该列上的操作符与加密算法不兼容。");
                    break;
                default:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "WHERE clause processing failed");
                    break;
            }
            statement_data->needs_rewrite = 0;
            statement_data->where_needs_rewrite = 0;
            return TECC_STMT_PROCESSOR_ERROR;
        }

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "WHERE clause processing completed successfully");
    }

    

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Process DELETE statement
 */
TeccStmtProcessorResult
tecc_stmt_processor_process_delete(const DeleteStmt *delete_stmt,
                                  TeccStatementData *statement_data)
{
    TeccColumnCacheItem **table_encrypted_columns = NULL;
    int table_encrypted_column_count = 0;
    int i;

    if (delete_stmt == NULL || statement_data == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    /* ReqID:TECC4.0 - Set stmt_type */
    statement_data->stmt_type = TECC_STMT_TYPE_DELETE;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing DELETE statement");

    /* Process target table */
    if (delete_stmt->relation != NULL)
    {
        /* Extract table information */
        const char *table_name = delete_stmt->relation->relname;
        const char *schema_name = delete_stmt->relation->schemaname;

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Processing DELETE target table: %s.%s",
                           schema_name ? schema_name : "public",
                           table_name ? table_name : "(null)");

        /* Check if table contains encrypted columns */
        /* Query metadata cache to check for encrypted columns in this table */
        if (table_name != NULL) {
            /* Load table metadata and get encrypted columns list */
            stmt_load_table_metadata(delete_stmt->relation, statement_data,
                                   &table_encrypted_columns, &table_encrypted_column_count);

            /* Populate encrypted_columns list for WHERE clause processing */
            /* 填充encrypted_columns供where_is_encrypted_column判断，但不强制needs_rewrite
             * 只有当WHERE中确实有加密列时，后续WHERE处理才会设needs_rewrite */
            if (table_encrypted_columns != NULL && table_encrypted_column_count > 0)
            {
                for (i = 0; i < table_encrypted_column_count; i++)
                {
                    TeccColumnCacheItem *cache_item = table_encrypted_columns[i];
                    if (cache_item != NULL && cache_item->column_name[0] != '\0')
                    {
                        TeccColumnRefData *col_ref;
                        col_ref = tecc_column_ref_data_create();
                        if (col_ref != NULL) {
                            tecc_column_ref_data_init(col_ref,
                                                     NULL,
                                                     schema_name ? schema_name : "public",
                                                     table_name,
                                                     cache_item->column_name);
                            /* ReqID:TECC4.0 - Set table alias if target table has one */
                            if (delete_stmt->relation->alias != NULL
                                && delete_stmt->relation->alias->aliasname != NULL)
                                tecc_column_ref_data_set_alias(col_ref,
                                    delete_stmt->relation->alias->aliasname);
                            statement_data->encrypted_columns = lappend(
                                statement_data->encrypted_columns, col_ref);
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                               "Added encrypted column %s.%s to encrypted_columns list",
                                               table_name, cache_item->column_name);
                        }
                    }
                }
                /* 不在这里设needs_rewrite=1，由WHERE处理阶段按需设置 */
            }
        }
    }

    /* ReqID:TECC4.0 - Process DELETE USING clause for multi-table DELETE.
     * Load metadata for USING tables so encrypted columns in WHERE conditions
     * referencing USING tables are properly recognized and encrypted. */
    if (delete_stmt->usingClause != NIL)
    {
        ListCell *us_lc;
        statement_data->multi_table_context = 1;
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Processing DELETE USING clause (%d items)",
                           list_length(delete_stmt->usingClause));
        for (us_lc = delete_stmt->usingClause->head; us_lc != NULL; us_lc = us_lc->next)
        {
            Node *using_item = (Node *) us_lc->data.ptr_value;
            if (using_item == NULL)
                continue;

            if (nodeTag(using_item) == FE_T_RangeVar)
            {
                RangeVar *rv = (RangeVar *) using_item;
                TeccColumnCacheItem **us_cols = NULL;
                int us_col_count = 0;
                const char *alias_name = NULL;

                if (rv->alias != NULL && rv->alias->aliasname != NULL)
                    alias_name = rv->alias->aliasname;

                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "DELETE USING table: %s.%s (alias=%s)",
                                   rv->schemaname ? rv->schemaname : "public",
                                   rv->relname ? rv->relname : "(null)",
                                   alias_name ? alias_name : "(none)");

                stmt_load_table_metadata(rv, statement_data, &us_cols, &us_col_count);
                if (us_cols != NULL && us_col_count > 0)
                {
                    int ci;
                    for (ci = 0; ci < us_col_count; ci++)
                    {
                        TeccColumnRefData *us_col_ref;
                        if (us_cols[ci] == NULL) continue;
                        us_col_ref = tecc_column_ref_data_create();
                        if (us_col_ref != NULL)
                        {
                            tecc_column_ref_data_init(us_col_ref,
                                                     NULL,
                                                     rv->schemaname,
                                                     rv->relname,
                                                     us_cols[ci]->column_name);
                            if (alias_name != NULL)
                                tecc_column_ref_data_set_alias(us_col_ref, alias_name);
                            statement_data->encrypted_columns = lappend(
                                statement_data->encrypted_columns, us_col_ref);
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                               "Added USING encrypted column: %s.%s (alias=%s)",
                                               rv->relname, us_cols[ci]->column_name,
                                               alias_name ? alias_name : rv->relname);
                        }
                    }
                    pg_free(us_cols);
                }
            }
            else if (nodeTag(using_item) == FE_T_JoinExpr)
            {
                /* ReqID:TECC4.0 - Recursively traverse JOIN arg tables */
                JoinExpr *j = (JoinExpr *) using_item;
                ListCell *jlc;
                List *join_items = NIL;
                join_items = lappend(join_items, j->larg);
                join_items = lappend(join_items, j->rarg);
                for (jlc = join_items->head; jlc != NULL; jlc = jlc->next)
                {
                    Node *ji = (Node *) jlc->data.ptr_value;
                    if (ji != NULL && nodeTag(ji) == FE_T_RangeVar)
                    {
                        RangeVar *jrv = (RangeVar *) ji;
                        TeccColumnCacheItem **jcols = NULL;
                        int jcol_count = 0;
                        const char *jalias = NULL;
                        if (jrv->alias != NULL && jrv->alias->aliasname != NULL)
                            jalias = jrv->alias->aliasname;
                        stmt_load_table_metadata(jrv, statement_data, &jcols, &jcol_count);
                        if (jcols != NULL && jcol_count > 0)
                        {
                            int ji2;
                            for (ji2 = 0; ji2 < jcol_count; ji2++)
                            {
                                TeccColumnRefData *jcol_ref;
                                if (jcols[ji2] == NULL) continue;
                                jcol_ref = tecc_column_ref_data_create();
                                if (jcol_ref != NULL)
                                {
                                    tecc_column_ref_data_init(jcol_ref, NULL,
                                                             jrv->schemaname,
                                                             jrv->relname,
                                                             jcols[ji2]->column_name);
                                    if (jalias != NULL)
                                        tecc_column_ref_data_set_alias(jcol_ref, jalias);
                                    statement_data->encrypted_columns = lappend(
                                        statement_data->encrypted_columns, jcol_ref);
                                }
                            }
                            pg_free(jcols);
                        }
                    }
                }
                list_free(join_items);
            }
        }
    }

    /* Process WHERE clause if present */
    if (delete_stmt->whereClause != NULL)
    {
        List *where_columns;
        ListCell *lc;
        bool has_encrypted_columns;
        TeccWhereClauseProcessorResult where_result;

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Processing DELETE WHERE clause");

        /* First, extract columns from WHERE clause to check for encrypted columns */
        where_columns = NIL;
        stmt_extract_columns_from_expression(delete_stmt->whereClause, &where_columns, statement_data);

        /* Check if any extracted columns are encrypted */
        if (where_columns != NULL)
        {
            has_encrypted_columns = false;

            for (lc = where_columns->head; lc != NULL; lc = lc->next)
            {
                TeccColumnRefData *tecc_col = (TeccColumnRefData *) lc->data.ptr_value;
                if (tecc_col != NULL && tecc_col->is_encrypted)
                {
                    has_encrypted_columns = true;
                    
                }
            }

            /* If WHERE clause contains encrypted columns, mark statement as needing rewrite */
            if (has_encrypted_columns)
            {
                statement_data->needs_rewrite = 1;
                
            }

            /* Clean up column list */
            if (where_columns != NULL)
            {
                for (lc = where_columns->head; lc != NULL; lc = lc->next)
                {
                    TeccColumnRefData *tecc_col = (TeccColumnRefData *) lc->data.ptr_value;
                    if (tecc_col != NULL)
                        tecc_column_ref_data_destroy(tecc_col);
                }
                list_free(where_columns);
            }
        }

        /* Delegate to WHERE clause processor for actual processing */
        where_result = tecc_where_clause_processor_process(
            delete_stmt->whereClause, statement_data);

        if (where_result != TECC_WHERE_PROCESSOR_SUCCESS)
        {
            switch (where_result)
            {
                case TECC_WHERE_PROCESSOR_MISSING_DET:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "操作符需要DET（确定性加密）算法，但该列未配置DET。");
                    break;
                case TECC_WHERE_PROCESSOR_MISSING_OPE:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "操作符需要OPE（保序加密）算法，但该列未配置OPE。");
                    break;
                case TECC_WHERE_PROCESSOR_MISSING_SEARCH:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "操作符需要SEARCH（可搜索加密）算法，但该列未配置SEARCH。");
                    break;
                case TECC_WHERE_PROCESSOR_MISSING_DET_OPE:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "操作符需要DET或OPE算法，但该列既未配置DET也未配置OPE。");
                    break;
                case TECC_WHERE_PROCESSOR_INCOMPATIBLE_TYPES:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "该列上的操作符与加密算法不兼容。");
                    break;
                default:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "WHERE clause processing failed");
                    break;
            }
            statement_data->needs_rewrite = 0;
            statement_data->where_needs_rewrite = 0;
            return TECC_STMT_PROCESSOR_ERROR;
        }
    }
    else
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "DELETE statement has no WHERE clause");
    }

    

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Process CREATE KEK statement
 */
TeccStmtProcessorResult
tecc_stmt_processor_process_create_kek(const CreateKekStmt *create_kek_stmt,
                                      TeccStatementData *statement_data)
{
    TeccCreateKekProcessorResult result;

    if (create_kek_stmt == NULL || statement_data == NULL)
        return TECC_STMT_PROCESSOR_ERROR;


    /* Delegate to CREATE KEK processor */
    result = tecc_create_kek_processor_run(create_kek_stmt, statement_data);

    /* Convert result */
    switch (result)
    {
        case TECC_CREATE_KEK_PROCESSOR_SUCCESS:
        {
            TeccState *state = tecc_get_state(statement_data->conn);
            if (state != NULL)
            {
                /* Store key name for post-SQL PENDING cleanup */
                const char *kek_name;
                kek_name = (create_kek_stmt->names != NIL) ?
                    strVal(llast(create_kek_stmt->names)) : NULL;
                if (kek_name != NULL)
                {
                    const char *current_schema;
                    const char *key_namespace;
                    TeccKekCacheItem *pending;

                    state->last_stmt_type = TECC_STMT_TYPE_CREATE_KEK;

                    strncpy(state->last_key_name, kek_name, NAMEDATALEN - 1);
                    state->last_key_name[NAMEDATALEN - 1] = '\0';

                    /* 从限定名中提取namespace（schema），无限定名则使用current_schema */
                    if (list_length(create_kek_stmt->names) > 1)
                    {
                        key_namespace = strVal(linitial(create_kek_stmt->names));
                    }
                    else
                    {
                        key_namespace = tecc_cache_get_guc_param(state, "current_schema");
                    }
                    if (key_namespace != NULL)
                    {
                        strncpy(state->last_key_namespace, key_namespace, NAMEDATALEN - 1);
                        state->last_key_namespace[NAMEDATALEN - 1] = '\0';
                    }
                    else
                    {
                        state->last_key_namespace[0] = '\0';
                    }

                    /* Insert PENDING cache item */
                    pending = (TeccKekCacheItem *)pg_malloc0(sizeof(TeccKekCacheItem));
                    pending->key_name = pg_strdup(kek_name);
                    pending->key_namespace = pg_strdup(key_namespace ? key_namespace : "");
                    pending->kek_oid = InvalidOid;
                    pending->cache_status = TECC_CACHE_STATUS_PENDING;
                    if (tecc_cache_insert_kek_item(state, pending))
                    {
                        pg_free(pending->key_name);
                        pg_free(pending->key_namespace);
                        pg_free(pending);
                    }
                }
            }
            return TECC_STMT_PROCESSOR_SUCCESS;
        }
        case TECC_CREATE_KEK_PROCESSOR_INVALID_INPUT:
        case TECC_CREATE_KEK_PROCESSOR_VALIDATION_ERROR:
        case TECC_CREATE_KEK_PROCESSOR_UNSUPPORTED_KMS:
            return TECC_STMT_PROCESSOR_ERROR;
        case TECC_CREATE_KEK_PROCESSOR_HSM_ERROR:
            tecc_processor_debug(TECC_LOG_ERROR,
                                "HSM error in CREATE KEK processing");
            return TECC_STMT_PROCESSOR_ERROR;
        case TECC_CREATE_KEK_PROCESSOR_INTERNAL_ERROR:
        default:
            return TECC_STMT_PROCESSOR_ERROR;
    }
}

/*
 * Helper function to skip whitespace
 */
static const char *
skip_whitespace(const char *str)
{
    while (*str && isspace((unsigned char)*str))
    {
        str++;
    }
    return str;
}

/*
 * Case-insensitive string comparison
 */
static int
ci_strncmp(const char *s1, const char *s2, size_t n)
{
    while (n-- > 0)
    {
        int diff = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
        if (diff != 0)
            return diff;
        if (*s1 == '\0')
            return 0;
        s1++;
        s2++;
    }
    return 0;
}

/*
 * Find "WITH VALUES" case-insensitively, skipping whitespace
 */
static const char *
find_with_values(const char *str)
{
    const char *p = str;

    while (*p)
    {
        /* Skip whitespace */
        p = skip_whitespace(p);

        /* Check if we have "WITH" */
        if (ci_strncmp(p, "WITH", 4) == 0)
        {
            const char *after_with = p + 4;
            after_with = skip_whitespace(after_with);

            /* Check if we have "VALUES" */
            if (ci_strncmp(after_with, "VALUES", 6) == 0)
            {
                return p;
            }
        }

        if (*p)
            p++;
    }

    return NULL;
}

/*
 * Find matching closing parenthesis
 */
static const char *
find_matching_close_paren(const char *open_paren)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    const char *p;
    int paren_count;

    if (*open_paren != '(')
    {
        return NULL;
    }

    p = open_paren + 1;
    paren_count = 1;

    while (*p && paren_count > 0)
    {
        if (*p == '(')
        {
            paren_count++;
        }
        else if (*p == ')')
        {
            paren_count--;
        }
        p++;
    }

    if (paren_count == 0)
    {
        return p - 1; /* p is now after the closing parenthesis */
    }

    return NULL; /* Unbalanced parentheses */
}

/*
 * Helper function to extract KEK name from CREATE DEK options
 */
static const char *
get_kek_name_from_options(const List *options)
{
    ListCell *lc;

    if (options == NULL)
        return NULL;

    foreach(lc, options)
    {
        DefElem *def = (DefElem *) lfirst(lc);
        if (def->defname && strcmp(def->defname, "kek") == 0)
        {
            if (def->arg)
            {
                if (nodeTag(def->arg) == FE_T_String)
                {
                    Value *val = (Value *) def->arg;
                    return val->val.str;
                }
                else if (nodeTag(def->arg) == FE_T_Const)
                {
                    Const *c = (Const *) def->arg;
                    if (c->consttype == 25 || c->consttype == 227)  /* TEXTOID or CSTRINGOID */
                    {
                        return c->constvalue.sval;
                    }
                }
            }
        }
    }

    return NULL;
}

/*
 * Helper function to encrypt DEK with appropriate method based on KEK type
 */
static int
encrypt_dek_based_on_kek_type(const unsigned char *plain_dek, size_t plain_dek_len,
                             const unsigned char *kek, size_t kek_len,
                             unsigned char **encrypted_dek, size_t *encrypted_dek_len)
{
    /* Check if this is an HSM placeholder key */
    bool is_hsm_placeholder = (kek_len == 16 && kek_len >= 11 &&
        (memcmp(kek + 4, "HSM_KEY", 7) == 0 || memcmp(kek + 4, "SWXAKEY", 7) == 0));
    if (is_hsm_placeholder)
    {
        /* HSM KEK: use HSM adapter for encryption */
        bool is_swxa = (memcmp(kek + 4, "SWXAKEY", 7) == 0);
        unsigned int key_index;
        HsmAdapterInterface *hsm_adapter = NULL;
        HsmDevice *device = NULL;
        HsmErrorCode error;

        /* Extract key index from placeholder */
        key_index = (unsigned int)kek[0] |
                   ((unsigned int)kek[1] << 8) |
                   ((unsigned int)kek[2] << 16) |
                   ((unsigned int)kek[3] << 24);

        /* Get HSM adapter based on marker */
        if (is_swxa)
            hsm_adapter = get_swxa_adapter();
        else
            hsm_adapter = get_tzhsm_adapter();
        if (!hsm_adapter)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                                "Failed to get %s adapter for DEK encryption",
                                is_swxa ? "SWXA" : "HSM");
            return -1;
        }

        /* Initialize HSM device */
        error = hsm_adapter->init(&device, NULL);
        if (error != HSM_SUCCESS)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                                "%s initialization failed: %s",
                                is_swxa ? "SWXA" : "HSM",
                                hsm_adapter->error_string(error));
            return -1;
        }

        /* Connect to HSM */
        error = hsm_adapter->connect(device);
        if (error != HSM_SUCCESS)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                                "%s connection failed: %s",
                                is_swxa ? "SWXA" : "HSM",
                                hsm_adapter->error_string(error));
            hsm_adapter->cleanup(device);
            return -1;
        }

        /* Encrypt DEK using HSM */
        error = hsm_adapter->encrypt(device, key_index,
                                    plain_dek, plain_dek_len,
                                    (void **)encrypted_dek, encrypted_dek_len);
        if (error != HSM_SUCCESS)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                                "%s encryption failed: %s",
                                is_swxa ? "SWXA" : "HSM",
                                hsm_adapter->error_string(error));
            hsm_adapter->disconnect(device);
            hsm_adapter->cleanup(device);
            return -1;
        }

        /* Clean up HSM connection */
        hsm_adapter->disconnect(device);
        hsm_adapter->cleanup(device);

        return 0;
    }
    else
    {
        /* Normal KEK: use security layer for encryption */
        return security_encrypt_dek_with_hmac(plain_dek, plain_dek_len,
                                             kek, kek_len,
                                             encrypted_dek, encrypted_dek_len);
    }
}

/*
 * Helper function to convert binary data to hex string
 */
static char *
binary_to_hex_string(const unsigned char *data, size_t data_len)
{
    char *hex_str;
    size_t i;

    if (data == NULL || data_len == 0)
        return NULL;

    hex_str = (char *)pg_malloc(data_len * 2 + 1);
    if (hex_str == NULL)
        return NULL;

    for (i = 0; i < data_len; i++)
    {
        /* Use snprintf to avoid buffer overrun */
        snprintf(hex_str + i * 2, 3, "%02x", data[i]);
    }
    hex_str[data_len * 2] = '\0';

    return hex_str;
}

/*
 * Process CREATE DEK statement
 */
TeccStmtProcessorResult
tecc_stmt_processor_process_create_dek(const CreateDekStmt *create_dek_stmt,
                                      TeccStatementData *statement_data)
{
    const char *dek_name;
    const char *kek_name;
    unsigned char *plain_dek = NULL;
    size_t plain_dek_len = 0;
    unsigned char *kek = NULL;
    size_t kek_len = 0;
    unsigned char *encrypted_dek = NULL;
    size_t encrypted_dek_len = 0;
    char *encrypted_dek_hex = NULL;
    int ret;
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    TeccState *state = NULL;
    const char *original_query = NULL;
    size_t query_len = 0;
    const char *with_values = NULL;
    const char *open_paren = NULL;
    const char *close_paren = NULL;
    const char *p = NULL;
    size_t prefix_len = 0;
    size_t suffix_len = 0;
    size_t new_query_len = 0;
    char *new_query = NULL;

    if (create_dek_stmt == NULL || statement_data == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing CREATE DEK statement");

    /* Extract DEK name from qualified name list */
    dek_name = (create_dek_stmt->names != NIL) ?
        strVal(llast(create_dek_stmt->names)) : NULL;
    if (dek_name == NULL)
    {
        tecc_processor_set_error(statement_data, TECC_PROCESSOR_INVALID_INPUT,
                                "CREATE DEK statement missing DEK name");
        return TECC_STMT_PROCESSOR_ERROR;
    }

    /* Extract KEK name from options */
    kek_name = get_kek_name_from_options(create_dek_stmt->options);
    if (kek_name == NULL)
    {
        tecc_processor_set_error(statement_data, TECC_PROCESSOR_INVALID_INPUT,
                                "CREATE DEK statement missing KEK parameter");
        return TECC_STMT_PROCESSOR_ERROR;
    }

    /* Reject empty or whitespace-only KEK names to avoid stacked errors */
    if (kek_name[0] == '\0' ||
        kek_name[strspn(kek_name, " \t\r\n")] == '\0')
    {
        tecc_processor_set_error(statement_data, TECC_PROCESSOR_INVALID_INPUT,
                                "KEK name is empty or invalid");
        return TECC_STMT_PROCESSOR_ERROR;
    }

    /* Step 1: Get KEK with TeccState for cache access */
    state = tecc_get_state(statement_data->conn);
    if (state == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                            "Failed to get TECC state for connection, cannot encrypt DEK");
        return TECC_STMT_PROCESSOR_ERROR;
    }

    kek = tecc_key_get_kek_with_state(state, kek_name, &kek_len);
    if (kek == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                            "KEK '%s' cache miss, cannot encrypt DEK", kek_name);
        return TECC_STMT_PROCESSOR_ERROR;
    }

    /* DEBUG: 打印加密时使用的KEK */
    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "[CREATE DEK] Got KEK '%s', len=%zu for DEK encryption",
                        kek_name, kek_len);

    /* Step 2: Generate random DEK (32 bytes for SM4) */
    ret = security_generate_dek_key(32, &plain_dek, &plain_dek_len);
    if (ret != SECURITY_SUCCESS)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                            "Failed to generate DEK: error %d", ret);
        tecc_key_free_key_data(kek, kek_len);
        return TECC_STMT_PROCESSOR_ERROR;
    }

    /* Step 3: Encrypt DEK with KEK */
    ret = encrypt_dek_based_on_kek_type(plain_dek, plain_dek_len,
                                       kek, kek_len,
                                       &encrypted_dek, &encrypted_dek_len);
    if (ret != 0)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                            "Failed to encrypt DEK with KEK '%s'", kek_name);
        security_secure_erase(plain_dek, plain_dek_len);
        pg_free(plain_dek);
        tecc_key_free_key_data(kek, kek_len);
        return TECC_STMT_PROCESSOR_ERROR;
    }

    /* Step 4: Convert encrypted DEK to hex string */
    encrypted_dek_hex = binary_to_hex_string(encrypted_dek, encrypted_dek_len);
    if (encrypted_dek_hex == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                            "Failed to convert encrypted DEK to hex string");
        security_secure_erase(plain_dek, plain_dek_len);
        pg_free(plain_dek);
        tecc_key_free_key_data(kek, kek_len);
        security_secure_erase(encrypted_dek, encrypted_dek_len);
        pg_free(encrypted_dek);
        return TECC_STMT_PROCESSOR_ERROR;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "DEK encrypted successfully, hex length: %zu",
                        strlen(encrypted_dek_hex));

    /* Step 5: Rewrite SQL to add ENCRYPTED_VALUE parameter */
    /* We need to modify: CREATE DEK dek_name WITH VALUES(KEK = kek_name) */
    /* To become: CREATE DEK dek_name WITH VALUES(KEK = kek_name, ENCRYPTED_VALUE = 'hex_string') */

    /* Find the position of the closing parenthesis in WITH VALUES(...) */
    original_query = statement_data->query;
    query_len = strlen(original_query);

    /* Find "WITH VALUES" case-insensitively */
    with_values = find_with_values(original_query);
    tecc_processor_debug(TECC_LOG_DEBUG1,
        "SQL REWRITE: with_values=%p, original_query=%p, offset=%td",
        with_values, original_query,
        with_values ? with_values - original_query : -1);
    if (with_values != NULL)
    {
        /* Find opening parenthesis after WITH VALUES */
        p = with_values;
        while (*p && *p != '(')
        {
            p++;
        }

        if (*p == '(')
        {
            open_paren = p;
            /* Find matching closing parenthesis */
            close_paren = find_matching_close_paren(open_paren);
        tecc_processor_debug(TECC_LOG_DEBUG1,
            "SQL REWRITE: close_paren offset=%td, prefix_len will be %td",
            close_paren ? close_paren - original_query : -1,
            close_paren ? close_paren - original_query : -1);
        }
    }

    if (with_values == NULL || open_paren == NULL || close_paren == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                            "Failed to parse CREATE DEK SQL for rewriting");
        security_secure_erase(plain_dek, plain_dek_len);
        pg_free(plain_dek);
        tecc_key_free_key_data(kek, kek_len);
        security_secure_erase(encrypted_dek, encrypted_dek_len);
        pg_free(encrypted_dek);
        pg_free(encrypted_dek_hex);
        return TECC_STMT_PROCESSOR_ERROR;
    }

    /* Calculate positions */
    /* prefix includes everything before the closing paren, suffix starts after it */
    prefix_len = close_paren - original_query;  /* up to but not including ')' */
    suffix_len = query_len - prefix_len - 1; /* skip the ')' */

    /* Build new query: prefix + ", ENCRYPTED_VALUE = 'hex', _TECC_PROCESSED = 'yes'" + ")" + suffix */
    new_query_len = prefix_len + strlen(", ENCRYPTED_VALUE = '") +
                   strlen(encrypted_dek_hex) + strlen("'") +
                   strlen(", _TECC_PROCESSED = 'yes'") + 1 + suffix_len + 1;
    new_query = (char *)pg_malloc(new_query_len);

    if (new_query == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                            "Failed to allocate memory for rewritten query");
        security_secure_erase(plain_dek, plain_dek_len);
        pg_free(plain_dek);
        tecc_key_free_key_data(kek, kek_len);
        security_secure_erase(encrypted_dek, encrypted_dek_len);
        pg_free(encrypted_dek);
        pg_free(encrypted_dek_hex);
        return TECC_STMT_PROCESSOR_ERROR;
    }

    /* Construct new query */
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    {
        size_t pos = 0;

    /* Copy original query prefix (everything before the closing paren) */
    memcpy(new_query + pos, original_query, prefix_len);
    pos += prefix_len;

    /* Insert ENCRYPTED_VALUE inside the parens, just before the closing paren */
    memcpy(new_query + pos, ", ENCRYPTED_VALUE = '", strlen(", ENCRYPTED_VALUE = '"));
    pos += strlen(", ENCRYPTED_VALUE = '");

    memcpy(new_query + pos, encrypted_dek_hex, strlen(encrypted_dek_hex));
    pos += strlen(encrypted_dek_hex);

    memcpy(new_query + pos, "'", 1);
    pos += 1;

    /* ReqID:TECC4.0 - Inject _TECC_PROCESSED marker so backend recognizes this as TECC-processed */
    memcpy(new_query + pos, ", _TECC_PROCESSED = 'yes'", strlen(", _TECC_PROCESSED = 'yes'"));
    pos += strlen(", _TECC_PROCESSED = 'yes'");

    /* Re-insert the closing paren and the suffix (e.g., ";") */
    new_query[pos++] = ')';
    memcpy(new_query + pos, close_paren + 1, suffix_len);
    new_query[new_query_len - 1] = '\0';

    
    
    }

    /* Store rewritten query in statement data */
    if (statement_data->rewritten_query != NULL)
    {
        pg_free(statement_data->rewritten_query);
    }
    statement_data->rewritten_query = new_query;
    /* Set needs_rewrite so the caller uses the rewritten query */
    tecc_stmt_set_needs_rewrite(statement_data, true);

    /* Clean up */
    security_secure_erase(plain_dek, plain_dek_len);
    pg_free(plain_dek);
    tecc_key_free_key_data(kek, kek_len);
    security_secure_erase(encrypted_dek, encrypted_dek_len);
    pg_free(encrypted_dek);
    pg_free(encrypted_dek_hex);

    /* Mark DEK cache for refresh and insert PENDING cache item */
    {
        TeccState *state = tecc_get_state(statement_data->conn);
        if (state != NULL)
        {
            const char *key_namespace;
            TeccDekCacheItem *pending;

            state->last_stmt_type = TECC_STMT_TYPE_CREATE_DEK;

            /* Store key name for post-SQL PENDING cleanup */
            strncpy(state->last_key_name, dek_name, NAMEDATALEN - 1);
            state->last_key_name[NAMEDATALEN - 1] = '\0';

            /* 从限定名中提取namespace（schema），无限定名则使用current_schema */
            if (list_length(create_dek_stmt->names) > 1)
            {
                key_namespace = strVal(linitial(create_dek_stmt->names));
            }
            else
            {
                key_namespace = tecc_cache_get_guc_param(state, "current_schema");
            }
            if (key_namespace != NULL)
            {
                strncpy(state->last_key_namespace, key_namespace, NAMEDATALEN - 1);
                state->last_key_namespace[NAMEDATALEN - 1] = '\0';
            }
            else
            {
                state->last_key_namespace[0] = '\0';
            }

            /* Insert PENDING cache item with known metadata */
            pending = (TeccDekCacheItem *)pg_malloc0(sizeof(TeccDekCacheItem));
            pending->key_name = pg_strdup(dek_name);
            pending->key_namespace = pg_strdup(key_namespace ? key_namespace : "");
            pending->kek_name = pg_strdup(kek_name);
            pending->dek_oid = InvalidOid;
            pending->cache_status = TECC_CACHE_STATUS_PENDING;
            if (tecc_cache_insert_dek_item(state, pending) != 0)
            {
                pg_free(pending->key_name);
                pg_free(pending->key_namespace);
                pg_free(pending->kek_name);
                pg_free(pending);
            }

        }
    }

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Process CREATE TABLE statement
 */
TeccStmtProcessorResult
tecc_stmt_processor_process_create_table(const CreateStmt *create_stmt,
                                        TeccStatementData *statement_data)
{
    TeccCreateStmtProcessorResult create_result;

    if (create_stmt == NULL || statement_data == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing CREATE TABLE statement");

    /* Delegate to CREATE statement processor */
    create_result = tecc_create_stmt_processor_run(create_stmt, statement_data);

    /* Convert CREATE statement processor result to statement processor result */
    switch (create_result)
    {
        case TECC_CREATE_PROCESSOR_SUCCESS:
            return TECC_STMT_PROCESSOR_SUCCESS;

        case TECC_CREATE_PROCESSOR_DUPLICATE_COLUMN:
            tecc_processor_debug(TECC_LOG_ERROR,
                               "CREATE TABLE duplicate column name");
            return TECC_STMT_PROCESSOR_ERROR;

        case TECC_CREATE_PROCESSOR_ERROR:
        default:
            tecc_processor_debug(TECC_LOG_ERROR,
                               "CREATE TABLE processing failed");
            return TECC_STMT_PROCESSOR_ERROR;
    }
}

/*
 * Process ALTER TABLE statement
 */
TeccStmtProcessorResult
tecc_stmt_processor_process_alter_table(const AlterTableStmt *alter_stmt,
                                        TeccStatementData *statement_data)
{
    TeccAlterStmtProcessorResult alter_result;

    if (alter_stmt == NULL || statement_data == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing ALTER TABLE statement");

    /* Delegate to ALTER statement processor */
    alter_result = tecc_alter_stmt_processor_run(alter_stmt, statement_data);

    switch (alter_result)
    {
        case TECC_ALTER_PROCESSOR_SUCCESS:
            return TECC_STMT_PROCESSOR_SUCCESS;

        case TECC_ALTER_PROCESSOR_ERROR:
        default:
            tecc_processor_debug(TECC_LOG_ERROR,
                               "ALTER TABLE processing failed");
            return TECC_STMT_PROCESSOR_ERROR;
    }
}

/* ReqID:TECC4.0 - Forward declaration for ALTER TABLE dispatch end marker */

/*
 * Process DROP statement
 */
TeccStmtProcessorResult
tecc_stmt_processor_process_drop(const DropStmt *drop_stmt,
                                TeccStatementData *statement_data)
{
    if (drop_stmt == NULL || statement_data == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing DROP statement (removeType=%d)", drop_stmt->removeType);

    /* ReqID:TECC4.0 - Save drop object type, behaviour and key OID
     * for selective adapter cache invalidation. */
    statement_data->drop_remove_type = (int)drop_stmt->removeType;
    statement_data->drop_behavior = (int)drop_stmt->behavior;
    statement_data->drop_key_oid = InvalidOid;

    /* Snapshot OID from metadata cache before server drops the key.
     * This runs pre-query, so the key still exists on the server. */
    if ((drop_stmt->removeType == OBJECT_KEK || drop_stmt->removeType == OBJECT_DEK) &&
        drop_stmt->objects != NIL)
    {
        const char *kname = strVal(linitial(drop_stmt->objects));
        TeccState *state = tecc_get_state(statement_data->conn);
        if (drop_stmt->removeType == OBJECT_KEK && state)
        {
            TeccKekCacheItem *kek = tecc_cache_get_kek_by_name_search_path(
                (const TeccState *)state, kname);
            if (kek) {
                statement_data->drop_key_oid = kek->kek_oid;
                /* ReqID:TECC4.0 - Save kms_path for client-side file deletion.
                 * key_path is on the client filesystem, server cannot delete it. */
                if (kek->kms_path != NULL && kek->kms_path[0] != '\0')
                {
                    strncpy(state->last_kek_path, kek->kms_path,
                            sizeof(state->last_kek_path) - 1);
                    state->last_kek_path[sizeof(state->last_kek_path) - 1] = '\0';
                }
            } else {
            }
        }
        else if (drop_stmt->removeType == OBJECT_DEK && state)
        {
            TeccDekCacheItem *dek = tecc_cache_get_dek_by_name_search_path(
                (const TeccState *)state, kname);
            if (dek) {
                statement_data->drop_key_oid = dek->dek_oid;
            } else {
            }
        }
    }

    /* Invalidate cache when KEK or DEK is dropped */
    if (drop_stmt->removeType == OBJECT_KEK)
    {
        TeccState *state = tecc_get_state(statement_data->conn);
        if (state != NULL)
            tecc_cache_mark_kek_refresh(state);
    }
    else if (drop_stmt->removeType == OBJECT_DEK)
    {
        TeccState *state = tecc_get_state(statement_data->conn);
        if (state != NULL)
            tecc_cache_mark_dek_refresh(state);
    }

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Process COPY statement
 */
TeccStmtProcessorResult
tecc_stmt_processor_process_copy(const CopyStmt *copy_stmt,
                                TeccStatementData *statement_data)
{
    if (copy_stmt == NULL || statement_data == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing COPY statement (is_from=%s)",
                        copy_stmt->is_from ? "true" : "false");

    /* ==================== 1. 验证加密列限制 ==================== */
    /* 加密列只支持STDIN/STDOUT，不支持文件操作 */
    if (copy_stmt->filename != NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "COPY with filename not supported for encrypted columns: %s",
                           copy_stmt->filename);
        return TECC_STMT_PROCESSOR_ERROR;
    }

    /* ==================== 2. 处理表引用信息 ==================== */
    if (copy_stmt->relation != NULL) {
        /* 获取表名信息 */
        const char *table_name = copy_stmt->relation->relname;
        const char *schema_name = copy_stmt->relation->schemaname;

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "COPY %s table: %s.%s",
                           copy_stmt->is_from ? "FROM" : "TO",
                           schema_name ? schema_name : "(default)",
                           table_name);

        /* 记录表信息到statement_data，供后续处理使用 */
        /* 注意：TeccStatementData结构体目前没有table_name和schema_name字段 */
        /* 如果需要这些信息，可以存储在扩展字段或通过其他机制传递 */
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Table info: %s.%s (not stored in statement_data)",
                           schema_name ? schema_name : "(default)",
                           table_name);
    }

    /* ==================== 3. 处理COPY (SELECT ...) 查询 ==================== */
    if (copy_stmt->query != NULL) {
        /* COPY (SELECT ...) TO STDOUT */
        

        if (copy_stmt->is_from) {
            /* COPY FROM不支持SELECT查询（前端解析器已检查，这里双重验证） */
            tecc_processor_debug(TECC_LOG_ERROR,
                               "COPY FROM with SELECT query not supported");
            return TECC_STMT_PROCESSOR_ERROR;
        }

        /* 处理SELECT查询中的加密列 */
        /* 需要递归处理SELECT语句中的列引用 */
        


        /* 递归处理SELECT查询 */
        if (nodeTag(copy_stmt->query) == FE_T_SelectStmt)
        {
            SelectStmt *select_stmt = (SelectStmt *) copy_stmt->query;
            TeccStmtProcessorResult select_result;

            select_result = tecc_stmt_processor_process_select(select_stmt, statement_data);
            if (select_result != TECC_STMT_PROCESSOR_SUCCESS)
            {
                tecc_processor_debug(TECC_LOG_ERROR,
                                   "Failed to process SELECT query in COPY statement: %d", select_result);
                return select_result;
            }

            
        }
        else
        {
            tecc_processor_debug(TECC_LOG_WARNING,
                               "COPY query is not a SELECT statement (type: %d)", nodeTag(copy_stmt->query));
        }
    }

    /* ==================== 4. 处理COPY方向 ==================== */
    if (copy_stmt->is_from) {
        /* COPY FROM - 导入数据到加密列 */
        

        /* 实现加密列的COPY FROM处理 */
        /* 这涉及：
           1. 识别目标表中的加密列
           2. 为传入数据设置加密
           3. 修改COPY命令以处理加密数据
        */

        /* 检查目标表是否有加密列 */
        if (copy_stmt->relation != NULL) {
            const char *table_name = copy_stmt->relation->relname;



            /* 如果有显式列列表，检查这些列是否加密 */
            if (copy_stmt->attlist != NULL) {
                ListCell *lc;
                bool has_encrypted_columns = false;

                for (lc = copy_stmt->attlist->head; lc != NULL; lc = lc->next) {
                    Node *node = (Node *) lc->data.ptr_value;
                    if (node != NULL && nodeTag(node) == FE_T_String) {
                        char *col_name = (char *) node;

                        /* 检查此列是否加密 */
                        bool is_encrypted = false;

                        /* 首先检查statement_data中的加密列列表 */
                        if (statement_data->encrypted_columns != NIL) {
                            ListCell *lc2;
                            foreach(lc2, statement_data->encrypted_columns) {
                                TeccColumnRefData *encrypted_col = (TeccColumnRefData *) lfirst(lc2);
                                if (encrypted_col != NULL &&
                                    encrypted_col->column_name[0] != '\0' &&
                                    strcmp(encrypted_col->column_name, col_name) == 0) {
                                    is_encrypted = true;
                                    break;
                                }
                            }
                        }

                        /* 如果statement_data中没有找到，使用缓存模块检查 */
                        if (!is_encrypted && table_name != NULL) {
                            is_encrypted = stmt_check_column_encrypted(statement_data, table_name, col_name);
                        }

                        /* 如果发现加密列，标记需要改写 */
                        if (is_encrypted) {
                            has_encrypted_columns = true;

                        }
                    }
                }

                if (has_encrypted_columns) {
                    statement_data->needs_rewrite = 1;
                    statement_data->is_copy_command = true;

                }
			} else {
				/* 没有显式列列表，查询元数据确认是否有加密列 */
				TeccColumnCacheItem **tbl_cols = NULL;
				int tbl_col_cnt = 0;
				if (stmt_load_table_metadata(copy_stmt->relation, statement_data,
											 &tbl_cols, &tbl_col_cnt)
					&& tbl_col_cnt > 0)
				{
					statement_data->needs_rewrite = 1;
					statement_data->is_copy_command = true;
				}
			}

		}
    } else {
        /* COPY TO - 从加密列导出数据 */
        

        /* 实现加密列的COPY TO处理 */
        /* 这涉及：
           1. 识别源表中的加密列
           2. 为传出数据设置解密
           3. 修改COPY命令以处理解密数据
        */

        /* 检查源表是否有加密列 */
        if (copy_stmt->relation != NULL) {


            /* 如果有显式列列表，检查这些列是否加密 */
            if (copy_stmt->attlist != NULL) {
                ListCell *lc;
                bool has_encrypted_columns = false;

                for (lc = copy_stmt->attlist->head; lc != NULL; lc = lc->next) {
                    Node *node = (Node *) lc->data.ptr_value;
                    if (node != NULL && nodeTag(node) == FE_T_String) {
                        char *col_name = (char *) node;

                        /* 检查此列是否加密 */
                        bool is_encrypted = false;

                        /* 首先检查statement_data中的加密列列表 */
                        if (statement_data->encrypted_columns != NIL) {
                            ListCell *lc2;
                            foreach(lc2, statement_data->encrypted_columns) {
                                TeccColumnRefData *encrypted_col = (TeccColumnRefData *) lfirst(lc2);
                                if (encrypted_col != NULL &&
                                    encrypted_col->column_name[0] != '\0' &&
                                    strcmp(encrypted_col->column_name, col_name) == 0) {
                                    is_encrypted = true;
                                    break;
                                }
                            }
                        }

                        /* 如果发现加密列，标记需要改写 */
                        if (is_encrypted) {
                            has_encrypted_columns = true;
                            
                        }
                    }
                }

                if (has_encrypted_columns) {
                    statement_data->needs_rewrite = 1;
                    statement_data->is_copy_command = true;
                    
                }
			} else {
				/* 没有显式列列表，查询元数据确认是否有加密列 */
				TeccColumnCacheItem **tbl_cols2 = NULL;
				int tbl_col_cnt2 = 0;
				if (stmt_load_table_metadata(copy_stmt->relation, statement_data,
											 &tbl_cols2, &tbl_col_cnt2)
					&& tbl_col_cnt2 > 0)
				{
					statement_data->needs_rewrite = 1;
					statement_data->is_copy_command = true;
				}
			}
        }

    }

    /* ==================== 5. 提取列引用 ==================== */
    if (copy_stmt->attlist != NULL) {
        ListCell *lc;

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "COPY has explicit column list with %d columns",
                           list_length(copy_stmt->attlist));

        for (lc = copy_stmt->attlist->head; lc != NULL; lc = lc->next) {
            Node *node = (Node *) lc->data.ptr_value;
            TeccColumnRefData *col_data;

            if (node != NULL && nodeTag(node) == FE_T_String) {
                char *col_name = (char *) node;
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "COPY column: %s", col_name);

                /* 创建列引用数据结构 */
                col_data = tecc_column_ref_data_create();
                if (col_data != NULL) {
                    tecc_column_ref_data_init(col_data,
                                             copy_stmt->relation ? copy_stmt->relation->relname : NULL,
                                             copy_stmt->relation ? copy_stmt->relation->schemaname : NULL,
                                             col_name,
                                             NULL);

                    /* TODO: 检查此列是否加密 - 需要查询元数据缓存 */
                    /* 目前先添加到列引用列表 */
                    statement_data->encrypted_columns = lappend(statement_data->encrypted_columns, col_data);
                }
            }
        }
    } else if (copy_stmt->relation != NULL) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "COPY has no explicit column list - all table columns implied");
		/* 没有显式列列表 */
    }

    /* ==================== 6. 处理COPY选项 ==================== */
    if (copy_stmt->options != NULL) {
        ListCell *lc;
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "COPY has %d options", list_length(copy_stmt->options));

        /* 处理COPY选项，特别是与加密相关的选项 */
        for (lc = copy_stmt->options->head; lc != NULL; lc = lc->next) {
            DefElem *def_elem = (DefElem *) lc->data.ptr_value;
            if (def_elem != NULL && def_elem->defname != NULL) {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "COPY option: %s", def_elem->defname);

                /* 检查可能影响加密处理的选项 */
                if (strcmp(def_elem->defname, "format") == 0) {
                    /* 格式选项可能影响加密数据的处理 */
                    
                } else if (strcmp(def_elem->defname, "encoding") == 0) {
                    /* 编码选项可能影响加密数据的处理 */
                    
                }
            }
        }
    }

    /* ==================== 7. 设置SQL改写支持 ==================== */
    /* 使用stmt_location和stmt_len支持处理器模块的SQL改写 */
    if (copy_stmt->stmt_location >= 0 && copy_stmt->stmt_len > 0) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "COPY statement location: %d, length: %d",
                           copy_stmt->stmt_location, copy_stmt->stmt_len);

        /* 将位置信息记录到statement_data，供SQL改写使用 */
        /* 注意：TeccStatementData结构体目前没有stmt_location和stmt_len字段 */
        /* 位置信息可以通过其他机制传递给SQL改写器 */
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Statement location info available (not stored in statement_data)");
    }

    /* 如果检测到加密列或需要特殊处理，标记需要SQL改写 */
    if (statement_data->needs_rewrite) {
        
    }

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Identify statement type from node
 */
TeccStatementType
tecc_stmt_processor_identify_statement(const Node *stmt)
{
    if (stmt == NULL)
        return TECC_STMT_TYPE_UNKNOWN;

    switch (nodeTag(stmt))
    {
        case FE_T_SelectStmt:
            return TECC_STMT_TYPE_SELECT;

        case FE_T_InsertStmt:
            return TECC_STMT_TYPE_INSERT;

        case FE_T_UpdateStmt:
            return TECC_STMT_TYPE_UPDATE;

        case FE_T_DeleteStmt:
            return TECC_STMT_TYPE_DELETE;

        case FE_T_CreateKekStmt:
            return TECC_STMT_TYPE_CREATE_KEK;

        case FE_T_CreateDekStmt:
            return TECC_STMT_TYPE_CREATE_DEK;

        case FE_T_CreateStmt:
            return TECC_STMT_TYPE_CREATE_TABLE;

        case FE_T_AlterTableStmt:
            return TECC_STMT_TYPE_ALTER_TABLE;

        case FE_T_DropStmt:
            return TECC_STMT_TYPE_DROP;

        case FE_T_CopyStmt:
            /* COPY statement */
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "COPY statement identified");
            return TECC_STMT_TYPE_COPY;

        case FE_T_PrepareStmt:
            /* PREPARE statement */
            return TECC_STMT_TYPE_PREPARE;

        case FE_T_ExecuteStmt:
            /* EXECUTE statement */
            return TECC_STMT_TYPE_EXECUTE;

        case FE_T_DeclareCursorStmt:
            /* DECLARE CURSOR statement */
            return TECC_STMT_TYPE_DECLARE_CURSOR;

        case FE_T_ExplainStmt:
            /* EXPLAIN statement */
            return TECC_STMT_TYPE_EXPLAIN;

        default:
            return TECC_STMT_TYPE_UNKNOWN;
    }
}

/*
 * Internal helper functions
 */
static TeccStmtProcessorResult
stmt_process_parse_tree(TeccStatementData *statement_data)
{
    ListCell *lc;

    if (statement_data == NULL || statement_data->parse_tree == NIL)
        return TECC_STMT_PROCESSOR_SUCCESS;

    /* Process each statement in the parse tree */
    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Processing parse tree with %d statements",
                       list_length(statement_data->parse_tree));

    foreach(lc, statement_data->parse_tree)
    {
        Node *stmt = (Node *)lfirst(lc);
        NodeTag stmt_type = nodeTag(stmt);

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Processing statement type: %d", stmt_type);

        /* 根据语句类型调用相应的处理函数 */
        switch (stmt_type)
        {
            case FE_T_InsertStmt:
                /* INSERT语句处理 */
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing INSERT statement");
                /* 具体的INSERT处理在values_processor中实现 */
                break;

            case FE_T_UpdateStmt:
                /* UPDATE语句处理 */
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing UPDATE statement");
                /* 具体的UPDATE处理在values_processor中实现 */
                break;

            case FE_T_DeleteStmt:
                /* DELETE语句处理 */
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing DELETE statement");
                /* DELETE语句通常不需要值处理，但可能需要WHERE子句处理 */
                break;

            case FE_T_SelectStmt:
                /* SELECT语句处理 */
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing SELECT statement");
                /* SELECT语句处理在专门的SELECT处理器中实现 */
                break;

            case FE_T_CreateStmt:
                /* CREATE TABLE语句处理 */
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing CREATE TABLE statement");
                /* CREATE TABLE语句在create_stmt_processor中处理 */
                break;

            case FE_T_ExplainStmt:
                /* EXPLAIN语句处理 */
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing EXPLAIN statement");
                break;

            default:
                tecc_processor_debug(TECC_LOG_WARNING,
                                   "Unhandled statement type: %d", stmt_type);
                break;
        }
    }

    return TECC_STMT_PROCESSOR_SUCCESS;
}

static TeccStmtProcessorResult
stmt_collect_encrypted_columns(TeccStatementData *statement_data)
{
    ListCell *lc;
    List *encrypted_columns = NIL;

    if (statement_data == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    /* Collect encrypted columns from all processed statements */
    /* First check if we already have encrypted columns identified */
    if (statement_data->encrypted_columns != NIL)
    {
        /* Use existing encrypted columns list */
        return TECC_STMT_PROCESSOR_SUCCESS;
    }

    /* Scan the parse tree to identify encrypted columns */
    if (statement_data->parse_tree != NIL)
    {
        for (lc = statement_data->parse_tree->head; lc != NULL; lc = lc->next)
        {
            Node *stmt = (Node *) lc->data.ptr_value;
            /* Extract column references from the statement and check if encrypted */
            TeccStmtProcessorResult result = stmt_extract_and_check_columns(stmt, statement_data, &encrypted_columns);
            if (result != TECC_STMT_PROCESSOR_SUCCESS)
            {
                tecc_processor_debug(TECC_LOG_WARNING,
                                    "Failed to extract columns from statement type: %d", nodeTag(stmt));
                /* Continue processing other statements */
            }
        }
    }

    /* Update statement data with found encrypted columns */
    statement_data->encrypted_columns = encrypted_columns;

    if (encrypted_columns != NIL)
    {
        
        /* ReqID:TECC4.0 - Don't set needs_rewrite here unconditionally.
         * For SELECT, target list encrypted columns need result decryption only.
         * WHERE clause, expression parameters set their own needs_rewrite flags. */
        if (statement_data->stmt_type != TECC_STMT_TYPE_SELECT)
        {
            statement_data->needs_rewrite = 1;
        }
    }

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Extract column references from a statement and check if they are encrypted
 */
static TeccStmtProcessorResult
stmt_extract_and_check_columns(const Node *stmt,
                              TeccStatementData *statement_data,
                              List **encrypted_columns)
{
    TeccStmtProcessorResult result = TECC_STMT_PROCESSOR_SUCCESS;
    List *stmt_columns = NIL;
    ListCell *lc;
    int is_encrypted;

    if (stmt == NULL || statement_data == NULL || encrypted_columns == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    /* Extract column references based on statement type */
    switch (nodeTag(stmt))
    {
        case FE_T_SelectStmt:
            result = stmt_extract_select_columns((SelectStmt *) stmt, &stmt_columns, statement_data);
            break;

        case FE_T_InsertStmt:
            result = stmt_extract_insert_columns((InsertStmt *) stmt, &stmt_columns, statement_data);
            break;

        case FE_T_UpdateStmt:
            result = stmt_extract_update_columns((UpdateStmt *) stmt, &stmt_columns, statement_data);
            break;

        case FE_T_DeleteStmt:
            result = stmt_extract_delete_columns((DeleteStmt *) stmt, &stmt_columns, statement_data);
            break;

        case FE_T_CreateStmt:
            result = stmt_extract_create_columns((CreateStmt *) stmt, &stmt_columns);
            break;

        case FE_T_AlterTableStmt:
            /* ALTER TABLE statement - extract columns from table modifications */
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "ALTER TABLE statement column extraction not yet implemented");
            /* For now, return success with no columns */
            result = TECC_STMT_PROCESSOR_SUCCESS;
            break;

        case FE_T_CopyStmt:
            /* COPY statement - extract columns from table or query */
            result = stmt_extract_copy_columns((CopyStmt *) stmt, &stmt_columns, statement_data);
            break;

        default:
            /* Unsupported statement type - no columns to extract */
            return TECC_STMT_PROCESSOR_SUCCESS;
    }

    if (result != TECC_STMT_PROCESSOR_SUCCESS)
        return result;

    /* Check each column against cache to see if it's encrypted */
    if (stmt_columns != NULL) {
        for (lc = stmt_columns->head; lc != NULL; lc = lc->next)
        {
            TeccColumnRefData *col_ref = (TeccColumnRefData *) lc->data.ptr_value;
            if (col_ref == NULL)
                continue;

            /* Check if column is encrypted */
            /* ReqID:TECC4.0 - Use is_encrypted from column_ref_data (set by tecc_expand_column_ref_with_stmt_data) */
            is_encrypted = col_ref->is_encrypted ? 1 : 0;

            if (is_encrypted)
            {
                /* Add to encrypted columns list */
                *encrypted_columns = lappend(*encrypted_columns, col_ref);
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Found encrypted column: %s.%s",
                                   col_ref->table_name, col_ref->column_name);
            }
            else
            {
                /* Free non-encrypted column reference */
                tecc_column_ref_data_destroy(col_ref);
            }
        }
    }

    /* Free the temporary column list (elements already freed or moved) */
    list_free(stmt_columns);

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Extract column references from SELECT statement
 */
static TeccStmtProcessorResult
stmt_extract_select_columns(const SelectStmt *select_stmt, List **columns,
                           const TeccStatementData *statement_data)
{
    ListCell *lc;

    if (select_stmt == NULL || columns == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Extracting columns from SELECT statement");

    /* ReqID:TECC4.0 - Traverse CTE definitions first */
    if (select_stmt->cteList != NIL)
    {
        ListCell *cte_lc;
        foreach(cte_lc, select_stmt->cteList)
        {
            CommonTableExpr *cte = (CommonTableExpr *) lfirst(cte_lc);
            if (cte->ctequery != NULL)
            {
                if (nodeTag(cte->ctequery) == FE_T_SelectStmt)
                {
                    stmt_extract_select_columns((SelectStmt *)cte->ctequery, columns, statement_data);
                }
                /* ReqID:TECC4.0 - DML CTE: extract columns from INSERT/UPDATE/DELETE */
                else if (nodeTag(cte->ctequery) == FE_T_InsertStmt)
                {
                    tecc_stmt_processor_process_insert((InsertStmt *)cte->ctequery, (TeccStatementData *)statement_data);
                }
                else if (nodeTag(cte->ctequery) == FE_T_UpdateStmt)
                {
                    tecc_stmt_processor_process_update((UpdateStmt *)cte->ctequery, (TeccStatementData *)statement_data);
                }
                else if (nodeTag(cte->ctequery) == FE_T_DeleteStmt)
                {
                    tecc_stmt_processor_process_delete((DeleteStmt *)cte->ctequery, (TeccStatementData *)statement_data);
                }
            }
        }
    }

    /* Extract columns from target list */
    if (select_stmt->targetList != NIL) {
        for (lc = select_stmt->targetList->head; lc != NULL; lc = lc->next)
        {
            /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
            Node *target_node = NULL;
            ResTarget *res_target = NULL;

            target_node = (Node *) lc->data.ptr_value;

            /* Check if this is a Const node */
            if (target_node == NULL || target_node->type == FE_T_Const) {
                /* It's a constant value or NULL, not a column reference */
                continue;
            }

            /* Otherwise, assume it's a ResTarget */
            res_target = (ResTarget *) target_node;
            if (res_target->val == NULL)
                continue;

            /* Check if this is a column reference */
            if (nodeTag(res_target->val) == FE_T_ColumnRef)
            {
                /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
                ColumnRef *col_ref = (ColumnRef *) res_target->val;
                TeccColumnRefData *tecc_col = NULL;

                /* Use tecc_expand_column_ref_with_stmt_data to extract column information */
                if (tecc_expand_column_ref_with_stmt_data(col_ref, &tecc_col, statement_data))
                {
                    if (tecc_col != NULL)
                    {
                        *columns = lappend(*columns, tecc_col);
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                           "Added column reference: %s (encrypted=%s)",
                                           tecc_col->alias_fqdn,
                                           tecc_col->is_encrypted ? "true" : "false");
                    }
                }
                else
                {
                    tecc_processor_debug(TECC_LOG_WARNING,
                                       "Failed to expand column reference in target list");
                }
            }
            else if (nodeTag(res_target->val) == FE_T_FuncCall)
            {
                /* Function call - extract column references from arguments */
                FuncCall *func_call = (FuncCall *) res_target->val;
                if (func_call->args != NULL)
                {
                    ListCell *arg_lc;
                    for (arg_lc = func_call->args->head; arg_lc != NULL; arg_lc = arg_lc->next)
                    {
                        /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
                        Node *arg = NULL;
                        ColumnRef *col_ref = NULL;
                        TeccColumnRefData *tecc_col = NULL;

                        arg = (Node *) arg_lc->data.ptr_value;
                        if (arg != NULL && nodeTag(arg) == FE_T_ColumnRef)
                        {
                            col_ref = (ColumnRef *) arg;

                            if (tecc_expand_column_ref_with_stmt_data(col_ref, &tecc_col, statement_data))

                            if (tecc_expand_column_ref_with_stmt_data(col_ref, &tecc_col, statement_data))
                            {
                                if (tecc_col != NULL)
                                {
                                    *columns = lappend(*columns, tecc_col);
                                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                                       "Added column reference from function argument: %s",
                                                       tecc_col->alias_fqdn);
                                }
                            }
                        }
                    }
                }
            }
            else if (nodeTag(res_target->val) == FE_T_A_Expr)
            {
                /* A_Expr (arithmetic expression, comparison, etc.) */
                A_Expr *a_expr = (A_Expr *) res_target->val;
                /* ReqID:TECC4.0 - Fix C90 mixed declaration warning - a_expr declared at block start */
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing A_Expr in target list (kind: %d)", a_expr->kind);

                /* Extract columns from left, right, and escape expressions */
                if (a_expr->lexpr != NULL)
                    stmt_extract_columns_from_expression(a_expr->lexpr, columns, statement_data);
                if (a_expr->rexpr != NULL)
                    stmt_extract_columns_from_expression(a_expr->rexpr, columns, statement_data);
                if (a_expr->escape != NULL)
                    stmt_extract_columns_from_expression(a_expr->escape, columns, statement_data);
            }
            else if (nodeTag(res_target->val) == FE_T_TypeCast)
            {
                /* Type cast expression */
                TypeCast *type_cast = (TypeCast *) res_target->val;
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing TypeCast in target list");

                /* Extract columns from the argument */
                if (type_cast->arg != NULL)
                    stmt_extract_columns_from_expression(type_cast->arg, columns, statement_data);
            }
            else if (nodeTag(res_target->val) == FE_T_SubLink)
            {
                /* ReqID:TECC4.0 - SubLink in target list (scalar subquery) */
                struct SubLink *sublink = (struct SubLink *)res_target->val;
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing SubLink in target list");
                stmt_extract_columns_from_expression((Node *)sublink, columns, statement_data);
            }
            else
            {
                /* Other expression types - use generic expression extractor */
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing other expression type in target list: %d", nodeTag(res_target->val));
                stmt_extract_columns_from_expression(res_target->val, columns, statement_data);
            }
        }
    }

    /* Extract columns from WHERE clause */
    if (select_stmt->whereClause != NULL)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Extracting columns from WHERE clause");

        /* Extract column references from WHERE clause expression */
        stmt_extract_columns_from_expression(select_stmt->whereClause, columns, statement_data);
    }

    /* ReqID:TECC4.0 - Extract columns from DISTINCT ON clause */
    if (select_stmt->distinctClause != NIL)
    {
        ListCell *lc;
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Extracting columns from DISTINCT ON clause");
        for (lc = list_head(select_stmt->distinctClause); lc != NULL; lc = lnext(lc))
        {
            Node *dist_expr = (Node *)lfirst(lc);
            if (dist_expr != NULL && nodeTag(dist_expr) != FE_T_Integer)
                stmt_extract_columns_from_expression(dist_expr, columns, statement_data);
        }
    }

    /* ReqID:TECC4.0 - Extract columns from ORDER BY (sortClause) */
    if (select_stmt->sortClause != NIL)
    {
        ListCell *lc;
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Extracting columns from ORDER BY clause");
        for (lc = list_head(select_stmt->sortClause); lc != NULL; lc = lnext(lc))
        {
            SortBy *sortby = (SortBy *)lfirst(lc);
            if (sortby != NULL && sortby->node != NULL)
                stmt_extract_columns_from_expression(sortby->node, columns, statement_data);
        }
    }

    /* Extract columns from FROM clause */
    if (select_stmt->fromClause != NULL) {
        ListCell *lc;
        for (lc = select_stmt->fromClause->head; lc != NULL; lc = lc->next)
        {
            Node *from_item = (Node *) lc->data.ptr_value;

            if (nodeTag(from_item) == FE_T_RangeVar)
            {
                RangeVar *range_var = (RangeVar *) from_item;
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "FROM clause table: %s.%s",
                                   range_var->schemaname ? range_var->schemaname : "(default)",
                                   range_var->relname ? range_var->relname : "(null)");

                /* 对于简单的表引用，我们无法提取具体的列信息，
                   但可以记录表名供后续元数据查询使用 */
                /* 列提取主要在targetList和WHERE子句中完成 */
            }
            else if (nodeTag(from_item) == FE_T_RangeSubselect)
            {
                /* ReqID:TECC4.0 - RangeSubselect in FROM clause */
                RangeSubselect *rss = (RangeSubselect *) from_item;
                if (rss->subquery != NULL && nodeTag(rss->subquery) == FE_T_SelectStmt)
                {
                    SelectStmt *subselect = (SelectStmt *) rss->subquery;
                    List *subselect_columns = NIL;
                    TeccStmtProcessorResult subselect_result;

                    subselect_result = stmt_extract_select_columns(subselect, &subselect_columns, statement_data);
                    if (subselect_result == TECC_STMT_PROCESSOR_SUCCESS)
                    {
                        ListCell *col_lc;
                        for (col_lc = subselect_columns->head; col_lc != NULL; col_lc = col_lc->next)
                        {
                            TeccColumnRefData *tecc_col = (TeccColumnRefData *) col_lc->data.ptr_value;
                            if (tecc_col != NULL)
                            {
                                *columns = lappend(*columns, tecc_col);
                            }
                        }
                        list_free(subselect_columns);
                    }
                }
            }
            else if (nodeTag(from_item) == FE_T_SelectStmt)
            {
                /* 子查询 - 递归提取列 */
                SelectStmt *subselect = (SelectStmt *) from_item;
                List *subselect_columns = NIL;
                TeccStmtProcessorResult subselect_result;

                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "FROM clause subselect detected");

                /* 递归提取子查询中的列 */
                subselect_result = stmt_extract_select_columns(subselect, &subselect_columns, statement_data);
                if (subselect_result == TECC_STMT_PROCESSOR_SUCCESS)
                {
                    /* 将子查询的列添加到主查询的列列表中 */
                    ListCell *col_lc;
                    for (col_lc = subselect_columns->head; col_lc != NULL; col_lc = col_lc->next)
                    {
                        TeccColumnRefData *tecc_col = (TeccColumnRefData *) col_lc->data.ptr_value;
                        if (tecc_col != NULL)
                        {
                            *columns = lappend(*columns, tecc_col);
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                               "Added column from subselect: %s",
                                               tecc_col->alias_fqdn);
                        }
                    }

                    /* 清理子查询列列表（列对象本身不释放，因为它们已添加到主列表） */
                    list_free(subselect_columns);
                }
                else
                {
                    tecc_processor_debug(TECC_LOG_WARNING,
                                       "Failed to extract columns from FROM subselect: %d", subselect_result);
                }
            }
            else
            {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "FROM clause item type: %d", nodeTag(from_item));
            }
        }
    }

    

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Extract column references from INSERT statement
 */
static TeccStmtProcessorResult
stmt_extract_insert_columns(const InsertStmt *insert_stmt, List **columns,
                           const TeccStatementData *statement_data)
{
    ListCell *lc;
    const char *table_name;

    if (insert_stmt == NULL || columns == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    /* Extract table name */
    table_name = insert_stmt->relation->relname;

    /* Extract columns from column list */
    if (insert_stmt->cols != NULL) {
        for (lc = insert_stmt->cols->head; lc != NULL; lc = lc->next)
        {
            Node *target_node = (Node *) lc->data.ptr_value;
            TeccColumnRefData *tecc_col;

            /* Check if this is a Const node */
            if (target_node == NULL || target_node->type == FE_T_Const) {
                /* It's a constant value or NULL, not a column reference */
                continue;
            }

            /* Otherwise, assume it's a ResTarget */
            /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
            {
                ResTarget *res_target = (ResTarget *) target_node;
                if (res_target->name == NULL)
                    continue;

                tecc_col = tecc_column_ref_data_create();
                if (tecc_col != NULL)
                {
                    tecc_column_ref_data_init(tecc_col, NULL, NULL, table_name, res_target->name);
                    *columns = lappend(*columns, tecc_col);
                }
            }
        }
    }

    /* Extract columns from VALUES clause */
    if (insert_stmt->selectStmt != NULL)
    {
        /* Extract columns from sub-select */
        /* ReqID:TECC4.0 - Recursively extract columns from sub-select */
        if (nodeTag(insert_stmt->selectStmt) == FE_T_SelectStmt)
        {
            SelectStmt *select_stmt = (SelectStmt *) insert_stmt->selectStmt;
            stmt_extract_select_columns(select_stmt, columns, statement_data);
        }
    }

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Extract column references from UPDATE statement
 */
static TeccStmtProcessorResult
stmt_extract_update_columns(const UpdateStmt *update_stmt, List **columns,
                           const TeccStatementData *statement_data)
{
    ListCell *lc;
    const char *table_name;

    if (update_stmt == NULL || columns == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    /* Extract table name */
    table_name = update_stmt->relation->relname;

    /* Extract columns from target list (SET clause) */
    if (update_stmt->targetList != NIL) {
        for (lc = update_stmt->targetList->head; lc != NULL; lc = lc->next)
        {
            Node *target_node = (Node *) lc->data.ptr_value;
            TeccColumnRefData *tecc_col;

            /* Check if this is a Const node */
            if (target_node == NULL || target_node->type == FE_T_Const) {
                /* It's a constant value or NULL, not a column reference */
                continue;
            }

            /* Otherwise, assume it's a ResTarget */
            /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
            {
                ResTarget *res_target = (ResTarget *) target_node;
                if (res_target->name == NULL)
                    continue;

                tecc_col = tecc_column_ref_data_create();
                if (tecc_col != NULL)
                {
                    tecc_column_ref_data_init(tecc_col, NULL, NULL, table_name, res_target->name);
                    *columns = lappend(*columns, tecc_col);
                }
            }
        }
    }

    /* Extract columns from WHERE clause */
    if (update_stmt->whereClause != NULL)
    {
        /* Begin - ReqID:TECC4.0 - UPDATE语句WHERE子句列提取实现 */
        List *where_columns = NIL;
        bool has_encrypted_columns = false;
        ListCell *lc;

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Extracting columns from UPDATE WHERE clause");

        /* Extract columns from WHERE clause expression */
        stmt_extract_columns_from_expression(update_stmt->whereClause, &where_columns, statement_data);

        /* Check if any extracted columns are encrypted */
        if (where_columns != NULL)
        {
            for (lc = where_columns->head; lc != NULL; lc = lc->next)
            {
                TeccColumnRefData *tecc_col = (TeccColumnRefData *) lc->data.ptr_value;
                if (tecc_col != NULL && tecc_col->is_encrypted)
                {
                    has_encrypted_columns = true;
                    
                }
            }

            /* If WHERE clause contains encrypted columns, log for debugging */
            if (has_encrypted_columns)
            {
                
            }

            /* Free the temporary column list */
            {
                ListCell *lc;
                for (lc = where_columns->head; lc != NULL; lc = lc->next)
                {
                    TeccColumnRefData *col_data = (TeccColumnRefData *)lc->data.ptr_value;
                    if (col_data != NULL)
                    {
                        tecc_column_ref_data_destroy(col_data);
                    }
                }
                list_free(where_columns);
            }
        }
        /* End - ReqID:TECC4.0 - UPDATE语句WHERE子句列提取实现 */
    }

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Extract column references from DELETE statement
 */
static TeccStmtProcessorResult
stmt_extract_delete_columns(const DeleteStmt *delete_stmt, List **columns,
                            const TeccStatementData *statement_data)
{
    const char *table_name;
    const char *schema_name;
    const char *catalog_name;

    if (delete_stmt == NULL || columns == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    /* Extract table information */
    if (delete_stmt->relation == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    table_name = delete_stmt->relation->relname;
    schema_name = delete_stmt->relation->schemaname;
    catalog_name = delete_stmt->relation->catalogname;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Extracting columns from DELETE statement for table: %s.%s.%s",
                       catalog_name ? catalog_name : "",
                       schema_name ? schema_name : "",
                       table_name);

    /* Extract columns from WHERE clause */
    if (delete_stmt->whereClause != NULL)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Extracting columns from DELETE WHERE clause");

        /* Use helper function to extract column references from WHERE clause expression */
        stmt_extract_columns_from_expression(delete_stmt->whereClause, columns, statement_data);

        /* Log extracted columns */
        if (*columns != NULL)
        {
            ListCell *lc;
            int col_count = 0;
            for (lc = (*columns)->head; lc != NULL; lc = lc->next)
            {
                TeccColumnRefData *tecc_col = (TeccColumnRefData *) lc->data.ptr_value;
                if (tecc_col != NULL)
                {
                    col_count++;
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                       "  Extracted column %d: %s (encrypted=%s)",
                                       col_count, tecc_col->alias_fqdn,
                                       tecc_col->is_encrypted ? "true" : "false");
                }
            }
            
        }
        else
        {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "No columns extracted from DELETE WHERE clause");
        }
    }
    else
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "DELETE statement has no WHERE clause");
    }

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Extract column references from CREATE TABLE statement
 */
static TeccStmtProcessorResult
stmt_extract_create_columns(const CreateStmt *create_stmt, List **columns)
{
    ListCell *lc;
    const char *table_name;
    const char *schema_name;
    const char *catalog_name;

    if (create_stmt == NULL || columns == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    /* Extract table information */
    if (create_stmt->relation == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    table_name = create_stmt->relation->relname;
    schema_name = create_stmt->relation->schemaname;
    catalog_name = create_stmt->relation->catalogname;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Extracting columns from CREATE TABLE statement for table: %s.%s.%s",
                        catalog_name ? catalog_name : "",
                        schema_name ? schema_name : "",
                        table_name);

    /* Extract columns from table elements */
    if (create_stmt->tableElts != NIL)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Processing table elements for CREATE TABLE");

        foreach(lc, create_stmt->tableElts)
        {
            Node *elem = (Node *) lfirst(lc);

            /* Check if this is a ColumnDef node */
            if (IsA(elem, ColumnDef))
            {
                ColumnDef *col_def = (ColumnDef *) elem;

                if (col_def->colname != NULL)
                {
                    TeccColumnRefData *tecc_col = tecc_column_ref_data_create();
                    if (tecc_col != NULL)
                    {
                        tecc_column_ref_data_init(tecc_col, catalog_name, schema_name, table_name, col_def->colname);
                        *columns = lappend(*columns, tecc_col);

                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                           "Extracted column: %s.%s.%s.%s",
                                           catalog_name ? catalog_name : "",
                                           schema_name ? schema_name : "",
                                           table_name,
                                           col_def->colname);
                    }
                }
            }
        }
    }
    else
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "No table elements found in CREATE TABLE statement");
    }

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Context for column extraction walker callback.
 */
typedef struct ColumnExtractCtx
{
    List **columns;
    const TeccStatementData *statement_data;
} ColumnExtractCtx;

/* ReqID:TECC4.0 - Pre-visit callback: extract ColumnRef into columns list */
static bool
stmt_extract_columns_pre_visit(Node *node, void *context)
{
    ColumnExtractCtx *ctx = (ColumnExtractCtx *) context;

    if (nodeTag(node) == FE_T_ColumnRef)
    {
        ColumnRef *col_ref = (ColumnRef *) node;
        TeccColumnRefData *tecc_col = NULL;

        if (tecc_expand_column_ref_with_stmt_data(col_ref, &tecc_col,
                                                  ctx->statement_data))
        {
            if (tecc_col != NULL)
            {
                *ctx->columns = lappend(*ctx->columns, tecc_col);
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Extracted column from expression: %s",
                                   tecc_col->alias_fqdn);
            }
        }
    }
    return true;
}

/*
 * Extract column references from expression using unified walker.
 */
static void
stmt_extract_columns_from_expression(const Node *expr, List **columns,
                                    const TeccStatementData *statement_data)
{
    ColumnExtractCtx ctx;
    TeccExprWalkerCallbacks callbacks;

    if (expr == NULL || columns == NULL)
        return;

    ctx.columns = columns;
    ctx.statement_data = statement_data;

    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.pre_visit = stmt_extract_columns_pre_visit;

    tecc_walk_expression_tree((Node *) expr, &callbacks, &ctx);
}

/*
 * Extract column references from COPY statement
 */
static TeccStmtProcessorResult
stmt_extract_copy_columns(const CopyStmt *copy_stmt, List **columns,
                          const TeccStatementData *statement_data)
{
    ListCell *lc;

    if (copy_stmt == NULL || columns == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Extracting columns from COPY statement (is_from=%s)",
                        copy_stmt->is_from ? "true" : "false");

    /* 1. Extract columns from attlist (explicit column list) */
    if (copy_stmt->attlist != NULL) {
        for (lc = copy_stmt->attlist->head; lc != NULL; lc = lc->next) {
            Node *node = (Node *) lc->data.ptr_value;
            if (node != NULL && nodeTag(node) == FE_T_String) {
                char *col_name = (char *) node;

                /* Create column reference data */
                TeccColumnRefData *col_data = tecc_column_ref_data_create();
                if (col_data == NULL) {
                    tecc_processor_debug(TECC_LOG_ERROR,
                                       "Failed to allocate memory for column reference");
                    continue;
                }

                /* Initialize with column name */
                tecc_column_ref_data_init(col_data,
                                         copy_stmt->relation ? copy_stmt->relation->relname : NULL,
                                         copy_stmt->relation ? copy_stmt->relation->schemaname : NULL,
                                         col_name,
                                         NULL);

                *columns = lappend(*columns, col_data);
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Added COPY column reference: %s",
                                   col_name);
            }
        }
    }

    /* 2. If no explicit column list and we have a relation, all columns are implied */
    else if (copy_stmt->relation != NULL) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "COPY has no explicit column list - all columns implied");
        /* TODO: When cache module is available, fetch all columns from table */
    }

    /* 3. If COPY has a SELECT query, extract columns from the query */
    else if (copy_stmt->query != NULL) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "COPY has SELECT query - columns will be extracted from query");
        /* Extract columns from SELECT query */
        if (nodeTag(copy_stmt->query) == FE_T_SelectStmt)
        {
            SelectStmt *select_stmt = (SelectStmt *) copy_stmt->query;
            TeccStmtProcessorResult select_result;

            select_result = stmt_extract_select_columns(select_stmt, columns, statement_data);
            if (select_result != TECC_STMT_PROCESSOR_SUCCESS)
            {
                tecc_processor_debug(TECC_LOG_WARNING,
                                   "Failed to extract columns from COPY SELECT query: %d", select_result);
            }
            else
            {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Successfully extracted columns from COPY SELECT query");
            }
        }
        else
        {
            tecc_processor_debug(TECC_LOG_WARNING,
                               "COPY query is not a SELECT statement (type: %d)", nodeTag(copy_stmt->query));
        }
    }

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Check if statement contains DML operations
 */
bool
tecc_stmt_processor_statement_has_dml(const Node *stmt)
{
    if (stmt == NULL)
        return false;

    switch (nodeTag(stmt))
    {
        case FE_T_SelectStmt:
        case FE_T_InsertStmt:
        case FE_T_UpdateStmt:
        case FE_T_DeleteStmt:
        case FE_T_CopyStmt:
            /* COPY is considered a DML operation for encryption purposes */
            return true;

        default:
            return false;
    }
}

/*
 * Check if statement references encrypted columns
 * This is a simplified version - actual implementation would check cache
 */
bool
tecc_stmt_processor_statement_has_encrypted_columns(const Node *stmt)
{
    if (stmt == NULL)
        return false;

    /* For now, we assume all DML statements might have encrypted columns */
    /* TODO: Implement proper cache-based checking when cache module is available */
    return tecc_stmt_processor_statement_has_dml(stmt);
}

/* ==================== Helper functions for metadata loading and checking ==================== */

/*
 * Load table metadata from cache
 * This is a simplified implementation for task 12
 * Returns encrypted columns list via encrypted_columns_ptr and column_count_ptr
 * Caller is responsible for freeing the encrypted columns list
 */
static bool
stmt_load_table_metadata(const RangeVar *range_var, const TeccStatementData *statement_data, TeccColumnCacheItem ***encrypted_columns_ptr, int *column_count_ptr)
{
    const char *table_name;
    const char *schema_name;
    TeccColumnCacheItem **encrypted_columns = NULL;
    int column_count = 0;
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    TeccState *state = NULL;
    const char *database_name = NULL;

    if (range_var == NULL || range_var->relname == NULL)
        return false;

    table_name = range_var->relname;
    schema_name = range_var->schemaname ? range_var->schemaname : "public";

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Loading metadata for table: %s.%s (range_var=%p, relname=%p, schemaname=%p)",
                        schema_name, table_name, range_var, range_var->relname, range_var->schemaname);

    /* Get TECC state from statement data */
    if (statement_data != NULL && statement_data->conn != NULL) {
        state = tecc_get_state(statement_data->conn);
        database_name = PQdb(statement_data->conn);
        if (state == NULL) {
            tecc_processor_debug(TECC_LOG_WARNING,
                                "Cannot get TECC state from connection, cache loading may fail");
        }
    } else {
        tecc_processor_debug(TECC_LOG_WARNING,
                            "Statement data or connection is NULL, cache loading may fail");
    }

    /* Query cache for all encrypted columns in this table */
    /* Use actual database name from connection; fallback to "postgres" if not available */
    encrypted_columns = tecc_cache_get_columns_by_table_name(state,
                                                            database_name ? database_name : "postgres",
                                                            schema_name,
                                                            table_name,
                                                            &column_count);

    /* ReqID:TECC4.0 - Cache miss fallback: prefer temp connection.
     *
     * A temp connection starts a fresh session with database defaults
     * (READ COMMITTED), avoiding SafeSnapshot blocking when the main
     * connection was set to SERIALIZABLE by a regression test.
     *
     * When state is not available, fall back to the main connection
     * (same behavior as before). */
    if ((encrypted_columns == NULL || column_count == 0) &&
        statement_data != NULL && statement_data->conn != NULL)
    {
        PGconn *mc = statement_data->conn;
        char qbuf[8192], *esc_t, *esc_s;
        PGresult *qr = NULL;
        int nr, ri;
        PGconn *exec_conn = NULL;
        bool used_temp_conn = false;

        esc_t = PQescapeLiteral(mc, table_name, strlen(table_name));
        esc_s = PQescapeLiteral(mc, schema_name, strlen(schema_name));
        if (esc_t != NULL && esc_s != NULL)
        {
            snprintf(qbuf, sizeof(qbuf),
                "SELECT column_attnum,column_name,dek_name,encryption_type,"
                "data_type_original_oid,data_type_original_mod,collation_locale,column_key_id "
                "FROM hg_encrypted_columns_info "
                "JOIN pg_class cls ON table_oid=cls.oid "
                "JOIN pg_namespace ns ON cls.relnamespace=ns.oid "
                "WHERE cls.relname=%s AND ns.nspname=%s "
                "AND current_database()='%s' ORDER BY column_attnum",
                esc_t, esc_s, database_name ? database_name : "postgres");
            PQfreemem(esc_t); PQfreemem(esc_s);

            /* Try temp connection first to avoid SafeSnapshot */
            if (state != NULL && state->conninfo != NULL)
            {
                static char env_buf[] = "PGENABLETECC=false";

                putenv(env_buf);
                exec_conn = PQconnectdb(state->conninfo);
                if (exec_conn != NULL && PQstatus(exec_conn) == CONNECTION_OK)
                {
                    exec_conn->enable_tecc = false;
                    used_temp_conn = true;
                }
                else if (exec_conn != NULL)
                {
                    PQfinish(exec_conn);
                    exec_conn = NULL;
                }
            }

            /* Fall back to main connection if temp connection failed */
            if (exec_conn == NULL)
            {
                exec_conn = mc;
                mc->enable_tecc = 0;
            }

            qr = PQexec(exec_conn, qbuf);

            if (used_temp_conn)
            {
                PQfinish(exec_conn);
                exec_conn = NULL;
            }
            else
            {
                mc->enable_tecc = 1; /* restore: saved_tecc is always 1 at this point */
            }

            if (qr != NULL && PQresultStatus(qr) == PGRES_TUPLES_OK &&
                (nr = PQntuples(qr)) > 0)
            {
                encrypted_columns = (TeccColumnCacheItem **)pg_malloc(nr * sizeof(TeccColumnCacheItem *));
                for (ri = 0; ri < nr; ri++)
                {
                    TeccColumnCacheItem *ci = (TeccColumnCacheItem *)pg_malloc0(sizeof(TeccColumnCacheItem));
                    ci->cache_status = TECC_CACHE_STATUS_ENCRYPTED;
                    ci->database_name = pg_strdup(database_name ? database_name : "postgres");
                    ci->schema_name = pg_strdup(schema_name);
                    ci->table_name = pg_strdup(table_name);
                    ci->column_attnum = atoi(PQgetvalue(qr, ri, 0));
                    ci->column_name = pg_strdup(PQgetvalue(qr, ri, 1));
                    ci->dek_name = pg_strdup(PQgetvalue(qr, ri, 2));
                    ci->encryption_type = atoi(PQgetvalue(qr, ri, 3));
                    ci->original_type_oid = atoi(PQgetvalue(qr, ri, 4));
                    ci->original_type_mod = atoi(PQgetvalue(qr, ri, 5));
                    {
                        const char *loc = PQgetvalue(qr, ri, 6);
                        if (!PQgetisnull(qr, ri, 6) && loc[0] != '\0')
                            ci->collation_locale = pg_strdup(loc);
                    }
                    ci->dek_id = atoi(PQgetvalue(qr, ri, 7));
                    encrypted_columns[ri] = ci;
                    column_count = nr;
                }
            }
            if (qr != NULL) PQclear(qr);
        }
        else
        {
            if (esc_t != NULL) PQfreemem(esc_t);
            if (esc_s != NULL) PQfreemem(esc_s);
        }

        /* Ensure enable_tecc is restored if we used main connection */
        if (!used_temp_conn && exec_conn == NULL)
            mc->enable_tecc = 1;
    }

    if (encrypted_columns != NULL && column_count > 0)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Found %d encrypted columns in table %s.%s",
                           column_count, schema_name, table_name);

        /* Return encrypted columns list to caller */
        if (encrypted_columns_ptr != NULL) {
            *encrypted_columns_ptr = encrypted_columns;
        }
        if (column_count_ptr != NULL) {
            *column_count_ptr = column_count;
        }
    }
    else
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "No encrypted columns found in table %s.%s",
                           schema_name, table_name);

        /* Return NULL/0 to caller */
        if (encrypted_columns_ptr != NULL) {
            *encrypted_columns_ptr = NULL;
        }
        if (column_count_ptr != NULL) {
            *column_count_ptr = 0;
        }

        /* Free memory if allocated */
        if (encrypted_columns != NULL) {
            pg_free(encrypted_columns);
        }
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Finished loading metadata for table: %s.%s",
                        schema_name, table_name);

    return true;
}

/*
 * Check if a column is encrypted by querying the metadata cache
 */
static bool
stmt_check_column_encrypted(const TeccStatementData *statement_data, const char *table_name, const char *column_name)
{
    bool is_encrypted = false;

    if (table_name == NULL || column_name == NULL)
        return false;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Checking if column %s.%s is encrypted",
                        table_name, column_name);

    /* Query cache for encrypted column metadata */
    is_encrypted = stmt_query_metadata_cache_simple(statement_data, table_name, column_name);

    if (is_encrypted) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Column %s.%s is encrypted",
                           table_name, column_name);
    } else {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Column %s.%s is not encrypted or not found in cache",
                           table_name, column_name);
    }

    return is_encrypted;
}

/*
 * Check if a column is in the encrypted columns list
 */
static bool
stmt_is_column_in_encrypted_list(const char *column_name, TeccColumnCacheItem **encrypted_columns, int column_count)
{
    if (column_name == NULL || encrypted_columns == NULL || column_count <= 0)
        return false;

    for (int i = 0; i < column_count; i++)
    {
        if (encrypted_columns[i] != NULL && encrypted_columns[i]->column_name != NULL)
        {
            if (strcmp(encrypted_columns[i]->column_name, column_name) == 0)
            {
                return true;
            }
        }
    }

    return false;
}

/*
 * Query metadata cache for encrypted column metadata (simplified version)
 * This function queries the cache for a specific column in a table
 * Returns true if the column is encrypted, false otherwise
 */
static bool
stmt_query_metadata_cache_simple(const TeccStatementData *statement_data, const char *table_name, const char *column_name)
{
    TeccColumnCacheItem **encrypted_columns = NULL;
    int column_count = 0;
    bool is_encrypted = false;
    TeccState *state = NULL;
    const char *db_name = NULL;
    const char *schema_name = NULL;

    if (table_name == NULL || column_name == NULL || statement_data == NULL)
        return false;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Querying metadata cache for column %s.%s",
                        table_name, column_name);

    /* Debug: Check statement_data and conn */
    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "statement_data=%p, statement_data->conn=%p",
                        statement_data, statement_data->conn);

    /* Get TeccState from PGconn */
    if (statement_data->conn != NULL) {
        state = tecc_get_state(statement_data->conn);
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Got TeccState from connection: %p", state);
    } else {
        tecc_processor_debug(TECC_LOG_WARNING,
                           "statement_data->conn is NULL, cannot get TeccState");
    }

    if (state == NULL) {
        tecc_processor_debug(TECC_LOG_WARNING,
                           "Cannot get TeccState from connection, cache query will fail");
        return false;
    }

    /* Get database name from connection */
    if (statement_data->conn != NULL) {
        db_name = PQdb(statement_data->conn);
        if (db_name == NULL || db_name[0] == '\0') {
            tecc_processor_debug(TECC_LOG_WARNING,
                               "Could not get database name from connection, using default 'postgres'");
            db_name = "postgres";
        }
    } else {
        db_name = "postgres";
    }

    /* Get current schema from cache manager */
    schema_name = tecc_cache_get_guc_param(state, "current_schema");
    if (schema_name == NULL || schema_name[0] == '\0') {
        tecc_processor_debug(TECC_LOG_WARNING,
                           "Could not get current schema from cache manager, using default 'public'");
        schema_name = "public";
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Using database='%s', schema='%s', table='%s' for metadata query",
                        db_name, schema_name, table_name);

    /* Query cache for all encrypted columns in this table */
    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "DEBUG LOOP: Calling tecc_cache_get_columns_by_table_name for %s.%s.%s",
                        db_name, schema_name, table_name);
    encrypted_columns = tecc_cache_get_columns_by_table_name(state,
                                                            db_name,
                                                            schema_name,
                                                            table_name,
                                                            &column_count);

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "DEBUG LOOP: tecc_cache_get_columns_by_table_name returned: encrypted_columns=%p, column_count=%d",
                        encrypted_columns, column_count);

    if (encrypted_columns != NULL && column_count > 0)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "DEBUG LOOP: Found %d encrypted columns, searching for column '%s'",
                           column_count, column_name);
        /* Search for the specific column */
        for (int i = 0; i < column_count; i++)
        {
            tecc_processor_debug(TECC_LOG_DEBUG2,
                               "DEBUG LOOP: Checking column[%d]: %s",
                               i, encrypted_columns[i] ? (encrypted_columns[i]->column_name ? encrypted_columns[i]->column_name : "(null)") : "(null)");
            if (encrypted_columns[i] != NULL &&
                encrypted_columns[i]->column_name != NULL &&
                strcmp(encrypted_columns[i]->column_name, column_name) == 0)
            {
                /* Found the column - it's encrypted */
                is_encrypted = true;
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Column %s.%s is encrypted (dek_id=%u, encryption_type=%d)",
                                   table_name, column_name,
                                   encrypted_columns[i]->dek_id,
                                   encrypted_columns[i]->encryption_type);
                break;
            }
        }

        /* Free the array (items are owned by cache) */
        pg_free(encrypted_columns);
    }

    return is_encrypted;
}
/* End - ReqID:TECC4.0 */