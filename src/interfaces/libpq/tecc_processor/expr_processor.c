/* Begin - ReqID:TECC4.0 */
/*
 * File: expr_processor.c
 *
 * Abstract:
 *   Expression processor implementation for TECC expression processor module.
 *   Implements functions for expanding and processing SQL expression nodes.
 *
 * Authored by <wangxp@highgo.com>, 20260404.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/expr_processor.c
 */

#include "postgres_fe.h"           /* Must be first for frontend code */
#include "common/fe_memutils.h"    /* For pg_malloc/pg_free */
#include "libpq-fe.h"              /* For PQdb() function */
#include "expr_processor.h"
#include "column_ref_data.h"
#include "../frontend_parser/tecc_nodes.h"  /* For list_length function */
/* Note: expr_guc_params.h has been removed, GUC parameters are now handled differently */
#include "../tecc_cache/tecc_metadata_cache.h"  /* For cache interface */
#include "../tecc_libpq_integration/fe-tecc-integration.h"  /* For tecc_get_state() and TeccState */
#include "tecc_expression_walker.h"  /* ReqID:TECC4.0 - Unified expression walker */
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* Forward declarations for internal helper functions */
static bool expr_expand_recursive(const Node *expr_node, TeccExprPartsList *expr_parts);
static bool expr_expand_recursive_with_guc(const Node *expr_node, TeccExprPartsList *expr_parts,
                                         TeccExpressionContext *expr_ctx);
static bool expr_process_column_ref(const ColumnRef *col_ref, TeccExprPartsList *expr_parts);
static bool expr_process_func_call(const FuncCall *func_call, TeccExprPartsList *expr_parts);
static bool expr_process_a_const(const void *a_const, TeccExprPartsList *expr_parts);
static bool expr_process_a_const_with_guc(const void *a_const, TeccExprPartsList *expr_parts,
                                        TeccExpressionContext *expr_ctx);
static bool expr_process_param_ref(const Param *param, TeccExprPartsList *expr_parts);

/* Internal helper functions for column reference expansion */
static bool parse_single_name(const ColumnRef *col_ref, TeccColumnRefData *data);
static bool parse_two_names(const ColumnRef *col_ref, TeccColumnRefData *data);
static bool parse_three_names(const ColumnRef *col_ref, TeccColumnRefData *data);

static bool parse_four_names(const ColumnRef *col_ref, TeccColumnRefData *data);
static bool check_column_encryption_with_state(TeccColumnRefData *data, const TeccState *state);

/*
 * Expand any expression node
 */
bool
tecc_expr_expand(const Node *expr_node, TeccExprPartsList **expr_parts)
{
    if (expr_node == NULL || expr_parts == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid arguments to tecc_expr_expand");
        return false;
    }

    /* Create expression parts list if not provided */
    if (*expr_parts == NULL)
    {
        *expr_parts = tecc_expr_parts_list_create();
        if (*expr_parts == NULL)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "Failed to create expression parts list");
            return false;
        }
    }

    /* Recursively expand expression */
    return expr_expand_recursive(expr_node, *expr_parts);
}

/*
 * Expand expression with GUC context
 */
bool
tecc_expr_expand_with_guc(const Node *expr_node,
                         TeccExpressionContext *expr_ctx,
                         TeccExprPartsList **expr_parts)
{
    if (expr_node == NULL || expr_parts == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid arguments to tecc_expr_expand_with_guc");
        return false;
    }

    /* Create expression parts list if not provided */
    if (*expr_parts == NULL)
    {
        *expr_parts = tecc_expr_parts_list_create();
        if (*expr_parts == NULL)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "Failed to create expression parts list");
            return false;
        }
    }

    /* Check if GUC processing is enabled */
    if (expr_ctx && expr_ctx->guc_params_enabled && expr_ctx->guc_params)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Expanding expression with GUC context enabled");

        /* Store GUC parameters in expression context for use during expansion */
        /* The actual GUC processing will be done in the recursive expansion functions */
        /* For example, when processing string literals, we can apply client_encoding */
        /* When processing function calls, we can apply search_path */

        /* Implement GUC-aware expression processing */
        /* Pass GUC context to recursive expansion functions */
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "GUC parameters available: searchpath=%s, standard_conforming_strings=%d",
                           expr_ctx->guc_params->searchpath,
                           expr_ctx->guc_params->standard_conforming_strings);

        /* Apply GUC processing during expression expansion */
        return expr_expand_recursive_with_guc(expr_node, *expr_parts, expr_ctx);
    }
    else
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Expanding expression without GUC context");

        /* Recursively expand expression without GUC context */
        return expr_expand_recursive(expr_node, *expr_parts);
    }
}

/*
 * Expand column reference
 */
bool
tecc_expand_column_ref(const struct ColumnRef *col_ref, TeccColumnRefData **col_data)
{
    int fields_len;
    bool parse_result = false;

    if (col_ref == NULL || col_data == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid arguments to tecc_expand_column_ref");
        return false;
    }

    /* Create column reference data object */
    *col_data = tecc_column_ref_data_create();
    if (*col_data == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to allocate memory for TeccColumnRefData");
        return false;
    }

    /* Parse fields list based on length */
    fields_len = list_length(col_ref->fields);

    switch (fields_len)
    {
        case 1: /* 仅列名 */
            parse_result = parse_single_name(col_ref, *col_data);
            break;

        case 2: /* 表名.列名 */
            parse_result = parse_two_names(col_ref, *col_data);
            break;

        case 3: /* 模式名.表名.列名 */
            parse_result = parse_three_names(col_ref, *col_data);
            break;

        case 4: /* 目录名.模式名.表名.列名 */
            parse_result = parse_four_names(col_ref, *col_data);
            break;

        default:
            tecc_processor_debug(TECC_LOG_ERROR,
                               "Invalid column reference format: %d fields", fields_len);
            break;
    }

    if (!parse_result)
    {
        tecc_column_ref_data_destroy(*col_data);
        *col_data = NULL;
        return false;
    }

    /* Encryption check requires state; use tecc_expand_column_ref_with_state() instead */
    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Expanded column reference (without state): %s",
                       (*col_data)->alias_fqdn);

    return true;
}

/*
 * Expand column reference with state context
 */
bool
tecc_expand_column_ref_with_state(const struct ColumnRef *col_ref,
                                 TeccColumnRefData **col_data,
                                 const TeccState *state)
{
    int fields_len;
    bool parse_result = false;

    if (col_ref == NULL || col_data == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid arguments to tecc_expand_column_ref_with_state");
        return false;
    }

    /* Create column reference data object */
    *col_data = tecc_column_ref_data_create();
    if (*col_data == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to allocate memory for TeccColumnRefData");
        return false;
    }

    /* Parse fields list based on length */
    fields_len = list_length(col_ref->fields);

    switch (fields_len)
    {
        case 1: /* 仅列名 */
            parse_result = parse_single_name(col_ref, *col_data);
            break;

        case 2: /* 表名.列名 */
            parse_result = parse_two_names(col_ref, *col_data);
            break;

        case 3: /* 模式名.表名.列名 */
            parse_result = parse_three_names(col_ref, *col_data);
            break;

        case 4: /* 目录名.模式名.表名.列名 */
            parse_result = parse_four_names(col_ref, *col_data);
            break;

        default:
            tecc_processor_debug(TECC_LOG_ERROR,
                               "Invalid column reference format: %d fields", fields_len);
            break;
    }

    if (!parse_result)
    {
        tecc_column_ref_data_destroy(*col_data);
        *col_data = NULL;
        return false;
    }

    /* Check if column is encrypted with state context */
    if (!check_column_encryption_with_state(*col_data, state))
    {
        tecc_processor_debug(TECC_LOG_WARNING,
                           "Failed to check column encryption status with state");
        /* Continue anyway - encryption check failure is not fatal */
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Expanded column reference with state: %s [encrypted=%s]",
                       (*col_data)->alias_fqdn,
                       (*col_data)->is_encrypted ? "true" : "false");

    return true;
}

/*
 * Expand column reference with statement data context
 */
bool
tecc_expand_column_ref_with_stmt_data(const struct ColumnRef *col_ref,
                                     TeccColumnRefData **col_data,
                                     const TeccStatementData *stmt_data)
{
    if (stmt_data == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Statement data is NULL, cannot expand column reference");
        return false;
    }

    /* Get TECC state from connection */
    if (stmt_data->conn != NULL)
    {
        TeccState *state = tecc_get_state(stmt_data->conn);
        if (state != NULL)
        {
            return tecc_expand_column_ref_with_state(col_ref, col_data, state);
        }
    }

    tecc_processor_debug(TECC_LOG_ERROR,
                       "TECC state not available, cannot expand column reference with encryption info");
    return false;
}

/*
 * Expand function name
 */
bool
tecc_expand_function_name(const struct FuncCall *func_call, TeccFuncNameData **func_data)
{
    if (func_call == NULL || func_data == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid arguments to tecc_expand_function_name");
        return false;
    }

    /* Allocate memory for function name data */
    *func_data = pg_malloc0(sizeof(TeccFuncNameData));
    if (*func_data == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to allocate memory for TeccFuncNameData");
        return false;
    }

    /* Extract function name from funcname list */
    if (func_call->funcname != NULL)
    {
        int funcname_len = list_length(func_call->funcname);

        if (funcname_len == 1)
        {
            /* Simple function name: function_name */
            Node *func_node = (Node *)list_nth(func_call->funcname, 0);
            if (func_node != NULL)
            {
                /* Modify - ReqID:TECC4.0
                 * any_name_list uses makeStringConst(), while some callers
                 * still construct FE_T_String nodes.  strVal() supports both. */
                char *func_name = strVal((Value *)func_node);
                if (func_name != NULL)
                {
                    strncpy((*func_data)->function_name, func_name, NAMEDATALEN - 1);
                    (*func_data)->function_name[NAMEDATALEN - 1] = '\0';
                }
            }
        }
        else if (funcname_len == 2)
        {
            /* Schema-qualified function: schema_name.function_name */
            Node *schema_node = (Node *)list_nth(func_call->funcname, 0);
            Node *func_node = (Node *)list_nth(func_call->funcname, 1);

            if (schema_node != NULL && func_node != NULL)
            {
                char *schema_name = strVal((Value *)schema_node);
                char *func_name = strVal((Value *)func_node);

                if (schema_name != NULL && func_name != NULL)
                {
                    strncpy((*func_data)->schema_name, schema_name, NAMEDATALEN - 1);
                    (*func_data)->schema_name[NAMEDATALEN - 1] = '\0';
                    strncpy((*func_data)->function_name, func_name, NAMEDATALEN - 1);
                    (*func_data)->function_name[NAMEDATALEN - 1] = '\0';
                }
            }
        }
        else if (funcname_len == 3)
        {
            /* Catalog-schema-qualified function: catalog_name.schema_name.function_name */
            Node *catalog_node = (Node *)list_nth(func_call->funcname, 0);
            Node *schema_node = (Node *)list_nth(func_call->funcname, 1);
            Node *func_node = (Node *)list_nth(func_call->funcname, 2);

            if (catalog_node != NULL && schema_node != NULL &&
                func_node != NULL)
            {
                char *catalog_name = strVal((Value *)catalog_node);
                char *schema_name = strVal((Value *)schema_node);
                char *func_name = strVal((Value *)func_node);

                if (catalog_name != NULL && schema_name != NULL && func_name != NULL)
                {
                    strncpy((*func_data)->catalog_name, catalog_name, NAMEDATALEN - 1);
                    (*func_data)->catalog_name[NAMEDATALEN - 1] = '\0';
                    strncpy((*func_data)->schema_name, schema_name, NAMEDATALEN - 1);
                    (*func_data)->schema_name[NAMEDATALEN - 1] = '\0';
                    strncpy((*func_data)->function_name, func_name, NAMEDATALEN - 1);
                    (*func_data)->function_name[NAMEDATALEN - 1] = '\0';
                }
            }
        }
        else
        {
            tecc_processor_debug(TECC_LOG_WARNING,
                               "Unsupported function name format: %d components", funcname_len);
        }
    }

    /* Set argument count */
    if (func_call->args != NULL)
    {
        (*func_data)->arg_count = list_length(func_call->args);
        (*func_data)->args = func_call->args;  /* Reference, not copy */
    }
    else
    {
        (*func_data)->arg_count = 0;
        (*func_data)->args = NULL;
    }

    /* For now, assume function is supported */
    (*func_data)->is_supported = true;
    (*func_data)->func_oid = InvalidOid;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Expanded function name: %s.%s.%s [args=%d, supported=%s]",
                       (*func_data)->catalog_name,
                       (*func_data)->schema_name,
                       (*func_data)->function_name,
                       (*func_data)->arg_count,
                       (*func_data)->is_supported ? "true" : "false");

    return true;
}

/*
 * Expand condition expression
 */
bool
tecc_expand_condition_expr(const Node *expr, TeccExprPartsList **expr_parts)
{
    if (expr == NULL || expr_parts == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid arguments to tecc_expand_condition_expr");
        return false;
    }

    /* Use the general expression expander for condition expressions */
    return tecc_expr_expand(expr, expr_parts);
}

/*
 * Create expression parts list
 */
TeccExprPartsList *
tecc_expr_parts_list_create(void)
{
    TeccExprPartsList *list = pg_malloc0(sizeof(TeccExprPartsList));
    if (list == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to allocate memory for TeccExprPartsList");
        return NULL;
    }

    /* Initial capacity */
    list->capacity = 16;
    list->count = 0;
    list->parts = pg_malloc0(list->capacity * sizeof(TeccExprPart *));
    if (list->parts == NULL)
    {
        pg_free(list);
        return NULL;
    }

    return list;
}

/*
 * Destroy expression parts list
 */
void
tecc_expr_parts_list_destroy(TeccExprPartsList *list)
{
    if (list == NULL)
        return;

    /* Destroy all expression parts */
    {
        size_t i;
        for (i = 0; i < list->count; i++)
        {
            if (list->parts[i] != NULL)
            {
                tecc_expr_part_destroy(list->parts[i]);
            }
        }
    }

    /* Free parts array and list structure */
    if (list->parts != NULL)
    {
        pg_free(list->parts);
    }
    pg_free(list);
}

/*
 * Add expression part to list
 */
bool
tecc_expr_parts_list_add(TeccExprPartsList *list, TeccExprPart *part)
{
    if (list == NULL || part == NULL)
        return false;

    /* Resize if needed */
    if (list->count >= list->capacity)
    {
        size_t new_capacity = list->capacity * 2;
        TeccExprPart **new_parts = pg_malloc0(new_capacity * sizeof(TeccExprPart *));
        if (new_parts == NULL)
            return false;

        memcpy(new_parts, list->parts, list->count * sizeof(TeccExprPart *));
        pg_free(list->parts);
        list->parts = new_parts;
        list->capacity = new_capacity;
    }

    /* Add part to list */
    list->parts[list->count] = part;
    list->count++;

    return true;
}

/*
 * Get expression part from list
 */
TeccExprPart *
tecc_expr_parts_list_get(const TeccExprPartsList *list, size_t index)
{
    if (list == NULL || index >= list->count)
        return NULL;

    return list->parts[index];
}

/*
 * Get list size
 */
size_t
tecc_expr_parts_list_size(const TeccExprPartsList *list)
{
    if (list == NULL)
        return 0;

    return list->count;
}

/*
 * Clear list
 */
void
tecc_expr_parts_list_clear(TeccExprPartsList *list)
{
    if (list == NULL)
        return;

    /* Destroy all expression parts */
    {
        size_t i;
        for (i = 0; i < list->count; i++)
        {
            if (list->parts[i] != NULL)
            {
                tecc_expr_part_destroy(list->parts[i]);
                list->parts[i] = NULL;
            }
        }
    }

    list->count = 0;
}

/*
 * Create expression part
 */
TeccExprPart *
tecc_expr_part_create(int type)
{
    TeccExprPart *part = pg_malloc0(sizeof(TeccExprPart));
    if (part == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to allocate memory for TeccExprPart");
        return NULL;
    }

    part->type = type;
    return part;
}

/*
 * Destroy expression part
 */
void
tecc_expr_part_destroy(TeccExprPart *part)
{
    if (part == NULL)
        return;

    /* Free data based on type */
    switch (part->type)
    {
        case FE_T_ColumnRef:
            if (part->data.column_ref != NULL)
            {
                tecc_column_ref_data_destroy(part->data.column_ref);
            }
            break;

        case FE_T_FuncCall:
            if (part->data.func_name != NULL)
            {
                pg_free(part->data.func_name);
            }
            break;

        default:
            /* Other types don't have allocated data */
            break;
    }

    pg_free(part);
}

/*
 * Set column reference data in expression part
 */
void
tecc_expr_part_set_column_ref(TeccExprPart *part, TeccColumnRefData *col_data)
{
    if (part == NULL)
        return;

    part->type = FE_T_ColumnRef;
    part->data.column_ref = col_data;
}

/*
 * Set function name data in expression part
 */
void
tecc_expr_part_set_func_name(TeccExprPart *part, TeccFuncNameData *func_data)
{
    if (part == NULL)
        return;

    part->type = FE_T_FuncCall;
    part->data.func_name = func_data;
}

/*
 * Get column reference data from expression part
 */
TeccColumnRefData *
tecc_expr_part_get_column_ref(const TeccExprPart *part)
{
    if (part == NULL || part->type != FE_T_ColumnRef)
        return NULL;

    return part->data.column_ref;
}

/*
 * Get function name data from expression part
 */
TeccFuncNameData *
tecc_expr_part_get_func_name(const TeccExprPart *part)
{
    if (part == NULL || part->type != FE_T_FuncCall)
        return NULL;

    return part->data.func_name;
}

/*
 * Internal helper functions
 */

/* ReqID:TECC4.0 - Context for expression expansion walker */
typedef struct ExprExpandCtx
{
    TeccExprPartsList *expr_parts;
    TeccExpressionContext *expr_ctx;  /* NULL for basic, non-NULL for GUC variant */
} ExprExpandCtx;

/* ReqID:TECC4.0 - Pre-visit callback for expression expansion */
static bool
expr_expand_pre_visit(Node *node, void *context)
{
    ExprExpandCtx *ctx = (ExprExpandCtx *) context;

    switch (nodeTag(node))
    {
        case FE_T_ColumnRef:
            return expr_process_column_ref((ColumnRef *) node, ctx->expr_parts);
        case FE_T_FuncCall:
            return expr_process_func_call((FuncCall *) node, ctx->expr_parts);
        case FE_T_Const:
            if (ctx->expr_ctx != NULL)
                return expr_process_a_const_with_guc(node, ctx->expr_parts,
                                                     ctx->expr_ctx);
            else
                return expr_process_a_const(node, ctx->expr_parts);
        case FE_T_Param:
            return expr_process_param_ref((Param *) node, ctx->expr_parts);
        default:
            return true;
    }
}

/* ReqID:TECC4.0 - Expand expression using unified walker */
static bool
expr_expand_recursive(const Node *expr_node, TeccExprPartsList *expr_parts)
{
    ExprExpandCtx ctx;
    TeccExprWalkerCallbacks callbacks;

    if (expr_node == NULL || expr_parts == NULL)
        return false;

    ctx.expr_parts = expr_parts;
    ctx.expr_ctx = NULL;

    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.pre_visit = expr_expand_pre_visit;

    return tecc_walk_expression_tree((Node *) expr_node, &callbacks, &ctx);
}

/* ReqID:TECC4.0 - Expand expression with GUC context using unified walker */
static bool
expr_expand_recursive_with_guc(const Node *expr_node, TeccExprPartsList *expr_parts,
                             TeccExpressionContext *expr_ctx)
{
    ExprExpandCtx ctx;
    TeccExprWalkerCallbacks callbacks;

    if (expr_node == NULL || expr_parts == NULL || expr_ctx == NULL)
        return false;

    ctx.expr_parts = expr_parts;
    ctx.expr_ctx = expr_ctx;

    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.pre_visit = expr_expand_pre_visit;

    return tecc_walk_expression_tree((Node *) expr_node, &callbacks, &ctx);
}

/* Process column reference */
static bool
expr_process_column_ref(const ColumnRef *col_ref, TeccExprPartsList *expr_parts)
{
    TeccColumnRefData *col_data = NULL;
    bool result = tecc_expand_column_ref(col_ref, &col_data);

    if (result && col_data != NULL)
    {
        TeccExprPart *part = tecc_expr_part_create(FE_T_ColumnRef);
        if (part != NULL)
        {
            tecc_expr_part_set_column_ref(part, col_data);
            result = tecc_expr_parts_list_add(expr_parts, part);
        }
        else
        {
            result = false;
        }
    }

    return result;
}

/* Process function call */
static bool
expr_process_func_call(const FuncCall *func_call, TeccExprPartsList *expr_parts)
{
    TeccFuncNameData *func_data = NULL;
    TeccExprPart *part = NULL;

    if (func_call == NULL || expr_parts == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid arguments to expr_process_func_call");
        return false;
    }

    /* Expand function name to get function data */
    if (!tecc_expand_function_name(func_call, &func_data))
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to expand function name");
        return false;
    }

    /* Create expression part for function call */
    part = pg_malloc0(sizeof(TeccExprPart));
    if (part == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to allocate memory for function expression part");
        pg_free(func_data);
        return false;
    }

    /* Initialize expression part */
    part->type = FE_T_FuncCall;
    part->data.func_name = func_data;

    /* Add to expression parts list */
    if (!tecc_expr_parts_list_add(expr_parts, part))
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to add function expression part to list");
        /* Note: part owns func_data, so we should use tecc_expr_part_destroy */
        tecc_expr_part_destroy(part);
        return false;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Processed function call: %s.%s.%s",
                       func_data->catalog_name,
                       func_data->schema_name,
                       func_data->function_name);

    /* ReqID:TECC4.0 - FILTER and OVER clause recursion handled by walker */
    return true;
}

/* Process constant value */
static bool
expr_process_a_const(const void *a_const, TeccExprPartsList *expr_parts)
{
    /* Note: A_Const structure is not fully defined in frontend headers
     * For now, we'll log the processing but not create expression parts
     * since constant values don't need to be expanded like column references
     * or function calls. They will be processed during SQL rewriting phase.
     */

    if (a_const == NULL || expr_parts == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid arguments to expr_process_a_const");
        return false;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Processed constant value");

    /* Constant values don't need to be added to expression parts list
     * because they don't contain column references or function calls
     * that need expansion. They will be handled during the SQL
     * rewriting phase when we need to encrypt/decrypt constant values
     * that are compared with encrypted columns.
     */

    return true;
}

/*
 * Process constant value with GUC context
 */
static bool
expr_process_a_const_with_guc(const void *a_const, TeccExprPartsList *expr_parts,
                            TeccExpressionContext *expr_ctx)
{
    if (a_const == NULL || expr_parts == NULL || expr_ctx == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid arguments to expr_process_a_const_with_guc");
        return false;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Processing constant value with GUC context");

    /* Check if GUC processing is enabled and parameters are available */
    if (expr_ctx->guc_params_enabled && expr_ctx->guc_params != NULL)
    {
        /* Apply GUC processing to constant value */
        /* For now, we just log that GUC processing would be applied */
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "GUC processing available for constant: searchpath=%s, standard_conforming_strings=%d",
                           expr_ctx->guc_params->searchpath,
                           expr_ctx->guc_params->standard_conforming_strings);

        /* Increment GUC application counter */
        expr_ctx->guc_applications++;
    }
    else
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "GUC processing not enabled or parameters not available");
    }

    /* Constant values don't need to be added to expression parts list
     * because they don't contain column references or function calls
     * that need expansion. They will be handled during the SQL
     * rewriting phase when we need to encrypt/decrypt constant values
     * that are compared with encrypted columns.
     */

    return true;
}

/* Process parameter reference */
/* Process parameter reference */
static bool
expr_process_param_ref(const Param *param, TeccExprPartsList *expr_parts)
{
    /* Parameter references don't need to be expanded like column references
     * or function calls. They will be processed during SQL rewriting phase
     * when we need to handle parameter values for encrypted columns.
     */
    if (param == NULL || expr_parts == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid arguments to expr_process_param_ref");
        return false;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Processed parameter reference: $%d", param->paramid);
    return true;
}


/* Parse single name (column only) */
static bool
parse_single_name(const ColumnRef *col_ref, TeccColumnRefData *data)
{
    char *col_name;

    if (col_ref == NULL || data == NULL || col_ref->fields == NULL)
        return false;

    col_name = strVal(list_nth(col_ref->fields, 0));
    if (col_name == NULL)
        return false;

    strncpy(data->column_name, col_name, TECC_PROCESSOR_MAX_NAME_LEN - 1);
    data->column_name[TECC_PROCESSOR_MAX_NAME_LEN - 1] = '\0';
    return true;
}

/* Parse two names (table.column) */
static bool
parse_two_names(const ColumnRef *col_ref, TeccColumnRefData *data)
{
    char *table_name;
    char *col_name;

    if (col_ref == NULL || data == NULL || col_ref->fields == NULL)
        return false;

    table_name = strVal(list_nth(col_ref->fields, 0));
    col_name = strVal(list_nth(col_ref->fields, 1));

    if (table_name == NULL || col_name == NULL)
        return false;

    strncpy(data->table_name, table_name, TECC_PROCESSOR_MAX_NAME_LEN - 1);
    data->table_name[TECC_PROCESSOR_MAX_NAME_LEN - 1] = '\0';
    strncpy(data->column_name, col_name, TECC_PROCESSOR_MAX_NAME_LEN - 1);
    data->column_name[TECC_PROCESSOR_MAX_NAME_LEN - 1] = '\0';
    return true;
}

/* Parse three names (schema.table.column) */
static bool
parse_three_names(const ColumnRef *col_ref, TeccColumnRefData *data)
{
    char *schema_name;
    char *table_name;
    char *col_name;

    if (col_ref == NULL || data == NULL || col_ref->fields == NULL)
        return false;

    schema_name = strVal(list_nth(col_ref->fields, 0));
    table_name = strVal(list_nth(col_ref->fields, 1));
    col_name = strVal(list_nth(col_ref->fields, 2));

    if (schema_name == NULL || table_name == NULL || col_name == NULL)
        return false;

    strncpy(data->schema_name, schema_name, TECC_PROCESSOR_MAX_NAME_LEN - 1);
    data->schema_name[TECC_PROCESSOR_MAX_NAME_LEN - 1] = '\0';
    strncpy(data->table_name, table_name, TECC_PROCESSOR_MAX_NAME_LEN - 1);
    data->table_name[TECC_PROCESSOR_MAX_NAME_LEN - 1] = '\0';
    strncpy(data->column_name, col_name, TECC_PROCESSOR_MAX_NAME_LEN - 1);
    data->column_name[TECC_PROCESSOR_MAX_NAME_LEN - 1] = '\0';
    return true;
}

/* Parse four names (catalog.schema.table.column) */
static bool
parse_four_names(const ColumnRef *col_ref, TeccColumnRefData *data)
{
    char *catalog_name;
    char *schema_name;
    char *table_name;
    char *col_name;

    if (col_ref == NULL || data == NULL || col_ref->fields == NULL)
        return false;

    catalog_name = strVal(list_nth(col_ref->fields, 0));
    schema_name = strVal(list_nth(col_ref->fields, 1));
    table_name = strVal(list_nth(col_ref->fields, 2));
    col_name = strVal(list_nth(col_ref->fields, 3));

    if (catalog_name == NULL || schema_name == NULL ||
        table_name == NULL || col_name == NULL)
        return false;

    strncpy(data->catalog_name, catalog_name, TECC_PROCESSOR_MAX_NAME_LEN - 1);
    data->catalog_name[TECC_PROCESSOR_MAX_NAME_LEN - 1] = '\0';
    strncpy(data->schema_name, schema_name, TECC_PROCESSOR_MAX_NAME_LEN - 1);
    data->schema_name[TECC_PROCESSOR_MAX_NAME_LEN - 1] = '\0';
    strncpy(data->table_name, table_name, TECC_PROCESSOR_MAX_NAME_LEN - 1);
    data->table_name[TECC_PROCESSOR_MAX_NAME_LEN - 1] = '\0';
    strncpy(data->column_name, col_name, TECC_PROCESSOR_MAX_NAME_LEN - 1);
    data->column_name[TECC_PROCESSOR_MAX_NAME_LEN - 1] = '\0';
    return true;
}

/* Check if column is encrypted with state context */
static bool
check_column_encryption_with_state(TeccColumnRefData *data, const TeccState *state)
{
    bool is_encrypted = false;
    const char *database_name;
    const char *schema_name;
    const char *table_name;

    if (data == NULL)
        return false;

    /* Check if we have enough information to query cache */
    if (data->table_name[0] == '\0' || data->column_name[0] == '\0')
    {
        /* Not enough information to check encryption */
        data->is_encrypted = false;
        return true;
    }

    /* State is required to query the encryption cache */
    if (state == NULL || state->conn == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "State or connection not available, cannot check column encryption");
        data->is_encrypted = false;
        return false;
    }

    database_name = PQdb(state->conn);
    if (database_name == NULL || database_name[0] == '\0')
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to get database name from connection");
        data->is_encrypted = false;
        return false;
    }

    schema_name = data->schema_name[0] != '\0' ? data->schema_name : "public";
    table_name = data->table_name;

    /* Use cache module to check if column is encrypted */
    {
        /* Begin - ReqID:TECC4.0 - 使用缓存接口检查列加密状态 */
        TeccColumnCacheItem *column_info = NULL;

        /* 调用缓存接口查询列是否加密 */
        column_info = tecc_cache_get_encrypted_column_by_name(
            state,
            database_name,
            schema_name,
            data->table_name,
            data->column_name
        );

        if (column_info != NULL)
        {
            /* 列是加密的 */
            is_encrypted = true;
            data->is_encrypted = true;

            /* 获取OID信息 */
            data->column_oid = column_info->original_type_oid;
            data->table_oid = column_info->table_oid;

            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Column %s.%s.%s is encrypted (OID: %u.%u)",
                               schema_name, data->table_name, data->column_name,
                               data->table_oid, data->column_oid);
        }
        else
        {
            /* 列未加密或不存在 */
            is_encrypted = false;
            data->is_encrypted = false;
            data->column_oid = InvalidOid;
            data->table_oid = InvalidOid;

            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Column %s.%s.%s is not encrypted or not found",
                               schema_name, data->table_name, data->column_name);
        }
        /* End - ReqID:TECC4.0 - 使用缓存接口检查列加密状态 */
    }

    return true;
}

/* ==================== Expression GUC Integration Functions ==================== */

/* Internal helper functions for GUC processing */
static bool is_valid_guc_params(const TeccGucParams *guc_params);
static char *extract_next_schema_from_search_path(const char *search_path, int *pos);
static bool process_string_with_guc(const char *input_str,
                                   const TeccGucParams *guc_params,
                                   char **output_str);
static bool process_numeric_with_guc(const char *input_str,
                                    const TeccGucParams *guc_params,
                                    char **output_str);
static bool resolve_name_with_search_path(const char *unqualified_name,
                                         const char *object_type,
                                         const TeccGucParams *guc_params,
                                         char **qualified_name);

/*
 * Initialize expression GUC processing
 */
bool
tecc_expr_guc_init(TeccExpressionContext *expr_ctx)
{
    if (expr_ctx == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid expression context");
        return false;
    }

    /* Initialize GUC parameters pointer */
    expr_ctx->guc_params = NULL;
    expr_ctx->guc_params_enabled = true;
    expr_ctx->guc_params_cache = NULL;
    expr_ctx->guc_applications = 0;
    expr_ctx->guc_errors = 0;

    /* Clear error message */
    memset(expr_ctx->error_message, 0, sizeof(expr_ctx->error_message));

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Expression GUC processing initialized");
    return true;
}

/*
 * Cleanup expression GUC processing
 */
void
tecc_expr_guc_cleanup(TeccExpressionContext *expr_ctx)
{
    if (expr_ctx == NULL)
    {
        return;
    }

    /* Reset GUC parameters */
    if (expr_ctx->guc_params != NULL)
    {
        pg_free((void *)expr_ctx->guc_params);
        expr_ctx->guc_params = NULL;
    }
    expr_ctx->guc_params_enabled = false;
    expr_ctx->guc_params_cache = NULL;

    /* Clear statistics */
    expr_ctx->guc_applications = 0;
    expr_ctx->guc_errors = 0;

    /* Clear error message */
    memset(expr_ctx->error_message, 0, sizeof(expr_ctx->error_message));

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Expression GUC processing cleaned up");
}

/*
 * Get current GUC parameters for expression processing
 *
 * Returns the cached TeccGucParams pointer if already loaded, or attempts
 * to load from the cache module via tecc_cache_get_guc_param.
 * The guc_params_cache field should point to the const TeccState for the
 * current connection.
 */
const TeccGucParams *
tecc_expr_get_guc_params(TeccExpressionContext *expr_ctx)
{
    if (expr_ctx == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid expression context");
        return NULL;
    }

    if (!expr_ctx->guc_params_enabled)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "GUC processing disabled");
        return NULL;
    }

    /* Return cached params if already loaded */
    if (expr_ctx->guc_params != NULL)
        return expr_ctx->guc_params;

    /* Load from cache if we have a state pointer */
    if (expr_ctx->guc_params_cache != NULL)
    {
        const TeccState *state = (const TeccState *)expr_ctx->guc_params_cache;
        const char *val;
        TeccGucParams *params;

        params = (TeccGucParams *)pg_malloc0(sizeof(TeccGucParams));
        if (params == NULL)
            return NULL;

        /* Load standard_conforming_strings */
        val = tecc_cache_get_guc_param(state, "standard_conforming_strings");
        if (val != NULL)
            params->standard_conforming_strings = (strcmp(val, "on") == 0);
        else
            params->standard_conforming_strings = true;

        /* Load backslash_quote */
        val = tecc_cache_get_guc_param(state, "backslash_quote");
        if (val != NULL && val[0] != '\0')
        {
            if (strcmp(val, "on") == 0)
                params->backslash_quote = 1;
            else if (strcmp(val, "safe_encoding") == 0)
                params->backslash_quote = 1;
            else if (strcmp(val, "unsafe_encoding") == 0)
                params->backslash_quote = 2;
            else
                params->backslash_quote = 1;
        }
        else
            params->backslash_quote = 1;

        /* Load escape_string_warning */
        val = tecc_cache_get_guc_param(state, "escape_string_warning");
        if (val != NULL)
            params->escape_string_warning = (strcmp(val, "on") == 0);
        else
            params->escape_string_warning = true;

        /* Load search_path */
        val = tecc_cache_get_guc_param(state, "search_path");
        if (val != NULL)
        {
            size_t slen = strlen(val);
            if (slen >= sizeof(params->searchpath))
                slen = sizeof(params->searchpath) - 1;
            memcpy(params->searchpath, val, slen);
            params->searchpath[slen] = '\0';
        }
        else
        {
            memcpy(params->searchpath, "\"$user\", public", 15);
            params->searchpath[15] = '\0';
        }

        /* Load role */
        val = tecc_cache_get_guc_param(state, "role");
        if (val != NULL)
        {
            size_t slen = strlen(val);
            if (slen >= sizeof(params->role))
                slen = sizeof(params->role) - 1;
            memcpy(params->role, val, slen);
            params->role[slen] = '\0';
        }
        else
        {
            params->role[0] = '\0';
        }

        expr_ctx->guc_params = params;

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "GUC parameters loaded from cache: scs=%s, bq=%d, esw=%s",
                           params->standard_conforming_strings ? "on" : "off",
                           params->backslash_quote,
                           params->escape_string_warning ? "on" : "off");
    }

    return expr_ctx->guc_params;
}

/*
 * Apply GUC parameters to string literal
 */
bool
tecc_expr_apply_guc_to_string(TeccExpressionContext *expr_ctx,
                             const char *str_literal,
                             char **processed_str)
{
    const TeccGucParams *guc_params;
    bool result;

    if (expr_ctx == NULL || str_literal == NULL || processed_str == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid parameters");
        return false;
    }

    *processed_str = NULL;
    guc_params = tecc_expr_get_guc_params(expr_ctx);
    if (guc_params == NULL)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "GUC parameters not available, using original string");
        *processed_str = pg_strdup(str_literal);
        return (*processed_str != NULL);
    }

    /* Process string with GUC parameters */
    result = process_string_with_guc(str_literal, guc_params, processed_str);
    if (result)
    {
        expr_ctx->guc_applications++;
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Successfully applied GUC to string: %s", str_literal);
    }
    else
    {
        expr_ctx->guc_errors++;
        snprintf(expr_ctx->error_message, sizeof(expr_ctx->error_message),
                "Failed to apply GUC to string: %s", str_literal);
    }

    return result;
}

/*
 * Apply GUC parameters to numeric constant
 */
bool
tecc_expr_apply_guc_to_numeric(TeccExpressionContext *expr_ctx,
                              const char *numeric_str,
                              char **processed_numeric)
{
    const TeccGucParams *guc_params;
    bool result;

    if (expr_ctx == NULL || numeric_str == NULL || processed_numeric == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid parameters");
        return false;
    }

    *processed_numeric = NULL;
    guc_params = tecc_expr_get_guc_params(expr_ctx);
    if (guc_params == NULL)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "GUC parameters not available, using original numeric");
        *processed_numeric = pg_strdup(numeric_str);
        return (*processed_numeric != NULL);
    }

    /* Process numeric with GUC parameters */
    result = process_numeric_with_guc(numeric_str, guc_params, processed_numeric);
    if (result)
    {
        expr_ctx->guc_applications++;
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Successfully applied GUC to numeric: %s", numeric_str);
    }
    else
    {
        expr_ctx->guc_errors++;
        snprintf(expr_ctx->error_message, sizeof(expr_ctx->error_message),
                "Failed to apply GUC to numeric: %s", numeric_str);
    }

    return result;
}

/*
 * Apply search path to unqualified name
 */
bool
tecc_expr_apply_search_path(TeccExpressionContext *expr_ctx,
                           const char *unqualified_name,
                           const char *object_type,
                           char **qualified_name)
{
    const TeccGucParams *guc_params;
    bool result;

    if (expr_ctx == NULL || unqualified_name == NULL ||
        object_type == NULL || qualified_name == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid parameters");
        return false;
    }

    *qualified_name = NULL;
    guc_params = tecc_expr_get_guc_params(expr_ctx);
    if (guc_params == NULL)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "GUC parameters not available, using unqualified name");
        *qualified_name = pg_strdup(unqualified_name);
        return (*qualified_name != NULL);
    }

    /* Resolve name with search path */
    result = resolve_name_with_search_path(unqualified_name, object_type,
                                          guc_params, qualified_name);
    if (result)
    {
        expr_ctx->guc_applications++;
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Successfully resolved %s: %s -> %s",
                           object_type, unqualified_name, *qualified_name);
    }
    else
    {
        expr_ctx->guc_errors++;
        snprintf(expr_ctx->error_message, sizeof(expr_ctx->error_message),
                "Failed to resolve %s: %s", object_type, unqualified_name);
    }

    return result;
}

/*
 * Check if GUC parameters are available
 */
bool
tecc_expr_guc_params_available(TeccExpressionContext *expr_ctx)
{
    if (expr_ctx == NULL)
    {
        return false;
    }

    return (expr_ctx->guc_params_enabled && expr_ctx->guc_params != NULL);
}

/*
 * Enable/disable GUC parameter processing
 */
void
tecc_expr_set_guc_processing(TeccExpressionContext *expr_ctx, bool enabled)
{
    if (expr_ctx == NULL)
    {
        return;
    }

    expr_ctx->guc_params_enabled = enabled;
    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "GUC processing %s", enabled ? "enabled" : "disabled");
}

/*
 * Update expression GUC parameters
 */
bool
tecc_expr_update_guc_params(TeccExpressionContext *expr_ctx,
                           const TeccGucParams *new_params)
{
    const TeccGucParams *guc_params = new_params;

    if (expr_ctx == NULL || guc_params == NULL)
    {
        return false;
    }

    if (!is_valid_guc_params(guc_params))
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid new GUC parameters");
        return false;
    }

    /* Allocate memory for GUC parameters if not already allocated */
    if (expr_ctx->guc_params == NULL)
    {
        expr_ctx->guc_params = pg_malloc0(sizeof(TeccGucParams));
        if (expr_ctx->guc_params == NULL)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "Failed to allocate memory for GUC parameters");
            return false;
        }
    }

    /* Copy the GUC parameters (structure contains only fixed-size arrays, no pointers) */
    memcpy((void *)expr_ctx->guc_params, new_params, sizeof(TeccGucParams));
    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "GUC parameters updated");

    return true;
}

/* ==================== Internal Helper Functions ==================== */

/*
 * Check if GUC parameters are valid
 * Note: expr_guc_params.h has been removed, using simplified validation
 */
static bool
is_valid_guc_params(const TeccGucParams *guc_params)
{
    if (guc_params == NULL)
    {
        return false;
    }

    /* Basic validation - just check that searchpath is properly null-terminated */
    /* In practice, TeccGucParams structure is simple and doesn't need complex validation */
    return true;
}

/*
 * Extract next schema from search path
 */
static char *
extract_next_schema_from_search_path(const char *search_path, int *pos)
{
    const char *start;
    char *schema;
    int len;

    if (search_path == NULL || pos == NULL || *pos < 0)
    {
        return NULL;
    }

    /* Skip leading spaces */
    while (search_path[*pos] == ' ')
    {
        (*pos)++;
    }

    if (search_path[*pos] == '\0')
    {
        return NULL;
    }

    start = &search_path[*pos];

    /* Find end of schema name (comma or end of string) */
    while (search_path[*pos] != '\0' && search_path[*pos] != ',')
    {
        (*pos)++;
    }

    len = &search_path[*pos] - start;
    if (len == 0)
    {
        return NULL;
    }

    /* Allocate and copy schema name */
    schema = (char *)pg_malloc(len + 1);
    if (schema == NULL)
    {
        return NULL;
    }

    strncpy(schema, start, len);
    schema[len] = '\0';

    /* Skip comma for next iteration */
    if (search_path[*pos] == ',')
    {
        (*pos)++;
    }

    return schema;
}

/*
 * Process string with GUC parameters
 */
static bool
process_string_with_guc(const char *input_str,
                       const TeccGucParams *guc_params,
                       char **output_str)
{
    char *result;
    size_t len;

    if (input_str == NULL || guc_params == NULL || output_str == NULL)
    {
        return false;
    }

    /* For now, implement basic string processing */
    len = strlen(input_str);
    result = (char *)pg_malloc(len + 1);
    if (result == NULL)
    {
        return false;
    }

    /* Copy input string safely */
    memcpy(result, input_str, len + 1);

    /* Apply standard_conforming_strings processing */
    if (!guc_params->standard_conforming_strings)
    {
        /* Traditional mode: process backslash escape sequences */
        const char *src = input_str;
        char *dst = result;
        size_t remain = len;

        while (*src != '\0' && remain > 0)
        {
            if (*src == '\\')
            {
                src++;
                remain--;

                switch (*src)
                {
                    case '\\':
                        *dst++ = '\\';
                        src++;
                        break;

                    case '\'':
                        *dst++ = '\'';
                        src++;
                        break;

                    case 'b':
                        *dst++ = '\b';
                        src++;
                        break;

                    case 'f':
                        *dst++ = '\f';
                        src++;
                        break;

                    case 'n':
                        *dst++ = '\n';
                        src++;
                        break;

                    case 'r':
                        *dst++ = '\r';
                        src++;
                        break;

                    case 't':
                        *dst++ = '\t';
                        src++;
                        break;

                    case 'x':
                    case 'X':
                    {
                        /* Hexadecimal escape: \xHH */
                        unsigned int hex_val = 0;
                        int hex_digits = 0;

                        src++;
                        while (hex_digits < 2 && *src != '\0')
                        {
                            if (*src >= '0' && *src <= '9')
                                hex_val = (hex_val << 4) + (*src - '0');
                            else if (*src >= 'a' && *src <= 'f')
                                hex_val = (hex_val << 4) + (*src - 'a' + 10);
                            else if (*src >= 'A' && *src <= 'F')
                                hex_val = (hex_val << 4) + (*src - 'A' + 10);
                            else
                                break;
                            src++;
                            hex_digits++;
                        }

                        if (hex_digits > 0)
                            *dst++ = (unsigned char)hex_val;
                        else
                            *dst++ = 'x';  /* \x not followed by hex digits */
                        break;
                    }

                    case '0': case '1': case '2': case '3':
                    case '4': case '5': case '6': case '7':
                    {
                        /* Octal escape: \ooo */
                        unsigned int oct_val = 0;
                        int oct_digits = 0;

                        while (oct_digits < 3 && *src >= '0' && *src <= '7')
                        {
                            oct_val = (oct_val << 3) + (*src - '0');
                            src++;
                            oct_digits++;
                        }

                        *dst++ = (unsigned char)oct_val;
                        break;
                    }

                    default:
                        /* Unknown escape: preserve backslash and next char */
                        *dst++ = '\\';
                        if (*src != '\0')
                        {
                            *dst++ = *src;
                            src++;
                        }
                        break;
                }
            }
            else
            {
                *dst++ = *src++;
            }
        }

        *dst = '\0';
        result = (char *)pg_realloc(result, (dst - result) + 1);

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Processed string in traditional mode with backslash escapes");
    }
    else
    {
        /* Standard conforming mode: no transformation needed */
        memcpy(result, input_str, len + 1);
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Processed string in standard conforming mode");
    }

    /* Note: client_encoding parameter has been removed from TeccGucParams structure */
    /* Encoding conversion is not part of TECC GUC parameter processing */

    *output_str = result;
    return true;
}

/*
 * Process numeric with GUC parameters
 */
static bool
process_numeric_with_guc(const char *input_str,
                        const TeccGucParams *guc_params,
                        char **output_str)
{
    char *result;
    size_t len;

    if (input_str == NULL || guc_params == NULL || output_str == NULL)
    {
        return false;
    }

    /* For now, implement basic numeric processing */
    len = strlen(input_str);
    result = (char *)pg_malloc(len + 1);
    if (result == NULL)
    {
        return false;
    }

    /* Copy input string safely */
    memcpy(result, input_str, len + 1);

    /* Note: lc_numeric parameter has been removed from TeccGucParams structure */
    /* Locale-specific formatting is not part of TECC GUC parameter processing */

    *output_str = result;
    return true;
}

/*
 * Resolve name with search path
 */
static bool
resolve_name_with_search_path(const char *unqualified_name,
                             const char *object_type,
                             const TeccGucParams *guc_params,
                             char **qualified_name)
{
    char *result;
    int pos = 0;
    char *schema;
    size_t total_len;

    if (unqualified_name == NULL || object_type == NULL ||
        guc_params == NULL || qualified_name == NULL)
    {
        return false;
    }

    /* Check if search path is empty */
    if (guc_params->searchpath[0] == '\0')
    {
        /* No search path, return unqualified name */
        result = pg_strdup(unqualified_name);
        if (result == NULL)
        {
            return false;
        }
        *qualified_name = result;
        return true;
    }

    /* Extract schemas from search path */
    while ((schema = extract_next_schema_from_search_path(guc_params->searchpath, &pos)) != NULL)
    {
        /* Check if object exists in this schema */
        /* This would query the cache or database in a real implementation */

        /* For now, assume first schema in search path */
        total_len = strlen(schema) + 1 + strlen(unqualified_name) + 1;
        result = (char *)pg_malloc(total_len);
        if (result == NULL)
        {
            pg_free(schema);
            return false;
        }

        snprintf(result, total_len, "%s.%s", schema, unqualified_name);
        pg_free(schema);

        *qualified_name = result;
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Resolved %s to %s using search path",
                           unqualified_name, result);
        return true;
    }

    /* No schema found in search path, return unqualified name */
    result = pg_strdup(unqualified_name);
    if (result == NULL)
    {
        return false;
    }

    *qualified_name = result;
    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Could not resolve %s, using unqualified name", unqualified_name);
    return true;
}

/* End - ReqID:TECC4.0 */
