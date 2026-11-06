/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_search_sm4.c
 *
 * Abstract:
 *   SEARCH algorithm implementation - Searchable encryption using SM3 hash for character data.
 *   Provides character-level encryption for LIKE queries on encrypted text.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/common/tecc_algorithm/tecc_search_sm4.c
 */

#include "../../../src/common/tecc_types.h"
#include "../../include/common/tecc_algorithm/tecc_search_sm4.h"
#include "../../include/common/tecc_algorithm/tecc_algorithm_api.h"
#include "../../include/common/tecc_algorithm/tecc_byteorder.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../../include/common/tecc_algorithm/tecc_sm3_internal.h"

/* SEARCH key size (bytes) */
#define SM4_SEARCH_KEY_SIZE 16

/* SEARCH maximum data length (bytes) */
#define SM4_SEARCH_MAX_DATA_LEN 65535

/* Character hash output size (3 bytes per character) */
#define CHAR_HASH_SIZE 3

/* Note: SEARCH_MIN_SIZE is defined in tecc_types.h */

/* Memory allocation wrappers for PostgreSQL independence */
static void *tecc_search_sm4_alloc(size_t size) { return malloc(size); }
static void tecc_search_sm4_free(void *ptr) { free(ptr); }

/* Byte order conversion functions - use unified functions from tecc_byteorder.h */
/* Note: tecc_hton32() and tecc_ntoh32() are now available from tecc_byteorder.h */

/*
 * SM3 hash function wrapper
 */
static int
sm3_hash_bytes(const unsigned char *input, size_t input_len,
               const unsigned char *key, size_t key_len,
               const unsigned char *pos_bytes, size_t pos_bytes_len,
               unsigned char *output, size_t output_len)
{
    TeccSM3Context *ctx = NULL;
    unsigned char hash_result[TECC_SM3_DIGEST_LENGTH];
    int result = 0;

    if (!input || input_len == 0 || !key || key_len == 0 || !output || output_len == 0)
        return 0;

    if (output_len > TECC_SM3_DIGEST_LENGTH)
        return 0;

    ctx = tecc_sm3_context_new();
    if (!ctx)
        return 0;

    /* Hash: SM3(input || key) */
    if (tecc_sm3_update(ctx, input, input_len) != 1)
        goto cleanup;

    if (tecc_sm3_update(ctx, key, key_len) != 1)
        goto cleanup;

    if (tecc_sm3_final(ctx, hash_result) != 1)
        goto cleanup;

    memcpy(output, hash_result, output_len);
    result = 1;

cleanup:
    if (ctx)
        tecc_sm3_context_free(ctx);
    return result;
}



/*
 * Validate SEARCH data format
 */
int
tecc_search_sm4_validate_format(const unsigned char *ciphertext, size_t cipher_len)
{
    uint32_t data_len;
    size_t expected_len;

    /* Parameter validation */
    if (!ciphertext || cipher_len == 0)
    {
        return SM4_SEARCH_ERROR_INVALID_PARAM;
    }

    /* Minimum length check */
    if (cipher_len < sizeof(uint32_t))
    {
        return SM4_SEARCH_ERROR_INVALID_FORMAT;
    }

    /* Extract data length */
    memcpy(&data_len, ciphertext, sizeof(uint32_t));
    data_len = tecc_ntoh32(data_len);

    /* Calculate expected length */
    expected_len = sizeof(uint32_t) + data_len * CHAR_HASH_SIZE;

    /* Validate length */
    if (cipher_len != expected_len)
    {
        return SM4_SEARCH_ERROR_INVALID_FORMAT;
    }

    /* Check data length limit */
    if (data_len > SM4_SEARCH_MAX_DATA_LEN)
    {
        return SM4_SEARCH_ERROR_DATA_TOO_LARGE;
    }

    return SM4_SEARCH_SUCCESS;
}

/*
 * Get SEARCH output buffer size
 * This is a legacy function, use tecc_search_sm4_calc_encrypt_buffer_size instead
 */
size_t
tecc_search_sm4_get_format_size(size_t plain_len)
{
    return tecc_search_sm4_calc_encrypt_buffer_size(plain_len);
}

/*
 * Clear sensitive memory
 */
void
tecc_search_sm4_clear_memory(void *data, size_t len)
{
    /* Simple secure cleanup implementation */
    if (data && len > 0)
    {
        volatile unsigned char *p = data;
        size_t i;
        for (i = 0; i < len; i++)
            p[i] = 0;
    }
}





/*
 * SEARCH encryption - zero-copy version, caller provides pre-allocated buffer
 */
int
tecc_search_sm4_encrypt(const unsigned char *key, size_t key_len,
                       const unsigned char *plaintext, size_t plain_len,
                       unsigned char *output_buffer, size_t buffer_size,
                       size_t *output_len,
                       bool is_wildcard_pattern)
{
    int result = SM4_SEARCH_SUCCESS;
    size_t total_len;
    size_t pos;
    size_t out_pos = 0;

    /* Parameter validation */
    if (!key || key_len != SM4_SEARCH_KEY_SIZE || !plaintext ||
        !output_buffer || !output_len)
    {
        return SM4_SEARCH_ERROR_INVALID_PARAM;
    }

    /* Check data length */
    if (plain_len > SM4_SEARCH_MAX_DATA_LEN)
    {
        return SM4_SEARCH_ERROR_DATA_TOO_LARGE;
    }

    /* ReqID:TECC4.0 - Calculate max total length: version + data_len + per-char
     * output.  Wildcard chars (% and _) output as 1 byte.  Other chars output
     * as 3-byte hash, but each hash byte may need escaping (prefix '\\') if it
     * collides with '%' (0x25), '_' (0x5F) or '\\' (0x5C).  Worst case is
     * 3 * 2 = 6 bytes per non-wildcard char. */
    {
        size_t data_out_len_max = 0;
        for (pos = 0; pos < plain_len; pos++)
        {
            if (is_wildcard_pattern && (plaintext[pos] == '%' || plaintext[pos] == '_'))
                data_out_len_max += 1;
            else
                data_out_len_max += CHAR_HASH_SIZE * 2;
        }
        total_len = sizeof(uint32_t) + data_out_len_max;
    }

    /* Check if output buffer is large enough */
    if (buffer_size < total_len)
    {
        return SM4_SEARCH_ERROR_MEMORY_ALLOC;
    }

    /* Initialize output length */
    *output_len = 0;

    /* Set header: data length */
    {
        uint32_t data_len = tecc_hton32((uint32_t)plain_len);
        memcpy(output_buffer, &data_len, sizeof(uint32_t));
    }

    /* Process each character */
    {
        out_pos = 0;
        for (pos = 0; pos < plain_len; pos++)
        {
            /* ReqID:TECC4.0 - Wildcards (% and _) only for pattern; data chars always hashed */
            if (is_wildcard_pattern && (plaintext[pos] == '%' || plaintext[pos] == '_'))
            {
                output_buffer[sizeof(uint32_t) + out_pos] = plaintext[pos];
                out_pos += 1;
                /* ReqID:TECC4.0 - Wildcard output */
            }
            else
            {
                unsigned char char_hash[CHAR_HASH_SIZE];
                unsigned char pos_bytes[4];

                pos_bytes[0] = (pos >> 24) & 0xFF;
                pos_bytes[1] = (pos >> 16) & 0xFF;
                pos_bytes[2] = (pos >> 8) & 0xFF;
                pos_bytes[3] = (pos >> 0) & 0xFF;

                if (!sm3_hash_bytes(&plaintext[pos], 1, key, key_len, pos_bytes, 4, char_hash, CHAR_HASH_SIZE))
                {
                    result = SM4_SEARCH_ERROR_OPENSSL;
                    goto cleanup;
                }

                /* ReqID:TECC4.0 - Char hash output with escaping.
                 * SM3 hash bytes may randomly equal '%' (0x25), '_' (0x5F)
                 * or '\\' (0x5C).  These collide with LIKE wildcards and
                 * the escape character.  Escape them by prefixing with
                 * '\\' so the server-side unescape can recover the real
                 * byte values, leaving only intentional wildcards for
                 * bytealike to match against. */
                {
                    int h;
                    for (h = 0; h < CHAR_HASH_SIZE; h++)
                    {
                        unsigned char b = char_hash[h];
                        if (b == '%' || b == '_' || b == '\\')
                        {
                            output_buffer[sizeof(uint32_t) + out_pos] = '\\';
                            out_pos++;
                        }
                        output_buffer[sizeof(uint32_t) + out_pos] = b;
                        out_pos++;
                    }
                }
            }
        }
    }

    /* Success: set output length (use actual, not max) */
    *output_len = sizeof(uint32_t) + out_pos;

cleanup:
    /* If error occurred, clear output buffer */
    if (result != SM4_SEARCH_SUCCESS && output_buffer)
    {
        tecc_search_sm4_clear_memory(output_buffer, buffer_size);
    }

    return result;
}

/*
 * SEARCH decryption - not supported for SEARCH
 */
int
tecc_search_sm4_decrypt(const unsigned char *key, size_t key_len,
                       const unsigned char *ciphertext, size_t cipher_len,
                       unsigned char *output_buffer, size_t buffer_size,
                       size_t *output_len)
{
    /* SEARCH algorithm does not support decryption */
    return SM4_SEARCH_ERROR_DECRYPT_NOT_SUPPORTED;
}

/*
 * Calculate required buffer size for encryption
 */
size_t
tecc_search_sm4_calc_encrypt_buffer_size(size_t plain_len)
{
    /* Base format size: version + data length + character hashes.
     * ReqID:TECC4.0 - Worst case: all chars are non-wildcard and
     * all hash bytes need escaping (6 bytes per char). */
    if (plain_len > SM4_SEARCH_MAX_DATA_LEN)
    {
        return 0; /* Invalid size */
    }

    return sizeof(uint32_t) + plain_len * CHAR_HASH_SIZE * 2;
}

/*
 * Calculate required buffer size for decryption
 */
size_t
tecc_search_sm4_calc_decrypt_buffer_size(size_t cipher_len)
{
    /* SEARCH algorithm does not support decryption */
    return 0;
}

/* =========================================================== */
/* Wildcard-preserving pattern matching functions              */
/* =========================================================== */

/*
 * Parse LIKE pattern into segments with wildcard preservation
 */
int
tecc_search_sm4_parse_pattern(const char *pattern, size_t pattern_len,
                             Sm4SearchParsedPattern **parsed_pattern)
{
    Sm4SearchParsedPattern *result;
    size_t i, segment_start;
    bool in_fixed_string;

    /* Parameter validation */
    if (!pattern || !parsed_pattern)
    {
        return SM4_SEARCH_PATTERN_ERROR_INVALID_PARAM;
    }

    /* Allocate parsed pattern structure */
    result = tecc_search_sm4_alloc(sizeof(Sm4SearchParsedPattern));
    if (!result)
    {
        return SM4_SEARCH_PATTERN_ERROR_MEMORY_ALLOC;
    }

    /* Initialize structure */
    memset(result, 0, sizeof(Sm4SearchParsedPattern));
    result->original_length = pattern_len;
    result->original_pattern = tecc_search_sm4_alloc(pattern_len + 1);
    if (!result->original_pattern)
    {
        tecc_search_sm4_free(result);
        return SM4_SEARCH_PATTERN_ERROR_MEMORY_ALLOC;
    }
    memcpy(result->original_pattern, pattern, pattern_len);
    result->original_pattern[pattern_len] = '\0';

    /* Allocate initial segment array */
    result->max_segments = 16;  /* Initial capacity */
    result->segments = tecc_search_sm4_alloc(result->max_segments * sizeof(Sm4SearchPatternSegment));
    if (!result->segments)
    {
        tecc_search_sm4_free(result->original_pattern);
        tecc_search_sm4_free(result);
        return SM4_SEARCH_PATTERN_ERROR_MEMORY_ALLOC;
    }

    /* Parse pattern */
    segment_start = 0;
    in_fixed_string = false;

    for (i = 0; i <= pattern_len; i++)
    {
        char current_char = (i < pattern_len) ? pattern[i] : '\0';
        bool is_wildcard = (current_char == '%' || current_char == '_');

        /* State transition */
        if (is_wildcard && in_fixed_string)
        {
            /* End of fixed string segment */
            if (result->segment_count >= result->max_segments)
            {
                /* Resize segment array */
                size_t new_max = result->max_segments * 2;
                Sm4SearchPatternSegment *new_segments = tecc_search_sm4_alloc(
                    new_max * sizeof(Sm4SearchPatternSegment));
                if (!new_segments)
                {
                    tecc_search_sm4_free_parsed_pattern(result);
                    return SM4_SEARCH_PATTERN_ERROR_MEMORY_ALLOC;
                }
                memcpy(new_segments, result->segments,
                       result->segment_count * sizeof(Sm4SearchPatternSegment));
                tecc_search_sm4_free(result->segments);
                result->segments = new_segments;
                result->max_segments = new_max;
            }

            /* Add fixed string segment */
            {
                Sm4SearchPatternSegment *segment = &result->segments[result->segment_count];
                segment->type = SM4_SEARCH_SEGMENT_FIXED_STRING;
                segment->start_pos = segment_start;
                segment->length = i - segment_start;
                segment->data = tecc_search_sm4_alloc(segment->length);
                if (!segment->data)
                {
                    tecc_search_sm4_free_parsed_pattern(result);
                    return SM4_SEARCH_PATTERN_ERROR_MEMORY_ALLOC;
                }
                memcpy(segment->data, pattern + segment_start, segment->length);
                segment->encrypted_data = NULL;
                segment->encrypted_len = 0;
                result->segment_count++;
            }

            in_fixed_string = false;
        }
        else if (!is_wildcard && !in_fixed_string && current_char != '\0')
        {
            /* Start of fixed string segment */
            segment_start = i;
            in_fixed_string = true;
        }

        /* Add wildcard segment */
        if (is_wildcard)
        {
            if (result->segment_count >= result->max_segments)
            {
                /* Resize segment array */
                size_t new_max = result->max_segments * 2;
                Sm4SearchPatternSegment *new_segments = tecc_search_sm4_alloc(
                    new_max * sizeof(Sm4SearchPatternSegment));
                if (!new_segments)
                {
                    tecc_search_sm4_free_parsed_pattern(result);
                    return SM4_SEARCH_PATTERN_ERROR_MEMORY_ALLOC;
                }
                memcpy(new_segments, result->segments,
                       result->segment_count * sizeof(Sm4SearchPatternSegment));
                tecc_search_sm4_free(result->segments);
                result->segments = new_segments;
                result->max_segments = new_max;
            }

            /* Add wildcard segment */
            {
                Sm4SearchPatternSegment *segment = &result->segments[result->segment_count];
                segment->type = (current_char == '%') ?
                    SM4_SEARCH_SEGMENT_WILDCARD_PERCENT :
                    SM4_SEARCH_SEGMENT_WILDCARD_UNDERSCORE;
                segment->start_pos = i;
                segment->length = 1;
                segment->data = NULL;
                segment->encrypted_data = NULL;
                segment->encrypted_len = 0;
                result->segment_count++;
            }
        }
    }

    /* Add final fixed string segment if needed */
    if (in_fixed_string)
    {
        if (result->segment_count >= result->max_segments)
        {
            /* Resize segment array */
            size_t new_max = result->max_segments * 2;
            Sm4SearchPatternSegment *new_segments = tecc_search_sm4_alloc(
                new_max * sizeof(Sm4SearchPatternSegment));
            if (!new_segments)
            {
                tecc_search_sm4_free_parsed_pattern(result);
                return SM4_SEARCH_PATTERN_ERROR_MEMORY_ALLOC;
            }
            memcpy(new_segments, result->segments,
                   result->segment_count * sizeof(Sm4SearchPatternSegment));
            tecc_search_sm4_free(result->segments);
            result->segments = new_segments;
            result->max_segments = new_max;
        }

        /* Add fixed string segment */
        {
            Sm4SearchPatternSegment *segment = &result->segments[result->segment_count];
            segment->type = SM4_SEARCH_SEGMENT_FIXED_STRING;
            segment->start_pos = segment_start;
            segment->length = pattern_len - segment_start;
            segment->data = tecc_search_sm4_alloc(segment->length);
            if (!segment->data)
            {
                tecc_search_sm4_free_parsed_pattern(result);
                return SM4_SEARCH_PATTERN_ERROR_MEMORY_ALLOC;
            }
            memcpy(segment->data, pattern + segment_start, segment->length);
            segment->encrypted_data = NULL;
            segment->encrypted_len = 0;
            result->segment_count++;
        }
    }

    *parsed_pattern = result;
    return SM4_SEARCH_PATTERN_SUCCESS;
}

/*
 * Encrypt fixed string segments in parsed pattern
 */
int
tecc_search_sm4_encrypt_pattern_segments(Sm4SearchParsedPattern *parsed_pattern,
                                        const unsigned char *key, size_t key_len)
{
    size_t i;
    int result;

    /* Parameter validation */
    if (!parsed_pattern || !key || key_len != SM4_SEARCH_KEY_SIZE)
    {
        return SM4_SEARCH_PATTERN_ERROR_INVALID_PARAM;
    }

    /* Encrypt each fixed string segment */
    for (i = 0; i < parsed_pattern->segment_count; i++)
    {
        Sm4SearchPatternSegment *segment = &parsed_pattern->segments[i];

        if (segment->type == SM4_SEARCH_SEGMENT_FIXED_STRING && segment->data)
        {
            size_t encrypted_len;
            size_t buffer_size;

            /* Calculate required buffer size */
            buffer_size = tecc_search_sm4_calc_encrypt_buffer_size(segment->length);
            if (buffer_size == 0)
            {
                return SM4_SEARCH_PATTERN_ERROR_ENCRYPTION_FAILED;
            }

            /* Allocate encrypted data buffer */
            segment->encrypted_data = tecc_search_sm4_alloc(buffer_size);
            if (!segment->encrypted_data)
            {
                return SM4_SEARCH_PATTERN_ERROR_MEMORY_ALLOC;
            }

            /* Encrypt the fixed string (pattern segment, treat wildcards as wildcards) */
            result = tecc_search_sm4_encrypt(key, key_len,
                                           segment->data, segment->length,
                                           segment->encrypted_data, buffer_size,
                                           &encrypted_len, true);
            if (result != SM4_SEARCH_SUCCESS)
            {
                tecc_search_sm4_free(segment->encrypted_data);
                segment->encrypted_data = NULL;
                return SM4_SEARCH_PATTERN_ERROR_ENCRYPTION_FAILED;
            }

            segment->encrypted_len = encrypted_len;
        }
    }

    return SM4_SEARCH_PATTERN_SUCCESS;
}


/*
 * Create pattern matching context
 */
Sm4SearchMatchContext *
tecc_search_sm4_create_match_context(Sm4SearchParsedPattern *parsed_pattern,
                                    const unsigned char *encrypted_data, size_t encrypted_len,
                                    const unsigned char *key, size_t key_len,
                                    bool case_sensitive)
{
    Sm4SearchMatchContext *context;

    /* Parameter validation */
    if (!parsed_pattern || !encrypted_data || encrypted_len == 0 || !key || key_len != SM4_SEARCH_KEY_SIZE)
    {
        return NULL;
    }

    /* Allocate context */
    context = tecc_search_sm4_alloc(sizeof(Sm4SearchMatchContext));
    if (!context)
    {
        return NULL;
    }

    /* Initialize context */
    memset(context, 0, sizeof(Sm4SearchMatchContext));
    context->pattern = parsed_pattern;
    context->encrypted_data = encrypted_data;
    context->encrypted_len = encrypted_len;
    context->key = key;
    context->key_len = key_len;
    context->case_sensitive = case_sensitive;

    return context;
}

/*
 * Free pattern matching context
 */
void
tecc_search_sm4_free_match_context(Sm4SearchMatchContext *context)
{
    if (!context)
        return;

    /* Note: we don't free parsed_pattern or encrypted_data here */
    /* They are owned by the caller */

    tecc_search_sm4_clear_memory(context, sizeof(Sm4SearchMatchContext));
    tecc_search_sm4_free(context);
}

/*
 * Free parsed pattern and all its resources
 */
void
tecc_search_sm4_free_parsed_pattern(Sm4SearchParsedPattern *parsed_pattern)
{
    size_t i;

    if (!parsed_pattern)
        return;

    /* Free segments */
    if (parsed_pattern->segments)
    {
        for (i = 0; i < parsed_pattern->segment_count; i++)
        {
            Sm4SearchPatternSegment *segment = &parsed_pattern->segments[i];

            if (segment->data)
            {
                tecc_search_sm4_free(segment->data);
            }

            if (segment->encrypted_data)
            {
                tecc_search_sm4_free(segment->encrypted_data);
            }
        }

        tecc_search_sm4_free(parsed_pattern->segments);
    }

    /* Free original pattern */
    if (parsed_pattern->original_pattern)
    {
        tecc_search_sm4_free(parsed_pattern->original_pattern);
    }

    /* Free structure itself */
    tecc_search_sm4_clear_memory(parsed_pattern, sizeof(Sm4SearchParsedPattern));
    tecc_search_sm4_free(parsed_pattern);
}

/*
 * Check if pattern contains wildcards
 */
bool
tecc_search_sm4_pattern_has_wildcards(const char *pattern, size_t pattern_len)
{
    size_t i;

    if (!pattern || pattern_len == 0)
        return false;

    for (i = 0; i < pattern_len; i++)
    {
        if (pattern[i] == '%' || pattern[i] == '_')
            return true;
    }

    return false;
}

/*
 * Check if pattern is a simple prefix match (e.g., "hello%")
 */
bool
tecc_search_sm4_is_simple_prefix_pattern(const char *pattern, size_t pattern_len)
{
    size_t i;

    /* Pattern must end with '%' and not contain any other wildcards */
    if (!pattern || pattern_len < 2 || pattern[pattern_len - 1] != '%')
        return false;

    /* Check if there are any other wildcards before the last character */
    for (i = 0; i < pattern_len - 1; i++)
    {
        if (pattern[i] == '%' || pattern[i] == '_')
            return false;
    }

    return true;
}

/*
 * Check if pattern is a simple suffix match (e.g., "%hello")
 */
bool
tecc_search_sm4_is_simple_suffix_pattern(const char *pattern, size_t pattern_len)
{
    size_t i;

    /* Pattern must start with '%' and not contain any other wildcards */
    if (!pattern || pattern_len < 2 || pattern[0] != '%')
        return false;

    /* Check if there are any other wildcards after the first character */
    for (i = 1; i < pattern_len; i++)
    {
        if (pattern[i] == '%' || pattern[i] == '_')
            return false;
    }

    return true;
}

/*
 * Check if pattern is a simple substring match (e.g., "%hello%")
 */
bool
tecc_search_sm4_is_simple_substring_pattern(const char *pattern, size_t pattern_len)
{
    size_t percent_count;
    size_t i;

    /* Pattern must start and end with '%' and contain exactly two '%' characters */
    if (!pattern || pattern_len < 3 || pattern[0] != '%' || pattern[pattern_len - 1] != '%')
        return false;

    /* Count '%' characters */
    percent_count = 0;
    for (i = 0; i < pattern_len; i++)
    {
        if (pattern[i] == '%')
            percent_count++;
        else if (pattern[i] == '_')
            return false;  /* Contains '_' wildcard */
    }

    return (percent_count == 2);
}

/* End - ReqID:TECC4.0 */