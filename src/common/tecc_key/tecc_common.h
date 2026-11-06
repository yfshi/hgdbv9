/*-------------------------------------------------------------------------
 *
 * tecc_common.h
 *    Common definitions and macros for TECC key management module
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/common/tecc_key/tecc_common.h
 *
 * ReqID:TECC4.0 - Key management common definitions
 * Authored by <wanghaoyan@highgo.com>, 20260413.
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_COMMON_H
#define TECC_COMMON_H

/* Enable GNU extensions for strdup */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/* Standard includes */
#include <stddef.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>  /* For strdup */
#ifdef _WIN32
#include <windows.h>
#include <port/win32/pthread.h>
#else
#include <pthread.h> /* For pthread types */
#endif

/* Include common TECC types - TeccAlgorithmType is defined here */
#include "../tecc_types.h"

/* PostgreSQL types */
/* Include PostgreSQL header for Oid definition */
#include "postgres_ext.h"

/* Always define Oid type - it's needed by both frontend and backend code */
#ifndef Oid
typedef unsigned int Oid;
#endif

/* Only define InvalidOid if not already defined by PostgreSQL headers */
#ifndef InvalidOid
#define InvalidOid 0
#endif

#ifndef NAMEDATALEN
#define NAMEDATALEN 64
#endif

#ifndef TimestampTz
typedef long long TimestampTz;
#endif

/* Memory allocation macros */
/* These must be provided by the including compilation unit.
 * Do NOT add fallback definitions here - they hide real problems.
 * ReqID:TECC4.0 */

/* Portable module auto-initialization (constructor equivalent)
 * GCC/Clang: __attribute__((constructor)) on the function
 * MSVC: CRT section .CRT$XCU function pointer (runs before main)
 *
 * Usage: replace "static void __attribute__((constructor)) fn(void)" with:
 *     TECC_CONSTRUCTOR(fn) { ... }
 */
#if defined(_MSC_VER)
#define TECC_CONSTRUCTOR(fn) \
    static void fn(void); \
    __pragma(section(".CRT$XCU", read)) \
    __declspec(allocate(".CRT$XCU")) static void (*fn##_crt_init_)(void) = fn; \
    static void fn(void)
#else
#define TECC_CONSTRUCTOR(fn) static void __attribute__((constructor)) fn(void)
#endif

/* Portable printf format attribute (GCC static check, MSVC no-op) */
#if defined(_MSC_VER)
#define TECC_PRINTF_ATTR(f,a)
#else
#define TECC_PRINTF_ATTR(f,a) __attribute__((format(printf, f, a)))
#endif

/* Logging macros - Note: tecc_key_log function is defined in utils.c */
/* These string constants are kept for compatibility */
#define LOG_LEVEL_DEBUG_STR "DEBUG"
#define LOG_LEVEL_INFO_STR "INFO"
#define LOG_LEVEL_WARNING_STR "WARNING"
#define LOG_LEVEL_ERROR_STR "ERROR"

/* Error codes are defined in tecc_types.h */

/* Key types */
typedef enum TeccKeyType
{
    TECC_KEY_KEK,
    TECC_KEY_DEK,
    TECC_KEY_DEK_ENCRYPTED
} TeccKeyType;

/* Algorithm types are defined in tecc_types.h */

/* KMS types */
typedef enum KmsType
{
    KMS_TYPE_LOCAL,     /* Local file system KMS */
    KMS_TYPE_HSM,       /* Hardware Security Module (Tianzheng/同智伟业) */
    KMS_TYPE_CLOUD,     /* Cloud KMS */
    KMS_TYPE_HSM_SWXA   /* HSM - SanWeiXinAn/三未信安 SWXA */
} KmsType;

#endif /* TECC_COMMON_H */
