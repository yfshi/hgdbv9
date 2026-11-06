/* Begin - ReqID:TECC4.0 */
/*
 * File: processor_utils.h
 *
 * Abstract:
 *   Processor utility functions for TECC processor module.
 *   Provides common utility functions used by multiple processor submodules.
 *   Based on openGauss processor_utils design, adapted for TECC C90 compatibility.
 *
 * Authored by <wangxp@highgo.com>, 20260412.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/processor_utils.h
 */

#ifndef PROCESSOR_UTILS_H
#define PROCESSOR_UTILS_H

#include "tecc_processor_types.h"  /* Include type definitions */

/* Standard C headers for frontend code */
#include <stddef.h>  /* For size_t */
#include <stdbool.h> /* For bool */

/* Include frontend parser types for List definition */
#include "../frontend_parser/tecc_nodes.h"

/*
 * Name list conversion result structure
 */
typedef struct TeccNameListResult
{
    char *buffer;           /* Converted buffer */
    size_t buffer_size;     /* Buffer size */
    size_t result_size;     /* Result size */
} TeccNameListResult;

/*
 * String argument structure
 */
typedef struct TeccStringArg
{
    char *key;              /* Argument key */
    char *value;            /* Argument value */
    size_t value_size;      /* Argument value size */
} TeccStringArg;

/*
 * String arguments list structure
 */
typedef struct TeccStringArgs
{
    TeccStringArg *args;    /* Arguments array */
    size_t size;            /* Number of arguments */
    size_t capacity;        /* Array capacity */
} TeccStringArgs;

/*
 * Name processing interface
 */

/* Convert name list to C string */
bool tecc_name_list_to_cstring(
    const List *names,
    char *buffer,
    size_t buffer_len);

/* Convert name list to string (dynamically allocated) */
char *tecc_name_list_to_cstring_alloc(
    const List *names);

/* Get object's fully qualified name */
bool tecc_get_object_fqdn(
    const char *object_name,
    bool is_global,
    char *fqdn_buffer,
    size_t buffer_size,
    size_t *fqdn_size);

/* Build fully qualified name */
bool tecc_build_fqdn(
    const char *database_name,
    const char *schema_name,
    const char *object_name,
    char *fqdn_buffer,
    size_t buffer_size);

/*
 * String processing interface
 */

/* Safe string copy with buffer overflow protection */
size_t tecc_strlcpy(char *dst, const char *src, size_t size);

/* Safe string concatenation with buffer overflow protection */
size_t tecc_strlcat(char *dst, const char *src, size_t size);

/* Duplicate string with memory allocation */
char *tecc_strdup(const char *str);

/* Duplicate string with length limit */
char *tecc_strndup(const char *str, size_t n);

/* Format string with memory allocation (similar to asprintf) */
char *tecc_aprintf(const char *fmt, ...);

/* Format string with va_list */
char *tecc_vaprintf(const char *fmt, va_list args);

/* Trim whitespace from string */
char *tecc_trim_string(char *str);

/* Convert string to lowercase */
void tecc_str_tolower(char *str);

/* Convert string to uppercase */
void tecc_str_toupper(char *str);

/* Check if string starts with prefix */
bool tecc_str_starts_with(const char *str, const char *prefix);

/* Check if string ends with suffix */
bool tecc_str_ends_with(const char *str, const char *suffix);

/* Replace substring in string */
char *tecc_str_replace(const char *str, const char *old, const char *new);

/*
 * Type conversion interface
 */

/* Convert OID to type name */
const char *tecc_oid_to_type_name(Oid type_oid);

/* Convert type name to OID */
Oid tecc_type_name_to_oid(const char *type_name);

/* Check if type is encrypted */
bool tecc_is_encrypted_type(Oid type_oid);

/* Check if type supports encryption */
bool tecc_type_supports_encryption(Oid type_oid);

/* Get type size */
size_t tecc_get_type_size(Oid type_oid);

/* Convert value to string based on type */
char *tecc_value_to_string(Oid type_oid, const void *value, size_t value_size);

/* Parse string to value based on type */
bool tecc_string_to_value(Oid type_oid, const char *str, void *value, size_t *value_size);

/*
 * Error handling interface
 */

/* Set error with formatted message */
void tecc_set_error(int *error_code, char *error_buffer, size_t buffer_size,
                   const char *format, ...);

/* Set error with va_list */
void tecc_vset_error(int *error_code, char *error_buffer, size_t buffer_size,
                    const char *format, va_list args);

/* Get error message for error code */
const char *tecc_get_error_message(int error_code);

/* Check if error is recoverable */
bool tecc_is_recoverable_error(int error_code);

/* Clear error state */
void tecc_clear_error(int *error_code, char *error_buffer);

/*
 * Memory management interface
 */

/* Allocate memory with error checking */
void *tecc_malloc(size_t size);

/* Allocate zero-initialized memory */
void *tecc_calloc(size_t nmemb, size_t size);

/* Reallocate memory with error checking */
void *tecc_processor_realloc(void *ptr, size_t size);

/* Free memory with NULL checking */
void tecc_processor_free(void *ptr);

/* Duplicate memory block */
void *tecc_memdup(const void *src, size_t size);

/*
 * String arguments management
 */

/* Create string arguments list */
TeccStringArgs *tecc_string_args_create(size_t initial_capacity);

/* Destroy string arguments list */
void tecc_string_args_destroy(TeccStringArgs *args);

/* Add argument to list */
bool tecc_string_args_add(TeccStringArgs *args, const char *key, const char *value);

/* Get argument value by key */
const char *tecc_string_args_get(const TeccStringArgs *args, const char *key);

/* Remove argument by key */
bool tecc_string_args_remove(TeccStringArgs *args, const char *key);

/* Clear all arguments */
void tecc_string_args_clear(TeccStringArgs *args);

/* Get number of arguments */
size_t tecc_string_args_size(const TeccStringArgs *args);

/* Check if arguments list is empty */
bool tecc_string_args_is_empty(const TeccStringArgs *args);

/*
 * Utility macros
 */

/* Check if pointer is NULL and set error */
#define TECC_CHECK_NULL(ptr, error_code, error_msg) \
    do { \
        if ((ptr) == NULL) { \
            *(error_code) = TECC_PROCESSOR_INVALID_INPUT; \
            snprintf((error_msg), sizeof(error_msg), "NULL pointer: %s", #ptr); \
            return false; \
        } \
    } while (0)

/* Check if condition is true and set error */
#define TECC_CHECK(condition, error_code, error_msg, format, ...) \
    do { \
        if (!(condition)) { \
            *(error_code) = TECC_PROCESSOR_ERROR; \
            snprintf((error_msg), sizeof(error_msg), format, ##__VA_ARGS__); \
            return false; \
        } \
    } while (0)

/* Safe memory free */
#define TECC_SAFE_FREE(ptr) \
    do { \
        if ((ptr) != NULL) { \
            pg_free(ptr); \
            (ptr) = NULL; \
        } \
    } while (0)

/* Array size calculation */
#define TECC_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* Minimum of two values */
#define TECC_MIN(a, b) ((a) < (b) ? (a) : (b))

/* Maximum of two values */
#define TECC_MAX(a, b) ((a) > (b) ? (a) : (b))

/* Clamp value between min and max */
#define TECC_CLAMP(val, min, max) \
    ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))

#endif /* PROCESSOR_UTILS_H */
/* End - ReqID:TECC4.0 */