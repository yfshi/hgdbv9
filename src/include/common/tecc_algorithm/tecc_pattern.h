/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_pattern.h
 *
 * Abstract:
 *   Unified pattern parsing and matching interface for TECC algorithm module.
 *   Provides complete LIKE pattern parsing with full ESCAPE semantics support.
 *
 * Authored by <wangxp@highgo.com>, 20260422.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/include/common/tecc_algorithm/tecc_pattern.h
 */

#ifndef TECC_PATTERN_H
#define TECC_PATTERN_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

/* Pattern segment types */
typedef enum TeccPatternSegmentType
{
    TECC_PATTERN_SEGMENT_FIXED_STRING = 0,      /* Fixed string segment */
    TECC_PATTERN_SEGMENT_WILDCARD_PERCENT,      /* '%' - zero or more characters */
    TECC_PATTERN_SEGMENT_WILDCARD_UNDERSCORE,   /* '_' - exactly one character */
    TECC_PATTERN_SEGMENT_ESCAPED_CHAR           /* Escaped character */
} TeccPatternSegmentType;

/* Pattern type classification */
typedef enum TeccPatternType
{
    TECC_PATTERN_UNKNOWN = 0,
    TECC_PATTERN_EXACT,          /* No wildcards */
    TECC_PATTERN_PREFIX,         /* 'abc%' */
    TECC_PATTERN_SUFFIX,         /* '%xyz' */
    TECC_PATTERN_INFIX,          /* '%middle%' */
    TECC_PATTERN_COMPLEX,        /* Complex pattern with multiple wildcards */
    TECC_PATTERN_WILDCARD        /* Contains '_' wildcard */
} TeccPatternType;

/* Pattern segment structure */
typedef struct TeccPatternSegment
{
    TeccPatternSegmentType type;          /* Segment type */
    size_t start_pos;                     /* Start position in original pattern */
    size_t length;                        /* Segment length */

    union {
        unsigned char *fixed_string;      /* For FIXED_STRING: plaintext data */
        char escaped_char;                /* For ESCAPED_CHAR: escaped character */
    } data;

    unsigned char *encrypted_data;        /* Encrypted data (for fixed strings) */
    size_t encrypted_len;                 /* Length of encrypted data */
} TeccPatternSegment;

/* Unified parsed pattern structure */
typedef struct TeccParsedPattern
{
    /* Pattern segments */
    TeccPatternSegment *segments;         /* Array of pattern segments */
    size_t segment_count;                 /* Number of segments */
    size_t max_segments;                  /* Maximum segments allocated */

    /* Original pattern information */
    size_t original_length;               /* Original pattern length */
    char *original_pattern;               /* Original pattern string */

    /* ESCAPE semantics */
    char escape_char;                     /* ESCAPE character (0 if no ESCAPE) */
    bool has_escape;                      /* Whether pattern has ESCAPE semantics */

    /* Pattern classification */
    TeccPatternType pattern_type;         /* Pattern type */
    bool is_simple_prefix;                /* Is simple prefix pattern (e.g., "abc%") */
    bool is_simple_suffix;                /* Is simple suffix pattern (e.g., "%xyz") */
    bool is_simple_substring;             /* Is simple substring pattern (e.g., "%middle%") */
    bool contains_underscore;             /* Contains '_' wildcard */
    bool contains_percent;                /* Contains '%' wildcard */
    int wildcard_count;                   /* Total number of wildcard characters */

    /* Performance optimization fields */
    unsigned char *optimized_encryption;  /* Optimized encryption for simple patterns */
    size_t optimized_len;                 /* Length of optimized encryption */
} TeccParsedPattern;

/* Pattern matching context */
typedef struct TeccPatternMatchContext
{
    TeccParsedPattern *pattern;           /* Parsed pattern to match against */
    const unsigned char *encrypted_data;  /* Encrypted data to search in */
    size_t encrypted_len;                 /* Length of encrypted data */
    const unsigned char *key;             /* Encryption key */
    size_t key_len;                       /* Key length */
    bool case_sensitive;                  /* Case sensitivity flag */

    /* Matching state */
    size_t current_position;              /* Current matching position */
    size_t *match_positions;              /* Segment match positions array */
    size_t match_position_count;          /* Number of match positions */
} TeccPatternMatchContext;

/* Pattern parsing error codes */
typedef enum TeccPatternErrorCode
{
    TECC_PATTERN_SUCCESS = 0,
    TECC_PATTERN_ERROR_INVALID_PARAM = -100,
    TECC_PATTERN_ERROR_MEMORY_ALLOC = -101,
    TECC_PATTERN_ERROR_INVALID_PATTERN = -102,
    TECC_PATTERN_ERROR_ENCRYPTION_FAILED = -103,
    TECC_PATTERN_ERROR_TOO_MANY_SEGMENTS = -104,
    TECC_PATTERN_ERROR_ESCAPE_INVALID = -105,
    TECC_PATTERN_ERROR_PATTERN_TOO_COMPLEX = -106
} TeccPatternErrorCode;

/*
 * Parse LIKE pattern with full ESCAPE semantics support
 *
 * Parameters:
 *   pattern - LIKE pattern string (e.g., "a%c", "a%b%c", "a_c")
 *   pattern_len - Length of pattern string
 *   escape_char - ESCAPE character (0 for no ESCAPE)
 *   parsed_pattern - Output parsed pattern structure
 *
 * Returns:
 *   TECC_PATTERN_SUCCESS on success, error code on failure
 */
TeccPatternErrorCode tecc_parse_pattern_full(
    const char *pattern,
    size_t pattern_len,
    char escape_char,
    TeccParsedPattern **parsed_pattern);

/*
 * Free parsed pattern and all its resources
 */
void tecc_free_parsed_pattern(TeccParsedPattern *parsed_pattern);

/*
 * Encrypt fixed string segments in parsed pattern
 *
 * Parameters:
 *   parsed_pattern - Parsed pattern with fixed string segments
 *   key - Encryption key
 *   key_len - Key length
 *
 * Returns:
 *   TECC_PATTERN_SUCCESS on success, error code on failure
 */
TeccPatternErrorCode tecc_encrypt_pattern_segments(
    TeccParsedPattern *parsed_pattern,
    const unsigned char *key,
    size_t key_len);

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
 *   TeccPatternMatchContext pointer on success, NULL on failure
 */
TeccPatternMatchContext *tecc_create_match_context(
    TeccParsedPattern *parsed_pattern,
    const unsigned char *encrypted_data,
    size_t encrypted_len,
    const unsigned char *key,
    size_t key_len,
    bool case_sensitive);

/*
 * Match encrypted data against encrypted pattern
 *
 * Parameters:
 *   context - Pattern matching context
 *   match_start - Output: position where match starts (if found)
 *   match_end - Output: position where match ends (if found)
 *
 * Returns:
 *   true if pattern matches, false otherwise
 */
bool tecc_match_pattern(
    TeccPatternMatchContext *context,
    size_t *match_start,
    size_t *match_end);

/*
 * Free pattern matching context
 */
void tecc_free_match_context(TeccPatternMatchContext *context);

/*
 * Check if pattern contains wildcards
 *
 * Parameters:
 *   pattern - Pattern string
 *   pattern_len - Pattern length
 *   escape_char - ESCAPE character (0 for no ESCAPE)
 *
 * Returns:
 *   true if pattern contains '%' or '_' (considering ESCAPE), false otherwise
 */
bool tecc_pattern_has_wildcards(
    const char *pattern,
    size_t pattern_len,
    char escape_char);

/*
 * Check if pattern is a simple prefix match (e.g., "hello%")
 *
 * Parameters:
 *   pattern - Pattern string
 *   pattern_len - Pattern length
 *   escape_char - ESCAPE character (0 for no ESCAPE)
 *
 * Returns:
 *   true if pattern is simple prefix match, false otherwise
 */
bool tecc_is_simple_prefix_pattern(
    const char *pattern,
    size_t pattern_len,
    char escape_char);

/*
 * Check if pattern is a simple suffix match (e.g., "%hello")
 *
 * Parameters:
 *   pattern - Pattern string
 *   pattern_len - Pattern length
 *   escape_char - ESCAPE character (0 for no ESCAPE)
 *
 * Returns:
 *   true if pattern is simple suffix match, false otherwise
 */
bool tecc_is_simple_suffix_pattern(
    const char *pattern,
    size_t pattern_len,
    char escape_char);

/*
 * Check if pattern is a simple substring match (e.g., "%hello%")
 *
 * Parameters:
 *   pattern - Pattern string
 *   pattern_len - Pattern length
 *   escape_char - ESCAPE character (0 for no ESCAPE)
 *
 * Returns:
 *   true if pattern is simple substring match, false otherwise
 */
bool tecc_is_simple_substring_pattern(
    const char *pattern,
    size_t pattern_len,
    char escape_char);

/*
 * Convert TeccParsedPattern to legacy TeccSearchPattern structure
 * (for backward compatibility with scheduler)
 *
 * Parameters:
 *   parsed_pattern - Unified parsed pattern
 *   legacy_pattern - Output legacy pattern structure
 *
 * Returns:
 *   TECC_PATTERN_SUCCESS on success, error code on failure
 */
TeccPatternErrorCode tecc_convert_to_legacy_pattern(
    const TeccParsedPattern *parsed_pattern,
    void *legacy_pattern);  /* Actual type: TeccSearchPattern* */

/*
 * Convert legacy TeccSearchPattern to unified TeccParsedPattern
 *
 * Parameters:
 *   legacy_pattern - Legacy pattern structure
 *   parsed_pattern - Output unified parsed pattern
 *
 * Returns:
 *   TECC_PATTERN_SUCCESS on success, error code on failure
 */
TeccPatternErrorCode tecc_convert_from_legacy_pattern(
    const void *legacy_pattern,  /* Actual type: const TeccSearchPattern* */
    TeccParsedPattern **parsed_pattern);

#endif /* TECC_PATTERN_H */
/* End - ReqID:TECC4.0 */