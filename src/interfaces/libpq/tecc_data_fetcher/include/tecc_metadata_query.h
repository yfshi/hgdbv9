/* Begin - ReqID:TECC4.0 */
/*-------------------------------------------------------------------------
 *
 * tecc_metadata_query.h
 *    TECC metadata query parsing functions
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/interfaces/libpq/tecc_data_fetcher/include/tecc_metadata_query.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_METADATA_QUERY_H
#define TECC_METADATA_QUERY_H

#include "tecc_data_fetcher_types.h"
#include "tecc_data_fetcher.h"

/* 元数据查询解析函数 */

/* 解析KEK元数据 */
extern TeccFetcherErrorCode tecc_parse_kek_metadata(const TeccState *state,
                                                    TeccCursorInterface *cursor,
                                                    TeccKekMetadata **result);

/* 解析DEK元数据 */
extern TeccFetcherErrorCode tecc_parse_dek_metadata(const TeccState *state,
                                                    TeccCursorInterface *cursor,
                                                    TeccDekMetadata **result);

/* 解析加密列元数据 */
extern TeccFetcherErrorCode tecc_parse_encrypted_column_metadata(
    TeccCursorInterface *cursor,
    TeccEncryptedColumnMetadata **result);


/* 验证查询结果结构 */
extern bool tecc_validate_kek_result(TeccCursorInterface *cursor);
extern bool tecc_validate_dek_result(TeccCursorInterface *cursor);
extern bool tecc_validate_column_result(TeccCursorInterface *cursor);

/* 批量解析函数 - 数据获取器负责解析原始查询结果为结构化元数据对象列表 */
extern TeccFetcherErrorCode tecc_parse_kek_list(const TeccState *state, TeccCursorInterface *cursor, List **result);
extern TeccFetcherErrorCode tecc_parse_dek_list(const TeccState *state, TeccCursorInterface *cursor, List **result);
extern TeccFetcherErrorCode tecc_parse_columns_list(TeccCursorInterface *cursor, List **result);

#endif /* TECC_METADATA_QUERY_H */
/* End - ReqID:TECC4.0 */