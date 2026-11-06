/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_text_normalizer.h
 *
 * Abstract:
 *   Text normalization interfaces for TECC data formatter module.
 *   Handles text encoding, ICU sort key generation, and text segmentation.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_text_normalizer.h
 */

#ifndef TECC_TEXT_NORMALIZER_H
#define TECC_TEXT_NORMALIZER_H

#include "tecc_formatter.h"
#include "tecc_formatter_internal.h"

/*
 * Generate ICU sort key for text data
 * Used by OPE algorithm to ensure order preservation
 */
extern TeccFormatterErrorCode tecc_text_normalizer_generate_icu_sortkey(
    const char *collation_locale,
    const unsigned char *input_text,
    size_t input_len,
    unsigned char **sortkey,
    size_t *sortkey_len
);


/*
 * Free text segments
 */
extern void tecc_text_normalizer_free_segments(
    unsigned char **segments,
    size_t *segment_lens,
    int segment_count
);

#endif /* TECC_TEXT_NORMALIZER_H */