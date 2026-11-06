/* Begin - ReqID:TECC4.0 */
/*-------------------------------------------------------------------------
 *
 * tecc_data_fetcher_types.h
 *    TECC data fetcher module type definitions
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/interfaces/libpq/tecc_data_fetcher/include/tecc_data_fetcher_types.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_DATA_FETCHER_TYPES_H
#define TECC_DATA_FETCHER_TYPES_H

#include "postgres_fe.h"
#include "libpq-fe.h"

/* KEK参数结构 */
typedef struct TeccKeyArg
{
    char *function_name;    /* 函数名称 */
    char *key;              /* 参数键 */
    char *value;            /* 参数值 */
} TeccKeyArg;

/* KEK元数据结构 */
typedef struct TeccKekMetadata
{
    Oid kek_oid;            /* KEK对象标识符 */
    char *key_name;         /* KEK名称 */
    char *key_namespace;    /* 命名空间 */
    Oid key_owner_oid;      /* 所有者OID - 新增字段 */
    char *key_owner_name;   /* 所有者名称 - 重命名原字段 */
    char *key_acl;          /* 访问控制列表 */
    time_t create_date;     /* 创建时间 */

    /* 参数列表 */
    TeccKeyArg *args;       /* KEK参数数组 */
    int args_count;         /* 参数数量 */
} TeccKekMetadata;

/* DEK元数据结构 */
typedef struct TeccDekMetadata
{
    char *key_name;         /* DEK名称 */
    Oid dek_oid;            /* DEK对象标识符 */
    Oid column_key_distributed_id; /* 分布式ID */
    Oid kek_oid;            /* 关联KEK OID */
    char *kek_name;         /* 关联的KEK名称 */
    char *key_namespace;    /* 命名空间 */
    Oid key_owner_oid;      /* 所有者OID - 新增字段 */
    char *key_owner_name;   /* 所有者名称 - 重命名原字段 */
    char *key_acl;          /* 访问控制列表 */
    time_t create_date;     /* 创建时间 */

    /* 参数列表 */
    TeccKeyArg *args;       /* DEK参数数组 */
    int args_count;         /* 参数数量 */
} TeccDekMetadata;

/* 加密列元数据结构 - 前向声明，实际定义在tecc_metadata_cache.h中 */
typedef struct TeccEncryptedColumnMetadata TeccEncryptedColumnMetadata;

/* 游标接口定义 */
typedef struct TeccCursorInterface
{
    /* 打开游标 */
    int (*cursor_open)(struct TeccCursorInterface *cursor, const char *query);

    /* 获取下一行 */
    int (*fetch_next)(struct TeccCursorInterface *cursor);

    /* 获取当前行的列值 */
    const char *(*get_column)(struct TeccCursorInterface *cursor, int column_index);

    /* 获取列数 */
    int (*get_column_count)(struct TeccCursorInterface *cursor);

    /* 获取列名 */
    const char *(*get_column_name)(struct TeccCursorInterface *cursor, int column_index);

    /* 移动到下一行（兼容现有代码） */
    int (*next)(struct TeccCursorInterface *cursor);

    /* 关闭游标 */
    void (*close)(struct TeccCursorInterface *cursor);

    /* 销毁游标 */
    void (*destroy)(struct TeccCursorInterface *cursor);

    /* 私有数据 */
    void *private_data;
} TeccCursorInterface;

/* libpq游标实现 */
typedef struct TeccLibPQCursor
{
    TeccCursorInterface base;   /* 基础接口 */
    PGconn *conn;               /* libpq连接 */
    PGresult *result;           /* 查询结果 */
    int current_row;            /* 当前行索引 */
    int row_count;              /* 总行数 */
    int column_count;           /* 列数 */
    bool conn_is_temp;          /* 连接是否为临时连接（需要关闭） */
} TeccLibPQCursor;

#endif /* TECC_DATA_FETCHER_TYPES_H */
/* End - ReqID:TECC4.0 */