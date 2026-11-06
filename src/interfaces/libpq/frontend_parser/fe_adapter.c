/* Begin - ReqID:TECC4.0 */
/*
 * File: fe_adapter.c
 * Abstract:
 *   Frontend adapter layer implementation for TECC simplified parser.
 *   Provides List* interface as required by the detailed design document.
 *
 * Authored by <wangxp@highgo.com>, 20260306.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/frontend_parser/fe_adapter.c
 */

#ifdef _MSC_VER
#define TECC_ATTR_UNUSED
#else
#define TECC_ATTR_UNUSED __attribute__((unused))
#endif

#include "fe_adapter.h"
#include "tecc_parser.h"
#include "tecc_semantic.h"
#include "tecc_error.h"
#include "feparser_memutils.h"
#include <string.h>
#include "port.h"  /* for pg_strncasecmp */

/* 包含处理器调试宏 */
#include "../tecc_processor/internal/tecc_processor_internal.h"

/* 函数声明在tecc_parser_simple.h中 */

/*
 * tecc_parser_parse - SQL语句解析接口
 *
 * 功能：解析SQL语句，生成List*格式的解析树
 * 兼容性要求：
 * 1. 与设计文档第5.1节要求的接口完全一致
 * 2. 内部调用tecc_parser_parse_list适配器函数
 * 3. 错误处理必须支持处理器模块的错误码定义
 *
 * @param query_string SQL语句字符串
 * @return 解析树列表，失败返回NULL
 */
List *tecc_parser_parse(const char *query_string) {
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    int is_empty = 1;
    const char *p;
    List *result;

    if (!query_string) {
        return NULL;
    }

    /* 检查是否为空字符串或只有空白字符 */
    for (p = query_string; *p; p++) {
        if (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') {
            is_empty = 0;
            break;
        }
    }

    if (is_empty) {
        return NULL;
    }

    /* 跳过空白字符 */
    while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) {
        p++;
    }

    /* 添加调试信息 */
    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "DEBUG: tecc_parser_parse: After skipping whitespace, p='%s' (first char='%c' ASCII %d)",
                        p, *p, *p);

    /* BEGIN/COMMIT/ROLLBACK/END transaction statements */
    if (pg_strncasecmp(p, "BEGIN", 5) == 0 ||
        pg_strncasecmp(p, "COMMIT", 6) == 0 ||
        pg_strncasecmp(p, "ROLLBACK", 8) == 0 ||
        pg_strncasecmp(p, "END", 3) == 0 ||
        pg_strncasecmp(p, "DO", 2) == 0) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "DEBUG: tecc_parser_parse: Transaction statement detected, bypassing parser: %s",
                            query_string);
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "DEBUG: tecc_parser_parse: First 10 chars after whitespace: '%.10s'",
                            p);
        /* 对于事务语句，返回NULL是正常的，不应该记录为错误 */
        return NULL;
    }


    /* 调用适配器函数获取List*格式的解析树 */
    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "DEBUG: tecc_parser_parse: Starting parse for query: %s",
                        query_string);
    result = tecc_parser_parse_list(query_string);
    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "DEBUG: tecc_parser_parse: tecc_parser_parse_list returned %p",
                        result);
    if (result == NULL) {
        /* 只有非事务语句解析失败时才记录为错误 */
        tecc_processor_debug(TECC_LOG_DEBUG1,
                         "DEBUG: tecc_parser_parse: Parser returned NULL for query: %s",
                         query_string);
    } else {
        /* 执行语义检查 */
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "DEBUG: tecc_parser_parse: Running semantic analysis");
        {
            int semantic_error_count = 0;
            int semantic_warning_count = 0;
            ParserError *semantic_errors = tecc_semantic_analyze(
                (const Node *)result, false,
                &semantic_error_count, &semantic_warning_count);

            /* 将语义错误追加到全局错误列表 */
            if (semantic_errors) {
                ParserError *error = semantic_errors;
                while (error) {
                    ParserError *next = error->next;
                    error->next = NULL;
                    tecc_error_append(error);
                    error = next;
                }
            }
        }
    }
    return result;
}

/*
 * tecc_parser_free_tree - 释放解析树
 *
 * 功能：释放解析树占用的所有内存
 * 兼容性要求：
 * 1. 内存释放方式必须与处理器模块的内存管理兼容
 * 2. 必须正确处理嵌套结构和循环引用
 * 3. 必须支持NULL指针的安全处理
 *
 * @param parse_tree 要释放的解析树
 */
void tecc_parser_free_tree(List *parse_tree) {
    tecc_parser_free_tree_list(parse_tree);
}

/* Value creation functions - 这些函数在tecc_nodes.c中实现，这里只做声明 */

/* 注意：这些函数已经在tecc_nodes.c中实现
 * makeIntConst, makeStringConst, makeRangeVar, makeAlias
 * 这里不需要重复实现
 */

/* 简化的Validation functions - 暂时注释掉，因为core_yyscan_t类型未定义 */

/*
void check_qualified_name(List *names, core_yyscan_t yyscanner) {
    // 简化实现：空函数
}

List *check_func_name(List *names, core_yyscan_t yyscanner) {
    // 简化实现：返回原始列表
    return names;
}

List *check_setting_name(List *names, core_yyscan_t yyscanner) {
    // 简化实现：返回原始列表
    return names;
}

List *check_indirection(List *indirection, core_yyscan_t yyscanner) {
    // 简化实现：返回原始列表
    return indirection;
}
*/

/* 简化的TECC expression processing functions */

TECC_ATTR_UNUSED static bool tecc_expr_expand_simple(const Node *expr_node, TeccExprPartsList **expr_parts) {
    /* 简化实现：返回false，实际使用时需要完整实现 */
    return false;
}

TECC_ATTR_UNUSED static bool tecc_expand_column_ref_simple(const struct ColumnRef *col_ref, TeccColumnRefData **col_data) {
    /* 简化实现：返回false，实际使用时需要完整实现 */
    return false;
}

TECC_ATTR_UNUSED static bool tecc_expand_function_name_simple(const struct FuncCall *func_call, TeccFuncNameData **func_data) {
    /* 简化实现：返回false，实际使用时需要完整实现 */
    return false;
}

TECC_ATTR_UNUSED static bool tecc_expand_condition_expr_simple(const Node *expr, TeccExprPartsList **expr_parts) {
    /* 简化实现：返回false，实际使用时需要完整实现 */
    return false;
}

/*
 * tecc_raw_parser - 原始解析器接口
 *
 * 这是libpq需要导出的函数，用于原始SQL解析
 * 返回List*格式的解析树
 */
List *tecc_raw_parser(const char *str) {
    /* 简化实现：调用现有的解析器接口 */
    if (!str) {
        return NULL;
    }

    /* 记录调试信息 */
    printf("[tecc_raw_parser] 解析查询: %s\n", str);

    /* 调用现有的解析器 */
    return tecc_parser_parse_list(str);
}

/* End - ReqID:TECC4.0 */