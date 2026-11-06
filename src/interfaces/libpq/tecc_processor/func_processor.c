#include "postgres_fe.h"
#include "common/fe_memutils.h"
/* Begin - ReqID:TECC4.0 */
/*
 * File: func_processor.c
 *
 * Abstract:
 *   TECC function processor implementation for analyzing function calls with encrypted parameters.
 *
 * Authored by <wangxp@highgo.com>, 20260410.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/func_processor.c
 */

#include "func_processor.h"
#include "internal/tecc_processor_internal.h"
#include "../tecc_scheduler/tecc_crypto_scheduler.h"  /* Crypto scheduler interface */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Include frontend parser types for Node and List definitions */
#include "../frontend_parser/tecc_nodes.h"

/* Internal helper functions */
static TeccFuncCallInfo *create_func_call_info(const Node *func_call_node);
static TeccFuncParamInfo *create_param_info(int param_index, Node *param_expr);
static bool is_encrypted_column_reference(Node *node);
static bool extract_column_reference(Node *node, char **table_name, char **column_name);

/* Encryption/decryption helper functions - REMOVED */
/* 根据最新要求，所有加密特定函数都不需要支持 */
static TeccFuncProcessorResult process_supported_aggregate_function(
    TeccFuncProcessorContext *context,
    TeccFuncParamInfo *param,
    TeccStatementData *statement_data);
static TeccFuncProcessorResult process_supported_scalar_function(
    TeccFuncProcessorContext *context,
    TeccFuncParamInfo *param,
    TeccStatementData *statement_data);
static bool is_encryption_specific_function(const char *function_name);
static bool is_supported_aggregate_function(const char *function_name);
static bool is_supported_scalar_function(const char *function_name);

/* ==================== 函数分类数组定义 ==================== */

/* 支持加密列的聚合函数（只涉及计数和大小比较） */
static const char *supported_aggregate_functions[] = {
    /* 计数函数 - 只计数，不涉及值内容 */
    "count",
    /* 比较函数 - 只需要大小比较（使用OPE加密） */
    "min", "max",
    /* 注意：布尔聚合函数（every, bool_and, bool_or）不支持加密列 */
    /* 注意：位聚合函数（bit_and, bit_or, bit_xor）不支持加密列 */
    NULL
};

/* 不支持加密列的聚合函数 */
static const char *unsupported_aggregate_functions[] = {
    /* 数学运算函数 */
    "sum", "avg",
    /* 统计函数 */
    "stddev", "stddev_pop", "stddev_samp",
    "variance", "var_pop", "var_samp",
    "corr", "covar_pop", "covar_samp",
    "regr_slope", "regr_intercept", "regr_count",
    "regr_r2", "regr_avgx", "regr_avgy",
    "regr_sxx", "regr_syy", "regr_sxy",
    /* 其他涉及值操作的函数 */
    "string_agg", "array_agg", "json_agg",
    "jsonb_agg", "xmlagg",
    /* 布尔聚合函数 - 不支持加密列 */
    "every", "bool_and", "bool_or",
    /* 位聚合函数 - 不支持加密列 */
    "bit_and", "bit_or", "bit_xor",
    NULL
};

/* 支持加密列的标量函数（只涉及比较的简单函数） */
static const char *supported_scalar_functions[] = {
    /* 条件函数 - 只涉及值比较 */
    "coalesce", "nullif", "greatest", "least",
    "case", "decode",
    /* 常量函数 - 不涉及参数值 */
    "current_date", "current_time", "current_timestamp",
    "localtime", "localtimestamp", "timeofday",
    "now", "today",
    "pi",
    NULL
};

/* 不支持加密列的标量函数（需要解密或涉及运算） */
static const char *unsupported_scalar_functions[] = {
    /* 字符串函数 - 需要解密后才能操作 */
    "upper", "lower", "trim", "ltrim", "rtrim", "substring",
    "length", "char_length", "octet_length", "position",
    "concat", "concat_ws", "replace", "reverse",
    "left", "right", "repeat", "space",
    "ascii", "chr", "initcap", "format",
    "quote_ident", "quote_literal", "quote_nullable",
    "md5", "sha256", "encode", "decode",
    "translate", "overlay", "pad", "lpad", "rpad",
    "split_part", "strpos", "substr",
    /* 日期函数 - 需要解密或涉及计算 */
    "extract", "date_part", "date_trunc", "age",
    "make_date", "make_time", "make_timestamp",
    "make_timestamptz", "make_interval",
    "to_date", "to_timestamp", "to_char",
    "to_number", "to_hex", "to_ascii",
    "date", "time", "timestamp",
    "timestamptz", "interval",
    "justify_days", "justify_hours", "justify_interval",
    /* 数学函数 - 涉及数学运算 */
    "abs", "ceil", "floor", "round", "trunc",
    "sqrt", "power", "exp", "ln", "log",
    "log10", "mod", "remainder", "div",
    "sin", "cos", "tan", "asin", "acos", "atan",
    "sinh", "cosh", "tanh", "asinh", "acosh", "atanh",
    "degrees", "radians", "random",
    "cbrt", "factorial", "gcd", "lcm",
    "sign", "width_bucket",
    /* 类型转换函数 - 需要解密后才能操作 */
    "cast", "to_char", "to_number", "to_date", "to_timestamp",
    "to_hex", "to_ascii", "to_json", "to_jsonb",
    "to_xml", "to_tsvector", "to_tsquery",
    /* 数组函数 - 需要解密后才能操作 */
    "array_length", "array_upper", "array_lower",
    "array_append", "array_prepend", "array_cat",
    "array_remove", "array_replace", "array_position",
    "array_positions", "array_dims", "array_fill",
    "unnest", "array_agg",
    /* JSON函数 - 需要解密后才能操作 */
    "json_build_object", "json_build_array",
    "json_object", "json_array",
    "json_extract_path", "json_extract_path_text",
    "json_strip_nulls", "json_typeof",
    "jsonb_build_object", "jsonb_build_array",
    "jsonb_object", "jsonb_array",
    "jsonb_extract_path", "jsonb_extract_path_text",
    "jsonb_strip_nulls", "jsonb_typeof",
    /* 网络地址函数 - 需要解密后才能操作 */
    "inet", "cidr", "macaddr",
    "host", "masklen", "network",
    "set_masklen", "broadcast", "hostmask", "netmask",
    /* 几何函数 - 涉及数学计算 */
    "area", "center", "diameter", "height", "width",
    "distance", "length", "perimeter",
    /* 全文搜索函数 - 需要解密后才能操作 */
    "tsvector", "tsquery", "plainto_tsquery",
    "phraseto_tsquery", "websearch_to_tsquery",
    "ts_headline", "ts_rank", "ts_rank_cd",
    /* 其他涉及数学运算的函数 */
    "isfinite", "isinf", "isnan",
    NULL
};

/* 加密特定函数列表 */
/* 注意：根据最新要求，所有加密特定函数都不需要支持 */
/* 这些函数包括tecc_encrypt、tecc_decrypt以及各种算法特定函数 */
/* 用户应该使用普通SQL语句，加密/解密由TECC系统自动处理 */
static const char *encryption_functions[] = {
    /* 根据最新要求，所有加密特定函数都不需要支持 */
    NULL
};

/* Initialize function processor context */
TeccFuncProcessorContext *
tecc_func_processor_init(PGconn *conn)
{
    TeccFuncProcessorContext *context;

    if (conn == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid connection parameter to function processor init");
        return NULL;
    }

    /* Allocate context */
    context = (TeccFuncProcessorContext *)pg_malloc(sizeof(TeccFuncProcessorContext));
    if (context == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to allocate function processor context");
        return NULL;
    }

    /* Initialize context */
    memset(context, 0, sizeof(TeccFuncProcessorContext));
    context->conn = conn;
    context->func_call = NULL;
    context->params = NULL;
    context->num_params = 0;
    context->num_encrypted_params = 0;
    context->error_code = TECC_FUNC_PROCESSOR_SUCCESS;
    context->error_msg[0] = '\0';

    

    return context;
}

/* Process a function call */
TeccFuncProcessorResult
tecc_func_processor_process_call(
    TeccFuncProcessorContext *context,
    const Node *func_call_node,
    TeccStatementData *statement_data)
{
    TeccFuncProcessorResult result;
    bool function_supported;

    if (context == NULL || func_call_node == NULL || statement_data == NULL) {
        return TECC_FUNC_PROCESSOR_INVALID_INPUT;
    }

    /* Create function call information */
    context->func_call = create_func_call_info(func_call_node);
    if (context->func_call == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to create function call info");
        context->error_code = TECC_FUNC_PROCESSOR_ERROR;
        return TECC_FUNC_PROCESSOR_ERROR;
    }

    /* Check if function supports encrypted parameters */
    function_supported = tecc_func_processor_check_support(
        context, context->func_call->function_name,
        context->func_call->schema_name);

    if (!function_supported) {
        tecc_processor_debug(TECC_LOG_WARNING,
                           "Function %s does not support encrypted parameters",
                           context->func_call->function_name);
        context->func_call->supports_encryption = false;
        return TECC_FUNC_PROCESSOR_UNSUPPORTED_FUNCTION;
    }

    context->func_call->supports_encryption = true;

    /* Analyze function arguments */
    result = tecc_func_processor_analyze_args(context,
                                             context->func_call->args,
                                             statement_data);
    if (result != TECC_FUNC_PROCESSOR_SUCCESS) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to analyze function arguments: error %d",
                           result);
        context->error_code = result;
        return result;
    }

    /* If no encrypted parameters, no further processing needed */
    if (context->num_encrypted_params == 0) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Function %s has no encrypted parameters",
                           context->func_call->function_name);
        return TECC_FUNC_PROCESSOR_SUCCESS;
    }

    /* Process function parameters (encryption/decryption) */
    result = tecc_func_processor_process_params(context, statement_data);
    if (result != TECC_FUNC_PROCESSOR_SUCCESS) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to process function parameters: error %d",
                           result);
        context->error_code = result;
        return result;
    }

    

    return TECC_FUNC_PROCESSOR_SUCCESS;
}

/* Check if a function supports encrypted parameters */
bool
tecc_func_processor_check_support(
    TeccFuncProcessorContext *context,
    const char *function_name,
    const char *schema_name)
{
    /* Begin - ReqID:TECC4.0 */
    /* Implement function support check */
    /* TECC project function support strategy:
     * 1. Aggregate functions that support encrypted columns: only involve comparison, no mathematical operations (count, min, max, etc.)
     * 2. Scalar functions that support encrypted columns: only involve comparison or constant functions (coalesce, greatest, current_date, etc.)
     * 3. Conditionally supported scalar functions: require decryption before operation (string functions, type conversion functions, etc.)
     * 4. Unsupported functions: involve mathematical operations (sum, avg, abs, sqrt, etc.)
     * Note: According to the latest requirements, all encryption-specific functions (such as tecc_encrypt, tecc_decrypt, etc.) do not need to be supported
     *
     * Check order:
     * 1. Encryption-specific functions
     * 2. Aggregate functions that support encrypted columns
     * 3. Scalar functions that support encrypted columns
     * 4. Conditionally supported scalar functions
     * 5. Unsupported aggregate functions
     * 6. Unsupported scalar functions
     * 7. User-defined functions
     */


    int i;
    char *func_name_only = NULL;
    char *schema_name_only = NULL;

    if (function_name == NULL) {
        return false;
    }

    /* 处理可能的模式名.函数名格式 */
    func_name_only = tecc_processor_strdup(function_name);
    if (func_name_only == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to allocate memory for function name");
        return false;
    }

    /* 如果提供了schema_name参数，使用它；否则尝试从function_name中提取 */
    if (schema_name != NULL && schema_name[0] != '\0') {
        schema_name_only = tecc_processor_strdup(schema_name);
    } else {
        /* 检查function_name是否包含模式名（如"public.func_name"） */
        char *dot_pos = strchr(func_name_only, '.');
        if (dot_pos != NULL) {
            char *old_func_name = func_name_only;
            *dot_pos = '\0';  /* 分割字符串 */
            schema_name_only = tecc_processor_strdup(func_name_only);
            func_name_only = tecc_processor_strdup(dot_pos + 1);
            pg_free(old_func_name);  /* 释放原来的内存 */
            if (schema_name_only == NULL || func_name_only == NULL) {
                tecc_processor_debug(TECC_LOG_ERROR,
                                   "Failed to allocate memory for schema/function name");
                if (schema_name_only != NULL) pg_free(schema_name_only);
                if (func_name_only != NULL) pg_free(func_name_only);
                return false;
            }
        }
    }

    /* 1. 检查是否为加密特定函数（可以处理加密数据） */
    for (i = 0; encryption_functions[i] != NULL; i++) {
        if (strcmp(func_name_only, encryption_functions[i]) == 0) {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Encryption-specific function %s%s%s is supported (can process encrypted data)",
                               schema_name_only ? schema_name_only : "",
                               schema_name_only ? "." : "",
                               func_name_only);
            if (schema_name_only != NULL) pg_free(schema_name_only);
            pg_free(func_name_only);
            return true;
        }
    }

    /* 2. 检查是否支持加密列的聚合函数（只涉及比较） */
    for (i = 0; supported_aggregate_functions[i] != NULL; i++) {
        if (strcmp(func_name_only, supported_aggregate_functions[i]) == 0) {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Aggregate function %s%s%s supports encrypted columns (comparison only)",
                               schema_name_only ? schema_name_only : "",
                               schema_name_only ? "." : "",
                               func_name_only);
            if (schema_name_only != NULL) pg_free(schema_name_only);
            pg_free(func_name_only);
            return true;
        }
    }

    /* 3. 检查是否支持加密列的标量函数（只涉及比较） */
    for (i = 0; supported_scalar_functions[i] != NULL; i++) {
        if (strcmp(func_name_only, supported_scalar_functions[i]) == 0) {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Scalar function %s%s%s supports encrypted columns (comparison only)",
                               schema_name_only ? schema_name_only : "",
                               schema_name_only ? "." : "",
                               func_name_only);
            if (schema_name_only != NULL) pg_free(schema_name_only);
            pg_free(func_name_only);
            return true;
        }
    }

    /* 4. 检查是否为有条件支持的标量函数（需要解密后操作） */
    /* 根据最新要求，所有需要解密的函数都不支持 */
    /* 此步骤已移除，直接进入下一步检查 */

    /* 5. 检查是否为不支持的聚合函数（涉及数学运算） */
    for (i = 0; unsupported_aggregate_functions[i] != NULL; i++) {
        if (strcmp(func_name_only, unsupported_aggregate_functions[i]) == 0) {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Aggregate function %s%s%s does not support encrypted columns (involves mathematical operations)",
                               schema_name_only ? schema_name_only : "",
                               schema_name_only ? "." : "",
                               func_name_only);
            if (schema_name_only != NULL) pg_free(schema_name_only);
            pg_free(func_name_only);
            return false;  /* 明确返回false，表示不支持 */
        }
    }

    /* 6. 检查是否为不支持的标量函数（涉及数学运算） */
    for (i = 0; unsupported_scalar_functions[i] != NULL; i++) {
        if (strcmp(func_name_only, unsupported_scalar_functions[i]) == 0) {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Scalar function %s%s%s does not support encrypted columns (involves mathematical operations)",
                               schema_name_only ? schema_name_only : "",
                               schema_name_only ? "." : "",
                               func_name_only);
            if (schema_name_only != NULL) pg_free(schema_name_only);
            pg_free(func_name_only);
            return false;  /* 明确返回false，表示不支持 */
        }
    }

    /* 检查用户自定义函数是否支持加密 */
    /* 注意：TECC项目不支持对加密字段进行函数运算 */
    /* 判断逻辑：如果函数的参数是加密列，就报错不支持 */
    /* 这个检查在函数参数分析阶段进行，这里只记录信息 */

    if (context != NULL) {
        /* 如果有上下文，可以记录更详细的信息 */
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Checking user-defined function %s%s%s - TECC does not support functions on encrypted columns",
                           schema_name_only ? schema_name_only : "",
                           schema_name_only ? "." : "",
                           func_name_only);
    }

    /* TECC项目设计原则：不支持对加密字段进行函数运算 */
    /* 如果函数参数包含加密列，将在参数分析阶段报错 */
    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Function %s%s%s - TECC does not support functions on encrypted columns",
                       schema_name_only ? schema_name_only : "",
                       schema_name_only ? "." : "",
                       func_name_only);

    /* 清理内存 */
    if (schema_name_only != NULL) pg_free(schema_name_only);
    pg_free(func_name_only);

    return false;
    /* End - ReqID:TECC4.0 */
}

/* Analyze function arguments for encryption requirements */
TeccFuncProcessorResult
tecc_func_processor_analyze_args(
    TeccFuncProcessorContext *context,
    List *args,
    TeccStatementData *statement_data)
{
    int param_index = 0;
    int num_args;

    if (context == NULL || args == NULL) {
        return TECC_FUNC_PROCESSOR_INVALID_INPUT;
    }

    /* Count number of arguments */
    num_args = tecc_processor_list_length(args);
    if (num_args <= 0) {
        context->num_params = 0;
        context->num_encrypted_params = 0;
        return TECC_FUNC_PROCESSOR_SUCCESS;
    }

    /* Allocate parameter info array */
    context->params = (TeccFuncParamInfo **)pg_malloc(
        sizeof(TeccFuncParamInfo *) * num_args);
    if (context->params == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to allocate parameter info array");
        context->error_code = TECC_FUNC_PROCESSOR_MEMORY_ERROR;
        return TECC_FUNC_PROCESSOR_MEMORY_ERROR;
    }

    /* Initialize array to NULL */
    for (param_index = 0; param_index < num_args; param_index++) {
        context->params[param_index] = NULL;
    }

    /* Analyze each argument */
    param_index = 0;
    context->num_encrypted_params = 0;

    /* Manual list traversal since foreach macro is not available in tecc_nodes.h */
    if (args != NULL) {
        ListCell *lc;
        for (lc = args->head; lc != NULL; lc = lc->next) {
            Node *arg = (Node *)lc->data.ptr_value;
            TeccFuncParamInfo *param_info;

            param_info = create_param_info(param_index, arg);
            if (param_info == NULL) {
                tecc_processor_debug(TECC_LOG_ERROR,
                                   "Failed to create parameter info for argument %d",
                                   param_index);
                context->error_code = TECC_FUNC_PROCESSOR_MEMORY_ERROR;
                return TECC_FUNC_PROCESSOR_MEMORY_ERROR;
            }

            /* Check if argument involves encrypted column */
            param_info->is_encrypted = is_encrypted_column_reference(arg);
            if (param_info->is_encrypted) {
                context->num_encrypted_params++;

                /* Extract column reference information */
                if (extract_column_reference(arg, &param_info->table_name,
                                            &param_info->column_name)) {
                    /* Check if column needs encryption/decryption */
                    /* TODO: Query metadata cache for actual encryption requirements */
                    param_info->needs_encryption = true;
                    param_info->needs_decryption = true;
                }
            }

            context->params[param_index] = param_info;
            param_index++;
        }
    }

    context->num_params = num_args;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Analyzed %d arguments, %d involve encrypted columns",
                       num_args, context->num_encrypted_params);

    return TECC_FUNC_PROCESSOR_SUCCESS;
}

/* Process function parameters (encryption/decryption) */
TeccFuncProcessorResult
tecc_func_processor_process_params(
    TeccFuncProcessorContext *context,
    TeccStatementData *statement_data)
{
    int i;
    TeccFuncProcessorResult result = TECC_FUNC_PROCESSOR_SUCCESS;
    const char *function_name;

    if (context == NULL || statement_data == NULL) {
        return TECC_FUNC_PROCESSOR_INVALID_INPUT;
    }

    if (context->num_encrypted_params == 0) {
        return TECC_FUNC_PROCESSOR_SUCCESS;
    }

    if (context->func_call == NULL || context->func_call->function_name == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Function call information is missing");
        return TECC_FUNC_PROCESSOR_INVALID_INPUT;
    }

    function_name = context->func_call->function_name;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Processing %d encrypted parameters for function %s",
                       context->num_encrypted_params, function_name);

    /* Begin - ReqID:TECC4.0 */
    /* 根据函数类型进行不同的处理 */

    /* 1. 加密特定函数处理 */
    if (is_encryption_specific_function(function_name)) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Processing encryption-specific function: %s",
                           function_name);

        for (i = 0; i < context->num_params; i++) {
            TeccFuncParamInfo *param = context->params[i];

            if (param == NULL || !param->is_encrypted) {
                continue;
            }

            /* 根据函数类型调用相应的处理函数 */
            /* 注意：根据最新要求，所有加密特定函数都不需要支持 */
            /* 这些函数包括tecc_encrypt、tecc_decrypt以及各种算法特定函数 */
            /* 用户应该使用普通SQL语句，加密/解密由TECC系统自动处理 */

            if (result != TECC_FUNC_PROCESSOR_SUCCESS) {
                return result;
            }
        }
    }
    /* 2. 支持的聚合函数处理 */
    else if (is_supported_aggregate_function(function_name)) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Processing supported aggregate function: %s",
                           function_name);

        for (i = 0; i < context->num_params; i++) {
            TeccFuncParamInfo *param = context->params[i];

            if (param == NULL || !param->is_encrypted) {
                continue;
            }

            result = process_supported_aggregate_function(context, param, statement_data);
            if (result != TECC_FUNC_PROCESSOR_SUCCESS) {
                return result;
            }
        }
    }
    /* 3. 支持的标量函数处理 */
    else if (is_supported_scalar_function(function_name)) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Processing supported scalar function: %s",
                           function_name);

        for (i = 0; i < context->num_params; i++) {
            TeccFuncParamInfo *param = context->params[i];

            if (param == NULL || !param->is_encrypted) {
                continue;
            }

            result = process_supported_scalar_function(context, param, statement_data);
            if (result != TECC_FUNC_PROCESSOR_SUCCESS) {
                return result;
            }
        }
    }
    /* 4. 不支持的函数 - 根据之前的检查，这里不应该有加密参数 */
    else {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Function %s does not support encrypted parameters, but found %d encrypted parameters",
                           function_name, context->num_encrypted_params);

        /* 设置错误信息 */
        if (context->error_msg[0] == '\0') {
            snprintf(context->error_msg, sizeof(context->error_msg),
                    "Function %s does not support operations on encrypted columns",
                    function_name);
        }
        context->error_code = TECC_FUNC_PROCESSOR_UNSUPPORTED_FUNCTION;
        return TECC_FUNC_PROCESSOR_UNSUPPORTED_FUNCTION;
    }
    /* End - ReqID:TECC4.0 */

    return result;
}


/* Clean up function processor context */
void
tecc_func_processor_cleanup(TeccFuncProcessorContext *context)
{
    int i;

    if (context == NULL) {
        return;
    }

    /* Free function call info */
    if (context->func_call != NULL) {
        if (context->func_call->function_name != NULL) {
            pg_free(context->func_call->function_name);
        }
        if (context->func_call->schema_name != NULL) {
            pg_free(context->func_call->schema_name);
        }
        if (context->func_call->catalog_name != NULL) {
            pg_free(context->func_call->catalog_name);
        }
        /* Note: args list should be freed by caller */
        pg_free(context->func_call);
    }

    /* Free parameter info array */
    if (context->params != NULL) {
        for (i = 0; i < context->num_params; i++) {
            if (context->params[i] != NULL) {
                if (context->params[i]->table_name != NULL) {
                    pg_free(context->params[i]->table_name);
                }
                if (context->params[i]->column_name != NULL) {
                    pg_free(context->params[i]->column_name);
                }
                pg_free(context->params[i]);
            }
        }
        pg_free(context->params);
    }

    /* Free context */
    pg_free(context);

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Function processor context cleaned up");
}

/* Unified function processing interface for external modules */
int
tecc_func_processor_handle_function_call(
    PGconn *conn,
    const Node *func_call_node,
    TeccStatementData *statement_data)
{
    TeccFuncProcessorContext *context;
    TeccFuncProcessorResult result;

    if (conn == NULL || func_call_node == NULL || statement_data == NULL) {
        return TECC_FUNC_PROCESSOR_INVALID_INPUT;
    }

    /* Initialize context */
    context = tecc_func_processor_init(conn);
    if (context == NULL) {
        return TECC_FUNC_PROCESSOR_ERROR;
    }

    /* Process function call */
    result = tecc_func_processor_process_call(context, func_call_node, statement_data);

    /* Clean up context */
    tecc_func_processor_cleanup(context);

    return result;
}

/* CREATE FUNCTION/PROCEDURE statement processing */
bool
tecc_func_processor_run_pre_create_function_stmt(
    const List *options,
    TeccStatementData *statement_data,
    bool is_do_stmt)
{
    bool result;

    if (options == NULL || statement_data == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid input parameters to CREATE FUNCTION processor");
        return false;
    }

    /* Process hardcoded values in function body */
    result = tecc_func_hardcoded_values_process(options, statement_data, is_do_stmt);

    if (result) {
        
    } else {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to process CREATE FUNCTION/PROCEDURE statement");
    }

    return result;
}

/* Function expression processing */
bool
tecc_func_processor_process(
    const TeccExprPartsList *target_expr_vec,
    TeccStatementData *statement_data)
{
    size_t i;
    size_t num_expr_parts;

    if (target_expr_vec == NULL || statement_data == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid input parameters to function expression processor");
        return false;
    }

    num_expr_parts = tecc_expr_parts_list_size(target_expr_vec);
    if (num_expr_parts == 0) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Empty expression parts list, nothing to process");
        return true;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Processing %zu expression parts", num_expr_parts);

    /* Process each expression part */
    for (i = 0; i < num_expr_parts; i++) {
        TeccExprPart *part = tecc_expr_parts_list_get(target_expr_vec, i);

        if (part == NULL) {
            continue;
        }

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Processing expression part %zu: type %d", i, part->type);

        /* 简化处理：只记录类型 */
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "  Expression part type: %d", part->type);
    }

    

    return true;
}

/* Internal helper functions */

/* Create function call information from parse tree node */
static TeccFuncCallInfo *
create_func_call_info(const Node *func_call_node)
{
    TeccFuncCallInfo *func_call;
    const FuncCall *fc_node = NULL;

    if (func_call_node == NULL || nodeTag(func_call_node) != FE_T_FuncCall) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid function call node");
        return NULL;
    }

    fc_node = (const FuncCall *)func_call_node;

    func_call = (TeccFuncCallInfo *)pg_malloc(sizeof(TeccFuncCallInfo));
    if (func_call == NULL) {
        return NULL;
    }

    memset(func_call, 0, sizeof(TeccFuncCallInfo));

    /* Extract function name from funcname list */
    if (fc_node->funcname != NIL && list_length(fc_node->funcname) > 0) {
        /* 函数名通常是最后一个元素（可能包含模式名） */
        char *last_name = (char *)list_nth(fc_node->funcname, list_length(fc_node->funcname) - 1);
        if (last_name != NULL) {
            func_call->function_name = pg_strdup(last_name);
        } else {
            func_call->function_name = pg_strdup("unknown_function");
        }

        /* 如果有模式名，提取它（通常是倒数第二个元素） */
        if (list_length(fc_node->funcname) > 1) {
            char *schema_name = (char *)list_nth(fc_node->funcname, list_length(fc_node->funcname) - 2);
            if (schema_name != NULL) {
                func_call->schema_name = pg_strdup(schema_name);
            }
        }
    } else {
        func_call->function_name = pg_strdup("unknown_function");
    }

    /* Extract arguments */
    func_call->args = fc_node->args;  /* 注意：这是原始节点列表，不是复制的 */
    func_call->location = fc_node->location;
    func_call->is_aggregate = (fc_node->agg_order != NIL || fc_node->agg_filter != NULL);
    func_call->is_window = (fc_node->over != NULL);
    func_call->is_variadic = fc_node->func_variadic;

    /* TODO: Determine actual return type - 需要查询系统表或类型推断 */
    func_call->return_type = 0;

    /* TODO: Check if function supports encryption - 需要查询函数元数据 */
    func_call->supports_encryption = false;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Extracted function call: %s (schema: %s, args: %d, location: %d)",
                       func_call->function_name,
                       func_call->schema_name ? func_call->schema_name : "NULL",
                       list_length(fc_node->args),
                       fc_node->location);

    return func_call;
}

/* Create parameter information */
static TeccFuncParamInfo *
create_param_info(int param_index, Node *param_expr)
{
    TeccFuncParamInfo *param_info;

    param_info = (TeccFuncParamInfo *)pg_malloc(sizeof(TeccFuncParamInfo));
    if (param_info == NULL) {
        return NULL;
    }

    memset(param_info, 0, sizeof(TeccFuncParamInfo));
    param_info->param_index = param_index;
    param_info->param_expr = param_expr;

    /* 检查参数表达式类型并提取信息 */
    if (param_expr != NULL) {
        NodeTag expr_type = nodeTag(param_expr);

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Creating param info for index %d, expr type: %d",
                           param_index, expr_type);

        switch (expr_type) {
            case FE_T_ColumnRef:
                /* 列引用 - 可能是加密列 */
                param_info->is_encrypted = is_encrypted_column_reference(param_expr);
                if (param_info->is_encrypted) {
                    /* 提取表名和列名 */
                    extract_column_reference(param_expr,
                                            &param_info->table_name,
                                            &param_info->column_name);
                    /* 加密列在函数参数中可能需要解密 */
                    param_info->needs_decryption = true;
                }
                break;

            case FE_T_Const:
                /* 常量表达式 - 如果传递给加密列参数，可能需要加密 */
                param_info->needs_encryption = true;
                break;

            case FE_T_ParamRef:
                /* 参数引用 - 需要运行时确定 */
                break;

            case FE_T_FuncCall:
                /* 函数调用 - 递归处理 */
                break;

            case FE_T_TypeCast:
                /* 类型转换 - 需要检查内部表达式 */
                break;

            default:
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Unhandled parameter expression type: %d", expr_type);
                break;
        }
    }

    /* TODO: Determine actual parameter type - 需要类型推断 */
    param_info->param_type = 0;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Created param info: index=%d, encrypted=%d, table=%s, column=%s",
                       param_info->param_index,
                       param_info->is_encrypted,
                       param_info->table_name ? param_info->table_name : "NULL",
                       param_info->column_name ? param_info->column_name : "NULL");

    return param_info;
}

/* Check if a node is an encrypted column reference */
static bool
is_encrypted_column_reference(Node *node)
{
    ColumnRef *col_ref = NULL;
    char *column_name = NULL;
    bool is_encrypted = false;

    if (node == NULL || nodeTag(node) != FE_T_ColumnRef) {
        return false;
    }

    col_ref = (ColumnRef *)node;

    /* 提取列名（fields列表的最后一个元素） */
    if (col_ref->fields != NIL && list_length(col_ref->fields) > 0) {
        char *last_field = (char *)list_nth(col_ref->fields, list_length(col_ref->fields) - 1);
        if (last_field != NULL) {
            column_name = last_field;

            /* TODO: 实际检查列是否加密 - 需要查询元数据缓存 */
            /* 暂时使用简单的启发式规则：列名以"_enc"结尾的认为是加密列 */
            if (strlen(column_name) > 4 &&
                strcmp(column_name + strlen(column_name) - 4, "_enc") == 0) {
                is_encrypted = true;
            }
        }
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Column reference %s is encrypted: %d",
                       column_name ? column_name : "unknown",
                       is_encrypted);

    return is_encrypted;
}

/* Extract column reference information from node */
static bool
extract_column_reference(Node *node, char **table_name, char **column_name)
{
    ColumnRef *col_ref = NULL;
    int num_fields;

    if (node == NULL || nodeTag(node) != FE_T_ColumnRef ||
        table_name == NULL || column_name == NULL) {
        return false;
    }

    col_ref = (ColumnRef *)node;
    num_fields = list_length(col_ref->fields);

    if (num_fields == 0) {
        return false;
    }

    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    {
    /* 提取列名（最后一个字段） */
    char *last_field = (char *)list_nth(col_ref->fields, num_fields - 1);
    if (last_field != NULL) {
        *column_name = pg_strdup(last_field);
    } else {
        *column_name = NULL;
        return false;
    }

    /* 提取表名（如果有的话，通常是倒数第二个字段） */
    if (num_fields >= 2) {
        /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
        {
        char *second_last_field = (char *)list_nth(col_ref->fields, num_fields - 2);
        if (second_last_field != NULL) {
            *table_name = pg_strdup(second_last_field);
        } else {
            *table_name = NULL;
        }
        }  /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    } else {
        *table_name = NULL;
    }
    }  /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Extracted column reference: table=%s, column=%s",
                       *table_name ? *table_name : "NULL",
                       *column_name ? *column_name : "NULL");

    return true;
}

/* Get function call information */
TeccFuncCallInfo *
tecc_func_processor_get_call_info(TeccFuncProcessorContext *context)
{
    if (context == NULL) {
        return NULL;
    }
    return context->func_call;
}

/* Get parameter information */
TeccFuncParamInfo *
tecc_func_processor_get_param_info(
    TeccFuncProcessorContext *context,
    int param_index)
{
    if (context == NULL || param_index < 0 || param_index >= context->num_params) {
        return NULL;
    }
    return context->params[param_index];
}

/* ==================== 辅助函数实现 ==================== */

/* Check if function is encryption-specific */
static bool
is_encryption_specific_function(const char *function_name)
{
    int i;

    if (function_name == NULL) {
        return false;
    }

    for (i = 0; encryption_functions[i] != NULL; i++) {
        if (strcmp(function_name, encryption_functions[i]) == 0) {
            return true;
        }
    }

    return false;
}

/* Check if function is supported aggregate function */
static bool
is_supported_aggregate_function(const char *function_name)
{
    int i;

    if (function_name == NULL) {
        return false;
    }

    for (i = 0; supported_aggregate_functions[i] != NULL; i++) {
        if (strcmp(function_name, supported_aggregate_functions[i]) == 0) {
            return true;
        }
    }

    return false;
}

/* Check if function is supported scalar function */
static bool
is_supported_scalar_function(const char *function_name)
{
    int i;

    if (function_name == NULL) {
        return false;
    }

    for (i = 0; supported_scalar_functions[i] != NULL; i++) {
        if (strcmp(function_name, supported_scalar_functions[i]) == 0) {
            return true;
        }
    }

    return false;
}

/* Process encryption function - REMOVED */
/* 根据最新要求，所有加密特定函数都不需要支持 */
/* 用户应该使用普通SQL语句，加密/解密由TECC系统自动处理 */

/* Process decryption function - REMOVED */
/* 根据最新要求，所有加密特定函数都不需要支持 */
/* 用户应该使用普通SQL语句，加密/解密由TECC系统自动处理 */

/* Process supported aggregate function */
static TeccFuncProcessorResult
process_supported_aggregate_function(
    TeccFuncProcessorContext *context,
    TeccFuncParamInfo *param,
    TeccStatementData *statement_data)
{
    /* Begin - ReqID:TECC4.0 */
    const char *function_name = context->func_call->function_name;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Processing supported aggregate function %s for parameter %d: %s.%s",
                       function_name,
                       param->param_index,
                       param->table_name ? param->table_name : "unknown",
                       param->column_name ? param->column_name : "unknown");

    /* 支持的聚合函数（count, min, max）可以在加密数据上操作
     * 因为它们只涉及计数或比较，不需要解密数据 */

    if (strcmp(function_name, "count") == 0) {
        /* count函数只需要计数，不需要解密 */
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "COUNT function on encrypted column - no decryption needed");
    } else if (strcmp(function_name, "min") == 0 || strcmp(function_name, "max") == 0) {
        /* min/max函数可以在加密数据上比较大小
         * 但需要确保使用支持比较的加密算法（如OPE） */
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "MIN/MAX function on encrypted column - comparison on encrypted data");

        /* 在实际实现中，这里会检查列是否支持比较算法
         * 如果不支持，应该报错 */
    }

    return TECC_FUNC_PROCESSOR_SUCCESS;
    /* End - ReqID:TECC4.0 */
}

/* Process supported scalar function */
static TeccFuncProcessorResult
process_supported_scalar_function(
    TeccFuncProcessorContext *context,
    TeccFuncParamInfo *param,
    TeccStatementData *statement_data)
{
    /* Begin - ReqID:TECC4.0 */
    const char *function_name = context->func_call->function_name;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Processing supported scalar function %s for parameter %d: %s.%s",
                       function_name,
                       param->param_index,
                       param->table_name ? param->table_name : "unknown",
                       param->column_name ? param->column_name : "unknown");

    /* 支持的标量函数（coalesce, nullif, greatest, least, case, decode等）
     * 可以在加密数据上操作，因为它们只涉及比较或选择，不需要解密数据 */

    if (strcmp(function_name, "coalesce") == 0 ||
        strcmp(function_name, "nullif") == 0 ||
        strcmp(function_name, "greatest") == 0 ||
        strcmp(function_name, "least") == 0) {
        /* 这些函数涉及比较，可以在加密数据上操作 */
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Comparison function on encrypted column - no decryption needed");
    } else if (strcmp(function_name, "case") == 0 ||
               strcmp(function_name, "decode") == 0) {
        /* 条件函数，可以在加密数据上操作 */
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Conditional function on encrypted column - no decryption needed");
    } else {
        /* 常量函数（current_date, current_time等） */
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Constant function - no column data involved");
    }

    return TECC_FUNC_PROCESSOR_SUCCESS;
    /* End - ReqID:TECC4.0 */
}

/* End - ReqID:TECC4.0 */