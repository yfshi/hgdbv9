/* Begin - ReqID:TECC4.0 */
/*
 * File: fe_memutils_adapter.c
 *
 * Abstract:
 *   Simple adapter for fe_memutils functions for TECC formatter test.
 *   Provides pg_malloc, pg_free, pg_malloc0, and pg_strncasecmp implementations.
 *
 * Authored by <wangxp@highgo.com>, 20260412.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_test/test_formatter/fe_memutils_adapter.c
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

/*
 * pg_malloc - Simple malloc wrapper
 */
void *
pg_malloc(size_t size)
{
    if (size == 0)
        size = 1;
    return malloc(size);
}

/*
 * pg_malloc0 - Simple malloc wrapper with zero initialization
 */
void *
pg_malloc0(size_t size)
{
    void *ptr;

    if (size == 0)
        size = 1;

    ptr = malloc(size);
    if (ptr)
        memset(ptr, 0, size);

    return ptr;
}

/*
 * pg_free - Simple free wrapper
 */
void
pg_free(void *ptr)
{
    if (ptr)
        free(ptr);
}

/*
 * pg_strncasecmp - Case-insensitive string comparison
 */
int
pg_strncasecmp(const char *s1, const char *s2, size_t n)
{
    if (n == 0)
        return 0;

    while (n-- > 0 && *s1 && *s2)
    {
        int c1 = tolower((unsigned char)*s1);
        int c2 = tolower((unsigned char)*s2);

        if (c1 != c2)
            return c1 - c2;

        s1++;
        s2++;
    }

    if (n == (size_t)-1)
        return 0;

    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

/*
 * pg_printf - Simple printf wrapper
 */
int
pg_printf(const char *fmt, ...)
{
    va_list args;
    int result;

    va_start(args, fmt);
    result = vprintf(fmt, args);
    va_end(args);

    return result;
}

/*
 * pg_snprintf - Simple snprintf wrapper
 */
int
pg_snprintf(char *str, size_t count, const char *fmt, ...)
{
    va_list args;
    int result;

    va_start(args, fmt);
    result = vsnprintf(str, count, fmt, args);
    va_end(args);

    return result;
}

/*
 * pg_fprintf - Simple fprintf wrapper
 */
int
pg_fprintf(FILE *stream, const char *fmt, ...)
{
    va_list args;
    int result;

    va_start(args, fmt);
    result = vfprintf(stream, fmt, args);
    va_end(args);

    return result;
}

/*
 * pg_vfprintf - Simple vfprintf wrapper
 */
int
pg_vfprintf(FILE *stream, const char *fmt, va_list args)
{
    return vfprintf(stream, fmt, args);
}

/* End - ReqID:TECC4.0 */