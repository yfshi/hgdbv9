/*-------------------------------------------------------------------------
 *
 * key_manager.c
 *    Core key manager implementation for TECC key management module
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/common/tecc_key/key_manager.c
 *
 * ReqID:TECC4.0 - Key management core manager
 * Authored by <wanghaoyan@highgo.com>, 20260408.
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *-------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <time.h>

#include "key_manager.h"
#include "server_interface.h"
#include "kms_adapter.h"
#include "utils.h"
#include "config.h"
#include "high_level_api.h"
#include "../../interfaces/libpq/tecc_cache/tecc_metadata_cache.h"

/* Forward declaration for TeccState structure */
struct TeccState;

/* Forward declarations for functions defined in this file */
bool key_manager_check_dek_permission(KeyManager *manager, const char *dek_name, Oid user_oid);
bool key_manager_check_kek_permission(KeyManager *manager, const char *kek_name, Oid user_oid);

/* Note: Cache configuration is handled by tecc_metadata_cache module */

/* Thread-local error buffer */
static pthread_key_t key_manager_error_key;
static pthread_once_t key_manager_error_key_once = PTHREAD_ONCE_INIT;

/*
 * create_key_manager_error_key
 *    Create thread-specific error key for key manager
 */
static void
create_key_manager_error_key(void)
{
    pthread_key_create(&key_manager_error_key, free);
}

/*
 * get_key_manager_error_buffer
 *    Get thread-specific error buffer for key manager
 */
static char *
get_key_manager_error_buffer(void)
{
    char *buffer;

    pthread_once(&key_manager_error_key_once, create_key_manager_error_key);

    buffer = pthread_getspecific(key_manager_error_key);
    if (buffer == NULL)
    {
        buffer = malloc(512);
        if (buffer != NULL)
        {
            memset(buffer, 0, 512);
            pthread_setspecific(key_manager_error_key, buffer);
        }
    }

    return buffer;
}

/*
 * set_key_manager_error
 *    Set key manager error message
 */
static void
set_key_manager_error(const char *format, ...)
{
    va_list args;
    char *buffer;

    buffer = get_key_manager_error_buffer();
    if (buffer == NULL)
        return;

    va_start(args, format);
    vsnprintf(buffer, 512, format, args);
    va_end(args);
}


/*
 * key_manager_create
 *    Create a new key manager instance
 */
KeyManager *
key_manager_create(void)
{
    KeyManager *manager = NULL;
    int ret;

    /* Allocate memory for key manager */
    manager = (KeyManager *)pg_malloc0(sizeof(KeyManager));
    if (manager == NULL)
    {
        set_key_manager_error("Failed to allocate memory for key manager");
        return NULL;
    }

    /* Initialize default configuration */
    manager->connection_valid = false;

    /* Initialize synchronization primitives */
    ret = pthread_mutex_init(&manager->lock, NULL);
    if (ret != 0)
    {
        set_key_manager_error("Failed to initialize mutex: %d", ret);
        free(manager);
        return NULL;
    }

    /* Note: Cache is handled by tecc_metadata_cache module */

    return manager;
}

/*
 * key_manager_create_with_state
 *    Create a new key manager instance with TeccState for cache access
 */
KeyManager *
key_manager_create_with_state(void *tecc_state)
{
    KeyManager *manager = NULL;
    int ret;

    /* Allocate memory for key manager */
    manager = (KeyManager *)pg_malloc0(sizeof(KeyManager));
    if (manager == NULL)
    {
        set_key_manager_error("Failed to allocate memory for key manager");
        return NULL;
    }

    /* Initialize default configuration */
    manager->connection_valid = false;
    manager->tecc_state = tecc_state;  /* Store TeccState for cache access */

    /* Initialize synchronization primitives */
    ret = pthread_mutex_init(&manager->lock, NULL);
    if (ret != 0)
    {
        set_key_manager_error("Failed to initialize mutex: %d", ret);
        free(manager);
        return NULL;
    }

    /* Note: Cache is handled by tecc_metadata_cache module */

    return manager;
}

/*
 * key_manager_destroy
 *    Destroy a key manager instance
 */
void
key_manager_destroy(KeyManager *manager)
{
    if (manager == NULL)
        return;

    /* Note: Cache cleanup is handled by tecc_metadata_cache module */

    /* Close database connection if open */
    if (manager->db_connection != NULL)
    {
        /* Use server_interface to destroy connection pool */
        connection_pool_destroy((ConnectionPool *)manager->db_connection);
        manager->db_connection = NULL;
    }

    /* Destroy synchronization primitives */
    pthread_mutex_destroy(&manager->lock);

    /* Free manager memory */
    free(manager);
}

/*
 * key_manager_init
 *    Initialize key manager (establish connections, load configuration)
 */
int
key_manager_init(KeyManager *manager)
{
    if (manager == NULL)
    {
        set_key_manager_error("Invalid key manager");
        return -1;
    }

    /* Load default configuration */
    /* Note: Configuration file parsing is not yet implemented in config.c */
    /* For now, we'll use default values and establish connection when needed */

    tecc_key_log(LOG_LEVEL_INFO, __func__, __FILE__, __LINE__,
                "Key manager initialized with default configuration");

    return 0;
}

/*
 * key_manager_cleanup
 *    Cleanup key manager resources
 */
void
key_manager_cleanup(KeyManager *manager)
{
    if (manager == NULL)
        return;

    /* Note: Cache cleanup is handled by tecc_metadata_cache module */

    /* Close database connection */
    key_manager_close_connection(manager);
}

/*
 * key_manager_get_kek_info
 *    Get KEK information from database
 */
/* Helper function to convert TeccKekCacheItem to PgTeccKekInfo */
static PgTeccKekInfo *
convert_kek_cache_to_info(TeccKekCacheItem *kek_item)
{
    PgTeccKekInfo *kek_info = NULL;

    if (kek_item == NULL)
        return NULL;

    kek_info = (PgTeccKekInfo *)pg_malloc0(sizeof(PgTeccKekInfo));
    if (kek_info == NULL)
        return NULL;

    /* Copy basic information */
    kek_info->kek_name = pg_strdup(kek_item->key_name);
    kek_info->kek_oid = kek_item->kek_oid;

    /* Convert KMS type */
    switch (kek_item->kms_type)
    {
        case 1:  /* localkms */
            kek_info->kms_type = KMS_TYPE_LOCAL;
            break;
        case 2:  /* hsmkms */
            kek_info->kms_type = KMS_TYPE_HSM;
            break;
        /* case 3: cloudkms - deprecated, removed */
        case 4:  /* swxakms */
            kek_info->kms_type = KMS_TYPE_HSM_SWXA;
            break;
        default:
            kek_info->kms_type = KMS_TYPE_LOCAL;
            break;
    }

    /* Copy KMS path */
    if (kek_item->kms_path != NULL)
        kek_info->kms_path = pg_strdup(kek_item->kms_path);

    /* Convert algorithm */
    if (kek_item->algorithm == 1)  /* AES_256_CBC */
        kek_info->algorithm = pg_strdup("AES_256_CBC");
    else if (kek_item->algorithm == 2)  /* SM4_CBC */
        kek_info->algorithm = pg_strdup("SM4_CBC");
    else
        kek_info->algorithm = pg_strdup("AES_256_CBC");

    /* Convert status */
    if (kek_item->status == 1)  /* active */
        kek_info->status = pg_strdup("active");
    else if (kek_item->status == 2)  /* inactive */
        kek_info->status = pg_strdup("inactive");
    else if (kek_item->status == 3)  /* revoked */
        kek_info->status = pg_strdup("revoked");
    else
        kek_info->status = pg_strdup("active");

    /* Key length */
    kek_info->key_len = kek_item->key_length / 8;  /* Convert bits to bytes */

    /* Note: key_data is not available from cache - will be loaded when needed */
    kek_info->key_data = NULL;

    return kek_info;
}

PgTeccKekInfo *
key_manager_get_kek_info(KeyManager *manager, const char *kek_name)
{
    PgTeccKekInfo *kek_info = NULL;
    TeccKekCacheItem *kek_item = NULL;
    /* Parsed KEK name and namespace (supports "schema.name" format) */
    /* For unqualified names (no dot), namespace is NULL to search all schemas */
    const char *lookup_name = kek_name;
    const char *lookup_namespace = NULL;
    char name_buf[NAMEDATALEN];
    char nsp_buf[NAMEDATALEN];

    if (manager == NULL || kek_name == NULL)
    {
        set_key_manager_error("Invalid parameters");
        return NULL;
    }

    /* Parse dotted KEK name: "schema.kek_name" -> namespace="schema", name="kek_name" */
    {
        const char *dot = strchr(kek_name, '.');
        if (dot != NULL)
        {
            size_t schema_len = dot - kek_name;
            if (schema_len > 0 && schema_len < NAMEDATALEN)
            {
                memcpy(nsp_buf, kek_name, schema_len);
                nsp_buf[schema_len] = '\0';
                lookup_namespace = nsp_buf;
            }
            /* Skip the dot */
            if (*(dot + 1) != '\0')
            {
                strncpy(name_buf, dot + 1, NAMEDATALEN - 1);
                name_buf[NAMEDATALEN - 1] = '\0';
                lookup_name = name_buf;
            }
        }
    }

    /* First try to get KEK from cache if TeccState is available */
    if (manager->tecc_state != NULL)
    {
        tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                   "TeccState available for cache access: %p", manager->tecc_state);
        /* 限定名使用指定namespace直接查找，非限定名使用search_path */
        if (lookup_namespace != NULL)
        {
            kek_item = tecc_cache_get_kek_by_name(
                (TeccState *)manager->tecc_state, lookup_name, lookup_namespace);
        }
        else
        {
            kek_item = tecc_cache_get_kek_by_name_search_path(
                (TeccState *)manager->tecc_state, lookup_name);
        }
        if (kek_item != NULL)
        {
            kek_info = convert_kek_cache_to_info(kek_item);
            if (kek_info != NULL)
            {
                tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                           "KEK '%s' retrieved from cache via search_path", kek_name);
                return kek_info;
            }
            else
            {
                tecc_key_log(LOG_LEVEL_ERROR, __func__, __FILE__, __LINE__,
                           "Failed to convert KEK cache item to info for '%s'", kek_name);
            }
        }
        else
        {
            tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                       "KEK '%s' not found in cache (search_path)", kek_name);
        }
    }
    else
    {
        tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                   "TeccState is NULL, cannot use cache");
    }

    /* Cache miss or TeccState not available - cannot query database directly */
    tecc_key_log(LOG_LEVEL_ERROR, __func__, __FILE__, __LINE__,
                "KEK '%s' not found in cache and direct database query is not allowed", kek_name);
    set_key_manager_error("KEK '%s' not found in cache. Direct database queries from key manager are disabled.", kek_name);
    return NULL;
}

/*
 * key_manager_free_kek_info
 *    Free KEK information structure
 */
void
key_manager_free_kek_info(PgTeccKekInfo *kek_info)
{
    if (kek_info == NULL)
        return;

    /* Free all dynamically allocated fields */
    if (kek_info->kek_name != NULL)
        free(kek_info->kek_name);
    if (kek_info->kms_path != NULL)
        free(kek_info->kms_path);
    if (kek_info->kms_args != NULL)
        free(kek_info->kms_args);
    if (kek_info->key_data != NULL)
    {
        /* Securely erase key data */
        memset(kek_info->key_data, 0, kek_info->key_len);
        free(kek_info->key_data);
    }
    if (kek_info->algorithm != NULL)
        free(kek_info->algorithm);
    if (kek_info->status != NULL)
        free(kek_info->status);

    /* Free the structure itself */
    free(kek_info);
}

/*
 * key_manager_get_dek_info
 *    Get DEK information (check cache first, then database)
 */
PgTeccDekInfo *
key_manager_get_dek_info(KeyManager *manager, const char *dek_name)
{
    PgTeccDekInfo *dek_info = NULL;
    int ret;
    TeccDekCacheItem *dek_item = NULL;
    const char *schema_name = NULL;
    const struct TeccState *state = NULL;

    if (manager == NULL || dek_name == NULL)
    {
        set_key_manager_error("Invalid parameters");
        return NULL;
    }

    /* Get TeccState from manager */
    state = (const struct TeccState *)manager->tecc_state;
    if (state == NULL)
    {
        set_key_manager_error("TeccState not available in key manager");
        /* Fall back to database loading */
        goto load_from_database;
    }

    /* Note: We cannot access state->conn directly in common directory */
    /* Cache lookup will use default values */
    schema_name = "pg_catalog";  /* KEK/DEK are created in pg_catalog namespace */

    /* Get DEK from cache using new interface */
    dek_item = tecc_cache_get_dek_by_name(state, dek_name, schema_name);
    if (dek_item == NULL)
    {
        /* Cache miss - fall back to database loading */
        goto load_from_database;
    }

    /* Cache hit - convert TeccDekCacheItem to PgTeccDekInfo */
    dek_info = (PgTeccDekInfo *)pg_malloc0(sizeof(PgTeccDekInfo));
    if (dek_info == NULL)
    {
        set_key_manager_error("Failed to allocate memory for DEK info");
        tecc_cache_free_dek_item(dek_item);
        return NULL;
    }

    /* Copy basic information */
    dek_info->dek_name = strdup(dek_name);
    dek_info->dek_oid = dek_item->dek_oid;

    /* Convert status from integer to string */
    switch (dek_item->status)
    {
        case 1: /* active */
            dek_info->status = strdup("active");
            break;
        case 2: /* inactive */
            dek_info->status = strdup("inactive");
            break;
        case 3: /* revoked */
            dek_info->status = strdup("revoked");
            break;
        default:
            dek_info->status = strdup("unknown");
            break;
    }

    /* Copy encrypted DEK from cache */
    if (dek_item->encrypted_key != NULL && dek_item->key_length > 0)
    {
        dek_info->encrypted_key = pg_malloc(dek_item->key_length);
        if (dek_info->encrypted_key == NULL)
        {
            set_key_manager_error("Failed to allocate memory for encrypted DEK");
            tecc_cache_free_dek_item(dek_item);
            key_manager_free_dek_info(dek_info);
            return NULL;
        }
        dek_info->encrypted_len = dek_item->key_length;
        memcpy(dek_info->encrypted_key, dek_item->encrypted_key, dek_item->key_length);
    }

    /* DEK不应该记录算法类型，算法类型在加密列中指定（如'det,ope'等）
     * 因此将算法支持字段设置为默认值（全部支持），不限制算法类型
     */
    dek_info->supports_det = true;   /* DET算法默认支持 */
    dek_info->supports_ope = true;   /* OPE算法默认支持 */
    dek_info->supports_search = true; /* SEARCH算法默认支持 */

    /* DEK不记录算法类型，设置为默认的RND算法类型 */
    dek_info->algo_type = TECC_ALGO_RND;  /* Default to RND algorithm */

    /* Set creation time */
    dek_info->created_time = dek_item->create_date;

    /* Note: Cache item is owned by the cache, do not free it here.
     * tecc_cache_free_dek_item would destroy the cache entry, leading
     * to use-after-free when the same DEK is accessed again. */

    /* Note: KEK name will be filled in by key_manager_load_dek if needed */
    return dek_info;

load_from_database:
    /* Cache miss or incomplete cache - load/complete from database */
    ret = key_manager_load_dek(manager, dek_name, &dek_info);
    if (ret != 0)
    {
        set_key_manager_error("Failed to load DEK '%s' from database", dek_name);
        if (dek_info != NULL)
            key_manager_free_dek_info(dek_info);
        return NULL;
    }

    return dek_info;
}

/*
 * key_manager_free_dek_info
 *    Free DEK information structure
 */
void
key_manager_free_dek_info(PgTeccDekInfo *dek_info)
{
    if (dek_info == NULL)
        return;

    /* Free all dynamically allocated fields */
    if (dek_info->dek_name != NULL)
        free(dek_info->dek_name);
    if (dek_info->kek_name != NULL)
        free(dek_info->kek_name);
    if (dek_info->encrypted_key != NULL)
        free(dek_info->encrypted_key);
    if (dek_info->decrypted_key != NULL)
    {
        /* Securely erase decrypted key */
        memset(dek_info->decrypted_key, 0, dek_info->decrypted_len);
        free(dek_info->decrypted_key);
    }
    if (dek_info->status != NULL)
        free(dek_info->status);

    /* Free the structure itself */
    free(dek_info);
}

/*
 * key_manager_load_dek
 *    Load DEK from database or complete DEK info from cache
 */
int
key_manager_load_dek(KeyManager *manager, const char *dek_name, PgTeccDekInfo **dek_info)
{
    DbConnection *db_conn = NULL;
    PgTeccDekInfo *loaded_dek = *dek_info;  /* May be partially filled from cache */
    unsigned char *kek = NULL;
    size_t kek_len = 0;
    int ret = 0;
    char *kek_name = NULL;

    if (manager == NULL || dek_name == NULL || dek_info == NULL)
    {
        set_key_manager_error("Invalid parameters for DEK loading");
        return -1;
    }

    /* If DEK info is already provided (from cache), we may need to complete it */
    if (loaded_dek == NULL)
    {
        /* Need to load DEK from database */
        ret = key_manager_ensure_connection(manager);
        if (ret != 0)
        {
            set_key_manager_error("Failed to establish database connection");
            return -1;
        }

        /* Get database connection */
        db_conn = (DbConnection *)manager->db_connection;
        if (db_conn == NULL || !db_conn->is_connected)
        {
            set_key_manager_error("Database connection not available");
            return -1;
        }

        /* Query DEK from database */
        ret = server_interface_query_dek(db_conn, dek_name, &loaded_dek);
        if (ret != 0 || loaded_dek == NULL)
        {
            set_key_manager_error("Failed to query DEK '%s' from database", dek_name);
            return -1;
        }
    }

    /* If DEK from cache doesn't have kek_name, we need to get it */
    if (loaded_dek->kek_name == NULL)
    {
        /* Need to query KEK name from database */
        if (db_conn == NULL)
        {
            ret = key_manager_ensure_connection(manager);
            if (ret != 0)
            {
                set_key_manager_error("Failed to establish database connection");
                key_manager_free_dek_info(loaded_dek);
                return -1;
            }
            db_conn = (DbConnection *)manager->db_connection;
        }

        /* Query KEK name for this DEK from database */
        kek_name = NULL;
        ret = server_interface_query_kek_for_dek(db_conn, dek_name, &kek_name);
        if (ret != 0 || kek_name == NULL)
        {
            set_key_manager_error("Failed to query KEK name for DEK '%s'", dek_name);
            key_manager_free_dek_info(loaded_dek);
            return -1;
        }
        loaded_dek->kek_name = kek_name;
    }

    /* Get KEK for decryption */
    kek = tecc_key_get_kek_with_state(manager->tecc_state, loaded_dek->kek_name, &kek_len);
    if (kek == NULL)
    {
        set_key_manager_error("Failed to get KEK for DEK '%s'", dek_name);
        key_manager_free_dek_info(loaded_dek);
        return -1;
    }

    /* Decrypt DEK if encrypted key exists and not already decrypted */
    if (loaded_dek->encrypted_key != NULL && loaded_dek->encrypted_len > 0 &&
        loaded_dek->decrypted_key == NULL)
    {
        /* encrypted_key is now binary data (converted from hex string) */
        {
            unsigned char *temp_decrypted_key = NULL;
            ret = tecc_key_decrypt_dek(loaded_dek->encrypted_key, loaded_dek->encrypted_len,
                                      kek, kek_len,
                                      &temp_decrypted_key, &loaded_dek->decrypted_len);
            loaded_dek->decrypted_key = temp_decrypted_key;
        }
        if (ret != 0)
        {
            set_key_manager_error("Failed to decrypt DEK '%s'", dek_name);
            tecc_key_secure_erase(kek, kek_len);
            free(kek);
            key_manager_free_dek_info(loaded_dek);
            return -1;
        }
    }

    /* Secure cleanup of KEK */
    tecc_key_secure_erase(kek, kek_len);
    free(kek);

    *dek_info = loaded_dek;
    return 0;
}

/*
 * Note: Cache functionality is now provided by tecc_metadata_cache module
 * Cache statistics should be obtained from tecc_metadata_cache_get_stats()
 */

/*
 * key_manager_check_dek_permission
 *    Check if user has permission to access DEK
 *    Note: Permission caching is now handled by tecc_metadata_cache module
 */
bool
key_manager_check_dek_permission(KeyManager *manager, const char *dek_name, Oid user_oid)
{
    PgTeccDekInfo *dek_info = NULL;
    bool has_permission = false;
    int ret;
    DbConnection *db_conn;

    if (manager == NULL || dek_name == NULL || user_oid == InvalidOid)
    {
        set_key_manager_error("Invalid parameters for permission check");
        return false;
    }

    /* Ensure database connection */
    ret = key_manager_ensure_connection(manager);
    if (ret != 0)
    {
        set_key_manager_error("Failed to establish database connection");
        return false;
    }

    /* Get DEK info to get OID */
    dek_info = key_manager_get_dek_info(manager, dek_name);
    if (dek_info == NULL)
    {
        set_key_manager_error("DEK '%s' not found", dek_name);
        return false;
    }

    /* Check permission on server */
    db_conn = (DbConnection *)manager->db_connection;
    has_permission = server_interface_check_dek_permission(db_conn, dek_info->dek_oid, user_oid);

    key_manager_free_dek_info(dek_info);
    return has_permission;
}

/*
 * key_manager_check_kek_permission
 *    Check if user has permission to access KEK
 */
bool
key_manager_check_kek_permission(KeyManager *manager, const char *kek_name, Oid user_oid)
{
    PgTeccKekInfo *kek_info = NULL;
    bool has_permission = false;
    int ret;
    DbConnection *db_conn;
    TeccKekCacheItem *kek_item = NULL;

    if (manager == NULL || kek_name == NULL || user_oid == InvalidOid)
    {
        set_key_manager_error("Invalid parameters for KEK permission check");
        return false;
    }

    /* First try to get KEK from cache if TeccState is available */
    if (manager->tecc_state != NULL)
    {
        tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                   "TeccState available for cache access: %p", manager->tecc_state);
        kek_item = tecc_cache_get_kek_by_name(
            (TeccState *)manager->tecc_state, kek_name, "pg_catalog");
        if (kek_item != NULL)
        {
            kek_info = convert_kek_cache_to_info(kek_item);
            if (kek_info != NULL)
            {
                tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                           "KEK '%s' retrieved from cache for permission check", kek_name);

                /* Check permission on server - still need database connection for this */
                ret = key_manager_ensure_connection(manager);
                if (ret != 0)
                {
                    set_key_manager_error("Failed to establish database connection for permission check");
                    key_manager_free_kek_info(kek_info);
                    return false;
                }

                db_conn = (DbConnection *)manager->db_connection;
                has_permission = server_interface_check_kek_permission(db_conn, kek_info->kek_oid, user_oid);
                key_manager_free_kek_info(kek_info);
                return has_permission;
            }
        }
        else
        {
            tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                       "KEK '%s' not found in cache", kek_name);
        }
    }
    else
    {
        tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                   "TeccState is NULL, cannot use cache");
    }

    /* Cache miss or TeccState not available - cannot query database directly */
    tecc_key_log(LOG_LEVEL_ERROR, __func__, __FILE__, __LINE__,
                "KEK '%s' not found in cache and direct database query is not allowed", kek_name);
    set_key_manager_error("KEK '%s' not found in cache. Direct database queries from key manager are disabled.", kek_name);
    return false;
}

/*
 * key_manager_ensure_connection
 *    Ensure database connection is established
 */
int
key_manager_ensure_connection(KeyManager *manager)
{
    char *conn_str = NULL;
    ConnectionPool *pool = NULL;
    DbConnection *conn = NULL;

    if (manager == NULL)
        return -1;

    if (manager->connection_valid && manager->db_connection != NULL)
        return 0;

    /* Create connection pool if not exists */
    if (manager->db_connection == NULL)
    {
        /* Get connection string from configuration */
        conn_str = client_config_get_connection_string(NULL);
        if (conn_str == NULL)
        {
            /* Fall back to default connection string */
            conn_str = strdup("host=localhost port=5432 dbname=postgres user=postgres");
        }

        pool = connection_pool_create(1, 5, 30, 300, conn_str);
        if (pool == NULL)
        {
            set_key_manager_error("Failed to create connection pool");
            if (conn_str != NULL)
                free(conn_str);
            return -1;
        }

        if (conn_str != NULL)
            free(conn_str);

        manager->db_connection = pool;
    }

    /* Get connection from pool */
    conn = connection_pool_get_connection((ConnectionPool *)manager->db_connection);
    if (conn == NULL || !conn->is_connected)
    {
        set_key_manager_error("Failed to get database connection");
        return -1;
    }

    manager->connection_valid = true;
    return 0;
}

/*
 * key_manager_close_connection
 *    Close database connection
 */
void
key_manager_close_connection(KeyManager *manager)
{
    if (manager == NULL)
        return;

    /* If we have a connection pool, close idle connections */
    if (manager->db_connection != NULL)
    {
        connection_pool_close_idle_connections((ConnectionPool *)manager->db_connection);
    }

    manager->connection_valid = false;
}

/*
 * Note: Cache functionality is now provided by tecc_metadata_cache module
 * Cache configuration should be handled by tecc_metadata_cache module
 */