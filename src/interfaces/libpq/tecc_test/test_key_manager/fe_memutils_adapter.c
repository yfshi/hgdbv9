/* Begin - ReqID:TECC4.0 */
/*
 * File: fe_memutils_adapter.c
 *
 * Abstract:
 *   Adapter for PostgreSQL frontend memory utilities in test environment.
 *   Provides pg_malloc, pg_free, and other required functions for key manager tests.
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
#include <time.h>

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

/* Mock PostgreSQL error reporting */
void elog(int level, const char *fmt, ...)
{
    const char *level_str;
    switch (level) {
        case 1: level_str = "DEBUG"; break;
        case 2: level_str = "LOG"; break;
        case 3: level_str = "INFO"; break;
        case 4: level_str = "NOTICE"; break;
        case 5: level_str = "WARNING"; break;
        case 6: level_str = "ERROR"; break;
        case 7: level_str = "FATAL"; break;
        case 8: level_str = "PANIC"; break;
        default: level_str = "UNKNOWN"; break;
    }

    va_list args;
    va_start(args, fmt);
    printf("[%s] ", level_str);
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

/* Mock PostgreSQL ereport */
void ereport(int elevel, int dummy, ...)
{
    /* Simplified version for testing */
    va_list args;
    const char *fmt;

    va_start(args, dummy);
    fmt = va_arg(args, const char *);

    elog(elevel, fmt);

    va_end(args);
}

/* Mock PostgreSQL GetCurrentTimestamp */
TimestampTz GetCurrentTimestamp(void)
{
    /* Return current time in microseconds since 2000-01-01 */
    struct timeval tv;
    gettimeofday(&tv, NULL);

    /* PostgreSQL epoch is 2000-01-01, Unix epoch is 1970-01-01 */
    /* Difference in seconds: 30 years + 8 leap days = 946684800 seconds */
    const long long postgres_epoch_offset = 946684800LL;

    long long microseconds = (tv.tv_sec - postgres_epoch_offset) * 1000000LL + tv.tv_usec;
    return microseconds;
}

/* Mock PostgreSQL MemoryContext */
void *MemoryContextAlloc(void *context, size_t size)
{
    (void)context; /* Unused in test environment */
    return pg_malloc(size);
}

void *MemoryContextAllocZero(void *context, size_t size)
{
    (void)context; /* Unused in test environment */
    return pg_malloc0(size);
}

void pfree(void *ptr)
{
    pg_free(ptr);
}

void *palloc(size_t size)
{
    return pg_malloc(size);
}

void *palloc0(size_t size)
{
    return pg_malloc0(size);
}

char *pstrdup(const char *str)
{
    return pg_strdup(str);
}

void *repalloc(void *ptr, size_t size)
{
    return pg_realloc(ptr, size);
}

/* Mock pthread functions for testing */
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
    (void)attr; /* Unused in test environment */
    /* Simple implementation for testing */
    mutex->__data.__lock = 0;
    mutex->__data.__count = 0;
    mutex->__data.__owner = 0;
    mutex->__data.__nusers = 0;
    mutex->__data.__kind = 0;
    mutex->__data.__spins = 0;
    mutex->__data.__list = NULL;
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    (void)mutex; /* Unused in test environment */
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    (void)mutex; /* Unused in test environment */
    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    (void)mutex; /* Unused in test environment */
    return 0;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    (void)mutex; /* Unused in test environment */
    return 0;
}

int pthread_key_create(pthread_key_t *key, void (*destructor)(void*))
{
    (void)destructor; /* Unused in test environment */
    static int next_key = 1;
    *key = next_key++;
    return 0;
}

int pthread_key_delete(pthread_key_t key)
{
    (void)key; /* Unused in test environment */
    return 0;
}

void *pthread_getspecific(pthread_key_t key)
{
    (void)key; /* Unused in test environment */
    /* Simple implementation for testing */
    static void *specific_data = NULL;
    return specific_data;
}

int pthread_setspecific(pthread_key_t key, const void *value)
{
    (void)key; /* Unused in test environment */
    /* Simple implementation for testing */
    static void *specific_data = NULL;
    specific_data = (void*)value;
    return 0;
}

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
    if (once_control->__sig == 0) {
        init_routine();
        once_control->__sig = 1;
    }
    return 0;
}

/* Mock gettimeofday for platforms that might not have it */
int gettimeofday(struct timeval *tv, void *tz)
{
    (void)tz; /* Unused in test environment */

    /* Simple implementation using time() */
    time_t now = time(NULL);
    tv->tv_sec = now;
    tv->tv_usec = 0;
    return 0;
}