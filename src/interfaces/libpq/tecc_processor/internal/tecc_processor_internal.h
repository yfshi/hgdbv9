/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_processor_internal.h
 *
 * Abstract:
 *   Internal definitions and utilities for TECC processor module.
 *   Provides common data structures, utility functions, and internal
 *   interfaces used across all processor submodules.
 *
 * Authored by <wangxp@highgo.com>, 20260403.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/internal/tecc_processor_internal.h
 */

#ifndef TECC_PROCESSOR_INTERNAL_H
#define TECC_PROCESSOR_INTERNAL_H

#include "../tecc_processor.h"

/* Frontend code should not include PostgreSQL backend headers */
/* Use standard C headers and forward declarations instead */
#include <stddef.h>  /* For size_t */
#include <stdbool.h> /* For bool */
#include <stdint.h>  /* For integer types */

/* Include frontend parser types for List and list functions */
#include "../../frontend_parser/tecc_nodes.h"

/* Forward declarations for PostgreSQL types used in frontend */
typedef struct List List;
typedef struct MemoryContextData *MemoryContext;

/*
 * Internal error codes for processor module
 */
typedef enum TeccProcessorInternalError
{
    TECC_PROCESSOR_INTERNAL_SUCCESS = 0,
    TECC_PROCESSOR_INTERNAL_GENERIC_ERROR = -1,
    TECC_PROCESSOR_INTERNAL_MEMORY_ERROR = -2,
    TECC_PROCESSOR_INTERNAL_INVALID_ARGUMENT = -3,
    TECC_PROCESSOR_INTERNAL_NOT_FOUND = -4,
    TECC_PROCESSOR_INTERNAL_ALREADY_EXISTS = -5,
    TECC_PROCESSOR_INTERNAL_UNSUPPORTED = -6
} TeccProcessorInternalError;

/*
 * Memory context management
 */
#define TECC_PROCESSOR_MEMORY_CONTEXT "TECC Processor Context"

/*
 * Utility macros
 */
#define TECC_PROCESSOR_MAX_ERROR_MSG_LEN 256
#define TECC_PROCESSOR_MAX_COLUMN_NAME_LEN 64
#define TECC_PROCESSOR_MAX_TABLE_NAME_LEN 64
#define TECC_PROCESSOR_MAX_SCHEMA_NAME_LEN 64

/*
 * Encryption type bitmask constants
 * encryption_type 字段的位掩码定义:
 *   0x01 = DET (Deterministic)
 *   0x02 = OPE (Order-Preserving)
 *   0x04 = SEARCH (Searchable)
 */
#define TECC_ENC_TYPE_DET    0x01
#define TECC_ENC_TYPE_OPE    0x02
#define TECC_ENC_TYPE_SEARCH 0x04
#define TECC_ENC_TYPE_HE_ADD 0x08
#define TECC_ENC_TYPE_HE_MUL 0x10
#define TECC_ENC_TYPE_DET_OPE (TECC_ENC_TYPE_DET | TECC_ENC_TYPE_OPE)

/*
 * Debug and logging
 */
/* Include common logging utilities */
#include "../../tecc_common/tecc_logging.h"

/* 向后兼容的调试级别定义（映射到PostgreSQL级别） */
#define TECC_PROCESSOR_DEBUG_LEVEL_ERROR   TECC_LOG_ERROR
#define TECC_PROCESSOR_DEBUG_LEVEL_WARN    TECC_LOG_WARNING
#define TECC_PROCESSOR_DEBUG_LEVEL_INFO    TECC_LOG_INFO
#define TECC_PROCESSOR_DEBUG_LEVEL_DEBUG   TECC_LOG_DEBUG1  /* 默认DEBUG1 */

/* 新增层级化调试级别 */
#define TECC_PROCESSOR_DEBUG_LEVEL_DEBUG1  TECC_LOG_DEBUG1  /* 最上层 */
#define TECC_PROCESSOR_DEBUG_LEVEL_DEBUG2  TECC_LOG_DEBUG2  /* 上层 */
#define TECC_PROCESSOR_DEBUG_LEVEL_DEBUG3  TECC_LOG_DEBUG3  /* 中层 */
#define TECC_PROCESSOR_DEBUG_LEVEL_DEBUG4  TECC_LOG_DEBUG4  /* 下层 */
#define TECC_PROCESSOR_DEBUG_LEVEL_DEBUG5  TECC_LOG_DEBUG5  /* 最底层 */

/* 模块级调试宏（使用通用日志工具） */
#define tecc_processor_debug(level, msg, ...) \
    TECC_LOG(level, msg, ##__VA_ARGS__)

/* 线程安全调试宏（推荐使用） */
#define tecc_processor_debug_safe(state, level, msg, ...) \
    tecc_processor_debug(level, msg, ##__VA_ARGS__)

/*
 * Forward declarations for metadata types (temporary until cache module is implemented)
 * These types are defined in tecc_metadata_cache.h but we need forward declarations
 * to avoid compilation conflicts with other modules.
 */
typedef struct TeccGlobalKeyMetadata TeccGlobalKeyMetadata;
typedef struct TeccColumnKeyMetadata TeccColumnKeyMetadata;
typedef struct TeccEncryptedColumnMetadata TeccEncryptedColumnMetadata;

/*
 * Temporary column reference data structure for processor module
 * This is a simplified version that doesn't depend on PostgreSQL backend types
 * Will be replaced by the proper TeccColumnRefData from cache module when implemented
 */
#define TECC_PROCESSOR_MAX_NAME_LEN 64

typedef struct TeccProcessorColumnRef {
    char catalog_name[TECC_PROCESSOR_MAX_NAME_LEN];   /* 目录名 */
    char schema_name[TECC_PROCESSOR_MAX_NAME_LEN];    /* 模式名 */
    char table_name[TECC_PROCESSOR_MAX_NAME_LEN];     /* 表名 */
    char table_alias[TECC_PROCESSOR_MAX_NAME_LEN];    /* 表别名，无别名时与 table_name 相同 */
    char column_name[TECC_PROCESSOR_MAX_NAME_LEN];    /* 列名 */
    char alias_fqdn[TECC_PROCESSOR_MAX_NAME_LEN * 4]; /* 列全名别名 */
    bool is_encrypted;                                /* 是否为加密列 */
    Oid column_oid;                                   /* 列OID */
    Oid table_oid;                                    /* 表OID */
    int location;                                     /* 在SQL中的位置 */
    /* Bug 9323 - 与 TeccColumnRefData 保持前缀布局一致：encrypted_columns
     * 链表实际存的是 TeccColumnRefData，历史代码按本类型强转读取公共前缀。 */
    char column_alias[TECC_PROCESSOR_MAX_NAME_LEN];   /* 输出列别名，未重命名时为空 */
} TeccProcessorColumnRef;

/*
 * Function prototypes for internal utilities
 */

/* Memory management utilities - use pg_malloc/pg_free from fe_memutils.h */
/* Note: tecc_processor_palloc/pfree functions are deprecated and should not be used */
/* Frontend code must use pg_malloc()/pg_free() from fe_memutils.h */

/* Begin - ReqID:TECC4.0 - 安全内存擦除函数 */

/*
 * tecc_secure_wipe_memory - 安全擦除内存中的敏感数据
 *
 * 参数:
 *   ptr - 要擦除的内存指针
 *   size - 要擦除的内存大小（字节）
 *
 * 注意:
 *   这个函数使用多次写入来确保敏感数据被彻底清除
 *   即使编译器优化也可能无法消除这些写入操作
 */
static inline void
tecc_secure_wipe_memory(void *ptr, size_t size)
{
    volatile unsigned char *p = (volatile unsigned char *)ptr;
    size_t i;

    if (ptr == NULL || size == 0)
        return;

    /* 多次写入不同的模式以确保彻底清除 */
    for (i = 0; i < size; i++)
        p[i] = 0x00;  /* 第一次写入：全零 */

    for (i = 0; i < size; i++)
        p[i] = 0xFF;  /* 第二次写入：全一 */

    for (i = 0; i < size; i++)
        p[i] = 0x00;  /* 第三次写入：全零 */

    /* 最后调用内存屏障确保写入完成 */
#ifdef _MSC_VER
    _ReadWriteBarrier();
#else
    __asm__ __volatile__("" ::: "memory");
#endif
}

/*
 * tecc_secure_free - 安全释放包含敏感数据的内存
 *
 * 参数:
 *   ptr - 要释放的内存指针
 *   size - 内存块的大小（字节）
 *
 * 注意:
 *   先安全擦除内存，然后调用pg_free释放
 */
static inline void
tecc_secure_free(void *ptr, size_t size)
{
    if (ptr == NULL)
        return;

    tecc_secure_wipe_memory(ptr, size);
    pg_free(ptr);
}

/* End - ReqID:TECC4.0 - 安全内存擦除函数 */

/* Crypto buffer management for efficient memory sharing between modules */
/* TeccCryptoBuffer is defined in tecc_common_types.h */
typedef struct TeccCryptoBuffer TeccCryptoBuffer;

/* Create buffer owned by scheduler (scheduler responsible for freeing) */
TeccCryptoBuffer *tecc_processor_create_buffer(size_t size);

/* Borrow buffer for temporary use (caller responsible for freeing) */
TeccCryptoBuffer *tecc_processor_borrow_buffer(size_t size);

/* Release buffer (frees if owned_by_caller is true) */
void tecc_processor_release_buffer(TeccCryptoBuffer *buffer);

/* Reset buffer for reuse */
void tecc_processor_reset_buffer(TeccCryptoBuffer *buffer, size_t new_size);

/* Get buffer data pointer */
#define tecc_buffer_get_data(buffer) \
    ((buffer) ? (buffer)->data : NULL)

/* Get buffer size */
#define tecc_buffer_get_size(buffer) \
    ((buffer) ? (buffer)->size : 0)

/* Get used portion size */
#define tecc_buffer_get_used(buffer) \
    ((buffer) ? (buffer)->used : 0)

/* Set used portion size */
#define tecc_buffer_set_used(buffer, used_value) \
    do { \
        if ((buffer) && (used_value) <= (buffer)->size) \
            (buffer)->used = (used_value); \
    } while (0)

/* String utilities */
char *tecc_processor_strdup(const char *string);
bool tecc_processor_string_equal(const char *str1, const char *str2);
size_t tecc_processor_string_length(const char *string);

/* List utilities */
List *tecc_processor_list_append_string(List *list, const char *string);
bool tecc_processor_list_member_string(List *list, const char *string);
void tecc_processor_list_free_deep(List *list);

/* Simple list utilities for frontend code */
#define tecc_processor_list_length(list) \
    list_length(list)  /* Use the frontend parser's list_length function */

/* Error handling utilities */
void tecc_processor_set_error(TeccStatementData *stmt_data,
                             int error_code,
                             const char *error_format, ...);
void tecc_processor_clear_error(TeccStatementData *stmt_data);
const char *tecc_processor_get_error_message(int error_code);
bool tecc_processor_is_recoverable_error(int error_code);
bool tecc_processor_attempt_recovery(TeccStatementData *stmt_data, int error_code);
bool tecc_processor_set_error_with_recovery(TeccStatementData *stmt_data,
                                           int error_code,
                                           const char *error_format, ...);


/* Key management utilities for import/export module */
/* Begin - ReqID:TECC4.0 - Add key name retrieval functions */
int tecc_processor_get_key_names(PGconn *conn,
                                const char *table_name,
                                const char *column_name,
                                char **kek_name,
                                char **dek_name);
bool tecc_processor_validate_key_names(PGconn *conn,
                                      const char *kek_name,
                                      const char *dek_name);
int tecc_processor_get_kek_name_for_dek(PGconn *conn,
                                       const char *dek_name,
                                       char **kek_name);
/* End - ReqID:TECC4.0 - Add key name retrieval functions */

#endif /* TECC_PROCESSOR_INTERNAL_H */
/* End - ReqID:TECC4.0 */