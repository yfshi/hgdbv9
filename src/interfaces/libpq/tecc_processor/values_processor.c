#ifdef _MSC_VER
#define TECC_ATTR_UNUSED
#else
#define TECC_ATTR_UNUSED __attribute__((unused))
#endif

/* Begin - ReqID:TECC4.0 */
/*
 * File: values_processor.c
 *
 * Abstract:
 *   Values processor implementation for TECC processor module.
 *   Handles encryption/decryption of values in SQL statements
 *   and manages value processing workflows.
 *
 * Authored by <wangxp@highgo.com>, 20260403.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/values_processor.c
 */

/* 前端代码必须包含postgres_fe.h作为第一个头文件 */
#include "postgres_fe.h"
#include <math.h>
#include "common/fe_memutils.h"

#include "values_processor.h"

/* 包含处理器内部函数，用于内存管理 */
#include "internal/tecc_processor_internal.h"

/* Use frontend adapter types instead of backend headers */
/* Note: All types should be defined in fe_adapter.h or tecc_nodes.h */
#include "../frontend_parser/fe_adapter.h"

/* Include libpq for PQunescapeBytea */
#include "libpq-fe.h"

/* Forward declarations for external module interfaces */
#include "../tecc_scheduler/tecc_crypto_scheduler.h"  /* Crypto scheduler interface */
#include "../tecc_formatter/tecc_formatter.h"         /* Data formatter interface */
#include "../tecc_cache/tecc_metadata_cache.h"        /* Metadata cache interface */
#include "../tecc_libpq_integration/fe-tecc-integration.h"  /* TECC integration interface for tecc_get_state */

/* Include common type definitions for OID constants */
#include "../tecc_common/tecc_common_types.h"

/* Include enhanced parameter expression processor */
#include "param_expression_processor.h"

/* ReqID:TECC4.0 - 基本类型OID到加密类型名的映射表
 * 用于INSERT/UPDATE重写时添加::type_name类型转换 */
static const struct {
    Oid         base_type_oid;
    const char *enc_type_name;
} base_to_enc_type_map[] = {
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

/* ReqID:TECC4.0 - 加密类型OID到基础类型OID的映射表 */
static const struct {
    Oid         enc_type_oid;
    Oid         base_type_oid;
} enc_to_base_oid_map[] = {
    {INT2_ENC_OID, INT2OID},
    {INT4_ENC_OID, INT4OID},
    {INT8_ENC_OID, INT8OID},
    {FLOAT4_ENC_OID, FLOAT4OID},
    {FLOAT8_ENC_OID, FLOAT8OID},
    {NUMERIC_ENC_OID, NUMERICOID},
    {VARCHAR_ENC_OID, VARCHAROID},
    {BPCHAR_ENC_OID, BPCHAROID},
    {TEXT_ENC_OID, TEXTOID},
    {InvalidOid, InvalidOid}
};

/* ReqID:TECC4.0 - 根据基本类型OID获取加密类型名
 * 支持传入基础类型OID（如INT4OID=23）或加密类型OID（如INT4_ENC_OID=50001） */
const char *
get_enc_type_name_from_base_oid(Oid type_oid)
{
    int i;
    Oid base_type_oid = type_oid;

    /* 如果传入的是加密类型OID（>=50000），转换为基础类型OID */
    if (type_oid >= INT2_ENC_OID && type_oid <= TEXT_ENC_OID)
    {
        for (i = 0; enc_to_base_oid_map[i].enc_type_oid != InvalidOid; i++)
        {
            if (enc_to_base_oid_map[i].enc_type_oid == type_oid)
            {
                base_type_oid = enc_to_base_oid_map[i].base_type_oid;
                break;
            }
        }
    }

    for (i = 0; base_to_enc_type_map[i].enc_type_name != NULL; i++)
    {
        if (base_to_enc_type_map[i].base_type_oid == base_type_oid)
            return base_to_enc_type_map[i].enc_type_name;
    }
    return NULL;
}


/*
 * Forward declarations
 */
static TeccValuesProcessorResult values_process_list(TeccRawValuesList *list,
                                                    TeccStatementData *statement_data);
static TeccValuesProcessorResult values_process_single_value(TeccRawValue *value,
                                                            TeccStatementData *statement_data);
static bool values_column_needs_encryption(const char *schema_name,
                                          const char *table_name,
                                          const char *column_name);
static TeccDecryptResult tecc_process_null_value(unsigned char **plain_text,
                                                size_t *plain_text_size,
                                                int original_typeid,
                                                TeccProcessStatus process_status);
static void tecc_process_text_format(unsigned char **plain_text,
                                    size_t *plain_text_size,
                                    TeccProcessStatus process_status,
                                    int original_typeid);
static void tecc_process_binary_format(unsigned char **plain_text,
                                      size_t *plain_text_size,
                                      TeccProcessStatus process_status,
                                      int original_typeid);
static void tecc_free_param_map(TeccParamMapEntry *param_map, int param_map_size);
static bool tecc_parameter_needs_encryption(TeccStatementData *statement_data,
                                           int param_index,
                                           Oid param_type,
                                           const char *param_value);
static int tecc_extract_param_mapping_from_parse_tree(TeccStatementData *statement_data,
                                                     TeccParamMapEntry **param_map,
                                                     int *param_map_size);

/*
 * Main value processing entry point
 */
TeccValuesProcessorResult
tecc_values_processor_process(TeccStatementData *statement_data)
{
    /* Begin - ReqID:TECC4.0 - 值处理器主处理函数 */
    TeccValuesProcessorContext *context;
    TeccValuesProcessorResult result = TECC_VALUES_PROCESSOR_SUCCESS;

    if (statement_data == NULL)
        return TECC_VALUES_PROCESSOR_ERROR;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Starting values processing");

    /* 1. 创建处理上下文 */
    context = tecc_values_processor_context_create();
    if (context == NULL)
    {
        tecc_values_processor_set_error(statement_data, TECC_PROCESSOR_MEMORY_ERROR,
                                       "Failed to create values processor context");
        return TECC_VALUES_PROCESSOR_MEMORY_ERROR;
    }

    /* 2. 检查是否有加密列需要处理 */
    if (statement_data->encrypted_columns == NIL)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "No encrypted columns to process");
        tecc_values_processor_context_destroy(context);
        return TECC_VALUES_PROCESSOR_SUCCESS;
    }

    context->encrypted_columns = statement_data->encrypted_columns;
    context->needs_processing = true;
    

    /* 3. 根据语句类型调用相应的处理函数 */
    if (statement_data->parse_tree != NIL)
    {
        ListCell *lc;
        int stmt_count = 0;
        int processed_count = 0;

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Processing parse tree with %d statements",
                           list_length(statement_data->parse_tree));

        /* 遍历解析树中的所有语句 */
        foreach(lc, statement_data->parse_tree)
        {
            Node *stmt = (Node *) lfirst(lc);
            NodeTag stmt_type;

            if (stmt == NULL)
                continue;

            stmt_count++;
            stmt_type = nodeTag(stmt);

            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Processing statement %d, type: %d",
                               stmt_count, stmt_type);

            switch (stmt_type)
            {
                case FE_T_InsertStmt:
                    {
                        struct InsertStmt *insert_stmt = (struct InsertStmt *) stmt;
                        List *values_lists = NIL;

                        /* 安全检查 */
                        if (insert_stmt == NULL) {
                            tecc_processor_debug(TECC_LOG_WARNING,
                                               "INSERT statement pointer is NULL");
                            break;
                        }

                        /* 获取值列表 */
                        /* 在PostgreSQL中，INSERT VALUES通过selectStmt字段表示 */
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                           "Checking INSERT selectStmt: ptr=%p, nodeTag=%d, FE_T_SelectStmt=%d",
                                           insert_stmt->selectStmt,
                                           insert_stmt->selectStmt ? nodeTag(insert_stmt->selectStmt) : -1,
                                           FE_T_SelectStmt);

                        if (insert_stmt->selectStmt != NULL && nodeTag(insert_stmt->selectStmt) == FE_T_SelectStmt) {
                            SelectStmt *select_stmt = (SelectStmt *)insert_stmt->selectStmt;
                            /* 对于VALUES子句，valuesLists包含值列表 */
                            values_lists = select_stmt->valuesLists;
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                               "INSERT values extraction: select_stmt=%p, valuesLists=%p, length=%d",
                                               select_stmt, values_lists, values_lists ? list_length(values_lists) : 0);
                        } else {
                            /* 如果没有selectStmt或不是SelectStmt类型，使用空列表 */
                            values_lists = NIL;
                            tecc_processor_debug(TECC_LOG_WARNING,
                                               "INSERT statement has no valid selectStmt for values extraction: selectStmt=%p, nodeTag=%d",
                                               insert_stmt->selectStmt,
                                               insert_stmt->selectStmt ? nodeTag(insert_stmt->selectStmt) : -1);
                        }

                        result = tecc_values_processor_process_insert_values(
                            values_lists,
                            insert_stmt->cols,
                            insert_stmt->relation ? insert_stmt->relation->schemaname : NULL,
                            insert_stmt->relation ? insert_stmt->relation->relname : NULL,
                            statement_data);

                        if (result == TECC_VALUES_PROCESSOR_SUCCESS)
                            processed_count++;
                    }
                    break;

                case FE_T_UpdateStmt:
                    {
                        struct UpdateStmt *up_stmt = (struct UpdateStmt *) stmt;
                        result = tecc_values_processor_process_update_values(
                            up_stmt->targetList,
                            NULL, /* TODO: Get target columns */
                            up_stmt->relation ? up_stmt->relation->schemaname : NULL,
                            up_stmt->relation ? up_stmt->relation->relname : NULL,
                            statement_data);
                    }

                    if (result == TECC_VALUES_PROCESSOR_SUCCESS)
                        processed_count++;
                    break;

                case FE_T_PrepareStmt:  /* 220 */
                {
                    struct PrepareStmt *prepare_stmt = (struct PrepareStmt *) stmt;

                    if (prepare_stmt != NULL && prepare_stmt->query != NULL)
                    {
                        NodeTag inner_tag = nodeTag(prepare_stmt->query);

                        if (inner_tag == FE_T_InsertStmt)
                        {
                            struct InsertStmt *inner_insert = (struct InsertStmt *) prepare_stmt->query;

                            if (inner_insert->selectStmt != NULL &&
                                nodeTag(inner_insert->selectStmt) == FE_T_SelectStmt)
                            {
                                SelectStmt *select_stmt = (SelectStmt *) inner_insert->selectStmt;
                                List *inner_values_lists = select_stmt->valuesLists;

                                result = tecc_values_processor_process_insert_values(
                                    inner_values_lists,
                                    inner_insert->cols,
                                    inner_insert->relation ? inner_insert->relation->schemaname : NULL,
                                    inner_insert->relation ? inner_insert->relation->relname : NULL,
                                    statement_data);
                                if (result == TECC_VALUES_PROCESSOR_SUCCESS)
                                    processed_count++;
                            }
                        }
                        else if (inner_tag == FE_T_UpdateStmt)
                        {
                            struct UpdateStmt *inner_update = (struct UpdateStmt *) prepare_stmt->query;

                            result = tecc_values_processor_process_update_values(
                                inner_update->targetList,
                                NULL,
                                inner_update->relation ? inner_update->relation->schemaname : NULL,
                                inner_update->relation ? inner_update->relation->relname : NULL,
                                statement_data);
                            if (result == TECC_VALUES_PROCESSOR_SUCCESS)
                                processed_count++;
                        }
                    }
                }
                break;

                default:
                    /* No values to process for other statement types */
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                       "Skipping statement type %d (no values to process)",
                                       stmt_type);
                    break;
            }

            /* 如果某个语句处理失败，记录错误但继续处理其他语句 */
            if (result != TECC_VALUES_PROCESSOR_SUCCESS)
            {
                tecc_processor_debug(TECC_LOG_WARNING,
                                   "Statement %d processing failed with result: %d",
                                   stmt_count, result);
                /* 继续处理下一个语句，但记录错误状态 */
                tecc_values_processor_set_error(statement_data, TECC_PROCESSOR_SEMANTIC_ERROR,
                                               "Failed to process statement %d", stmt_count);
            }
        }

        
    }

    /* 4. 清理上下文 */
    tecc_values_processor_context_destroy(context);

    

    return result;
    /* End - ReqID:TECC4.0 - 值处理器主处理函数 */
}

/*
 * Main value deprocessing entry point
 */
TeccValuesProcessorResult
tecc_values_processor_deprocess(TeccStatementData *statement_data)
{
    /* Begin - ReqID:TECC4.0 - 查询结果反处理逻辑 */
    /* Variable declarations at function beginning for ISO C90 compliance */
    int encrypted_count = 0;
    ListCell *cell;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Starting values deprocessing");

    if (statement_data == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                            "Invalid statement data for deprocessing");
        return TECC_VALUES_PROCESSOR_ERROR;
    }

    /* Check if there are encrypted columns to deprocess */
    if (statement_data->encrypted_columns == NULL ||
        statement_data->encrypted_columns == NIL)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "No encrypted columns to deprocess");
        return TECC_VALUES_PROCESSOR_SUCCESS;
    }

    /* For now, implement a basic deprocessing logic that logs the operation */
    

    /* In a full implementation, this would:
     * 1. Iterate through query result columns
     * 2. Check if each column is encrypted (using metadata cache)
     * 3. For encrypted columns, call tecc_values_processor_deprocess_value()
     * 4. Replace encrypted values with decrypted values in result set
     */

    /* Placeholder: Log the number of encrypted columns */

    foreach(cell, statement_data->encrypted_columns)
    {
        TeccProcessorColumnRef *col_info = (TeccProcessorColumnRef *) cell->data.ptr_value;
        if (col_info != NULL)
        {
            /* 类型安全检查：确保指针指向有效的TeccProcessorColumnRef结构 */
            if (col_info->table_name[0] != '\0' || col_info->column_name[0] != '\0')
            {
                encrypted_count++;
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                    "Encrypted column %d: %s.%s",
                                    encrypted_count,
                                    col_info->table_name,
                                    col_info->column_name);
            }
            else
            {
                tecc_processor_debug(TECC_LOG_WARNING,
                                    "Invalid TeccProcessorColumnRef structure in encrypted_columns list");
            }
        }
    }

    

    /* End - ReqID:TECC4.0 - 查询结果反处理逻辑 */
    return TECC_VALUES_PROCESSOR_SUCCESS;
}

/*
 * Process values for INSERT statement
 */
TeccValuesProcessorResult
tecc_values_processor_process_insert_values(List *values_lists,
                                           List *target_columns,
                                           const char *schema_name,
                                           const char *table_name,
                                           TeccStatementData *statement_data)
{
    /* Begin - ReqID:TECC4.0 - INSERT语句值处理函数 */
    const char *resolved_schema = schema_name;
    TeccRawValuesList *raw_list = NULL;
    TeccValuesProcessorResult result = TECC_VALUES_PROCESSOR_SUCCESS;
    int row_index = 0;
    int values_count;
    int i;
    List *values;
    int values_in_row;
    int col_index;
    int j;
    Node *value_node;
    TeccRawValue *raw_value;
    struct ResTarget *target;
    const char *column_name;
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    int estimated_capacity = 0;
    const char *scan_start = NULL;
    const char *query = NULL;
    bool from_const = false;    /* ReqID:TECC4.0 - track if current value is from Const node */
    bool is_unary_minus = false; /* ReqID:TECC4.0 - track if value is unary minus A_Expr */

    /* 添加调试信息 */
    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "tecc_values_processor_process_insert_values called: values_lists=%p (length=%d), target_columns=%p (length=%d)",
                       values_lists, values_lists ? list_length(values_lists) : 0,
                       target_columns, target_columns ? list_length(target_columns) : 0);

    /* 1. 参数检查 */
    if (values_lists == NIL)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "No values lists to process");
        return TECC_VALUES_PROCESSOR_SUCCESS;
    }

    /* 2. 创建原始值列表 */
    /* 计算需要的容量：行数 × 每行的列数 */
    /* ReqID:TECC4.0 - estimated_capacity moved to function scope beginning */
    if (values_lists != NIL)
    {
        List *first_row = (List *) linitial(values_lists);
        estimated_capacity = list_length(values_lists) * list_length(first_row);
    }
    else
    {
        estimated_capacity = list_length(values_lists);
    }

    raw_list = tecc_raw_values_list_create(estimated_capacity > 0 ? estimated_capacity : 16);
    if (raw_list == NULL)
    {
        tecc_values_processor_set_error(statement_data, TECC_PROCESSOR_MEMORY_ERROR,
                                       "Failed to create raw values list");
        return TECC_VALUES_PROCESSOR_MEMORY_ERROR;
    }

    /* 2.5 计算VALUES子句在原始SQL中的偏移，用于后续值定位 */
    query = (statement_data && statement_data->query) ? statement_data->query : NULL;
    scan_start = NULL;
    if (query != NULL)
    {
        const char *p = query;
        /* 跳过前导空白 */
        while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
        /* 查找VALUES关键词（大小写不敏感） */
        {
            const char *cp = p;
            while (*cp)
            {
                if (pg_strncasecmp(cp, "VALUES", 6) == 0)
                {
                    p = cp;
                    break;
                }
                cp++;
            }
            if (*cp == '\0')
                p = NULL;
        }
        if (p != NULL)
        {
            p += 6; /* 跳过 "VALUES" */
            /* 跳过空白和左括号 */
            while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
            if (*p == '(') p++;
            while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
            scan_start = p;
        }
        else
        {
            tecc_processor_debug(TECC_LOG_WARNING,
                               "Could not find VALUES keyword in query for scan positioning");
        }
    }

    /* 3. 遍历值列表 */
    values_count = list_length(values_lists);
    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Processing %d value lists in INSERT statement", values_count);

    for (i = 0; i < values_count; i++)
    {
        values = (List *) list_nth(values_lists, i);
        values_in_row = list_length(values);
        col_index = 0;

        /*
         * Advance scan_start past the row separator "), (" when entering
         * the second or later row in a multi-row INSERT.
         * Re-validate the query pointer: the original query string in
         * statement_data->query may have been freed or reallocated during
         * the previous row's processing (e.g. by temp connections in
         * column metadata lookup).  If so, we cannot safely follow
         * scan_start and must fall back to a NULL scan_start, meaning
         * "force all encryption to hex-injection path."
         */
        if (i > 0 && scan_start != NULL)
        {
            const char *query_check = (statement_data && statement_data->query)
                ? statement_data->query : NULL;
            /* Validate scan_start is still within the query buffer */
            if (query_check == NULL ||
                scan_start < query_check ||
                scan_start >= query_check + strlen(query_check))
            {
                tecc_processor_debug(TECC_LOG_WARNING,
                    "scan_start out of bounds after row %d, falling back to hex-injection",
                    i - 1);
                scan_start = NULL;
            }
            else
            {
                while (*scan_start && *scan_start != '(')
                    scan_start++;
                if (*scan_start == '(')
                    scan_start++;
                while (*scan_start && (*scan_start == ' ' || *scan_start == '\t' ||
                       *scan_start == '\n' || *scan_start == '\r'))
                    scan_start++;
            }
        }

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Value list %d: list=%p, length=%d", i, values, values_in_row);

        if (values == NIL)
        {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Skipping empty value list at index %d", i);
            continue;
        }

        /* 遍历当前行的所有值 */
        for (j = 0; j < values_in_row; j++)
        {
            value_node = (Node *) list_nth(values, j);
            raw_value = NULL;
            target = NULL;
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Processing value at row %d, col %d: node=%p, type=%d",
                               row_index, j, value_node, value_node ? value_node->type : -1);

            /* 3.1 创建原始值对象 */
            /* 检查是否是ResTarget节点，如果是则提取val字段 */
            {
                Node *actual_value_node = value_node;
            if (value_node->type == FE_T_ResTarget) {
                ResTarget *res_target = (ResTarget *)value_node;
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Found ResTarget node, extracting val field: res_target=%p, val=%p",
                                   res_target, res_target->val);
                if (res_target->val != NULL) {
                    actual_value_node = (Node *)res_target->val;
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                       "Extracted actual value node: type=%d",
                                       actual_value_node ? actual_value_node->type : -1);
                }
            }

            raw_value = tecc_create_raw_value_from_node(actual_value_node);

            from_const = false;  /* Reset for each value */

            /* 获取值的准确位置和长度：使用Const->location/data_size（由scanner精确提供）
             * 不依赖 scan_start：statement_data->query 可能在处理行间被重分配，
             * 导致 scan_start 成为悬空指针被置为 NULL。
             * Const 节点自带 location/data_size，只需 nodeTag 判断即可。 */
            if (raw_value != NULL)
            {
                if (actual_value_node != NULL && nodeTag(actual_value_node) == FE_T_Const)
                {
                    Const *cnode = (Const *)actual_value_node;
                    if (cnode->location >= 0)
                    {
                        raw_value->location = (size_t)cnode->location;
                        if (cnode->data_size > 0)
                        {
                            raw_value->data_size = (size_t)cnode->data_size;
                        }
                        else if (query != NULL)
                        {
                            const char *p = query + raw_value->location;
                            int paren_depth = 0;
                            size_t computed_size = 0;
                            while (*p && computed_size < 1024 * 1024)
                            {
                                if (*p == '(') paren_depth++;
                                else if (*p == ')')
                                {
                                    if (paren_depth == 0) break;
                                    paren_depth--;
                                }
                                else if (*p == ',' && paren_depth == 0) break;
                                else if (paren_depth == 0 && (*p == ';' || *p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) break;
                                p++; computed_size++;
                            }
                            if (computed_size > 0)
                                raw_value->data_size = computed_size;
                        }
                        from_const = true;

                        /* Begin - ReqID:TECC4.0 - Bug 7997/7973/7835: preserve exact FLOAT text
                         * from original SQL to avoid precision loss from strtod→double→Ryu.
                         * Grammar stores float constants as FLOAT8OID Const with strtod precision
                         * loss; extract exact text from SQL via location/data_size instead. */
                        if (cnode->consttype == FLOAT8OID &&
                            raw_value != NULL &&
                            raw_value->type == TECC_RAW_VALUE_TYPE_FLOAT &&
                            query != NULL &&
                            (size_t)(cnode->location + cnode->data_size) <= strlen(query))
                        {
                            char *exact_text = (char *)pg_malloc(cnode->data_size + 1);
                            memcpy(exact_text, query + cnode->location, cnode->data_size);
                            exact_text[cnode->data_size] = '\0';
                            raw_value->original.string_value = exact_text;
                            raw_value->type = TECC_RAW_VALUE_TYPE_STRING;
                            raw_value->length = (size_t)cnode->data_size;
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "Preserved exact FLOAT text from SQL: \"%s\"", exact_text);
                        }
                        /* End - ReqID:TECC4.0 - Bug 7997/7973/7835 */
                    }
                }
                /* ReqID:TECC4.0 - Bug 8003: TypeCast(Const) like 1::int2 should be treated as const.
                 * Set location to the start of the value but let the fallback scan
                 * find the full extent including ::typename. */
                if (!from_const && actual_value_node != NULL &&
                    nodeTag(actual_value_node) == FE_T_TypeCast)
                {
                    TypeCast *tc = (TypeCast *)actual_value_node;
                    if (tc->arg != NULL && nodeTag(tc->arg) == FE_T_Const)
                    {
                        Const *cnode = (Const *)tc->arg;
                        if (cnode->location >= 0)
                        {
                            raw_value->location = (size_t)cnode->location;
                        }
                    }
                }
                /* ReqID:TECC4.0 - 一元负号常量（如 -1.0、-32768）被PG解析为 A_Expr，
                 * tecc_create_raw_value_from_node 已正确处理取负逻辑。
                 * 此处仅做检测，让后续SQL扫描设置location/data_size，
                 * 之后再将 from_const 置回 true，跳过"函数表达式"拒绝路径。 */
                if (!from_const && actual_value_node != NULL &&
                    nodeTag(actual_value_node) == FE_T_A_Expr)
                {
                    A_Expr *aexpr = (A_Expr *)actual_value_node;
                    if (aexpr->kind == AEXPR_OP && aexpr->lexpr == NULL && aexpr->name != NIL)
                    {
                        Node *op_node = (Node *)linitial(aexpr->name);
                        char *op_name = strVal((Value *)op_node);
                        if (op_name != NULL && strcmp(op_name, "-") == 0 && aexpr->rexpr != NULL)
                            is_unary_minus = true;
                    }
                }
                /* ReqID:TECC4.0 - Handle A_Expr(TypeCast(Const)) like -20000::int2 */
                if (!from_const && actual_value_node != NULL &&
                    nodeTag(actual_value_node) == FE_T_A_Expr)
                {
                    A_Expr *aexpr = (A_Expr *)actual_value_node;
                    if (aexpr->kind == AEXPR_OP && aexpr->lexpr == NULL && aexpr->rexpr != NULL &&
                        nodeTag(aexpr->rexpr) == FE_T_TypeCast)
                    {
                        TypeCast *tc = (TypeCast *)aexpr->rexpr;
                        if (tc->arg != NULL && nodeTag(tc->arg) == FE_T_Const)
                        {
                            Const *cv = (Const *)tc->arg;
                            if (cv->location >= 0 && cv->data_size > 0)
                            {
                                raw_value->location = (size_t)cv->location;
                                raw_value->data_size = (size_t)cv->data_size;
                                from_const = true;
                            }
                        }
                    }
                }

                if (!from_const && scan_start != NULL)
                {
                    /* 通过扫描SQL字符串计算（备用路径，非Const节点） */
                    while (*scan_start && (*scan_start == ' ' || *scan_start == '\t' ||
                           *scan_start == '\n' || *scan_start == '\r' || *scan_start == ','))
                        scan_start++;
                    {
                        size_t val_start = (size_t)(scan_start - query);
                        size_t val_end = val_start;
                        /* 非Const节点：带括号深度和字符串状态追踪的扫描，
                         * 正确处理函数调用（如 REPEAT('a', 5)）和表达式中的逗号。
                         * 仅当深度为0时遇到逗号或右括号才停止。 */
                        {
                            int paren_depth = 0;
                            bool in_string = false;
                            while (*scan_start)
                            {
                                char c = *scan_start;
                                /* 处理字符串字面量中的转义单引号 */
                                if (in_string)
                                {
                                    if (c == '\'' && scan_start[1] == '\'')
                                    {
                                        scan_start += 2; /* 跳过转义的单引号 '' */
                                        continue;
                                    }
                                    if (c == '\'')
                                    {
                                        in_string = false;
                                        scan_start++;
                                        continue;
                                    }
                                    scan_start++;
                                    continue;
                                }
                                if (c == '\'')
                                {
                                    in_string = true;
                                    scan_start++;
                                    continue;
                                }
                                if (c == '(')
                                {
                                    paren_depth++;
                                    scan_start++;
                                    continue;
                                }
                                if (c == ')')
                                {
                                    if (paren_depth == 0)
                                        break; /* 顶层右括号：结束当前行 */
                                    paren_depth--;
                                    scan_start++;
                                    continue;
                                }
                                /* 顶层逗号表示下一个值 */
                                if (paren_depth == 0 && c == ',')
                                    break;
                                scan_start++;
                            }
                        }
                        val_end = (size_t)(scan_start - query);
                        raw_value->location = val_start;
                        raw_value->data_size = val_end - val_start;
                    }
                }

                /* Advance scan_start past this value so the fallback
                 * path for subsequent non-Const values starts at the
                 * correct position in the SQL text. ReqID:TECC4.0 */
                {
                    const char *past_value = query + raw_value->location + raw_value->data_size;
                    if (past_value > scan_start)
                        scan_start = past_value;
                }
            }
			/* ReqID:TECC4.0 - unary minus A_Expr: SQL scan done, restore from_const */
			if (is_unary_minus)
			{
				from_const = true;
				is_unary_minus = false;

				/* Begin - ReqID:TECC4.0 - Bug 7997/7973/7835: preserve exact FLOAT text
				 * for unary minus (e.g. -999999999999999.999999999). SQL scan correctly
				 * included the minus sign in the extent. */
				if (raw_value != NULL &&
					raw_value->type == TECC_RAW_VALUE_TYPE_FLOAT &&
					query != NULL &&
					raw_value->location > 0 &&
					raw_value->data_size > 0 &&
					raw_value->location + raw_value->data_size <= strlen(query))
				{
					char *exact_text = (char *)pg_malloc(raw_value->data_size + 1);
					memcpy(exact_text, query + raw_value->location, raw_value->data_size);
					exact_text[raw_value->data_size] = '\0';
					raw_value->original.string_value = exact_text;
					raw_value->type = TECC_RAW_VALUE_TYPE_STRING;
					tecc_processor_debug(TECC_LOG_DEBUG1,
						"Preserved exact FLOAT text (unary minus): \"%s\"", exact_text);
				}
				/* End - ReqID:TECC4.0 - Bug 7997/7973/7835 */
			}

			/* ReqID:TECC4.0 - Bug 8003: TypeCast(Const) SQL scan done, restore from_const */
			if (!from_const && actual_value_node != NULL &&
				nodeTag(actual_value_node) == FE_T_TypeCast)
			{
				TypeCast *tc = (TypeCast *)actual_value_node;
				if (tc->arg != NULL && nodeTag(tc->arg) == FE_T_Const)
					from_const = true;
				/* ReqID:TECC4.0 - TypeCast(A_Expr(Const)) like -20000::int2 */
				if (!from_const && tc->arg != NULL &&
					nodeTag(tc->arg) == FE_T_A_Expr)
				{
					A_Expr *ae = (A_Expr *)tc->arg;
					if (ae->kind == AEXPR_OP && ae->lexpr == NULL &&
						ae->rexpr != NULL && nodeTag(ae->rexpr) == FE_T_Const)
						from_const = true;
				}
			}

            if (raw_value == NULL)
            {
                tecc_processor_debug(TECC_LOG_WARNING,
                                   "Failed to create raw value at row %d, col %d, node type=%d (original type=%d)",
                                   row_index, col_index, actual_value_node ? actual_value_node->type : -1,
                                   value_node ? value_node->type : -1);
                col_index++;
                continue;
            }
            else
            {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Successfully created raw value at row %d, col %d, type=%d, adding to raw_list",
                                   row_index, col_index, raw_value->type);

                /* 添加到原始值列表 */
                /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
                {
                    int add_result = tecc_raw_values_list_add(raw_list, raw_value);
                    if (add_result < 0)
                    {
                        tecc_processor_debug(TECC_LOG_WARNING,
                                           "Failed to add raw value to list at row %d, col %d, result=%d",
                                           row_index, col_index, add_result);
                        tecc_raw_value_destroy(raw_value);
                    }
                    else
                    {
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                           "Successfully added raw value to list at index %d, list now has %d values",
                                           add_result, tecc_raw_values_list_count(raw_list));
                    }
                }  /* ReqID:TECC4.0 - Close brace for add_result scope */
            }
            }

            /* 3.2 设置列信息 */
            column_name = NULL;

            /* 使用调用者传入的表名设置列信息 */
            if (table_name != NULL)
            {
                tecc_raw_value_set_table_info(raw_value, resolved_schema, table_name);
            }
            else
            {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "No table name available for VALUES processing");
            }

            if (target_columns != NIL)
            {
                /* 有字段名的情况：获取目标列 */
                if (col_index < list_length(target_columns))
                {
                    target = (struct ResTarget *) list_nth(target_columns, col_index);
                    if (target != NULL && target->name != NULL)
                    {
                        column_name = target->name;
                        /* 设置列名（暂时不设置类型，后面获取后更新） */
                        tecc_raw_value_set_column_info(raw_value, column_name, NULL, InvalidOid);

                        /* 检查是否需要加密 */
                        if (tecc_column_needs_encryption(statement_data, NULL, table_name, column_name))
                        {
                            raw_value->needs_encryption = true;

                            /* ReqID:TECC4.0 - 检查是否为常量值。非Const节点（如函数调用）无法在前端预求值，
                             * 直接报错提示用户。前端无法执行SQL函数（如 REPEAT、CONCAT 等），
                             * 需用户预求值后以字面量传入。
                             * DEFAULT关键字（SetToDefault节点）例外，原样保留即可。 */
                            if (!from_const)
                            {
                                /* ReqID:TECC4.0 - Bug 7972: DEFAULT keyword should not trigger "function expression" error */
                                if (raw_value->is_default)
                                {
                                    raw_value->needs_encryption = false;
                                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                                       "DEFAULT keyword for encrypted column %s.%s, skipping encryption",
                                                       table_name ? table_name : "NULL",
                                                       column_name);
                                }
                                else
                                {
                                    tecc_values_processor_set_error(statement_data,
                                        TECC_PROCESSOR_INVALID_FUNCTION_CALL,
                                        "function expressions are not supported for encrypted column \"%s\"; "
                                        "pre-compute the value and insert it as a literal",
                                        column_name);
                                    result = TECC_VALUES_PROCESSOR_ENCRYPTION_ERROR;
                                    goto done;
                                }
                            }
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                               "Column %s.%s needs encryption",
                                               table_name ? table_name : "NULL",
                                               column_name);

                            /* 查询加密列缓存获取original_type_oid */
                            {
                                TeccState *s = NULL;
                                const char *db = NULL;
                                const char *schema = NULL;
                                if (statement_data->conn != NULL)
                                {
                                    s = tecc_get_state(statement_data->conn);
                                    db = PQdb(statement_data->conn);
                                    schema = (s != NULL) ? tecc_cache_get_guc_param(s, "current_schema") : NULL;
                                }
                                if (s != NULL && db != NULL && schema != NULL)
                                {
                                    TeccColumnCacheItem *col_cache = tecc_cache_get_encrypted_column_by_name(
                                        s, db, schema, table_name, column_name);
                                    if (col_cache != NULL)
                                    {
                                        const char *enc_type = get_enc_type_name_from_base_oid(col_cache->original_type_oid);
                                        if (enc_type != NULL)
                                        {
                                            /* INSERT values don't need ::type_name cast — the backend
                                         * column is already the correct encrypted type and accepts
                                         * \x hex input directly. Set column_type to NULL. ReqID:TECC4.0 */
                                        tecc_raw_value_set_column_info(raw_value, column_name, NULL, col_cache->original_type_oid);
                                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                                               "Set column_type=NULL, type_oid=%u for column %s",
                                                               col_cache->original_type_oid, column_name);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                /* 没有字段名的情况：通过表名获取所有加密列，根据位置映射 */
                if (table_name != NULL)
                {
                    /* 获取表的所有加密列 */
                    TeccColumnCacheItem **encrypted_columns = NULL;
                    int column_count = 0;

                    /* 获取数据库名和模式名 */
                    const char *database_name = NULL;
                    const char *schema_name = NULL;

                    /* 尝试从连接获取数据库名 */
                    if (statement_data != NULL && statement_data->conn != NULL) {
                        database_name = PQdb(statement_data->conn);
                        if (database_name == NULL || database_name[0] == '\0') {
                            /* 无法获取数据库名，记录错误并跳过 */
                            tecc_processor_debug(TECC_LOG_WARNING,
                                               "tecc_values_processor_process_insert_values: Could not get database name from connection");
                            continue;
                        }
                    } else {
                        /* 没有连接，无法获取数据库名，跳过 */
                        tecc_processor_debug(TECC_LOG_WARNING,
                                           "tecc_values_processor_process_insert_values: No connection available");
                        continue;
                    }

                    /* 获取state参数 */
                    {
                        TeccState *state = NULL;
                    if (statement_data != NULL && statement_data->conn != NULL) {
                        state = tecc_get_state(statement_data->conn);
                    }

                    if (state != NULL) {
                        /* 从缓存获取当前schema */
                        schema_name = tecc_cache_get_guc_param(state, "current_schema");
                        if (schema_name == NULL || schema_name[0] == '\0') {
                            /* 无法获取schema名，记录错误并跳过 */
                            tecc_processor_debug(TECC_LOG_WARNING,
                                               "tecc_values_processor_process_insert_values: Could not get current schema from cache");
                            continue;
                        }
                    } else {
                        /* 没有state，无法获取schema名，跳过 */
                        tecc_processor_debug(TECC_LOG_WARNING,
                                           "tecc_values_processor_process_insert_values: No state available");
                        continue;
                    }

                    encrypted_columns = tecc_cache_get_columns_by_table_name(state,
                                                                            database_name,
                                                                            schema_name,
                                                                            table_name,
                                                                            &column_count);
                    }

                    if (encrypted_columns != NULL && column_count > 0)
                    {
                        /* 根据列位置查找对应的加密列 */
                        for (int k = 0; k < column_count; k++)
                        {
                            if (encrypted_columns[k] != NULL &&
                                encrypted_columns[k]->column_attnum == col_index + 1)  /* attnum从1开始 */
                            {
                                column_name = encrypted_columns[k]->column_name;
                                {
                                    Oid base_oid = encrypted_columns[k]->original_type_oid;
                                    const char *enc_type = get_enc_type_name_from_base_oid(base_oid);
                                    tecc_raw_value_set_column_info(raw_value, column_name,
                                                                   enc_type, base_oid);
                                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                                       "Column %s.%s (position %d) needs encryption, set column_type='%s', type_oid=%u",
                                                       table_name, column_name, col_index,
                                                       enc_type ? enc_type : "(null)", base_oid);
                                }
                                raw_value->needs_encryption = true;

                                /* ReqID:TECC4.0 - 非Const值不适用于加密列 */
                                if (!from_const)
                                {
                                    /* ReqID:TECC4.0 - Bug 7972: DEFAULT keyword should not trigger "function expression" error */
                                    if (raw_value->is_default)
                                    {
                                        raw_value->needs_encryption = false;
                                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                                           "DEFAULT keyword for encrypted column %s.%s, skipping encryption",
                                                           table_name ? table_name : "NULL",
                                                           column_name);
                                    }
                                    else
                                    {
                                        tecc_values_processor_set_error(statement_data,
                                            TECC_PROCESSOR_INVALID_FUNCTION_CALL,
                                            "function expressions are not supported for encrypted column \"%s\"; "
                                            "pre-compute the value and insert it as a literal",
                                            column_name);
                                        result = TECC_VALUES_PROCESSOR_ENCRYPTION_ERROR;
                                        goto done;
                                    }
                                }
                                break;
                            }
                        }

                        /* 清理缓存项数组 */
                        pg_free(encrypted_columns);
                    }
                    else
                    {
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                           "No encrypted columns found for table %s or failed to query",
                                           table_name);
                    }
                }
                else
                {
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                       "Cannot determine column name: no table name and no target columns");
                }
            }

            col_index++;
        }

        row_index++;
    }

done:
    /* 4. 处理列表中的所有值（如果已有错误则跳过处理，保留错误码） */
    if (result == TECC_VALUES_PROCESSOR_SUCCESS)
        result = tecc_process_values_list(raw_list, statement_data);

    /* 5. 将raw_list保存到statement_data用于后续SQL重写 */
    if (result == TECC_VALUES_PROCESSOR_SUCCESS && raw_list != NULL)
    {
        if (tecc_raw_values_list_count(raw_list) > 0)
            statement_data->raw_values_list = raw_list;
        else
            tecc_raw_values_list_destroy(raw_list);
    }
    else
    {
        tecc_raw_values_list_destroy(raw_list);
    }

    /* 6. 返回结果 */
    return result;
    /* End - ReqID:TECC4.0 - INSERT语句值处理函数 */
}

/*
 * Process values for UPDATE statement
 */
TeccValuesProcessorResult
tecc_values_processor_process_update_values(List *target_list,
                                           List *target_columns,
                                           const char *schema_name,
                                           const char *table_name,
                                           TeccStatementData *statement_data)
{
    /* Begin - ReqID:TECC4.0 - UPDATE语句值处理函数 */
    TeccRawValuesList *raw_list = NULL;
    TeccValuesProcessorResult result = TECC_VALUES_PROCESSOR_SUCCESS;
    int col_index = 0;
    int target_count;
    int i;
    struct ResTarget *target;
    TeccRawValue *raw_value;
    const char *resolved_schema = schema_name;
    const char *resolved_table = table_name;

    /* 1. 参数检查 */
    if (target_list == NIL)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "No target list to process");
        return TECC_VALUES_PROCESSOR_SUCCESS;
    }

    /* 2. 创建原始值列表 */
    raw_list = tecc_raw_values_list_create(list_length(target_list));
    if (raw_list == NULL)
    {
        tecc_values_processor_set_error(statement_data, TECC_PROCESSOR_MEMORY_ERROR,
                                       "Failed to create raw values list");
        return TECC_VALUES_PROCESSOR_MEMORY_ERROR;
    }

    /* 3. 遍历目标列表 */
    target_count = list_length(target_list);
    for (i = 0; i < target_count; i++)
    {
        target = (struct ResTarget *) list_nth(target_list, i);
        raw_value = NULL;

        if (target == NULL || target->val == NULL)
        {
            tecc_processor_debug(TECC_LOG_WARNING,
                               "Skipping NULL target at index %d", col_index);
            col_index++;
            continue;
        }

        /* 3.1 跳过非常量值（ColumnRef/A_Expr等）。
         * 对于 SET enc_col = another_enc_col，客户端无法也不需加密——
         * 服务端直接进行密文拷贝（兼容加密数字类型间互相转换）。
         * ReqID:TECC4.0 */
        {
            int target_val_type = nodeTag(target->val);
            if (target_val_type != FE_T_Const
                && target_val_type != FE_T_Integer
                && target_val_type != FE_T_Float
                && target_val_type != FE_T_String
                && target_val_type != FE_T_A_Expr
                && target_val_type != FE_T_RowExpr
                && target_val_type != FE_T_TypeCast)   /* ReqID:TECC4.0 - 处理 ::int2 类型转换 */
            {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                    "Skipping non-Const value (type=%d) for column %s — "
                    "pass-through for server-side ciphertext copy",
                    target_val_type,
                    target->name ? target->name : "?");
                col_index++;
                continue;
            }
        }

        /* 3.2 创建原始值对象 */
        raw_value = tecc_create_raw_value_from_node(target->val);
        if (raw_value == NULL)
        {
            tecc_processor_debug(TECC_LOG_WARNING,
                               "Failed to create raw value for target at index %d",
                               col_index);
            col_index++;
            continue;
        }

        /* Copy location and data_size from Const node if available */
        if (target->val != NULL && nodeTag(target->val) == FE_T_Const)
        {
            Const *cv = (Const *)target->val;
            if (cv->location >= 0 && cv->data_size > 0)
            {
                raw_value->location = (size_t)cv->location;
                raw_value->data_size = (size_t)cv->data_size;
            }
        }
        /* ReqID:TECC4.0 - TypeCast like ::int2: unwrap to get inner Const location */
        if (target->val != NULL && nodeTag(target->val) == FE_T_TypeCast)
        {
            TypeCast *tc = (TypeCast *)target->val;
            if (tc->arg != NULL && nodeTag(tc->arg) == FE_T_Const)
            {
                Const *cv = (Const *)tc->arg;
                if (cv->location >= 0 && cv->data_size > 0)
                {
                    raw_value->location = (size_t)cv->location;
                    raw_value->data_size = (size_t)cv->data_size;
                }
            }
        }
        /* ReqID:TECC4.0 - A_Expr unary minus like -1.00: extract inner Const location.
         * Include the '-' sign by going back 1 byte from the Const location. */
        if (raw_value != NULL && target->val != NULL &&
            nodeTag(target->val) == FE_T_A_Expr)
        {
            A_Expr *ae = (A_Expr *)target->val;
            if (ae->kind == AEXPR_OP && ae->lexpr == NULL && ae->rexpr != NULL)
            {
                size_t inner_loc = 0;
                size_t inner_size = 0;
                Node *inner = ae->rexpr;
                if (nodeTag(inner) == FE_T_Const)
                {
                    Const *cv = (Const *)inner;
                    if (cv->location >= 0 && cv->data_size > 0)
                    {
                        inner_loc = (size_t)cv->location;
                        inner_size = (size_t)cv->data_size;
                    }
                }
                else if (nodeTag(inner) == FE_T_TypeCast)
                {
                    TypeCast *tc = (TypeCast *)inner;
                    if (tc->arg != NULL && nodeTag(tc->arg) == FE_T_Const)
                    {
                        Const *cv = (Const *)tc->arg;
                        if (cv->location >= 0 && cv->data_size > 0)
                        {
                            inner_loc = (size_t)cv->location;
                            inner_size = (size_t)cv->data_size;
                        }
                    }
                }
                /* Include the '-' sign before the value */
                if (inner_loc > 0)
                {
                    raw_value->location = inner_loc - 1;
                    raw_value->data_size = inner_size + 1;
                }
            }
        }
        /* 3.2 设置列信息 */
        if (target->name != NULL)
        {
            /* 设置列名 */
            tecc_raw_value_set_column_info(raw_value, target->name, NULL, InvalidOid);

            /* 使用调用者传入的表名和schema设置列信息 */
            if (resolved_table != NULL)
            {
                tecc_raw_value_set_table_info(raw_value, resolved_schema, resolved_table);
            }
            else
            {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "No table name available for UPDATE processing");
            }

            /* 检查是否需要加密 */
            if (tecc_column_needs_encryption(statement_data, NULL, resolved_table, target->name))
            {
                raw_value->needs_encryption = true;
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Column %s.%s needs encryption",
                                   resolved_table ? resolved_table : "NULL",
                                   target->name);

                /* 查询加密列缓存获取original_type_oid并设置enc_type */
                {
                    TeccState *state = NULL;
                    const char *database_name = NULL;
                    const char *lookup_schema = resolved_schema;
                    if (statement_data->conn != NULL) {
                        state = tecc_get_state(statement_data->conn);
                        database_name = PQdb(statement_data->conn);
                    }
                    if (state != NULL && database_name != NULL && lookup_schema != NULL)
                    {
                        TeccColumnCacheItem *col_cache = tecc_cache_get_encrypted_column_by_name(
                            state, database_name, lookup_schema, resolved_table, target->name);
                        if (col_cache != NULL)
                        {
                            const char *enc_type = get_enc_type_name_from_base_oid(col_cache->original_type_oid);
                            if (enc_type != NULL)
                            {
                                /* ReqID:TECC4.0 - Set column_type for UPDATE SET values so the
                                 * SQL rewrite can produce 'hex'::type_name cast. Without the cast,
                                 * the server sees a text literal assigned to an encrypted type
                                 * column and fails. */
                                tecc_raw_value_set_column_info(raw_value, target->name, enc_type, col_cache->original_type_oid);
                                tecc_processor_debug(TECC_LOG_DEBUG1,
                                                   "UPDATE: Set column_type=%s, type_oid=%u for column %s",
                                                   enc_type, col_cache->original_type_oid, target->name);
                            }
                        }
                    }
                }
            }
        }

        /* 3.3 添加到列表 */
        if (tecc_raw_values_list_add(raw_list, raw_value) < 0)
        {
            tecc_raw_value_destroy(raw_value);
            tecc_raw_values_list_destroy(raw_list);
            tecc_values_processor_set_error(statement_data, TECC_PROCESSOR_MEMORY_ERROR,
                                           "Failed to add raw value to list");
            return TECC_VALUES_PROCESSOR_MEMORY_ERROR;
        }

        col_index++;
    }

    /* 4. 处理列表中的所有值 */
    result = tecc_process_values_list(raw_list, statement_data);

    /* 5. 将raw_list保存到statement_data用于后续SQL重写 */
    if (result == TECC_VALUES_PROCESSOR_SUCCESS && raw_list != NULL)
    {
        if (tecc_raw_values_list_count(raw_list) > 0)
            statement_data->raw_values_list = raw_list;
        else
            tecc_raw_values_list_destroy(raw_list);
    }
    else
    {
        tecc_raw_values_list_destroy(raw_list);
    }

    /* 6. 返回结果 */
    return result;
    /* End - ReqID:TECC4.0 - UPDATE语句值处理函数 */
}

/*
 * Process individual value
 */
TeccValuesProcessorResult
tecc_values_processor_process_value(TeccRawValue *raw_value,
                                   const char *column_name,
                                   const char *column_type,
                                   TeccStatementData *statement_data)
{
    /* Begin - ReqID:TECC4.0 - 单个值处理函数 */
    if (raw_value == NULL)
        return TECC_VALUES_PROCESSOR_ERROR;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing value for column %s",
                        column_name ? column_name : "unknown");

    /* Set column information if provided */
    if (column_name != NULL || column_type != NULL)
    {
        tecc_raw_value_set_column_info(raw_value, column_name, column_type, InvalidOid);
    }

    /* Check if encryption is needed */
    if (raw_value->needs_encryption)
    {
        TeccValuesProcessorResult encrypt_result;
        const char *algorithm = "RND";  /* Default algorithm for now */
        const char *key = "default_key";  /* Default key for now */

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Value needs encryption - calling scheduler");

        /* Call encryption scheduler */
        encrypt_result = tecc_values_processor_encrypt_value(
            raw_value, algorithm, key, statement_data);

        if (encrypt_result != TECC_VALUES_PROCESSOR_SUCCESS)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "Encryption failed for column %s: %d",
                               column_name ? column_name : "unknown",
                               encrypt_result);
            return encrypt_result;
        }

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Encryption completed successfully for column %s",
                           column_name ? column_name : "unknown");
    }

    return TECC_VALUES_PROCESSOR_SUCCESS;
    /* End - ReqID:TECC4.0 - 单个值处理函数 */
}

/* Begin - ReqID:TECC4.0 - 输入校验函数 */

/*
 * Map encrypted type name to PostgreSQL base type name for error messages.
 * Matches PostgreSQL backend error message conventions.
 */
static const char *
tecc_validation_type_name(const char *enc_type_name)
{
    if (enc_type_name == NULL)
        return NULL;
    if (strcmp(enc_type_name, "int2_enc") == 0)
        return "smallint";
    if (strcmp(enc_type_name, "int4_enc") == 0)
        return "integer";
    if (strcmp(enc_type_name, "int8_enc") == 0)
        return "bigint";
    if (strcmp(enc_type_name, "float4_enc") == 0)
        return "real";
    if (strcmp(enc_type_name, "float8_enc") == 0)
        return "double precision";
    if (strcmp(enc_type_name, "numeric_enc") == 0)
        return "numeric";
    return enc_type_name;
}

/*
 * Validate plaintext value against encrypted column type constraints.
 * INTEGER: check int2/int4 range; FLOAT: reject for integer types;
 * STRING: parse and validate. Matches PostgreSQL backend input behavior.
 */
static TeccValuesProcessorResult
tecc_validate_plaintext_for_type(const TeccRawValue *raw_value,
                                 const char *plaintext,
                                 TeccStatementData *statement_data)
{
    const char *col_type;
    const char *base_type_name;

    if (raw_value == NULL || plaintext == NULL)
        return TECC_VALUES_PROCESSOR_SUCCESS;

    col_type = raw_value->column_type;

    /* 若column_type为NULL（named-column INSERT），通过type_oid获取 */
    if (col_type == NULL && raw_value->type_oid != InvalidOid)
    {
        col_type = get_enc_type_name_from_base_oid(raw_value->type_oid);
    }

    if (col_type == NULL)
        return TECC_VALUES_PROCESSOR_SUCCESS;

    base_type_name = tecc_validation_type_name(col_type);
    if (base_type_name == NULL)
        return TECC_VALUES_PROCESSOR_SUCCESS;

    /* ReqID:TECC4.0 - INTEGER raw values */
    if (raw_value->type == TECC_RAW_VALUE_TYPE_INTEGER)
    {
        int64_t int_val = raw_value->original.integer_value;

        if (strcmp(col_type, "int2_enc") == 0)
        {
            if (int_val < PG_INT16_MIN || int_val > PG_INT16_MAX)
            {
                char val_buf[32];
                snprintf(val_buf, sizeof(val_buf), "%lld", (long long) int_val);
                tecc_values_processor_set_error(statement_data,
                    TECC_PROCESSOR_DATA_FORMAT_ERROR,
                    "value \"%s\" is out of range for type %s",
                    val_buf, base_type_name);
                return TECC_VALUES_PROCESSOR_ENCRYPTION_ERROR;
            }
        }
        else if (strcmp(col_type, "int4_enc") == 0)
        {
            if (int_val < PG_INT32_MIN || int_val > PG_INT32_MAX)
            {
                char val_buf[32];
                snprintf(val_buf, sizeof(val_buf), "%lld", (long long) int_val);
                tecc_values_processor_set_error(statement_data,
                    TECC_PROCESSOR_DATA_FORMAT_ERROR,
                    "value \"%s\" is out of range for type %s",
                    val_buf, base_type_name);
                return TECC_VALUES_PROCESSOR_ENCRYPTION_ERROR;
            }
        }
    }
    /* ReqID:TECC4.0 - FLOAT raw values: reject for integer types */
    else if (raw_value->type == TECC_RAW_VALUE_TYPE_FLOAT)
    {
        if (strcmp(col_type, "int2_enc") == 0 ||
            strcmp(col_type, "int4_enc") == 0 ||
            strcmp(col_type, "int8_enc") == 0)
        {
            tecc_values_processor_set_error(statement_data,
                TECC_PROCESSOR_DATA_FORMAT_ERROR,
                "invalid input syntax for type %s: \"%s\"",
                base_type_name, plaintext);
            return TECC_VALUES_PROCESSOR_ENCRYPTION_ERROR;
        }
    }
    /* ReqID:TECC4.0 - STRING raw values: parse and validate */
    else if (raw_value->type == TECC_RAW_VALUE_TYPE_STRING)
    {
        if (strcmp(col_type, "int2_enc") == 0 ||
            strcmp(col_type, "int4_enc") == 0 ||
            strcmp(col_type, "int8_enc") == 0)
        {
            char *endptr = NULL;
            int64_t parsed;

            errno = 0;
            parsed = strtoll(plaintext, &endptr, 10);

            while (*endptr != '\0' && isspace((unsigned char) *endptr))
                endptr++;

            if (errno == ERANGE || *endptr != '\0')
            {
                tecc_values_processor_set_error(statement_data,
                    TECC_PROCESSOR_DATA_FORMAT_ERROR,
                    "invalid input syntax for type %s: \"%s\"",
                    base_type_name, plaintext);
                return TECC_VALUES_PROCESSOR_ENCRYPTION_ERROR;
            }

            if (strcmp(col_type, "int2_enc") == 0)
            {
                if (parsed < PG_INT16_MIN || parsed > PG_INT16_MAX)
                {
                    tecc_values_processor_set_error(statement_data,
                        TECC_PROCESSOR_DATA_FORMAT_ERROR,
                        "value \"%s\" is out of range for type %s",
                        plaintext, base_type_name);
                    return TECC_VALUES_PROCESSOR_ENCRYPTION_ERROR;
                }
            }
            else if (strcmp(col_type, "int4_enc") == 0)
            {
                if (parsed < PG_INT32_MIN || parsed > PG_INT32_MAX)
                {
                    tecc_values_processor_set_error(statement_data,
                        TECC_PROCESSOR_DATA_FORMAT_ERROR,
                        "value \"%s\" is out of range for type %s",
                        plaintext, base_type_name);
                    return TECC_VALUES_PROCESSOR_ENCRYPTION_ERROR;
                }
            }
        }
        else if (strcmp(col_type, "float4_enc") == 0 ||
                 strcmp(col_type, "float8_enc") == 0 ||
                 strcmp(col_type, "numeric_enc") == 0)
        {
            char *endptr = NULL;
            double value;

            errno = 0;
            value = strtod(plaintext, &endptr);

            while (*endptr != '\0' && isspace((unsigned char) *endptr))
                endptr++;

            if (endptr == plaintext ||
                (errno == ERANGE && *endptr == '\0' && isinf(value)))
            {
                tecc_values_processor_set_error(statement_data,
                    TECC_PROCESSOR_DATA_FORMAT_ERROR,
                    "invalid input syntax for type %s: \"%s\"",
                    base_type_name, plaintext);
                return TECC_VALUES_PROCESSOR_ENCRYPTION_ERROR;
            }
        }
    }

    return TECC_VALUES_PROCESSOR_SUCCESS;
}
/* End - ReqID:TECC4.0 - 输入校验函数 */

/*
 * Encrypt single value
 */
TeccValuesProcessorResult
tecc_values_processor_encrypt_value(TeccRawValue *raw_value,
                                   const char *algorithm,
                                   const char *key,
                                   TeccStatementData *statement_data)
{
    /* Begin - ReqID:TECC4.0 - 单个值加密函数 */
    char *plaintext = NULL;
    TeccUnifiedEncryptResult *encrypt_result = NULL;
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    char *dek_name = NULL;
    TeccState *state = tecc_get_state(statement_data->conn);

    /* 1. 参数检查 */
    if (raw_value == NULL || algorithm == NULL || key == NULL)
    {
        tecc_values_processor_set_error(statement_data, TECC_PROCESSOR_INTERNAL_ERROR,
                                       "Invalid parameters for encryption");
        return TECC_VALUES_PROCESSOR_ERROR;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Encrypting value for column %s with algorithm %s",
                        raw_value->column_name ? raw_value->column_name : "unknown",
                        algorithm);

    /* 2. 将原始值转换为字符串 */
    plaintext = tecc_raw_value_to_string(raw_value);
    if (plaintext == NULL)
    {
        tecc_values_processor_set_error(statement_data, TECC_PROCESSOR_INTERNAL_ERROR,
                                       "Failed to convert raw value to string");
        return TECC_VALUES_PROCESSOR_ERROR;
    }

    /* Begin - ReqID:TECC4.0 - 输入校验 */
    {
        TeccValuesProcessorResult validation_result;
        validation_result = tecc_validate_plaintext_for_type(raw_value, plaintext, statement_data);
        if (validation_result != TECC_VALUES_PROCESSOR_SUCCESS)
        {
            if (plaintext != raw_value->original.string_value)
            {
                size_t wipe_len = strlen(plaintext);
                tecc_secure_wipe_memory(plaintext, wipe_len + 1);
                pg_free(plaintext);
            }
            return validation_result;
        }
    }
    /* End - ReqID:TECC4.0 - 输入校验 */

    /* 3. 获取DEK名称 */
    /* ReqID:TECC4.0 - dek_name declared at function scope beginning */
    if (raw_value->table_name != NULL && raw_value->column_name != NULL) {
        dek_name = tecc_get_column_dek_name(statement_data, NULL,
                                           raw_value->table_name,
                                           raw_value->column_name);
    }

    if (dek_name == NULL) {
        tecc_values_processor_set_error(statement_data, TECC_PROCESSOR_INTERNAL_ERROR,
                                       "Failed to get DEK name for column %s.%s",
                                       raw_value->table_name ? raw_value->table_name : "unknown_table",
                                       raw_value->column_name ? raw_value->column_name : "unknown_column");
        if (plaintext != raw_value->original.string_value) {
            pg_free(plaintext);
        }
        return TECC_VALUES_PROCESSOR_ENCRYPTION_ERROR;
    }

    /* 4. 调用加密调度器 */
    encrypt_result = tecc_unified_encrypt_string(
        statement_data->conn,  /* 添加连接参数 */
        plaintext,
        raw_value->table_name ? raw_value->table_name : "unknown_table",
        raw_value->column_name ? raw_value->column_name : "unknown_column",
        NULL,  /* 使用默认KEK */
        dek_name,  /* 使用实际的DEK名称 */
        state ? state->crypto_ctx : NULL
    );

    /* 释放DEK名称内存 */
    pg_free(dek_name);

    /* 5. 检查加密结果 */
    if (encrypt_result == NULL || encrypt_result->error_code != TECC_CRYPTO_SUCCESS)
    {
        /* Use scheduler-recorded error if available, otherwise fall back */
        if (encrypt_result == NULL)
        {
            const char *sched_error = tecc_get_last_error(statement_data->conn);
            tecc_values_processor_set_error(statement_data, TECC_PROCESSOR_ENCRYPTION_ERROR,
                                           "Encryption failed: %s",
                                           sched_error ? sched_error : "scheduler returned no result");
        }
        else
        {
            tecc_values_processor_set_error(statement_data, TECC_PROCESSOR_ENCRYPTION_ERROR,
                                           "Encryption failed: %s",
                                           encrypt_result->error_msg);
        }
        if (encrypt_result != NULL)
            tecc_unified_free_encrypt_result(encrypt_result);
        if (plaintext != raw_value->original.string_value)
        {
            /* Begin - ReqID:TECC4.0 - 安全内存擦除加强 */
            /* Securely wipe plaintext data before freeing */
            if (plaintext)
            {
                /* 使用实际字符串长度进行安全擦除 */
                size_t plaintext_len = strlen(plaintext);
                /* 只擦除实际字符串长度+1（空字符）的范围
             * 注意：不能假设分配大小是固定的，pg_strdup只分配strlen+1，
             * 提高到32会导致堆缓冲区溢出 */
                size_t wipe_size = plaintext_len + 1;
                /* 使用安全擦除函数 */
                tecc_secure_wipe_memory(plaintext, wipe_size);
            }
            pg_free(plaintext);
            /* End - ReqID:TECC4.0 - 安全内存擦除加强 */
        }
        return TECC_VALUES_PROCESSOR_ENCRYPTION_ERROR;
    }

    /* 6. 存储加密结果 */
    if (raw_value->processed.encrypted_value != NULL)
        pg_free(raw_value->processed.encrypted_value);

    raw_value->processed.encrypted_value = tecc_processor_strdup(encrypt_result->hex_string);
    raw_value->state = TECC_RAW_VALUE_STATE_ENCRYPTED;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Value encrypted successfully, result size: %zu",
                        strlen(encrypt_result->hex_string));

    /* 7. 清理资源 */
    tecc_unified_free_encrypt_result(encrypt_result);
    if (plaintext != raw_value->original.string_value)
    {
        /* Begin - ReqID:TECC4.0 - 安全内存擦除加强 */
        /* Securely wipe plaintext data before freeing */
        if (plaintext)
        {
            size_t plaintext_len = strlen(plaintext);
            /* 只擦除实际字符串长度+1（空字符）的范围，不能假设分配大小 */
            size_t wipe_size = plaintext_len + 1;
            tecc_secure_wipe_memory(plaintext, wipe_size);
        }
        pg_free(plaintext);
        /* End - ReqID:TECC4.0 - 安全内存擦除加强 */
    }

    /* 7. 返回成功 */
    return TECC_VALUES_PROCESSOR_SUCCESS;
    /* End - ReqID:TECC4.0 - 单个值加密函数 */
}

/*
 * Decrypt single value
 */
TeccValuesProcessorResult
tecc_values_processor_decrypt_value(TeccRawValue *raw_value,
                                   const char *algorithm,
                                   const char *key,
                                   TeccStatementData *statement_data)
{
    /* Begin - ReqID:TECC4.0 - 单个值解密函数 */
    TeccUnifiedDecryptResult *decrypt_result = NULL;
    const char *encrypted_value = NULL;

    /* 1. 参数检查 */
    if (raw_value == NULL)
    {
        tecc_values_processor_set_error(statement_data, TECC_PROCESSOR_INTERNAL_ERROR,
                                       "Invalid raw_value parameter for decryption");
        return TECC_VALUES_PROCESSOR_ERROR;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Decrypting value for column %s",
                        raw_value->column_name ? raw_value->column_name : "unknown");

    /* 2. 获取加密值 */
    if (raw_value->state == TECC_RAW_VALUE_STATE_ENCRYPTED)
    {
        encrypted_value = raw_value->processed.encrypted_value;
    }
    else if (raw_value->original.string_value != NULL)
    {
        /* 如果状态不是ENCRYPTED，但原始值是字符串，尝试解密 */
        encrypted_value = raw_value->original.string_value;
    }
    else
    {
        tecc_values_processor_set_error(statement_data, TECC_PROCESSOR_INTERNAL_ERROR,
                                       "No encrypted value available for decryption");
        return TECC_VALUES_PROCESSOR_ERROR;
    }

    if (encrypted_value == NULL || strlen(encrypted_value) == 0)
    {
        tecc_values_processor_set_error(statement_data, TECC_PROCESSOR_INTERNAL_ERROR,
                                       "Empty encrypted value");
        return TECC_VALUES_PROCESSOR_ERROR;
    }

    /* 3. 调用解密调度器 */
    /* Note: Passing NULL connection for now - needs to be fixed for thread safety */
    decrypt_result = tecc_unified_decrypt_hex(
        (statement_data != NULL) ? statement_data->conn : NULL,  /* Get connection from statement_data if available */
        encrypted_value,
        raw_value->table_name ? raw_value->table_name : "unknown_table",
        raw_value->column_name ? raw_value->column_name : "unknown_column",
        NULL);

    /* 4. 检查解密结果 */
    if (decrypt_result == NULL || decrypt_result->error_code != TECC_CRYPTO_SUCCESS)
    {
        tecc_values_processor_set_error(statement_data, TECC_PROCESSOR_INTERNAL_ERROR,
                                       "Decryption failed: %s",
                                       decrypt_result ? decrypt_result->error_msg : "Unknown error");
        if (decrypt_result != NULL)
            tecc_unified_free_decrypt_result(decrypt_result);
        return TECC_VALUES_PROCESSOR_DECRYPTION_ERROR;
    }

    /* 5. 存储解密结果 */
    if (raw_value->processed.decrypted_value != NULL)
        pg_free(raw_value->processed.decrypted_value);

    raw_value->processed.decrypted_value = tecc_processor_strdup(decrypt_result->plaintext);
    raw_value->state = TECC_RAW_VALUE_STATE_DECRYPTED;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Value decrypted successfully, result size: %zu",
                        decrypt_result->plain_len);

    /* 6. 清理资源 */
    tecc_unified_free_decrypt_result(decrypt_result);

    /* 7. 返回成功 */
    return TECC_VALUES_PROCESSOR_SUCCESS;
    /* End - ReqID:TECC4.0 - 单个值解密函数 */
}



/*
 * Context management functions
 */
TeccValuesProcessorContext *
tecc_values_processor_context_create(void)
{
    TeccValuesProcessorContext *context;

    context = (TeccValuesProcessorContext *)pg_malloc(sizeof(TeccValuesProcessorContext));
    if (context == NULL)
        return NULL;

    memset(context, 0, sizeof(TeccValuesProcessorContext));
    context->raw_values_list = tecc_raw_values_list_create(16);
    if (context->raw_values_list == NULL)
    {
        pg_free(context);
        return NULL;
    }

    context->encrypted_columns = NIL;
    context->encryption_algorithm = 0;
    context->encryption_key = NULL;
    context->needs_processing = false;

    return context;
}

void
tecc_values_processor_context_destroy(TeccValuesProcessorContext *context)
{
    if (context == NULL)
        return;

    if (context->raw_values_list != NULL)
        tecc_raw_values_list_destroy(context->raw_values_list);

    if (context->encryption_key != NULL)
        pg_free(context->encryption_key);

    pg_free(context);
}

/*
 * Error handling functions
 */
void
tecc_values_processor_set_error(TeccStatementData *statement_data,
                               int error_code,
                               const char *format, ...)
{
    va_list args;
    char buffer[1024];

    if (statement_data == NULL)
        return;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    /* ReqID:TECC4.0 - Do not overwrite if a specific error already exists */
    if (statement_data->error_message[0] != '\0')
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Not overwriting existing error message: %s",
                           statement_data->error_message);
        return;
    }

    /* Use the common error handling function */
    tecc_processor_set_error(statement_data, error_code, "%s", buffer);
}

const char *
tecc_values_processor_get_error(TeccStatementData *statement_data)
{
    if (statement_data == NULL)
        return "Invalid statement data";

    /* Begin - ReqID:TECC4.0 - 错误消息检索 */
    /* Check if there's an error stored in statement data */
    if (statement_data->error_message[0] != '\0')
    {
        return statement_data->error_message;
    }

    /* Check if there's an error in the values processor context */
    /* Note: values_processor_context field is not available in TeccStatementData structure
     * For now, we'll check the raw_values_list which might contain error information */
    if (statement_data->raw_values_list != NULL)
    {
        /* In a full implementation, we would check for errors in the raw values list */
        /* For now, just skip this check */
    }

    /* Return error message based on error_code */
    if (statement_data->error_code == TECC_VALUES_PROCESSOR_ERROR)
    {
        return "General error in values processor";
    }
    else if (statement_data->error_code == TECC_VALUES_PROCESSOR_MEMORY_ERROR)
    {
        return "Memory allocation error in values processor";
    }
    else if (statement_data->error_code == TECC_VALUES_PROCESSOR_ENCRYPTION_ERROR)
    {
        return "Encryption error in values processor";
    }
    else if (statement_data->error_code == TECC_VALUES_PROCESSOR_DECRYPTION_ERROR)
    {
        return "Decryption error in values processor";
    }
    else if (statement_data->error_code == TECC_VALUES_PROCESSOR_TYPE_MISMATCH)
    {
        return "Type mismatch error in values processor";
    }

    /* Default generic error message */
    /* End - ReqID:TECC4.0 - 错误消息检索 */
    return "Error in values processor";
}

/*
 * Helper functions
 */
TeccRawValue *
tecc_create_raw_value_from_node(Node *value_node)
{
    TeccRawValue *raw_value = NULL;

    if (value_node == NULL)
        return NULL;

    /* 添加调试信息查看节点类型 */
    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "tecc_create_raw_value_from_node: node type=%d (0x%x)",
                        value_node->type, value_node->type);

    /* Begin - ReqID:TECC4.0 - 节点类型检测和值提取 */
    switch (value_node->type)
    {
        case FE_T_Integer:
            {
                /* Handle integer constants - frontend uses Value struct */
                Value *value_node_ptr = (Value *) value_node;
                raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_INTEGER);
                if (raw_value != NULL)
                {
                    raw_value->original.integer_value = value_node_ptr->val.ival;
                    raw_value->length = sizeof(int64_t);
                }
            }
            break;

        case FE_T_Float:
            {
                /* Handle float constants - frontend uses Value struct */
                Value *value_node_ptr = (Value *) value_node;
                raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_FLOAT);
                if (raw_value != NULL)
                {
                    raw_value->original.float_value = value_node_ptr->val.dval;
                    raw_value->length = sizeof(double);
                }
            }
            break;

        case FE_T_String:
            {
                /* Handle string constants - frontend uses Value struct */
                Value *value_node_ptr = (Value *) value_node;
                raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_STRING);
                if (raw_value != NULL && value_node_ptr->val.str != NULL)
                {
                    raw_value->original.string_value = tecc_processor_strdup(value_node_ptr->val.str);
                    if (raw_value->original.string_value == NULL)
                    {
                        tecc_raw_value_destroy(raw_value);
                        raw_value = NULL;
                    }
                    else
                    {
                        raw_value->length = strlen(value_node_ptr->val.str);
                    }
                }
            }
            break;

        case FE_T_Const:
            {
                /* Handle Const nodes - these are generic constant nodes */
                Const *const_node = (Const *) value_node;
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                    "Processing Const node: consttype=%d, constisnull=%d",
                                    const_node->consttype, const_node->constisnull);

                if (const_node->constisnull)
                {
                    /* NULL constant */
                    raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_NULL);
                    if (raw_value != NULL)
                    {
                        raw_value->length = 0;
                        raw_value->is_null = true;  /* ReqID:TECC4.0 - Mark NULL for skip in processing */
                    }
                }
                else
                {
                    /* Determine the type based on consttype */
                    /* Handle common PostgreSQL OID types */
                    if (const_node->consttype == BOOLOID)
                    {
                        raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_BOOLEAN);
                        if (raw_value != NULL)
                        {
                            raw_value->original.boolean_value = const_node->constvalue.ival != 0;
                            raw_value->length = sizeof(bool);
                        }
                    }
                    else if (const_node->consttype == INT8OID)
                    {
                        raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_INTEGER);
                        if (raw_value != NULL)
                        {
                            raw_value->original.integer_value = const_node->constvalue.ival;
                            raw_value->length = sizeof(int64_t);
                        }
                    }
                    else if (const_node->consttype == INT2OID)
                    {
                        raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_INTEGER);
                        if (raw_value != NULL)
                        {
                            raw_value->original.integer_value = const_node->constvalue.ival;
                            raw_value->length = sizeof(int64_t);
                        }
                    }
                    else if (const_node->consttype == INT4OID)
                    {
                        raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_INTEGER);
                        if (raw_value != NULL)
                        {
                            raw_value->original.integer_value = const_node->constvalue.ival;
                            raw_value->length = sizeof(int64_t);
                        }
                    }
                    else if (const_node->consttype == TEXTOID)
                    {
                        raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_STRING);
                        if (raw_value != NULL && const_node->constvalue.sval != NULL)
                        {
                            raw_value->original.string_value = tecc_processor_strdup(const_node->constvalue.sval);
                            if (raw_value->original.string_value == NULL)
                            {
                                tecc_raw_value_destroy(raw_value);
                                raw_value = NULL;
                            }
                            else
                            {
                                raw_value->length = strlen(const_node->constvalue.sval);
                            }
                        }
                    }
                    else if (const_node->consttype == FLOAT4OID)
                    {
                        raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_FLOAT);
                        if (raw_value != NULL)
                        {
                            raw_value->original.float_value = const_node->constvalue.dval;
                            raw_value->length = sizeof(float);
                        }
                    }
                    else if (const_node->consttype == FLOAT8OID)
                    {
                        /* Handle float8 (double precision) constants */
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                            "Creating FLOAT raw value for FLOAT8OID (consttype=%d)",
                                            const_node->consttype);
                        raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_FLOAT);
                        if (raw_value != NULL)
                        {
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                                "Created raw_value=%p, type=%d (should be 3 for FLOAT)",
                                                raw_value, raw_value->type);
                            /* Note: Const node stores float values in dval field */
                            raw_value->original.float_value = const_node->constvalue.dval;
                            raw_value->length = sizeof(double);
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                                "Set float_value=%f, length=%zu",
                                                raw_value->original.float_value, raw_value->length);
                        }
                        else
                        {
                            tecc_processor_debug(TECC_LOG_WARNING,
                                                "Failed to create raw value for FLOAT8OID");
                        }
                    }
                    else if (const_node->consttype == BPCHAROID)
                    {
                        /* Handle bpchar (blank-padded char) constants */
                        raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_STRING);
                        if (raw_value != NULL && const_node->constvalue.sval != NULL)
                        {
                            raw_value->original.string_value = tecc_processor_strdup(const_node->constvalue.sval);
                            if (raw_value->original.string_value == NULL)
                            {
                                tecc_raw_value_destroy(raw_value);
                                raw_value = NULL;
                            }
                            else
                            {
                                raw_value->length = strlen(const_node->constvalue.sval);
                            }
                        }
                    }
                    else if (const_node->consttype == VARCHAROID)
                    {
                        /* Handle varchar constants */
                        raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_STRING);
                        if (raw_value != NULL && const_node->constvalue.sval != NULL)
                        {
                            raw_value->original.string_value = tecc_processor_strdup(const_node->constvalue.sval);
                            if (raw_value->original.string_value == NULL)
                            {
                                tecc_raw_value_destroy(raw_value);
                                raw_value = NULL;
                            }
                            else
                            {
                                raw_value->length = strlen(const_node->constvalue.sval);
                            }
                        }
                    }
                    else if (const_node->consttype == NUMERICOID)
                    {
                        tecc_processor_debug(TECC_LOG_ERROR,
                                            "NUMERIC type Const node not supported for encryption");
                        raw_value = NULL;
                    }
                    else
                    {
                        tecc_processor_debug(TECC_LOG_ERROR,
                                            "Unknown consttype %d in Const node, cannot encrypt",
                                            const_node->consttype);
                        raw_value = NULL;
                    }
                }
            }
            break;

        case FE_T_Null:
            {
                /* Handle NULL values */
                raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_NULL);
                if (raw_value != NULL)
                    raw_value->is_null = true;  /* ReqID:TECC4.0 */
            }
            break;

        case FE_T_Param:
            {
                /*
                 * Parameter references ($1, $2, ...) inside PREPARE VALUES.
                 * These have no concrete value at PREPARE time — the value
                 * arrives at EXECUTE.  Skip encryption here; the EXECUTE
                 * handler (process_param_values) will encrypt the actual
                 * parameter value if the column is encrypted.
                 *
                 * Fix: use Param (from grammar) not ParamRef (#110).
                 */
                Param *param_node;
                char param_str[32];

                param_node = (Param *) value_node;
                raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_STRING);
                if (raw_value != NULL)
                {
                    snprintf(param_str, sizeof(param_str), "$%d", param_node->paramid);
                    raw_value->original.string_value = pg_strdup(param_str);
                    if (raw_value->original.string_value == NULL)
                    {
                        tecc_raw_value_destroy(raw_value);
                        raw_value = NULL;
                    }
                    else
                    {
                        raw_value->length = strlen(param_str);
                        /* Param node values are NOT encrypted at PREPARE time */
                        raw_value->needs_encryption = false;
                        raw_value->state = TECC_RAW_VALUE_STATE_PROCESSED;
                        raw_value->location = (size_t)param_node->location;
                    }
                }
            }
            break;

	/* ReqID:TECC4.0 - ParamRef for extended query protocol ($N params) */
	case FE_T_ParamRef:
		{
			ParamRef *param_node;
			char param_str[32];

			param_node = (ParamRef *) value_node;
			raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_STRING);
			if (raw_value != NULL)
			{
				snprintf(param_str, sizeof(param_str), "$%d", param_node->number);
				raw_value->original.string_value = pg_strdup(param_str);
				if (raw_value->original.string_value == NULL)
				{
					tecc_raw_value_destroy(raw_value);
					raw_value = NULL;
				}
				else
				{
					raw_value->length = strlen(param_str);
					raw_value->needs_encryption = false;
					raw_value->state = TECC_RAW_VALUE_STATE_PROCESSED;
					raw_value->location = (size_t)param_node->location;
				}
			}
		}
		break;

        /* ReqID:TECC4.0 - DEFAULT keyword as expression value */
        case FE_T_SetToDefault:
            {
                raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_NULL);
                if (raw_value != NULL)
                {
                    raw_value->is_default = true;
                    raw_value->needs_encryption = false;
                    raw_value->state = TECC_RAW_VALUE_STATE_PROCESSED;
                }
            }
            break;

        case FE_T_ColumnRef:
            {
                /* Handle column references - store as string */
                ColumnRef *colref_node = (ColumnRef *) value_node;
                raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_STRING);
                if (raw_value != NULL)
                {
                    /* Build column reference string from fields list */
                    char *colref_str = NULL;
                    size_t colref_len = 0;
                    ListCell *cell;

                    /* Count total length needed */
                    foreach(cell, colref_node->fields)
                    {
                        if (cell->data.ptr_value != NULL)
                        {
                            /* 安全假设：在T_ColumnRef节点的fields列表中，元素应该是Value节点 */
                            Value *val = (Value *) cell->data.ptr_value;
                            /* 类型安全检查：确保这是FE_T_String类型的Value节点 */
                            if (val->type == FE_T_String && val->val.str != NULL)
                            {
                                colref_len += strlen(val->val.str) + 1; /* +1 for dot */
                            }
                        }
                    }

                    if (colref_len > 0)
                    {
                        colref_str = pg_malloc(colref_len);
                        if (colref_str != NULL)
                        {
                            char *ptr = colref_str;
                            bool first = true;

                            foreach(cell, colref_node->fields)
                            {
                                if (cell->data.ptr_value != NULL)
                                {
                                    /* 安全假设：在T_ColumnRef节点的fields列表中，元素应该是Value节点 */
                                    Value *val = (Value *) cell->data.ptr_value;
                                    /* 类型安全检查：确保这是FE_FE_T_String类型的Value节点 */
                                    if (val->type == FE_T_String && val->val.str != NULL)
                                    {
                                        if (!first)
                                        {
                                            *ptr++ = '.';
                                        }
                                        strcpy(ptr, val->val.str);
                                        ptr += strlen(val->val.str);
                                        first = false;
                                    }
                                }
                            }
                            *ptr = '\0';

                            raw_value->original.string_value = colref_str;
                            raw_value->length = strlen(colref_str);
                        }
                        else
                        {
                            tecc_raw_value_destroy(raw_value);
                            raw_value = NULL;
                        }
                    }
                    else
                    {
                        /* Empty column reference */
                        raw_value->original.string_value = tecc_processor_strdup("");
                        if (raw_value->original.string_value == NULL)
                        {
                            tecc_raw_value_destroy(raw_value);
                            raw_value = NULL;
                        }
                        else
                        {
                            raw_value->length = 0;
                        }
                    }
                }
            }
            break;

        case FE_T_FuncCall:
            {
                /* Handle function calls - store function name as string */
                FuncCall *func_node = (FuncCall *) value_node;
                raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_STRING);
                if (raw_value != NULL)
                {
                    /* Extract function name */
                    char *func_name = NULL;
                    if (func_node->funcname != NULL && func_node->funcname != NIL)
                    {
                        ListCell *cell = func_node->funcname->head;
                        if (cell != NULL && cell->data.ptr_value != NULL)
                        {
                            /* 安全假设：在T_FuncCall节点的funcname列表中，元素应该是Value节点 */
                            Value *val = (Value *) cell->data.ptr_value;
                            /* 类型安全检查：确保这是FE_T_String类型的Value节点 */
                            if (val->type == FE_T_String && val->val.str != NULL)
                            {
                                func_name = tecc_processor_strdup(val->val.str);
                            }
                        }
                    }

                    if (func_name != NULL)
                    {
                        raw_value->original.string_value = func_name;
                        raw_value->length = strlen(func_name);
                    }
                    else
                    {
                        raw_value->original.string_value = tecc_processor_strdup("unknown_function");
                        if (raw_value->original.string_value == NULL)
                        {
                            tecc_raw_value_destroy(raw_value);
                            raw_value = NULL;
                        }
                        else
                        {
                            raw_value->length = 16;
                        }
                    }
                }
            }
            break;

        case FE_T_TypeCast:
        {
                TypeCast *tc = (TypeCast *) value_node;
                if (tc->arg != NULL)
                {
                        raw_value = tecc_create_raw_value_from_node(tc->arg);
                }
        }
        break;

        default:
            {
                /* ReqID:TECC4.0 - Handle unary minus (A_Expr with '-' operator)
                 * PG parser represents -32768 as A_Expr(kind=OP, name="-", lexpr=NULL, rexpr=Integer(32768)) */
                if (value_node->type == FE_T_A_Expr)
                {
                    A_Expr *aexpr = (A_Expr *)value_node;
                    if (aexpr->kind == AEXPR_OP && aexpr->lexpr == NULL && aexpr->name != NIL)
                    {
                        Node *op_node = (Node *)linitial(aexpr->name);
                        char *op_name = strVal((Value *)op_node);
                        if (op_name != NULL && strcmp(op_name, "-") == 0 && aexpr->rexpr != NULL)
                        {
                            raw_value = tecc_create_raw_value_from_node(aexpr->rexpr);
                            if (raw_value != NULL)
                            {
                                if (raw_value->type == TECC_RAW_VALUE_TYPE_INTEGER)
                                {
                                    raw_value->original.integer_value = -raw_value->original.integer_value;
                                }
                                else if (raw_value->type == TECC_RAW_VALUE_TYPE_FLOAT)
                                {
                                    raw_value->original.float_value = -raw_value->original.float_value;
                                }
                                /* ReqID:TECC4.0 - STRING from overflowed integer literal
                                 * (e.g. 9223372036854775808).  Prepend '-' to negate,
                                 * preserving full precision. */
                                else if (raw_value->type == TECC_RAW_VALUE_TYPE_STRING &&
                                         raw_value->original.string_value != NULL)
                                {
                                    char *old = raw_value->original.string_value;
                                    size_t slen = strlen(old);
                                    char *ns = (char *)pg_malloc(slen + 2);
                                    ns[0] = '-';
                                    memcpy(ns + 1, old, slen + 1);
                                    raw_value->original.string_value = ns;
                                    pg_free(old);
                                }
                            }
                            break;  /* Exit switch, return raw_value */
                        }
                    }
                }

                /* For unknown node types, create a placeholder */
                tecc_processor_debug(TECC_LOG_WARNING,
                                    "Unknown node type %d in tecc_create_raw_value_from_node, using placeholder",
                                    value_node->type);
                raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_STRING);
                if (raw_value != NULL)
                {
                    raw_value->original.string_value = tecc_processor_strdup("[unknown_node_type]");
                    if (raw_value->original.string_value == NULL)
                    {
                        tecc_raw_value_destroy(raw_value);
                        raw_value = NULL;
                    }
                    else
                    {
                        raw_value->length = 18;
                    }
                }
            }
            break;
    }

    /* If raw value creation failed, return NULL */
    if (raw_value == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                            "Failed to create raw value from node type %d",
                            value_node->type);
    }

    /* End - ReqID:TECC4.0 - 节点类型检测和值提取 */
    return raw_value;
}

bool
tecc_column_needs_encryption(TeccStatementData *statement_data,
                            const char *schema_name,
                            const char *table_name,
                            const char *column_name)
{
    TeccColumnCacheItem *cache_result = NULL;
    TeccState *state = NULL;
    const char *database_name = NULL;
    const char *actual_schema_name = schema_name;

    /* 检查参数 */
    if (statement_data == NULL || column_name == NULL || table_name == NULL) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "tecc_column_needs_encryption: Invalid parameters - statement_data=%p, table_name=%s, column_name=%s",
                            statement_data,
                            table_name ? table_name : "NULL",
                            column_name ? column_name : "NULL");
        return false;
    }

    /* 获取state和database_name */
    if (statement_data->conn != NULL) {
        state = tecc_get_state(statement_data->conn);
        database_name = PQdb(statement_data->conn);
    }

    if (state == NULL || database_name == NULL) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "tecc_column_needs_encryption: Cannot get state or database name - state=%p, database_name=%s, conn=%p",
                            state, database_name ? database_name : "NULL", statement_data->conn);
        return false;
    }

    /* schema为NULL时由缓存接口按search_path顺序查找 */
    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "tecc_column_needs_encryption: state=%p, db=%s, schema=%s, table=%s, column=%s",
                        state, database_name,
                        actual_schema_name ? actual_schema_name : "(null)",
                        table_name, column_name);

    /* 查询元数据缓存 — schema可为NULL，缓存层按search_path查找 */
    cache_result = tecc_cache_get_encrypted_column_by_name(
        state, database_name, actual_schema_name, table_name, column_name
    );

    if (cache_result == NULL) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "tecc_column_needs_encryption: Cache miss - column %s.%s is not encrypted",
                            table_name, column_name);
        return false;
    }

    
    return true;
}

/* Begin - ReqID:TECC4.0 - 获取列的DEK名称函数 */
char *
tecc_get_column_dek_name(TeccStatementData *statement_data,
                        const char *schema_name,
                        const char *table_name,
                        const char *column_name)
{
    TeccColumnCacheItem *cache_result = NULL;
    TeccState *state = NULL;
    const char *database_name = NULL;
    const char *actual_schema_name = schema_name;
    char *dek_name = NULL;

    /* 检查参数 */
    if (statement_data == NULL || column_name == NULL || table_name == NULL) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "tecc_get_column_dek_name: Invalid parameters - statement_data=%p, table_name=%s, column_name=%s",
                            statement_data,
                            table_name ? table_name : "NULL",
                            column_name ? column_name : "NULL");
        return NULL;
    }

    /* 获取state和database_name */
    if (statement_data->conn != NULL) {
        state = tecc_get_state(statement_data->conn);
        database_name = PQdb(statement_data->conn);
    }

    if (state == NULL || database_name == NULL) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "tecc_get_column_dek_name: Cannot get state or database name - state=%p, database_name=%s, conn=%p",
                            state, database_name ? database_name : "NULL", statement_data->conn);
        return NULL;
    }

    /* 如果schema_name为NULL，从缓存管理器获取当前schema */
    if (actual_schema_name == NULL) {
        actual_schema_name = tecc_cache_get_guc_param(state, "current_schema");
        if (actual_schema_name == NULL || actual_schema_name[0] == '\0') {
            tecc_processor_debug(TECC_LOG_WARNING,
                               "tecc_get_column_dek_name: Could not get current schema from cache manager, returning NULL");
            return NULL;
        }
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "tecc_get_column_dek_name: Got state=%p, database_name=%s, schema_name=%s, table_name=%s, column_name=%s",
                        state, database_name, actual_schema_name, table_name, column_name);

    /* 查询元数据缓存 */
    cache_result = tecc_cache_get_encrypted_column_by_name(
        state,  /* state - from connection */
        database_name,  /* database_name - from connection */
        actual_schema_name,  /* schema_name - 使用实际schema名称 */
        table_name,
        column_name
    );

    if (cache_result == NULL) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "tecc_get_column_dek_name: Cache miss - column %s.%s is not encrypted",
                            table_name, column_name);
        return NULL;
    }

    /* 检查DEK名称是否存在 */
    if (cache_result->dek_name == NULL || cache_result->dek_name[0] == '\0') {
        tecc_processor_debug(TECC_LOG_ERROR,
                            "tecc_get_column_dek_name: DEK name is NULL or empty for column %s.%s",
                            table_name, column_name);
        return NULL;
    }

    /* 复制DEK名称字符串 */
    dek_name = pg_strdup(cache_result->dek_name);
    if (dek_name == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                            "tecc_get_column_dek_name: Failed to duplicate DEK name for column %s.%s",
                            table_name, column_name);
        return NULL;
    }

    
    return dek_name;
}
/* End - ReqID:TECC4.0 - 获取列的DEK名称函数 */

TeccValuesProcessorResult
tecc_process_values_list(TeccRawValuesList *list,
                        TeccStatementData *statement_data)
{
    int i;
    int count;
    TeccRawValue *value;
    TeccValuesProcessorResult result;

    if (list == NULL)
        return TECC_VALUES_PROCESSOR_SUCCESS;

    count = tecc_raw_values_list_count(list);
    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing %d values in list (list=%p, list->num_values=%d, list->max_values=%d)",
                        count, list, list ? list->num_values : -1, list ? list->max_values : -1);

    for (i = 0; i < count; i++)
    {
        value = tecc_raw_values_list_get(list, i);
        if (value == NULL)
            continue;

        result = values_process_single_value(value, statement_data);
        if (result != TECC_VALUES_PROCESSOR_SUCCESS)
        {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Failed to process value at index %d", i);
            return result;
        }
    }

    return TECC_VALUES_PROCESSOR_SUCCESS;
}

/*
 * Internal helper functions
 */
static TeccValuesProcessorResult TECC_ATTR_UNUSED
values_process_list(TeccRawValuesList *list, TeccStatementData *statement_data)
{
    int i;
    int count;
    TeccRawValue *value;
    TeccValuesProcessorResult result;

    if (list == NULL)
        return TECC_VALUES_PROCESSOR_SUCCESS;

    count = tecc_raw_values_list_count(list);
    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing %d values in list (list=%p)", count, list);

    for (i = 0; i < count; i++)
    {
        value = tecc_raw_values_list_get(list, i);
        if (value == NULL)
            continue;

        result = values_process_single_value(value, statement_data);
        if (result != TECC_VALUES_PROCESSOR_SUCCESS)
        {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Failed to process value at index %d", i);
            return result;
        }
    }

    return TECC_VALUES_PROCESSOR_SUCCESS;
}

static TeccValuesProcessorResult
values_process_single_value(TeccRawValue *value, TeccStatementData *statement_data)
{
    const char *algorithm;
    const char *key;

    if (value == NULL)
        return TECC_VALUES_PROCESSOR_SUCCESS;

    /* Skip if value is null or default */
    if (value->is_null || value->is_default)
        return TECC_VALUES_PROCESSOR_SUCCESS;

    /* Process the value */
    if (tecc_raw_value_process(value) != 0)
    {
        tecc_processor_set_error(statement_data, TECC_PROCESSOR_INTERNAL_ERROR,
                                "Failed to process raw value");
        return TECC_VALUES_PROCESSOR_ERROR;
    }

    /* Encrypt if needed */
    if (value->needs_encryption)
    {
        /* Get algorithm and key for the column */
        algorithm = tecc_values_processor_get_column_algorithm_with_state(statement_data, value->schema_name, value->table_name, value->column_name, NULL);
        key = tecc_values_processor_get_column_key_with_state(statement_data, value->schema_name, value->table_name, value->column_name);

        if (algorithm != NULL && key != NULL)
        {
            return tecc_values_processor_encrypt_value(value, algorithm, key, statement_data);
        }
        else
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "Missing algorithm or key for column %s",
                               value->column_name ? value->column_name : "unknown");
            return TECC_VALUES_PROCESSOR_ERROR;
        }
    }

    return TECC_VALUES_PROCESSOR_SUCCESS;
}

static bool TECC_ATTR_UNUSED
values_column_needs_encryption(const char *schema_name,
                              const char *table_name,
                              const char *column_name)
{
    /* TODO: Query metadata cache to check if column is encrypted */
    /* Placeholder implementation */
    if (column_name != NULL && strstr(column_name, "_enc") != NULL)
        return true;

    return false;
}

/*
 * Process query parameters for encryption
 *
 * This function processes parameter values in a parameterized query,
 * encrypting values that correspond to encrypted columns.
 *
 * Returns: TECC_VALUES_PROCESSOR_SUCCESS on success, error code on failure
 */
TeccValuesProcessorResult
tecc_values_processor_process_parameters(TeccStatementData *statement_data)
{
    size_t i;
    bool has_encrypted_params = false;
    TeccParamMapEntry *param_map = NULL;
    int param_map_size = 0;
    int mapping_result;
    TeccState *state = tecc_get_state(statement_data->conn);

    if (statement_data == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid statement_data in parameter processing");
        return TECC_VALUES_PROCESSOR_ERROR;
    }

    /* Check if we have parameters to process */
    if (statement_data->nParams == 0 || statement_data->paramValues == NULL) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "No parameters to process");
        return TECC_VALUES_PROCESSOR_SUCCESS;
    }

    

    /* Step 1: Get parameter-to-column mapping from parse tree */
    /* Use enhanced parameter mapping extraction */
    mapping_result = tecc_extract_param_mapping_enhanced(statement_data);

    if (mapping_result != 0) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Enhanced parameter mapping extraction failed, cannot process VALUES");
        return TECC_VALUES_PROCESSOR_ERROR;
    } else {
        /* Enhanced extraction successful, use the param_map from statement_data */
        param_map = statement_data->param_map;
        param_map_size = statement_data->param_map_size;
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Enhanced parameter mapping extracted: %d parameters mapped",
                           param_map_size);
    }

    /* Step 2: Process each parameter */
    for (i = 0; i < statement_data->nParams; i++) {
        const char *param_value = statement_data->paramValues[i];
        Oid param_type = statement_data->paramTypes ? statement_data->paramTypes[i] : 0;
        int param_length = statement_data->paramLengths ? statement_data->paramLengths[i] : -1;
        TeccUnifiedEncryptResult *encrypt_result;
        const char *table_name;
        const char *column_name;

        if (param_value != NULL) {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Processing parameter %zu: type=%u, length=%d",
                               i, param_type, param_length);

            /* Check if this parameter corresponds to an encrypted column */
            /* In a real implementation, we would:
               1. Look up the column name from parameter map
               2. Query metadata cache to check if column is encrypted
               3. Get encryption metadata (KEK/DEK names, algorithm support)
               4. Encrypt the value if needed
            */

            /* Check if this parameter corresponds to an encrypted column */
            if (param_map != NULL && i < param_map_size) {
                /* Use enhanced helper function to check if parameter needs encryption */
                bool needs_encryption = false;

                /* Create parameter info for enhanced checking */
                TeccParamInfo param_info;
                memset(&param_info, 0, sizeof(TeccParamInfo));
                param_info.param_index = i;
                param_info.param_node = NULL; /* Not available in this context */
                param_info.expression_depth = 0;
                param_info.value_type = VALUE_TYPE_UNKNOWN;

                /* Set context flags from parameter map if available */
                if (param_map != NULL && i < param_map_size) {
                    param_info.is_in_function_call = param_map[i].is_in_function_call;
                    param_info.is_in_subquery = param_map[i].is_in_subquery;
                    param_info.is_in_op_expr = param_map[i].is_in_op_expr;
                    param_info.parent_context = param_map[i].expression_context;
                }

                /* Use enhanced encryption check */
                needs_encryption = tecc_param_needs_encryption_enhanced(
                    &param_info,
                    statement_data
                );

                if (needs_encryption) {
                    param_map[i].needs_encryption = true;
                    has_encrypted_params = true;

                    /* Encrypt the parameter value */
                    encrypt_result = NULL;

                    /* Get table and column names from parameter map or use defaults */
                    table_name = param_map[i].table_name ? param_map[i].table_name : "unknown_table";
                    column_name = param_map[i].column_name ? param_map[i].column_name : "unknown_column";

                    /* Call unified encryption interface */
                    encrypt_result = tecc_unified_encrypt_string(
                        statement_data->conn,  /* 添加连接参数 */
                        param_value,
                        table_name,
                        column_name,
                        NULL,  /* Use default KEK */
                        NULL,  /* Use default DEK */
                        state ? state->crypto_ctx : NULL
                    );

                    if (encrypt_result != NULL && encrypt_result->error_code == TECC_CRYPTO_SUCCESS) {
                        /* Store encrypted value */
                        param_map[i].encrypted_value = pg_strdup(encrypt_result->hex_string);
                        param_map[i].encrypted_value_size = encrypt_result->hex_len;

                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                           "Parameter %zu encrypted successfully", i);

                        /* Update parameter value in statement data */
                        /* Note: In a real implementation, we would need to manage memory properly */
                        /* For now, we'll just mark that encryption was performed */

                        /* Free encryption result */
                        tecc_unified_free_encrypt_result(encrypt_result);
                    } else {
                        tecc_processor_debug(TECC_LOG_ERROR,
                                           "Failed to encrypt parameter %zu: %s",
                                           i, encrypt_result ? encrypt_result->error_msg : "Unknown error");
                        if (encrypt_result != NULL) {
                            tecc_unified_free_encrypt_result(encrypt_result);
                        }
                    }
                }
            }
        }
    }

    /* Step 3: Update statement data with encryption results */
    if (has_encrypted_params) {
        /* Mark that parameter encryption was performed */
        statement_data->needs_rewrite = 1;

        /* Store parameter map in statement data for later use */
        /* In a real implementation, this would be used during query reconstruction */
        if (statement_data->param_map != NULL) {
            /* Free existing parameter map */
            tecc_free_param_map(statement_data->param_map, statement_data->param_map_size);
        }
        statement_data->param_map = param_map;
        statement_data->param_map_size = param_map_size;

        
    } else {
        /* No encryption needed, free parameter map */
        if (param_map != NULL) {
            pg_free(param_map);
        }
    }

    

    return TECC_VALUES_PROCESSOR_SUCCESS;
}

/*
 * Decrypt and restore value (参考openGauss设计)
 */
TeccDecryptResult
tecc_values_processor_deprocess_value(PGconn *conn,
                                     const unsigned char *processed_data,
                                     size_t processed_data_size,
                                     int original_typeid,
                                     int format,
                                     unsigned char **plain_text,
                                     size_t *plain_text_size,
                                     TeccProcessStatus process_status,
                                     const char *table_name,
                                     const char *column_name)
{
    /* Begin - ReqID:TECC4.0 - 值反处理函数（解密和恢复） */
    unsigned char *unescaped_processed_data = NULL;
    size_t unescaped_processed_data_size = 0;
    size_t plain_text_size_tmp = 0;

    /* 1. 输入验证 */
    if (processed_data == NULL || processed_data_size <= 0)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid input data for deprocessing");
        return TECC_DECRYPT_ERROR;
    }

    /* 2. 数据解转义（文本格式） */
    if (format == 0)  /* 文本格式 */
    {
        unsigned char *processed_data_tmp = NULL;
        char *text_to_deprocess = NULL;
        char *final = NULL;

        /* 2.1 添加字符串结束符 */
        processed_data_tmp = (unsigned char *)pg_malloc(processed_data_size + 1);
        if (processed_data_tmp == NULL)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "Memory allocation failed for text processing");
            return TECC_MEMORY_ERROR;
        }

        memcpy(processed_data_tmp, processed_data, processed_data_size);
        processed_data_tmp[processed_data_size] = 0;

        /* 2.2 处理默认值格式（包含::的部分） */
        final = strchr((char *)processed_data_tmp, ':');
        if (final != NULL && final[1] == ':')
        {
            /* 提取加密数据部分（跳过::type部分） */
            text_to_deprocess = (char *)processed_data_tmp;
            *final = 0;  /* 截断字符串 */
        }
        else
        {
            text_to_deprocess = (char *)processed_data_tmp;
        }

        /* 2.3 解转义BYTEA格式数据 */
        unescaped_processed_data = PQunescapeBytea((const unsigned char *)text_to_deprocess,
                                                  &unescaped_processed_data_size);
        pg_free(processed_data_tmp);

        if (unescaped_processed_data == NULL)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "Failed to unescape BYTEA data");
            return TECC_FORMAT_ERROR;
        }
    }
    else  /* 二进制格式 */
    {
        unescaped_processed_data = (unsigned char *)processed_data;
        unescaped_processed_data_size = processed_data_size;
    }

    /* 3. 空值处理 */
    if (unescaped_processed_data_size == 0)
    {
        *plain_text_size = 0;
        if (format == 0)
            PQfreemem(unescaped_processed_data);
        return tecc_process_null_value(plain_text, plain_text_size,
                                      original_typeid, process_status);
    }

    /* 4. 数据大小验证 */
    if (unescaped_processed_data_size < sizeof(Oid))
    {
        if (format == 0)
            PQfreemem(unescaped_processed_data);
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid data length: %zu", unescaped_processed_data_size);
        return TECC_INVALID_DATA_LENGTH;
    }

    /* 5. 核心解密：调用调度器 */
    {
        TeccUnifiedDecryptResult *unified_decrypt_result = NULL;
        const char *hex_string = NULL;

        /* 将处理后的数据转换为十六进制字符串 */
        /* 注意：unescaped_processed_data可能已经是十六进制格式 */
        hex_string = (const char *)unescaped_processed_data;

        /* 使用线程安全版本 */
        unified_decrypt_result = tecc_unified_decrypt_hex(
            conn,
            hex_string,
            table_name ? table_name : "unknown_table",
            column_name ? column_name : "unknown_column",
            NULL
        );

        /* 检查解密结果 */
        if (unified_decrypt_result == NULL ||
            unified_decrypt_result->error_code != TECC_CRYPTO_SUCCESS)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "Decryption failed: %s",
                               unified_decrypt_result ? unified_decrypt_result->error_msg : "Unknown error");

            if (format == 0)
                PQfreemem(unescaped_processed_data);

            if (unified_decrypt_result != NULL)
                tecc_unified_free_decrypt_result(unified_decrypt_result);

            return TECC_DECRYPT_ERROR;
        }

        /* 存储解密结果 */
        plain_text_size_tmp = unified_decrypt_result->plain_len;
        *plain_text = (unsigned char *)pg_malloc(plain_text_size_tmp + 1);
        if (*plain_text == NULL)
        {
            if (format == 0)
                PQfreemem(unescaped_processed_data);

            tecc_unified_free_decrypt_result(unified_decrypt_result);
            return TECC_MEMORY_ERROR;
        }

        memcpy(*plain_text, unified_decrypt_result->plaintext, plain_text_size_tmp);
        (*plain_text)[plain_text_size_tmp] = 0;

        /* 清理解密结果 */
        tecc_unified_free_decrypt_result(unified_decrypt_result);
    }

    /* 6. 清理临时数据 */
    if (format == 0)
        PQfreemem(unescaped_processed_data);

    /* 8. 特殊处理：无权限情况 */
    /* 注意：如果解密失败，函数已经返回错误 */
    /* 如果解密成功但plain_text_size_tmp为0，表示解密结果是空字符串 */
    if (plain_text_size_tmp == 0)
    {
        /* 解密结果是空字符串，这是有效的 */
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Decryption result is empty string");
    }

    /* 9. 格式转换 */
    *plain_text_size = plain_text_size_tmp;
    if (format == 0)  /* 文本格式 */
    {
        tecc_process_text_format(plain_text, plain_text_size,
                                process_status, original_typeid);
    }
    else  /* 二进制格式 */
    {
        /* 实现二进制格式恢复 */
        tecc_process_binary_format(plain_text, plain_text_size,
                                   process_status, original_typeid);
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Value deprocessing completed successfully, size: %zu",
                        *plain_text_size);

    return TECC_DECRYPT_SUCCESS;
    /* End - ReqID:TECC4.0 - 值反处理函数（解密和恢复） */
}

/*
 * Process NULL value
 */
static TeccDecryptResult
tecc_process_null_value(unsigned char **plain_text,
                       size_t *plain_text_size,
                       int original_typeid,
                       TeccProcessStatus process_status)
{
    const char *null_str = "NULL";

    /* 根据处理状态返回适当的NULL表示 */
    switch (process_status)
    {
        case TECC_ONLY_VALUE:
            null_str = "NULL";
            break;
        case TECC_ADD_QUOTATION:
            null_str = "'NULL'";
            break;
        case TECC_ADD_TYPE:
            null_str = "'NULL'::unknown";
            break;
        default:
            null_str = "NULL";
            break;
    }

    *plain_text = (unsigned char *)tecc_processor_strdup(null_str);
    if (*plain_text == NULL)
        return TECC_MEMORY_ERROR;

    *plain_text_size = strlen(null_str);
    return TECC_DECRYPT_SUCCESS;
}

/*
 * Process text format
 */
static void
tecc_process_text_format(unsigned char **plain_text,
                        size_t *plain_text_size,
                        TeccProcessStatus process_status,
                        int original_typeid)
{
    char *result = NULL;
    size_t result_size = 0;

    if (*plain_text == NULL || *plain_text_size == 0)
        return;

    /* 根据处理状态调整文本格式 */
    switch (process_status)
    {
        case TECC_ONLY_VALUE:
            /* 保持原样 */
            break;
        case TECC_ADD_QUOTATION:
            /* 添加引号 */
            result_size = *plain_text_size + 2;
            result = (char *)pg_malloc(result_size + 1);
            if (result != NULL)
            {
                result[0] = '\'';
                memcpy(result + 1, *plain_text, *plain_text_size);
                result[result_size - 1] = '\'';
                result[result_size] = 0;

                pg_free(*plain_text);
                *plain_text = (unsigned char *)result;
                *plain_text_size = result_size;
            }
            break;
        case TECC_ADD_TYPE:
            /* 添加类型后缀 */
            result_size = *plain_text_size + 2 + 10;  /* 引号 + 类型名 */
            result = (char *)pg_malloc(result_size + 1);
            if (result != NULL)
            {
                result[0] = '\'';
                memcpy(result + 1, *plain_text, *plain_text_size);
                result[*plain_text_size + 1] = '\'';

                /* 添加类型后缀（简化实现） */
                strcpy(result + *plain_text_size + 2, "::text");

                pg_free(*plain_text);
                *plain_text = (unsigned char *)result;
                *plain_text_size = strlen(result);
            }
            break;
        default:
            break;
    }
}

/*
 * Rewrite values in SQL query
 *
 * This function replaces plaintext values with encrypted values in the SQL query.
 * It processes all raw values that have been encrypted and need to be rewritten.
 */
TeccValuesProcessorResult
tecc_values_processor_rewrite_values(TeccStatementData *statement_data)
{
    TeccRawValuesList *values_list;
    size_t i;
    size_t orig_pos = 0;
    char *new_query = NULL;
    size_t new_query_len = 0;
    size_t new_query_capacity = 0;
    const char *orig_query;
    size_t orig_len;
    size_t encrypted_total_extra = 0;

    if (statement_data == NULL || statement_data->raw_values_list == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid arguments to tecc_values_processor_rewrite_values");
        return TECC_VALUES_PROCESSOR_ERROR;
    }

    values_list = statement_data->raw_values_list;

    if (values_list->num_values == 0)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "No values to rewrite");
        return TECC_VALUES_PROCESSOR_SUCCESS;
    }

    /* Sort raw values by location to ensure correct ordering even when
     * entries are added out of order (e.g., SET values before WHERE values,
     * or multi-statement entries from different stmts). ReqID:TECC4.0 */
    {
        int si, sj;
        for (si = 0; si < (int)values_list->num_values - 1; si++)
        {
            for (sj = 0; sj < (int)values_list->num_values - 1 - si; sj++)
            {
                TeccRawValue *a = values_list->values[sj];
                TeccRawValue *b = values_list->values[sj + 1];
                if (a == NULL || b == NULL) continue;
                if (a->location > b->location)
                {
                    TeccRawValue *tmp = values_list->values[sj];
                    values_list->values[sj] = values_list->values[sj + 1];
                    values_list->values[sj + 1] = tmp;
                }
            }
        }
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Starting SQL query rewrite for %d values", values_list->num_values);

    orig_query = statement_data->query;
    orig_len = (orig_query != NULL) ? strlen(orig_query) : 0;
    if (orig_query == NULL || orig_len == 0)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Original query is NULL or empty, cannot rewrite");
        return TECC_VALUES_PROCESSOR_ERROR;
    }

    /* Pass 1: calculate total buffer size needed.
     * For each encrypted value, the replacement is:
     *   '\xhex...'::type_name
     * instead of the original value text.
     * We also need to account for the copy of non-replaced text between values.
     * Start with original length + estimated extra for all encrypted values.
     * Bounds-check every raw_value location/data_size against orig_query. */
    for (i = 0; i < values_list->num_values; i++)
    {
        TeccRawValue *rv = values_list->values[i];
        size_t encrypted_repr_len;
        if (rv == NULL)
            continue;
        /* Clamp location/data_size to valid range */
        if (rv->location > orig_len)
            rv->location = orig_len;
        if (rv->location + rv->data_size > orig_len)
            rv->data_size = (rv->location <= orig_len) ? (orig_len - rv->location) : 0;

        if (rv->state != TECC_RAW_VALUE_STATE_ENCRYPTED &&
            rv->state != TECC_RAW_VALUE_STATE_PROCESSED)
            continue;
        if (rv->processed.encrypted_value == NULL)
            continue;

        /* encrypted hex string + 2 quotes + "::" + type_name */
        encrypted_repr_len = 2 + strlen(rv->processed.encrypted_value) + 2;
        if (rv->column_type != NULL && rv->column_type[0] != '\0')
            encrypted_repr_len += strlen(rv->column_type);

        /* extra = new_repr_len - original_value_len */
        if (rv->data_size < encrypted_repr_len)
            encrypted_total_extra += encrypted_repr_len - rv->data_size;
    }

    new_query_capacity = orig_len + encrypted_total_extra + 1024;
    new_query = (char *)pg_malloc(new_query_capacity);
    if (new_query == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Memory allocation failed for rewritten query");
        return TECC_VALUES_PROCESSOR_MEMORY_ERROR;
    }

    /* Process all values in the list.
     * Algorithm: sequentially scan the ORIGINAL query from left to right.
     * For each value that needs rewriting:
     *   1. Copy original text from orig_pos to raw_value->location (preserving delimiters)
     *   2. Append the encrypted representation '\xhex...'::type_name
     *   3. Advance orig_pos past the original value (location + data_size)
     * Values that don't need rewriting are left as-is in the original text.
     */
    for (i = 0; i < values_list->num_values; i++)
    {
        TeccRawValue *raw_value = values_list->values[i];
        size_t value_len;
        const char *encrypted_value;

        if (raw_value == NULL)
            continue;

        /* Copy original text from current position up to this value's location */
        if (raw_value->location > orig_pos)
        {
            size_t copy_len = raw_value->location - orig_pos;
            while (new_query_len + copy_len + 1 > new_query_capacity)
            {
                new_query_capacity *= 2;
                new_query = (char *)pg_realloc(new_query, new_query_capacity);
                if (new_query == NULL)
                    return TECC_VALUES_PROCESSOR_MEMORY_ERROR;
            }
            memcpy(new_query + new_query_len, orig_query + orig_pos, copy_len);
            new_query_len += copy_len;
        }

        /* Check if value needs to be rewritten */
        if (raw_value->state != TECC_RAW_VALUE_STATE_ENCRYPTED &&
            raw_value->state != TECC_RAW_VALUE_STATE_PROCESSED)
        {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Skipping value at position %zu (state: %d, data_size: %zu)",
                               raw_value->location, raw_value->state, raw_value->data_size);
            /* Copy the value's original text into new_query, since the
             * prefix copy only went up to (not including) raw_value->location.
             * The value text starts at location and has length data_size. */
            if (raw_value->location + raw_value->data_size > orig_pos)
            {
                size_t skip_copy_len = (raw_value->location + raw_value->data_size) - orig_pos;
                while (new_query_len + skip_copy_len + 1 > new_query_capacity)
                {
                    new_query_capacity *= 2;
                    new_query = (char *)pg_realloc(new_query, new_query_capacity);
                    if (new_query == NULL)
                        return TECC_VALUES_PROCESSOR_MEMORY_ERROR;
                }
                /* Copy from raw_value->location (not from orig_pos) to get
                 * just this value's text, not the text before it. */
                memcpy(new_query + new_query_len, orig_query + raw_value->location,
                       raw_value->data_size);
                new_query_len += raw_value->data_size;
                orig_pos = raw_value->location + raw_value->data_size;
            }
            continue;
        }

        /* Get encrypted value */
        encrypted_value = raw_value->processed.encrypted_value;
        if (encrypted_value == NULL)
        {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "No encrypted value for raw value at position %zu",
                               raw_value->location);
            /* Copy original value text, then advance orig_pos */
            if (raw_value->location + raw_value->data_size > orig_pos)
            {
                while (new_query_len + raw_value->data_size + 1 > new_query_capacity)
                {
                    new_query_capacity *= 2;
                    new_query = (char *)pg_realloc(new_query, new_query_capacity);
                    if (new_query == NULL)
                        return TECC_VALUES_PROCESSOR_MEMORY_ERROR;
                }
                memcpy(new_query + new_query_len, orig_query + raw_value->location,
                       raw_value->data_size);
                new_query_len += raw_value->data_size;
                orig_pos = raw_value->location + raw_value->data_size;
            }
            continue;
        }

        value_len = strlen(encrypted_value);

        /* Ensure capacity for encrypted value representation:
         * '\xhex...'::type_name
         */
        {
            size_t type_name_len = (raw_value->column_type != NULL) ?
                                   strlen(raw_value->column_type) : 0;
            size_t needed = new_query_len + 2 + value_len + 2 + type_name_len + 1;
            while (needed > new_query_capacity)
            {
                new_query_capacity *= 2;
                new_query = (char *)pg_realloc(new_query, new_query_capacity);
                if (new_query == NULL)
                    return TECC_VALUES_PROCESSOR_MEMORY_ERROR;
            }
        }

        /* Write: 'hex'::type_name — use plain quotes, strip \x prefix.
         * PG14 default (standard_conforming_strings=on) treats \ as literal
         * in plain strings, avoiding E'...' issues where \xHH becomes a hex
         * escape and corrupts binary ciphertext (e.g. \x7c → '|'). */
        new_query[new_query_len++] = '\'';
        {
            const char *src = encrypted_value;
            const char *src_end = encrypted_value + value_len;
            /* Skip \x prefix if present */
            if (value_len >= 2 && src[0] == '\\' && src[1] == 'x')
            {
                src += 2;
                value_len -= 2;
            }
            memcpy(new_query + new_query_len, src, value_len);
            new_query_len += value_len;
        }
        new_query[new_query_len++] = '\'';

        if (raw_value->column_type != NULL && raw_value->column_type[0] != '\0')
        {
            new_query[new_query_len++] = ':';
            new_query[new_query_len++] = ':';
            memcpy(new_query + new_query_len, raw_value->column_type,
                   strlen(raw_value->column_type));
            new_query_len += strlen(raw_value->column_type);
        }

        /* Advance orig_pos past the original value in the original query */
        {
            size_t value_end = raw_value->location + raw_value->data_size;
            if (value_end > orig_pos)
                orig_pos = value_end;
        }

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Rewrote value at location %zu (orig_size: %zu, encrypted_str_len: %zu, col_type: %s, col_name: %s)",
                           raw_value->location, raw_value->data_size, value_len,
                           raw_value->column_type ? raw_value->column_type : "NULL",
                           raw_value->column_name ? raw_value->column_name : "NULL");

        /* Add debug for first 20 chars of encrypted value */
        /* Add debug for sequential copy state */
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "  [state] orig_pos=%zu, new_query_len=%zu, value_end=%zu",
                           orig_pos, new_query_len,
                           raw_value->location + raw_value->data_size);
    }

    /* Copy the remaining part of the original query */
    if (orig_pos < orig_len)
    {
        size_t remaining_len = orig_len - orig_pos;
        while (new_query_len + remaining_len + 1 > new_query_capacity)
        {
            new_query_capacity *= 2;
            new_query = (char *)pg_realloc(new_query, new_query_capacity);
            if (new_query == NULL)
                return TECC_VALUES_PROCESSOR_MEMORY_ERROR;
        }
        memcpy(new_query + new_query_len, orig_query + orig_pos, remaining_len);
        new_query_len += remaining_len;
    }

    /* Null-terminate the new query */
    new_query[new_query_len] = '\0';


    /* Update the statement data with the rewritten query */
    if (statement_data->rewritten_query != NULL)
    {
        pg_free(statement_data->rewritten_query);
    }
    statement_data->rewritten_query = new_query;

    

    return TECC_VALUES_PROCESSOR_SUCCESS;
}

/*
 * Process binary format for decrypted values
 *
 * This function handles binary format restoration for decrypted values.
 * Binary format requires special handling compared to text format.
 */
static void
tecc_process_binary_format(unsigned char **plain_text,
                          size_t *plain_text_size,
                          TeccProcessStatus process_status,
                          int original_typeid)
{
    /* 对于二进制格式，通常不需要额外的格式处理 */
    /* 二进制数据应该保持原样，不需要添加引号或类型后缀 */

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Processing binary format: size=%zu, status=%d, typeid=%d",
                       *plain_text_size, process_status, original_typeid);

    /* 根据处理状态进行相应处理 */
    switch (process_status)
    {
        case TECC_ONLY_VALUE:
            /* 二进制格式保持原样 */
            break;

        case TECC_ADD_QUOTATION:
            /* 二进制格式通常不需要引号 */
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Binary format skipping quotation marks");
            break;

        case TECC_ADD_TYPE:
            /* 二进制格式可以添加类型转换，但通常不需要 */
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Binary format skipping type suffix");
            break;

        default:
            tecc_processor_debug(TECC_LOG_WARNING,
                               "Unknown process status for binary format: %d",
                               process_status);
            break;
    }
}

/*
 * tecc_free_param_map - 释放参数映射表内存
 *
 * 参数:
 *   param_map - 参数映射表指针
 *   param_map_size - 参数映射表大小
 */
static void
tecc_free_param_map(TeccParamMapEntry *param_map, int param_map_size)
{
    int i;

    if (param_map == NULL) {
        return;
    }

    /* 释放每个条目的字符串字段 */
    for (i = 0; i < param_map_size; i++) {
        if (param_map[i].column_name != NULL) {
            pg_free(param_map[i].column_name);
        }
        if (param_map[i].table_name != NULL) {
            pg_free(param_map[i].table_name);
        }
        if (param_map[i].schema_name != NULL) {
            pg_free(param_map[i].schema_name);
        }
        if (param_map[i].encrypted_value != NULL) {
            /* Securely erase encrypted value */
            memset(param_map[i].encrypted_value, 0, param_map[i].encrypted_value_size);
            pg_free(param_map[i].encrypted_value);
        }
    }

    /* 释放参数映射表本身 */
    pg_free(param_map);
}

/*
 * tecc_parameter_needs_encryption - 检查参数是否需要加密
 *
 * 参数:
 *   statement_data - 语句数据
 *   param_index - 参数索引
 *   param_type - 参数类型OID
 *   param_value - 参数值
 *
 * 返回值:
 *   true - 参数需要加密
 *   false - 参数不需要加密
 */
static bool TECC_ATTR_UNUSED
tecc_parameter_needs_encryption(TeccStatementData *statement_data,
                               int param_index,
                               Oid param_type,
                               const char *param_value)
{
    /* Begin - ReqID:TECC4.0 - 参数加密需求检查 */
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    TeccParamMapEntry *param_entry = NULL;
    TeccState *state = NULL;
    TeccCacheManager *cache_manager = NULL;
    const char *database_name = NULL;
    const char *schema_name = NULL;
    TeccColumnCacheItem *column_info = NULL;

    /* 基本参数检查 */
    if (statement_data == NULL || param_value == NULL) {
        return false;
    }

    /* 检查参数映射表中是否有该参数的信息 */
    if (statement_data->param_map != NULL && param_index < statement_data->param_map_size) {
        param_entry = &statement_data->param_map[param_index];

        /* 如果参数映射表中已经标记了是否需要加密，直接返回 */
        if (param_entry->needs_encryption) {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Parameter %d marked as needing encryption in param_map",
                               param_index);
            return true;
        }

        /* 如果参数映射表中有列名信息，查询缓存检查是否加密 */
        if (param_entry->column_name != NULL && param_entry->table_name != NULL) {
            /* 获取缓存管理器 */
            state = tecc_get_state(statement_data->conn);
            cache_manager = tecc_cache_get_manager(state);
            if (cache_manager != NULL) {
                /* 查询加密列信息 */
                /* ReqID:TECC4.0 - database_name declared at function scope beginning */
                if (statement_data->conn != NULL) {
                    database_name = PQdb(statement_data->conn);
                    if (database_name == NULL || database_name[0] == '\0') {
                        /* 无法获取数据库名，返回false */
                        tecc_processor_debug(TECC_LOG_WARNING,
                                           "tecc_values_processor_check_param_needs_encryption: Could not get database name from connection");
                        return false;
                    }
                } else {
                    /* 没有连接，无法获取数据库名，返回false */
                    tecc_processor_debug(TECC_LOG_WARNING,
                                       "tecc_values_processor_check_param_needs_encryption: No connection available");
                    return false;
                }

                /* 获取schema名 */
                /* ReqID:TECC4.0 - schema_name declared at function scope beginning */
                if (param_entry->schema_name != NULL) {
                    schema_name = param_entry->schema_name;
                } else if (state != NULL) {
                    /* 从缓存获取当前schema */
                    schema_name = tecc_cache_get_guc_param(state, "current_schema");
                    if (schema_name == NULL || schema_name[0] == '\0') {
                        /* 无法获取schema名，返回false */
                        tecc_processor_debug(TECC_LOG_WARNING,
                                           "tecc_values_processor_check_param_needs_encryption: Could not get current schema from cache");
                        return false;
                    }
                } else {
                    /* 没有state，无法获取schema名，返回false */
                    tecc_processor_debug(TECC_LOG_WARNING,
                                       "tecc_values_processor_check_param_needs_encryption: No state available");
                    return false;
                }

                column_info = tecc_cache_get_encrypted_column_by_name(
                    state,  /* 使用从连接获取的state参数 */
                    database_name,  /* 从连接获取数据库名 */
                    schema_name,
                    param_entry->table_name,
                    param_entry->column_name
                );

                if (column_info != NULL) {
                    /* 找到加密列信息 */
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                       "Parameter %d corresponds to encrypted column %s.%s",
                                       param_index, param_entry->table_name, param_entry->column_name);
                    /* 标记参数需要加密 */
                    param_entry->needs_encryption = true;
                    return true;
                } else {
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                       "Parameter %d corresponds to non-encrypted column %s.%s",
                                       param_index, param_entry->table_name, param_entry->column_name);
                    return false;
                }
            }
        }
    }

    /* 如果没有参数映射信息，使用简化逻辑 */
    /* 检查查询中是否包含加密类型提示 */
    if (strstr(statement_data->query, "_enc") != NULL) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Parameter %d may need encryption (query contains '_enc')",
                           param_index);
        return true;
    }

    /* 默认返回false */
    return false;
    /* End - ReqID:TECC4.0 */
}

/*
 * tecc_extract_param_mapping_from_parse_tree - 从解析树中提取参数映射
 *
 * 参数:
 *   statement_data - 语句数据
 *   param_map - 输出参数映射表指针
 *   param_map_size - 输出参数映射表大小
 *
 * 返回值:
 *   0 - 成功
 *   -1 - 失败
 */
static int TECC_ATTR_UNUSED
tecc_extract_param_mapping_from_parse_tree(TeccStatementData *statement_data,
                                          TeccParamMapEntry **param_map,
                                          int *param_map_size)
{
    /* Begin - ReqID:TECC4.0 - 从解析树提取参数映射 */

    /* 初始化输出参数 */
    *param_map = NULL;
    *param_map_size = 0;

    /* 基本检查 */
    if (statement_data == NULL || statement_data->parse_tree == NIL) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "No parse tree available for parameter mapping extraction");
        return -1;
    }

    /* 检查是否有参数需要处理 */
    if (statement_data->nParams == 0) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "No parameters to map");
        return -1;
    }

    /* 在实际实现中，我们需要:
       1. 遍历解析树，查找参数节点(ParamRef节点)
       2. 确定每个参数对应的列(表名、字段名)
       3. 构建参数映射表
    */

    /* 简化实现: 创建一个基本的参数映射表 */
    *param_map_size = statement_data->nParams;
    *param_map = pg_malloc0(sizeof(TeccParamMapEntry) * (*param_map_size));
    if (*param_map == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Memory allocation failed for parameter map extraction");
        return -1;
    }

    /* 初始化参数映射表 */
    for (int i = 0; i < *param_map_size; i++) {
        (*param_map)[i].param_index = i;
        (*param_map)[i].column_name = NULL;
        (*param_map)[i].table_name = NULL;
        (*param_map)[i].schema_name = NULL;
        (*param_map)[i].needs_encryption = false;
        (*param_map)[i].encrypted_value = NULL;
        (*param_map)[i].encrypted_value_size = 0;

        /* 在实际实现中，这里会从解析树中提取实际的列名信息 */
        /* 例如:
           if (find_param_in_parse_tree(statement_data->parse_tree, i, &table_name, &column_name)) {
               (*param_map)[i].table_name = pg_strdup(table_name);
               (*param_map)[i].column_name = pg_strdup(column_name);
           }
        */
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Created basic parameter mapping for %d parameters",
                       *param_map_size);

    return 0;
    /* End - ReqID:TECC4.0 */
}

/*
 * Get encryption key for column with statement data
 */
const char *
tecc_values_processor_get_column_key_with_state(TeccStatementData *statement_data,
                                               const char *schema_name,
                                               const char *table_name,
                                               const char *column_name)
{
    /* Begin - ReqID:TECC4.0 - 从缓存模块获取列加密密钥（带状态） */
    /* Variable declarations at function beginning for ISO C90 compliance */
    TeccColumnCacheItem *column_item = NULL;
    TeccDekCacheItem *dek_item = NULL;
    TeccState *state = NULL;

    if (column_name == NULL)
        return NULL;

    /* Get TeccState from statement data */
    if (statement_data != NULL && statement_data->conn != NULL)
    {
        state = tecc_get_state(statement_data->conn);
    }

    /* First, try to get column metadata from cache */
    if (table_name != NULL)
    {
        /* Get database name from connection */
        const char *database_name = NULL;
        if (statement_data != NULL && statement_data->conn != NULL) {
            database_name = PQdb(statement_data->conn);
            if (database_name == NULL || database_name[0] == '\0') {
                    /* 无法获取数据库名，返回NULL */
                tecc_processor_debug(TECC_LOG_WARNING,
                                   "Could not get database name from connection, returning error");
                return NULL;
            }
        } else {
                /* 无法获取数据库名，返回NULL */
                tecc_processor_debug(TECC_LOG_WARNING,
                                   "Could not get database name from connection, returning error");
                return NULL;
        }

        /* Use simplified cache interface with table name */
        column_item = tecc_cache_get_encrypted_column_by_name(
            state,  /* state - use actual state if available */
            database_name,  /* database_name - from connection */
            schema_name,  /* schema_name */
            table_name,
            column_name
        );
    }

    if (column_item != NULL)
    {
        /* Try OID-based lookup first, then fallback to name-based */
        if (column_item->dek_id != InvalidOid)
        {
            dek_item = tecc_cache_get_dek_by_oid(state, column_item->dek_id);
            if (dek_item != NULL && dek_item->key_name != NULL)
                return dek_item->key_name;
        }

        /* Fallback: lookup DEK by name */
        if (column_item->dek_name != NULL && column_item->dek_name[0] != '\0')
        {
            dek_item = tecc_cache_get_dek_by_name_search_path(state, column_item->dek_name);
            if (dek_item != NULL && dek_item->key_name != NULL)
                return dek_item->key_name;
        }
    }

    /* No key found for encrypted column */
    tecc_processor_debug(TECC_LOG_ERROR,
                       "tecc_values_processor_get_column_key_with_state: "
                       "no key found for column '%s', table '%s'",
                       column_name ? column_name : "(null)",
                       table_name ? table_name : "(null)");
    return NULL;
    /* End - ReqID:TECC4.0 - 从缓存模块获取列加密密钥（带状态） */
}

/*
 * Get algorithm for column with statement data
 */
const char *
tecc_values_processor_get_column_algorithm_with_state(TeccStatementData *statement_data,
                                                     const char *schema_name,
                                                     const char *table_name,
                                                     const char *column_name,
                                                     const char *operator_name)
{
    /* Begin - ReqID:TECC4.0 - 从缓存模块获取列加密算法（带状态） */
    /* Variable declarations at function beginning for ISO C90 compliance */
    TeccColumnCacheItem *column_item = NULL;
    TeccState *state = NULL;

    if (column_name == NULL)
        return NULL;

    /* Get TeccState from statement data */
    if (statement_data != NULL && statement_data->conn != NULL)
    {
        state = tecc_get_state(statement_data->conn);
    }

    /* First, try to get column metadata from cache */
    if (table_name != NULL)
    {
        /* Get database name from connection */
        const char *database_name = NULL;
        if (statement_data != NULL && statement_data->conn != NULL) {
            database_name = PQdb(statement_data->conn);
            if (database_name == NULL || database_name[0] == '\0') {
                    /* 无法获取数据库名，返回NULL */
                tecc_processor_debug(TECC_LOG_WARNING,
                                   "Could not get database name from connection, returning error");
                return NULL;
            }
        } else {
                /* 无法获取数据库名，返回NULL */
                tecc_processor_debug(TECC_LOG_WARNING,
                                   "Could not get database name from connection, returning error");
                return NULL;
        }

        /* Use simplified cache interface with table name */
        column_item = tecc_cache_get_encrypted_column_by_name(
            state,  /* state - use actual state if available */
            database_name,  /* database_name - from connection */
            schema_name,  /* schema_name */
            table_name,
            column_name
        );
    }

    if (column_item != NULL)
    {
        /* Determine algorithm based on operator type and encryption type.
         * ReqID:TECC4.0 - Fix: Select algorithm based on operator, not on
         * bitmask order. For range operators (>,>=,<,<=) prefer OPE,
         * for equality (=,<>) prefer DET, for LIKE prefer SEARCH.
         * Only fallback to bitmask order if operator_name is NULL (e.g.,
         * when called from VALUES processing context). */
        if (operator_name != NULL)
        {
            /* Range operators require OPE (HG_ENCRYPTION_TYPE_OPE = 0x02) */
            if ((strcmp(operator_name, ">") == 0 ||
                 strcmp(operator_name, ">=") == 0 ||
                 strcmp(operator_name, "<") == 0 ||
                 strcmp(operator_name, "<=") == 0) &&
                (column_item->encryption_type & 0x02))
            {
                return "OPE";
            }

            /* LIKE operator requires SEARCH (HG_ENCRYPTION_TYPE_SEARCH = 0x04) */
            if ((strcmp(operator_name, "~~") == 0 ||
                 strcmp(operator_name, "!~~") == 0 ||
                 strcmp(operator_name, "LIKE") == 0 ||
                 strcmp(operator_name, "ILIKE") == 0 ||
                 strcmp(operator_name, "NOT LIKE") == 0 ||
                 strcmp(operator_name, "NOT ILIKE") == 0) &&
                (column_item->encryption_type & 0x04))
            {
                return "SEARCH";
            }

            /* Equality operators prefer DET (HG_ENCRYPTION_TYPE_DET = 0x01) */
            if ((strcmp(operator_name, "=") == 0 ||
                 strcmp(operator_name, "<>") == 0) &&
                (column_item->encryption_type & 0x01))
            {
                return "DET";
            }
        }

        /* Fallback: select algorithm by bitmask priority order */
        if (column_item->encryption_type & 0x02)
        {
            return "OPE";
        }
        else if (column_item->encryption_type & 0x01)
        {
            return "DET";
        }
        else if (column_item->encryption_type & 0x04)
        {
            return "SEARCH";
        }
        else
        {
            /* No algorithm configured — RND only */
            return "RND";
        }
    }

    /* Column not found in cache - cannot determine algorithm */
    tecc_processor_debug(TECC_LOG_ERROR,
                       "tecc_values_processor_get_column_algorithm_with_state: "
                       "column '%s' in table '%s' not found in encrypted columns cache",
                       column_name ? column_name : "(null)",
                       table_name ? table_name : "(null)");
    return NULL;
}
/* End - ReqID:TECC4.0 */
