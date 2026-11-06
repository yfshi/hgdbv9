/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_search_sm4.h
 *
 * Abstract:
 *   SEARCH algorithm header for TECC algorithm module.
 *   Provides searchable encryption using SM3 hash for character data.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/common/tecc_algorithm/tecc_search_sm4.h
 */

#ifndef TECC_SEARCH_SM4_H
#define TECC_SEARCH_SM4_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include "../../../../src/common/tecc_types.h"

/* For context-parameter functions, include algorithm API in implementation files */

/* SEARCH algorithm error codes */
typedef enum
{
    SM4_SEARCH_SUCCESS = 0,
    SM4_SEARCH_ERROR_INVALID_PARAM = -1,
    SM4_SEARCH_ERROR_MEMORY_ALLOC = -2,
    SM4_SEARCH_ERROR_OPENSSL = -3,
    SM4_SEARCH_ERROR_INVALID_FORMAT = -4,
    SM4_SEARCH_ERROR_DECRYPT_FAILED = -5,
    SM4_SEARCH_ERROR_DATA_TOO_LARGE = -6,
    SM4_SEARCH_ERROR_DECRYPT_NOT_SUPPORTED = -7,
    SM4_SEARCH_ERROR_DATA_TYPE_ERROR = -8,
    SM4_SEARCH_ERROR_KEY_DERIVATION = -9
} Sm4SearchErrorCode;

/*
 * SEARCH encryption interface - 零拷贝版本，调用者提供预分配的输出缓冲区
 */
int tecc_search_sm4_encrypt(const unsigned char *key, size_t key_len,
                           const unsigned char *plaintext, size_t plain_len,
                           unsigned char *output_buffer, size_t buffer_size,
                           size_t *output_len,
                           bool is_wildcard_pattern);

/*
 * SEARCH decryption interface - NOT SUPPORTED
 */
int tecc_search_sm4_decrypt(const unsigned char *key, size_t key_len,
                           const unsigned char *ciphertext, size_t cipher_len,
                           unsigned char *output_buffer, size_t buffer_size,
                           size_t *output_len);

/*
 * Data format validation interface
 */
int tecc_search_sm4_validate_format(const unsigned char *ciphertext, size_t cipher_len);

/*
 * Get SEARCH output buffer size
 */
size_t tecc_search_sm4_get_format_size(size_t plain_len);


/*
 * Clear sensitive memory
 */
void tecc_search_sm4_clear_memory(void *data, size_t len);


/* Calculate required buffer size for encryption */
size_t tecc_search_sm4_calc_encrypt_buffer_size(size_t plain_len);

/* Calculate required buffer size for decryption */
size_t tecc_search_sm4_calc_decrypt_buffer_size(size_t cipher_len);

/* =========================================================== */
/* Wildcard-preserving pattern matching functions              */
/* =========================================================== */

/* Pattern segment types */
typedef enum
{
    SM4_SEARCH_SEGMENT_WILDCARD_PERCENT,    /* '%' - zero or more characters */
    SM4_SEARCH_SEGMENT_WILDCARD_UNDERSCORE, /* '_' - exactly one character */
    SM4_SEARCH_SEGMENT_FIXED_STRING         /* Fixed string to be encrypted */
} Sm4SearchSegmentType;

/* Pattern segment structure */
typedef struct Sm4SearchPatternSegment
{
    Sm4SearchSegmentType type;           /* Segment type */
    size_t start_pos;                    /* Start position in original pattern */
    size_t length;                       /* Segment length */
    unsigned char *data;                 /* For fixed strings: plaintext data */
    unsigned char *encrypted_data;       /* For fixed strings: encrypted data */
    size_t encrypted_len;                /* Length of encrypted data */
} Sm4SearchPatternSegment;

/* Parsed pattern structure */
typedef struct Sm4SearchParsedPattern
{
    Sm4SearchPatternSegment *segments;   /* Array of pattern segments */
    size_t segment_count;                /* Number of segments */
    size_t max_segments;                 /* Maximum segments allocated */
    size_t original_length;              /* Original pattern length */
    char *original_pattern;              /* Original pattern string */
} Sm4SearchParsedPattern;

/* Pattern matching context */
typedef struct Sm4SearchMatchContext
{
    Sm4SearchParsedPattern *pattern;     /* Parsed pattern to match against */
    const unsigned char *encrypted_data; /* Encrypted data to search in */
    size_t encrypted_len;                /* Length of encrypted data */
    const unsigned char *key;            /* Encryption key */
    size_t key_len;                      /* Key length */
    bool case_sensitive;                 /* Case sensitivity flag */
} Sm4SearchMatchContext;

/* Pattern parsing error codes */
typedef enum
{
    SM4_SEARCH_PATTERN_SUCCESS = 0,
    SM4_SEARCH_PATTERN_ERROR_INVALID_PARAM = -100,
    SM4_SEARCH_PATTERN_ERROR_MEMORY_ALLOC = -101,
    SM4_SEARCH_PATTERN_ERROR_INVALID_PATTERN = -102,
    SM4_SEARCH_PATTERN_ERROR_ENCRYPTION_FAILED = -103,
    SM4_SEARCH_PATTERN_ERROR_TOO_MANY_SEGMENTS = -104
} Sm4SearchPatternErrorCode;

/*
 * Parse LIKE pattern into segments with wildcard preservation
 *
 * Parameters:
 *   pattern - LIKE pattern string (e.g., "%hello%world%")
 *   pattern_len - Length of pattern string
 *   parsed_pattern - Output parsed pattern structure
 *
 * Returns:
 *   SM4_SEARCH_PATTERN_SUCCESS on success, error code on failure
 */
int tecc_search_sm4_parse_pattern(const char *pattern, size_t pattern_len,
                                 Sm4SearchParsedPattern **parsed_pattern);

/*
 * Encrypt fixed string segments in parsed pattern
 *
 * Parameters:
 *   parsed_pattern - Parsed pattern with fixed string segments
 *   key - Encryption key
 *   key_len - Key length (must be 16 bytes for SM4)
 *
 * Returns:
 *   SM4_SEARCH_PATTERN_SUCCESS on success, error code on failure
 */
int tecc_search_sm4_encrypt_pattern_segments(Sm4SearchParsedPattern *parsed_pattern,
                                            const unsigned char *key, size_t key_len);


/*
 * Create pattern matching context
 *
 * Parameters:
 *   parsed_pattern - Parsed pattern (must have encrypted segments)
 *   encrypted_data - Encrypted data to search in
 *   encrypted_len - Length of encrypted data
 *   key - Encryption key (same as used for pattern encryption)
 *   key_len - Key length
 *   case_sensitive - Case sensitivity flag
 *
 * Returns:
 *   Sm4SearchMatchContext pointer on success, NULL on failure
 */
Sm4SearchMatchContext *tecc_search_sm4_create_match_context(
    Sm4SearchParsedPattern *parsed_pattern,
    const unsigned char *encrypted_data, size_t encrypted_len,
    const unsigned char *key, size_t key_len,
    bool case_sensitive);

/*
 * Free pattern matching context
 */
void tecc_search_sm4_free_match_context(Sm4SearchMatchContext *context);

/*
 * Free parsed pattern and all its resources
 */
void tecc_search_sm4_free_parsed_pattern(Sm4SearchParsedPattern *parsed_pattern);


/*
 * Check if pattern contains wildcards
 *
 * Parameters:
 *   pattern - Pattern string
 *   pattern_len - Pattern length
 *
 * Returns:
 *   true if pattern contains '%' or '_', false otherwise
 */
bool tecc_search_sm4_pattern_has_wildcards(const char *pattern, size_t pattern_len);

/*
 * Check if pattern is a simple prefix match (e.g., "hello%")
 *
 * Parameters:
 *   pattern - Pattern string
 *   pattern_len - Pattern length
 *
 * Returns:
 *   true if pattern is simple prefix match, false otherwise
 */
bool tecc_search_sm4_is_simple_prefix_pattern(const char *pattern, size_t pattern_len);

/*
 * Check if pattern is a simple suffix match (e.g., "%hello")
 *
 * Parameters:
 *   pattern - Pattern string
 *   pattern_len - Pattern length
 *
 * Returns:
 *   true if pattern is simple suffix match, false otherwise
 */
bool tecc_search_sm4_is_simple_suffix_pattern(const char *pattern, size_t pattern_len);

/*
 * Check if pattern is a simple substring match (e.g., "%hello%")
 *
 * Parameters:
 *   pattern - Pattern string
 *   pattern_len - Pattern length
 *
 * Returns:
 *   true if pattern is simple substring match, false otherwise
 */
bool tecc_search_sm4_is_simple_substring_pattern(const char *pattern, size_t pattern_len);


#endif /* TECC_SEARCH_SM4_H */
/* End - ReqID:TECC4.0 */