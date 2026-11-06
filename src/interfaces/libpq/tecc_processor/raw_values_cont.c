/* Begin - ReqID:TECC4.0 */
/*
 * File: raw_values_cont.c
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
 *   src/interfaces/libpq/tecc_processor/raw_values_cont.c
 */

#include "postgres_fe.h"
#include "raw_values_cont.h"
#include "raw_value.h"
#include "raw_values_list.h"
#include "processor_utils.h"
#include "internal/tecc_processor_internal.h"
#include <string.h>
#include <stdio.h>

/* Include frontend parser node definitions */
#include "../frontend_parser/tecc_nodes.h"

/* Include raw value creation functions */
#include "raw_value.h"

/*
 * Internal utility functions (not exposed in public API)
 * These correspond to private methods in openGauss RawValues class
 */

/* Count literals in string */
static size_t
tecc_raw_values_count_literals_internal(const char *str, size_t size)
{
    size_t count = 0;
    size_t i;

    if (str == NULL || size == 0)
        return 0;

    for (i = 0; i < size; i++) {
        if (str[i] == '\'') {
            count++;
        }
    }

    return count;
}

/* Get cached default values for table */
static bool
tecc_raw_values_get_cached_default_values_internal(
    const char *db_name,
    const char *schema_name,
    const char *table_name,
    TeccRawValuesList *raw_values_list)
{
    /* Placeholder implementation - will be implemented when cache module is ready */
    (void)db_name;
    (void)schema_name;
    (void)table_name;
    (void)raw_values_list;

    

    return true;
}

/* Extract raw values from INSERT...SELECT statement */
static bool
tecc_raw_values_from_insert_select_stmt_internal(
    const SelectStmt *select_stmt,
    TeccStatementData *statement_data,
    TeccRawValuesList *raw_values_list)
{
    /* Placeholder implementation - will be implemented when SELECT statement processing is ready */
    (void)select_stmt;
    (void)statement_data;
    (void)raw_values_list;

    

    return true;
}

/* Extract raw values from value lists */
static bool
tecc_raw_values_from_value_lists_internal(
    List *exprList,
    TeccStatementData *statement_data,
    TeccRawValuesList *raw_values_list)
{
    ListCell *lc;

    if (raw_values_list == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                            "Raw values list is NULL in tecc_raw_values_from_value_lists");
        return false;
    }

    if (exprList == NIL) {
        tecc_processor_debug(TECC_LOG_WARNING,
                            "Expression list is NULL in tecc_raw_values_from_value_lists");
        return true; /* Empty list is not an error */
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Extracting raw values from value list with %d expressions",
                        list_length(exprList));

    foreach(lc, exprList)
    {
        Node *node = (Node *)lfirst(lc);

        if (node == NULL) {
            tecc_processor_debug(TECC_LOG_WARNING,
                                "NULL node in expression list, skipping");
            continue;
        }

        /* Implement full node type handling based on openGauss design */
        /* Handle different node types for raw value extraction */
        switch (nodeTag(node))
        {
            case FE_T_ColumnRef:
                /* Column reference - extract column information */
                {
                    ColumnRef *cref = (ColumnRef *)node;
                    ListCell *lc;
                    char *column_name = NULL;

                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                        "Processing ColumnRef node");

                    /* Extract column name from fields list */
                    if (cref->fields != NULL) {
                        /* Get the last element as column name */
                        ListCell *last_cell = NULL;
                        foreach(lc, cref->fields) {
                            last_cell = lc;
                        }

                        if (last_cell != NULL) {
                            Value *val = (Value *)lfirst(last_cell);
                            if (val != NULL && val->type == FE_T_String) {
                                column_name = val->val.str;
                            }
                        }
                    }

                    if (column_name != NULL) {
                        /* Create raw value for column reference */
                        TeccRawValue *raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_STRING);
                        if (raw_value != NULL) {
                            raw_value->type = TECC_RAW_VALUE_TYPE_STRING;
                            raw_value->original.string_value = pg_strdup(column_name);
                            raw_value->length = strlen(column_name);
                            raw_value->column_name = pg_strdup(column_name);
                            raw_value->location = cref->location;
                            raw_value->is_param = false;
                            raw_value->needs_encryption = true; /* Column references may need encryption */

                            /* Add to raw values list */
                            if (raw_values_list != NULL) {
                                if (!tecc_raw_values_list_add(raw_values_list, raw_value)) {
                                    tecc_processor_debug(TECC_LOG_WARNING,
                                                        "Failed to add ColumnRef raw value to list");
                                    tecc_raw_value_destroy(raw_value);
                                }
                            }
                        }
                    }
                }
                break;

            case FE_T_Const:
                /* Constant value - extract constant value from Const node */
                {
                    Const *const_node = (Const *)node;
                    TeccRawValue *raw_value = NULL;

                    

                    /* Handle NULL constant */
                    if (const_node->constisnull)
                    {
                        raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_NULL);
                        if (raw_value != NULL) {
                            raw_value->type = TECC_RAW_VALUE_TYPE_NULL;
                            raw_value->location = const_node->location;
                            raw_value->is_param = false;
                            raw_value->needs_encryption = false;
                        }
                    }
                    /* Handle text/string constants (TEXTOID = 25) */
                    else if (const_node->consttype == 25)
                    {
                        if (const_node->constvalue.sval != NULL)
                        {
                            raw_value = tecc_raw_value_create_from_string(
                                const_node->constvalue.sval,
                                strlen(const_node->constvalue.sval));
                            if (raw_value != NULL) {
                                raw_value->type = TECC_RAW_VALUE_TYPE_STRING;
                                raw_value->location = const_node->location;
                                raw_value->is_param = false;
                                raw_value->needs_encryption = true;
                            }
                        }
                    }
                    /* Handle integer constants (INT4OID = 23, INT8OID = 20) */
                    else if (const_node->consttype == 23 || const_node->consttype == 20)
                    {
                        raw_value = tecc_raw_value_create_from_int((int32)const_node->constvalue.ival);
                        if (raw_value != NULL) {
                            raw_value->type = TECC_RAW_VALUE_TYPE_INTEGER;
                            raw_value->location = const_node->location;
                            raw_value->is_param = false;
                            raw_value->needs_encryption = true;
                        }
                    }
                    /* Handle float constants (FLOAT4OID = 700, FLOAT8OID = 701) */
                    else if (const_node->consttype == 700 || const_node->consttype == 701)
                    {
                        raw_value = tecc_raw_value_create_from_float(const_node->constvalue.dval);
                        if (raw_value != NULL) {
                            raw_value->type = TECC_RAW_VALUE_TYPE_FLOAT;
                            raw_value->location = const_node->location;
                            raw_value->is_param = false;
                            raw_value->needs_encryption = true;
                        }
                    }
                    else
                    {
                        tecc_processor_debug(TECC_LOG_WARNING,
                                            "Unhandled Const consttype: %d", const_node->consttype);
                    }

                    /* Add to raw values list if created successfully */
                    if (raw_value != NULL && raw_values_list != NULL) {
                        if (!tecc_raw_values_list_add(raw_values_list, raw_value)) {
                            tecc_processor_debug(TECC_LOG_WARNING,
                                                "Failed to add Const raw value to list");
                            tecc_raw_value_destroy(raw_value);
                        }
                    }
                }
                break;

            case FE_T_FuncCall:
                /* Function call - extract function information */
                {
                    FuncCall *fcall = (FuncCall *)node;
                    ListCell *lc;
                    char *func_name = NULL;

                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                        "Processing FuncCall node");

                    /* Extract function name from funcname list */
                    if (fcall->funcname != NULL) {
                        /* Get the last element as function name */
                        ListCell *last_cell = NULL;
                        foreach(lc, fcall->funcname) {
                            last_cell = lc;
                        }

                        if (last_cell != NULL) {
                            Value *val = (Value *)lfirst(last_cell);
                            if (val != NULL && val->type == FE_T_String) {
                                func_name = val->val.str;
                            }
                        }
                    }

                    if (func_name != NULL) {
                        /* Create raw value for function call */
                        TeccRawValue *raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_STRING);
                        if (raw_value != NULL) {
                            raw_value->type = TECC_RAW_VALUE_TYPE_STRING;
                            raw_value->original.string_value = pg_strdup(func_name);
                            raw_value->length = strlen(func_name);
                            raw_value->column_name = pg_strdup("function");
                            raw_value->location = fcall->location;
                            raw_value->is_param = false;
                            raw_value->needs_encryption = false; /* Function names don't need encryption */

                            /* Add to raw values list */
                            if (raw_values_list != NULL) {
                                if (!tecc_raw_values_list_add(raw_values_list, raw_value)) {
                                    tecc_processor_debug(TECC_LOG_WARNING,
                                                        "Failed to add FuncCall raw value to list");
                                    tecc_raw_value_destroy(raw_value);
                                }
                            }
                        }
                    }

                    /* Process function arguments recursively */
                    if (fcall->args != NULL) {
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                            "Processing FuncCall arguments recursively");
                        /* Recursively process arguments */
                        if (!tecc_raw_values_from_value_lists_internal(
                                fcall->args, statement_data, raw_values_list)) {
                            tecc_processor_debug(TECC_LOG_WARNING,
                                                "Failed to process FuncCall arguments");
                        }
                    }
                }
                break;

            case FE_T_A_Expr:
                /* Expression - extract expression information */
                {
                    A_Expr *aexpr = (A_Expr *)node;
                    ListCell *lc;
                    char *operator_name = NULL;

                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                        "Processing A_Expr node");

                    /* Extract operator name from name list */
                    if (aexpr->name != NULL) {
                        /* Get the last element as operator name */
                        ListCell *last_cell = NULL;
                        foreach(lc, aexpr->name) {
                            last_cell = lc;
                        }

                        if (last_cell != NULL) {
                            Value *val = (Value *)lfirst(last_cell);
                            if (val != NULL && val->type == FE_T_String) {
                                operator_name = val->val.str;
                            }
                        }
                    }

                    if (operator_name != NULL) {
                        /* Create raw value for operator */
                        TeccRawValue *raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_STRING);
                        if (raw_value != NULL) {
                            raw_value->type = TECC_RAW_VALUE_TYPE_STRING;
                            raw_value->original.string_value = pg_strdup(operator_name);
                            raw_value->length = strlen(operator_name);
                            raw_value->column_name = pg_strdup("operator");
                            raw_value->location = aexpr->location;
                            raw_value->is_param = false;
                            raw_value->needs_encryption = false; /* Operators don't need encryption */

                            /* Add to raw values list */
                            if (raw_values_list != NULL) {
                                if (!tecc_raw_values_list_add(raw_values_list, raw_value)) {
                                    tecc_processor_debug(TECC_LOG_WARNING,
                                                        "Failed to add A_Expr raw value to list");
                                    tecc_raw_value_destroy(raw_value);
                                }
                            }
                        }
                    }

                    /* Process left expression recursively */
                    if (aexpr->lexpr != NULL) {
                        List *temp_list;

                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                            "Processing A_Expr left expression recursively");
                        /* Create a temporary list for recursive processing */
                        temp_list = list_make1(aexpr->lexpr);
                        if (temp_list != NULL) {
                            if (!tecc_raw_values_from_value_lists_internal(
                                    temp_list, statement_data, raw_values_list)) {
                                tecc_processor_debug(TECC_LOG_WARNING,
                                                    "Failed to process A_Expr left expression");
                            }
                            /* Free the temporary list */
                            /* Note: list_free only frees the list structure, not the elements */
                            list_free(temp_list);
                        }
                    }

                    /* Process right expression recursively */
                    if (aexpr->rexpr != NULL) {
                        List *temp_list;

                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                            "Processing A_Expr right expression recursively");
                        /* Create a temporary list for recursive processing */
                        temp_list = list_make1(aexpr->rexpr);
                        if (temp_list != NULL) {
                            if (!tecc_raw_values_from_value_lists_internal(
                                    temp_list, statement_data, raw_values_list)) {
                                tecc_processor_debug(TECC_LOG_WARNING,
                                                    "Failed to process A_Expr right expression");
                            }
                            /* Free the temporary list */
                            list_free(temp_list);
                        }
                    }
                }
                break;

            case FE_T_Param:
                /* Parameter reference - extract parameter information */
                {
                    Param *param = (Param *)node;
                    TeccRawValue *raw_value;
                    char param_str[32];

                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                        "Processing Param node: paramid=%d, paramtype=%d",
                                        param->paramid, param->paramtype);

                    /* Create raw value for parameter */
                    raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_STRING);
                    if (raw_value != NULL) {
                        /* Format parameter reference as $n */
                        snprintf(param_str, sizeof(param_str), "$%d", param->paramid);

                        raw_value->type = TECC_RAW_VALUE_TYPE_STRING;
                        raw_value->original.string_value = pg_strdup(param_str);
                        raw_value->length = strlen(param_str);
                        raw_value->column_name = pg_strdup("parameter");
                        raw_value->type_oid = param->paramtype;
                        raw_value->location = param->location;
                        raw_value->is_param = true;
                        raw_value->param_index = param->paramid;
                        raw_value->needs_encryption = true; /* Parameters may need encryption */

                        /* Add to raw values list */
                        if (raw_values_list != NULL) {
                            if (!tecc_raw_values_list_add(raw_values_list, raw_value)) {
                                tecc_processor_debug(TECC_LOG_WARNING,
                                                    "Failed to add Param raw value to list");
                                tecc_raw_value_destroy(raw_value);
                            }
                        }
                    }
                }
                break;

            case FE_T_TypeCast:
                /* Type cast - extract type cast information */
                {
                    TypeCast *typecast = (TypeCast *)node;
                    char *type_name = NULL;

                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                        "Processing TypeCast node");

                    /* Extract type name from typeName */
                    if (typecast->typeName != NULL) {
                        /* TypeName structure may have name list */
                        /* For simplicity, we'll just mark it as a type cast */
                        type_name = "type_cast";
                    }

                    if (type_name != NULL) {
                        /* Create raw value for type cast */
                        TeccRawValue *raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_STRING);
                        if (raw_value != NULL) {
                            raw_value->type = TECC_RAW_VALUE_TYPE_STRING;
                            raw_value->original.string_value = pg_strdup(type_name);
                            raw_value->length = strlen(type_name);
                            raw_value->column_name = pg_strdup("type_cast");
                            raw_value->location = typecast->location;
                            raw_value->is_param = false;
                            raw_value->needs_encryption = false; /* Type casts don't need encryption */

                            /* Add to raw values list */
                            if (raw_values_list != NULL) {
                                if (!tecc_raw_values_list_add(raw_values_list, raw_value)) {
                                    tecc_processor_debug(TECC_LOG_WARNING,
                                                        "Failed to add TypeCast raw value to list");
                                    tecc_raw_value_destroy(raw_value);
                                }
                            }
                        }
                    }

                    /* Process the argument expression recursively */
                    if (typecast->arg != NULL) {
                        List *temp_list;

                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                            "Processing TypeCast argument recursively");
                        /* Create a temporary list for recursive processing */
                        temp_list = list_make1(typecast->arg);
                        if (temp_list != NULL) {
                            if (!tecc_raw_values_from_value_lists_internal(
                                    temp_list, statement_data, raw_values_list)) {
                                tecc_processor_debug(TECC_LOG_WARNING,
                                                    "Failed to process TypeCast argument");
                            }
                            /* Free the temporary list */
                            list_free(temp_list);
                        }
                    }
                }
                break;

            default:
                /* Unhandled node type - log and continue */
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                    "Unhandled node type in raw value extraction: %d", nodeTag(node));
                break;
        }
    }

    return true;
}

/* Extract raw values from node tag (value type) */
static void
tecc_raw_values_from_nodetag_internal(
    const Value *value,
    int *end,
    int loc,
    const TeccExprPart *xpr,
    bool *is_emptyb,
    bool *is_str,
    const TeccStatementData *statement_data)
{
    /* Placeholder implementation - will be implemented when expression processing is ready */
    (void)value;
    (void)end;
    (void)loc;
    (void)xpr;
    (void)is_emptyb;
    (void)is_str;
    (void)statement_data;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Extracting raw values from node tag");
}

/* Extract raw values from expression parts */
static TeccRawValue *
tecc_raw_values_from_expr_parts_internal(
    const TeccExprPart *expr_parts,
    TeccStatementData *statement_data,
    size_t offset)
{
    /* Placeholder implementation - will be implemented when expression processing is ready */
    (void)expr_parts;
    (void)statement_data;
    (void)offset;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Extracting raw values from expression parts");

    return NULL;
}

/*
 * Public API functions
 */

/* Extract raw values from INSERT statement */
bool
tecc_raw_values_from_insert_stmt(
    const InsertStmt *insert_stmt,
    TeccStatementData *statement_data,
    TeccRawValuesList *raw_values_list)
{
    SelectStmt *select_stmt;

    /* Input validation */
    if (insert_stmt == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                            "Insert statement is NULL in tecc_raw_values_from_insert_stmt");
        return false;
    }

    if (insert_stmt->relation == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                            "Insert statement relation is NULL");
        return false;
    }

    

    /* Check for SELECT statement */
    select_stmt = (SelectStmt *)insert_stmt->selectStmt;

    if (select_stmt == NULL) {
        /* INSERT ... DEFAULT VALUES - use cached default values */
        const char *db_name = NULL;
        const char *schema_name;
        const char *table_name;

        /* Get database name from connection */
        if (statement_data != NULL && statement_data->conn != NULL) {
            db_name = PQdb(statement_data->conn);
            if (db_name == NULL || db_name[0] == '\0') {
                tecc_processor_debug(TECC_LOG_WARNING,
                                    "Could not get database name from connection");
                db_name = NULL;
            }
        }
        schema_name = insert_stmt->relation->schemaname;
        table_name = insert_stmt->relation->relname;

        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "INSERT with DEFAULT VALUES, using cached defaults");

        return tecc_raw_values_get_cached_default_values_internal(
            db_name, schema_name, table_name, raw_values_list);
    } else {
        /* INSERT ... SELECT - extract values from SELECT statement */
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "INSERT with SELECT statement");

        return tecc_raw_values_from_insert_select_stmt_internal(
            select_stmt, statement_data, raw_values_list);
    }
}

/* Extract raw values from UPDATE statement */
bool
tecc_raw_values_from_update_stmt(
    const UpdateStmt *update_stmt,
    TeccStatementData *statement_data,
    TeccRawValuesList *raw_values_list)
{
    /* Input validation */
    if (update_stmt == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                            "Update statement is NULL in tecc_raw_values_from_update_stmt");
        return false;
    }

    if (update_stmt->targetList == NIL) {
        tecc_processor_debug(TECC_LOG_WARNING,
                            "Update statement targetList is NULL - not yet supported");
        return false;
    }

    

    /* Process target list */
    return tecc_raw_values_from_value_lists_internal(
        update_stmt->targetList, statement_data, raw_values_list);
}

/* Extract raw values from EXECUTE statement */
bool
tecc_raw_values_from_execute_stmt(
    const ExecuteStmt *execute_stmt,
    TeccStatementData *statement_data,
    TeccRawValuesList *raw_values_list)
{
    /* Input validation */
    if (execute_stmt == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                            "Execute statement is NULL in tecc_raw_values_from_execute_stmt");
        return false;
    }

    if (execute_stmt->params == NULL) {
        tecc_processor_debug(TECC_LOG_WARNING,
                            "Execute statement params is NULL - not yet supported");
        return false;
    }

    

    /* Process parameter list */
    return tecc_raw_values_from_value_lists_internal(
        execute_stmt->params, statement_data, raw_values_list);
}

/* Extract raw values from constants vector */
bool
tecc_raw_values_from_consts_vec(
    const TeccExprPartsList *expr_parts_list,
    TeccStatementData *statement_data,
    int offset,
    TeccRawValuesList *raw_values_list)
{
    /* Placeholder implementation - will be implemented when expression processing is ready */
    (void)expr_parts_list;
    (void)statement_data;
    (void)offset;
    (void)raw_values_list;

    

    return true;
}

/* Reverse lookup: get unprocessed data from processed data */
bool
tecc_raw_values_get_unprocessed_data(
    const TeccRawValuesList *raw_values_list,
    const unsigned char *processed_data,
    unsigned char *unprocessed_data,
    size_t *size)
{
    /* Placeholder implementation - will be implemented when reverse lookup is needed */
    (void)raw_values_list;
    (void)processed_data;
    (void)unprocessed_data;
    (void)size;

    

    return true;
}

/*
 * Internal utility functions (exposed as per header file)
 */

/* Count literals in string */
size_t
tecc_raw_values_count_literals(const char *str, size_t size)
{
    return tecc_raw_values_count_literals_internal(str, size);
}

/* Get cached default values for table */
bool
tecc_raw_values_get_cached_default_values(
    const char *db_name,
    const char *schema_name,
    const char *table_name,
    TeccRawValuesList *raw_values_list)
{
    return tecc_raw_values_get_cached_default_values_internal(
        db_name, schema_name, table_name, raw_values_list);
}

/* Extract raw values from INSERT...SELECT statement */
bool
tecc_raw_values_from_insert_select_stmt(
    const SelectStmt *select_stmt,
    TeccStatementData *statement_data,
    TeccRawValuesList *raw_values_list)
{
    return tecc_raw_values_from_insert_select_stmt_internal(
        select_stmt, statement_data, raw_values_list);
}

/* Extract raw values from value lists */
bool
tecc_raw_values_from_value_lists(
    List *exprList,
    TeccStatementData *statement_data,
    TeccRawValuesList *raw_values_list)
{
    return tecc_raw_values_from_value_lists_internal(
        exprList, statement_data, raw_values_list);
}

/* Extract raw values from node tag (value type) */
void
tecc_raw_values_from_nodetag(
    const Value *value,
    int *end,
    int loc,
    const TeccExprPart *xpr,
    bool *is_emptyb,
    bool *is_str,
    const TeccStatementData *statement_data)
{
    tecc_raw_values_from_nodetag_internal(
        value, end, loc, xpr, is_emptyb, is_str, statement_data);
}

/* Extract raw values from expression parts */
TeccRawValue *
tecc_raw_values_from_expr_parts(
    const TeccExprPart *expr_parts,
    TeccStatementData *statement_data,
    size_t offset)
{
    return tecc_raw_values_from_expr_parts_internal(
        expr_parts, statement_data, offset);
}

/*
 * Advanced container operations (extended beyond openGauss)
 * These provide enhanced functionality for TECC
 */

/* Create advanced raw values container */
TeccRawValuesCont *
tecc_raw_values_cont_create(int initial_capacity)
{
    TeccRawValuesCont *cont;

    if (initial_capacity <= 0) {
        initial_capacity = 16; /* Default initial capacity */
    }

    cont = pg_malloc(sizeof(TeccRawValuesCont));
    if (!cont) {
        return NULL;
    }

    cont->values = pg_malloc(sizeof(TeccRawValue *) * initial_capacity);
    if (!cont->values) {
        pg_free(cont);
        return NULL;
    }

    cont->num_values = 0;
    cont->capacity = initial_capacity;
    cont->sorted_by_column = false;
    cont->column_index_map = NULL;
    cont->max_column_index = 0;
    cont->total_memory = sizeof(TeccRawValuesCont) + sizeof(TeccRawValue *) * initial_capacity;
    cont->last_error = 0;
    cont->last_error_msg[0] = '\0';

    return cont;
}

/* Destroy advanced raw values container */
void
tecc_raw_values_cont_destroy(TeccRawValuesCont *cont)
{
    if (!cont) {
        return;
    }

    /* Clear all values first */
    tecc_raw_values_cont_clear(cont);

    /* Free the values array */
    if (cont->values) {
        pg_free(cont->values);
    }

    /* Free column index map */
    if (cont->column_index_map) {
        pg_free(cont->column_index_map);
    }

    /* Free the container itself */
    pg_free(cont);
}

/* Add value to advanced container */
bool
tecc_raw_values_cont_add(TeccRawValuesCont *cont, TeccRawValue *value)
{
    if (!cont || !value) {
        return false;
    }

    /* Check if we need to expand the array */
    if (cont->num_values >= cont->capacity) {
        int new_capacity = cont->capacity * 2;
        TeccRawValue **new_values;

        new_values = pg_realloc(cont->values, sizeof(TeccRawValue *) * new_capacity);
        if (!new_values) {
            return false;
        }

        cont->values = new_values;
        cont->capacity = new_capacity;
        cont->total_memory += sizeof(TeccRawValue *) * (new_capacity - cont->capacity / 2);
    }

    /* Add the value */
    cont->values[cont->num_values] = value;
    cont->num_values++;

    /* Mark as unsorted since we added a new value */
    cont->sorted_by_column = false;

    return true;
}

/* Get value from advanced container by index */
TeccRawValue *
tecc_raw_values_cont_get(const TeccRawValuesCont *cont, int index)
{
    if (!cont || index < 0 || index >= cont->num_values) {
        return NULL;
    }

    return cont->values[index];
}

/* Get value from advanced container by column name */
TeccRawValue *
tecc_raw_values_cont_get_by_column(const TeccRawValuesCont *cont, const char *column_name)
{
    int i;

    if (!cont || !column_name) {
        return NULL;
    }

    /* If not sorted, do linear search */
    if (!cont->sorted_by_column) {
        for (i = 0; i < cont->num_values; i++) {
            TeccRawValue *value = cont->values[i];
            if (value && value->column_name &&
                strcmp(value->column_name, column_name) == 0) {
                return value;
            }
        }
        return NULL;
    }

    /* Implement binary search when sorted */
    if (cont->sorted_by_column && cont->num_values > 0) {
        int left = 0;
        int right = cont->num_values - 1;
        int cmp;

        while (left <= right) {
            int mid = left + (right - left) / 2;
            TeccRawValue *value = cont->values[mid];

            if (value == NULL || value->column_name == NULL) {
                /* Invalid entry, fall back to linear search */
                break;
            }

            cmp = strcmp(value->column_name, column_name);

            if (cmp == 0) {
                /* Found the value */
                return value;
            } else if (cmp < 0) {
                /* Search in the right half */
                left = mid + 1;
            } else {
                /* Search in the left half */
                right = mid - 1;
            }
        }

        /* Binary search didn't find the value */
        return NULL;
    }

    /* Fall back to linear search if not sorted or binary search failed */
    for (i = 0; i < cont->num_values; i++) {
        TeccRawValue *value = cont->values[i];
        if (value && value->column_name &&
            strcmp(value->column_name, column_name) == 0) {
            return value;
        }
    }

    return NULL;
}

/* Get number of values in advanced container */
int
tecc_raw_values_cont_count(const TeccRawValuesCont *cont)
{
    return cont ? cont->num_values : 0;
}

/* Check if advanced container is empty */
bool
tecc_raw_values_cont_is_empty(const TeccRawValuesCont *cont)
{
    return !cont || cont->num_values == 0;
}

/* Clear all values from advanced container */
void
tecc_raw_values_cont_clear(TeccRawValuesCont *cont)
{
    int i;

    if (!cont) {
        return;
    }

    /* Free all values in the container */
    for (i = 0; i < cont->num_values; i++) {
        if (cont->values[i]) {
            tecc_raw_value_destroy(cont->values[i]);
        }
    }

    cont->num_values = 0;
    cont->sorted_by_column = false;
}

/* Sort advanced container by column name */
bool
tecc_raw_values_cont_sort_by_column(TeccRawValuesCont *cont)
{
    /* Placeholder implementation - will implement when needed */
    (void)cont;

    

    return true;
}

/* Merge two advanced containers */
bool
tecc_raw_values_cont_merge(TeccRawValuesCont *dest, const TeccRawValuesCont *src)
{
    int i;

    if (!dest || !src) {
        return false;
    }

    for (i = 0; i < src->num_values; i++) {
        if (!tecc_raw_values_cont_add(dest, src->values[i])) {
            return false;
        }
    }

    return true;
}

/* Filter advanced container by column pattern */
TeccRawValuesCont *
tecc_raw_values_cont_filter_by_column(const TeccRawValuesCont *cont, const char *column_pattern)
{
    /* Placeholder implementation - will implement when needed */
    (void)cont;
    (void)column_pattern;

    

    return NULL;
}
/* End - ReqID:TECC4.0 */