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

// /*function warpper that hold all support function in extension*/
// SeMethod *se_method = NULL;
SeFeature se_feature = {
    false, 
    false, 
    InvalidOid,
    dummy_grant_sysdba_priv_fn
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

static void
dummy_grant_sysdba_priv_fn(Oid roleid, bool is_grant)
{
    /*do nothing*/
}