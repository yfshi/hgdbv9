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
#include <stdlib.h>
#include "port.h"  /* for pg_strncasecmp */

/* 包含处理器调试宏 */
#include "../tecc_processor/internal/tecc_processor_internal.h"

/* 函数声明在tecc_parser_simple.h中 */

/*
 * ReqID:TECC4.0 - 前端语法未建模语句的掩码支持
 *
 * 前端语法只建模了 SELECT/INSERT/UPDATE/DELETE/WITH/COPY/EXPLAIN/PREPARE/
 * EXECUTE/DEALLOCATE/DECLARE/SET 以及 CREATE|ALTER|DROP 的 KEK/DEK/TABLE/
 * VIEW/MATVIEW 等语句。批量语句（单次 PQexec 多条 SQL）中出现其它语句
 * （BEGIN/COMMIT/SHOW/TRUNCATE/COMMENT/ANALYZE/DROP VIEW/CREATE INDEX ...）
 * 会让整批解析失败、整批按原文放行，批内密态常量因此以明文发往服务端
 * （服务端报 invalid hex string）。
 *
 * 处理方式：整批解析失败时逐条试解析，把无法解析的语句在“解析器视野内”
 * 整条掩码为等长空格（含结尾分号），长度不变，因此解析树中的字节偏移仍与
 * 原始 SQL 一致；真正发往服务端的 SQL 仍是原始文本，被掩码语句原样保留。
 */

/* 语句区间：end 指向语句结尾分号之后（无分号则为文本结尾） */
typedef struct TeccStmtSpan
{
    const char *start;
    const char *end;
} TeccStmtSpan;

/* 取语句的第 n 个词（1-based）写入 out，返回是否取到 */
static bool
tecc_parser_span_word(const char *stmt, const char *limit, int nth,
                      char *out, size_t out_size)
{
    const char *p = stmt;
    size_t      len;
    int         index = 0;

    while (index < nth)
    {
        while (p < limit && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
            p++;

        len = 0;
        while (p + len < limit && len + 1 < out_size)
        {
            char c = p[len];

            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') || c == '_'))
                break;
            out[len] = c;
            len++;
        }
        out[len] = '\0';

        if (len == 0)
            return false;

        p += len;
        index++;
    }

    return true;
}

/*
 * ReqID:TECC4.0 - 判断语句是否属于“前端语法解析不出、但必须由客户端处理”的
 * 密态 DDL：pg_dump 还原输出的是规范括号形式 `CREATE KEK schema.name (...)`，
 * 前端语法解析失败，但客户端要对其做 key 文件校验（缺 .pub 必须中止还原）与
 * 改写，因此这类语句不允许被掩码掉。
 */
static bool
tecc_parser_span_is_client_handled_ddl(const char *stmt, const char *limit)
{
    char first[16];
    char second[24];

    if (!tecc_parser_span_word(stmt, limit, 1, first, sizeof(first)))
        return false;
    if (!tecc_parser_span_word(stmt, limit, 2, second, sizeof(second)))
        return false;

    if (pg_strcasecmp(first, "CREATE") != 0 &&
        pg_strcasecmp(first, "ALTER") != 0 &&
        pg_strcasecmp(first, "DROP") != 0)
        return false;

    return (pg_strcasecmp(second, "KEK") == 0 ||
            pg_strcasecmp(second, "DEK") == 0 ||
            pg_strcasecmp(second, "TABLE") == 0);
}

/*
 * 按顶层分号切分 SQL 文本为语句区间（识别单引号/双引号/美元引用/注释，
 * 不把字符串或注释里的分号当分隔符）。spans 由调用方 free。
 */
static TeccStmtSpan *
tecc_parser_split_statements(const char *query, int *span_count)
{
    size_t       query_len = strlen(query);
    TeccStmtSpan *spans;
    int          count = 0;
    const char   *stmt_start = query;
    const char   *p = query;

    /* 语句数不超过分号数 + 1 */
    spans = (TeccStmtSpan *) malloc(sizeof(TeccStmtSpan) * (query_len + 2));
    if (spans == NULL)
        return NULL;

    while (*p != '\0')
    {
        char c = *p;

        /* 单引号字符串（'' 与 \' 转义） */
        if (c == '\'')
        {
            p++;
            while (*p != '\0')
            {
                if (*p == '\\' && p[1] != '\0')
                {
                    p += 2;
                    continue;
                }
                if (*p == '\'')
                {
                    if (p[1] == '\'')
                    {
                        p += 2;
                        continue;
                    }
                    p++;
                    break;
                }
                p++;
            }
            continue;
        }

        /* 双引号标识符（"" 转义） */
        if (c == '"')
        {
            p++;
            while (*p != '\0')
            {
                if (*p == '"')
                {
                    if (p[1] == '"')
                    {
                        p += 2;
                        continue;
                    }
                    p++;
                    break;
                }
                p++;
            }
            continue;
        }

        /* 美元引用 $tag$ ... $tag$ */
        if (c == '$')
        {
            const char *tag_end = p + 1;

            while ((*tag_end >= 'a' && *tag_end <= 'z') ||
                   (*tag_end >= 'A' && *tag_end <= 'Z') ||
                   (*tag_end >= '0' && *tag_end <= '9') || *tag_end == '_')
                tag_end++;

            if (*tag_end == '$')
            {
                size_t tag_len = (size_t) (tag_end - p) + 1;
                const char *close = NULL;
                const char *scan = tag_end + 1;

                while (*scan != '\0')
                {
                    if (*scan == '$' && strncmp(scan, p, tag_len) == 0)
                    {
                        close = scan;
                        break;
                    }
                    scan++;
                }
                if (close != NULL)
                {
                    p = close + tag_len;
                    continue;
                }
            }
            p++;
            continue;
        }

        /* 行注释 */
        if (c == '-' && p[1] == '-')
        {
            while (*p != '\0' && *p != '\n')
                p++;
            continue;
        }

        /* 块注释（支持嵌套） */
        if (c == '/' && p[1] == '*')
        {
            int depth = 1;

            p += 2;
            while (*p != '\0' && depth > 0)
            {
                if (p[0] == '/' && p[1] == '*')
                {
                    depth++;
                    p += 2;
                }
                else if (p[0] == '*' && p[1] == '/')
                {
                    depth--;
                    p += 2;
                }
                else
                    p++;
            }
            continue;
        }

        if (c == ';')
        {
            spans[count].start = stmt_start;
            spans[count].end = p + 1;
            count++;
            stmt_start = p + 1;
            p++;
            continue;
        }

        p++;
    }

    /* 末尾残余（可能只有空白） */
    if (stmt_start < query + query_len)
    {
        spans[count].start = stmt_start;
        spans[count].end = query + query_len;
        count++;
    }

    *span_count = count;
    return spans;
}

/*
 * ReqID:TECC4.0 - 批量语句容错：返回一份把“前端语法无法解析的语句”掩码为
 * 等长空格的 SQL 副本，供解析器使用；全部语句均可解析时返回 NULL
 * （调用方直接用原文本）。返回缓冲区由调用方 free。
 *
 * 单条语句能否解析由“试解析”判定，因此不需要维护语句类型白名单；
 * 掩码不改变长度，解析树中的字节偏移仍对原始 SQL 有效。
 */
static char *
tecc_parser_mask_unparsable_stmts(const char *query)
{
    TeccStmtSpan *spans;
    char         *masked = NULL;
    char         *piece = NULL;
    int          span_count = 0;
    int          i;

    spans = tecc_parser_split_statements(query, &span_count);
    if (spans == NULL)
        return NULL;

    for (i = 0; i < span_count; i++)
    {
        const char *stmt = spans[i].start;
        const char *limit = spans[i].end;
        const char *q = stmt;
        size_t      piece_len = (size_t) (limit - stmt);
        Node        *piece_tree;
        bool        blank = false;

        /* 跳过空白；空语句（如 ";;"）也掩码掉 */
        while (q < limit && (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r'))
            q++;

        if (q >= limit || *q == ';')
            blank = true;
        else if (tecc_parser_span_is_client_handled_ddl(stmt, limit))
        {
            /* 密态 DDL（CREATE/ALTER/DROP KEK|DEK|TABLE）必须留给客户端处理，
             * 不掩码；若其无法被前端语法解析，后面的整体校验会放弃掩码。 */
            continue;
        }
        else
        {
            piece = (char *) malloc(piece_len + 1);
            if (piece == NULL)
            {
                free(masked);
                free(spans);
                return NULL;
            }
            memcpy(piece, stmt, piece_len);
            piece[piece_len] = '\0';

            piece_tree = tecc_parser_parse_node(piece);
            if (piece_tree == NULL)
                blank = true;        /* 前端语法无法解析该语句 → 掩码 */
            else
                tecc_parser_free_tree_node(piece_tree);

            free(piece);
            piece = NULL;
        }

        if (!blank)
            continue;

        if (masked == NULL)
        {
            masked = (char *) malloc(strlen(query) + 1);
            if (masked == NULL)
            {
                free(spans);
                return NULL;
            }
            memcpy(masked, query, strlen(query) + 1);
        }

        {
            size_t off_start = (size_t) (spans[i].start - query);
            size_t off_end = (size_t) (spans[i].end - query);
            size_t k;

            for (k = off_start; k < off_end; k++)
            {
                if (masked[k] != '\n')
                    masked[k] = ' ';
            }
        }
    }

    free(spans);

    /* 掩码后仍解析失败（或解析结果为空语句列表）时放弃掩码，调用方按原文
     * 处理（与既有行为一致）：CREATE KEK 还原语句等需要走解析树为空的分支
     * 做 key 文件校验，不能被掩码成空文本。 */
    if (masked != NULL)
    {
        Node *masked_tree = tecc_parser_parse_node(masked);

        if (masked_tree == NULL)
        {
            free(masked);
            return NULL;
        }
        if (nodeTag(masked_tree) == FE_T_List &&
            ((List *) masked_tree)->length == 0)
        {
            tecc_parser_free_tree_node(masked_tree);
            free(masked);
            return NULL;
        }
        tecc_parser_free_tree_node(masked_tree);
    }

    return masked;
}

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

    /* ReqID:TECC4.0 - 整批解析失败时（批量语句中夹带前端未建模语句，
     * 如 BEGIN/COMMIT/SHOW/TRUNCATE/COMMENT/ANALYZE/DROP VIEW/CREATE INDEX），
     * 逐条试解析并把无法解析的语句在解析器视野内掩码为等长空格后重试，
     * 使同批次其它语句照常识别与加密。发往服务端的 SQL 仍是原始文本，
     * 被掩码语句原样保留。 */
    if (result == NULL)
    {
        char *masked_query = tecc_parser_mask_unparsable_stmts(query_string);

        if (masked_query != NULL)
        {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "DEBUG: tecc_parser_parse: Retrying parse with unparsable statements masked");
            result = tecc_parser_parse_list(masked_query);
            free(masked_query);
        }
    }
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