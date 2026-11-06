/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_processor_internal_test.c
 *
 * Abstract:
 *   Simplified version of tecc_processor_internal.c for test environment.
 *   Provides memory management and utility functions without PostgreSQL dependencies.
 *
 * Authored by <wangxp@highgo.com>, 20260412.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_test/test_processor/tecc_processor_internal_test.c
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "test_functions.h"

/* C90 doesn't have stdbool.h, define our own */
#ifndef __cplusplus
typedef int bool;
#define true 1
#define false 0
#endif

/* Functions are declared in test_functions.h */

/* Redefine tecc_processor_debug macro to avoid pg_fprintf */
#undef tecc_processor_debug
#define tecc_processor_debug(level, msg, ...) \
    do { \
        if (tecc_processor_debug_level >= (level)) { \
            /* Use standard fprintf for testing */ \
            fprintf(stderr, "TECC TEST: " msg "\n", ##__VA_ARGS__); \
        } \
    } while (0)

/* PostgreSQL function aliases are defined in test_functions.h */

/* Define debug level constants */
#define TECC_LOG_DEBUG1 3
#define TECC_LOG_INFO 2
#define TECC_LOG_WARNING 1
#define TECC_LOG_ERROR 0

/* Debug level variable for testing */
/* Note: This is a test-only variable, not used in production code */
int tecc_processor_debug_level = TECC_LOG_DEBUG1;

/* Simple string utilities */
char *tecc_processor_strdup(const char *string)
{
    if (string == NULL)
        return NULL;

    size_t len = strlen(string) + 1;
    char *result = test_palloc(len);
    if (result != NULL)
        memcpy(result, string, len);
    return result;
}

/* PostgreSQL list function stubs - match tecc_nodes.h */
List *lappend(List *list, void *datum)
{
    (void)list;
    (void)datum;
    return NULL; /* Stub implementation */
}

void *lfirst(const ListCell *cell)
{
    (void)cell;
    return NULL; /* Stub implementation */
}

void list_free(List *list)
{
    (void)list;
    /* Stub implementation */
}

/* Simple list utilities (stub implementations) */
void *tecc_processor_list_append_string(void *list, const char *str)
{
    (void)list;
    (void)str;
    return NULL; /* Stub implementation */
}

int tecc_processor_list_member_string(void *list, const char *str)
{
    (void)list;
    (void)str;
    return 0; /* Stub implementation */
}

void tecc_processor_list_free_deep(void *list)
{
    (void)list;
    /* Stub implementation */
}

/* Error handling utilities */
void tecc_processor_set_error(void *stmt_data, int error_code, const char *fmt, ...)
{
    (void)stmt_data;
    (void)error_code;
    (void)fmt;
    /* Stub implementation */
}

void tecc_processor_clear_error(void *stmt_data)
{
    (void)stmt_data;
    /* Stub implementation */
}

int tecc_processor_attempt_recovery(void *stmt_data, const char *recovery_action)
{
    (void)stmt_data;
    (void)recovery_action;
    return 0; /* Stub implementation */
}

void tecc_processor_set_error_with_recovery(void *stmt_data, int error_code,
                                           const char *recovery_action,
                                           const char *fmt, ...)
{
    (void)stmt_data;
    (void)error_code;
    (void)recovery_action;
    (void)fmt;
    /* Stub implementation */
}

/* Parameter utilities */
const char *tecc_find_param_value(void *params, const char *name)
{
    (void)params;
    (void)name;
    return NULL; /* Stub implementation */
}

/* Encryption function stubs - match actual signatures */
TeccUnifiedEncryptResult *tecc_unified_encrypt_string(const char *plaintext,
                                                     const char *table_name,
                                                     const char *column_name,
                                                     const char *kek_name,
                                                     const char *dek_name)
{
    (void)plaintext;
    (void)table_name;
    (void)column_name;
    (void)kek_name;
    (void)dek_name;
    return NULL; /* Stub implementation */
}

TeccUnifiedDecryptResult *tecc_unified_decrypt_hex(PGconn *conn,
                                                  const char *hex_string,
                                                  const char *table_name,
                                                  const char *column_name)
{
    (void)conn;
    (void)hex_string;
    (void)table_name;
    (void)column_name;
    return NULL; /* Stub implementation */
}

void tecc_unified_free_encrypt_result(TeccUnifiedEncryptResult *result)
{
    (void)result;
    /* Stub implementation */
}

void tecc_unified_free_decrypt_result(TeccUnifiedDecryptResult *result)
{
    (void)result;
    /* Stub implementation */
}

/* End - ReqID:TECC4.0 */