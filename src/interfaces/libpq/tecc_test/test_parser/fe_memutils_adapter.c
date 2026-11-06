/* Begin - ReqID:TECC4.0 */
/*
 * File: fe_memutils_adapter.c
 *
 * Abstract:
 *   Adapter for PostgreSQL frontend memory utilities in test environment.
 *   Provides pg_malloc, pg_free, and other required functions.
 *
 * Authored by <wanghaoyan@highgo.com>, 20260413.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   fe_memutils_adapter.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

/* PostgreSQL frontend memory utilities simulation */
void *pg_malloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr && size > 0) {
        memset(ptr, 0, size);
    }
    return ptr;
}

void *pg_malloc0(size_t size)
{
    return calloc(1, size);
}

void *pg_realloc(void *ptr, size_t size)
{
    return realloc(ptr, size);
}

void pg_free(void *ptr)
{
    free(ptr);
}

char *pg_strdup(const char *str)
{
    if (!str) return NULL;
    return strdup(str);
}

/* Simple printf for testing */
int pg_printf(const char *fmt, ...)
{
    /* For testing, we'll just use printf */
    va_list args;
    va_start(args, fmt);
    int result = vprintf(fmt, args);
    va_end(args);
    return result;
}

int pg_snprintf(char *str, size_t count, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int result = vsnprintf(str, count, fmt, args);
    va_end(args);
    return result;
}

int pg_fprintf(FILE *stream, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int result = vfprintf(stream, fmt, args);
    va_end(args);
    return result;
}

/* Other PostgreSQL functions that might be needed */
void freeRangeVar(void *ptr)
{
    /* Simplified version for testing */
    if (ptr) {
        free(ptr);
    }
}

void freeA_Expr(void *ptr)
{
    /* Simplified version for testing */
    if (ptr) {
        free(ptr);
    }
}

void freeColumnRef(void *ptr)
{
    /* Simplified version for testing */
    if (ptr) {
        free(ptr);
    }
}

void freeValue(void *ptr)
{
    /* Simplified version for testing */
    if (ptr) {
        free(ptr);
    }
}

void freeList(void *ptr)
{
    /* Simplified version for testing */
    if (ptr) {
        free(ptr);
    }
}

/* TECC memory functions - provided by tecc_frontend_list.c */
/*
void *tecc_palloc(size_t size)
{
    return pg_malloc(size);
}

void tecc_pfree(void *ptr)
{
    pg_free(ptr);
}

char *tecc_pstrdup(const char *str)
{
    return pg_strdup(str);
}
*/

/* PostgreSQL strncasecmp implementation for testing */
int pg_strncasecmp(const char *s1, const char *s2, size_t n)
{
    if (n == 0) return 0;
    while (n-- && *s1 && *s2) {
        char c1 = *s1++;
        char c2 = *s2++;
        if (c1 >= 'A' && c1 <= 'Z') c1 += 'a' - 'A';
        if (c2 >= 'A' && c2 <= 'Z') c2 += 'a' - 'A';
        if (c1 != c2) return (c1 - c2);
    }
    if (n == (size_t)-1) return 0;
    return (*s1 - *s2);
}

/* PostgreSQL vsnprintf implementation for testing */
int pg_vsnprintf(char *str, size_t count, const char *fmt, va_list args)
{
    return vsnprintf(str, count, fmt, args);
}
