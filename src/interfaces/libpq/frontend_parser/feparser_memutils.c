/*
 * PostgreSQL密态数据库扩展 - 前端解析器内存管理实现
 *
 * 移植自openGauss前端解析器
 */

#include "feparser_memutils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/* Thread-local storage for memory vector */
#if defined(__GNUC__) || defined(__clang__)
#define TECC_THREAD_LOCAL __thread
#elif defined(_WIN32) && defined(_MSC_VER)
#define TECC_THREAD_LOCAL __declspec(thread)
#elif __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
#define TECC_THREAD_LOCAL _Thread_local
#else
#error "Thread-local storage is required. Platform lacks __thread, __declspec(thread), or _Thread_local support."
#endif

static TECC_THREAD_LOCAL void **g_vec = NULL;
static TECC_THREAD_LOCAL size_t g_vec_size = 0;
static const size_t g_resize_factor = 256;

/* 插入内存指针到向量中 */
static void insert_into_vector(void *tmp)
{
    if (!g_vec) {
        g_vec = (void **)malloc(g_resize_factor * sizeof(void *));
    } else if (g_vec_size % g_resize_factor == 0) {
        g_vec = (void **)realloc(g_vec, (g_vec_size + g_resize_factor) * sizeof(*g_vec));
    }
    if (g_vec == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(EXIT_FAILURE);
    }
    g_vec[g_vec_size] = tmp;
    g_vec_size++;
}

/*
 * "Safe" wrapper around strdup()
 */
char *feparser_strdup(const char *string)
{
    char *tmp = NULL;

    if (NULL == string) {
        fprintf(stderr, "feparser_strdup: cannot duplicate null pointer (internal error)\n");
        exit(EXIT_FAILURE);
    }
    tmp = strdup(string);
    if (NULL == tmp) {
        fprintf(stderr, "out of memory\n");
        exit(EXIT_FAILURE);
    }
    insert_into_vector(tmp);
    return tmp;
}

/*
 * "Safe" wrapper around strndup()
 */
char *feparser_strndup(const char *string, const size_t n)
{
    char *tmp = NULL;
    if (NULL == string) {
        fprintf(stderr, "feparser_strndup: cannot duplicate null pointer (internal error)\n");
        exit(EXIT_FAILURE);
    }
#ifdef _MSC_VER
    {
        size_t len = (n < strlen(string)) ? n : strlen(string);
        tmp = (char *)malloc(len + 1);
        if (tmp) {
            memcpy(tmp, string, len);
            tmp[len] = '\0';
        }
    }
#else
    tmp = strndup(string, n);
#endif
    if (NULL == tmp) {
        fprintf(stderr, "out of memory\n");
        exit(EXIT_FAILURE);
    }
    insert_into_vector(tmp);
    return tmp;
}

void *feparser_malloc(size_t size)
{
    void *tmp = NULL;

    /* Avoid unportable behavior of malloc(0) */
    if (size == 0) {
        size = 1;
    }
    tmp = malloc(size);
    if (NULL == tmp) {
        fprintf(stderr, "out of memory\n");
        exit(EXIT_FAILURE);
    }
    insert_into_vector(tmp);
    return tmp;
}

void *feparser_malloc0(size_t size)
{
    void *tmp = NULL;

    tmp = feparser_malloc(size);
    memset(tmp, 0, size);
    return tmp;
}

void *feparser_malloc_extended(size_t size, int flags)
{
    /* 简化实现，忽略flags参数 */
    return feparser_malloc(size);
}

void *feparser_realloc(void *ptr, size_t size)
{
    void *tmp = NULL;

    /* When realloc failed gsql will exit, with no memory leak for ptr. */
    tmp = (void *)realloc(ptr, size);
    if (NULL == tmp) {
        fprintf(stderr, "out of memory\n");
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < g_vec_size; i++) {
        if (g_vec[i] == ptr) {
            g_vec[i] = tmp;
        }
    }
    return tmp;
}

void feparser_free(void *pointer)
{
    for (size_t i = 0; i < g_vec_size; i++) {
        if (g_vec[i] == pointer) {
            g_vec[i] = g_vec[g_vec_size - 1];
            free(pointer);
            g_vec_size--;
            if (g_vec_size % g_resize_factor == 0) {
                g_vec = (void **)realloc(g_vec, g_vec_size * sizeof(*g_vec));
                if (g_vec == NULL) {
                    return;
                }
            }
        }
    }
}

void free_memory(void)
{
    for (size_t i = 0; i < g_vec_size; i++) {
        free(g_vec[i]);
    }
    g_vec_size = 0;
    free(g_vec);
    g_vec = NULL;
}