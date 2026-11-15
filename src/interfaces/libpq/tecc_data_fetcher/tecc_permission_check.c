/* Begin - ReqID:TECC4.0 */
/*-------------------------------------------------------------------------
 *
 * tecc_permission_check.c
 *    TECC permission checking implementation
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/interfaces/libpq/tecc_data_fetcher/src/tecc_permission_check.c
 *
 *-------------------------------------------------------------------------
 */
#include "tecc_permission_check.h"
#include "tecc_error_handler.h"
#include "tecc_data_fetcher.h"

/* TECC状态管理接口 */
#include "../tecc_libpq_integration/fe-tecc-integration.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 辅助函数声明 */
static char *tecc_escape_string(const char *str);
static TeccFetcherErrorCode tecc_check_key_permission(const TeccState *state,
                                                       const char *table_name,
                                                       const char *key_column,
                                                       const char *priv_func,
                                                       const char *key_name,
                                                       const char *key_namespace);

/* 当前用户缓存 - 已移除，改为从PGconn连接获取 */

/*
 * tecc_check_key_permission - 检查密钥权限（公共实现）
 *
 * state: TECC状态结构
 * table_name: 系统表名（hg_kek 或 hg_dek）
 * key_column: 密钥名字段（kek_name 或 dek_name）
 * priv_func: 权限检查函数名（has_kek_privilege 或 has_dek_privilege）
 * key_name: 密钥名称
 * key_namespace: 命名空间
 * 返回: 错误码，TECC_FETCHER_OK表示有权限
 */
static TeccFetcherErrorCode
tecc_check_key_permission(const TeccState *state,
                           const char *table_name,
                           const char *key_column,
                           const char *priv_func,
                           const char *key_name,
                           const char *key_namespace)
{
    PGconn *conn;
    char *query = NULL;
    char *escaped_key_name = NULL;
    char *escaped_namespace = NULL;
    int query_len;
    bool has_permission = false;

    /* 参数检查 */
    if (state == NULL || key_name == NULL || table_name == NULL ||
        key_column == NULL || priv_func == NULL)
    {
        return TECC_FETCHER_ERROR_INVALID_PARAM;
    }

    /* 获取连接（主连接空闲时直接使用，繁忙时自动创建临时连接） */
    conn = tecc_data_fetcher_get_connection(state);
    if (conn == NULL)
    {
        return TECC_FETCHER_ERROR_CONNECTION;
    }

    /* 转义输入字符串 */
    escaped_key_name = tecc_escape_string(key_name);
    escaped_namespace = (key_namespace != NULL) ? tecc_escape_string(key_namespace) : NULL;

    if (escaped_key_name == NULL)
    {
        tecc_data_fetcher_release_connection(state, conn);
        return TECC_FETCHER_ERROR_MEMORY;
    }

    /* 构建权限检查查询 */
    query_len = strlen(escaped_key_name) * 3 + 450;
    if (escaped_namespace != NULL)
        query_len += strlen(escaped_namespace) * 3;

    query = (char *)pg_malloc(query_len);
    if (query == NULL)
    {
        pg_free(escaped_key_name);
        if (escaped_namespace != NULL)
            pg_free(escaped_namespace);
        tecc_data_fetcher_release_connection(state, conn);
        return TECC_FETCHER_ERROR_MEMORY;
    }

    if (escaped_namespace != NULL)
    {
        snprintf(query, query_len,
                 "SELECT %s("
                 "  (SELECT k.oid FROM %s k "
                 "   JOIN pg_namespace n ON k.key_namespace = n.oid "
                 "   WHERE lower(k.%s) = lower('%s') AND lower(n.nspname) = lower('%s')), "
                 "'USAGE')",
                 priv_func, table_name, key_column,
                 escaped_key_name, escaped_namespace);
    }
    else
    {
        snprintf(query, query_len,
                 "SELECT %s("
                 "  (SELECT k.oid FROM %s k "
                 "   JOIN pg_namespace n ON k.key_namespace = n.oid "
                 "   WHERE lower(k.%s) = lower('%s') "
                 "   LIMIT 1), "
                 "'USAGE')",
                 priv_func, table_name, key_column,
                 escaped_key_name);
    }

    /* 释放转义字符串 */
    pg_free(escaped_key_name);
    if (escaped_namespace != NULL)
        pg_free(escaped_namespace);

    /* 直接使用PQexec执行权限检查（避免tecc_fetcher_execute_query的游标开销） */
    {
        PGresult *pg_result;
        pg_result = PQexec(conn, query);
        if (pg_result && PQresultStatus(pg_result) == PGRES_TUPLES_OK && PQntuples(pg_result) > 0)
        {
            const char *val = PQgetvalue(pg_result, 0, 0);
            if (val && strcmp(val, "t") == 0)
                has_permission = true;
        }
        if (pg_result) PQclear(pg_result);
    }
    pg_free(query);

    tecc_data_fetcher_release_connection(state, conn);

    if (!has_permission)
    {
        return TECC_FETCHER_ERROR_PERMISSION;
    }

    return TECC_FETCHER_OK;
}

/*
 * tecc_check_kek_permission - 检查KEK权限
 *
 * state: TECC状态结构
 * key_name: KEK名称
 * key_namespace: 命名空间
 * 返回: 错误码，TECC_FETCHER_OK表示有权限
 */
TeccFetcherErrorCode
tecc_check_kek_permission(const TeccState *state,
                          const char *key_name, const char *key_namespace)
{
    return tecc_check_key_permission(state, "hg_kek", "kek_name",
                                     "has_kek_privilege", key_name, key_namespace);
}

/*
 * tecc_check_dek_permission - 检查DEK权限
 *
 * state: TECC状态结构
 * key_name: DEK名称
 * key_namespace: 命名空间
 * 返回: 错误码，TECC_FETCHER_OK表示有权限
 */
TeccFetcherErrorCode
tecc_check_dek_permission(const TeccState *state,
                          const char *key_name, const char *key_namespace)
{
    return tecc_check_key_permission(state, "hg_dek", "dek_name",
                                     "has_dek_privilege", key_name, key_namespace);
}

/*
 * tecc_get_current_user - 获取当前用户名
 *
 * state: TECC状态结构
 * 返回: 当前用户名
 */
const char *
tecc_get_current_user(const TeccState *state)
{
    const char *username;

    /* 参数检查 */
    if (state == NULL || state->conn == NULL)
    {
        return "unknown";
    }

    /* 直接从PGconn连接获取用户名 */
    username = PQuser(state->conn);

    /* 如果PQuser返回NULL，使用默认值 */
    if (username == NULL || username[0] == '\0')
    {
        return "unknown";
    }

    return username;
}

/*
 * tecc_log_permission_error - 记录权限错误
 *
 * state: TECC状态结构
 * resource_type: 资源类型（如"KEK"、"DEK"）
 * resource_name: 资源名称
 */
void
tecc_log_permission_error(const TeccState *state,
                          const char *resource_type, const char *resource_name)
{
    const char *user = tecc_get_current_user(state);

    TECC_LOG(TECC_LOG_WARNING, "Permission denied: User '%s' cannot access %s '%s'",
            user, resource_type, resource_name);
}

/*
 * tecc_escape_string - 转义字符串用于SQL查询
 *
 * str: 要转义的字符串
 * 返回: 转义后的字符串，需要调用者释放
 */
static char *
tecc_escape_string(const char *str)
{
    char *escaped;
    size_t len;
    size_t i, j;

    if (str == NULL)
    {
        return NULL;
    }

    /* 计算转义后的长度 */
    len = strlen(str);
    for (i = 0; i < len; i++)
    {
        if (str[i] == '\'' || str[i] == '\\' || str[i] == '%')
        {
            len++; /* 每个需要转义的字符增加一个位置 */
        }
    }

    /* 分配内存 */
    escaped = (char *)pg_malloc(len + 1);
    if (escaped == NULL)
    {
        return NULL;
    }

    /* 执行转义 */
    for (i = 0, j = 0; str[i] != '\0'; i++)
    {
        if (str[i] == '\'')
        {
            escaped[j++] = '\'';
            escaped[j++] = '\'';
        }
        else if (str[i] == '\\')
        {
            escaped[j++] = '\\';
            escaped[j++] = '\\';
        }
        else if (str[i] == '%')
        {
            escaped[j++] = '%';
            escaped[j++] = '%';
        }
        else
        {
            escaped[j++] = str[i];
        }
    }
    escaped[j] = '\0';

    return escaped;
}
/* End - ReqID:TECC4.0 */