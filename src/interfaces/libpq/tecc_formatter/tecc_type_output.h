/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_type_output.h
 *
 * Abstract:
 *   Binary-to-text output conversion for TECC decrypted data.
 *   Converts PostgreSQL internal binary format to human-readable text strings.
 *   Reference: backend *_out functions (int2out, int4out, int8out, float4out,
 *   float8out, numeric_out, textout, bpcharout, varcharout).
 *
 * Authored by <wangxp@highgo.com>, 20260424.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_type_output.h
 */

#ifndef TECC_TYPE_OUTPUT_H
#define TECC_TYPE_OUTPUT_H

#include "postgres_fe.h"
#include <stddef.h>

/* ReqID:TECC4.0 - For TeccFormatterErrorCode type */
#include "../tecc_common/tecc_common_types.h"

/* OID constants for base data types */
#define TECC_OUTPUT_INT2OID     21
#define TECC_OUTPUT_INT4OID     23
#define TECC_OUTPUT_INT8OID     20
#define TECC_OUTPUT_FLOAT4OID   700
#define TECC_OUTPUT_FLOAT8OID   701
#define TECC_OUTPUT_NUMERICOID  1700
#define TECC_OUTPUT_VARCHAROID  1043
#define TECC_OUTPUT_BPCHAROID   1042
#define TECC_OUTPUT_TEXTOID     25

/*
 * tecc_type_output_data - Convert decrypted binary data to text string
 *
 * Converts PostgreSQL internal binary format to human-readable text,
 * based on the base data type OID. Reference: backend *_out functions.
 *
 * @param type_oid     Base data type OID (INT2OID=21, INT4OID=23, etc.)
 * @param type_mod     Type modifier (atttypmod) for precision/scale info
 * @param binary_data  Pointer to decrypted binary data
 * @param binary_len   Length of binary data in bytes
 *
 * @return pg_malloc'd null-terminated text string, or NULL on error
 *
 * Caller is responsible for pg_free()ing the returned string.
 */
extern char *tecc_type_output_data(Oid type_oid, int type_mod,
                                   const char *binary_data,
                                   size_t binary_len);

/*
 * Type-specific output functions (exposed for testing)
 */

/* int2: binary is 2-byte little-endian int16 */
extern char *tecc_type_output_int2(const char *data, size_t len);

/* int4: binary is 4-byte little-endian int32 */
extern char *tecc_type_output_int4(const char *data, size_t len);

/* int8: binary is 8-byte little-endian int64 */
extern char *tecc_type_output_int8(const char *data, size_t len);

/* float4: binary is 4-byte IEEE 754 float */
extern char *tecc_type_output_float4(const char *data, size_t len);

/* float8: binary is 8-byte IEEE 754 double */
extern char *tecc_type_output_float8(const char *data, size_t len);

/* numeric: binary is NumericData varlena structure */
extern char *tecc_type_output_numeric(const char *data, size_t len,
                                       int type_mod);

/* text/varchar/bpchar:
 *   RND:  4-byte network byte order length prefix + text data
 *   DET/SEARCH: raw text, no prefix */
extern char *tecc_type_output_text(const char *data, size_t len);

/*
 * tecc_type_input_numeric_from_text - Convert numeric text to NumericData varlena binary
 *
 * Converts a numeric text string (e.g., "50000.00") to PostgreSQL internal
 * NumericData varlena binary format suitable for RND encryption.
 *
 * @param text       Null-terminated numeric text string
 * @param text_len   Length of text string
 * @param binary_out Output: pg_malloc'd NumericData binary
 * @param binary_len Output: length of binary data
 * @return           0 on success, -1 on error
 */
extern int tecc_type_input_numeric_from_text(const char *text, size_t text_len,
                                              unsigned char **binary_out,
                                              size_t *binary_len);

/*
 * ReqID:TECC4.0 - tecc_formatter_prepare_plaintext
 *   Input validation + text to PG internal binary conversion.
 *
 * Converts SQL text representation to RND-ready plaintext format.
 * For numeric types: text -> PG internal binary (int16/int32/int64/float/double/NumericData).
 * For text types:   text -> 4-byte length prefix + text (RND format).
 *
 * This is the single entry point for pre-encryption data preparation,
 * shared by all algorithm types. DET/OPE/SEARCH normalization is applied
 * on top of this result when needed.
 *
 * @param data_type   Base data type OID (INT2OID=21, INT4OID=23, etc.)
 * @param type_mod    Type modifier (atttypmod), used for NUMERIC scale
 * @param text        Null-terminated input text from SQL layer
 * @param text_len    Length of text string
 * @param binary_out  Output: pg_malloc'd PG internal binary
 * @param binary_len  Output: length of binary data
 * @return            TECC_FORMATTER_SUCCESS on success, error code otherwise
 */
extern TeccFormatterErrorCode tecc_formatter_prepare_plaintext(
    Oid data_type, int type_mod,
    const char *text, size_t text_len,
    unsigned char **binary_out, size_t *binary_len);

#endif /* TECC_TYPE_OUTPUT_H */
/* End - ReqID:TECC4.0 */
