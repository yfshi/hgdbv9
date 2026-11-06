/*-------------------------------------------------------------------------
 *
 * hg_dek.h
 *	  definition of the "HG Data Encryption Key" system catalog (hg_dek)
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/catalog/hg_dek.h
 *
 * NOTES
 *	  The Catalog.pm module reads this file and derives schema
 *	  information.
 *
 *-------------------------------------------------------------------------
 */
#ifndef HG_DEK_H
#define HG_DEK_H

#include "catalog/genbki.h"
#include "utils/timestamp.h"
#include "catalog/hg_dek_d.h"

/* ----------------
 *		hg_dek definition.  cpp turns this into
 *		typedef struct FormData_hg_dek
 * ----------------
 */
CATALOG(hg_dek,11010,HgDekRelationId)
{
	Oid			oid;			/* oid */

	/* DEK名称 */
	NameData	dek_name BKI_FORCE_NOT_NULL;

	/* 分布式ID（哈希生成的唯一标识） */
	Oid			column_key_distributed_id BKI_FORCE_NOT_NULL;

	/* 关联的KEK OID */
	Oid			kek_id BKI_LOOKUP(hg_kek) BKI_FORCE_NOT_NULL;

	/* 命名空间OID */
	Oid			key_namespace BKI_LOOKUP(pg_namespace) BKI_FORCE_NOT_NULL;

	/* 所有者OID */
	Oid			key_owner BKI_LOOKUP(pg_authid) BKI_FORCE_NOT_NULL;

#ifdef CATALOG_VARLEN
	/* 访问权限 */
	aclitem		key_acl[1];
#endif

	/* 创建时间 */
#ifndef timestamptz
#define timestamptz TimestampTz
#endif
	timestamptz	create_date BKI_DEFAULT(now());
} FormData_hg_dek;

/* ----------------
 *		Form_hg_dek corresponds to a pointer to a tuple with
 *		the format of hg_dek relation.
 * ----------------
 */
typedef FormData_hg_dek *Form_hg_dek;

DECLARE_UNIQUE_INDEX(hg_dek_dek_name_index, 11011, on hg_dek using btree(dek_name name_ops, key_namespace oid_ops));
#define HgDekDekNameIndexId 11011

DECLARE_UNIQUE_INDEX_PKEY(hg_dek_oid_index, 11012, on hg_dek using btree(oid oid_ops));
#define HgDekOidIndexId 11012

DECLARE_UNIQUE_INDEX(hg_dek_distributed_id_index, 11013, on hg_dek using btree(column_key_distributed_id oid_ops));
#define HgDekDistributedIdIndexId 11013

DECLARE_INDEX(hg_dek_kek_id_index, 11014, on hg_dek using btree(kek_id oid_ops));
#define HgDekKekIdIndexId 11014

DECLARE_INDEX(hg_dek_key_namespace_index, 11015, on hg_dek using btree(key_namespace oid_ops));
#define HgDekNamespaceIndexId 11015

DECLARE_INDEX(hg_dek_key_owner_index, 11016, on hg_dek using btree(key_owner oid_ops));
#define HgDekOwnerIndexId 11016

DECLARE_TOAST(hg_dek, 11017, 11018);

#endif	/* HG_DEK_H */