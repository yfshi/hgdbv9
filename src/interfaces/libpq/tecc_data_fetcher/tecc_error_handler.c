/* Begin - ReqID:TECC4.0 */
/*-------------------------------------------------------------------------
 *
 * tecc_error_handler.c
 *    TECC error handling implementation
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/interfaces/libpq/tecc_data_fetcher/src/tecc_error_handler.c
 *
 *-------------------------------------------------------------------------
 */
#include "tecc_error_handler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ReqID:TECC4.0 - 线程局部存储宏：MSVC 用 __declspec(thread)，GCC/Clang 用 __thread，
 * 与 tecc_parser.c / tecc_metadata_cache.h 的 TECC_THREAD_LOCAL 保持一致。 */
#ifndef TECC_THREAD_LOCAL
#if defined(__GNUC__) || defined(__clang__)
#define TECC_THREAD_LOCAL __thread
#elif defined(_WIN32) && defined(_MSC_VER)
#define TECC_THREAD_LOCAL __declspec(thread)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
#define TECC_THREAD_LOCAL _Thread_local
#else
#error "Thread-local storage is required for TECC error handler."
#endif
#endif

/* 错误上下文（线程局部，多线程隔离） */
static TECC_THREAD_LOCAL char *error_context = NULL;
static TECC_THREAD_LOCAL char *detailed_error = NULL;

/* 错误统计（线程局部，多线程隔离） */
static TECC_THREAD_LOCAL int error_counts[TECC_FETCHER_ERROR_INVALID_PARAM + 1];

/*
 * tecc_set_error_context - 设置错误上下文
 *
 * context: 上下文信息
 */
void
tecc_set_error_context(const char *context)
{
    if (error_context != NULL)
    {
        pg_free(error_context);
        error_context = NULL;
    }

    if (context != NULL)
    {
        error_context = pg_strdup(context);
    }
}

/*
 * tecc_get_error_context - 获取错误上下文
 *
 * 返回: 上下文信息
 */
const char *
tecc_get_error_context(void)
{
    return error_context;
}

/*
 * tecc_convert_libpq_error - 转换libpq错误码
 *
 * libpq_error: libpq错误码
 * 返回: TECC错误码
 */
TeccFetcherErrorCode
tecc_convert_libpq_error(int libpq_error)
{
    /* TODO: 实现libpq错误码到TECC错误码的映射 */
    switch (libpq_error)
    {
        case 0: /* CONNECTION_OK */
            return TECC_FETCHER_OK;
        default:
            return TECC_FETCHER_ERROR_CONNECTION;
    }
}

/*
 * tecc_convert_sql_state - 转换SQL状态码
 *
 * sql_state: SQL状态码
 * 返回: TECC错误码
 */
TeccFetcherErrorCode
tecc_convert_sql_state(const char *sql_state)
{
    if (sql_state == NULL)
    {
        return TECC_FETCHER_ERROR_QUERY;
    }

    /* 检查权限错误 */
    if (strncmp(sql_state, "42501", 5) == 0) /* insufficient_privilege */
    {
        return TECC_FETCHER_ERROR_PERMISSION;
    }

    /* 检查语法错误 */
    if (strncmp(sql_state, "42601", 5) == 0) /* syntax_error */
    {
        return TECC_FETCHER_ERROR_QUERY;
    }

    /* 默认返回查询错误 */
    return TECC_FETCHER_ERROR_QUERY;
}

/*
 * tecc_set_detailed_error - 设置详细错误信息
 *
 * message: 错误消息
 */
void
tecc_set_detailed_error(const char *message)
{
    if (detailed_error != NULL)
    {
        pg_free(detailed_error);
        detailed_error = NULL;
    }

    if (message != NULL)
    {
        detailed_error = pg_strdup(message);
    }
}

/*
 * tecc_get_detailed_error - 获取详细错误信息
 *
 * 返回: 错误消息
 */
const char *
tecc_get_detailed_error(void)
{
    return detailed_error;
}

/*
 * tecc_increment_error_count - 增加错误计数
 *
 * error_code: 错误码
 */
void
tecc_increment_error_count(TeccFetcherErrorCode error_code)
{
    if (error_code >= 0 && error_code <= TECC_FETCHER_ERROR_INVALID_PARAM)
    {
        error_counts[error_code]++;
    }
}

/*
 * tecc_get_error_count - 获取错误计数
 *
 * error_code: 错误码
 * 返回: 错误次数
 */
int
tecc_get_error_count(TeccFetcherErrorCode error_code)
{
    if (error_code >= 0 && error_code <= TECC_FETCHER_ERROR_INVALID_PARAM)
    {
        return error_counts[error_code];
    }
    return 0;
}

/*
 * tecc_reset_error_stats - 重置错误统计
 */
void
tecc_reset_error_stats(void)
{
    int i;
    for (i = 0; i <= TECC_FETCHER_ERROR_INVALID_PARAM; i++)
    {
        error_counts[i] = 0;
    }
}
/* End - ReqID:TECC4.0 */