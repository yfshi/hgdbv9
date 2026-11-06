/*-------------------------------------------------------------------------
 *
 * tecc_key_adapter.h
 *    Header adapter for TECC key management module in test environment
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/interfaces/libpq/tecc_test/test_key_manager/tecc_key_adapter.h
 *
 * ReqID:TECC4.0 - Key management test adapter
 * Authored by <wanghaoyan@highgo.com>, 20260413.
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_KEY_ADAPTER_H
#define TECC_KEY_ADAPTER_H

/* PostgreSQL frontend header - must be included first */
#include "postgres_fe.h"

/* Mock definitions for backend-only headers */
#ifndef FRONTEND
#define FRONTEND 1
#endif

/* Include key management headers with adapted paths */
#include "common/tecc_key/high_level_api.h"
#include "common/tecc_key/key_manager.h"
#include "common/tecc_key/kms_adapter.h"
#include "common/tecc_key/server_interface.h"
#include "common/tecc_key/utils.h"
#include "common/tecc_key/config.h"

/* Mock backend memory context functions for frontend testing */
#ifndef MemoryContext
typedef void *MemoryContext;
#endif

#ifndef CurrentMemoryContext
extern MemoryContext CurrentMemoryContext;
#endif

#ifndef AllocSetContextCreate
static inline MemoryContext
AllocSetContextCreate(MemoryContext parent,
                      const char *name,
                      int flags)
{
    (void)parent; (void)name; (void)flags;
    return (MemoryContext)malloc(1);
}
#endif

#ifndef MemoryContextAlloc
static inline void *
MemoryContextAlloc(MemoryContext context, Size size)
{
    (void)context;
    return malloc(size);
}
#endif

#ifndef MemoryContextAllocZero
static inline void *
MemoryContextAllocZero(MemoryContext context, Size size)
{
    (void)context;
    void *ptr = malloc(size);
    if (ptr)
        memset(ptr, 0, size);
    return ptr;
}
#endif

#ifndef palloc
#define palloc(size) malloc(size)
#endif

#ifndef palloc0
#define palloc0(size) ({ void *ptr = malloc(size); if (ptr) memset(ptr, 0, size); ptr; })
#endif

#ifndef pfree
#define pfree(ptr) free(ptr)
#endif

#ifndef pstrdup
#define pstrdup(str) strdup(str)
#endif

/* Mock elog for testing */
#ifndef elog
#define elog(level, ...) \
    do { \
        if (level >= ERROR) { \
            fprintf(stderr, "ERROR: " __VA_ARGS__); \
            fprintf(stderr, "\n"); \
        } \
    } while(0)
#endif

/* Mock ereport for testing */
#ifndef ereport
#define ereport(level, ...) \
    do { \
        if (level >= ERROR) { \
            fprintf(stderr, "ERROR: "); \
            fprintf(stderr, __VA_ARGS__); \
            fprintf(stderr, "\n"); \
        } \
    } while(0)
#endif

#endif /* TECC_KEY_ADAPTER_H */