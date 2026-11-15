/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_algorithm_scheduler.c
 *
 * Abstract:
 *   Algorithm scheduler implementation for TECC crypto scheduler module.
 *   Implements algorithm selection and execution scheduling.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_scheduler/tecc_algorithm_scheduler.c
 */

#include "tecc_algorithm_scheduler.h"
#include "../tecc_processor/internal/tecc_processor_internal.h"
#include "../tecc_formatter/tecc_formatter.h"  /* Data formatter interface */
#include "../tecc_formatter/tecc_formatter_internal.h"  /* Internal constants */
#include "../tecc_formatter/tecc_formatter_utils.h"  /* Formatter utilities */
#include "../tecc_formatter/tecc_normalization_common.h"  /* Common normalization functions */
#include "../../../common/tecc_types.h"  /* Formatter error code definitions */
#include "common/fe_memutils.h"  /* Frontend memory management */
#include "../tecc_hsm_adapter/fe-hsm-adapter.h"  /* HSM adapter for SM4 encryption */
#include <string.h>
#include <stdlib.h>
/* ReqID:TECC4.0 - HE 批量解密多线程并发：pthread_once 保护惰性初始化、每线程 BN_CTX */
#include <pthread.h>

/* ReqID:TECC4.0 - KEK/DEK unwrap for HE private key decryption (problem 3) */
#undef VARSIZE
#undef SET_VARSIZE
#include "../../../common/tecc_key/security.h"
#include "../../../common/tecc_key/high_level_api.h"
/* Modify - ReqID:TECC4.0: HE 私钥信封解密（tecc_sm2_envelope_decrypt）需显式
 * 包含 tecc_sm2.h（TECC_SM2_SUCCESS / tecc_sm2_envelope_decrypt 声明） */
#include "../../../src/include/common/tecc_key/tecc_sm2.h"

/* ReqID:TECC4.0 - SM4-RND with pre-derived keys (avoid per-row HKDF) */
extern int tecc_sm4_rnd_encrypt_with_keys(const unsigned char *key, size_t key_len,
                        const unsigned char *plaintext, size_t plain_len,
                        unsigned char *output_buffer, size_t buffer_size,
                        size_t *output_len,
                        const unsigned char *derived_enc_key,
                        const unsigned char *derived_auth_key);

/* Algorithm names */
static const char *algorithm_names[] = {
    "UNKNOWN",
    "SM4-RND",
    "DET-SM4",
    "SM4-OPE",
    "SEARCH-SM4",
    "EC-ELGAMAL",    /* ReqID:TECC4.0 - EC-ElGamal additive homomorphic */
    "ELGAMAL"        /* ReqID:TECC4.0 - Finite field ElGamal multiplicative */
};

/* Algorithm capabilities */
static const TeccAlgorithmCaps algorithm_caps[] = {
    {false, false, false, false, 0, 0},                    /* UNKNOWN */
    {true, true, false, false, 65536, 32},                 /* RND */
    {true, true, false, false, 65536, 16},                 /* DET */
    {true, false, true, false, 65536, 64},                 /* OPE */
    {true, false, false, true, 65536, 256},                /* SEARCH */
    {true, false, false, false, 65536, 0},                 /* EC-ELGAMAL (ReqID:TECC4.0) */
    {true, false, false, false, 65536, 0}                 /* ELGAMAL (ReqID:TECC4.0) */
};

/* LIKE pattern types */
/* Pattern type classification - now defined in tecc_pattern.h */
#include "../../../src/include/common/tecc_algorithm/tecc_pattern.h"

/* New unified version of SEARCH-LIKE execution */
static int tecc_execute_algo_search_like_unified(TeccCryptoContextExtended *context,
                                               TeccOperatorType operator_type,
                                               unsigned char **search_ciphertext,
                                               size_t *search_cipher_len);

static int tecc_execute_algo_search_equal(TeccCryptoContextExtended *context,
                                         TeccOperatorType operator_type,
                                         unsigned char **search_ciphertext,
                                         size_t *search_cipher_len);

/* New unified pattern parsing function */
static TeccPatternErrorCode tecc_parse_pattern_unified(
    const char *pattern,
    size_t pattern_len,
    char escape_char,
    TeccParsedPattern **parsed_pattern);

static int tecc_algo_search_encrypt_prefix(const unsigned char *key,
                                          size_t key_len,
                                          const char *prefix,
                                          size_t prefix_len,
                                          unsigned char **ciphertext,
                                          size_t *cipher_len,
                                          char escape_char);

static int tecc_algo_search_encrypt_exact(const unsigned char *key,
                                         size_t key_len,
                                         const char *pattern,
                                         size_t pattern_len,
                                         unsigned char **ciphertext,
                                         size_t *cipher_len,
                                         char escape_char);

/*
 * normalize_data_for_algorithm - Normalize data based on algorithm and data type
 *
 * This function calls the data formatter module to normalize data before
 * passing it to the algorithm module. For OPE and DET algorithms, data
 * must be normalized to ensure correct ordering and comparison.
 */
static TeccAlgorithmErrorCode
normalize_data_for_algorithm(TeccCryptoContext *context,
                            TeccAlgorithmType algo_type,
                            const unsigned char **normalized_data,
                            size_t *normalized_len)
{
    TeccFormatterErrorCode fmt_result = TECC_FORMATTER_ERROR_SYSTEM;
    unsigned char *fmt_data = NULL;
    size_t fmt_len = 0;
    bool need_normalization = false;
    TeccCryptoContextCore core_context;
    TeccCryptoBuffer *buffer = NULL;  /* For zero-copy normalization */
    size_t required_size = 0;

    if (!context || !normalized_data || !normalized_len)
        return TECC_ALGO_INVALID_PARAM;

    /* Initialize output parameters */
    *normalized_data = NULL;
    *normalized_len = 0;

    /* Check if normalization is needed */

    switch (algo_type)
    {
        case TECC_ALGO_OPE:
        case TECC_ALGO_DET:
            /* OPE and DET algorithms require normalization */
            need_normalization = true;
            break;

        case TECC_ALGO_RND:
        case TECC_ALGO_SEARCH:
            /* RND and SEARCH algorithms don't require normalization */
            need_normalization = false;
            break;

        default:
            return TECC_ALGO_OPERATION_NOT_SUPPORTED;
    }

    if (!need_normalization)
    {
        /* For algorithms that don't require normalization, use original data */
        *normalized_data = context->plaintext;
        *normalized_len = context->plaintext_len;
        return TECC_ALGO_SUCCESS;
    }

	/* ReqID:TECC4.0 - Use pre-normalized data when available to avoid
	 * re-normalizing with the full padded plaintext_len for text types.
	 * tecc_crypto_scheduler.c pre-computes normalized_plaintext with
	 * rtrim applied (plaintext_len - text_space_count), which ensures
	 * BPCHAR and TEXT produce identical encrypted cores for semantically
	 * identical values. Without this, BPCHAR's padded plaintext_len
	 * (e.g. 10 for 'abc' in bpchar(10)) would be used instead of the
	 * rtrim'd length (3), producing a different ciphertext from TEXT. */
	/* ReqID:TECC4.0 - Pre-normalized bypass: only valid for DET.
	 * OPE encryption needs algorithm-specific normalize (12-byte chunk
	 * alignment for text types) which produces different normalized_len
	 * than DET. Using the DET pre-normalized result for OPE causes
	 * INSERT and WHERE paths to produce different OPE ciphertext
	 * lengths, breaking server-side range comparison. */
	if (context->data_pre_normalized &&
		context->normalized_plaintext != NULL &&
		context->normalized_plaintext_len > 0 &&
		algo_type == TECC_ALGO_DET)
	{
		*normalized_data = context->normalized_plaintext;
		*normalized_len = context->normalized_plaintext_len;
		return TECC_ALGO_SUCCESS;
	}


    /* Call data formatter module for normalization */
    /* We need to convert TeccCryptoContext to TeccCryptoContextCore for the formatter API */

    /* Copy relevant fields from context to core_context */
    memset(&core_context, 0, sizeof(TeccCryptoContextCore));
    core_context.data_type = context->core.data_type;
    core_context.collation_oid = context->core.collation_oid;
    core_context.type_mod = context->core.type_mod;
    strlcpy(core_context.collation_locale, context->core.collation_locale, NAMEDATALEN);
    core_context.algo_type = algo_type;  /* Set algorithm type for formatter */
    /* Copy algorithm support and selection from context->core */
    core_context.algo_support = context->core.algo_support;
    core_context.selected_algorithms = context->core.selected_algorithms;

    /* Try zero-copy normalization first if possible */

    /* Calculate required buffer size for normalized data */
    required_size = tecc_formatter_calculate_normalized_size(&core_context,
                                                            context->plaintext_len);

    /* ReqID:TECC4.0 - Empty data: always fall through to formatter.
     * All text types must produce a canonical 16-byte zero block for
     * empty-string cores (handled by tecc_normalize_text_to_buffer).
     * Previously non-text/non-collation paths returned 0-length data,
     * which caused DET/OPE to encrypt empty input inconsistently. */
    /* Fall through: formatter handles plaintext_len==0 uniformly */

    if (required_size > 0)
    {
        /* Allocate buffer for zero-copy operation */
        buffer = pg_malloc0(sizeof(TeccCryptoBuffer));
        if (buffer)
        {
            buffer->data = pg_malloc(required_size);
            if (buffer->data)
            {
                /* ReqID:TECC4.0 - For text types, use rtrim'd length
                 * (plaintext_len - text_space_count) instead of the full
                 * plaintext_len. This ensures OPE encryption uses the same
                 * input data as DET (rtrimmed content), producing consistent
                 * ciphertext lengths between INSERT and WHERE paths.
                 * WHERE path already reduces plaintext_len via rtrim;
                 * INSERT path keeps plaintext_len at the padded length,
                 * so the text_space_count adjustment is critical there. */
                buffer->size = required_size;
                if (TECC_IS_TEXT_TYPE(context->core.data_type) &&
                    context->text_space_count > 0 &&
                    context->plaintext_len > context->text_space_count)
                    buffer->used = context->plaintext_len - context->text_space_count;
                else
                    buffer->used = context->plaintext_len;
                buffer->owned_by_caller = true;

                /* Copy input data to buffer */
                memcpy(buffer->data, context->plaintext, buffer->used);
                /* ReqID:TECC4.0 - null-terminate for strtod validation */
                if (buffer->size > buffer->used)
                    ((unsigned char *)buffer->data)[buffer->used] = '\0';

                /* Try zero-copy in-place normalization */
                fmt_result = tecc_formatter_normalize_data(&core_context, buffer->data, buffer->used, buffer);
                if (fmt_result == TECC_FORMATTER_SUCCESS)
                {
                    /* Zero-copy succeeded, use buffer data directly */
                    fmt_data = (unsigned char *)buffer->data;
                    fmt_len = buffer->used;

                    /* Detach data from buffer before freeing buffer structure */
                    /* The data is now owned by fmt_data and will be freed by caller */
                    buffer->data = NULL;  /* Detach data from buffer */
                    pg_free(buffer);      /* Free only the buffer structure, not the data */
                    buffer = NULL;

                    /* Skip to success handling */
                    goto normalization_success;
                }

                /* Zero-copy failed or allocation failed, cleanup and fallback */
                if (buffer)
                {
                    if (buffer->data)
                        pg_free(buffer->data);
                    pg_free(buffer);
                    buffer = NULL;
                }
            }
            else
            {
                pg_free(buffer);
                buffer = NULL;
            }
        }
    }

    /* Zero-copy normalization should always succeed if buffer allocation succeeded */
    /* If we reach here, it means zero-copy normalization failed */
    if (fmt_data)
        pg_free(fmt_data);

    /* Convert formatter error to algorithm error */
    switch (fmt_result)
    {
        /* Parameter errors (maps to TECC_ALGO_INVALID_PARAM) */
        case TECC_FORMATTER_ERROR_INVALID_PARAM:
        case TECC_FORMATTER_ERROR_NULL_POINTER:
        case TECC_FORMATTER_ERROR_INVALID_TYPE:
        case TECC_FORMATTER_ERROR_INVALID_INPUT:
        case TECC_FORMATTER_ERROR_UNSUPPORTED_TYPE:
            return TECC_ALGO_INVALID_PARAM;

        /* Data size error (maps to TECC_ALGO_DATA_TOO_LARGE) */
        case TECC_FORMATTER_ERROR_DATA_TOO_LARGE:
            return TECC_ALGO_DATA_TOO_LARGE;

        /* Memory errors (maps to TECC_ALGO_MEMORY_ERROR) */
        case TECC_FORMATTER_ERROR_MEMORY_ALLOC:
        case TECC_FORMATTER_ERROR_BUFFER_TOO_SMALL:
            return TECC_ALGO_MEMORY_ERROR;

        /* Data format errors (maps to TECC_ALGO_DATA_FORMAT_ERROR) */
        case TECC_FORMATTER_ERROR_DATA_FORMAT:
        case TECC_FORMATTER_ERROR_ENCODING_FAILED:
        case TECC_FORMATTER_ERROR_DECODING_FAILED:
        case TECC_FORMATTER_ERROR_CONVERSION_FAILED:
        case TECC_FORMATTER_ERROR_INVALID_NUMERIC:
        case TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW:
            return TECC_ALGO_DATA_FORMAT_ERROR;

        /* Format errors (maps to TECC_ALGO_SYSTEM_ERROR) */
        case TECC_FORMATTER_ERROR_INVALID_HEADER:
        case TECC_FORMATTER_ERROR_VERSION_MISMATCH:
        case TECC_FORMATTER_ERROR_BUILD_FAILED:
            return TECC_ALGO_SYSTEM_ERROR;

        /* ICU collation errors (maps to TECC_ALGO_SYSTEM_ERROR) */
        case TECC_FORMATTER_ERROR_ICU_NOT_AVAILABLE:
        case TECC_FORMATTER_ERROR_COLLATION_INVALID:
        case TECC_FORMATTER_ERROR_SORTKEY_FAILED:
            return TECC_ALGO_SYSTEM_ERROR;

        /* System errors (maps to TECC_ALGO_SYSTEM_ERROR) */
        case TECC_FORMATTER_ERROR_SYSTEM:
            return TECC_ALGO_SYSTEM_ERROR;

        /* Success case (should not reach here) */
        case TECC_FORMATTER_SUCCESS:
        default:
            return TECC_ALGO_SYSTEM_ERROR;
    }

normalization_success:
    /* ReqID:TECC4.0 - dscale 由调用方
     * (tecc_execute_algo_det / tecc_execute_algo_ope) 在加密前截掉,
     * 此处不做, 避免双重截断导致丢失有效 digit 数据。 */

    *normalized_data = fmt_data;
    *normalized_len = fmt_len;

    /* Note: The caller is responsible for freeing fmt_data */
    return TECC_ALGO_SUCCESS;
}

/*
 * Select algorithms based on context configuration
 */
int
tecc_select_algorithms(TeccCryptoContext *context)
{
    if (!context)
        return TECC_CRYPTO_INVALID_PARAM;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Selecting algorithms for %s.%s (type=%u, collation=%u)",
                        context->table_name, context->column_name,
                        context->core.data_type, context->core.collation_oid);

    /* Reset algorithm selection */
    context->core.selected_algorithms = 0;

    /* RND algorithm is always required */
    context->core.selected_algorithms |= TECC_ALGO_RND;

    /* Select DET if supported */
    if (context->core.algo_support.supports_det)
    {
        context->core.selected_algorithms |= TECC_ALGO_DET;
        tecc_processor_debug(TECC_LOG_DEBUG1, "Selected DET algorithm");
    }

    /* Select OPE if supported */
    if (context->core.algo_support.supports_ope)
    {
        context->core.selected_algorithms |= TECC_ALGO_OPE;
        tecc_processor_debug(TECC_LOG_DEBUG1, "Selected OPE algorithm");
    }

    /* Select SEARCH if supported */
    if (context->core.algo_support.supports_search)
    {
        context->core.selected_algorithms |= TECC_ALGO_SEARCH;
        tecc_processor_debug(TECC_LOG_DEBUG1, "Selected SEARCH algorithm");
    }

    

    return TECC_CRYPTO_SUCCESS;
}

/*
 * Select algorithms with specific criteria
 */
int
tecc_select_algorithms_with_criteria(TeccCryptoContext *context,
                                    const TeccAlgorithmSelection *criteria)
{
    if (!context || !criteria)
        return TECC_CRYPTO_INVALID_PARAM;

    /* Reset algorithm selection */
    context->core.selected_algorithms = 0;

    /* Always include RND */
    context->core.selected_algorithms |= TECC_ALGO_RND;

    /* Apply criteria */
    if (criteria->require_det && context->core.algo_support.supports_det)
    {
        context->core.selected_algorithms |= TECC_ALGO_DET;
    }
    else if (criteria->require_det && !criteria->allow_fallback)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "DET algorithm required but not supported");
        return TECC_CRYPTO_ALGO_ERROR;
    }

    if (criteria->require_ope && context->core.algo_support.supports_ope)
    {
        context->core.selected_algorithms |= TECC_ALGO_OPE;
    }
    else if (criteria->require_ope && !criteria->allow_fallback)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "OPE algorithm required but not supported");
        return TECC_CRYPTO_ALGO_ERROR;
    }

    if (criteria->require_search && context->core.algo_support.supports_search)
    {
        context->core.selected_algorithms |= TECC_ALGO_SEARCH;
    }
    else if (criteria->require_search && !criteria->allow_fallback)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "SEARCH algorithm required but not supported");
        return TECC_CRYPTO_ALGO_ERROR;
    }

    return TECC_CRYPTO_SUCCESS;
}

/*
 * Check if algorithm is supported
 */
bool
tecc_is_algorithm_supported(const TeccCryptoContext *context,
                           TeccAlgorithmType algo_type)
{
    if (!context || algo_type <= 0 || algo_type >= TECC_ALGO_MAX)
        return false;

    switch (algo_type)
    {
        case TECC_ALGO_RND:
            return true;  /* Always supported */
        case TECC_ALGO_DET:
            return context->core.algo_support.supports_det;
        case TECC_ALGO_OPE:
            return context->core.algo_support.supports_ope;
        case TECC_ALGO_SEARCH:
            return context->core.algo_support.supports_search;
        default:
            return false;
    }
}

/*
 * Execute all selected algorithms
 */
int
tecc_execute_algorithms(TeccCryptoContext *context)
{
    int ret;

    if (!context)
        return TECC_CRYPTO_INVALID_PARAM;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Executing algorithms: 0x%02X", context->core.selected_algorithms);

    /* ReqID:TECC4.0 - Save original plaintext before RND execution.
     * RND may replace context->plaintext with pre-normalized binary;
     * DET and OPE still need the original string for their own normalize. */
    {
        const unsigned char *saved_pt = context->plaintext;
        size_t saved_pt_len = context->plaintext_len;


        /* Execute RND algorithm (mandatory) */
        if (context->core.selected_algorithms & TECC_ALGO_RND)
        {
            ret = tecc_execute_algo_rnd(context, NULL, NULL);
            if (ret != TECC_ALGO_SUCCESS)
            {
                tecc_processor_debug(TECC_LOG_ERROR,
                                   "RND algorithm execution failed: %d", ret);
                return TECC_CRYPTO_SCHEDULE_ERROR;
            }
        }

        /* Restore original plaintext for DET/OPE normalize */
        context->plaintext = saved_pt;
        context->plaintext_len = saved_pt_len;

        /* Execute DET algorithm */
        if (context->core.selected_algorithms & TECC_ALGO_DET)
        {
            ret = tecc_execute_algo_det(context);
            if (ret != TECC_ALGO_SUCCESS)
            {
                tecc_processor_debug(TECC_LOG_WARNING,
                                   "DET algorithm execution failed: %d (non-fatal)", ret);
                /* DET failure is not fatal */
            }
        }

        /* Execute OPE algorithm */
        if (context->core.selected_algorithms & TECC_ALGO_OPE)
        {
            ret = tecc_execute_algo_ope(context);
            if (ret != TECC_ALGO_SUCCESS)
            {
                tecc_processor_debug(TECC_LOG_WARNING,
                                   "OPE algorithm execution failed: %d (non-fatal)", ret);
                /* OPE failure is not fatal */
            }
        }
    }

    /* Execute SEARCH algorithm */
    if (context->core.selected_algorithms & TECC_ALGO_SEARCH)
    {
        ret = tecc_execute_algo_search(context);
        if (ret != TECC_ALGO_SUCCESS)
        {
            tecc_processor_debug(TECC_LOG_WARNING,
                               "SEARCH algorithm execution failed: %d (non-fatal)", ret);
            /* SEARCH failure is not fatal */
        }
    }

    return TECC_CRYPTO_SUCCESS;
}


/*
 * Execute RND algorithm
 */
int
tecc_execute_algo_rnd(TeccCryptoContext *context,
                      const unsigned char *derived_enc_key,
                      const unsigned char *derived_auth_key)
{
    int ret;
    unsigned char *output_buffer = NULL;
    size_t output_len = 0;
    size_t required_size = 0;
    TeccCryptoContextExtended *ext_context = NULL;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Executing RND algorithm with zero-copy interface");

    /* Validate context */
    if (!context || !context->plaintext)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid context for RND algorithm");
        return TECC_ALGO_INVALID_PARAM;
    }

    /* Allow empty plaintext (plaintext_len == 0) — encrypt as empty data */
    if (context->plaintext_len == 0)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "RND algorithm: encrypting empty plaintext (len=0)");
    }

    /* Get extended context for key access */
    ext_context = TECC_CONTEXT_EXTENDED(context);
    if (!ext_context)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to get extended context for RND algorithm");
        return TECC_ALGO_INVALID_PARAM;
    }

    /* ReqID:TECC4.0 - For numeric types the scheduler pre-normalizes.
     * Use normalized_plaintext when available. */
    if (ext_context->normalized_plaintext != NULL &&
        ext_context->normalized_plaintext_len > 0)
    {
        context->plaintext = ext_context->normalized_plaintext;
        context->plaintext_len = ext_context->normalized_plaintext_len;
    }

    /* Check if DEK key is available */
    if (!ext_context->dek_key || ext_context->dek_key_len == 0)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "DEK key not available for RND algorithm");
        return TECC_ALGO_INVALID_KEY_LENGTH;
    }

    /* Calculate required buffer size using algorithm API */
    required_size = tecc_algo_calc_encrypt_buffer_size(TECC_ALGO_RND,
                                                      context->plaintext_len);
    if (required_size == 0)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to calculate RND algorithm buffer size");
        return TECC_ALGO_INVALID_PARAM;
    }

    /* Allocate output buffer */
    output_buffer = pg_malloc(required_size);
    if (!output_buffer)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to allocate RND algorithm output buffer");
        return TECC_ALGO_MEMORY_ERROR;
    }

    /* ReqID:TECC4.0 - Check if HSM SM4 encryption is available.
     * HSM path is only reached when tecc_setup_hsm_context() already
     * verified HSM availability at the crypto_scheduler level. */
    if (ext_context->hsm_adapter != NULL && ext_context->hsm_device != NULL)
    {
        HsmAdapterInterface *hsm = (HsmAdapterInterface *)ext_context->hsm_adapter;

        if (hsm->sm4_encrypt == NULL)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "HSM adapter configured but sm4_encrypt is NULL");
            pg_free(output_buffer);
            return TECC_ALGO_ENCRYPTION_FAILED;
        }

        {
            unsigned char sm4_key[TECC_SM4_KEY_SIZE];
            unsigned char iv[16];
            size_t actual_key_len;
            size_t enc_len;
            HsmErrorCode hsm_ret;
            unsigned char iv_save[16];

            actual_key_len = (ext_context->dek_key_len >= TECC_SM4_KEY_SIZE) ?
                             TECC_SM4_KEY_SIZE : ext_context->dek_key_len;
            memcpy(sm4_key, ext_context->dek_key, actual_key_len);

            /* ReqID:TECC4.0 - 使用密码学安全随机数生成IV */
            security_generate_random_bytes(iv, 16);
            memcpy(iv_save, iv, 16);

            enc_len = required_size - 16;
            TECC_LOG(TECC_LOG_DEBUG1, "TECC_ROUTE: executing RND via HSM SM4 (HARDWARE encryption)");
            hsm_ret = hsm->sm4_encrypt(ext_context->hsm_device,
                                       sm4_key, actual_key_len,
                                       iv,
                                       context->plaintext, context->plaintext_len,
                                       output_buffer + 16, &enc_len);
            memset(sm4_key, 0, sizeof(sm4_key));

            if (hsm_ret != HSM_SUCCESS)
            {
                tecc_processor_debug(TECC_LOG_ERROR,
                                   "HSM SM4 encrypt failed for RND: %d", hsm_ret);
                pg_free(output_buffer);
                return TECC_ALGO_ENCRYPTION_FAILED;
            }

            memcpy(output_buffer, iv_save, 16);
            output_len = 16 + enc_len;
        }

        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "RND algorithm executed via HSM SM4, output_len=%zu",
                            output_len);
        ret = TECC_ALGO_SUCCESS;
    }
    else
    {
        /* ReqID:TECC4.0 - Software SM4 path (normal path for software KEKs).
         * Use pre-derived keys when available to skip per-row HKDF. */
        unsigned char sm4_key[TECC_SM4_KEY_SIZE];
        size_t actual_key_len;

        TECC_LOG(TECC_LOG_DEBUG1, "TECC_ROUTE: executing RND via SOFTWARE SM4");
        actual_key_len = (ext_context->dek_key_len >= TECC_SM4_KEY_SIZE) ?
                         TECC_SM4_KEY_SIZE : ext_context->dek_key_len;
        memcpy(sm4_key, ext_context->dek_key, actual_key_len);

        if (derived_enc_key != NULL && derived_auth_key != NULL)
        {
            ret = tecc_sm4_rnd_encrypt_with_keys(sm4_key, actual_key_len,
                                                 context->plaintext, context->plaintext_len,
                                                 output_buffer, required_size, &output_len,
                                                 derived_enc_key, derived_auth_key);
        }
        else
        {
            ret = tecc_algo_encrypt_zero_copy(TECC_ALGO_RND,
                                             sm4_key, actual_key_len,
                                             context->plaintext, context->plaintext_len,
                                             output_buffer, required_size, &output_len, false, 0);
        }
        memset(sm4_key, 0, sizeof(sm4_key));

        if (ret != TECC_ALGO_SUCCESS)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "RND algorithm encryption failed: %d", ret);
            pg_free(output_buffer);
            return ret;
        }
    }

    /* Free any previously allocated buffer */
    if (context->rnd_ciphertext)
    {
        pg_free(context->rnd_ciphertext);
        context->rnd_ciphertext = NULL;
    }

    /* Set ciphertext in context */
    context->rnd_cipher_len = output_len;
    context->rnd_ciphertext = output_buffer;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "RND algorithm execution completed, length: %zu",
                        output_len);

    return TECC_ALGO_SUCCESS;
}

/*
 * Execute DET algorithm
 */
int
tecc_execute_algo_det(TeccCryptoContext *context)
{
    int ret;
    const unsigned char *normalized_data = NULL;
    size_t normalized_len = 0;
    bool data_normalized = false;
    size_t required_size;
    unsigned char *output_buffer;
    size_t output_len;
    TeccCryptoContextExtended *ext_context = NULL;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Executing DET algorithm with zero-copy interface");

    /* Validate context */
    if (!context || !context->plaintext)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid context for DET algorithm");
        return TECC_ALGO_INVALID_PARAM;
    }

    if (context->plaintext_len == 0)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "DET algorithm: encrypting empty plaintext (len=0)");
    }

    /* Get extended context for key access */
    ext_context = TECC_CONTEXT_EXTENDED(context);
    if (!ext_context)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to get extended context for DET algorithm");
        return TECC_ALGO_INVALID_PARAM;
    }

    /* Check if DEK key is available */
    if (!ext_context->dek_key || ext_context->dek_key_len == 0)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "DEK key not available for DET algorithm");
        return TECC_ALGO_INVALID_KEY_LENGTH;
    }

    /* Step 1: Normalize data for DET algorithm */
    ret = normalize_data_for_algorithm(context, TECC_ALGO_DET,
                                      &normalized_data, &normalized_len);
    if (ret == TECC_ALGO_SUCCESS)
    {
        /* Normalization succeeded — check if new buffer was allocated */
        if (!data_normalized)
            data_normalized = (normalized_data != context->plaintext);
    }
    else
    {
        /* ReqID:TECC4.0 - Normalization failed (e.g. binary input). Fall back
         * to context->plaintext (possibly already RND-normalized binary with
         * dscale at end). */
        tecc_processor_debug(TECC_LOG_DEBUG1,
            "DET normalization failed (code=%d), using context->plaintext (len=%zu)",
            ret, context->plaintext_len);
        normalized_data = context->plaintext;
        normalized_len = context->plaintext_len;
        data_normalized = false;
    }

    /* Calculate required buffer size using algorithm API */
    required_size = tecc_algo_calc_encrypt_buffer_size(TECC_ALGO_DET,
                                                      normalized_len);
    if (required_size == 0)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to calculate DET algorithm buffer size");
        /* Cleanup normalized data if allocated */
        if (data_normalized && normalized_data && normalized_data != ext_context->normalized_plaintext)
            pg_free((void *)normalized_data);
        return TECC_ALGO_INVALID_PARAM;
    }

    /* Allocate output buffer */
    output_buffer = pg_malloc(required_size);
    if (!output_buffer)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to allocate DET algorithm output buffer");
        /* Cleanup normalized data if allocated */
        if (data_normalized && normalized_data && normalized_data != ext_context->normalized_plaintext)
            pg_free((void *)normalized_data);
        return TECC_ALGO_MEMORY_ERROR;
    }

    /* ReqID:TECC4.0 - DET加密忽略末尾2字节dscale，确保相同数值（如1.0和1）
     * 产生相同密文。dscale只在RND密文中传递用于解密输出精度恢复。 */
    if (normalized_len >= 2 &&
        (context->core.data_type == 21 /* INT2OID */ ||
         context->core.data_type == 23 /* INT4OID */ ||
         context->core.data_type == 20 /* INT8OID */ ||
         context->core.data_type == 700 /* FLOAT4OID */ ||
         context->core.data_type == 701 /* FLOAT8OID */ ||
         context->core.data_type == 1700 /* NUMERICOID */))
    {
        normalized_len -= 2;
    }

    /* ReqID:TECC4.0 - Debug: log normalized data before DET encryption */
    /* Call zero-copy algorithm API with normalized data and real DEK key */
    output_len = 0;
    ret = tecc_algo_encrypt_zero_copy(TECC_ALGO_DET,
                                     ext_context->dek_key, ext_context->dek_key_len,
                                     normalized_data, normalized_len,
                                     output_buffer, required_size, &output_len, false, 0);

    if (ret != TECC_ALGO_SUCCESS)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "DET algorithm encryption failed: %d", ret);
        pg_free(output_buffer);
        /* Cleanup normalized data if allocated */
        if (data_normalized && normalized_data && normalized_data != ext_context->normalized_plaintext)
            pg_free((void *)normalized_data);
        return ret;
    }

    /* Free any previously allocated buffer */
    if (context->det_ciphertext)
    {
        pg_free(context->det_ciphertext);
        context->det_ciphertext = NULL;
    }

    /* ReqID:TECC4.0 - Encode text trailing-space format for encrypted comparison.
     * Text types wrap DET ciphertext with [format_byte][encrypted_core][space_count:4]. */
    if (TECC_IS_TEXT_TYPE(context->core.data_type) && context->text_space_count > 0)
    {
        const uint32_t space_cnt = context->text_space_count;
        size_t new_len;
        unsigned char *new_buf;

        new_len = 1 + output_len + 4;  /* format_byte + core + space_count */
        new_buf = (unsigned char *)pg_malloc(new_len);
        if (new_buf != NULL)
        {
            new_buf[0] = 0x01;  /* has trailing spaces */
            memcpy(new_buf + 1, output_buffer, output_len);
            new_buf[1 + output_len + 0] = (unsigned char)((space_cnt >> 24) & 0xFF);
            new_buf[1 + output_len + 1] = (unsigned char)((space_cnt >> 16) & 0xFF);
            new_buf[1 + output_len + 2] = (unsigned char)((space_cnt >> 8) & 0xFF);
            new_buf[1 + output_len + 3] = (unsigned char)(space_cnt & 0xFF);
            pg_free(output_buffer);
            output_buffer = new_buf;
            output_len = new_len;
        }
    }
    else if (TECC_IS_TEXT_TYPE(context->core.data_type))
    {
        unsigned char *new_buf2;
        size_t new_len;

        new_len = 1 + output_len + 4;  /* format_byte + core + space_count */
        new_buf2 = (unsigned char *)pg_malloc(new_len);
        if (new_buf2 != NULL)
        {
            new_buf2[0] = 0x00;  /* no trailing spaces */
            memcpy(new_buf2 + 1, output_buffer, output_len);
            /* space_count = 0, 4 bytes */
            new_buf2[1 + output_len + 0] = 0;
            new_buf2[1 + output_len + 1] = 0;
            new_buf2[1 + output_len + 2] = 0;
            new_buf2[1 + output_len + 3] = 0;
            pg_free(output_buffer);
            output_buffer = new_buf2;
            output_len = new_len;
        }
    }

    /* Set ciphertext in context */
    context->det_cipher_len = output_len;
    context->det_ciphertext = output_buffer;

    /* Cleanup normalized data if allocated */
    if (data_normalized && normalized_data && normalized_data != ext_context->normalized_plaintext)
            pg_free((void *)normalized_data);

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "DET algorithm execution completed, length: %zu",
                        output_len);

    return TECC_ALGO_SUCCESS;
}

/*
 * Execute OPE algorithm
 */
int
tecc_execute_algo_ope(TeccCryptoContext *context)
{
    int ret;
    const unsigned char *normalized_data = NULL;
    size_t normalized_len = 0;
    bool data_normalized = false;
    size_t required_size;
    unsigned char *output_buffer;
    size_t output_len;
    TeccCryptoContextExtended *ext_context = NULL;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Executing OPE algorithm with buffer");

    /* Validate context */
    if (!context || !context->plaintext)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid context for OPE algorithm: context=%p, plaintext=%p, plaintext_len=%zu",
                           context, context ? context->plaintext : NULL, context ? context->plaintext_len : 0);
        return TECC_ALGO_INVALID_PARAM;
    }

    /* Get extended context for key access */
    ext_context = TECC_CONTEXT_EXTENDED(context);
    if (!ext_context)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to get extended context for OPE algorithm");
        return TECC_ALGO_INVALID_PARAM;
    }

    /* Check if DEK key is available */
    if (!ext_context->dek_key || ext_context->dek_key_len == 0)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "DEK key not available for OPE algorithm");
        return TECC_ALGO_INVALID_KEY_LENGTH;
    }

    /* Step 1: Normalize data for OPE algorithm */
    ret = normalize_data_for_algorithm(context, TECC_ALGO_OPE,
                                      &normalized_data, &normalized_len);
    if (ret == TECC_ALGO_SUCCESS)
    {
        /* Normalization succeeded — check if new buffer was allocated */
        if (!data_normalized)
            data_normalized = (normalized_data != context->plaintext);
    }
    else
    {
        /* ReqID:TECC4.0 - Normalization failed (e.g. binary input). Fall back
         * to context->plaintext (possibly already RND-normalized binary with
         * dscale at end) and downgrade data_normalized to false. */
        tecc_processor_debug(TECC_LOG_DEBUG1,
            "OPE normalization failed (code=%d), using context->plaintext (len=%zu)",
            ret, context->plaintext_len);
        normalized_data = context->plaintext;
        normalized_len = context->plaintext_len;
        data_normalized = false;
    }

    /* ReqID:TECC4.0 - For text types with OPE, pad to 12-byte multiple.
     * normalize_data_for_algorithm(TECC_ALGO_OPE) above already handles
     * ICU sort key + 12-byte padding via tecc_normalize_text_to_buffer
     * when collation is set. Here we only need to handle the no-collation
     * case (binary padding). */
    if (TECC_IS_TEXT_TYPE(context->core.data_type))
    {
        if (context->core.collation_locale[0] != '\0'
            && !tecc_is_raw_byte_collation(context->core.collation_locale))
        {
            /* ICU path: normalize_data_for_algorithm already generated
             * ICU sort key and padded to 12-byte multiple. Skip secondary
             * processing to avoid double sort key generation. */
            tecc_processor_debug(TECC_LOG_DEBUG1, "OPE ICU path: normalized_len=%zu (skipping secondary padding)",
                                normalized_len);
        }
        else
        {
            unsigned char *ope_padded = NULL;
            size_t ope_padded_len = 0;
            TeccFormatterErrorCode pad_ret;

            /* Binary path: pad raw text to 12-byte multiple */
            pad_ret = tecc_formatter_ope_text_pad(normalized_data, normalized_len,
                                                 &ope_padded, &ope_padded_len);
            if (pad_ret != TECC_FORMATTER_SUCCESS)
            {
                tecc_processor_debug(TECC_LOG_ERROR,
                                   "Failed to normalize text for OPE: %d", (int)pad_ret);
                if (data_normalized && normalized_data != ext_context->normalized_plaintext)
                    pg_free((void *)normalized_data);
                return TECC_ALGO_INVALID_PARAM;
            }

            /* Free old normalized data if it was allocated */
            if (data_normalized && normalized_data != ext_context->normalized_plaintext)
                pg_free((void *)normalized_data);

            normalized_data = ope_padded;
            normalized_len = ope_padded_len;
            data_normalized = true;
        }
    }

    /* Calculate required buffer size using algorithm API */
    required_size = tecc_algo_calc_encrypt_buffer_size(TECC_ALGO_OPE,
                                                             normalized_len);
    if (required_size == 0)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to calculate OPE algorithm buffer size");
        /* Cleanup normalized data if allocated */
        if (data_normalized && normalized_data && normalized_data != ext_context->normalized_plaintext)
            pg_free((void *)normalized_data);
        return TECC_ALGO_INVALID_PARAM;
    }

    /* Allocate output buffer */
    output_buffer = pg_malloc(required_size);
    if (!output_buffer)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to allocate OPE algorithm output buffer");
        /* Cleanup normalized data if allocated */
        if (data_normalized && normalized_data && normalized_data != ext_context->normalized_plaintext)
            pg_free((void *)normalized_data);
        return TECC_ALGO_MEMORY_ERROR;
    }

    /* ReqID:TECC4.0 - OPE加密忽略末尾2字节dscale，确保相同数值产生相同密文 */
    if (normalized_len >= 2 &&
        (context->core.data_type == 21 || context->core.data_type == 23 ||
         context->core.data_type == 20 || context->core.data_type == 700 ||
         context->core.data_type == 701 || context->core.data_type == 1700))
    {
        normalized_len -= 2;
    }

    /* Call zero-copy algorithm API with normalized data and real DEK key */
    output_len = 0;
    ret = tecc_algo_encrypt_zero_copy(TECC_ALGO_OPE,
                                     ext_context->dek_key, ext_context->dek_key_len,
                                     normalized_data, normalized_len,
                                     output_buffer, required_size, &output_len, false, 0);

    if (ret != TECC_ALGO_SUCCESS)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "OPE algorithm encryption failed: %d", ret);
        pg_free(output_buffer);
        /* Cleanup normalized data if allocated */
        if (data_normalized && normalized_data && normalized_data != ext_context->normalized_plaintext)
            pg_free((void *)normalized_data);
        return ret;
    }

    /* Free any previously allocated buffer */
    if (context->ope_ciphertext)
    {
        pg_free(context->ope_ciphertext);
        context->ope_ciphertext = NULL;
    }

    /* ReqID:TECC4.0 - Encode text trailing-space format for encrypted comparison.
     * Text types wrap OPE ciphertext with [format_byte][encrypted_core][space_count:4]. */
    if (TECC_IS_TEXT_TYPE(context->core.data_type) && context->text_space_count > 0)
    {
        const uint32_t space_cnt = context->text_space_count;
        unsigned char *new_buf;
        size_t new_len;

        new_len = 1 + output_len + 4;
        new_buf = (unsigned char *)pg_malloc(new_len);
        if (new_buf != NULL)
        {
            new_buf[0] = 0x01;
            memcpy(new_buf + 1, output_buffer, output_len);
            new_buf[1 + output_len + 0] = (unsigned char)((space_cnt >> 24) & 0xFF);
            new_buf[1 + output_len + 1] = (unsigned char)((space_cnt >> 16) & 0xFF);
            new_buf[1 + output_len + 2] = (unsigned char)((space_cnt >> 8) & 0xFF);
            new_buf[1 + output_len + 3] = (unsigned char)(space_cnt & 0xFF);
            pg_free(output_buffer);
            output_buffer = new_buf;
            output_len = new_len;
        }
    }
    else if (TECC_IS_TEXT_TYPE(context->core.data_type))
    {
        unsigned char *new_buf;
        size_t new_len;

        new_len = 1 + output_len + 4;
        new_buf = (unsigned char *)pg_malloc(new_len);
        if (new_buf != NULL)
        {
            new_buf[0] = 0x00;
            memcpy(new_buf + 1, output_buffer, output_len);
            new_buf[1 + output_len + 0] = 0;
            new_buf[1 + output_len + 1] = 0;
            new_buf[1 + output_len + 2] = 0;
            new_buf[1 + output_len + 3] = 0;
            pg_free(output_buffer);
            output_buffer = new_buf;
            output_len = new_len;
        }
    }

    /* Set ciphertext in context */
    context->ope_cipher_len = output_len;
    context->ope_ciphertext = output_buffer;

    /* Cleanup normalized data if allocated */
    if (data_normalized && normalized_data && normalized_data != ext_context->normalized_plaintext)
            pg_free((void *)normalized_data);

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "OPE algorithm execution completed, length: %zu",
                        output_len);

    return TECC_ALGO_SUCCESS;
}

/*
 * Execute SEARCH algorithm
 */
int
tecc_execute_algo_search(TeccCryptoContext *context)
{
    int ret;
    size_t required_size;
    unsigned char *output_buffer;
    size_t output_len;
    TeccCryptoContextExtended *ext_context = NULL;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Executing SEARCH algorithm with zero-copy interface");

    /* Validate context */
    if (!context || !context->plaintext)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid context for SEARCH algorithm");
        return TECC_ALGO_INVALID_PARAM;
    }

    /* Get extended context for key access */
    ext_context = TECC_CONTEXT_EXTENDED(context);
    if (!ext_context)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to get extended context for SEARCH algorithm");
        return TECC_ALGO_INVALID_PARAM;
    }

    /* Check if DEK key is available */
    if (!ext_context->dek_key || ext_context->dek_key_len == 0)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "DEK key not available for SEARCH algorithm");
        return TECC_ALGO_INVALID_KEY_LENGTH;
    }

    /* Calculate required buffer size using algorithm API */
    required_size = tecc_algo_calc_encrypt_buffer_size(TECC_ALGO_SEARCH,
                                                             context->plaintext_len);
    if (required_size == 0)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to calculate SEARCH algorithm buffer size");
        return TECC_ALGO_INVALID_PARAM;
    }

    /* Allocate output buffer */
    output_buffer = pg_malloc(required_size);
    if (!output_buffer)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to allocate SEARCH algorithm output buffer");
        return TECC_ALGO_MEMORY_ERROR;
    }

    /* Call zero-copy algorithm API with real DEK key.
     * ReqID:TECC4.0 - Use rtrimmed plaintext length for SEARCH to avoid
     * encrypting bpchar padding spaces. BPCHAR pads to declared length before
     * encryption, but SEARCH should only encrypt meaningful content. The >=
     * guard prevents size_t underflow when WHERE path already rtrimmed
     * plaintext_len (text_space_count > plaintext_len after rtrim). */
    output_len = 0;
    {
        size_t effective_len = context->plaintext_len;
        if (context->plaintext_len >= context->text_space_count)
            effective_len = context->plaintext_len - context->text_space_count;
        ret = tecc_algo_encrypt_zero_copy(TECC_ALGO_SEARCH,
                                         ext_context->dek_key, ext_context->dek_key_len,
                                         context->plaintext, effective_len,
                                         output_buffer, required_size, &output_len,
                                         false, 0);
    }

    if (ret != TECC_ALGO_SUCCESS)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "SEARCH algorithm encryption failed: %d", ret);
        pg_free(output_buffer);
        return ret;
    }

    /* Free any previously allocated buffer */
    if (context->search_ciphertext)
    {
        pg_free(context->search_ciphertext);
        context->search_ciphertext = NULL;
    }

    /* Set ciphertext in context */
    context->search_cipher_len = output_len;
    context->search_ciphertext = output_buffer;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "SEARCH algorithm execution completed, length: %zu",
                        output_len);

    return TECC_ALGO_SUCCESS;
}

/*********************************************************************
 * HE algorithm execution functions (EC-ElGamal / finite field ElGamal)
 *********************************************************************/

/* ReqID:TECC4.0 */
#include "../../../src/include/common/tecc_algorithm/tecc_ec_elgamal.h"
#include "../../../src/include/common/tecc_key/tecc_sm2.h"
#include "../../../src/include/common/tecc_algorithm/tecc_elgamal.h"

/* ReqID:TECC4.0 - Cached HE crypto contexts (process-local, single-user mode) */
static EC_GROUP *he_ec_group = NULL;
static EC_POINT *he_ec_pubkey = NULL;
static BIGNUM *he_ec_privkey = NULL;
static TeccECCRTParams he_crt_params;
static TeccECBSGSTable *he_bsgs_tables[TECC_EC_CRT_NUM_PRIMES];
static bool he_crypto_initialized = false;

/*
 * ReqID:TECC4.0 - 不再使用硬编码 EC-ElGamal 测试私钥。
 *
 * EC-ElGamal 私钥/公钥同样来自每个 DEK 自己的密钥对（CREATE DEK
 * KEY_ALGO='he-add' 时由 tecc_he_gen_ec_keypair() 生成，私钥经 KEK 的 SM2
 * 信封加密后入库），解密时由 tecc_he_load_dek_privkey() 解开。he_ec_pubkey /
 * he_ec_privkey 保持 NULL：若某 DEK 无法加载到私钥则解密显式失败，绝不自动
 * 回退到公开已知的硬编码密钥。
 */

/* Forward declaration for atexit cleanup */
static void he_global_cleanup(void);

/* ReqID:TECC4.0 - pthread_once 保证惰性初始化只执行一次（并发安全） */
static pthread_once_t he_crypto_once = PTHREAD_ONCE_INIT;

/* ReqID:TECC4.0 - HE crypto 初始化主体（由 pthread_once 调用，失败仅保持标志 false） */
static void he_crypto_init_once(void)
{
    int ret;
    int i;

    /* Create P-256 EC group */
    he_ec_group = tecc_ec_elgamal_create_group();
    if (!he_ec_group) return;

    /* Private/public keys are per-DEK (loaded via key-management interfaces),
     * never cached here.  he_ec_pubkey/he_ec_privkey stay NULL. */

    /* Initialize CRT params (8×12-bit partitions for int2/4/8 coverage) */
    ret = tecc_ec_crt_params_init_ex(&he_crt_params, TECC_EC_CRT_NUM_PRIMES);
    if (ret != 0) return;

    /* Initialize BSGS tables for each partition（并行建表，缩短冷启动） */
    memset(he_bsgs_tables, 0, sizeof(he_bsgs_tables));
    for (i = 0; i < TECC_EC_CRT_NUM_PRIMES; i++)
    {
        he_bsgs_tables[i] = (TeccECBSGSTable *)pg_malloc(sizeof(TeccECBSGSTable));
        if (!he_bsgs_tables[i]) return;
    }
    /* ReqID:TECC4.0 - 8 表并行初始化（各表独立，多线程加速约 8 倍），
     * 控制 HE 客户端一次性冷启动开销。 */
    ret = tecc_ec_bsgs_tables_init_parallel(he_bsgs_tables, he_ec_group,
                                            TECC_EC_BSGS_TABLE_SIZE,
                                            TECC_EC_CRT_NUM_PRIMES);
    if (ret != 0) return;

    he_crypto_initialized = true;
    atexit(he_global_cleanup);
}

/* ReqID:TECC4.0 - Initialize HE crypto context（pthread_once 包装，并发安全） */
static int he_crypto_init(void)
{
    pthread_once(&he_crypto_once, he_crypto_init_once);
    return he_crypto_initialized ? TECC_ALGO_SUCCESS : TECC_ALGO_ENCRYPTION_FAILED;
}

/* ReqID:TECC4.0 - EC-ElGamal encryption */
int
tecc_execute_algo_he_add(TeccCryptoContext *context)
{
    TeccCryptoContextExtended *ext_ctx;
    int ret;
    int64 int_value;
    BIGNUM *bn_value = NULL;
    TeccECCRTCiphertext ct;
    uint8_t serialized[TECC_EC_CRT_MAX_CIPHERTEXT_SIZE];
    int serialized_len;

    if (context == NULL) return TECC_ALGO_INVALID_PARAM;
    ext_ctx = TECC_CONTEXT_EXTENDED(context);
    if (ext_ctx == NULL) return TECC_ALGO_INVALID_PARAM;

    /* Ensure HE crypto is initialized */
    ret = he_crypto_init();
    if (ret != TECC_ALGO_SUCCESS)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
            "HE_ADD: crypto init failed: %d", ret);
        return ret;
    }

    /* Parse plaintext as int64 */
    if (context->plaintext == NULL || context->plaintext_len == 0)
        return TECC_ALGO_INVALID_PARAM;

    tecc_processor_debug(TECC_LOG_DEBUG1,
        "HE_ADD: plaintext='%.*s' len=%zu",
        (int)context->plaintext_len, (const char *)context->plaintext,
        context->plaintext_len);

    int_value = strtoll((const char *)context->plaintext, NULL, 10);

    /* Convert to BIGNUM */
    bn_value = BN_new();
    if (!bn_value) return TECC_ALGO_MEMORY_ERROR;
    BN_set_word(bn_value, (BN_ULONG)(int_value >= 0 ? int_value : -int_value));
    if (int_value < 0)
    {
        BN_set_negative(bn_value, 1);
    }

	/* ReqID:TECC4.0 - CRT 加密必须使用 per-DEK 公钥；缺失或加密失败时报错，
	 * 不再回退到硬编码测试密钥。 */
	memset(&ct, 0, sizeof(ct));
	ret = tecc_ec_crt_ciphertext_init(&ct, he_ec_group, he_crt_params.num_primes);
	if (ret != 0)
	{
		tecc_processor_debug(TECC_LOG_ERROR,
			"HE_ADD: ciphertext init failed: %d", ret);
		BN_free(bn_value);
		return TECC_ALGO_ENCRYPTION_FAILED;
	}
	if (ext_ctx->he_ec_pubkey_hex == NULL)
	{
		tecc_ec_crt_ciphertext_free(&ct, he_ec_group);
		BN_free(bn_value);
		tecc_processor_debug(TECC_LOG_ERROR,
			"HE_ADD: per-DEK EC public key not loaded, cannot encrypt");
		return TECC_ALGO_ENCRYPTION_FAILED;
	}
	{
		EC_POINT *per_dek_pub = EC_POINT_new(he_ec_group);
		unsigned char *pub_bin = NULL;
		size_t pub_bin_len = strlen(ext_ctx->he_ec_pubkey_hex) / 2;
		int j;

		if (per_dek_pub != NULL && pub_bin_len > 0)
		{
			pub_bin = (unsigned char *)pg_malloc(pub_bin_len);
			for (j = 0; j < (int)pub_bin_len; j++)
			{
				char c1 = ext_ctx->he_ec_pubkey_hex[j * 2];
				char c2 = ext_ctx->he_ec_pubkey_hex[j * 2 + 1];
				unsigned char n1, n2;
				n1 = (c1 >= '0' && c1 <= '9') ? (unsigned char)(c1 - '0') :
					(c1 >= 'a' && c1 <= 'f') ? (unsigned char)(c1 - 'a' + 10) :
					(c1 >= 'A' && c1 <= 'F') ? (unsigned char)(c1 - 'A' + 10) : 0;
				n2 = (c2 >= '0' && c2 <= '9') ? (unsigned char)(c2 - '0') :
					(c2 >= 'a' && c2 <= 'f') ? (unsigned char)(c2 - 'a' + 10) :
					(c2 >= 'A' && c2 <= 'F') ? (unsigned char)(c2 - 'A' + 10) : 0;
				pub_bin[j] = (n1 << 4) | n2;
			}
			if (EC_POINT_oct2point(he_ec_group, per_dek_pub, pub_bin,
								   pub_bin_len, NULL) == 1)
			{
				ret = tecc_ec_crt_encrypt(&ct, he_ec_group, per_dek_pub,
										  bn_value, &he_crt_params, NULL);
			}
			else
				ret = -1;
			pg_free(pub_bin);
		}
		else
			ret = -1;
		EC_POINT_free(per_dek_pub);

		if (ret != 0)
		{
			tecc_ec_crt_ciphertext_free(&ct, he_ec_group);
			BN_free(bn_value);
			tecc_processor_debug(TECC_LOG_ERROR,
				"HE_ADD: per-DEK pubkey encrypt failed: %d", ret);
			return TECC_ALGO_ENCRYPTION_FAILED;
		}
	}
    BN_free(bn_value);
    if (ret != 0)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
            "HE_ADD: CRT encrypt failed: %d", ret);
        tecc_ec_crt_ciphertext_free(&ct, he_ec_group);
        return TECC_ALGO_ENCRYPTION_FAILED;
    }

    /* Serialize */
    serialized_len = tecc_ec_crt_serialize(serialized, sizeof(serialized),
                                            &ct, he_ec_group);
    tecc_ec_crt_ciphertext_free(&ct, he_ec_group);
    if (serialized_len <= 0) return TECC_ALGO_ENCRYPTION_FAILED;

    /* Store in context */
    ext_ctx->he_add_ciphertext = (unsigned char *)pg_malloc(serialized_len);
    memcpy(ext_ctx->he_add_ciphertext, serialized, serialized_len);
    ext_ctx->he_add_cipher_len = serialized_len;

    return TECC_ALGO_SUCCESS;
}

/* ReqID:TECC4.0 - Cached ElGamal contexts (process-local) */
static TeccElGamalCRTParams he_eg_crt_params;
static BIGNUM *he_eg_privkey = NULL;
static BIGNUM *he_eg_pubkey1 = NULL;
static BIGNUM *he_eg_pubkey2 = NULL;
static BIGNUM *he_eg_pubkey = NULL;	/* non-CRT pubkey = g^x mod p */
static bool he_eg_initialized = false;
static BIGNUM *he_eg_p = NULL;
static BIGNUM *he_eg_q = NULL;
static BIGNUM *he_eg_g = NULL;

/*
 * ReqID:TECC4.0 - 不再使用硬编码测试私钥。
 *
 * FF ElGamal 私钥必须来自每个 DEK 自己生成的密钥对（CREATE DEK KEY_ALGO='he-mul'
 * 时由 tecc_he_gen_ff_keypair() 生成，私钥经 KEK 的 SM2 信封加密后入库），解密时由
 * tecc_he_load_dek_privkey() 解开。he_eg_privkey / he_eg_pubkey1 / he_eg_pubkey2 /
 * he_eg_pubkey 保持 NULL，若某 DEK 无法加载到私钥则解密显式失败，绝不自动回退到
 * 公开已知的硬编码密钥（否则任何受害者私钥都被公开，he-mul 无语义安全）。
 */

/* ReqID:TECC4.0 - pthread_once 保证惰性初始化只执行一次（并发安全） */
static pthread_once_t he_eg_once = PTHREAD_ONCE_INIT;

/* ReqID:TECC4.0 - ElGamal crypto 初始化主体（由 pthread_once 调用，失败仅保持标志 false） */
static void he_eg_init_once(void)
{
    int ret;

    /* Get default 2048-bit parameters (RFC 5114 §2.3, validated at load) */
    ret = tecc_elgamal_get_default_params(&he_eg_p, &he_eg_q, &he_eg_g);
    if (ret != TECC_ELGAMAL_SUCCESS)
    {
        TECC_LOG(TECC_LOG_INFO, "HE: failed to load ElGamal default params (%d), "
                 "homomorphic operations will be unavailable", ret);
        return;
    }

    /* Initialize CRT params (legacy parallel-group scaffolding, not on the
     * live he-mul path; kept for tecc_elgamal_crt_params_free symmetry). */
    ret = tecc_elgamal_crt_params_init(&he_eg_crt_params);
    if (ret != TECC_ELGAMAL_SUCCESS)
    {
        TECC_LOG(TECC_LOG_INFO, "HE: failed to init ElGamal CRT params (%d), "
                 "homomorphic operations will be unavailable", ret);
        return;
    }

    /* Private/public keys are per-DEK (loaded via the key-management
     * interfaces), never cached here.  he_eg_privkey/pubkey* stay NULL. */
    he_eg_initialized = true;
    atexit(he_global_cleanup);
}

/* ReqID:TECC4.0 - Initialize ElGamal crypto context（pthread_once 包装，并发安全） */
static int he_eg_init(void)
{
    pthread_once(&he_eg_once, he_eg_init_once);
    return he_eg_initialized ? TECC_ALGO_SUCCESS : TECC_ALGO_ENCRYPTION_FAILED;
}

/* ReqID:TECC4.0 - Finite field ElGamal encryption */
int
tecc_execute_algo_he_mul(TeccCryptoContext *context)
{
    TeccCryptoContextExtended *ext_ctx;
    int ret;
    int64 int_value;
    BIGNUM *bn_value = NULL;
    BIGNUM *c1, *c2;
    uint8_t serialized[TECC_ELGAMAL_CIPHERTEXT_SIZE];

    if (context == NULL) return TECC_ALGO_INVALID_PARAM;
    ext_ctx = TECC_CONTEXT_EXTENDED(context);
    if (ext_ctx == NULL) return TECC_ALGO_INVALID_PARAM;

    /* Ensure ElGamal crypto is initialized */
    ret = he_eg_init();
    if (ret != TECC_ALGO_SUCCESS) return ret;

    /* Parse plaintext as int64 */
    if (context->plaintext == NULL || context->plaintext_len == 0)
        return TECC_ALGO_INVALID_PARAM;

    int_value = strtoll((const char *)context->plaintext, NULL, 10);

    /* Convert to BIGNUM */
    bn_value = BN_new();
    if (!bn_value) return TECC_ALGO_MEMORY_ERROR;
    if (int_value >= 0)
        BN_set_word(bn_value, (BN_ULONG)int_value);
    else
    {
        BN_set_word(bn_value, (BN_ULONG)(-int_value));
        BN_set_negative(bn_value, 1);
    }

    /* Non-CRT ElGamal encrypt (matching server-side he_mul_generic deserialize).
     * Use per-DEK public key if loaded, else hardcoded test key. */
    c1 = BN_new(); c2 = BN_new();
    if (!c1 || !c2)
    {
        BN_free(c1); BN_free(c2); BN_free(bn_value);
        return TECC_ALGO_MEMORY_ERROR;
    }
    if (ext_ctx->he_eg_pubkey_hex != NULL)
    {
        BIGNUM *per_dek_pub = BN_bin2bn((const unsigned char *)ext_ctx->he_eg_pubkey_hex,
                                        (int)strlen(ext_ctx->he_eg_pubkey_hex), NULL);
        if (per_dek_pub != NULL)
        {
            /* he_pubkey stored as ASCII hex of 256-byte h — decode properly */
            unsigned char pub_bin[256];
            size_t pub_bin_len = strlen(ext_ctx->he_eg_pubkey_hex) / 2;
            int j;

            if (pub_bin_len > 0 && pub_bin_len <= sizeof(pub_bin))
            {
                for (j = 0; j < (int)pub_bin_len; j++)
                {
                    char ch1 = ext_ctx->he_eg_pubkey_hex[j * 2];
                    char ch2 = ext_ctx->he_eg_pubkey_hex[j * 2 + 1];
                    unsigned char h1, h2;
                    h1 = (ch1 >= '0' && ch1 <= '9') ? (unsigned char)(ch1 - '0') :
                         (ch1 >= 'a' && ch1 <= 'f') ? (unsigned char)(ch1 - 'a' + 10) :
                         (ch1 >= 'A' && ch1 <= 'F') ? (unsigned char)(ch1 - 'A' + 10) : 0;
                    h2 = (ch2 >= '0' && ch2 <= '9') ? (unsigned char)(ch2 - '0') :
                         (ch2 >= 'a' && ch2 <= 'f') ? (unsigned char)(ch2 - 'a' + 10) :
                         (ch2 >= 'A' && ch2 <= 'F') ? (unsigned char)(ch2 - 'A' + 10) : 0;
                    pub_bin[j] = (h1 << 4) | h2;
                }
                BN_free(per_dek_pub);
                per_dek_pub = BN_bin2bn(pub_bin, (int)pub_bin_len, NULL);
            }

            if (per_dek_pub != NULL)
            {
                ret = tecc_elgamal_encrypt(he_eg_p, he_eg_q, he_eg_g, per_dek_pub,
                                           bn_value, c1, c2, NULL);
                BN_free(per_dek_pub);
            }
            else
                ret = -1;

            if (ret != TECC_ELGAMAL_SUCCESS)
            {
                BN_free(c1); BN_free(c2); BN_free(bn_value);
                tecc_processor_debug(TECC_LOG_ERROR,
                    "HE_MUL: per-DEK pubkey encrypt failed: %d", ret);
                return TECC_ALGO_ENCRYPTION_FAILED;
            }
        }
        else
        {
            BN_free(c1); BN_free(c2); BN_free(bn_value);
            tecc_processor_debug(TECC_LOG_ERROR,
                "HE_MUL: per-DEK ElGamal public key decode failed, cannot encrypt");
            return TECC_ALGO_ENCRYPTION_FAILED;
        }
    }
    else
    {
        BN_free(c1); BN_free(c2); BN_free(bn_value);
        tecc_processor_debug(TECC_LOG_ERROR,
            "HE_MUL: per-DEK ElGamal public key not loaded, cannot encrypt");
        return TECC_ALGO_ENCRYPTION_FAILED;
    }
    BN_free(bn_value);
    if (ret != TECC_ELGAMAL_SUCCESS)
    {
        BN_free(c1); BN_free(c2);
        tecc_processor_debug(TECC_LOG_ERROR, "HE_MUL: encrypt failed: %d", ret);
        return TECC_ALGO_ENCRYPTION_FAILED;
    }

    /* Serialize */
    memset(serialized, 0, sizeof(serialized));
    ret = tecc_elgamal_serialize(serialized, sizeof(serialized), c1, c2);
    BN_free(c1); BN_free(c2);
    if (ret <= 0) return TECC_ALGO_ENCRYPTION_FAILED;

    /* Store in context */
    ext_ctx->he_mul_ciphertext = (unsigned char *)pg_malloc(TECC_ELGAMAL_CIPHERTEXT_SIZE);
    memcpy(ext_ctx->he_mul_ciphertext, serialized, TECC_ELGAMAL_CIPHERTEXT_SIZE);
    ext_ctx->he_mul_cipher_len = TECC_ELGAMAL_CIPHERTEXT_SIZE;

    return TECC_ALGO_SUCCESS;
}

/*
 * Get algorithm name
 */
const char *
tecc_get_algorithm_name(TeccAlgorithmType algo_type)
{
    if (algo_type <= 0 || algo_type >= TECC_ALGO_MAX)
        return "UNKNOWN";

    return algorithm_names[algo_type];
}

/*
 * Get algorithm capabilities
 */
TeccAlgorithmCaps
tecc_get_algorithm_capabilities(TeccAlgorithmType algo_type)
{
    TeccAlgorithmCaps empty_caps = {0};

    if (algo_type <= 0 || algo_type >= TECC_ALGO_MAX)
        return empty_caps;

    return algorithm_caps[algo_type];
}

/*
 * Check if algorithm requires collation
 */
bool
tecc_algorithm_requires_collation(TeccAlgorithmType algo_type)
{
    return (algo_type == TECC_ALGO_OPE);
}

/*
 * Validate algorithm selection
 */
bool
tecc_validate_algorithm_selection(const TeccCryptoContext *context)
{
    if (!context)
        return false;

    /* Must have RND algorithm */
    if (!(context->core.selected_algorithms & TECC_ALGO_RND))
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Algorithm selection invalid: RND algorithm not selected");
        return false;
    }

    /* Check that selected algorithms are supported */
    if ((context->core.selected_algorithms & TECC_ALGO_DET) &&
        !context->core.algo_support.supports_det)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Algorithm selection invalid: DET not supported");
        return false;
    }

    if ((context->core.selected_algorithms & TECC_ALGO_OPE) &&
        !context->core.algo_support.supports_ope)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Algorithm selection invalid: OPE not supported");
        return false;
    }

    if ((context->core.selected_algorithms & TECC_ALGO_SEARCH) &&
        !context->core.algo_support.supports_search)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Algorithm selection invalid: SEARCH not supported");
        return false;
    }

    return true;
}

/*
 * Check algorithm compatibility
 */
int
tecc_check_algorithm_compatibility(const TeccCryptoContext *context,
                                   TeccAlgorithmType algo_type)
{
    if (!context)
        return TECC_CRYPTO_INVALID_PARAM;

    if (!tecc_is_algorithm_supported(context, algo_type))
        return TECC_CRYPTO_ALGO_ERROR;

    /* Check collation requirements */
    if (tecc_algorithm_requires_collation(algo_type) &&
        context->core.collation_oid == 0)
    {
        tecc_processor_debug(TECC_LOG_WARNING,
                           "Algorithm %s requires collation but none specified",
                           tecc_get_algorithm_name(algo_type));
        return TECC_CRYPTO_COLLATION_ERROR;
    }

    return TECC_CRYPTO_SUCCESS;
}

/*
 * Get algorithm error message
 */
const char *
tecc_get_algorithm_error_message(int error_code)
{
    switch (error_code)
    {
        case TECC_ALGO_SUCCESS:
            return "Success";
        case TECC_ALGO_OPERATION_NOT_SUPPORTED:
            return "Algorithm not supported";
        case TECC_ALGO_INVALID_PARAM:
            return "Invalid algorithm parameters";
        case TECC_ALGO_INVALID_KEY_LENGTH:
            return "Algorithm key error";
        case TECC_ALGO_ENCRYPTION_FAILED:
            return "Encryption failed";
        case TECC_ALGO_DECRYPTION_FAILED:
            return "Decryption failed";
        case TECC_ALGO_MEMORY_ERROR:
            return "Memory allocation failed";
        case TECC_ALGO_SYSTEM_ERROR:
            return "Internal algorithm error";
        default:
            return "Unknown algorithm error";
    }
}

/*
 * Handle algorithm failure
 */
void
tecc_handle_algorithm_failure(TeccCryptoContext *context,
                             TeccAlgorithmType algo_type,
                             int error_code)
{
    if (!context)
        return;

    tecc_processor_debug(TECC_LOG_WARNING,
                        "Algorithm %s failed with error %d: %s",
                        tecc_get_algorithm_name(algo_type),
                        error_code,
                        tecc_get_algorithm_error_message(error_code));

    /* Remove failed algorithm from selection */
    context->core.selected_algorithms &= ~(1 << algo_type);

    /* Set error information if not already set */
    if (context->error_code == TECC_CRYPTO_SUCCESS)
    {
        context->error_code = TECC_CRYPTO_SCHEDULE_ERROR;
        snprintf(context->error_msg, sizeof(context->error_msg),
                "Algorithm %s failed: %s",
                tecc_get_algorithm_name(algo_type),
                tecc_get_algorithm_error_message(error_code));
    }
}

/*
 * Count selected algorithms
 */
int
tecc_count_selected_algorithms(const TeccCryptoContext *context)
{
    int count = 0;
    int i;

    if (!context)
        return 0;

    for (i = 0; i < TECC_ALGO_MAX; i++)
    {
        if (context->core.selected_algorithms & (1 << i))
            count++;
    }

    return count;
}

/*
 * Get primary algorithm
 */
TeccAlgorithmType
tecc_get_primary_algorithm(const TeccCryptoContext *context)
{
    if (!context)
        return 0;

    /* RND is always primary */
    if (context->core.selected_algorithms & TECC_ALGO_RND)
        return TECC_ALGO_RND;

    return 0;  /* No algorithms selected */
}

/*
 * Check if has mandatory algorithms
 */
bool
tecc_has_mandatory_algorithms(const TeccCryptoContext *context)
{
    if (!context)
        return false;

    /* RND is mandatory */
    return (context->core.selected_algorithms & TECC_ALGO_RND) != 0;
}

/*
 * SEARCH algorithm encryption with operator type support
 */
int
tecc_algorithm_scheduler_encrypt_search_with_operator(
    TeccCryptoContext *context,
    TeccOperatorType operator_type,
    unsigned char **search_ciphertext,
    size_t *search_cipher_len)
{
    int ret;
    size_t required_size;
    unsigned char *output_buffer;
    size_t output_len;
    TeccCryptoContextExtended *ext_context = NULL;

    /* Validate parameters */
    if (!context || !search_ciphertext || !search_cipher_len)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid parameters to tecc_algorithm_scheduler_encrypt_search_with_operator");
        return TECC_ALGO_INVALID_PARAM;
    }

    /* Check if context is extended type */
    if (TECC_IS_EXTENDED_CONTEXT(context))
    {
        ext_context = TECC_CONTEXT_EXTENDED(context);

        /* Set operator type in context */
        ext_context->operator_type = operator_type;

        /* Check if this is a LIKE pattern */
        if (operator_type == TECC_OP_LIKE || operator_type == TECC_OP_NOT_LIKE ||
            operator_type == TECC_OP_ILIKE || operator_type == TECC_OP_NOT_ILIKE)
        {
            ext_context->is_like_pattern = true;
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Processing LIKE pattern with operator type %d", operator_type);
        }
        else
        {
            ext_context->is_like_pattern = false;
        }
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Executing SEARCH algorithm with operator type %d using extended zero-copy interface",
                        operator_type);

    /* Check if SEARCH algorithm is selected */
    if (!(context->core.selected_algorithms & (1 << TECC_ALGO_SEARCH)))
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "SEARCH algorithm not selected in context");
        return TECC_ALGO_OPERATION_NOT_SUPPORTED;
    }

    /* Validate context */
    if (!context->plaintext)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid context for SEARCH algorithm");
        return TECC_ALGO_INVALID_PARAM;
    }

    /* Calculate required buffer size using algorithm API */
    required_size = tecc_algo_calc_encrypt_buffer_size(TECC_ALGO_SEARCH,
                                                       context->plaintext_len);
    if (required_size == 0)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to calculate SEARCH algorithm buffer size");
        return TECC_ALGO_INVALID_PARAM;
    }

    /* Allocate output buffer */
    output_buffer = pg_malloc(required_size);
    if (!output_buffer)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to allocate SEARCH algorithm output buffer");
        return TECC_ALGO_MEMORY_ERROR;
    }

    /* Use extended zero-copy interface if operator type is specified */
    if (ext_context != NULL && ext_context->operator_type != TECC_OP_UNKNOWN)
    {
        /* Dispatch based on operator type */
        switch (ext_context->operator_type)
        {
            case TECC_OP_LIKE:
            case TECC_OP_NOT_LIKE:
            case TECC_OP_ILIKE:
            case TECC_OP_NOT_ILIKE:
                /* LIKE pattern processing - use unified parser */
                ret = tecc_execute_algo_search_like_unified(ext_context,
                                                           ext_context->operator_type,
                                                           &output_buffer,
                                                           &output_len);
                break;

            case TECC_OP_EQUAL:
            case TECC_OP_NOT_EQUAL:
                /* Equality comparison processing */
                ret = tecc_execute_algo_search_equal(ext_context,
                                                    ext_context->operator_type,
                                                    &output_buffer,
                                                    &output_len);
                break;

            default:
                /* Use extended zero-copy interface with operator type */
                ret = tecc_algo_encrypt_zero_copy_ex(TECC_ALGO_SEARCH,
                                                    ext_context->operator_type,
                                                    context->dek_key,
                                                    context->dek_key_len,
                                                    context->plaintext,
                                                    context->plaintext_len,
                                                    output_buffer,
                                                    required_size,
                                                    &output_len);
                break;
        }
    }
    else
    {
        /* Fall back to basic zero-copy interface */
        ret = tecc_algo_encrypt_zero_copy(TECC_ALGO_SEARCH,
                                         context->dek_key,
                                         context->dek_key_len,
                                         context->plaintext,
                                         context->plaintext_len,
                                         output_buffer,
                                         required_size,
                                         &output_len,
                                         false, 0);
    }

    if (ret != TECC_ALGO_SUCCESS)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "SEARCH algorithm encryption failed with error %d", ret);
        pg_free(output_buffer);
        return ret;
    }

    /* Return ciphertext */
    *search_ciphertext = output_buffer;
    *search_cipher_len = output_len;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "SEARCH algorithm encryption with operator type %d completed successfully, output size: %zu",
                        operator_type, output_len);

    return TECC_ALGO_SUCCESS;
}

/*
 * RND algorithm encryption
 */
int
tecc_algorithm_scheduler_encrypt_rnd(
    TeccCryptoContext *context,
    unsigned char **rnd_ciphertext,
    size_t *rnd_cipher_len,
    const unsigned char *derived_enc_key,
    const unsigned char *derived_auth_key)
{
    int ret;
    size_t required_size;
    unsigned char *output_buffer;
    size_t output_len;

    /* Validate parameters */
    if (!context || !rnd_ciphertext || !rnd_cipher_len)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid parameters to tecc_algorithm_scheduler_encrypt_rnd");
        return TECC_ALGO_INVALID_PARAM;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Executing RND algorithm encryption");

    /* Validate context */
    if (!context->plaintext || context->plaintext_len == 0)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid context for RND algorithm");
        return TECC_ALGO_INVALID_PARAM;
    }

    /* Calculate required buffer size using algorithm API */
    required_size = tecc_algo_calc_encrypt_buffer_size(TECC_ALGO_RND,
                                                       context->plaintext_len);
    if (required_size == 0)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to calculate RND algorithm buffer size");
        return TECC_ALGO_INVALID_PARAM;
    }

    /* Allocate output buffer */
    output_buffer = pg_malloc(required_size);
    if (!output_buffer)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to allocate RND algorithm output buffer");
        return TECC_ALGO_MEMORY_ERROR;
    }

    /* ReqID:TECC4.0 - Use pre-derived keys to skip per-row HKDF when available */
    if (derived_enc_key != NULL && derived_auth_key != NULL)
    {
        ret = tecc_sm4_rnd_encrypt_with_keys(context->dek_key, context->dek_key_len,
                                             context->plaintext, context->plaintext_len,
                                             output_buffer, required_size, &output_len,
                                             derived_enc_key, derived_auth_key);
    }
    else
    {
        ret = tecc_algo_encrypt_zero_copy(TECC_ALGO_RND,
                                         context->dek_key,
                                         context->dek_key_len,
                                         context->plaintext,
                                         context->plaintext_len,
                                         output_buffer,
                                         required_size,
                                         &output_len, false, 0);
    }

    if (ret != TECC_ALGO_SUCCESS)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "RND algorithm encryption failed with error %d", ret);
        pg_free(output_buffer);
        return ret;
    }

    /* Return ciphertext */
    *rnd_ciphertext = output_buffer;
    *rnd_cipher_len = output_len;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "RND algorithm encryption completed successfully, output size: %zu",
                        output_len);

    return TECC_ALGO_SUCCESS;
}

/* Begin - ReqID:TECC4.0 - SEARCH算法LIKE模式处理函数 */

/*
 * tecc_execute_algo_search_equal - Execute SEARCH algorithm for equality comparisons
 */
static int
tecc_execute_algo_search_equal(TeccCryptoContextExtended *context,
                              TeccOperatorType operator_type,
                              unsigned char **search_ciphertext,
                              size_t *search_cipher_len)
{
    if (!context || !search_ciphertext || !search_cipher_len)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid parameters to tecc_execute_algo_search_equal");
        return TECC_ALGO_INVALID_PARAM;
    }

    /* Validate context fields */
    if (!context->plaintext)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid plaintext in context for equality comparison");
        return TECC_ALGO_INVALID_PARAM;
    }

    if (!context->dek_key || context->dek_key_len == 0)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid DEK key in context for equality comparison");
        return TECC_ALGO_INVALID_PARAM;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing equality comparison with operator type %d (plain_len=%zu)",
                        operator_type, context->plaintext_len);

    /* For equality comparisons, use exact match encryption */
    return tecc_algo_search_encrypt_exact(context->dek_key,
                                         context->dek_key_len,
                                         (const char *)context->plaintext,
                                         context->plaintext_len,
                                         search_ciphertext,
                                         search_cipher_len,
                                         0);
}

/*
 * tecc_algo_search_encrypt_prefix - Encrypt prefix for SEARCH algorithm
 */
static int
tecc_algo_search_encrypt_prefix(const unsigned char *key,
                               size_t key_len,
                               const char *prefix,
                               size_t prefix_len,
                               unsigned char **ciphertext,
                               size_t *cipher_len,
                               char escape_char)
{
    /* For now, use the standard zero-copy interface */
    /* In a real implementation, this would use special prefix encryption */
    size_t required_size;
    unsigned char *output_buffer;
    size_t output_len;
    int ret;

    /* Validate parameters */
    if (!key || key_len == 0 || !prefix || !ciphertext || !cipher_len)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid parameters to tecc_algo_search_encrypt_prefix");
        return TECC_ALGO_INVALID_PARAM;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Encrypting prefix: %.*s (len=%zu)",
                        (int)prefix_len, prefix, prefix_len);

    required_size = tecc_algo_calc_encrypt_buffer_size(TECC_ALGO_SEARCH, prefix_len);
    if (required_size == 0)
    {
        return TECC_ALGO_INVALID_PARAM;
    }

    output_buffer = pg_malloc(required_size);
    if (!output_buffer)
    {
        return TECC_ALGO_MEMORY_ERROR;
    }

    /* Use basic zero-copy interface for now.
     * ReqID:TECC4.0 - LIKE patterns with escape_char need it passed through
     * to tecc_search_sm4_encrypt so escaped wildcards are hashed, not treated
     * as wildcards.  Get escape_char from the extended context if available. */
    {
        ret = tecc_algo_encrypt_zero_copy(TECC_ALGO_SEARCH,
                                         key,
                                         key_len,
                                         (const unsigned char *)prefix,
                                         prefix_len,
                                         output_buffer,
                                         required_size,
                                         &output_len,
                                         true,
                                         escape_char);
    }

    if (ret != TECC_ALGO_SUCCESS)
    {
        pg_free(output_buffer);
        return ret;
    }

    *ciphertext = output_buffer;
    *cipher_len = output_len;

    return TECC_ALGO_SUCCESS;
}

/*
 * tecc_algo_search_encrypt_exact - Encrypt exact pattern for SEARCH algorithm
 */
static int
tecc_algo_search_encrypt_exact(const unsigned char *key,
                              size_t key_len,
                              const char *pattern,
                              size_t pattern_len,
                              unsigned char **ciphertext,
                              size_t *cipher_len,
                              char escape_char)
{
    /* Similar to prefix encryption */
    return tecc_algo_search_encrypt_prefix(key, key_len, pattern, pattern_len,
                                          ciphertext, cipher_len, escape_char);
}

/* End - ReqID:TECC4.0 - SEARCH算法LIKE模式处理函数 */

/*
 * tecc_parse_pattern_unified - Unified pattern parsing function
 *
 * This function replaces the simplified tecc_parse_like_pattern with
 * the full-featured pattern parsing implementation.
 */
static TeccPatternErrorCode
tecc_parse_pattern_unified(
    const char *pattern,
    size_t pattern_len,
    char escape_char,
    TeccParsedPattern **parsed_pattern)
{
    TeccPatternErrorCode ret;

    /* Call the unified pattern parsing function */
    ret = tecc_parse_pattern_full(pattern, pattern_len, escape_char, parsed_pattern);
    if (ret != TECC_PATTERN_SUCCESS)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to parse pattern with unified parser: error=%d",
                           ret);
        return ret;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Successfully parsed pattern with unified parser: type=%d, segments=%zu",
                        (*parsed_pattern)->pattern_type,
                        (*parsed_pattern)->segment_count);

    return TECC_PATTERN_SUCCESS;
}

/*
 * tecc_convert_error_code - Convert TeccPatternErrorCode to TECC algorithm error code
 */
static int
tecc_convert_error_code(TeccPatternErrorCode pattern_error)
{
    switch (pattern_error)
    {
        case TECC_PATTERN_SUCCESS:
            return TECC_ALGO_SUCCESS;
        case TECC_PATTERN_ERROR_INVALID_PARAM:
            return TECC_ALGO_INVALID_PARAM;
        case TECC_PATTERN_ERROR_MEMORY_ALLOC:
            return TECC_ALGO_MEMORY_ERROR;
        case TECC_PATTERN_ERROR_INVALID_PATTERN:
        case TECC_PATTERN_ERROR_ESCAPE_INVALID:
            return TECC_ALGO_DATA_FORMAT_ERROR;
        case TECC_PATTERN_ERROR_ENCRYPTION_FAILED:
            return TECC_ALGO_SYSTEM_ERROR;
        case TECC_PATTERN_ERROR_TOO_MANY_SEGMENTS:
        case TECC_PATTERN_ERROR_PATTERN_TOO_COMPLEX:
            return TECC_ALGO_DATA_FORMAT_ERROR;
        default:
            return TECC_ALGO_SYSTEM_ERROR;
    }
}

/*
 * tecc_execute_algo_search_like_unified - Unified SEARCH algorithm for LIKE patterns
 *
 * This function replaces the simplified implementation with full-featured
 * pattern parsing and encryption.
 */
static int
tecc_execute_algo_search_like_unified(TeccCryptoContextExtended *context,
                                     TeccOperatorType operator_type,
                                     unsigned char **search_ciphertext,
                                     size_t *search_cipher_len)
{
    TeccPatternErrorCode pattern_ret;
    TeccParsedPattern *parsed_pattern = NULL;
    const char *pattern;
    size_t pattern_len;
    int ret = TECC_ALGO_SUCCESS;
    char escape_char;

    if (!context || !search_ciphertext || !search_cipher_len)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid parameters to tecc_execute_algo_search_like_unified");
        return TECC_ALGO_INVALID_PARAM;
    }

    /* Validate context fields */
    if (!context->plaintext)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid plaintext in context for LIKE pattern processing");
        return TECC_ALGO_INVALID_PARAM;
    }

    if (!context->dek_key || context->dek_key_len == 0)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid DEK key in context for LIKE pattern processing");
        return TECC_ALGO_INVALID_PARAM;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing LIKE pattern with unified parser, operator type %d",
                        operator_type);

    /* ReqID:TECC4.0 - Handle empty pattern (LIKE '') as exact match on empty string */
    if (context->plaintext_len == 0)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "Empty LIKE pattern treated as exact match on empty string");
        return tecc_algo_search_encrypt_exact(context->dek_key,
                                             context->dek_key_len,
                                             "", 0,
                                             search_ciphertext,
                                             search_cipher_len,
                                             0);
    }

    /* Parse LIKE pattern using unified parser */
    pattern = (const char *)context->plaintext;
    pattern_len = context->plaintext_len;

    /* Get escape character from context (set by WHERE clause processor) */
    /* ReqID:TECC4.0 */
    escape_char = context->like_escape_char;

    pattern_ret = tecc_parse_pattern_unified(pattern, pattern_len, escape_char, &parsed_pattern);
    if (pattern_ret != TECC_PATTERN_SUCCESS)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to parse LIKE pattern with unified parser: error=%d, pattern=%.*s",
                           pattern_ret, (int)pattern_len, pattern);
        return tecc_convert_error_code(pattern_ret);
    }

    /* Dispatch based on pattern type */
    switch (parsed_pattern->pattern_type)
    {
        case TECC_PATTERN_EXACT:
            /* Exact match - encrypt the entire pattern */
            if (parsed_pattern->segment_count == 1 &&
                parsed_pattern->segments[0].type == TECC_PATTERN_SEGMENT_FIXED_STRING)
            {
                TeccPatternSegment *segment = &parsed_pattern->segments[0];
                ret = tecc_algo_search_encrypt_exact(context->dek_key,
                                                    context->dek_key_len,
                                                    (const char *)segment->data.fixed_string,
                                                    segment->length,
                                                    search_ciphertext,
                                                    search_cipher_len, context->like_escape_char);
            }
            else
            {
                /* Should not happen for exact patterns */
                ret = TECC_ALGO_SYSTEM_ERROR;
            }
            break;

        case TECC_PATTERN_PREFIX:
            /* Prefix match 'abc%' - encrypt full pattern with wildcards.
             * ReqID:TECC4.0: SEARCH encryption preserves % wildcard. */
            if (parsed_pattern->is_simple_prefix)
            {
                ret = tecc_algo_search_encrypt_exact(context->dek_key,
                                                    context->dek_key_len,
                                                    pattern,
                                                    pattern_len,
                                                    search_ciphertext,
                                                    search_cipher_len, context->like_escape_char);
            }
            else
            {
                /* Complex prefix pattern - use exact encryption for now */
                tecc_processor_debug(TECC_LOG_WARNING,
                                   "Complex prefix pattern, using exact encryption");
                ret = tecc_algo_search_encrypt_exact(context->dek_key,
                                                    context->dek_key_len,
                                                    pattern,
                                                    pattern_len,
                                                    search_ciphertext,
                                                    search_cipher_len, context->like_escape_char);
            }
            break;

        case TECC_PATTERN_SUFFIX:
            /* Suffix match '%xyz' */
            if (parsed_pattern->is_simple_suffix)
            {
                ret = tecc_algo_search_encrypt_exact(context->dek_key,
                                                    context->dek_key_len,
                                                    pattern,
                                                    pattern_len,
                                                    search_ciphertext,
                                                    search_cipher_len, context->like_escape_char);
            }
            else
            {
                /* Complex suffix pattern - use exact encryption for now */
                tecc_processor_debug(TECC_LOG_WARNING,
                                   "Complex suffix pattern, using exact encryption");
                ret = tecc_algo_search_encrypt_exact(context->dek_key,
                                                    context->dek_key_len,
                                                    pattern,
                                                    pattern_len,
                                                    search_ciphertext,
                                                    search_cipher_len, context->like_escape_char);
            }
            break;

        case TECC_PATTERN_INFIX:
            /* Infix match '%middle%' - encrypt the full pattern including wildcards.
             * ReqID:TECC4.0: SEARCH encryption preserves % and _ as 1-byte wildcards,
             * so the full pattern produces proper LIKE-matching ciphertext. */
            if (parsed_pattern->is_simple_substring)
            {
                ret = tecc_algo_search_encrypt_exact(context->dek_key,
                                                    context->dek_key_len,
                                                    pattern,
                                                    pattern_len,
                                                    search_ciphertext,
                                                    search_cipher_len, context->like_escape_char);
            }
            else
            {
                /* Complex infix pattern - use exact encryption for now */
                tecc_processor_debug(TECC_LOG_WARNING,
                                   "Complex infix pattern, using exact encryption");
                ret = tecc_algo_search_encrypt_exact(context->dek_key,
                                                    context->dek_key_len,
                                                    pattern,
                                                    pattern_len,
                                                    search_ciphertext,
                                                    search_cipher_len, context->like_escape_char);
            }
            break;

        case TECC_PATTERN_WILDCARD:
        {
            /* Pattern contains '_' wildcard(s).
             * ReqID:TECC4.0 - Expand each _ to ___ (3 bytes) so that bytealike
             * matches exactly 3 bytes per encrypted character, preserving
             * alignment. _ in bytealike is single-byte but each encrypted
             * character is 3 bytes (CHAR_HASH_SIZE). */
            size_t i;
            size_t expanded_len = 0;
            size_t pos;
            char *expanded_pattern = NULL;

            /* Calculate expanded length from parsed segments */
            for (i = 0; i < parsed_pattern->segment_count; i++)
            {
                TeccPatternSegment *seg = &parsed_pattern->segments[i];
                if (seg->type == TECC_PATTERN_SEGMENT_WILDCARD_UNDERSCORE)
                    expanded_len += 3;  /* _ → ___ */
                else if (seg->type == TECC_PATTERN_SEGMENT_WILDCARD_PERCENT)
                    expanded_len += 1;  /* % stays 1 byte */
                else
                    expanded_len += seg->length;  /* Fixed segment as-is */
            }

            /* Build expanded pattern */
            expanded_pattern = (char *)pg_malloc(expanded_len + 1);
            if (!expanded_pattern)
            {
                tecc_free_parsed_pattern(parsed_pattern);
                return TECC_ALGO_MEMORY_ERROR;
            }
            pos = 0;
            for (i = 0; i < parsed_pattern->segment_count; i++)
            {
                TeccPatternSegment *seg = &parsed_pattern->segments[i];
                if (seg->type == TECC_PATTERN_SEGMENT_WILDCARD_UNDERSCORE)
                {
                    expanded_pattern[pos++] = '_';
                    expanded_pattern[pos++] = '_';
                    expanded_pattern[pos++] = '_';
                }
                else if (seg->type == TECC_PATTERN_SEGMENT_WILDCARD_PERCENT)
                {
                    expanded_pattern[pos++] = '%';
                }
                else if (seg->data.fixed_string && seg->length > 0)
                {
                    memcpy(expanded_pattern + pos,
                           seg->data.fixed_string, seg->length);
                    pos += seg->length;
                }
            }
            expanded_pattern[expanded_len] = '\0';

            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Expanded _ wildcard pattern: original=%.*s expanded=%s",
                               (int)pattern_len, pattern, expanded_pattern);

            ret = tecc_algo_search_encrypt_exact(context->dek_key,
                                                context->dek_key_len,
                                                expanded_pattern,
                                                expanded_len,
                                                search_ciphertext,
                                                search_cipher_len, context->like_escape_char);
            pg_free(expanded_pattern);
            break;
        }

        case TECC_PATTERN_COMPLEX:
            /* Complex pattern with multiple wildcards */
            tecc_processor_debug(TECC_LOG_WARNING,
                               "Complex pattern with multiple wildcards, using exact encryption: %.*s",
                               (int)pattern_len, pattern);
            ret = tecc_algo_search_encrypt_exact(context->dek_key,
                                                context->dek_key_len,
                                                pattern,
                                                pattern_len,
                                                search_ciphertext,
                                                search_cipher_len, context->like_escape_char);
            break;

        default:
            tecc_processor_debug(TECC_LOG_ERROR,
                               "Unknown pattern type: %d", parsed_pattern->pattern_type);
            ret = TECC_ALGO_INVALID_PARAM;
            break;
    }

    /* Clean up parsed pattern */
    tecc_free_parsed_pattern(parsed_pattern);

    if (ret != TECC_ALGO_SUCCESS)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "SEARCH algorithm LIKE encryption failed with error %d", ret);
        return ret;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "SEARCH algorithm LIKE encryption completed successfully with unified parser");

    return TECC_ALGO_SUCCESS;
}

/* End - ReqID:TECC4.0 */

/* ReqID:TECC4.0 - Load a DEK's real HE private key (EC for he-add, FF for he-mul),
 * unwrapped with the KEK via SM2 envelope decryption.  Returns 0 on success with
 * *out_priv allocated (caller frees with BN_free); -1 on error.  Used by batch
 * decrypt to load the key once per DEK instead of per row. */
int
tecc_he_load_dek_privkey(void *tecc_state, const char *dek_name,
                         int is_ec, BIGNUM **out_priv)
{
    char *he_priv_hex = NULL;
    char *kek_name_hex = NULL;
    const char *priv_key_name;
    BIGNUM *per_dek_priv = NULL;

    if (out_priv == NULL) return -1;
    *out_priv = NULL;
    if (tecc_state == NULL || dek_name == NULL) return -1;

    priv_key_name = is_ec ? "he_ec_privkey" : "he_eg_privkey";

    if (tecc_he_load_dek_key_hex(tecc_state, dek_name, priv_key_name, &he_priv_hex) != 0 ||
        he_priv_hex == NULL)
        return -1;

    if (tecc_he_load_dek_key_hex(tecc_state, dek_name, "kek", &kek_name_hex) != 0 ||
        kek_name_hex == NULL)
    {
        pg_free(he_priv_hex);
        return -1;
    }

    {
        unsigned char *enc_priv = NULL;
        size_t enc_priv_len;
        unsigned char *plain_priv = NULL;
        size_t plain_priv_len = 0;
        unsigned char *plain_acc = NULL;
        size_t plain_acc_len = 0;
        unsigned char *kek = NULL;
        size_t kek_len = 0;
        int nchunks;
        int ci;
        int j;

        /* ReqID:TECC4.0 - 逐块解封后拼接：KEK 托管在密码机时 he-mul 的 256
         * 字节 FF-ElGamal 私钥被拆成 ≤128 字节的多块分别封装（设备
         * ECCCipher.C 上限 136 字节，真机实测）；legacy 单信封（无 'v2:'
         * 前缀）按 1 块处理，路径与原来完全一致。 */
        nchunks = tecc_sm2_env_chunk_count(he_priv_hex);
        if (nchunks <= 0)
        {
            pg_free(he_priv_hex);
            pg_free(kek_name_hex);
            return -1;
        }

        for (ci = 0; ci < nchunks; ci++)
        {
        char       *chunk_hex = NULL;

        if (tecc_sm2_env_chunk_hex(he_priv_hex, ci, &chunk_hex) != TECC_SM2_SUCCESS)
            break;

        enc_priv_len = strlen(chunk_hex) / 2;
        enc_priv = (unsigned char *)pg_malloc(enc_priv_len);
        if (enc_priv != NULL)
        {
            for (j = 0; j < (int)enc_priv_len; j++)
            {
                char c1 = chunk_hex[j * 2];
                char c2 = chunk_hex[j * 2 + 1];
                unsigned char n1, n2;
                n1 = (c1 >= '0' && c1 <= '9') ? (unsigned char)(c1 - '0') :
                     (c1 >= 'a' && c1 <= 'f') ? (unsigned char)(c1 - 'a' + 10) :
                     (c1 >= 'A' && c1 <= 'F') ? (unsigned char)(c1 - 'A' + 10) : 0;
                n2 = (c2 >= '0' && c2 <= '9') ? (unsigned char)(c2 - '0') :
                     (c2 >= 'a' && c2 <= 'f') ? (unsigned char)(c2 - 'a' + 10) :
                     (c2 >= 'A' && c2 <= 'F') ? (unsigned char)(c2 - 'A' + 10) : 0;
                enc_priv[j] = (n1 << 4) | n2;
            }

			/* ReqID:TECC4.0 - Scheme B: unwrap HE private key with the KEK's
			 * SM2 private key (PKCS#8 DER). */
		kek = tecc_key_get_kek_with_state(tecc_state, kek_name_hex, &kek_len);
		if (kek != NULL)
		{
			/* ReqID:TECC4.0 - HSM-hosted SM2 KEK: unwrap via device sm2_decrypt,
			 * so the SM2 private key never leaves the device. */
				if (kek_len == 16 &&
					(memcmp(kek + 4, "HSM_KEY", 7) == 0 ||
					 memcmp(kek + 4, "SWXAKEY", 7) == 0))
				{
					bool is_swxa = (memcmp(kek + 4, "SWXAKEY", 7) == 0);
					unsigned int key_index = (unsigned int)kek[0] |
						((unsigned int)kek[1] << 8) |
						((unsigned int)kek[2] << 16) |
						((unsigned int)kek[3] << 24);
					HsmAdapterInterface *hsm_adapter =
						is_swxa ? get_swxa_adapter() : get_tzhsm_adapter();
					HsmDevice *device = NULL;

					if (hsm_adapter != NULL && hsm_adapter->sm2_decrypt != NULL &&
						hsm_adapter->init(&device, NULL) == HSM_SUCCESS)
					{
						if (hsm_adapter->connect(device) == HSM_SUCCESS)
						{
							if (hsm_adapter->sm2_decrypt(device, key_index,
														 enc_priv, enc_priv_len,
														 (void **)&plain_priv,
														 &plain_priv_len) == HSM_SUCCESS &&
								plain_priv != NULL && plain_priv_len > 0)
							{
								per_dek_priv = BN_bin2bn(plain_priv, (int)plain_priv_len, NULL);
								tecc_processor_debug(TECC_LOG_DEBUG1,
									"HE decrypt: unwrapped per-DEK private key via HSM sm2_decrypt (len=%zu)",
									plain_priv_len);
							}
							hsm_adapter->disconnect(device);
						}
						hsm_adapter->cleanup(device);
					}
				}
				else
				{
					if (tecc_sm2_envelope_decrypt(kek, kek_len,
												  enc_priv, enc_priv_len,
												  &plain_priv, &plain_priv_len) == TECC_SM2_SUCCESS &&
						plain_priv != NULL && plain_priv_len > 0)
					{
						per_dek_priv = BN_bin2bn(plain_priv, (int)plain_priv_len, NULL);
						tecc_processor_debug(TECC_LOG_DEBUG1,
							"HE decrypt: loaded per-DEK private key for '%s' (len=%zu)",
							dek_name, plain_priv_len);
					}
				}
				tecc_key_free_key_data(kek, kek_len);
			}
            pg_free(enc_priv);
        }
        pg_free(chunk_hex);
        chunk_hex = NULL;

        if (plain_priv == NULL || plain_priv_len == 0)
        {
            if (plain_priv != NULL)
                OPENSSL_free(plain_priv);
            break;
        }

        /* 追加本块明文，得到完整私钥 */
        plain_acc = (unsigned char *)pg_realloc(plain_acc, plain_acc_len + plain_priv_len);
        if (plain_acc == NULL)
        {
            OPENSSL_free(plain_priv);
            plain_acc_len = 0;
            break;
        }
        memcpy(plain_acc + plain_acc_len, plain_priv, plain_priv_len);
        plain_acc_len += plain_priv_len;
        OPENSSL_cleanse(plain_priv, plain_priv_len);
        OPENSSL_free(plain_priv);
        plain_priv = NULL;
        plain_priv_len = 0;
        }

        /* kek 已在每轮循环内解封后释放（见上），此处不得再释放，
         * 否则是对已释放指针的二次释放。 */
        if (per_dek_priv != NULL)
        {
            /* 循环内按「单块明文」建过 BIGNUM，分块情形下那是中间结果 */
            BN_free(per_dek_priv);
            per_dek_priv = NULL;
        }
        if (plain_acc != NULL && plain_acc_len > 0)
            per_dek_priv = BN_bin2bn(plain_acc, (int)plain_acc_len, NULL);
        if (plain_acc != NULL)
        {
            OPENSSL_cleanse(plain_acc, plain_acc_len);
            pg_free(plain_acc);
        }
    }
    pg_free(he_priv_hex);
    pg_free(kek_name_hex);

    if (per_dek_priv == NULL) return -1;
    *out_priv = per_dek_priv;
    return 0;
}

/* ReqID:TECC4.0 - Pure HE decrypt core (no private-key load, no BN_CTX create/free).
 * Accepts an externally owned BN_CTX and a pre-loaded per-DEK private key, so
 * batch decrypt can reuse one BN_CTX and one private key across all rows.
 * The private key MUST be provided: no hardcoded/test-key fallback exists for
 * the FF ElGamal (he-mul) path.  If it is absent, decryption fails explicitly. */
int
tecc_he_decrypt_cipherdata_ex(const unsigned char *eg_add_data, size_t eg_add_len,
                              const unsigned char *eg_mul_data, size_t eg_mul_len,
                              const BIGNUM *per_dek_priv, BN_CTX *bn_ctx,
                              char **plaintext, size_t *plain_len)
{
    int ret;
    BIGNUM *bn_result;
    char *dec_str;

    if (plaintext == NULL || plain_len == NULL || bn_ctx == NULL)
        return TECC_ALGO_INVALID_PARAM;
    *plaintext = NULL;
    *plain_len = 0;

    bn_result = BN_new();
    if (!bn_result) return TECC_ALGO_MEMORY_ERROR;

    if (eg_add_data && eg_add_len > 0)
    {
        TeccECCRTCiphertext ct;
        ret = he_crypto_init();
        if (ret) { BN_free(bn_result); return ret; }
        memset(&ct, 0, sizeof(ct));
        /* Skip the 4-byte per-segment DEK ID prefix */
        if (eg_add_len <= CIPHERDATA_HE_DEK_ID_SIZE)
        { BN_free(bn_result); return TECC_ALGO_DECRYPTION_FAILED; }
        ret = tecc_ec_crt_deserialize(&ct, eg_add_data + CIPHERDATA_HE_DEK_ID_SIZE,
                                      eg_add_len - CIPHERDATA_HE_DEK_ID_SIZE, he_ec_group);
        if (ret) { tecc_ec_crt_ciphertext_free(&ct, he_ec_group); BN_free(bn_result); return TECC_ALGO_DECRYPTION_FAILED; }
        /* ReqID:TECC4.0 - EC-ElGamal 私钥必须来自密钥管理（per-DEK）。无回退到
         * 硬编码密钥：per_dek_priv 为空则显式失败。 */
        if (per_dek_priv == NULL)
        {
            tecc_ec_crt_ciphertext_free(&ct, he_ec_group);
            BN_free(bn_result);
            return TECC_ALGO_DECRYPTION_FAILED;
        }
        ret = tecc_ec_crt_decrypt(bn_result, he_ec_group,
                                  per_dek_priv,
                                  &ct, &he_crt_params, he_bsgs_tables, bn_ctx);
        tecc_ec_crt_ciphertext_free(&ct, he_ec_group);
        if (ret) { BN_free(bn_result); return TECC_ALGO_DECRYPTION_FAILED; }
    }
    else if (eg_mul_data && eg_mul_len > 0)
    {
        BIGNUM *eg_c1, *eg_c2;
        ret = he_eg_init();
        if (ret) { BN_free(bn_result); return ret; }
        eg_c1 = BN_new(); eg_c2 = BN_new();
        if (!eg_c1 || !eg_c2)
        {
            BN_free(eg_c1); BN_free(eg_c2);
            BN_free(bn_result);
            return TECC_ALGO_MEMORY_ERROR;
        }
        /* Skip the 4-byte per-segment DEK ID prefix */
        if (eg_mul_len <= CIPHERDATA_HE_DEK_ID_SIZE)
        {
            BN_free(eg_c1); BN_free(eg_c2);
            BN_free(bn_result);
            return TECC_ALGO_DECRYPTION_FAILED;
        }
        ret = tecc_elgamal_deserialize(eg_c1, eg_c2,
                                       eg_mul_data + CIPHERDATA_HE_DEK_ID_SIZE,
                                       (int)(eg_mul_len - CIPHERDATA_HE_DEK_ID_SIZE));
        if (ret != TECC_ELGAMAL_SUCCESS)
        {
            BN_free(eg_c1); BN_free(eg_c2);
            BN_free(bn_result);
            return TECC_ALGO_DECRYPTION_FAILED;
        }
        /* ReqID:TECC4.0 - he-mul 私钥必须来自密钥管理（per-DEK）。无回退到硬编码
         * 密钥：per_dek_priv 为空则显式失败，避免用公开私钥解密产生错误明文。 */
        if (per_dek_priv == NULL)
        {
            BN_free(eg_c1); BN_free(eg_c2);
            BN_free(bn_result);
            return TECC_ALGO_DECRYPTION_FAILED;
        }
        ret = tecc_elgamal_decrypt(he_eg_p, per_dek_priv,
                                   eg_c1, eg_c2, bn_result, bn_ctx);
        BN_free(eg_c1); BN_free(eg_c2);
        if (ret != TECC_ELGAMAL_SUCCESS)
        {
            BN_free(bn_result);
            return TECC_ALGO_DECRYPTION_FAILED;
        }
    }
    else
    {
        BN_free(bn_result);
        return TECC_ALGO_INVALID_PARAM;
    }

    /* Convert BIGNUM to decimal string (avoids int64 overflow) */
    dec_str = BN_bn2dec(bn_result);
    BN_free(bn_result);
    if (!dec_str) return TECC_ALGO_MEMORY_ERROR;

    *plain_len = strlen(dec_str);
    *plaintext = (char *)pg_malloc(*plain_len + 1);
    memcpy(*plaintext, dec_str, *plain_len + 1);
    OPENSSL_free(dec_str);
    return TECC_ALGO_SUCCESS;
}

/* ReqID:TECC4.0 - Decrypt HE-only cipherdata (EG_ADD or EG_MUL slot).  Wrapper that
 * loads the DEK private key and owns a BN_CTX for single-row callers. */
int
tecc_he_decrypt_cipherdata(const unsigned char *eg_add_data, size_t eg_add_len,
                           const unsigned char *eg_mul_data, size_t eg_mul_len,
                           const char *dek_name, void *tecc_state,
                           char **plaintext, size_t *plain_len)
{
    int ret;
    BIGNUM *per_dek_priv = NULL;
    BN_CTX *bn_ctx;
    int is_ec;

    if (plaintext == NULL || plain_len == NULL) return TECC_ALGO_INVALID_PARAM;

    is_ec = (eg_add_data && eg_add_len > 0) ? 1 : 0;
    if (dek_name != NULL && tecc_state != NULL)
    {
        /* ReqID:TECC4.0 - 无私钥客户端解封 HE 私钥信封失败（如本地无 .priv
         * 文件）时，必须明确返回解密失败，不能静默回退到硬编码测试密钥
         * （否则会返回错误明文，造成静默数据错误）。 */
        if (tecc_he_load_dek_privkey(tecc_state, dek_name, is_ec, &per_dek_priv) != 0)
            return TECC_ALGO_DECRYPTION_FAILED;
    }

    bn_ctx = BN_CTX_new();
    if (!bn_ctx)
    {
        if (per_dek_priv) BN_free(per_dek_priv);
        return TECC_ALGO_MEMORY_ERROR;
    }

    ret = tecc_he_decrypt_cipherdata_ex(eg_add_data, eg_add_len,
                                        eg_mul_data, eg_mul_len,
                                        per_dek_priv, bn_ctx,
                                        plaintext, plain_len);

    BN_CTX_free(bn_ctx);
    if (per_dek_priv) BN_free(per_dek_priv);
    return ret;
}

/* End - ReqID:TECC4.0 */
/* ReqID:TECC4.0 - Cleanup HE EC crypto resources */
static void he_crypto_cleanup(void)
{
    int i;
    if (!he_crypto_initialized) return;
    he_crypto_initialized = false;
    for (i = 0; i < TECC_EC_CRT_NUM_PRIMES; i++)
    {
        if (he_bsgs_tables[i])
        {
            tecc_ec_bsgs_table_free(he_bsgs_tables[i]);
            pg_free(he_bsgs_tables[i]);
            he_bsgs_tables[i] = NULL;
        }
    }
    tecc_ec_crt_params_free(&he_crt_params);
    EC_POINT_free(he_ec_pubkey); he_ec_pubkey = NULL;
    BN_free(he_ec_privkey); he_ec_privkey = NULL;
    EC_GROUP_free(he_ec_group); he_ec_group = NULL;
}

/* ReqID:TECC4.0 - Cleanup FF ElGamal crypto resources */
static void he_eg_cleanup(void)
{
    if (!he_eg_initialized) return;
    he_eg_initialized = false;
    BN_free(he_eg_pubkey1); he_eg_pubkey1 = NULL;
    BN_free(he_eg_pubkey2); he_eg_pubkey2 = NULL;
    BN_free(he_eg_pubkey); he_eg_pubkey = NULL;
    BN_free(he_eg_privkey); he_eg_privkey = NULL;
    BN_free(he_eg_p); he_eg_p = NULL;
    BN_free(he_eg_q); he_eg_q = NULL;
    BN_free(he_eg_g); he_eg_g = NULL;
    tecc_elgamal_crt_params_free(&he_eg_crt_params);
}

/* atexit handler for both HE subsystems */
static void he_global_cleanup(void)
{
    he_crypto_cleanup();
    he_eg_cleanup();
}

