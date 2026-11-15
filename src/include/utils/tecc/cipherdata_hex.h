/*-------------------------------------------------------------------------
 *
 * cipherdata_hex.h
 *	  Hexadecimal encoding/decoding utilities for CipherDataType
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/utils/cipherdata_hex.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef CIPHERDATA_HEX_H
#define CIPHERDATA_HEX_H

#include "c.h"
#include "utils/cipherdatatype.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Begin - ReqID:TECC4.0 */

/*
 * PostgreSQL hex string prefix
 */
#define HEX_PREFIX			"\\x"
#define HEX_PREFIX_LEN		2

/*
 * Validate hexadecimal string format
 *
 * Returns true if the string is a valid hexadecimal string (with or without prefix).
 * If the string has the PostgreSQL hex prefix ("\x"), it is stripped before validation.
 */
extern bool is_valid_hex_string(const char *str);

/*
 * Convert hexadecimal string to CipherDataType
 *
 * Converts a hexadecimal string (with optional "\x" prefix) to a CipherDataType.
 * The hex string should contain the complete binary representation including header.
 */
extern CipherDataType *hex_to_cipherdata(const char *hex_str);

/*
 * Convert binary data to hexadecimal string
 *
 * Converts binary data to a hexadecimal string with PostgreSQL prefix ("\x").
 * The caller is responsible for freeing the returned string.
 */
extern char *binary_to_hex(const unsigned char *data, size_t len);

/*
 * Convert hexadecimal string to binary data
 *
 * Converts a hexadecimal string (with optional "\x" prefix) to binary data.
 * The caller is responsible for freeing the returned buffer.
 */
extern unsigned char *hex_to_binary(const char *hex_str, size_t *len);

/*
 * Strip PostgreSQL hex prefix if present
 *
 * Returns a pointer to the first character after the prefix, or the original
 * string if no prefix is found.
 */
extern const char *strip_hex_prefix(const char *hex_str);

/*
 * Check if string has PostgreSQL hex prefix
 */
extern bool has_hex_prefix(const char *hex_str);

/* End - ReqID:TECC4.0 */

#ifdef __cplusplus
}
#endif

#endif	/* CIPHERDATA_HEX_H */