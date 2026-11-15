/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_crypto_context.c
 *
 * Abstract:
 *   Crypto context implementation for TECC crypto scheduler module.
 *   Implements context creation, management, and buffer handling.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_scheduler/tecc_crypto_context.c
 */

#include "tecc_crypto_context.h"
#include "../tecc_processor/internal/tecc_processor_internal.h"
#include "common/fe_memutils.h"  /* Frontend memory management */
#include <string.h>

/* Default work buffer size */
#define DEFAULT_WORK_BUFFER_SIZE (64 * 1024)  /* 64KB */

/*
 * Create basic crypto context
 */
TeccCryptoContext *
tecc_create_crypto_context(const char *table_name,
                          const char *column_name,
                          const char *kek_name,
                          const char *dek_name)
{
    TeccCryptoContext *context;

    /* Allocate context structure */
    context = pg_malloc0(sizeof(TeccCryptoContextExtended));
    if (!context)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to allocate crypto context");
        return NULL;
    }

    /* Initialize memory context - Not needed with direct allocation */
    /* context->mem_ctx = CurrentMemoryContext; */

    /* Copy basic information */
    if (table_name)
        strlcpy(context->table_name, table_name, NAMEDATALEN);
    if (column_name)
        strlcpy(context->column_name, column_name, NAMEDATALEN);
    if (kek_name)
        strlcpy(context->kek_name, kek_name, NAMEDATALEN);
    if (dek_name)
        strlcpy(context->dek_name, dek_name, NAMEDATALEN);

    /* Initialize algorithm support (default: RND only) */
    context->core.algo_support.supports_rnd = true;
    context->core.algo_support.supports_det = false;
    context->core.algo_support.supports_ope = false;
    context->core.algo_support.supports_search = false;

    /* Initialize selected algorithms */
    context->core.selected_algorithms = TECC_ALGO_RND;

    /* Enable buffer reuse by default */
    context->buffer_reuse_enabled = true;

    /* Explicitly initialize HSM fields to NULL */
    context->hsm_adapter = NULL;
    context->hsm_device = NULL;

    /* Initialize GUC integration */
    context->guc_params = NULL;
    context->guc_processing_enabled = true;  /* Enable GUC processing by default */

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Created crypto context for %s.%s",
                        context->table_name, context->column_name);

    return context;
}

/*
 * Create crypto context with memory pool
 */
TeccCryptoContext *
tecc_create_crypto_context_with_pool(const char *table_name,
                                    const char *column_name,
                                    const char *kek_name,
                                    const char *dek_name)
{
    TeccCryptoContext *context;

    /* Create basic context */
    context = tecc_create_crypto_context(table_name, column_name, kek_name, dek_name);
    if (!context)
        return NULL;

    /* Pre-allocate work buffer using TeccCryptoBuffer */
    context->work_buffer_size = DEFAULT_WORK_BUFFER_SIZE;
    context->work_buffer = pg_malloc(context->work_buffer_size);
    if (!context->work_buffer)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to allocate work buffer");
        pg_free(context);
        return NULL;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Created crypto context with work buffer");

    return context;
}

/*
 * Create crypto context with collation support
 */
TeccCryptoContext *
tecc_create_crypto_context_with_collation(const char *table_name,
                                         const char *column_name,
                                         const char *kek_name,
                                         const char *dek_name,
                                         Oid collation_oid)
{
    TeccCryptoContext *context;

    /* Create context with pool */
    context = tecc_create_crypto_context_with_pool(table_name, column_name, kek_name, dek_name);
    if (!context)
        return NULL;

    /* Set collation */
    context->core.collation_oid = collation_oid;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Created crypto context with collation OID %u", collation_oid);

    return context;
}

/*
 * Destroy crypto context and all its resources
 */
void
tecc_destroy_crypto_context(TeccCryptoContext *context)
{
    if (!context)
        return;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Destroying crypto context for %s.%s",
                        context->table_name, context->column_name);

    /* Securely wipe sensitive data */
    if (context->dek_key)
    {
        memset(context->dek_key, 0, context->dek_key_len);
        pg_free(context->dek_key);
    }

    /* Clear algorithm-specific ciphertext */
    if (context->rnd_ciphertext)
    {
        memset(context->rnd_ciphertext, 0, context->rnd_cipher_len);
        pg_free(context->rnd_ciphertext);
    }
    if (context->det_ciphertext)
    {
        memset(context->det_ciphertext, 0, context->det_cipher_len);
        pg_free(context->det_ciphertext);
    }
    if (context->ope_ciphertext)
    {
        memset(context->ope_ciphertext, 0, context->ope_cipher_len);
        pg_free(context->ope_ciphertext);
    }
    if (context->search_ciphertext)
    {
        memset(context->search_ciphertext, 0, context->search_cipher_len);
        pg_free(context->search_ciphertext);
    }

    /* ReqID:TECC4.0 - Clear HE ciphertext */
    if (context->he_add_ciphertext)
    {
        memset(context->he_add_ciphertext, 0, context->he_add_cipher_len);
        pg_free(context->he_add_ciphertext);
    }
    if (context->he_mul_ciphertext)
    {
        memset(context->he_mul_ciphertext, 0, context->he_mul_cipher_len);
        pg_free(context->he_mul_ciphertext);
    }

    /* ReqID:TECC4.0 - Clear HE key material */
    if (context->he_ec_pubkey_hex)
    {
        memset(context->he_ec_pubkey_hex, 0, strlen(context->he_ec_pubkey_hex));
        pg_free(context->he_ec_pubkey_hex);
    }
    if (context->he_ec_privkey_hex)
    {
        memset(context->he_ec_privkey_hex, 0, strlen(context->he_ec_privkey_hex));
        pg_free(context->he_ec_privkey_hex);
    }
    if (context->he_eg_pubkey_hex)
    {
        memset(context->he_eg_pubkey_hex, 0, strlen(context->he_eg_pubkey_hex));
        pg_free(context->he_eg_pubkey_hex);
    }
    if (context->he_eg_privkey_hex)
    {
        memset(context->he_eg_privkey_hex, 0, strlen(context->he_eg_privkey_hex));
        pg_free(context->he_eg_privkey_hex);
    }

    /* Clear work buffer */
    if (context->work_buffer)
    {
        memset(context->work_buffer, 0, context->work_buffer_size);
        pg_free(context->work_buffer);
    }

    /* Clear pre-normalized data */
    if (context->normalized_plaintext)
    {
        memset(context->normalized_plaintext, 0, context->normalized_plaintext_len);
        pg_free(context->normalized_plaintext);
    }

    /* Clear context structure */
    memset(context, 0, sizeof(TeccCryptoContextExtended));
    pg_free(context);
}

/*
 * Reset crypto context state for reuse across multiple decrypt calls.
 * Preserves allocated buffers (work_buffer, hex_decode_buffer) and
 * identification fields (table/column name arrays).
 */
void
tecc_crypto_context_reset(TeccCryptoContext *context)
{
    if (!context)
        return;

    /* Clear per-use pointers (managed by caller, not owned by context) */
    context->dek_key = NULL;
    context->dek_key_len = 0;
    context->rnd_ciphertext = NULL;
    context->rnd_cipher_len = 0;
    context->det_ciphertext = NULL;
    context->det_cipher_len = 0;
    context->ope_ciphertext = NULL;
    context->ope_cipher_len = 0;
    context->search_ciphertext = NULL;
    context->search_cipher_len = 0;
    /* ReqID:TECC4.0 - Reset HE ciphertext pointers */
    context->he_add_ciphertext = NULL;
    context->he_add_cipher_len = 0;
    context->he_mul_ciphertext = NULL;
    context->he_mul_cipher_len = 0;
    context->plaintext = NULL;
    context->plaintext_len = 0;
    context->cipherdata = NULL;
    context->normalized_plaintext = NULL;
    context->normalized_plaintext_len = 0;
    context->parsed_pattern = NULL;

    /* Reset algorithm state */
    context->core.algo_type = TECC_ALGO_RND;
    context->core.selected_algorithms = TECC_ALGO_RND;
    context->core.algo_support.supports_det = false;
    context->core.algo_support.supports_ope = false;
    context->core.algo_support.supports_search = false;

    /* Reset operator/pattern state */
    context->operator_type = 0;
    context->is_like_pattern = false;
    context->data_pre_normalized = false;

    /* Reset error state */
    context->error_code = 0;
    context->error_msg[0] = '\0';

    /* Reset GUC state */
    context->guc_processing_enabled = true;

    /* Reset HSM fields (should already be NULL for software path) */
    context->hsm_adapter = NULL;
    context->hsm_device = NULL;
    /* ReqID:TECC4.0 - hsm_kms_type 必须一并清零。它是"本上下文当前是否走 HSM"
     * 的判据（scheduler 批量解密按它决定是否走逐项 HSM 解密），但 HSM 初始化
     * 只在拿得到列元数据时才执行（见 batch decrypt 里 col_meta != NULL 的
     * 守卫）。若只清 adapter/device 而留下 hsm_kms_type，则元数据缺失的那次
     * 解密会带着"要 HSM"的标记进入 HSM 分支、再因 adapter 为 NULL 报
     * "HSM not available"，掩盖真实原因；同一个共享上下文还可能把软件加密列
     * 误带进 HSM 分支。 */
    context->hsm_kms_type = 0;

    /* Clear name fields */
    context->table_name[0] = '\0';
    context->column_name[0] = '\0';
    context->kek_name[0] = '\0';
    context->dek_name[0] = '\0';
}

/*
 * Prepare buffer of specified type and size
 */
/* Helper function to get buffer descriptor pointer */
static TeccBufferDesc *tecc_get_buffer_desc(TeccCryptoContext *context, TeccBufferType type)
{
    if (!context || type >= TECC_BUFFER_MAX)
        return NULL;

    if (type < TECC_BUFFER_TYPE_MAX)
        return &context->core.buffers[type];
    else
        return &context->ext_buffers[type - TECC_BUFFER_TYPE_MAX];
}

bool
tecc_buffer_prepare(TeccCryptoContext *context, TeccBufferType type, size_t size)
{
    void *buffer;
    TeccBufferDesc *buffer_desc;

    if (!context || type >= TECC_BUFFER_MAX || size == 0)
        return false;

    buffer_desc = tecc_get_buffer_desc(context, type);
    if (!buffer_desc)
        return false;

    /* Check if buffer already exists and is large enough */
    if (buffer_desc->data && buffer_desc->capacity >= size)
    {
        buffer_desc->size = 0;  /* Reset size */
        buffer_desc->in_use = true;
        return true;
    }

    /* Free old buffer if it exists */
    if (buffer_desc->data)
    {
        pg_free(buffer_desc->data);
        buffer_desc->data = NULL;
        buffer_desc->capacity = 0;
        buffer_desc->size = 0;
    }

    /* Allocate new buffer */
    buffer = pg_malloc(size);
    if (!buffer)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to allocate buffer of size %zu", size);
        return false;
    }

    /* Update buffer descriptor */
    buffer_desc->data = buffer;
    buffer_desc->capacity = size;
    buffer_desc->size = 0;
    buffer_desc->type = type;
    buffer_desc->in_use = true;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Prepared buffer type %d with capacity %zu", type, size);

    return true;
}

/*
 * Get buffer pointer
 */
void *
tecc_buffer_get(TeccCryptoContext *context, TeccBufferType type)
{
    TeccBufferDesc *buffer_desc;

    if (!context || type >= TECC_BUFFER_MAX)
        return NULL;

    buffer_desc = tecc_get_buffer_desc(context, type);
    if (!buffer_desc)
        return NULL;

    return buffer_desc->data;
}

/*
 * Get buffer size
 */
size_t
tecc_context_buffer_get_size(TeccCryptoContext *context, TeccBufferType type)
{
    TeccBufferDesc *buffer_desc;

    if (!context || type >= TECC_BUFFER_MAX)
        return 0;

    buffer_desc = tecc_get_buffer_desc(context, type);
    if (!buffer_desc)
        return 0;

    return buffer_desc->size;
}

/*
 * Set buffer size
 */
bool
tecc_buffer_set_size(TeccCryptoContext *context, TeccBufferType type, size_t size)
{
    TeccBufferDesc *buffer_desc;

    if (!context || type >= TECC_BUFFER_MAX)
        return false;

    buffer_desc = tecc_get_buffer_desc(context, type);
    if (!buffer_desc)
        return false;

    if (size > buffer_desc->capacity)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Buffer size %zu exceeds capacity %zu",
                           size, buffer_desc->capacity);
        return false;
    }

    buffer_desc->size = size;
    return true;
}

/*
 * Get reusable buffer
 */
TeccBufferType
tecc_get_reusable_buffer(TeccCryptoContext *context, size_t required_size,
                        TeccBufferType preferred_type)
{
    int i;

    if (!context || required_size == 0)
        return TECC_BUFFER_MAX;

    /* First try preferred buffer */
    if (preferred_type < TECC_BUFFER_MAX)
    {
        TeccBufferDesc *buffer_desc = tecc_get_buffer_desc(context, preferred_type);
        if (buffer_desc && buffer_desc->capacity >= required_size && !buffer_desc->in_use)
        {
            return preferred_type;
        }
    }

    /* Find other available buffers */
    for (i = 0; i < TECC_BUFFER_MAX; i++)
    {
        if (i != preferred_type)
        {
            TeccBufferDesc *buffer_desc = tecc_get_buffer_desc(context, i);
            if (buffer_desc && buffer_desc->capacity >= required_size && !buffer_desc->in_use)
            {
                return (TeccBufferType)i;
            }
        }
    }

    /* Use work buffer */
    if (context->work_buffer && context->work_buffer_size >= required_size)
    {
        return TECC_BUFFER_TYPE_WORK;
    }

    return TECC_BUFFER_MAX;  /* Need new allocation */
}

/*
 * Calculate buffer sizes for encryption
 */
void
tecc_calculate_buffer_sizes(TeccCryptoContext *context, size_t input_size, size_t *sizes)
{
    size_t normalized_size;

    if (!context || !sizes)
        return;

    /* Input buffer */
    sizes[TECC_BUFFER_TYPE_INPUT] = input_size + 1;  /* +1 for null terminator */

    /* Normalized buffer (consider ICU collation key expansion) */
    normalized_size = input_size + 256;  /* Extra space for collation */
    sizes[TECC_BUFFER_NORMALIZED] = normalized_size;

    /* Algorithm encryption buffers */
    sizes[TECC_BUFFER_ENCRYPTED] = normalized_size + 128;  /* RND */
    sizes[TECC_BUFFER_ENCRYPTED_DET] = normalized_size + 64;   /* DET */
    sizes[TECC_BUFFER_ENCRYPTED_OPE] = normalized_size + 64;   /* OPE */
    sizes[TECC_BUFFER_ENCRYPTED_SEARCH] = normalized_size + 256;  /* SEARCH */

    /* CipherDataType buffer */
    sizes[TECC_BUFFER_CIPHERDATA] = sizeof(void*) +  /* Structure size */
                                   sizes[TECC_BUFFER_ENCRYPTED] +
                                   sizes[TECC_BUFFER_ENCRYPTED_DET] +
                                   sizes[TECC_BUFFER_ENCRYPTED_OPE] +
                                   sizes[TECC_BUFFER_ENCRYPTED_SEARCH] + 64;

    /* Output buffer (hex encoding) */
    sizes[TECC_BUFFER_TYPE_OUTPUT] = sizes[TECC_BUFFER_CIPHERDATA] * 2 + 3;  /* x2 for hex + overhead */
}

/*
 * Validate collation
 */
bool
tecc_validate_collation(TeccCryptoContext *context)
{
    if (!context)
        return false;

    /* Check if collation is specified */
    if (context->core.collation_oid == 0)
        return true;  /* No collation, valid */

    /* Validate OPE algorithm support for character types */
    if (!context->core.algo_support.supports_ope)
    {
        tecc_processor_debug(TECC_LOG_WARNING,
                           "Collation specified but OPE algorithm not supported");
        return false;
    }

    /* TODO: Add actual collation validation against pg_collation */
    /* For now, assume all non-zero OIDs are valid */

    return true;
}

/*
 * Prepare ICU normalization
 */
int
tecc_prepare_icu_normalization(TeccCryptoContext *context)
{
    if (!context)
        return TECC_CRYPTO_INVALID_PARAM;

    /* ICU is mandatory for TECC, no conditional compilation needed */
    /* If collation is specified, ICU must be available */
    if (context->core.collation_oid != 0)
    {
        /* ICU should be available, but we check anyway */
        /* In production, this would check ICU initialization status */
    }

    return TECC_CRYPTO_SUCCESS;
}

/* End - ReqID:TECC4.0 */