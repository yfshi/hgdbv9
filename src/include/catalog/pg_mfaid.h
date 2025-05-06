/*-------------------------------------------------------------------------
 *
 * pg_mfaid.h
 *	  definition of the "Multifactor authorization identifier" system catalog (pg_mfaid)
 *
 * Author: suzy@highgo.com, 20241220.
 *
 * Copyright:
 *		Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/include/catalog/pg_mfaid.h
 *
 * NOTES
 *	  The Catalog.pm module reads this file and derives schema
 *	  information.
 *
 * add the file for requirement "SRS-SECURITY-IDENTITYAUTH"
 * 
 *-------------------------------------------------------------------------
 */
#ifndef PG_MFAID_H
#define PG_MFAID_H

#include "catalog/genbki.h"
#include "catalog/pg_mfaid_d.h"

/* ----------------
 *		pg_mfaid definition.  cpp turns this into
 *		typedef struct FormData_pg_mfaid
 * ----------------
 */
CATALOG(pg_mfaid,4562,MfaIdRelationId) BKI_SHARED_RELATION BKI_ROWTYPE_OID(4563,MfaIdRelation_Rowtype_Id) BKI_SCHEMA_MACRO
{
	Oid			oid;			/* oid */

	/* remaining fields may be null; use heap_getattr to read them! */
#ifdef CATALOG_VARLEN			/* variable-length fields start here */
	text		mfa_type;	/* Multifactor authorization type */
	text		mfa_key;	/* Multifactor authorization key */
#endif
} FormData_pg_mfaid;

/* ----------------
 *		Form_pg_mfaid corresponds to a pointer to a tuple with
 *		the format of pg_mfaid relation.
 * ----------------
 */
typedef FormData_pg_mfaid *Form_pg_mfaid;

DECLARE_UNIQUE_INDEX_PKEY(pg_mfaid_oid_index, 4564, on pg_mfaid using btree(oid oid_ops));
#define MfaIdOidIndexId	4564

#endif							/* PG_MFAID_H */
