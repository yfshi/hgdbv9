/* Begin - ReqID:TECC4.0 */
/*-------------------------------------------------------------------------
 *
 * tecc_bytea_decoder.h
 *    TECC bytea value decoder interface
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/interfaces/libpq/tecc_data_fetcher/src/tecc_bytea_decoder.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_BYTEA_DECODER_H
#define TECC_BYTEA_DECODER_H

#include "postgres_fe.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * decode_bytea_value - 解码bytea类型的值
 *
 * bytea_str: bytea字符串（可能以\x开头）
 * 返回: 解码后的字符串，需要调用者释放
 */
char *decode_bytea_value(const char *bytea_str);

#ifdef __cplusplus
}
#endif

#endif /* TECC_BYTEA_DECODER_H */
/* End - ReqID:TECC4.0 */