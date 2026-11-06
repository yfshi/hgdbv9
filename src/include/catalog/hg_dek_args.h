/*-------------------------------------------------------------------------
 *
 * hg_dek_args.h
 *	  definition of the "HG DEK arguments" system catalog (hg_dek_args)
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/catalog/hg_dek_args.h
 *
 * NOTES
 *	  The Catalog.pm module reads this file and derives schema
 *	  information.
 *
 *-------------------------------------------------------------------------
 */
#ifndef HG_DEK_ARGS_H
#define HG_DEK_ARGS_H

#include "catalog/genbki.h"
#include "catalog/hg_dek_args_d.h"

/* ----------------
 *		hg_dek_args definition.  cpp turns this into
 *		typedef struct FormData_hg_dek_args
 * ----------------
 */
CATALOG(hg_dek_args,11030,HgDekArgsRelationId)
{
	Oid			oid;			/* oid */

	/* 关联的DEK OID */
	Oid			dek_id BKI_LOOKUP(hg_dek) BKI_FORCE_NOT_NULL;

	/* 函数名称（算法类型标识） */
	NameData	function_name BKI_FORCE_NOT_NULL;

	/* 参数键名 */
	NameData	key BKI_FORCE_NOT_NULL;

	/* 参数值（二进制格式存储） */
	bytea		value BKI_FORCE_NOT_NULL;
} FormData_hg_dek_args;

/* ----------------
 *		Form_hg_dek_args corresponds to a pointer to a tuple with
 *		the format of hg_dek_args relation.
 * ----------------
 */
typedef FormData_hg_dek_args *Form_hg_dek_args;

DECLARE_UNIQUE_INDEX_PKEY(hg_dek_args_pkey, 11031, on hg_dek_args using btree(dek_id oid_ops, key name_ops));
#define HgDekArgsPkeyIndexId 11031

DECLARE_INDEX(hg_dek_args_dek_id_index, 11032, on hg_dek_args using btree(dek_id oid_ops));
#define HgDekArgsDekIdIndexId 11032

DECLARE_UNIQUE_INDEX(hg_dek_args_oid_index, 11035, on hg_dek_args using btree(oid oid_ops));
#define HgDekArgsOidIndexId 11035

DECLARE_TOAST(hg_dek_args, 11033, 11034);

#endif	/* HG_DEK_ARGS_H */