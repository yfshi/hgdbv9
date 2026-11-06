/* Begin - ReqID:TECC4.0 */
/*
 * File: feparser_memutils.h
 * Abstract:
 *   Memory management utilities for TECC frontend parser.
 *
 * Authored by <wangxp@highgo.com>, 20260306.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/frontend_parser/feparser_memutils.h
 */

#ifndef FEPARSER_MEMUTILS_H
#define FEPARSER_MEMUTILS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef FRONTEND
/*
 * "Safe" memory allocation functions
 */
char *feparser_strdup(const char *in);
char *feparser_strndup(const char *in, size_t n);
void *feparser_malloc(size_t size);
void *feparser_malloc0(size_t size);
void *feparser_malloc_extended(size_t size, int flags);
void *feparser_realloc(void *pointer, size_t size);
void feparser_free(void *pointer);
void feparser_free_all(void);  /* Free all tracked memory */
void free_memory(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* FEPARSER_MEMUTILS_H */
/* End - ReqID:TECC4.0 */