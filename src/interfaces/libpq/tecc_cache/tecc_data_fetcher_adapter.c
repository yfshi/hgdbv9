/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_data_fetcher_adapter.c
 * Abstract:
 *   TECC数据获取器适配器实现，基于简化设计的两层架构。
 *   提供与数据获取器模块的集成接口，实现委托加载机制。
 *   更新：去除工厂模式，使用TeccState参数，与数据获取器实际接口一致。
 *
 * Authored by <wangxp@highgo.com>, 2026-04-16.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_cache/tecc_data_fetcher_adapter.c
 */

#include "postgres_fe.h"
#include "tecc_metadata_cache.h"
#include "tecc_data_fetcher_adapter.h"
#include "../tecc_data_fetcher/include/tecc_data_fetcher.h"
#include "../tecc_data_fetcher/include/tecc_data_fetcher_types.h"
#include "../tecc_common/tecc_frontend_list.h"
#include "../tecc_libpq_integration/fe-tecc-integration.h"  /* 包含TeccState完整定义 */
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>  /* For snprintf */

/* PostgreSQL头文件，包含OidIsValid等宏 */
#include "c.h"

/* 十六进制编码/解码工具 - 使用前端格式化器 */
#include "../tecc_formatter/tecc_formatter.h"

/* 前向声明数据获取器相关结构，避免头文件冲突 */
typedef struct TeccState TeccState;

/* 使用数据获取器头文件中的类型定义 */
/* 这些类型在tecc_data_fetcher_types.h中定义 */

/* 数据获取器函数前向声明 */
/* TeccFetcherErrorCode枚举已在tecc_data_fetcher.h中定义 */

/* 定义TeccDataFetcherFactory为void指针 */
typedef void* TeccDataFetcherFactory;

/* 数据获取器函数前向声明 - 使用数据获取器头文件中的类型 */
/* 这些函数在tecc_data_fetcher.h中声明，使用TeccKekMetadata和TeccDekMetadata类型 */

/* 简化的适配器函数原型声明 */
TeccFetcherErrorCode tecc_fetch_kek_for_cache(const TeccState *state,
                                             const TeccCacheQueryParams *query,
                                             TeccKekCacheItem **result,
                                             int *count);
TeccFetcherErrorCode tecc_fetch_dek_for_cache(const TeccState *state,
                                             const TeccCacheQueryParams *query,
                                             TeccDekCacheItem **result,
                                             int *count);
TeccFetcherErrorCode tecc_fetch_column_for_cache(const TeccState *state,
                                                const TeccCacheQueryParams *query,
                                                TeccColumnCacheItem **result,
                                                int *count);
TeccFetcherErrorCode tecc_fetch_guc_for_cache(const TeccState *state,
                                             TeccCacheManager *manager);


/* 内部函数声明 */

static int convert_fetcher_error_to_cache_error(int fetcher_error);
static TeccKekCacheItem *convert_kek_metadata_to_cache_item(const TeccState *state, const void *kek_metadata);
static TeccDekCacheItem *convert_dek_metadata_to_cache_item(const TeccState *state, const void *dek_metadata);
static TeccColumnCacheItem *convert_column_metadata_to_cache_item(const TeccState *state, const void *column_metadata);
static void free_kek_cache_item(TeccKekCacheItem *item);
static void free_dek_cache_item(TeccDekCacheItem *item);
static void free_column_cache_item(TeccColumnCacheItem *item);

/* 简化接口的内部辅助函数 */
static TeccFetcherErrorCode fetch_kek_by_name(const TeccState *state,
                                             const char *key_name,
                                             const char *key_namespace,
                                             PGconn *ext_conn,
                                             TeccKekCacheItem **result);
static TeccFetcherErrorCode fetch_kek_batch(const TeccState *state,
                                           const char *namespace_filter,
                                           TeccKekCacheItem ***result,
                                           int *count);
static TeccFetcherErrorCode fetch_dek_by_name(const TeccState *state,
                                             const char *key_name,
                                             const char *key_namespace,
                                             PGconn *ext_conn,
                                             TeccDekCacheItem **result);
static TeccFetcherErrorCode fetch_dek_by_oid(const TeccState *state,
                                             Oid dek_oid,
                                             TeccDekCacheItem **result);
static TeccFetcherErrorCode fetch_dek_batch(const TeccState *state,
                                           const char *namespace_filter,
                                           TeccDekCacheItem ***result,
                                           int *count);
static TeccFetcherErrorCode fetch_column_by_oid(const TeccState *state,
                                               Oid table_oid,
                                               int column_attnum,
                                               TeccColumnCacheItem **result);
static TeccFetcherErrorCode fetch_column_by_name(const TeccState *state,
                                                const char *database_name,
                                                const char *schema_name,
                                                const char *table_name,
                                                const char *column_name,
                                                TeccColumnCacheItem **result);
static TeccFetcherErrorCode fetch_columns_batch_by_oid(const TeccState *state,
                                                      Oid table_oid,
                                                      TeccColumnCacheItem ***result,
                                                      int *count);
static TeccFetcherErrorCode fetch_columns_batch_by_name(const TeccState *state,
                                                       const char *database_name,
                                                       const char *schema_name,
                                                       const char *table_name,
                                                       TeccColumnCacheItem ***result,
                                                       int *count);

/*
 * tecc_fetch_kek_metadata_for_cache - 为缓存模块获取KEK元数据
 *
 * 参数:
 *   state - TECC状态结构，包含连接和上下文信息
 *   key_name - KEK名称
 *   key_namespace - 命名空间
 *   kek_item - 返回的KEK缓存项指针
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
static int tecc_fetch_kek_metadata_for_cache(const TeccState *state,
                                  const char *key_name,
                                  const char *key_namespace,
                                  PGconn *ext_conn,
                                  TeccKekCacheItem **kek_item)
{
    TeccKekMetadata *kek_metadata = NULL;
    TeccFetcherErrorCode fetcher_error;

    /* 参数检查 */
    if (state == NULL || key_name == NULL || kek_item == NULL)
    {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    *kek_item = NULL;

    /* 调用数据获取器获取KEK元数据 */
    fetcher_error = tecc_fetch_kek_metadata(state, key_name, key_namespace, ext_conn, &kek_metadata);
    if (fetcher_error != TECC_FETCHER_OK)
    {
        return convert_fetcher_error_to_cache_error(fetcher_error);
    }

    if (kek_metadata == NULL)
    {
        return TECC_CACHE_ERROR_NOT_FOUND;
    }

    /* 转换数据结构 */
    TECC_LOG(TECC_LOG_DEBUG1, "fetch_kek_by_name: Before conversion, kek_metadata->kek_oid = %u", kek_metadata->kek_oid);
    *kek_item = convert_kek_metadata_to_cache_item(state, kek_metadata);
    if (*kek_item != NULL) {
        TECC_LOG(TECC_LOG_DEBUG1, "fetch_kek_by_name: After conversion, (*kek_item)->kek_oid = %u", (*kek_item)->kek_oid);
    }

    /* 释放数据获取器分配的内存 */
    tecc_free_kek_metadata(kek_metadata);

    if (*kek_item == NULL)
    {
        return TECC_CACHE_ERROR_CONVERSION;
    }

    return TECC_CACHE_OK;
}

/*
 * tecc_fetch_dek_metadata_for_cache - 为缓存模块获取DEK元数据
 *
 * 参数:
 *   state - TECC状态结构，包含连接和上下文信息
 *   key_name - DEK名称
 *   key_namespace - 命名空间
 *   dek_item - 返回的DEK缓存项指针
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
static int tecc_fetch_dek_metadata_for_cache(const TeccState *state,
                                  const char *key_name,
                                  const char *key_namespace,
                                  PGconn *ext_conn,
                                  TeccDekCacheItem **dek_item)
{
    TeccDekMetadata *dek_metadata = NULL;
    TeccFetcherErrorCode fetcher_error;

    /* 参数检查 */
    if (state == NULL || key_name == NULL || dek_item == NULL)
    {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    *dek_item = NULL;

    /* 调用数据获取器获取DEK元数据 */
    fetcher_error = tecc_fetch_dek_metadata(state, key_name, key_namespace, ext_conn, &dek_metadata);
    if (fetcher_error != TECC_FETCHER_OK)
    {
        return convert_fetcher_error_to_cache_error(fetcher_error);
    }

    if (dek_metadata == NULL)
    {
        return TECC_CACHE_ERROR_NOT_FOUND;
    }

    /* 转换数据结构 */
    *dek_item = convert_dek_metadata_to_cache_item(state, dek_metadata);

    /* 释放数据获取器分配的内存 */
    tecc_free_dek_metadata(dek_metadata);

    if (*dek_item == NULL)
    {
        return TECC_CACHE_ERROR_CONVERSION;
    }

    return TECC_CACHE_OK;
}

/*
 * tecc_fetch_dek_metadata_for_cache_by_oid - 按OID为缓存模块获取DEK元数据
 *
 * 参数:
 *   state - TECC状态结构，包含连接和上下文信息
 *   dek_oid - DEK对象标识符
 *   dek_item - 返回的DEK缓存项指针
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
static int tecc_fetch_dek_metadata_for_cache_by_oid(const TeccState *state,
                                          Oid dek_oid,
                                          TeccDekCacheItem **dek_item)
{
    TeccDekMetadata *dek_metadata = NULL;
    TeccFetcherErrorCode fetcher_error;

    /* 参数检查 */
    if (state == NULL || dek_item == NULL)
    {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    *dek_item = NULL;

    /* 调用数据获取器按OID获取DEK元数据 */
    fetcher_error = tecc_fetch_dek_metadata_by_oid(state, dek_oid, &dek_metadata);
    if (fetcher_error != TECC_FETCHER_OK)
    {
        return convert_fetcher_error_to_cache_error(fetcher_error);
    }

    if (dek_metadata == NULL)
    {
        return TECC_CACHE_ERROR_NOT_FOUND;
    }

    /* 转换数据结构 */
    *dek_item = convert_dek_metadata_to_cache_item(state, dek_metadata);

    /* 释放数据获取器分配的内存 */
    tecc_free_dek_metadata(dek_metadata);

    if (*dek_item == NULL)
    {
        return TECC_CACHE_ERROR_CONVERSION;
    }

    return TECC_CACHE_OK;
}

/*
 * tecc_fetch_column_metadata_for_cache - 为缓存模块获取加密列元数据
 *
 * 参数:
 *   state - TECC状态结构，包含连接和上下文信息
 *   table_oid - 表对象标识符
 *   column_attnum - 列号
 *   column_item - 返回的加密列缓存项指针
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
static int tecc_fetch_column_metadata_for_cache(const TeccState *state,
                                     Oid table_oid,
                                     int column_attnum,
                                     TeccColumnCacheItem **column_item)
{
    TeccEncryptedColumnMetadata *column_metadata = NULL;
    TeccFetcherErrorCode fetcher_error;

    /* 参数检查 */
    if (state == NULL || column_item == NULL)
    {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    *column_item = NULL;

    /* 调用数据获取器获取加密列元数据 */
    fetcher_error = tecc_fetch_encrypted_column_metadata(state, table_oid, column_attnum, &column_metadata);
    if (fetcher_error != TECC_FETCHER_OK)
    {
        return convert_fetcher_error_to_cache_error(fetcher_error);
    }

    if (column_metadata == NULL)
    {
        return TECC_CACHE_ERROR_NOT_FOUND;
    }

    /* 转换数据结构 */
    *column_item = convert_column_metadata_to_cache_item(state, column_metadata);

    /* 释放数据获取器分配的内存 */
    tecc_free_encrypted_column_metadata(column_metadata);

    if (*column_item == NULL)
    {
        return TECC_CACHE_ERROR_CONVERSION;
    }

    return TECC_CACHE_OK;
}

/*
 * tecc_fetch_column_metadata_by_name_for_cache - 为缓存模块按名称获取加密列元数据
 *
 * 参数:
 *   state - TECC状态结构，包含连接和上下文信息
 *   database_name - 数据库名称
 *   schema_name - 模式名称
 *   table_name - 表名称
 *   column_name - 列名称
 *   column_item - 返回的加密列缓存项指针
 *
 * 返回:
 *   0表示成功，非0表示错误码
 *
 * 实现说明:
 *   使用两套哈希表架构后，可以直接通过名称查询数据库，无需先转换为OID。
 *   数据获取器需要提供按名称查询的接口，返回包含OID和名称的完整缓存项。
 */

static int tecc_fetch_column_metadata_by_name_for_cache(const TeccState *state,
                                             const char *database_name,
                                             const char *schema_name,
                                             const char *table_name,
                                             const char *column_name,
                                             TeccColumnCacheItem **column_item)
{
    TeccEncryptedColumnMetadata *column_metadata = NULL;
    TeccFetcherErrorCode fetcher_error;

    /* 参数检查 */
    if (state == NULL || database_name == NULL || schema_name == NULL ||
        table_name == NULL || column_name == NULL || column_item == NULL)
    {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    /* 初始化输出参数 */
    *column_item = NULL;

    /* 调用数据获取器的按名称查询接口 */
    fetcher_error = tecc_fetch_encrypted_column_metadata_by_name(state,
                                                                database_name,
                                                                schema_name,
                                                                table_name,
                                                                column_name,
                                                                &column_metadata);
    if (fetcher_error != TECC_FETCHER_OK || column_metadata == NULL)
    {
        return convert_fetcher_error_to_cache_error(fetcher_error);
    }

    /* 转换数据获取器返回的元数据为缓存项 */
    *column_item = convert_column_metadata_to_cache_item(state, column_metadata);
    if (*column_item == NULL)
    {
        /* 转换失败，清理资源 */
        tecc_free_encrypted_column_metadata(column_metadata);
        return TECC_CACHE_ERROR_CONVERSION;
    }

    /* 释放数据获取器分配的内存 */
    tecc_free_encrypted_column_metadata(column_metadata);

    /* 确保缓存项包含完整的名称信息 */
    if ((*column_item)->database_name == NULL) {
        (*column_item)->database_name = pg_strdup(database_name);
    }
    if ((*column_item)->schema_name == NULL) {
        (*column_item)->schema_name = pg_strdup(schema_name);
    }
    if ((*column_item)->table_name == NULL) {
        (*column_item)->table_name = pg_strdup(table_name);
    }
    if ((*column_item)->column_name == NULL) {
        (*column_item)->column_name = pg_strdup(column_name);
    }

    /* 补填OID字段（如果为空） - 使用数据获取器的连接，需临时禁用TECC避免递归 */
    if (((*column_item)->schema_oid == InvalidOid || (*column_item)->database_oid == InvalidOid) &&
        state->conn != NULL)
    {
        char oid_query[256];
        PGresult *res;

        tecc_disable_once(state->conn);

        /* 查询schema OID */
        if ((*column_item)->schema_oid == InvalidOid && schema_name != NULL)
        {
            snprintf(oid_query, sizeof(oid_query),
                     "SELECT oid FROM pg_namespace WHERE nspname = '%s'",
                     schema_name);
            res = PQexec(state->conn, oid_query);
            if (res && PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
            {
                (*column_item)->schema_oid = (Oid)atol(PQgetvalue(res, 0, 0));
            }
            if (res) PQclear(res);
        }

        /* 查询database OID */
        if ((*column_item)->database_oid == InvalidOid)
        {
            snprintf(oid_query, sizeof(oid_query),
                     "SELECT oid FROM pg_database WHERE datname = '%s'",
                     database_name);
            res = PQexec(state->conn, oid_query);
            if (res && PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
            {
                (*column_item)->database_oid = (Oid)atol(PQgetvalue(res, 0, 0));
            }
            if (res) PQclear(res);
        }

        tecc_reset_disable_once(state->conn);
    }

    return TECC_CACHE_OK;
}

/*
 * tecc_fetch_all_columns_for_table_for_cache - 为缓存模块获取表中所有加密列元数据
 *
 * 参数:
 *   state - TECC状态结构，包含连接和上下文信息
 *   table_oid - 表对象标识符
 *   column_items - 返回的加密列缓存项指针数组
 *   count - 返回的加密列数量
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */

static int tecc_fetch_all_columns_for_table_for_cache(const TeccState *state,
                                           Oid table_oid,
                                           TeccColumnCacheItem ***column_items,
                                           int *count)
{
    List *column_list = NULL;
    TeccFetcherErrorCode fetcher_error;
    int i;
    int column_count = 0;
    TeccColumnCacheItem **items = NULL;
    ListCell *cell = NULL;

    /* 参数检查 */
    if (state == NULL || column_items == NULL || count == NULL)
    {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    *column_items = NULL;
    *count = 0;

    /* 调用数据获取器批量获取加密列元数据 */
    fetcher_error = tecc_fetch_columns_by_table(state, table_oid, &column_list);
    if (fetcher_error != TECC_FETCHER_OK)
    {
        /* 转换错误码并返回 */
        return convert_fetcher_error_to_cache_error(fetcher_error);
    }

    /* 检查是否获取到数据 */
    if (column_list == NULL)
    {
        return TECC_CACHE_OK;  /* 没有加密列，返回成功但数量为0 */
    }

    /* 获取列表长度 */
    column_count = list_length(column_list);
    if (column_count == 0)
    {
        /* 列表为空，释放列表并返回成功 */
        list_free(column_list);
        return TECC_CACHE_OK;
    }

    /* 分配结果数组 */
    items = (TeccColumnCacheItem **)pg_malloc(column_count * sizeof(TeccColumnCacheItem *));
    if (items == NULL)
    {
        list_free(column_list);
        return TECC_CACHE_ERROR_MEMORY;
    }

    /* 转换每个列元数据 */
    i = 0;
    foreach(cell, column_list)
    {
        TeccEncryptedColumnMetadata *column_metadata = (TeccEncryptedColumnMetadata *)lfirst(cell);

        if (column_metadata != NULL)
        {
            items[i] = convert_column_metadata_to_cache_item(state, column_metadata);
            if (items[i] == NULL)
            {
                /* 转换失败，清理已分配的资源 */
                {
                    int j;
                    for (j = 0; j < i; j++)
                    {
                        free_column_cache_item(items[j]);
                    }
                }
                pg_free(items);
                list_free(column_list);
                return TECC_CACHE_ERROR_CONVERSION;
            }
        }
        else
        {
            items[i] = NULL;
        }
        i++;
    }

    /* 设置输出参数 */
    *column_items = items;
    *count = column_count;

    /* 释放数据获取器分配的内存 */
    list_free(column_list);

    return TECC_CACHE_OK;
}

/*
 * tecc_fetch_all_columns_for_table_by_name_for_cache - 为缓存模块按名称获取表中所有加密列元数据
 *
 * 参数:
 *   state - TECC状态结构，包含连接和上下文信息
 *   database_name - 数据库名称
 *   schema_name - 模式名称
 *   table_name - 表名称
 *   column_items - 返回的加密列缓存项指针数组
 *   count - 返回的加密列数量
 *
 * 返回:
 *   0表示成功，非0表示错误码
 *
 * 实现说明:
 *   使用两套哈希表架构后，可以直接通过名称查询数据库，无需先转换为OID。
 *   数据获取器需要提供按名称批量查询的接口，返回包含OID和名称的完整缓存项数组。
 */
int
tecc_fetch_all_columns_for_table_by_name_for_cache(const TeccState *state,
                                                   const char *database_name,
                                                   const char *schema_name,
                                                   const char *table_name,
                                                   TeccColumnCacheItem ***column_items,
                                                   int *count)
{
    TeccFetcherErrorCode fetcher_error;
    int i;
    List *column_list = NULL;
    ListCell *cell = NULL;

    /* 参数检查 */
    if (state == NULL || database_name == NULL || schema_name == NULL ||
        table_name == NULL || column_items == NULL || count == NULL)
    {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    /* 初始化输出参数 */
    *column_items = NULL;
    *count = 0;

    /* 调用数据获取器的按名称批量查询接口 */
    TECC_LOG(TECC_LOG_DEBUG1, "DEBUG LOOP: Calling tecc_fetch_columns_by_table_by_name for %s.%s.%s",
            database_name, schema_name, table_name);
    fetcher_error = tecc_fetch_columns_by_table_by_name(state,
                                                       database_name,
                                                       schema_name,
                                                       table_name,
                                                       &column_list);
    TECC_LOG(TECC_LOG_DEBUG1, "DEBUG LOOP: tecc_fetch_columns_by_table_by_name returned: fetcher_error=%d, column_list=%p",
            fetcher_error, column_list);
    if (fetcher_error != TECC_FETCHER_OK || column_list == NULL)
    {
        TECC_LOG(TECC_LOG_DEBUG1, "DEBUG LOOP: tecc_fetch_columns_by_table_by_name failed, returning error");
        return convert_fetcher_error_to_cache_error(fetcher_error);
    }

    /* 获取列表长度 */
    *count = list_length(column_list);
    if (*count <= 0)
    {
        list_free(column_list);
        return TECC_CACHE_ERROR_NOT_FOUND;
    }

    /* 分配缓存项数组 */
    *column_items = (TeccColumnCacheItem **)pg_malloc(sizeof(TeccColumnCacheItem *) * (*count));
    if (*column_items == NULL)
    {
        /* 清理数据获取器返回的元数据数组 */
        /* free_column_metadata_array(column_metadatas, *count); */
        return TECC_CACHE_ERROR_MEMORY;
    }

    /* 转换每个元数据为缓存项 */
    i = 0;
    cell = NULL;
    foreach(cell, column_list) {
        TeccEncryptedColumnMetadata *column_metadata = (TeccEncryptedColumnMetadata *)lfirst(cell);

        TECC_LOG(TECC_LOG_DEBUG1, "DEBUG LOOP: Converting column metadata %p, column_name=%s, dek_name=%s",
                column_metadata,
                column_metadata ? column_metadata->column_name : "(null)",
                column_metadata && column_metadata->dek_name ? column_metadata->dek_name : "(null)");

        (*column_items)[i] = convert_column_metadata_to_cache_item(state, column_metadata);
        if ((*column_items)[i] == NULL)
        {
            /* 转换失败，清理已分配的资源 */
            {
                int j;
                for (j = 0; j < i; j++)
                {
                    free_column_cache_item((*column_items)[j]);
                }
            }
            pg_free(*column_items);
            *column_items = NULL;
            *count = 0;
            /* 清理数据获取器返回的元数据列表 */
            list_free(column_list);
            return TECC_CACHE_ERROR_MEMORY;
        }

        /* 确保缓存项包含完整的名称信息 */
        if ((*column_items)[i]->database_name == NULL) {
            (*column_items)[i]->database_name = pg_strdup(database_name);
        }
        if ((*column_items)[i]->schema_name == NULL) {
            (*column_items)[i]->schema_name = pg_strdup(schema_name);
        }
        if ((*column_items)[i]->table_name == NULL) {
            (*column_items)[i]->table_name = pg_strdup(table_name);
        }

        i++;
    }

    /* 补填OID字段（如果为空）- 只执行一次 */
    if (*count > 0 && (*column_items)[0] != NULL &&
        ((*column_items)[0]->schema_oid == InvalidOid || (*column_items)[0]->database_oid == InvalidOid) &&
        state->conn != NULL)
    {
        char oid_query[256];
        PGresult *res;

        tecc_disable_once(state->conn);

        /* 查询schema OID */
        if ((*column_items)[0]->schema_oid == InvalidOid && schema_name != NULL)
        {
            snprintf(oid_query, sizeof(oid_query),
                     "SELECT oid FROM pg_namespace WHERE nspname = '%s'",
                     schema_name);
            res = PQexec(state->conn, oid_query);
            if (res && PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
            {
                Oid schema_oid = (Oid)atol(PQgetvalue(res, 0, 0));
                int j;
                for (j = 0; j < *count; j++)
                {
                    if ((*column_items)[j] != NULL)
                        (*column_items)[j]->schema_oid = schema_oid;
                }
            }
            if (res) PQclear(res);
        }

        /* 查询database OID */
        if ((*column_items)[0]->database_oid == InvalidOid)
        {
            snprintf(oid_query, sizeof(oid_query),
                     "SELECT oid FROM pg_database WHERE datname = '%s'",
                     database_name);
            res = PQexec(state->conn, oid_query);
            if (res && PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
            {
                Oid database_oid = (Oid)atol(PQgetvalue(res, 0, 0));
                int j;
                for (j = 0; j < *count; j++)
                {
                    if ((*column_items)[j] != NULL)
                        (*column_items)[j]->database_oid = database_oid;
                }
            }
            if (res) PQclear(res);
        }

        tecc_reset_disable_once(state->conn);
    }

    /* 清理数据获取器返回的元数据列表 */
    list_free(column_list);
    return TECC_CACHE_OK;
}

/*
 * convert_fetcher_error_to_cache_error - 将数据获取器错误码转换为缓存模块错误码
 *
 * 参数:
 *   fetcher_error - 数据获取器错误码
 *
 * 返回:
 *   缓存模块错误码
 *
 * 映射关系说明:
 *   TECC_FETCHER_OK → TECC_CACHE_OK
 *   TECC_FETCHER_ERROR_CONNECTION → TECC_CACHE_ERROR_CONNECTION
 *   TECC_FETCHER_ERROR_PERMISSION → TECC_CACHE_ERROR_PERMISSION
 *   TECC_FETCHER_ERROR_QUERY → TECC_CACHE_ERROR_QUERY
 *   TECC_FETCHER_ERROR_PARSE → TECC_CACHE_ERROR_CONVERSION
 *   TECC_FETCHER_ERROR_MEMORY → TECC_CACHE_ERROR_MEMORY
 *   TECC_FETCHER_ERROR_TIMEOUT → TECC_CACHE_ERROR_TIMEOUT
 *   TECC_FETCHER_ERROR_INVALID_PARAM → TECC_CACHE_ERROR_INVALID_PARAM
 *   TECC_FETCHER_ERROR_CACHE → TECC_CACHE_ERROR_HASH_TABLE
 *   TECC_FETCHER_ERROR_SERIALIZATION → TECC_CACHE_ERROR_SERIALIZATION
 *   TECC_FETCHER_ERROR_VALIDATION → TECC_CACHE_ERROR_CONVERSION
 *   TECC_FETCHER_ERROR_CONFIG → TECC_CACHE_ERROR_CONFIG
 *   TECC_FETCHER_ERROR_INTERNAL → TECC_CACHE_ERROR_INTERNAL
 *   TECC_FETCHER_ERROR_NOT_FOUND → TECC_CACHE_ERROR_NOT_FOUND
 *   TECC_FETCHER_ERROR_UNSUPPORTED → TECC_CACHE_ERROR_UNSUPPORTED
 */
static int
convert_fetcher_error_to_cache_error(int fetcher_error)
{
    switch (fetcher_error)
    {
        case TECC_FETCHER_OK:
            return TECC_CACHE_OK;

        case TECC_FETCHER_ERROR_CONNECTION:
            return TECC_CACHE_ERROR_CONNECTION;

        case TECC_FETCHER_ERROR_PERMISSION:
            return TECC_CACHE_ERROR_PERMISSION;

        case TECC_FETCHER_ERROR_QUERY:
            return TECC_CACHE_ERROR_QUERY;

        case TECC_FETCHER_ERROR_MEMORY:
            return TECC_CACHE_ERROR_MEMORY;

        case TECC_FETCHER_ERROR_INVALID_PARAM:
            return TECC_CACHE_ERROR_INVALID_PARAM;

        /* 以下错误映射为更具体的缓存模块错误 */
        case TECC_FETCHER_ERROR_PARSE:
            return TECC_CACHE_ERROR_CONVERSION;
        case TECC_FETCHER_ERROR_TIMEOUT:
            return TECC_CACHE_ERROR_TIMEOUT;
        case TECC_FETCHER_ERROR_CACHE:
            return TECC_CACHE_ERROR_HASH_TABLE;
        case TECC_FETCHER_ERROR_SERIALIZATION:
            return TECC_CACHE_ERROR_SERIALIZATION;
        case TECC_FETCHER_ERROR_VALIDATION:
            return TECC_CACHE_ERROR_CONVERSION;
        case TECC_FETCHER_ERROR_CONFIG:
            return TECC_CACHE_ERROR_CONFIG;
        case TECC_FETCHER_ERROR_INTERNAL:
            return TECC_CACHE_ERROR_INTERNAL;

        /* 新增的错误码映射 */
        case TECC_FETCHER_ERROR_NOT_FOUND:
            return TECC_CACHE_ERROR_NOT_FOUND;
        case TECC_FETCHER_ERROR_UNSUPPORTED:
            return TECC_CACHE_ERROR_UNSUPPORTED;

        default:
            /* 未知错误码，映射为内部错误 */
            return TECC_CACHE_ERROR_INTERNAL;
    }
}


/*
 * convert_kek_metadata_to_cache_item - 将KEK元数据转换为缓存项
 *
 * 参数:
 *   kek_metadata_ptr - KEK元数据指针
 *
 * 返回:
 *   转换后的KEK缓存项指针
 */
static TeccKekCacheItem *
convert_kek_metadata_to_cache_item(const TeccState *state, const void *kek_metadata_ptr)
{
    const TeccKekMetadata *kek_metadata = (const TeccKekMetadata *)kek_metadata_ptr;
    TeccKekCacheItem *cache_item = NULL;
    int i;

    /* 检查输入参数 */
    if (kek_metadata == NULL)
    {
        return NULL;
    }

    cache_item = (TeccKekCacheItem *)pg_malloc(sizeof(TeccKekCacheItem));
    if (cache_item == NULL)
    {
        return NULL;
    }

    memset(cache_item, 0, sizeof(TeccKekCacheItem));

    /* 转换基本字段 */
    TECC_LOG(TECC_LOG_DEBUG1, "convert_kek_metadata_to_cache_item: kek_metadata->kek_oid = %u", kek_metadata->kek_oid);
    cache_item->kek_oid = kek_metadata->kek_oid;
    TECC_LOG(TECC_LOG_DEBUG1, "convert_kek_metadata_to_cache_item: After assignment, cache_item->kek_oid = %u", cache_item->kek_oid);

    if (kek_metadata->key_name != NULL)
    {
        cache_item->key_name = pg_strdup(kek_metadata->key_name);
        if (cache_item->key_name == NULL)
        {
            pg_free(cache_item);
            return NULL;
        }
    }

    if (kek_metadata->key_namespace != NULL)
    {
        cache_item->key_namespace = pg_strdup(kek_metadata->key_namespace);
        if (cache_item->key_namespace == NULL)
        {
            /* 释放已分配的内存 */
            if (cache_item->key_name != NULL)
            {
                pg_free(cache_item->key_name);
            }
            pg_free(cache_item);
            return NULL;
        }
    }

    /* 所有者字段 - 直接使用key_owner_oid字段，无需转换 */
    cache_item->key_owner = kek_metadata->key_owner_oid;
    if (cache_item->key_owner == InvalidOid)
    {
        /* 如果OID无效，记录日志 */
        TECC_LOG(TECC_LOG_WARNING, "Invalid key_owner_oid for KEK '%s', using InvalidOid",
                       kek_metadata->key_name ? kek_metadata->key_name : "(NULL)");
    }

    /* 创建时间 */
    cache_item->create_date = kek_metadata->create_date;
    cache_item->modify_date = kek_metadata->create_date;  /* 修改时间默认为创建时间 */

    /* 解析KEK参数 */
    if (kek_metadata->args != NULL && kek_metadata->args_count > 0)
    {
        for (i = 0; i < kek_metadata->args_count; i++)
        {
            TeccKeyArg *arg = &kek_metadata->args[i];

            /* 只处理function_name为"kek_config"的参数 */
            if (arg->function_name != NULL && arg->key != NULL && arg->value != NULL &&
                strcmp(arg->function_name, "kek_config") == 0)
            {
                /* 处理key_store字段（合并到kms_type） - 兼容大小写 */
                if (strcmp(arg->key, "KEY_STORE") == 0 || strcmp(arg->key, "key_store") == 0)
                {
                    /* 将key_store信息合并到kms_type字段 */
                    if (strcmp(arg->value, "localkms") == 0)
                    {
                        cache_item->kms_type = 1;
                    }
                    else if (strcmp(arg->value, "hsmkms") == 0)
                    {
                        cache_item->kms_type = 2;
                    }
                    else if (strcmp(arg->value, "cloudkms") == 0)
                    {
                        cache_item->kms_type = 3;
                    }
                    else if (strcmp(arg->value, "swxakms") == 0)
                    {
                        cache_item->kms_type = 4;
                    }
                }
                /* 处理key_path字段（合并到kms_path） - 兼容大小写 */
                else if (strcmp(arg->key, "KEY_PATH") == 0 || strcmp(arg->key, "key_path") == 0)
                {
                    /* 如果kms_path为空，则分配内存并复制 */
                    if (cache_item->kms_path == NULL)
                    {
                        cache_item->kms_path = pg_strdup(arg->value);
                    }
                }
                /* 处理algorithm字段 */
                else if (strcmp(arg->key, "algorithm") == 0)
                {
                    /* 解析算法类型 */
                    if (strcmp(arg->value, "AES_256_CBC") == 0)
                    {
                        cache_item->algorithm = 1;
                    }
                    else if (strcmp(arg->value, "SM4_CBC") == 0)
                    {
                        cache_item->algorithm = 2;
                    }
                }
                /* 处理key_length字段 */
                else if (strcmp(arg->key, "key_length") == 0)
                {
                    cache_item->key_length = atoi(arg->value);
                }
                /* 注意：移除了对kms_type和kek_algorithm旧键名的支持 */
            }
        }
    }

    /* 设置默认值（如果参数中未提供） */
    if (cache_item->kms_type == 0)
    {
        cache_item->kms_type = 1;  /* 默认：localkms */
    }

    if (cache_item->key_length == 0)
    {
        cache_item->key_length = 256;  /* 默认密钥长度 */
    }

    if (cache_item->algorithm == 0)
    {
        cache_item->algorithm = 1;  /* 默认算法：AES_256_CBC */
    }

    /* kek_oid已经在前面从kek_metadata->kek_oid赋值，不需要重新设置为0 */
    cache_item->status = 1;  /* 默认状态：激活 */
    cache_item->hit_times = 0;  /* 初始命中次数 */
    /* 移除了plain_key_value字段 */

    return cache_item;
}

/*
 * convert_dek_metadata_to_cache_item - 将DEK元数据转换为缓存项
 *
 * 参数:
 *   dek_metadata_ptr - DEK元数据指针
 *
 * 返回:
 *   转换后的DEK缓存项指针
 */
static TeccDekCacheItem *
convert_dek_metadata_to_cache_item(const TeccState *state, const void *dek_metadata_ptr)
{
    const TeccDekMetadata *dek_metadata = (const TeccDekMetadata *)dek_metadata_ptr;
    TeccDekCacheItem *cache_item = NULL;
    int i;

    /* 检查输入参数 */
    if (dek_metadata == NULL)
    {
        return NULL;
    }

    cache_item = (TeccDekCacheItem *)pg_malloc(sizeof(TeccDekCacheItem));
    if (cache_item == NULL)
    {
        return NULL;
    }

    memset(cache_item, 0, sizeof(TeccDekCacheItem));

    /* 转换基本字段 */
    if (dek_metadata->key_name != NULL)
    {
        cache_item->key_name = pg_strdup(dek_metadata->key_name);
        if (cache_item->key_name == NULL)
        {
            pg_free(cache_item);
            return NULL;
        }
    }

    if (dek_metadata->key_namespace != NULL)
    {
        cache_item->key_namespace = pg_strdup(dek_metadata->key_namespace);
        if (cache_item->key_namespace == NULL)
        {
            /* 释放已分配的内存 */
            if (cache_item->key_name != NULL)
            {
                pg_free(cache_item->key_name);
            }
            pg_free(cache_item);
            return NULL;
        }
    }

    /* 使用DEK OID */
    cache_item->dek_oid = dek_metadata->dek_oid;

    /* 跨实例DEK身份（hash of schema.dek_name） */
    cache_item->column_key_distributed_id = dek_metadata->column_key_distributed_id;

    /* 关联KEK OID */
    cache_item->kek_oid = dek_metadata->kek_oid;

    /* 复制KEK名称到缓存项 */
    if (dek_metadata->kek_name != NULL)
    {
        cache_item->kek_name = pg_strdup(dek_metadata->kek_name);
    }

    /* 所有者字段 - 直接使用key_owner_oid字段，无需转换 */
    cache_item->key_owner = dek_metadata->key_owner_oid;
    if (cache_item->key_owner == InvalidOid)
    {
        /* 如果OID无效，记录日志 */
        TECC_LOG(TECC_LOG_WARNING, "Invalid key_owner_oid for DEK '%s', using InvalidOid",
                       dek_metadata->key_name ? dek_metadata->key_name : "(NULL)");
    }

    /* 创建时间 */
    cache_item->create_date = dek_metadata->create_date;
    cache_item->modify_date = dek_metadata->create_date;  /* 修改时间默认为创建时间 */

    /* 解析DEK参数 */
    if (dek_metadata->args != NULL && dek_metadata->args_count > 0)
    {
        for (i = 0; i < dek_metadata->args_count; i++)
        {
            TeccKeyArg *arg = &dek_metadata->args[i];

            /* 只处理function_name为"dek_config"的参数 */
            if (arg->function_name != NULL && arg->key != NULL && arg->value != NULL &&
                strcmp(arg->function_name, "dek_config") == 0)
            {
                /* 处理encrypted_value字段（合并到encrypted_key） */
                if (strcmp(arg->key, "encrypted_value") == 0)
                {
                    /* encrypted_value在数据库中以hex编码字符串存储（如"00010001df79..."），
                     * decode_bytea_value将bytea解码后得到的是这个hex字符串（ASCII文本），
                     * 因此需要先通过tecc_formatter_hex_decode将其解码为真正的二进制数据。 */
                    /* encrypted_value在数据库中以hex编码字符串存储（如"00010001df79..."），
                     * decode_bytea_value将bytea解码后得到的是hex字符串（ASCII文本），
                     * 需要用tecc_formatter_hex_decode解码为真正的二进制数据。 */
                    if (arg->value != NULL && strlen(arg->value) > 0)
                    {
                        unsigned char *binary_data = NULL;
                        size_t binary_len = 0;
                        int decode_ret = tecc_formatter_hex_decode(arg->value, &binary_data, &binary_len);
                        if (decode_ret == 0 && binary_data != NULL && binary_len > 0)
                        {
                            cache_item->encrypted_key = binary_data;
                            cache_item->key_length = binary_len;
                            TECC_LOG(TECC_LOG_DEBUG1, "Successfully decoded encrypted_value from hex, binary length: %zu", cache_item->key_length);
                        }
                        else
                        {
                            TECC_LOG(TECC_LOG_WARNING, "Hex decode failed for encrypted_value, ret=%d", decode_ret);
                            cache_item->encrypted_key = NULL;
                            cache_item->key_length = 0;
                            if (binary_data != NULL) pg_free(binary_data);
                        }
                    }
                    else
                    {
                        TECC_LOG(TECC_LOG_WARNING, "Empty encrypted_value");
                        cache_item->encrypted_key = NULL;
                        cache_item->key_length = 0;
                    }
                }
                /* DEK不应该记录算法类型，忽略support_algo字段 */
                else if (strcmp(arg->key, "support_algo") == 0)
                {
                    /* DEK不应该记录算法类型，算法类型只在加密字段中记录（如'det,ope'等）
                     * 因此忽略这个参数，不设置algorithm_mask
                     */
                    TECC_LOG(TECC_LOG_DEBUG1, "Ignoring support_algo parameter for DEK '%s': %s",
                             dek_metadata->key_name ? dek_metadata->key_name : "(NULL)", arg->value);
                }
                else if (strcmp(arg->key, "key_length") == 0)
                {
                    /* 仅当encrypted_key为空时设置key_length（DEK部分），
                     * 避免覆盖encrypted_value解码后得到的数据长度。
                     * key_length参数表示DEK明文的位长度（如256），
                     * 而非加密DEK数据的字节长度。 */
                    if (cache_item->encrypted_key == NULL)
                    {
                        cache_item->key_length = atoi(arg->value) / 8;  /* 转换为字节 */
                    }
                }
            }
        }
    }

    /* 设置默认值（如果参数中未提供） */
    if (cache_item->key_length == 0)
    {
        cache_item->key_length = 256;  /* 默认密钥长度 */
    }

    /* DEK不应该记录算法类型，因此不需要设置algorithm_mask */

    /* 如果encrypted_key为空，清除key_length（避免与前面设置的默认值冲突） */
    if (cache_item->encrypted_key == NULL)
    {
        cache_item->key_length = 0;
    }

    cache_item->status = 1;  /* 默认状态：激活 */
    cache_item->hit_times = 0;  /* 初始命中次数 */
    /* 移除了plain_key_value字段 */

    return cache_item;
}

/*
 * convert_column_metadata_to_cache_item - 将加密列元数据转换为缓存项
 *
 * 参数:
 *   column_metadata_ptr - 加密列元数据指针
 *
 * 返回:
 *   转换后的加密列缓存项指针
 */
static TeccColumnCacheItem *
convert_column_metadata_to_cache_item(const TeccState *state, const void *column_metadata_ptr)
{
    const TeccEncryptedColumnMetadata *column_metadata = (const TeccEncryptedColumnMetadata *)column_metadata_ptr;
    TeccColumnCacheItem *cache_item = NULL;

    /* 检查输入参数 */
    if (column_metadata == NULL)
    {
        return NULL;
    }

    /* 分配缓存项内存 */
    cache_item = (TeccColumnCacheItem *)pg_malloc(sizeof(TeccColumnCacheItem));
    if (cache_item == NULL)
    {
        return NULL;
    }

    memset(cache_item, 0, sizeof(TeccColumnCacheItem));

    /* 转换基本字段 */
    cache_item->table_oid = column_metadata->table_oid;
    cache_item->column_attnum = column_metadata->column_attnum;
    cache_item->original_type_oid = column_metadata->data_type_original_oid;
    cache_item->original_type_mod = column_metadata->data_type_original_mod;
    cache_item->encryption_type = column_metadata->encryption_type;
    cache_item->create_date = column_metadata->create_date;

    /* 转换字符串字段 */
    if (column_metadata->column_name != NULL)
    {
        cache_item->column_name = pg_strdup(column_metadata->column_name);
        if (cache_item->column_name == NULL)
        {
            pg_free(cache_item);
            return NULL;
        }
    }

    /* 转换排序规则字段 */
    cache_item->collation_oid = column_metadata->collation_oid;

    TECC_LOG(TECC_LOG_DEBUG1, "DEBUG LOOP: collation_oid=%u, collation_locale=%p (0x%lx)",
            column_metadata->collation_oid,
            column_metadata->collation_locale,
            (unsigned long)column_metadata->collation_locale);

    if (column_metadata->collation_locale != NULL)
    {
        cache_item->collation_locale = pg_strdup(column_metadata->collation_locale);
        if (cache_item->collation_locale == NULL)
        {
            /* 释放已分配的内存 */
            if (cache_item->column_name != NULL)
            {
                pg_free(cache_item->column_name);
            }
            pg_free(cache_item);
            return NULL;
        }
    }

    /* DEK名称转换为DEK ID - column_key_id即为DEK OID */
    cache_item->dek_id = column_metadata->column_key_id;

    /* 复制DEK名称字段 */
    if (column_metadata->dek_name != NULL)
    {
        cache_item->dek_name = pg_strdup(column_metadata->dek_name);
        if (cache_item->dek_name == NULL)
        {
            // 释放已分配的内存
            if (cache_item->column_name != NULL)
            {
                pg_free(cache_item->column_name);
            }
            if (cache_item->collation_locale != NULL)
            {
                pg_free(cache_item->collation_locale);
            }
            pg_free(cache_item);
            return NULL;
        }
    }
    else
    {
        // DEK名称为NULL，记录日志
        TECC_CACHE_DEBUG("convert_column_metadata_to_cache_item: DEK name is NULL for column %s",
                        column_metadata->column_name ? column_metadata->column_name : "(unknown)");
        cache_item->dek_name = NULL;
    }

    /* 从元数据复制表名 */
    cache_item->table_name = column_metadata->table_name
        ? pg_strdup(column_metadata->table_name) : NULL;
    /* ReqID:TECC4.0 - DEK权限来自查询中的has_dek_privilege_name() */
    cache_item->has_dek_permission = column_metadata->has_dek_permission;
    cache_item->dek_permission_checked = true;

    cache_item->schema_oid = 0;  /* TODO: 需要从其他途径获取模式OID */
    cache_item->schema_name = NULL;  /* TODO: 需要从其他途径获取模式名称 */
    cache_item->database_oid = 0;  /* TODO: 需要从其他途径获取数据库OID */
    cache_item->database_name = NULL;  /* TODO: 需要从其他途径获取数据库名称 */
    cache_item->hit_times = 0;  /* 初始命中次数 */

    /* 添加调试日志 */
    TECC_CACHE_DEBUG("convert_column_metadata_to_cache_item: Created cache item %p for column %s, DEK name: %s, DEK ID: %u",
                    cache_item,
                    column_metadata->column_name ? column_metadata->column_name : "(unknown)",
                    cache_item->dek_name ? cache_item->dek_name : "(NULL)",
                    cache_item->dek_id);

    return cache_item;
}

/*
 * free_kek_cache_item - 释放KEK缓存项内存
 *
 * 参数:
 *   item - KEK缓存项指针
 */
static void
free_kek_cache_item(TeccKekCacheItem *item)
{
    if (item == NULL)
    {
        return;
    }

    /* 释放字符串字段 */
    if (item->key_name != NULL)
    {
        pg_free(item->key_name);
    }
    if (item->key_namespace != NULL)
    {
        pg_free(item->key_namespace);
    }
    if (item->kms_path != NULL)
    {
        pg_free(item->kms_path);
    }
    /* 移除了冗余字段的释放：key_store, key_path, kek_algorithm, plain_key_value */

    /* 释放缓存项本身 */
    pg_free(item);
}

/*
 * free_dek_cache_item - 释放DEK缓存项内存
 *
 * 参数:
 *   item - DEK缓存项指针
 */
static void
free_dek_cache_item(TeccDekCacheItem *item)
{
    if (item == NULL)
    {
        return;
    }

    /* 释放字符串字段 */
    if (item->key_name != NULL)
    {
        pg_free(item->key_name);
    }
    if (item->key_namespace != NULL)
    {
        pg_free(item->key_namespace);
    }
    /* 移除了冗余字段的释放：encrypted_value, support_algo, plain_key_value */

    /* 释放加密密钥数据 */
    if (item->encrypted_key != NULL)
    {
        pg_free(item->encrypted_key);
    }

    /* 释放缓存项本身 */
    pg_free(item);
}

/*
 * free_column_cache_item - 释放加密列缓存项内存
 *
 * 参数:
 *   item - 加密列缓存项指针
 */
static void
free_column_cache_item(TeccColumnCacheItem *item)
{
    if (item == NULL)
    {
        return;
    }

    /* 释放字符串字段 */
    if (item->table_name != NULL)
    {
        pg_free(item->table_name);
    }
    if (item->schema_name != NULL)
    {
        pg_free(item->schema_name);
    }
    if (item->database_name != NULL)
    {
        pg_free(item->database_name);
    }
    if (item->column_name != NULL)
    {
        pg_free(item->column_name);
    }
    if (item->dek_name != NULL)
    {
        pg_free(item->dek_name);
    }
    if (item->collation_locale != NULL)
    {
        pg_free(item->collation_locale);
    }

    /* 释放缓存项本身 */
    pg_free(item);
}

/*
 * tecc_fetch_guc_params_for_cache - 为缓存模块获取GUC参数
 *
 * 参数:
 *   state - TECC状态结构，包含连接和上下文信息
 *   manager - 缓存管理器指针
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
static int
tecc_fetch_guc_params_for_cache(const TeccState *state,
                               TeccCacheManager *manager)
{
    /* 通过数据获取器模块查询GUC参数 */
    TeccCursorInterface *cursor = NULL;
    char query[512];

    /* 参数检查 */
    if (state == NULL || manager == NULL) {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    TECC_LOG(TECC_LOG_DEBUG1, "tecc_fetch_guc_params_for_cache: Starting GUC parameters fetch");

    

    /* 构建查询语句 - 查询常用的GUC参数 */
    /* 注意：current_schema不是GUC参数，需要使用current_schema()函数查询 */
    snprintf(query, sizeof(query),
             "SELECT name, setting FROM pg_settings "
             "WHERE name IN ('search_path', 'sql_compatibility', "
             "'standard_conforming_strings', 'backslash_quote', 'escape_string_warning', 'role')");

    /* 执行查询 - 使用数据获取器模块的查询接口 */
    cursor = tecc_fetcher_execute_query(state, query);
    if (cursor == NULL) {
        return TECC_CACHE_ERROR_QUERY;
    }

    /* 解析查询结果 — next()返回0(TECC_FETCHER_OK)表示有数据 */
    while (cursor->next(cursor) == TECC_FETCHER_OK) {
        const char *name = cursor->get_column(cursor, 0);
        const char *value = cursor->get_column(cursor, 1);

        if (name == NULL || value == NULL) {
            continue;
        }

        /* 根据参数名设置对应的字段 */
        if (strcmp(name, "search_path") == 0) {
            manager->guc_params.search_path = pg_strdup(value);
        } else if (strcmp(name, "sql_compatibility") == 0) {
            manager->guc_params.sql_compatibility = pg_strdup(value);
        } else if (strcmp(name, "standard_conforming_strings") == 0) {
            manager->guc_params.standard_conforming_strings = (strcmp(value, "on") == 0);
        } else if (strcmp(name, "backslash_quote") == 0) {
            manager->guc_params.backslash_quote = (value[0] != '\0') ? value[0] : 's';
        } else if (strcmp(name, "escape_string_warning") == 0) {
            manager->guc_params.escape_string_warning = (strcmp(value, "on") == 0);
        } else if (strcmp(name, "role") == 0) {
            manager->guc_params.role = pg_strdup(value);
        }
    }

    /* 清理游标 */
    cursor->close(cursor);
    cursor->destroy(cursor);

    /* 查询current_schema()函数 - 这不是GUC参数，需要单独查询 */
    cursor = tecc_fetcher_execute_query(state, "SELECT current_schema()");
    if (cursor != NULL && cursor->next(cursor) == TECC_FETCHER_OK) {
        const char *current_schema = cursor->get_column(cursor, 0);
        if (current_schema != NULL) {
            manager->guc_params.current_schema = pg_strdup(current_schema);
        }
        cursor->close(cursor);
        cursor->destroy(cursor);
    }

    /* 记录成功获取GUC参数 */

    /* current_schema 必须从后端获取，不fallback到硬编码值 */
    if (manager->guc_params.current_schema == NULL) {
        TECC_LOG(TECC_LOG_ERROR,
                 "tecc_fetch_guc_params_for_cache: "
                 "Failed to get current_schema from database, "
                 "schema-dependent operations will fail");
        /* 不fallback到"public" — 保持NULL，让调用者显式处理 */
    }

    /* 其他参数未获取到时使用合理默认值 */
    if (manager->guc_params.search_path == NULL) {
        manager->guc_params.search_path = pg_strdup("\"$user\", public");
    }
    if (manager->guc_params.sql_compatibility == NULL) {
        manager->guc_params.sql_compatibility = pg_strdup("postgresql");
    }
    if (manager->guc_params.role == NULL) {
        manager->guc_params.role = pg_strdup("postgres");
    }

    /* 检查内存分配是否成功 */
    if (manager->guc_params.search_path == NULL ||
        manager->guc_params.sql_compatibility == NULL ||
        manager->guc_params.role == NULL) {
        /* 清理已分配的内存 */
        if (manager->guc_params.search_path != NULL) pg_free(manager->guc_params.search_path);
        if (manager->guc_params.sql_compatibility != NULL) pg_free(manager->guc_params.sql_compatibility);
        if (manager->guc_params.current_schema != NULL) pg_free(manager->guc_params.current_schema);
        if (manager->guc_params.role != NULL) pg_free(manager->guc_params.role);
        return TECC_CACHE_ERROR_MEMORY;
    }

    /* 解析search_path为schema列表，用于按PG原生规则匹配表 */
    if (state->conn != NULL)
        tecc_cache_parse_search_path(manager, state->conn);

    manager->guc_loaded = true;
    return TECC_CACHE_OK;
}

/*
 * tecc_fetch_kek_list_for_cache - 为缓存模块批量获取KEK元数据列表
 *
 * 参数:
 *   state - TECC状态结构，包含连接和上下文信息
 *   namespace_filter - 命名空间过滤器（可为NULL表示不过滤）
 *   kek_items - 返回的KEK缓存项指针数组
 *   count - 返回的KEK数量
 *
 * 返回:
 *   0表示成功，非0表示错误码
 *
 * 实现说明:
 *   TODO: 实现批量KEK查询接口
 */

static int tecc_fetch_kek_list_for_cache(const TeccState *state,
                             const char *namespace_filter,
                             TeccKekCacheItem ***kek_items,
                             int *count)
{
    List *kek_list = NULL;
    TeccFetcherErrorCode fetcher_error;
    int kek_count;
    TeccKekCacheItem **items;
    int i;
    ListCell *cell;

    /* 参数检查 */
    if (state == NULL || kek_items == NULL || count == NULL) {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    *kek_items = NULL;
    *count = 0;

    /* 调用数据获取器批量获取KEK元数据 */
    fetcher_error = tecc_fetch_kek_list(state, namespace_filter, &kek_list);

    if (fetcher_error != TECC_FETCHER_OK) {
        return convert_fetcher_error_to_cache_error(fetcher_error);
    }

    /* 检查是否获取到数据 */
    if (kek_list == NULL) {
        return TECC_CACHE_OK;  /* 没有KEK，返回成功但数量为0 */
    }

    /* 获取列表长度 */
    kek_count = list_length(kek_list);
    if (kek_count == 0) {
        /* 列表为空，释放列表并返回成功 */
        list_free(kek_list);
        return TECC_CACHE_OK;
    }

    /* 分配结果数组 */
    items = (TeccKekCacheItem **)pg_malloc(kek_count * sizeof(TeccKekCacheItem *));
    if (items == NULL) {
        list_free(kek_list);
        return TECC_CACHE_ERROR_MEMORY;
    }

    /* 转换每个KEK元数据 */
    i = 0;
    cell = NULL;
    foreach(cell, kek_list) {
        TeccKekMetadata *kek_metadata = (TeccKekMetadata *)lfirst(cell);

        if (kek_metadata != NULL) {
            items[i] = convert_kek_metadata_to_cache_item(state, kek_metadata);
            if (items[i] == NULL) {
                /* 转换失败，清理已分配的资源 */
                {
                    int j;
                    for (j = 0; j < i; j++) {
                        free_kek_cache_item(items[j]);
                    }
                }
                pg_free(items);
                list_free(kek_list);
                return TECC_CACHE_ERROR_CONVERSION;
            }
        } else {
            items[i] = NULL;
        }
        i++;
    }

    /* 设置输出参数 */
    *kek_items = items;
    *count = kek_count;

    /* 释放数据获取器分配的内存 */
    list_free(kek_list);

    return TECC_CACHE_OK;
}

/*
 * tecc_fetch_dek_list_for_cache - 为缓存模块批量获取DEK元数据列表
 *
 * 参数:
 *   state - TECC状态结构，包含连接和上下文信息
 *   namespace_filter - 命名空间过滤器（可为NULL表示不过滤）
 *   dek_items - 返回的DEK缓存项指针数组
 *   count - 返回的DEK数量
 *
 * 返回:
 *   0表示成功，非0表示错误码
 *
 * 实现说明:
 *   TODO: 实现批量DEK查询接口
 */

static int tecc_fetch_dek_list_for_cache(const TeccState *state,
                             const char *namespace_filter,
                             TeccDekCacheItem ***dek_items,
                             int *count)
{
    List *dek_list = NULL;
    TeccFetcherErrorCode fetcher_error;
    int dek_count;
    TeccDekCacheItem **items;
    int i;
    ListCell *cell;

    /* 参数检查 */
    if (state == NULL || dek_items == NULL || count == NULL) {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    *dek_items = NULL;
    *count = 0;

    /* 调用数据获取器批量获取DEK元数据 */
    fetcher_error = tecc_fetch_dek_list(state, namespace_filter, &dek_list);

    if (fetcher_error != TECC_FETCHER_OK) {
        return convert_fetcher_error_to_cache_error(fetcher_error);
    }

    /* 检查是否获取到数据 */
    if (dek_list == NULL) {
        return TECC_CACHE_OK;  /* 没有DEK，返回成功但数量为0 */
    }

    /* 获取列表长度 */
    dek_count = list_length(dek_list);
    if (dek_count == 0) {
        /* 列表为空，释放列表并返回成功 */
        list_free(dek_list);
        return TECC_CACHE_OK;
    }

    /* 分配结果数组 */
    items = (TeccDekCacheItem **)pg_malloc(dek_count * sizeof(TeccDekCacheItem *));
    if (items == NULL) {
        list_free(dek_list);
        return TECC_CACHE_ERROR_MEMORY;
    }

    /* 转换每个DEK元数据 */
    i = 0;
    cell = NULL;
    foreach(cell, dek_list) {
        TeccDekMetadata *dek_metadata = (TeccDekMetadata *)lfirst(cell);

        if (dek_metadata != NULL) {
            items[i] = convert_dek_metadata_to_cache_item(state, dek_metadata);
            if (items[i] == NULL) {
                /* 转换失败，清理已分配的资源 */
                {
                    int j;
                    for (j = 0; j < i; j++) {
                        free_dek_cache_item(items[j]);
                    }
                }
                pg_free(items);
                list_free(dek_list);
                return TECC_CACHE_ERROR_CONVERSION;
            }
        } else {
            items[i] = NULL;
        }
        i++;
    }

    /* 设置输出参数 */
    *dek_items = items;
    *count = dek_count;

    /* 释放数据获取器分配的内存 */
    list_free(dek_list);

    return TECC_CACHE_OK;
}

/* Begin - ReqID:TECC4.0 - 缓存模块错误描述函数实现 */

/*
 * tecc_cache_error_message - 获取缓存模块错误消息
 *
 * 参数:
 *   error_code - 缓存模块错误码
 *
 * 返回:
 *   错误消息字符串
 */
const char *
tecc_cache_error_message(int error_code)
{
    switch (error_code)
    {
        case TECC_CACHE_OK:
            return "操作成功";
        case TECC_CACHE_ERROR_INVALID_PARAM:
            return "无效参数";
        case TECC_CACHE_ERROR_NOT_FOUND:
            return "未找到数据";
        case TECC_CACHE_ERROR_CONVERSION:
            return "数据转换错误";
        case TECC_CACHE_ERROR_MEMORY:
            return "内存分配错误";
        case TECC_CACHE_ERROR_CONNECTION:
            return "连接错误";
        case TECC_CACHE_ERROR_PERMISSION:
            return "权限错误";
        case TECC_CACHE_ERROR_QUERY:
            return "查询错误";
        case TECC_CACHE_ERROR_INTERNAL:
            return "内部错误";
        case TECC_CACHE_ERROR_NOT_INITIALIZED:
            return "缓存未初始化";
        case TECC_CACHE_ERROR_HASH_TABLE:
            return "哈希表操作错误";
        case TECC_CACHE_ERROR_LOCK:
            return "锁操作错误";
        case TECC_CACHE_ERROR_REFRESH:
            return "缓存刷新错误";
        case TECC_CACHE_ERROR_INIT:
            return "初始化错误";
        case TECC_CACHE_ERROR_CLEANUP:
            return "清理错误";
        case TECC_CACHE_ERROR_STATE:
            return "状态错误";
        case TECC_CACHE_ERROR_SERIALIZATION:
            return "序列化错误";
        case TECC_CACHE_ERROR_DESERIALIZATION:
            return "反序列化错误";
        case TECC_CACHE_ERROR_VERSION:
            return "版本不兼容错误";
        case TECC_CACHE_ERROR_CORRUPTION:
            return "数据损坏错误";
        case TECC_CACHE_ERROR_TIMEOUT:
            return "超时错误";
        case TECC_CACHE_ERROR_RESOURCE:
            return "资源不足错误";
        case TECC_CACHE_ERROR_CONFIG:
            return "配置错误";
        case TECC_CACHE_ERROR_UNSUPPORTED:
            return "不支持的操作";
        default:
            return "未知错误";
    }
}

/*
 * tecc_cache_error_context - 获取带上下文的错误消息
 *
 * 参数:
 *   error_code - 缓存模块错误码
 *   function_name - 函数名
 *   line_number - 行号
 *   additional_info - 附加信息（可为NULL）
 *
 * 返回:
 *   带上下文的错误消息字符串
 */
const char *
tecc_cache_error_context(int error_code, const char *function_name,
                         int line_number, const char *additional_info)
{
    static TECC_THREAD_LOCAL char error_buffer[512];
    const char *error_msg = tecc_cache_error_message(error_code);

    if (additional_info != NULL && additional_info[0] != '\0') {
        snprintf(error_buffer, sizeof(error_buffer),
                 "[%s:%d] %s: %s", function_name, line_number, error_msg, additional_info);
    } else {
        snprintf(error_buffer, sizeof(error_buffer),
                 "[%s:%d] %s", function_name, line_number, error_msg);
    }

    return error_buffer;
}

/* End - ReqID:TECC4.0 - 缓存模块错误描述函数实现 */

/* Begin - ReqID:TECC4.0 - 简化接口实现 */

/*
 * tecc_fetch_kek_for_cache - 简化的KEK查询接口
 */
TeccFetcherErrorCode
tecc_fetch_kek_for_cache(const TeccState *state,
                        const TeccCacheQueryParams *query,
                        TeccKekCacheItem **result,
                        int *count)
{
    /* 参数检查 */
    if (state == NULL || query == NULL || result == NULL) {
        TECC_LOG(TECC_LOG_DEBUG1, "tecc_fetch_kek_for_cache: Invalid parameters, state=%p, query=%p, result=%p",
                 state, query, result);
        return TECC_FETCHER_ERROR_INVALID_PARAM;
    }

    TECC_LOG(TECC_LOG_DEBUG1, "tecc_fetch_kek_for_cache: Starting, flags=0x%x", query->flags);

    /* 根据查询类型调用相应的函数 */
    if (query->flags & TECC_QUERY_BATCH) {
        /* 批量查询 */
        if (count == NULL) {
            return TECC_FETCHER_ERROR_INVALID_PARAM;
        }
        return fetch_kek_batch(state, query->params.batch.filter,
                              (TeccKekCacheItem ***)result, count);
    } else if (query->flags & TECC_QUERY_BY_NAME) {
        /* 按名称查询 */
        if (query->params.by_name.name == NULL) {
            return TECC_FETCHER_ERROR_INVALID_PARAM;
        }
        if (count != NULL) {
            *count = 1;  /* 单个查询 */
        }
        return fetch_kek_by_name(state, query->params.by_name.name,
                                query->params.by_name.namespace,
                                query->external_conn, result);
    } else if (query->flags & TECC_QUERY_BY_OID) {
        /* 按OID查询 - 数据获取器目前不支持，返回错误 */
        /* TODO: 未来可以添加按OID查询的支持 */
        return TECC_FETCHER_ERROR_UNSUPPORTED;
    } else {
        /* 无效的查询类型 */
        return TECC_FETCHER_ERROR_INVALID_PARAM;
    }
}

/*
 * tecc_fetch_dek_for_cache - 简化的DEK查询接口
 */
TeccFetcherErrorCode
tecc_fetch_dek_for_cache(const TeccState *state,
                        const TeccCacheQueryParams *query,
                        TeccDekCacheItem **result,
                        int *count)
{
    /* 参数检查 */
    if (state == NULL || query == NULL || result == NULL) {
        return TECC_FETCHER_ERROR_INVALID_PARAM;
    }

    /* 根据查询类型调用相应的函数 */
    if (query->flags & TECC_QUERY_BATCH) {
        /* 批量查询 */
        if (count == NULL) {
            return TECC_FETCHER_ERROR_INVALID_PARAM;
        }
        return fetch_dek_batch(state, query->params.batch.filter,
                              (TeccDekCacheItem ***)result, count);
    } else if (query->flags & TECC_QUERY_BY_NAME) {
        /* 按名称查询 */
        if (query->params.by_name.name == NULL) {
            return TECC_FETCHER_ERROR_INVALID_PARAM;
        }
        if (count != NULL) {
            *count = 1;  /* 单个查询 */
        }
        return fetch_dek_by_name(state, query->params.by_name.name,
                                query->params.by_name.namespace,
                                query->external_conn, result);
    } else if (query->flags & TECC_QUERY_BY_OID) {
        /* 按OID查询 */
        if (count != NULL) {
            *count = 1;
        }
        return fetch_dek_by_oid(state, query->params.by_oid.oid, result);
    } else {
        /* 无效的查询类型 */
        return TECC_FETCHER_ERROR_INVALID_PARAM;
    }
}

/*
 * tecc_fetch_column_for_cache - 简化的加密列查询接口
 */
TeccFetcherErrorCode
tecc_fetch_column_for_cache(const TeccState *state,
                           const TeccCacheQueryParams *query,
                           TeccColumnCacheItem **result,
                           int *count)
{
    /* 参数检查 */
    if (state == NULL || query == NULL || result == NULL) {
        return TECC_FETCHER_ERROR_INVALID_PARAM;
    }

    /* 根据查询类型调用相应的函数 */
    if (query->flags & TECC_QUERY_BATCH) {
        /* 批量查询 */
        if (count == NULL) {
            return TECC_FETCHER_ERROR_INVALID_PARAM;
        }

        if (query->flags & TECC_QUERY_BY_OID) {
            /* 按OID批量查询 */
            return fetch_columns_batch_by_oid(state, query->params.by_oid.oid,
                                             (TeccColumnCacheItem ***)result, count);
        } else if (query->flags & TECC_QUERY_BY_NAME) {
            /* 按名称批量查询 */
            if (query->table_name == NULL) {
                return TECC_FETCHER_ERROR_INVALID_PARAM;
            }
            return fetch_columns_batch_by_name(state, query->database_name,
                                              query->schema_name, query->table_name,
                                              (TeccColumnCacheItem ***)result, count);
        } else {
            return TECC_FETCHER_ERROR_INVALID_PARAM;
        }
    } else {
        /* 单个查询 */
        if (count != NULL) {
            *count = 1;  /* 单个查询 */
        }

        if (query->flags & TECC_QUERY_BY_OID) {
            /* 按OID单个查询 */
            return fetch_column_by_oid(state, query->params.by_oid.oid,
                                      query->params.by_oid.attnum, result);
        } else if (query->flags & TECC_QUERY_BY_NAME) {
            /* 按名称单个查询 */
            if (query->table_name == NULL || query->column_name == NULL) {
                return TECC_FETCHER_ERROR_INVALID_PARAM;
            }
            return fetch_column_by_name(state, query->database_name,
                                       query->schema_name, query->table_name,
                                       query->column_name, result);
        } else {
            return TECC_FETCHER_ERROR_INVALID_PARAM;
        }
    }
}

/*
 * tecc_fetch_guc_for_cache - 简化的GUC参数查询接口
 */
TeccFetcherErrorCode
tecc_fetch_guc_for_cache(const TeccState *state,
                        TeccCacheManager *manager)
{
    int cache_error;

    /* 参数检查 */
    if (state == NULL || manager == NULL) {
        return TECC_FETCHER_ERROR_INVALID_PARAM;
    }

    /* 调用现有的GUC查询函数，但返回数据获取器错误码 */
    cache_error = tecc_fetch_guc_params_for_cache(state, manager);
    if (cache_error == TECC_CACHE_OK) {
        return TECC_FETCHER_OK;
    } else {
        /* 将缓存错误码转换为数据获取器错误码 */
        switch (cache_error) {
            case TECC_CACHE_ERROR_CONNECTION:
                return TECC_FETCHER_ERROR_CONNECTION;
            case TECC_CACHE_ERROR_QUERY:
                return TECC_FETCHER_ERROR_QUERY;
            case TECC_CACHE_ERROR_MEMORY:
                return TECC_FETCHER_ERROR_MEMORY;
            case TECC_CACHE_ERROR_INVALID_PARAM:
                return TECC_FETCHER_ERROR_INVALID_PARAM;
            default:
                return TECC_FETCHER_ERROR_INTERNAL;
        }
    }
}

/*
 * tecc_cache_convert_error - 将数据获取器错误码转换为缓存模块错误码
 */
int
tecc_cache_convert_error(TeccFetcherErrorCode fetcher_error)
{
    return convert_fetcher_error_to_cache_error(fetcher_error);
}

/*
 * tecc_cache_log_error - 记录缓存模块错误日志
 */
void
tecc_cache_log_error(TeccFetcherErrorCode fetcher_error, const char *context)
{
    tecc_log_fetcher_error(fetcher_error, context);
}

/* 简化接口的内部辅助函数实现 */

static TeccFetcherErrorCode
fetch_kek_by_name(const TeccState *state,
                 const char *key_name,
                 const char *key_namespace,
                 PGconn *ext_conn,
                 TeccKekCacheItem **result)
{
    int cache_error;

    TECC_LOG(TECC_LOG_DEBUG1, "fetch_kek_by_name: Starting to fetch KEK by name, key_name='%s', key_namespace='%s'",
             key_name, key_namespace ? key_namespace : "(null)");

    cache_error = tecc_fetch_kek_metadata_for_cache(state, key_name, key_namespace, ext_conn, result);

    TECC_LOG(TECC_LOG_DEBUG1, "fetch_kek_by_name: tecc_fetch_kek_metadata_for_cache returned cache_error: %d, result=%p",
             cache_error, result ? *result : NULL);

    return (cache_error == TECC_CACHE_OK) ? TECC_FETCHER_OK : TECC_FETCHER_ERROR_INTERNAL;
}

static TeccFetcherErrorCode
fetch_kek_batch(const TeccState *state,
               const char *namespace_filter,
               TeccKekCacheItem ***result,
               int *count)
{
    int cache_error;

    cache_error = tecc_fetch_kek_list_for_cache(state, namespace_filter, result, count);
    return (cache_error == TECC_CACHE_OK) ? TECC_FETCHER_OK : TECC_FETCHER_ERROR_INTERNAL;
}

static TeccFetcherErrorCode
fetch_dek_by_name(const TeccState *state,
                 const char *key_name,
                 const char *key_namespace,
                 PGconn *ext_conn,
                 TeccDekCacheItem **result)
{
    int cache_error;

    cache_error = tecc_fetch_dek_metadata_for_cache(state, key_name, key_namespace, ext_conn, result);
    return (cache_error == TECC_CACHE_OK) ? TECC_FETCHER_OK : TECC_FETCHER_ERROR_INTERNAL;
}

static TeccFetcherErrorCode
fetch_dek_by_oid(const TeccState *state,
                 Oid dek_oid,
                 TeccDekCacheItem **result)
{
    int cache_error;

    cache_error = tecc_fetch_dek_metadata_for_cache_by_oid(state, dek_oid, result);
    return (cache_error == TECC_CACHE_OK) ? TECC_FETCHER_OK : TECC_FETCHER_ERROR_INTERNAL;
}

static TeccFetcherErrorCode
fetch_dek_batch(const TeccState *state,
               const char *namespace_filter,
               TeccDekCacheItem ***result,
               int *count)
{
    int cache_error;

    cache_error = tecc_fetch_dek_list_for_cache(state, namespace_filter, result, count);
    return (cache_error == TECC_CACHE_OK) ? TECC_FETCHER_OK : TECC_FETCHER_ERROR_INTERNAL;
}

static TeccFetcherErrorCode
fetch_column_by_oid(const TeccState *state,
                   Oid table_oid,
                   int column_attnum,
                   TeccColumnCacheItem **result)
{
    int cache_error;

    cache_error = tecc_fetch_column_metadata_for_cache(state, table_oid, column_attnum, result);
    return (cache_error == TECC_CACHE_OK) ? TECC_FETCHER_OK : TECC_FETCHER_ERROR_INTERNAL;
}

static TeccFetcherErrorCode
fetch_column_by_name(const TeccState *state,
                    const char *database_name,
                    const char *schema_name,
                    const char *table_name,
                    const char *column_name,
                    TeccColumnCacheItem **result)
{
    int cache_error;

    cache_error = tecc_fetch_column_metadata_by_name_for_cache(state, database_name,
                                                              schema_name, table_name,
                                                              column_name, result);
    return (cache_error == TECC_CACHE_OK) ? TECC_FETCHER_OK : TECC_FETCHER_ERROR_INTERNAL;
}

static TeccFetcherErrorCode
fetch_columns_batch_by_oid(const TeccState *state,
                          Oid table_oid,
                          TeccColumnCacheItem ***result,
                          int *count)
{
    int cache_error;

    cache_error = tecc_fetch_all_columns_for_table_for_cache(state, table_oid, result, count);
    return (cache_error == TECC_CACHE_OK) ? TECC_FETCHER_OK : TECC_FETCHER_ERROR_INTERNAL;
}

static TeccFetcherErrorCode
fetch_columns_batch_by_name(const TeccState *state,
                           const char *database_name,
                           const char *schema_name,
                           const char *table_name,
                           TeccColumnCacheItem ***result,
                           int *count)
{
    int cache_error;

    cache_error = tecc_fetch_all_columns_for_table_by_name_for_cache(state, database_name,
                                                                    schema_name, table_name,
                                                                    result, count);
    return (cache_error == TECC_CACHE_OK) ? TECC_FETCHER_OK : TECC_FETCHER_ERROR_INTERNAL;
}

/* End - ReqID:TECC4.0 - 简化接口实现 */

/* End - ReqID:TECC4.0 */
