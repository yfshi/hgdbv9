/*-------------------------------------------------------------------------
 *
 * pg_role_status.h
 *	  definition of the "roles status" system catalog
 *	  (pg_role_status).
 *
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/catalog/pg_role_status.h
 *
 * NOTES
 *	  The Catalog.pm module reads this file and derives schema
 *	  information.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_ROLE_STATUS_H
#define PG_ROLE_STATUS_H

#include "catalog/genbki.h"
#include "catalog/pg_role_status_d.h"

/* ----------------
 *		pg_role_status definition.  cpp turns this into
 *		typedef struct FormData_pg_role_status
 * ----------------
 */
CATALOG(pg_role_status,6700,RoleStatusId) BKI_SHARED_RELATION
{
	Oid			roleid ;	/* ID of a role */
	NameData	rolname;	/* name of role */
	bool		rolstatus;	/* enable or disable */
} FormData_pg_role_status;

/* ----------------
 *		Form_pg_role_status corresponds to a pointer to a tuple with
 *		the format of pg_role_status relation.
 * ----------------
 */
typedef FormData_pg_role_status *Form_pg_role_status;

DECLARE_UNIQUE_INDEX_PKEY(pg_role_status_index, 6701, on pg_role_status using btree(roleid oid_ops));
#define RoleStatusOidIndexId	6701
#endif							/* PG_ROLE_STATUS_H */
