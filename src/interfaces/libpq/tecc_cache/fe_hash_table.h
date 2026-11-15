/* Begin - ReqID:TECC4.0 */
/*
 * File: fe_hash_table.h
 * Abstract:
 *   前端可用的简单哈希表实现，用于TECC缓存模块。
 *   提供基本的哈希表功能，避免依赖后端hsearch.h。
 *
 * Authored by <wanghaoyan>, 2026-04-16.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_cache/fe_hash_table.h
 */

#ifndef FE_HASH_TABLE_H
#define FE_HASH_TABLE_H

#include "postgres_fe.h"
#include <stdbool.h>
#include <stddef.h>
#ifdef _WIN32
#include <port/win32/pthread.h>
#else
#include <pthread.h>
#endif

/* 哈希表句柄 */
typedef struct FeHashTable FeHashTable;

/* 顺序扫描状态 */
typedef struct FeHashSeqStatus {
    FeHashTable *htab;           /* 哈希表 */
    size_t cur_idx;              /* 当前索引 */
    bool scan_in_progress;       /* 扫描是否进行中 */
} FeHashSeqStatus;

/* 哈希函数类型 */
typedef uint32 (*FeHashFunc)(const void *key, size_t keysize);

/* 比较函数类型 */
typedef int (*FeHashCompareFunc)(const void *key1, const void *key2, size_t keysize);

/* 创建哈希表 */
FeHashTable *fe_hash_create(const char *tabname,
                           size_t keysize,
                           size_t entrysize,
                           FeHashFunc hashfunc,
                           FeHashCompareFunc matchfunc,
                           size_t nelem);

/* 销毁哈希表 */
void fe_hash_destroy(FeHashTable *htab);

/* 在哈希表中查找或创建条目 */
void *fe_hash_search(FeHashTable *htab,
                    const void *keyPtr,
                    int action,  /* HASH_FIND, HASH_ENTER, HASH_REMOVE, HASH_ENTER_NULL */
                    bool *foundPtr);

/* 线程安全版本：调用者可以指定是否已经持有外部锁 */
void *fe_hash_search_ex(FeHashTable *htab,
                       const void *keyPtr,
                       int action,
                       bool *foundPtr,
                       bool external_lock_held);

/* 开始顺序扫描哈希表 */
void fe_hash_seq_init(FeHashSeqStatus *scanstatus, FeHashTable *htab);

/* 获取下一个条目 */
void *fe_hash_seq_search(FeHashSeqStatus *scanstatus);

/* 结束顺序扫描 */
void fe_hash_seq_term(FeHashSeqStatus *scanstatus);

/* 哈希表统计信息 */
size_t fe_hash_get_size(FeHashTable *htab);
size_t fe_hash_get_count(FeHashTable *htab);
double fe_hash_get_load_factor(FeHashTable *htab);

/* 哈希表动态扩容 */
int fe_hash_resize(FeHashTable *htab, size_t new_size);

/* 哈希表操作标志 */
#define HASH_FIND        0
#define HASH_ENTER       1
#define HASH_REMOVE      2
#define HASH_ENTER_NULL  3

/* ==================== 哈希表线程安全锁操作宏 ==================== */

/* 哈希表锁操作宏（兼容性保留） */
#define FE_HASH_LOCK(htab) \
    do { \
        if ((htab) != NULL && (htab)->lock_initialized) { \
            pthread_mutex_lock(&(htab)->lock); \
        } \
    } while(0)

#define FE_HASH_UNLOCK(htab) \
    do { \
        if ((htab) != NULL && (htab)->lock_initialized) { \
            pthread_mutex_unlock(&(htab)->lock); \
        } \
    } while(0)

#define FE_HASH_TRY_LOCK(htab) \
    ((htab) != NULL && (htab)->lock_initialized ? \
     pthread_mutex_trylock(&(htab)->lock) == 0 : false)

/* ==================== 哈希表读写锁操作宏 ==================== */

/* 哈希表读锁 */
#define FE_HASH_RLOCK(htab) \
    do { \
        if ((htab) != NULL && (htab)->lock_initialized) { \
            pthread_rwlock_rdlock(&(htab)->rwlock); \
        } \
    } while(0)

/* 哈希表写锁 */
#define FE_HASH_WLOCK(htab) \
    do { \
        if ((htab) != NULL && (htab)->lock_initialized) { \
            pthread_rwlock_wrlock(&(htab)->rwlock); \
        } \
    } while(0)

/* 哈希表读写锁解锁 */
#define FE_HASH_RWUNLOCK(htab) \
    do { \
        if ((htab) != NULL && (htab)->lock_initialized) { \
            pthread_rwlock_unlock(&(htab)->rwlock); \
        } \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif /* FE_HASH_TABLE_H */
/* End - ReqID:TECC4.0 */