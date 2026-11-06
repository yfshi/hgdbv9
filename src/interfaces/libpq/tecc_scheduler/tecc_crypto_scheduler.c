/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_crypto_scheduler.c
 *
 * Abstract:
 *   Main implementation of unified crypto scheduler for TECC module.
 *   Provides unified encryption/decryption interfaces with zero-copy support.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_scheduler/tecc_crypto_scheduler.c
 */

#include "tecc_crypto_scheduler.h"
#include "tecc_algorithm_scheduler.h"
#include "tecc_key_manager_adapter.h"
#include "../tecc_processor/internal/tecc_processor_internal.h"
#include "../tecc_formatter/tecc_formatter.h"  /* Data formatter interface */
#include "../tecc_formatter/tecc_formatter_utils.h"  /* Formatter utilities */
#include "../tecc_formatter/tecc_data_validator.h"  /* Data validator interface */
#include "../tecc_cache/tecc_metadata_cache.h"  /* Metadata cache interface */
#include "../tecc_data_fetcher/include/tecc_data_fetcher.h"  /* For tecc_free_encrypted_column_metadata */
#include "../../../common/tecc_types.h"  /* Formatter error code definitions */
#include "common/fe_memutils.h"  /* Frontend memory management */
#include "../tecc_libpq_integration/fe-tecc-integration.h"  /* TECC state management */
#include "../tecc_common/tecc_logging.h"  /* TECC logging system */
#include "../tecc_formatter/tecc_type_output.h"  /* Numeric text-to-binary conversion */
#include "../../../common/tecc_key/high_level_api.h"  /* tecc_key_get_last_error */
/* ReqID:TECC4.0 - forward declarations (no include to avoid bool conflict) */
extern int tecc_sm4_rnd_decrypt_with_keys(const unsigned char *key, size_t key_len,
    const unsigned char *ciphertext, size_t cipher_len,
    unsigned char *output_buffer, size_t buffer_size, size_t *output_len,
    const unsigned char *derived_enc_key, const unsigned char *derived_auth_key);
extern int tecc_sm4_rnd_encrypt_with_keys(const unsigned char *key, size_t key_len,
    const unsigned char *plaintext, size_t plain_len,
    unsigned char *output_buffer, size_t buffer_size, size_t *output_len,
    const unsigned char *derived_enc_key, const unsigned char *derived_auth_key);
extern int tecc_sm4_rnd_batch_decrypt_with_keys(
    const unsigned char *enc_key, const unsigned char *auth_key,
    const unsigned char **ciphertexts, const size_t *ciphertext_lens,
    size_t count, unsigned char **plaintexts, size_t *plaintext_lens);
#include "../tecc_hsm_adapter/fe-hsm-adapter.h"  /* HsmAdapterInterface, HsmDevice */
#include <string.h>
#include <stdlib.h>  /* For free() function */
#include <stdio.h>   /* For fprintf */
#include <sys/time.h> /* For gettimeofday */

/* Forward declaration from stmt_processor.h for error propagation */
extern void tecc_record_error(struct TeccState *state, int error_code,
                              const char *error_msg, const char *function_name,
                              int line_number, bool recoverable);

/* Utility macros */
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

/* Frontend error handling macros (replace ereport) */
/* Use tecc_processor_debug instead of direct stderr access for shared library compatibility */
#define TECC_SCHEDULER_ERROR(format, ...) \
    do { \
        tecc_processor_debug(TECC_LOG_ERROR, "TECC SCHEDULER ERROR: " format, ##__VA_ARGS__); \
    } while (0)

#define TECC_SCHEDULER_WARNING(format, ...) \
    do { \
        tecc_processor_debug(TECC_LOG_WARNING, "TECC SCHEDULER WARNING: " format, ##__VA_ARGS__); \
    } while (0)

#define TECC_SCHEDULER_WARN TECC_SCHEDULER_WARNING

#define TECC_SCHEDULER_DEBUG(format, ...) \
    do { \
        TECC_LOG(TECC_LOG_DEBUG1, "SCHEDULER: " format, ##__VA_ARGS__); \
    } while (0)

/* Performance measurement helper macros */
#define TECC_START_TIMER(start_tv) gettimeofday(&(start_tv), NULL)
#define TECC_END_TIMER(start_tv, end_tv, elapsed_ms) \
    do { \
        gettimeofday(&(end_tv), NULL); \
        (elapsed_ms) = ((end_tv).tv_sec - (start_tv).tv_sec) * 1000.0 + \
                      ((end_tv).tv_usec - (start_tv).tv_usec) / 1000.0; \
    } while (0)

/* Forward declarations for incomplete types */
typedef struct TeccMetadataCache TeccMetadataCache;
typedef struct TeccKeyManager TeccKeyManager;

/*
 * Map encrypted type OID to original PostgreSQL type OID.
 * E.g., NUMERIC_ENC_OID(50005) -> NUMERICOID(1700).
 * If the OID is not an encrypted type, returns it unchanged.
 * ReqID:TECC4.0
 */
static inline Oid
tecc_map_to_original_type_oid(Oid enc_oid)
{
    switch (enc_oid)
    {
        case INT2_ENC_OID:    return INT2OID;
        case INT4_ENC_OID:    return INT4OID;
        case INT8_ENC_OID:    return INT8OID;
        case FLOAT4_ENC_OID:  return FLOAT4OID;
        case FLOAT8_ENC_OID:  return FLOAT8OID;
        case NUMERIC_ENC_OID: return NUMERICOID;
        case VARCHAR_ENC_OID: return VARCHAROID;
        case BPCHAR_ENC_OID:  return BPCHAROID;
        case TEXT_ENC_OID:    return TEXTOID;
        default:              return enc_oid;
    }
}

/* Global scheduler instance - REMOVED for thread safety */
/* Connection-specific scheduler should be used instead, stored in TeccState structure */

/*
 * Set up HSM SM4 encryption on context when the KEK's KMS type is HSM.
 * Checks the KEK cache item's kms_type field (2 = hsmkms, 4 = swxakms)
 * and selects the appropriate HSM adapter (Tianzheng or SWXA).
 *
 * HSM initialization is attempted once per adapter type and cached statically.
 * The decision to USE HSM is per-call based on the current column's KEK type,
 * so soft (localkms) and hard (hsmkms/swxakms) columns can coexist.
 * If no HSM info or not an HSM KEK, leaves context fields NULL (software SM4).
 */
static int
tecc_setup_hsm_context(TeccCryptoContextExtended *context,
                       TeccEncryptedColumnMetadata *col_meta,
                       void *tecc_state)
{
    /* Static caches for both HSM adapter types */
    static HsmAdapterInterface *tz_hsm_adapter = NULL;
    static HsmDevice *tz_hsm_device = NULL;
    static int tz_hsm_tried = 0;  /* 0=not tried, 1=success, -1=failed */
    static HsmAdapterInterface *swxa_hsm_adapter = NULL;
    static HsmDevice *swxa_hsm_device = NULL;
    static int swxa_hsm_tried = 0;

    HsmAdapterInterface **cached_adapter;
    HsmDevice **cached_device;
    int *cached_tried;
    bool is_swxa;

    if (!context)
        return 0;

    /* Need KEK info and tecc_state to look up KEK cache */
    if (!col_meta || !tecc_state)
    {
        TECC_LOG(TECC_LOG_DEBUG1, "TECC_ROUTE: no col_meta/tecc_state, using SOFTWARE encryption");
        context->hsm_kms_type = 0;
        return 0;
    }

    /* Resolve KEK name: from col_meta->kek_name directly, or via DEK cache if NULL */
    if (!col_meta->kek_name || col_meta->kek_name[0] == '\0')
    {
        /* Try to resolve KEK name from DEK cache using col_meta->dek_name */
        if (col_meta->dek_name && col_meta->dek_name[0] != '\0')
        {
            TeccDekCacheItem *dek_item = tecc_cache_get_dek_by_name_search_path(
                (const TeccState *)tecc_state, col_meta->dek_name);
            if (dek_item && dek_item->kek_oid != InvalidOid)
            {
                TeccKekCacheItem *kek_item = tecc_cache_get_kek_by_oid(
                    (const TeccState *)tecc_state, dek_item->kek_oid);
                if (kek_item && kek_item->key_name)
                {
                    col_meta->kek_name = pg_strdup(kek_item->key_name);
                }
            }
        }
        /* Still no kek_name, cannot determine HSM type */
        if (!col_meta->kek_name || col_meta->kek_name[0] == '\0')
        {
            TECC_LOG(TECC_LOG_DEBUG1, "TECC_ROUTE: no kek_name, using SOFTWARE encryption");
            context->hsm_kms_type = 0;
            return;
        }
    }

    /* Look up KEK cache to check KMS type for THIS column */
    {
        TeccKekCacheItem *kek_item = tecc_cache_get_kek_by_name_search_path(
            (const TeccState *)tecc_state, col_meta->kek_name);

        /* KEK kms_type: 1=localkms, 2=hsmkms, 3=cloudkms, 4=swxakms */
        if (!kek_item)
        {
            TECC_LOG(TECC_LOG_DEBUG1, "TECC_ROUTE: KEK '%s' not found in cache, using SOFTWARE encryption", col_meta->kek_name);
            context->hsm_kms_type = 0;
            return 0;
        }
        if (kek_item->kms_type == 2)
        {
            is_swxa = false;
        }
        else if (kek_item->kms_type == 4)
        {
            is_swxa = true;
        }
        else
        {
            TECC_LOG(TECC_LOG_DEBUG1, "TECC_ROUTE: KEK '%s' kms_type=%d, using SOFTWARE encryption", col_meta->kek_name, kek_item->kms_type);
            context->hsm_kms_type = 0;
            return 0;  /* Not an HSM KEK, use software encryption */
        }
    }

    /* Select which static cache to use */
    if (is_swxa)
    {
        cached_adapter = &swxa_hsm_adapter;
        cached_device = &swxa_hsm_device;
        cached_tried = &swxa_hsm_tried;
    }
    else
    {
        cached_adapter = &tz_hsm_adapter;
        cached_device = &tz_hsm_device;
        cached_tried = &tz_hsm_tried;
    }

    /* If we already tried and failed, don't retry */
    if (*cached_tried < 0)
    {
        TECC_LOG(TECC_LOG_ERROR, "TECC_ROUTE: %s init previously failed for KEK '%s', refusing software fallback",
                 is_swxa ? "SWXA" : "HSM", col_meta->kek_name);
        context->hsm_kms_type = 0;
        return -1;
    }

    /* If already initialized, reuse */
    if (*cached_tried > 0 && *cached_adapter != NULL && *cached_device != NULL)
    {
        context->hsm_adapter = (void *)*cached_adapter;
        context->hsm_device = (void *)*cached_device;
        context->hsm_kms_type = is_swxa ? 4 : 2;
        TECC_LOG(TECC_LOG_DEBUG1, "TECC_ROUTE: KEK '%s' kms_type=%s, reusing %s adapter, using HARDWARE encryption",
                 col_meta->kek_name, is_swxa ? "SWXA" : "HSM",
                 is_swxa ? "SWXA" : "HSM");
        return 0;
    }

    /* First time: try to initialize the appropriate HSM adapter */
    if (is_swxa)
        *cached_adapter = get_swxa_adapter();
    else
        *cached_adapter = get_tzhsm_adapter();
    if (!*cached_adapter)
    {
        *cached_tried = -1;
        TECC_LOG(TECC_LOG_ERROR, "TECC_ROUTE: Failed to get %s adapter for KEK '%s', refusing software fallback",
                 is_swxa ? "SWXA" : "HSM", col_meta->kek_name);
        context->hsm_kms_type = 0;
        return -1;
    }

    /* Tianzheng adapter uses init(NULL)/connect; SWXA uses init(id)/connect.
     * For column data encryption we pass NULL config - adapter defaults work. */
    if ((*cached_adapter)->init(cached_device, NULL) != HSM_SUCCESS)
    {
        *cached_tried = -1;
        *cached_adapter = NULL;
        TECC_LOG(TECC_LOG_ERROR, "TECC_ROUTE: %s init failed for KEK '%s', refusing software fallback",
                 is_swxa ? "SWXA" : "HSM", col_meta->kek_name);
        context->hsm_kms_type = 0;
        return -1;
    }

    if ((*cached_adapter)->connect(*cached_device) != HSM_SUCCESS)
    {
        (*cached_adapter)->cleanup(*cached_device);
        *cached_device = NULL;
        *cached_tried = -1;
        *cached_adapter = NULL;
        TECC_LOG(TECC_LOG_ERROR, "TECC_ROUTE: %s connect failed for KEK '%s', refusing software fallback",
                 is_swxa ? "SWXA" : "HSM", col_meta->kek_name);
        context->hsm_kms_type = 0;
        return -1;
    }

    *cached_tried = 1;
    context->hsm_adapter = (void *)*cached_adapter;
    context->hsm_device = (void *)*cached_device;
    context->hsm_kms_type = is_swxa ? 4 : 2;
    TECC_LOG(TECC_LOG_DEBUG1, "TECC_ROUTE: %s initialized for KEK '%s', using HARDWARE encryption",
             is_swxa ? "SWXA" : "HSM", col_meta->kek_name);
    return 0;
}

/*
 * Get crypto scheduler from connection state
 * Returns NULL if connection or scheduler is not available
 */
static TeccCryptoScheduler *
tecc_get_scheduler_from_conn(PGconn *conn)
{
    TeccState *state;

    if (conn == NULL)
    {
        TECC_SCHEDULER_DEBUG("NULL connection passed to tecc_get_scheduler_from_conn");
        return NULL;
    }

    state = tecc_get_state(conn);
    if (state == NULL)
    {
        TECC_SCHEDULER_DEBUG("Failed to get TECC state from connection");
        return NULL;
    }

    if (!state->scheduler_initialized || state->crypto_scheduler == NULL)
    {
        TECC_SCHEDULER_DEBUG("Crypto scheduler not initialized for connection");
        return NULL;
    }

    return state->crypto_scheduler;
}


/* Begin - ReqID:TECC4.0 - Decryption validation functions */
static bool tecc_validate_hex_string(const char *hex_string, size_t *valid_length);
static bool tecc_validate_cipherdata_integrity(const CipherDataType *cipherdata, size_t cipher_len);
static bool tecc_detect_tampering(const CipherDataType *cipherdata, size_t cipher_len,
                                 const unsigned char *dek_key, size_t dek_key_len);
/* End - ReqID:TECC4.0 - Decryption validation functions */

/* Begin - ReqID:TECC4.0 - Batch optimization functions */
static TeccUnifiedEncryptResult *tecc_unified_encrypt_string_with_preloaded_key(
    TeccCryptoScheduler *scheduler,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name,
    const unsigned char *preloaded_dek_key,
    size_t preloaded_dek_key_len,
    const unsigned char *preloaded_derived_enc_key,
    const unsigned char *preloaded_derived_auth_key);
/* End - ReqID:TECC4.0 - Batch optimization functions */

#if 0
/* DEPRECATED: This function uses global scheduler and is not thread-safe */
static TeccUnifiedDecryptResult *tecc_unified_decrypt_hex_internal(
    const char *hex_string,
    const char *table_name,
    const char *column_name)
{
    TeccCryptoScheduler *scheduler = g_crypto_scheduler;

    /* Validate parameters */
    if (!hex_string || !table_name || !column_name)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to unified decrypt");
        return NULL;
    }

    TECC_SCHEDULER_DEBUG("Unified decrypt: %s.%s (hex_len=%zu)",
                        table_name, column_name, strlen(hex_string));

    /* Ensure scheduler is initialized */
    if (!tecc_crypto_scheduler_ensure_initialized())
    {
        TECC_SCHEDULER_ERROR("Failed to initialize scheduler for decryption");
        return NULL;
    }

    /* Call internal implementation with scheduler */
    return tecc_unified_decrypt_hex_internal_with_scheduler(conn,
                                                           scheduler,
                                                           hex_string,
                                                           table_name,
                                                           column_name);
}
#endif

static TeccAlgorithmSupport tecc_unified_get_algorithm_support_internal_with_scheduler(
    PGconn *conn,
    TeccCryptoScheduler *scheduler,
    const char *table_name,
    const char *column_name);

static TeccUnifiedDecryptResult *tecc_unified_decrypt_hex_internal_with_scheduler(
    PGconn *conn,
    TeccCryptoScheduler *scheduler,
    const char *hex_string,
    const char *table_name,
    const char *column_name,
    TeccCryptoContext *external_context);

static TeccUnifiedEncryptResult *tecc_unified_encrypt_string_with_operator_internal_with_scheduler(
    PGconn *conn,
    TeccCryptoScheduler *scheduler,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name,
    TeccOperatorType operator_type);

static TeccUnifiedEncryptResult *tecc_unified_encrypt_string_internal_with_scheduler(
    TeccCryptoScheduler *scheduler,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name,
    Oid collation_oid,
    void *tecc_state,
    TeccEncryptedColumnMetadata *col_meta_in,
    TeccCryptoContext *external_context);

/* Thread-safe encryption function with connection parameter */
TeccUnifiedEncryptResult *tecc_unified_encrypt_string_with_conn(
    PGconn *conn,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name);

/* Stream crypto context creation with explicit scheduler */
static TeccStreamCryptoContext *tecc_create_stream_crypto_context_internal(
    TeccCryptoScheduler *scheduler,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name);

/* Note: Mock metadata functions have been removed. Real cache should be used instead. */

/* Helper function to get KEK name from column key ID - REMOVED */
/* Use tecc_convert_column_cache_item_to_metadata instead, which returns KEK/DEK names from cache */

/* Helper function to get DEK name from column key ID - REMOVED */
/* Use tecc_convert_column_cache_item_to_metadata instead, which returns KEK/DEK names from cache */

/*
 * Initialize crypto scheduler
 */
TeccCryptoScheduler *
tecc_crypto_scheduler_init(TeccMetadataCache *cache, TeccKeyManagerAdapter *key_mgr)
{
    TeccCryptoScheduler *scheduler;

    /* Allocate scheduler structure */
    scheduler = pg_malloc0(sizeof(TeccCryptoScheduler));
    if (!scheduler)
    {
        TECC_SCHEDULER_ERROR(
                           "Failed to allocate crypto scheduler");
        return NULL;
    }

    /* Set module references */
    scheduler->metadata_cache = cache;
    scheduler->key_manager = key_mgr;

    /* Default configuration */
    scheduler->max_retry_count = 3;

    /* Initialize GUC integration */
    scheduler->guc_params = NULL;
    scheduler->guc_enabled = true;  /* Enable GUC processing by default */

    /* Initialize statistics */
    scheduler->encryption_count = 0;
    scheduler->decryption_count = 0;
    scheduler->error_count = 0;

    /* Initialize batch processing statistics */
    scheduler->batch_encryption_count = 0;
    scheduler->batch_decryption_count = 0;
    scheduler->batch_error_count = 0;
    scheduler->total_batch_items_processed = 0;
    scheduler->successful_batch_items = 0;

    /* Use the provided key manager as key adapter */
    scheduler->key_adapter = key_mgr;
    /* Note: key_mgr should already be initialized by the caller */

    /* Global scheduler removed for thread safety - use connection-specific scheduler */

    /* Temporarily disable debug log to avoid segmentation fault */
    /* TECC_SCHEDULER_DEBUG("Initialized crypto scheduler"); */

    return scheduler;
}

/*
 * Cleanup crypto scheduler
 */
void
tecc_crypto_scheduler_cleanup(TeccCryptoScheduler *scheduler)
{
    if (!scheduler)
        return;

    TECC_SCHEDULER_DEBUG(
                        "Cleaning up crypto scheduler (enc=%d, dec=%d, err=%d)",
                        scheduler->encryption_count,
                        scheduler->decryption_count,
                        scheduler->error_count);

    /* Cleanup key manager adapter */
    if (scheduler->key_adapter)
    {
        tecc_key_manager_adapter_destroy(scheduler->key_adapter);
        scheduler->key_adapter = NULL;
    }

    /* Clear scheduler */
    memset(scheduler, 0, sizeof(TeccCryptoScheduler));
    pg_free(scheduler);

    /* Global scheduler reference removed for thread safety */
}

/*
 * Set debug mode
 */
void
tecc_crypto_scheduler_set_debug(TeccCryptoScheduler *scheduler, bool enable)
{
    if (!scheduler)
        return;

    scheduler->debug_mode = enable;

    TECC_SCHEDULER_DEBUG(
                        "Crypto scheduler debug mode: %s",
                        enable ? "enabled" : "disabled");
}

/*
 * Get statistics
 */
void
tecc_crypto_scheduler_get_stats(TeccCryptoScheduler *scheduler,
                               int *enc_count, int *dec_count, int *err_count)
{
    if (!scheduler)
        return;

    if (enc_count)
        *enc_count = scheduler->encryption_count;
    if (dec_count)
        *dec_count = scheduler->decryption_count;
    if (err_count)
        *err_count = scheduler->error_count;
}

/*
 * Get batch processing statistics
 */
void
tecc_crypto_scheduler_get_batch_stats(TeccCryptoScheduler *scheduler,
                                    int *batch_enc_count,
                                    int *batch_dec_count,
                                    int *batch_error_count,
                                    size_t *total_batch_items,
                                    size_t *successful_batch_items)
{
    if (!scheduler)
        return;

    if (batch_enc_count)
        *batch_enc_count = scheduler->batch_encryption_count;
    if (batch_dec_count)
        *batch_dec_count = scheduler->batch_decryption_count;
    if (batch_error_count)
        *batch_error_count = scheduler->batch_error_count;
    if (total_batch_items)
        *total_batch_items = scheduler->total_batch_items_processed;
    if (successful_batch_items)
        *successful_batch_items = scheduler->successful_batch_items;
}

/*
 * Ensure crypto scheduler is initialized (lazy initialization)
 * DEPRECATED: Use connection-specific scheduler instead
 */
bool
tecc_crypto_scheduler_ensure_initialized(void)
{
    TECC_SCHEDULER_ERROR("tecc_crypto_scheduler_ensure_initialized is deprecated, use connection-specific scheduler");
    return false;

    /* Check if already initialized */
    /* if (g_crypto_scheduler != NULL) */
    /*     return true; */

    /* Initialize formatter module first */
    /* tecc_formatter_initialize(); */

    /* Initialize with NULL cache and key manager */
    /* g_crypto_scheduler = tecc_crypto_scheduler_init(NULL, NULL); */
    /* if (!g_crypto_scheduler) */
    /* { */
    /*     TECC_SCHEDULER_ERROR("Failed to initialize crypto scheduler"); */
    /*     return false; */
    /* } */

    /* TECC_SCHEDULER_DEBUG("Lazy initialized crypto scheduler"); */
    /* return true; */
}

/*
 * Cleanup global scheduler instance
 * DEPRECATED: Use connection-specific scheduler instead
 */
void
tecc_crypto_scheduler_cleanup_global(void)
{
    TECC_SCHEDULER_ERROR("tecc_crypto_scheduler_cleanup_global is deprecated, use connection-specific scheduler");

    /* if (g_crypto_scheduler != NULL) */
    /* { */
    /*     TECC_SCHEDULER_DEBUG("Cleaning up global crypto scheduler"); */
    /*     tecc_crypto_scheduler_cleanup(g_crypto_scheduler); */
    /*     g_crypto_scheduler = NULL; */
    /* } */

    /* Cleanup formatter module */
    /* tecc_formatter_cleanup(); */
}

/*
 * Unified encryption interface
 */
TeccUnifiedEncryptResult *
tecc_unified_encrypt_string(PGconn *conn,
                           const char *plaintext,
                           const char *table_name,
                           const char *column_name,
                           const char *kek_name,
                           const char *dek_name,
                           TeccCryptoContext *external_context)
{
    TeccCryptoScheduler *scheduler;

    /* Validate parameters */
    if (conn == NULL || plaintext == NULL || table_name == NULL || column_name == NULL)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_unified_encrypt_string");
        return NULL;
    }

    /* Get scheduler from connection state */
    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return NULL;
    }

    /* Call internal implementation with scheduler */
    return tecc_unified_encrypt_string_internal_with_scheduler(scheduler,
                                                              plaintext,
                                                              table_name,
                                                              column_name,
                                                              kek_name,
                                                              dek_name,
                                                              0, /* No collation */
                                                              tecc_get_state(conn),
                                                              NULL,
                                                              external_context);
}

/*
 * ReqID:TECC4.0 - Unified encryption with explicit algorithm options.
 * For scenarios where column metadata is not in the cache (e.g. CREATE TABLE
 * DEFAULT encryption). The caller specifies algorithm support directly.
 */
TeccUnifiedEncryptResult *
tecc_unified_encrypt_string_with_options(
    PGconn *conn,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name,
    Oid data_type_oid,
    int32 type_mod,
    bool supports_det,
    bool supports_ope,
    bool supports_search,
    TeccCryptoContext *external_context)
{
    TeccCryptoScheduler *scheduler;
    TeccEncryptedColumnMetadata col_meta_local;

    if (conn == NULL || plaintext == NULL || table_name == NULL || column_name == NULL)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_unified_encrypt_string_with_options");
        return NULL;
    }

    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return NULL;
    }

    /* Build temporary metadata with caller-supplied options */
    memset(&col_meta_local, 0, sizeof(col_meta_local));
    col_meta_local.table_name = (char *)table_name;
    col_meta_local.column_name = (char *)column_name;
    col_meta_local.data_type_original_oid = data_type_oid;
    col_meta_local.data_type_original_mod = type_mod;
    col_meta_local.enable_det = supports_det;
    col_meta_local.enable_ope = supports_ope;
    col_meta_local.enable_search = supports_search;
    col_meta_local.dek_name = (char *)dek_name;
    col_meta_local.kek_name = (char *)kek_name;
    /* ReqID:TECC4.0 - Look up collation from cache so DET/OPE normalization
     * produces the same ICU sort key as the INSERT VALUES path. Without this,
     * constants in INSERT...SELECT get encrypted with collation=InvalidOid,
     * producing different DET ciphertext than the VALUES path → equality fails. */
    col_meta_local.collation_oid = InvalidOid;
    col_meta_local.collation_locale = NULL;
    {
        TeccState *state = tecc_get_state(conn);
        if (state != NULL && table_name != NULL && column_name != NULL)
        {
            const char *db_name = PQdb(conn);
            TeccColumnCacheItem *cache_item = NULL;
            if (db_name == NULL || db_name[0] == '\0')
                db_name = "postgres";
            cache_item =
                tecc_cache_get_encrypted_column_by_name(
                    state, db_name, "public", table_name, column_name);
            if (cache_item != NULL)
            {
                col_meta_local.collation_oid = cache_item->collation_oid;
                col_meta_local.collation_locale = cache_item->collation_locale;
            }
        }
    }

    return tecc_unified_encrypt_string_internal_with_scheduler(
        scheduler,
        plaintext,
        table_name,
        column_name,
        kek_name,
        dek_name,
        0, /* No collation */
        tecc_get_state(conn),
        &col_meta_local,
        external_context);
}

/*
 * Unified encryption with operator type support for SEARCH algorithm interface
 * Note: This function now requires connection parameter for thread safety
 */
TeccUnifiedEncryptResult *
tecc_unified_encrypt_string_with_operator(PGconn *conn,
                                         const char *plaintext,
                                         const char *table_name,
                                         const char *column_name,
                                         const char *kek_name,
                                         const char *dek_name,
                                         TeccOperatorType operator_type,
                                         TeccCryptoContext *external_context)
{
    TeccCryptoScheduler *scheduler;

    /* Validate parameters */
    if (conn == NULL || plaintext == NULL || table_name == NULL || column_name == NULL)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_unified_encrypt_string_with_operator");
        return NULL;
    }

    /* Get scheduler from connection state */
    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return NULL;
    }

    /* Call internal implementation with scheduler */
    return tecc_unified_encrypt_string_with_operator_internal_with_scheduler(conn,
                                                                            scheduler,
                                                                            plaintext,
                                                                            table_name,
                                                                            column_name,
                                                                            kek_name,
                                                                            dek_name,
                                                                            operator_type);
}

/*
 * Internal encryption with operator type support and scheduler parameter
 */
static TeccUnifiedEncryptResult *
tecc_unified_encrypt_string_with_operator_internal_with_scheduler(
    PGconn *conn,
    TeccCryptoScheduler *scheduler,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name,
    TeccOperatorType operator_type)
{
    TeccCryptoContextExtended *context = NULL;
    TeccUnifiedEncryptResult *result = NULL;
    TeccEncryptedColumnMetadata *col_meta = NULL;
    unsigned char *dek_key = NULL;
    size_t dek_key_len = 0;
    unsigned char *derived_enc_key = NULL;    /* ReqID:TECC4.0 */
    unsigned char *derived_auth_key = NULL;   /* ReqID:TECC4.0 */
    int ret;
    CipherDataType *cipherdata = NULL;
    size_t cipherdata_len = 0;
    TeccFormatterErrorCode fmt_ret;
    unsigned char *rnd_ciphertext;
    size_t rnd_cipher_len;
    unsigned char *search_ciphertext;
    size_t search_cipher_len;
    TeccCryptoBuffer *cipherdata_buffer;

    /* Validate parameters */
    if (plaintext == NULL || table_name == NULL || column_name == NULL)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_unified_encrypt_string_with_operator_internal_with_scheduler");
        return NULL;
    }

    /* Get metadata for the column */
    if (conn != NULL)
    {
        /* Use real cache with connection */
        TeccState *state = tecc_get_state(conn);
        TeccColumnCacheItem *cache_item = NULL;

        if (state != NULL)
        {
            /* 从连接中获取实际数据库名，替代硬编码的"postgres" */
            const char *db_name = PQdb(conn);
            if (db_name == NULL || db_name[0] == '\0')
                db_name = "postgres";
            cache_item = tecc_cache_get_encrypted_column_by_name(state, db_name, NULL, table_name, column_name);
            if (cache_item != NULL)
            {
                col_meta = tecc_convert_column_cache_item_to_metadata(state, cache_item);
            }
        }
    }

    if (col_meta == NULL)
    {
        TECC_SCHEDULER_ERROR("Column %s.%s not found in metadata cache", table_name, column_name);
        return NULL;
    }

    /* Check if SEARCH algorithm is enabled for this column */
    if (!col_meta->enable_search)
    {
        TECC_SCHEDULER_ERROR("SEARCH algorithm not enabled for column %s.%s", table_name, column_name);
        /* Free metadata if it was allocated */
        if (conn != NULL && col_meta != NULL)
        {
            tecc_free_encrypted_column_metadata(col_meta);
        }
        return NULL;
    }

    /* Allocate context */
    context = (TeccCryptoContextExtended *)pg_malloc(sizeof(TeccCryptoContextExtended));
    if (context == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate crypto context");
        /* Use appropriate free function based on source */
        if (conn != NULL)
        {
            tecc_free_encrypted_column_metadata(col_meta);
        }
        /* Note: If conn is NULL, col_meta should also be NULL since we don't use mock data anymore */
        return NULL;
    }

    /* Initialize context */
    memset(context, 0, sizeof(TeccCryptoContextExtended));
    context->core.data_type = tecc_map_to_original_type_oid(col_meta->data_type_original_oid);
    context->core.type_mod = col_meta->data_type_original_mod;
    context->core.collation_oid = col_meta->collation_oid;
    if (col_meta->collation_locale != NULL)
        strlcpy(context->core.collation_locale, col_meta->collation_locale, NAMEDATALEN);
    context->core.algo_type = TECC_ALGO_SEARCH;
    /* Initialize algo_support structure */
    context->core.algo_support.supports_rnd = false;
    context->core.algo_support.supports_det = false;
    context->core.algo_support.supports_ope = false;
    context->core.algo_support.supports_search = false;
    context->core.selected_algorithms = 0;
    context->core.buffer_manager = NULL;

    /* Set extended identification fields */
    strlcpy(context->table_name, table_name, NAMEDATALEN);
    strlcpy(context->column_name, column_name, NAMEDATALEN);
    if (kek_name != NULL)
        strlcpy(context->kek_name, kek_name, NAMEDATALEN);
    if (dek_name != NULL)
        strlcpy(context->dek_name, dek_name, NAMEDATALEN);

    /* Set input data */
    TECC_SCHEDULER_DEBUG("DEBUG: Setting plaintext: plaintext=%p, strlen=%zu",
                        plaintext, plaintext ? strlen(plaintext) : 0);
    context->plaintext = (const unsigned char *)plaintext;
    context->plaintext_len = plaintext ? strlen(plaintext) : 0;

    /* Set operator type for SEARCH algorithm */
    context->operator_type = operator_type;

    /* Check if this is a LIKE pattern */
    if (operator_type == TECC_OP_LIKE || operator_type == TECC_OP_NOT_LIKE ||
        operator_type == TECC_OP_ILIKE || operator_type == TECC_OP_NOT_ILIKE)
    {
        context->is_like_pattern = true;
        TECC_SCHEDULER_DEBUG("Processing LIKE pattern: %s", plaintext);
    }
    else
    {
        context->is_like_pattern = false;
    }

    /* Set algorithm support based on metadata */
    context->core.algo_support.supports_rnd = true;  /* RND is always supported */
    context->core.algo_support.supports_det = col_meta->enable_det;
    context->core.algo_support.supports_ope = col_meta->enable_ope;
    context->core.algo_support.supports_search = col_meta->enable_search;

    /* Set selected algorithms - SEARCH algorithm is selected for this operation */
    context->core.selected_algorithms = TECC_ALGO_RND;

    /* Get DEK key with pre-derived SM4-RND keys */
    ret = tecc_key_manager_adapter_get_dek_key_ext(scheduler->key_adapter,
                                              context->dek_name,
                                              &dek_key,
                                              &dek_key_len,
                                              &derived_enc_key,
                                              &derived_auth_key);
    if (ret != TECC_CRYPTO_SUCCESS)
    {
        TECC_SCHEDULER_ERROR("Failed to get DEK key for %s", context->dek_name);
        pg_free(context);
        /* Use appropriate free function based on source */
        if (conn != NULL)
        {
            tecc_free_encrypted_column_metadata(col_meta);
        }
        /* Note: If conn is NULL, col_meta should also be NULL since we don't use mock data anymore */
        return NULL;
    }

    context->dek_key = dek_key;
    context->dek_key_len = dek_key_len;

    /* Apply GUC parameters if enabled */
    if (scheduler->guc_enabled && scheduler->guc_params != NULL)
    {
        context->guc_params = scheduler->guc_params;
        context->guc_processing_enabled = true;

        ret = tecc_crypto_scheduler_apply_guc_to_data(scheduler, context);
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_ERROR("Failed to apply GUC parameters to data");
            tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
            /* Clear dangling pointer after key release */
            context->dek_key = NULL;
            context->dek_key_len = 0;
            pg_free(context);
            /* Use appropriate free function based on source */
        if (conn != NULL)
        {
            tecc_free_encrypted_column_metadata(col_meta);
        }
        /* Note: If conn is NULL, col_meta should also be NULL since we don't use mock data anymore */
            return NULL;
        }
    }

    /* Allocate result structure */
    result = (TeccUnifiedEncryptResult *)pg_malloc(sizeof(TeccUnifiedEncryptResult));
    if (result == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate result structure");
        tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
        /* Clear dangling pointer after key release */
        context->dek_key = NULL;
        context->dek_key_len = 0;
        pg_free(context);
        /* Use appropriate free function based on source */
        if (conn != NULL)
        {
            tecc_free_encrypted_column_metadata(col_meta);
        }
        /* Note: If conn is NULL, col_meta should also be NULL since we don't use mock data anymore */
        return NULL;
    }

    memset(result, 0, sizeof(TeccUnifiedEncryptResult));

    /* Call algorithm scheduler for SEARCH algorithm with operator type */
    ret = tecc_algorithm_scheduler_encrypt_search_with_operator(context,
                                                               operator_type,
                                                               &search_ciphertext,
                                                               &search_cipher_len);
    if (ret != TECC_CRYPTO_SUCCESS)
    {
        TECC_SCHEDULER_ERROR("SEARCH algorithm encryption failed with error %d", ret);
        strlcpy(result->error_msg, "SEARCH algorithm encryption failed", sizeof(result->error_msg));
        result->error_code = ret;
        tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
        /* Clear dangling pointer after key release */
        context->dek_key = NULL;
        context->dek_key_len = 0;
        pg_free(context);
        pg_free(result);
        /* Use appropriate free function based on source */
        if (conn != NULL)
        {
            tecc_free_encrypted_column_metadata(col_meta);
        }
        /* Note: If conn is NULL, col_meta should also be NULL since we don't use mock data anymore */
        return NULL;
    }

    /* Store SEARCH ciphertext in context */
    context->search_ciphertext = search_ciphertext;
    context->search_cipher_len = search_cipher_len;

    /* For SEARCH algorithm, we also need RND ciphertext for storage */
    ret = tecc_algorithm_scheduler_encrypt_rnd(context,
                                              &rnd_ciphertext,
                                              &rnd_cipher_len,
                                              derived_enc_key,
                                              derived_auth_key);
    if (ret != TECC_CRYPTO_SUCCESS)
    {
        TECC_SCHEDULER_ERROR("RND algorithm encryption failed with error %d", ret);
        strlcpy(result->error_msg, "RND algorithm encryption failed", sizeof(result->error_msg));
        result->error_code = ret;
        tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
        /* Clear dangling pointer after key release */
        context->dek_key = NULL;
        context->dek_key_len = 0;
        pg_free(search_ciphertext);
        pg_free(context);
        pg_free(result);
        /* Use appropriate free function based on source */
        if (conn != NULL)
        {
            tecc_free_encrypted_column_metadata(col_meta);
        }
        /* Note: If conn is NULL, col_meta should also be NULL since we don't use mock data anymore */
        return NULL;
    }

    context->rnd_ciphertext = rnd_ciphertext;
    context->rnd_cipher_len = rnd_cipher_len;

    /* Format cipherdata for transmission */
    fmt_ret = tecc_formatter_format_cipherdata(context, &cipherdata, &cipherdata_len);
    if (fmt_ret != TECC_FORMATTER_SUCCESS)
    {
        TECC_SCHEDULER_ERROR("Failed to format cipherdata with error %d", fmt_ret);
        strlcpy(result->error_msg, "Failed to format cipherdata", sizeof(result->error_msg));
        result->error_code = tecc_formatter_error_to_scheduler_error(fmt_ret);
        tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
        /* Clear dangling pointer after key release */
        context->dek_key = NULL;
        context->dek_key_len = 0;
        pg_free(search_ciphertext);
        pg_free(rnd_ciphertext);
        pg_free(context);
        pg_free(result);
        /* Use appropriate free function based on source */
        if (conn != NULL)
        {
            tecc_free_encrypted_column_metadata(col_meta);
        }
        /* Note: If conn is NULL, col_meta should also be NULL since we don't use mock data anymore */
        return NULL;
    }

    /* Create cipherdata buffer */
    cipherdata_buffer = (TeccCryptoBuffer *)pg_malloc(sizeof(TeccCryptoBuffer));
    if (cipherdata_buffer == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate cipherdata buffer");
        tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
        /* Clear dangling pointer after key release */
        context->dek_key = NULL;
        context->dek_key_len = 0;
        pg_free(search_ciphertext);
        pg_free(rnd_ciphertext);
        pg_free(cipherdata);
        pg_free(context);
        pg_free(result);
        /* Use appropriate free function based on source */
        if (conn != NULL)
        {
            tecc_free_encrypted_column_metadata(col_meta);
        }
        /* Note: If conn is NULL, col_meta should also be NULL since we don't use mock data anymore */
        return NULL;
    }

    cipherdata_buffer->data = cipherdata;
    cipherdata_buffer->size = cipherdata_len;
    cipherdata_buffer->used = cipherdata_len;
    cipherdata_buffer->owned_by_caller = true;
    cipherdata_buffer->next = NULL;

    /* Fill result structure */
    result->cipherdata_buffer = cipherdata_buffer;
    result->error_code = TECC_CRYPTO_SUCCESS;
    strlcpy(result->error_msg, "Success", sizeof(result->error_msg));

    /* Debug: print cipherdata before hex encode */
    TECC_SCHEDULER_DEBUG("cipherdata payload hex dump (%d bytes)",
                         (int)(cipherdata_len - sizeof(CipherDataType)));

    /* Convert to hex string for compatibility */
    fmt_ret = tecc_formatter_hex_encode((const unsigned char *)cipherdata, cipherdata_len, &result->hex_string);
            if (fmt_ret != TECC_FORMATTER_SUCCESS)
    {
        TECC_SCHEDULER_ERROR("Failed to convert cipherdata to hex");
        tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
        /* Clear dangling pointer after key release */
        context->dek_key = NULL;
        context->dek_key_len = 0;
        pg_free(search_ciphertext);
        pg_free(rnd_ciphertext);
        pg_free(cipherdata);
        pg_free(cipherdata_buffer);
        pg_free(context);
        pg_free(result);
        /* Use appropriate free function based on source */
        if (conn != NULL)
        {
            tecc_free_encrypted_column_metadata(col_meta);
        }
        /* Note: If conn is NULL, col_meta should also be NULL since we don't use mock data anymore */
        return NULL;
    }

    result->hex_len = strlen(result->hex_string);

    /* Update statistics */
    scheduler->encryption_count++;

    /* Cleanup */
    tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
    /* Clear dangling pointer after key release */
    context->dek_key = NULL;
    context->dek_key_len = 0;
    pg_free(search_ciphertext);
    pg_free(rnd_ciphertext);
    pg_free(context);
    /* Free metadata if it was allocated from real cache */
    if (conn != NULL && col_meta != NULL)
    {
        tecc_free_encrypted_column_metadata(col_meta);
    }
    /* Note: If conn is NULL, col_meta should also be NULL since we don't use mock data anymore */

    TECC_SCHEDULER_DEBUG("SEARCH algorithm encryption with operator type %d completed successfully", operator_type);
    return result;
}


/*
 * Unified decryption interface
 * Note: This function now includes connection parameter for thread safety
 */
TeccUnifiedDecryptResult *
tecc_unified_decrypt_hex(PGconn *conn,
                        const char *hex_string,
                        const char *table_name,
                        const char *column_name,
                        TeccCryptoContext *external_context)
{
    TeccCryptoScheduler *scheduler;

    /* Validate parameters */
    if (conn == NULL || hex_string == NULL || table_name == NULL || column_name == NULL)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_unified_decrypt_hex");
        return NULL;
    }

    /* Get scheduler from connection state */
    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return NULL;
    }

    /* Call internal implementation with scheduler */
    return tecc_unified_decrypt_hex_internal_with_scheduler(conn,
                                                           scheduler,
                                                           hex_string,
                                                           table_name,
                                                           column_name,
                                                           external_context);
}



/*
 * Get algorithm support
 * Note: This function now requires connection parameter for thread safety
 */
TeccAlgorithmSupport
tecc_unified_get_algorithm_support(PGconn *conn,
                                  const char *table_name, const char *column_name)
{
    TeccCryptoScheduler *scheduler;
    TeccAlgorithmSupport support = {0};

    /* Validate parameters */
    if (conn == NULL || !table_name || !column_name)
    {
        return support;
    }

    /* Get scheduler from connection state */
    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_WARNING("Failed to get scheduler for algorithm support");
        /* Return default support */
        support.supports_rnd = true;
        support.supports_det = true;
        support.supports_ope = true;
        support.supports_search = false;
        return support;
    }

    /* Call internal implementation with scheduler */
    return tecc_unified_get_algorithm_support_internal_with_scheduler(conn,
                                                                     scheduler,
                                                                     table_name,
                                                                     column_name);
}

/*
 * Internal algorithm support function with scheduler parameter
 */
static TeccAlgorithmSupport tecc_unified_get_algorithm_support_internal_with_scheduler(
    PGconn *conn,
    TeccCryptoScheduler *scheduler,
    const char *table_name,
    const char *column_name)
{
    TeccAlgorithmSupport support = {0};
    TeccEncryptedColumnMetadata *col_meta = NULL;

    if (!table_name || !column_name)
        return support;

    TECC_SCHEDULER_DEBUG(
                        "Getting algorithm support for %s.%s with scheduler",
                        table_name, column_name);

    /* Get from cache */
    if (conn != NULL)
    {
        /* Use real cache with connection */
        TeccState *state = tecc_get_state(conn);
        TeccColumnCacheItem *cache_item = NULL;

        if (state != NULL)
        {
            const char *db_name = PQdb(conn);
            if (db_name == NULL || db_name[0] == '\0')
                db_name = "postgres";
            cache_item = tecc_cache_get_encrypted_column_by_name(state, db_name, NULL, table_name, column_name);
            if (cache_item != NULL)
            {
                col_meta = tecc_convert_column_cache_item_to_metadata(state, cache_item);
            }
        }
    }

    if (col_meta)
    {
        support.supports_rnd = true;  /* Always supported */
        support.supports_det = col_meta->enable_det;
        support.supports_ope = col_meta->enable_ope;
        support.supports_search = col_meta->enable_search;

        /* Free metadata - only if it came from real cache (conn != NULL) */
        if (conn != NULL)
        {
            tecc_free_encrypted_column_metadata(col_meta);
        }
    }
    else
    {
        /* If not in cache, return default support */
        support.supports_rnd = true;
        support.supports_det = true;
        support.supports_ope = true;
        support.supports_search = false;  /* Not implemented yet */
    }

    /* Note: col_meta is already freed in the if block above */

    return support;
}

/*
 * Free encrypt result
 */
void
tecc_unified_free_encrypt_result(TeccUnifiedEncryptResult *result)
{
    if (!result)
        return;

    /* Free cipherdata buffer if present */
    if (result->cipherdata_buffer)
    {
        tecc_processor_release_buffer(result->cipherdata_buffer);
        result->cipherdata_buffer = NULL;
    }

    if (result->hex_string)
    {
        /* Securely wipe sensitive data */
        memset(result->hex_string, 0, result->hex_len);
        pg_free(result->hex_string);
        result->hex_string = NULL;
    }

    memset(result, 0, sizeof(TeccUnifiedEncryptResult));
    pg_free(result);
}

/*
 * Free decrypt result
 */
void
tecc_unified_free_decrypt_result(TeccUnifiedDecryptResult *result)
{
    if (!result)
        return;

    if (result->plaintext)
    {
        /* Securely wipe sensitive data */
        memset(result->plaintext, 0, result->plain_len);
        pg_free(result->plaintext);
    }

    memset(result, 0, sizeof(TeccUnifiedDecryptResult));
    pg_free(result);
}

/*
 * Data validation interface
 *
 * Validates input data for encryption/decryption operations.
 * This function provides direct access to data validation logic
 * without requiring full normalization.
 */
int
tecc_crypto_scheduler_validate_input_data(
    const unsigned char *data,
    size_t data_len,
    Oid data_type,
    TeccAlgorithmType algo_type,
    char *error_msg,
    size_t error_msg_size)
{
    TeccFormatterErrorCode fmt_result;

    /* Parameter validation */
    if (!data || data_len == 0) {
        if (error_msg && error_msg_size > 0) {
            snprintf(error_msg, error_msg_size, "Invalid input data: null pointer or zero length");
        }
        return TECC_CRYPTO_INVALID_PARAM;
    }

    if (!error_msg || error_msg_size == 0) {
        /* Caller must provide error message buffer */
        return TECC_CRYPTO_INVALID_PARAM;
    }

    /* Call formatter data validation function */
    fmt_result = tecc_validate_input_data(data, data_len, data_type, algo_type);

    if (fmt_result == TECC_FORMATTER_SUCCESS) {
        snprintf(error_msg, error_msg_size, "Data validation successful");
        return TECC_CRYPTO_SUCCESS;
    }

    /* Convert formatter error to scheduler error */
    switch (fmt_result) {
        /* Parameter errors (maps to TECC_CRYPTO_INVALID_PARAM) */
        case TECC_FORMATTER_ERROR_INVALID_PARAM:
            snprintf(error_msg, error_msg_size, "Invalid parameters for data validation");
            return TECC_CRYPTO_INVALID_PARAM;

        case TECC_FORMATTER_ERROR_NULL_POINTER:
            snprintf(error_msg, error_msg_size, "Null pointer parameter");
            return TECC_CRYPTO_INVALID_PARAM;

        case TECC_FORMATTER_ERROR_INVALID_TYPE:
            snprintf(error_msg, error_msg_size, "Invalid data type: %u", data_type);
            return TECC_CRYPTO_INVALID_PARAM;

        case TECC_FORMATTER_ERROR_INVALID_INPUT:
            snprintf(error_msg, error_msg_size, "Invalid input data for type %u", data_type);
            return TECC_CRYPTO_DATA_ERROR;

        case TECC_FORMATTER_ERROR_UNSUPPORTED_TYPE:
            snprintf(error_msg, error_msg_size, "Unsupported data type: %u", data_type);
            return TECC_CRYPTO_INVALID_PARAM;

        case TECC_FORMATTER_ERROR_DATA_TOO_LARGE:
            snprintf(error_msg, error_msg_size, "Data too large: %zu bytes", data_len);
            return TECC_CRYPTO_DATA_ERROR;

        /* Memory errors (maps to TECC_CRYPTO_MEMORY_ERROR) */
        case TECC_FORMATTER_ERROR_MEMORY_ALLOC:
            snprintf(error_msg, error_msg_size, "Memory allocation error during validation");
            return TECC_CRYPTO_MEMORY_ERROR;

        case TECC_FORMATTER_ERROR_BUFFER_TOO_SMALL:
            snprintf(error_msg, error_msg_size, "Buffer too small for data validation");
            return TECC_CRYPTO_MEMORY_ERROR;

        /* Data errors (maps to TECC_CRYPTO_DATA_ERROR) */
        case TECC_FORMATTER_ERROR_DATA_FORMAT:
            snprintf(error_msg, error_msg_size, "Invalid data format for type %u", data_type);
            return TECC_CRYPTO_DATA_ERROR;

        case TECC_FORMATTER_ERROR_ENCODING_FAILED:
            snprintf(error_msg, error_msg_size, "Encoding failed for data validation");
            return TECC_CRYPTO_DATA_ERROR;

        case TECC_FORMATTER_ERROR_DECODING_FAILED:
            snprintf(error_msg, error_msg_size, "Decoding failed for data validation");
            return TECC_CRYPTO_DATA_ERROR;

        case TECC_FORMATTER_ERROR_CONVERSION_FAILED:
            snprintf(error_msg, error_msg_size, "Data conversion failed for type %u", data_type);
            return TECC_CRYPTO_DATA_ERROR;

        case TECC_FORMATTER_ERROR_INVALID_NUMERIC:
            snprintf(error_msg, error_msg_size, "Invalid numeric data for type %u", data_type);
            return TECC_CRYPTO_DATA_ERROR;

        case TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW:
            snprintf(error_msg, error_msg_size, "Numeric overflow for type %u", data_type);
            return TECC_CRYPTO_DATA_ERROR;

        /* Format errors (maps to TECC_CRYPTO_FORMAT_ERROR) */
        case TECC_FORMATTER_ERROR_INVALID_HEADER:
            snprintf(error_msg, error_msg_size, "Invalid data header");
            return TECC_CRYPTO_FORMAT_ERROR;

        case TECC_FORMATTER_ERROR_VERSION_MISMATCH:
            snprintf(error_msg, error_msg_size, "Version mismatch in data");
            return TECC_CRYPTO_FORMAT_ERROR;

        case TECC_FORMATTER_ERROR_BUILD_FAILED:
            snprintf(error_msg, error_msg_size, "Data structure build failed");
            return TECC_CRYPTO_FORMAT_ERROR;

        /* ICU collation errors (maps to TECC_CRYPTO_COLLATION_ERROR) */
        case TECC_FORMATTER_ERROR_ICU_NOT_AVAILABLE:
            snprintf(error_msg, error_msg_size, "ICU collation not available");
            return TECC_CRYPTO_COLLATION_ERROR;

        case TECC_FORMATTER_ERROR_COLLATION_INVALID:
            snprintf(error_msg, error_msg_size, "Invalid collation for data type %u", data_type);
            return TECC_CRYPTO_COLLATION_ERROR;

        case TECC_FORMATTER_ERROR_SORTKEY_FAILED:
            snprintf(error_msg, error_msg_size, "Sort key generation failed");
            return TECC_CRYPTO_COLLATION_ERROR;

        /* System errors (maps to TECC_CRYPTO_SYSTEM_ERROR) */
        case TECC_FORMATTER_ERROR_SYSTEM:
            snprintf(error_msg, error_msg_size, "System error during data validation");
            return TECC_CRYPTO_SYSTEM_ERROR;

        /* Success case (should not reach here) */
        case TECC_FORMATTER_SUCCESS:
        default:
            snprintf(error_msg, error_msg_size, "Data validation failed with error code: %d", fmt_result);
            return TECC_CRYPTO_DATA_ERROR;
    }
}

/*
 * Self test
 */
int
tecc_crypto_scheduler_self_test(void)
{
    TeccUnifiedEncryptResult *enc_result;
    TeccUnifiedDecryptResult *dec_result;
    const char *test_plain = "test_encryption_data";
    const char *test_table = "test_table";
    const char *test_column = "test_column";

    TECC_SCHEDULER_DEBUG(
                        "Running crypto scheduler self test");

    /* Test encryption */
    /* Note: Self test uses NULL connection - this is for testing only */
    enc_result = tecc_unified_encrypt_string(NULL, test_plain, test_table, test_column, NULL, NULL, NULL);
    if (!enc_result)
    {
        TECC_SCHEDULER_ERROR(
                           "Self test: encryption failed");
        return -1;
    }

    if (enc_result->error_code != TECC_CRYPTO_SUCCESS)
    {
        TECC_SCHEDULER_ERROR(
                           "Self test: encryption returned error: %s",
                           enc_result->error_msg);
        tecc_unified_free_encrypt_result(enc_result);
        return -1;
    }

    /* Test decryption */
    /* Note: Self test uses NULL connection - this is for testing only */
    dec_result = tecc_unified_decrypt_hex(NULL, enc_result->hex_string, test_table, test_column, NULL);
    if (!dec_result)
    {
        TECC_SCHEDULER_ERROR(
                           "Self test: decryption failed");
        tecc_unified_free_encrypt_result(enc_result);
        return -1;
    }

    if (dec_result->error_code != TECC_CRYPTO_SUCCESS)
    {
        TECC_SCHEDULER_ERROR(
                           "Self test: decryption returned error: %s",
                           dec_result->error_msg);
        tecc_unified_free_encrypt_result(enc_result);
        tecc_unified_free_decrypt_result(dec_result);
        return -1;
    }

    /* Verify round-trip */
    if (strlen(test_plain) != dec_result->plain_len ||
        memcmp(test_plain, dec_result->plaintext, dec_result->plain_len) != 0)
    {
        TECC_SCHEDULER_ERROR(
                           "Self test: round-trip verification failed");
        tecc_unified_free_encrypt_result(enc_result);
        tecc_unified_free_decrypt_result(dec_result);
        return -1;
    }

    TECC_SCHEDULER_DEBUG(
                        "Self test passed successfully");

    tecc_unified_free_encrypt_result(enc_result);
    tecc_unified_free_decrypt_result(dec_result);

    return 0;
}

/* ==================== GUC Parameter Integration ==================== */

/*
 * Set GUC parameters for scheduler
 */
void
tecc_crypto_scheduler_set_guc_params(TeccCryptoScheduler *scheduler,
                                    const TeccGucParams *guc_params)
{
    if (!scheduler)
    {
        TECC_SCHEDULER_ERROR("Cannot set GUC parameters: scheduler is NULL");
        return;
    }

    scheduler->guc_params = guc_params;
    if (guc_params)
    {
        TECC_SCHEDULER_DEBUG("GUC parameters set for scheduler");
    }
    else
    {
        TECC_SCHEDULER_DEBUG("GUC parameters cleared for scheduler");
    }
}

/*
 * Get current GUC parameters
 */
const TeccGucParams *
tecc_crypto_scheduler_get_guc_params(const TeccCryptoScheduler *scheduler)
{
    if (!scheduler)
    {
        TECC_SCHEDULER_ERROR("Cannot get GUC parameters: scheduler is NULL");
        return NULL;
    }

    return scheduler->guc_params;
}

/*
 * Enable/disable GUC processing
 */
void
tecc_crypto_scheduler_set_guc_enabled(TeccCryptoScheduler *scheduler,
                                     bool enabled)
{
    if (!scheduler)
    {
        TECC_SCHEDULER_ERROR("Cannot set GUC enabled: scheduler is NULL");
        return;
    }

    scheduler->guc_enabled = enabled;
    TECC_SCHEDULER_DEBUG("GUC processing %s", enabled ? "enabled" : "disabled");
}

/*
 * Check if GUC processing is enabled
 */
bool
tecc_crypto_scheduler_is_guc_enabled(const TeccCryptoScheduler *scheduler)
{
    if (!scheduler)
    {
        TECC_SCHEDULER_ERROR("Cannot check GUC enabled: scheduler is NULL");
        return false;
    }

    return scheduler->guc_enabled;
}


/*
 * Apply GUC parameters to data before encryption
 */
int
tecc_crypto_scheduler_apply_guc_to_data(TeccCryptoScheduler *scheduler,
                                       TeccCryptoContext *context)
{
    if (!scheduler || !context)
    {
        TECC_SCHEDULER_ERROR("Cannot apply GUC to data: invalid parameters");
        return TECC_CRYPTO_INVALID_PARAM;
    }

    /* Check if GUC processing is enabled */
    if (!scheduler->guc_enabled)
    {
        TECC_SCHEDULER_DEBUG("GUC processing disabled, skipping GUC application");
        return TECC_CRYPTO_SUCCESS;
    }

    /* Check if we have GUC parameters */
    if (!scheduler->guc_params)
    {
        TECC_SCHEDULER_WARNING("No GUC parameters available, skipping GUC application");
        return TECC_CRYPTO_SUCCESS;
    }

    /* Apply GUC parameters based on data type */
    if (context->core.data_type == VARCHAROID ||
        context->core.data_type == BPCHAROID ||
        context->core.data_type == TEXTOID)
    {
        /* Text data - note: client_encoding parameter has been removed */
        TECC_SCHEDULER_DEBUG("Applying GUC to text data");

        /* Note: client_encoding parameter has been removed from TeccGucParams structure */
        /* Character encoding conversion is not part of TECC GUC parameter processing */

        /* Check if collation is specified */
        if (context->core.collation_oid != 0)
        {
            TECC_SCHEDULER_DEBUG("Text data has collation OID %u", context->core.collation_oid);
        }
    }
    else if (context->core.data_type == NUMERICOID ||
             context->core.data_type == INT2OID ||
             context->core.data_type == INT4OID ||
             context->core.data_type == INT8OID ||
             context->core.data_type == FLOAT4OID ||
             context->core.data_type == FLOAT8OID)
    {
        /* Numeric data - note: lc_numeric parameter has been removed */
        TECC_SCHEDULER_DEBUG("Applying GUC to numeric data");

        /* Note: lc_numeric parameter has been removed from TeccGucParams structure */
        /* Numeric locale formatting is not part of TECC GUC parameter processing */
    }
    else
    {
        /* Other data types - minimal GUC processing */
        TECC_SCHEDULER_DEBUG("Applying GUC to data type OID %u", context->core.data_type);
    }

    /* Apply standard_conforming_strings if applicable */
    if (!scheduler->guc_params->standard_conforming_strings)
    {
        TECC_SCHEDULER_DEBUG("standard_conforming_strings is false - applying escape processing");
        /* TODO: Implement escape sequence processing for non-standard strings */
    }

    /* Update statistics */
    scheduler->encryption_count++;  /* This will be incremented later in encryption, but track GUC applications */
    TECC_SCHEDULER_DEBUG("GUC parameters applied to data successfully");
    return TECC_CRYPTO_SUCCESS;
}

/* ==================== Thread-Safe Functions with Connection Parameter ==================== */

/*
 * Internal encryption implementation with explicit scheduler parameter
 */
static TeccUnifiedEncryptResult *
tecc_unified_encrypt_string_internal_with_scheduler(
    TeccCryptoScheduler *scheduler,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name,
    Oid collation_oid,
    void *tecc_state,
    TeccEncryptedColumnMetadata *col_meta_in,
    TeccCryptoContext *external_context)
{
    TeccCryptoContextExtended *context = NULL;
    bool context_is_external = (external_context != NULL);
    TeccUnifiedEncryptResult *result = NULL;
    TeccEncryptedColumnMetadata *col_meta = NULL;
    unsigned char *dek_key = NULL;
    size_t dek_key_len = 0;
    unsigned char *derived_enc_key = NULL;    /* ReqID:TECC4.0 */
    unsigned char *derived_auth_key = NULL;   /* ReqID:TECC4.0 */
    int ret;
    CipherDataType *cipherdata = NULL;
    size_t cipherdata_len = 0;
    TeccFormatterErrorCode fmt_ret;
    TeccCryptoBuffer *cipherdata_buffer;
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    const char *dek_name_to_use;

    /* Validate parameters */
    if (!scheduler || !plaintext || !table_name || !column_name)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to unified encrypt with scheduler");
        return NULL;
    }

    TECC_SCHEDULER_DEBUG("Unified encrypt with scheduler: %s.%s (len=%zu), plaintext=%p",
                        table_name, column_name, strlen(plaintext), plaintext);

    /* Debug: Check scheduler and key adapter */
    TECC_SCHEDULER_DEBUG("DEBUG: scheduler=%p, key_adapter=%p, dek_name=%s",
                        scheduler, scheduler ? scheduler->key_adapter : NULL,
                        dek_name ? dek_name : "NULL");

    /* Create or reuse encryption context */
    if (context_is_external)
    {
        context = external_context;
        tecc_crypto_context_reset(context);
        strlcpy(context->table_name, table_name, NAMEDATALEN);
        strlcpy(context->column_name, column_name, NAMEDATALEN);
        if (kek_name)
            strlcpy(context->kek_name, kek_name, NAMEDATALEN);
        if (dek_name)
            strlcpy(context->dek_name, dek_name, NAMEDATALEN);
    }
    else
    {
        context = tecc_create_crypto_context_with_collation(table_name, column_name,
                                                           kek_name, dek_name,
                                                           collation_oid);
        if (!context)
        {
            TECC_SCHEDULER_ERROR("Failed to create crypto context");
            return NULL;
        }
    }

    /* Debug: Check context creation */
    TECC_SCHEDULER_DEBUG("DEBUG: Created context=%p", context);

    /* Set plaintext data in context */
    context->plaintext = (const unsigned char *)plaintext;
    context->plaintext_len = plaintext ? strlen(plaintext) : 0;

    TECC_SCHEDULER_DEBUG("DEBUG: Set plaintext in context: plaintext=%p, len=%zu",
                        context->plaintext, context->plaintext_len);

    /* Get column metadata: use caller-supplied metadata if available,
     * otherwise look up from cache. No fallback — metadata is required. */
    if (col_meta_in != NULL)
    {
        col_meta = col_meta_in;
        TECC_SCHEDULER_DEBUG("DEBUG: Using caller-supplied column metadata: col_meta=%p", col_meta);
    }
    else
    {
        TECC_SCHEDULER_DEBUG("DEBUG: Checking metadata cache: scheduler->metadata_cache=%p", scheduler->metadata_cache);

        /* Try to get metadata from real cache if tecc_state is available */
        if (tecc_state != NULL)
        {
            TeccState *state = (TeccState *)tecc_state;
            TeccColumnCacheItem *cache_item = NULL;

            /* 从连接中获取实际数据库名，替代硬编码的"postgres" */
            {
                const char *db_name = (state->conn != NULL) ? PQdb(state->conn) : NULL;
                if (db_name == NULL || db_name[0] == '\0')
                    db_name = "postgres";
                cache_item = tecc_cache_get_encrypted_column_by_name(state, db_name, NULL, table_name, column_name);
            }
            if (cache_item != NULL)
            {
                col_meta = tecc_convert_column_cache_item_to_metadata(state, cache_item);
                TECC_SCHEDULER_DEBUG("DEBUG: Got column metadata from real cache: col_meta=%p", col_meta);
            }
        }

        if (col_meta == NULL)
        {
            TECC_SCHEDULER_ERROR("Column %s.%s not found in metadata cache and no metadata provided",
                                table_name, column_name);
            if (!context_is_external) pg_free(context);
            return NULL;
        }
    }

    /* Update algorithm support from metadata if available */
    if (col_meta)
    {
        /* Update algorithm support from metadata */
        context->core.algo_support.supports_det = col_meta->enable_det;
        context->core.algo_support.supports_ope = col_meta->enable_ope;
        context->core.algo_support.supports_search = col_meta->enable_search;
        context->core.data_type = tecc_map_to_original_type_oid(col_meta->data_type_original_oid);
        context->core.type_mod = col_meta->data_type_original_mod;
        context->core.collation_oid = col_meta->collation_oid;

                if (col_meta->collation_locale != NULL)
            strlcpy(context->core.collation_locale, col_meta->collation_locale, NAMEDATALEN);

        /* Use metadata KEK/DEK if not specified */
        if (!kek_name && col_meta->kek_name != NULL && strlen(col_meta->kek_name) > 0)
            strlcpy(context->kek_name, col_meta->kek_name, NAMEDATALEN);
        if (!dek_name && col_meta->dek_name != NULL && strlen(col_meta->dek_name) > 0)
            strlcpy(context->dek_name, col_meta->dek_name, NAMEDATALEN);

        /*
         * ReqID:TECC4.0 - Prepare RND plaintext.
         *
         * Numeric types (INT2/4/8, FLOAT4/8, NUMERIC):
         *   Normalize once via tecc_formatter_normalize_data to produce the
         *   shared normalized binary format (sign+weight+reserved+digits).
         *   RND, DET, and OPE all encrypt the same normalized binary for
         *   cross-type ciphertext compatibility.
         *
         * Text types (VARCHAR, BPCHAR, TEXT):
         *   Use tecc_formatter_prepare_plaintext to add 4-byte length prefix
         *   for RND. DET/SEARCH normalization is applied separately below.
         */
        {
            bool is_numeric_type = (context->core.data_type == INT2OID ||
                                    context->core.data_type == INT4OID ||
                                    context->core.data_type == INT8OID ||
                                    context->core.data_type == FLOAT4OID ||
                                    context->core.data_type == FLOAT8OID ||
                                    context->core.data_type == NUMERICOID);

            if (is_numeric_type)
            {
                /* ReqID:TECC4.0 - RND normalize (no OPE padding).  DET and
                 * OPE each call normalize_data_for_algorithm independently
                 * in their tecc_execute_algo_* paths. */
                TeccFormatterErrorCode norm_ret;
                size_t req_size;
                TeccCryptoContextCore norm_core;

                memset(&norm_core, 0, sizeof(norm_core));
                norm_core.data_type = context->core.data_type;
                norm_core.type_mod = context->core.type_mod;
                norm_core.collation_oid = context->core.collation_oid;
                norm_core.algo_support = context->core.algo_support;
                norm_core.algo_support.supports_ope = false;
                norm_core.selected_algorithms = context->core.selected_algorithms;
                if (col_meta->collation_locale != NULL)
                    strlcpy(norm_core.collation_locale, col_meta->collation_locale, NAMEDATALEN);
                norm_core.algo_type = TECC_ALGO_DET;

                req_size = tecc_formatter_calculate_normalized_size(&norm_core,
                                                                     strlen(plaintext));
                if (req_size == 0)
                {
                    TECC_SCHEDULER_ERROR("Failed to calculate normalized size");
                    if (tecc_state != NULL)
                    {
                        TeccState *err_state;
                        err_state = (TeccState *)tecc_state;
                        tecc_record_error(err_state, TECC_ERR_ENCRYPT,
                                        "Failed to calculate normalized size",
                                        __func__, __LINE__, false);
                    }
                    tecc_free_encrypted_column_metadata(col_meta);
                    if (!context_is_external) pg_free(context);
                    return NULL;
                }

                context->normalized_plaintext = pg_malloc(req_size + 1);
                if (context->normalized_plaintext == NULL)
                {
                    TECC_SCHEDULER_ERROR("Failed to allocate normalized buffer");
                    tecc_free_encrypted_column_metadata(col_meta);
                    if (!context_is_external) pg_free(context);
                    return NULL;
                }

                {
                    TeccCryptoBuffer norm_buf;
                    size_t plaintext_len;
                    plaintext_len = strlen(plaintext);
                    if (plaintext_len > req_size)
                        plaintext_len = req_size;
                    norm_buf.data = context->normalized_plaintext;
                    norm_buf.size = req_size;
                    norm_buf.used = plaintext_len;
                    norm_buf.owned_by_caller = true;
                    memcpy(norm_buf.data, plaintext, plaintext_len);
                    ((unsigned char *)norm_buf.data)[plaintext_len] = '\0';
                    norm_ret = tecc_formatter_normalize_data(&norm_core,
                                                             norm_buf.data,
                                                             norm_buf.used,
                                                             &norm_buf);
                    if (norm_ret != TECC_FORMATTER_SUCCESS)
                    {
                        {
                            TeccState *err_state;
                            const char *err_desc;
                            err_state = (tecc_state != NULL) ? (TeccState *)tecc_state : NULL;
                            switch (norm_ret)
                            {
                                case TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW:
                                    err_desc = "numeric field overflow"; break;
                                case TECC_FORMATTER_ERROR_INVALID_INPUT:
                                    err_desc = "invalid input syntax for type"; break;
                                default:
                                    err_desc = "data normalization error"; break;
                            }
                            TECC_SCHEDULER_ERROR("RND normalize failed: %s (code=%d)", err_desc, (int)norm_ret);
                            if (err_state != NULL)
                                tecc_record_error(err_state, TECC_ERR_ENCRYPT,
                                                err_desc, __func__, __LINE__, false);
                        }
                        pg_free(context->normalized_plaintext);
                        context->normalized_plaintext = NULL;
                        tecc_free_encrypted_column_metadata(col_meta);
                        if (!context_is_external) pg_free(context);
                        return NULL;
                    }
                    context->normalized_plaintext_len = norm_buf.used;
                }
                /* RND reads from normalized_plaintext; plaintext is
                 * kept as original string for DET/OPE normalize. */
            }
            else
            {
                /* Text types: use prepare_plaintext for RND format */
                TeccFormatterErrorCode prep_ret;
                unsigned char *rnd_binary = NULL;
                size_t rnd_binary_len = 0;

                prep_ret = tecc_formatter_prepare_plaintext(
                    context->core.data_type, context->core.type_mod,
                    plaintext, strlen(plaintext),
                    &rnd_binary, &rnd_binary_len);

                if (prep_ret != TECC_FORMATTER_SUCCESS)
                {
                    TECC_SCHEDULER_ERROR("Failed to prepare RND plaintext (code=%d)",
                                        (int)prep_ret);
                    /* Record error for propagation to user */
                    if (tecc_state != NULL)
                    {
                        TeccState *err_state;
                        const char *err_desc;
                        err_state = (TeccState *)tecc_state;
                        switch (prep_ret)
                        {
                            case TECC_FORMATTER_ERROR_DATA_TOO_LARGE:
                                err_desc = "input value too large for encryption";
                                break;
                            default:
                                err_desc = "input data format error";
                                break;
                        }
                        tecc_record_error(err_state, TECC_ERR_ENCRYPT,
                                        err_desc, __func__, __LINE__, false);
                    }
                    tecc_free_encrypted_column_metadata(col_meta);
                    if (!context_is_external) pg_free(context);
                    return NULL;
                }

                context->plaintext = rnd_binary;
                context->plaintext_len = rnd_binary_len;
            }
        }
    }

    /* Allocate result structure */
    result = pg_malloc0(sizeof(TeccUnifiedEncryptResult));
    if (!result)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate encrypt result");
        if (!context_is_external) pg_free(context);
        return NULL;
    }

    /* Initialize result */
    result->error_code = TECC_CRYPTO_SUCCESS;
    result->error_msg[0] = '\0';

    /* Get DEK key from key manager */
    TECC_SCHEDULER_DEBUG("DEBUG: Before getting DEK key: dek_name=%s, context->dek_name=%s",
                        dek_name ? dek_name : "NULL",
                        context->dek_name);

    /* Use context->dek_name if dek_name is NULL */
    /* ReqID:TECC4.0 - dek_name_to_use declared at function scope beginning */

    dek_name_to_use = dek_name ? dek_name : context->dek_name;

    ret = tecc_key_manager_adapter_get_dek_key_ext(scheduler->key_adapter,
                                              dek_name_to_use,
                                              &dek_key, &dek_key_len,
                                              &derived_enc_key, &derived_auth_key);
    if (ret != TECC_CRYPTO_SUCCESS)
    {
        TECC_SCHEDULER_ERROR("Failed to get DEK key for encryption");
        result->error_code = ret;
        /* ReqID:TECC4.0 - Include specific key error message from key layer */
        {
            const char *key_err = tecc_key_get_last_error();
            if (key_err != NULL && key_err[0] != '\0')
            {
                snprintf(result->error_msg, sizeof(result->error_msg),
                         "Cannot encrypt: %s", key_err);
            }
            else
            {
                snprintf(result->error_msg, sizeof(result->error_msg),
                         "Failed to get DEK key '%s'", dek_name_to_use);
            }
        }
        if (!context_is_external) pg_free(context);
        /* ReqID:TECC4.0 - Return result with error instead of NULL,
         * so caller can propagate the specific error message */
        return result;
    }

    /* Apply GUC parameters if enabled */
    if (scheduler->guc_enabled && scheduler->guc_params)
    {
        ret = tecc_crypto_scheduler_apply_guc_to_data(scheduler, (TeccCryptoContext *)context);
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_ERROR("Failed to apply GUC parameters to data");
            result->error_code = ret;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "Failed to apply GUC parameters");
            tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
            /* Clear dangling pointer after key release */
            context->dek_key = NULL;
            context->dek_key_len = 0;
            if (!context_is_external) pg_free(context);
            pg_free(result);
            return NULL;
        }
    }

    /* Algorithm support already set from metadata above */

    /* ReqID:TECC4.0 - 修复HSM路由：在DEK/KEK加载完成后补填col_meta->kek_name。
     * col_meta在DEK/KEK被缓存之前就已创建，导致kek_name为NULL，
     * 进而tecc_setup_hsm_context无法检测到HSM KEK，使硬加密错误地走了软件加密。 */
    if (col_meta && (col_meta->kek_name == NULL || col_meta->kek_name[0] == '\0') &&
        tecc_state != NULL && dek_name_to_use != NULL)
    {
        TeccDekCacheItem *dek_item = tecc_cache_get_dek_by_name_search_path(
            (const TeccState *)tecc_state, dek_name_to_use);
        if (dek_item != NULL && dek_item->kek_name != NULL && dek_item->kek_name[0] != '\0')
        {
            col_meta->kek_name = pg_strdup(dek_item->kek_name);
            strlcpy(context->kek_name, dek_item->kek_name, NAMEDATALEN);
        }
    }

    /* ReqID:TECC4.0 - Multi-algorithm INSERT encryption.
     * Always encrypt with RND, plus all declared optional algorithms (DET/OPE/SEARCH).
     * For numeric types, normalization is done once and shared across RND/DET/OPE
     * for cross-type ciphertext compatibility. All ciphertexts are packed into
     * a single CipherDataType. */

    /* RND: always required for INSERT */
    context->dek_key = dek_key;
    context->dek_key_len = dek_key_len;

    /* Set up HSM SM4 encryption based on KEK's KMS type.
     * If HSM is configured (kms_type=2/4) but init failed, abort — no software fallback. */
    if (tecc_setup_hsm_context(context, col_meta, tecc_state) < 0)
    {
        TECC_SCHEDULER_ERROR("HSM init failed for KEK, refusing software fallback");
        result->error_code = TECC_CRYPTO_KEY_ERROR;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "HSM hardware required but initialization failed");
        context->dek_key = NULL;
        context->dek_key_len = 0;
        if (!context_is_external) pg_free(context);
        pg_free(result);
        return NULL;
    }

    ret = tecc_execute_algo_rnd((TeccCryptoContextExtended *)context,
                                derived_enc_key, derived_auth_key);
    if (ret != TECC_CRYPTO_SUCCESS)
    {
        TECC_SCHEDULER_ERROR("RND algorithm encryption failed");
        result->error_code = ret;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "RND algorithm encryption failed");
        context->dek_key = NULL;
        context->dek_key_len = 0;
        if (!context_is_external) pg_free(context);
        pg_free(result);
        return NULL;
    }

    /* ReqID:TECC4.0 - Post-RND cleanup and DET/OPE normalization.
     *
     * For numeric types: normalization was already done before RND.
     *   plaintext == normalized_plaintext (shared), so DO NOT free it here.
     *   normalized_plaintext is already set and valid.
     *
     * For text types: free the RND binary, restore original text pointer,
     *   then normalize for DET/SEARCH as before. */
    {
        bool is_numeric_type = (context->core.data_type == INT2OID ||
                                context->core.data_type == INT4OID ||
                                context->core.data_type == INT8OID ||
                                context->core.data_type == FLOAT4OID ||
                                context->core.data_type == FLOAT8OID ||
                                context->core.data_type == NUMERICOID);

        if (is_numeric_type)
        {
            /* ReqID:TECC4.0 - Restore original raw string so DET/OPE
             * normalize_data_for_algorithm gets the text, not binary. */
            context->plaintext = (const unsigned char *)plaintext;
            context->plaintext_len = strlen(plaintext);
        }
        else
        {
            /* Text types: free RND binary, restore original text */
            if (context->plaintext)
            {
                pg_free((void *)context->plaintext);
                context->plaintext = (const unsigned char *)plaintext;
                context->plaintext_len = strlen(plaintext);
            }

            /* ReqID:TECC4.0 - BPCHAR blank-padding: pad to declared length
             * so trailing spaces are consistently stored before rtrim.
             * PG backend pads bpchar(N) values to N bytes with spaces,
             * and the frontend must match this before encryption. */
            if (context->core.data_type == BPCHAROID && context->core.type_mod >= (int32)sizeof(int32))
            {
                /* PG stores bpchar typmod as (declared_length + 4), e.g. bpchar(20)→24 */
                int32 bpchar_len = context->core.type_mod - 4;
                size_t orig_len = context->plaintext_len;

                if (bpchar_len > 0 && (size_t)bpchar_len > orig_len)
                {
                    const char *orig_text;
                    unsigned char *padded;
                    size_t pad_count;
                    size_t i;

                    orig_text = (const char *)context->plaintext;
                    padded = (unsigned char *)pg_malloc((size_t)bpchar_len + 1);
                    if (padded != NULL)
                    {
                        memcpy(padded, orig_text, orig_len);
                        pad_count = (size_t)bpchar_len - orig_len;
                        for (i = 0; i < pad_count; i++)
                            padded[orig_len + i] = ' ';
                        padded[bpchar_len] = '\0';
                        context->plaintext = padded;
                        context->plaintext_len = (size_t)bpchar_len;
                    }
                }
            }

            /* ReqID:TECC4.0 - Universal rtrim for text types: strip trailing
             * spaces before DET/OPE normalization. The space_count is stored
             * in the context for post-encryption format encoding. This ensures
             * all text types produce the same encrypted_core for semantically
             * identical values (e.g., varchar 'hello' and bpchar 'hello' pad
             * to 20 bytes have the same 'hello' core after rtrim). */
            context->text_space_count = 0;
            if (TECC_IS_TEXT_TYPE(context->core.data_type) &&
                context->plaintext_len > 0)
            {
                const unsigned char *pd = context->plaintext;
                size_t used = context->plaintext_len;

                while (used > 0 && pd[used - 1] == ' ')
                    used--;
                context->text_space_count = (uint32_t)(context->plaintext_len - used);
                {
                }
            }

            /* Normalize for DET/SEARCH algorithms */
            if (context->core.algo_support.supports_det ||
                context->core.algo_support.supports_ope)
            {
                TeccFormatterErrorCode norm_ret;
                TeccCryptoContextCore norm_core;

                memset(&norm_core, 0, sizeof(norm_core));
                norm_core.data_type = context->core.data_type;
                norm_core.type_mod = context->core.type_mod;
                norm_core.collation_oid = context->core.collation_oid;
                norm_core.algo_support = context->core.algo_support;
                norm_core.selected_algorithms = context->core.selected_algorithms;
                if (col_meta->collation_locale != NULL)
                    strlcpy(norm_core.collation_locale, col_meta->collation_locale, NAMEDATALEN);
                norm_core.algo_type = TECC_ALGO_DET;

                {
                    size_t norm_buf_used_val = context->plaintext_len - context->text_space_count;
                    size_t req_size = tecc_formatter_calculate_normalized_size(&norm_core,
                                                                              context->plaintext_len);
                    /* ReqID:TECC4.0 - When norm_buf_used is 0 (pure trailing-space
                     * values rtrimmed to empty), ensure req_size can hold the
                     * canonical 16-byte zero block for empty-string cores.
                     * tecc_normalize_text_to_buffer needs buffer->size >= 16
                     * to write the zero block; a req_size based on the full
                     * plaintext_len (not the rtrim'd length) may be too small. */
                    if (norm_buf_used_val == 0 && req_size < 16)
                        req_size = 16;
                    if (req_size > 0)
                    {
                        context->normalized_plaintext = pg_malloc(req_size);
                        if (context->normalized_plaintext)
                        {
                            TeccCryptoBuffer norm_buf;
                            norm_buf.data = context->normalized_plaintext;
                            norm_buf.size = req_size;
                            /* ReqID:TECC4.0 - Use rtrim'd length from universal rtrim above.
                             * context->plaintext_len is the full (possibly padded) length,
                             * context->text_space_count was computed by the rtrim logic. */
                            norm_buf.used = norm_buf_used_val;
                            norm_buf.owned_by_caller = true;

                            memcpy(norm_buf.data, context->plaintext, norm_buf.used);
                            /* ReqID:TECC4.0 - null-terminate for strtod validation */
                            if (norm_buf.size > norm_buf.used)
                                ((unsigned char *)norm_buf.data)[norm_buf.used] = '\0';
                            norm_ret = tecc_formatter_normalize_data(&norm_core,
                                                                     norm_buf.data,
                                                                     norm_buf.used,
                                                                     &norm_buf);
                            if (norm_ret == TECC_FORMATTER_SUCCESS)
                            {
                                context->normalized_plaintext_len = norm_buf.used;
                                context->data_pre_normalized = true;
                                TECC_SCHEDULER_DEBUG("DET/OPE normalization: input=%zu bytes -> output=%zu bytes",
                                                    strlen(plaintext), norm_buf.used);
                            }
                            else
                            {
                                const char *err_desc;
                                pg_free(context->normalized_plaintext);
                                context->normalized_plaintext = NULL;
                                switch (norm_ret)
                                {
                                    case TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW:
                                        err_desc = "numeric field overflow"; break;
                                    default:
                                        err_desc = "DET/OPE normalization failed"; break;
                                }
                                TECC_SCHEDULER_ERROR("DET/OPE normalize failed: %s (code=%d)",
                                                     err_desc, (int)norm_ret);
                                if (tecc_state != NULL)
                                    tecc_record_error((TeccState *)tecc_state, TECC_ERR_ENCRYPT,
                                                     err_desc, __func__, __LINE__, false);
                                tecc_free_encrypted_column_metadata(col_meta);
                                if (!context_is_external) pg_free(context);
                                return NULL;
                            }
                        }
                    }
                }
            }
        }
    }

    /* DET: conditional — failure is fatal since DET is required for equality queries */
    if (context->core.algo_support.supports_det)
    {
        ret = tecc_execute_algo_det((TeccCryptoContextExtended *)context);
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_ERROR("DET algorithm encryption failed for column with support_algo='det': %d", ret);
            result->error_code = TECC_CRYPTO_ALGO_ERROR;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "DET encryption failed");
            tecc_free_encrypted_column_metadata(col_meta);
            if (!context_is_external) pg_free(context);
            return NULL;
        }
    }

    /* OPE: conditional */
    if (context->core.algo_support.supports_ope)
    {
        ret = tecc_execute_algo_ope((TeccCryptoContextExtended *)context);
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_ERROR("OPE algorithm encryption failed for column with support_algo='ope': %d", ret);
            result->error_code = TECC_CRYPTO_ALGO_ERROR;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "OPE encryption failed");
            tecc_free_encrypted_column_metadata(col_meta);
            if (!context_is_external) pg_free(context);
            return NULL;
        }
    }

    /* SEARCH: conditional */
    if (context->core.algo_support.supports_search)
    {
        ret = tecc_execute_algo_search((TeccCryptoContextExtended *)context);
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_ERROR("SEARCH algorithm encryption failed for column with support_algo='search': %d", ret);
            result->error_code = TECC_CRYPTO_ALGO_ERROR;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "SEARCH encryption failed");
            tecc_free_encrypted_column_metadata(col_meta);
            if (!context_is_external) pg_free(context);
            return NULL;
        }
    }

    /* ReqID:TECC4.0 - Build CipherDataType via formatter.
     * tecc_formatter_format_cipherdata correctly packs all 4 algorithm
     * ciphertexts (RND, DET, OPE, SEARCH) with proper length fields. */
    fmt_ret = tecc_formatter_format_cipherdata(context, &cipherdata, &cipherdata_len);
    if (fmt_ret != TECC_FORMATTER_SUCCESS)
    {
        TECC_SCHEDULER_ERROR("Failed to format cipherdata");
        result->error_code = TECC_CRYPTO_MEMORY_ERROR;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "Failed to format cipherdata");
        context->dek_key = NULL;
        context->dek_key_len = 0;
        if (!context_is_external) pg_free(context);
        pg_free(result);
        return NULL;
    }

    /* Set DEK distributed ID from cache if available */
    if (tecc_state != NULL)
    {
        TeccState *state = (TeccState *)tecc_state;
        const char *dek_name_lookup = col_meta ? col_meta->dek_name : NULL;
        if (dek_name_lookup != NULL)
        {
            TeccDekCacheItem *dek_item = tecc_cache_get_dek_by_name_search_path(
                state, dek_name_lookup);
            if (dek_item != NULL && OidIsValid(dek_item->column_key_distributed_id))
                cipherdata->dek_dist_oid = dek_item->column_key_distributed_id;
        }
    }

    /* Build cipherdata_buffer wrapper for ownership tracking */
    cipherdata_buffer = pg_malloc0(sizeof(TeccCryptoBuffer));
    if (cipherdata_buffer)
    {
        cipherdata_buffer->data = cipherdata;
        cipherdata_buffer->size = cipherdata_len;
        cipherdata_buffer->used = cipherdata_len;
    }

    /* Convert to hex string for compatibility */
    fmt_ret = tecc_formatter_hex_encode((const unsigned char *)cipherdata, cipherdata_len, &result->hex_string);
    TECC_SCHEDULER_DEBUG("cipherdata hex dump (first 80 payload bytes)");
        if (fmt_ret != TECC_FORMATTER_SUCCESS)
    {
        TECC_SCHEDULER_ERROR("Failed to convert cipherdata to hex");
        result->error_code = TECC_CRYPTO_FORMAT_ERROR;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "Failed to convert cipherdata to hex");
        tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
        /* Clear dangling pointer after key release */
        context->dek_key = NULL;
        context->dek_key_len = 0;
        pg_free(cipherdata);
        pg_free(cipherdata_buffer);
        if (!context_is_external) pg_free(context);
        pg_free(result);
        return NULL;
    }

    result->hex_len = strlen(result->hex_string);
    result->cipherdata_buffer = cipherdata_buffer;

    /* Update statistics */
    scheduler->encryption_count++;

    /* Cleanup */
    tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
    /* Clear dangling pointer after key release */
    context->dek_key = NULL;
    context->dek_key_len = 0;
    if (!context_is_external) pg_free(context);

    /* Free metadata if it was allocated from real cache (not caller-supplied) */
    if (tecc_state != NULL && col_meta != NULL && col_meta_in == NULL)
    {
        tecc_free_encrypted_column_metadata(col_meta);
    }

    TECC_SCHEDULER_DEBUG("Unified encryption completed successfully");
    return result;
}

/*
 * Unified encryption with preloaded key for batch optimization
 */
static TeccUnifiedEncryptResult *
tecc_unified_encrypt_string_with_preloaded_key(
    TeccCryptoScheduler *scheduler,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name,
    const unsigned char *preloaded_dek_key,
    size_t preloaded_dek_key_len,
    const unsigned char *preloaded_derived_enc_key,
    const unsigned char *preloaded_derived_auth_key)
{
    TeccCryptoContextExtended *context = NULL;
    TeccUnifiedEncryptResult *result = NULL;
    TeccEncryptedColumnMetadata *col_meta = NULL;
    int ret;
    CipherDataType *cipherdata = NULL;
    size_t cipherdata_len = 0;
    TeccFormatterErrorCode fmt_ret;
    unsigned char *primary_ciphertext;
    size_t primary_cipher_len;
    TeccCryptoBuffer *cipherdata_buffer;
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    const char *dek_name_to_use = NULL;
    /* ReqID:TECC4.0 - local pointers to preloaded or fallback-derived keys */
    const unsigned char *effective_derived_enc_key = preloaded_derived_enc_key;
    const unsigned char *effective_derived_auth_key = preloaded_derived_auth_key;

    /* Validate parameters */
    if (!scheduler || !plaintext || !table_name || !column_name)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to unified encrypt with preloaded key");
        return NULL;
    }

    TECC_SCHEDULER_DEBUG("Unified encrypt with preloaded key: %s.%s (len=%zu, key_len=%zu)",
                        table_name, column_name, strlen(plaintext), preloaded_dek_key_len);

    /* Create encryption context with memory pool */
    context = tecc_create_crypto_context_with_collation(table_name, column_name,
                                                       kek_name, dek_name,
                                                       0); /* No collation */
    if (!context)
    {
        TECC_SCHEDULER_ERROR("Failed to create crypto context");
        return NULL;
    }

    /* Note: This function is used for batch optimization and doesn't have connection parameter */
    /* Without connection, we cannot access real cache, so use default values */
    TECC_SCHEDULER_DEBUG("DEBUG: Using default values for batch optimization for column %s.%s (no connection available)", table_name, column_name);

    /* Set default algorithm support */
    context->core.algo_support.supports_det = true;
    context->core.algo_support.supports_ope = true;
    context->core.algo_support.supports_search = false;
    context->core.data_type = TEXTOID;  /* Default to text type */

    /* Allocate result structure */
    result = pg_malloc0(sizeof(TeccUnifiedEncryptResult));
    if (!result)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate encrypt result");
        pg_free(context);
        return NULL;
    }

    /* Initialize result */
    result->error_code = TECC_CRYPTO_SUCCESS;
    result->error_msg[0] = '\0';

    /* Use preloaded DEK key */
    if (preloaded_dek_key && preloaded_dek_key_len > 0)
    {
        /* Copy preloaded key to context */
        context->dek_key = (unsigned char *)pg_malloc(preloaded_dek_key_len);
        if (!context->dek_key)
        {
            TECC_SCHEDULER_ERROR("Failed to allocate memory for preloaded key copy");
            result->error_code = TECC_CRYPTO_MEMORY_ERROR;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "Failed to copy preloaded DEK key");
            pg_free(context);
            pg_free(result);
            return NULL;
        }
        memcpy(context->dek_key, preloaded_dek_key, preloaded_dek_key_len);
        context->dek_key_len = preloaded_dek_key_len;
    }
    else
    {
        /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
        unsigned char *dek_key = NULL;
        size_t dek_key_len = 0;
        unsigned char *derived_enc_fb = NULL;
        unsigned char *derived_auth_fb = NULL;

        /* Fall back to regular key loading */
        TECC_SCHEDULER_WARN("No preloaded key available, falling back to regular key loading");

        /* Use context->dek_name if dek_name is NULL */
        dek_name_to_use = dek_name ? dek_name : context->dek_name;
        TECC_SCHEDULER_DEBUG("DEBUG: Using DEK name: %s", dek_name_to_use);

        ret = tecc_key_manager_adapter_get_dek_key_ext(scheduler->key_adapter,
                                                  dek_name_to_use,
                                                  &dek_key, &dek_key_len,
                                                  &derived_enc_fb, &derived_auth_fb);
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_ERROR("Failed to get DEK key for encryption");
            result->error_code = ret;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "Failed to get DEK key '%s'", dek_name_to_use);
            pg_free(context);
            pg_free(result);
            return NULL;
        }

        context->dek_key = dek_key;
        context->dek_key_len = dek_key_len;
        effective_derived_enc_key = derived_enc_fb;
        effective_derived_auth_key = derived_auth_fb;
    }

    if (scheduler->guc_enabled && scheduler->guc_params)
    {
        ret = tecc_crypto_scheduler_apply_guc_to_data(scheduler, (TeccCryptoContext *)context);
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_ERROR("Failed to apply GUC parameters to data");
            result->error_code = ret;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "Failed to apply GUC parameters");
            /* Note: Don't release preloaded key - it's managed by caller */
            if (!preloaded_dek_key || preloaded_dek_key_len == 0)
            {
                /* Only release if we loaded it ourselves */
                tecc_key_manager_adapter_release_key(scheduler->key_adapter, context->dek_key, context->dek_key_len);
            }
            /* Clear dangling pointer after key release */
            context->dek_key = NULL;
            context->dek_key_len = 0;
            pg_free(context);
            pg_free(result);
            return NULL;
        }
    }

    /* Determine algorithm support */
    if (col_meta)
    {
        /* Use metadata from cache - already set in algo_support */
        /* No need to set again */
    }
    else
    {
        /* Default to RND only if no metadata */
        context->core.algo_support.supports_det = false;
        context->core.algo_support.supports_ope = false;
        context->core.algo_support.supports_search = false;
    }

    /* Execute encryption based on algorithm support */
    if (context->core.algo_support.supports_search)
    {
        /* SEARCH algorithm - use default operator type */
        ret = tecc_execute_algo_search((TeccCryptoContextExtended *)context);
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_ERROR("SEARCH algorithm execution failed");
            result->error_code = ret;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "SEARCH algorithm failed");
            goto cleanup;
        }
    }
    else if (context->core.algo_support.supports_ope)
    {
        /* OPE algorithm */
        ret = tecc_execute_algo_ope((TeccCryptoContextExtended *)context);
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_ERROR("OPE algorithm execution failed");
            result->error_code = ret;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "OPE algorithm failed");
            goto cleanup;
        }
    }
    else if (context->core.algo_support.supports_det)
    {
        /* DET algorithm */
        ret = tecc_execute_algo_det((TeccCryptoContextExtended *)context);
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_ERROR("DET algorithm execution failed");
            result->error_code = ret;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "DET algorithm failed");
            goto cleanup;
        }
    }
    else
    {
        /* RND algorithm (default) */
        {
            /* HSM setup based on KEK's KMS type via key_adapter.
             * If HSM is configured but init failed, abort — no software fallback. */
            void *hsm_state = (scheduler && scheduler->key_adapter) ?
                              scheduler->key_adapter->tecc_state : NULL;
            if (tecc_setup_hsm_context(context, col_meta, hsm_state) < 0)
            {
                TECC_SCHEDULER_ERROR("HSM init failed for KEK, refusing software fallback");
                result->error_code = TECC_CRYPTO_KEY_ERROR;
                snprintf(result->error_msg, sizeof(result->error_msg),
                         "HSM hardware required but initialization failed");
                ret = TECC_CRYPTO_KEY_ERROR;
                goto cleanup;
            }
        }
        ret = tecc_execute_algo_rnd((TeccCryptoContextExtended *)context,
                                    effective_derived_enc_key,
                                    effective_derived_auth_key);
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_ERROR("RND algorithm execution failed");
            result->error_code = ret;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "RND algorithm failed");
            goto cleanup;
        }
    }

    /* Get primary ciphertext from context based on algorithm support */
    if (context->core.algo_support.supports_search && context->search_ciphertext)
    {
        primary_ciphertext = context->search_ciphertext;
        primary_cipher_len = context->search_cipher_len;
    }
    else if (context->core.algo_support.supports_ope && context->ope_ciphertext)
    {
        primary_ciphertext = context->ope_ciphertext;
        primary_cipher_len = context->ope_cipher_len;
    }
    else if (context->core.algo_support.supports_det && context->det_ciphertext)
    {
        primary_ciphertext = context->det_ciphertext;
        primary_cipher_len = context->det_cipher_len;
    }
    else if (context->rnd_ciphertext)
    {
        primary_ciphertext = context->rnd_ciphertext;
        primary_cipher_len = context->rnd_cipher_len;
    }
    else
    {
        primary_ciphertext = NULL;
        primary_cipher_len = 0;
    }

    if (!primary_ciphertext || primary_cipher_len == 0)
    {
        TECC_SCHEDULER_ERROR("No primary ciphertext generated");
        result->error_code = TECC_CRYPTO_SYSTEM_ERROR;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "No ciphertext generated");
        goto cleanup;
    }

    /* Format cipherdata */
    cipherdata = (CipherDataType *)context->cipherdata;
    /* Get cipherdata length from varlena header */
    if (cipherdata)
    {
        /* VARSIZE returns the total length of the varlena structure */
        cipherdata_len = VARSIZE(cipherdata);
    }
    else
    {
        cipherdata_len = 0;
    }

    if (!cipherdata || cipherdata_len == 0)
    {
        TECC_SCHEDULER_ERROR("No cipherdata generated");
        result->error_code = TECC_CRYPTO_SYSTEM_ERROR;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "No cipherdata generated");
        goto cleanup;
    }

    /* Convert to hex string */
    fmt_ret = tecc_formatter_hex_encode((const unsigned char *)cipherdata, cipherdata_len,
                                              &result->hex_string);
    if (fmt_ret != TECC_FORMATTER_SUCCESS)
    {
        TECC_SCHEDULER_ERROR("Failed to convert cipherdata to hex");
        result->error_code = TECC_CRYPTO_FORMAT_ERROR;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "Failed to format cipherdata as hex");
        goto cleanup;
    }

    /* Create cipherdata buffer for result */
    cipherdata_buffer = pg_malloc0(sizeof(TeccCryptoBuffer));
    if (!cipherdata_buffer)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate cipherdata buffer");
        result->error_code = TECC_CRYPTO_MEMORY_ERROR;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "Failed to allocate cipherdata buffer");
        goto cleanup;
    }

    cipherdata_buffer->data = (unsigned char *)cipherdata;
    cipherdata_buffer->size = cipherdata_len;
    cipherdata_buffer->used = cipherdata_len;
    cipherdata_buffer->owned_by_caller = false; /* Owned by context */

    /* Transfer ownership of cipherdata from context to result */
    context->cipherdata = NULL;

    result->cipherdata_buffer = cipherdata_buffer;

    /* Update statistics */
    scheduler->encryption_count++;

cleanup:
    /* Note: col_meta is not used in this function anymore, so no cleanup needed */

    /* Clean up context */
    if (context)
    {
        /* Note: Don't release preloaded key - it's managed by caller */
        if (!preloaded_dek_key || preloaded_dek_key_len == 0)
        {
            /* Only release if we loaded it ourselves */
            if (context->dek_key && context->dek_key_len > 0)
            {
                tecc_key_manager_adapter_release_key(scheduler->key_adapter, context->dek_key, context->dek_key_len);
                context->dek_key = NULL;
                context->dek_key_len = 0;
            }
        }
        else
        {
            /* Free the copy of preloaded key */
            if (context->dek_key)
            {
                pg_free(context->dek_key);
                context->dek_key = NULL;
                context->dek_key_len = 0;
            }
        }

        tecc_destroy_crypto_context(context);
    }

    if (result->error_code != TECC_CRYPTO_SUCCESS)
    {
        /* Free any allocated resources on error */
        if (result->hex_string)
        {
            pg_free(result->hex_string);
            result->hex_string = NULL;
        }
        if (result->cipherdata_buffer)
        {
            if (result->cipherdata_buffer->data)
                pg_free(result->cipherdata_buffer->data);
            pg_free(result->cipherdata_buffer);
            result->cipherdata_buffer = NULL;
        }
    }

    if (result->error_code == TECC_CRYPTO_SUCCESS)
    {
        TECC_SCHEDULER_DEBUG("Unified encryption with preloaded key completed successfully");
    }
    else
    {
        TECC_SCHEDULER_ERROR("Unified encryption with preloaded key failed: %s", result->error_msg);
    }

    return result;
}

/*
 * Unified encryption interface with connection parameter (thread-safe version)
 */
TeccUnifiedEncryptResult *
tecc_unified_encrypt_string_with_conn(
    PGconn *conn,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name)
{
    TeccCryptoScheduler *scheduler;

    /* Validate parameters */
    if (conn == NULL || plaintext == NULL || table_name == NULL || column_name == NULL)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_unified_encrypt_string_with_conn");
        return NULL;
    }

    /* Get scheduler from connection state */
    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return NULL;
    }

    /* Call internal implementation with scheduler */
    return tecc_unified_encrypt_string_internal_with_scheduler(scheduler,
                                                              plaintext,
                                                              table_name,
                                                              column_name,
                                                              kek_name,
                                                              dek_name,
                                                              0, /* No collation */
                                                              tecc_get_state(conn),
                                                              NULL,
                                                              NULL);
}


/* Begin - ReqID:TECC4.0 - Decryption validation functions implementation */

/*
 * tecc_validate_hex_string - Validate hex string format
 *
 * Validates that the hex string contains only valid hex characters
 * (0-9, a-f, A-F) and has an even length.
 *
 * Returns true if valid, false otherwise.
 * If valid_length is not NULL, stores the length of valid hex string.
 */
static bool
tecc_validate_hex_string(const char *hex_string, size_t *valid_length)
{
    size_t i;
    size_t len;

    if (hex_string == NULL)
        return false;

    len = strlen(hex_string);

    /* Hex string must have even length */
    if (len == 0 || (len % 2) != 0)
        return false;

    /* Validate each character */
    for (i = 0; i < len; i++)
    {
        char c = hex_string[i];
        if (!((c >= '0' && c <= '9') ||
              (c >= 'a' && c <= 'f') ||
              (c >= 'A' && c <= 'F')))
        {
            return false;
        }
    }

    if (valid_length != NULL)
        *valid_length = len;

    return true;
}

/*
 * tecc_validate_cipherdata_integrity - Validate cipherdata integrity
 *
 * Validates cipherdata structure integrity including:
 * 1. Minimum size check
 * 2. Version validation
 * 3. Length consistency
 * 4. Data range validation
 *
 * Returns true if valid, false otherwise.
 */
static bool
tecc_validate_cipherdata_integrity(const CipherDataType *cipherdata, size_t cipher_len)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    size_t calculated_len;

    /* Debug: print all CipherDataType fields */
                            
    /* Check minimum size */
    if (cipher_len < sizeof(CipherDataType))
    {
                return false;
    }

    /* Check version */
    if (!CIPHERDATA_VERSION_VALID(cipherdata->version))
    {
                return false;
    }

    /* Check individual data lengths don't exceed total */
    calculated_len = sizeof(CipherDataType) +
                           cipherdata->rnd_length +
                           cipherdata->det_length +
                           cipherdata->ope_length +
                           cipherdata->search_length;

    if (calculated_len != cipher_len)
    {
                return false;
    }

    /* ReqID:TECC4.0 - Negative lengths caught by cipherdata_validate */
    return true;
}

/*
 * tecc_detect_tampering - Detect cipherdata tampering
 *
 * Attempts to detect if cipherdata has been tampered with.
 * Currently implements basic checks - can be enhanced with HMAC verification.
 *
 * Returns true if tampering detected, false otherwise.
 */
static bool
tecc_detect_tampering(const CipherDataType *cipherdata, size_t cipher_len,
                     const unsigned char *dek_key, size_t dek_key_len)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    size_t max_safe_len;

    /* Basic tampering detection:
     * 1. Check for unexpected zeros in critical fields
     * 2. Verify data ranges are reasonable
     * 3. Check for overflow/underflow in calculations
     * 4. Check for suspicious patterns
     */


    /* Check for unexpected zeros in all data length fields */
    if (cipherdata->rnd_length == 0 &&
        cipherdata->det_length == 0 &&
        cipherdata->ope_length == 0 &&
        cipherdata->search_length == 0)
    {
        TECC_SCHEDULER_WARNING("All ciphertext lengths are zero - suspicious");
        return true; /* All data lengths are zero - suspicious */
    }

    /* Check for overflow in total length calculation */
    max_safe_len = SIZE_MAX - sizeof(CipherDataType);
    if (cipherdata->rnd_length > max_safe_len ||
        cipherdata->det_length > max_safe_len ||
        cipherdata->ope_length > max_safe_len ||
        cipherdata->search_length > max_safe_len)
    {
        TECC_SCHEDULER_WARNING("Potential overflow detected");
        return true; /* Potential overflow */
    }

    /* Check for suspicious patterns in data lengths */
    /* All lengths being exactly the same might indicate tampering */
    if (cipherdata->rnd_length > 0 &&
        cipherdata->det_length > 0 &&
        cipherdata->rnd_length == cipherdata->det_length &&
        cipherdata->det_length == cipherdata->ope_length &&
        cipherdata->ope_length == cipherdata->search_length)
    {
        /* This could be legitimate, but worth logging as suspicious */
        TECC_SCHEDULER_WARNING("All ciphertext lengths are identical - possible tampering");
    }

    /* HMAC verification when key is available */
    if (dek_key != NULL && dek_key_len > 0)
    {

        /* For now, we'll just log that HMAC verification would happen here */
        /* In a real implementation, we would:
         * 1. Derive HMAC key from DEK using PBKDF2
         * 2. Calculate HMAC of cipherdata
         * 3. Compare with stored HMAC (if available)
         */


        /* TODO: Implement actual HMAC verification */
        /* For testing purposes, we'll just return false (no tampering) */
    }
    else
    {
    }

    return false;
}

/* End - ReqID:TECC4.0 - Decryption validation functions implementation */

/*
 * Internal decryption implementation with explicit scheduler parameter
 */
static TeccUnifiedDecryptResult *
tecc_unified_decrypt_hex_internal_with_scheduler(
    PGconn *conn,
    TeccCryptoScheduler *scheduler,
    const char *hex_string,
    const char *table_name,
    const char *column_name,
    TeccCryptoContext *external_context)
{
    /* Begin - ReqID:TECC4.0 */
    TeccCryptoContextExtended *context = NULL;
    bool context_is_external = (external_context != NULL);
    TeccUnifiedDecryptResult *result = NULL;
    TeccEncryptedColumnMetadata *col_meta = NULL;
    unsigned char *dek_key = NULL;
    size_t dek_key_len = 0;
    unsigned char *derived_enc_key = NULL;    /* ReqID:TECC4.0 - cached derived */
    unsigned char *derived_auth_key = NULL;   /* ReqID:TECC4.0 - cached derived */
    unsigned char *cipher_data = NULL;
    size_t cipher_len = 0;
    TeccFormatterErrorCode fmt_ret;
    CipherDataType *cipherdata = NULL;
    TeccParsedCipherdata parsed_data;
    int ret;
    unsigned char *plaintext = NULL;
    size_t plain_len = 0;
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    size_t valid_hex_len = 0;
    TeccAlgorithmType algo_type = TECC_ALGO_RND;
    const unsigned char *ciphertext = NULL;
    size_t ciphertext_len = 0;

    /* Validate parameters */
    if (!scheduler || !hex_string || !table_name || !column_name)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to unified decrypt with scheduler");
        return NULL;
    }

    TECC_SCHEDULER_DEBUG("Unified decrypt with scheduler: %s.%s (hex_len=%zu)",
                        table_name, column_name, strlen(hex_string));

    /* Begin - ReqID:TECC4.0 - Enhanced decryption validation */
    /* Validate hex string format */
    valid_hex_len = 0;
    if (!tecc_validate_hex_string(hex_string, &valid_hex_len))
    {
        TECC_SCHEDULER_ERROR("Invalid hex string format for decryption");
        return NULL;
    }

    TECC_SCHEDULER_DEBUG("Hex string validation passed (length=%zu)", valid_hex_len);
    /* End - ReqID:TECC4.0 - Enhanced decryption validation */

    /* Create or reuse decryption context */
    if (context_is_external)
    {
        context = external_context;
        tecc_crypto_context_reset(context);
        strlcpy(context->table_name, table_name, NAMEDATALEN);
        strlcpy(context->column_name, column_name, NAMEDATALEN);
    }
    else
    {
        context = tecc_create_crypto_context_with_pool(table_name, column_name,
                                                      NULL, NULL);
        if (!context)
        {
            TECC_SCHEDULER_ERROR("Failed to create crypto context");
            return NULL;
        }
    }

    /* Get column metadata from cache */
    /* Try to get metadata from real cache if connection is available */
    if (conn != NULL)
    {
        TeccState *state = tecc_get_state(conn);
        TeccColumnCacheItem *cache_item = NULL;

        if (state != NULL)
        {
            /* 从连接中获取实际数据库名，替代硬编码的"postgres" */
            const char *db_name = PQdb(conn);
            if (db_name == NULL || db_name[0] == '\0')
                db_name = "postgres";
            cache_item = tecc_cache_get_encrypted_column_by_name(state, db_name, NULL, table_name, column_name);
            if (cache_item != NULL)
            {
                col_meta = tecc_convert_column_cache_item_to_metadata(state, cache_item);
                TECC_SCHEDULER_DEBUG("DEBUG: Got column metadata from real cache for decryption: col_meta=%p", col_meta);
            }
        }
    }

    /* ReqID:TECC4.0 - Check DEK permission before decryption */
    /* ReqID:TECC4.0 - 放宽条件：无论dek_name是否为空，都尝试权限检查 */
    if (conn != NULL && col_meta != NULL)
    {
        TeccState *decrypt_state;
        bool has_permission;

        decrypt_state = tecc_get_state(conn);
        if (decrypt_state != NULL)
        {
            has_permission = tecc_cache_check_column_dek_permission(
                decrypt_state, NULL, table_name, column_name);
            if (!has_permission)
            {
                /* No DEK permission: skip decrypt, return hex ciphertext */
                result = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
                if (result != NULL)
                {
                    size_t hex_len;
                    hex_len = strlen(hex_string);
                    result->plaintext = (char *)pg_malloc(hex_len + 3);
                    if (result->plaintext != NULL)
                    {
                        result->plaintext[0] = '\\';
                        result->plaintext[1] = 'x';
                        memcpy(result->plaintext + 2, hex_string, hex_len);
                        result->plaintext[hex_len + 2] = '\0';
                        result->plain_len = hex_len + 2;
                    }
                    result->skipped_decrypt = true;
                    result->error_code = TECC_CRYPTO_SUCCESS;
                    result->error_msg[0] = '\0';
                }
                if (!context_is_external) pg_free(context);
                if (col_meta != NULL)
                    tecc_free_encrypted_column_metadata(col_meta);
                return result;
            }
        }
    }

    if (col_meta)
    {
        /* Update algorithm support from metadata */
        context->core.algo_support.supports_det = col_meta->enable_det;
        context->core.algo_support.supports_ope = col_meta->enable_ope;
        context->core.algo_support.supports_search = col_meta->enable_search;
        context->core.data_type = tecc_map_to_original_type_oid(col_meta->data_type_original_oid);
        context->core.type_mod = col_meta->data_type_original_mod;
        context->core.collation_oid = col_meta->collation_oid;
        if (col_meta->collation_locale != NULL)
            strlcpy(context->core.collation_locale, col_meta->collation_locale, NAMEDATALEN);
    }

    /* Allocate result structure */
    result = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
    if (!result)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate decrypt result");
        if (!context_is_external) pg_free(context);
        return NULL;
    }

    /* Initialize result */
    result->error_code = TECC_CRYPTO_SUCCESS;
    result->error_msg[0] = '\0';

    /* Debug: print hex string before decode */
    TECC_SCHEDULER_DEBUG("hex string to decode (first 20 chars): %.*s", 20, hex_string);

    /* Convert hex string to binary */
    fmt_ret = tecc_formatter_hex_decode(hex_string, &cipher_data, &cipher_len);
    if (fmt_ret != TECC_FORMATTER_SUCCESS)
    {
        TECC_SCHEDULER_ERROR("Failed to decode hex string");
        result->error_code = TECC_CRYPTO_FORMAT_ERROR;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "Failed to decode hex string");
        if (!context_is_external) pg_free(context);
        pg_free(result);
        return NULL;
    }

    /* Parse cipherdata to determine algorithm */
    cipherdata = (CipherDataType *)cipher_data;

    /* Debug: print full decoded data with field markers */
    if (cipher_len >= 24) {
        int version = *(int32*)(cipher_data+4);
        int rnd_len = *(int32*)(cipher_data+8);
        int det_len = *(int32*)(cipher_data+12);
        int ope_len = *(int32*)(cipher_data+16);
        int srch_len = *(int32*)(cipher_data+20);
        TECC_SCHEDULER_DEBUG("CipherDataType: version=0x%08x rnd_len=%d det_len=%d ope_len=%d srch_len=%d payload=%zu bytes",
                             version, rnd_len, det_len, ope_len, srch_len, cipher_len-24);
    }

    /* Begin - ReqID:TECC4.0 - Enhanced cipherdata validation */
    /* Validate cipherdata integrity */
    if (!tecc_validate_cipherdata_integrity(cipherdata, cipher_len))
    {
        TECC_SCHEDULER_ERROR("Cipherdata integrity validation failed");
        result->error_code = TECC_CRYPTO_FORMAT_ERROR;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "Cipherdata integrity validation failed");
        pg_free(cipher_data);
        if (!context_is_external) pg_free(context);
        pg_free(result);
        return NULL;
    }

    TECC_SCHEDULER_DEBUG("Cipherdata integrity validation passed");
    /* End - ReqID:TECC4.0 - Enhanced cipherdata validation */

    fmt_ret = tecc_formatter_parse_cipherdata(cipherdata, cipher_len, &parsed_data);
    if (fmt_ret != TECC_FORMATTER_SUCCESS)
    {
        TECC_SCHEDULER_ERROR("Failed to parse cipherdata");
        result->error_code = TECC_CRYPTO_FORMAT_ERROR;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "Failed to parse cipherdata");
        pg_free(cipher_data);
        if (!context_is_external) pg_free(context);
        pg_free(result);
        return NULL;
    }

    /* ReqID:TECC4.0 - SELECT decryption always uses RND ciphertext.
     * DET/OPE/SEARCH ciphertexts are used for backend comparison operators,
     * not for decryption. Only RND can reconstruct the original plaintext. */
    algo_type = TECC_ALGO_RND;
    if (parsed_data.rnd_data_len > 0)
    {
        context->rnd_ciphertext = (unsigned char *)parsed_data.rnd_data;
        context->rnd_cipher_len = parsed_data.rnd_data_len;
    }
    else
    {
        TECC_SCHEDULER_ERROR("No RND ciphertext found in cipherdata for decryption");
        result->error_code = TECC_CRYPTO_FORMAT_ERROR;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "No RND ciphertext found in cipherdata");
        pg_free(cipher_data);
        if (!context_is_external) pg_free(context);
        pg_free(result);
        return NULL;
    }

    /* Get DEK key from key manager */
    /* Use actual DEK name from column metadata if available */
    {
        const char *dek_name_to_use;

        dek_name_to_use = NULL;
        if (col_meta != NULL && col_meta->dek_name != NULL && col_meta->dek_name[0] != '\0')
        {
            dek_name_to_use = col_meta->dek_name;
        }
        else
        {
            /* ReqID:TECC4.0 - col_meta unavailable (e.g. expression result),
             * try to find DEK by distributed_id from CipherDataType */
            Oid dist_id;
            const char *found_name;

            dist_id = cipherdata->dek_dist_oid;
            if (OidIsValid(dist_id) && conn != NULL)
            {
                TeccState *lookup_state;
                lookup_state = tecc_get_state(conn);
                if (lookup_state != NULL)
                {
                    found_name = tecc_cache_get_dek_name_by_distributed_id(
                        lookup_state, dist_id);
                    if (found_name != NULL)
                        dek_name_to_use = found_name;
                }
            }
        }

        /* ReqID:TECC4.0 - If still no DEK name, user lacks permission.
         * Return hex ciphertext instead of error for SELECT/decrypt path. */
        if (dek_name_to_use == NULL)
        {
            TECC_SCHEDULER_DEBUG("No DEK available for decrypt — returning hex ciphertext");
            result->error_code = TECC_CRYPTO_SUCCESS;
            result->error_msg[0] = '\0';
            {
                size_t hex_len;
                hex_len = strlen(hex_string);
                result->plaintext = (char *)pg_malloc(hex_len + 3);
                if (result->plaintext != NULL)
                {
                    result->plaintext[0] = '\\';
                    result->plaintext[1] = 'x';
                    memcpy(result->plaintext + 2, hex_string, hex_len);
                    result->plaintext[hex_len + 2] = '\0';
                    result->plain_len = hex_len + 2;
                }
                result->skipped_decrypt = true;
            }
            pg_free(cipher_data);
            if (!context_is_external) pg_free(context);
            return result;
        }

        ret = tecc_key_manager_adapter_get_dek_key_ext(scheduler->key_adapter,
                                                  dek_name_to_use,
                                                  &dek_key, &dek_key_len,
                                                  &derived_enc_key,
                                                  &derived_auth_key);
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            /* ReqID:TECC4.0 - DEK不可用（无权限或缓存未命中）：
             * 返回密文替代报错。覆盖权限检查被绕过（如tecc_get_state返回NULL）
             * 但DEK名称仍被解析出的边界情况。 */
            TECC_SCHEDULER_DEBUG("DEK key retrieval failed (%d) — returning hex ciphertext", ret);
            result->error_code = TECC_CRYPTO_SUCCESS;
            result->error_msg[0] = '\0';
            {
                size_t hex_len;
                hex_len = strlen(hex_string);
                result->plaintext = (char *)pg_malloc(hex_len + 3);
                if (result->plaintext != NULL)
                {
                    result->plaintext[0] = '\\';
                    result->plaintext[1] = 'x';
                    memcpy(result->plaintext + 2, hex_string, hex_len);
                    result->plaintext[hex_len + 2] = '\0';
                    result->plain_len = hex_len + 2;
                }
                result->skipped_decrypt = true;
            }
            pg_free(cipher_data);
            if (!context_is_external) pg_free(context);
            return result;
        }
    }

    /* Set key in context - note: key will be released by tecc_key_manager_adapter_release_key */
    context->dek_key = dek_key;
    context->dek_key_len = dek_key_len;

    /* Set up HSM SM4 decryption if column uses HSM KEK.
     * If HSM is configured but init failed, abort — no software fallback. */
    if (col_meta != NULL && conn != NULL)
    {
        TeccState *hsm_state = tecc_get_state(conn);
        if (hsm_state != NULL)
        {
            if (tecc_setup_hsm_context(context, col_meta, hsm_state) < 0)
            {
                TECC_SCHEDULER_ERROR("HSM init failed for KEK, refusing software fallback");
                result->error_code = TECC_CRYPTO_KEY_ERROR;
                snprintf(result->error_msg, sizeof(result->error_msg),
                         "HSM hardware required but initialization failed");
                tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
                pg_free(cipher_data);
                if (!context_is_external) pg_free(context);
                pg_free(result);
                return NULL;
            }
        }
    }

    /* Begin - ReqID:TECC4.0 - Tampering detection */
    /* Check for potential tampering in cipherdata */
    if (tecc_detect_tampering(cipherdata, cipher_len, dek_key, dek_key_len))
    {
        TECC_SCHEDULER_ERROR("Potential tampering detected in cipherdata");
        result->error_code = TECC_CRYPTO_DATA_ERROR;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "Potential tampering detected in cipherdata");
        tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
        pg_free(cipher_data);
        if (!context_is_external) pg_free(context);
        pg_free(result);
        return NULL;
    }

    TECC_SCHEDULER_DEBUG("Tampering detection passed");
    /* End - ReqID:TECC4.0 - Tampering detection */

    /* Execute decryption using zero-copy algorithm interface */
    {
        size_t buffer_size;

        /*
         * For HSM-decrypted RND data, the stored format is
         * [16-byte IV][SM4-CBC ciphertext]. The software-format
         * function returns 0 for this layout (expects a larger
         * software-specific header), so we calculate the buffer
         * size directly from ciphertext length.
         */
        if (algo_type == TECC_ALGO_RND && context->hsm_kms_type != 0)
            buffer_size = context->rnd_cipher_len;
        else
            buffer_size = tecc_algo_calc_decrypt_buffer_size(algo_type,
                                                           (algo_type == TECC_ALGO_SEARCH) ? context->search_cipher_len :
                                                           (algo_type == TECC_ALGO_OPE) ? context->ope_cipher_len :
                                                           (algo_type == TECC_ALGO_DET) ? context->det_cipher_len :
                                                           context->rnd_cipher_len);

        plaintext = pg_malloc(buffer_size);
        if (!plaintext)
        {
            TECC_SCHEDULER_ERROR("Failed to allocate plaintext buffer");
            result->error_code = TECC_CRYPTO_MEMORY_ERROR;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "Failed to allocate plaintext buffer");
            tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
            pg_free(cipher_data);
            if (!context_is_external) pg_free(context);
            pg_free(result);
            return NULL;
        }

        ciphertext = NULL;
        ciphertext_len = 0;

        if (algo_type == TECC_ALGO_SEARCH)
        {
            ciphertext = context->search_ciphertext;
            ciphertext_len = context->search_cipher_len;
        }
        else if (algo_type == TECC_ALGO_OPE)
        {
            ciphertext = context->ope_ciphertext;
            ciphertext_len = context->ope_cipher_len;
        }
        else if (algo_type == TECC_ALGO_DET)
        {
            ciphertext = context->det_ciphertext;
            ciphertext_len = context->det_cipher_len;
        }
        else /* TECC_ALGO_RND */
        {
            ciphertext = context->rnd_ciphertext;
            ciphertext_len = context->rnd_cipher_len;
        }

        /* Try HSM SM4 decryption (for RND with HSM-enabled columns).
         * If HSM is configured, it MUST be used — no software fallback. */
        if (algo_type == TECC_ALGO_RND &&
            context->hsm_adapter != NULL && context->hsm_device != NULL)
        {
            HsmAdapterInterface *hsm = (HsmAdapterInterface *)context->hsm_adapter;

            if (hsm->sm4_decrypt == NULL)
            {
                TECC_SCHEDULER_ERROR("HSM adapter configured but sm4_decrypt is NULL");
                result->error_code = TECC_CRYPTO_KEY_ERROR;
                snprintf(result->error_msg, sizeof(result->error_msg),
                         "HSM adapter configured but sm4_decrypt is NULL");
                tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
                memset(plaintext, 0, buffer_size);
                pg_free(plaintext);
                pg_free(cipher_data);
                if (!context_is_external) pg_free(context);
                pg_free(result);
                return NULL;
            }

            if (ciphertext_len > 16)
            {
                unsigned char sm4_key[TECC_SM4_KEY_SIZE];
                size_t actual_key_len = (dek_key_len >= TECC_SM4_KEY_SIZE) ?
                                        TECC_SM4_KEY_SIZE : dek_key_len;
                HsmErrorCode hsm_ret;
                size_t hsm_plain_len = buffer_size;

                memcpy(sm4_key, dek_key, actual_key_len);
                TECC_LOG(TECC_LOG_DEBUG1, "TECC_ROUTE: executing RND decrypt via HSM SM4 (HARDWARE)");
                hsm_ret = hsm->sm4_decrypt(
                    (HsmDevice *)context->hsm_device,
                    sm4_key, actual_key_len,
                    ciphertext,           /* IV = first 16 bytes */
                    ciphertext + 16,      /* ciphertext after IV */
                    ciphertext_len - 16,
                    plaintext, &hsm_plain_len);
                memset(sm4_key, 0, sizeof(sm4_key));

                if (hsm_ret == HSM_SUCCESS)
                {
                    /* Strip PKCS#7 padding from HSM decrypt result.
                     * SWXA adapter already strips padding internally, so skip
                     * the safety net for SWXA to avoid double-stripping. */
                    if (context->hsm_kms_type != 4 && hsm_plain_len > 0)
                    {
                        unsigned char pkcs7_pad = plaintext[hsm_plain_len - 1];
                        if (pkcs7_pad > 0 && pkcs7_pad <= 16 &&
                            hsm_plain_len >= pkcs7_pad)
                        {
                            bool pad_valid = true;
                            size_t p;
                            for (p = hsm_plain_len - pkcs7_pad; p < hsm_plain_len; p++)
                            {
                                if ((unsigned char)plaintext[p] != pkcs7_pad)
                                {
                                    pad_valid = false;
                                    break;
                                }
                            }
                            if (pad_valid)
                                hsm_plain_len -= pkcs7_pad;
                        }
                    }
                    plain_len = hsm_plain_len;
                    ret = TECC_ALGO_SUCCESS;
                }
                else
                {
                    TECC_SCHEDULER_ERROR("HSM SM4 decrypt failed: %d", (int)hsm_ret);
                    result->error_code = TECC_CRYPTO_KEY_ERROR;
                    snprintf(result->error_msg, sizeof(result->error_msg),
                             "HSM SM4 decrypt failed: %d", (int)hsm_ret);
                    tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
                    memset(plaintext, 0, buffer_size);
                    pg_free(plaintext);
                    pg_free(cipher_data);
                    if (!context_is_external) pg_free(context);
                    pg_free(result);
                    return NULL;
                }
            }
            else
            {
                TECC_SCHEDULER_ERROR("Ciphertext too short for HSM SM4 decrypt: %zu bytes",
                                     ciphertext_len);
                result->error_code = TECC_CRYPTO_DATA_ERROR;
                snprintf(result->error_msg, sizeof(result->error_msg),
                         "Ciphertext too short for HSM SM4 decrypt: %zu bytes",
                         ciphertext_len);
                tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
                memset(plaintext, 0, buffer_size);
                pg_free(plaintext);
                pg_free(cipher_data);
                if (!context_is_external) pg_free(context);
                pg_free(result);
                return NULL;
            }
        }
        else
        {
            /* No HSM configured, use software SM4 */
            ret = TECC_ALGO_OPERATION_NOT_SUPPORTED;
        }

        /* Software SM4 (when no HSM configured) */
        if (ret != TECC_ALGO_SUCCESS)
        {
            TECC_LOG(TECC_LOG_DEBUG1, "TECC_ROUTE: executing RND decrypt via SOFTWARE SM4");
            if (algo_type == TECC_ALGO_RND && derived_enc_key != NULL && derived_auth_key != NULL)
            {
                ret = tecc_sm4_rnd_decrypt_with_keys(dek_key, TECC_SM4_KEY_SIZE,
                                             ciphertext, ciphertext_len,
                                             plaintext, buffer_size,
                                             &plain_len,
                                             derived_enc_key, derived_auth_key);
            }
            else
            {
                ret = tecc_algo_decrypt_zero_copy(algo_type,
                                             dek_key, dek_key_len,
                                             ciphertext, ciphertext_len,
                                             plaintext, buffer_size,
                                             &plain_len);
            }
        }
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_ERROR("%s algorithm decryption failed",
                               tecc_get_algorithm_name(algo_type));
            result->error_code = ret;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "%s algorithm decryption failed",
                     tecc_get_algorithm_name(algo_type));
            tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
            /* Securely wipe plaintext data before freeing */
            if (plaintext)
                memset(plaintext, 0, buffer_size);
            pg_free(plaintext);
            pg_free(cipher_data);
            if (!context_is_external) pg_free(context);
            pg_free(result);
            return NULL;
        }
    }

    result->plaintext = (char *)plaintext;
    result->plain_len = plain_len;

    /* Update statistics */
    scheduler->decryption_count++;

    /* Cleanup */
    tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
    /* Clear dangling pointer after key release */
    context->dek_key = NULL;
    context->dek_key_len = 0;
    pg_free(cipher_data);
    if (context_is_external)
    {
        /* External context: only clear per-use pointers, keep buffers */
        context->rnd_ciphertext = NULL;
        context->det_ciphertext = NULL;
        context->ope_ciphertext = NULL;
        context->search_ciphertext = NULL;
    }
    else
    {
        if (!context_is_external) pg_free(context);
    }

    /* Free metadata if it was allocated from real cache */
    if (conn != NULL && col_meta != NULL)
    {
        tecc_free_encrypted_column_metadata(col_meta);
    }
    /* Note: If conn is NULL, col_meta should also be NULL since we don't use mock data anymore */

    TECC_SCHEDULER_DEBUG("Unified decryption completed successfully (plain_len=%zu)",
                        result->plain_len);
    return result;
    /* End - ReqID:TECC4.0 */
}

/* Begin - ReqID:TECC4.0 */
/*
 * Batch hex decrypt — decrypts multiple hex strings for the same column
 * at once. Shares one SM4 context (no per-row key expansion) and one
 * DEK lookup across all items. localkms RND path only; HSM deferred.
 *
 * Returns an array of count result pointers, or NULL on fatal error.
 * On item-level failure (e.g. one corrupt row), that item's result has
 * error_code set but the rest succeed.
 * Caller frees each result[i] via tecc_unified_free_decrypt_result().
 */
static TeccUnifiedDecryptResult **
tecc_unified_batch_decrypt_hex_internal_with_scheduler(
    PGconn *conn,
    TeccCryptoScheduler *scheduler,
    const char **hex_strings,
    size_t count,
    const char *schema_name,
    const char *table_name,
    const char *column_name,
    TeccCryptoContext *external_context)
{
    TeccUnifiedDecryptResult **results;
    TeccCryptoContextExtended *context;
    bool context_is_external;
    TeccEncryptedColumnMetadata *col_meta;
    unsigned char *dek_key;
    size_t dek_key_len;
    unsigned char *derived_enc_key;
    unsigned char *derived_auth_key;
    const char *dek_name_to_use;
    unsigned char *batch_binary_base;
    unsigned char **binary_data;
    size_t *binary_lens;
    const unsigned char **rnd_ciphertexts;
    size_t *rnd_ciphertext_lens;
    TeccFormatterErrorCode fmt_ret;
    bool all_skipped;
    int ret;
    size_t i;

    /* Validate parameters */
    if (!scheduler || !hex_strings || count == 0 || !table_name || !column_name)
        return NULL;

    /* Allocate results array */
    results = pg_malloc0(sizeof(TeccUnifiedDecryptResult *) * count);
    if (!results)
        return NULL;

    /* Allocate per-item working arrays */
    binary_data = pg_malloc(sizeof(unsigned char *) * count);
    binary_lens = pg_malloc(sizeof(size_t) * count);
    rnd_ciphertexts = pg_malloc(sizeof(const unsigned char *) * count);
    rnd_ciphertext_lens = pg_malloc(sizeof(size_t) * count);
    if (!binary_data || !binary_lens || !rnd_ciphertexts || !rnd_ciphertext_lens)
    {
        pg_free(results);
        if (binary_data) pg_free(binary_data);
        if (binary_lens) pg_free(binary_lens);
        if (rnd_ciphertexts) pg_free(rnd_ciphertexts);
        if (rnd_ciphertext_lens) pg_free(rnd_ciphertext_lens);
        return NULL;
    }

    /* Create or reuse context */
    context_is_external = (external_context != NULL);
    if (context_is_external)
    {
        context = external_context;
        tecc_crypto_context_reset(context);
        strlcpy(context->table_name, table_name, NAMEDATALEN);
        strlcpy(context->column_name, column_name, NAMEDATALEN);
    }
    else
    {
        context = tecc_create_crypto_context_with_pool(table_name, column_name,
                                                       NULL, NULL);
        if (!context)
        {
            pg_free(results);
            pg_free(binary_data);
            pg_free(binary_lens);
            pg_free(rnd_ciphertexts);
            pg_free(rnd_ciphertext_lens);
            return NULL;
        }
    }

    /* Get column metadata from cache (once for all items) */
    col_meta = NULL;
    if (conn != NULL)
    {
        TeccState *state;
        TeccColumnCacheItem *cache_item;
        const char *db_name;

        state = tecc_get_state(conn);
        if (state != NULL)
        {
            db_name = PQdb(conn);
            if (db_name == NULL || db_name[0] == '\0')
                db_name = "postgres";
            cache_item = tecc_cache_get_encrypted_column_by_name(state, db_name,
                                                                 schema_name, table_name,
                                                                 column_name);
            if (cache_item != NULL)
                col_meta = tecc_convert_column_cache_item_to_metadata(state, cache_item);
        }
    }

    /* Permission check (once for all items) */
    all_skipped = false;
    if (conn != NULL && col_meta != NULL)
    {
        TeccState *perm_state;

        perm_state = tecc_get_state(conn);
        if (perm_state != NULL)
        {
            if (!tecc_cache_check_column_dek_permission(perm_state,
                                                        schema_name,
                                                        table_name, column_name))
            {
                all_skipped = true;
                TECC_SCHEDULER_DEBUG("Batch decrypt: no DEK permission, skipping all");
            }
        }
    }

    /* Determine DEK name */
    dek_name_to_use = NULL;
    if (!all_skipped)
    {
        if (col_meta != NULL && col_meta->dek_name != NULL &&
            col_meta->dek_name[0] != '\0')
        {
            dek_name_to_use = col_meta->dek_name;
        }
        else
        {
            /* For batch, we can't peek at individual cipherdata for dist_id
             * without decoding first. We'll do it per-item below if needed. */
        }
    }

    /* Resolve DEK name by decoding first item if needed */
    if (!all_skipped && dek_name_to_use == NULL && count > 0 && hex_strings[0])
    {
        unsigned char *first_binary;
        size_t first_binary_len;

        fmt_ret = tecc_formatter_hex_decode(hex_strings[0],
                                             &first_binary, &first_binary_len);
        if (fmt_ret == TECC_FORMATTER_SUCCESS)
        {
            CipherDataType *cd;
            Oid dist_id;

            cd = (CipherDataType *)first_binary;
            dist_id = cd->dek_dist_oid;
            if (OidIsValid(dist_id) && conn != NULL)
            {
                TeccState *lookup_state;

                lookup_state = tecc_get_state(conn);
                if (lookup_state != NULL)
                {
                    const char *found_name;

                    found_name = tecc_cache_get_dek_name_by_distributed_id(
                        lookup_state, dist_id);
                    if (found_name != NULL)
                        dek_name_to_use = found_name;
                }
            }
            pg_free(first_binary);
        }
    }

    if (!all_skipped && dek_name_to_use == NULL)
        all_skipped = true;

    /* Get DEK key (once for all items) */
    dek_key = NULL;
    dek_key_len = 0;
    derived_enc_key = NULL;
    derived_auth_key = NULL;

    if (!all_skipped)
    {
        ret = tecc_key_manager_adapter_get_dek_key_ext(
            scheduler->key_adapter, dek_name_to_use,
            &dek_key, &dek_key_len, &derived_enc_key, &derived_auth_key);
        if (ret != TECC_CRYPTO_SUCCESS)
            all_skipped = true;
    }

    if (all_skipped)
    {
        /* Return hex ciphertexts for all items */
        for (i = 0; i < count; i++)
        {
            results[i] = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
            if (results[i] != NULL)
            {
                size_t hex_len;

                hex_len = (hex_strings[i] != NULL) ? strlen(hex_strings[i]) : 0;
                results[i]->plaintext = (char *)pg_malloc(hex_len + 3);
                if (results[i]->plaintext != NULL)
                {
                    if (hex_strings[i] != NULL)
                    {
                        results[i]->plaintext[0] = '\\';
                        results[i]->plaintext[1] = 'x';
                        memcpy(results[i]->plaintext + 2, hex_strings[i], hex_len);
                        results[i]->plaintext[hex_len + 2] = '\0';
                        results[i]->plain_len = hex_len + 2;
                    }
                    else
                    {
                        results[i]->plaintext[0] = '\0';
                        results[i]->plain_len = 0;
                    }
                }
                results[i]->skipped_decrypt = true;
                results[i]->error_code = TECC_CRYPTO_SUCCESS;
            }
        }
        if (dek_key)
            tecc_key_manager_adapter_release_key(scheduler->key_adapter,
                                                 dek_key, dek_key_len);
        pg_free(binary_data);
        pg_free(binary_lens);
        pg_free(rnd_ciphertexts);
        pg_free(rnd_ciphertext_lens);
        if (col_meta)
            tecc_free_encrypted_column_metadata(col_meta);
        if (!context_is_external) pg_free(context);
        return results;
    }

    /* Update algorithm support from metadata */
    if (col_meta)
    {
        context->core.algo_support.supports_det = col_meta->enable_det;
        context->core.algo_support.supports_ope = col_meta->enable_ope;
        context->core.algo_support.supports_search = col_meta->enable_search;
        context->core.data_type = tecc_map_to_original_type_oid(col_meta->data_type_original_oid);
        context->core.type_mod = col_meta->data_type_original_mod;
        context->core.collation_oid = col_meta->collation_oid;
        if (col_meta->collation_locale != NULL)
            strlcpy(context->core.collation_locale, col_meta->collation_locale, NAMEDATALEN);
    }

    /* Detect HSM SM4 encryption for this column.
     * If HSM is configured, the batch software RND decrypt path cannot be used
     * because HSM encrypts data as raw SM4-CBC ([16-byte IV][ciphertext]) without
     * the software Sm4RndDataFormat wrapper (version + auth_tag + IV + ct_len + ct).
     * We fall through to per-item HSM decrypt below. */
    {
        TeccState *local_state = tecc_get_state(conn);
        if (local_state != NULL && col_meta != NULL)
        {
            if (tecc_setup_hsm_context(context, col_meta, local_state) < 0)
            {
                TECC_SCHEDULER_ERROR("Batch decrypt: HSM init failed for column %s.%s",
                                     table_name, column_name);
                /* HSM init failure is fatal — no software fallback.
                 * Mark all items as errors and let caller handle. */
                for (i = 0; i < count; i++)
                {
                    results[i] = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
                    if (results[i])
                    {
                        results[i]->error_code = TECC_CRYPTO_KEY_ERROR;
                        snprintf(results[i]->error_msg, sizeof(results[i]->error_msg),
                                 "Batch decrypt: HSM init failed for column %s.%s",
                                 table_name, column_name);
                    }
                }
                tecc_key_manager_adapter_release_key(scheduler->key_adapter,
                                                     dek_key, dek_key_len);
                pg_free(binary_data);
                pg_free(binary_lens);
                pg_free(rnd_ciphertexts);
                pg_free(rnd_ciphertext_lens);
                if (col_meta)
                    tecc_free_encrypted_column_metadata(col_meta);
                if (!context_is_external) pg_free(context);
                return results;
            }
        }
    }

    /* Batch hex decode: single allocation for all binary data */
    fmt_ret = tecc_formatter_batch_hex_decode(hex_strings, count,
                                               binary_data, binary_lens);
    if (fmt_ret != TECC_FORMATTER_SUCCESS)
    {
        for (i = 0; i < count; i++)
            results[i] = NULL;
        tecc_key_manager_adapter_release_key(scheduler->key_adapter,
                                             dek_key, dek_key_len);
        pg_free(binary_data);
        pg_free(binary_lens);
        pg_free(rnd_ciphertexts);
        pg_free(rnd_ciphertext_lens);
        if (col_meta)
            tecc_free_encrypted_column_metadata(col_meta);
        if (!context_is_external) pg_free(context);
        return results;
    }
    batch_binary_base = binary_data[0];

    /* Per-item: parse CipherDataType, extract RND ciphertext */
    for (i = 0; i < count; i++)
    {
        CipherDataType *cipherdata;
        TeccParsedCipherdata parsed_data;

        results[i] = NULL;
        rnd_ciphertexts[i] = NULL;
        rnd_ciphertext_lens[i] = 0;

        if (!binary_data[i] || binary_lens[i] < 24)
        {
            results[i] = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
            if (results[i])
            {
                results[i]->error_code = TECC_CRYPTO_FORMAT_ERROR;
                snprintf(results[i]->error_msg, sizeof(results[i]->error_msg),
                         "Batch decrypt: item %zu too short", i);
            }
            continue;
        }

        cipherdata = (CipherDataType *)binary_data[i];

        if (!tecc_validate_cipherdata_integrity(cipherdata, binary_lens[i]))
        {
            results[i] = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
            if (results[i])
            {
                results[i]->error_code = TECC_CRYPTO_FORMAT_ERROR;
                snprintf(results[i]->error_msg, sizeof(results[i]->error_msg),
                         "Batch decrypt: item %zu integrity check failed", i);
            }
            continue;
        }

        fmt_ret = tecc_formatter_parse_cipherdata(cipherdata, binary_lens[i],
                                                   &parsed_data);
        if (fmt_ret != TECC_FORMATTER_SUCCESS)
        {
            results[i] = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
            if (results[i])
            {
                results[i]->error_code = TECC_CRYPTO_FORMAT_ERROR;
                snprintf(results[i]->error_msg, sizeof(results[i]->error_msg),
                         "Batch decrypt: item %zu parse failed", i);
            }
            continue;
        }

        if (parsed_data.rnd_data_len == 0)
        {
            results[i] = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
            if (results[i])
            {
                results[i]->error_code = TECC_CRYPTO_FORMAT_ERROR;
                snprintf(results[i]->error_msg, sizeof(results[i]->error_msg),
                         "Batch decrypt: item %zu has no RND data", i);
            }
            continue;
        }

        rnd_ciphertexts[i] = parsed_data.rnd_data;
        rnd_ciphertext_lens[i] = parsed_data.rnd_data_len;
        results[i] = NULL; /* Will be filled by batch decrypt below */
    }

    /* Batch SM4-RND decrypt: shared SM4 context, one key expansion */
    {
        unsigned char **plaintexts;
        size_t *plaintext_lens;
        const unsigned char **valid_ct;
        size_t *valid_ct_lens;
        size_t *idx_map;
        size_t valid_count;
        bool batch_ok;

        plaintexts = pg_malloc(sizeof(unsigned char *) * count);
        plaintext_lens = pg_malloc(sizeof(size_t) * count);
        valid_ct = pg_malloc(sizeof(const unsigned char *) * count);
        valid_ct_lens = pg_malloc(sizeof(size_t) * count);
        idx_map = pg_malloc(sizeof(size_t) * count);
        if (!plaintexts || !plaintext_lens || !valid_ct || !valid_ct_lens || !idx_map)
        {
            pg_free(batch_binary_base);
            tecc_key_manager_adapter_release_key(scheduler->key_adapter,
                                                 dek_key, dek_key_len);
            pg_free(results);
            pg_free(binary_data);
            pg_free(binary_lens);
            pg_free(rnd_ciphertexts);
            pg_free(rnd_ciphertext_lens);
            if (col_meta) tecc_free_encrypted_column_metadata(col_meta);
            if (!context_is_external) pg_free(context);
            return NULL;
        }

        /* Build list of valid items for batch decrypt.
         * If HSM is active for this column, skip software batch decrypt —
         * HSM-encrypted RND data uses raw [IV + SM4-CBC ciphertext] format,
         * which is incompatible with the software Sm4RndDataFormat wrapper
         * (version + auth_tag + IV + ct_len + ct) expected by the batch API.
         * Each HSM item is decrypted individually below. */
        valid_count = 0;
        for (i = 0; i < count; i++)
        {
            if (rnd_ciphertexts[i] != NULL && rnd_ciphertext_lens[i] > 0 &&
                results[i] == NULL)
            {
                if (context->hsm_kms_type != 0)
                {
                    /* HSM column: decrypt individually instead of batching.
                     * The ciphertext layout is [16-byte IV][SM4-CBC ciphertext]. */
                    HsmAdapterInterface *hsm = (HsmAdapterInterface *)context->hsm_adapter;
                    unsigned char sm4_key[TECC_SM4_KEY_SIZE];
                    size_t actual_key_len;
                    size_t iv_len = 16;
                    unsigned char *ct_buf;
                    HsmErrorCode hsm_ret;
                    size_t plain_len;

                    if (hsm == NULL || hsm->sm4_decrypt == NULL ||
                        context->hsm_device == NULL)
                    {
                        results[i] = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
                        if (results[i])
                        {
                            results[i]->error_code = TECC_CRYPTO_KEY_ERROR;
                            snprintf(results[i]->error_msg,
                                     sizeof(results[i]->error_msg),
                                     "Batch decrypt: item %zu HSM not available", i);
                        }
                        continue;
                    }

                    if (rnd_ciphertext_lens[i] <= iv_len)
                    {
                        results[i] = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
                        if (results[i])
                        {
                            results[i]->error_code = TECC_CRYPTO_DATA_ERROR;
                            snprintf(results[i]->error_msg,
                                     sizeof(results[i]->error_msg),
                                     "Batch decrypt: item %zu HSM data too short", i);
                        }
                        continue;
                    }

                    /* Copy key for HSM decrypt */
                    actual_key_len = (dek_key_len >= TECC_SM4_KEY_SIZE) ?
                                     TECC_SM4_KEY_SIZE : dek_key_len;
                    memcpy(sm4_key, dek_key, actual_key_len);

                    /* Allocate plaintext buffer */
                    plain_len = rnd_ciphertext_lens[i] - iv_len;
                    ct_buf = (unsigned char *)pg_malloc(plain_len);
                    if (!ct_buf)
                    {
                        results[i] = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
                        if (results[i])
                        {
                            results[i]->error_code = TECC_CRYPTO_MEMORY_ERROR;
                            snprintf(results[i]->error_msg,
                                     sizeof(results[i]->error_msg),
                                     "Batch decrypt: item %zu OOM", i);
                        }
                        continue;
                    }

                    hsm_ret = hsm->sm4_decrypt(
                        (HsmDevice *)context->hsm_device,
                        sm4_key, actual_key_len,
                        rnd_ciphertexts[i],              /* IV = first 16 bytes */
                        rnd_ciphertexts[i] + iv_len,     /* ciphertext after IV */
                        rnd_ciphertext_lens[i] - iv_len,
                        ct_buf, &plain_len);
                    memset(sm4_key, 0, sizeof(sm4_key));

                    if (hsm_ret == HSM_SUCCESS)
                    {
                        /* Strip PKCS#7 padding (except SWXA which does it internally) */
                        if (context->hsm_kms_type != 4 && plain_len > 0)
                        {
                            unsigned char pkcs7_pad = ct_buf[plain_len - 1];
                            if (pkcs7_pad > 0 && pkcs7_pad <= 16 &&
                                plain_len >= pkcs7_pad)
                            {
                                bool pad_valid = true;
                                size_t p;
                                for (p = plain_len - pkcs7_pad; p < plain_len; p++)
                                {
                                    if ((unsigned char)ct_buf[p] != pkcs7_pad)
                                    {
                                        pad_valid = false;
                                        break;
                                    }
                                }
                                if (pad_valid)
                                    plain_len -= pkcs7_pad;
                            }
                        }

                        results[i] = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
                        if (results[i])
                        {
                            results[i]->plaintext = (char *)ct_buf;
                            results[i]->plain_len = plain_len;
                            results[i]->error_code = TECC_CRYPTO_SUCCESS;
                        }
                        else
                        {
                            pg_free(ct_buf);
                        }
                    }
                    else
                    {
                        pg_free(ct_buf);
                        results[i] = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
                        if (results[i])
                        {
                            results[i]->error_code = TECC_CRYPTO_DATA_ERROR;
                            snprintf(results[i]->error_msg,
                                     sizeof(results[i]->error_msg),
                                     "Batch decrypt: item %zu HSM decrypt failed", i);
                        }
                    }
                }
                else
                {
                    /* Software column: add to batch for shared key expansion */
                    valid_ct[valid_count] = rnd_ciphertexts[i];
                    valid_ct_lens[valid_count] = rnd_ciphertext_lens[i];
                    idx_map[valid_count] = i;
                    valid_count++;
                }
            }
        }

        if (valid_count > 0)
        {
            ret = tecc_sm4_rnd_batch_decrypt_with_keys(
                derived_enc_key, derived_auth_key,
                valid_ct, valid_ct_lens, valid_count,
                plaintexts, plaintext_lens);
            batch_ok = (ret == 0); /* SM4_RND_SUCCESS == 0 */

            /* Build results from batch decrypt output */
            for (i = 0; i < valid_count; i++)
            {
                size_t orig_idx;

                orig_idx = idx_map[i];
                results[orig_idx] = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
                if (results[orig_idx] != NULL)
                {
                    if (batch_ok && plaintexts[i] != NULL)
                    {
                        results[orig_idx]->plaintext = (char *)plaintexts[i];
                        results[orig_idx]->plain_len = plaintext_lens[i];
                        results[orig_idx]->error_code = TECC_CRYPTO_SUCCESS;
                    }
                    else
                    {
                        results[orig_idx]->error_code = TECC_CRYPTO_DATA_ERROR;
                        snprintf(results[orig_idx]->error_msg,
                                 sizeof(results[orig_idx]->error_msg),
                                 "Batch decrypt: item %zu failed", orig_idx);
                    }
                }
                else if (batch_ok && plaintexts[i] != NULL)
                {
                    free(plaintexts[i]);
                }
            }
        }

        pg_free(plaintexts);
        pg_free(plaintext_lens);
        pg_free(valid_ct);
        pg_free(valid_ct_lens);
        pg_free(idx_map);
    }

    /* Cleanup */
    tecc_key_manager_adapter_release_key(scheduler->key_adapter,
                                         dek_key, dek_key_len);
    context->dek_key = NULL;
    context->dek_key_len = 0;
    pg_free(batch_binary_base);
    if (context_is_external)
    {
        context->rnd_ciphertext = NULL;
        context->det_ciphertext = NULL;
        context->ope_ciphertext = NULL;
        context->search_ciphertext = NULL;
    }
    else
    {
        if (!context_is_external) pg_free(context);
    }
    if (col_meta)
        tecc_free_encrypted_column_metadata(col_meta);
    pg_free(binary_data);
    pg_free(binary_lens);
    pg_free(rnd_ciphertexts);
    pg_free(rnd_ciphertext_lens);

    /* Update statistics */
    scheduler->decryption_count += count;

    return results;
}

/*
 * Public batch decrypt hex wrapper
 */
TeccUnifiedDecryptResult **
tecc_unified_batch_decrypt_hex(PGconn *conn,
                               const char **hex_strings,
                               size_t count,
                               const char *schema_name,
                               const char *table_name,
                               const char *column_name,
                               TeccCryptoContext *external_context)
{
    TeccCryptoScheduler *scheduler;

    if (conn == NULL || hex_strings == NULL || count == 0 ||
        table_name == NULL || column_name == NULL)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_unified_batch_decrypt_hex");
        return NULL;
    }

    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return NULL;
    }

    return tecc_unified_batch_decrypt_hex_internal_with_scheduler(
        conn, scheduler, hex_strings, count,
        schema_name, table_name, column_name, external_context);
}
/* End - ReqID:TECC4.0 */

/*
 * Unified encryption with collation support and connection parameter (thread-safe version)
 */
TeccUnifiedEncryptResult *
tecc_unified_encrypt_string_with_collation_and_conn(
    PGconn *conn,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name,
    Oid collation_oid,
    TeccCryptoContext *external_context)
{
    TeccCryptoScheduler *scheduler;

    /* Validate parameters */
    if (conn == NULL || plaintext == NULL || table_name == NULL || column_name == NULL)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_unified_encrypt_string_with_collation_and_conn");
        return NULL;
    }

    /* Get scheduler from connection state */
    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return NULL;
    }

    /* Call internal implementation with scheduler */
    return tecc_unified_encrypt_string_internal_with_scheduler(scheduler,
                                                              plaintext,
                                                              table_name,
                                                              column_name,
                                                              kek_name,
                                                              dek_name,
                                                              collation_oid,
                                                              tecc_get_state(conn),
                                                              NULL,
                                                              external_context);
}

/* ==================== Batch Processing Implementation ==================== */
/* Begin - ReqID:TECC4.0 - Batch processing implementation for import/export module */


/*
 * Batch encryption interface with connection parameter (thread-safe version)
 */
TeccBatchEncryptResult *
tecc_unified_batch_encrypt_strings_with_conn(
    PGconn *conn,
    const char **plaintexts,
    const size_t *plaintext_lengths,
    size_t count,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    TeccCryptoScheduler *scheduler;
    TeccBatchEncryptResult *batch_result = NULL;
    size_t i;
    unsigned char *dek_key = NULL;
    size_t dek_key_len = 0;
    unsigned char *derived_enc_key = NULL;    /* ReqID:TECC4.0 */
    unsigned char *derived_auth_key = NULL;   /* ReqID:TECC4.0 */
    int key_ret = TECC_CRYPTO_SUCCESS;
    size_t success_count = 0;
    size_t error_count = 0;

    /* Validate parameters */
    if (conn == NULL || !plaintexts || !plaintext_lengths || count == 0 || !table_name || !column_name)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_unified_batch_encrypt_strings_with_conn");
        return NULL;
    }

    /* Get scheduler from connection state */
    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return NULL;
    }

    TECC_SCHEDULER_DEBUG("Batch encryption: %s.%s (count=%zu)", table_name, column_name, count);

    /* Allocate batch result structure */
    batch_result = pg_malloc0(sizeof(TeccBatchEncryptResult));
    if (!batch_result)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate batch encrypt result");
        return NULL;
    }

    batch_result->count = count;

    /* Allocate arrays for individual results */
    batch_result->cipherdata_buffers = pg_malloc0(sizeof(TeccCryptoBuffer *) * count);
    batch_result->hex_strings = pg_malloc0(sizeof(char *) * count);
    batch_result->error_codes = pg_malloc0(sizeof(int) * count);

    if (!batch_result->cipherdata_buffers || !batch_result->hex_strings || !batch_result->error_codes)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate arrays for batch encrypt result");
        tecc_unified_free_batch_encrypt_result(batch_result);
        return NULL;
    }

    /* Initialize error codes to success */
    for (i = 0; i < count; i++)
    {
        batch_result->error_codes[i] = TECC_CRYPTO_SUCCESS;
    }

    /* Preload DEK key for batch optimization */
    dek_key = NULL;
    dek_key_len = 0;
    key_ret = TECC_CRYPTO_SUCCESS;

    if (dek_name)
    {
        key_ret = tecc_key_manager_adapter_get_dek_key_ext(scheduler->key_adapter,
                                                      dek_name,
                                                      &dek_key, &dek_key_len,
                                                      &derived_enc_key,
                                                      &derived_auth_key);
        if (key_ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_WARN("Failed to preload DEK key '%s' for batch encryption, will load individually", dek_name);
            dek_key = NULL;
            dek_key_len = 0;
            derived_enc_key = NULL;
            derived_auth_key = NULL;
        }
        else
        {
            TECC_SCHEDULER_DEBUG("Preloaded DEK key for batch encryption: %s (len=%zu)", dek_name, dek_key_len);
        }
    }

    /* Process each item in the batch */
    for (i = 0; i < count; i++)
    {
        TeccUnifiedEncryptResult *single_result = NULL;
        const char *plaintext = plaintexts[i];

        /* Validate current item */
        if (!plaintext)
        {
            batch_result->error_codes[i] = TECC_CRYPTO_INVALID_PARAM;
            if (batch_result->overall_error_code == TECC_CRYPTO_SUCCESS)
            {
                snprintf(batch_result->overall_error_msg, sizeof(batch_result->overall_error_msg),
                         "Item %zu: NULL plaintext", i);
                batch_result->overall_error_code = TECC_CRYPTO_INVALID_PARAM;
            }
            continue;
        }

        /* Perform single encryption using batch-optimized function */
        single_result = tecc_unified_encrypt_string_with_preloaded_key(scheduler,
            plaintext, table_name, column_name, kek_name, dek_name,
            dek_key, dek_key_len,
            derived_enc_key, derived_auth_key);

        if (single_result && single_result->error_code == TECC_CRYPTO_SUCCESS)
        {
            /* Store successful result */
            batch_result->cipherdata_buffers[i] = single_result->cipherdata_buffer;
            batch_result->hex_strings[i] = single_result->hex_string;
            batch_result->error_codes[i] = TECC_CRYPTO_SUCCESS;

            /* Transfer ownership of buffer and hex string to batch result */
            single_result->cipherdata_buffer = NULL;
            single_result->hex_string = NULL;
        }
        else
        {
            /* Record error */
            if (single_result)
            {
                batch_result->error_codes[i] = single_result->error_code;
                if (batch_result->overall_error_code == TECC_CRYPTO_SUCCESS)
                {
                    snprintf(batch_result->overall_error_msg, sizeof(batch_result->overall_error_msg),
                             "Item %zu: %s", i, single_result->error_msg);
                    batch_result->overall_error_code = single_result->error_code;
                }

                /* Free resources if encryption failed */
                if (single_result->cipherdata_buffer)
                {
                    pg_free(single_result->cipherdata_buffer);
                    single_result->cipherdata_buffer = NULL;
                }
                if (single_result->hex_string)
                {
                    pg_free(single_result->hex_string);
                    single_result->hex_string = NULL;
                }
            }
            else
            {
                batch_result->error_codes[i] = TECC_CRYPTO_SYSTEM_ERROR;
                if (batch_result->overall_error_code == TECC_CRYPTO_SUCCESS)
                {
                    snprintf(batch_result->overall_error_msg, sizeof(batch_result->overall_error_msg),
                             "Item %zu: Encryption failed", i);
                    batch_result->overall_error_code = TECC_CRYPTO_SYSTEM_ERROR;
                }
            }
        }

        /* Free single result structure (but not the transferred buffers) */
        if (single_result)
        {
            pg_free(single_result);
        }
    }

    /* Calculate success and error counts for statistics and logging */
    success_count = 0;
    error_count = 0;
    for (i = 0; i < count; i++)
    {
        if (batch_result->error_codes[i] == TECC_CRYPTO_SUCCESS)
        {
            success_count++;
        }
        else
        {
            error_count++;
        }
    }

    /* Update statistics */
    if (scheduler)
    {
        scheduler->encryption_count += success_count;
        scheduler->error_count += error_count;

        /* Update batch-specific statistics */
        scheduler->batch_encryption_count++;
        scheduler->total_batch_items_processed += count;
        scheduler->successful_batch_items += success_count;
        scheduler->batch_error_count += error_count;
    }

    TECC_SCHEDULER_DEBUG("Batch encryption completed (success=%zu/%zu)",
                         success_count, count);

    /* Release preloaded DEK key if it was loaded */
    if (dek_key && dek_key_len > 0)
    {
        tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
        dek_key = NULL;
        dek_key_len = 0;
    }

    return batch_result;
}

/*
 * Internal batch decryption implementation with explicit scheduler parameter
 */
static TeccBatchDecryptResult *
tecc_unified_batch_decrypt_hex_strings_internal(
    TeccCryptoScheduler *scheduler,
    const char **hex_strings,
    const size_t *hex_lengths,
    size_t count,
    const char *table_name,
    const char *column_name)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    TeccBatchDecryptResult *batch_result = NULL;
    size_t i;
    size_t success_count = 0;
    size_t error_count = 0;

    /* Validate parameters */
    if (!scheduler || !hex_strings || !hex_lengths || count == 0 || !table_name || !column_name)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_unified_batch_decrypt_hex_strings_internal");
        return NULL;
    }

    TECC_SCHEDULER_DEBUG("Batch decryption internal: %s.%s (count=%zu)", table_name, column_name, count);

    /* Allocate batch result structure */
    batch_result = pg_malloc0(sizeof(TeccBatchDecryptResult));
    if (!batch_result)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate batch decrypt result");
        return NULL;
    }

    batch_result->count = count;
    batch_result->overall_error_code = TECC_CRYPTO_SUCCESS;
    batch_result->overall_error_msg[0] = '\0';

    /* Allocate arrays for individual results */
    batch_result->plaintexts = pg_malloc0(sizeof(char *) * count);
    batch_result->plaintext_lengths = pg_malloc0(sizeof(size_t) * count);
    batch_result->error_codes = pg_malloc0(sizeof(int) * count);

    if (!batch_result->plaintexts || !batch_result->plaintext_lengths || !batch_result->error_codes)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate arrays for batch decrypt result");
        tecc_unified_free_batch_decrypt_result(batch_result);
        return NULL;
    }

    /* Initialize error codes to success */
    for (i = 0; i < count; i++)
    {
        batch_result->error_codes[i] = TECC_CRYPTO_SUCCESS;
        batch_result->plaintext_lengths[i] = 0;
    }

    /* Process each item in the batch */
    for (i = 0; i < count; i++)
    {
        TeccUnifiedDecryptResult *single_result = NULL;
        const char *hex_string = hex_strings[i];
        /* Validate current item */
        if (!hex_string)
        {
            batch_result->error_codes[i] = TECC_CRYPTO_INVALID_PARAM;
            if (batch_result->overall_error_code == TECC_CRYPTO_SUCCESS)
            {
                snprintf(batch_result->overall_error_msg, sizeof(batch_result->overall_error_msg),
                         "Item %zu: NULL hex string", i);
                batch_result->overall_error_code = TECC_CRYPTO_INVALID_PARAM;
            }
            continue;
        }

        /* Optional: validate length if provided */
        if (hex_lengths[i] > 0)
        {
            size_t actual_len = strlen(hex_string);
            if (actual_len != hex_lengths[i])
            {
                batch_result->error_codes[i] = TECC_CRYPTO_INVALID_PARAM;
                if (batch_result->overall_error_code == TECC_CRYPTO_SUCCESS)
                {
                    snprintf(batch_result->overall_error_msg, sizeof(batch_result->overall_error_msg),
                             "Item %zu: Hex length mismatch (expected %zu, got %zu)",
                             i, hex_lengths[i], actual_len);
                    batch_result->overall_error_code = TECC_CRYPTO_INVALID_PARAM;
                }
                continue;
            }
        }

        /* Perform single decryption with scheduler */
        /* Note: conn is NULL for internal batch processing */
        single_result = tecc_unified_decrypt_hex_internal_with_scheduler(NULL,
                                                                        scheduler,
                                                                        hex_string,
                                                                        table_name,
                                                                        column_name,
                                                                        NULL);

        if (single_result && single_result->error_code == TECC_CRYPTO_SUCCESS)
        {
            /* Store successful result */
            batch_result->plaintexts[i] = single_result->plaintext;
            batch_result->plaintext_lengths[i] = single_result->plain_len;
            batch_result->error_codes[i] = TECC_CRYPTO_SUCCESS;

            /* Transfer ownership of plaintext to batch result */
            single_result->plaintext = NULL;

            /* Success will be counted in batch statistics at the end */
        }
        else
        {
            /* Record error */
            if (single_result)
            {
                batch_result->error_codes[i] = single_result->error_code;
                if (batch_result->overall_error_code == TECC_CRYPTO_SUCCESS)
                {
                    snprintf(batch_result->overall_error_msg, sizeof(batch_result->overall_error_msg),
                             "Item %zu: %s", i, single_result->error_msg);
                    batch_result->overall_error_code = single_result->error_code;
                }

                /* Free plaintext if it was allocated but decryption failed */
                if (single_result->plaintext)
                {
                    /* Securely wipe plaintext data before freeing */
                    /* Note: plain_len may not be set correctly on decryption failure */
                    /* Use a safe default or check if plain_len is valid */
                    if (single_result->plain_len > 0 && single_result->plain_len <= 65536) /* Reasonable upper bound */
                        memset(single_result->plaintext, 0, single_result->plain_len);
                    else
                        memset(single_result->plaintext, 0, 256); /* Safe default */
                    pg_free(single_result->plaintext);
                    single_result->plaintext = NULL;
                }
            }
            else
            {
                batch_result->error_codes[i] = TECC_CRYPTO_BATCH_ERROR;
                if (batch_result->overall_error_code == TECC_CRYPTO_SUCCESS)
                {
                    snprintf(batch_result->overall_error_msg, sizeof(batch_result->overall_error_msg),
                             "Item %zu: Decryption failed", i);
                    batch_result->overall_error_code = TECC_CRYPTO_BATCH_ERROR;
                }
            }
        }

        /* Free single result structure */
        if (single_result)
        {
            pg_free(single_result);
        }
    }

    /* Calculate actual success count */
    success_count = 0;
    error_count = 0;
    for (i = 0; i < count; i++)
    {
        if (batch_result->error_codes[i] == TECC_CRYPTO_SUCCESS)
        {
            success_count++;
        }
        else
        {
            error_count++;
        }
    }

    /* Update statistics */
    scheduler->decryption_count += success_count;
    scheduler->error_count += error_count;

    /* Update batch-specific statistics */
    scheduler->batch_decryption_count++;
    scheduler->total_batch_items_processed += count;
    scheduler->successful_batch_items += success_count;
    scheduler->batch_error_count += error_count;

    TECC_SCHEDULER_DEBUG("Batch decryption completed (success=%zu/%zu)",
                         success_count, count);

    return batch_result;
}

/*
 * Batch decryption interface
 */
TeccBatchDecryptResult *
tecc_unified_batch_decrypt_hex_strings(
    const char **hex_strings,
    const size_t *hex_lengths,
    size_t count,
    const char *table_name,
    const char *column_name)
{
    /* This function is deprecated - use tecc_unified_batch_decrypt_hex_strings_with_conn instead */
    TECC_SCHEDULER_ERROR("tecc_unified_batch_decrypt_hex_strings is deprecated, use tecc_unified_batch_decrypt_hex_strings_with_conn with connection parameter");
    return NULL;
}
TeccBatchDecryptResult *
tecc_unified_batch_decrypt_hex_strings_with_conn(
    PGconn *conn,
    const char **hex_strings,
    const size_t *hex_lengths,
    size_t count,
    const char *table_name,
    const char *column_name)
{
    TeccCryptoScheduler *scheduler;

    /* Validate parameters */
    if (conn == NULL || !hex_strings || !hex_lengths || count == 0 || !table_name || !column_name)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_unified_batch_decrypt_hex_strings_with_conn");
        return NULL;
    }

    /* Get scheduler from connection state */
    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return NULL;
    }

    /* Use the internal implementation with scheduler */
    return tecc_unified_batch_decrypt_hex_strings_internal(scheduler, hex_strings, hex_lengths, count,
                                                          table_name, column_name);
}

/*
 * Free batch encryption result
 */
void
tecc_unified_free_batch_encrypt_result(TeccBatchEncryptResult *result)
{
    size_t i;

    if (!result)
        return;

    /* Free individual cipherdata buffers and hex strings */
    if (result->cipherdata_buffers)
    {
        for (i = 0; i < result->count; i++)
        {
            if (result->cipherdata_buffers[i])
            {
                /* Free the buffer data if it exists */
                if (result->cipherdata_buffers[i]->data)
                {
                    pg_free(result->cipherdata_buffers[i]->data);
                }
                pg_free(result->cipherdata_buffers[i]);
            }
        }
        pg_free(result->cipherdata_buffers);
    }

    if (result->hex_strings)
    {
        for (i = 0; i < result->count; i++)
        {
            if (result->hex_strings[i])
            {
                pg_free(result->hex_strings[i]);
            }
        }
        pg_free(result->hex_strings);
    }

    if (result->error_codes)
    {
        pg_free(result->error_codes);
    }

    /* Free the result structure itself */
    pg_free(result);
}

/*
 * Free batch decryption result
 */
void
tecc_unified_free_batch_decrypt_result(TeccBatchDecryptResult *result)
{
    size_t i;

    if (!result)
        return;

    /* Free individual plaintexts */
    if (result->plaintexts)
    {
        for (i = 0; i < result->count; i++)
        {
            if (result->plaintexts[i])
            {
                /* Securely wipe plaintext data before freeing */
                if (result->plaintext_lengths && result->plaintext_lengths[i] > 0)
                    memset(result->plaintexts[i], 0, result->plaintext_lengths[i]);
                pg_free(result->plaintexts[i]);
            }
        }
        pg_free(result->plaintexts);
    }

    if (result->plaintext_lengths)
    {
        pg_free(result->plaintext_lengths);
    }

    if (result->error_codes)
    {
        pg_free(result->error_codes);
    }

    /* Free the result structure itself */
    pg_free(result);
}

/*
 * Get batch processing statistics (deprecated - use thread-safe version)
 */
int
tecc_get_batch_processing_stats(
    size_t *total_processed,
    size_t *success_count,
    size_t *error_count,
    double *average_time_per_item)
{
    /* This function is deprecated - it cannot get scheduler without connection */
    TECC_SCHEDULER_ERROR("tecc_get_batch_processing_stats is deprecated, use tecc_get_batch_processing_stats_with_conn with connection parameter");

    /* Return default values */
    if (total_processed)
        *total_processed = 0;
    if (success_count)
        *success_count = 0;
    if (error_count)
        *error_count = 0;
    if (average_time_per_item)
        *average_time_per_item = 0.0;

    return TECC_CRYPTO_SYSTEM_ERROR;
}

/*
 * Get batch processing statistics with connection parameter (thread-safe version)
 */
int
tecc_get_batch_processing_stats_with_conn(
    PGconn *conn,
    size_t *total_processed,
    size_t *success_count,
    size_t *error_count,
    double *average_time_per_item)
{
    TeccCryptoScheduler *scheduler;

    /* Validate parameters */
    if (conn == NULL)
    {
        TECC_SCHEDULER_ERROR("NULL connection passed to tecc_get_batch_processing_stats_with_conn");
        return TECC_CRYPTO_INVALID_PARAM;
    }

    /* Get scheduler from connection */
    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return TECC_CRYPTO_SYSTEM_ERROR;
    }

    /* Return batch-specific statistics from scheduler */
    if (total_processed)
        *total_processed = scheduler->total_batch_items_processed;
    if (success_count)
        *success_count = scheduler->successful_batch_items;
    if (error_count)
        *error_count = scheduler->batch_error_count;
    if (average_time_per_item)
        *average_time_per_item = 0.0;  /* Not implemented yet - would require timing data */

    TECC_SCHEDULER_DEBUG("Batch statistics: total=%zu, success=%zu, errors=%d, batch_ops=%d",
                         scheduler->total_batch_items_processed,
                         scheduler->successful_batch_items,
                         scheduler->batch_error_count,
                         scheduler->batch_encryption_count + scheduler->batch_decryption_count);

    return TECC_CRYPTO_SUCCESS;
}

/*
 * Get detailed batch error statistics
 */
int
tecc_get_batch_error_details(
    PGconn *conn,
    int *total_errors,
    int *invalid_param_errors,
    int *memory_errors,
    int *format_errors,
    int *system_errors,
    int *batch_errors)
{
    TeccCryptoScheduler *scheduler;

    /* Validate parameters */
    if (conn == NULL)
    {
        TECC_SCHEDULER_ERROR("NULL connection passed to tecc_get_batch_error_details");
        return TECC_CRYPTO_INVALID_PARAM;
    }

    /* Get scheduler from connection */
    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return TECC_CRYPTO_SYSTEM_ERROR;
    }

    /* Return batch-specific error statistics */
    if (total_errors)
        *total_errors = scheduler->batch_error_count;
    if (invalid_param_errors)
        *invalid_param_errors = 0;  /* Not tracked separately yet */
    if (memory_errors)
        *memory_errors = 0;         /* Not tracked separately yet */
    if (format_errors)
        *format_errors = 0;         /* Not tracked separately yet */
    if (system_errors)
        *system_errors = 0;         /* Not tracked separately yet */
    if (batch_errors)
        *batch_errors = scheduler->batch_error_count;  /* Same as total for now */

    TECC_SCHEDULER_DEBUG("Batch error details: total=%d, batch_ops=%d",
                         scheduler->batch_error_count,
                         scheduler->batch_encryption_count + scheduler->batch_decryption_count);

    return TECC_CRYPTO_SUCCESS;
}

/*
 * Optimize batch size based on performance metrics
 */
size_t
tecc_optimize_batch_size(
    size_t data_size,
    int algorithm_type,
    size_t available_memory)
{
    size_t optimal_batch_size;
    size_t estimated_memory_per_item = 0;

    /* Basic optimization logic */
    if (data_size == 0)
        return 100;  /* Default batch size for small data */

    /* Estimate memory usage per item */

    switch (algorithm_type)
    {
        case TECC_ALGO_RND:
            estimated_memory_per_item = data_size + 64;  /* SM4-CBC with padding and HMAC */
            break;
        case TECC_ALGO_DET:
            estimated_memory_per_item = data_size + 16;  /* Deterministic encryption */
            break;
        case TECC_ALGO_OPE:
            estimated_memory_per_item = data_size + 32;  /* Order-preserving encryption */
            break;
        case TECC_ALGO_SEARCH:
            estimated_memory_per_item = data_size * 2;   /* Searchable encryption */
            break;
        default:
            estimated_memory_per_item = data_size + 128; /* Conservative estimate */
    }

    /* Calculate optimal batch size based on available memory */
    if (estimated_memory_per_item == 0)
        return 100;

    optimal_batch_size = available_memory / estimated_memory_per_item;

    /* Apply limits */
    if (optimal_batch_size < 10)
        optimal_batch_size = 10;
    if (optimal_batch_size > 1000)
        optimal_batch_size = 1000;

    return optimal_batch_size;
}

/*
 * Smart batch size optimization with performance history
 */
size_t
tecc_smart_optimize_batch_size(
    PGconn *conn,
    size_t data_size,
    int algorithm_type,
    size_t available_memory)
{
    TeccCryptoScheduler *scheduler;
    size_t base_batch_size;
    size_t adjusted_batch_size;

    /* Validate parameters */
    if (conn == NULL)
    {
        TECC_SCHEDULER_ERROR("NULL connection passed to tecc_smart_optimize_batch_size");
        return tecc_optimize_batch_size(data_size, algorithm_type, available_memory);
    }

    /* Get scheduler from connection */
    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return tecc_optimize_batch_size(data_size, algorithm_type, available_memory);
    }

    /* Get base batch size from basic optimization */
    base_batch_size = tecc_optimize_batch_size(data_size, algorithm_type, available_memory);

    /* Adjust based on batch error rate if we have enough batch statistics */
    if (scheduler->total_batch_items_processed > 100)
    {
        double batch_error_rate = (double)scheduler->batch_error_count / scheduler->total_batch_items_processed;

        /* Reduce batch size if batch error rate is high */
        if (batch_error_rate > 0.1)  /* 10% error rate */
        {
            adjusted_batch_size = base_batch_size / 2;
            TECC_SCHEDULER_DEBUG("High batch error rate (%.2f%%), reducing batch size from %zu to %zu",
                                batch_error_rate * 100, base_batch_size, adjusted_batch_size);
        }
        else if (batch_error_rate > 0.05)  /* 5% error rate */
        {
            adjusted_batch_size = base_batch_size * 3 / 4;
            TECC_SCHEDULER_DEBUG("Moderate batch error rate (%.2f%%), reducing batch size from %zu to %zu",
                                batch_error_rate * 100, base_batch_size, adjusted_batch_size);
        }
        else
        {
            /* Low batch error rate, can increase batch size */
            adjusted_batch_size = base_batch_size * 5 / 4;
            TECC_SCHEDULER_DEBUG("Low batch error rate (%.2f%%), increasing batch size from %zu to %zu",
                                batch_error_rate * 100, base_batch_size, adjusted_batch_size);
        }
    }
    else
    {
        /* Not enough batch statistics, use base batch size */
        adjusted_batch_size = base_batch_size;
        TECC_SCHEDULER_DEBUG("Insufficient batch statistics, using base batch size: %zu", base_batch_size);
    }

    /* Apply limits */
    if (adjusted_batch_size < 10)
        adjusted_batch_size = 10;
    if (adjusted_batch_size > 1000)
        adjusted_batch_size = 1000;

    TECC_SCHEDULER_DEBUG("Smart batch size optimization: data_size=%zu, algo=%d, mem=%zu -> batch=%zu",
                        data_size, algorithm_type, available_memory, adjusted_batch_size);

    return adjusted_batch_size;
}

/* End - ReqID:TECC4.0 - Batch processing implementation for import/export module */
/* End - ReqID:TECC4.0 */
/* ==================== Stream Processing Implementation ==================== */
/* Begin - ReqID:TECC4.0 - Stream processing implementation for import/export module */

/*
 * Create stream crypto context
 */
TeccStreamCryptoContext *
tecc_create_stream_crypto_context(
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name)
{
    TeccCryptoScheduler *scheduler = NULL;

    /* Validate parameters */
    if (!table_name || !column_name || !kek_name || !dek_name)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_create_stream_crypto_context");
        return NULL;
    }

    /* Check if scheduler is available */
    if (!scheduler)
    {
        TECC_SCHEDULER_ERROR("Crypto scheduler not initialized");
        return NULL;
    }

    /* Call internal implementation with global scheduler */
    return tecc_create_stream_crypto_context_internal(scheduler, table_name, column_name, kek_name, dek_name);
}

/*
 * Internal stream crypto context creation with explicit scheduler
 */
static TeccStreamCryptoContext *
tecc_create_stream_crypto_context_internal(
    TeccCryptoScheduler *scheduler,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name)
{
    TeccStreamCryptoContext *context = NULL;

    /* Validate parameters */
    if (!scheduler || !table_name || !column_name || !kek_name || !dek_name)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_create_stream_crypto_context_internal");
        return NULL;
    }

    /* Allocate context */
    context = (TeccStreamCryptoContext *)pg_malloc(sizeof(TeccStreamCryptoContext));
    if (!context)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate stream crypto context");
        return NULL;
    }

    /* Initialize context */
    memset(context, 0, sizeof(TeccStreamCryptoContext));

    /* Store scheduler reference */
    context->scheduler = scheduler;

    /* Create base crypto context */
    context->base_context = tecc_create_crypto_context_with_pool(
        table_name, column_name, kek_name, dek_name);
    if (!context->base_context)
    {
        pg_free(context);
        TECC_SCHEDULER_ERROR("Failed to create base crypto context");
        return NULL;
    }

    /* Initialize other fields */
    context->processed_count = 0;
    context->initialized = true;
    context->algorithm_type = TECC_ALGO_RND; /* Default algorithm: RND (SM4-CBC with HMAC) */
    context->collation_oid = InvalidOid; /* Default: no collation */
    context->operator_type = TECC_OP_EQUAL; /* Default operator */

    /* Initialize performance statistics */
    gettimeofday(&context->start_time, NULL);
    gettimeofday(&context->last_operation_time, NULL);
    context->total_encrypted_bytes = 0;
    context->total_decrypted_bytes = 0;
    context->encryption_count = 0;
    context->decryption_count = 0;
    context->batch_operations = 0;
    context->single_operations = 0;
    context->avg_encryption_time = 0.0;
    context->avg_decryption_time = 0.0;
    context->memory_saved = 0;

    /* Create memory pool for stream processing */
    context->stream_pool = tecc_stream_memory_pool_create(64); /* Default: 64 buffers */
    if (!context->stream_pool)
    {
        TECC_SCHEDULER_WARNING("Failed to create memory pool for stream context, continuing without pool");
        /* Continue without memory pool - not a fatal error */
    }

    /* Allocate key ID */
    if (dek_name)
    {
        context->key_id = pg_malloc(strlen(dek_name) + 1);
        if (context->key_id)
        {
            strcpy(context->key_id, dek_name);
        }
    }

    TECC_SCHEDULER_DEBUG("Created stream crypto context for table '%s', column '%s' with scheduler %p",
                        table_name, column_name, scheduler);

    return context;
}

/*
 * Create stream crypto context with connection parameter (thread-safe version)
 */
TeccStreamCryptoContext *
tecc_create_stream_crypto_context_with_conn(
    PGconn *conn,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name)
{
    TeccCryptoScheduler *scheduler;

    /* Validate parameters */
    if (conn == NULL || !table_name || !column_name || !kek_name || !dek_name)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_create_stream_crypto_context_with_conn");
        return NULL;
    }

    /* Get scheduler from connection state */
    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return NULL;
    }

    /* Create context with connection-specific scheduler */
    return tecc_create_stream_crypto_context_internal(scheduler, table_name, column_name, kek_name, dek_name);
}

/*
 * Stream encryption of single data item
 */
TeccUnifiedEncryptResult *
tecc_stream_encrypt_data(
    TeccStreamCryptoContext *context,
    const char *plaintext,
    size_t plaintext_len)
{
    TeccUnifiedEncryptResult *result = NULL;
    TeccCryptoScheduler *scheduler = NULL;
    struct timeval start_time, end_time;
    double elapsed_ms = 0.0;

    /* Validate parameters */
    if (!context || !context->initialized || !plaintext)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_stream_encrypt_data");
        return NULL;
    }

    /* Start timer for performance measurement */
    TECC_START_TIMER(start_time);

    /* Check if base context is available */
    if (!context->base_context)
    {
        TECC_SCHEDULER_ERROR("Base crypto context not available in stream context");
        return NULL;
    }

    /* Get scheduler from context, fall back to global scheduler if not set */
    scheduler = context->scheduler;
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Stream encryption requires connection-specific scheduler, global scheduler not available");
        return NULL;
    }

    /* Perform encryption based on algorithm type */
    switch (context->algorithm_type)
    {
        case TECC_ALGO_RND:
        case TECC_ALGO_DET:
        case TECC_ALGO_OPE:
            /* Use standard encryption with scheduler */
            result = tecc_unified_encrypt_string_internal_with_scheduler(
                scheduler,
                plaintext,
                context->base_context->table_name,
                context->base_context->column_name,
                context->base_context->kek_name,
                context->base_context->dek_name,
                context->collation_oid,
                NULL, NULL, NULL); /* tecc_state, col_meta_in, external_context not available in stream context */
            break;

        case TECC_ALGO_SEARCH:
            /* Use operator-based encryption with scheduler */
            /* Note: Stream context doesn't have connection, pass NULL */
            result = tecc_unified_encrypt_string_with_operator_internal_with_scheduler(
                NULL,  /* No connection available in stream context */
                scheduler,
                plaintext,
                context->base_context->table_name,
                context->base_context->column_name,
                context->base_context->kek_name,
                context->base_context->dek_name,
                context->operator_type);
            break;

        default:
            TECC_SCHEDULER_ERROR("Unsupported algorithm type %d in stream encryption",
                               context->algorithm_type);
            return NULL;
    }

    /* End timer and update performance statistics */
    TECC_END_TIMER(start_time, end_time, elapsed_ms);

    /* Update statistics */
    if (result && result->error_code == TECC_CRYPTO_SUCCESS)
    {
        context->processed_count++;

        /* Update performance statistics */
        tecc_stream_update_performance_stats(context, true, plaintext_len, elapsed_ms, false);
    }

    return result;
}

/*
 * Stream decryption of single data item
 */
TeccUnifiedDecryptResult *
tecc_stream_decrypt_data(
    TeccStreamCryptoContext *context,
    const char *hex_string,
    size_t hex_len)
{
    TeccUnifiedDecryptResult *result = NULL;
    TeccCryptoScheduler *scheduler = NULL;
    struct timeval start_time, end_time;
    double elapsed_ms = 0.0;

    /* Validate parameters */
    if (!context || !context->initialized || !hex_string)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_stream_decrypt_data");
        return NULL;
    }

    /* Start timer for performance measurement */
    TECC_START_TIMER(start_time);

    /* Check if base context is available */
    if (!context->base_context)
    {
        TECC_SCHEDULER_ERROR("Base crypto context not available in stream context");
        return NULL;
    }

    /* Get scheduler from context, fall back to global scheduler if not set */
    scheduler = context->scheduler;
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Stream decryption requires connection-specific scheduler, global scheduler not available");
        return NULL;
    }

    /* Perform decryption with scheduler */
    /* Note: conn is NULL for stream processing */
    result = tecc_unified_decrypt_hex_internal_with_scheduler(
        NULL,
        scheduler,
        hex_string,
        context->base_context->table_name,
        context->base_context->column_name,
        NULL);

    /* End timer and update performance statistics */
    TECC_END_TIMER(start_time, end_time, elapsed_ms);

    /* Update statistics */
    if (result && result->error_code == TECC_CRYPTO_SUCCESS)
    {
        context->processed_count++;

        /* Update performance statistics */
        tecc_stream_update_performance_stats(context, false, hex_len, elapsed_ms, false);
    }

    return result;
}

/*
 * Batch encryption using stream context
 */
TeccBatchEncryptResult *
tecc_stream_batch_encrypt_data(
    TeccStreamCryptoContext *context,
    const char **plaintexts,
    const size_t *plaintext_lengths,
    size_t count)
{
    TeccBatchEncryptResult *batch_result = NULL;
    size_t i;
    struct timeval start_time, end_time;
    double elapsed_ms = 0.0;
    size_t total_data_size = 0;

    /* Validate parameters */
    if (!context || !context->initialized || !plaintexts || !plaintext_lengths || count == 0)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_stream_batch_encrypt_data");
        return NULL;
    }

    /* Start timer for performance measurement */
    TECC_START_TIMER(start_time);

    /* Calculate total data size for performance statistics */
    for (i = 0; i < count; i++)
    {
        total_data_size += plaintext_lengths[i];
    }

    /* Check if base context is available */
    if (!context->base_context)
    {
        TECC_SCHEDULER_ERROR("Base crypto context not available in stream context");
        return NULL;
    }

    /* Allocate batch result */
    batch_result = (TeccBatchEncryptResult *)pg_malloc(sizeof(TeccBatchEncryptResult));
    if (!batch_result)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate batch encrypt result");
        return NULL;
    }

    memset(batch_result, 0, sizeof(TeccBatchEncryptResult));
    batch_result->count = count;
    batch_result->overall_error_code = TECC_CRYPTO_SUCCESS;

    /* Allocate and initialize arrays using memory pool if available */
    if (context->stream_pool)
    {
        /* Try to allocate arrays from memory pool */
        batch_result->cipherdata_buffers = (TeccCryptoBuffer **)
            tecc_stream_memory_pool_alloc(context->stream_pool, count * sizeof(TeccCryptoBuffer *));
        batch_result->hex_strings = (char **)
            tecc_stream_memory_pool_alloc(context->stream_pool, count * sizeof(char *));
        batch_result->error_codes = (int *)
            tecc_stream_memory_pool_alloc(context->stream_pool, count * sizeof(int));
    }
    else
    {
        /* Fall back to regular allocation */
        batch_result->cipherdata_buffers = (TeccCryptoBuffer **)pg_malloc0(count * sizeof(TeccCryptoBuffer *));
        batch_result->hex_strings = (char **)pg_malloc0(count * sizeof(char *));
        batch_result->error_codes = (int *)pg_malloc0(count * sizeof(int));
    }

    if (!batch_result->cipherdata_buffers || !batch_result->hex_strings || !batch_result->error_codes)
    {
        tecc_unified_free_batch_encrypt_result(batch_result);
        TECC_SCHEDULER_ERROR("Failed to allocate arrays for batch encrypt result");
        return NULL;
    }
    memset(batch_result->error_codes, 0, count * sizeof(int));

    /* Process each item */
    for (i = 0; i < count; i++)
    {
        TeccUnifiedEncryptResult *single_result;

        single_result = tecc_stream_encrypt_data(context, plaintexts[i], plaintext_lengths[i]);

        if (single_result)
        {
            batch_result->cipherdata_buffers[i] = single_result->cipherdata_buffer;
            batch_result->hex_strings[i] = single_result->hex_string;
            batch_result->error_codes[i] = single_result->error_code;

            /* Free result structure but keep buffers */
            single_result->cipherdata_buffer = NULL;
            single_result->hex_string = NULL;
            tecc_unified_free_encrypt_result(single_result);

            if (batch_result->error_codes[i] != TECC_CRYPTO_SUCCESS)
            {
                batch_result->overall_error_code = TECC_CRYPTO_BATCH_ERROR;
                snprintf(batch_result->overall_error_msg, sizeof(batch_result->overall_error_msg),
                        "Batch encryption failed at item %zu", i);
            }
        }
        else
        {
            batch_result->error_codes[i] = TECC_CRYPTO_BATCH_ERROR;
            batch_result->overall_error_code = TECC_CRYPTO_BATCH_ERROR;
            snprintf(batch_result->overall_error_msg, sizeof(batch_result->overall_error_msg),
                    "Batch encryption failed at item %zu (null result)", i);
        }
    }

    /* End timer and update performance statistics */
    TECC_END_TIMER(start_time, end_time, elapsed_ms);

    /* Update processed count */
    context->processed_count += count;

    /* Update performance statistics */
    tecc_stream_update_performance_stats(context, true, total_data_size, elapsed_ms, true);

    return batch_result;
}

/*
 * Batch decryption using stream context
 */
TeccBatchDecryptResult *
tecc_stream_batch_decrypt_data(
    TeccStreamCryptoContext *context,
    const char **hex_strings,
    const size_t *hex_lengths,
    size_t count)
{
    TeccBatchDecryptResult *batch_result = NULL;
    size_t i;
    struct timeval start_time, end_time;
    double elapsed_ms = 0.0;
    size_t total_data_size = 0;

    /* Validate parameters */
    if (!context || !context->initialized || !hex_strings || !hex_lengths || count == 0)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_stream_batch_decrypt_data");
        return NULL;
    }

    /* Start timer for performance measurement */
    TECC_START_TIMER(start_time);

    /* Calculate total data size for performance statistics */
    for (i = 0; i < count; i++)
    {
        total_data_size += hex_lengths[i];
    }

    /* Check if base context is available */
    if (!context->base_context)
    {
        TECC_SCHEDULER_ERROR("Base crypto context not available in stream context");
        return NULL;
    }

    /* Allocate batch result */
    batch_result = (TeccBatchDecryptResult *)pg_malloc(sizeof(TeccBatchDecryptResult));
    if (!batch_result)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate batch decrypt result");
        return NULL;
    }

    memset(batch_result, 0, sizeof(TeccBatchDecryptResult));
    batch_result->count = count;
    batch_result->overall_error_code = TECC_CRYPTO_SUCCESS;

    /* Allocate and initialize arrays */
    batch_result->plaintexts = (char **)pg_malloc0(count * sizeof(char *));
    batch_result->plaintext_lengths = (size_t *)pg_malloc0(count * sizeof(size_t));
    batch_result->error_codes = (int *)pg_malloc0(count * sizeof(int));

    if (!batch_result->plaintexts || !batch_result->plaintext_lengths || !batch_result->error_codes)
    {
        tecc_unified_free_batch_decrypt_result(batch_result);
        TECC_SCHEDULER_ERROR("Failed to allocate arrays for batch decrypt result");
        return NULL;
    }
    memset(batch_result->error_codes, 0, count * sizeof(int));

    /* Process each item */
    for (i = 0; i < count; i++)
    {
        TeccUnifiedDecryptResult *single_result;

        single_result = tecc_stream_decrypt_data(context, hex_strings[i], hex_lengths[i]);

        if (single_result)
        {
            batch_result->plaintexts[i] = single_result->plaintext;
            batch_result->plaintext_lengths[i] = single_result->plain_len;
            batch_result->error_codes[i] = single_result->error_code;

            /* Free result structure but keep plaintext */
            single_result->plaintext = NULL;
            tecc_unified_free_decrypt_result(single_result);

            if (batch_result->error_codes[i] != TECC_CRYPTO_SUCCESS)
            {
                batch_result->overall_error_code = TECC_CRYPTO_BATCH_ERROR;
                snprintf(batch_result->overall_error_msg, sizeof(batch_result->overall_error_msg),
                        "Batch decryption failed at item %zu", i);
            }
        }
        else
        {
            batch_result->error_codes[i] = TECC_CRYPTO_BATCH_ERROR;
            batch_result->overall_error_code = TECC_CRYPTO_BATCH_ERROR;
            snprintf(batch_result->overall_error_msg, sizeof(batch_result->overall_error_msg),
                    "Batch decryption failed at item %zu (null result)", i);
        }
    }

    /* End timer and update performance statistics */
    TECC_END_TIMER(start_time, end_time, elapsed_ms);

    /* Update processed count */
    context->processed_count += count;

    /* Update performance statistics */
    tecc_stream_update_performance_stats(context, false, total_data_size, elapsed_ms, true);

    return batch_result;
}

/*
 * Set algorithm type for stream context
 */
int
tecc_stream_set_algorithm_type(
    TeccStreamCryptoContext *context,
    int algorithm_type)
{
    /* Validate parameters */
    if (!context || !context->initialized)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_stream_set_algorithm_type");
        return TECC_CRYPTO_INVALID_PARAM;
    }

    /* Validate algorithm type */
    if (algorithm_type < TECC_ALGO_RND || algorithm_type > TECC_ALGO_SEARCH)
    {
        TECC_SCHEDULER_ERROR("Invalid algorithm type: %d", algorithm_type);
        return TECC_CRYPTO_INVALID_PARAM;
    }

    context->algorithm_type = algorithm_type;
    return TECC_CRYPTO_SUCCESS;
}

/*
 * Set collation for stream context
 */
int
tecc_stream_set_collation(
    TeccStreamCryptoContext *context,
    Oid collation_oid)
{
    /* Validate parameters */
    if (!context || !context->initialized)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_stream_set_collation");
        return TECC_CRYPTO_INVALID_PARAM;
    }

    context->collation_oid = collation_oid;
    return TECC_CRYPTO_SUCCESS;
}

/*
 * Set operator type for stream context (SEARCH algorithm)
 */
int
tecc_stream_set_operator_type(
    TeccStreamCryptoContext *context,
    TeccOperatorType operator_type)
{
    /* Validate parameters */
    if (!context || !context->initialized)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_stream_set_operator_type");
        return TECC_CRYPTO_INVALID_PARAM;
    }

    /* Validate operator type */
    if (operator_type < TECC_OP_EQUAL || operator_type > TECC_OP_LIKE)
    {
        TECC_SCHEDULER_ERROR("Invalid operator type: %d", operator_type);
        return TECC_CRYPTO_INVALID_PARAM;
    }

    context->operator_type = operator_type;
    return TECC_CRYPTO_SUCCESS;
}

/*
 * Reset stream context (for reuse)
 */
int
tecc_stream_reset_context(
    TeccStreamCryptoContext *context)
{
    /* Validate parameters */
    if (!context || !context->initialized)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_stream_reset_context");
        return TECC_CRYPTO_INVALID_PARAM;
    }

    /* Reset statistics */
    context->processed_count = 0;

    /* Reset algorithm settings to defaults */
    context->algorithm_type = TECC_ALGO_RND;
    context->collation_oid = InvalidOid;
    context->operator_type = TECC_OP_EQUAL;

    return TECC_CRYPTO_SUCCESS;
}

/*
 * Create stream memory pool
 */
TeccStreamMemoryPool *
tecc_stream_memory_pool_create(int max_buffers)
{
    TeccStreamMemoryPool *pool;

    if (max_buffers <= 0)
    {
        TECC_SCHEDULER_ERROR("Invalid max_buffers parameter: %d", max_buffers);
        return NULL;
    }

    pool = (TeccStreamMemoryPool *)pg_malloc(sizeof(TeccStreamMemoryPool));
    if (!pool)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate memory pool");
        return NULL;
    }

    memset(pool, 0, sizeof(TeccStreamMemoryPool));
    pool->max_buffers = max_buffers;

    /* Allocate arrays */
    pool->buffers = (char **)pg_malloc0(max_buffers * sizeof(char *));
    pool->buffer_sizes = (size_t *)pg_malloc0(max_buffers * sizeof(size_t));

    if (!pool->buffers || !pool->buffer_sizes)
    {
        if (pool->buffers) pg_free(pool->buffers);
        if (pool->buffer_sizes) pg_free(pool->buffer_sizes);
        pg_free(pool);
        TECC_SCHEDULER_ERROR("Failed to allocate memory pool arrays");
        return NULL;
    }

    /* Initialize performance statistics */
    gettimeofday(&pool->creation_time, NULL);
    pool->total_allocations = 0;
    pool->total_allocation_size = 0;
    pool->peak_allocated = 0;
    pool->cache_hits = 0;
    pool->cache_misses = 0;

    TECC_SCHEDULER_DEBUG("Created stream memory pool with max_buffers=%d", max_buffers);
    return pool;
}

/*
 * Allocate buffer from stream memory pool
 */
char *
tecc_stream_memory_pool_alloc(TeccStreamMemoryPool *pool, size_t size)
{
    char *buffer;

    if (!pool || size == 0)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_stream_memory_pool_alloc");
        return NULL;
    }

    /* Check if we have space in the pool */
    if (pool->buffer_count >= pool->max_buffers)
    {
        /* Pool is full, allocate directly */
        buffer = (char *)pg_malloc(size);
        if (buffer)
        {
            TECC_SCHEDULER_DEBUG("Pool full, allocated buffer directly: size=%zu", size);
            /* Update statistics for direct allocation */
            pool->total_allocations++;
            pool->total_allocation_size += size;
            pool->cache_misses++;
        }
        return buffer;
    }

    /* Allocate buffer and add to pool */
    buffer = (char *)pg_malloc(size);
    if (!buffer)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate buffer of size %zu", size);
        return NULL;
    }

    /* Add to pool */
    pool->buffers[pool->buffer_count] = buffer;
    pool->buffer_sizes[pool->buffer_count] = size;
    pool->buffer_count++;
    pool->total_allocated += size;

    /* Update performance statistics */
    pool->total_allocations++;
    pool->total_allocation_size += size;
    pool->cache_hits++;

    /* Update peak allocated memory */
    if (pool->total_allocated > pool->peak_allocated)
    {
        pool->peak_allocated = pool->total_allocated;
    }

    TECC_SCHEDULER_DEBUG("Allocated buffer from pool: size=%zu, pool_count=%d, total_allocated=%zu",
                        size, pool->buffer_count, pool->total_allocated);
    return buffer;
}

/*
 * Destroy stream memory pool
 */
void
tecc_stream_memory_pool_destroy(TeccStreamMemoryPool *pool)
{
    int i;

    if (!pool)
        return;

    /* Free all buffers in the pool */
    for (i = 0; i < pool->buffer_count; i++)
    {
        if (pool->buffers[i])
        {
            pg_free(pool->buffers[i]);
        }
    }

    /* Free arrays */
    if (pool->buffers)
        pg_free(pool->buffers);
    if (pool->buffer_sizes)
        pg_free(pool->buffer_sizes);

    /* Free pool structure */
    pg_free(pool);

    TECC_SCHEDULER_DEBUG("Destroyed stream memory pool");
}

/*
 * Destroy stream context
 */
void
tecc_stream_destroy_context(
    TeccStreamCryptoContext *context)
{
    if (!context)
        return 0;

    /* Destroy base context */
    if (context->base_context)
    {
        tecc_destroy_crypto_context(context->base_context);
    }

    /* Free key ID */
    if (context->key_id)
    {
        pg_free(context->key_id);
    }

    /* Destroy stream pool if allocated */
    if (context->stream_pool)
    {
        tecc_stream_memory_pool_destroy(context->stream_pool);
    }

    /* Free context itself */
    pg_free(context);
}

/* ==================== Stream Performance Statistics Implementation ==================== */
/* Begin - ReqID:TECC4.0 - Stream performance statistics implementation for import/export module */

/*
 * tecc_stream_get_performance_stats - Get stream context performance statistics
 */
int
tecc_stream_get_performance_stats(
    TeccStreamCryptoContext *context,
    size_t *total_processed,
    size_t *encryption_count,
    size_t *decryption_count,
    size_t *batch_operations,
    size_t *single_operations,
    double *avg_encryption_time,
    double *avg_decryption_time,
    size_t *memory_saved)
{
    if (!context || !total_processed || !encryption_count || !decryption_count ||
        !batch_operations || !single_operations || !avg_encryption_time ||
        !avg_decryption_time || !memory_saved)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_stream_get_performance_stats");
        return -1;
    }

    *total_processed = context->processed_count;
    *encryption_count = context->encryption_count;
    *decryption_count = context->decryption_count;
    *batch_operations = context->batch_operations;
    *single_operations = context->single_operations;
    *avg_encryption_time = context->avg_encryption_time;
    *avg_decryption_time = context->avg_decryption_time;
    *memory_saved = context->memory_saved;

    return 0;
}

/*
 * tecc_stream_get_memory_pool_stats - Get memory pool statistics
 */
int
tecc_stream_get_memory_pool_stats(
    TeccStreamCryptoContext *context,
    size_t *total_allocations,
    size_t *total_allocation_size,
    size_t *peak_allocated,
    size_t *cache_hits,
    size_t *cache_misses,
    double *cache_hit_rate)
{
    if (!context || !total_allocations || !total_allocation_size || !peak_allocated ||
        !cache_hits || !cache_misses || !cache_hit_rate)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_stream_get_memory_pool_stats");
        return -1;
    }

    if (!context->stream_pool)
    {
        /* No memory pool available */
        *total_allocations = 0;
        *total_allocation_size = 0;
        *peak_allocated = 0;
        *cache_hits = 0;
        *cache_misses = 0;
        *cache_hit_rate = 0.0;
        return 0;
    }

    *total_allocations = context->stream_pool->total_allocations;
    *total_allocation_size = context->stream_pool->total_allocation_size;
    *peak_allocated = context->stream_pool->peak_allocated;
    *cache_hits = context->stream_pool->cache_hits;
    *cache_misses = context->stream_pool->cache_misses;

    /* Calculate cache hit rate */
    if (context->stream_pool->cache_hits + context->stream_pool->cache_misses > 0)
    {
        *cache_hit_rate = (double)context->stream_pool->cache_hits /
                         (context->stream_pool->cache_hits + context->stream_pool->cache_misses) * 100.0;
    }
    else
    {
        *cache_hit_rate = 0.0;
    }

    return 0;
}

/*
 * tecc_stream_reset_statistics - Reset stream context statistics
 */
int
tecc_stream_reset_statistics(
    TeccStreamCryptoContext *context)
{
    if (!context)
    {
        TECC_SCHEDULER_ERROR("Invalid parameter to tecc_stream_reset_statistics");
        return -1;
    }

    /* Reset performance statistics */
    gettimeofday(&context->start_time, NULL);
    gettimeofday(&context->last_operation_time, NULL);
    context->total_encrypted_bytes = 0;
    context->total_decrypted_bytes = 0;
    context->encryption_count = 0;
    context->decryption_count = 0;
    context->batch_operations = 0;
    context->single_operations = 0;
    context->avg_encryption_time = 0.0;
    context->avg_decryption_time = 0.0;
    context->memory_saved = 0;

    /* Reset memory pool statistics if available */
    if (context->stream_pool)
    {
        context->stream_pool->total_allocations = 0;
        context->stream_pool->total_allocation_size = 0;
        context->stream_pool->peak_allocated = 0;
        context->stream_pool->cache_hits = 0;
        context->stream_pool->cache_misses = 0;
    }

    TECC_SCHEDULER_DEBUG("Reset performance statistics for stream context");
    return 0;
}

/*
 * tecc_stream_update_performance_stats - Update stream context performance statistics after operation
 */
int
tecc_stream_update_performance_stats(
    TeccStreamCryptoContext *context,
    bool is_encryption,
    size_t data_size,
    double operation_time_ms,
    bool is_batch)
{
    if (!context)
    {
        TECC_SCHEDULER_ERROR("Invalid parameter to tecc_stream_update_performance_stats");
        return -1;
    }

    /* Update last operation time */
    gettimeofday(&context->last_operation_time, NULL);

    /* Update processed count */
    context->processed_count++;

    /* Update encryption/decryption statistics */
    if (is_encryption)
    {
        context->encryption_count++;
        context->total_encrypted_bytes += data_size;

        /* Update average encryption time */
        if (context->encryption_count == 1)
        {
            context->avg_encryption_time = operation_time_ms;
        }
        else
        {
            context->avg_encryption_time = (context->avg_encryption_time * (context->encryption_count - 1) +
                                           operation_time_ms) / context->encryption_count;
        }
    }
    else
    {
        context->decryption_count++;
        context->total_decrypted_bytes += data_size;

        /* Update average decryption time */
        if (context->decryption_count == 1)
        {
            context->avg_decryption_time = operation_time_ms;
        }
        else
        {
            context->avg_decryption_time = (context->avg_decryption_time * (context->decryption_count - 1) +
                                           operation_time_ms) / context->decryption_count;
        }
    }

    /* Update operation type statistics */
    if (is_batch)
    {
        context->batch_operations++;
    }
    else
    {
        context->single_operations++;
    }

    /* Update memory saved estimation (simplified: assume 20% memory saving with stream optimization) */
    if (context->stream_pool && context->stream_pool->cache_hits > 0)
    {
        /* Estimate memory saved by reusing buffers from pool */
        size_t estimated_saved = context->stream_pool->total_allocation_size * 20 / 100;
        if (estimated_saved > context->memory_saved)
        {
            context->memory_saved = estimated_saved;
        }
    }

    TECC_SCHEDULER_DEBUG("Updated performance stats: %s, size=%zu, time=%.2fms, batch=%s",
                        is_encryption ? "encryption" : "decryption",
                        data_size, operation_time_ms, is_batch ? "yes" : "no");
    return 0;
}

/*
 * tecc_stream_compare_performance - Compare stream vs standard performance
 */
int
tecc_stream_compare_performance(
    TeccStreamCryptoContext *stream_context,
    size_t standard_operation_count,
    double standard_total_time_ms,
    double *performance_improvement,
    double *memory_saving_ratio)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    size_t stream_total_operations;
    double stream_avg_time;
    double standard_avg_time;

    if (!stream_context || !performance_improvement || !memory_saving_ratio)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_stream_compare_performance");
        return -1;
    }

    /* Calculate total operations in stream context */
    stream_total_operations = stream_context->encryption_count + stream_context->decryption_count;

    if (stream_total_operations == 0 || standard_operation_count == 0)
    {
        *performance_improvement = 0.0;
        *memory_saving_ratio = 0.0;
        return 0;
    }

    /* Calculate average time per operation for stream context */
    stream_avg_time = 0.0;
    if (stream_context->encryption_count > 0)
    {
        stream_avg_time += stream_context->avg_encryption_time * stream_context->encryption_count;
    }
    if (stream_context->decryption_count > 0)
    {
        stream_avg_time += stream_context->avg_decryption_time * stream_context->decryption_count;
    }
    stream_avg_time /= stream_total_operations;

    /* Calculate average time per operation for standard processing */
    standard_avg_time = standard_total_time_ms / standard_operation_count;

    /* Calculate performance improvement (positive means stream is faster) */
    if (standard_avg_time > 0)
    {
        *performance_improvement = ((standard_avg_time - stream_avg_time) / standard_avg_time) * 100.0;
    }
    else
    {
        *performance_improvement = 0.0;
    }

    /* Calculate memory saving ratio */
    if (stream_context->stream_pool && stream_context->stream_pool->total_allocation_size > 0)
    {
        /* Simplified: assume standard processing would allocate 30% more memory */
        size_t estimated_standard_memory = stream_context->stream_pool->total_allocation_size * 130 / 100;
        *memory_saving_ratio = ((double)(estimated_standard_memory - stream_context->stream_pool->total_allocated) /
                               estimated_standard_memory) * 100.0;
    }
    else
    {
        *memory_saving_ratio = 0.0;
    }

    TECC_SCHEDULER_DEBUG("Performance comparison: stream_avg=%.2fms, standard_avg=%.2fms, improvement=%.2f%%, memory_saving=%.2f%%",
                        stream_avg_time, standard_avg_time, *performance_improvement, *memory_saving_ratio);
    return 0;
}

/*
 * tecc_stream_generate_performance_report - Generate performance report
 */
char *
tecc_stream_generate_performance_report(
    TeccStreamCryptoContext *context)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    struct timeval current_time;
    double total_time_sec;
    size_t total_processed, encryption_count, decryption_count, batch_ops, single_ops, memory_saved;
    double avg_enc_time, avg_dec_time;
    size_t total_allocations, total_allocation_size, peak_allocated, cache_hits, cache_misses;
    double cache_hit_rate;
    double bytes_per_second = 0.0;
    double rows_per_second = 0.0;

    if (!context)
    {
        TECC_SCHEDULER_ERROR("Invalid parameter to tecc_stream_generate_performance_report");
        return NULL;
    }

    /* Calculate total processing time */
    gettimeofday(&current_time, NULL);
    total_time_sec = (current_time.tv_sec - context->start_time.tv_sec) +
                     (current_time.tv_usec - context->start_time.tv_usec) / 1000000.0;

    /* Get performance statistics */
    tecc_stream_get_performance_stats(context, &total_processed, &encryption_count,
                                     &decryption_count, &batch_ops, &single_ops,
                                     &avg_enc_time, &avg_dec_time, &memory_saved);

    /* Get memory pool statistics */
    tecc_stream_get_memory_pool_stats(context, &total_allocations, &total_allocation_size,
                                     &peak_allocated, &cache_hits, &cache_misses, &cache_hit_rate);

    /* Calculate throughput */
    if (total_time_sec > 0)
    {
        bytes_per_second = (context->total_encrypted_bytes + context->total_decrypted_bytes) / total_time_sec;
        rows_per_second = total_processed / total_time_sec;
    }

    /* Generate report string */
    {
        char *report = pg_malloc(2048); /* Allocate enough space for report */
    if (!report)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate memory for performance report");
        return NULL;
    }

    snprintf(report, 2048,
        "TECC Stream Optimization Performance Report\n"
        "===========================================\n"
        "Processing Time: %.2f seconds\n"
        "Total Processed: %zu items\n"
        "  - Encryptions: %zu (%.2f ms avg)\n"
        "  - Decryptions: %zu (%.2f ms avg)\n"
        "Operation Types:\n"
        "  - Batch Operations: %zu\n"
        "  - Single Operations: %zu\n"
        "Throughput:\n"
        "  - Bytes/Second: %.2f\n"
        "  - Rows/Second: %.2f\n"
        "Memory Pool Statistics:\n"
        "  - Total Allocations: %zu\n"
        "  - Total Allocation Size: %zu bytes\n"
        "  - Peak Allocated: %zu bytes\n"
        "  - Cache Hits: %zu\n"
        "  - Cache Misses: %zu\n"
        "  - Cache Hit Rate: %.2f%%\n"
        "  - Estimated Memory Saved: %zu bytes\n",
        total_time_sec,
        total_processed,
        encryption_count, avg_enc_time,
        decryption_count, avg_dec_time,
        batch_ops,
        single_ops,
        bytes_per_second,
        rows_per_second,
        total_allocations,
        total_allocation_size,
        peak_allocated,
        cache_hits,
        cache_misses,
        cache_hit_rate,
        memory_saved);

        return report;
    }
}

/* End - ReqID:TECC4.0 - Stream performance statistics implementation for import/export module */
/* End - ReqID:TECC4.0 - Stream processing implementation for import/export module */

/*
 * Unified encryption for WHERE clause comparisons.
 * Encrypts plaintext using only the algorithm needed for the operator:
 *   = / <> -> DET
 *   < / > / <= / >= -> OPE
 *   LIKE / ILIKE / NOT LIKE / NOT ILIKE -> SEARCH
 */
TeccUnifiedEncryptResult *
tecc_unified_encrypt_for_where(PGconn *conn,
                              const char *plaintext,
                              const char *table_name,
                              const char *column_name,
                              const char *kek_name,
                              const char *dek_name,
                              TeccOperatorType operator_type,
                              TeccCryptoContext *external_context)
{
    TeccCryptoScheduler *scheduler;
    TeccUnifiedEncryptResult *result = NULL;
    int algo_type;
    const char *algo_name;
    bool algo_fallback_allowed;

    if (conn == NULL || plaintext == NULL || table_name == NULL || column_name == NULL)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_unified_encrypt_for_where");
        return NULL;
    }

    /* Map operator type to algorithm */
    algo_type = TECC_ALGO_DET;  /* default for equality */
    algo_name = "DET";
    algo_fallback_allowed = false; /* may switch to OPE for = */

    switch (operator_type)
    {
        case TECC_OP_EQUAL:
        case TECC_OP_NOT_EQUAL:
        case TECC_OP_IS_NOT_DISTINCT_FROM:
        case TECC_OP_IS_DISTINCT_FROM:
            algo_type = TECC_ALGO_DET;
            algo_name = "DET";
            algo_fallback_allowed = true; /* OPE also works for equality */
            break;
        case TECC_OP_LESS_THAN:
        case TECC_OP_GREATER_THAN:
        case TECC_OP_LESS_EQUAL:
        case TECC_OP_GREATER_EQUAL:
            algo_type = TECC_ALGO_OPE;
            algo_name = "OPE";
            break;
        case TECC_OP_LIKE:
        case TECC_OP_NOT_LIKE:
        case TECC_OP_ILIKE:
        case TECC_OP_NOT_ILIKE:
            algo_type = TECC_ALGO_SEARCH;
            algo_name = "SEARCH";
            break;
        default:
            TECC_SCHEDULER_ERROR("Unsupported operator type for WHERE encryption: %d", operator_type);
            return NULL;
    }

    TECC_SCHEDULER_DEBUG("WHERE encrypt: %s on %s.%s using %s",
                         plaintext, table_name, column_name, algo_name);

    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return NULL;
    }

    /* Build context and encrypt with the selected algorithm only */
    {
        TeccState *state = tecc_get_state(conn);
        TeccColumnCacheItem *cache_item = NULL;
        TeccEncryptedColumnMetadata *col_meta = NULL;

        if (state != NULL)
        {
            const char *db_name = PQdb(conn);
            if (db_name == NULL || db_name[0] == '\0')
                db_name = "postgres";
            cache_item = tecc_cache_get_encrypted_column_by_name(
                state, db_name, "public", table_name, column_name);
            if (cache_item != NULL)
                col_meta = tecc_convert_column_cache_item_to_metadata(state, cache_item);
        }

        if (col_meta == NULL)
        {
            TECC_SCHEDULER_ERROR("Column %s.%s not found in metadata cache", table_name, column_name);
            return NULL;
        }

        /* ReqID:TECC4.0 - For =/<> operators, if the column lacks DET
         * but has OPE, fall back to OPE. OPE also supports equality. */
        if (algo_fallback_allowed &&
            algo_type == TECC_ALGO_DET &&
            !col_meta->enable_det &&
            col_meta->enable_ope)
        {
            algo_type = TECC_ALGO_OPE;
            algo_name = "OPE";
            TECC_SCHEDULER_DEBUG("WHERE encrypt: column has OPE but not DET, "
                               "using OPE for equality comparison");
        }

        {
            TeccCryptoContextExtended *context;
            unsigned char *dek_key = NULL;
            unsigned char *bpchar_trim_buf = NULL;
            size_t dek_key_len = 0;
            int ret;
            CipherDataType *cipherdata = NULL;
            size_t cipherdata_len = 0;
            TeccFormatterErrorCode fmt_ret;
            unsigned char *primary_ct = NULL;
            size_t primary_ct_len = 0;
            const char *dek_name_to_use;
            TeccCryptoBuffer *cipherdata_buffer;

            context = tecc_create_crypto_context_with_collation(table_name, column_name,
                                                                kek_name, dek_name, 0);
            if (context == NULL)
            {
                tecc_free_encrypted_column_metadata(col_meta);
                return NULL;
            }

            context->core.data_type = tecc_map_to_original_type_oid(col_meta->data_type_original_oid);
            /* ReqID:TECC4.0 - MUST set type_mod BEFORE bpchar padding and rtrim.
             * type_mod is initialized to 0 by pg_malloc0 in context creation.
             * The bpchar padding check (core.type_mod >= 4) and padding length
             * calculation need the correct typmod from column metadata.
             * See also: collation_locale and supports_ope which are needed by
             * normalize_data_for_algorithm inside tecc_execute_algo_ope. */
            context->core.type_mod = col_meta->data_type_original_mod;
            if (col_meta->collation_locale != NULL)
                strlcpy(context->core.collation_locale, col_meta->collation_locale, NAMEDATALEN);
            context->core.algo_support.supports_det = col_meta->enable_det;
            context->core.algo_support.supports_ope = col_meta->enable_ope;
            context->core.algo_support.supports_search = col_meta->enable_search;

            /* ReqID:TECC4.0 - Unified text preparation for WHERE encryption:
             * 1. BPCHAR: pad to declared length (PG backend pads bpchar(N) to N bytes)
             * 2. Universal rtrim: compute space_count (but do NOT reduce plaintext_len)
             * This matches the INSERT path (tecc_unified_encrypt_string_internal_with_scheduler)
             * exactly: INSERT also keeps plaintext_len at the padded length and computes
             * text_space_count separately. normalize_data_for_algorithm inside
             * tecc_execute_algo_ope reads context->plaintext_len bytes for ICU sort key
             * generation, so the padded (not rtrimmed) length is critical. */
            {
                const unsigned char *pt = (const unsigned char *)plaintext;

                context->plaintext = pt;
                context->plaintext_len = strlen(plaintext);

                /* 1. BPCHAR blank-padding to declared length */
                if (context->core.data_type == BPCHAROID && context->core.type_mod >= (int32)sizeof(int32))
                {
                    int32 bpchar_len = context->core.type_mod - 4;
                    size_t orig_len = context->plaintext_len;

                    if (bpchar_len > 0 && (size_t)bpchar_len > orig_len)
                    {
                        const unsigned char *orig_text = context->plaintext;
                        unsigned char *padded;
                        size_t pad_count, i;

                        padded = (unsigned char *)pg_malloc((size_t)bpchar_len + 1);
                        if (padded != NULL)
                        {
                            memcpy(padded, orig_text, orig_len);
                            pad_count = (size_t)bpchar_len - orig_len;
                            for (i = 0; i < pad_count; i++)
                                padded[orig_len + i] = ' ';
                            padded[bpchar_len] = '\0';
                            context->plaintext = padded;
                            context->plaintext_len = (size_t)bpchar_len;
                            bpchar_trim_buf = padded;  /* track for cleanup */
                        }
                    }
                }

                /* 2. Universal rtrim for text types: compute space_count
                 * and reduce plaintext_len so DET/OPE always encrypt the
                 * rtrimmed data. space_count is stored for ciphertext format
                 * encoding; the server-side bpchar_enc_eq strips it and
                 * compares only the core ciphertext. */
                context->text_space_count = 0;
                if (TECC_IS_TEXT_TYPE(context->core.data_type) &&
                    context->plaintext_len > 0)
                {
                    const unsigned char *pd = context->plaintext;
                    size_t used = context->plaintext_len;

                    while (used > 0 && pd[used - 1] == ' ')
                        used--;
                    context->text_space_count = (uint32_t)(context->plaintext_len - used);
                    context->plaintext_len = used;
                }
            }

            /* ReqID:TECC4.0 - Set operator type for SEARCH LIKE/NOT LIKE pattern encryption */
            context->operator_type = operator_type;
            if (operator_type == TECC_OP_LIKE || operator_type == TECC_OP_NOT_LIKE ||
                operator_type == TECC_OP_ILIKE || operator_type == TECC_OP_NOT_ILIKE)
            {
                context->is_like_pattern = true;
            }

            /* ReqID:TECC4.0 - Set selected algorithms based on operator type. */
            if (algo_type == TECC_ALGO_SEARCH)
                context->core.selected_algorithms = (1 << TECC_ALGO_RND) | (1 << TECC_ALGO_SEARCH);
            else
                context->core.selected_algorithms = (1 << TECC_ALGO_RND) | (1 << algo_type);

            dek_name_to_use = dek_name ? dek_name : col_meta->dek_name;
            ret = tecc_key_manager_adapter_get_dek_key(scheduler->key_adapter,
                                                      dek_name_to_use,
                                                      &dek_key, &dek_key_len);
            if (ret != TECC_CRYPTO_SUCCESS)
            {
                TECC_SCHEDULER_ERROR("Failed to get DEK key for WHERE encryption");
                tecc_free_encrypted_column_metadata(col_meta);
                if (bpchar_trim_buf) pg_free(bpchar_trim_buf);
                pg_free(context);
                return NULL;
            }

            context->dek_key = dek_key;
            context->dek_key_len = dek_key_len;

            /* ReqID:TECC4.0 - HSM路由支持：补填col_meta->kek_name并设置HSM上下文
             * 同INSERT路径的修复模式，确保col_meta->kek_name不为NULL，
             * 使tecc_setup_hsm_context能检测到HSM KEK。
             * DET/OPE算法不使用HSM做数据加密，但正确的上下文状态对后续处理至关重要。 */
            if (col_meta && (col_meta->kek_name == NULL || col_meta->kek_name[0] == '\0') &&
                state != NULL && dek_name_to_use != NULL)
            {
                TeccDekCacheItem *dek_item = tecc_cache_get_dek_by_name_search_path(
                    (const TeccState *)state, dek_name_to_use);
                if (dek_item != NULL && dek_item->kek_name != NULL && dek_item->kek_name[0] != '\0')
                {
                    col_meta->kek_name = pg_strdup(dek_item->kek_name);
                    strlcpy(context->kek_name, dek_item->kek_name, NAMEDATALEN);
                }
            }

            /* Set up HSM SM4 encryption context based on KEK's KMS type.
             * If HSM is configured but init failed, abort — no software fallback. */
            if (state != NULL)
            {
                if (tecc_setup_hsm_context(context, col_meta, state) < 0)
                {
                    TECC_SCHEDULER_ERROR("HSM init failed for KEK, refusing software fallback");
                    tecc_free_encrypted_column_metadata(col_meta);
                    if (bpchar_trim_buf) pg_free(bpchar_trim_buf);
                    pg_free(context);
                    return NULL;
                }
            }

            /* Execute the selected algorithm */
            switch (algo_type)
            {
                case TECC_ALGO_DET:
                    ret = tecc_execute_algo_det(context);
                    primary_ct = context->det_ciphertext;
                    primary_ct_len = context->det_cipher_len;
                    break;
                case TECC_ALGO_OPE:
                    ret = tecc_execute_algo_ope(context);
                    primary_ct = context->ope_ciphertext;
                    primary_ct_len = context->ope_cipher_len;
                    break;
                case TECC_ALGO_SEARCH:
                    /* ReqID:TECC4.0 - Use operator-aware encryption for LIKE patterns */
                    if (operator_type == TECC_OP_LIKE || operator_type == TECC_OP_NOT_LIKE ||
                        operator_type == TECC_OP_ILIKE || operator_type == TECC_OP_NOT_ILIKE)
                    {
                        ret = tecc_algorithm_scheduler_encrypt_search_with_operator(
                            (TeccCryptoContext *)context, operator_type,
                            &primary_ct, &primary_ct_len);
                    }
                    else
                    {
                        ret = tecc_execute_algo_search(context);
                        primary_ct = context->search_ciphertext;
                        primary_ct_len = context->search_cipher_len;
                    }
                    break;
                default:
                    tecc_free_encrypted_column_metadata(col_meta);
                    if (bpchar_trim_buf) pg_free(bpchar_trim_buf);
                    pg_free(context);
                    return NULL;
            }

            if (ret != TECC_CRYPTO_SUCCESS || primary_ct == NULL)
            {
                TECC_SCHEDULER_ERROR("%s algorithm encryption failed for WHERE clause", algo_name);
                tecc_free_encrypted_column_metadata(col_meta);
                if (bpchar_trim_buf) pg_free(bpchar_trim_buf);
                pg_free(context);
                return NULL;
            }

            /* Build a minimal CipherDataType with only the needed algorithm's data */
            {
                cipherdata_len = sizeof(CipherDataType) + primary_ct_len;
                cipherdata = pg_malloc0(cipherdata_len);
                if (cipherdata == NULL)
                {
                    tecc_free_encrypted_column_metadata(col_meta);
                    if (bpchar_trim_buf) pg_free(bpchar_trim_buf);
                    pg_free(context);
                    return NULL;
                }

                {
                    char *cd_ptr;
                    int32 version;
                    Oid dek_dist_oid;
                    int32 zero_val;
                    int32 algo_len_val;

                    SET_VARSIZE(cipherdata, cipherdata_len);
                    cd_ptr = (char *)cipherdata;
                    cd_ptr += sizeof(int32);       /* skip vl_len_ */

                    version = CIPHERDATA_VERSION_1_0;
                    memcpy(cd_ptr, &version, sizeof(int32));
                    cd_ptr += sizeof(int32);

                    /* dek_dist_oid -- not known at this level */
                    dek_dist_oid = InvalidOid;
                    memcpy(cd_ptr, &dek_dist_oid, sizeof(Oid));
                    cd_ptr += sizeof(Oid);

                    zero_val = 0;
                    algo_len_val = (int32)primary_ct_len;

                    /* rnd_length */
                    memcpy(cd_ptr, (algo_type == TECC_ALGO_RND) ? &algo_len_val : &zero_val, sizeof(int32));
                    cd_ptr += sizeof(int32);
                    /* det_length */
                    memcpy(cd_ptr, (algo_type == TECC_ALGO_DET) ? &algo_len_val : &zero_val, sizeof(int32));
                    cd_ptr += sizeof(int32);
                    /* ope_length */
                    memcpy(cd_ptr, (algo_type == TECC_ALGO_OPE) ? &algo_len_val : &zero_val, sizeof(int32));
                    cd_ptr += sizeof(int32);
                    /* search_length */
                    memcpy(cd_ptr, (algo_type == TECC_ALGO_SEARCH) ? &algo_len_val : &zero_val, sizeof(int32));
                    cd_ptr += sizeof(int32);

                    /* Copy ciphertext */
                    memcpy(cd_ptr, primary_ct, primary_ct_len);
                }
            }

            /* Set DEK distributed ID from cache if available */
            if (state != NULL && col_meta != NULL && col_meta->dek_name != NULL)
            {
                TeccDekCacheItem *dek_item = tecc_cache_get_dek_by_name_search_path(
                    state, col_meta->dek_name);
                if (dek_item != NULL && OidIsValid(dek_item->column_key_distributed_id))
                    cipherdata->dek_dist_oid = dek_item->column_key_distributed_id;
            }

            cipherdata_buffer = pg_malloc0(sizeof(TeccCryptoBuffer));
            if (cipherdata_buffer)
            {
                cipherdata_buffer->data = cipherdata;
                cipherdata_buffer->size = cipherdata_len;
                cipherdata_buffer->used = cipherdata_len;
            }

            result = pg_malloc0(sizeof(TeccUnifiedEncryptResult));
            if (result)
            {
                result->error_code = TECC_CRYPTO_SUCCESS;
                fmt_ret = tecc_formatter_hex_encode((const unsigned char *)cipherdata, cipherdata_len,
                                                    &result->hex_string);
                if (fmt_ret == TECC_FORMATTER_SUCCESS)
                    result->hex_len = strlen(result->hex_string);
                result->cipherdata_buffer = cipherdata_buffer;
            }

            tecc_free_encrypted_column_metadata(col_meta);
            if (bpchar_trim_buf) pg_free(bpchar_trim_buf);
            pg_free(context);

            return result;
        }
    }
}

/* End - ReqID:TECC4.0 */
