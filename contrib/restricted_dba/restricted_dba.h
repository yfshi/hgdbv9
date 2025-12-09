/* -------------------------------------------------------------------------
 *
 * File: restricted_dba.h
 *
 * Abstract: 
 * 		The header file that contains declarations and data structures
 *
 * Authored by sunchengxi@highgo.com, 20240510.
 *
 * Copyright:
 * 		Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/restricted_dba/restricted_dba.h
 * -------------------------------------------------------------------------
 */

#ifndef RESTRICTED_DBA_H
#define RESTRICTED_DBA_H
#include "postgres.h"

#include "catalog/objectaddress.h"
#include "nodes/parsenodes.h"
#include "parser/parse_node.h"
#include "storage/lwlock.h"
#include "utils/acl.h"

#include "se_features_interface.h"

#define MAX_SE_PARAMS_ENTRIES	16

#define SE_FEATURE_ON  (1 << 0)	// 0x0001
#define	RESTRICTED_DBA (1 << 1)	// 0x0002

typedef struct {
	int			target_backend_pid;
	Oid			target_db_id;
	int32		infos;
	int32		is_valid;
}SeParamsEntry;

typedef struct {
	LWLock		   *lock;
	SeParamsEntry   entries[MAX_SE_PARAMS_ENTRIES];
}SeParamCtl;

extern SeParamCtl *se_param_ctl;

/*aclchk.c*/
extern bool grant_object_check(GrantStmt *stmt);
extern AclMode pg_class_aclmask_nosuper(Oid table_oid, Oid roleid,
									 	AclMode mask, AclMaskHow how);
extern AclMode pg_class_aclmask_ext_nosuper(Oid table_oid, Oid roleid, AclMode mask,
											AclMaskHow how, bool *is_missing);
extern AclMode pg_namespace_aclmask_nosuper(Oid nsp_oid, Oid roleid,
											AclMode mask, AclMaskHow how);
extern AclResult pg_attribute_aclcheck_nosuper(Oid table_oid, AttrNumber attnum,
					  Oid roleid, AclMode mode);
extern AclResult pg_attribute_aclcheck_all_nosuper(Oid table_oid, Oid roleid, AclMode mode,
						  AclMaskHow how);
extern AclMode pg_proc_aclmask_nosuper(Oid proc_oid, Oid roleid,
										AclMode mask, AclMaskHow how);
extern AclResult pg_proc_aclcheck_nosuper(Oid proc_oid, Oid roleid, AclMode mode);


/*user.c*/
extern bool grant_role_check(GrantRoleStmt *stmt);
extern bool alter_role_check(AlterRoleStmt *stmt);
extern bool create_role_check(ParseState *pstate, CreateRoleStmt *stmt);
extern bool alter_role_set_check(AlterRoleSetStmt *stmt);
extern bool drop_role_check(DropRoleStmt *stmt);
extern bool rename_role_check(const char *oldname, const char *newname);
extern void grant_sysdba_priv(Oid roleid, bool is_grant);

/*alter.c*/
extern bool rename_object_check(RenameStmt *stmt);
extern bool alter_owner_check(AlterOwnerStmt *stmt);
extern bool alter_object_schema_check(AlterObjectSchemaStmt *stmt,
									  ObjectAddress *oldSchemaAddr);

/*database.c*/
extern bool alter_database_set_check(AlterDatabaseSetStmt *stmt);
extern bool create_db_check(ParseState *pstate, const CreatedbStmt *stmt);

/*instance.c*/
extern bool alter_system_set_check(AlterSystemStmt *altersysstmt);

/*tablespace.c*/
extern bool create_tablespace_check(CreateTableSpaceStmt *stmt);

/*schema.c*/
extern bool create_schema_check(CreateSchemaStmt *stmt, const char *queryString,
								int stmt_location, int stmt_len);
extern bool use_schema_check(Oid namespaceid, Oid roleid);
extern bool alter_schema_owner_check(const char *name, Oid newOwnerId);
extern bool alter_schema_rename_check(const char *oldname, const char *newname);
extern bool drop_schema_check(ObjectAddress *object);

/*relation.c*/
extern bool create_table_check(CreateStmt *stmt, char relkind, Oid ownerId,
							   ObjectAddress *typaddress, const char *queryString);
extern bool create_sequence_check(ParseState *pstate, CreateSeqStmt *seq);
extern bool create_view_check(ViewStmt *stmt, const char *queryString,
							   int stmt_location, int stmt_len);
extern bool create_index_check(IndexStmt *stmt);
extern bool alter_relation_check(AlterTableStmt *stmt);
extern bool alter_relation_rename_check(RenameStmt *stmt);
extern bool alter_relation_schema_check(AlterObjectSchemaStmt *stmt, Oid *oldschema);
extern bool drop_relation_check(DropStmt *stmt);
extern bool alter_sequence_check(AlterSeqStmt *stmt);
extern void truncate_relation_check(Oid relid);

/*proc.c*/
extern bool create_proc_check(ParseState *pstate, CreateFunctionStmt *stmt);
extern bool alter_proc_owner_check(AlterOwnerStmt *stmt, 
									const char *name, Oid newOwnerId);
extern bool alter_proc_rename_check(RenameStmt *stmt);
extern bool alter_proc_schema_check(AlterObjectSchemaStmt *stmt, Oid *oldschema);
extern bool alter_proc_check(ParseState *pstate, AlterFunctionStmt *stmt);
extern bool drop_proc_check(DropStmt *stmt);
extern bool compile_function_check(CompileFunctionStmt *stmt);

/*superuser.c*/
extern bool se_role_is_superuser(Oid roleid);
extern bool se_role_has_createrole_priv(Oid roleid);
extern bool se_role_has_createdb_priv(Oid roleid);
#endif