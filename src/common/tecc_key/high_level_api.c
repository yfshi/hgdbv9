/*-------------------------------------------------------------------------
 *
 * high_level_api.c
 *    High-level API implementation for TECC key management module
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/common/tecc_key/high_level_api.c
 *
 * ReqID:TECC4.0 - Key management high-level API
 * Authored by <wanghaoyan@highgo.com>, 20260408.
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *-------------------------------------------------------------------------
 */
#include "postgres_fe.h"
#include "c.h"
#include <pthread.h>
#include <sys/stat.h>
#include "high_level_api.h"
#include "key_manager.h"
#include "security.h"
#include "utils.h"
#include "kms_adapter.h"  /* For KMS adapter functions */
#include "common/tecc_key/tecc_sm2.h"  /* For SM2 envelope decrypt (auto DEK format detection) */
#include "tecc_mem.h"  /* Use our memory wrapper */
#include "../../interfaces/libpq/tecc_hsm_adapter/fe-hsm-adapter.h"  /* For HSM adapter */
#include "../../interfaces/libpq/tecc_cache/tecc_metadata_cache.h"  /* For cache access */

/* Forward declaration for TeccState structure */
struct TeccState;

/* Module static variables */
/* Thread-local error storage - using pthread for portability */
static pthread_key_t last_error_key;
static pthread_once_t last_error_key_once = PTHREAD_ONCE_INIT;


/*
 * create_last_error_key
 *    Create thread-specific error key
 */
static void
create_last_error_key(void)
{
    pthread_key_create(&last_error_key, free);
}

/*
 * get_last_error_buffer
 *    Get thread-specific error buffer
 */
static char *
get_last_error_buffer(void)
{
    char *buffer;

    pthread_once(&last_error_key_once, create_last_error_key);

    buffer = pthread_getspecific(last_error_key);
    if (buffer == NULL)
    {
        buffer = malloc(512);
        if (buffer != NULL)
        {
            memset(buffer, 0, 512);
            pthread_setspecific(last_error_key, buffer);
        }
    }

    return buffer;
}

/*
 * set_last_error
 *    Set the last error message
 */
static void
set_last_error(const char *format, ...)
{
    va_list args;
    char *buffer;

    buffer = get_last_error_buffer();
    if (buffer == NULL)
        return;

    va_start(args, format);
    vsnprintf(buffer, 512, format, args);
    va_end(args);
}

/*
 * tecc_key_check_api_version
 *    Check if the API version meets requirements
 */
int
tecc_key_check_api_version(int required_major, int required_minor)
{
    if (required_major > TECC_KEY_API_VERSION_MAJOR)
        return -1;

    if (required_major == TECC_KEY_API_VERSION_MAJOR &&
        required_minor > TECC_KEY_API_VERSION_MINOR)
        return -1;

    return 0;
}

/*
 * tecc_key_get_api_info
 *    Get API information
 */
TeccKeyApiInfo *
tecc_key_get_api_info(void)
{
    TeccKeyApiInfo *info = (TeccKeyApiInfo *)tecc_mem_alloc(sizeof(TeccKeyApiInfo));

    info->api_version_major = TECC_KEY_API_VERSION_MAJOR;
    info->api_version_minor = TECC_KEY_API_VERSION_MINOR;
    info->api_version_patch = TECC_KEY_API_VERSION_PATCH;
    info->module_name = tecc_mem_strdup("TECC Key Management Module");
    info->build_date = tecc_mem_strdup(__DATE__ " " __TIME__);
    info->git_hash = tecc_mem_strdup("unknown");  /* Would be set by build system */
    info->features = 0x01;  /* Basic key management features */

    return info;
}

/*
 * tecc_key_get_dek - REMOVED
 *    Use tecc_cache_get_dek_by_name instead
 */

/*
 * tecc_key_get_kek - REMOVED
 *    Use tecc_key_get_kek_with_state instead for thread-safe cache access
 */

/*
 * tecc_key_decrypt_dek
 *    Decrypt DEK using KEK
 */
int
tecc_key_decrypt_dek(const unsigned char *encrypted_dek,
                    size_t encrypted_len,
                    const unsigned char *kek,
                    size_t kek_len,
                    unsigned char **decrypted_dek,
                    size_t *decrypted_len)
{
    int ret = 0;
    unsigned int key_index = 0;
    HsmAdapterInterface *hsm_adapter = NULL;
    HsmDevice *device = NULL;
    HsmErrorCode error;
    void *plaintext = NULL;
    size_t plaintext_len = 0;

    /* Debug: log function entry */
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                "tecc_key_decrypt_dek called: encrypted_len=%zu, kek_len=%zu",
                encrypted_len, kek_len);

    /* Parameter validation */
    if (encrypted_dek == NULL || encrypted_len == 0 ||
        kek == NULL || kek_len == 0 ||
        decrypted_dek == NULL || decrypted_len == NULL)
    {
        set_last_error("Invalid parameters for DEK decryption");
        tecc_key_log(LOG_LEVEL_ERROR, __func__, __FILE__, __LINE__,
                    "Invalid parameters for DEK decryption: enc_dek=%p, enc_len=%zu, kek=%p, kek_len=%zu, dec_dek=%p, dec_len=%p",
                    (const void*)encrypted_dek, encrypted_len,
                    (const void*)kek, kek_len,
                    (const void*)decrypted_dek, (const void*)decrypted_len);
        return -1;
    }

    /* Check if this is an HSM placeholder key */
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                "Checking if KEK is HSM placeholder: kek_len=%zu", kek_len);
    if (kek_len == 16 && kek_len >= 11 &&
        (memcmp(kek + 4, "HSM_KEY", 7) == 0 || memcmp(kek + 4, "SWXAKEY", 7) == 0))
    {
        /* This is an HSM placeholder key - determine which HSM */
        bool is_swxa = (memcmp(kek + 4, "SWXAKEY", 7) == 0);
        tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                    "KEK is %s HSM placeholder key, using HSM path",
                    is_swxa ? "SWXA" : "Tianzheng");

        /* Extract key index from placeholder */
        key_index = (unsigned int)kek[0] |
                   ((unsigned int)kek[1] << 8) |
                   ((unsigned int)kek[2] << 16) |
                   ((unsigned int)kek[3] << 24);

        /* Get HSM adapter based on marker */
        if (is_swxa)
            hsm_adapter = get_swxa_adapter();
        else
            hsm_adapter = get_tzhsm_adapter();
        if (!hsm_adapter)
        {
            set_last_error("Failed to get %s adapter for DEK decryption",
                         is_swxa ? "SWXA" : "HSM");
            return -1;
        }

        /* Initialize HSM device */
        error = hsm_adapter->init(&device, NULL);
        if (error != HSM_SUCCESS)
        {
            set_last_error("%s initialization failed for DEK decryption: %s",
                         is_swxa ? "SWXA" : "HSM",
                         hsm_adapter->error_string(error));
            return -1;
        }

        /* Connect to HSM */
        error = hsm_adapter->connect(device);
        if (error != HSM_SUCCESS)
        {
            set_last_error("%s connection failed for DEK decryption: %s",
                         is_swxa ? "SWXA" : "HSM",
                         hsm_adapter->error_string(error));
            hsm_adapter->cleanup(device);
            return -1;
        }

        /* Check if key exists */
        error = hsm_adapter->check_key_exists(device, key_index);
        if (error != HSM_SUCCESS)
        {
            set_last_error("%s key %u not found for DEK decryption: %s",
                         is_swxa ? "SWXA" : "HSM",
                         key_index, hsm_adapter->error_string(error));
            hsm_adapter->disconnect(device);
            hsm_adapter->cleanup(device);
            return -1;
        }

        /* Use HSM adapter to decrypt the DEK */
        error = hsm_adapter->decrypt(device, key_index,
                                     encrypted_dek, encrypted_len,
                                     &plaintext, &plaintext_len);

        if (error != HSM_SUCCESS)
        {
            set_last_error("%s decryption failed for DEK: %s",
                         is_swxa ? "SWXA" : "HSM",
                         hsm_adapter->error_string(error));
            hsm_adapter->disconnect(device);
            hsm_adapter->cleanup(device);
            return -1;
        }

        /* Copy decrypted data to output buffer */
        *decrypted_dek = (unsigned char *)tecc_mem_alloc(plaintext_len);
        if (*decrypted_dek == NULL)
        {
            set_last_error("Failed to allocate memory for decrypted DEK");
            free(plaintext);
            hsm_adapter->disconnect(device);
            hsm_adapter->cleanup(device);
            return -1;
        }

        memcpy(*decrypted_dek, plaintext, plaintext_len);
        *decrypted_len = plaintext_len;

        /* Free the plaintext buffer returned by HSM adapter */
        free(plaintext);

        hsm_adapter->disconnect(device);
        hsm_adapter->cleanup(device);

        return 0;
    }
    else
    {
        /* Normal KEK: use security layer to decrypt DEK */
        tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                    "KEK is normal key, using security layer path");
        tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                    "Calling security_decrypt_dek_auto, encrypted_len=%zu, kek_len=%zu",
                    encrypted_len, kek_len);
        ret = security_decrypt_dek_auto(encrypted_dek, encrypted_len,
                                       kek, kek_len,
                                       decrypted_dek, decrypted_len);
        if (ret != 0)
        {
            /* ReqID:TECC4.0 — Bug: tecc_ktool --force 覆盖 KEK 后报错过多。
             * 该日志是纯诊断（错误码已通过 set_last_error 传给上层用户报错），
             * 且 SELECT 每列/行解密失败都会刷屏；降为 DEBUG，用户可见错误由
             * 上层适配器统一输出。需要深度诊断时 TECC_LOG_LEVEL=DEBUG 仍可见。 */
            tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                        "security_decrypt_dek_auto failed with ret=%d", ret);
            set_last_error("DEK decryption failed with error %d", ret);
            return ret;
        }
        tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                    "security_decrypt_dek_auto succeeded, decrypted_len=%zu",
                    *decrypted_len);
    }

    return 0;
}

/*
 * tecc_key_get_decrypted_dek
 *    Get decrypted DEK (convenience function)
 */
unsigned char *
tecc_key_get_decrypted_dek(const void *tecc_state, const char *dek_name, size_t *key_len)
{
    const struct TeccState *state = (const struct TeccState *)tecc_state;
    TeccDekCacheItem *dek_item = NULL;
    unsigned char *kek = NULL;
    size_t kek_len = 0;
    unsigned char *decrypted_dek = NULL;
    size_t decrypted_len = 0;
    int ret = 0;
    const char *schema_name = NULL;

    /* Parameter validation */
    if (state == NULL || dek_name == NULL || key_len == NULL)
    {
        set_last_error("Invalid parameters");
        return NULL;
    }

    /* Note: We cannot access state->conn directly in common directory */
    /* Cache lookup will use default values */
    schema_name = "public";

    /* Get DEK from cache using new interface */
    dek_item = tecc_cache_get_dek_by_name(state, dek_name, schema_name);
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                "[DEK DEBUG] tecc_cache_get_dek_by_name('%s', '%s'): dek_item=%p",
                dek_name, schema_name, (const void*)dek_item);
    if (dek_item == NULL)
    {
        set_last_error("DEK '%s' not found in cache", dek_name);
        return NULL;
    }

    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                "[DEK DEBUG] dek_item: encrypted_key=%p, key_length=%zu, kek_name=%s, status=%d",
                (const void*)dek_item->encrypted_key, dek_item->key_length,
                dek_item->kek_name ? dek_item->kek_name : "(nil)", dek_item->status);

    /* ReqID:TECC4.0 — 检查解密后DEK缓存，命中则直接返回副本 */
    if (dek_item->plain_dek != NULL && dek_item->plain_dek_length > 0)
    {
        unsigned char *cached_copy;

        *key_len = dek_item->plain_dek_length;
        cached_copy = (unsigned char *)tecc_mem_alloc(*key_len);
        if (cached_copy != NULL)
        {
            memcpy(cached_copy, dek_item->plain_dek, *key_len);
            tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                        "DEK '%s' retrieved from plaintext cache (len=%zu)",
                        dek_name, *key_len);
            return cached_copy;
        }
        /* 分配失败则回退到正常解密路径 */
    }

    /* Check DEK status */
    if (dek_item->status != 1)  /* Assuming 1 means active */
    {
        set_last_error("DEK '%s' is not active (status: %d)", dek_name, dek_item->status);
        return NULL;
    }

    /* ReqID:TECC4.0 - Bug 9256: 优先按 KEK OID 解析 —— KEK 可位于任意 schema，
     * 用 DEK 上的裸 KEK 名 + search_path 查找时，KEK 不在 search_path 上就找不到。
     * 无有效 OID 时才退回按名解析（见 tecc_key_get_kek_by_oid_or_name）。 */
    kek = tecc_key_get_kek_by_oid_or_name((void *)tecc_state, dek_item->kek_oid,
                                          dek_item->kek_name, &kek_len);
    if (kek == NULL)
    {
        set_last_error("Failed to get KEK for DEK '%s'", dek_name);
        return NULL;
    }

    /* Decrypt DEK */
    ret = tecc_key_decrypt_dek(dek_item->encrypted_key, dek_item->key_length,
                              kek, kek_len,
                              &decrypted_dek, &decrypted_len);

    /* Secure cleanup */
    tecc_key_secure_erase(kek, kek_len);
    tecc_mem_free(kek);

    if (ret != 0)
    {
        set_last_error("Failed to decrypt DEK '%s'", dek_name);
        return NULL;
    }

    *key_len = decrypted_len;

    /* ReqID:TECC4.0 — 缓存解密后的DEK明文，避免重复SM4-CBC解密 */
    if (decrypted_dek != NULL && decrypted_len > 0 && dek_item->plain_dek == NULL)
    {
        TeccCacheManager *mgr = get_cache_manager(state);
        CACHE_LOCK(mgr);
        if (dek_item->plain_dek == NULL)  /* 双重检查 */
        {
            dek_item->plain_dek = (unsigned char *)tecc_mem_alloc(decrypted_len);
            if (dek_item->plain_dek != NULL)
            {
                memcpy(dek_item->plain_dek, decrypted_dek, decrypted_len);
                dek_item->plain_dek_length = decrypted_len;
                tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                            "Cached decrypted DEK '%s' (len=%zu)", dek_name, decrypted_len);
            }
        }
        CACHE_UNLOCK(mgr);
    }

    return decrypted_dek;
}

/*
 * tecc_key_get_kek_for_dek
 *    Get KEK for a specific DEK
 */
unsigned char *
tecc_key_get_kek_for_dek(const void *tecc_state, const char *dek_name, size_t *kek_len)
{
    KeyManager *manager = NULL;
    PgTeccDekInfo *dek_info = NULL;
    unsigned char *kek = NULL;

    if (dek_name == NULL || kek_len == NULL)
    {
        set_last_error("Invalid parameters");
        return NULL;
    }

    /* Initialize key manager with TeccState for cache access */
    manager = key_manager_create_with_state((void *) tecc_state);
    if (manager == NULL)
    {
        set_last_error("Failed to create key manager");
        return NULL;
    }

    /* Get DEK information to find associated KEK */
    dek_info = key_manager_get_dek_info(manager, dek_name);
    if (dek_info == NULL)
    {
        set_last_error("DEK '%s' not found", dek_name);
        key_manager_destroy(manager);
        return NULL;
    }

    /* Get KEK (always returns fixed KEK) */
    kek = tecc_key_get_kek_with_state((void *)tecc_state, dek_info->kek_name, kek_len);

    /* Clean up */
    key_manager_free_dek_info(dek_info);
    key_manager_destroy(manager);

    return kek;
}

/*
 * tecc_key_validate_dek
 *    Validate DEK
 */
bool
tecc_key_validate_dek(const char *dek_name)
{
    KeyManager *manager = NULL;
    PgTeccDekInfo *dek_info = NULL;
    bool is_valid = false;

    if (dek_name == NULL)
        return false;

    manager = key_manager_create();
    if (manager == NULL)
        return false;

    dek_info = key_manager_get_dek_info(manager, dek_name);
    if (dek_info != NULL)
    {
        /* Check if DEK is active */
        if (strcmp(dek_info->status, "active") == 0)
            is_valid = true;

        key_manager_free_dek_info(dek_info);
    }

    key_manager_destroy(manager);
    return is_valid;
}

/*
 * tecc_key_validate_kek
 *    Validate KEK
 */
bool
tecc_key_validate_kek(const char *kek_name)
{
    /* For fixed KEK, always return true if name matches expected pattern */
    if (kek_name == NULL)
        return false;

    /* In this implementation, we only have one fixed KEK */
    return true;
}

/*
 * tecc_key_free_key_data
 *    Free key data with secure erase
 */
void
tecc_key_free_key_data(unsigned char *key_data, size_t key_len)
{
    if (key_data != NULL && key_len > 0)
    {
        tecc_mem_secure_erase(key_data, key_len);
        tecc_mem_free(key_data);
    }
}

/*
 * tecc_key_secure_erase
 *    Securely erase memory
 */
void
tecc_key_secure_erase(void *data, size_t length)
{
    tecc_mem_secure_erase(data, length);
}

/*
 * tecc_key_get_kek_with_state
 *    Get KEK plaintext with TeccState for cache access.
 *
 * tecc_key_get_kek_with_state_refresh
 *    Same, but bypasses the connection-level plaintext cache and forces a
 *    storage read (file for localkms), so callers can verify the backing
 *    private key still exists even within a warm session. Used by CREATE DEK
 *    to enforce "KEK private key unavailable => error" strictly.
 */
static unsigned char *
tecc_key_get_kek_with_state_internal(void *tecc_state, const char *kek_name,
                                     size_t *key_len, bool skip_cache_read)
{
    KeyManager *manager = NULL;
    PgTeccKekInfo *kek_info = NULL;
    unsigned char *kek_data = NULL;

    if (key_len == NULL)
    {
        set_last_error("Invalid parameter: key_len is NULL");
        return NULL;
    }

    *key_len = 0;

    /* KEK name is required — no fallback to fixed key */
    if (kek_name == NULL || strlen(kek_name) == 0)
    {
        set_last_error("KEK name is NULL or empty, cannot retrieve KEK");
        return NULL;
    }

    /* ReqID:TECC4.0 — 检查明文KEK缓存，命中则直接返回副本，避免文件I/O+PBKDF2。
     * skip_cache_read 为 true 时跳过缓存读取（强制重读存储），供 CREATE DEK
     * 探测私钥可用性使用——同一连接内删除 .priv 后也能稳定报错。 */
    if (!skip_cache_read && tecc_state != NULL)
    {
        TeccKekCacheItem *kek_item = tecc_cache_get_kek_by_name_search_path(
            (const TeccState *)tecc_state, kek_name);
        if (kek_item != NULL && kek_item->plain_kek != NULL && kek_item->plain_kek_length > 0)
        {
            unsigned char *cached_copy;

            *key_len = kek_item->plain_kek_length;
            cached_copy = (unsigned char *)tecc_mem_alloc(*key_len);
            if (cached_copy != NULL)
            {
                memcpy(cached_copy, kek_item->plain_kek, *key_len);
                tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                            "KEK '%s' retrieved from plaintext cache (len=%zu)",
                            kek_name, *key_len);
                return cached_copy;
            }
            /* 分配失败则回退到正常加载路径 */
        }
    }

    /* Initialize key manager with TeccState for cache access */
    manager = key_manager_create_with_state((void *) tecc_state);
    if (manager == NULL)
    {
        set_last_error("Failed to create key manager for KEK '%s'", kek_name);
        return NULL;
    }

    /* Get KEK information (will use cache if TeccState is available) */
    kek_info = key_manager_get_kek_info(manager, kek_name);
    if (kek_info == NULL)
    {
        set_last_error("KEK '%s' not found", kek_name);
        key_manager_destroy(manager);
        return NULL;
    }

    /* Handle different KMS types */
    switch (kek_info->kms_type)
    {
        case KMS_TYPE_LOCAL:
            /* Local KMS: load key data from file using KMS adapter */
            if (kek_info->kms_path != NULL)
            {
                LocalKmsContext context;
                char *last_slash;
                char *key_filename;
                int ret;

                tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                           "Loading local KEK '%s' from file: %s", kek_name, kek_info->kms_path);

                /* Initialize LocalKmsContext */
                memset(&context, 0, sizeof(LocalKmsContext));

                /* Extract base path and filename from full path */
#ifdef _WIN32
                {
                    const char *last_fwd = strrchr(kek_info->kms_path, '/');
                    const char *last_bwd = strrchr(kek_info->kms_path, '\\');
                    last_slash = (last_fwd > last_bwd) ? (char *)last_fwd : (char *)last_bwd;
                }
#else
                last_slash = strrchr(kek_info->kms_path, '/');
#endif
                if (last_slash != NULL)
                {
                    /* Split path into directory and filename */
                    size_t dir_len = last_slash - kek_info->kms_path;

                    /*
                     * Cross-platform support: if TECC_KEK_PATH environment variable
                     * is set, use it as the base directory instead of the stored
                     * absolute path.  This allows KEK files created on one OS
                     * (e.g. Linux with path "/data/keys/mykey.key") to be found
                     * on another OS (e.g. Windows) where the directory structure
                     * differs — the user copies the key files to a known location
                     * and sets TECC_KEK_PATH to that directory.
                     */
                    {
                        const char *env_path = getenv("TECC_KEK_PATH");
                        if (env_path != NULL && env_path[0] != '\0')
                        {
                            context.base_path = tecc_mem_strdup(env_path);
                            tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                                        "TECC_KEK_PATH=%s overrides base_path '%s' -> '%s'",
                                        env_path, kek_info->kms_path, context.base_path);
                        }
                        else
                        {
                            context.base_path = (char *)tecc_mem_alloc(dir_len + 1);
                            if (context.base_path == NULL)
                            {
                                set_last_error("Failed to allocate memory for base path");
                                break;
                            }
                            memcpy(context.base_path, kek_info->kms_path, dir_len);
                            context.base_path[dir_len] = '\0';
                        }
                    }

                    key_filename = last_slash + 1;
                }
                else
                {
                    /* No directory in path, use TECC_KEK_PATH or current directory */
                    {
                        const char *env_path = getenv("TECC_KEK_PATH");
                        if (env_path != NULL && env_path[0] != '\0')
                        {
                            context.base_path = tecc_mem_strdup(env_path);
                            tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                                        "TECC_KEK_PATH=%s overrides base_path (no directory in stored path)",
                                        env_path);
                        }
                        else
                        {
                            context.base_path = tecc_mem_strdup(".");
                        }
                    }
                    key_filename = kek_info->kms_path;
                }

                /* Store the full filename (including .key extension) in context */
                context.key_filename = tecc_mem_strdup(key_filename);
                if (context.key_filename == NULL)
                {
                    tecc_mem_free(context.base_path);
                    set_last_error("Failed to allocate memory for key filename");
                    break;
                }

                /* Note: We keep the .key extension in context.key_filename because
                 * local_kms_get_key_path expects the full filename including extension.
                 * The actual key name (without extension) is passed as the key_name
                 * parameter to local_kms_load_key. */

                context.file_mode = 0600;  /* Default file mode */
                context.encrypt_files = true;  /* Assume files are encrypted */

                /* Load key using KMS adapter (this will decrypt if encrypted) */
                tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                            "[KEK DEBUG] About to call local_kms_load_key: file=%s, base_path=%s",
                            context.key_filename, context.base_path);
                ret = local_kms_load_key(&context, context.key_filename, &kek_data, key_len);
                tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                            "[KEK DEBUG] local_kms_load_key returned: ret=%d, kek_data=%p, key_len=%zu",
                            ret, (const void*)kek_data, *key_len);
                if (ret != 0)
                {
                    set_last_error("Failed to load KEK '%s' from file '%s': "
                                   "check if key file exists, is readable, and was not replaced",
                                   kek_name, kek_info->kms_path);
                    /* ReqID:TECC4.0 — Bug: tecc_ktool delete 删除 KEK 文件后重连查询报错。
                     * 该日志是纯诊断（可执行信息已通过 set_last_error 传给上层用户报错），
                     * 且 SELECT 每个列/行解密失败都会刷屏；降为 DEBUG。文件缺失在调度器
                     * 层本就被归类为 skipped_decrypt（静默返回密文），不需要 ERROR 提示。
                     * 需要深度诊断时把日志级别调到 DEBUG 仍可见。 */
                    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                                "[KEK ERROR] local_kms_load_key FAILED for kek '%s', path='%s'",
                                kek_name, kek_info->kms_path);
                    tecc_mem_free(context.base_path);
                    tecc_mem_free(context.key_filename);
                    break;
                }

                tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                            "[KEK DATA] Loaded KEK '%s' from file, len=%zu", kek_name, *key_len);

                /* Clean up */
                tecc_mem_free(context.base_path);
                tecc_mem_free(context.key_filename);
            }
            else
            {
                set_last_error("Local KEK '%s' has no key path specified", kek_name);
            }
            break;

        case KMS_TYPE_HSM:
            /* HSM KMS: return placeholder with key index (like backend get_kek_from_system_table),
             * so that callers detect HSM key via kek_len==16 and "HSM_KEY" marker,
             * and route to hsm_adapter->encrypt/decrypt for actual DEK operations. */
            {
                unsigned int key_index = 0;

                /* Parse key index from kms_path (format: hsm:id=<index>) */
                if (!parse_hsm_key_path(kek_info->kms_path, &key_index))
                {
                    set_last_error("Invalid HSM key path format for KEK '%s': %s",
                                 kek_name, kek_info->kms_path);
                    break;
                }

                /* Create HSM placeholder key (16 bytes) */
                *key_len = 16;
                kek_data = (unsigned char *)tecc_mem_alloc(*key_len);
                if (kek_data == NULL)
                {
                    set_last_error("Failed to allocate HSM placeholder for KEK '%s'", kek_name);
                    break;
                }
                memset(kek_data, 0, *key_len);

                /* Store key index in first 4 bytes (little-endian) */
                kek_data[0] = (key_index >> 0) & 0xFF;
                kek_data[1] = (key_index >> 8) & 0xFF;
                kek_data[2] = (key_index >> 16) & 0xFF;
                kek_data[3] = (key_index >> 24) & 0xFF;

                /* Mark as HSM key */
                memcpy(kek_data + 4, "HSM_KEY", 7);

                tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                            "Created HSM placeholder for KEK '%s', key_index=%u",
                            kek_name, key_index);
            }
            break;

        case KMS_TYPE_HSM_SWXA:
            /* SWXA HSM KMS: return placeholder with key index,
             * using "SWXAKEY" marker so callers route to SWXA adapter. */
            {
                unsigned int key_index = 0;

                if (!parse_hsm_key_path(kek_info->kms_path, &key_index))
                {
                    set_last_error("Invalid SWXA HSM key path format for KEK '%s': %s",
                                 kek_name, kek_info->kms_path);
                    break;
                }

                *key_len = 16;
                kek_data = (unsigned char *)tecc_mem_alloc(*key_len);
                if (kek_data == NULL)
                {
                    set_last_error("Failed to allocate SWXA HSM placeholder for KEK '%s'", kek_name);
                    break;
                }
                memset(kek_data, 0, *key_len);

                kek_data[0] = (key_index >> 0) & 0xFF;
                kek_data[1] = (key_index >> 8) & 0xFF;
                kek_data[2] = (key_index >> 16) & 0xFF;
                kek_data[3] = (key_index >> 24) & 0xFF;

                /* Mark as SWXA HSM key */
                memcpy(kek_data + 4, "SWXAKEY", 7);

                tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                            "Created SWXA HSM placeholder for KEK '%s', key_index=%u",
                            kek_name, key_index);
            }
            break;

        default:
            set_last_error("Unsupported KMS type %d for KEK '%s'",
                         kek_info->kms_type, kek_name);
            break;
    }

    /* ReqID:TECC4.0 — 将加载的KEK明文缓存到TeccKekCacheItem，避免后续重复加载 */
    if (kek_data != NULL && *key_len > 0 && tecc_state != NULL && kek_name != NULL)
    {
        TeccKekCacheItem *kek_item = tecc_cache_get_kek_by_name_search_path(
            (const TeccState *)tecc_state, kek_name);
        if (kek_item != NULL && kek_item->plain_kek == NULL)
        {
            /* ReqID:TECC4.0 - 增加 mgr NULL 检查，防止缓存管理器失效时段错误 */
            TeccCacheManager *mgr = get_cache_manager((const TeccState *)tecc_state);
            if (mgr == NULL)
            {
                tecc_key_log(LOG_LEVEL_WARNING, __func__, __FILE__, __LINE__,
                             "Cache manager unavailable, skipping plaintext KEK cache for '%s'",
                             kek_name);
            }
            CACHE_LOCK(mgr);
            if (kek_item->plain_kek == NULL)  /* 双重检查，防止并发写入 */
            {
                kek_item->plain_kek = (unsigned char *)tecc_mem_alloc(*key_len);
                if (kek_item->plain_kek != NULL)
                {
                    memcpy(kek_item->plain_kek, kek_data, *key_len);
                    kek_item->plain_kek_length = *key_len;
                    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                                "Cached plaintext KEK '%s' (len=%zu)", kek_name, *key_len);
                }
            }
            CACHE_UNLOCK(mgr);
        }
    }

    /* Cleanup */
    if (kek_info != NULL)
        key_manager_free_kek_info(kek_info);
    if (manager != NULL)
        key_manager_destroy(manager);

    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                "tecc_key_get_kek_with_state returning: kek_data=%p, key_len=%zu",
                (const void*)kek_data, *key_len);
    return kek_data;
}

/*
 * tecc_key_get_kek_with_state
 *    Get KEK plaintext with TeccState for cache access.
 */
unsigned char *
tecc_key_get_kek_with_state(void *tecc_state, const char *kek_name, size_t *key_len)
{
    return tecc_key_get_kek_with_state_internal(tecc_state, kek_name, key_len, false);
}

/*
 * tecc_key_get_kek_with_state_refresh
 *    Get KEK plaintext bypassing the connection-level cache, forcing a storage
 *    read. Returns NULL (and sets last_error) if the backing key file is
 *    missing or unreadable, regardless of cached state.
 */
unsigned char *
tecc_key_get_kek_with_state_refresh(void *tecc_state, const char *kek_name, size_t *key_len)
{
    return tecc_key_get_kek_with_state_internal(tecc_state, kek_name, key_len, true);
}

/*
 * tecc_key_get_kek_by_oid_or_name
 *    Get the KEK bound to a DEK, resolved by OID first (Bug 9256).
 *
 * DEK 的目录记录里存的是 KEK 的 kek_id(OID)，而 KEK 可创建在任意 schema。
 * 只按裸名 + search_path 解析时，KEK 不在 search_path 上就永远找不到（在
 * public 下用 sch1.kek 建的加密表 INSERT 直接报 "Failed to get KEK"）。
 * 故先用 OID 拿到 KEK 缓存项，再用限定名 "schema.name" 去取明文 —— 限定名在
 * tecc_cache_get_kek_by_name_search_path 与 key_manager_get_kek_info 里都是
 * 精确查找、不回退 search_path。
 *
 * OID 有效但解析不到时不回退按名解析：同名 KEK 可能是另一个 schema 里的另一
 * 把密钥，用错密钥只会把问题变成难以定位的"解密失败"。
 */
unsigned char *
tecc_key_get_kek_by_oid_or_name(void *tecc_state, Oid kek_oid,
                                const char *kek_name, size_t *key_len)
{
    char *qualified = NULL;
    unsigned char *kek = NULL;

    if (key_len == NULL)
    {
        set_last_error("Invalid parameter: key_len is NULL");
        return NULL;
    }

    *key_len = 0;

    if (tecc_state != NULL && kek_oid != InvalidOid)
    {
        TeccKekCacheItem *kek_item = tecc_cache_get_kek_by_oid(
            (const TeccState *)tecc_state, kek_oid);

        if (kek_item == NULL || kek_item->key_name == NULL || kek_item->key_name[0] == '\0')
        {
            set_last_error("KEK with OID %u not found for DEK's KEK", kek_oid);
            return NULL;
        }

        if (kek_item->key_namespace != NULL && kek_item->key_namespace[0] != '\0')
        {
            size_t qualified_len = strlen(kek_item->key_namespace) + 1 +
                                   strlen(kek_item->key_name) + 1;

            qualified = (char *)tecc_mem_alloc(qualified_len);
            if (qualified == NULL)
            {
                set_last_error("Out of memory qualifying KEK '%s'", kek_item->key_name);
                return NULL;
            }
            snprintf(qualified, qualified_len, "%s.%s",
                     kek_item->key_namespace, kek_item->key_name);
        }

        kek = tecc_key_get_kek_with_state_internal(tecc_state,
                                                   qualified ? qualified : kek_item->key_name,
                                                   key_len, false);
        if (qualified != NULL)
            tecc_mem_free(qualified);

        return kek;
    }

    /* 无有效 OID（异常/旧缓存项）：退回按名解析 */
    if (kek_name == NULL || kek_name[0] == '\0')
    {
        set_last_error("KEK name is NULL and KEK OID is invalid, cannot retrieve KEK");
        return NULL;
    }

    return tecc_key_get_kek_with_state_internal(tecc_state, kek_name, key_len, false);
}

/*
 * tecc_key_get_last_error
 *    Get the last error message
 */
const char *
tecc_key_get_last_error(void)
{
    char *buffer = get_last_error_buffer();
    if (buffer == NULL)
        return "";
    return buffer;
}

/*
 * tecc_key_clear_last_error
 *    Clear the last error message
 */
void
tecc_key_clear_last_error(void)
{
    char *buffer = get_last_error_buffer();
    if (buffer != NULL)
        memset(buffer, 0, 512);
}