/*-------------------------------------------------------------------------
 *
 * hg_encrypted_column_args.h
 *	  definition of the "HG encrypted column arguments" system catalog
 *	  (hg_encrypted_column_args)
 *
 * ReqID:TECC4.0 - 一列多 DEK 绑定元数据（P0-2）。
 * 列与 DEK 的多对多绑定关系采用与 hg_dek_args/hg_kek_args 一致的 args
 * 扩展表模式存储：对称 DEK 仍由 hg_encrypted_columns.column_key_id 承载，
 * he-add/he-mul 同态 DEK 以 'he_add_dek'/'he_mul_dek' 条目写入本表，
 * value 为 DEK 的分布式身份标识（column_key_distributed_id）。
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/catalog/hg_encrypted_column_args.h
 *
 * NOTES
 *	  The Catalog.pm module reads this file and derives schema
 *	  information.
 *
 *-------------------------------------------------------------------------
 */
#ifndef HG_ENCRYPTED_COLUMN_ARGS_H
#define HG_ENCRYPTED_COLUMN_ARGS_H

#include "catalog/genbki.h"
#include "catalog/hg_encrypted_column_args_d.h"

/* ----------------
 *		hg_encrypted_column_args definition.  cpp turns this into
 *		typedef struct FormData_hg_encrypted_column_args
 * ----------------
 */
CATALOG(hg_encrypted_column_args,11050,HgEncryptedColumnArgsRelationId)
{
	Oid			oid;			/* oid */

	/* 关联的加密列 OID（引用 hg_encrypted_columns.oid） */
	Oid			column_id BKI_LOOKUP(hg_encrypted_columns) BKI_FORCE_NOT_NULL;

	/* 函数名称（算法类型标识） */
	NameData	function_name BKI_FORCE_NOT_NULL;

	/* 参数键名：'he_add_dek' / 'he_mul_dek' */
	NameData	key BKI_FORCE_NOT_NULL;

	/* 参数值：DEK 分布式身份标识（column_key_distributed_id，4B int4） */
	bytea		value BKI_FORCE_NOT_NULL;
} FormData_hg_encrypted_column_args;

/* ----------------
 *		Form_hg_encrypted_column_args corresponds to a pointer to a tuple
 *		with the format of hg_encrypted_column_args relation.
 * ----------------
 */
typedef FormData_hg_encrypted_column_args *Form_hg_encrypted_column_args;

DECLARE_UNIQUE_INDEX_PKEY(hg_encrypted_column_args_pkey, 11051, on hg_encrypted_column_args using btree(column_id oid_ops, key name_ops));
#define HgEncryptedColumnArgsPkeyIndexId 11051

DECLARE_UNIQUE_INDEX(hg_encrypted_column_args_oid_index, 11052, on hg_encrypted_column_args using btree(oid oid_ops));
#define HgEncryptedColumnArgsOidIndexId 11052

DECLARE_TOAST(hg_encrypted_column_args, 11053, 11054);

#define HgEncryptedColumnArgsOidIndexId 11052
#endif	/* HG_ENCRYPTED_COLUMN_ARGS_H */
