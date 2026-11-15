/* Begin - ReqID:TECC4.0 */
/*-------------------------------------------------------------------------
 *
 * tecc_error_handler.h
 *    TECC error handling functions
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/interfaces/libpq/tecc_data_fetcher/include/tecc_error_handler.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_ERROR_HANDLER_H
#define TECC_ERROR_HANDLER_H

#include "tecc_data_fetcher.h"

/* 错误处理函数 */

/* 设置错误上下文 */
extern void tecc_set_error_context(const char *context);
extern const char *tecc_get_error_context(void);

/* 错误码转换 */
extern TeccFetcherErrorCode tecc_convert_libpq_error(int libpq_error);
extern TeccFetcherErrorCode tecc_convert_sql_state(const char *sql_state);

/* 详细错误信息 */
extern void tecc_set_detailed_error(const char *message);
extern const char *tecc_get_detailed_error(void);

/* 错误统计 */
extern void tecc_increment_error_count(TeccFetcherErrorCode error_code);
extern int tecc_get_error_count(TeccFetcherErrorCode error_code);
extern void tecc_reset_error_stats(void);

#endif /* TECC_ERROR_HANDLER_H */
/* End - ReqID:TECC4.0 */