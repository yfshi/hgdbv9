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
#include "nodes/parsenodes.h"

typedef void (*grant_sysdba_priv_hook_type) (Oid roleid, bool is_grant);
extern PGDLLIMPORT grant_sysdba_priv_hook_type grant_sysdba_priv_hook;

/*
 * Since we limit only sysdba_role can have SUPERUSER\CREATEROLE\CREATEDB
 * attributes, to bind these privileges with sysdba_role, beside we set 
 * these attributes to new sysdba_role member, we also need some support 
 * funciton to replace origin privilege check function. In these check
 * functions we ignore those non-sysdba_role user's privileges.
 */
typedef bool (*role_is_superuser_hook_type) (Oid roleid);
extern PGDLLIMPORT role_is_superuser_hook_type role_is_superuser_hook;

typedef bool (*role_has_createrole_priv_hook_type) (Oid roleid);
extern PGDLLIMPORT role_has_createrole_priv_hook_type role_has_createrole_priv_hook;

typedef bool (*role_has_createdb_priv_hook_type) (Oid roleid);
extern PGDLLIMPORT role_has_createdb_priv_hook_type role_has_createdb_priv_hook;

typedef bool (*use_sequence_check_hook_type) (Oid seq_oid, Oid roleid, AclMode mode);
extern PGDLLIMPORT use_sequence_check_hook_type use_sequence_check_hook;


/* use it with caution, it may make a normal user a superuser,maybe changed latter */
extern PGDLLIMPORT Oid tmp_superuser_oid;
/* se_feature switch */
extern PGDLLIMPORT bool se_feature_mode;
extern PGDLLIMPORT bool restricted_dba;


#define SYSDBA_ROLE "sysdba_role"
#define SYSSSO_ROLE "syssso_role"
#define SYSSAO_ROLE "syssao_role"


extern void create_db_seroles(Oid dboid, Oid templateid);
extern void drop_db_seroles(Oid dboid);
extern bool is_member_of_serole (Oid member, char* role_name);
extern void check_se_rolespec_name(const RoleSpec *role, const char *detail_msg);

#define IS_SYSDBA(role_oid) \
	(is_member_of_serole(role_oid,SYSDBA_ROLE))
#define IS_SYSSSO(role_oid) \
	(is_member_of_serole(role_oid,SYSSSO_ROLE))
#define IS_SYSSAO(role_oid) \
	(is_member_of_serole(role_oid,SYSSAO_ROLE))

#endif
