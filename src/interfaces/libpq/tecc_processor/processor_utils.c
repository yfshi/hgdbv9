#include "postgres_fe.h"
#include "common/fe_memutils.h"
/* Begin - ReqID:TECC4.0 */
/*
 * File: processor_utils.c
 *
 * Abstract:
 *   Processor utility functions implementation for TECC processor module.
 *   Provides common utility functions used by multiple processor submodules.
 *   Based on openGauss processor_utils design, adapted for TECC C90 compatibility.
 *
 * Authored by <wangxp@highgo.com>, 20260412.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/processor_utils.c
 */

#include "processor_utils.h"
#include "internal/tecc_processor_internal.h"

#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

/* Include frontend parser types */
#include "../frontend_parser/tecc_nodes.h"

/*
 * Name processing functions
 */

/* Convert name list to C string */
bool
tecc_name_list_to_cstring(
    const List *names,
    char *buffer,
    size_t buffer_len)
{
    ListCell *lc;
    size_t pos = 0;
    bool first = true;

    if (names == NULL || buffer == NULL || buffer_len == 0) {
        return false;
    }

    buffer[0] = '\0';

    foreach(lc, names)
    {
        const char *name = (const char *) lfirst(lc);
        size_t name_len;

        if (name == NULL) {
            continue;
        }

        name_len = strlen(name);

        /* Check if we have enough space (name + comma + space + null terminator) */
        if (pos + name_len + (first ? 0 : 2) + 1 > buffer_len) {
            return false;
        }

        if (!first) {
            buffer[pos++] = ',';
            buffer[pos++] = ' ';
        }

        strcpy(buffer + pos, name);
        pos += name_len;
        first = false;
    }

    buffer[pos] = '\0';
    return true;
}

/* Convert name list to string (dynamically allocated) */
char *
tecc_name_list_to_cstring_alloc(
    const List *names)
{
    ListCell *lc;
    size_t total_len = 0;
    char *result;
    size_t pos = 0;
    bool first = true;

    if (names == NULL) {
        return NULL;
    }

    /* Calculate total length needed */
    foreach(lc, names)
    {
        const char *name = (const char *) lfirst(lc);
        if (name != NULL) {
            total_len += strlen(name);
            if (!first) {
                total_len += 2; /* ", " */
            }
            first = false;
        }
    }

    if (total_len == 0) {
        return pg_strdup("");
    }

    /* Allocate buffer */
    result = pg_malloc(total_len + 1);
    if (result == NULL) {
        return NULL;
    }

    /* Build the string */
    first = true;
    foreach(lc, names)
    {
        const char *name = (const char *) lfirst(lc);
        size_t name_len;

        if (name == NULL) {
            continue;
        }

        name_len = strlen(name);

        if (!first) {
            result[pos++] = ',';
            result[pos++] = ' ';
        }

        strcpy(result + pos, name);
        pos += name_len;
        first = false;
    }

    result[pos] = '\0';
    return result;
}

/* Get object's fully qualified name */
bool
tecc_get_object_fqdn(
    const char *object_name,
    bool is_global,
    char *fqdn_buffer,
    size_t buffer_size,
    size_t *fqdn_size)
{
    size_t needed;

    if (object_name == NULL || fqdn_buffer == NULL || buffer_size == 0) {
        return false;
    }

    if (is_global) {
        /* Global object: just the object name */
        needed = strlen(object_name) + 1;
        if (needed > buffer_size) {
            return false;
        }
        strcpy(fqdn_buffer, object_name);
    } else {
        /* Local object: prepend with dot? This depends on the actual FQDN format */
        /* For now, just use the object name */
        needed = strlen(object_name) + 1;
        if (needed > buffer_size) {
            return false;
        }
        strcpy(fqdn_buffer, object_name);
    }

    if (fqdn_size != NULL) {
        *fqdn_size = needed - 1; /* Exclude null terminator */
    }

    return true;
}

/* Build fully qualified name */
bool
tecc_build_fqdn(
    const char *database_name,
    const char *schema_name,
    const char *object_name,
    char *fqdn_buffer,
    size_t buffer_size)
{
    size_t needed;
    size_t pos = 0;

    if (object_name == NULL || fqdn_buffer == NULL || buffer_size == 0) {
        return false;
    }

    /* Format: database.schema.object or schema.object or just object */
    if (database_name != NULL && database_name[0] != '\0') {
        needed = strlen(database_name) + 1; /* +1 for dot */
        if (pos + needed > buffer_size) {
            return false;
        }
        strcpy(fqdn_buffer + pos, database_name);
        pos += strlen(database_name);
        fqdn_buffer[pos++] = '.';
    }

    if (schema_name != NULL && schema_name[0] != '\0') {
        needed = strlen(schema_name) + 1; /* +1 for dot */
        if (pos + needed > buffer_size) {
            return false;
        }
        strcpy(fqdn_buffer + pos, schema_name);
        pos += strlen(schema_name);
        fqdn_buffer[pos++] = '.';
    }

    needed = strlen(object_name) + 1; /* +1 for null terminator */
    if (pos + needed > buffer_size) {
        return false;
    }
    strcpy(fqdn_buffer + pos, object_name);

    return true;
}

/*
 * String processing functions
 */

/* Safe string copy with buffer overflow protection */
size_t
tecc_strlcpy(char *dst, const char *src, size_t size)
{
    size_t src_len;

    if (dst == NULL || size == 0) {
        return 0;
    }

    if (src == NULL) {
        dst[0] = '\0';
        return 0;
    }

    src_len = strlen(src);
    if (src_len >= size) {
        src_len = size - 1;
    }

    memcpy(dst, src, src_len);
    dst[src_len] = '\0';

    return src_len;
}

/* Safe string concatenation with buffer overflow protection */
size_t
tecc_strlcat(char *dst, const char *src, size_t size)
{
    size_t dst_len;
    size_t src_len;
    size_t copy_len;

    if (dst == NULL || src == NULL || size == 0) {
        return 0;
    }

    dst_len = strlen(dst);
    src_len = strlen(src);

    if (dst_len >= size) {
        return dst_len + src_len;
    }

    copy_len = size - dst_len - 1;
    if (src_len < copy_len) {
        copy_len = src_len;
    }

    if (copy_len > 0) {
        memcpy(dst + dst_len, src, copy_len);
    }
    dst[dst_len + copy_len] = '\0';

    return dst_len + src_len;
}

/* Duplicate string with memory allocation */
char *
tecc_strdup(const char *str)
{
    if (str == NULL) {
        return NULL;
    }
    return pg_strdup(str);
}

/* Duplicate string with length limit */
char *
tecc_strndup(const char *str, size_t n)
{
    char *result;
    size_t len;

    if (str == NULL) {
        return NULL;
    }

    len = strlen(str);
    if (len > n) {
        len = n;
    }

    result = pg_malloc(len + 1);
    if (result == NULL) {
        return NULL;
    }

    memcpy(result, str, len);
    result[len] = '\0';

    return result;
}

/* Format string with memory allocation (similar to asprintf) */
char *
tecc_aprintf(const char *fmt, ...)
{
    va_list args;
    char *result;

    if (fmt == NULL) {
        return NULL;
    }

    va_start(args, fmt);
    result = tecc_vaprintf(fmt, args);
    va_end(args);

    return result;
}

/* Format string with va_list */
char *
tecc_vaprintf(const char *fmt, va_list args)
{
    char buffer[1024];
    int len;
    char *result;

    if (fmt == NULL) {
        return NULL;
    }

    /* Try with fixed-size buffer first */
    len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    if (len < 0) {
        return NULL;
    }

    if ((size_t)len < sizeof(buffer)) {
        /* Fits in buffer */
        return pg_strdup(buffer);
    }

    /* Need larger buffer */
    result = pg_malloc(len + 1);
    if (result == NULL) {
        return NULL;
    }

    /* vsnprintf again with the correct size */
    vsnprintf(result, len + 1, fmt, args);

    return result;
}

/* Trim whitespace from string */
char *
tecc_trim_string(char *str)
{
    char *end;

    if (str == NULL) {
        return NULL;
    }

    /* Trim leading space */
    while (isspace((unsigned char)*str)) {
        str++;
    }

    if (*str == '\0') {
        return str;
    }

    /* Trim trailing space */
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }

    /* Write new null terminator */
    *(end + 1) = '\0';

    return str;
}

/* Convert string to lowercase */
void
tecc_str_tolower(char *str)
{
    char *p;

    if (str == NULL) {
        return;
    }

    for (p = str; *p != '\0'; p++) {
        *p = tolower((unsigned char)*p);
    }
}

/* Convert string to uppercase */
void
tecc_str_toupper(char *str)
{
    char *p;

    if (str == NULL) {
        return;
    }

    for (p = str; *p != '\0'; p++) {
        *p = toupper((unsigned char)*p);
    }
}

/* Check if string starts with prefix */
bool
tecc_str_starts_with(const char *str, const char *prefix)
{
    if (str == NULL || prefix == NULL) {
        return false;
    }

    while (*prefix != '\0') {
        if (*str != *prefix) {
            return false;
        }
        str++;
        prefix++;
    }

    return true;
}

/* Check if string ends with suffix */
bool
tecc_str_ends_with(const char *str, const char *suffix)
{
    size_t str_len;
    size_t suffix_len;

    if (str == NULL || suffix == NULL) {
        return false;
    }

    str_len = strlen(str);
    suffix_len = strlen(suffix);

    if (suffix_len > str_len) {
        return false;
    }

    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

/* Replace substring in string */
char *
tecc_str_replace(const char *str, const char *old, const char *new)
{
    char *result;
    size_t str_len, old_len, new_len;
    size_t count = 0;
    const char *tmp;
    char *out;

    if (str == NULL || old == NULL || new == NULL) {
        return NULL;
    }

    str_len = strlen(str);
    old_len = strlen(old);
    new_len = strlen(new);

    /* Count occurrences of old in str */
    tmp = str;
    while ((tmp = strstr(tmp, old)) != NULL) {
        count++;
        tmp += old_len;
    }

    /* Allocate memory for result */
    result = pg_malloc(str_len + count * (new_len - old_len) + 1);
    if (result == NULL) {
        return NULL;
    }

    /* Build result string */
    out = result;
    tmp = str;
    while (*tmp != '\0') {
        if (strstr(tmp, old) == tmp) {
            memcpy(out, new, new_len);
            out += new_len;
            tmp += old_len;
        } else {
            *out++ = *tmp++;
        }
    }

    *out = '\0';
    return result;
}

/*
 * Type conversion functions
 */

/* Convert OID to type name */
const char *
tecc_oid_to_type_name(Oid type_oid)
{
    /* Simplified implementation - should query system catalog in real implementation */
    switch (type_oid)
    {
        case INT2OID: return "int2";
        case INT4OID: return "int4";
        case INT8OID: return "int8";
        case FLOAT4OID: return "float4";
        case FLOAT8OID: return "float8";
        case NUMERICOID: return "numeric";
        case VARCHAROID: return "varchar";
        case BPCHAROID: return "bpchar";
        case TEXTOID: return "text";
        default: return "unknown";
    }
}

/* Convert type name to OID */
Oid
tecc_type_name_to_oid(const char *type_name)
{
    if (type_name == NULL) {
        return 0;
    }

    if (strcmp(type_name, "int2") == 0) return INT2OID;
    if (strcmp(type_name, "int4") == 0) return INT4OID;
    if (strcmp(type_name, "int8") == 0) return INT8OID;
    if (strcmp(type_name, "float4") == 0) return FLOAT4OID;
    if (strcmp(type_name, "float8") == 0) return FLOAT8OID;
    if (strcmp(type_name, "numeric") == 0) return NUMERICOID;
    if (strcmp(type_name, "varchar") == 0) return VARCHAROID;
    if (strcmp(type_name, "bpchar") == 0) return BPCHAROID;
    if (strcmp(type_name, "text") == 0) return TEXTOID;

    return 0;
}

/* Check if type is encrypted */
bool
tecc_is_encrypted_type(Oid type_oid)
{
    /* Check if type OID corresponds to an encrypted type */
    /* This would check against known encrypted type OIDs */
    /* For now, return false as placeholder */
    (void)type_oid;
    return false;
}

/* Check if type supports encryption */
bool
tecc_type_supports_encryption(Oid type_oid)
{
    /* Check if type supports encryption */
    switch (type_oid)
    {
        case INT2OID:
        case INT4OID:
        case INT8OID:
        case FLOAT4OID:
        case FLOAT8OID:
        case NUMERICOID:
        case VARCHAROID:
        case BPCHAROID:
        case TEXTOID:
            return true;
        default:
            return false;
    }
}

/* Get type size */
size_t
tecc_get_type_size(Oid type_oid)
{
    /* Get size of type in bytes */
    switch (type_oid)
    {
        case INT2OID: return 2;
        case INT4OID: return 4;
        case INT8OID: return 8;
        case FLOAT4OID: return 4;
        case FLOAT8OID: return 8;
        case NUMERICOID: return 0; /* Variable size - cannot use sizeof(Numeric) in frontend */
        case VARCHAROID: return 0; /* Variable size */
        case BPCHAROID: return 0; /* Variable size */
        case TEXTOID: return 0; /* Variable size */
        default: return 0;
    }
}

/* Convert value to string based on type */
char *
tecc_value_to_string(Oid type_oid, const void *value, size_t value_size)
{
    char buffer[256];
    char *result;

    if (value == NULL) {
        return pg_strdup("NULL");
    }

    switch (type_oid)
    {
        case INT2OID:
            snprintf(buffer, sizeof(buffer), "%d", *(int16_t *)value);
            break;
        case INT4OID:
            snprintf(buffer, sizeof(buffer), "%d", *(int32_t *)value);
            break;
        case INT8OID:
            snprintf(buffer, sizeof(buffer), "%ld", *(int64_t *)value);
            break;
        case FLOAT4OID:
            snprintf(buffer, sizeof(buffer), "%f", *(float *)value);
            break;
        case FLOAT8OID:
            snprintf(buffer, sizeof(buffer), "%f", *(double *)value);
            break;
        case VARCHAROID:
        case BPCHAROID:
        case TEXTOID:
            if (value_size > 0) {
                /* For string types, copy the string */
                size_t copy_size = value_size;
                if (copy_size >= sizeof(buffer)) {
                    copy_size = sizeof(buffer) - 1;
                }
                memcpy(buffer, value, copy_size);
                buffer[copy_size] = '\0';
            } else {
                buffer[0] = '\0';
            }
            break;
        default:
            snprintf(buffer, sizeof(buffer), "<unknown type %u>", type_oid);
            break;
    }

    result = pg_strdup(buffer);
    return result;
}

/* Parse string to value based on type */
bool
tecc_string_to_value(Oid type_oid, const char *str, void *value, size_t *value_size)
{
    if (str == NULL || value == NULL) {
        return false;
    }

    if (strcmp(str, "NULL") == 0) {
        /* NULL value */
        if (value_size != NULL) {
            *value_size = 0;
        }
        return true;
    }

    switch (type_oid)
    {
        case INT2OID:
            *(int16_t *)value = (int16_t)atoi(str);
            if (value_size != NULL) {
                *value_size = sizeof(int16_t);
            }
            return true;
        case INT4OID:
            *(int32_t *)value = atoi(str);
            if (value_size != NULL) {
                *value_size = sizeof(int32_t);
            }
            return true;
        case INT8OID:
            *(int64_t *)value = atol(str);
            if (value_size != NULL) {
                *value_size = sizeof(int64_t);
            }
            return true;
        case FLOAT4OID:
            *(float *)value = (float)atof(str);
            if (value_size != NULL) {
                *value_size = sizeof(float);
            }
            return true;
        case FLOAT8OID:
            *(double *)value = atof(str);
            if (value_size != NULL) {
                *value_size = sizeof(double);
            }
            return true;
        case VARCHAROID:
        case BPCHAROID:
        case TEXTOID:
            {
                size_t len = strlen(str);
                if (value_size != NULL) {
                    *value_size = len;
                }
                memcpy(value, str, len);
                return true;
            }
        default:
            return false;
    }
}

/*
 * Error handling functions
 */

/* Set error with formatted message */
void
tecc_set_error(int *error_code, char *error_buffer, size_t buffer_size,
               const char *format, ...)
{
    va_list args;

    if (error_code == NULL || error_buffer == NULL || buffer_size == 0 || format == NULL) {
        return;
    }

    *error_code = TECC_PROCESSOR_ERROR;

    va_start(args, format);
    vsnprintf(error_buffer, buffer_size, format, args);
    va_end(args);
}

/* Set error with va_list */
void
tecc_vset_error(int *error_code, char *error_buffer, size_t buffer_size,
                const char *format, va_list args)
{
    if (error_code == NULL || error_buffer == NULL || buffer_size == 0 || format == NULL) {
        return;
    }

    *error_code = TECC_PROCESSOR_ERROR;
    vsnprintf(error_buffer, buffer_size, format, args);
}

/* Get error message for error code */
const char *
tecc_get_error_message(int error_code)
{
    /* Use the existing error message function from tecc_processor_internal.c */
    return tecc_processor_get_error_message(error_code);
}

/* Check if error is recoverable */
bool
tecc_is_recoverable_error(int error_code)
{
    /* Use the existing function from tecc_processor_internal.c */
    return tecc_processor_is_recoverable_error(error_code);
}

/* Clear error state */
void
tecc_clear_error(int *error_code, char *error_buffer)
{
    if (error_code != NULL) {
        *error_code = TECC_PROCESSOR_SUCCESS;
    }
    if (error_buffer != NULL) {
        error_buffer[0] = '\0';
    }
}

/*
 * Memory management functions
 */

/* Allocate memory with error checking */
void *
tecc_malloc(size_t size)
{
    void *ptr;

    if (size == 0) {
        return NULL;
    }

    ptr = pg_malloc(size);
    if (ptr == NULL) {
        /* Memory allocation failed */
        /* In a real implementation, we might want to log this error */
    }

    return ptr;
}

/* Allocate zero-initialized memory */
void *
tecc_calloc(size_t nmemb, size_t size)
{
    void *ptr;
    size_t total_size;

    if (nmemb == 0 || size == 0) {
        return NULL;
    }

    total_size = nmemb * size;
    ptr = pg_malloc(total_size);
    if (ptr != NULL) {
        memset(ptr, 0, total_size);
    }

    return ptr;
}

/* Reallocate memory with error checking */
void *
tecc_processor_realloc(void *ptr, size_t size)
{
    if (size == 0) {
        if (ptr != NULL) {
            pg_free(ptr);
        }
        return NULL;
    }

    return pg_realloc(ptr, size);
}

/* Free memory with NULL checking */
void
tecc_processor_free(void *ptr)
{
    if (ptr != NULL) {
        pg_free(ptr);
    }
}

/* Duplicate memory block */
void *
tecc_memdup(const void *src, size_t size)
{
    void *dst;

    if (src == NULL || size == 0) {
        return NULL;
    }

    dst = pg_malloc(size);
    if (dst != NULL) {
        memcpy(dst, src, size);
    }

    return dst;
}

/*
 * String arguments management functions
 */

/* Create string arguments list */
TeccStringArgs *
tecc_string_args_create(size_t initial_capacity)
{
    TeccStringArgs *args;

    if (initial_capacity == 0) {
        initial_capacity = 16; /* Default capacity */
    }

    args = (TeccStringArgs *)pg_malloc(sizeof(TeccStringArgs));
    if (args == NULL) {
        return NULL;
    }

    args->args = (TeccStringArg *)pg_malloc(sizeof(TeccStringArg) * initial_capacity);
    if (args->args == NULL) {
        pg_free(args);
        return NULL;
    }

    args->size = 0;
    args->capacity = initial_capacity;

    return args;
}

/* Destroy string arguments list */
void
tecc_string_args_destroy(TeccStringArgs *args)
{
    size_t i;

    if (args == NULL) {
        return;
    }

    if (args->args != NULL) {
        for (i = 0; i < args->size; i++) {
            if (args->args[i].key != NULL) {
                pg_free(args->args[i].key);
            }
            if (args->args[i].value != NULL) {
                pg_free(args->args[i].value);
            }
        }
        pg_free(args->args);
    }

    pg_free(args);
}

/* Add argument to list */
bool
tecc_string_args_add(TeccStringArgs *args, const char *key, const char *value)
{
    TeccStringArg *new_args;
    size_t new_capacity;
    size_t key_len, value_len;

    if (args == NULL || key == NULL) {
        return false;
    }

    /* Check if we need to resize */
    if (args->size >= args->capacity) {
        new_capacity = args->capacity * 2;
        new_args = (TeccStringArg *)pg_realloc(args->args, sizeof(TeccStringArg) * new_capacity);
        if (new_args == NULL) {
            return false;
        }
        args->args = new_args;
        args->capacity = new_capacity;
    }

    /* Add new argument */
    key_len = strlen(key) + 1;
    args->args[args->size].key = (char *)pg_malloc(key_len);
    if (args->args[args->size].key == NULL) {
        return false;
    }
    strcpy(args->args[args->size].key, key);

    if (value != NULL) {
        value_len = strlen(value) + 1;
        args->args[args->size].value = (char *)pg_malloc(value_len);
        if (args->args[args->size].value == NULL) {
            pg_free(args->args[args->size].key);
            return false;
        }
        strcpy(args->args[args->size].value, value);
        args->args[args->size].value_size = value_len - 1;
    } else {
        args->args[args->size].value = NULL;
        args->args[args->size].value_size = 0;
    }

    args->size++;
    return true;
}

/* Get argument value by key */
const char *
tecc_string_args_get(const TeccStringArgs *args, const char *key)
{
    size_t i;

    if (args == NULL || key == NULL || args->args == NULL) {
        return NULL;
    }

    for (i = 0; i < args->size; i++) {
        if (strcmp(args->args[i].key, key) == 0) {
            return args->args[i].value;
        }
    }

    return NULL;
}

/* Remove argument by key */
bool
tecc_string_args_remove(TeccStringArgs *args, const char *key)
{
    size_t i;

    if (args == NULL || key == NULL || args->args == NULL) {
        return false;
    }

    for (i = 0; i < args->size; i++) {
        if (strcmp(args->args[i].key, key) == 0) {
            /* Found it - free memory and shift remaining elements */
            pg_free(args->args[i].key);
            if (args->args[i].value != NULL) {
                pg_free(args->args[i].value);
            }

            /* Shift remaining elements */
            if (i < args->size - 1) {
                memmove(&args->args[i], &args->args[i + 1],
                       (args->size - i - 1) * sizeof(TeccStringArg));
            }

            args->size--;
            return true;
        }
    }

    return false;
}

/* Clear all arguments */
void
tecc_string_args_clear(TeccStringArgs *args)
{
    size_t i;

    if (args == NULL || args->args == NULL) {
        return;
    }

    for (i = 0; i < args->size; i++) {
        if (args->args[i].key != NULL) {
            pg_free(args->args[i].key);
        }
        if (args->args[i].value != NULL) {
            pg_free(args->args[i].value);
        }
    }

    args->size = 0;
}

/* Get number of arguments */
size_t
tecc_string_args_size(const TeccStringArgs *args)
{
    if (args == NULL) {
        return 0;
    }
    return args->size;
}

/* Check if arguments list is empty */
bool
tecc_string_args_is_empty(const TeccStringArgs *args)
{
    if (args == NULL) {
        return true;
    }
    return args->size == 0;
}
/* End - ReqID:TECC4.0 */