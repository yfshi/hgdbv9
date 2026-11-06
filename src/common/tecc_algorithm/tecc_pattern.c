/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_pattern.c
 *
 * Abstract:
 *   Unified pattern parsing and matching implementation for TECC algorithm module.
 *   Provides complete LIKE pattern parsing with full ESCAPE semantics support.
 *
 * Authored by <wangxp@highgo.com>, 20260422.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/common/tecc_algorithm/tecc_pattern.c
 */

#include "../../../src/common/tecc_types.h"
#include "../../include/common/tecc_algorithm/tecc_pattern.h"
#include "../../include/common/tecc_algorithm/tecc_search_sm4.h"
#include "../../include/common/tecc_algorithm/tecc_algorithm_api.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

/* Memory allocation wrappers */
static void *tecc_pattern_alloc(size_t size) { return malloc(size); }
static void tecc_pattern_free(void *ptr) { free(ptr); }

/* Default maximum segments for pattern parsing */
#define TECC_PATTERN_MAX_SEGMENTS 256
#define TECC_PATTERN_INITIAL_SEGMENTS 16

/* Internal helper functions */
static TeccPatternErrorCode tecc_pattern_add_segment(
    TeccParsedPattern *parsed_pattern,
    TeccPatternSegmentType type,
    size_t start_pos,
    size_t length,
    const unsigned char *data);

static void tecc_pattern_classify(TeccParsedPattern *parsed_pattern);

/*
 * tecc_parse_pattern_full - Parse LIKE pattern with full ESCAPE semantics
 */
TeccPatternErrorCode
tecc_parse_pattern_full(
    const char *pattern,
    size_t pattern_len,
    char escape_char,
    TeccParsedPattern **parsed_pattern)
{
    TeccParsedPattern *result = NULL;
    TeccPatternErrorCode ret = TECC_PATTERN_SUCCESS;
    size_t i;
    size_t segment_start = 0;
    bool in_fixed_string = false;
    bool last_was_escape = false;
    TeccPatternSegmentType wildcard_type;

    /* Validate parameters */
    if (!pattern || !parsed_pattern)
        return TECC_PATTERN_ERROR_INVALID_PARAM;

    if (pattern_len == 0)
        return TECC_PATTERN_ERROR_INVALID_PATTERN;

    /* Allocate parsed pattern structure */
    result = (TeccParsedPattern *)tecc_pattern_alloc(sizeof(TeccParsedPattern));
    if (!result)
        return TECC_PATTERN_ERROR_MEMORY_ALLOC;

    /* Initialize structure */
    memset(result, 0, sizeof(TeccParsedPattern));
    result->escape_char = escape_char;
    result->has_escape = (escape_char != 0);
    result->original_length = pattern_len;
    result->pattern_type = TECC_PATTERN_UNKNOWN;

    /* Allocate segment array */
    result->max_segments = TECC_PATTERN_INITIAL_SEGMENTS;
    result->segments = (TeccPatternSegment *)tecc_pattern_alloc(
        result->max_segments * sizeof(TeccPatternSegment));
    if (!result->segments)
    {
        tecc_pattern_free(result);
        return TECC_PATTERN_ERROR_MEMORY_ALLOC;
    }
    memset(result->segments, 0, result->max_segments * sizeof(TeccPatternSegment));

    /* Copy original pattern */
    result->original_pattern = (char *)tecc_pattern_alloc(pattern_len + 1);
    if (!result->original_pattern)
    {
        tecc_pattern_free(result->segments);
        tecc_pattern_free(result);
        return TECC_PATTERN_ERROR_MEMORY_ALLOC;
    }
    memcpy(result->original_pattern, pattern, pattern_len);
    result->original_pattern[pattern_len] = '\0';

    /* Parse pattern into segments */
    for (i = 0; i < pattern_len; i++)
    {
        char current_char = pattern[i];
        bool is_escaped = false;

        /* Check if current character is escaped */
        if (escape_char != 0 && !last_was_escape && current_char == escape_char)
        {
            /* This is an escape character */
            last_was_escape = true;

            /* If we were in a fixed string, end it */
            if (in_fixed_string && i > segment_start)
            {
                ret = tecc_pattern_add_segment(result,
                    TECC_PATTERN_SEGMENT_FIXED_STRING,
                    segment_start,
                    i - segment_start,
                    (const unsigned char *)(pattern + segment_start));
                if (ret != TECC_PATTERN_SUCCESS)
                    goto error;
                in_fixed_string = false;
            }

            continue;
        }

        if (last_was_escape)
        {
            /* Current character is escaped */
            is_escaped = true;
            last_was_escape = false;
        }

        if (is_escaped)
        {
            /* Add escaped character as a separate segment */
            ret = tecc_pattern_add_segment(result,
                TECC_PATTERN_SEGMENT_ESCAPED_CHAR,
                i,
                1,
                (const unsigned char *)&current_char);
            if (ret != TECC_PATTERN_SUCCESS)
                goto error;
        }
        else if (current_char == '%' || current_char == '_')
        {
            /* End current fixed string segment if any */
            if (in_fixed_string && i > segment_start)
            {
                ret = tecc_pattern_add_segment(result,
                    TECC_PATTERN_SEGMENT_FIXED_STRING,
                    segment_start,
                    i - segment_start,
                    (const unsigned char *)(pattern + segment_start));
                if (ret != TECC_PATTERN_SUCCESS)
                    goto error;
                in_fixed_string = false;
            }

            /* Add wildcard segment */
            if (current_char == '%')
                wildcard_type = TECC_PATTERN_SEGMENT_WILDCARD_PERCENT;
            else
                wildcard_type = TECC_PATTERN_SEGMENT_WILDCARD_UNDERSCORE;

            ret = tecc_pattern_add_segment(result,
                wildcard_type,
                i,
                1,
                NULL);
            if (ret != TECC_PATTERN_SUCCESS)
                goto error;

            /* Update wildcard counters */
            if (current_char == '%')
                result->contains_percent = true;
            else
                result->contains_underscore = true;
            result->wildcard_count++;
        }
        else
        {
            /* Regular character - start or continue fixed string */
            if (!in_fixed_string)
            {
                segment_start = i;
                in_fixed_string = true;
            }
        }
    }

    /* Handle last fixed string segment */
    if (in_fixed_string && pattern_len > segment_start)
    {
        ret = tecc_pattern_add_segment(result,
            TECC_PATTERN_SEGMENT_FIXED_STRING,
            segment_start,
            pattern_len - segment_start,
            (const unsigned char *)(pattern + segment_start));
        if (ret != TECC_PATTERN_SUCCESS)
            goto error;
    }

    /* Check for trailing escape character */
    if (last_was_escape)
    {
        ret = TECC_PATTERN_ERROR_ESCAPE_INVALID;
        goto error;
    }

    /* Classify pattern type */
    tecc_pattern_classify(result);

    *parsed_pattern = result;
    return TECC_PATTERN_SUCCESS;

error:
    tecc_free_parsed_pattern(result);
    return ret;
}

/*
 * tecc_pattern_add_segment - Add a segment to parsed pattern
 */
static TeccPatternErrorCode
tecc_pattern_add_segment(
    TeccParsedPattern *parsed_pattern,
    TeccPatternSegmentType type,
    size_t start_pos,
    size_t length,
    const unsigned char *data)
{
    TeccPatternSegment *segment;

    /* Check segment count */
    if (parsed_pattern->segment_count >= parsed_pattern->max_segments)
    {
        /* Resize segment array */
        size_t new_max = parsed_pattern->max_segments * 2;
        TeccPatternSegment *new_segments;

        if (new_max > TECC_PATTERN_MAX_SEGMENTS)
            return TECC_PATTERN_ERROR_TOO_MANY_SEGMENTS;

        new_segments = (TeccPatternSegment *)tecc_pattern_alloc(
            new_max * sizeof(TeccPatternSegment));
        if (!new_segments)
            return TECC_PATTERN_ERROR_MEMORY_ALLOC;

        memcpy(new_segments, parsed_pattern->segments,
               parsed_pattern->segment_count * sizeof(TeccPatternSegment));
        memset(new_segments + parsed_pattern->segment_count, 0,
               (new_max - parsed_pattern->segment_count) * sizeof(TeccPatternSegment));

        tecc_pattern_free(parsed_pattern->segments);
        parsed_pattern->segments = new_segments;
        parsed_pattern->max_segments = new_max;
    }

    /* Get segment pointer */
    segment = &parsed_pattern->segments[parsed_pattern->segment_count];

    /* Initialize segment */
    segment->type = type;
    segment->start_pos = start_pos;
    segment->length = length;
    segment->encrypted_data = NULL;
    segment->encrypted_len = 0;

    /* Set segment data based on type */
    if (type == TECC_PATTERN_SEGMENT_FIXED_STRING && data && length > 0)
    {
        segment->data.fixed_string = (unsigned char *)tecc_pattern_alloc(length);
        if (!segment->data.fixed_string)
            return TECC_PATTERN_ERROR_MEMORY_ALLOC;
        memcpy(segment->data.fixed_string, data, length);
    }
    else if (type == TECC_PATTERN_SEGMENT_ESCAPED_CHAR && data && length == 1)
    {
        segment->data.escaped_char = (char)data[0];
    }
    else
    {
        segment->data.fixed_string = NULL;
    }

    parsed_pattern->segment_count++;
    return TECC_PATTERN_SUCCESS;
}

/*
 * tecc_pattern_classify - Classify pattern type
 */
static void
tecc_pattern_classify(TeccParsedPattern *parsed_pattern)
{
    size_t i;
    bool has_percent = false;
    bool has_underscore = false;
    int percent_count = 0;

    /* Count wildcards and check types */
    for (i = 0; i < parsed_pattern->segment_count; i++)
    {
        TeccPatternSegment *segment = &parsed_pattern->segments[i];

        if (segment->type == TECC_PATTERN_SEGMENT_WILDCARD_PERCENT)
        {
            has_percent = true;
            percent_count++;
        }
        else if (segment->type == TECC_PATTERN_SEGMENT_WILDCARD_UNDERSCORE)
        {
            has_underscore = true;
        }
    }

    /* Update pattern fields */
    parsed_pattern->contains_percent = has_percent;
    parsed_pattern->contains_underscore = has_underscore;
    parsed_pattern->wildcard_count = percent_count + (has_underscore ? 1 : 0);

    /* Determine pattern type */
    if (!has_percent && !has_underscore)
    {
        /* No wildcards - exact match */
        parsed_pattern->pattern_type = TECC_PATTERN_EXACT;
    }
    else if (has_underscore)
    {
        /* Contains '_' wildcard */
        parsed_pattern->pattern_type = TECC_PATTERN_WILDCARD;
    }
    else if (parsed_pattern->segment_count == 2 &&
             parsed_pattern->segments[0].type == TECC_PATTERN_SEGMENT_FIXED_STRING &&
             parsed_pattern->segments[1].type == TECC_PATTERN_SEGMENT_WILDCARD_PERCENT)
    {
        /* Fixed string followed by '%' - prefix pattern */
        parsed_pattern->pattern_type = TECC_PATTERN_PREFIX;
        parsed_pattern->is_simple_prefix = true;
    }
    else if (parsed_pattern->segment_count == 2 &&
             parsed_pattern->segments[0].type == TECC_PATTERN_SEGMENT_WILDCARD_PERCENT &&
             parsed_pattern->segments[1].type == TECC_PATTERN_SEGMENT_FIXED_STRING)
    {
        /* '%' followed by fixed string - suffix pattern */
        parsed_pattern->pattern_type = TECC_PATTERN_SUFFIX;
        parsed_pattern->is_simple_suffix = true;
    }
    else if (parsed_pattern->segment_count == 3 &&
             parsed_pattern->segments[0].type == TECC_PATTERN_SEGMENT_WILDCARD_PERCENT &&
             parsed_pattern->segments[1].type == TECC_PATTERN_SEGMENT_FIXED_STRING &&
             parsed_pattern->segments[2].type == TECC_PATTERN_SEGMENT_WILDCARD_PERCENT)
    {
        /* '%' + fixed string + '%' - infix pattern */
        parsed_pattern->pattern_type = TECC_PATTERN_INFIX;
        parsed_pattern->is_simple_substring = true;
    }
    else
    {
        /* Complex pattern with multiple wildcards */
        parsed_pattern->pattern_type = TECC_PATTERN_COMPLEX;
    }
}

/*
 * tecc_free_parsed_pattern - Free parsed pattern and all its resources
 */
void
tecc_free_parsed_pattern(TeccParsedPattern *parsed_pattern)
{
    size_t i;

    if (!parsed_pattern)
        return;

    /* Free segments */
    if (parsed_pattern->segments)
    {
        for (i = 0; i < parsed_pattern->segment_count; i++)
        {
            TeccPatternSegment *segment = &parsed_pattern->segments[i];

            if (segment->type == TECC_PATTERN_SEGMENT_FIXED_STRING &&
                segment->data.fixed_string)
            {
                tecc_pattern_free(segment->data.fixed_string);
            }

            if (segment->encrypted_data)
            {
                tecc_pattern_free(segment->encrypted_data);
            }
        }
        tecc_pattern_free(parsed_pattern->segments);
    }

    /* Free original pattern */
    if (parsed_pattern->original_pattern)
        tecc_pattern_free(parsed_pattern->original_pattern);

    /* Free optimized encryption */
    if (parsed_pattern->optimized_encryption)
        tecc_pattern_free(parsed_pattern->optimized_encryption);

    /* Free the structure itself */
    tecc_pattern_free(parsed_pattern);
}


/*
 * tecc_encrypt_pattern_segments - Encrypt fixed string segments in parsed pattern
 */
TeccPatternErrorCode
tecc_encrypt_pattern_segments(
    TeccParsedPattern *parsed_pattern,
    const unsigned char *key,
    size_t key_len)
{
    size_t i;

    /* Validate parameters */
    if (!parsed_pattern || !key || key_len == 0)
        return TECC_PATTERN_ERROR_INVALID_PARAM;

    /* Encrypt each fixed string segment */
    for (i = 0; i < parsed_pattern->segment_count; i++)
    {
        TeccPatternSegment *segment = &parsed_pattern->segments[i];

        if (segment->type == TECC_PATTERN_SEGMENT_FIXED_STRING &&
            segment->data.fixed_string && segment->length > 0)
        {
            /* TODO: Implement actual encryption using SM4 algorithm */
            /* For now, just copy the data as placeholder */
            segment->encrypted_data = (unsigned char *)tecc_pattern_alloc(segment->length);
            if (!segment->encrypted_data)
                return TECC_PATTERN_ERROR_MEMORY_ALLOC;

            memcpy(segment->encrypted_data, segment->data.fixed_string, segment->length);
            segment->encrypted_len = segment->length;
        }
    }

    return TECC_PATTERN_SUCCESS;
}

/*
 * tecc_create_match_context - Create pattern matching context
 */
TeccPatternMatchContext *
tecc_create_match_context(
    TeccParsedPattern *parsed_pattern,
    const unsigned char *encrypted_data,
    size_t encrypted_len,
    const unsigned char *key,
    size_t key_len,
    bool case_sensitive)
{
    TeccPatternMatchContext *context;

    /* Validate parameters */
    if (!parsed_pattern || !encrypted_data || encrypted_len == 0 || !key || key_len == 0)
        return NULL;

    /* Allocate context */
    context = (TeccPatternMatchContext *)tecc_pattern_alloc(sizeof(TeccPatternMatchContext));
    if (!context)
        return NULL;

    /* Initialize context */
    memset(context, 0, sizeof(TeccPatternMatchContext));
    context->pattern = parsed_pattern;
    context->encrypted_data = encrypted_data;
    context->encrypted_len = encrypted_len;
    context->key = key;
    context->key_len = key_len;
    context->case_sensitive = case_sensitive;

    /* Allocate match positions array */
    if (parsed_pattern->segment_count > 0)
    {
        context->match_positions = (size_t *)tecc_pattern_alloc(
            parsed_pattern->segment_count * sizeof(size_t));
        if (!context->match_positions)
        {
            tecc_pattern_free(context);
            return NULL;
        }
    }

    return context;
}

/*
 * tecc_free_match_context - Free pattern matching context
 */
void
tecc_free_match_context(TeccPatternMatchContext *context)
{
    if (!context)
        return;

    /* Free match positions array */
    if (context->match_positions)
        tecc_pattern_free(context->match_positions);

    /* Free the context itself */
    tecc_pattern_free(context);
}

/*
 * tecc_match_pattern - Match encrypted data against encrypted pattern
 *
 * This is a simplified implementation that handles common pattern types.
 * Complex patterns with multiple wildcards require a full backtracking algorithm.
 */
bool
tecc_match_pattern(
    TeccPatternMatchContext *context,
    size_t *match_start,
    size_t *match_end)
{
    TeccParsedPattern *pattern;
    size_t i;

    /* Validate parameters */
    if (!context || !context->pattern || !context->encrypted_data)
        return false;

    pattern = context->pattern;

    /* Handle simple pattern types first for performance */
    if (pattern->pattern_type == TECC_PATTERN_EXACT)
    {
        /* Exact match - compare entire data */
        if (context->encrypted_len != pattern->segments[0].encrypted_len)
            return false;

        if (memcmp(context->encrypted_data, pattern->segments[0].encrypted_data,
                   pattern->segments[0].encrypted_len) != 0)
            return false;

        if (match_start) *match_start = 0;
        if (match_end) *match_end = context->encrypted_len;
        return true;
    }
    else if (pattern->is_simple_prefix)
    {
        /* Prefix match - check if data starts with the fixed string */
        TeccPatternSegment *prefix_segment = &pattern->segments[0];

        if (context->encrypted_len < prefix_segment->encrypted_len)
            return false;

        if (memcmp(context->encrypted_data, prefix_segment->encrypted_data,
                   prefix_segment->encrypted_len) != 0)
            return false;

        if (match_start) *match_start = 0;
        if (match_end) *match_end = context->encrypted_len;
        return true;
    }
    else if (pattern->is_simple_suffix)
    {
        /* Suffix match - check if data ends with the fixed string */
        TeccPatternSegment *suffix_segment = &pattern->segments[1];

        size_t start_pos;

        if (context->encrypted_len < suffix_segment->encrypted_len)
            return false;

        start_pos = context->encrypted_len - suffix_segment->encrypted_len;
        if (memcmp(context->encrypted_data + start_pos, suffix_segment->encrypted_data,
                   suffix_segment->encrypted_len) != 0)
            return false;

        if (match_start) *match_start = 0;
        if (match_end) *match_end = context->encrypted_len;
        return true;
    }
    else if (pattern->is_simple_substring)
    {
        /* Substring match - search for fixed string anywhere in data */
        TeccPatternSegment *substring_segment = &pattern->segments[1];

        if (context->encrypted_len < substring_segment->encrypted_len)
            return false;

        /* Simple linear search for now */
        for (i = 0; i <= context->encrypted_len - substring_segment->encrypted_len; i++)
        {
            if (memcmp(context->encrypted_data + i, substring_segment->encrypted_data,
                       substring_segment->encrypted_len) == 0)
            {
                if (match_start) *match_start = i;
                if (match_end) *match_end = i + substring_segment->encrypted_len;
                return true;
            }
        }
        return false;
    }

    /* TODO: Implement full backtracking algorithm for complex patterns */
    /* For now, return false for complex patterns */
    return false;
}

/*
 * tecc_pattern_has_wildcards - Check if pattern contains wildcards
 */
bool
tecc_pattern_has_wildcards(
    const char *pattern,
    size_t pattern_len,
    char escape_char)
{
    size_t i;
    bool last_was_escape = false;

    if (!pattern || pattern_len == 0)
        return false;

    for (i = 0; i < pattern_len; i++)
    {
        char current_char = pattern[i];

        /* Check if current character is escape character */
        if (escape_char != 0 && !last_was_escape && current_char == escape_char)
        {
            last_was_escape = true;
            continue;
        }

        if (last_was_escape)
        {
            last_was_escape = false;
            continue;
        }

        /* Check for wildcards */
        if (current_char == '%' || current_char == '_')
            return true;
    }

    return false;
}

/*
 * tecc_is_simple_prefix_pattern - Check if pattern is a simple prefix match
 */
bool
tecc_is_simple_prefix_pattern(
    const char *pattern,
    size_t pattern_len,
    char escape_char)
{
    TeccParsedPattern *parsed_pattern = NULL;
    TeccPatternErrorCode ret;
    bool result = false;

    /* Parse the pattern */
    ret = tecc_parse_pattern_full(pattern, pattern_len, escape_char, &parsed_pattern);
    if (ret != TECC_PATTERN_SUCCESS)
        return false;

    /* Check if it's a simple prefix pattern */
    result = parsed_pattern->is_simple_prefix;

    /* Cleanup */
    tecc_free_parsed_pattern(parsed_pattern);

    return result;
}

/*
 * tecc_is_simple_suffix_pattern - Check if pattern is a simple suffix match
 */
bool
tecc_is_simple_suffix_pattern(
    const char *pattern,
    size_t pattern_len,
    char escape_char)
{
    TeccParsedPattern *parsed_pattern = NULL;
    TeccPatternErrorCode ret;
    bool result = false;

    /* Parse the pattern */
    ret = tecc_parse_pattern_full(pattern, pattern_len, escape_char, &parsed_pattern);
    if (ret != TECC_PATTERN_SUCCESS)
        return false;

    /* Check if it's a simple suffix pattern */
    result = parsed_pattern->is_simple_suffix;

    /* Cleanup */
    tecc_free_parsed_pattern(parsed_pattern);

    return result;
}

/*
 * tecc_is_simple_substring_pattern - Check if pattern is a simple substring match
 */
bool
tecc_is_simple_substring_pattern(
    const char *pattern,
    size_t pattern_len,
    char escape_char)
{
    TeccParsedPattern *parsed_pattern = NULL;
    TeccPatternErrorCode ret;
    bool result = false;

    /* Parse the pattern */
    ret = tecc_parse_pattern_full(pattern, pattern_len, escape_char, &parsed_pattern);
    if (ret != TECC_PATTERN_SUCCESS)
        return false;

    /* Check if it's a simple substring pattern */
    result = parsed_pattern->is_simple_substring;

    /* Cleanup */
    tecc_free_parsed_pattern(parsed_pattern);

    return result;
}

/*
 * tecc_convert_to_legacy_pattern - Convert unified pattern to legacy structure
 */
TeccPatternErrorCode
tecc_convert_to_legacy_pattern(
    const TeccParsedPattern *parsed_pattern,
    void *legacy_pattern)
{
    /* TODO: Implement conversion to TeccSearchPattern structure */
    /* For now, return success as placeholder */
    return TECC_PATTERN_SUCCESS;
}

/*
 * tecc_convert_from_legacy_pattern - Convert legacy structure to unified pattern
 */
TeccPatternErrorCode
tecc_convert_from_legacy_pattern(
    const void *legacy_pattern,
    TeccParsedPattern **parsed_pattern)
{
    /* TODO: Implement conversion from TeccSearchPattern structure */
    /* For now, return error as placeholder */
    return TECC_PATTERN_ERROR_INVALID_PARAM;
}

/* End - ReqID:TECC4.0 */