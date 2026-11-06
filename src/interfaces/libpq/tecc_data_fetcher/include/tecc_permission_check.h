/* Begin - ReqID:TECC4.0 */
/*-------------------------------------------------------------------------
 *
 * tecc_permission_check.h
 *    TECC permission checking functions
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/interfaces/libpq/tecc_data_fetcher/include/tecc_permission_check.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_PERMISSION_CHECK_H
#define TECC_PERMISSION_CHECK_H

#include "postgres_fe.h"
#include "tecc_data_fetcher.h"

/* 权限检查函数 */

/* 检查KEK权限 */
extern TeccFetcherErrorCode tecc_check_kek_permission(const TeccState *state,
                                                      const char *key_name,
                                                      const char *key_namespace);

/* 检查DEK权限 */
extern TeccFetcherErrorCode tecc_check_dek_permission(const TeccState *state,
                                                      const char *key_name,
                                                      const char *key_namespace);

/* 获取当前用户名 */
extern const char *tecc_get_current_user(const TeccState *state);

/* 权限错误处理 */
extern void tecc_log_permission_error(const TeccState *state,
                                      const char *resource_type, const char *resource_name);

#endif /* TECC_PERMISSION_CHECK_H */
/* End - ReqID:TECC4.0 */