/*-------------------------------------------------------------------------
 *
 * pg_resgroupcapability.h
 *	  definition of the system "resource group capability" relation (pg_resgroupcapability).
 *
 * Authored by zhangyilin@highgo.com
 * Portions Copyright (c) 2009-2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 *
 * NOTES
 *	  the genbki.sh script reads this file and generates .bki
 *	  information from the DATA() statements.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_RESGROUPCAPABILITY_H
#define PG_RESGROUPCAPABILITY_H

#include "catalog/genbki.h"
#include "catalog/pg_resgroupcapability_d.h"

CATALOG(pg_resgroupcapability,6439,ResGroupCapabilityRelationId) BKI_SHARED_RELATION
{
	Oid			resgroupid;	/* OID of the group with this capability  */

	int16		reslimittype;	/* resource limit type id (RESGROUP_LIMIT_TYPE_XXX) */

	Oid			rgroleid;
#ifdef CATALOG_VARLEN
	text		value;		/* resource limit (opaque type)  */
	jsonb		option BKI_DEFAULT(_null_);

#endif
} FormData_pg_resgroupcapability;

/* ----------------
 *	Form_pg_resgroupcapability corresponds to a pointer to a tuple with
 *	the format of pg_resgroupcapability relation.
 * ----------------
 */
typedef FormData_pg_resgroupcapability *Form_pg_resgroupcapability;

DECLARE_TOAST(pg_resgroupcapability, 6442, 6443);
#define PgResGroupCapabilityToastTable 6442
#define PgResGroupCapabilityToastIndex 6443

DECLARE_FOREIGN_KEY((resgroupid), pg_resgroup, (oid));

DECLARE_INDEX(pg_resgroupcapability_resgroupid_reslimittype_index, 6445, on pg_resgroupcapability using btree(resgroupid oid_ops, reslimittype int2_ops));
#define ResGroupCapabilityResgroupidResLimittypeIndexId 6445

DECLARE_INDEX(pg_resgroupcapability_resgroupid_index, 6446, on pg_resgroupcapability using btree(resgroupid oid_ops));
#define ResGroupCapabilityResgroupidIndexId     6446

DECLARE_UNIQUE_INDEX(pg_resgroupcapability_resgroupid_rgroleid_index, 6449, on pg_resgroupcapability using btree(resgroupid oid_ops, reslimittype int2_ops, rgroleid oid_ops));
#define ResGroupCapabilityResgroupidRgroleidIndexId     6449

#endif   /* PG_RESGROUPCAPABILITY_H */
