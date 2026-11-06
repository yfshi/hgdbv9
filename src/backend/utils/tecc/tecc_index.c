/*-------------------------------------------------------------------------
 *
 * tecc_index.c
 *	  Index support functions for encrypted data types
 *
 * This module provides functions to validate index creation on encrypted
 * columns, ensuring that the required encryption algorithms are enabled.
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/utils/tecc/tecc_index.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/htup_details.h"
#include "access/sysattr.h"
#include "catalog/pg_am.h"
#include "datatype/timestamp.h"
#include "catalog/hg_encrypted_columns.h"
#include "catalog/indexing.h"
#include "commands/encrypted_column.h"
#include "storage/lmgr.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"
#include "utils/syscache.h"
#include "utils/tecc/tecc_index.h"

/* Begin - ReqID:TECC4.0 */

/*
 * get_encrypted_column_encryption_type - 获取加密列的encryption_type位掩码
 *
 * 查询hg_encrypted_columns系统表，获取指定列的加密类型位掩码。
 * 返回值: true（列在hg_encrypted_columns中存在，即该列是加密列）
 *         false（列不是加密列，未在hg_encrypted_columns中注册）
 * 加密类型位掩码通过encryption_type输出参数返回；当列是RND-only时位掩码为0。
 */
static bool
get_encrypted_column_encryption_type(Oid table_oid, int16 column_attnum,
									 uint32 *encryption_type)
{
	HeapTuple	tuple;

	tuple = SearchSysCache2(HGENCRYPTEDCOLUMNSTABLEATTNUM,
							ObjectIdGetDatum(table_oid),
							Int16GetDatum(column_attnum));

	if (HeapTupleIsValid(tuple))
	{
		*encryption_type = ((Form_hg_encrypted_columns) GETSTRUCT(tuple))->encryption_type;
		ReleaseSysCache(tuple);
		return true;	/* column is encrypted */
	}

	*encryption_type = 0;
	return false;		/* column not in hg_encrypted_columns */
}

/*
 * check_index_support_for_encrypted_column - 检查加密列是否支持索引
 *
 * 在创建索引前验证加密列的算法配置。
 * 调用时机: DefineIndex函数中解析索引列后调用。
 *
 * 返回值: true（允许创建索引），false（不允许创建索引）
 * 注意: 对于不支持的场景，函数会直接报错（ereport(ERROR)），不会返回false。
 *
 * 检查逻辑:
 * - 非加密列: 直接返回true
 * - B-Tree索引: 需要加密列启用OPE加密
 * - Hash索引: 需要加密列启用DET加密
 * - 加密列不支持索引时: ereport(ERROR)
 *
 * ReqID:TECC4.0
 */
bool
check_index_support_for_encrypted_column(Oid table_oid, int16 column_attnum,
										 Oid index_method)
{
	uint32		encryption_type;

	/* 1. 获取加密列信息，区分"非加密列"和"RND-only加密列" */
	if (!get_encrypted_column_encryption_type(table_oid, column_attnum,
											  &encryption_type))
	{
		/* 不是加密列（未在hg_encrypted_columns中注册），允许创建索引 */
		return true;
	}

	/* 2. 根据索引类型验证算法配置 */
	if (index_method == BTREE_AM_OID)
	{
		/* B-Tree索引需要OPE加密（RND-only时encryption_type=0，不通过） */
		if (!(encryption_type & HG_ENCRYPTION_TYPE_OPE))
		{
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("cannot create B-tree index on encrypted column without OPE encryption"),
					 errdetail("Column \"%s\" in table \"%s\" does not have OPE encryption enabled.",
							   get_attname(table_oid, column_attnum, false),
							   get_rel_name(table_oid)),
					 errhint("Enable OPE encryption for the column or use Hash index instead.")));
			return false;
		}
	}
	else if (index_method == HASH_AM_OID)
	{
		/* ReqID:TECC4.0 - Hash索引需要DET或OPE加密（RND-only时encryption_type=0，不通过） */
		if (!(encryption_type & (HG_ENCRYPTION_TYPE_DET | HG_ENCRYPTION_TYPE_OPE)))
		{
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("cannot create Hash index on encrypted column without DET or OPE encryption"),
					 errdetail("Column \"%s\" in table \"%s\" has neither DET nor OPE encryption enabled.",
							   get_attname(table_oid, column_attnum, false),
							   get_rel_name(table_oid)),
					 errhint("Enable DET or OPE encryption for the column.")));
			return false;
		}
	}

	return true;
}

/* Begin - ReqID:TECC4.0 */

/*
 * tecc_column_supports_ordering - check whether a column supports ordering
 *		via OPE encryption.
 *
 * Returns true if the column is NOT encrypted (not found in
 * hg_encrypted_columns), or if it has OPE encryption enabled.
 * Returns false only for encrypted columns without OPE support
 * (DET-only, RND-only, SEARCH-only).
 *
 * This is used by std_typanalyze to decide whether to collect scalar
 * statistics (which require sorting) or fall back to distinct-only
 * statistics (which only need equality comparison).
 */
bool
tecc_column_supports_ordering(Oid table_oid, int16 attnum)
{
	HeapTuple	tuple;
	uint32		enc_type;

	tuple = SearchSysCache2(HGENCRYPTEDCOLUMNSTABLEATTNUM,
							ObjectIdGetDatum(table_oid),
							Int16GetDatum(attnum));
	if (!HeapTupleIsValid(tuple))
		return true;	/* not an encrypted column */

	enc_type = ((Form_hg_encrypted_columns) GETSTRUCT(tuple))->encryption_type;
	ReleaseSysCache(tuple);

	return (enc_type & HG_ENCRYPTION_TYPE_OPE) != 0;
}

/* End - ReqID:TECC4.0 */
