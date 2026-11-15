/* Begin - ReqID:TECC4.0 */
/*-------------------------------------------------------------------------
 *
 * tecc_data_fetcher.h
 *    TECC data fetcher module public interface
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/interfaces/libpq/tecc_data_fetcher/include/tecc_data_fetcher.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_DATA_FETCHER_H
#define TECC_DATA_FETCHER_H

#include "postgres_fe.h"
#include "libpq-fe.h"
#include "common/fe_memutils.h"
#include "tecc_data_fetcher_types.h"
#include "../frontend_parser/tecc_nodes.h"  /* 前端List类型定义 */

/* Forward declarations */
typedef struct TeccState TeccState;

/* 错误码定义 */
typedef enum {
    TECC_FETCHER_OK = 0,
    TECC_FETCHER_ERROR_CONNECTION = 1,
    TECC_FETCHER_ERROR_PERMISSION = 2,
    TECC_FETCHER_ERROR_QUERY = 3,
    TECC_FETCHER_ERROR_PARSE = 4,
    TECC_FETCHER_ERROR_MEMORY = 5,
    TECC_FETCHER_ERROR_TIMEOUT = 6,
    TECC_FETCHER_ERROR_INVALID_PARAM = 7,
    TECC_FETCHER_ERROR_CACHE = 8,
    TECC_FETCHER_ERROR_SERIALIZATION = 9,
    TECC_FETCHER_ERROR_VALIDATION = 10,
    TECC_FETCHER_ERROR_CONFIG = 11,
    TECC_FETCHER_ERROR_INTERNAL = 12,
    TECC_FETCHER_ERROR_NOT_FOUND = 13,      /* 数据未找到 */
    TECC_FETCHER_ERROR_UNSUPPORTED = 14     /* 不支持的操作 */
} TeccFetcherErrorCode;

/* 加密算法位掩码定义 */
typedef enum {
    TECC_ALG_RND = 0x01,
    TECC_ALG_DET = 0x02,
    TECC_ALG_OPE = 0x04,
    TECC_ALG_SEARCH = 0x08
} TeccEncryptionAlgorithm;

/* 元数据结构定义 */
typedef struct TeccKekMetadata TeccKekMetadata;
typedef struct TeccDekMetadata TeccDekMetadata;
typedef struct TeccEncryptedColumnMetadata TeccEncryptedColumnMetadata;
typedef struct TeccKeyArg TeccKeyArg;

/* 游标接口 */
typedef struct TeccCursorInterface TeccCursorInterface;

/* 公共函数声明 */

/* 获取KEK元数据 */
extern TeccFetcherErrorCode tecc_fetch_kek_metadata(const TeccState *state,
                                                    const char *key_name,
                                                    const char *key_namespace,
                                                    PGconn *ext_conn,
                                                    TeccKekMetadata **result);

/* ReqID:TECC4.0 - Bug 9256: 按OID获取KEK元数据。
 * DEK→KEK 的绑定在目录里是 kek_id(OID)，跨 schema 时按名字+search_path 解析必然会
 * 找不到（KEK 可以创建在任何 schema），OID 才是无歧义的解析入口。 */
extern TeccFetcherErrorCode tecc_fetch_kek_metadata_by_oid(const TeccState *state,
                                                           Oid kek_oid,
                                                           TeccKekMetadata **result);


/* 获取DEK元数据 */
extern TeccFetcherErrorCode tecc_fetch_dek_metadata(const TeccState *state,
                                                    const char *key_name,
                                                    const char *key_namespace,
                                                    PGconn *ext_conn,
                                                    TeccDekMetadata **result);

/* 按OID获取DEK元数据 */
extern TeccFetcherErrorCode tecc_fetch_dek_metadata_by_oid(const TeccState *state,
                                                           Oid dek_oid,
                                                           TeccDekMetadata **result);


/* 获取加密列元数据 */
extern TeccFetcherErrorCode tecc_fetch_encrypted_column_metadata(
    const TeccState *state,
    Oid table_oid,
    int column_attnum,
    TeccEncryptedColumnMetadata **result);

/* 按名称获取加密列元数据 */
extern TeccFetcherErrorCode tecc_fetch_encrypted_column_metadata_by_name(
    const TeccState *state,
    const char *database_name,
    const char *schema_name,
    const char *table_name,
    const char *column_name,
    TeccEncryptedColumnMetadata **result);

/* 按名称获取表中所有加密列元数据 */
extern TeccFetcherErrorCode tecc_fetch_columns_by_table_by_name(
    const TeccState *state,
    const char *database_name,
    const char *schema_name,
    const char *table_name,
    List **result);

/* ReqID:TECC4.0 - 按 search_path 解析未限定表名的真实 schema */
extern TeccFetcherErrorCode tecc_fetch_resolve_table_schema(
    const TeccState *state,
    const char *database_name,
    const char *table_name,
    const char **schemas,
    int nschemas,
    char **resolved_schema);

/* ReqID:TECC4.0 - 判断指定 schema 下是否存在该关系。缓存层用它区分
 * "关系不存在"与"关系存在但无加密列"：只有后者可为会话固化"该表无加密列"
 * 的负缓存哨兵，前者固化后会让该表后续 DML 明文外发。 */
extern TeccFetcherErrorCode tecc_fetch_relation_exists(
    const TeccState *state,
    const char *database_name,
    const char *schema_name,
    const char *table_name,
    bool *exists);


/* 批量查询接口 */
extern TeccFetcherErrorCode tecc_fetch_kek_list(
    const TeccState *state,
    const char *namespace_filter,
    List **result);

extern TeccFetcherErrorCode tecc_fetch_dek_list(
    const TeccState *state,
    const char *namespace_filter,
    List **result);

extern TeccFetcherErrorCode tecc_fetch_columns_by_table(
    const TeccState *state,
    Oid table_oid,
    List **result);

/* 释放元数据内存 */
extern void tecc_free_kek_metadata(TeccKekMetadata *kek);
extern void tecc_free_dek_metadata(TeccDekMetadata *dek);
extern void tecc_free_encrypted_column_metadata(TeccEncryptedColumnMetadata *column);

/* 错误处理函数 */
extern const char *tecc_fetcher_error_message(TeccFetcherErrorCode error_code);
extern void tecc_log_fetcher_error(TeccFetcherErrorCode error_code, const char *context);

/* 调试日志函数 - 使用通用日志系统 */
/* 包含通用日志头文件以获取TECC_LOG宏定义 */
#ifndef TECC_LOGGING_H
#include "../tecc_common/tecc_logging.h"
#endif

/* 位掩码解析函数 */
extern bool tecc_has_algorithm(int bitmask, TeccEncryptionAlgorithm alg);
extern int tecc_parse_algorithms(const char *bitmask_str);

/* 权限检查函数 */
extern TeccFetcherErrorCode tecc_check_kek_permission(const TeccState *state,
                                                      const char *key_name, const char *key_namespace);
extern TeccFetcherErrorCode tecc_check_dek_permission(const TeccState *state,
                                                      const char *key_name, const char *key_namespace);
/* 查询执行函数 */
extern TeccCursorInterface *tecc_fetcher_execute_query(const TeccState *state, const char *query);

/* 连接级数据获取器管理函数 */
extern bool tecc_data_fetcher_init_for_state(TeccState *state);
extern void tecc_data_fetcher_cleanup_for_state(TeccState *state);
extern PGconn *tecc_create_temp_connection(const TeccState *state, const char *caller_desc);
extern void tecc_close_temp_connection(PGconn *conn, const char *caller_desc);
extern PGconn *tecc_data_fetcher_get_connection(const TeccState *state);
extern void tecc_data_fetcher_release_connection(const TeccState *state, PGconn *conn);

/* 用户OID转换函数 */
extern TeccFetcherErrorCode tecc_username_to_oid(const TeccState *state,
                                                const char *username,
                                                Oid *oid);
extern TeccFetcherErrorCode tecc_oid_to_username(const TeccState *state,
                                                Oid oid,
                                                char **username);

#endif /* TECC_DATA_FETCHER_H */
/* End - ReqID:TECC4.0 */