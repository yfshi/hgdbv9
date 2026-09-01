/*-------------------------------------------------------------------------
 *
 * pg_collection_type.h
 *	  definition of the "collection type" system catalog (pg_collection_type)
 *
 * This catalog stores metadata for Oracle-compatible SQL collection types
 * (nested tables and VARRAY).
 *
 * Portions Copyright (c) 2023-2026, IvorySQL Global Development Team
 *
 * src/include/catalog/pg_collection_type.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_COLLECTION_TYPE_H
#define PG_COLLECTION_TYPE_H

#include "catalog/genbki.h"
#include "catalog/objectaddress.h"
#include "catalog/pg_collection_type_d.h"


/* ----------------
 *		pg_collection_type definition.
 * ----------------
 */
CATALOG(pg_collection_type,9140,CollectionTypeRelationId)
{
	/* OID of the collection type in pg_type */
	Oid			pgtypoid;

	/* OID of the element type */
	Oid			elemtype;

	/* typtypmod of the element type */
	int32		elemtypmod BKI_DEFAULT(-1);

	/* element not null */
	bool		elemtypnotnull BKI_DEFAULT(false);

	/* max elements (for nested table, climit is 0) */
	int32		climit BKI_DEFAULT(0);

	/* 'v' = VARRAY, 'n' = Nested Table */
	char		kind;

	/* flags: such as the PERSISTABLE attribute */
	int16		flags;

} FormData_pg_collection_type;

/* ----------------
 *		Form_pg_collection_type corresponds to a pointer to a tuple with
 *		the format of pg_collection_type relation.
 * ----------------
 */
typedef FormData_pg_collection_type *Form_pg_collection_type;

DECLARE_UNIQUE_INDEX_PKEY(pg_collection_type_pgtypoid_index, 9141, on pg_collection_type using btree(pgtypoid oid_ops));
#define CollectionPgTypeOidIndexId 9141

/* flags bitmask */
#define COLLECTION_FLAG_PERSISTABLE		0x0001
#define COLLECTION_FLAG_NOT_PERSISTABLE	0x0002

#endif /* PG_COLLECTION_TYPE_H */
