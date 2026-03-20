/*-------------------------------------------------------------------------
 *
 * pg_access_history_last.h
 *
 *
 * src/include/catalog/pg_access_history_last.h
 *
 * NOTES
 *	  The Catalog.pm module reads this file and derives schema
 *	  information.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_ACCESS_HISTORY_LAST_H
#define PG_ACCESS_HISTORY_LAST_H

#include "catalog/genbki.h"
#include "catalog/pg_access_history_last_d.h"

/* ----------------
 *		pg_access_history definition.  cpp turns this into
 *		typedef struct FormData_pg_access_history
 * ----------------
 */

CATALOG(pg_access_history_last,6670,AccessHistoryLastId) BKI_SHARED_RELATION
{
	NameData	rolename;		/* name of role */
	int64		fail_sum;
#ifdef CATALOG_VARLEN
	text		last_success_login_time;
	text		last_success_login_app;
	text		last_success_login_method;
	text		last_success_login_ip;
	text		last_fail_login_time;
	text		last_fail_login_app;
	text		last_fail_login_method;
	text		last_fail_login_ip;
#endif
} FormData_pg_access_history_last;

/* ----------------
 *		Form_pg_access_history corresponds to a pointer to a tuple with
 *		the format of pg_access_history relation.
 * ----------------
 */
typedef FormData_pg_access_history_last *Form_pg_access_history_last;

/* add a index */
DECLARE_UNIQUE_INDEX(pg_access_history_last_name_index, 6673, on pg_access_history_last using btree(rolename name_ops));
#define AccessHistoryLastRolNameIndexId   6673


#endif							/* PG_ACCESS_HISTORY_L_H */
