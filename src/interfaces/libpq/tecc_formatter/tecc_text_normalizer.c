/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_text_normalizer.c
 *
 * Abstract:
 *   Text normalization implementation for TECC data formatter module.
 *   Handles text encoding, ICU sort key generation, and text segmentation.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_text_normalizer.c
 */

#include "tecc_text_normalizer.h"
#include "common/fe_memutils.h"  /* For pg_malloc, pg_free */
#include "tecc_formatter_internal.h"
#include "tecc_icu_normalizer.h"

#include <string.h>
#include <ctype.h>

/*
 * tecc_text_normalizer_generate_icu_sortkey - Generate ICU sort key for text data
 *
 * Uses ICU library to generate sort key for text, ensuring OPE encryption
 * maintains order-preserving property.
 */
TeccFormatterErrorCode
tecc_text_normalizer_generate_icu_sortkey(
    const char *collation_locale,
    const unsigned char *input_text,
    size_t input_len,
    unsigned char **sortkey,
    size_t *sortkey_len)
{
    TeccFormatterErrorCode ret;
    void *collator = NULL;

    TECC_RETURN_IF_NULL(collation_locale, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(input_text, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(sortkey, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(sortkey_len, TECC_FORMATTER_ERROR_INVALID_PARAM);

    if (input_len == 0)
        return TECC_FORMATTER_ERROR_DATA_FORMAT;

    /* Check if ICU is available */
    if (!tecc_icu_normalizer_is_available())
        return TECC_FORMATTER_ERROR_ICU_NOT_AVAILABLE;

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
 * tecc_text_normalizer_free_segments - Free text segments
 *
 * Frees memory allocated for text segments.
 */
void
tecc_text_normalizer_free_segments(
    unsigned char **segments,
    size_t *segment_lens,
    int segment_count)
{
    if (!segments || !segment_lens || segment_count <= 0)
        return;

    {
        int i;
        for (i = 0; i < segment_count; i++) {
            if (segments[i])
                pg_free(segments[i]);
        }
    }

    pg_free(segments);
    pg_free(segment_lens);
}

/* End - ReqID:TECC4.0 */