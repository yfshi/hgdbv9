/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_dek_cache.c
 * Abstract:
 *   TECC DEK缓存实现，基于简化设计的两层架构。
 *   提供DEK相关的缓存查询和管理功能。
 *
 * Authored by <Claude Code>, 2026-04-15.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_cache/tecc_dek_cache.c
 */

#include "postgres_fe.h"
#include "tecc_metadata_cache.h"
#include "tecc_data_fetcher_adapter.h"
#include "../tecc_libpq_integration/fe-tecc-integration.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/* 内部函数声明 */
static void destroy_dek_cache_item(TeccDekCacheItem *item);
static TeccDekCacheItem *find_dek_in_cache_by_oid(const TeccState *state, Oid dek_oid);
static TeccDekCacheItem *find_dek_in_cache_by_name(const TeccState *state, const char *key_name, const char *key_namespace);
static int insert_dek_into_cache(const TeccState *state, TeccDekCacheItem *item);
static int load_dek_from_database_by_oid(const TeccState *state, Oid dek_oid, TeccDekCacheItem **item);
static int load_dek_from_database_by_name(const TeccState *state, const char *key_name, const char *key_namespace, TeccDekCacheItem **item);

/* 辅助函数声明 */

/*
 * tecc_cache_get_dek_by_oid - 根据OID查询DEK信息
 *
 * 参数:
 *   dek_oid - DEK对象标识符
 *
 * 返回:
 *   DEK缓存项指针，NULL表示DEK不存在
 */
TeccDekCacheItem *
tecc_cache_get_dek_by_oid(const TeccState *state, Oid dek_oid)
{
    TeccCacheManager *manager = NULL;
    TeccDekCacheItem *cached_item = NULL;
    TeccDekCacheItem *new_item = NULL;
    int ret;
    bool auto_recovery_attempted = false;

    /* 输入验证和边界检查 */
    if (state == NULL) {
        TECC_CACHE_DEBUG("无效参数：state为NULL");
        return NULL;
    }
    if (dek_oid == InvalidOid) {
        TECC_CACHE_DEBUG("无效参数：dek_oid为InvalidOid");
        return NULL;
    }

retry_with_recovery:
    /* 1. 获取缓存管理器 */
    manager = get_cache_manager(state);
    if (manager == NULL) {
        /* 如果管理器为NULL且未尝试过自动恢复，尝试恢复 */
        if (!auto_recovery_attempted) {
            auto_recovery_attempted = true;
            TECC_CACHE_DEBUG("缓存管理器为NULL，尝试自动恢复");
            if (tecc_cache_auto_recover(state) == TECC_CACHE_OK) {
                goto retry_with_recovery;
            }
        }
        return NULL;
    }

    /* 2. 检查并执行定时刷新（如果需要） */
    perform_timed_refresh_if_needed(manager);

    /* 3. 检查DEK缓存是否需要刷新并加锁 */
    CACHE_LOCK(manager);
    if (manager->dek_cache_needs_refresh) {
        CACHE_UNLOCK(manager);
        /* 执行延迟刷新：获取写锁并清空缓存 */
        CACHE_WLOCK(manager);
        if (manager->dek_cache_needs_refresh) {
            int clear_ret;

            TECC_CACHE_DEBUG("执行DEK缓存延迟刷新");

            // 调用缓存清理函数
            clear_ret = tecc_cache_clear_specific(manager, TECC_CACHE_REFRESH_DEK);
            if (clear_ret != TECC_CACHE_OK) {
                TECC_CACHE_DEBUG("DEK缓存清理失败，错误码: %d", clear_ret);
                /* 清理失败，但仍然重置标记避免无限重试 */
                manager->dek_cache_needs_refresh = false;
            }

            TECC_CACHE_DEBUG("DEK缓存延迟刷新完成");
        }
        CACHE_RWUNLOCK(manager);
        /* 重新获取锁继续查询 */
        CACHE_LOCK(manager);
    }

    /* 3. 检查缓存命中 */
    cached_item = find_dek_in_cache_by_oid(state, dek_oid);
    if (cached_item != NULL) {
        /* 更新命中次数 */
        cached_item->hit_times++;
        /* 解锁并返回结果 */
        CACHE_UNLOCK(manager);
        return cached_item;
    }

    /* 4. 缓存未命中，解锁以便进行数据库查询（避免长时间持有锁） */
    CACHE_UNLOCK(manager);

    /* 5. 从数据库加载 */
    ret = load_dek_from_database_by_oid(state, dek_oid, &new_item);
    if (ret != TECC_CACHE_OK || new_item == NULL) {
        /* 查询失败或DEK不存在 */
        /* 如果是可恢复错误且未尝试过自动恢复，尝试恢复 */
        if (!auto_recovery_attempted && tecc_cache_is_recoverable_error(state, ret)) {
            auto_recovery_attempted = true;
            TECC_CACHE_DEBUG("数据库查询失败（错误码: %d），尝试自动恢复", ret);
            if (tecc_cache_auto_recover(state) == TECC_CACHE_OK) {
                goto retry_with_recovery;
            }
        }
        return NULL;
    }

    /* 6. 加锁插入缓存 */
    CACHE_LOCK(manager);

    /* 7. 再次检查缓存（防止其他线程已插入） */
    cached_item = find_dek_in_cache_by_oid(state, dek_oid);
    if (cached_item != NULL) {
        /* 其他线程已插入，使用现有缓存项 */
        destroy_dek_cache_item(new_item);
        cached_item->hit_times++;
        CACHE_UNLOCK(manager);
        return cached_item;
    }

    /* 8. 更新缓存 */
    if (insert_dek_into_cache(state, new_item) != TECC_CACHE_OK) {
        /* 插入缓存失败，清理资源 */
        CACHE_UNLOCK(manager);
        destroy_dek_cache_item(new_item);
        /* 如果是可恢复错误且未尝试过自动恢复，尝试恢复 */
        if (!auto_recovery_attempted) {
            auto_recovery_attempted = true;
            TECC_CACHE_DEBUG("缓存插入失败，尝试自动恢复");
            if (tecc_cache_auto_recover(state) == TECC_CACHE_OK) {
                goto retry_with_recovery;
            }
        }
        return NULL;
    }

    /* 9. 更新加载次数统计和缓存大小 */
    manager->load_count++;
    manager->current_size++;

    /* 10. 解锁并返回结果 */
    CACHE_UNLOCK(manager);
    return new_item;
}

/*
 * tecc_cache_get_dek_by_name - 根据名称查询DEK信息
 *
 * 参数:
 *   key_name - DEK名称
 *   key_namespace - 命名空间
 *
 * 返回:
 *   DEK缓存项指针，NULL表示DEK不存在
 */
TeccDekCacheItem *
tecc_cache_get_dek_by_name(const TeccState *state, const char *key_name, const char *key_namespace)
{
    TeccCacheManager *manager = NULL;
    TeccDekCacheItem *cached_item = NULL;
    TeccDekCacheItem *new_item = NULL;
    int ret;
    bool auto_recovery_attempted = false;
    const char *null_pos;

    TECC_CACHE_DEBUG("DEBUG: tecc_cache_get_dek_by_name: 开始查询DEK，name=%s.%s", key_namespace, key_name);

    /* 输入验证和边界检查 */
    if (state == NULL) {
        TECC_CACHE_DEBUG("无效参数：state为NULL");
        return NULL;
    }
    if (key_name == NULL) {
        TECC_CACHE_DEBUG("无效参数：key_name为NULL");
        return NULL;
    }
    if (key_namespace == NULL) {
        TECC_CACHE_DEBUG("无效参数：key_namespace为NULL");
        return NULL;
    }
    /* 检查字符串长度，防止缓冲区溢出 */
    if (strlen(key_name) >= NAMEDATALEN) {
        TECC_CACHE_DEBUG("无效参数：key_name长度超过限制（%d）", NAMEDATALEN);
        return NULL;
    }
    if (strlen(key_namespace) >= NAMEDATALEN) {
        TECC_CACHE_DEBUG("无效参数：key_namespace长度超过限制（%d）", NAMEDATALEN);
        return NULL;
    }
    /* 检查字符串是否包含非法字符（如空字符、控制字符等） */
    /* strchr会返回第一个空字符的位置，如果字符串中间有空字符，它会返回中间位置 */
    /* 我们需要检查空字符是否在字符串末尾（正常情况）还是在字符串中间（异常情况） */
    null_pos = strchr(key_name, '\0');
    if (null_pos != key_name + strlen(key_name)) {
        TECC_CACHE_DEBUG("无效参数：key_name字符串中间包含空字符");
        return NULL;
    }

    null_pos = strchr(key_namespace, '\0');
    if (null_pos != key_namespace + strlen(key_namespace)) {
        TECC_CACHE_DEBUG("无效参数：key_namespace字符串中间包含空字符");
        return NULL;
    }

retry_with_recovery:
    /* 1. 获取缓存管理器 */
    manager = get_cache_manager(state);
    if (manager == NULL) {
        /* 如果管理器为NULL且未尝试过自动恢复，尝试恢复 */
        if (!auto_recovery_attempted) {
            auto_recovery_attempted = true;
            TECC_CACHE_DEBUG("缓存管理器为NULL，尝试自动恢复");
            if (tecc_cache_auto_recover(state) == TECC_CACHE_OK) {
                goto retry_with_recovery;
            }
        }
        return NULL;
    }

    /* 2. 检查并执行定时刷新（如果需要） */
    perform_timed_refresh_if_needed(manager);

    /* 3. 检查DEK缓存是否需要刷新并加锁 */
    CACHE_LOCK(manager);
    if (manager->dek_cache_needs_refresh) {
        CACHE_UNLOCK(manager);
        /* 执行延迟刷新：获取写锁并清空缓存 */
        CACHE_WLOCK(manager);
        if (manager->dek_cache_needs_refresh) {
            int clear_ret;

            TECC_CACHE_DEBUG("执行DEK缓存延迟刷新");

            // 调用缓存清理函数
            clear_ret = tecc_cache_clear_specific(manager, TECC_CACHE_REFRESH_DEK);
            if (clear_ret != TECC_CACHE_OK) {
                TECC_CACHE_DEBUG("DEK缓存清理失败，错误码: %d", clear_ret);
                /* 清理失败，但仍然重置标记避免无限重试 */
                manager->dek_cache_needs_refresh = false;
            }

            TECC_CACHE_DEBUG("DEK缓存延迟刷新完成");
        }
        CACHE_RWUNLOCK(manager);
        /* 重新获取锁继续查询 */
        CACHE_LOCK(manager);
    }

    /* 3. 检查缓存命中 */
    cached_item = find_dek_in_cache_by_name(state, key_name, key_namespace);
    if (cached_item != NULL) {
        /* 负缓存命中 */
        if (cached_item->cache_status == TECC_CACHE_STATUS_NOT_FOUND) {
            TECC_CACHE_DEBUG("DEBUG: tecc_cache_get_dek_by_name: 负缓存命中（不存在），name=%s.%s",
                            key_namespace, key_name);
            CACHE_UNLOCK(manager);
            return NULL;
        }
        if (cached_item->cache_status == TECC_CACHE_STATUS_PERMISSION_DENIED) {
            TECC_CACHE_DEBUG("DEBUG: tecc_cache_get_dek_by_name: 负缓存命中（无权限），name=%s.%s",
                            key_namespace, key_name);
            CACHE_UNLOCK(manager);
            return NULL;
        }
        TECC_CACHE_DEBUG("DEBUG: tecc_cache_get_dek_by_name: 缓存命中，name=%s.%s, OID=%u",
                        key_namespace, key_name, cached_item->dek_oid);

        /* 批量查询加载的缓存项可能缺少 encrypted_key（args未包含在批量查询中）。
         * 如果 encrypted_key 为 NULL，需要单独查询加载完整数据。 */
        if (cached_item->encrypted_key == NULL && cached_item->key_length == 0)
        {
            TECC_CACHE_DEBUG("DEBUG: tecc_cache_get_dek_by_name: 缓存命中但encrypted_key为空，需从数据库重新加载");
            CACHE_UNLOCK(manager);
            goto load_from_database;
        }

        /* 更新命中次数 */
        cached_item->hit_times++;
        /* 解锁并返回结果 */
        CACHE_UNLOCK(manager);
        return cached_item;
    }
    TECC_CACHE_DEBUG("DEBUG: tecc_cache_get_dek_by_name: 缓存未命中，name=%s.%s", key_namespace, key_name);

load_from_database:

    /* 4. 缓存未命中，解锁以便进行数据库查询（避免长时间持有锁） */
    CACHE_UNLOCK(manager);

    /* 5. 从数据库加载 */
    ret = load_dek_from_database_by_name(state, key_name, key_namespace, &new_item);
    TECC_LOG(TECC_LOG_DEBUG1, "DEK CACHE: load_dek_from_database_by_name returned ret=%d, new_item=%p", ret, new_item);
    if (ret != TECC_CACHE_OK || new_item == NULL) {
        TECC_CACHE_DEBUG("DEBUG: tecc_cache_get_dek_by_name: 数据库加载失败，ret=%d, new_item=%p", ret, new_item);
        /* 如果是可恢复错误且未尝试过自动恢复，尝试恢复 */
        if (!auto_recovery_attempted && tecc_cache_is_recoverable_error(state, ret)) {
            auto_recovery_attempted = true;
            TECC_CACHE_DEBUG("数据库查询失败（错误码: %d），尝试自动恢复", ret);
            if (tecc_cache_auto_recover(state) == TECC_CACHE_OK) {
                goto retry_with_recovery;
            }
        }
        /* 对不可恢复的错误（不存在/无权限）插入负缓存 */
        if (ret == TECC_CACHE_ERROR_NOT_FOUND || ret == TECC_CACHE_ERROR_PERMISSION) {
            TeccDekCacheItem *sentinel = (TeccDekCacheItem *)pg_malloc0(sizeof(TeccDekCacheItem));
            if (sentinel != NULL) {
                sentinel->key_name = pg_strdup(key_name);
                sentinel->key_namespace = pg_strdup(key_namespace);
                if (ret == TECC_CACHE_ERROR_PERMISSION)
                    sentinel->cache_status = TECC_CACHE_STATUS_PERMISSION_DENIED;
                else
                    sentinel->cache_status = TECC_CACHE_STATUS_NOT_FOUND;
                CACHE_LOCK(manager);
                (void)insert_dek_into_cache(state, sentinel);
                CACHE_UNLOCK(manager);
            }
        }
        return NULL;
    }
    TECC_CACHE_DEBUG("DEBUG: tecc_cache_get_dek_by_name: 数据库加载成功，OID=%u, name=%s.%s",
                    new_item->dek_oid, new_item->key_namespace, new_item->key_name);

    /* 6. 加锁插入缓存 */
    CACHE_LOCK(manager);

    /* 7. 再次检查缓存（防止其他线程已插入）。
     *    如果缓存已有有效项（encrypted_key非空），用现有项；
     *    如果缓存项存在但encrypted_key为空（批量加载时丢失args导致），
     *    用新加载的完整项覆盖。 */
    cached_item = find_dek_in_cache_by_name(state, key_name, key_namespace);
    if (cached_item != NULL && cached_item->encrypted_key != NULL) {
        /* 其他线程已插入有效项，使用现有缓存项 */
        destroy_dek_cache_item(new_item);
        cached_item->hit_times++;
        CACHE_UNLOCK(manager);
        return cached_item;
    }
    if (cached_item != NULL && cached_item->encrypted_key == NULL) {
        /* 缓存有旧项但encrypted_key为NULL，用新加载的项替换 */
        TECC_CACHE_DEBUG("DEBUG: tecc_cache_get_dek_by_name: 缓存项encrypted_key为空，替换为新加载的完整项");
    }

    /* 8. 更新缓存 */
    if (insert_dek_into_cache(state, new_item) != TECC_CACHE_OK) {
        /* 插入缓存失败，清理资源 */
        CACHE_UNLOCK(manager);
        destroy_dek_cache_item(new_item);
        /* 如果是可恢复错误且未尝试过自动恢复，尝试恢复 */
        if (!auto_recovery_attempted) {
            auto_recovery_attempted = true;
            TECC_CACHE_DEBUG("缓存插入失败，尝试自动恢复");
            if (tecc_cache_auto_recover(state) == TECC_CACHE_OK) {
                goto retry_with_recovery;
            }
        }
        return NULL;
    }

    /* 9. 更新加载次数统计和缓存大小 */
    manager->load_count++;
    manager->current_size++;

    /* 10. 解锁并返回结果 */
    CACHE_UNLOCK(manager);
    return new_item;
}

/*
 * destroy_dek_cache_item - 销毁DEK缓存项
 *
 * 参数:
 *   item - DEK缓存项指针
 */
static void
destroy_dek_cache_item(TeccDekCacheItem *item)
{
    if (item == NULL)
    {
        return;
    }

    /* 安全清理敏感密钥数据 */
    tecc_cache_secure_clear_dek_key(item);

    /* 释放字符串字段（安全清理后释放内存） */
    if (item->key_name != NULL)
    {
        pg_free(item->key_name);
        item->key_name = NULL;
    }
    if (item->key_namespace != NULL)
    {
        pg_free(item->key_namespace);
        item->key_namespace = NULL;
    }

    /* 释放二进制数据字段（安全清理后释放内存） */
    if (item->encrypted_key != NULL)
    {
        pg_free(item->encrypted_key);
        item->encrypted_key = NULL;
    }
    /* 移除了冗余字段的释放：encrypted_value, support_algo, plain_key_value */

    /* ReqID:TECC4.0 — 可靠擦除并释放缓存的解密后DEK */
    if (item->plain_dek != NULL)
    {
        tecc_cache_secure_clear_memory(item->plain_dek, item->plain_dek_length);
        pg_free(item->plain_dek);
        item->plain_dek = NULL;
        item->plain_dek_length = 0;
    }

    /* 安全清理结构体剩余部分并释放内存 */
    tecc_cache_secure_clear_memory(item, sizeof(TeccDekCacheItem));
    pg_free(item);
}

/*
 * find_dek_in_cache_by_oid - 在缓存中按OID查找DEK
 *
 * 参数:
 *   dek_oid - DEK对象标识符
 *
 * 返回:
 *   找到的DEK缓存项指针，NULL表示未找到
 */
static TeccDekCacheItem *
find_dek_in_cache_by_oid(const TeccState *state, Oid dek_oid)
{
    TeccCacheManager *manager = NULL;
    DekHashKey key;
    bool found = false;
    void *entry = NULL;

    /* 获取缓存管理器 */
    manager = get_cache_manager(state);
    if (manager == NULL || !manager->initialized || manager->dek_cache == NULL) {
        TECC_CACHE_DEBUG("缓存管理器未初始化或无效，无法查找DEK（OID: %u）", dek_oid);
        return NULL;
    }

    /* 设置哈希键 */
    key.oid = dek_oid;

    /* 在哈希表中查找 */
    entry = fe_hash_search(manager->dek_cache, &key, HASH_FIND, &found);
    if (!found || entry == NULL) {
        return NULL;
    }

    /* 返回缓存项指针 */
    return *(TeccDekCacheItem **)entry;
}

/*
 * find_dek_in_cache_by_name - 在缓存中按名称查找DEK
 *
 * 参数:
 *   key_name - DEK名称
 *   key_namespace - 命名空间
 *
 * 返回:
 *   找到的DEK缓存项指针，NULL表示未找到
 */
static TeccDekCacheItem *
find_dek_in_cache_by_name(const TeccState *state, const char *key_name, const char *key_namespace)
{
    TeccCacheManager *manager = NULL;
    DekNameHashKey name_key;
    void *entry = NULL;
    TeccDekCacheItem *item = NULL;

    /* 参数检查 */
    if (key_name == NULL || key_namespace == NULL) {
        return NULL;
    }

    /* 获取缓存管理器 */
    manager = get_cache_manager(state);
    if (manager == NULL || !manager->initialized || manager->dek_name_cache == NULL) {
        TECC_CACHE_DEBUG("缓存管理器未初始化或无效，无法查找DEK（名称: %s.%s）", key_namespace, key_name);
        return NULL;
    }

    /* 构建名称键 */
    strncpy(name_key.key_name, key_name, NAMEDATALEN - 1);
    strncpy(name_key.key_namespace, key_namespace, NAMEDATALEN - 1);
    name_key.key_name[NAMEDATALEN - 1] = '\0';
    name_key.key_namespace[NAMEDATALEN - 1] = '\0';

    /* 直接查询名称哈希表 */
    entry = fe_hash_search(manager->dek_name_cache, &name_key, HASH_FIND, NULL);
    if (entry != NULL) {
        item = *(TeccDekCacheItem **)entry;
        return item;
    }

    /* 未找到匹配项 */
    return NULL;
}

/*
 * load_dek_from_database_by_oid - 从数据库按OID加载DEK信息
 *
 * 参数:
 *   dek_oid - DEK对象标识符
 *   item - 返回的DEK缓存项指针
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
static int
load_dek_from_database_by_oid(const TeccState *state, Oid dek_oid, TeccDekCacheItem **item)
{
    /* 通过数据获取器适配器按OID查询DEK元数据 */
    TeccCacheQueryParams query;
    TeccFetcherErrorCode fetcher_error;

    /* 参数检查 */
    if (state == NULL || item == NULL) {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    if (dek_oid == InvalidOid) {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    /* 使用简化的适配器接口查询DEK元数据 */
    memset(&query, 0, sizeof(query));
    query.flags = TECC_QUERY_BY_OID;
    query.params.by_oid.oid = dek_oid;

    fetcher_error = tecc_fetch_dek_for_cache(state, &query, item, NULL);
    if (fetcher_error != TECC_FETCHER_OK) {
        /* 查询失败，转换为缓存错误码 */
        return tecc_cache_convert_error(fetcher_error);
    }

    /* 检查是否找到DEK */
    if (*item == NULL) {
        return TECC_CACHE_ERROR_NOT_FOUND;
    }

    return TECC_CACHE_OK;
}

/*
 * load_dek_from_database_by_name - 从数据库按名称加载DEK信息
 *
 * 参数:
 *   key_name - DEK名称
 *   key_namespace - 命名空间
 *   item - 返回的DEK缓存项指针
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
static int
load_dek_from_database_by_name(const TeccState *state,
                               const char *key_name,
                               const char *key_namespace,
                               TeccDekCacheItem **item)
{
    /* 通过数据获取器适配器查询DEK元数据 */
    TeccCacheQueryParams query;
    TeccFetcherErrorCode fetcher_error;

    /* 参数检查 */
    if (state == NULL || key_name == NULL || item == NULL) {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    /* 使用简化的适配器接口查询DEK元数据 */
    memset(&query, 0, sizeof(query));
    query.flags = TECC_QUERY_BY_NAME;
    query.params.by_name.name = key_name;
    query.params.by_name.namespace = key_namespace;

    fetcher_error = tecc_fetch_dek_for_cache(state, &query, item, NULL);
    if (fetcher_error != TECC_FETCHER_OK) {
        /* 查询失败，转换为缓存错误码 */
        return tecc_cache_convert_error(fetcher_error);
    }

    /* 检查是否找到DEK */
    if (*item == NULL) {
        return TECC_CACHE_ERROR_NOT_FOUND;
    }

    return TECC_CACHE_OK;
}


/*
 * insert_dek_into_cache - 将DEK缓存项插入到缓存中
 *
 * 参数:
 *   item - DEK缓存项指针
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
static int
insert_dek_into_cache(const TeccState *state, TeccDekCacheItem *item)
{
    TeccCacheManager *manager = NULL;
    DekHashKey oid_key;
    DekNameHashKey name_key;
    void *entry = NULL;
    bool found = false;

    /* 参数检查 */
    if (item == NULL) {
        TECC_CACHE_DEBUG("DEBUG: insert_dek_into_cache: 无效参数，item为NULL");
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }
    if (item->dek_oid == InvalidOid && item->cache_status == TECC_CACHE_STATUS_ENCRYPTED) {
        TECC_CACHE_DEBUG("DEBUG: insert_dek_into_cache: 无效参数，非负缓存项缺少OID");
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    /* 获取缓存管理器 */
    manager = get_cache_manager(state);
    if (manager == NULL || !manager->initialized) {
        TECC_CACHE_DEBUG("DEBUG: insert_dek_into_cache: 缓存管理器未初始化或无效");
        return TECC_CACHE_ERROR_NOT_INITIALIZED;
    }

    /* 1. 插入到OID哈希表（负缓存项无有效OID则跳过） */
    if (item->dek_oid != InvalidOid && manager->dek_cache != NULL) {
        oid_key.oid = item->dek_oid;
        entry = fe_hash_search(manager->dek_cache, &oid_key, HASH_ENTER, &found);
        if (entry == NULL) {
            return TECC_CACHE_ERROR_MEMORY;
        }
        *(TeccDekCacheItem **)entry = item;
    }

    /* 2. 插入到名称哈希表（如果名称可用） */
    if (manager->dek_name_cache != NULL && item->key_name != NULL && item->key_namespace != NULL) {
        /* 构建名称键 */
        strncpy(name_key.key_name, item->key_name, NAMEDATALEN - 1);
        strncpy(name_key.key_namespace, item->key_namespace, NAMEDATALEN - 1);
        name_key.key_name[NAMEDATALEN - 1] = '\0';
        name_key.key_namespace[NAMEDATALEN - 1] = '\0';

        entry = fe_hash_search(manager->dek_name_cache, &name_key, HASH_ENTER, &found);
        if (entry != NULL) {
            /* 指向同一个缓存项 */
            *(TeccDekCacheItem **)entry = item;
        }
        /* 注意：名称哈希表插入失败不影响整体功能，可以继续 */
    }

    /* 3. 更新缓存统计信息 */
    if (!found) {
        manager->current_size++;
    }

    TECC_CACHE_DEBUG("DEBUG: insert_dek_into_cache: 成功插入DEK到缓存，OID=%u, name=%s.%s, found=%d, current_size=%d",
                    item->dek_oid,
                    item->key_namespace ? item->key_namespace : "NULL",
                    item->key_name ? item->key_name : "NULL",
                    found, manager->current_size);

    return TECC_CACHE_OK;
}

/*
 * tecc_cache_get_dek_name_by_distributed_id - 通过分布式ID查找DEK名称
 *
 * 遍历DEK名称缓存，查找column_key_distributed_id匹配的DEK，
 * 返回其key_name。用于表达式结果解密场景（col_meta为NULL时）。
 *
 * 参数:
 *   state - TECC状态结构
 *   dist_id - DEK分布式ID（即CipherDataType.dek_dist_oid）
 *
 * 返回:
 *   DEK名称指针（缓存拥有），NULL表示未找到（用户无权限或DEK不存在）
 */
const char *
tecc_cache_get_dek_name_by_distributed_id(const TeccState *state, Oid dist_id)
{
    TeccCacheManager *manager;
    FeHashSeqStatus status;
    void *entry;

    if (state == NULL || dist_id == InvalidOid)
        return NULL;

    manager = get_cache_manager(state);
    if (manager == NULL || manager->dek_name_cache == NULL)
        return NULL;

    CACHE_LOCK(manager);

    /* ReqID:TECC4.0 - Scan both name cache and OID cache.
     * DEK may be in either depending on whether key_namespace was populated. */
    if (manager->dek_name_cache != NULL)
    {
        fe_hash_seq_init(&status, manager->dek_name_cache);
        while ((entry = fe_hash_seq_search(&status)) != NULL)
        {
            TeccDekCacheItem *item = *(TeccDekCacheItem **)entry;
            if (item != NULL && item->column_key_distributed_id == dist_id
                && item->key_name != NULL)
            {
                const char *name = item->key_name;
                CACHE_UNLOCK(manager);
                return name;
            }
        }
    }

    if (manager->dek_cache != NULL)
    {
        fe_hash_seq_init(&status, manager->dek_cache);
        while ((entry = fe_hash_seq_search(&status)) != NULL)
        {
            TeccDekCacheItem *item = *(TeccDekCacheItem **)entry;
            if (item != NULL && item->column_key_distributed_id == dist_id
                && item->key_name != NULL)
            {
                const char *name = item->key_name;
                CACHE_UNLOCK(manager);
                return name;
            }
        }
    }

    CACHE_UNLOCK(manager);

    /* ReqID:TECC4.0 - Not in cache, try to load from database. */
    if (state != NULL)
    {
        PGconn *conn;
        char query[256];
        PGresult *pg_result;

        /* Use pool connection to avoid nested PQexec on main connection */
        conn = state->conn;
        if (conn == NULL)
            return NULL;

        snprintf(query, sizeof(query),
                 "SELECT dek_name FROM hg_dek_info "
                 "WHERE column_key_distributed_id = %u LIMIT 1",
                 dist_id);
        pg_result = PQexec(conn, query);
        if (pg_result != NULL && PQresultStatus(pg_result) == PGRES_TUPLES_OK
            && PQntuples(pg_result) > 0)
        {
            const char *raw_name = PQgetvalue(pg_result, 0, 0);
            if (raw_name != NULL && raw_name[0] != '\0')
            {
                /* ReqID:TECC4.0 - Copy name before PQclear invalidates result */
                char *name_copy = pg_strdup(raw_name);
                PQclear(pg_result);
                if (name_copy != NULL)
                {
                    tecc_cache_get_dek_by_name(state, name_copy, "public");
                    return name_copy;
                }
                return NULL;
            }
        }
        if (pg_result != NULL)
            PQclear(pg_result);
    }

    return NULL;
}


/*
 * tecc_cache_get_dek_by_name_search_path - 按名称查找DEK（search_path包装）
 *
 * 按current_schema → search_path_schemas[]顺序遍历查找DEK。
 */
TeccDekCacheItem *
tecc_cache_get_dek_by_name_search_path(const TeccState *state, const char *key_name)
{
	const char *current_schema;
	const char **schemas;
	int nschemas, i;
	TeccDekCacheItem *item;

	/* 1. 先查current_schema */
	current_schema = tecc_cache_get_guc_param(state, "current_schema");
	if (current_schema != NULL)
	{
		item = tecc_cache_get_dek_by_name(state, key_name, current_schema);
		if (item != NULL)
			return item;
	}

	/* 2. 遍历search_path_schemas */
	schemas = tecc_cache_get_search_path_schemas(state, &nschemas);
	for (i = 0; i < nschemas; i++)
	{
		if (schemas[i] == NULL)
			continue;
		item = tecc_cache_get_dek_by_name(state, key_name, schemas[i]);
		if (item != NULL)
			return item;
	}

	TECC_LOG(TECC_LOG_WARNING, "DEK search_path: key '%s' not found in current_schema/search_path (GUC may not be loaded)",
			 key_name);
	return NULL;
}

/*
 * tecc_cache_insert_dek_item - 公开的DEK缓存项插入接口
 */
int
tecc_cache_insert_dek_item(TeccState *state, TeccDekCacheItem *item)
{
    const TeccState *cstate = (const TeccState *)state;
    return insert_dek_into_cache(cstate, item);
}

/*
 * tecc_cache_remove_pending_dek - 移除PENDING DEK缓存项
 */
void
tecc_cache_remove_pending_dek(TeccState *state, const char *key_name, const char *key_namespace)
{
    TeccCacheManager *manager;
    DekNameHashKey name_key;
    void *entry;

    if (state == NULL || key_name == NULL || key_namespace == NULL)
        return;

    manager = get_cache_manager((const TeccState *)state);
    if (manager == NULL || manager->dek_name_cache == NULL)
        return;

    memset(&name_key, 0, sizeof(name_key));
    strncpy(name_key.key_name, key_name, NAMEDATALEN - 1);
    strncpy(name_key.key_namespace, key_namespace, NAMEDATALEN - 1);

    CACHE_LOCK(manager);
    entry = fe_hash_search(manager->dek_name_cache, &name_key, HASH_REMOVE, NULL);
    if (entry != NULL)
    {
        TeccDekCacheItem *item = *(TeccDekCacheItem **)entry;
        if (item != NULL)
            destroy_dek_cache_item(item);
        pg_free(entry);
    }
    CACHE_UNLOCK(manager);
}

/* End - ReqID:TECC4.0 */