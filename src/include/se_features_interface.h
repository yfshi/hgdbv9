/*-------------------------------------------------------------------------
 *
 * se_features_interface.h
 *	  all secure enhancement features interfaces
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
#ifndef SE_FEATURES_INTF_H
#define SE_FEATURES_INTF_H
#include "postgres.h"

typedef void (*grant_sysdba_priv_fn) (Oid roleid, bool is_grant);

typedef struct 
{
	bool mode;
	bool restricted_dba;
	Oid tmp_superuser_oid;
	/* use it with caution, it may make a normal user a superuser,maybe changed latter*/
	grant_sysdba_priv_fn grant_sysdba_priv;
}SeFeature;

extern PGDLLIMPORT SeFeature se_feature;

#define SYSDBA_ROLE "sysdba_role"
#define SYSSSO_ROLE "syssso_role"
#define SYSSAO_ROLE "syssao_role"


extern void create_db_seroles(Oid dboid, Oid templateid);
extern void drop_db_seroles(Oid dboid);
extern bool is_member_of_serole (Oid member, char* role_name);

#define IS_SYSDBA(role_oid) \
	(is_member_of_serole(role_oid,SYSDBA_ROLE))
#define IS_SYSSSO(role_oid) \
	(is_member_of_serole(role_oid,SYSSSO_ROLE))
#define IS_SYSSAO(role_oid) \
	(is_member_of_serole(role_oid,SYSSAO_ROLE))

#endif