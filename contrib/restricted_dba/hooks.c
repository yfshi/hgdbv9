/* -------------------------------------------------------------------------
 *
 * File: hooks.c
 *
 * Abstract: 
 * 		This file contains extern declarations for GUC. and all hooks 
 * 		used by kernel.
 *
 * Authored by sunchengxi@highgo.com, 20240510.
 *
 * Copyright:
 * 		Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/restricted_dba/hooks.c
 * -------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/xact.h"
#include "catalog/dependency.h"
#include "catalog/objectaccess.h"
#include "catalog/pg_class.h"
#include "catalog/pg_tablespace.h"
#include "catalog/pg_database.h"
#include "catalog/pg_namespace.h"
#include "catalog/pg_extension.h"
#include "catalog/pg_proc.h"
#include "commands/copy.h"
#include "commands/extension.h"
#include "executor/executor.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "parser/parse_utilcmd.h"
#include "storage/ipc.h"
#include "storage/lwlock.h"
#include "storage/shmem.h"
#include "storage/spin.h"
#include "tcop/utility.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "utils/queryenvironment.h"
#include "utils/syscache.h"
#include "utils/elog.h"


#include "se_features_interface.h"
#include "restricted_dba.h"

PG_MODULE_MAGIC;

/*
 * Declarations
 */
void	_PG_init(void);
/*
 * Saved hook entries (if stacked)
 */
static object_access_hook_type next_object_access_hook = NULL;
static ExecutorCheckPerms_hook_type next_exec_check_perms_hook = NULL;
static ProcessUtility_hook_type next_ProcessUtility_hook = NULL;
static shmem_startup_hook_type prev_shmem_startup_hook = NULL;
static needs_fmgr_hook_type next_needs_fmgr_hook = NULL;


SeParamCtl *se_param_ctl = NULL;

static bool 
drop_check(DropStmt *stmt, bool isTopLevel)
{
	bool result = true;

	switch (stmt->removeType)
	{
		case OBJECT_INDEX:
		case OBJECT_TABLE:
		case OBJECT_SEQUENCE:
		case OBJECT_VIEW:
		case OBJECT_MATVIEW:
		case OBJECT_FOREIGN_TABLE:
			result = drop_relation_check(stmt);
			break;
		case OBJECT_FUNCTION:
		case OBJECT_PROCEDURE:
		case OBJECT_ROUTINE:
			result = drop_proc_check(stmt);
			break;
		default:
			/* do nothing for now*/
			break;
	}

	return result;
}

/*
 * restricted_utility
 *
 * a fine-grained checker function to block ddl command
 */
static void
restricted_utility(PlannedStmt *pstmt,
					const char *queryString,
					bool readOnlyTree,
					ProcessUtilityContext context,
					ParamListInfo params,
					QueryEnvironment *queryEnv,
					DestReceiver *dest,
					QueryCompletion *qc)
{
	bool	call_next = true;	
	Node   *parsetree = pstmt->utilityStmt;
	ParseState *pstate;
	bool		isTopLevel = (context == PROCESS_UTILITY_TOPLEVEL);

	pstate = make_parsestate(NULL);
	pstate->p_sourcetext = queryString;
	pstate->p_queryEnv = queryEnv;

	if (se_feature.mode)
	{
		switch (nodeTag(parsetree))
		{
			case T_AlterRoleStmt:
				/* no event triggers for global objects */
				call_next = alter_role_check((AlterRoleStmt *) parsetree);
				break;
			case T_AlterRoleSetStmt:
				call_next = alter_role_set_check((AlterRoleSetStmt *)  parsetree);
				break;
			case T_AlterDatabaseSetStmt:
				call_next = alter_database_set_check((AlterDatabaseSetStmt *) parsetree);
				break;
			case T_AlterOwnerStmt:
				call_next = alter_owner_check((AlterOwnerStmt *) parsetree);
				break;
			case T_AlterSystemStmt:
				PreventInTransactionBlock(isTopLevel, "ALTER SYSTEM");
				call_next = alter_system_set_check((AlterSystemStmt *) parsetree);
				break;
			case T_CopyStmt:
				{
					uint64		processed;
					call_next = true;

					if((IS_SYSSSO(GetUserId()) || IS_SYSSAO(GetUserId())) &&
						!((CopyStmt *) parsetree)->is_from)
					{
						se_feature.tmp_superuser_oid = GetUserId();
						PG_TRY();
						{	
							DoCopy(pstate, (CopyStmt *) parsetree,
									pstmt->stmt_location, pstmt->stmt_len,
									&processed);
							if (qc)
								SetQueryCompletion(qc, CMDTAG_COPY, processed);
						}
						PG_FINALLY();
						{
							se_feature.tmp_superuser_oid = InvalidOid;
						}
						PG_END_TRY();
						se_feature.tmp_superuser_oid = InvalidOid;
						call_next = false;
					}
				}
				break;
			case T_CreatedbStmt:
				call_next = create_db_check(pstate,(CreatedbStmt *)parsetree);
				break;
			case T_CreateRoleStmt:
				call_next = create_role_check(pstate, (CreateRoleStmt *) parsetree);
				break;
			case T_DropRoleStmt:
				call_next = drop_role_check((DropRoleStmt *) parsetree);
				break;
			case T_GrantRoleStmt:
				/* no event triggers for global objects */
				call_next = grant_role_check((GrantRoleStmt *) parsetree);
				break;
			case T_CreateTableSpaceStmt:
				call_next = create_tablespace_check((CreateTableSpaceStmt*) parsetree);
				break;
			case T_CreateSchemaStmt:
				call_next = create_schema_check((CreateSchemaStmt *) parsetree,
												queryString,
												pstmt->stmt_location,
												pstmt->stmt_len);
				break;
			/* Begin - bug#z6270 */
			case T_CreateTableAsStmt:
				call_next = create_table_as_check((CreateTableAsStmt *) parsetree);
				break;
			/* End - bug#z6270 */
			case T_CreateStmt:
			case T_CreateForeignTableStmt:
				{
					/* transformCreateStmt() may modify the parsetree, so make a copy */
					Node *cparsetree = copyObject(parsetree);
					List	   *stmts;

					/* Run parse analysis ... */
					stmts = transformCreateStmt((CreateStmt *)cparsetree,
												queryString);

					/*
					 * ... and do it.  We can't use foreach() because we may
					 * modify the list midway through, so pick off the
					 * elements one at a time, the hard way.
					 */
					while (stmts != NIL)
					{
						Node	   *stmt = (Node *) linitial(stmts);

						stmts = list_delete_first(stmts);

						if (IsA(stmt, CreateStmt))
						{
							CreateStmt *cstmt = (CreateStmt *) stmt;

							/* Create the table itself */
							create_table_check(cstmt,
												RELKIND_RELATION,
												InvalidOid, NULL,
												queryString);
						}
						else if (IsA(stmt, CreateForeignTableStmt))
						{
							CreateForeignTableStmt *cstmt = (CreateForeignTableStmt *) stmt;

							/* Create the table itself */
							create_table_check(&cstmt->base,
												RELKIND_FOREIGN_TABLE,
												InvalidOid, NULL,
												queryString);

						}	
						/*don't care about others */
					}
				}
				call_next = true;
				break;
			case T_CreateSeqStmt:
				call_next = create_sequence_check(pstate, (CreateSeqStmt *) parsetree);
				break;

			case T_ViewStmt:	/* CREATE VIEW */
				call_next = create_view_check((ViewStmt *) parsetree, queryString,
												pstmt->stmt_location, pstmt->stmt_len);
				break;
			case T_IndexStmt:	/* CREATE INDEX */
				call_next = create_index_check((IndexStmt *) parsetree);
				break;
			case T_AlterTableStmt:
				call_next = alter_relation_check((AlterTableStmt *) parsetree);
				break;
			case T_DropStmt:
				call_next = drop_check((DropStmt *) parsetree, isTopLevel);
				break;
			case T_CreateFunctionStmt:	/* CREATE FUNCTION */
				call_next = create_proc_check(pstate, (CreateFunctionStmt *) parsetree);
				break;
			case T_AlterFunctionStmt:	/* CREATE FUNCTION */
				call_next = alter_proc_check(pstate, (AlterFunctionStmt *) parsetree);
				break;
			case T_RenameStmt:
				call_next = rename_object_check((RenameStmt *) parsetree);
				break;
			case T_GrantStmt:
				call_next = grant_object_check((GrantStmt *) parsetree);
				break;
			case T_AlterObjectSchemaStmt:
				call_next = alter_object_schema_check((AlterObjectSchemaStmt *) parsetree, NULL);
				break;
			case T_AlterSeqStmt:
				call_next = alter_sequence_check((AlterSeqStmt *) parsetree);
				break;
			case T_CreateExtensionStmt:
				{
					bool saved_se_feature_param = se_feature.mode;

					PG_TRY();
					{	
						se_feature.mode = false;
						CreateExtension(pstate, (CreateExtensionStmt *) parsetree);
					}
					PG_FINALLY();
					{
						se_feature.mode = saved_se_feature_param;
					}
					PG_END_TRY();
					call_next = false;
				}
				break;

			case T_AlterExtensionStmt:
				{
					bool saved_se_feature_param = se_feature.mode;

					PG_TRY();
					{	
						se_feature.mode = false;
						ExecAlterExtensionStmt(pstate, (AlterExtensionStmt *) parsetree);
					}
					PG_FINALLY();
					{
						se_feature.mode = saved_se_feature_param;
					}
					PG_END_TRY();
					call_next = false;
				}
				break;
			case T_CompileFunctionStmt:		/*bug z4253*/
				call_next = compile_function_check((CompileFunctionStmt *) parsetree);
				break;

			default:

				/*
					* Right now we don't check any other utility commands,
					* because it needs more detailed information to make access
					* control decision here, but we don't want to have two parse
					* and analyze routines individually.
					*/
				break;
		}

	}

	free_parsestate(pstate);

	if (call_next)
	{
		/* if not error out above, proceed the execution */
		if (next_ProcessUtility_hook)
			(*next_ProcessUtility_hook) (pstmt, queryString, readOnlyTree,
											context, params, queryEnv,
											dest, qc);
		else
			standard_ProcessUtility(pstmt, queryString, readOnlyTree,
									context, params, queryEnv,
									dest, qc);
	}
}

static void
se_feature_shmem_startup(void)
{
	bool found;

	if (prev_shmem_startup_hook)
		prev_shmem_startup_hook();

	/* reset in case this is a restart within the postmaster */
	se_param_ctl = NULL;

	/*
	 * Create or attach to the shared memory state
	 */
	LWLockAcquire(AddinShmemInitLock, LW_EXCLUSIVE);

	se_param_ctl = ShmemInitStruct("se_feature",
								   sizeof(SeParamCtl),
								   &found);

	if (!found)
	{
		int i;
		/* First time through ... */
		se_param_ctl->lock = &(GetNamedLWLockTranche("se_feature"))->lock;
		for (i = 0; i < MAX_SE_PARAMS_ENTRIES; i++)
		{
			se_param_ctl->entries[i].is_valid = 0;
			se_param_ctl->entries[i].target_db_id = InvalidOid;
			se_param_ctl->entries[i].target_backend_pid = InvalidBackendId;
		}
	}

	LWLockRelease(AddinShmemInitLock);

}

bool restricted_dba_check (bool *newval, void **extra, GucSource source);
void restricted_dba_assign (bool newval, void *extra);
const char * restricted_dba_show (void);


bool
restricted_dba_check (bool *newval, void **extra, GucSource source)
{
	// if(!se_feature.mode && *newval)
	// {
	// 	// ereport(WARNING,
	// 	// 		(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
	// 	// 		 errmsg("se_feature.mode is currently off, "
	// 	// 				"setting restricted_dba is invalid")));
	// 	return false;
	// }

	if (source < PGC_S_TEST)
		return true;

	if (IS_SYSSSO(GetUserId()))
	{
		return true;
	}
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("Only syssso users can set this value")));
		return false;
	}
}

void 
restricted_dba_assign (bool newval, void *extra)
{
	/*currently do nothing*/
}

const char * 
restricted_dba_show (void)
{
	return (se_feature.mode && se_feature.restricted_dba) ? "on" : "off";
}

bool se_feature_mode_check (bool *newval, void **extra, GucSource source);
void se_feature_mode_assign (bool newval, void *extra);
const char * se_feature_mode_show (void);


bool
se_feature_mode_check (bool *newval, void **extra, GucSource source)
{
	if (source < PGC_S_TEST)
		return true;

	if (IS_SYSSSO(GetUserId()))
	{
		return true;
	}
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("Only syssso users can set this value")));
		return false;
	}
}

void 
se_feature_mode_assign (bool newval, void *extra)
{
	/*currently do nothing*/
}

const char * 
se_feature_mode_show (void)
{
	return se_feature.mode ? "on" : "off";
}


/*
 * check_rte_perms_modified - similar to ExecCheckRTPerms.
 *
 * Check INSERT or UPDATE access permissions for a single RTE (these are processed uniformly).
 */
static bool
check_rte_perms_modified(Oid relOid, Oid userid, Bitmapset *modifiedCols,
						  AclMode requiredPerms)
{
	int			col = -1;

	/*
	 * When the query doesn't explicitly update any columns, allow the query
	 * if we have permission on any column of the rel.  This is to handle
	 * SELECT FOR UPDATE as well as possible corner cases in UPDATE.
	 */
	if (bms_is_empty(modifiedCols))
	{
		if (pg_attribute_aclcheck_all_nosuper(relOid, userid, requiredPerms,
									  ACLMASK_ANY) != ACLCHECK_OK)
			return false;
	}

	while ((col = bms_next_member(modifiedCols, col)) >= 0)
	{
		/* bit #s are offset by FirstLowInvalidHeapAttributeNumber */
		AttrNumber	attno = col + FirstLowInvalidHeapAttributeNumber;

		if (attno == InvalidAttrNumber)
		{
			/* whole-row reference can't happen here */
			elog(ERROR, "whole-row update is not implemented");
		}
		else
		{
			if (pg_attribute_aclcheck_nosuper(relOid, attno, userid,
									  requiredPerms) != ACLCHECK_OK)
				return false;
		}
	}
	return true;
}

/*
 * check_rte_perms
 *
 * Check access permissions for a single RTE.
 * Similar to ExecCheckRTEPerms
 */
static bool
check_rte_perms(RangeTblEntry *rte)
{
	AclMode		requiredPerms;
	AclMode		relPerms;
	AclMode		remainingPerms;
	Oid			relOid;
	Oid			userid;

	/*
	 * No work if requiredPerms is empty.
	 */
	requiredPerms = rte->requiredPerms;
	if (requiredPerms == 0)
		return true;

	relOid = rte->relid;

	/*
	 * userid to check as: current user unless we have a setuid indication.
	 *
	 * Note: GetUserId() is presently fast enough that there's no harm in
	 * calling it separately for each RTE.  If that stops being true, we could
	 * call it once in ExecCheckRTPerms and pass the userid down from there.
	 * But for now, no need for the extra clutter.
	 */
	userid = rte->checkAsUser ? rte->checkAsUser : GetUserId();

	/*
	 * We must have *all* the requiredPerms bits, but some of the bits can be
	 * satisfied from column-level rather than relation-level permissions.
	 * First, remove any bits that are satisfied by relation permissions.
	 */
	relPerms = pg_class_aclmask_nosuper(relOid, userid, requiredPerms, ACLMASK_ALL);
	remainingPerms = requiredPerms & ~relPerms;
	if (remainingPerms != 0)
	{
		int			col = -1;

		/*
		 * If we lack any permissions that exist only as relation permissions,
		 * we can fail straight away.
		 */
		if (remainingPerms & ~(ACL_SELECT | ACL_INSERT | ACL_UPDATE))
			return false;

		/*
		 * Check to see if we have the needed privileges at column level.
		 *
		 * Note: failures just report a table-level error; it would be nicer
		 * to report a column-level error if we have some but not all of the
		 * column privileges.
		 */
		if (remainingPerms & ACL_SELECT)
		{
			/*
			 * When the query doesn't explicitly reference any columns (for
			 * example, SELECT COUNT(*) FROM table), allow the query if we
			 * have SELECT on any column of the rel, as per SQL spec.
			 */
			if (bms_is_empty(rte->selectedCols))
			{
				if (pg_attribute_aclcheck_all_nosuper(relOid, userid, ACL_SELECT,
											  ACLMASK_ANY) != ACLCHECK_OK)
					return false;
			}

			while ((col = bms_next_member(rte->selectedCols, col)) >= 0)
			{
				/* bit #s are offset by FirstLowInvalidHeapAttributeNumber */
				AttrNumber	attno = col + FirstLowInvalidHeapAttributeNumber;

				if (attno == InvalidAttrNumber)
				{
					/* Whole-row reference, must have priv on all cols */
					if (pg_attribute_aclcheck_all_nosuper(relOid, userid, ACL_SELECT,
												  ACLMASK_ALL) != ACLCHECK_OK)
						return false;
				}
				else
				{
					if (pg_attribute_aclcheck_nosuper(relOid, attno, userid,
											  ACL_SELECT) != ACLCHECK_OK)
						return false;
				}
			}
		}

		/*
		 * Basically the same for the mod columns, for both INSERT and UPDATE
		 * privilege as specified by remainingPerms.
		 */
		if (remainingPerms & ACL_INSERT && !check_rte_perms_modified(relOid,
																	  userid,
																	  rte->insertedCols,
																	  ACL_INSERT))
			return false;

		if (remainingPerms & ACL_UPDATE && !check_rte_perms_modified(relOid,
																	  userid,
																	  rte->updatedCols,
																	  ACL_UPDATE))
			return false;
	}
	return true;
}

static bool
se_feature_check_perms(List *rangeTabls, bool abort)
{
	ListCell   *l;
	bool		result = true;

	if(se_feature.mode)
	{
		foreach(l, rangeTabls)
		{
			RangeTblEntry *rte = (RangeTblEntry *) lfirst(l);
			Oid			relOid = rte->relid;
			HeapTuple	tup;
			Form_pg_class classform;
			Oid			relowner;

			if (rte->rtekind != RTE_RELATION)
				continue;

			tup = SearchSysCache1(RELOID, ObjectIdGetDatum(relOid));
			if (HeapTupleIsValid(tup))
			{
				classform = (Form_pg_class) GETSTRUCT(tup);
				relowner = classform->relowner;

				if(IS_SYSSSO(relowner) || IS_SYSSAO(relowner))
				{
					result = check_rte_perms(rte);
					if (!result)
					{
						Assert(rte->rtekind == RTE_RELATION);
						if (abort)
							aclcheck_error(ACLCHECK_NO_PRIV, get_relkind_objtype(get_rel_relkind(rte->relid)),
										get_rel_name(rte->relid));
						return false;
					}
				}
				else if (se_feature.restricted_dba && IS_SYSDBA(GetUserId()))
				{
					result = check_rte_perms(rte);
					if (!result)
					{
						Assert(rte->rtekind == RTE_RELATION);
						if (abort)
							aclcheck_error(ACLCHECK_NO_PRIV, get_relkind_objtype(get_rel_relkind(rte->relid)),
										get_rel_name(rte->relid));
						return false;
					}
				}
			}
			
			ReleaseSysCache(tup);
		}
	}

	if (next_exec_check_perms_hook)
		result = (*next_exec_check_perms_hook) (rangeTabls,
												abort);
	return result;
}

/*
 * sepgsql_object_access
 *
 * Entrypoint of the object_access_hook. This routine performs as
 * a dispatcher of invocation based on access type and object classes.
 */
static void
object_access_check(ObjectAccessType access,
					Oid classId,
					Oid objectId,
					int subId,
					void *arg)
{
	if (se_feature.mode)
	{
		switch (access)
		{
			case OAT_POST_CREATE:
			{
				/*currently do nothing*/
			}
			break;

			case OAT_DROP:
			{
				ObjectAccessDrop *drop_arg = (ObjectAccessDrop *) arg;
				ObjectAddress object;
				char	   *audit_name;

				object.classId = classId;
				object.objectId = objectId;
				object.objectSubId = 0;
				
				/*
				* No need to apply permission checks on object deletion due
				* to internal cleanups; such as removal of temporary database
				* object on session closed.
				*/
				if ((drop_arg->dropflags & PERFORM_DELETION_INTERNAL) != 0)
					break;

				switch (classId)
				{
					case ExtensionRelationId:
					{
						/* Begin -- bug#z5554 */
						audit_name = getObjectIdentity(&object, false);
						/* End -- bug#z5554 */
						if (pg_strcasecmp(audit_name,"restricted_dba") == 0)
							ereport(ERROR,
									(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
									 errmsg("permission denied"),
									 errdetail("cannot drop extension \"%s\" while se_feature.mode = on",
									 			audit_name)));
					}
					break;
					case NamespaceRelationId:
					{
						drop_schema_check(&object);
					}		
					break;

					default:
						/* Ignore unsupported object classes */
						break;
				}
			}
			break;

			case OAT_TRUNCATE:
			{
				/* bug#z6271: add truncate check */
				truncate_relation_check(objectId);
			}
			break;

			case OAT_POST_ALTER:
			{
				/*do nothing*/
			}
			break;

			case OAT_NAMESPACE_SEARCH:
			{
				/*do nothing*/
			}
			break;

			case OAT_FUNCTION_EXECUTE:
			{
				/*do nothing*/
			}
			break;

			default:
				elog(ERROR, "unexpected object access type: %d", (int) access);
				break;
		}
	}

	if (next_object_access_hook)
		(*next_object_access_hook) (access, classId, objectId, subId, arg);
}


/*
 * Function execution check for se_feature check
 * 
 * This function is attached on needs_fmgr_hook, it used to check whewher
 * a function need to call an extra fmgr handler. it will be called for all
 * function. but we only care some of them.
 */
static bool
func_execute_check(Oid proc_oid)
{
	bool result = false;
	HeapTuple	tuple;
	Form_pg_proc procForm;
	Oid			ownerId;

	/* get origin result */
	if (next_needs_fmgr_hook)
		result = next_needs_fmgr_hook(proc_oid);

	/* if se_feature.mode is off, just return */
	if (!se_feature.mode)
		return result;

	/*
	 * we only care about tuple valid case
	 */
	tuple = SearchSysCache1(PROCOID, ObjectIdGetDatum(proc_oid));
	if (HeapTupleIsValid(tuple))
	{
		procForm = (Form_pg_proc) GETSTRUCT(tuple);
		ownerId = procForm->proowner;

		/* currently we only care procedure and function*/
		if (procForm->prokind == PROKIND_FUNCTION ||
			procForm->prokind == PROKIND_PROCEDURE)
		{
			AclResult	aclresult;
			ObjectType	obj_type = OBJECT_FUNCTION;

			if (procForm->prokind == PROKIND_FUNCTION)
				obj_type = OBJECT_FUNCTION;
			else if (procForm->prokind == PROKIND_PROCEDURE)
				obj_type = OBJECT_PROCEDURE;

			if (IS_SYSSSO(ownerId) || IS_SYSSAO(ownerId))
			{
				aclresult = pg_proc_aclcheck_nosuper(proc_oid, GetUserId(),
													 ACL_EXECUTE);
			
				if (aclresult != ACLCHECK_OK)
					aclcheck_error(aclresult, obj_type,
									get_func_name(proc_oid));
			}
			else if (se_feature.restricted_dba && 
					 procForm->pronamespace != PG_CATALOG_NAMESPACE &&
					 procForm->pronamespace != PG_PUBLIC_NAMESPACE &&
					 procForm->pronamespace != PG_SYS_NAMESPACE)
					 /* Bug#z6190: we skip checking function under sys schema */
			{
				/*
				 * Note: for those function in pg_catalog schema we skip extra
				 * check in case other dba user cannot call it. 
				 */
				aclresult = pg_proc_aclcheck_nosuper(proc_oid, GetUserId(),
													 ACL_EXECUTE);
			
				if (aclresult != ACLCHECK_OK)
					aclcheck_error(aclresult, obj_type,
								get_func_name(proc_oid));
			}
		}
		ReleaseSysCache(tuple);
	}

	return result;
}

/*
 * Module load/unload callback
 */
void
_PG_init(void)
{

	/*
	 * must load under postmaster 
	 */
#ifndef WIN32
	if (IsUnderPostmaster)
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("restricted_dba must be loaded via shared_preload_libraries")));
#endif


	/*
	 * restricted_dba = (on|off)
	 *
	 * This variable controls restricted_dba mode is on or off
	 */
	DefineCustomBoolVariable("se_feature.restricted_dba",
							 "Turn on/off  restricted_dba",
							 NULL,
							 &se_feature.restricted_dba,
							 false,
							 PGC_USERSET,
							 GUC_NOT_IN_SAMPLE | GUC_NO_RESET_ALL,
							 restricted_dba_check,
							 restricted_dba_assign,
							 restricted_dba_show);

	DefineCustomBoolVariable("se_feature.mode",
							 "Turn on/off  se_features",
							 NULL,
							 &se_feature.mode,
							 false,
							 PGC_USERSET,
							 GUC_NOT_IN_SAMPLE | GUC_NO_RESET_ALL,
							 se_feature_mode_check,
							 se_feature_mode_assign,
							 se_feature_mode_show);

	RequestAddinShmemSpace(MAXALIGN(sizeof(SeParamCtl)));
	RequestNamedLWLockTranche("se_feature", 1);

	prev_shmem_startup_hook = shmem_startup_hook;
	shmem_startup_hook = se_feature_shmem_startup;

	// /* Object access hook */
	next_object_access_hook = object_access_hook;
	object_access_hook = object_access_check;

	/* DML permission check */
	/*Begin -- bug#z4735*/
	next_exec_check_perms_hook = ExecutorCheckPerms_hook;
	ExecutorCheckPerms_hook = se_feature_check_perms;
	/*End -- bug#z4735*/

	/* ProcessUtility hook */
	next_ProcessUtility_hook = ProcessUtility_hook;
	ProcessUtility_hook = restricted_utility;

	next_needs_fmgr_hook = needs_fmgr_hook;
	needs_fmgr_hook = func_execute_check;

	se_feature.grant_sysdba_priv = grant_sysdba_priv;
	se_feature.role_is_superuser = se_role_is_superuser;
	se_feature.role_has_createrole_priv = se_role_has_createrole_priv;
	se_feature.role_has_createdb_priv = se_role_has_createdb_priv;
	se_feature.use_sequence_check = se_use_sequence_check;

	// /* init contextual info */
	// memset(&sepgsql_context_info, 0, sizeof(sepgsql_context_info));
}
