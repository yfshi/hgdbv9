/* Begin - ReqID:TECC4.0 */
/*-------------------------------------------------------------------------
 *
 * tecc_bitmask_parser.h
 *    TECC bitmask parsing functions for encryption algorithms
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/interfaces/libpq/tecc_data_fetcher/include/tecc_bitmask_parser.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_BITMASK_PARSER_H
#define TECC_BITMASK_PARSER_H

#include "tecc_data_fetcher.h"

/* 位掩码解析函数 */

/* 检查是否包含特定算法 */
extern bool tecc_has_algorithm(int bitmask, TeccEncryptionAlgorithm alg);

/* 解析算法位掩码字符串 */
extern int tecc_parse_algorithms(const char *bitmask_str);

/* 将位掩码转换为字符串 */
extern char *tecc_algorithms_to_string(int bitmask);

/* 获取算法名称 */
extern const char *tecc_get_encryption_algorithm_name(TeccEncryptionAlgorithm alg);

/* 检查算法组合是否有效 */
extern bool tecc_validate_algorithm_combination(int bitmask);

/* 获取算法数量 */
extern int tecc_count_algorithms(int bitmask);

/* 算法位掩码操作 */
extern int tecc_add_algorithm(int bitmask, TeccEncryptionAlgorithm alg);
extern int tecc_remove_algorithm(int bitmask, TeccEncryptionAlgorithm alg);
extern bool tecc_has_multiple_algorithms(int bitmask);

#endif /* TECC_BITMASK_PARSER_H */
/* End - ReqID:TECC4.0 */