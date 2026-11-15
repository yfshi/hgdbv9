/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_key_manager_adapter.h
 *
 * Abstract:
 *   Key manager adapter interface for TECC crypto scheduler module.
 *   Provides unified interface to key management module.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_scheduler/tecc_key_manager_adapter.h
 */

#ifndef TECC_KEY_MANAGER_ADAPTER_H
#define TECC_KEY_MANAGER_ADAPTER_H

#include "postgres_fe.h"
#include "c.h"

/* Use common definitions */
#include "../../../common/tecc_key/tecc_common.h"

/* Key types are already defined in tecc_common.h */

/* Key information structure */
typedef struct TeccKeyInfo
{
    char key_name[NAMEDATALEN];     /* Key name */
    TeccKeyType key_type;           /* Key type */
    Oid key_oid;                    /* Key OID in system tables */
    bool is_valid;                  /* Whether key is valid */
    time_t created_time;            /* Creation time */
    time_t expires_time;            /* Expiration time */
} TeccKeyInfo;

/* Key cache entry */
typedef struct TeccKeyCacheEntry
{
    TeccKeyInfo key_info;           /* Key information */
    unsigned char *key_data;        /* Decrypted key data */
    size_t key_length;              /* Key length */
    unsigned char derived_enc_key[16];  /* ReqID:TECC4.0 - cached HKDF-derived enc key */
    unsigned char derived_auth_key[32]; /* ReqID:TECC4.0 - cached HKDF-derived auth key */
    bool derived_keys_valid;        /* ReqID:TECC4.0 - true if derived keys are populated */
    int ref_count;                  /* Reference count */
    time_t last_access;             /* Last access time */
} TeccKeyCacheEntry;

/* Key manager adapter */
typedef struct TeccKeyManagerAdapter
{
    void *key_manager;              /* Reference to actual key manager */
    void *tecc_state;               /* TeccState pointer for cache access */
    TeccKeyCacheEntry *key_cache;   /* Key cache array */
    int cache_size;                 /* Cache size */
    int max_cache_size;             /* Maximum cache size */
    bool cache_enabled;             /* Whether caching is enabled */
    int last_error;                 /* Last error code (thread-safe per adapter) */
    /* ReqID:TECC4.0 — 负缓存：本连接内已解密失败的 DEK OID 集合。
     * KEK 明文按连接固定（仅连接建立时加载一次），某 DEK 一旦解密失败
     * （如 KEK 密钥文件被 tecc_ktool --force 覆盖），重试必然再次失败，
     * 且会对每个列/每行重复报错。记录后短路重试并只报一次错。 */
    Oid        failed_dek_oids[16];
    int        num_failed_deks;
} TeccKeyManagerAdapter;

/* Function prototypes */

/* Real key management function declarations (from high_level_api.h) */
/* tecc_key_get_kek removed - use tecc_key_get_kek_with_state instead */
extern int tecc_key_decrypt_dek(const unsigned char *encrypted_dek,
                               size_t encrypted_len,
                               const unsigned char *kek,
                               size_t kek_len,
                               unsigned char **decrypted_dek,
                               size_t *decrypted_len);
extern bool tecc_key_validate_dek(const char *dek_name);
extern bool tecc_key_validate_kek(const char *kek_name);
extern void tecc_key_free_key_data(unsigned char *key_data, size_t key_len);
extern void tecc_key_secure_erase(void *data, size_t length);
extern const char *tecc_key_get_last_error(void);
extern void tecc_key_clear_last_error(void);

/* Key manager initialization */
extern TeccKeyManagerAdapter *tecc_key_manager_adapter_create(void);
extern TeccKeyManagerAdapter *tecc_key_manager_adapter_create_with_state(void *tecc_state);
extern void tecc_key_manager_adapter_destroy(TeccKeyManagerAdapter *adapter);
extern bool tecc_key_manager_adapter_init(TeccKeyManagerAdapter *adapter, void *key_manager);

/* Key operations - backward compatibility wrappers */
extern unsigned char *tecc_key_adapter_get_kek(const char *kek_name, size_t *key_len);

/* Thread-safe key operations with adapter parameter */
extern unsigned char *tecc_key_get_kek_with_adapter(TeccKeyManagerAdapter *adapter,
                                                   const char *kek_name, size_t *key_len);
extern unsigned char *tecc_key_get_dek_with_adapter(TeccKeyManagerAdapter *adapter,
                                                   const char *dek_name, size_t *key_len,
                                                   void *tecc_state);

/* Key adapter interface for crypto scheduler */
extern int tecc_key_manager_adapter_get_dek_key(TeccKeyManagerAdapter *adapter,
                                               const char *dek_name,
                                               unsigned char **dek_key,
                                               size_t *dek_key_len);
extern int tecc_key_manager_adapter_get_dek_key_ext(TeccKeyManagerAdapter *adapter,
                                               const char *dek_name,
                                               unsigned char **dek_key,
                                               size_t *dek_key_len,
                                               unsigned char **derived_enc_key,
                                               unsigned char **derived_auth_key);
extern void tecc_key_manager_adapter_release_key(TeccKeyManagerAdapter *adapter,
                                                unsigned char *key,
                                                size_t key_len);

/* Key validation */
extern bool tecc_key_validate(const char *key_name, TeccKeyType key_type);
extern bool tecc_key_is_expired(const char *key_name);
extern TeccKeyInfo *tecc_key_get_info(const char *key_name);

/* Key caching — DEK only, keyed by OID (globally unique).
 * KEK is cached at the TeccKekCacheItem level (plain_kek field). */
extern bool tecc_key_cache_enable(TeccKeyManagerAdapter *adapter, bool enable);
extern bool tecc_key_cache_add(TeccKeyManagerAdapter *adapter, Oid key_oid,
                              const char *key_name,
                              const unsigned char *key_data, size_t key_len);
extern unsigned char *tecc_key_cache_get(TeccKeyManagerAdapter *adapter,
                                        Oid key_oid, size_t *key_len);
extern void tecc_key_cache_clear(TeccKeyManagerAdapter *adapter);
extern void tecc_key_cache_expire(TeccKeyManagerAdapter *adapter, Oid key_oid);

/* Key utilities */
extern const char *tecc_key_type_to_string(TeccKeyType key_type);
extern bool tecc_key_name_is_valid(const char *key_name);
extern char *tecc_key_generate_default_name(TeccKeyType key_type);

/* Key adapter interface for crypto scheduler (duplicate removed, see above) */

/* Error handling */
extern const char *tecc_key_get_error_message(int error_code);
extern const char *tecc_key_adapter_get_last_error(void);
extern const char *tecc_key_get_last_error_with_adapter(TeccKeyManagerAdapter *adapter);
extern void tecc_key_set_last_error(TeccKeyManagerAdapter *adapter, int error_code);


/* ==================== HE Key Stub Functions (ReqID:TECC4.0) ==================== */

/*
 * Get HE public key for encryption.
 * Stub implementation returns pre-generated test keys.
 * Real implementation will retrieve from hg_dek_args when key management is ready.
 *
 * @param adapter Key manager adapter
 * @param dek_oid DEK OID (used to locate public key metadata)
 * @param algo_type TECC_ALGO_EC_ELGAMAL or TECC_ALGO_ELGAMAL
 * @param pubkey Output: allocated public key bytes
 * @param pubkey_len Output: public key length in bytes
 * @return TECC_CRYPTO_SUCCESS on success
 */
int tecc_key_manager_adapter_get_he_pubkey(TeccKeyManagerAdapter *adapter,
    Oid dek_oid, int algo_type,
    unsigned char **pubkey, size_t *pubkey_len);

/*
 * Get HE private key for decryption.
 * Stub implementation returns pre-generated test keys.
 *
 * @param adapter Key manager adapter
 * @param dek_oid DEK OID
 * @param algo_type TECC_ALGO_EC_ELGAMAL or TECC_ALGO_ELGAMAL
 * @param privkey Output: allocated private key bytes
 * @param privkey_len Output: private key length in bytes
 * @return TECC_CRYPTO_SUCCESS on success
 */
int tecc_key_manager_adapter_get_he_privkey(TeccKeyManagerAdapter *adapter,
    Oid dek_oid, int algo_type,
    unsigned char **privkey, size_t *privkey_len);

/* Release HE key buffer allocated by get_he_pubkey/get_he_privkey */
void tecc_key_manager_adapter_release_he_key(TeccKeyManagerAdapter *adapter,
    unsigned char *key, size_t key_len);

#endif /* TECC_KEY_MANAGER_ADAPTER_H */
/* End - ReqID:TECC4.0 */