/* Begin - ReqID:TECC4.0 */
/*-------------------------------------------------------------------------
 *
 * tecc_data_fetcher.c
 *    TECC data fetcher module main implementation
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/interfaces/libpq/tecc_data_fetcher/src/tecc_data_fetcher.c
 *
 *-------------------------------------------------------------------------
 */
#include "tecc_data_fetcher.h"
#include "tecc_data_fetcher_types.h"
#include "tecc_libpq_cursor.h"
#include "tecc_metadata_query.h"
#include "tecc_permission_check.h"
#include "tecc_bitmask_parser.h"
/* 通用日志系统 */
#include "../tecc_common/tecc_logging.h"
#include "tecc_key_args_parser.h"
#include "tecc_error_handler.h"
#include "tecc_metadata_serializer.h"

/* TECC状态管理接口 */
#include "../tecc_libpq_integration/fe-tecc-integration.h"

/* 辅助函数声明 */
static char *tecc_escape_string(const char *str);
TeccCursorInterface *tecc_fetcher_execute_query(const TeccState *state, const char *query);

/* 日志函数声明 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

/* 连接级管理，每连接通过TeccState管理元数据查询连接 */



/*
 * tecc_fetch_kek_metadata - 获取KEK元数据
 *
 * state: TECC状态结构
 * key_name: KEK名称
 * key_namespace: 命名空间
 * result: 输出KEK元数据指针
 * 返回: 错误码，TECC_FETCHER_OK表示成功
 */
TeccFetcherErrorCode
tecc_fetch_kek_metadata(const TeccState *state,
                        const char *key_name,
                        const char *key_namespace,
                        PGconn *ext_conn,
                        TeccKekMetadata **result)
{
    TeccKekMetadata *kek_metadata = NULL;
    TeccCursorInterface *cursor = NULL;
    char *query = NULL;
    char *escaped_key_name = NULL;
    char *escaped_namespace = NULL;
    char *new_query = NULL;
    int query_len;
    TeccFetcherErrorCode error_code = TECC_FETCHER_OK;

    /* 参数检查 */
    if (key_name == NULL || result == NULL)
    {
        return TECC_FETCHER_ERROR_INVALID_PARAM;
    }

    /* 初始化输出参数 */
    *result = NULL;


    TECC_LOG(TECC_LOG_DEBUG1, "tecc_fetch_kek_metadata: Starting to fetch KEK metadata for key_name='%s', key_namespace='%s'",
             key_name, key_namespace ? key_namespace : "(null)");

    /* 转义输入字符串 */
    escaped_key_name = tecc_escape_string(key_name);
    escaped_namespace = (key_namespace != NULL) ? tecc_escape_string(key_namespace) : NULL;

    if (escaped_key_name == NULL)
    {
        return TECC_FETCHER_ERROR_MEMORY;
    }

    /* 构建查询语句 */
    query_len = strlen(escaped_key_name) + 500; /* 预留足够空间 - 增加因为包含JOIN语句 */
    if (escaped_namespace != NULL)
    {
        query_len += strlen(escaped_namespace);
    }

    query = (char *)pg_malloc(query_len);
    if (query == NULL)
    {
        pg_free(escaped_key_name);
        if (escaped_namespace != NULL)
            pg_free(escaped_namespace);
        return TECC_FETCHER_ERROR_MEMORY;
    }

    if (escaped_namespace != NULL)
    {
        int written = snprintf(query, query_len,
                 "SELECT kek_oid, kek_name, key_namespace_name, key_owner_oid, key_owner_name, "
                 "key_acl, create_date, function_name, arg_key, arg_value "
                 "FROM hg_kek_info "
                 "WHERE lower(kek_name) = lower('%s') AND lower(key_namespace_name) = lower('%s') "
                 "ORDER BY kek_oid, function_name, arg_key",
                 escaped_key_name, escaped_namespace);
        if (written >= query_len) {
            /* 缓冲区不足，重新分配 */
            query_len = written + 1;
            new_query = (char *)pg_realloc(query, query_len);
            if (new_query == NULL) {
                pg_free(escaped_key_name);
                pg_free(escaped_namespace);
                pg_free(query);
                return TECC_FETCHER_ERROR_MEMORY;
            }
            query = new_query;
            snprintf(query, query_len,
                     "SELECT k.oid as kek_oid, k.kek_name, n.nspname as key_namespace, "
                     "k.key_owner as key_owner_oid, r.rolname as key_owner_name, "
                     "k.key_acl, k.create_date, "
                     "ka.function_name, ka.\"key\" as arg_key, ka.value as arg_value "
                     "FROM hg_kek k "
                     "JOIN pg_namespace n ON k.key_namespace = n.oid "
                     "JOIN pg_roles r ON k.key_owner = r.oid "
                     "LEFT JOIN hg_kek_args ka ON k.oid = ka.kek_id AND ka.function_name = 'kek_config' "
                     "WHERE lower(k.kek_name) = lower('%s') AND lower(n.nspname) = lower('%s') "
                     "ORDER BY k.oid, ka.function_name, ka.\"key\"",
                     escaped_key_name, escaped_namespace);
        }
    }
    else
    {
        int written = snprintf(query, query_len,
                 "SELECT kek_oid, kek_name, key_namespace_name, key_owner_oid, key_owner_name, "
                 "key_acl, create_date, function_name, arg_key, arg_value "
                 "FROM hg_kek_info "
                 "WHERE lower(kek_name) = lower('%s') "
                 "ORDER BY kek_oid, function_name, arg_key",
                 escaped_key_name);
        if (written >= query_len) {
            /* 缓冲区不足，重新分配 */
            query_len = written + 1;
            new_query = (char *)pg_realloc(query, query_len);
            if (new_query == NULL) {
                pg_free(escaped_key_name);
                pg_free(query);
                return TECC_FETCHER_ERROR_MEMORY;
            }
            query = new_query;
            snprintf(query, query_len,
                     "SELECT k.oid as kek_oid, k.kek_name, n.nspname as key_namespace, "
                     "k.key_owner as key_owner_oid, r.rolname as key_owner_name, "
                     "k.key_acl, k.create_date, "
                     "ka.function_name, ka.\"key\" as arg_key, ka.value as arg_value "
                     "FROM hg_kek k "
                     "JOIN pg_namespace n ON k.key_namespace = n.oid "
                     "JOIN pg_roles r ON k.key_owner = r.oid "
                     "LEFT JOIN hg_kek_args ka ON k.oid = ka.kek_id AND ka.function_name = 'kek_config' "
                     "WHERE lower(k.kek_name) = lower('%s') "
                     "ORDER BY k.oid, ka.function_name, ka.\"key\"",
                     escaped_key_name);
        }
    }

    /* 释放转义字符串 */
    pg_free(escaped_key_name);
    if (escaped_namespace != NULL)
        pg_free(escaped_namespace);

    /* 执行查询 — 使用外部连接或创建临时连接 */
    if (ext_conn != NULL)
    {
        TeccLibPQCursor *lp_cursor;
        cursor = tecc_libpq_create_cursor(ext_conn);
        if (cursor == NULL)
        {
            pg_free(query);
            return TECC_FETCHER_ERROR_MEMORY;
        }
        lp_cursor = (TeccLibPQCursor *)cursor->private_data;
        lp_cursor->conn_is_temp = false;
        if (cursor->cursor_open(cursor, query) != TECC_FETCHER_OK)
        {
            pg_free(query);
            cursor->destroy(cursor);
            return TECC_FETCHER_ERROR_QUERY;
        }
        pg_free(query);
    }
    else
    {
        cursor = tecc_fetcher_execute_query(state, query);
        pg_free(query);
        if (cursor == NULL)
        {
            TECC_LOG(TECC_LOG_ERROR, "tecc_fetch_kek_by_name: Failed to execute query");
            return TECC_FETCHER_ERROR_QUERY;
        }
    }

    TECC_LOG(TECC_LOG_DEBUG1, "tecc_fetch_kek_by_name: Query executed successfully, calling tecc_parse_kek_metadata");

    /* 解析结果 */
    error_code = tecc_parse_kek_metadata(state, cursor, &kek_metadata);

    TECC_LOG(TECC_LOG_DEBUG1, "tecc_fetch_kek_by_name: tecc_parse_kek_metadata returned error_code: %d", error_code);

    /* 关闭游标 */
    if (cursor != NULL)
    {
        cursor->close(cursor);
        cursor->destroy(cursor);
    }

    if (error_code == TECC_FETCHER_OK)
    {
        *result = kek_metadata;
    }

    return error_code;
}

/*
 * tecc_fetch_dek_metadata - 获取DEK元数据
 *
 * state: TECC状态结构
 * key_name: DEK名称
 * key_namespace: 命名空间
 * result: 输出DEK元数据指针
 * 返回: 错误码
 */
TeccFetcherErrorCode
tecc_fetch_dek_metadata(const TeccState *state,
                        const char *key_name,
                        const char *key_namespace,
                        PGconn *ext_conn,
                        TeccDekMetadata **result)
{
    TeccDekMetadata *dek_metadata = NULL;
    TeccCursorInterface *cursor = NULL;
    char *query = NULL;
    char *escaped_key_name = NULL;
    char *escaped_namespace = NULL;
    int query_len;
    TeccFetcherErrorCode error_code = TECC_FETCHER_OK;

    /* 参数检查 */
    if (key_name == NULL || result == NULL)
    {
        return TECC_FETCHER_ERROR_INVALID_PARAM;
    }

    /* 初始化输出参数 */
    *result = NULL;



    /* 转义输入字符串 */
    escaped_key_name = tecc_escape_string(key_name);
    escaped_namespace = (key_namespace != NULL) ? tecc_escape_string(key_namespace) : NULL;

    if (escaped_key_name == NULL)
    {
        return TECC_FETCHER_ERROR_MEMORY;
    }

    /* 构建查询语句 */
    query_len = strlen(escaped_key_name) + 600; /* 预留足够空间 - 增加因为包含多个JOIN语句，需要至少600字符 */
    if (escaped_namespace != NULL)
    {
        query_len += strlen(escaped_namespace);
    }

    query = (char *)pg_malloc(query_len);
    if (query == NULL)
    {
        pg_free(escaped_key_name);
        if (escaped_namespace != NULL)
            pg_free(escaped_namespace);
        return TECC_FETCHER_ERROR_MEMORY;
    }

    if (escaped_namespace != NULL)
    {
        snprintf(query, query_len,
                 "SELECT dek_oid, dek_name, column_key_distributed_id, kek_oid, kek_name, "
                 "key_namespace_name, key_owner_oid, key_owner_name, key_acl, create_date, "
                 "function_name, arg_key, arg_value "
                 "FROM hg_dek_info "
                 "WHERE dek_name = '%s' AND key_namespace_name = '%s' "
                 "ORDER BY dek_oid, function_name, arg_key",
                 escaped_key_name, escaped_namespace);
    }
    else
    {
        snprintf(query, query_len,
                 "SELECT dek_oid, dek_name, column_key_distributed_id, kek_oid, kek_name, "
                 "key_namespace_name, key_owner_oid, key_owner_name, key_acl, create_date, "
                 "function_name, arg_key, arg_value "
                 "FROM hg_dek_info "
                 "WHERE dek_name = '%s' "
                 "ORDER BY dek_oid, function_name, arg_key",
                 escaped_key_name);
    }

    /* 释放转义字符串 */
    pg_free(escaped_key_name);
    if (escaped_namespace != NULL)
        pg_free(escaped_namespace);

    /* 执行查询 — 使用外部连接或创建临时连接 */
    if (ext_conn != NULL)
    {
        TeccLibPQCursor *lp_cursor;
        cursor = tecc_libpq_create_cursor(ext_conn);
        if (cursor == NULL)
        {
            pg_free(query);
            return TECC_FETCHER_ERROR_MEMORY;
        }
        lp_cursor = (TeccLibPQCursor *)cursor->private_data;
        lp_cursor->conn_is_temp = false;
        if (cursor->cursor_open(cursor, query) != TECC_FETCHER_OK)
        {
            pg_free(query);
            cursor->destroy(cursor);
            return TECC_FETCHER_ERROR_QUERY;
        }
        pg_free(query);
    }
    else
    {
        cursor = tecc_fetcher_execute_query(state, query);
        pg_free(query);
        if (cursor == NULL)
        {
            return TECC_FETCHER_ERROR_QUERY;
        }
    }

    /* 解析结果 */
    error_code = tecc_parse_dek_metadata(state, cursor, &dek_metadata);

    /* 关闭游标 */
    if (cursor != NULL)
    {
        cursor->close(cursor);
        cursor->destroy(cursor);
    }

    if (error_code == TECC_FETCHER_OK && dek_metadata != NULL)
    {
        *result = dek_metadata;
    }

    return error_code;
}

/*
 * tecc_fetch_dek_metadata_by_oid - 按OID获取DEK元数据
 */
TeccFetcherErrorCode
tecc_fetch_dek_metadata_by_oid(const TeccState *state,
                               Oid dek_oid,
                               TeccDekMetadata **result)
{
    char query[512];
    TeccCursorInterface *cursor = NULL;
    TeccFetcherErrorCode error_code;

    if (state == NULL || result == NULL)
        return TECC_FETCHER_ERROR_INVALID_PARAM;

    *result = NULL;

    snprintf(query, sizeof(query),
             "SELECT dek_oid, dek_name, column_key_distributed_id, kek_oid, kek_name, "
             "key_namespace_name, key_owner_oid, key_owner_name, key_acl, create_date, "
             "function_name, arg_key, arg_value "
             "FROM hg_dek_info WHERE dek_oid = %u "
             "ORDER BY dek_oid, function_name, arg_key",
             dek_oid);

    cursor = tecc_fetcher_execute_query(state, query);
    if (cursor == NULL)
        return TECC_FETCHER_ERROR_QUERY;

    /* Note: Do NOT call cursor->next() here — tecc_parse_dek_metadata
     * already calls cursor->next() internally.  A duplicate next() would
     * skip the first row, which is the one carrying the encrypted_value
     * arg (alphabetically first under ORDER BY arg_key).  Bug 7826. */
    error_code = tecc_parse_dek_metadata(state, cursor, result);
    cursor->close(cursor);
    cursor->destroy(cursor);

    if (error_code != TECC_FETCHER_OK)
    {
        tecc_free_dek_metadata(*result);
        *result = NULL;
        return error_code;
    }

    return TECC_FETCHER_OK;
}

/*
 * tecc_fetch_encrypted_column_metadata - 获取加密列元数据
 *
 * state: TECC状态结构
 * table_oid: 表OID
 * column_attnum: 列属性编号
 * result: 输出加密列元数据指针
 * 返回: 错误码
 */
TeccFetcherErrorCode
tecc_fetch_encrypted_column_metadata(const TeccState *state,
                                     Oid table_oid,
                                     int column_attnum,
                                     TeccEncryptedColumnMetadata **result)
{
    TeccEncryptedColumnMetadata *column_metadata = NULL;
    TeccCursorInterface *cursor = NULL;
    char query[512]; /* 增加缓冲区大小 */
    TeccFetcherErrorCode error_code = TECC_FETCHER_OK;

    /* 参数检查 */
    if (table_oid == InvalidOid || column_attnum <= 0 || result == NULL)
    {
        return TECC_FETCHER_ERROR_INVALID_PARAM;
    }

    /* 初始化输出参数 */
    *result = NULL;

    /* 构建查询语句 - 只返回解析函数期望的10个字段 */
    /* 注意：hg_encrypted_columns表在当前数据库中，所以不需要额外的数据库名条件 */
    snprintf(query, sizeof(query),
             "SELECT table_oid, cls.relname AS table_name, column_attnum, column_name, data_type_original_oid, "
             "data_type_original_mod, dek_name, encryption_type, collation_locale, column_key_id, create_date, "
             "has_dek_privilege(column_key_id, 'USAGE') AS has_dek_permission "
             "FROM hg_encrypted_columns_info "
             "JOIN pg_class cls ON table_oid = cls.oid "
             "WHERE table_oid = %u AND column_attnum = %d",
             table_oid, column_attnum);

    /* 执行查询 */
    cursor = tecc_fetcher_execute_query(state, query);
    if (cursor == NULL)
    {
        return TECC_FETCHER_ERROR_QUERY;
    }

    /* 移动到第一行 */
    if (cursor->next(cursor) != TECC_FETCHER_OK)
    {
        TECC_LOG(TECC_LOG_WARNING, "No encrypted column found for table_oid=%u, column_attnum=%d",
                 table_oid, column_attnum);
        cursor->close(cursor);
        cursor->destroy(cursor);
        return TECC_FETCHER_ERROR_NOT_FOUND;
    }

    /* 解析结果 */
    error_code = tecc_parse_encrypted_column_metadata(cursor, &column_metadata);

    /* 关闭游标 */
    cursor->close(cursor);
    cursor->destroy(cursor);

    if (error_code == TECC_FETCHER_OK && column_metadata != NULL)
    {
        *result = column_metadata;
    }

    return error_code;
}

/*
 * tecc_fetch_kek_list - 获取指定命名空间的KEK列表
 *
 * state: TECC状态结构
 * namespace_filter: 命名空间过滤条件，NULL表示所有命名空间
 * result: 输出KEK元数据列表
 * 返回: 错误码
 *
 * 设计原则: 数据获取器负责解析原始查询结果为结构化元数据对象列表。
 * 缓存模块直接存储这些已解析的对象，无需再次解析。
 */
TeccFetcherErrorCode
tecc_fetch_kek_list(const TeccState *state,
                    const char *namespace_filter,
                    List **result)
{
    TeccCursorInterface *cursor = NULL;
    char *query = NULL;
    char *escaped_namespace = NULL;
    int query_len;
    TeccFetcherErrorCode error_code = TECC_FETCHER_OK;
    List *kek_list = NULL;

    /* 参数检查 */
    if (result == NULL)
        return TECC_FETCHER_ERROR_INVALID_PARAM;

    /* 初始化输出参数 */
    *result = NULL;

    /* 转义命名空间过滤条件 */
    if (namespace_filter != NULL)
    {
        escaped_namespace = tecc_escape_string(namespace_filter);
        if (escaped_namespace == NULL)
            return TECC_FETCHER_ERROR_MEMORY;
    }

    /* 构建查询语句 */
    query_len = 400; /* 基础查询长度 - 增加因为包含JOIN语句 */
    if (escaped_namespace != NULL)
        query_len += strlen(escaped_namespace) * 2;

    query = (char *)pg_malloc(query_len);
    if (query == NULL)
    {
        if (escaped_namespace != NULL)
            pg_free(escaped_namespace);
        return TECC_FETCHER_ERROR_MEMORY;
    }

    if (escaped_namespace != NULL)
    {
        snprintf(query, query_len,
                 "SELECT kek_oid, kek_name, key_namespace_name, key_owner_oid, key_owner_name, "
                 "key_acl, create_date, function_name, arg_key, arg_value "
                 "FROM hg_kek_info "
                 "WHERE key_namespace_name = '%s' "
                 "ORDER BY key_namespace_name, kek_name, arg_key",
                 escaped_namespace);
    }
    else
    {
        snprintf(query, query_len,
                 "SELECT kek_oid, kek_name, key_namespace_name, key_owner_oid, key_owner_name, "
                 "key_acl, create_date, function_name, arg_key, arg_value "
                 "FROM hg_kek_info "
                 "ORDER BY key_namespace_name, kek_name, arg_key");
    }

    /* 释放转义字符串 */
    if (escaped_namespace != NULL)
        pg_free(escaped_namespace);

    /* 记录调试信息 */
    TECC_LOG(TECC_LOG_DEBUG1, "Executing KEK list query: %s", query);

    /* 执行查询 */
    cursor = tecc_fetcher_execute_query(state, query);
    pg_free(query);

    if (cursor == NULL)
    {
        TECC_LOG(TECC_LOG_WARNING, "Failed to execute KEK list query");
        return TECC_FETCHER_ERROR_QUERY;
    }

    TECC_LOG(TECC_LOG_DEBUG1, "KEK list query executed successfully, parsing results");

    /* 解析结果集为KEK元数据列表 */
    error_code = tecc_parse_kek_list(state, cursor, &kek_list);

    /* 关闭游标 */
    cursor->close(cursor);
    cursor->destroy(cursor);

    if (error_code == TECC_FETCHER_OK)
    {
        *result = kek_list;
    }

    return error_code;
}

/*
 * tecc_fetch_dek_list - 获取指定命名空间的DEK列表
 *
 * state: TECC状态结构
 * namespace_filter: 命名空间过滤条件，NULL表示所有命名空间
 * result: 输出DEK元数据列表
 * 返回: 错误码
 *
 * 设计原则: 数据获取器负责解析原始查询结果为结构化元数据对象列表。
 * 缓存模块直接存储这些已解析的对象，无需再次解析。
 */
TeccFetcherErrorCode
tecc_fetch_dek_list(const TeccState *state,
                    const char *namespace_filter,
                    List **result)
{
    TeccCursorInterface *cursor = NULL;
    char *query = NULL;
    char *escaped_namespace = NULL;
    int query_len;
    TeccFetcherErrorCode error_code = TECC_FETCHER_OK;
    List *dek_list = NULL;

    /* 参数检查 */
    if (result == NULL)
        return TECC_FETCHER_ERROR_INVALID_PARAM;

    /* 初始化输出参数 */
    *result = NULL;

    /* 转义命名空间过滤条件 */
    if (namespace_filter != NULL)
    {
        escaped_namespace = tecc_escape_string(namespace_filter);
        if (escaped_namespace == NULL)
            return TECC_FETCHER_ERROR_MEMORY;
    }

    /* 构建查询语句 */
    query_len = 500; /* 基础查询长度 - 增加因为包含多个JOIN语句 */
    if (escaped_namespace != NULL)
        query_len += strlen(escaped_namespace) * 2;

    query = (char *)pg_malloc(query_len);
    if (query == NULL)
    {
        if (escaped_namespace != NULL)
            pg_free(escaped_namespace);
        return TECC_FETCHER_ERROR_MEMORY;
    }

    if (escaped_namespace != NULL)
    {
        snprintf(query, query_len,
                 "SELECT dek_oid, dek_name, column_key_distributed_id, kek_oid, kek_name, "
                 "key_namespace_name, key_owner_oid, key_owner_name, key_acl, create_date, "
                 "function_name, arg_key, arg_value "
                 "FROM hg_dek_info "
                 "WHERE key_namespace_name = '%s' "
                 "ORDER BY dek_oid, function_name, arg_key",
                 escaped_namespace);
    }
    else
    {
        snprintf(query, query_len,
                 "SELECT dek_oid, dek_name, column_key_distributed_id, kek_oid, kek_name, "
                 "key_namespace_name, key_owner_oid, key_owner_name, key_acl, create_date, "
                 "function_name, arg_key, arg_value "
                 "FROM hg_dek_info "
                 "ORDER BY dek_oid, function_name, arg_key");
    }

    /* 释放转义字符串 */
    if (escaped_namespace != NULL)
        pg_free(escaped_namespace);

    /* 记录调试信息 */
    TECC_LOG(TECC_LOG_DEBUG1, "Executing DEK list query: %s", query);

    /* 执行查询 */
    cursor = tecc_fetcher_execute_query(state, query);
    pg_free(query);

    if (cursor == NULL)
    {
        TECC_LOG(TECC_LOG_WARNING, "Failed to execute DEK list query");
        return TECC_FETCHER_ERROR_QUERY;
    }

    TECC_LOG(TECC_LOG_DEBUG1, "DEK list query executed successfully, parsing results");

    /* 解析结果集为DEK元数据列表 */
    error_code = tecc_parse_dek_list(state, cursor, &dek_list);

    /* 关闭游标 */
    cursor->close(cursor);
    cursor->destroy(cursor);

    if (error_code == TECC_FETCHER_OK)
    {
        *result = dek_list;
    }

    return error_code;
}

/*
 * tecc_fetch_columns_by_table - 获取指定表的加密列列表
 *
 * state: TECC状态结构
 * table_oid: 表OID
 * result: 输出加密列元数据列表
 * 返回: 错误码
 *
 * 设计原则: 数据获取器负责解析原始查询结果为结构化元数据对象列表。
 * 缓存模块直接存储这些已解析的对象，无需再次解析。
 */
TeccFetcherErrorCode
tecc_fetch_columns_by_table(const TeccState *state,
                            Oid table_oid,
                            List **result)
{
    TeccCursorInterface *cursor = NULL;
    char *query = NULL;
    int query_len;
    TeccFetcherErrorCode error_code = TECC_FETCHER_OK;
    List *column_list = NULL;

    /* 参数检查 */
    if (result == NULL)
        return TECC_FETCHER_ERROR_INVALID_PARAM;

    /* 初始化输出参数 */
    *result = NULL;

    /* 构建查询语句 */
    query_len = 500; /* 基础查询长度 - 增加因为包含JOIN语句 */
    query = (char *)pg_malloc(query_len);
    if (query == NULL)
        return TECC_FETCHER_ERROR_MEMORY;

    snprintf(query, query_len,
             "SELECT table_oid, cls.relname AS table_name, column_attnum, column_name, data_type_original_oid, "
             "data_type_original_mod, dek_name, encryption_type, collation_locale, column_key_id, create_date, "
             "has_dek_privilege(column_key_id, 'USAGE') AS has_dek_permission "
             "FROM hg_encrypted_columns_info "
             "JOIN pg_class cls ON table_oid = cls.oid "
             "WHERE table_oid = %u ORDER BY column_attnum",
             table_oid);

    /* 记录调试信息 */
    TECC_LOG(TECC_LOG_DEBUG1, "Executing encrypted columns query for table OID: %u", table_oid);
    TECC_LOG(TECC_LOG_DEBUG1, "Query: %s", query);

    /* 执行查询 */
    cursor = tecc_fetcher_execute_query(state, query);
    pg_free(query);

    if (cursor == NULL)
    {
        TECC_LOG(TECC_LOG_WARNING, "Failed to execute encrypted columns query for table OID: %u", table_oid);
        return TECC_FETCHER_ERROR_QUERY;
    }

    TECC_LOG(TECC_LOG_DEBUG1, "Encrypted columns query executed successfully, parsing results");

    /* 解析结果集为加密列元数据列表 */
    error_code = tecc_parse_columns_list(cursor, &column_list);

    /* 关闭游标 */
    cursor->close(cursor);
    cursor->destroy(cursor);

    if (error_code == TECC_FETCHER_OK)
    {
        *result = column_list;
    }

    return error_code;
}

/*
 * tecc_free_kek_metadata - 释放KEK元数据内存
 *
 * kek: 要释放的KEK元数据
 */
void
tecc_free_kek_metadata(TeccKekMetadata *kek)
{
    int i;

    if (kek == NULL)
    {
        return;
    }

    /* 释放字符串字段 */
    if (kek->key_name != NULL)
        pg_free(kek->key_name);
    if (kek->key_namespace != NULL)
        pg_free(kek->key_namespace);
    if (kek->key_owner_name != NULL)
        pg_free(kek->key_owner_name);
    if (kek->key_acl != NULL)
        pg_free(kek->key_acl);

    /* 释放参数数组 */
    if (kek->args != NULL)
    {
        for (i = 0; i < kek->args_count; i++)
        {
            if (kek->args[i].function_name != NULL)
                pg_free(kek->args[i].function_name);
            if (kek->args[i].key != NULL)
                pg_free(kek->args[i].key);
            if (kek->args[i].value != NULL)
                pg_free(kek->args[i].value);
        }
        pg_free(kek->args);
    }

    /* 释放结构体 */
    pg_free(kek);
}

/*
 * tecc_free_dek_metadata - 释放DEK元数据内存
 *
 * dek: 要释放的DEK元数据
 */
void
tecc_free_dek_metadata(TeccDekMetadata *dek)
{
    int i;

    if (dek == NULL)
    {
        return;
    }

    /* 释放字符串字段 */
    if (dek->key_name != NULL)
        pg_free(dek->key_name);
    if (dek->kek_name != NULL)
        pg_free(dek->kek_name);
    if (dek->key_namespace != NULL)
        pg_free(dek->key_namespace);
    if (dek->key_owner_name != NULL)
        pg_free(dek->key_owner_name);
    if (dek->key_acl != NULL)
        pg_free(dek->key_acl);

    /* 释放参数数组 */
    if (dek->args != NULL)
    {
        for (i = 0; i < dek->args_count; i++)
        {
            if (dek->args[i].function_name != NULL)
                pg_free(dek->args[i].function_name);
            if (dek->args[i].key != NULL)
                pg_free(dek->args[i].key);
            if (dek->args[i].value != NULL)
                pg_free(dek->args[i].value);
        }
        pg_free(dek->args);
    }

    /* 释放结构体 */
    pg_free(dek);
}

/*
 * tecc_free_encrypted_column_metadata - 释放加密列元数据内存
 *
 * column: 要释放的加密列元数据
 */
void
tecc_free_encrypted_column_metadata(TeccEncryptedColumnMetadata *column)
{
    if (column == NULL)
    {
        return;
    }

    /* 释放字符串字段 */
    if (column->table_name != NULL)
        pg_free(column->table_name);
    if (column->column_name != NULL)
        pg_free(column->column_name);
    if (column->dek_name != NULL)
        pg_free(column->dek_name);
    if (column->kek_name != NULL)
        pg_free(column->kek_name);
    if (column->collation_locale != NULL)
        pg_free(column->collation_locale);

    /* 释放结构体 */
    pg_free(column);
}

/*
 * tecc_fetcher_error_message - 获取错误码对应的错误消息
 *
 * error_code: 错误码
 * 返回: 错误消息字符串
 */
const char *
tecc_fetcher_error_message(TeccFetcherErrorCode error_code)
{
    switch (error_code)
    {
        case TECC_FETCHER_OK:
            return "Success";
        case TECC_FETCHER_ERROR_CONNECTION:
            return "Database connection error - failed to establish or maintain connection";
        case TECC_FETCHER_ERROR_PERMISSION:
            return "Permission denied - insufficient privileges to access the resource";
        case TECC_FETCHER_ERROR_QUERY:
            return "Query execution error - SQL query failed to execute";
        case TECC_FETCHER_ERROR_PARSE:
            return "Result parsing error - failed to parse query results";
        case TECC_FETCHER_ERROR_MEMORY:
            return "Memory allocation error - insufficient memory available";
        case TECC_FETCHER_ERROR_TIMEOUT:
            return "Operation timeout - operation exceeded time limit";
        case TECC_FETCHER_ERROR_INVALID_PARAM:
            return "Invalid parameter - one or more parameters are invalid or missing";
        case TECC_FETCHER_ERROR_CACHE:
            return "Cache error - failed to access or update cache";
        case TECC_FETCHER_ERROR_SERIALIZATION:
            return "Serialization error - failed to serialize or deserialize data";
        case TECC_FETCHER_ERROR_VALIDATION:
            return "Validation error - data validation failed";
        case TECC_FETCHER_ERROR_CONFIG:
            return "Configuration error - invalid or missing configuration";
        case TECC_FETCHER_ERROR_INTERNAL:
            return "Internal error - unexpected internal state";
        default:
            return "Unknown error - unrecognized error code";
    }
}

/*
 * tecc_log_fetcher_error - 记录数据获取器错误
 *
 * error_code: 错误码
 * context: 错误上下文信息
 */
void
tecc_log_fetcher_error(TeccFetcherErrorCode error_code, const char *context)
{
    const char *error_msg = tecc_fetcher_error_message(error_code);
    const char *error_ctx = NULL;

    /* 增加错误计数 */
    tecc_increment_error_count(error_code);

    /* 获取错误上下文 */
    error_ctx = tecc_get_error_context();

    /* 设置详细错误信息 */
    if (context != NULL)
    {
        char detailed_msg[1024];
        if (error_ctx != NULL)
        {
            snprintf(detailed_msg, sizeof(detailed_msg),
                     "%s - Context: %s - Details: %s",
                     error_msg, error_ctx, context);
        }
        else
        {
            snprintf(detailed_msg, sizeof(detailed_msg),
                     "%s - Details: %s",
                     error_msg, context);
        }
        tecc_set_detailed_error(detailed_msg);

        /* 使用新的日志系统输出错误 */
        if (error_ctx != NULL)
        {
            TECC_LOG(TECC_LOG_ERROR, "TECC Data Fetcher Error [%d]: %s - Context: %s - Details: %s",
                     error_code, error_msg, error_ctx, context);
        }
        else
        {
            TECC_LOG(TECC_LOG_ERROR, "TECC Data Fetcher Error [%d]: %s - Details: %s",
                     error_code, error_msg, context);
        }
    }
    else
    {
        char detailed_msg[512];
        if (error_ctx != NULL)
        {
            snprintf(detailed_msg, sizeof(detailed_msg),
                     "%s - Context: %s",
                     error_msg, error_ctx);
        }
        else
        {
            snprintf(detailed_msg, sizeof(detailed_msg),
                     "%s", error_msg);
        }
        tecc_set_detailed_error(detailed_msg);

        /* 使用新的日志系统输出错误 */
        if (error_ctx != NULL)
        {
            TECC_LOG(TECC_LOG_ERROR, "TECC Data Fetcher Error [%d]: %s - Context: %s",
                     error_code, error_msg, error_ctx);
        }
        else
        {
            TECC_LOG(TECC_LOG_ERROR, "TECC Data Fetcher Error [%d]: %s",
                     error_code, error_msg);
        }
    }
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



/*
 * tecc_create_temp_connection - 创建临时数据库连接
 *
 * 仅在主连接繁忙（PQisBusy）时调用，正常运行时不应触发。
 * WARNING级别日志记录，便于追踪缓存过期等异常场景。
 *
 * state: TECC状态结构
 * caller_desc: 调用者描述（如 "permission_check:check_kek_usage"）
 * 返回: 临时连接指针，失败返回NULL
 */
/* Begin - ReqID:TECC4.0 */
PGconn *
tecc_create_temp_connection(const TeccState *state, const char *caller_desc)
{
    PGconn *conn;
    static char env_buf[] = "PGENABLETECC=false";

    if (state == NULL || state->conninfo == NULL)
    {
        TECC_LOG(TECC_LOG_ERROR, "TEMP_CONN: CREATE FAILED - invalid state or conninfo, caller=\"%s\"",
                 caller_desc ? caller_desc : "unknown");
        return NULL;
    }

    /*
     * 跳过 replication 连接：pg_basebackup 等工具使用 replication 协议，
     * 没有对应的物理数据库，temp connection 无法连接。
     */
    if (strstr(state->conninfo, "dbname='replication'") ||
        strstr(state->conninfo, "replication=database"))
    {
        TECC_LOG(TECC_LOG_DEBUG1, "TEMP_CONN: SKIP replication connection, caller=\"%s\"",
                 caller_desc ? caller_desc : "unknown");
        return NULL;
    }

    putenv(env_buf);

    conn = PQconnectdb(state->conninfo);
    if (PQstatus(conn) != CONNECTION_OK)
    {
        TECC_LOG(TECC_LOG_ERROR, "TEMP_CONN: CREATE FAILED caller=\"%s\" conn=%p error=\"%s\"",
                 caller_desc ? caller_desc : "unknown", (void *)conn, PQerrorMessage(conn));
        PQfinish(conn);
        return NULL;
    }

    conn->enable_tecc = false;
    TECC_LOG(TECC_LOG_INFO, "TEMP_CONN: CREATE caller=\"%s\" conn=%p",
             caller_desc ? caller_desc : "unknown", (void *)conn);
    return conn;
}

/*
 * tecc_close_temp_connection - 关闭临时数据库连接
 *
 * conn: 临时连接指针
 * caller_desc: 调用者描述
 */
void
tecc_close_temp_connection(PGconn *conn, const char *caller_desc)
{
    if (conn == NULL)
        return;

    TECC_LOG(TECC_LOG_INFO, "TEMP_CONN: CLOSE caller=\"%s\" conn=%p",
             caller_desc ? caller_desc : "unknown", (void *)conn);
    PQfinish(conn);
}
/* End - ReqID:TECC4.0 */

/*
 * tecc_fetcher_execute_query - 执行查询并返回游标
 *
 * state: TECC状态结构
 * query: SQL查询语句
 * 返回: 游标接口指针，NULL表示查询失败
 *
 * 注意: 游标destroy时会自动归还连接到连接池。
 * 所有数据获取器查询使用连接池的独立PGconn，绝不使用主连接。
 */
/* Begin - ReqID:TECC4.0 */
TeccCursorInterface *
tecc_fetcher_execute_query(const TeccState *state, const char *query)
{
    PGconn *conn;
    TeccCursorInterface *cursor;
    bool conn_is_temp;

    /*
     * 跳过 replication 连接：元数据查询对 replication 协议无意义，
     * 且 temp connection 无法连接到虚拟的 "replication" 数据库。
     */
    if (state != NULL && state->conninfo != NULL &&
        (strstr(state->conninfo, "dbname='replication'") ||
         strstr(state->conninfo, "replication=database")))
    {
        TECC_LOG(TECC_LOG_DEBUG1, "FETCHER: SKIP replication connection");
        return NULL;
    }

    conn = tecc_data_fetcher_get_connection(state);
    if (conn == NULL)
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_CONNECTION,
                               "Failed to get database connection from TECC state");
        return NULL;
    }

    conn_is_temp = (conn != state->conn);

    cursor = tecc_libpq_create_cursor(conn);
    if (cursor == NULL)
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_MEMORY,
                               "Failed to create cursor");
        tecc_data_fetcher_release_connection(state, conn);
        return NULL;
    }

    /* 标记连接是否为临时连接，游标销毁时据此决定是否关闭 */
    {
        TeccLibPQCursor *lp_cursor = (TeccLibPQCursor *)cursor->private_data;
        lp_cursor->conn_is_temp = conn_is_temp;
    }

    /* 执行查询 */
    if (cursor->cursor_open(cursor, query) != TECC_FETCHER_OK)
    {
        TECC_LOG(TECC_LOG_ERROR, "CURSOR OPEN FAILED: query='%s'", query);
        cursor->destroy(cursor);
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_QUERY,
                               "Failed to execute query");
        return NULL;
    }

    TECC_LOG(TECC_LOG_DEBUG1, "tecc_fetcher_execute_query returning cursor=%p", cursor);
    return cursor;
}
/* End - ReqID:TECC4.0 */


/*
 * tecc_data_fetcher_init_for_state - 为指定TECC状态初始化数据获取器
 *
 * state: TECC状态结构
 * 返回: true表示成功，false表示失败
 *
 * 数据获取器不再使用连接池。元数据查询优先使用主连接（初始化/预处理阶段主连接空闲），
 * 主连接繁忙时自动创建临时连接（tecc_create_temp_connection），用完即关。
 */
bool
tecc_data_fetcher_init_for_state(TeccState *state)
{
    if (state == NULL || state->conn == NULL)
    {
        TECC_LOG(TECC_LOG_ERROR, "Invalid TECC state or connection");
        return false;
    }

    /* 检查是否已初始化 */
    if (state->data_fetcher_initialized)
    {
        TECC_LOG(TECC_LOG_DEBUG1, "Data fetcher already initialized for this connection");
        return true;
    }

    /* 分配数据获取器管理器占位 */
    state->data_fetcher_manager = pg_malloc(sizeof(int));
    if (state->data_fetcher_manager == NULL)
    {
        TECC_LOG(TECC_LOG_ERROR, "Failed to allocate memory for data fetcher manager");
        return false;
    }

    state->data_fetcher_initialized = true;
    TECC_LOG(TECC_LOG_DEBUG1, "Data fetcher initialized for connection %p", state->conn);

    return true;
}

/*
 * tecc_data_fetcher_cleanup_for_state - 清理指定TECC状态的数据获取器
 *
 * state: TECC状态结构
 */
void
tecc_data_fetcher_cleanup_for_state(TeccState *state)
{
    if (state == NULL || !state->data_fetcher_initialized)
    {
        return;
    }

    /* 清理数据获取器管理器 */
    if (state->data_fetcher_manager != NULL)
    {
        pg_free(state->data_fetcher_manager);
        state->data_fetcher_manager = NULL;
    }

    state->data_fetcher_initialized = false;
    TECC_LOG(TECC_LOG_DEBUG1, "Data fetcher cleaned up for connection %p", state->conn);
}

/*
 * tecc_data_fetcher_get_connection - 从TECC状态获取数据库连接
 *
 * state: TECC状态结构
 * 返回: 数据库连接指针，NULL表示无法获取连接
 *
 * 始终创建临时连接。不能使用主连接，因为PQexec会触发TECC hook
 * 拦截内部元数据查询，导致无限递归。
 */
/* Begin - ReqID:TECC4.0 */
PGconn *
tecc_data_fetcher_get_connection(const TeccState *state)
{
    if (state == NULL || state->conn == NULL)
    {
        TECC_LOG(TECC_LOG_ERROR, "Invalid TECC state or connection");
        return NULL;
    }

    if (!state->data_fetcher_initialized)
    {
        TECC_LOG(TECC_LOG_ERROR, "Data fetcher not initialized for this connection");
        return NULL;
    }

    return tecc_create_temp_connection(state, "get_connection");
}
/* End - ReqID:TECC4.0 */

/*
 * tecc_data_fetcher_release_connection - 释放通过get_connection获取的连接
 *
 * state: TECC状态结构
 * conn: 要释放的连接
 *
 * 临时连接会被关闭（WARNING日志），主连接不做任何操作。
 */
/* Begin - ReqID:TECC4.0 */
void
tecc_data_fetcher_release_connection(const TeccState *state, PGconn *conn)
{
    if (conn == NULL || conn == state->conn)
        return;  /* 主连接，无需操作 */

    tecc_close_temp_connection(conn, "release_connection");
}
/* End - ReqID:TECC4.0 */

/*
 * tecc_fetch_encrypted_column_metadata_by_name - 按名称获取加密列元数据
 *
 * state: TECC状态结构
 * database_name: 数据库名称
 * schema_name: 模式名称
 * table_name: 表名称
 * column_name: 列名称
 * result: 输出加密列元数据指针
 * 返回: 错误码
 */
TeccFetcherErrorCode
tecc_fetch_encrypted_column_metadata_by_name(const TeccState *state,
                                            const char *database_name,
                                            const char *schema_name,
                                            const char *table_name,
                                            const char *column_name,
                                            TeccEncryptedColumnMetadata **result)
{
    TeccEncryptedColumnMetadata *column_metadata = NULL;
    TeccCursorInterface *cursor = NULL;
    char query[1024]; /* 增加缓冲区大小以容纳更复杂的查询 */
    char *escaped_dbname = NULL;
    char *escaped_schema = NULL;
    char *escaped_table = NULL;
    char *escaped_column = NULL;
    TeccFetcherErrorCode error_code = TECC_FETCHER_OK;

    /* 参数检查 */
    if (database_name == NULL || schema_name == NULL ||
        table_name == NULL || column_name == NULL || result == NULL)
    {
        return TECC_FETCHER_ERROR_INVALID_PARAM;
    }

    /* 初始化输出参数 */
    *result = NULL;

    /* 转义输入字符串 */
    escaped_dbname = tecc_escape_string(database_name);
    escaped_schema = tecc_escape_string(schema_name);
    escaped_table = tecc_escape_string(table_name);
    escaped_column = tecc_escape_string(column_name);

    if (escaped_dbname == NULL || escaped_schema == NULL ||
        escaped_table == NULL || escaped_column == NULL)
    {
        if (escaped_dbname) pg_free(escaped_dbname);
        if (escaped_schema) pg_free(escaped_schema);
        if (escaped_table) pg_free(escaped_table);
        if (escaped_column) pg_free(escaped_column);
        return TECC_FETCHER_ERROR_MEMORY;
    }

    /* 构建查询语句 - 通过名称查找加密列，只返回解析函数期望的10个字段 */
    /* 注意：需要添加数据库名条件，通过current_database()函数获取当前数据库名 */
    snprintf(query, sizeof(query),
             "SELECT table_oid, cls.relname AS table_name, column_attnum, column_name, data_type_original_oid, "
             "data_type_original_mod, dek_name, encryption_type, collation_locale, column_key_id, create_date, "
             "has_dek_privilege(column_key_id, 'USAGE') AS has_dek_permission "
             "FROM hg_encrypted_columns_info "
             "JOIN pg_class cls ON table_oid = cls.oid "
             "JOIN pg_namespace ns ON cls.relnamespace = ns.oid "
             "WHERE cls.relname = '%s' AND ns.nspname = '%s' AND column_name = '%s' "
             "AND current_database() = '%s'",
             escaped_table, escaped_schema, escaped_column, escaped_dbname);

    /* 清理转义字符串 */
    pg_free(escaped_dbname);
    pg_free(escaped_schema);
    pg_free(escaped_table);
    pg_free(escaped_column);

    /* 执行查询 */
    cursor = tecc_fetcher_execute_query(state, query);
    if (cursor == NULL)
    {
        return TECC_FETCHER_ERROR_QUERY;
    }

    /* 移动到第一行 */
    if (cursor->next(cursor) != TECC_FETCHER_OK)
    {
        TECC_LOG(TECC_LOG_DEBUG1, "No encrypted column found by name for %s.%s.%s.%s",
                 database_name, schema_name, table_name, column_name);
        cursor->close(cursor);
        cursor->destroy(cursor);
        return TECC_FETCHER_ERROR_NOT_FOUND;
    }

    /* 解析结果 */
    error_code = tecc_parse_encrypted_column_metadata(cursor, &column_metadata);

    /* 关闭游标 */
    cursor->close(cursor);
    cursor->destroy(cursor);

    if (error_code == TECC_FETCHER_OK && column_metadata != NULL)
    {
        *result = column_metadata;
    }

    return error_code;
}

/*
 * tecc_fetch_columns_by_table_by_name - 按名称获取表中所有加密列元数据
 *
 * state: TECC状态结构
 * database_name: 数据库名称
 * schema_name: 模式名称
 * table_name: 表名称
 * result: 输出加密列元数据列表
 * 返回: 错误码
 */
TeccFetcherErrorCode
tecc_fetch_columns_by_table_by_name(const TeccState *state,
                                   const char *database_name,
                                   const char *schema_name,
                                   const char *table_name,
                                   List **result)
{
    TeccCursorInterface *cursor = NULL;
    char query[1024]; /* 增加缓冲区大小 */
    char *escaped_dbname = NULL;
    char *escaped_schema = NULL;
    char *escaped_table = NULL;
    TeccFetcherErrorCode error_code = TECC_FETCHER_OK;
    List *column_list = NULL;

    /* 参数检查 */
    if (database_name == NULL || schema_name == NULL ||
        table_name == NULL || result == NULL)
    {
        return TECC_FETCHER_ERROR_INVALID_PARAM;
    }

    /* 初始化输出参数 */
    *result = NULL;

    /* 转义输入字符串 */
    escaped_dbname = tecc_escape_string(database_name);
    escaped_schema = tecc_escape_string(schema_name);
    escaped_table = tecc_escape_string(table_name);

    if (escaped_dbname == NULL || escaped_schema == NULL || escaped_table == NULL)
    {
        if (escaped_dbname) pg_free(escaped_dbname);
        if (escaped_schema) pg_free(escaped_schema);
        if (escaped_table) pg_free(escaped_table);
        return TECC_FETCHER_ERROR_MEMORY;
    }

    /* 构建查询语句 - 通过名称查找表中所有加密列，只返回解析函数期望的10个字段 */
    TECC_LOG(TECC_LOG_DEBUG1, "DEBUG LOOP: Building query for table %s.%s", escaped_schema, escaped_table);
    snprintf(query, sizeof(query),
             "SELECT table_oid, cls.relname AS table_name, column_attnum, column_name, data_type_original_oid, "
             "data_type_original_mod, dek_name, encryption_type, collation_locale, column_key_id, create_date, "
             "has_dek_privilege(column_key_id, 'USAGE') AS has_dek_permission "
             "FROM hg_encrypted_columns_info "
             "JOIN pg_class cls ON table_oid = cls.oid "
             "JOIN pg_namespace ns ON cls.relnamespace = ns.oid "
             "WHERE cls.relname = '%s' AND ns.nspname = '%s' "
             "AND current_database() = '%s' "
             "ORDER BY column_attnum",
             escaped_table, escaped_schema, escaped_dbname);
    TECC_LOG(TECC_LOG_DEBUG1, "DEBUG LOOP: Query: %s", query);

    /* 清理转义字符串 */
    pg_free(escaped_dbname);
    pg_free(escaped_schema);
    pg_free(escaped_table);

    /* 执行查询 */
    TECC_LOG(TECC_LOG_DEBUG1, "DEBUG LOOP: Calling tecc_fetcher_execute_query");
    cursor = tecc_fetcher_execute_query(state, query);
    TECC_LOG(TECC_LOG_DEBUG1, "DEBUG LOOP: tecc_fetcher_execute_query returned: cursor=%p", cursor);
    if (cursor == NULL)
    {
        TECC_LOG(TECC_LOG_DEBUG1, "DEBUG LOOP: tecc_fetcher_execute_query failed, returning TECC_FETCHER_ERROR_QUERY");
        return TECC_FETCHER_ERROR_QUERY;
    }

    /* TEST: Check if cursor is valid */
    TECC_LOG(TECC_LOG_DEBUG1, "DEBUG LOOP: cursor->fetch_next=%p, cursor->close=%p",
            cursor ? cursor->fetch_next : NULL, cursor ? cursor->close : NULL);

    /* 解析结果列表 */
    TECC_LOG(TECC_LOG_DEBUG1, "DEBUG LOOP: Calling tecc_parse_columns_list");
    error_code = tecc_parse_columns_list(cursor, &column_list);
    TECC_LOG(TECC_LOG_DEBUG1, "DEBUG LOOP: tecc_parse_columns_list returned: error_code=%d, column_list=%p", error_code, column_list);

    /* 关闭游标 */
    cursor->close(cursor);
    cursor->destroy(cursor);

    if (error_code == TECC_FETCHER_OK && column_list != NULL)
    {
        *result = column_list;
        TECC_LOG(TECC_LOG_DEBUG1, "DEBUG LOOP: Successfully fetched %d columns", list_length(column_list));
    }
    else
    {
        TECC_LOG(TECC_LOG_DEBUG1, "DEBUG LOOP: Failed to fetch columns or empty result");
    }

    return error_code;
}

/*
 * tecc_username_to_oid - 将用户名转换为OID
 *
 * state: TECC状态结构
 * username: 用户名
 * oid: 输出参数，存储转换后的OID
 * 返回: 错误码，TECC_FETCHER_OK表示成功
 *
 * 注意: 此函数查询pg_roles系统视图将用户名转换为OID。
 * 需要临时禁用TECC处理以避免递归。
 */
TeccFetcherErrorCode
tecc_username_to_oid(const TeccState *state, const char *username, Oid *oid)
{
    TeccCursorInterface *cursor = NULL;
    char query[256];
    char *escaped_username = NULL;
    const char *oid_str = NULL;
    TeccFetcherErrorCode error_code = TECC_FETCHER_OK;

    /* 参数检查 */
    if (username == NULL || oid == NULL)
    {
        return TECC_FETCHER_ERROR_INVALID_PARAM;
    }

    /* 初始化输出参数 */
    *oid = InvalidOid;

    /* 转义用户名 */
    escaped_username = tecc_escape_string(username);
    if (escaped_username == NULL)
    {
        return TECC_FETCHER_ERROR_MEMORY;
    }

    /* 构建查询语句 - 查询pg_roles系统视图 */
    snprintf(query, sizeof(query),
             "SELECT oid FROM pg_roles WHERE rolname = '%s'",
             escaped_username);

    /* 清理转义字符串 */
    pg_free(escaped_username);

    /* 执行查询 - 需要临时禁用TECC处理 */
    cursor = tecc_fetcher_execute_query(state, query);
    if (cursor == NULL)
    {
        return TECC_FETCHER_ERROR_QUERY;
    }

    /* 获取结果 */
    if (cursor->fetch_next(cursor) == 0) /* 0表示有数据 */
    {
        oid_str = cursor->get_column(cursor, 0);
        if (oid_str != NULL)
        {
            *oid = (Oid)strtoul(oid_str, NULL, 10);
        }
        else
        {
            error_code = TECC_FETCHER_ERROR_NOT_FOUND;
        }
    }
    else
    {
        /* 未找到用户 */
        error_code = TECC_FETCHER_ERROR_NOT_FOUND;
    }

    /* 关闭游标 */
    cursor->close(cursor);
    cursor->destroy(cursor);

    return error_code;
}

/*
 * tecc_oid_to_username - 将OID转换为用户名
 *
 * state: TECC状态结构
 * oid: 用户OID
 * username: 输出参数，存储转换后的用户名（需要调用者释放）
 * 返回: 错误码，TECC_FETCHER_OK表示成功
 *
 * 注意: 此函数查询pg_roles系统视图将OID转换为用户名。
 * 需要临时禁用TECC处理以避免递归。
 */
TeccFetcherErrorCode
tecc_oid_to_username(const TeccState *state, Oid oid, char **username)
{
    TeccCursorInterface *cursor = NULL;
    char query[256];
    const char *username_str = NULL;
    TeccFetcherErrorCode error_code = TECC_FETCHER_OK;

    /* 参数检查 */
    if (username == NULL || oid == InvalidOid)
    {
        return TECC_FETCHER_ERROR_INVALID_PARAM;
    }

    /* 初始化输出参数 */
    *username = NULL;

    /* 构建查询语句 - 查询pg_roles系统视图 */
    snprintf(query, sizeof(query),
             "SELECT rolname FROM pg_roles WHERE oid = %u",
             oid);

    /* 执行查询 - 需要临时禁用TECC处理 */
    cursor = tecc_fetcher_execute_query(state, query);
    if (cursor == NULL)
    {
        return TECC_FETCHER_ERROR_QUERY;
    }

    /* 获取结果 */
    if (cursor->fetch_next(cursor) == 0) /* 0表示有数据 */
    {
        username_str = cursor->get_column(cursor, 0);
        if (username_str != NULL)
        {
            *username = pg_strdup(username_str);
            if (*username == NULL)
            {
                error_code = TECC_FETCHER_ERROR_MEMORY;
            }
        }
        else
        {
            error_code = TECC_FETCHER_ERROR_NOT_FOUND;
        }
    }
    else
    {
        /* 未找到用户 */
        error_code = TECC_FETCHER_ERROR_NOT_FOUND;
    }

    /* 关闭游标 */
    cursor->close(cursor);
    cursor->destroy(cursor);

    return error_code;
}

/* End - ReqID:TECC4.0 */