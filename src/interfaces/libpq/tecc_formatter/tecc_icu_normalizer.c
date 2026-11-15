/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_icu_normalizer.c
 *
 * Abstract:
 *   ICU normalization implementation for TECC data formatter module.
 *   Provides ICU-based text normalization and sort key generation.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_icu_normalizer.c
 */

#include "tecc_icu_normalizer.h"
#include "common/fe_memutils.h"  /* For pg_malloc, pg_free */
#include "tecc_formatter_internal.h"

#include <unicode/ucol.h>
#include <unicode/ustring.h>
#include <unicode/uclean.h>
#include <unicode/uversion.h>
#include <unicode/unorm2.h>

static bool icu_initialized = false;

/*
 * tecc_icu_normalizer_init - Initialize ICU normalizer
 */
TeccFormatterErrorCode
tecc_icu_normalizer_init(void)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    UErrorCode status = U_ZERO_ERROR;

    if (icu_initialized)
        return TECC_FORMATTER_SUCCESS;
    u_init(&status);

    if (U_FAILURE(status))
        return TECC_FORMATTER_ERROR_ICU_NOT_AVAILABLE;

    icu_initialized = true;
    return TECC_FORMATTER_SUCCESS;
}

/*
 * tecc_icu_normalizer_get_collation_name - Get collation name from locale string (direct copy)
 */
TeccFormatterErrorCode
tecc_icu_normalizer_get_collation_name(const char *collation_locale, char *out_buf, size_t buf_size)
{
    if (!collation_locale || !out_buf || buf_size == 0)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    /* Directly copy the locale string to output buffer */
    strlcpy(out_buf, collation_locale, buf_size);

    return TECC_FORMATTER_SUCCESS;
}

/*
 * tecc_icu_normalizer_create_collator - Create ICU collator
 */
TeccFormatterErrorCode
tecc_icu_normalizer_create_collator(const char *collation_locale, void **collator)
{
    UErrorCode status = U_ZERO_ERROR;
    UCollator *icu_collator = NULL;

    if (!collation_locale || !collator)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    if (!icu_initialized)
        return TECC_FORMATTER_ERROR_ICU_NOT_AVAILABLE;

    /* Create ICU collator */
    icu_collator = ucol_open(collation_locale, &status);
    if (U_FAILURE(status) || !icu_collator) {
        return TECC_FORMATTER_ERROR_COLLATION_INVALID;
    }

    *collator = (void *)icu_collator;
    return TECC_FORMATTER_SUCCESS;
}

/*
 * tecc_icu_normalizer_generate_sortkey_with_collator - Generate sort key with existing collator
 */
TeccFormatterErrorCode
tecc_icu_normalizer_generate_sortkey_with_collator(void *collator,
                                                   const unsigned char *input_text,
                                                   size_t input_len,
                                                   unsigned char **sortkey,
                                                   size_t *sortkey_len)
{
    UCollator *icu_collator = (UCollator *)collator;
    UErrorCode status = U_ZERO_ERROR;
    int32_t uchar_len;
    UChar *utext;
    int32_t key_capacity;
    int32_t key_length;

    if (!icu_collator || !input_text || !sortkey || !sortkey_len)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    if (input_len == 0)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    /* Convert input to UChar (UTF-16) */
    uchar_len = 0;
    utext = NULL;

    /* Calculate required buffer size */
    u_strFromUTF8(NULL, 0, &uchar_len, (const char *)input_text, input_len, &status);
    if (status == U_BUFFER_OVERFLOW_ERROR) {
        status = U_ZERO_ERROR;  /* Reset error */
        utext = pg_malloc((uchar_len + 1) * sizeof(UChar));
        if (!utext)
            return TECC_FORMATTER_ERROR_MEMORY_ALLOC;

        u_strFromUTF8(utext, uchar_len + 1, NULL, (const char *)input_text, input_len, &status);
    }

    if (U_FAILURE(status)) {
        if (utext)
            pg_free(utext);
        return TECC_FORMATTER_ERROR_ENCODING_FAILED;
    }

    /* Get sort key size */
    key_capacity = ucol_getSortKey(icu_collator, utext, uchar_len, NULL, 0);
    if (key_capacity <= 0) {
        pg_free(utext);
        return TECC_FORMATTER_ERROR_SORTKEY_FAILED;
    }

    /* Allocate sort key buffer */
    *sortkey = pg_malloc(key_capacity);
    if (!*sortkey) {
        pg_free(utext);
        return TECC_FORMATTER_ERROR_MEMORY_ALLOC;
    }

    /* Generate sort key */
    key_length = ucol_getSortKey(icu_collator, utext, uchar_len, *sortkey, key_capacity);
    pg_free(utext);

    if (key_length <= 0) {
        pg_free(*sortkey);
        *sortkey = NULL;
        return TECC_FORMATTER_ERROR_SORTKEY_FAILED;
    }

    *sortkey_len = key_length;
    return TECC_FORMATTER_SUCCESS;
}

/*
 * tecc_icu_normalizer_destroy_collator - Destroy ICU collator
 */
void
tecc_icu_normalizer_destroy_collator(void *collator)
{
    if (collator) {
        ucol_close((UCollator *)collator);
    }
}

/*
 * tecc_icu_normalizer_normalize_text - Normalize text using ICU
 */
TeccFormatterErrorCode
tecc_icu_normalizer_normalize_text(
    const char *locale,
    const unsigned char *input_text,
    size_t input_len,
    unsigned char **normalized_text,
    size_t *normalized_len)
{
    UErrorCode status;
    UNormalizer2 *normalizer;
    UChar *utext;
    UChar *normalized_utext;
    int32_t utext_len;
    int32_t normalized_utext_len;
    int32_t capacity;
    int32_t utf8_len;
    TeccFormatterErrorCode result;

    TECC_RETURN_IF_NULL(locale, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(input_text, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(normalized_text, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(normalized_len, TECC_FORMATTER_ERROR_INVALID_PARAM);

    if (!icu_initialized)
        return TECC_FORMATTER_ERROR_ICU_NOT_AVAILABLE;

    status = U_ZERO_ERROR;
    normalizer = NULL;
    utext = NULL;
    normalized_utext = NULL;
    utext_len = 0;
    normalized_utext_len = 0;
    capacity = 0;
    result = TECC_FORMATTER_SUCCESS;

    /* Initialize output parameters */
    *normalized_text = NULL;
    *normalized_len = 0;

    /* Step 1: Get ICU normalizer for the requested normalization form */
    /* For now, use NFC (Normalization Form C) as default */
    normalizer = (UNormalizer2 *)unorm2_getNFCInstance(&status);
    if (U_FAILURE(status) || !normalizer) {
        result = TECC_FORMATTER_ERROR_ICU_NOT_AVAILABLE;
        goto cleanup;
    }

    /* Step 2: Convert input text to UChar (UTF-16) */
    utext_len = input_len * 2;  /* Maximum possible length */
    utext = (UChar *)pg_malloc(utext_len * sizeof(UChar));
    if (!utext) {
        result = TECC_FORMATTER_ERROR_MEMORY_ALLOC;
        goto cleanup;
    }

    /* Convert from UTF-8 to UTF-16 */
    u_strFromUTF8(utext, utext_len, &utext_len,
                  (const char *)input_text, input_len, &status);
    if (U_FAILURE(status)) {
        result = TECC_FORMATTER_ERROR_ENCODING_FAILED;
        goto cleanup;
    }

    /* Step 3: Pre-calculate normalized text size */
    normalized_utext_len = unorm2_normalize(normalizer, utext, utext_len,
                                            NULL, 0, &status);
    if (status == U_BUFFER_OVERFLOW_ERROR) {
        status = U_ZERO_ERROR;  /* Reset error for actual allocation */
    } else if (U_FAILURE(status)) {
        result = TECC_FORMATTER_ERROR_CONVERSION_FAILED;
        goto cleanup;
    }

    /* Step 4: Allocate buffer for normalized text */
    capacity = normalized_utext_len + 1;  /* +1 for safety */
    normalized_utext = (UChar *)pg_malloc(capacity * sizeof(UChar));
    if (!normalized_utext) {
        result = TECC_FORMATTER_ERROR_MEMORY_ALLOC;
        goto cleanup;
    }

    /* Step 5: Perform normalization */
    normalized_utext_len = unorm2_normalize(normalizer, utext, utext_len,
                                            normalized_utext, capacity, &status);
    if (U_FAILURE(status)) {
        result = TECC_FORMATTER_ERROR_CONVERSION_FAILED;
        goto cleanup;
    }

    /* Step 6: Convert normalized text back to UTF-8 */
    /* First calculate required buffer size */
    utf8_len = 0;
    u_strToUTF8(NULL, 0, &utf8_len, normalized_utext, normalized_utext_len, &status);
    if (status == U_BUFFER_OVERFLOW_ERROR) {
        status = U_ZERO_ERROR;  /* Reset error for actual allocation */
    } else if (U_FAILURE(status)) {
        result = TECC_FORMATTER_ERROR_ENCODING_FAILED;
        goto cleanup;
    }

    /* Allocate UTF-8 buffer */
    *normalized_text = pg_malloc(utf8_len + 1);  /* +1 for null terminator */
    if (!*normalized_text) {
        result = TECC_FORMATTER_ERROR_MEMORY_ALLOC;
        goto cleanup;
    }

    /* Convert to UTF-8 */
    u_strToUTF8((char *)*normalized_text, utf8_len + 1, NULL,
                normalized_utext, normalized_utext_len, &status);
    if (U_FAILURE(status)) {
        result = TECC_FORMATTER_ERROR_ENCODING_FAILED;
        goto cleanup;
    }

    *normalized_len = utf8_len;

cleanup:
    /* Cleanup resources */
    if (utext)
        pg_free(utext);
    if (normalized_utext)
        pg_free(normalized_utext);

    /* If error occurred, cleanup normalized_text buffer */
    if (result != TECC_FORMATTER_SUCCESS && *normalized_text) {
        pg_free(*normalized_text);
        *normalized_text = NULL;
        *normalized_len = 0;
    }

    return result;
}



/*
 * tecc_icu_normalizer_generate_sortkey - Generate ICU sort key
 */
TeccFormatterErrorCode
tecc_icu_normalizer_generate_sortkey(
    const char *collation_locale,
    const unsigned char *input_text,
    size_t input_len,
    unsigned char **sortkey,
    size_t *sortkey_len)
{
    void *collator = NULL;
    TeccFormatterErrorCode ret;

    TECC_RETURN_IF_NULL(collation_locale, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(input_text, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(sortkey, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(sortkey_len, TECC_FORMATTER_ERROR_INVALID_PARAM);

    if (!icu_initialized)
        return TECC_FORMATTER_ERROR_ICU_NOT_AVAILABLE;

    /* Use the helper functions */

    /* Create ICU collator directly using locale string */
    ret = tecc_icu_normalizer_create_collator(collation_locale, &collator);
    if (ret != TECC_FORMATTER_SUCCESS)
        return ret;

    /* Generate sort key */
    ret = tecc_icu_normalizer_generate_sortkey_with_collator(collator, input_text, input_len, sortkey, sortkey_len);

    /* Cleanup collator */
    tecc_icu_normalizer_destroy_collator(collator);

    return ret;
}

/*
 * tecc_icu_normalizer_is_available - Check if ICU is available
 */
bool
tecc_icu_normalizer_is_available(void)
{
    return icu_initialized;
}

/* End - ReqID:TECC4.0 */
