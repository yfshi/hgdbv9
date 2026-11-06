/*-------------------------------------------------------------------------
 *
 * hg_kek_args.h
 *	  definition of the "HG KEK arguments" system catalog (hg_kek_args)
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/catalog/hg_kek_args.h
 *
 * NOTES
 *	  The Catalog.pm module reads this file and derives schema
 *	  information.
 *
 *-------------------------------------------------------------------------
 */
#ifndef HG_KEK_ARGS_H
#define HG_KEK_ARGS_H

#include "catalog/genbki.h"
#include "catalog/hg_kek_args_d.h"

/* ----------------
 *		hg_kek_args definition.  cpp turns this into
 *		typedef struct FormData_hg_kek_args
 * ----------------
 */
CATALOG(hg_kek_args,11020,HgKekArgsRelationId)
{
	Oid			oid;			/* oid */

	/* 关联的KEK OID */
	Oid			kek_id BKI_LOOKUP(hg_kek) BKI_FORCE_NOT_NULL;

	/* 函数名称（参数使用位置） */
	NameData	function_name BKI_FORCE_NOT_NULL;

	/* 参数键名 */
	NameData	key BKI_FORCE_NOT_NULL;

	/* 参数值（二进制格式存储） */
	bytea		value BKI_FORCE_NOT_NULL;
} FormData_hg_kek_args;

/* ----------------
 *		Form_hg_kek_args corresponds to a pointer to a tuple with
 *		the format of hg_kek_args relation.
 * ----------------
 */
typedef FormData_hg_kek_args *Form_hg_kek_args;

DECLARE_UNIQUE_INDEX_PKEY(hg_kek_args_pkey, 11021, on hg_kek_args using btree(kek_id oid_ops, key name_ops));
#define HgKekArgsPkeyIndexId 11021

DECLARE_INDEX(hg_kek_args_kek_id_index, 11022, on hg_kek_args using btree(kek_id oid_ops));
#define HgKekArgsKekIdIndexId 11022

DECLARE_UNIQUE_INDEX(hg_kek_args_oid_index, 11025, on hg_kek_args using btree(oid oid_ops));
#define HgKekArgsOidIndexId 11025

DECLARE_TOAST(hg_kek_args, 11023, 11024);

#endif	/* HG_KEK_ARGS_H */