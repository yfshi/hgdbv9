/*-------------------------------------------------------------------------
 *
 * tecc_index.h
 *	  Declarations for index support functions for encrypted data types
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/utils/tecc/tecc_index.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_INDEX_H
#define TECC_INDEX_H

#include "postgres.h"
#include "catalog/objectaddress.h"

/* Begin - ReqID:TECC4.0 */

/*
 * check_index_support_for_encrypted_column - 检查加密列是否支持索引
 *
 * 在创建索引前验证加密列的算法配置。
 *
 * 参数:
 *   table_oid    - 表的OID
 *   column_attnum - 列编号（从1开始）
 *   index_method - 索引访问方法OID（BTREE_AM_OID或HASH_AM_OID）
 *
 * 返回值:
 *   true - 支持创建索引
 *   false - 不支持（对于不支持的场景会报错，不会返回false）
 */
extern bool check_index_support_for_encrypted_column(Oid table_oid,
													 int16 column_attnum,
													 Oid index_method);

extern bool tecc_get_column_encryption_info(Oid table_oid, int16 attnum,
											bool *is_encrypted,
											uint32 *encryption_type,
											Oid *dek_oid);
extern bool tecc_column_supports_ordering(Oid table_oid, int16 attnum);
extern bool tecc_column_supports_equality(Oid table_oid, int16 attnum);

/* End - ReqID:TECC4.0 */

#endif /* TECC_INDEX_H */
