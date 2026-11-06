/*-------------------------------------------------------------------------
 *
 * hg_encrypted_columns.h
 *	  definition of the "HG encrypted columns" system catalog (hg_encrypted_columns)
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/catalog/hg_encrypted_columns.h
 *
 * NOTES
 *	  The Catalog.pm module reads this file and derives schema
 *	  information.
 *
 *-------------------------------------------------------------------------
 */
#ifndef HG_ENCRYPTED_COLUMNS_H
#define HG_ENCRYPTED_COLUMNS_H

#include "catalog/genbki.h"
#include "catalog/hg_encrypted_columns_d.h"

/* ----------------
 *		hg_encrypted_columns definition.  cpp turns this into
 *		typedef struct FormData_hg_encrypted_columns
 * ----------------
 */
CATALOG(hg_encrypted_columns,11040,HgEncryptedColumnsRelationId)
{
	Oid			oid;			/* oid */

	/* 表对象ID（引用pg_class.oid） */
	Oid			table_oid BKI_FORCE_NOT_NULL;

	/* 列编号（在表中的位置） */
	int16		column_attnum BKI_FORCE_NOT_NULL;

	/* 列名称 */
	NameData	column_name BKI_FORCE_NOT_NULL;

	/* 原始数据类型OID（引用pg_type.oid） */
	Oid			data_type_original_oid BKI_LOOKUP(pg_type) BKI_FORCE_NOT_NULL;

	/* 原始数据类型修饰符 */
	int32		data_type_original_mod BKI_FORCE_NOT_NULL;

	/* 使用的DEK OID */
	Oid			column_key_id BKI_LOOKUP(hg_dek) BKI_FORCE_NOT_NULL;

	/* 加密类型（位掩码表示算法组合） */
	int32		encryption_type BKI_FORCE_NOT_NULL;

	/* 排序规则OID（引用pg_collation.oid，默认0） */
	Oid			collation_oid BKI_LOOKUP_OPT(pg_collation) BKI_DEFAULT(0);
#ifdef CATALOG_VARLEN
	timestamptz	create_date BKI_DEFAULT(now());
#endif
} FormData_hg_encrypted_columns;

/* ----------------
 *		Form_hg_encrypted_columns corresponds to a pointer to a tuple with
 *		the format of hg_encrypted_columns relation.
 * ----------------
 */
typedef FormData_hg_encrypted_columns *Form_hg_encrypted_columns;

DECLARE_UNIQUE_INDEX_PKEY(hg_encrypted_columns_pkey, 11041, on hg_encrypted_columns using btree(table_oid oid_ops, column_attnum int2_ops));
#define HgEncryptedColumnsPkeyIndexId 11041

DECLARE_INDEX(hg_encrypted_columns_column_key_id_index, 11042, on hg_encrypted_columns using btree(column_key_id oid_ops));
#define HgEncryptedColumnsColumnKeyIdIndexId 11042

DECLARE_INDEX(hg_encrypted_columns_data_type_index, 11043, on hg_encrypted_columns using btree(data_type_original_oid oid_ops));
#define HgEncryptedColumnsDataTypeIndexId 11043

DECLARE_INDEX(hg_encrypted_columns_collation_idx, 11044, on hg_encrypted_columns using btree(collation_oid oid_ops));
#define HgEncryptedColumnsCollationIndexId 11044

DECLARE_INDEX(hg_encrypted_columns_created_time_index, 11045, on hg_encrypted_columns using btree(create_date timestamptz_ops));
#define HgEncryptedColumnsCreatedTimeIndexId 11045

DECLARE_UNIQUE_INDEX(hg_encrypted_columns_oid_index, 11046, on hg_encrypted_columns using btree(oid oid_ops));
#define HgEncryptedColumnsOidIndexId 11046

#endif	/* HG_ENCRYPTED_COLUMNS_H */