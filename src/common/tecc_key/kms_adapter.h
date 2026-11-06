/*-------------------------------------------------------------------------
 *
 * kms_adapter.h
 *    KMS adapter layer for TECC key management module
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/common/tecc_key/kms_adapter.h
 *
 * ReqID:TECC4.0 - Key management KMS adapter layer
 * Authored by <wanghaoyan@highgo.com>, 20260408.
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_KEY_KMS_ADAPTER_H
#define TECC_KEY_KMS_ADAPTER_H

#include "postgres_fe.h"
#include "c.h"
#include "tecc_common.h"

/* KMS operation types */
typedef enum KmsOperationType
{
    KMS_OP_CREATE_KEY,      /* Create key */
    KMS_OP_LOAD_KEY,        /* Load key */
    KMS_OP_DELETE_KEY,      /* Delete key */
    KMS_OP_LIST_KEYS,       /* List keys */
    KMS_OP_ROTATE_KEY,      /* Rotate key */
    KMS_OP_ENCRYPT,         /* Encrypt data */
    KMS_OP_DECRYPT          /* Decrypt data */
} KmsOperationType;

/* KMS configuration structure */
typedef struct KmsConfig
{
    KmsType kms_type;           /* KMS type: LOCAL/HSM/CLOUD */
    char *kms_path;             /* KMS path or identifier */
    char *kms_args;             /* Additional KMS arguments */
    char *algorithm;            /* Encryption algorithm */
    int key_length;             /* Key length */
    bool require_hmac;          /* Whether HMAC is required */
    char *hmac_algorithm;       /* HMAC algorithm */
    int hmac_key_length;        /* HMAC key length */
} KmsConfig;

/* KMS operation result */
typedef struct KmsOperationResult
{
    KmsOperationType op_type;   /* Operation type */
    int error_code;             /* Error code (0 for success) */
    char *error_message;        /* Error message */
    void *result_data;          /* Operation result data */
    size_t result_size;         /* Result data size */
    time_t operation_time;      /* Operation timestamp */
} KmsOperationResult;

/* KMS adapter interface */
typedef struct KmsAdapter
{
    /* KMS type */
    KmsType kms_type;

    /* Configuration */
    KmsConfig *config;

    /* State */
    bool initialized;
    bool connected;

    /* KMS-specific context */
    void *kms_context;

    /* Operation methods */
    int (*init)(struct KmsAdapter *adapter, const KmsConfig *config);
    int (*connect)(struct KmsAdapter *adapter);
    int (*disconnect)(struct KmsAdapter *adapter);
    void (*cleanup)(struct KmsAdapter *adapter);

    /* Key operations */
    int (*create_key)(struct KmsAdapter *adapter, const char *key_name,
                     const unsigned char *key_data, size_t key_len,
                     KmsOperationResult *result);
    int (*load_key)(struct KmsAdapter *adapter, const char *key_name,
                   unsigned char **key_data, size_t *key_len,
                   KmsOperationResult *result);
    int (*delete_key)(struct KmsAdapter *adapter, const char *key_name,
                     KmsOperationResult *result);
    int (*list_keys)(struct KmsAdapter *adapter, char ***key_names,
                    int *key_count, KmsOperationResult *result);

    /* Data operations */
    int (*encrypt_data)(struct KmsAdapter *adapter, const char *key_name,
                       const unsigned char *plaintext, size_t plain_len,
                       unsigned char **ciphertext, size_t *cipher_len,
                       KmsOperationResult *result);
    int (*decrypt_data)(struct KmsAdapter *adapter, const char *key_name,
                       const unsigned char *ciphertext, size_t cipher_len,
                       unsigned char **plaintext, size_t *plain_len,
                       KmsOperationResult *result);

    /* Utility methods */
    int (*test_connection)(struct KmsAdapter *adapter, KmsOperationResult *result);
    int (*get_status)(struct KmsAdapter *adapter, KmsOperationResult *result);
    const char *(*get_kms_info)(struct KmsAdapter *adapter);
} KmsAdapter;

/* Local KMS specific structure (file-based) */
typedef struct LocalKmsContext
{
    char *base_path;            /* Base directory for key storage */
    char *key_filename;         /* Key filename (extracted from key_path) */
    char *key_file_pattern;     /* Key file naming pattern */
    mode_t file_mode;           /* File permissions */
    bool encrypt_files;         /* Whether to encrypt key files */
    char *encryption_key;       /* Key for file encryption */
} LocalKmsContext;

/* HSM KMS specific structure */
typedef struct HsmKmsContext
{
    char *hsm_library_path;     /* Path to HSM library */
    char *hsm_slot;             /* HSM slot identifier */
    char *hsm_pin;              /* HSM PIN/password */
    void *hsm_handle;           /* HSM library handle */
    void *hsm_session;          /* HSM session handle */
} HsmKmsContext;

/* KMS factory function type */
typedef KmsAdapter *(*KmsFactoryFunc)(const KmsConfig *config);

/* Function prototypes */

/* KMS factory */
extern KmsAdapter *kms_adapter_create(KmsType kms_type, const KmsConfig *config);
extern void kms_adapter_destroy(KmsAdapter *adapter);

/* KMS configuration */
extern KmsConfig *kms_config_create(KmsType kms_type, const char *kms_path,
                                   const char *kms_args);
extern void kms_config_destroy(KmsConfig *config);
extern int kms_config_validate(const KmsConfig *config);

/* Local KMS adapter */
extern KmsAdapter *local_kms_adapter_create(const KmsConfig *config);
extern int local_kms_adapter_init(KmsAdapter *adapter, const KmsConfig *config);
extern int local_kms_save_key(LocalKmsContext *context, const char *key_name,
                             const unsigned char *key_data, size_t key_len);
extern int local_kms_load_key(LocalKmsContext *context, const char *key_name,
                             unsigned char **key_data, size_t *key_len);
extern int local_kms_delete_key(LocalKmsContext *context, const char *key_name);
extern char *local_kms_get_key_path(LocalKmsContext *context, const char *key_name);

/* HSM KMS adapter (stub for now) */
extern KmsAdapter *hsm_kms_adapter_create(const KmsConfig *config);
extern int hsm_kms_adapter_init(KmsAdapter *adapter, const KmsConfig *config);

/* KMS operation result handling */
extern KmsOperationResult *kms_operation_result_create(KmsOperationType op_type);
extern void kms_operation_result_destroy(KmsOperationResult *result);
extern void kms_operation_result_set_error(KmsOperationResult *result,
                                          int error_code, const char *error_message);
extern void kms_operation_result_set_success(KmsOperationResult *result,
                                            void *data, size_t size);

/* KMS utility functions */
extern const char *kms_type_to_string(KmsType kms_type);
extern KmsType string_to_kms_type(const char *kms_type_str);
extern bool kms_type_supported(KmsType kms_type);
extern int kms_test_connection(KmsAdapter *adapter);

/* KMS registry */
extern int kms_register_factory(KmsType kms_type, KmsFactoryFunc factory);
extern int kms_unregister_factory(KmsType kms_type);
extern KmsFactoryFunc kms_get_factory(KmsType kms_type);

#endif /* TECC_KEY_KMS_ADAPTER_H */