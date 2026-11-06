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

/* 错误上下文 */
static char *error_context = NULL;
static char *detailed_error = NULL;

/* 错误统计 */
static int error_counts[TECC_FETCHER_ERROR_INVALID_PARAM + 1] = {0};

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

/* 以下函数是存根实现，供未来扩展 */

void
tecc_push_error_stack(const char *function_name, TeccFetcherErrorCode error_code)
{
    (void)function_name;
    (void)error_code;
    /* TODO: 实现错误堆栈 */
}

void
tecc_pop_error_stack(void)
{
    /* TODO: 实现错误堆栈 */
}

void
tecc_clear_error_stack(void)
{
    /* TODO: 实现错误堆栈 */
}

void
tecc_print_error_stack(void)
{
    /* TODO: 实现错误堆栈 */
}

bool
tecc_can_recover_error(TeccFetcherErrorCode error_code)
{
    /* 某些错误可以恢复，如连接错误 */
    switch (error_code)
    {
        case TECC_FETCHER_ERROR_CONNECTION:
        case TECC_FETCHER_ERROR_TIMEOUT:
            return true;
        default:
            return false;
    }
}

TeccFetcherErrorCode
tecc_try_recover(TeccFetcherErrorCode error_code)
{
    /* TODO: 实现错误恢复逻辑 */
    return error_code;
}

void
tecc_set_error_log_level(int level)
{
    (void)level;
    /* TODO: 实现日志级别设置 */
}

void
tecc_enable_error_logging(bool enable)
{
    (void)enable;
    /* TODO: 实现日志启用/禁用 */
}
/* End - ReqID:TECC4.0 */