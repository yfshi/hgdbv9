/* Begin - ReqID:TECC4.0 */
/*
 * File: test_functions.h
 *
 * Abstract:
 *   Function declarations for test environment.
 *
 * Authored by <wangxp@highgo.com>, 20260412.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_test/test_processor/test_functions.h
 */

#ifndef TEST_FUNCTIONS_H
#define TEST_FUNCTIONS_H

#include <stdlib.h>
#include <string.h>

/* Test memory management functions */
void *test_palloc(size_t size);
void *test_palloc0(size_t size);
void test_pfree(void *pointer);
char *test_pstrdup(const char *string);

/* PostgreSQL function aliases for test environment */
#define pg_fprintf fprintf
#define pg_snprintf snprintf
#define pg_vsnprintf vsnprintf

/* Provide implementations for pg_malloc/pg_free for standalone testing */
/* These should match the declarations in fe_memutils.h */
static void *pg_malloc(size_t size)
{
    void *ptr = malloc(size);
    if (!ptr && size > 0) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    return ptr;
}

static void *pg_malloc0(size_t size)
{
    void *ptr = pg_malloc(size);
    if (ptr && size > 0) {
        memset(ptr, 0, size);
    }
    return ptr;
}

static void pg_free(void *ptr)
{
    if (ptr) {
        free(ptr);
    }
}

static char *pg_strdup(const char *str)
{
    char *result;

    if (!str) {
        return NULL;
    }

    result = pg_malloc(strlen(str) + 1);
    if (result) {
        strcpy(result, str);
    }
    return result;
}

/* PostgreSQL list function stubs - match tecc_nodes.h */
typedef struct List List;
typedef struct ListCell ListCell;

List *lappend(List *list, void *datum);
void *lfirst(const ListCell *cell);
void list_free(List *list);

/* Encryption function stubs - match actual signatures */
typedef struct TeccUnifiedEncryptResult TeccUnifiedEncryptResult;
typedef struct TeccUnifiedDecryptResult TeccUnifiedDecryptResult;

TeccUnifiedEncryptResult *tecc_unified_encrypt_string(const char *plaintext,
                                                     const char *table_name,
                                                     const char *column_name,
                                                     const char *kek_name,
                                                     const char *dek_name);
TeccUnifiedDecryptResult *tecc_unified_decrypt_hex(const char *hex_string,
                                                  const char *table_name,
                                                  const char *column_name,
                                                  TeccCryptoContext *external_context);
void tecc_unified_free_encrypt_result(TeccUnifiedEncryptResult *result);
void tecc_unified_free_decrypt_result(TeccUnifiedDecryptResult *result);

#endif /* TEST_FUNCTIONS_H */
/* End - ReqID:TECC4.0 */