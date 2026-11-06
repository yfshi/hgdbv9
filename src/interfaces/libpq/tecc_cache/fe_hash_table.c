/* Begin - ReqID:TECC4.0 */
/*
 * File: fe_hash_table.c
 * Abstract:
 *   前端可用的简单哈希表实现，用于TECC缓存模块。
 *   基于开放地址法的线性探测哈希表。
 *
 * Authored by <Claude Code>, 2026-04-16.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_cache/fe_hash_table.c
 */

#include "postgres_fe.h"
#include "fe_hash_table.h"
#include "../tecc_common/tecc_logging.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* 哈希表条目状态 */
typedef enum {
    FE_HASH_EMPTY = 0,
    FE_HASH_OCCUPIED,
    FE_HASH_DELETED
} FeHashEntryState;

/* 哈希表条目 */
typedef struct FeHashEntry {
    FeHashEntryState state;      /* 条目状态 */
    void *key;                   /* 键数据 */
    void *data;                  /* 值数据 */
} FeHashEntry;

/* 哈希表结构 */
struct FeHashTable {
    char *tabname;               /* 表名（用于调试） */
    size_t keysize;              /* 键大小 */
    size_t entrysize;            /* 条目大小（键+值） */
    FeHashFunc hashfunc;         /* 哈希函数 */
    FeHashCompareFunc matchfunc; /* 比较函数 */
    size_t nelem;                /* 初始大小 */
    size_t size;                 /* 当前大小 */
    size_t count;                /* 已使用条目数 */
    FeHashEntry *entries;        /* 条目数组 */
    pthread_mutex_t lock;        /* 哈希表内部互斥锁（线程安全，兼容性保留） */
    pthread_rwlock_t rwlock;     /* 哈希表内部读写锁（线程安全） */
    bool lock_initialized;       /* 锁是否已初始化 */
};

/* 哈希表常量 */
#define FE_HASH_DEFAULT_LOAD_FACTOR 0.75  /* 默认负载因子阈值 */
#define FE_HASH_MIN_SIZE 16               /* 最小哈希表大小 */
#define FE_HASH_GROWTH_FACTOR 2          /* 扩容增长因子 */

/* 内部函数声明 */


/*
 * fe_hash_create - 创建哈希表
 */
FeHashTable *
fe_hash_create(const char *tabname,
               size_t keysize,
               size_t entrysize,
               FeHashFunc hashfunc,
               FeHashCompareFunc matchfunc,
               size_t nelem)
{
    FeHashTable *htab;
    size_t i;

    /* 参数检查 */
    if (tabname == NULL || keysize == 0 || entrysize == 0 ||
        hashfunc == NULL || matchfunc == NULL || nelem == 0) {
        return NULL;
    }

    /* 分配哈希表结构 */
    htab = (FeHashTable *)pg_malloc(sizeof(FeHashTable));
    if (htab == NULL) {
        return NULL;
    }

    /* 复制表名 */
    htab->tabname = strdup(tabname);
    if (htab->tabname == NULL) {
        pg_free(htab);
        return NULL;
    }

    /* 设置参数 */
    htab->keysize = keysize;
    htab->entrysize = entrysize;
    htab->hashfunc = hashfunc;
    htab->matchfunc = matchfunc;
    htab->nelem = nelem;
    htab->size = nelem * 2;  /* 使用两倍大小以减少冲突 */
    htab->count = 0;

    /* 分配条目数组 */
    htab->entries = (FeHashEntry *)calloc(htab->size, sizeof(FeHashEntry));
    if (htab->entries == NULL) {
        pg_free(htab->tabname);
        pg_free(htab);
        return NULL;
    }

    /* 初始化所有条目为空 */
    for (i = 0; i < htab->size; i++) {
        htab->entries[i].state = FE_HASH_EMPTY;
        htab->entries[i].key = NULL;
        htab->entries[i].data = NULL;
    }

    /* 初始化哈希表锁 */
    htab->lock_initialized = false;

    /* 初始化互斥锁（兼容性保留） */
    if (pthread_mutex_init(&htab->lock, NULL) != 0) {
        TECC_LOG(TECC_LOG_WARNING, "Failed to initialize hash table mutex lock for '%s'", tabname);
    }

    /* 初始化读写锁 */
    if (pthread_rwlock_init(&htab->rwlock, NULL) == 0) {
        htab->lock_initialized = true;
    } else {
        /* 读写锁初始化失败，记录错误但继续 */
        TECC_LOG(TECC_LOG_WARNING, "Failed to initialize hash table rwlock for '%s'", tabname);
    }

    return htab;
}

/*
 * fe_hash_destroy - 销毁哈希表
 */
void
fe_hash_destroy(FeHashTable *htab)
{
    size_t i;

    if (htab == NULL) {
        return;
    }

    /* 释放所有条目 */
    if (htab->entries != NULL) {
        for (i = 0; i < htab->size; i++) {
            if (htab->entries[i].key != NULL) {
                pg_free(htab->entries[i].key);
            }
            if (htab->entries[i].data != NULL) {
                pg_free(htab->entries[i].data);
            }
        }
        pg_free(htab->entries);
    }

    /* 销毁哈希表锁 */
    if (htab->lock_initialized) {
        pthread_rwlock_destroy(&htab->rwlock);
        htab->lock_initialized = false;
    }

    /* 销毁互斥锁（兼容性保留） */
    pthread_mutex_destroy(&htab->lock);

    /* 释放表名 */
    if (htab->tabname != NULL) {
        pg_free(htab->tabname);
    }

    /* 释放哈希表结构 */
    pg_free(htab);
}

/*
 * find_entry - 查找键对应的条目
 */
static FeHashEntry *
find_entry(FeHashTable *htab, const void *keyPtr, uint32 *hashval)
{
    uint32 hash;
    size_t idx;
    size_t start_idx;
    size_t probe;

    /* 计算哈希值 */
    hash = htab->hashfunc(keyPtr, htab->keysize);
    if (hashval != NULL) {
        *hashval = hash;
    }

    /* 计算初始索引 */
    start_idx = hash % htab->size;
    idx = start_idx;
    probe = 0;

    /* 线性探测 */
    while (probe < htab->size) {
        FeHashEntry *entry = &htab->entries[idx];

        if (entry->state == FE_HASH_EMPTY) {
            /* 找到空槽位 */
            return entry;
        } else if (entry->state == FE_HASH_OCCUPIED &&
                   htab->matchfunc(entry->key, keyPtr, htab->keysize) == 0) {
            /* 找到匹配的条目 */
            return entry;
        }

        /* 继续探测 */
        probe++;
        idx = (start_idx + probe) % htab->size;
    }

    /* 哈希表已满 */
    return NULL;
}

/*
 * fe_hash_search - 在哈希表中查找或创建条目
 */
void *
fe_hash_search(FeHashTable *htab,
               const void *keyPtr,
               int action,
               bool *foundPtr)
{
    FeHashEntry *entry;
    uint32 hashval;
    bool found = false;
    void *result = NULL;
    double load_factor;
    bool need_resize;
    size_t new_size;
    int resize_result;

    /* 参数检查 */
    if (htab == NULL || keyPtr == NULL) {
        if (foundPtr != NULL) {
            *foundPtr = false;
        }
        return NULL;
    }

    /* 根据操作类型获取适当的锁 */
    if (action == HASH_FIND) {
        /* 读操作：使用读锁 */
        FE_HASH_RLOCK(htab);
    } else {
        /* 写操作：使用写锁 */
        FE_HASH_WLOCK(htab);
    }

    /* 查找条目 */
    entry = find_entry(htab, keyPtr, &hashval);
    if (entry == NULL) {
        /* 哈希表已满 */
        if (foundPtr != NULL) {
            *foundPtr = false;
        }
        FE_HASH_RWUNLOCK(htab);
        return NULL;
    }

    /* 检查是否找到 */
    if (entry->state == FE_HASH_OCCUPIED) {
        found = true;
    }

    /* 根据操作类型处理 */
    switch (action) {
        case HASH_FIND:
            if (!found) {
                if (foundPtr != NULL) {
                    *foundPtr = false;
                }
                FE_HASH_RWUNLOCK(htab);
                return NULL;
            }
            result = entry->data;
            break;

        case HASH_ENTER:
        case HASH_ENTER_NULL:
            if (!found) {
                /* 检查是否需要扩容（先检查负载因子，避免在持有锁的情况下扩容） */
                load_factor = (double)htab->count / (double)htab->size;
                need_resize = (load_factor > FE_HASH_DEFAULT_LOAD_FACTOR);

                if (need_resize) {
                    /* 需要扩容：释放锁，执行扩容，然后重新获取锁并查找 */
                    FE_HASH_RWUNLOCK(htab);

                    /* 执行扩容 */
                    new_size = htab->size * FE_HASH_GROWTH_FACTOR;
                    if (new_size < FE_HASH_MIN_SIZE) {
                        new_size = FE_HASH_MIN_SIZE;
                    }
                    resize_result = fe_hash_resize(htab, new_size);

                    /* 重新获取锁并查找 */
                    if (action == HASH_FIND) {
                        FE_HASH_RLOCK(htab);
                    } else {
                        FE_HASH_WLOCK(htab);
                    }

                    if (resize_result == 0) {
                        /* 扩容成功，需要重新查找（因为哈希表结构已改变） */
                        entry = find_entry(htab, keyPtr, &hashval);
                        if (entry == NULL) {
                            /* 哈希表已满（这不应该发生，因为刚刚扩容） */
                            if (foundPtr != NULL) {
                                *foundPtr = false;
                            }
                            FE_HASH_RWUNLOCK(htab);
                            return NULL;
                        }

                        /* 检查是否找到了现有条目 */
                        if (entry->state == FE_HASH_OCCUPIED) {
                            found = true;
                        }
                    }
                    /* 如果扩容失败，继续使用当前哈希表 */
                }

                if (!found) {
                    /* 分配键内存 */
                    entry->key = pg_malloc(htab->keysize);
                    if (entry->key == NULL) {
                        if (foundPtr != NULL) {
                            *foundPtr = false;
                        }
                        FE_HASH_RWUNLOCK(htab);
                        return NULL;
                    }
                    memcpy(entry->key, keyPtr, htab->keysize);

                    /* 分配值内存 */
                    entry->data = pg_malloc(htab->entrysize - htab->keysize);
                    if (entry->data == NULL) {
                        pg_free(entry->key);
                        entry->key = NULL;
                        if (foundPtr != NULL) {
                            *foundPtr = false;
                        }
                        FE_HASH_RWUNLOCK(htab);
                        return NULL;
                    }
                    memset(entry->data, 0, htab->entrysize - htab->keysize);

                    /* 更新状态和计数 */
                    entry->state = FE_HASH_OCCUPIED;
                    htab->count++;
                }
            }
            result = entry->data;
            break;

        case HASH_REMOVE:
            if (found) {
                /* 转移data缓冲区所有权给调用者，不释放data */
                result = entry->data;
                pg_free(entry->key);
                entry->key = NULL;
                entry->data = NULL;
                entry->state = FE_HASH_DELETED;
                htab->count--;
            } else {
                result = NULL;
            }
            break;

        default:
            /* 无效操作 */
            if (foundPtr != NULL) {
                *foundPtr = false;
            }
            FE_HASH_RWUNLOCK(htab);
            return NULL;
    }

    /* 设置found指针 */
    if (foundPtr != NULL) {
        *foundPtr = found;
    }

    /* 释放锁并返回结果 */
    FE_HASH_RWUNLOCK(htab);

    /* 返回数据指针 */
    return result;
}

/*
 * fe_hash_seq_init - 开始顺序扫描哈希表
 */
void
fe_hash_seq_init(FeHashSeqStatus *scanstatus, FeHashTable *htab)
{
    if (scanstatus == NULL || htab == NULL) {
        return;
    }

    scanstatus->htab = htab;
    scanstatus->cur_idx = 0;
    scanstatus->scan_in_progress = false;
}

/*
 * fe_hash_seq_search - 获取下一个条目
 */
void *
fe_hash_seq_search(FeHashSeqStatus *scanstatus)
{
    FeHashTable *htab;
    FeHashEntry *entry;
    void *result = NULL;

    if (scanstatus == NULL || scanstatus->htab == NULL) {
        return NULL;
    }

    htab = scanstatus->htab;

    /* 获取哈希表读锁（顺序扫描是读操作） */
    FE_HASH_RLOCK(htab);

    /* 标记扫描进行中 */
    scanstatus->scan_in_progress = true;

    /* 查找下一个被占用的条目 */
    while (scanstatus->cur_idx < htab->size) {
        entry = &htab->entries[scanstatus->cur_idx];
        scanstatus->cur_idx++;

        if (entry->state == FE_HASH_OCCUPIED) {
            result = entry->data;
            break;
        }
    }

    /* 如果没有更多条目，标记扫描结束 */
    if (result == NULL) {
        scanstatus->scan_in_progress = false;
    }

    /* 释放锁 */
    FE_HASH_RWUNLOCK(htab);

    return result;
}

/*
 * fe_hash_seq_term - 结束顺序扫描
 */
void
fe_hash_seq_term(FeHashSeqStatus *scanstatus)
{
    if (scanstatus == NULL) {
        return;
    }

    /* 如果扫描还在进行中，确保释放锁 */
    if (scanstatus->scan_in_progress && scanstatus->htab != NULL) {
        /* 注意：这里不能直接解锁，因为锁在fe_hash_seq_search中已经释放了
         * 我们只需要重置状态 */
    }

    /* 重置状态 */
    scanstatus->htab = NULL;
    scanstatus->cur_idx = 0;
    scanstatus->scan_in_progress = false;
}

/*
 * fe_hash_get_size - 获取哈希表当前大小
 */
size_t
fe_hash_get_size(FeHashTable *htab)
{
    size_t result;

    if (htab == NULL) {
        return 0;
    }

    /* 获取哈希表读锁（统计信息是读操作） */
    FE_HASH_RLOCK(htab);
    result = htab->size;
    FE_HASH_RWUNLOCK(htab);

    return result;
}

/*
 * fe_hash_get_count - 获取哈希表当前条目数
 */
size_t
fe_hash_get_count(FeHashTable *htab)
{
    size_t result;

    if (htab == NULL) {
        return 0;
    }

    /* 获取哈希表读锁（统计信息是读操作） */
    FE_HASH_RLOCK(htab);
    result = htab->count;
    FE_HASH_RWUNLOCK(htab);

    return result;
}

/*
 * fe_hash_get_load_factor - 计算哈希表负载因子
 */
double
fe_hash_get_load_factor(FeHashTable *htab)
{
    size_t count, size;

    if (htab == NULL) {
        return 0.0;
    }

    /* 获取哈希表读锁（统计信息是读操作） */
    FE_HASH_RLOCK(htab);

    count = htab->count;
    size = htab->size;

    FE_HASH_RWUNLOCK(htab);

    if (size == 0) {
        return 0.0;
    }

    return (double)count / (double)size;
}

/*
 * rehash_entry - 重新哈希单个条目到新哈希表
 */
static void
rehash_entry(FeHashTable *new_htab, FeHashEntry *old_entry)
{
    uint32 hashval;
    FeHashEntry *new_entry;

    if (old_entry == NULL || old_entry->state != FE_HASH_OCCUPIED) {
        return;
    }

    /* 在新哈希表中查找或创建位置 */
    new_entry = find_entry(new_htab, old_entry->key, &hashval);
    if (new_entry == NULL) {
        /* 新哈希表已满，这不应该发生 */
        return;
    }

    /* 检查是否已存在相同键的条目 */
    if (new_entry->state == FE_HASH_OCCUPIED) {
        /* 键已存在，这不应该发生，因为原哈希表中键是唯一的 */
        /* 跳过这个条目，避免重复 */
        return;
    }

    /* 分配新内存并复制数据 */
    new_entry->key = pg_malloc(new_htab->keysize);
    if (new_entry->key == NULL) {
        return;
    }
    memcpy(new_entry->key, old_entry->key, new_htab->keysize);

    new_entry->data = pg_malloc(new_htab->entrysize - new_htab->keysize);
    if (new_entry->data == NULL) {
        pg_free(new_entry->key);
        new_entry->key = NULL;
        return;
    }
    memcpy(new_entry->data, old_entry->data, new_htab->entrysize - new_htab->keysize);

    /* 更新状态 */
    new_entry->state = FE_HASH_OCCUPIED;

    /* 增加计数 */
    new_htab->count++;
}

/*
 * fe_hash_resize - 调整哈希表大小
 *
 * 参数:
 *   htab - 哈希表
 *   new_size - 新大小（必须大于当前条目数）
 *
 * 返回:
 *   0表示成功，-1表示失败
 */
int
fe_hash_resize(FeHashTable *htab, size_t new_size)
{
    FeHashTable *new_htab = NULL;
    FeHashEntry *old_entries = NULL;
    size_t old_size = 0;
    size_t i;

    /* 参数检查 */
    if (htab == NULL || new_size == 0) {
        return -1;
    }

    /* 获取哈希表写锁（扩容是写操作） */
    FE_HASH_WLOCK(htab);

    /* 新大小必须大于当前条目数 */
    if (new_size <= htab->count) {
        FE_HASH_RWUNLOCK(htab);
        return -1;
    }

    /* 创建临时哈希表结构 */
    new_htab = (FeHashTable *)pg_malloc(sizeof(FeHashTable));
    if (new_htab == NULL) {
        FE_HASH_RWUNLOCK(htab);
        return -1;
    }

    /* 复制基本参数 */
    new_htab->tabname = strdup(htab->tabname);
    if (new_htab->tabname == NULL) {
        pg_free(new_htab);
        FE_HASH_RWUNLOCK(htab);
        return -1;
    }

    new_htab->keysize = htab->keysize;
    new_htab->entrysize = htab->entrysize;
    new_htab->hashfunc = htab->hashfunc;
    new_htab->matchfunc = htab->matchfunc;
    new_htab->nelem = htab->nelem;
    new_htab->size = new_size;
    new_htab->count = 0;

    /* 分配新条目数组 */
    new_htab->entries = (FeHashEntry *)calloc(new_size, sizeof(FeHashEntry));
    if (new_htab->entries == NULL) {
        pg_free(new_htab->tabname);
        pg_free(new_htab);
        FE_HASH_RWUNLOCK(htab);
        return -1;
    }

    /* 初始化新条目数组 */
    for (i = 0; i < new_size; i++) {
        new_htab->entries[i].state = FE_HASH_EMPTY;
        new_htab->entries[i].key = NULL;
        new_htab->entries[i].data = NULL;
    }

    /* 保存旧条目数组和大小 */
    old_entries = htab->entries;
    old_size = htab->size;

    /* 重新哈希所有条目 */
    for (i = 0; i < old_size; i++) {
        if (old_entries[i].state == FE_HASH_OCCUPIED) {
            rehash_entry(new_htab, &old_entries[i]);
            /* 释放旧条目的内存 */
            pg_free(old_entries[i].key);
            pg_free(old_entries[i].data);
            old_entries[i].key = NULL;
            old_entries[i].data = NULL;
        }
    }

    /* 更新哈希表结构 */
    htab->size = new_size;
    htab->entries = new_htab->entries;

    /* 释放旧条目数组 */
    pg_free(old_entries);

    /* 释放临时结构 */
    pg_free(new_htab->tabname);
    pg_free(new_htab);

    /* 释放锁并返回成功 */
    FE_HASH_RWUNLOCK(htab);
    return 0;
}


/*
 * fe_hash_search_ex - 在哈希表中查找或创建条目（扩展版本）
 *
 * 参数:
 *   htab - 哈希表
 *   keyPtr - 键指针
 *   action - 操作类型
 *   foundPtr - 是否找到的指针
 *   external_lock_held - 是否已经持有外部锁
 *
 * 返回:
 *   数据指针或NULL
 */
void *
fe_hash_search_ex(FeHashTable *htab,
                  const void *keyPtr,
                  int action,
                  bool *foundPtr,
                  bool external_lock_held)
{
    FeHashEntry *entry;
    uint32 hashval;
    bool found = false;
    void *result = NULL;
    double load_factor;
    bool need_resize;
    size_t new_size;
    int resize_result;

    /* 参数检查 */
    if (htab == NULL || keyPtr == NULL) {
        if (foundPtr != NULL) {
            *foundPtr = false;
        }
        return NULL;
    }

    /* 根据操作类型获取适当的锁（除非已经持有外部锁） */
    if (!external_lock_held) {
        if (action == HASH_FIND) {
            /* 读操作：使用读锁 */
            FE_HASH_RLOCK(htab);
        } else {
            /* 写操作：使用写锁 */
            FE_HASH_WLOCK(htab);
        }
    }

    /* 查找条目 */
    entry = find_entry(htab, keyPtr, &hashval);
    if (entry == NULL) {
        /* 哈希表已满 */
        if (foundPtr != NULL) {
            *foundPtr = false;
        }
        if (!external_lock_held) {
            FE_HASH_RWUNLOCK(htab);
        }
        return NULL;
    }

    /* 检查是否找到 */
    if (entry->state == FE_HASH_OCCUPIED) {
        found = true;
    }

    /* 根据操作类型处理 */
    switch (action) {
        case HASH_FIND:
            if (!found) {
                if (foundPtr != NULL) {
                    *foundPtr = false;
                }
                if (!external_lock_held) {
                    FE_HASH_RWUNLOCK(htab);
                }
                return NULL;
            }
            result = entry->data;
            break;

        case HASH_ENTER:
        case HASH_ENTER_NULL:
            if (!found) {
                /* 检查是否需要扩容（先检查负载因子） */
                load_factor = (double)htab->count / (double)htab->size;
                need_resize = (load_factor > FE_HASH_DEFAULT_LOAD_FACTOR);

                if (need_resize) {
                    /* 需要扩容 */
                    if (!external_lock_held) {
                        /* 释放锁，执行扩容，然后重新获取锁并查找 */
                        FE_HASH_RWUNLOCK(htab);

                        /* 执行扩容 */
                        new_size = htab->size * FE_HASH_GROWTH_FACTOR;
                        if (new_size < FE_HASH_MIN_SIZE) {
                            new_size = FE_HASH_MIN_SIZE;
                        }
                        resize_result = fe_hash_resize(htab, new_size);

                        /* 重新获取锁并查找 */
                        if (action == HASH_FIND) {
                            FE_HASH_RLOCK(htab);
                        } else {
                            FE_HASH_WLOCK(htab);
                        }

                        if (resize_result == 0) {
                            /* 扩容成功，需要重新查找 */
                            entry = find_entry(htab, keyPtr, &hashval);
                            if (entry == NULL) {
                                /* 哈希表已满 */
                                if (foundPtr != NULL) {
                                    *foundPtr = false;
                                }
                                if (!external_lock_held) {
                                    FE_HASH_RWUNLOCK(htab);
                                }
                                return NULL;
                            }

                            /* 检查是否找到了现有条目 */
                            if (entry->state == FE_HASH_OCCUPIED) {
                                found = true;
                            }
                        }
                        /* 如果扩容失败，继续使用当前哈希表 */
                    } else {
                        /* 外部锁已持有，不能释放锁，跳过扩容（避免死锁） */
                        /* 记录警告或采取其他策略 */
                    }
                }

                if (!found) {
                    /* 分配键内存 */
                    entry->key = pg_malloc(htab->keysize);
                    if (entry->key == NULL) {
                        if (foundPtr != NULL) {
                            *foundPtr = false;
                        }
                        if (!external_lock_held) {
                            FE_HASH_RWUNLOCK(htab);
                        }
                        return NULL;
                    }
                    memcpy(entry->key, keyPtr, htab->keysize);

                    /* 分配值内存 */
                    entry->data = pg_malloc(htab->entrysize - htab->keysize);
                    if (entry->data == NULL) {
                        pg_free(entry->key);
                        entry->key = NULL;
                        if (foundPtr != NULL) {
                            *foundPtr = false;
                        }
                        if (!external_lock_held) {
                            FE_HASH_RWUNLOCK(htab);
                        }
                        return NULL;
                    }
                    memset(entry->data, 0, htab->entrysize - htab->keysize);

                    /* 更新状态和计数 */
                    entry->state = FE_HASH_OCCUPIED;
                    htab->count++;
                }
            }
            result = entry->data;
            break;

        case HASH_REMOVE:
            if (found) {
                /* 转移data缓冲区所有权给调用者，不释放data */
                result = entry->data;
                pg_free(entry->key);
                entry->key = NULL;
                entry->data = NULL;
                entry->state = FE_HASH_DELETED;
                htab->count--;
            } else {
                result = NULL;
            }
            break;

        default:
            /* 无效操作 */
            if (foundPtr != NULL) {
                *foundPtr = false;
            }
            if (!external_lock_held) {
                FE_HASH_RWUNLOCK(htab);
            }
            return NULL;
    }

    /* 设置found指针 */
    if (foundPtr != NULL) {
        *foundPtr = found;
    }

    /* 释放锁（除非已经持有外部锁）并返回结果 */
    if (!external_lock_held) {
        FE_HASH_RWUNLOCK(htab);
    }

    /* 返回数据指针 */
    return result;
}

/* End - ReqID:TECC4.0 */