/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_buffer_manager.h
 *
 * Abstract:
 *   Buffer management interfaces for TECC data formatter module.
 *   Provides efficient buffer allocation and management services.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_buffer_manager.h
 */

#ifndef TECC_BUFFER_MANAGER_H
#define TECC_BUFFER_MANAGER_H

#include "tecc_formatter.h"
#include "tecc_formatter_internal.h"

/*
 * Buffer pool operations
 */
extern TeccFormatterBufferPool *tecc_buffer_manager_create_pool(
    int capacity
);

extern void *tecc_buffer_manager_alloc(
    TeccFormatterBufferPool *pool,
    size_t size
);

extern bool tecc_buffer_manager_free(
    TeccFormatterBufferPool *pool,
    void *buffer
);

extern void tecc_buffer_manager_destroy_pool(
    TeccFormatterBufferPool *pool
);

/*
 * Direct buffer allocation
 */
extern void *tecc_buffer_manager_alloc_direct(
    size_t size
);

extern void tecc_buffer_manager_free_direct(
    void *buffer
);

/*
 * Buffer statistics
 */
extern size_t tecc_buffer_manager_get_pool_stats(
    TeccFormatterBufferPool *pool,
    size_t *total_allocated,
    size_t *total_used,
    int *active_buffers
);

#endif /* TECC_BUFFER_MANAGER_H */