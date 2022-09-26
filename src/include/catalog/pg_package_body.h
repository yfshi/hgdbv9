/*-------------------------------------------------------------------------
 *
 * File:pg_package_body.h
 *
 * Abstract:
 *		Definitions for the PLISQL Package Body
 *
 *
 * ReqID:SRS-SQL-PACKAGE
 *
 * Authored by dwdai@highgo.com,20220929.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  src/include/catalog/pg_package_body.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_PACKAGE_BODY_H
#define PG_PACKAGE_BODY_H

#include "catalog/genbki.h"
#include "catalog/objectaddress.h"
#include "catalog/pg_package_body_d.h"

/* ----------------
 *		pg_package_body definition.  cpp turns this into
 *		typedef struct FormData_pg_package_body
 * ----------------
 */

CATALOG(pg_package_body,9106,PackageBodyRelationId)
{
	Oid			oid;			/* oid */
	Oid 		pkgoid BKI_LOOKUP(pg_package);
#ifdef CATALOG_VARLEN			/* variable-length fields start here */
	text		bodysrc BKI_DEFAULT(_null_);
#endif
} FormData_pg_package_body;


/* ----------------
 * 	 Form_pg_package_body corresponds to a pointer to a tuple with
 * 	 the format of pg_package_body relation.
 * ----------------
 */
typedef FormData_pg_package_body *Form_pg_package_body;

DECLARE_TOAST(pg_package_body, 9107, 9108);

DECLARE_UNIQUE_INDEX_PKEY(pg_package_body_oid_index, 9109, on pg_package_body using btree(oid oid_ops));
#define PackageBodyObjectIndexId 9109
DECLARE_UNIQUE_INDEX(pg_package_body_pkgoid_index, 9110, on pg_package_body using btree(pkgoid oid_ops));
#define PackageBodyPkgOidIndex 9110
#endif			/* PG_PACKAGE_BODY_H */

