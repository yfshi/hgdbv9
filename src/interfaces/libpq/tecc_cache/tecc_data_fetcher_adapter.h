/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_data_fetcher_adapter.h
 * Abstract:
 *   TECC数据获取器适配器接口定义，基于简化设计的两层架构。
 *   提供与数据获取器模块的集成接口，实现委托加载机制。
 *   更新：去除工厂模式，使用TeccState参数，与数据获取器实际接口一致。
 *
 * Authored by <wangxp@highgo.com>, 2026-04-16.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_cache/tecc_data_fetcher_adapter.h
 */

#ifndef TECC_DATA_FETCHER_ADAPTER_H
#define TECC_DATA_FETCHER_ADAPTER_H

#include "tecc_metadata_cache.h"
#include "../tecc_data_fetcher/include/tecc_data_fetcher.h"

/* 前向声明TeccState结构，避免头文件循环依赖 */
typedef struct TeccState TeccState;

/* 缓存模块错误码定义 */
typedef enum {
    TECC_CACHE_OK = 0,                    /* 操作成功 */
    TECC_CACHE_ERROR_INVALID_PARAM = 1,   /* 无效参数 */
    TECC_CACHE_ERROR_NOT_FOUND = 2,       /* 未找到数据 */
    TECC_CACHE_ERROR_CONVERSION = 3,      /* 数据转换错误 */
    TECC_CACHE_ERROR_MEMORY = 4,          /* 内存分配错误 */
    TECC_CACHE_ERROR_CONNECTION = 5,      /* 连接错误 */
    TECC_CACHE_ERROR_PERMISSION = 6,      /* 权限错误 */
    TECC_CACHE_ERROR_QUERY = 7,           /* 查询错误 */
    TECC_CACHE_ERROR_INTERNAL = 8,        /* 内部错误 */
    TECC_CACHE_ERROR_NOT_INITIALIZED = 9, /* 缓存未初始化 */
    /* Begin - ReqID:TECC4.0 - 扩展细粒度错误码 */
    TECC_CACHE_ERROR_HASH_TABLE = 10,     /* 哈希表操作错误 */
    TECC_CACHE_ERROR_LOCK = 11,           /* 锁操作错误 */
    TECC_CACHE_ERROR_REFRESH = 12,        /* 缓存刷新错误 */
    TECC_CACHE_ERROR_INIT = 13,           /* 初始化错误 */
    TECC_CACHE_ERROR_CLEANUP = 14,        /* 清理错误 */
    TECC_CACHE_ERROR_STATE = 15,          /* 状态错误 */
    TECC_CACHE_ERROR_SERIALIZATION = 16,  /* 序列化错误 */
    TECC_CACHE_ERROR_DESERIALIZATION = 17, /* 反序列化错误 */
    TECC_CACHE_ERROR_VERSION = 18,        /* 版本不兼容错误 */
    TECC_CACHE_ERROR_CORRUPTION = 19,     /* 数据损坏错误 */
    TECC_CACHE_ERROR_TIMEOUT = 20,        /* 超时错误 */
    TECC_CACHE_ERROR_RESOURCE = 21,       /* 资源不足错误 */
    TECC_CACHE_ERROR_CONFIG = 22,         /* 配置错误 */
    TECC_CACHE_ERROR_UNSUPPORTED = 23,    /* 不支持的操作 */
    /* Begin - ReqID:TECC4.0 - 添加安全性和完整性错误码 */
    TECC_CACHE_ERROR_SECURITY = 24,       /* 安全性错误（如HMAC密钥生成失败） */
    TECC_CACHE_ERROR_INTEGRITY = 25       /* 数据完整性错误（如HMAC签名验证失败） */
    /* End - ReqID:TECC4.0 - 添加安全性和完整性错误码 */
} TeccCacheErrorCode;

/* ==================== 简化的适配器接口定义 ==================== */

/* 查询类型标志位 */
typedef enum {
    TECC_QUERY_BY_NAME = 0x01,      /* 按名称查询 */
    TECC_QUERY_BY_OID = 0x02,       /* 按OID查询 */
    TECC_QUERY_BATCH = 0x04,        /* 批量查询 */
    TECC_QUERY_WITH_NAMESPACE = 0x08 /* 包含命名空间过滤 */
} TeccQueryFlags;

/* 统一的缓存查询参数结构 */
typedef struct {
    TeccQueryFlags flags;           /* 查询标志位 */
    PGconn     *external_conn;      /* 外部连接（非NULL时使用，不创建临时连接） */
    union {
        struct {
            const char *name;       /* 名称查询参数 */
            const char *namespace;  /* 命名空间参数 */
        } by_name;
        struct {
            Oid oid;                /* OID查询参数 */
            int attnum;             /* 列号（仅用于列查询） */
        } by_oid;
        struct {
            const char *filter;     /* 批量查询过滤器 */
        } batch;
    } params;

    /* 表/列名称查询参数（仅用于列查询） */
    const char *database_name;
    const char *schema_name;
    const char *table_name;
    const char *column_name;
} TeccCacheQueryParams;

/*
 * tecc_fetch_kek_for_cache - 简化的KEK查询接口
 *
 * 参数:
 *   state - TECC状态结构
 *   query - 查询参数
 *   result - 返回结果（单个KEK或KEK数组）
 *   count - 返回数量（批量查询时有效）
 *
 * 返回:
 *   数据获取器错误码（直接使用，无需转换）
 */
TeccFetcherErrorCode tecc_fetch_kek_for_cache(const TeccState *state,
                                             const TeccCacheQueryParams *query,
                                             TeccKekCacheItem **result,
                                             int *count);

/*
 * tecc_fetch_dek_for_cache - 简化的DEK查询接口
 *
 * 参数:
 *   state - TECC状态结构
 *   query - 查询参数
 *   result - 返回结果（单个DEK或DEK数组）
 *   count - 返回数量（批量查询时有效）
 *
 * 返回:
 *   数据获取器错误码（直接使用，无需转换）
 */
TeccFetcherErrorCode tecc_fetch_dek_for_cache(const TeccState *state,
                                             const TeccCacheQueryParams *query,
                                             TeccDekCacheItem **result,
                                             int *count);

/*
 * tecc_fetch_column_for_cache - 简化的加密列查询接口
 *
 * 参数:
 *   state - TECC状态结构
 *   query - 查询参数
 *   result - 返回结果（单个列或列数组）
 *   count - 返回数量（批量查询时有效）
 *
 * 返回:
 *   数据获取器错误码（直接使用，无需转换）
 */
TeccFetcherErrorCode tecc_fetch_column_for_cache(const TeccState *state,
                                                const TeccCacheQueryParams *query,
                                                TeccColumnCacheItem **result,
                                                int *count);

/*
 * tecc_fetch_guc_for_cache - 简化的GUC参数查询接口
 *
 * 参数:
 *   state - TECC状态结构
 *   manager - 缓存管理器
 *
 * 返回:
 *   数据获取器错误码（直接使用，无需转换）
 */
TeccFetcherErrorCode tecc_fetch_guc_for_cache(const TeccState *state,
                                             TeccCacheManager *manager);

/* ==================== 简化的辅助函数接口 ==================== */

/*
 * tecc_cache_convert_error - 将数据获取器错误码转换为缓存模块错误码
 * （仅用于需要与缓存模块错误码兼容的场景）
 *
 * 参数:
 *   fetcher_error - 数据获取器错误码
 *
 * 返回:
 *   缓存模块错误码
 */
int tecc_cache_convert_error(TeccFetcherErrorCode fetcher_error);

/*
 * tecc_fetch_all_columns_for_table_by_name_for_cache - 为缓存模块按名称获取表中所有加密列元数据
 *
 * 参数:
 *   state - TECC状态结构
 *   database_name - 数据库名称
 *   schema_name - 模式名称
 *   table_name - 表名称
 *   column_items - 返回的列缓存项数组
 *   count - 返回的列数量
 *
 * 返回:
 *   缓存错误码
 */
int tecc_fetch_all_columns_for_table_by_name_for_cache(const TeccState *state,
                                                      const char *database_name,
                                                      const char *schema_name,
                                                      const char *table_name,
                                                      TeccColumnCacheItem ***column_items,
                                                      int *count);

/*
 * tecc_cache_error_message - 获取缓存模块错误消息
 *
 * 参数:
 *   error_code - 缓存错误码
 *
 * 返回:
 *   错误消息字符串
 */
const char *tecc_cache_error_message(int error_code);

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
const char *tecc_cache_error_context(int error_code, const char *function_name,
                                     int line_number, const char *additional_info);

/*
 * tecc_cache_log_error - 记录缓存模块错误日志
 *
 * 参数:
 *   fetcher_error - 数据获取器错误码
 *   context - 错误上下文信息
 */
void tecc_cache_log_error(TeccFetcherErrorCode fetcher_error, const char *context);

#endif /* TECC_DATA_FETCHER_ADAPTER_H */
/* End - ReqID:TECC4.0 */