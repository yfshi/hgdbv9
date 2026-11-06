/* Begin - ReqID:TECC4.0 */
/*-------------------------------------------------------------------------
 *
 * tecc_libpq_cursor.h
 *    TECC libpq cursor interface
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/interfaces/libpq/tecc_data_fetcher/include/tecc_libpq_cursor.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_LIBPQ_CURSOR_H
#define TECC_LIBPQ_CURSOR_H

#include "tecc_data_fetcher_types.h"

/* libpq游标函数声明 */

/* 创建libpq游标 */
extern TeccCursorInterface *tecc_libpq_create_cursor(PGconn *conn);

/* 执行查询并返回游标 */
extern TeccCursorInterface *tecc_libpq_execute_query(PGconn *conn,
                                                     const char *query);

/* 获取连接信息 */
extern const char *tecc_libpq_get_conninfo(PGconn *conn);

/* libpq游标具体实现函数 */
extern int tecc_libpq_cursor_open(TeccCursorInterface *cursor, const char *query);
extern int tecc_libpq_cursor_fetch_next(TeccCursorInterface *cursor);
extern const char *tecc_libpq_cursor_get_column(TeccCursorInterface *cursor, int column_index);
extern int tecc_libpq_cursor_get_column_count(TeccCursorInterface *cursor);
extern const char *tecc_libpq_cursor_get_column_name(TeccCursorInterface *cursor, int column_index);
extern void tecc_libpq_cursor_close(TeccCursorInterface *cursor);
extern void tecc_libpq_cursor_destroy(TeccCursorInterface *cursor);

#endif /* TECC_LIBPQ_CURSOR_H */
/* End - ReqID:TECC4.0 */