/*-------------------------------------------------------------------------
 *
 * File:pg_force_view.h
 *
 * Abstract:
 *		Definition of the "force view" system catalog(pg_force_view)
 *
 *
 * ReqID:SRS-SQL-FORCEVIEW
 *
 * Authored by zhenmingyang@highgo.com,20230906.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  src/include/catalog/pg_force_view.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_FORCE_VIEW_H
#define PG_FORCE_VIEW_H

#include "catalog/genbki.h"
#include "catalog/pg_force_view_d.h"

/* ----------------
 *		pg_force_view definition.  cpp turns this into
 *		typedef struct FormData_pg_force_view
 * ----------------
 */
CATALOG(pg_force_view,9120,ForceViewRelationId)
{
	Oid			fvoid;			/* the oid of force view */
	char		ident_case;		/* see IDENT_CASE__xxx constants below */

#ifdef CATALOG_VARLEN			/* variable-length fields start here */
	/*
	 * OA-REQ-1459:
	 * attlist holds the plain text of the view column names enclosed
	 * in parentheses, for example (c1,c2). Because force view does not
	 * record column information in pg_attribute. in some cases we don't
	 * know what the type of the view column is, For example,
	 * in the following example, Oracle is ok:
	 *
	 *	CREATE FORCE VIEW V1(A, B, C) AS SELECT D FROM T1;
	 *
	 * In addition, neither Oracle nor Postgres has syntax for adding or
	 * removing columns from a view, so I treat the columns of a view as
	 * constants, rather than as variables like the view name. Postgres
	 * has syntax for renaming view names. Therefore, variables such as
	 * view names must be obtained dynamically when the view is compiled.
	 */
	text		attlist;
	text		source;
#endif
} FormData_pg_force_view;

/* ----------------
 *		Form_pg_force_view corresponds to a pointer to a tuple with
 *		the format of pg_force_view relation.
 * ----------------
 */
typedef FormData_pg_force_view *Form_pg_force_view;

DECLARE_TOAST(pg_force_view, 9121, 9122);

DECLARE_UNIQUE_INDEX_PKEY(pg_force_view_fvoid_index, 9123, on pg_force_view using btree(fvoid oid_ops));
#define ForceViewFvoidIndexId	9123

/*
 * If the view fails to compile during CREATE, the definition of
 * force view is obtained from the queryString of the query, so
 * we must record the current identifier_case_switch value, otherwise
 * we will not know the meaning of the identifier within double quotes
 * when restoring.
 */
#define		  IDENT_CASE_NORMAL			'n'		/* NORMAL */
#define		  IDENT_CASE_INTERCHANGE	'i'		/* INTERCHANGE */
#define		  IDENT_CASE_LOWERCASE		'l'		/* LOWERCASE */
#define		  IDENT_CASE_UNDEFINE		'u'		/* UNDEFINE */

#endif							/* PG_FORCE_VIEW_H */
