/*-------------------------------------------------------------------------
 *
 * pg_resgroup.h
 *	  definition of the system "resource group" relation (pg_resgroup).
 *
 *
 * Authored by zhangyilin@highgo.com
 * Portions Copyright (c) 2009-2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * NOTES
 *	  the genbki.sh script reads this file and generates .bki
 *	  information from the DATA() statements.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_RESGROUP_H
#define PG_RESGROUP_H

#include "catalog/genbki.h"
#include "catalog/pg_resgroup_d.h"

/* ----------------
 *	pg_resgroup definition.  cpp turns this into
 *	typedef struct FormData_pg_resgroup
 * ----------------
 */

CATALOG(pg_resgroup,6436,ResGroupRelationId) BKI_SHARED_RELATION
{
	Oid			oid;			/* oid */

	NameData	rsgname;		/* name of resource group */
	
	Oid			parent;			/* parent of resource group, not used */
	
} FormData_pg_resgroup;

/* ----------------
 *	Form_pg_resgroup corresponds to a pointer to a tuple with
 *	the format of pg_resgroup relation.
 * ----------------
 */
typedef FormData_pg_resgroup *Form_pg_resgroup;

DECLARE_UNIQUE_INDEX_PKEY(pg_resgroup_oid_index, 6447, on pg_resgroup using btree(oid oid_ops));
#define ResGroupOidIndexId	6447

DECLARE_UNIQUE_INDEX(pg_resgroup_rsgname_index, 6444, on pg_resgroup using btree(rsgname name_ops));
#define ResGroupRsgnameIndexId	6444

#endif   /* PG_RESGROUP_H */
