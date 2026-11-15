/* Begin - ReqID:TECC4.0 */
/*
 * File: param_expression_processor.c
 *
 * Abstract:
 *   Parameter expression processor for TECC module.
 *   Provides functions for processing complex parameters in nested expressions.
 *   Implements enhanced parameter mapping, type inference, and encryption need detection.
 *
 * Authored by <wangxp@highgo.com>, 20260423.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/param_expression_processor.c
 */

#include "param_expression_processor.h"
#include "tecc_processor.h"
#include "internal/tecc_processor_internal.h"
#include "../tecc_cache/tecc_metadata_cache.h"
#include "../tecc_libpq_integration/fe-tecc-integration.h"  /* ReqID:TECC4.0 - for tecc_get_state */
#include <string.h>
#include <stdlib.h>

/* ReqID:TECC4.0 - Forward declarations for static functions to avoid C90 implicit declaration conflicts */
static Oid infer_type_from_function_context(const TeccParamInfo *param_info,
                                            const TeccStatementData *stmt_data);
static Oid infer_type_from_operator_context(const TeccParamInfo *param_info,
                                            const TeccStatementData *stmt_data);
static Oid infer_type_from_subquery_context(const TeccParamInfo *param_info,
                                            const TeccStatementData *stmt_data);
static bool check_column_encryption_from_cache(const TeccStatementData *stmt_data,
                                               const char *schema, const char *table,
                                               const char *column);
static bool infer_encryption_from_context(const TeccParamInfo *param_info,
                                          const TeccStatementData *stmt_data);
static bool check_function_encryption_context(const TeccParamInfo *param_info,
                                              const TeccStatementData *stmt_data);
static bool check_operator_encryption_context(const TeccParamInfo *param_info,
                                              const TeccStatementData *stmt_data);

/* ==================== 内部辅助函数 ==================== */

/*
 * 在表达式中查找参数节点（递归实现）
 */
static List *
find_params_in_expression_recursive(const Node *expr, int depth, List *param_list)
{
    ListCell *lc;

    if (expr == NULL) {
        return param_list;
    }

    /* 检查当前节点是否为参数节点 */
    if (expr->type == FE_T_Param) {
        const Param *param = (const Param *)expr;
        TeccParamInfo *param_info;

        /* 创建参数信息结构 */
        param_info = pg_malloc0(sizeof(TeccParamInfo));
        if (param_info == NULL) {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "Memory allocation failed for param_info");
            return param_list;
        }

        param_info->param_index = param->paramid - 1; /* 转换为0-based索引 */
        param_info->param_node = (void *)param;
        param_info->expression_depth = depth;
        param_info->value_type = VALUE_TYPE_UNKNOWN;
        param_info->is_in_function_call = false;
        param_info->is_in_subquery = false;
        param_info->is_in_op_expr = false;
        param_info->parent_context = NULL;

        /* 添加到参数列表 */
        param_list = lappend(param_list, param_info);

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Found parameter $%d at depth %d",
                           param->paramid, depth);
    }

    /* 根据节点类型递归处理子节点 */
    switch (expr->type) {
        case FE_T_FuncCall: {
            /* 函数调用节点 */
            const FuncCall *func_call = (const FuncCall *)expr;

            /* 处理函数参数 */
            if (func_call->args != NIL) {
                for (lc = func_call->args->head; lc != NULL; lc = lc->next) {
                    Node *arg = (Node *)lc->data.ptr_value;
                    param_list = find_params_in_expression_recursive(arg, depth + 1, param_list);
                }
            }
            break;
        }

        case FE_T_A_Expr: {
            /* 操作符表达式节点 */
            const A_Expr *a_expr = (const A_Expr *)expr;

            /* 处理左操作数 */
            if (a_expr->lexpr != NULL) {
                param_list = find_params_in_expression_recursive(a_expr->lexpr, depth + 1, param_list);
            }

            /* 处理右操作数 */
            if (a_expr->rexpr != NULL) {
                param_list = find_params_in_expression_recursive(a_expr->rexpr, depth + 1, param_list);
            }
            break;
        }

        /* ReqID:TECC4.0 - INSERT 语句：参数在 VALUES 子句（selectStmt 为
         * SelectStmt，其 valuesLists 含参数引用）。缺此分支导致参数化
         * INSERT 的参数映射为 0（找不到 $N），参数无法加密。 */
        case FE_T_InsertStmt: {
            /* INSERT语句节点 */
            const InsertStmt *insert_stmt = (const InsertStmt *)expr;

            /* 处理 VALUES/SELECT 子句 */
            if (insert_stmt->selectStmt != NULL) {
                param_list = find_params_in_expression_recursive(insert_stmt->selectStmt, depth + 1, param_list);
            }

            /* 处理 ON CONFLICT 子句 */
            if (insert_stmt->onConflictClause != NIL) {
                for (lc = insert_stmt->onConflictClause->head; lc != NULL; lc = lc->next) {
                    Node *item = (Node *)lc->data.ptr_value;
                    param_list = find_params_in_expression_recursive(item, depth + 1, param_list);
                }
            }

            /* 处理 RETURNING 子句 */
            if (insert_stmt->returningList != NIL) {
                for (lc = insert_stmt->returningList->head; lc != NULL; lc = lc->next) {
                    Node *item = (Node *)lc->data.ptr_value;
                    param_list = find_params_in_expression_recursive(item, depth + 1, param_list);
                }
            }
            break;
        }

        /* ReqID:TECC4.0 - UPDATE 语句：参数在 SET 目标列表（targetList）和
         * WHERE 子句。缺此分支导致参数化 UPDATE 的 SET/WHERE 参数映射为 0
         * （参数明文发送，服务端报 "invalid hex string"）。 */
        case FE_T_UpdateStmt: {
            /* UPDATE语句节点 */
            const UpdateStmt *update_stmt = (const UpdateStmt *)expr;

            /* 处理 SET 目标列表 */
            if (update_stmt->targetList != NIL) {
                for (lc = update_stmt->targetList->head; lc != NULL; lc = lc->next) {
                    Node *target = (Node *)lc->data.ptr_value;
                    param_list = find_params_in_expression_recursive(target, depth + 1, param_list);
                }
            }

            /* 处理 WHERE 子句 */
            if (update_stmt->whereClause != NULL) {
                param_list = find_params_in_expression_recursive(update_stmt->whereClause, depth + 1, param_list);
            }

            /* 处理 RETURNING 子句 */
            if (update_stmt->returningList != NIL) {
                for (lc = update_stmt->returningList->head; lc != NULL; lc = lc->next) {
                    Node *item = (Node *)lc->data.ptr_value;
                    param_list = find_params_in_expression_recursive(item, depth + 1, param_list);
                }
            }
            break;
        }

        /* ReqID:TECC4.0 - DELETE 语句：参数在 WHERE 子句。缺此分支导致
         * 参数化 DELETE 的 WHERE 参数映射为 0（参数明文发送）。 */
        case FE_T_DeleteStmt: {
            /* DELETE语句节点 */
            const DeleteStmt *delete_stmt = (const DeleteStmt *)expr;

            /* 处理 WHERE 子句 */
            if (delete_stmt->whereClause != NULL) {
                param_list = find_params_in_expression_recursive(delete_stmt->whereClause, depth + 1, param_list);
            }

            /* 处理 RETURNING 子句 */
            if (delete_stmt->returningList != NIL) {
                for (lc = delete_stmt->returningList->head; lc != NULL; lc = lc->next) {
                    Node *item = (Node *)lc->data.ptr_value;
                    param_list = find_params_in_expression_recursive(item, depth + 1, param_list);
                }
            }
            break;
        }

        /* ReqID:TECC4.0 - FE_T_SubLink not supported in frontend parser */
        case FE_T_SelectStmt: {
            /* SELECT语句节点 */
            const SelectStmt *select_stmt = (const SelectStmt *)expr;

            /* 处理目标列表 */
            if (select_stmt->targetList != NIL) {
                for (lc = select_stmt->targetList->head; lc != NULL; lc = lc->next) {
                    Node *target = (Node *)lc->data.ptr_value;
                    param_list = find_params_in_expression_recursive(target, depth + 1, param_list);
                }
            }

            /* ReqID:TECC4.0 - 处理 VALUES 子句（INSERT ... VALUES ($1,...) 的
             * 参数在 valuesLists 中，缺此遍历导致参数化 INSERT 映射为 0）。 */
            if (select_stmt->valuesLists != NIL) {
                /* ReqID:TECC4.0 - VALUES 行是 type 未设置的 List（前端 parser
                 * 产生的行 List type=0，find 递归无法按 type 分发）。
                 * 直接遍历行内元素（list_nth），对每个值递归查找参数。 */
                int vrow;
                for (vrow = 0; vrow < list_length(select_stmt->valuesLists); vrow++)
                {
                    void *values_row = list_nth(select_stmt->valuesLists, vrow);
                    int velem;

                    if (values_row == NULL)
                        continue;
                    for (velem = 0; velem < list_length((List *) values_row); velem++)
                    {
                        void *vitem = list_nth((List *) values_row, velem);
                        if (vitem != NULL)
                            param_list = find_params_in_expression_recursive(
                                (const Node *) vitem, depth + 2, param_list);
                    }
                }
            }

            /* 处理WHERE子句 */
            if (select_stmt->whereClause != NULL) {
                param_list = find_params_in_expression_recursive(select_stmt->whereClause, depth + 1, param_list);
            }

            /* 处理HAVING子句 */
            if (select_stmt->havingClause != NULL) {
                param_list = find_params_in_expression_recursive(select_stmt->havingClause, depth + 1, param_list);
            }

            /* 处理ORDER BY子句 */
            if (select_stmt->sortClause != NIL) {
                for (lc = select_stmt->sortClause->head; lc != NULL; lc = lc->next) {
                    Node *sort_elem = (Node *)lc->data.ptr_value;
                    param_list = find_params_in_expression_recursive(sort_elem, depth + 1, param_list);
                }
            }
            break;
        }

        case FE_T_SortBy: {
            /* SortBy节点 - 解包并处理内部的表达式 */
            const SortBy *sortby = (const SortBy *)expr;
            if (sortby->node != NULL) {
                param_list = find_params_in_expression_recursive(sortby->node, depth + 1, param_list);
            }
            break;
        }

        case FE_T_ResTarget: {
            /* 结果目标节点 */
            const ResTarget *res_target = (const ResTarget *)expr;

            if (res_target->val != NULL) {
                param_list = find_params_in_expression_recursive(res_target->val, depth + 1, param_list);
            }
            break;
        }

        case FE_T_TypeCast: {
            /* 类型转换节点 */
            const TypeCast *type_cast = (const TypeCast *)expr;

            if (type_cast->arg != NULL) {
                param_list = find_params_in_expression_recursive(type_cast->arg, depth + 1, param_list);
            }
            break;
        }

        /* ReqID:TECC4.0 - 布尔表达式（AND/OR/NOT）节点：BETWEEN 展开为
         * "col >= $1 AND col <= $2"，参数在 BoolExpr->args 中，缺此分支
         * 会导致 WHERE 参数完全找不到（BETWEEN 参数化加密失效）。 */
        case FE_T_BoolExpr: {
            /* 布尔表达式节点 */
            const BoolExpr *bool_expr = (const BoolExpr *)expr;

            if (bool_expr->args != NIL) {
                for (lc = bool_expr->args->head; lc != NULL; lc = lc->next) {
                    Node *arg = (Node *)lc->data.ptr_value;
                    param_list = find_params_in_expression_recursive(arg, depth + 1, param_list);
                }
            }
            break;
        }

        case FE_T_List: {
            /* 列表节点 */
            const List *list = (const List *)expr;

            for (lc = list->head; lc != NULL; lc = lc->next) {
                Node *item = (Node *)lc->data.ptr_value;
                param_list = find_params_in_expression_recursive(item, depth + 1, param_list);
            }
            break;
        }

        default:
            /* 其他节点类型，不包含子表达式 */
            break;
    }

    return param_list;
}

/*
 * 在函数调用中查找参数
 */
List *
tecc_find_params_in_func_call(const void *func_call_node, int depth)
{
    const FuncCall *func_call = (const FuncCall *)func_call_node;
    List *param_list = NIL;
    ListCell *lc;

    if (func_call == NULL) {
        return NIL;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Searching for parameters in function call");

    /* 处理函数参数 */
    if (func_call->args != NIL) {
        for (lc = func_call->args->head; lc != NULL; lc = lc->next) {
            Node *arg = (Node *)lc->data.ptr_value;
            param_list = find_params_in_expression_recursive(arg, depth + 1, param_list);
        }
    }

    return param_list;
}

/*
 * 在子查询中查找参数
 */
List *
tecc_find_params_in_subquery(const void *subquery_node, int depth)
{
    const SelectStmt *subquery = (const SelectStmt *)subquery_node;

    if (subquery == NULL) {
        return NIL;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Searching for parameters in subquery");

    /* 递归处理子查询 */
    return find_params_in_expression_recursive((const Node *)subquery, depth + 1, NIL);
}

/*
 * 在操作符表达式中查找参数
 */
List *
tecc_find_params_in_op_expr(const void *op_expr_node, int depth)
{
    const A_Expr *a_expr = (const A_Expr *)op_expr_node;
    List *param_list = NIL;

    if (a_expr == NULL) {
        return NIL;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Searching for parameters in operator expression");

    /* 处理左操作数 */
    if (a_expr->lexpr != NULL) {
        param_list = find_params_in_expression_recursive(a_expr->lexpr, depth + 1, param_list);
    }

    /* 处理右操作数 */
    if (a_expr->rexpr != NULL) {
        param_list = find_params_in_expression_recursive(a_expr->rexpr, depth + 1, param_list);
    }

    return param_list;
}

/*
 * 在表达式中查找所有参数
 */
List *
tecc_find_params_in_expression(const Node *expr, int depth)
{
    if (expr == NULL) {
        return NIL;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Searching for parameters in expression at depth %d",
                       depth);

    return find_params_in_expression_recursive(expr, depth, NIL);
}

/* ==================== 增强参数映射提取 ==================== */

/* ReqID:TECC4.0 - WHERE/HAVING 比较操作符参数映射补全上下文 */
typedef struct TeccComparisonMapContext
{
    TeccStatementData *stmt_data;
    TeccParamMapEntry *param_map;
    int param_map_size;
} TeccComparisonMapContext;

/*
 * ReqID:TECC4.0 - 从 ColumnRef 节点解析列名（最后一个字段）。
 * 返回指向节点内字段名的指针（不分配内存），无法解析返回 NULL。
 */
static const char *
tecc_columnref_get_name(const Node *node)
{
    const struct ColumnRef *col_ref;

    if (node == NULL || nodeTag(node) != FE_T_ColumnRef)
        return NULL;
    col_ref = (const struct ColumnRef *) node;
    if (col_ref->fields == NIL)
        return NULL;
    return strVal((Value *) llast(col_ref->fields));
}

/*
 * ReqID:TECC4.0 - 在 statement_data->encrypted_columns 中按列名查找加密列。
 * 返回加密列的 TeccProcessorColumnRef 指针，未找到返回 NULL。
 * 比较操作符一侧为 ColumnRef 时，处理器已把解析出的加密列（含真实
 * schema/table/column）登记到 encrypted_columns，这里按列名匹配。
 */
static const TeccProcessorColumnRef *
tecc_find_encrypted_col_by_name(TeccStatementData *stmt_data,
                                const char *column_name)
{
    ListCell *lc;

    if (stmt_data == NULL || column_name == NULL ||
        stmt_data->encrypted_columns == NIL)
        return NULL;

    foreach(lc, stmt_data->encrypted_columns)
    {
        TeccProcessorColumnRef *ed = (TeccProcessorColumnRef *) lfirst(lc);

        /* ReqID:TECC4.0 - 不依赖 is_encrypted 标志：encrypted_columns 列表
         * 本身只含加密列（各处理器登记时 is_encrypted 置位不一致，如
         * DELETE/INSERT 路径未置位，但列确实是加密列）。按列名匹配即可。 */
        if (ed == NULL)
            continue;
        if (ed->column_name[0] != '\0' &&
            strcmp(ed->column_name, column_name) == 0)
            return ed;
    }
    return NULL;
}

/*
 * ReqID:TECC4.0 - 处理单个比较操作符 A_Expr：
 * 若一侧是加密列 ColumnRef、另一侧是参数（FE_T_Param），把列的
 * schema/table/column 登记到 param_map[param_index]，并标记需要加密。
 */
static void
tecc_map_comparison_param(A_Expr *a_expr, TeccComparisonMapContext *ctx)
{
    const Node *col_node = NULL;
    const Node *param_node = NULL;
    const TeccProcessorColumnRef *ed;
    const char *column_name;
    int param_index = -1;

    if (a_expr == NULL || ctx == NULL || ctx->stmt_data == NULL ||
        ctx->param_map == NULL)
        return;

    /* 确定哪一侧是列引用、哪一侧是参数 */
    if (a_expr->lexpr != NULL && a_expr->rexpr != NULL)
    {
        if (nodeTag(a_expr->lexpr) == FE_T_ColumnRef &&
            nodeTag(a_expr->rexpr) == FE_T_Param)
        {
            col_node = a_expr->lexpr;
            param_node = a_expr->rexpr;
        }
        else if (nodeTag(a_expr->rexpr) == FE_T_ColumnRef &&
                 nodeTag(a_expr->lexpr) == FE_T_Param)
        {
            col_node = a_expr->rexpr;
            param_node = a_expr->lexpr;
        }
    }

    if (col_node == NULL || param_node == NULL)
        return;

    /* 参数索引（0-based） */
    param_index = ((const struct Param *) param_node)->paramid - 1;
    if (param_index < 0 || param_index >= ctx->param_map_size)
        return;

    /* 列名与加密列匹配 */
    column_name = tecc_columnref_get_name(col_node);
    if (column_name == NULL)
        return;
    ed = tecc_find_encrypted_col_by_name(ctx->stmt_data, column_name);
    if (ed == NULL)
        return;

    /* 登记列信息并标记需要加密 */
    {
        TeccParamMapEntry *entry = &ctx->param_map[param_index];

        if (entry->schema_name == NULL && ed->schema_name[0] != '\0')
            entry->schema_name = pg_strdup(ed->schema_name);
        if (entry->table_name == NULL && ed->table_name[0] != '\0')
            entry->table_name = pg_strdup(ed->table_name);
        if (entry->column_name == NULL && ed->column_name[0] != '\0')
            entry->column_name = pg_strdup(ed->column_name);
        entry->needs_encryption = true;

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "WHERE param $%d mapped to encrypted column %s.%s.%s",
                           param_index + 1,
                           entry->schema_name ? entry->schema_name : "?",
                           entry->table_name ? entry->table_name : "?",
                           entry->column_name ? entry->column_name : "?");
    }
}

/*
 * ReqID:TECC4.0 - 递归遍历表达式树，在 A_Expr 比较节点上补全参数映射。
 * 遍历骨架与 find_params_in_expression_recursive 一致（支持语句节点、
 * WHERE/HAVING 子句、目标列表、VALUES 子句等）。
 */
static void
tecc_map_comparison_params_recursive(const Node *expr,
                                     TeccComparisonMapContext *ctx)
{
    ListCell *lc;

    if (expr == NULL || ctx == NULL)
        return;

    switch (expr->type)
    {
        case FE_T_A_Expr:
            /* 处理比较操作符（列 = $N 或 $N = 列） */
            tecc_map_comparison_param((A_Expr *) expr, ctx);
            if (((const A_Expr *) expr)->lexpr != NULL)
                tecc_map_comparison_params_recursive(
                    ((const A_Expr *) expr)->lexpr, ctx);
            if (((const A_Expr *) expr)->rexpr != NULL)
                tecc_map_comparison_params_recursive(
                    ((const A_Expr *) expr)->rexpr, ctx);
            break;

        case FE_T_FuncCall:
            {
                const FuncCall *func_call = (const FuncCall *) expr;

                if (func_call->args != NIL)
                {
                    for (lc = func_call->args->head; lc != NULL; lc = lc->next)
                        tecc_map_comparison_params_recursive(
                            (const Node *) lc->data.ptr_value, ctx);
                }
            }
            break;

        /* ReqID:TECC4.0 - INSERT 语句：遍历 VALUES/SELECT、ON CONFLICT、
         * RETURNING 子句中的比较。 */
        case FE_T_InsertStmt:
            {
                const InsertStmt *insert_stmt = (const InsertStmt *) expr;

                if (insert_stmt->selectStmt != NULL)
                    tecc_map_comparison_params_recursive(
                        insert_stmt->selectStmt, ctx);
                if (insert_stmt->onConflictClause != NIL)
                {
                    for (lc = insert_stmt->onConflictClause->head; lc != NULL; lc = lc->next)
                        tecc_map_comparison_params_recursive(
                            (const Node *) lc->data.ptr_value, ctx);
                }
                if (insert_stmt->returningList != NIL)
                {
                    for (lc = insert_stmt->returningList->head; lc != NULL; lc = lc->next)
                        tecc_map_comparison_params_recursive(
                            (const Node *) lc->data.ptr_value, ctx);
                }
            }
            break;

        /* ReqID:TECC4.0 - UPDATE 语句：WHERE 比较参数映射 */
        case FE_T_UpdateStmt:
            {
                const UpdateStmt *update_stmt = (const UpdateStmt *) expr;

                if (update_stmt->targetList != NIL)
                {
                    for (lc = update_stmt->targetList->head; lc != NULL; lc = lc->next)
                        tecc_map_comparison_params_recursive(
                            (const Node *) lc->data.ptr_value, ctx);
                }
                if (update_stmt->whereClause != NULL)
                    tecc_map_comparison_params_recursive(
                        update_stmt->whereClause, ctx);
                if (update_stmt->returningList != NIL)
                {
                    for (lc = update_stmt->returningList->head; lc != NULL; lc = lc->next)
                        tecc_map_comparison_params_recursive(
                            (const Node *) lc->data.ptr_value, ctx);
                }
            }
            break;

        /* ReqID:TECC4.0 - DELETE 语句：WHERE 比较参数映射 */
        case FE_T_DeleteStmt:
            {
                const DeleteStmt *delete_stmt = (const DeleteStmt *) expr;

                if (delete_stmt->whereClause != NULL)
                    tecc_map_comparison_params_recursive(
                        delete_stmt->whereClause, ctx);
                if (delete_stmt->returningList != NIL)
                {
                    for (lc = delete_stmt->returningList->head; lc != NULL; lc = lc->next)
                        tecc_map_comparison_params_recursive(
                            (const Node *) lc->data.ptr_value, ctx);
                }
            }
            break;

        /* ReqID:TECC4.0 - FE_T_SubLink not supported in frontend parser */
        case FE_T_SelectStmt:
            {
                const SelectStmt *select_stmt = (const SelectStmt *) expr;

                if (select_stmt->targetList != NIL)
                {
                    for (lc = select_stmt->targetList->head; lc != NULL; lc = lc->next)
                        tecc_map_comparison_params_recursive(
                            (const Node *) lc->data.ptr_value, ctx);
                }
                if (select_stmt->valuesLists != NIL)
                {
                    for (lc = select_stmt->valuesLists->head; lc != NULL; lc = lc->next)
                        tecc_map_comparison_params_recursive(
                            (const Node *) lc->data.ptr_value, ctx);
                }
                if (select_stmt->whereClause != NULL)
                    tecc_map_comparison_params_recursive(
                        select_stmt->whereClause, ctx);
                if (select_stmt->havingClause != NULL)
                    tecc_map_comparison_params_recursive(
                        select_stmt->havingClause, ctx);
                if (select_stmt->sortClause != NIL)
                {
                    for (lc = select_stmt->sortClause->head; lc != NULL; lc = lc->next)
                        tecc_map_comparison_params_recursive(
                            (const Node *) lc->data.ptr_value, ctx);
                }
            }
            break;

        case FE_T_SortBy:
            if (((const SortBy *) expr)->node != NULL)
                tecc_map_comparison_params_recursive(
                    ((const SortBy *) expr)->node, ctx);
            break;

        case FE_T_ResTarget:
            if (((const ResTarget *) expr)->val != NULL)
                tecc_map_comparison_params_recursive(
                    ((const ResTarget *) expr)->val, ctx);
            break;

        case FE_T_TypeCast:
            if (((const TypeCast *) expr)->arg != NULL)
                tecc_map_comparison_params_recursive(
                    ((const TypeCast *) expr)->arg, ctx);
            break;

        /* ReqID:TECC4.0 - 布尔表达式（AND/OR）：BETWEEN 展开的比较参数 */
        case FE_T_BoolExpr:
            {
                const BoolExpr *bool_expr = (const BoolExpr *) expr;

                if (bool_expr->args != NIL)
                {
                    for (lc = bool_expr->args->head; lc != NULL; lc = lc->next)
                        tecc_map_comparison_params_recursive(
                            (const Node *) lc->data.ptr_value, ctx);
                }
            }
            break;

        case FE_T_List:
            {
                const List *list = (const List *) expr;

                for (lc = list->head; lc != NULL; lc = lc->next)
                    tecc_map_comparison_params_recursive(
                        (const Node *) lc->data.ptr_value, ctx);
            }
            break;

        default:
            /* 其他节点类型，不包含子表达式 */
            break;
    }
}

/*
 * ReqID:TECC4.0 - 补全参数映射：遍历解析树中的比较操作符，把与加密列
 * 比较的参数（WHERE/HAVING 中的 col = $N）映射到对应列。
 *
 * 在 tecc_extract_param_mapping_enhanced 建立基础映射后调用。
 * 此前 WHERE 处理器只标记"参数将由参数处理器处理"，不登记列信息，
 * 导致 tecc_param_needs_encryption_enhanced 无法识别加密需求（参数以
 * 明文发送，服务端报 "invalid encrypted data: too short"）。
 */
static void
tecc_map_comparison_params(TeccStatementData *stmt_data,
                           TeccParamMapEntry *param_map,
                           int param_map_size)
{
    TeccComparisonMapContext ctx;
    ListCell *lc;

    if (stmt_data == NULL || stmt_data->parse_tree == NIL ||
        param_map == NULL || param_map_size <= 0)
        return;

    memset(&ctx, 0, sizeof(ctx));
    ctx.stmt_data = stmt_data;
    ctx.param_map = param_map;
    ctx.param_map_size = param_map_size;

    /* parse_tree 是语句列表（List 元素为语句节点） */
    foreach(lc, stmt_data->parse_tree)
    {
        Node *stmt = (Node *) lfirst(lc);

        if (stmt != NULL)
            tecc_map_comparison_params_recursive(stmt, &ctx);
    }
}

/*
 * ReqID:TECC4.0 - 补全 INSERT 参数映射：VALUES 中的参数按列位置对应列
 * （$N → 第 N 列）。若该列是加密列，把列信息填入 param_map[param_index]。
 *
 * 与 WHERE 比较不同，INSERT VALUES 的参数没有"列 = $N"的 A_Expr 结构，
 * 而是按位置对应（INSERT INTO t VALUES ($1, $2, $3) → $3 对应第 3 列）。
 * 显式列名时按 cols 列表；无列名时按表元数据的 attnum 顺序。
 */
static void
tecc_map_insert_params(TeccStatementData *stmt_data,
                       TeccParamMapEntry *param_map,
                       int param_map_size)
{
    ListCell *lc;

    if (stmt_data == NULL || stmt_data->parse_tree == NIL ||
        param_map == NULL || param_map_size <= 0)
        return;

    foreach(lc, stmt_data->parse_tree)
    {
        Node *stmt = (Node *) lfirst(lc);

        if (stmt == NULL || nodeTag(stmt) != FE_T_InsertStmt)
            continue;
        {
            InsertStmt *insert_stmt = (InsertStmt *) stmt;
            SelectStmt *select_stmt;
            List *cols;
            int vrow;

            if (insert_stmt->selectStmt == NULL ||
                nodeTag(insert_stmt->selectStmt) != FE_T_SelectStmt)
                continue;
            select_stmt = (SelectStmt *) insert_stmt->selectStmt;
            if (select_stmt->valuesLists == NIL)
                continue;

            cols = insert_stmt->cols;   /* 显式列名列表（可为 NIL） */

            for (vrow = 0; vrow < list_length(select_stmt->valuesLists); vrow++)
            {
                void *values_row = list_nth(select_stmt->valuesLists, vrow);
                int velem;
                int col_pos;

                if (values_row == NULL)
                    continue;

                /* 每行元素按位置对应列：元素 i → 列位置 i+1 */
                for (velem = 0; velem < list_length((List *) values_row); velem++)
                {
                    void *vitem = list_nth((List *) values_row, velem);
                    const Param *param;
                    int param_index;
                    const char *column_name = NULL;
                    const char *schema_name;
                    const char *table_name;
                    TeccColumnCacheItem **enc_cols = NULL;
                    int enc_count = 0;
                    int k;

                    if (vitem == NULL || ((Node *) vitem)->type != FE_T_Param)
                        continue;
                    param = (const Param *) vitem;
                    param_index = param->paramid - 1;
                    if (param_index < 0 || param_index >= param_map_size)
                        continue;

                    col_pos = velem + 1;    /* attnum 从 1 开始 */

                    /* 确定列名：显式列名优先，否则按位置从缓存取 */
                    if (cols != NIL && velem < list_length(cols))
                    {
                        ResTarget *col_target = (ResTarget *) list_nth(cols, velem);
                        if (col_target != NULL && col_target->name != NULL)
                            column_name = col_target->name;
                    }

                    if (column_name == NULL)
                    {
                        /* 无显式列名：从缓存按位置取加密列 */
                        TeccState *state = stmt_data->conn ?
                            tecc_get_state(stmt_data->conn) : NULL;
                        const char *db = stmt_data->conn ? PQdb(stmt_data->conn) : NULL;

                        if (state != NULL && db != NULL)
                        {
                            table_name = insert_stmt->relation ?
                                insert_stmt->relation->relname : NULL;
                            schema_name = insert_stmt->relation ?
                                tecc_stmt_resolve_table_schema(stmt_data,
                                    insert_stmt->relation->schemaname,
                                    insert_stmt->relation->relname) : NULL;
                            if (table_name != NULL)
                            {
                                enc_cols = tecc_cache_get_columns_by_table_name(
                                    state, db, schema_name, table_name, &enc_count);
                                for (k = 0; k < enc_count; k++)
                                {
                                    if (enc_cols[k] != NULL &&
                                        enc_cols[k]->column_attnum == col_pos)
                                    {
                                        column_name = enc_cols[k]->column_name;
                                        break;
                                    }
                                }
                                if (enc_cols != NULL)
                                    pg_free(enc_cols);
                            }
                        }
                    }

                    if (column_name == NULL)
                        continue;

                    /* ReqID:TECC4.0 - 校验该列确为加密列（显式列名时也要查，
                     * 避免非加密列被误映射加密——如 INSERT (id, salary)
                     * VALUES ($1, $2) 中 $1 对应非加密 id 列）。 */
                    {
                        TeccState *state = stmt_data->conn ?
                            tecc_get_state(stmt_data->conn) : NULL;
                        const char *db = stmt_data->conn ? PQdb(stmt_data->conn) : NULL;
                        TeccColumnCacheItem *col_info = NULL;

                        if (state != NULL && db != NULL)
                        {
                            table_name = insert_stmt->relation ?
                                insert_stmt->relation->relname : NULL;
                            schema_name = insert_stmt->relation ?
                                tecc_stmt_resolve_table_schema(stmt_data,
                                    insert_stmt->relation->schemaname,
                                    insert_stmt->relation->relname) : NULL;
                            if (table_name != NULL)
                                col_info = tecc_cache_get_encrypted_column_by_name(
                                    state, db, schema_name, table_name, column_name);
                        }
                        if (col_info == NULL)
                            continue;   /* 非加密列，跳过 */
                    }

                    /* 登记列信息并标记需要加密 */
                    {
                        TeccParamMapEntry *entry = &param_map[param_index];
                        const char *tbl = insert_stmt->relation ?
                            insert_stmt->relation->relname : NULL;
                        const char *sch = insert_stmt->relation ?
                            tecc_stmt_resolve_table_schema(stmt_data,
                                insert_stmt->relation->schemaname,
                                insert_stmt->relation->relname) : NULL;

                        if (entry->schema_name == NULL && sch != NULL)
                            entry->schema_name = pg_strdup(sch);
                        if (entry->table_name == NULL && tbl != NULL)
                            entry->table_name = pg_strdup(tbl);
                        if (entry->column_name == NULL)
                            entry->column_name = pg_strdup(column_name);
                        entry->needs_encryption = true;

                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                           "INSERT param $%d mapped to column %s.%s.%s (pos %d)",
                                           param_index + 1,
                                           sch ? sch : "?",
                                           tbl ? tbl : "?",
                                           column_name, col_pos);
                    }
                }
            }
        }
    }
}

/*
 * ReqID:TECC4.0 - 补全 UPDATE SET 参数映射：SET 子句的列名直接对应
 * （SET salary = $1 → $1 对应 salary 列）。若该列是加密列，把列信息
 * 填入 param_map[param_index]。
 */
static void
tecc_map_update_params(TeccStatementData *stmt_data,
                       TeccParamMapEntry *param_map,
                       int param_map_size)
{
    ListCell *lc;

    if (stmt_data == NULL || stmt_data->parse_tree == NIL ||
        param_map == NULL || param_map_size <= 0)
        return;

    foreach(lc, stmt_data->parse_tree)
    {
        Node *stmt = (Node *) lfirst(lc);

        if (stmt == NULL || nodeTag(stmt) != FE_T_UpdateStmt)
            continue;
        {
            UpdateStmt *update_stmt = (UpdateStmt *) stmt;
            const char *table_name = update_stmt->relation ?
                update_stmt->relation->relname : NULL;
            const char *schema_name = update_stmt->relation ?
                tecc_stmt_resolve_table_schema(stmt_data,
                    update_stmt->relation->schemaname,
                    update_stmt->relation->relname) : NULL;
            ListCell *tlc;

            if (update_stmt->targetList == NIL || table_name == NULL)
                continue;

            foreach(tlc, update_stmt->targetList)
            {
                ResTarget *target = (ResTarget *) lfirst(tlc);

                if (target == NULL || target->name == NULL ||
                    target->val == NULL ||
                    nodeTag(target->val) != FE_T_Param)
                    continue;
                {
                    const Param *param = (const Param *) target->val;
                    int param_index = param->paramid - 1;
                    TeccParamMapEntry *entry;

                    if (param_index < 0 || param_index >= param_map_size)
                        continue;

                    /* 校验该列是否为加密列（用缓存） */
                    {
                        TeccState *state = stmt_data->conn ?
                            tecc_get_state(stmt_data->conn) : NULL;
                        const char *db = stmt_data->conn ?
                            PQdb(stmt_data->conn) : NULL;
                        TeccColumnCacheItem *col_info = NULL;

                        if (state != NULL && db != NULL)
                            col_info = tecc_cache_get_encrypted_column_by_name(
                                state, db, schema_name, table_name, target->name);
                        if (col_info == NULL)
                            continue;
                    }

                    entry = &param_map[param_index];
                    if (entry->schema_name == NULL && schema_name != NULL)
                        entry->schema_name = pg_strdup(schema_name);
                    if (entry->table_name == NULL)
                        entry->table_name = pg_strdup(table_name);
                    if (entry->column_name == NULL)
                        entry->column_name = pg_strdup(target->name);
                    entry->needs_encryption = true;

                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                       "UPDATE SET param $%d mapped to column %s.%s.%s",
                                       param_index + 1,
                                       schema_name ? schema_name : "?",
                                       table_name, target->name);
                }
            }
        }
    }
}

/*
 * 从解析树中提取增强参数映射
 */
int
tecc_extract_param_mapping_enhanced(TeccStatementData *stmt_data)
{
    List *param_list = NIL;
    ListCell *lc;
    int param_count = 0;
    TeccParamMapEntry *param_map = NULL;

    if (stmt_data == NULL || stmt_data->parse_tree == NIL) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "No parse tree available for enhanced parameter mapping");
        return -1;
    }

    /* 检查是否有参数需要处理 */
    if (stmt_data->nParams == 0) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "No parameters to map");
        return -1;
    }

    

    /* 在解析树中查找所有参数 */
    param_list = tecc_find_params_in_expression((const Node *)stmt_data->parse_tree, 0);

    /* 统计找到的参数数量 */
    param_count = list_length(param_list);
    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Found %d parameter references in parse tree",
                        param_count);

    /* 创建参数映射表 */
    if (param_count > 0) {
        param_map = pg_malloc0(sizeof(TeccParamMapEntry) * stmt_data->nParams);
        if (param_map == NULL) {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "Memory allocation failed for enhanced parameter map");
            tecc_free_param_info_list(param_list);
            return -1;
        }

        /* 初始化所有参数映射项 */
        for (int i = 0; i < stmt_data->nParams; i++) {
            param_map[i].param_index = i;
            param_map[i].column_name = NULL;
            param_map[i].table_name = NULL;
            param_map[i].schema_name = NULL;
            param_map[i].needs_encryption = false;
            param_map[i].encrypted_value = NULL;
            param_map[i].encrypted_value_size = 0;
            param_map[i].expression_context = NULL;
            param_map[i].expression_depth = 0;
            param_map[i].inferred_type_oid = InvalidOid;
            param_map[i].is_in_function_call = false;
            param_map[i].is_in_subquery = false;
            param_map[i].is_in_op_expr = false;
        }

        /* 处理找到的参数信息 */
        foreach(lc, param_list) {
            TeccParamInfo *param_info = (TeccParamInfo *)lfirst(lc);
            int param_index = param_info->param_index;

            if (param_index >= 0 && param_index < stmt_data->nParams) {
                TeccParamMapEntry *entry = &param_map[param_index];

                /* 设置上下文信息 */
                entry->expression_context = param_info->parent_context;
                entry->expression_depth = param_info->expression_depth;
                entry->is_in_function_call = param_info->is_in_function_call;
                entry->is_in_subquery = param_info->is_in_subquery;
                entry->is_in_op_expr = param_info->is_in_op_expr;

                /* 推断参数类型 */
                entry->inferred_type_oid = tecc_infer_param_type(param_info, stmt_data);

                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Enhanced mapping for parameter %d: depth=%d, in_func=%d, in_subquery=%d, in_op=%d",
                                   param_index,
                                   entry->expression_depth,
                                   entry->is_in_function_call,
                                   entry->is_in_subquery,
                                   entry->is_in_op_expr);
            }
        }

        /* 设置statement_data中的参数映射 */
        if (stmt_data->param_map != NULL) {
            pg_free(stmt_data->param_map);
        }

        stmt_data->param_map = param_map;
        stmt_data->param_map_size = stmt_data->nParams;

        
    } else {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "No parameter references found in parse tree");
    }

    /* 清理参数信息列表 */
    tecc_free_param_info_list(param_list);

    /* ReqID:TECC4.0 - 补全比较操作符参数映射（WHERE/HAVING 中
     * 加密列 = $N 的参数需加密，此前缺失导致明文发送）。 */
    tecc_map_comparison_params(stmt_data,
                               stmt_data->param_map,
                               stmt_data->param_map_size);

    /* ReqID:TECC4.0 - 补全 INSERT VALUES 参数映射（$N 按列位置对应加密列）。 */
    tecc_map_insert_params(stmt_data,
                           stmt_data->param_map,
                           stmt_data->param_map_size);

    /* ReqID:TECC4.0 - 补全 UPDATE SET 参数映射（SET col = $N 按列名对应）。 */
    tecc_map_update_params(stmt_data,
                           stmt_data->param_map,
                           stmt_data->param_map_size);

    return 0;
}

/* ==================== 参数类型推断 ==================== */

/*
 * 根据上下文推断参数类型
 */
Oid
tecc_infer_param_type(const TeccParamInfo *param_info,
                     const TeccStatementData *stmt_data)
{
    Oid inferred_type = InvalidOid;

    if (param_info == NULL || stmt_data == NULL) {
        return InvalidOid;
    }

    /* 根据上下文推断类型 */
    if (param_info->is_in_function_call) {
        /* 在函数调用中，尝试推断函数参数类型 */
        inferred_type = infer_type_from_function_context(param_info, stmt_data);
    } else if (param_info->is_in_op_expr) {
        /* 在操作符表达式中，尝试推断操作数类型 */
        inferred_type = infer_type_from_operator_context(param_info, stmt_data);
    } else if (param_info->is_in_subquery) {
        /* 在子查询中，尝试推断子查询上下文类型 */
        inferred_type = infer_type_from_subquery_context(param_info, stmt_data);
    }

    /* 如果无法推断，使用默认类型 */
    if (inferred_type == InvalidOid) {
        /* 检查是否有显式的参数类型信息 */
        if (stmt_data->paramTypes != NULL &&
            param_info->param_index >= 0 &&
            param_info->param_index < stmt_data->nParams) {
            inferred_type = stmt_data->paramTypes[param_info->param_index];
        }

        /* 如果还是没有类型信息，使用默认的text类型 */
        if (inferred_type == InvalidOid) {
            inferred_type = TEXTOID; /* 假设TEXTOID已定义 */
        }
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Inferred type OID %u for parameter %d",
                       inferred_type, param_info->param_index);

    return inferred_type;
}

/*
 * 从函数调用上下文推断类型（简化实现）
 */
static Oid
infer_type_from_function_context(const TeccParamInfo *param_info,
                                const TeccStatementData *stmt_data)
{
    /* 简化实现：返回默认的text类型 */
    (void)param_info;
    (void)stmt_data;
    return TEXTOID;
}

/*
 * 从操作符上下文推断类型（简化实现）
 */
static Oid
infer_type_from_operator_context(const TeccParamInfo *param_info,
                                const TeccStatementData *stmt_data)
{
    /* 简化实现：返回默认的text类型 */
    (void)param_info;
    (void)stmt_data;
    return TEXTOID;
}

/*
 * 从子查询上下文推断类型（简化实现）
 */
static Oid
infer_type_from_subquery_context(const TeccParamInfo *param_info,
                                const TeccStatementData *stmt_data)
{
    /* 简化实现：返回默认的text类型 */
    (void)param_info;
    (void)stmt_data;
    return TEXTOID;
}

/* ==================== 增强参数加密需求检查 ==================== */

/*
 * 检查参数是否需要加密（增强版本）
 */
bool
tecc_param_needs_encryption_enhanced(const TeccParamInfo *param_info,
                                    const TeccStatementData *stmt_data)
{
    bool needs_encryption = false;

    if (param_info == NULL || stmt_data == NULL) {
        return false;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Checking enhanced encryption need for parameter %d",
                       param_info->param_index);

    /* 检查参数映射表中是否有该参数的信息 */
    if (stmt_data->param_map != NULL &&
        param_info->param_index >= 0 &&
        param_info->param_index < stmt_data->param_map_size) {

        TeccParamMapEntry *param_entry = &stmt_data->param_map[param_info->param_index];

        /* 如果参数映射表中已经标记了是否需要加密，直接返回 */
        if (param_entry->needs_encryption) {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Parameter %d marked as needing encryption in param_map",
                               param_info->param_index);
            return true;
        }

        /* 检查是否有列名信息 */
        if (param_entry->column_name != NULL && param_entry->table_name != NULL) {
            /* 查询缓存检查是否加密 */
            needs_encryption = check_column_encryption_from_cache(
                stmt_data,
                param_entry->schema_name,
                param_entry->table_name,
                param_entry->column_name
            );

            if (needs_encryption) {
                /* 标记参数需要加密 */
                param_entry->needs_encryption = true;
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Parameter %d corresponds to encrypted column %s.%s",
                                   param_info->param_index,
                                   param_entry->table_name,
                                   param_entry->column_name);
            }

            return needs_encryption;
        }
    }

    /* 如果没有列名信息，使用上下文推断 */
    needs_encryption = infer_encryption_from_context(param_info, stmt_data);

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Enhanced encryption check for parameter %d: %s",
                       param_info->param_index,
                       needs_encryption ? "needs encryption" : "no encryption needed");

    return needs_encryption;
}

/*
 * 从缓存检查列是否加密
 */
static bool
check_column_encryption_from_cache(const TeccStatementData *stmt_data,
                                  const char *schema_name,
                                  const char *table_name,
                                  const char *column_name)
{
    TeccState *state = NULL;
    const char *database_name = NULL;
    const char *actual_schema_name = schema_name;
    TeccColumnCacheItem *column_info;

    if (stmt_data == NULL || column_name == NULL || table_name == NULL) {
        return false;
    }

    /* 获取TECC状态 */
    if (stmt_data->conn != NULL) {
        state = tecc_get_state(stmt_data->conn);
        if (state == NULL) {
            tecc_processor_debug(TECC_LOG_WARNING,
                               "No TECC state available for cache query");
            return false;
        }
    } else {
        tecc_processor_debug(TECC_LOG_WARNING,
                           "No connection available for cache query");
        return false;
    }

    /* 获取数据库名 */
    database_name = PQdb(stmt_data->conn);
    if (database_name == NULL || database_name[0] == '\0') {
        tecc_processor_debug(TECC_LOG_WARNING,
                           "Could not get database name from connection");
        return false;
    }

    /* 如果schema名为空，获取当前schema */
    if (actual_schema_name == NULL || actual_schema_name[0] == '\0') {
        actual_schema_name = tecc_cache_get_guc_param(state, "current_schema");
        if (actual_schema_name == NULL || actual_schema_name[0] == '\0') {
            tecc_processor_debug(TECC_LOG_WARNING,
                               "Could not get current schema from cache");
            return false;
        }
    }

    /* 查询加密列信息 */
    column_info = tecc_cache_get_encrypted_column_by_name(
        state,
        database_name,
        actual_schema_name,
        table_name,
        column_name
    );

    return (column_info != NULL);
}

/*
 * 从上下文推断加密需求
 */
static bool
infer_encryption_from_context(const TeccParamInfo *param_info,
                             const TeccStatementData *stmt_data)
{
    /* 简化实现：检查查询中是否包含加密类型提示 */
    if (stmt_data->query != NULL && strstr(stmt_data->query, "_enc") != NULL) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Parameter %d may need encryption (query contains '_enc')",
                           param_info->param_index);
        return true;
    }

    /* 检查参数是否在加密列上下文中 */
    if (param_info->is_in_function_call) {
        /* 在函数调用中，检查函数是否处理加密数据 */
        return check_function_encryption_context(param_info, stmt_data);
    } else if (param_info->is_in_op_expr) {
        /* 在操作符表达式中，检查操作符是否处理加密数据 */
        return check_operator_encryption_context(param_info, stmt_data);
    }

    return false;
}

/*
 * 检查函数加密上下文（简化实现）
 */
static bool
check_function_encryption_context(const TeccParamInfo *param_info,
                                 const TeccStatementData *stmt_data)
{
    /* 简化实现：返回false */
    (void)param_info;
    (void)stmt_data;
    return false;
}

/*
 * 检查操作符加密上下文（简化实现）
 */
static bool
check_operator_encryption_context(const TeccParamInfo *param_info,
                                 const TeccStatementData *stmt_data)
{
    /* 简化实现：返回false */
    (void)param_info;
    (void)stmt_data;
    return false;
}

/* ==================== 复杂参数处理 ==================== */

/*
 * 处理复杂参数
 */
TeccProcessorResult
tecc_process_complex_parameters(TeccStatementData *stmt_data)
{
    int mapping_result;

    if (stmt_data == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid statement_data in complex parameter processing");
        return TECC_PROCESSOR_INVALID_INPUT;
    }

    

    /* 提取增强参数映射 */
    mapping_result = tecc_extract_param_mapping_enhanced(stmt_data);
    if (mapping_result != 0) {
        tecc_processor_debug(TECC_LOG_WARNING,
                           "Enhanced parameter mapping extraction failed");
        return TECC_PROCESSOR_ERROR;
    }

    

    return TECC_PROCESSOR_SUCCESS;
}

/* ==================== 工具函数 ==================== */

/*
 * 创建增强参数映射项
 */
TeccParamMapEntry *
tecc_create_enhanced_param_entry(const TeccParamInfo *param_info,
                                const TeccStatementData *stmt_data)
{
    TeccParamMapEntry *entry = NULL;

    if (param_info == NULL || stmt_data == NULL) {
        return NULL;
    }

    entry = pg_malloc0(sizeof(TeccParamMapEntry));
    if (entry == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Memory allocation failed for enhanced param entry");
        return NULL;
    }

    /* 设置基本字段 */
    entry->param_index = param_info->param_index;
    entry->column_name = NULL;
    entry->table_name = NULL;
    entry->schema_name = NULL;
    entry->needs_encryption = false;
    entry->encrypted_value = NULL;
    entry->encrypted_value_size = 0;

    /* 设置增强字段 */
    entry->expression_context = param_info->parent_context;
    entry->expression_depth = param_info->expression_depth;
    entry->inferred_type_oid = tecc_infer_param_type(param_info, stmt_data);
    entry->is_in_function_call = param_info->is_in_function_call;
    entry->is_in_subquery = param_info->is_in_subquery;
    entry->is_in_op_expr = param_info->is_in_op_expr;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Created enhanced param entry for parameter %d",
                       param_info->param_index);

    return entry;
}

/*
 * 释放参数信息列表
 */
void
tecc_free_param_info_list(List *param_list)
{
    ListCell *lc;

    if (param_list == NIL) {
        return;
    }

    foreach(lc, param_list) {
        TeccParamInfo *param_info = (TeccParamInfo *)lfirst(lc);
        if (param_info != NULL) {
            pg_free(param_info);
        }
    }

    list_free(param_list);
}

/*
 * 调试函数：打印参数信息
 */
void
tecc_debug_print_param_info(const TeccParamInfo *param_info)
{
    if (param_info == NULL) {
        tecc_processor_debug(TECC_LOG_DEBUG1, "ParamInfo: NULL");
        return;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "ParamInfo[%d]: depth=%d, type=%d, in_func=%d, in_subquery=%d, in_op=%d",
                       param_info->param_index,
                       param_info->expression_depth,
                       param_info->value_type,
                       param_info->is_in_function_call,
                       param_info->is_in_subquery,
                       param_info->is_in_op_expr);
}
/* End - ReqID:TECC4.0 */