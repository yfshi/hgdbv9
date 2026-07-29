/*-------------------------------------------------------------------------
 *
 * pg_object_type.h
 *	  Definition of the "pg_object_type" system catalog.
 *
 *-------------------------------------------------------------------------
 */

#ifndef PG_OBJECT_TYPE_H
#define PG_OBJECT_TYPE_H

#include "catalog/genbki.h"
#include "catalog/objectaddress.h"
#include "catalog/pg_object_type_d.h"

CATALOG(pg_object_type,9127,ObjectTypeRelationId)
{
	Oid			oid;
	Oid			typid BKI_DEFAULT(0) BKI_LOOKUP_OPT(pg_type);
	Oid			parenttypid BKI_DEFAULT(0) BKI_LOOKUP_OPT(pg_type);
	Oid			objnamespace BKI_DEFAULT(pg_catalog) BKI_LOOKUP(pg_namespace);
	Oid			objowner BKI_DEFAULT(POSTGRES) BKI_LOOKUP(pg_authid);
	bool		is_final BKI_DEFAULT(t);
	bool		is_valid BKI_DEFAULT(f);

#ifdef CATALOG_VARLEN
	text		objsrc BKI_DEFAULT(_null_);
#endif
} FormData_pg_object_type;

typedef FormData_pg_object_type *Form_pg_object_type;

DECLARE_TOAST(pg_object_type, 9128, 9129);

DECLARE_UNIQUE_INDEX_PKEY(pg_object_type_oid_index, 9130, on pg_object_type using btree(oid oid_ops));
#define ObjectTypeObjectIndexId 9130
DECLARE_UNIQUE_INDEX(pg_object_type_typid_index, 9131, on pg_object_type using btree(typid oid_ops));
#define ObjectTypeTypIdIndexId 9131

#endif
