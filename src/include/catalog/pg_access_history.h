/*-------------------------------------------------------------------------
 *
 * pg_access_history.h
 *
 *
 * src/include/catalog/pg_access_history.h
 *
 * NOTES
 *	  The Catalog.pm module reads this file and derives schema
 *	  information.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_ACCESS_HISTORY_H
#define PG_ACCESS_HISTORY_H

#include "catalog/genbki.h"
#include "catalog/pg_access_history_d.h"

/*
 *	pg_access_history definition.  cpp turns this into
 *	typedef struct FormData_pg_access_history
 *
 */
CATALOG(pg_access_history,6666,AccessHistoryId) BKI_SHARED_RELATION
{
	Oid			roloid;			/* oid of role */
	NameData	rolename;		/* name of role */
	Oid			seqoid;			/* sequence id of access history */
	bool		login_status;	/* login results, success or failure */
#ifdef CATALOG_VARLEN
	text		login_time;
	text		login_app;
	text		login_method;
	text		login_ip;
#endif
} FormData_pg_access_history;


/*
 *	Form_pg_access_history corresponds to a pointer to a tuple with
 *	the format of pg_access_history relation.
 * ----------------
 */
typedef FormData_pg_access_history *Form_pg_access_history;

/* add a index */
DECLARE_UNIQUE_INDEX(pg_access_history_seqoid_index, 6669, on pg_access_history using btree(seqoid oid_ops));
#define AccessHistorySeqoidIndexId   6669


#endif							/* PG_ACCESS_HISTORY_H */
