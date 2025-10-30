/*-------------------------------------------------------------------------
 *
 * superuser.c
 *	  The superuser() function.  Determines if user has superuser privilege.
 *
 * All code should use either of these two functions to find out
 * whether a given user is a superuser, rather than examining
 * pg_authid.rolsuper directly, so that the escape hatch built in for
 * the single-user case works.
 *
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/utils/misc/superuser.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/htup_details.h"
#include "catalog/pg_authid.h"
#include "miscadmin.h"
#include "utils/inval.h"
#include "utils/syscache.h"

/* Begin - ReqID:SRS-SECURITY-POWERS */
/* add for secure enhancement features*/
#include "se_features_interface.h"
/* End - ReqID:SRS-SECURITY-POWERS */

/*
 * In common cases the same roleid (ie, the session or current ID) will
 * be queried repeatedly.  So we maintain a simple one-entry cache for
 * the status of the last requested roleid.  The cache can be flushed
 * at need by watching for cache update events on pg_authid.
 */
static Oid	last_roleid = InvalidOid;	/* InvalidOid == cache not valid */
static bool last_roleid_is_super = false;
static bool roleid_callback_registered = false;

static void RoleidCallback(Datum arg, int cacheid, uint32 hashvalue);


/*
 * The Postgres user running this command has Postgres superuser privileges
 */
bool
superuser(void)
{
	return superuser_arg(GetUserId());
}


/*
 * The specified role has Postgres superuser privileges
 */
bool
superuser_arg(Oid roleid)
{
	bool		result;
	HeapTuple	rtup;

	/* Begin - ReqID:SRS-SECURITY-POWERS */
	/* since syssso or syssao may need superuser priv, we make them a temp superuser*/
	/* and this value could only set by code, so we always give code writer superuser privs */
	if (se_feature.tmp_superuser_oid == roleid)
		return true;
	/* 
	 * some processes may also check superuser() during initialization.
	 * At that point sysdba_role haven't created.
	 * In other case, if se_feature.mode is on, make sure only sysdba_role
	 * member can be superuser.
	 */
	if (IsUnderPostmaster && se_feature.mode)
		return IS_SYSDBA(roleid);
	/* End - ReqID:SRS-SECURITY-POWERS */

	/* Quick out for cache hit */
	if (OidIsValid(last_roleid) && last_roleid == roleid)
		return last_roleid_is_super;

	/* Special escape path in case you deleted all your users. */
	if (!IsUnderPostmaster && roleid == BOOTSTRAP_SUPERUSERID)
		return true;

	/* OK, look up the information in pg_authid */
	rtup = SearchSysCache1(AUTHOID, ObjectIdGetDatum(roleid));
	if (HeapTupleIsValid(rtup))
	{
		result = ((Form_pg_authid) GETSTRUCT(rtup))->rolsuper;
		ReleaseSysCache(rtup);
	}
	else
	{
		/* Report "not superuser" for invalid roleids */
		result = false;
	}

	/* If first time through, set up callback for cache flushes */
	if (!roleid_callback_registered)
	{
		CacheRegisterSyscacheCallback(AUTHOID,
									  RoleidCallback,
									  (Datum) 0);
		roleid_callback_registered = true;
	}

	/* Cache the result for next time */
	last_roleid = roleid;
	last_roleid_is_super = result;

	/* Begin - ReqID:SRS-SECURITY-POWERS */
	/* make sure only sysdba_role member can be superuser */
	if (se_feature.mode && IS_SYSDBA(roleid) &&
	    result)
		return true;
	/* End - ReqID:SRS-SECURITY-POWERS */

	return result;
}

/*
 * RoleidCallback
 *		Syscache inval callback function
 */
static void
RoleidCallback(Datum arg, int cacheid, uint32 hashvalue)
{
	/* Invalidate our local cache in case role's superuserness changed */
	last_roleid = InvalidOid;
}