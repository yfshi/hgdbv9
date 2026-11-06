/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_formatter_utils.h
 *
 * Abstract:
 *   Utility interfaces for TECC data formatter module.
 *   Provides common utility functions and helpers.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_formatter_utils.h
 */

#ifndef TECC_FORMATTER_UTILS_H
#define TECC_FORMATTER_UTILS_H

#include "tecc_formatter.h"
#include "tecc_formatter_internal.h"

/*
 * Safe memory allocation wrapper
 */
extern void *tecc_formatter_utils_malloc(size_t size);
extern void *tecc_formatter_utils_calloc(size_t count, size_t size);
extern void *tecc_formatter_utils_realloc(void *ptr, size_t size);
extern char *tecc_formatter_utils_strdup(const char *str);
extern void tecc_formatter_utils_free(void *ptr);

/*
 * Safe string operations
 */
extern size_t tecc_formatter_utils_strlcpy(char *dst, const char *src, size_t size);
extern size_t tecc_formatter_utils_strlcat(char *dst, const char *src, size_t size);
extern int tecc_formatter_utils_strcasecmp(const char *s1, const char *s2);
extern int tecc_formatter_utils_strncasecmp(const char *s1, const char *s2, size_t n);

/*
 * Data validation helpers
 */
extern bool tecc_formatter_utils_is_valid_oid(Oid oid);
extern bool tecc_formatter_utils_is_valid_algorithm(TeccAlgorithmType algo);
extern bool tecc_formatter_utils_is_valid_data_type(Oid data_type);

/*
 * Memory alignment helpers
 */
extern void *tecc_formatter_utils_align_pointer(void *ptr, size_t alignment);
extern size_t tecc_formatter_utils_align_size(size_t size, size_t alignment);

/*
 * Error handling helpers
 */
extern void tecc_formatter_utils_set_error(TeccCryptoContextCore *context,
                                           TeccFormatterErrorCode error_code,
                                           const char *format, ...);
extern const char *tecc_formatter_utils_error_to_string(TeccFormatterErrorCode error);

/*
 * Numeric text parsing helpers
 */
extern TeccFormatterErrorCode tecc_formatter_parse_numeric_text(const char *numeric_text,
                                                                size_t text_len,
                                                                uint8_t *sign,
                                                                int16_t *weight,
                                                                uint16_t *digits,
                                                                int *ndigits);
extern TeccFormatterErrorCode tecc_formatter_encode_numeric_normalized(TeccNumericNormalized *norm_num,
                                                                       unsigned char **output_data,
                                                                       size_t *output_len);
extern TeccFormatterErrorCode tecc_formatter_validate_numeric_range(int16_t weight, int ndigits);

#endif /* TECC_FORMATTER_UTILS_H */