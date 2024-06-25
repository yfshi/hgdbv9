/*-------------------------------------------------------------------------
 *
 * pg_object.h
 *    definition of the "procedure" system catalog (pg_object)
 *
 * Portions Copyright (c) 2024, Highgo Ltd. MySQL Compatible Development Group
 *
 * src/include/catalog/pg_object.h
 *
 * NOTES
 *    The Catalog.pm module reads this file and derives schema
 *    information.
 *
 * add the file for requirement "SRS-SQL-Object"
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_OBJECT_H
#define PG_OBJECT_H

#include "catalog/genbki.h"
#include "catalog/pg_object_d.h"

/* ----------------
 *		pg_object definition.  cpp turns this into
 *		typedef struct FormData_pg_object
 * ----------------
 */
CATALOG(pg_object,9251,ObjectRelationId)
{
	Oid				obj_oid;

	/* coulun serial number in pg_attribute */
	int32			objsubid;

	/* object name */
	NameData		objname;

	/* the type of this object */
	Oid				refclassid;

	/* the type schema, schema and database value is public*/
	Oid				schemaoid;

	/* see PROKIND_ categories below */
	char			objkind;

	/* In which mode the object was created, oracle is o, mysql is m */
	char			compatible_mode;

	/* object create time */
	Timestamp		created;

	/* object last modify time */
	Timestamp		modified;

#ifdef CATALOG_VARLEN

	/* object attribute */
	jsonb			option BKI_DEFAULT(_null_);

#endif
} FormData_pg_object;

/* ----------------
 *		Form_pg_object corresponds to a pointer to a tuple with
 *		the format of pg_object relation.
 * ----------------
 */
typedef FormData_pg_object *Form_pg_object;

DECLARE_TOAST(pg_object, 9252, 9253);

DECLARE_UNIQUE_INDEX(pg_refclassid_obj_oid_subid_index, 9254, on pg_object using btree(refclassid oid_ops, obj_oid oid_ops, objsubid int4_ops));
#define ObjectRefidOidSubidIndexId  9254

/* The following index is not currently in use, but it is retained.
 * If you encounter a situation where it needs to be modified before
 * it can be used, you can modify it.*/
DECLARE_UNIQUE_INDEX(pg_obj_name_kind_oid_index, 9255, on pg_object using btree(objname name_ops, objkind char_ops, obj_oid oid_ops));
#define ObjectNameKindOidIndexId  9255

#endif							/* PG_OBJECT_H */
