/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_normalization_common.h
 *
 * Abstract:
 *   Common normalization interfaces for TECC encryption algorithms.
 *   Provides shared normalization functions for OPE and DET algorithms.
 *
 * Authored by <wangxp@highgo.com>, 20260409.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_normalization_common.h
 */

#ifndef TECC_NORMALIZATION_COMMON_H
#define TECC_NORMALIZATION_COMMON_H

#include "postgres_fe.h"
#include "c.h"

/* Normalized numeric structure for OPE/DET algorithms */
typedef struct TeccNumericNormalizedCommon
{
    uint8_t     sign;       /* Sign: 0=positive, 1=negative, 2=zero, 3=NaN, 4=+inf, 5=-inf */
    int16_t     weight;     /* Weight (decimal point position) */
    int         ndigits;    /* Number of decimal digits */
    uint16_t   *digits;     /* Packed decimal digits (4 digits per uint16) */
} TeccNumericNormalizedCommon;

/* Common normalization error codes */
typedef enum
{
    TECC_NORMALIZATION_SUCCESS = 0,
    TECC_NORMALIZATION_ERROR_INVALID_PARAM = -1,
    TECC_NORMALIZATION_ERROR_MEMORY_ALLOC = -2,
    TECC_NORMALIZATION_ERROR_NUMERIC_OVERFLOW = -3,
    TECC_NORMALIZATION_ERROR_INVALID_TYPE = -4,
    TECC_NORMALIZATION_ERROR_ENCODING_FAILED = -5,
    TECC_NORMALIZATION_ERROR_ICU_NOT_AVAILABLE = -6,
    TECC_NORMALIZATION_ERROR_COLLATION_INVALID = -7,
    TECC_NORMALIZATION_ERROR_SORTKEY_FAILED = -8,
    TECC_NORMALIZATION_ERROR_BUFFER_TOO_SMALL = -9
} TeccNormalizationErrorCode;

/* Common normalization constants */
#define TECC_NUMERIC_HEADER_SIZE_COMMON    8  /* sign(1) + weight(2) + ndigits(2) + reserved(3) */
#define TECC_NUMERIC_MAX_DIGITS_COMMON     1000  /* Maximum decimal digits */
#define TECC_NUMERIC_MIN_WEIGHT_COMMON     -1000
#define TECC_NUMERIC_MAX_WEIGHT_COMMON     1000

/* Numeric sign values */
#define TECC_NUMERIC_SIGN_POSITIVE_COMMON  0
#define TECC_NUMERIC_SIGN_NEGATIVE_COMMON  1
#define TECC_NUMERIC_SIGN_ZERO_COMMON      2
#define TECC_NUMERIC_SIGN_NAN_COMMON       3
#define TECC_NUMERIC_SIGN_INF_COMMON       4
#define TECC_NUMERIC_SIGN_NINF_COMMON      5

/*
 * Common numeric normalization functions
 */

/* Normalize numeric text string to common format */
extern int tecc_normalize_numeric_text_common(const char *numeric_text,
                                              size_t text_len,
                                              TeccNumericNormalizedCommon *norm_num);

/* Encode normalized numeric structure to byte stream */
extern int tecc_encode_numeric_normalized_common(TeccNumericNormalizedCommon *norm_num,
                                                 unsigned char **output_data,
                                                 size_t *output_len);

/* Encode normalized numeric structure to existing buffer */
extern int tecc_encode_numeric_normalized_to_buffer_common(TeccNumericNormalizedCommon *norm_num,
                                                           unsigned char *buffer,
                                                           size_t buffer_size,
                                                           size_t *output_len);

/* Free normalized numeric structure */
extern void tecc_free_numeric_normalized_common(TeccNumericNormalizedCommon *norm_num);

/*
 * Common numeric type normalization functions
 */

/* Normalize int2 text string to common format */
extern int tecc_normalize_int2_text_common(const char *int2_text,
                                           size_t text_len,
                                           TeccNumericNormalizedCommon *norm_num);

/* Normalize int4 text string to common format */
extern int tecc_normalize_int4_text_common(const char *int4_text,
                                           size_t text_len,
                                           TeccNumericNormalizedCommon *norm_num);

/* Normalize int8 text string to common format */
extern int tecc_normalize_int8_text_common(const char *int8_text,
                                           size_t text_len,
                                           TeccNumericNormalizedCommon *norm_num);

/* Normalize float4 text string to common format */
extern int tecc_normalize_float4_text_common(const char *float4_text,
                                             size_t text_len,
                                             TeccNumericNormalizedCommon *norm_num);

/* Normalize float8 text string to common format */
extern int tecc_normalize_float8_text_common(const char *float8_text,
                                             size_t text_len,
                                             TeccNumericNormalizedCommon *norm_num);

/* Normalize numeric text string to common format */
extern int tecc_normalize_numeric_text_common(const char *numeric_text,
                                              size_t text_len,
                                              TeccNumericNormalizedCommon *norm_num);

/*
 * Common text normalization functions
 */

/* Normalize text with ICU sort key generation */
extern int tecc_normalize_text_with_icu_common(const char *collation_locale,
                                               const char *text,
                                               size_t text_len,
                                               unsigned char **output_data,
                                               size_t *output_len);

/* Normalize text with ICU sort key to existing buffer */
extern int tecc_normalize_text_with_icu_to_buffer_common(const char *collation_locale,
                                                         const char *text,
                                                         size_t text_len,
                                                         unsigned char *buffer,
                                                         size_t buffer_size,
                                                         size_t *output_len);

/* Normalize text without ICU (simple length prefix) */
extern int tecc_normalize_text_simple_common(const char *text,
                                             size_t text_len,
                                             unsigned char **output_data,
                                             size_t *output_len);

/* Normalize text without ICU to existing buffer */
extern int tecc_normalize_text_simple_to_buffer_common(const char *text,
                                                       size_t text_len,
                                                       unsigned char *buffer,
                                                       size_t buffer_size,
                                                       size_t *output_len);

#endif /* TECC_NORMALIZATION_COMMON_H */
/* End - ReqID:TECC4.0 */