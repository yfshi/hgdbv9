/* Begin - ReqID:TECC4.0 */
/*-------------------------------------------------------------------------
 *
 * tecc_libpq_cursor.c
 *    TECC libpq cursor implementation
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/interfaces/libpq/tecc_data_fetcher/src/tecc_libpq_cursor.c
 *
 *-------------------------------------------------------------------------
 */
#include "tecc_libpq_cursor.h"
#include "tecc_error_handler.h"
#include "tecc_data_fetcher.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
 * tecc_libpq_create_cursor - 创建libpq游标
 *
 * factory: 数据获取器工厂
 * 返回: 游标接口指针
 */
TeccCursorInterface *
tecc_libpq_create_cursor(PGconn *conn)
{
    TeccLibPQCursor *cursor;

    if (conn == NULL)
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_INVALID_PARAM,
                               "NULL connection in create_cursor");
        return NULL;
    }

    cursor = (TeccLibPQCursor *)pg_malloc(sizeof(TeccLibPQCursor));
    if (cursor == NULL)
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_MEMORY,
                               "Failed to allocate cursor");
        return NULL;
    }

    /* 初始化基础接口 */
    cursor->base.cursor_open = tecc_libpq_cursor_open;
    cursor->base.fetch_next = tecc_libpq_cursor_fetch_next;
    cursor->base.get_column = tecc_libpq_cursor_get_column;
    cursor->base.get_column_count = tecc_libpq_cursor_get_column_count;
    cursor->base.get_column_name = tecc_libpq_cursor_get_column_name;
    cursor->base.next = tecc_libpq_cursor_fetch_next; /* next是fetch_next的别名 */
    cursor->base.close = tecc_libpq_cursor_close;
    cursor->base.destroy = tecc_libpq_cursor_destroy;
    cursor->base.private_data = cursor;

    /* 初始化libpq特定字段 */
    cursor->conn = conn;
    cursor->result = NULL;
    cursor->current_row = -1;
    cursor->row_count = 0;
    cursor->column_count = 0;
    cursor->conn_is_temp = false;

    return (TeccCursorInterface *)cursor;
}

/*
 * tecc_libpq_execute_query - 执行查询并返回游标
 *
 * factory: 数据获取器工厂
 * query: SQL查询语句
 * 返回: 游标接口指针
 */
TeccCursorInterface *
tecc_libpq_execute_query(PGconn *conn, const char *query)
{
    TeccCursorInterface *cursor;
    int result;

    if (conn == NULL || query == NULL)
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_INVALID_PARAM,
                               "NULL parameter in execute_query");
        return NULL;
    }

    /* 创建游标 */
    cursor = tecc_libpq_create_cursor(conn);
    if (cursor == NULL)
    {
        return NULL;
    }

    /* 打开游标（执行查询） */
    result = cursor->cursor_open(cursor, query);
    if (result != TECC_FETCHER_OK)
    {
        cursor->destroy(cursor);
        return NULL;
    }

    return cursor;
}

/*
 * tecc_libpq_get_conninfo - 获取连接信息
 *
 * factory: 数据获取器工厂
 * 返回: 连接信息字符串
 */
const char *
tecc_libpq_get_conninfo(PGconn *conn)
{
    if (conn == NULL)
    {
        return NULL;
    }

    /* PQconninfo()返回PQconninfoOption*链表，不是字符串
     * 这里返回NULL，因为实际使用中可能不需要这个函数
     * 如果需要连接信息，应该使用其他方式获取
     */
    return NULL;
}

/*
 * tecc_libpq_cursor_open - 打开游标（执行查询）
 *
 * cursor: 游标接口
 * query: SQL查询语句
 * 返回: 错误码
 */
int
tecc_libpq_cursor_open(TeccCursorInterface *cursor, const char *query)
{
    TeccLibPQCursor *libpq_cursor;
    PGconn *conn;

    if (cursor == NULL || query == NULL)
    {
        return TECC_FETCHER_ERROR_INVALID_PARAM;
    }

    libpq_cursor = (TeccLibPQCursor *)cursor->private_data;

    /* 获取连接 */

    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    {
    ConnStatusType conn_status;
    ExecStatusType result_status;

    /* 获取连接 */
    conn = libpq_cursor->conn;
    if (conn == NULL)
    {
        return TECC_FETCHER_ERROR_INVALID_PARAM;
    }

    /* 验证连接状态 */
    conn_status = PQstatus(conn);
    if (conn_status != CONNECTION_OK)
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_CONNECTION,
                               PQerrorMessage(conn));
        return TECC_FETCHER_ERROR_CONNECTION;
    }

    /* 执行查询 */
    TECC_LOG(TECC_LOG_DEBUG1, "PQexec query='%s'", query);
    libpq_cursor->result = PQexec(conn, query);

    if (libpq_cursor->result == NULL)
    {
        TECC_LOG(TECC_LOG_DEBUG1, "PQexec returned NULL");
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_QUERY,
                               PQerrorMessage(conn));
        return TECC_FETCHER_ERROR_QUERY;
    }

    result_status = PQresultStatus(libpq_cursor->result);
    TECC_LOG(TECC_LOG_DEBUG1, "PQresultStatus=%d, ntuples=%d, error='%s'",
             result_status, PQntuples(libpq_cursor->result),
             PQresultErrorMessage(libpq_cursor->result));
    if (result_status != PGRES_TUPLES_OK)
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_QUERY,
                               PQerrorMessage(conn));
        PQclear(libpq_cursor->result);
        libpq_cursor->result = NULL;
        return TECC_FETCHER_ERROR_QUERY;
    }

    /* 初始化游标状态 */
    libpq_cursor->row_count = PQntuples(libpq_cursor->result);
    libpq_cursor->column_count = PQnfields(libpq_cursor->result);
    libpq_cursor->current_row = -1; /* 在第一个fetch之前 */

    return TECC_FETCHER_OK;
    }
}

/*
 * tecc_libpq_cursor_fetch_next - 获取下一行
 *
 * cursor: 游标接口
 * 返回: 错误码
 */
int
tecc_libpq_cursor_fetch_next(TeccCursorInterface *cursor)
{
    TeccLibPQCursor *libpq_cursor;

    if (cursor == NULL)
    {
        return TECC_FETCHER_ERROR_INVALID_PARAM;
    }

    libpq_cursor = (TeccLibPQCursor *)cursor->private_data;

    if (libpq_cursor->result == NULL)
    {
        return TECC_FETCHER_ERROR_INVALID_PARAM;
    }

    /* 移动到下一行 */
    libpq_cursor->current_row++;

    /* 检查是否还有更多行 */
    if (libpq_cursor->current_row >= libpq_cursor->row_count)
    {
        return TECC_FETCHER_ERROR_PARSE; /* 没有更多行 */
    }

    return TECC_FETCHER_OK;
}

/*
 * tecc_libpq_cursor_get_column - 获取当前行的列值
 *
 * cursor: 游标接口
 * column_index: 列索引
 * 返回: 列值字符串
 */
const char *
tecc_libpq_cursor_get_column(TeccCursorInterface *cursor, int column_index)
{
    TeccLibPQCursor *libpq_cursor;

    if (cursor == NULL)
    {
        return NULL;
    }

    libpq_cursor = (TeccLibPQCursor *)cursor->private_data;

    /* 参数检查 */
    if (libpq_cursor->result == NULL ||
        libpq_cursor->current_row < 0 ||
        libpq_cursor->current_row >= libpq_cursor->row_count ||
        column_index < 0 ||
        column_index >= libpq_cursor->column_count)
    {
        return NULL;
    }

    return PQgetvalue(libpq_cursor->result,
                      libpq_cursor->current_row,
                      column_index);
}

/*
 * tecc_libpq_cursor_get_column_count - 获取列数
 *
 * cursor: 游标接口
 * 返回: 列数
 */
int
tecc_libpq_cursor_get_column_count(TeccCursorInterface *cursor)
{
    TeccLibPQCursor *libpq_cursor;

    if (cursor == NULL)
    {
        return 0;
    }

    libpq_cursor = (TeccLibPQCursor *)cursor->private_data;
    return libpq_cursor->column_count;
}

/*
 * tecc_libpq_cursor_get_column_name - 获取列名
 *
 * cursor: 游标接口
 * column_index: 列索引
 * 返回: 列名字符串
 */
const char *
tecc_libpq_cursor_get_column_name(TeccCursorInterface *cursor, int column_index)
{
    TeccLibPQCursor *libpq_cursor;

    if (cursor == NULL)
    {
        return NULL;
    }

    libpq_cursor = (TeccLibPQCursor *)cursor->private_data;

    /* 参数检查 */
    if (libpq_cursor->result == NULL ||
        column_index < 0 ||
        column_index >= libpq_cursor->column_count)
    {
        return NULL;
    }

    return PQfname(libpq_cursor->result, column_index);
}

/*
 * tecc_libpq_cursor_close - 关闭游标
 *
 * cursor: 游标接口
 */
void
tecc_libpq_cursor_close(TeccCursorInterface *cursor)
{
    TeccLibPQCursor *libpq_cursor;

    if (cursor == NULL)
    {
        return;
    }

    libpq_cursor = (TeccLibPQCursor *)cursor->private_data;

    /* 清理查询结果 */
    if (libpq_cursor->result != NULL)
    {
        PQclear(libpq_cursor->result);
        libpq_cursor->result = NULL;
    }

    /* 重置状态 */
    libpq_cursor->current_row = -1;
    libpq_cursor->row_count = 0;
    libpq_cursor->column_count = 0;
}

/*
 * tecc_libpq_cursor_destroy - 销毁游标
 *
 * cursor: 游标接口
 */
void
tecc_libpq_cursor_destroy(TeccCursorInterface *cursor)
{
    TeccLibPQCursor *libpq_cursor;

    if (cursor == NULL)
    {
        return;
    }

    libpq_cursor = (TeccLibPQCursor *)cursor->private_data;

    /* 关闭游标（清理result） */
    cursor->close(cursor);

    /* 临时连接需要关闭，主连接不做操作 */
    if (libpq_cursor->conn_is_temp && libpq_cursor->conn != NULL)
    {
        tecc_close_temp_connection(libpq_cursor->conn, "cursor_destroy");
    }

    /* 将连接指针设为NULL（连接已归还或由外部管理） */
    libpq_cursor->conn = NULL;

    /* 释放游标结构 */
    pg_free(libpq_cursor);
}
/* End - ReqID:TECC4.0 */