/* -------------------------------------------------------------------------
 *
 * File: relation.c
 *
 * Abstract: 
 * 		This file contains permission check of objects in pg_class.
 * 		if any change in src/backend/commands/tablecmds.c, it's better
 * 		also change here.
 *
 * Authored by sunchengxi@highgo.com, 20240510.
 *
 * Copyright:
 * 		Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/restricted_dba/relation.c
 * -------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/attmap.h"
#include "access/genam.h"
#include "access/heapam.h"
#include "access/heapam_xlog.h"
#include "access/multixact.h"
#include "access/reloptions.h"
#include "access/relscan.h"
#include "access/sysattr.h"
#include "access/tableam.h"
#include "access/toast_compression.h"
#include "access/xact.h"
#include "access/xlog.h"
#include "catalog/catalog.h"
#include "catalog/heap.h"
#include "catalog/index.h"
#include "catalog/namespace.h"
#include "catalog/objectaccess.h"
#include "catalog/partition.h"
#include "catalog/pg_am.h"
#include "catalog/pg_collation.h"
#include "catalog/pg_constraint.h"
#include "catalog/pg_depend.h"
#include "catalog/pg_foreign_table.h"
#include "catalog/pg_inherits.h"
#include "catalog/pg_namespace.h"
#include "catalog/pg_opclass.h"
#include "catalog/pg_rewrite.h" /* ReqID:SRS-SQL-FORCEVIEW */
#include "catalog/pg_tablespace.h"
#include "catalog/pg_statistic_ext.h"
#include "catalog/pg_trigger.h"
#include "catalog/pg_type.h"
#include "catalog/storage.h"
#include "catalog/storage_xlog.h"
#include "catalog/toasting.h"
#include "catalog/storage_gtt.h"
#include "commands/cluster.h"
#include "commands/comment.h"
#include "commands/defrem.h"
#include "commands/event_trigger.h"
#include "commands/policy.h"
#include "commands/sequence.h"
#include "commands/tablecmds.h"
#include "commands/tablespace.h"
#include "commands/trigger.h"
#include "commands/typecmds.h"
#include "commands/user.h"
#include "commands/view.h"	/* ReqID:SRS-SQL-FORCEVIEW */
#include "executor/executor.h"
#include "foreign/fdwapi.h"
#include "foreign/foreign.h"
#include "miscadmin.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "nodes/parsenodes.h"
#include "optimizer/optimizer.h"
#include "parser/parse_clause.h"
#include "parser/parse_coerce.h"
#include "parser/parse_collate.h"
#include "parser/parse_expr.h"
#include "parser/parse_oper.h"
#include "parser/parse_relation.h"
#include "parser/parse_type.h"
#include "parser/parse_utilcmd.h"
#include "parser/parser.h"
#include "parser/pg_object_hook.h"
#include "partitioning/partbounds.h"
#include "partitioning/partdesc.h"
#include "pgstat.h"
#include "rewrite/rewriteDefine.h"
#include "rewrite/rewriteHandler.h"
#include "rewrite/rewriteManip.h"
#include "storage/bufmgr.h"
#include "storage/lmgr.h"
#include "storage/lock.h"
#include "storage/predicate.h"
#include "storage/smgr.h"
#include "tcop/utility.h"
#include "utils/acl.h"
#include "utils/attoptcache.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/inval.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/partcache.h"
#include "utils/relcache.h"
#include "utils/ruleutils.h"
#include "utils/snapmgr.h"
#include "utils/syscache.h"
#include "utils/timestamp.h"
#include "utils/typcache.h"
/* Begin - ReqID:SRS-PLSQL-TYPE */
#include "utils/guc.h"
#include "utils/ora_compatible.h"
#include "utils/packagecache.h"
#include "funcapi.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_package_body.h"
#include "lib/qunique.h"
/* End - ReqID:SRS-PLSQL-TYPE */
/* Begin - BUG:M0000325 */
#include "postmaster/bgwriter.h"
/* End - BUG:M0000325 */
/* BEGIN - ReqID:SRS-CMD-PGDUMP */
#include "parser/pg_object_hook.h"
#include "utils/jsonb.h"
#include "catalog/objectaddress.h"
/* END - ReqID:SRS-CMD-PGDUMP */
/* Begin - ReqID:SRS-MYSQL-CASE */
#include "mysql_parser/mys_case_sensitive.h"
/* End - ReqID:SRS-MYSQL-CASE */

#include "catalog/dependency.h" /* BUG#Z1279 */
#ifdef ENABLE_AUDIT
#include "pg_contrib_audit.h"
#endif


#include "restricted_dba.h"


bool
create_table_check(CreateStmt *stmt, char relkind, Oid ownerId,
				   ObjectAddress *typaddress, const char *queryString)
{

	Oid creater = OidIsValid(ownerId) ? ownerId : GetUserId();
	Oid namespace_id;

	if (IS_SYSSSO(creater) || IS_SYSSAO(creater))
	{
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				errmsg("permission denied"),
				errdetail("member of syssso_role and syssao_role "
							"cannot create table")));
	}
	else if (IS_SYSDBA(creater))
	{
		/*make sure we get the active namespace id we create table on*/
		namespace_id = RangeVarGetCreationNamespace(stmt->relation);
		/*check whewher creater able to use the schema*/
		use_schema_check(namespace_id,creater);
	
	}
	/*we leave nomral user to standard acl check*/

	
	return true;
}

bool
create_sequence_check(ParseState *pstate, CreateSeqStmt *seq)
{
	Oid creater = GetUserId();
	Oid namespace_id;

	if (IS_SYSSSO(creater) || IS_SYSSAO(creater))
	{
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				errmsg("permission denied"),
				errdetail("member of syssso_role and syssao_role "
							"cannot create sequence")));
	}
	else if (IS_SYSDBA(creater))
	{
		/*make sure we get the active namespace id */
		namespace_id = RangeVarGetCreationNamespace(seq->sequence);
		/*check whewher creater able to use the schema*/
		use_schema_check(namespace_id,creater);
	
	}
	/*we leave nomral user to standard acl check*/

	return true;
}


bool
create_view_check(ViewStmt *stmt, const char *queryString,
		   int stmt_location, int stmt_len)
{
	Oid creater = GetUserId();
	Oid namespace_id;

	if (IS_SYSSSO(creater) || IS_SYSSAO(creater))
	{
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				errmsg("permission denied"),
				errdetail("member of syssso_role and syssao_role "
							"cannot create view")));
	}
	else if (IS_SYSDBA(creater))
	{
		/*make sure we get the active namespace id we create table on*/
		namespace_id = RangeVarGetCreationNamespace(stmt->view);
		/*check whewher creater able to use the schema*/
		use_schema_check(namespace_id,creater);
	
	}
	/*we leave nomral user to standard acl check*/

	return true;
}

bool
create_index_check(IndexStmt *stmt)
{
	Oid creater = GetUserId();

	if (IS_SYSSSO(creater) || IS_SYSSAO(creater))
	{
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				errmsg("permission denied"),
				errdetail("member of syssso_role and syssao_role "
							"cannot create index")));
	}
	else if (IS_SYSDBA(creater))
	{
		HeapTuple	tuple;
		Oid			ownerId;
		Oid			reloid;

		reloid = RangeVarGetRelid(stmt->relation, AccessShareLock, false);

		tuple = SearchSysCache1(RELOID, ObjectIdGetDatum(reloid));
		if (HeapTupleIsValid(tuple))
		{
			ownerId = ((Form_pg_class) GETSTRUCT(tuple))->relowner;
			
			if(IS_SYSSSO(ownerId) || IS_SYSSAO(ownerId))
				ereport(ERROR,
						(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
						errmsg("permission denied"),
						errdetail("member of sysdba_role can only create index "
									"on its own relation")));

			if (se_feature.restricted_dba && (ownerId != GetUserId()))
				ereport(ERROR,
						(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							errmsg("permission denied"),
							errdetail("member of sysdba_role can only create index "
									"on its own relation")));

			ReleaseSysCache(tuple);
		}

		UnlockRelationOid(reloid, AccessShareLock);
	}

	return true;
}

bool
alter_relation_check(AlterTableStmt *stmt)
{
	ListCell *lcmd;
	Oid		relid;

	relid = AlterTableLookupRelation(stmt, NoLock);

	/*
	 * we only care the relation existed case,
	 * leave non-exist case to original error check
	 */
	if (OidIsValid(relid))
	{

		HeapTuple	tuple;
		Oid			ownerId;

		tuple = SearchSysCache1(RELOID, ObjectIdGetDatum(relid));

		ownerId = ((Form_pg_class) GETSTRUCT(tuple))->relowner;

		if (IS_SYSSSO(ownerId) || IS_SYSSAO(ownerId))
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					errmsg("permission denied"),
					errdetail("cannot alter relation of syssso_role/syssao_role"
								" member")));			
		}

		if (se_feature.restricted_dba && IS_SYSDBA(GetUserId()))
		{
			if (ownerId != GetUserId())
			{
				ereport(ERROR,
						(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							errmsg("permission denied"),
							errdetail("sysdba_role member can only alter relation of "
									"itself")));
			}
		}

		ReleaseSysCache(tuple);

		foreach(lcmd, stmt->cmds)
		{
			AlterTableCmd *cmd = (AlterTableCmd *) lfirst(lcmd);

			if(cmd->subtype == AT_ChangeOwner)
			{
				Oid newowner = get_rolespec_oid(cmd->newowner, false);

				if (IS_SYSSSO(newowner) || IS_SYSSAO(newowner))
				{
					ereport(ERROR,
							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							errmsg("permission denied"),
							errdetail("syssso_role/syssao_role member cannot"
										" be owner of relation")));	
				}

				if (se_feature.restricted_dba && IS_SYSDBA(GetUserId()))
				{
					if (!IS_SYSDBA(newowner))
					{
						ereport(ERROR,
								(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
									errmsg("permission denied"),
									errdetail("only sysdba_role member is allowed to own a "
												"relation of another sysdba_role member")));
					}
				}

			}
		}

	}
	return true;
}


bool
alter_relation_rename_check(RenameStmt *stmt)
{
	Oid			relid;
	HeapTuple	tup;
	Relation	rel;
	Form_pg_class classform;
	Oid			relowner;

	rel = table_open(RelationRelationId, RowExclusiveLock);

	relid = RangeVarGetRelidExtended(stmt->relation, NoLock,
									 true,
									 NULL,
									 NULL);

	tup = SearchSysCache1(RELOID, ObjectIdGetDatum(relid));

	/*
	 * we only care the relation existed case,
	 * leave non-exist case to original error check
	 */
	if (HeapTupleIsValid(tup))
	{
		classform = (Form_pg_class) GETSTRUCT(tup);
		relowner = classform->relowner;

		if(IS_SYSSSO(relowner) || IS_SYSSAO(relowner))
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					errmsg("permission denied"),
					errdetail("cannot alter relation of syssso_role/syssao_role "
								"member")));

		if (se_feature.restricted_dba && IS_SYSDBA(GetUserId()))
		{
			if (relowner != GetUserId())
			{
				ereport(ERROR,
						(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							errmsg("permission denied"),
							errdetail("sysdba_role member can only alter relation of "
									"itself")));
			}
		}
		
		ReleaseSysCache(tup);
	}	

	table_close(rel, RowExclusiveLock);

	return true;
}

bool
alter_relation_schema_check(AlterObjectSchemaStmt *stmt, Oid *oldschema)
{
	Oid			relid;
	HeapTuple	tup;
	Form_pg_class classform;
	Oid			relowner;
	Oid			newnspoid;
	char	   *relname;
	RangeVar   *newrv;



	relid = RangeVarGetRelidExtended(stmt->relation, NoLock,
									 RVR_MISSING_OK,
									 NULL,
									 NULL);

	tup = SearchSysCache1(RELOID, ObjectIdGetDatum(relid));

	/*
	 * we only care the relation existed case,
	 * leave non-exist case to original error check
	 */
	if (HeapTupleIsValid(tup))
	{
		classform = (Form_pg_class) GETSTRUCT(tup);
		relowner = classform->relowner;
		relname = NameStr(classform->relname);


		if(IS_SYSSSO(relowner) || IS_SYSSAO(relowner))
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					errmsg("permission denied"),
					errdetail("cannot alter relation of syssso_role/syssao_role "
								"member")));
		}

		if (se_feature.restricted_dba && IS_SYSDBA(GetUserId()))
		{
			if (relowner != GetUserId())
			{
				ereport(ERROR,
						(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							errmsg("permission denied"),
							errdetail("sysdba_role member can only alter relation of "
									"itself")));
			}
		}

		/* Get and lock schema OID and check its permissions. */
		newrv = makeRangeVar(stmt->newschema,relname, -1);
		newnspoid = RangeVarGetAndCheckCreationNamespace(newrv, NoLock, NULL);
		/*check whewher creater able to use the schema*/
		use_schema_check(newnspoid, GetUserId());

		ReleaseSysCache(tup);
	}

	return true;

}

bool drop_relation_check(DropStmt *stmt)
{
	ListCell   *cell;

	foreach(cell, stmt->objects)
	{
		RangeVar   *rel = makeRangeVarFromNameList((List *) lfirst(cell));
		Oid			relOid;
		HeapTuple	tup;
		Form_pg_class classform;
		Oid			relowner;

		/*
		 * These next few steps are a great deal like relation_openrv, but we
		 * don't bother building a relcache entry since we don't need it.
		 *
		 * Check for shared-cache-inval messages before trying to access the
		 * relation.  This is needed to cover the case where the name
		 * identifies a rel that has been dropped and recreated since the
		 * start of our transaction: if we don't flush the old syscache entry,
		 * then we'll latch onto that entry and suffer an error later.
		 */
		AcceptInvalidationMessages();


		relOid = RangeVarGetRelidExtended(rel, NoLock, RVR_MISSING_OK,
										  NULL,NULL);

		tup = SearchSysCache1(RELOID, ObjectIdGetDatum(relOid));
		/*
		 * we only care the relation existed case,
		 * leave non-exist case to original error check
		 */
		if (HeapTupleIsValid(tup))
		{
			classform = (Form_pg_class) GETSTRUCT(tup);
			relowner = classform->relowner;

			if(IS_SYSSSO(relowner) || IS_SYSSAO(relowner))
			{
				ereport(ERROR,
						(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
						errmsg("permission denied"),
						errdetail("cannot drop relation of syssso_role/syssao_role "
									"member")));
			}

			if (se_feature.restricted_dba && IS_SYSDBA(GetUserId()))
			{
				if (relowner != GetUserId())
				{
					ereport(ERROR,
							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
								errmsg("permission denied"),
								errdetail("sysdba_role member can only drop relation of "
										"itself")));
				}
			}

			ReleaseSysCache(tup);
		}

		
	}

	return true;
}

bool 
alter_sequence_check(AlterSeqStmt *stmt)
{
	Oid			relid;

	relid = RangeVarGetRelid(stmt->sequence, NoLock, true);
	/*
	 * we only care the relation existed case,
	 * leave non-exist case to original error check
	 */
	if (OidIsValid(relid))
	{

		HeapTuple	tuple;
		Oid			ownerId;

		tuple = SearchSysCache1(RELOID, ObjectIdGetDatum(relid));

		ownerId = ((Form_pg_class) GETSTRUCT(tuple))->relowner;

		if (IS_SYSSSO(ownerId) || IS_SYSSAO(ownerId))
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					errmsg("permission denied"),
					errdetail("cannot alter relation of syssso_role/syssao_role"
								" member")));			
		}

		if (se_feature.restricted_dba && IS_SYSDBA(GetUserId()))
		{
			if (ownerId != GetUserId())
			{
				ereport(ERROR,
						(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							errmsg("permission denied"),
							errdetail("sysdba_role member can only alter relation of "
									"itself")));
			}
		}

		ReleaseSysCache(tuple);
	}

	return true;
}

