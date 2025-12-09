/* -------------------------------------------------------------------------
 *
 * File: proc.c
 *
 * Abstract: 
 * 		This file contains permission check of objects in pg_proc.
 * 		if any change in src/backend/commands/functioncmds.c, it's better
 * 		also change here.
 *
 * Authored by sunchengxi@highgo.com, 20240510.
 *
 * Copyright:
 * 		Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/restricted_dba/proc.c
 * -------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/genam.h"
#include "access/htup_details.h"
#include "access/sysattr.h"
#include "access/table.h"
#include "catalog/catalog.h"
#include "catalog/dependency.h"
#include "catalog/indexing.h"
#include "catalog/objectaccess.h"
#include "catalog/pg_aggregate.h"
#include "catalog/pg_cast.h"
#include "catalog/pg_language.h"
#include "catalog/pg_namespace.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_transform.h"
#include "catalog/pg_type.h"
#include "commands/alter.h"
#include "commands/defrem.h"
#include "commands/extension.h"
#include "commands/proclang.h"
/* Begin - ReqID:SRS-SQL-PACKAGE */
#include "commands/packagecmds.h"
/* End - ReqID:SRS-SQL-PACKAGE */
#include "executor/execdesc.h"
#include "executor/executor.h"
#include "executor/functions.h"
#include "funcapi.h"
#include "miscadmin.h"
#include "nodes/nodeFuncs.h"
#include "optimizer/optimizer.h"
#include "parser/analyze.h"
#include "parser/parse_coerce.h"
#include "parser/parse_collate.h"
#include "parser/parse_expr.h"
#include "parser/parse_func.h"
#include "parser/parse_type.h"
/* Begin - ReqID:SRS-SQL-PACKAGE */
#include "parser/parse_package.h"
/* End - ReqID:SRS-SQL-PACKAGE */
#include "pgstat.h"
#include "tcop/pquery.h"
#include "tcop/utility.h"
#include "utils/acl.h"
#include "utils/builtins.h"
/* Begin - ReqID:SRS-PLSQL-RECORD */
#include "utils/datum.h"
/* End - ReqID:SRS-PLSQL-RECORD */
#include "utils/fmgroids.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
/* BEGIN - SQL PARSER */
#include "utils/ora_compatible.h"
/* END - SQL PARSER */
#include "utils/rel.h"
#include "utils/snapmgr.h"
#include "utils/syscache.h"
#include "utils/typcache.h"
/* Begin - ReqID:SRS-SQL-ANONYOUT */
#include "parser/parse_param.h"
/* End - ReqID:SRS-SQL-ANONYOUT */
/* Begin - ReqID:SRS-SQL-LIBPQOUT */
#include "access/printtup.h"
/* End - ReqID:SRS-SQL-LIBPQOUT */
/* Begin - ReqID:SRS-PLSQL-TYPE */
#include "nodes/makefuncs.h"
#include "access/xact.h"
/* End - ReqID:SRS-PLSQL-TYPE */
/* BEGIN - ReqID:SRS-CMD-PGDUMP */
#include "parser/pg_object_hook.h"
#include "utils/jsonb.h"
#include "catalog/objectaddress.h"
/* END - ReqID:SRS-CMD-PGDUMP */
/* Begin - ReqID:SRS-MYSQL-CASE */
#include "mysql_parser/mys_case_sensitive.h"
/* End - ReqID:SRS-MYSQL-CASE */

#include "restricted_dba.h"

/*
 * CreateFunction
 *	 Execute a CREATE FUNCTION (or CREATE PROCEDURE) utility statement.
 */
bool
create_proc_check(ParseState *pstate, CreateFunctionStmt *stmt)
{
	Oid creater = GetUserId();
	Oid namespace_id;
	char *funcname;


	if (IS_SYSSSO(creater) || IS_SYSSAO(creater))
	{
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				errmsg("permission denied"),
				errdetail("member of syssso_role and syssao_role "
							"cannot create function/procedure")));
	}
	else if (IS_SYSDBA(creater))
	{
		namespace_id = QualifiedNameGetCreationNamespace(stmt->funcname,
														&funcname);
		/*check whewher creater able to use the schema*/
		use_schema_check(namespace_id,creater);

	}
	/*we leave nomral user to standard acl check*/

	return true;
}

bool
alter_proc_owner_check(AlterOwnerStmt *stmt, const char *name, Oid newOwnerId)
{
	Relation	rel;
	Oid			procoid;
	Oid			procowner;
	HeapTuple	tup;
	Form_pg_proc procform;
	bool		result = true;

	rel = table_open(ProcedureRelationId, RowShareLock);

	procoid = LookupFuncWithArgs(stmt->objectType, 
								 castNode(ObjectWithArgs, stmt->object),
								 false);

	tup = SearchSysCache1(PROCOID, procoid);

	/*
	 * we only care the relation existed case,
	 * leave non-exist case to original error check
	 */
	if (HeapTupleIsValid(tup))
	{
		procform = (Form_pg_proc) GETSTRUCT(tup);
		procowner = procform->proowner;

		if (IS_SYSSSO(procowner) || IS_SYSSAO(procowner))
		{
			result = false;
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					errmsg("permission denied"),
					errdetail("cannot alter function/procedure/routine of "
								"syssso_role/syssao_role member")));
		}
		else if (se_feature.restricted_dba && IS_SYSDBA(GetUserId()))
		{
			if (procowner != GetUserId())
			{
				ereport(ERROR,
						(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							errmsg("permission denied"),
							errdetail("sysdba_role member can only alter "
									"function/procedure/routine of itself")));
			}
		}

		if(IS_SYSSSO(newOwnerId) || IS_SYSSAO(newOwnerId))
		{	
			result = false;
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					errmsg("permission denied"),
					errdetail("syssso_role/syssao_role member "
							"cannot be owner of function/procedure/routine")));
		}
		else if (se_feature.restricted_dba && IS_SYSDBA(GetUserId()))
		{
			if (!IS_SYSDBA(newOwnerId))
			{
				ereport(ERROR,
						(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							errmsg("permission denied"),
							errdetail("only sysdba_role member is allowed to own a "
									  "function/procedure/routine of another "
									  "sysdba_role member")));
			}
		}

		ReleaseSysCache(tup);
	}

	table_close(rel, RowShareLock);

	return result;
}

bool alter_proc_rename_check(RenameStmt *stmt)
{
	Relation	rel;
	Oid			procid;
	HeapTuple	tup;
	Form_pg_proc procform;
	Oid			procowner;

	rel = table_open(ProcedureRelationId, RowShareLock);

	procid = LookupFuncWithArgs(stmt->renameType, 
								 castNode(ObjectWithArgs, stmt->object),
								 false);

	tup = SearchSysCache1(PROCOID, procid);

	/*
	 * we only care the relation existed case,
	 * leave non-exist case to original error check
	 */
	if (HeapTupleIsValid(tup))
	{
		procform = (Form_pg_proc) GETSTRUCT(tup);
		procowner = procform->proowner;

		if(IS_SYSSSO(procowner) || IS_SYSSAO(procowner))
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					errmsg("permission denied"),
					errdetail("cannot alter function/procedure/routine of "
								"syssso_role/syssao_role member")));

		if (se_feature.restricted_dba && IS_SYSDBA(GetUserId()))
		{
			if (procowner != GetUserId())
			{
				ereport(ERROR,
						(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							errmsg("permission denied"),
							errdetail("sysdba_role member can only alter "
									"function/procedure/routine of itself")));
			}
		}

		ReleaseSysCache(tup);
	}	

	table_close(rel, RowShareLock);

	return true;
}

bool
alter_proc_schema_check(AlterObjectSchemaStmt *stmt, Oid *oldschema)
{
	Relation	rel;
	Oid			procid;
	HeapTuple	tup;
	Form_pg_proc procform;
	Oid			procowner;
	Oid			newnspoid;
	ObjectWithArgs *func = castNode(ObjectWithArgs, stmt->object);

	rel = table_open(ProcedureRelationId, RowShareLock);

	procid = LookupFuncWithArgs(stmt->objectType, func, false);
	tup = SearchSysCache1(PROCOID, procid);

	/*
	 * we only care the relation existed case,
	 * leave non-exist case to original error check
	 */
	if (HeapTupleIsValid(tup))
	{
		procform = (Form_pg_proc) GETSTRUCT(tup);
		procowner = procform->proowner;

		if(IS_SYSSSO(procowner) || IS_SYSSAO(procowner))
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					errmsg("permission denied"),
					errdetail("cannot alter function/procedure/routine of "
								"syssso_role/syssao_role member")));
		}

		if (se_feature.restricted_dba && IS_SYSDBA(GetUserId()))
		{
			if (procowner != GetUserId())
			{
				ereport(ERROR,
						(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							errmsg("permission denied"),
							errdetail("sysdba_role member can only alter "
									"function/procedure/routine of itself")));
			}
		}

		newnspoid = LookupCreationNamespace(stmt->newschema);
		/*check whewher creater able to use the schema*/
		use_schema_check(newnspoid, GetUserId());

		ReleaseSysCache(tup);
	}

	table_close(rel, RowShareLock);

	return true;

}

bool
alter_proc_check(ParseState *pstate, AlterFunctionStmt *stmt)
{
	HeapTuple	tup;
	Oid			procid;
	Oid			procowner;
	Form_pg_proc procform;
	Relation	rel;


	rel = table_open(ProcedureRelationId, RowShareLock);

	procid = LookupFuncWithArgs(stmt->objtype, stmt->func, false);
	tup = SearchSysCache1(PROCOID, procid);

	/*
	 * we only care the relation existed case,
	 * leave non-exist case to original error check
	 */
	if (HeapTupleIsValid(tup))
	{
		procform = (Form_pg_proc) GETSTRUCT(tup);
		procowner = procform->proowner;

		if(IS_SYSSSO(procowner) || IS_SYSSAO(procowner))
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					errmsg("permission denied"),
					errdetail("cannot alter function/procedure/routine of "
								"syssso_role/syssao_role member")));
		}

		if (se_feature.restricted_dba && IS_SYSDBA(GetUserId()))
		{
			if (procowner != GetUserId())
			{
				ereport(ERROR,
						(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							errmsg("permission denied"),
							errdetail("sysdba_role member can only alter "
									"function/procedure/routine of itself")));
			}
		}

		ReleaseSysCache(tup);
	}

	table_close(rel, RowShareLock);

	return true;
}

bool drop_proc_check(DropStmt *stmt)
{
	ListCell   *cell;
	Relation	rel;

	rel = table_open(ProcedureRelationId, RowShareLock);

	foreach(cell, stmt->objects)
	{	
		HeapTuple	tup;
		Oid			procid;
		Oid			procowner;
		Form_pg_proc procform;
		ObjectWithArgs *func = castNode(ObjectWithArgs, lfirst(cell));
		
		procid = LookupFuncWithArgs(stmt->removeType, func, false);
		tup = SearchSysCache1(PROCOID, procid);

		/*
		 * we only care the relation existed case,
		 * leave non-exist case to original error check
		 */
		if (HeapTupleIsValid(tup))
		{
			procform = (Form_pg_proc) GETSTRUCT(tup);
			procowner = procform->proowner;

			if(IS_SYSSSO(procowner) || IS_SYSSAO(procowner))
			{
				ereport(ERROR,
						(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
						errmsg("permission denied"),
						errdetail("cannot drop function/procedure/routine of "
									"syssso_role/syssao_role member")));
			}

			if (se_feature.restricted_dba && IS_SYSDBA(GetUserId()))
			{
				if (procowner != GetUserId())
				{
					ereport(ERROR,
							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
								errmsg("permission denied"),
								errdetail("sysdba_role member can only drop "
										"function/procedure/routine of itself")));
				}
			}
		
			ReleaseSysCache(tup);
		}
	}

	table_close(rel, RowShareLock);

	return true;
}


/* 
 * Basiclly, this function did what func_execute_check() did , 
 * but just for compile statement; 
 */
bool
compile_function_check(CompileFunctionStmt *stmt)
{
	Oid 	proc_oid = InvalidOid;

	proc_oid = LookupFuncWithArgs(stmt->objtype, stmt->func, false);
	if (OidIsValid(proc_oid))
	{
		HeapTuple	tuple;
		Oid			ownerId;
		Form_pg_proc	procForm;	

		tuple = SearchSysCache1(PROCOID, ObjectIdGetDatum(proc_oid));
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
	return true;
}