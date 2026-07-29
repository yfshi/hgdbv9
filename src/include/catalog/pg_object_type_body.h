/*-------------------------------------------------------------------------
 *
 * pg_object_type_body.h
 *	  Definition of the "pg_object_type_body" system catalog.
 *
 *-------------------------------------------------------------------------
 */

#ifndef PG_OBJECT_TYPE_BODY_H
#define PG_OBJECT_TYPE_BODY_H

#include "catalog/genbki.h"
#include "catalog/objectaddress.h"
#include "catalog/pg_object_type_body_d.h"

CATALOG(pg_object_type_body,9132,ObjectTypeBodyRelationId)
{
	Oid			oid;
	Oid			objtypoid BKI_LOOKUP(pg_object_type);
	bool		is_valid BKI_DEFAULT(f);

#ifdef CATALOG_VARLEN
	text		bodysrc BKI_DEFAULT(_null_);
#endif
} FormData_pg_object_type_body;

typedef FormData_pg_object_type_body *Form_pg_object_type_body;

DECLARE_TOAST(pg_object_type_body, 9133, 9134);

DECLARE_UNIQUE_INDEX_PKEY(pg_object_type_body_oid_index, 9135, on pg_object_type_body using btree(oid oid_ops));
#define ObjectTypeBodyObjectIndexId 9135
DECLARE_UNIQUE_INDEX(pg_object_type_body_objtypoid_index, 9136, on pg_object_type_body using btree(objtypoid oid_ops));
#define ObjectTypeBodyObjTypOidIndexId 9136

#endif
