#include "postgres_fe.h"
#include "common/fe_memutils.h"
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
 *   src/interfaces/libpq/tecc_expression/expr_processor.c
 */

#include "expr_processor.h"
#include "column_ref_data.h"
#include "../frontend_parser/tecc_nodes.h"  /* For list_length function */
#include <string.h>
#include <stdio.h>

/* Forward declarations for internal helper functions */
static bool expr_expand_recursive(const Node *expr_node, TeccExprPartsList *expr_parts);
static bool expr_process_column_ref(const ColumnRef *col_ref, TeccExprPartsList *expr_parts);
static bool expr_process_func_call(const FuncCall *func_call, TeccExprPartsList *expr_parts);
static bool expr_process_a_const(const A_Const *a_const, TeccExprPartsList *expr_parts);
static bool expr_process_a_expr(const A_Expr *a_expr, TeccExprPartsList *expr_parts);
static bool expr_process_bool_expr(const BoolExpr *bool_expr, TeccExprPartsList *expr_parts);
/* Frontend types are defined in fe_adapter.h */
/* static bool expr_process_param_ref(const ParamRef *param_ref, TeccExprPartsList *expr_parts); */
/* static bool expr_process_type_cast(const TypeCast *type_cast, TeccExprPartsList *expr_parts); */
/* static bool expr_process_array_expr(const ArrayExpr *array_expr, TeccExprPartsList *expr_parts); */
/* static bool expr_process_set_op(const SetOperationStmt *set_op, TeccExprPartsList *expr_parts); */

/* Internal helper functions for column reference expansion */
static bool parse_single_name(const ColumnRef *col_ref, TeccColumnRefData *data);
static bool parse_two_names(const ColumnRef *col_ref, TeccColumnRefData *data);
static bool parse_three_names(const ColumnRef *col_ref, TeccColumnRefData *data);
static bool parse_four_names(const ColumnRef *col_ref, TeccColumnRefData *data);
static bool check_column_encryption(TeccColumnRefData *data);

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

    /* Check if column is encrypted */
    if (!check_column_encryption(*col_data))
    {
        tecc_processor_debug(TECC_LOG_WARNING,
                           "Failed to check column encryption status");
        /* Continue anyway - encryption check failure is not fatal */
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Expanded column reference: %s [encrypted=%s]",
                       (*col_data)->alias_fqdn,
                       (*col_data)->is_encrypted ? "true" : "false");

    return true;
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
            char *func_name = (char *)list_nth(func_call->funcname, 0);
            if (func_name != NULL)
            {
                strncpy((*func_data)->function_name, func_name, NAMEDATALEN - 1);
                (*func_data)->function_name[NAMEDATALEN - 1] = '\0';
            }
        }
        else if (funcname_len == 2)
        {
            /* Schema-qualified function: schema_name.function_name */
            char *schema_name = (char *)list_nth(func_call->funcname, 0);
            char *func_name = (char *)list_nth(func_call->funcname, 1);

            if (schema_name != NULL && func_name != NULL)
            {
                strncpy((*func_data)->schema_name, schema_name, NAMEDATALEN - 1);
                (*func_data)->schema_name[NAMEDATALEN - 1] = '\0';
                strncpy((*func_data)->function_name, func_name, NAMEDATALEN - 1);
                (*func_data)->function_name[NAMEDATALEN - 1] = '\0';
            }
        }
        else if (funcname_len == 3)
        {
            /* Catalog-schema-qualified function: catalog_name.schema_name.function_name */
            char *catalog_name = (char *)list_nth(func_call->funcname, 0);
            char *schema_name = (char *)list_nth(func_call->funcname, 1);
            char *func_name = (char *)list_nth(func_call->funcname, 2);

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
/* Simplified version for compilation */
