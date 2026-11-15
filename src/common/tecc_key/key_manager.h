/*-------------------------------------------------------------------------
 *
 * key_manager.h
 *    Core key manager for TECC key management module
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/common/tecc_key/key_manager.h
 *
 * ReqID:TECC4.0 - Key management core manager
 * Authored by <wanghaoyan@highgo.com>, 20260408.
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_KEY_MANAGER_H
#define TECC_KEY_MANAGER_H

#include "tecc_common.h"
#ifdef _WIN32
#include <port/win32/pthread.h>
#else
#include <pthread.h>
#endif

/* KEK information structure */
typedef struct PgTeccKekInfo
{
    char *kek_name;            /* KEK name */
    KmsType kms_type;          /* KMS type */
    char *kms_path;            /* KMS path */
    void *key_data;            /* KEK data (plaintext) */
    size_t key_len;            /* KEK length */
    Oid kek_oid;               /* Server-side KEK OID */
    char *algorithm;           /* Encryption algorithm */
    char *status;              /* Key status */
    TimestampTz created_time;  /* Creation time */
} PgTeccKekInfo;

/* DEK information structure */
typedef struct PgTeccDekInfo
{
    char *dek_name;            /* DEK name */
    char *kek_name;            /* Associated KEK name */
    void *encrypted_key;       /* Encrypted DEK */
    size_t encrypted_len;      /* Encrypted DEK length */
    void *decrypted_key;       /* Decrypted DEK (cached) */
    size_t decrypted_len;      /* Decrypted DEK length */
    TeccAlgorithmType algo_type; /* Supported algorithm type */
    bool supports_det;         /* Supports DET algorithm */
    bool supports_ope;         /* Supports OPE algorithm */
    bool supports_search;      /* Supports SEARCH algorithm */
    Oid dek_oid;               /* Server-side DEK OID */
    char *status;              /* Key status */
    TimestampTz created_time;  /* Creation time */
} PgTeccDekInfo;

/* Note: Cache functionality is now provided by tecc_metadata_cache module */

/* Key manager structure */
typedef struct KeyManager
{
    /* Connection management */
    void *db_connection;                /* Database connection */
    bool connection_valid;              /* Whether connection is valid */

    /* TECC state for cache access */
    void *tecc_state;                   /* TeccState pointer for cache access */

    /* Synchronization */
    pthread_mutex_t lock;               /* Manager lock */

    /* Note: Cache functionality is now provided by tecc_metadata_cache module */
} KeyManager;

/* Function prototypes */

/* Key manager lifecycle */
extern KeyManager *key_manager_create(void);
extern KeyManager *key_manager_create_with_state(void *tecc_state);
extern void key_manager_destroy(KeyManager *manager);
extern int key_manager_init(KeyManager *manager);
extern void key_manager_cleanup(KeyManager *manager);

/* KEK operations */
extern PgTeccKekInfo *key_manager_get_kek_info(KeyManager *manager, const char *kek_name);
extern void key_manager_free_kek_info(PgTeccKekInfo *kek_info);
extern int key_manager_load_kek(KeyManager *manager, const char *kek_name, PgTeccKekInfo **kek_info);

/* DEK operations */
extern PgTeccDekInfo *key_manager_get_dek_info(KeyManager *manager, const char *dek_name);
extern void key_manager_free_dek_info(PgTeccDekInfo *dek_info);
extern int key_manager_load_dek(KeyManager *manager, const char *dek_name, PgTeccDekInfo **dek_info);

/* Note: Cache management functions are removed - use tecc_metadata_cache module instead */

/* Permission checking */
extern bool key_manager_check_dek_permission(KeyManager *manager, const char *dek_name, Oid user_oid);
extern bool key_manager_check_kek_permission(KeyManager *manager, const char *kek_name, Oid user_oid);

/* Connection management */
extern int key_manager_ensure_connection(KeyManager *manager);
extern void key_manager_close_connection(KeyManager *manager);

/* Configuration - cache configuration is handled by tecc_metadata_cache module */

#endif /* TECC_KEY_MANAGER_H */