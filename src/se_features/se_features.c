/*-------------------------------------------------------------------------
 *
 * se_features.c
 *	  all secure enhancement features kernel implimentation.
 *    
 * This could be included by database kernels or other modules, so for 
 * those features that needed by kernel should be implimented here, other
 * implimentation may loaded via shared-load library.
 *
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1995, Regents of the University of California
 *
 * src/include/se_features_interface.h
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "utils/acl.h"

#include "se_features_interface.h"

static void dummy_grant_sysdba_priv_fn(Oid roleid, bool is_grant);
static bool dummy_role_is_superuser_fn(Oid roleid);
static bool dummy_role_has_createrole_priv_fn(Oid roleid);
static bool dummy_role_has_createdb_priv_fn(Oid roleid);

// /*function warpper that hold all support function in extension*/
// SeMethod *se_method = NULL;
SeFeature se_feature = {
    false, 
    false, 
    InvalidOid,
    dummy_grant_sysdba_priv_fn,
    dummy_role_is_superuser_fn,
	dummy_role_has_createrole_priv_fn,
	dummy_role_has_createdb_priv_fn
};


bool is_member_of_serole (Oid member, char* role_name)
{
	Oid role_id;
	bool result;

	role_id = get_role_oid(role_name, false);

	if(se_feature.mode)
		result = is_member_of_role_nosuper(member, role_id);
	else
		result = is_member_of_role(member, role_id);

	return result;
}

/*
 * Just like check_rolespec_name(), we should prevent user 
 * alter 3 security roles which created at initalization ,
 * also disallow user change its role attributes, 
 * just like how postgres treat those predefined roles
 */
void
check_se_rolespec_name(const RoleSpec *role, const char *detail_msg)
{
	if (!role)
		return;

	if (role->roletype != ROLESPEC_CSTRING)
		return;

	if (strcmp(role->rolename, SYSDBA_ROLE) == 0 ||
		strcmp(role->rolename, SYSSSO_ROLE) == 0 ||
		strcmp(role->rolename, SYSSAO_ROLE) == 0)
	{
		if (detail_msg)
			ereport(ERROR,
					(errcode(ERRCODE_RESERVED_NAME),
					 errmsg("role name \"%s\" is reserved",
							role->rolename),
					 errdetail_internal("%s", detail_msg)));
		else
			ereport(ERROR,
					(errcode(ERRCODE_RESERVED_NAME),
					 errmsg("role name \"%s\" is reserved",
							role->rolename)));
	}
}


/* dummy support functions that make compiler quiet */
static void
dummy_grant_sysdba_priv_fn(Oid roleid, bool is_grant)
{
    /*do nothing*/
}

static bool
dummy_role_is_superuser_fn(Oid roleid)
{
	Assert(se_feature.mode);
    elog(ERROR, "dummy_role_is_superuser() should never be called");
	return false;;
}

static bool
dummy_role_has_createrole_priv_fn(Oid roleid)
{
	Assert(se_feature.mode);
    elog(ERROR, "dummy_role_is_superuser() should never be called");
	return false;
}

static bool
dummy_role_has_createdb_priv_fn(Oid roleid)
{
	Assert(se_feature.mode);
    elog(ERROR, "dummy_role_is_superuser() should never be called");
	return false;
}