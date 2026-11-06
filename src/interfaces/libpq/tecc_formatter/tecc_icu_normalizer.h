/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_icu_normalizer.h
 *
 * Abstract:
 *   ICU normalization interfaces for TECC data formatter module.
 *   Provides ICU-based text normalization and sort key generation.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_icu_normalizer.h
 */

#ifndef TECC_ICU_NORMALIZER_H
#define TECC_ICU_NORMALIZER_H

#include "tecc_formatter.h"
#include "tecc_formatter_internal.h"

/*
 * Initialize ICU normalizer
 */
extern TeccFormatterErrorCode tecc_icu_normalizer_init(void);

/*
 * Cleanup ICU normalizer
 */
extern void tecc_icu_normalizer_cleanup(void);

/*
 * Generate ICU sort key
 */
extern TeccFormatterErrorCode tecc_icu_normalizer_generate_sortkey(
    const char *collation_locale,
    const unsigned char *input_text,
    size_t input_len,
    unsigned char **sortkey,
    size_t *sortkey_len
);

/*
 * Normalize text using ICU
 */
extern TeccFormatterErrorCode tecc_icu_normalizer_normalize_text(
    const char *locale,
    const unsigned char *input_text,
    size_t input_len,
    unsigned char **normalized_text,
    size_t *normalized_len
);

/*
 * Check if ICU is available
 */
extern bool tecc_icu_normalizer_is_available(void);

/*
 * Get collation name from locale string (direct copy)
 */
extern TeccFormatterErrorCode tecc_icu_normalizer_get_collation_name(
    const char *collation_locale,
    char *out_buf,
    size_t buf_size);

/*
 * Create ICU collator
 */
extern TeccFormatterErrorCode tecc_icu_normalizer_create_collator(
    const char *collation_locale,
    void **collator);

/*
 * Generate sort key with existing collator
 */
extern TeccFormatterErrorCode tecc_icu_normalizer_generate_sortkey_with_collator(
    void *collator,
    const unsigned char *input_text,
    size_t input_len,
    unsigned char **sortkey,
    size_t *sortkey_len);

/*
 * Destroy ICU collator
 */
extern void tecc_icu_normalizer_destroy_collator(void *collator);

#endif /* TECC_ICU_NORMALIZER_H */