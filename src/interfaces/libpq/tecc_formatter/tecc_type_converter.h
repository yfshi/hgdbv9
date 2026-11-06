/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_type_converter.h
 *
 * Abstract:
 *   Type conversion interfaces for TECC data formatter module.
 *   Provides data type conversion services between different representations.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_type_converter.h
 */

#ifndef TECC_TYPE_CONVERTER_H
#define TECC_TYPE_CONVERTER_H

#include "tecc_formatter.h"
#include "tecc_formatter_internal.h"

/*
 * Convert text representation to binary format
 */
extern TeccFormatterErrorCode tecc_type_converter_text_to_binary(
    const char *text_data,
    size_t text_len,
    Oid data_type,
    unsigned char **binary_data,
    size_t *binary_len
);

/*
 * Convert binary format to text representation
 */
extern TeccFormatterErrorCode tecc_type_converter_binary_to_text(
    const unsigned char *binary_data,
    size_t binary_len,
    Oid data_type,
    char **text_data,
    size_t *text_len
);

/*
 * Get type information
 */
extern TeccFormatterErrorCode tecc_type_converter_get_type_info(
    Oid data_type,
    int *type_length,
    bool *type_by_val,
    char *type_align
);

/*
 * Check type compatibility
 */
extern bool tecc_type_converter_is_compatible(
    Oid src_type,
    Oid dst_type
);

#endif /* TECC_TYPE_CONVERTER_H */