/* Begin - ReqID:TECC4.0 */
/*-------------------------------------------------------------------------
 *
 * tecc_key_args_parser.c
 *    TECC key arguments parsing implementation
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/interfaces/libpq/tecc_data_fetcher/src/tecc_key_args_parser.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres_fe.h"
#include "libpq-fe.h"
#include "libpq-int.h"
#include "tecc_key_args_parser.h"
#include "tecc_error_handler.h"
#include "tecc_data_fetcher.h"
#include "tecc_data_fetcher_types.h"

/* libpq函数声明 */
#include "libpq-fe.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* bytea解码函数声明 */
#include "tecc_bytea_decoder.h"

/* 本地函数声明 */
static char *tecc_build_kek_args_query(const char *key_name, const char *key_namespace);
static char *tecc_build_dek_args_query(const char *key_name, const char *key_namespace);
static TeccKeyArg *tecc_execute_args_query(const TeccState *state, const char *query, int *count);

/*
 * tecc_parse_kek_args - 解析KEK参数
 *
 * cursor: 查询游标
 * count: 输出参数数量
 * 返回: 参数数组
 */
TeccKeyArg *
tecc_parse_kek_args(const TeccState *state, TeccCursorInterface *cursor, int *count)
{
    TeccKeyArg *args = NULL;
    int arg_count = 0;
    int column_count;
    char *query_str = NULL;
    const char *key_name;
    const char *key_namespace;

    if (cursor == NULL || count == NULL)
    {
        return NULL;
    }

    /* 获取KEK名称和命名空间 */
    /* 根据列数确定索引：6列查询（批量）或9列查询（单个） */
    column_count = cursor->get_column_count(cursor);
    if (column_count >= 9)
    {
        /* 单个查询：列0是kek_oid，列1是key_name，列2是key_namespace */
        key_name = cursor->get_column(cursor, 1);
        key_namespace = cursor->get_column(cursor, 2);
    }
    else
    {
        /* 批量查询：列0是kek_oid，列1是key_name，列2是key_namespace */
        key_name = cursor->get_column(cursor, 1);
        key_namespace = cursor->get_column(cursor, 2);
    }

    if (key_name == NULL)
    {
        if (count != NULL)
            *count = 0;
        return NULL;
    }

    /* 根据列数决定如何获取参数 */
    if (column_count >= 9)
    {
        /* 单个查询：参数信息已经在游标中（列6-8），但tecc_parse_kek_metadata已经处理了所有行 */
        /* 这里只返回NULL，让tecc_parse_kek_metadata处理参数收集 */
        if (count != NULL)
            *count = 0;
        return NULL;
    }
    else
    {
        /* 批量查询：需要执行额外的查询获取参数 */
        /* 构建查询语句 */
        query_str = tecc_build_kek_args_query(key_name, key_namespace);
        if (query_str == NULL)
        {
            if (count != NULL)
                *count = 0;
            return NULL;
        }

        /* 执行查询并解析结果 */
        args = tecc_execute_args_query(state, query_str, &arg_count);
        pg_free(query_str);
    }

    if (count != NULL)
        *count = arg_count;

    return args;
}

/*
 * tecc_parse_dek_args - 解析DEK参数
 *
 * cursor: 查询游标
 * count: 输出参数数量
 * 返回: 参数数组
 */
TeccKeyArg *
tecc_parse_dek_args(const TeccState *state, TeccCursorInterface *cursor, int *count)
{
    TeccKeyArg *args = NULL;
    int arg_count = 0;
    int column_count;
    char *query_str = NULL;
    const char *key_name;
    const char *key_namespace;

    if (cursor == NULL || count == NULL)
    {
        return NULL;
    }

    /* 获取DEK名称和命名空间 */
    /* 根据列数确定索引：9列查询（批量）或13列查询（单个） */
    column_count = cursor->get_column_count(cursor);
    if (column_count >= 13)
    {
        /* 单个查询：列1是key_name，列5是key_namespace */
        key_name = cursor->get_column(cursor, 1);
        key_namespace = cursor->get_column(cursor, 5);
    }
    else
    {
        /* 批量查询：列1是key_name，列5是key_namespace */
        key_name = cursor->get_column(cursor, 1);
        key_namespace = cursor->get_column(cursor, 5);
    }

    if (key_name == NULL)
    {
        if (count != NULL)
            *count = 0;
        return NULL;
    }

    /* 根据列数决定如何获取参数 */
    if (column_count >= 13)
    {
        /* 多行参数由 tecc_parse_dek_metadata 的 do-while 处理（含OID边界检查）。
         * 这里返回 NULL，不消费游标行 — 与 KEK 端 tecc_parse_kek_args 模式一致。 */
        if (count != NULL)
            *count = 0;
        return NULL;
    }
    else
    {
        /* 批量查询：需要执行额外的查询获取参数 */
        /* 构建查询语句 */
        query_str = tecc_build_dek_args_query(key_name, key_namespace);
        if (query_str == NULL)
        {
            if (count != NULL)
                *count = 0;
            return NULL;
        }

        /* 执行查询并解析结果 */
        args = tecc_execute_args_query(state, query_str, &arg_count);
        pg_free(query_str);
    }

    if (count != NULL)
        *count = arg_count;

    return args;
}

/*
 * tecc_free_key_args - 释放参数数组
 *
 * args: 参数数组
 * count: 参数数量
 */
void
tecc_free_key_args(TeccKeyArg *args, int count)
{
    int i;

    if (args == NULL)
    {
        return;
    }

    for (i = 0; i < count; i++)
    {
        if (args[i].function_name != NULL)
            pg_free(args[i].function_name);
        if (args[i].key != NULL)
            pg_free(args[i].key);
        if (args[i].value != NULL)
            pg_free(args[i].value);
    }

    pg_free(args);
}

/*
 * tecc_find_key_arg - 查找特定参数
 *
 * args: 参数数组
 * count: 参数数量
 * function_name: 函数名称
 * key: 参数键
 * 返回: 参数值
 */
const char *
tecc_find_key_arg(TeccKeyArg *args, int count,
                  const char *function_name, const char *key)
{
    int i;

    if (args == NULL || function_name == NULL || key == NULL)
    {
        return NULL;
    }

    for (i = 0; i < count; i++)
    {
        if (strcmp(args[i].function_name, function_name) == 0 &&
            strcmp(args[i].key, key) == 0)
        {
            return args[i].value;
        }
    }

    return NULL;
}

/*
 * tecc_validate_key_args - 验证参数格式
 *
 * args: 参数数组
 * count: 参数数量
 * 返回: true如果参数格式有效
 */
bool
tecc_validate_key_args(TeccKeyArg *args, int count)
{
    int i;

    if (args == NULL && count == 0)
    {
        return true; /* 空参数列表是有效的 */
    }

    if (args == NULL || count < 0)
    {
        return false;
    }

    for (i = 0; i < count; i++)
    {
        if (args[i].function_name == NULL ||
            args[i].key == NULL ||
            args[i].value == NULL)
        {
            return false;
        }
    }

    return true;
}

/*
 * tecc_build_kek_args_query - 构建KEK参数查询语句
 *
 * key_name: KEK名称
 * key_namespace: 命名空间
 * 返回: 查询语句字符串，需要调用者释放
 */
static char *
tecc_build_kek_args_query(const char *key_name, const char *key_namespace)
{
    char *query;
    size_t query_len;

    if (key_name == NULL)
    {
        return NULL;
    }

    /* 计算查询长度 */
    if (key_namespace != NULL)
    {
        query_len = strlen(key_name) + strlen(key_namespace) + 300;
    }
    else
    {
        query_len = strlen(key_name) + 250;
    }

    /* 分配内存 */
    query = (char *)pg_malloc(query_len);
    if (query == NULL)
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_MEMORY,
                               "Failed to allocate KEK args query string");
        return NULL;
    }

    /* 构建查询语句 */
    if (key_namespace != NULL)
    {
        snprintf(query, query_len,
                 "SELECT a.function_name, a.\"key\", a.value "
                 "FROM hg_kek_args a "
                 "JOIN hg_kek k ON a.kek_id = k.oid "
                 "JOIN pg_namespace n ON k.key_namespace = n.oid "
                 "WHERE k.kek_name = '%s' AND n.nspname = '%s' AND a.function_name = 'kek_config' "
                 "ORDER BY a.function_name, a.\"key\"",
                 key_name, key_namespace);
    }
    else
    {
        snprintf(query, query_len,
                 "SELECT a.function_name, a.\"key\", a.value "
                 "FROM hg_kek_args a "
                 "JOIN hg_kek k ON a.kek_id = k.oid "
                 "WHERE k.kek_name = '%s' AND a.function_name = 'kek_config' "
                 "ORDER BY a.function_name, a.\"key\"",
                 key_name);
    }

    return query;
}

/*
 * tecc_build_dek_args_query - 构建DEK参数查询语句
 *
 * key_name: DEK名称
 * key_namespace: 命名空间
 * 返回: 查询语句字符串，需要调用者释放
 */
static char *
tecc_build_dek_args_query(const char *key_name, const char *key_namespace)
{
    char *query;
    size_t query_len;

    if (key_name == NULL)
    {
        return NULL;
    }

    /* 计算查询长度 */
    if (key_namespace != NULL)
    {
        query_len = strlen(key_name) + strlen(key_namespace) + 300;
    }
    else
    {
        query_len = strlen(key_name) + 250;
    }

    /* 分配内存 */
    query = (char *)pg_malloc(query_len);
    if (query == NULL)
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_MEMORY,
                               "Failed to allocate DEK args query string");
        return NULL;
    }

    /* 构建查询语句 */
    if (key_namespace != NULL)
    {
        snprintf(query, query_len,
                 "SELECT a.function_name, a.key, a.value "
                 "FROM hg_dek_args a "
                 "JOIN hg_dek d ON a.dek_id = d.oid "
                 "JOIN pg_namespace n ON d.key_namespace = n.oid "
                 "WHERE d.dek_name = '%s' AND n.nspname = '%s' AND a.function_name = 'dek_config' "
                 "ORDER BY a.function_name, a.key",
                 key_name, key_namespace);
    }
    else
    {
        snprintf(query, query_len,
                 "SELECT a.function_name, a.key, a.value "
                 "FROM hg_dek_args a "
                 "JOIN hg_dek d ON a.dek_id = d.oid "
                 "WHERE d.dek_name = '%s' AND a.function_name = 'dek_config' "
                 "ORDER BY a.function_name, a.key",
                 key_name);
    }

    return query;
}

/*
 * tecc_execute_args_query - 执行参数查询并解析结果
 *
 * parent_cursor: 父游标（用于获取连接）
 * query: 查询语句
 * count: 输出参数数量
 * 返回: 参数数组
 */
static TeccKeyArg *
tecc_execute_args_query(const TeccState *state, const char *query, int *count)
{
    TeccKeyArg *args = NULL;
    TeccCursorInterface *cursor = NULL;
    int arg_count = 0;
    int max_args = 10;
    int i;

    if (query == NULL || count == NULL || state == NULL)
    {
        return NULL;
    }

    /* 执行查询 */
    cursor = tecc_fetcher_execute_query(state, query);
    if (cursor == NULL)
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_QUERY,
                               "Failed to execute args query");
        return NULL;
    }

    /* 分配初始参数数组 */
    args = (TeccKeyArg *)pg_malloc(sizeof(TeccKeyArg) * max_args);
    if (args == NULL)
    {
        cursor->close(cursor);
        cursor->destroy(cursor);
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_MEMORY,
                               "Failed to allocate key args array");
        if (count != NULL)
            *count = 0;
        return NULL;
    }

    /* 初始化参数数组 */
    for (i = 0; i < max_args; i++)
    {
        args[i].function_name = NULL;
        args[i].key = NULL;
        args[i].value = NULL;
    }

    /* 遍历结果集 */
    while (cursor->fetch_next(cursor) == 0)
    {
        const char *function_name;
        const char *key;
        const char *value;

        /* 检查是否需要扩展数组 */
        if (arg_count >= max_args)
        {
            max_args *= 2;
            args = (TeccKeyArg *)pg_realloc(args, sizeof(TeccKeyArg) * max_args);
            if (args == NULL)
            {
                cursor->close(cursor);
                cursor->destroy(cursor);
                tecc_log_fetcher_error(TECC_FETCHER_ERROR_MEMORY,
                                       "Failed to reallocate key args array");
                if (count != NULL)
                    *count = 0;
                return NULL;
            }

            /* 初始化新分配的部分 */
            for (i = arg_count; i < max_args; i++)
            {
                args[i].function_name = NULL;
                args[i].key = NULL;
                args[i].value = NULL;
            }
        }

        /* 获取列值 */
        function_name = cursor->get_column(cursor, 0);
        key = cursor->get_column(cursor, 1);
        value = cursor->get_column(cursor, 2);

        /* 复制字符串 */
        if (function_name != NULL)
            args[arg_count].function_name = pg_strdup(function_name);
        if (key != NULL)
            args[arg_count].key = pg_strdup(key);
        if (value != NULL)
            args[arg_count].value = decode_bytea_value(value);

        arg_count++;
    }

    /* 关闭游标 */
    cursor->close(cursor);
    cursor->destroy(cursor);

    /* 如果没有参数，清理内存 */
    if (arg_count == 0)
    {
        pg_free(args);
        args = NULL;
    }

    if (count != NULL)
        *count = arg_count;

    return args;
}
/* End - ReqID:TECC4.0 */