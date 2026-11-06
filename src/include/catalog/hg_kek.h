/*-------------------------------------------------------------------------
 *
 * hg_kek.h
 *	  definition of the "HG Key Encryption Key" system catalog (hg_kek)
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/catalog/hg_kek.h
 *
 * NOTES
 *	  The Catalog.pm module reads this file and derives schema
 *	  information.
 *
 *-------------------------------------------------------------------------
 */
#ifndef HG_KEK_H
#define HG_KEK_H

#include "catalog/genbki.h"
#include "catalog/hg_kek_d.h"
#include "utils/timestamp.h"

/* ----------------
 *		hg_kek definition.  cpp turns this into
 *		typedef struct FormData_hg_kek
 * ----------------
 */
CATALOG(hg_kek,11000,HgKekRelationId)
{
	Oid			oid;			/* oid */

	/* KEK名称 */
	NameData	kek_name BKI_FORCE_NOT_NULL;

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
} FormData_hg_kek;

/* ----------------
 *		Form_hg_kek corresponds to a pointer to a tuple with
 *		the format of hg_kek relation.
 * ----------------
 */
typedef FormData_hg_kek *Form_hg_kek;

DECLARE_UNIQUE_INDEX_PKEY(hg_kek_oid_index, 11001, on hg_kek using btree(oid oid_ops));
#define HgKekOidIndexId 11001

DECLARE_UNIQUE_INDEX(hg_kek_kek_name_index, 11002, on hg_kek using btree(kek_name name_ops, key_namespace oid_ops));
#define HgKekKekNameIndexId 11002

DECLARE_INDEX(hg_kek_key_namespace_index, 11003, on hg_kek using btree(key_namespace oid_ops));
#define HgKekNamespaceIndexId 11003

DECLARE_INDEX(hg_kek_key_owner_index, 11004, on hg_kek using btree(key_owner oid_ops));
#define HgKekOwnerIndexId 11004

DECLARE_TOAST(hg_kek, 11005, 11006);

#endif	/* HG_KEK_H */