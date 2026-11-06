/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_buffer_manager.c
 *
 * Abstract:
 *   Buffer management implementation for TECC data formatter module.
 *   Provides efficient buffer allocation and management services.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_buffer_manager.c
 */

#include "tecc_buffer_manager.h"
#include "common/fe_memutils.h"  /* For pg_malloc, pg_free */

#include <string.h>

/*
 * tecc_buffer_manager_create_pool - Create buffer pool
 *
 * Creates a new buffer pool for efficient memory management.
 */
TeccFormatterBufferPool *
tecc_buffer_manager_create_pool(
    int capacity)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    TeccFormatterBufferPool *pool;

    if (capacity <= 0)
        return NULL;

    /* Allocate pool structure */
    pool = pg_malloc(sizeof(TeccFormatterBufferPool));
    if (!pool)
        return NULL;

    /* Initialize pool */
    pool->count = 0;
    pool->capacity = capacity;
    pool->total_allocated = 0;
    pool->total_used = 0;

    /* Allocate arrays */
    pool->buffers = pg_malloc(sizeof(void *) * capacity);
    pool->sizes = pg_malloc(sizeof(size_t) * capacity);
    pool->in_use = pg_malloc0(sizeof(bool) * capacity);

    if (!pool->buffers || !pool->sizes || !pool->in_use) {
        /* Cleanup on failure */
        if (pool->buffers) pg_free(pool->buffers);
        if (pool->sizes) pg_free(pool->sizes);
        if (pool->in_use) pg_free(pool->in_use);
        pg_free(pool);
        return NULL;
    }

    return pool;
}

/*
 * tecc_buffer_manager_alloc - Allocate buffer from pool
 *
 * Allocates a buffer from the pool, reusing existing buffers when possible.
 */
void *
tecc_buffer_manager_alloc(
    TeccFormatterBufferPool *pool,
    size_t size)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    int i;

    if (!pool || size == 0)
        return NULL;

    /* Try to find a reusable buffer */
    for (i = 0; i < pool->count; i++) {
        if (!pool->in_use[i] && pool->sizes[i] >= size) {
            pool->in_use[i] = true;
            pool->total_used += size;
            return pool->buffers[i];
        }
    }

    /* No suitable buffer found, allocate new one */
    if (pool->count < pool->capacity) {
        void *buffer = pg_malloc(size);
        if (buffer) {
            pool->buffers[pool->count] = buffer;
            pool->sizes[pool->count] = size;
            pool->in_use[pool->count] = true;
            pool->count++;
            pool->total_allocated += size;
            pool->total_used += size;
            return buffer;
        }
    }

    /* Pool full or allocation failed, use direct allocation */
    return tecc_buffer_manager_alloc_direct(size);
}

/*
 * tecc_buffer_manager_free - Free buffer to pool
 *
 * Returns a buffer to the pool for reuse.
 */
bool
tecc_buffer_manager_free(
    TeccFormatterBufferPool *pool,
    void *buffer)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    int i;

    if (!pool || !buffer)
        return false;

    /* Check if buffer belongs to pool */
    for (i = 0; i < pool->count; i++) {
        if (pool->buffers[i] == buffer && pool->in_use[i]) {
            pool->in_use[i] = false;
            pool->total_used -= pool->sizes[i];
            return true;
        }
    }

    /* Buffer not in pool, free directly */
    tecc_buffer_manager_free_direct(buffer);
    return true;
}

/*
 * tecc_buffer_manager_destroy_pool - Destroy buffer pool
 *
 * Destroys a buffer pool and frees all associated memory.
 */
void
tecc_buffer_manager_destroy_pool(
    TeccFormatterBufferPool *pool)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    int i;

    if (!pool)
        return;

    /* Free all buffers in pool */
    for (i = 0; i < pool->count; i++) {
        if (pool->buffers[i])
            pg_free(pool->buffers[i]);
    }

    /* Free pool arrays */
    if (pool->buffers)
        pg_free(pool->buffers);
    if (pool->sizes)
        pg_free(pool->sizes);
    if (pool->in_use)
        pg_free(pool->in_use);

    /* Free pool structure */
    pg_free(pool);
}

/*
 * tecc_buffer_manager_alloc_direct - Direct buffer allocation
 *
 * Allocates a buffer directly without using the pool.
 */
void *
tecc_buffer_manager_alloc_direct(
    size_t size)
{
    if (size == 0)
        return NULL;

    return pg_malloc(size);
}

/*
 * tecc_buffer_manager_free_direct - Direct buffer deallocation
 *
 * Frees a buffer that was allocated directly.
 */
void
tecc_buffer_manager_free_direct(
    void *buffer)
{
    if (buffer)
        pg_free(buffer);
}

/*
 * tecc_buffer_manager_get_pool_stats - Get pool statistics
 *
 * Retrieves statistics about buffer pool usage.
 */
size_t
tecc_buffer_manager_get_pool_stats(
    TeccFormatterBufferPool *pool,
    size_t *total_allocated,
    size_t *total_used,
    int *active_buffers)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    int active = 0;
    int i;

    if (!pool)
        return 0;
    for (i = 0; i < pool->count; i++) {
        if (pool->in_use[i])
            active++;
    }

    if (total_allocated)
        *total_allocated = pool->total_allocated;

    if (total_used)
        *total_used = pool->total_used;

    if (active_buffers)
        *active_buffers = active;

    return pool->total_allocated - pool->total_used;  /* Free space */
}

/* End - ReqID:TECC4.0 */