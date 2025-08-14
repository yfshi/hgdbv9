/*-------------------------------------------------------------------------
 *
 * pg_synonym.h
 *	  definition of the "synonym" system catalog (pg_synonym)
 *
 * Copyright:
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Authored by lanke@highgo.com,20250812.
 *
 * src/include/catalog/pg_synonym.h
 *
 * NOTES
 *	  The Catalog.pm module reads this file and derives schema
 *	  information.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_SYNONYM_H
#define PG_SYNONYM_H

#include "catalog/genbki.h"
#include "catalog/objectaddress.h"
#include "catalog/pg_synonym_d.h"
#include "nodes/nodes.h"

/* ----------------
 *		pg_synonym.h definition.  cpp turns this into
 *		typedef struct FormData_pg_synonym
 *
 *		Some of the values in a pg_type instance are copied into
 *		pg_attribute instances.  Some parts of Postgres use the pg_type copy,
 *		while others use the pg_attribute copy, so they must match.
 *		See struct FormData_pg_attribute for details.
 * ----------------
 */

CATALOG(pg_synonym,9124,SynonymRelationId)
{
	Oid			oid;
	NameData	synname;
	Oid		synnamespace BKI_DEFAULT(pg_catalog) BKI_LOOKUP(pg_namespace);
	Oid		synowner  BKI_DEFAULT(POSTGRES) BKI_LOOKUP(pg_authid);
	bool		synispub;
	bool		synisedit BKI_DEFAULT(f);
	NameData		table_namespace;
	NameData		objname;
	NameData		db_link;
}FormData_pg_synonym;

/* ----------------
 *		Form_pg_synonym corresponds to a pointer to a row with
 *		the format of pg_synonym relation.
 * ----------------
 */
typedef FormData_pg_synonym *Form_pg_synonym;

DECLARE_UNIQUE_INDEX(pg_synonym_oid_index, 9125, on pg_synonym using btree(oid oid_ops));
#define SynonymOidIndexId 9125

DECLARE_UNIQUE_INDEX(pg_synonym_name_nsp_index, 9126, on pg_synonym using btree(synname name_ops, synnamespace oid_ops, synispub bool_ops));
#define SynonymNameNspIndexId	9126

#endif			/*	PG_SYNONYM_H */
