/* Begin - ReqID:TECC4.0 */
/*-------------------------------------------------------------------------
 *
 * tecc_key_args_parser.h
 *    TECC key arguments parsing functions
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/interfaces/libpq/tecc_data_fetcher/include/tecc_key_args_parser.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_KEY_ARGS_PARSER_H
#define TECC_KEY_ARGS_PARSER_H

#include "tecc_data_fetcher_types.h"
#include "tecc_data_fetcher.h"

/* 密钥参数解析函数 */

/* 解析KEK参数 */
extern TeccKeyArg *tecc_parse_kek_args(const TeccState *state, TeccCursorInterface *cursor, int *count);

/* 解析DEK参数 */
extern TeccKeyArg *tecc_parse_dek_args(const TeccState *state, TeccCursorInterface *cursor, int *count);

/* 释放参数数组 */
extern void tecc_free_key_args(TeccKeyArg *args, int count);

/* 查找特定参数 */
extern const char *tecc_find_key_arg(TeccKeyArg *args, int count,
                                     const char *function_name,
                                     const char *key);

/* 验证参数格式 */
extern bool tecc_validate_key_args(TeccKeyArg *args, int count);

#endif /* TECC_KEY_ARGS_PARSER_H */
/* End - ReqID:TECC4.0 */