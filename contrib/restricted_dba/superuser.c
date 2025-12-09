/* -------------------------------------------------------------------------
 *
 * File: superuser.c
 *
 * Abstract: 
 * 		A special version of superuser() function.  
 *	  	Determines whewher user has superuser privilege under our rules.
 *		se_role_is_superuser() will replace original logic in 
 *		superuser_arg();
 *
 * Authored by sunchengxi@highgo.com, 20251201.
 *
 * Copyright:
 * 		Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/restricted_dba/superuser.c
 * -------------------------------------------------------------------------
 */

#include "postgres.h"

#include "catalog/pg_authid.h"
#include "miscadmin.h"

#include "restricted_dba.h"


/*
 * our version of superuser_arg()
 */
bool
se_role_is_superuser(Oid roleid)
{
	/* we should never get here if se_feature.mode is off */
	Assert(se_feature.mode);
	Assert(roleid != InvalidOid);

	/* Begin - ReqID:SRS-SECURITY-POWERS */
	/* since syssso or syssao may need superuser priv, we make them a temp superuser*/
	/* and this value could only set by code, so we always give code writer superuser privs */
	/* since 'PUBLIC' oid is also 0, we should only make a valid user a temp superuser  */
	if (se_feature.tmp_superuser_oid != InvalidOid &&
		se_feature.tmp_superuser_oid == roleid)
		return true;
	/* 
	 * some processes may also check superuser() during initialization.
	 * At that point sysdba_role haven't created.
	 * In other case, if se_feature.mode is on, make sure only sysdba_role
	 * member can be superuser.
	 */
	if (IsUnderPostmaster)
		return IS_SYSDBA(roleid);
	/* End - ReqID:SRS-SECURITY-POWERS */

	/* Special escape path in case you deleted all your users. */
	if (!IsUnderPostmaster && roleid == BOOTSTRAP_SUPERUSERID)
		return true;

	/* we skip cache lookup part, give it a quick result */

	return false;
}


bool 
se_role_has_createrole_priv(Oid roleid)
{
	Assert(se_feature.mode);
	if (IsUnderPostmaster)
		return IS_SYSDBA(roleid);

	return false;
}


bool 
se_role_has_createdb_priv(Oid roleid)
{
	Assert(se_feature.mode);
	if (IsUnderPostmaster)
		return IS_SYSDBA(roleid);
	
	return false;
}