/*-------------------------------------------------------------------------
 *
 * tecc_mem.h
 *    Memory allocation wrapper for TECC key management module
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/common/tecc_key/tecc_mem.h
 *
 * ReqID:TECC4.0 - Memory allocation wrapper
 * Authored by <Claude Code>, 20260415.
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_MEM_H
#define TECC_MEM_H

#include "c.h"

/*
 * This module provides a thin wrapper around frontend memory allocators.
 * It's designed to make it clear that this module uses frontend allocators
 * and to provide a consistent interface.
 *
 * IMPORTANT: This is for FRONTEND use only.
 */

#ifdef FRONTEND

#include "common/fe_memutils.h"

/* Direct wrapper functions - just redirect to frontend allocators */
static inline void *
tecc_mem_alloc(size_t size)
{
    return pg_malloc(size);
}

static inline void *
tecc_mem_alloc0(size_t size)
{
    return pg_malloc0(size);
}

static inline void *
tecc_mem_realloc(void *ptr, size_t size)
{
    return pg_realloc(ptr, size);
}

static inline void
tecc_mem_free(void *ptr)
{
    pg_free(ptr);
}

static inline char *
tecc_mem_strdup(const char *str)
{
    return pg_strdup(str);
}

static inline void
tecc_mem_secure_erase(void *data, size_t length)
{
    if (data != NULL && length > 0)
    {
        /* Simple volatile pointer loop to prevent compiler optimization */
        volatile unsigned char *p = (volatile unsigned char *)data;
        while (length--)
            *p++ = 0;
    }
}

/* Type-safe allocation macros */
#define tecc_mem_alloc_object(type) \
    ((type *)tecc_mem_alloc(sizeof(type)))

#define tecc_mem_alloc0_object(type) \
    ((type *)tecc_mem_alloc0(sizeof(type)))

#define tecc_mem_alloc_array(type, count) \
    ((type *)tecc_mem_alloc(sizeof(type) * (count)))

#define tecc_mem_alloc0_array(type, count) \
    ((type *)tecc_mem_alloc0(sizeof(type) * (count)))

#else /* !FRONTEND */

/*
 * If included by backend code, generate a compile error with a clear message
 */
#error "tecc_mem.h should only be included by frontend code. \
If backend needs key management, create a separate backend implementation."

#endif /* FRONTEND */

#endif /* TECC_MEM_H */