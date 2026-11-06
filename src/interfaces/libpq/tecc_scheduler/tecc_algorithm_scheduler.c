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
    "SEARCH-SM4"
};

/* Algorithm capabilities */
static const TeccAlgorithmCaps algorithm_caps[] = {
    {false, false, false, false, 0, 0},                    /* UNKNOWN */
    {true, true, false, false, 65536, 32},                 /* RND */
    {true, true, false, false, 65536, 16},                 /* DET */
    {true, false, true, false, 65536, 64},                 /* OPE */
    {true, false, false, true, 65536, 256}                 /* SEARCH */
};

/* LIKE pattern types */
/* Pattern type classification - now defined in tecc_pattern.h */
#include "../../../src/include/common/tecc_algorithm/tecc_pattern.h"

/* Parsed LIKE pattern structure */
typedef struct TeccSearchPattern
{
    TeccPatternType pattern_type;   /* Pattern type */
    char *pattern;                  /* Original pattern */
    size_t pattern_len;             /* Pattern length */

    /* Pattern components */
    char *prefix;                   /* Prefix for prefix patterns */
    size_t prefix_len;              /* Prefix length */
    char *suffix;                   /* Suffix for suffix patterns */
    size_t suffix_len;              /* Suffix length */
    char *infix;                    /* Infix for infix patterns */
    size_t infix_len;               /* Infix length */

    /* Wildcard information */
    bool has_wildcards;             /* Whether pattern contains wildcards */
    int wildcard_count;             /* Number of wildcard characters */

    /* ESCAPE semantics support */
    char escape_char;               /* ESCAPE character (0 if no ESCAPE) */
    bool has_escape;                /* Whether pattern has ESCAPE semantics */
} TeccSearchPattern;

#if 0
/* Helper function to clean up parsed pattern */
static void
tecc_cleanup_parsed_pattern(TeccSearchPattern *parsed_pattern)
{
    if (!parsed_pattern)
        return;

    if (parsed_pattern->pattern)
        pg_free(parsed_pattern->pattern);
    if (parsed_pattern->prefix)
        pg_free(parsed_pattern->prefix);
    if (parsed_pattern->suffix)
        pg_free(parsed_pattern->suffix);
    if (parsed_pattern->infix)
        pg_free(parsed_pattern->infix);

    memset(parsed_pattern, 0, sizeof(TeccSearchPattern));
}
#endif

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
                                          size_t *cipher_len);

static int tecc_algo_search_encrypt_exact(const unsigned char *key,
                                         size_t key_len,
                                         const char *pattern,
                                         size_t pattern_len,
                                         unsigned char **ciphertext,
                                         size_t *cipher_len);

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
            int i;

            actual_key_len = (ext_context->dek_key_len >= TECC_SM4_KEY_SIZE) ?
                             TECC_SM4_KEY_SIZE : ext_context->dek_key_len;
            memcpy(sm4_key, ext_context->dek_key, actual_key_len);

            for (i = 0; i < 16; i++)
                iv[i] = rand() % 256;
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
                                             output_buffer, required_size, &output_len, false);
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
                                     output_buffer, required_size, &output_len, false);

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
                                     output_buffer, required_size, &output_len, false);

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

    /* Call zero-copy algorithm API with real DEK key */
    output_len = 0;
    ret = tecc_algo_encrypt_zero_copy(TECC_ALGO_SEARCH,
                                     ext_context->dek_key, ext_context->dek_key_len,
                                     context->plaintext, context->plaintext_len,
                                     output_buffer, required_size, &output_len,
                                     false);

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
                                         false);
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
                                         &output_len, false);
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

#if 0
/*
 * tecc_execute_algo_search_like - Execute SEARCH algorithm for LIKE patterns
 * (Legacy implementation - kept for backward compatibility)
 */
static int
tecc_execute_algo_search_like(TeccCryptoContextExtended *context,
                             TeccOperatorType operator_type,
                             unsigned char **search_ciphertext,
                             size_t *search_cipher_len)
{
    /* Suppress unused parameter warnings */
    (void)context;
    (void)operator_type;
    (void)search_ciphertext;
    (void)search_cipher_len;

    /* This function is kept for backward compatibility but not used */
    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Legacy tecc_execute_algo_search_like called - using unified implementation");
    return TECC_ALGO_NOT_IMPLEMENTED;
}
#endif

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
                                         search_cipher_len);
}

#if 0
/*
 * tecc_parse_like_pattern - Parse LIKE pattern into components
 */
static int
tecc_parse_like_pattern(const char *pattern,
                       size_t pattern_len,
                       char escape_char,
                       TeccSearchPattern *parsed_pattern)
{
    int i;
    bool has_percent = false;
    bool has_underscore = false;
    int percent_count = 0;
    int first_percent = -1;
    int last_percent = -1;

    if (!pattern || pattern_len == 0 || !parsed_pattern)
    {
        return TECC_ALGO_INVALID_PARAM;
    }

    /* Initialize parsed pattern */
    memset(parsed_pattern, 0, sizeof(TeccSearchPattern));
    parsed_pattern->pattern_type = TECC_PATTERN_UNKNOWN;
    parsed_pattern->has_wildcards = false;
    parsed_pattern->wildcard_count = 0;
    parsed_pattern->escape_char = escape_char;
    parsed_pattern->has_escape = (escape_char != 0);

    /* Copy pattern for analysis */
    parsed_pattern->pattern = pg_malloc(pattern_len + 1);
    if (!parsed_pattern->pattern)
    {
        return TECC_ALGO_MEMORY_ERROR;
    }
    memcpy(parsed_pattern->pattern, pattern, pattern_len);
    parsed_pattern->pattern[pattern_len] = '\0';
    parsed_pattern->pattern_len = pattern_len;

    /* Analyze pattern for wildcards with ESCAPE semantics */
    for (i = 0; i < (int)pattern_len; i++)
    {
        /* Check for escape character */
        if (escape_char != 0 && pattern[i] == escape_char)
        {
            /* Skip the next character - it's escaped */
            i++;
            if (i >= (int)pattern_len)
            {
                /* Escape character at end of pattern is invalid */
                tecc_cleanup_parsed_pattern(parsed_pattern);
                return TECC_ALGO_DATA_FORMAT_ERROR;
            }
            continue;
        }

        if (pattern[i] == '%')
        {
            has_percent = true;
            percent_count++;
            if (first_percent == -1)
                first_percent = i;
            last_percent = i;
        }
        else if (pattern[i] == '_')
        {
            has_underscore = true;
            parsed_pattern->wildcard_count++;
        }
    }

    parsed_pattern->has_wildcards = has_percent || has_underscore;

    /* Determine pattern type */
    if (!has_percent && !has_underscore)
    {
        /* No wildcards - exact match */
        parsed_pattern->pattern_type = TECC_PATTERN_EXACT;
    }
    else if (has_underscore)
    {
        /* Contains '_' wildcard */
        parsed_pattern->pattern_type = TECC_PATTERN_WILDCARD;
    }
    else if (percent_count == 1)
    {
        /* Single '%' wildcard */
        if (first_percent == 0)
        {
            /* '%xyz' - suffix pattern */
            parsed_pattern->pattern_type = TECC_PATTERN_SUFFIX;
            parsed_pattern->suffix_len = pattern_len - 1;
            parsed_pattern->suffix = pg_malloc(parsed_pattern->suffix_len + 1);
            if (!parsed_pattern->suffix)
            {
                tecc_cleanup_parsed_pattern(parsed_pattern);
                return TECC_ALGO_MEMORY_ERROR;
            }
            memcpy(parsed_pattern->suffix, pattern + 1, parsed_pattern->suffix_len);
            parsed_pattern->suffix[parsed_pattern->suffix_len] = '\0';
        }
        else if (first_percent == (int)pattern_len - 1)
        {
            /* 'abc%' - prefix pattern */
            parsed_pattern->pattern_type = TECC_PATTERN_PREFIX;
            parsed_pattern->prefix_len = pattern_len - 1;
            parsed_pattern->prefix = pg_malloc(parsed_pattern->prefix_len + 1);
            if (!parsed_pattern->prefix)
            {
                tecc_cleanup_parsed_pattern(parsed_pattern);
                return TECC_ALGO_MEMORY_ERROR;
            }
            memcpy(parsed_pattern->prefix, pattern, parsed_pattern->prefix_len);
            parsed_pattern->prefix[parsed_pattern->prefix_len] = '\0';
        }
        else
        {
            /* 'a%c' - treated as infix for simplicity */
            parsed_pattern->pattern_type = TECC_PATTERN_INFIX;
            /* Extract the part before '%' as infix for simplicity */
            parsed_pattern->infix_len = first_percent;
            parsed_pattern->infix = pg_malloc(parsed_pattern->infix_len + 1);
            if (!parsed_pattern->infix)
            {
                tecc_cleanup_parsed_pattern(parsed_pattern);
                return TECC_ALGO_MEMORY_ERROR;
            }
            memcpy(parsed_pattern->infix, pattern, parsed_pattern->infix_len);
            parsed_pattern->infix[parsed_pattern->infix_len] = '\0';
        }
    }
    else if (percent_count == 2 && first_percent == 0 && last_percent == (int)pattern_len - 1)
    {
        /* '%middle%' - infix pattern */
        parsed_pattern->pattern_type = TECC_PATTERN_INFIX;
        parsed_pattern->infix_len = pattern_len - 2;
        parsed_pattern->infix = pg_malloc(parsed_pattern->infix_len + 1);
        if (!parsed_pattern->infix)
        {
            tecc_cleanup_parsed_pattern(parsed_pattern);
            return TECC_ALGO_MEMORY_ERROR;
        }
        memcpy(parsed_pattern->infix, pattern + 1, parsed_pattern->infix_len);
        parsed_pattern->infix[parsed_pattern->infix_len] = '\0';
    }
    else
    {
        /* Complex pattern with multiple '%' */
        parsed_pattern->pattern_type = TECC_PATTERN_INFIX;
        /* For complex patterns, use the entire pattern as infix for simplicity */
        parsed_pattern->infix_len = pattern_len;
        parsed_pattern->infix = pg_malloc(parsed_pattern->infix_len + 1);
        if (!parsed_pattern->infix)
        {
            tecc_cleanup_parsed_pattern(parsed_pattern);
            return TECC_ALGO_MEMORY_ERROR;
        }
        memcpy(parsed_pattern->infix, pattern, parsed_pattern->infix_len);
        parsed_pattern->infix[parsed_pattern->infix_len] = '\0';
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Parsed LIKE pattern: type=%d, has_wildcards=%d, wildcard_count=%d",
                        parsed_pattern->pattern_type,
                        parsed_pattern->has_wildcards,
                        parsed_pattern->wildcard_count);

    return TECC_ALGO_SUCCESS;
}
#endif

/*
 * tecc_algo_search_encrypt_prefix - Encrypt prefix for SEARCH algorithm
 */
static int
tecc_algo_search_encrypt_prefix(const unsigned char *key,
                               size_t key_len,
                               const char *prefix,
                               size_t prefix_len,
                               unsigned char **ciphertext,
                               size_t *cipher_len)
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

    /* Use basic zero-copy interface for now */
    ret = tecc_algo_encrypt_zero_copy(TECC_ALGO_SEARCH,
                                     key,
                                     key_len,
                                     (const unsigned char *)prefix,
                                     prefix_len,
                                     output_buffer,
                                     required_size,
                                     &output_len,
                                     true);

    if (ret != TECC_ALGO_SUCCESS)
    {
        pg_free(output_buffer);
        return ret;
    }

    *ciphertext = output_buffer;
    *cipher_len = output_len;

    return TECC_ALGO_SUCCESS;
}

#if 0
/*
 * tecc_algo_search_encrypt_suffix - Encrypt suffix for SEARCH algorithm
 */
static int
tecc_algo_search_encrypt_suffix(const unsigned char *key,
                               size_t key_len,
                               const char *suffix,
                               size_t suffix_len,
                               unsigned char **ciphertext,
                               size_t *cipher_len)
{
    /* Similar to prefix encryption */
    return tecc_algo_search_encrypt_prefix(key, key_len, suffix, suffix_len,
                                          ciphertext, cipher_len);
}

/*
 * tecc_algo_search_encrypt_infix - Encrypt infix for SEARCH algorithm
 */
static int
tecc_algo_search_encrypt_infix(const unsigned char *key,
                              size_t key_len,
                              const char *infix,
                              size_t infix_len,
                              unsigned char **ciphertext,
                              size_t *cipher_len)
{
    /* Similar to prefix encryption */
    return tecc_algo_search_encrypt_prefix(key, key_len, infix, infix_len,
                                          ciphertext, cipher_len);
}
#endif

/*
 * tecc_algo_search_encrypt_exact - Encrypt exact pattern for SEARCH algorithm
 */
static int
tecc_algo_search_encrypt_exact(const unsigned char *key,
                              size_t key_len,
                              const char *pattern,
                              size_t pattern_len,
                              unsigned char **ciphertext,
                              size_t *cipher_len)
{
    /* Similar to prefix encryption */
    return tecc_algo_search_encrypt_prefix(key, key_len, pattern, pattern_len,
                                          ciphertext, cipher_len);
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
                                             search_cipher_len);
    }

    /* Parse LIKE pattern using unified parser */
    pattern = (const char *)context->plaintext;
    pattern_len = context->plaintext_len;

    /* Get escape character from context if available */
    escape_char = 0;
    /* TODO: Extract escape character from context if available */
    /* For now, use default (no escape) */

    pattern_ret = tecc_parse_pattern_unified(pattern, pattern_len, escape_char, &parsed_pattern);
    if (pattern_ret != TECC_PATTERN_SUCCESS)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to parse LIKE pattern with unified parser: error=%d, pattern=%.*s",
                           pattern_ret, (int)pattern_len, pattern);
        return tecc_convert_error_code(pattern_ret);
    }

    /* Encrypt pattern segments */
    pattern_ret = tecc_encrypt_pattern_segments(parsed_pattern,
                                               context->dek_key,
                                               context->dek_key_len);
    if (pattern_ret != TECC_PATTERN_SUCCESS)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to encrypt pattern segments: error=%d",
                           pattern_ret);
        tecc_free_parsed_pattern(parsed_pattern);
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
                                                    search_cipher_len);
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
                                                    search_cipher_len);
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
                                                    search_cipher_len);
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
                                                    search_cipher_len);
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
                                                    search_cipher_len);
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
                                                    search_cipher_len);
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
                                                    search_cipher_len);
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
                                                search_cipher_len);
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
                                                search_cipher_len);
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
