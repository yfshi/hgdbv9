/* -------------------------------------------------------------------------
 *
 * File: aclchk.c
 *
 * Abstract: 
 * 		This file contains functions of aclchk. if any function in
 * 		src/backend/catalog/aclchk.c changed, you better also change
 *		at here.
 *
 * Authored by sunchengxi@highgo.com, 20240510.
 *
 * Copyright:
 * 		Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/restricted_dba/aclchk.c
 * -------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/genam.h"
#include "access/heapam.h"
#include "access/htup_details.h"
#include "access/sysattr.h"
#include "access/tableam.h"
#include "access/xact.h"
#include "catalog/binary_upgrade.h"
#include "catalog/catalog.h"
#include "catalog/dependency.h"
#include "catalog/indexing.h"
#include "catalog/objectaccess.h"
#include "catalog/pg_aggregate.h"
#include "catalog/pg_am.h"
#include "catalog/pg_authid.h"
#include "catalog/pg_cast.h"
#include "catalog/pg_collation.h"
#include "catalog/pg_conversion.h"
#include "catalog/pg_database.h"
#include "catalog/pg_default_acl.h"
#include "catalog/pg_event_trigger.h"
#include "catalog/pg_extension.h"
#include "catalog/pg_foreign_data_wrapper.h"
#include "catalog/pg_foreign_server.h"
#include "catalog/pg_init_privs.h"
#include "catalog/pg_language.h"
#include "catalog/pg_largeobject.h"
#include "catalog/pg_largeobject_metadata.h"
#include "catalog/pg_namespace.h"
#include "catalog/pg_opclass.h"
#include "catalog/pg_operator.h"
#include "catalog/pg_opfamily.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_statistic_ext.h"
#include "catalog/pg_subscription.h"
#include "catalog/pg_tablespace.h"
#include "catalog/pg_transform.h"
#include "catalog/pg_ts_config.h"
#include "catalog/pg_ts_dict.h"
#include "catalog/pg_ts_parser.h"
#include "catalog/pg_ts_template.h"
#include "catalog/pg_type.h"
/* Begin - ReqID:SRS-SQL-PACKAGE */
#include "catalog/pg_package.h"
#include "catalog/pg_package_body.h"
#include "commands/packagecmds.h"
/* End - ReqID:SRS-SQL-PACKAGE */
#include "commands/dbcommands.h"
#include "commands/event_trigger.h"
#include "commands/extension.h"
#include "commands/proclang.h"
#include "commands/tablespace.h"
#include "foreign/foreign.h"
#include "miscadmin.h"
#include "nodes/makefuncs.h"
#include "parser/parse_func.h"
#include "parser/parse_type.h"
#include "utils/acl.h"
#include "utils/aclchk_internal.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"
#include "utils/syscache.h"
/* Begin - ReqID:SRS-SQL-PACKAGE */
#include "utils/packagecache.h"
/* End - ReqID:SRS-SQL-PACKAGE */
#include "utils/guc.h"
#include "utils/ora_compatible.h"

#include "restricted_dba.h"


static void
grant_tablespace_check(InternalGrant *istmt)
{
	ListCell   *cell;

	/*make sure only member of sysdba_role can do this*/
	if(!IS_SYSDBA(GetUserId()))
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied"),
				 errdetail("only member of sysdba_role can grant/revoke tablespace.")));
	
	foreach(cell, istmt->grantees)
	{
		Oid grantee = lfirst_oid(cell);
		
		/*ssosso_role and syssao_role member cannot be grantee of grant tablespace*/
		if(IS_SYSSSO(grantee) || IS_SYSSAO(grantee))
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					 errmsg("permission denied"),
					 errdetail("cannot grant privilege on tablespace to syssso_role/"
					 			"syssao_role member.")));
		}	

		/*grant option cannot be grant to non sysdba_role member*/	
		if(istmt->grant_option && !IS_SYSDBA(grantee))
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					errmsg("permission denied"),
					errdetail("cannot grant privilege on tablespace with grant option "
							  "to non-sysdba_role member")));
		}	
	}

	/*if the grantor don't have privilege, it will blocked by following function*/
}

static void
grant_database_check(InternalGrant *istmt)
{
	ListCell   *cell;

	/*make sure only member of sysdba_role can do this*/
	if (!IS_SYSDBA(GetUserId()))
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied"),
				 errdetail("only member of sysdba_role can grant/revoke database.")));

	if (istmt->all_privs && istmt->privileges == ACL_NO_RIGHTS)
		istmt->privileges = ACL_ALL_RIGHTS_DATABASE;

	foreach(cell, istmt->grantees)
	{
		Oid grantee = lfirst_oid(cell);
		
		/*we allow dba grant connect/temp on database to sso/sao */
		if ((istmt->privileges & ACL_CREATE) &&
			(IS_SYSSSO(grantee) || IS_SYSSAO(grantee)))
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					errmsg("permission denied"),
					errdetail("cannot grant privilege on database to syssso_role/"
					 			"syssao_role member.")));
		}
			
		/*grant option cannot be grant to non sysdba_role member*/	
		if (istmt->grant_option && !IS_SYSDBA(grantee))
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					errmsg("permission denied"),
					errdetail("cannot grant privilege on database with grant option"
							"to non-sysdba_role member")));
		}	
	}



	/*if the grantor don't have privilege, it will blocked by following function*/
}

static void
grant_schema_check(InternalGrant *istmt)
{
	Relation	relation;
	ListCell   *cell_obj;
	

	relation = table_open(NamespaceRelationId, RowExclusiveLock);

	foreach(cell_obj, istmt->objects)
	{
		Oid			nspid = lfirst_oid(cell_obj);
		Form_pg_namespace pg_namespace_tuple;
		Oid			ownerId;
		HeapTuple	tuple;
		ListCell   *cell_grantee;


		tuple = SearchSysCache1(NAMESPACEOID, ObjectIdGetDatum(nspid));
		if (!HeapTupleIsValid(tuple))
			elog(ERROR, "cache lookup failed for namespace %u", nspid);

		pg_namespace_tuple = (Form_pg_namespace) GETSTRUCT(tuple);

		/*
		 * Get owner ID and working copy of existing ACL. If there's no ACL,
		 * substitute the proper default.
		 */
		ownerId = pg_namespace_tuple->nspowner;

		if(IS_SYSSSO(ownerId) || IS_SYSSAO(ownerId))
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					errmsg("permission denied"),
					errdetail("cannot grant privilege of syssso_role/syssao_role"
								" member to anyone")));

		}
		else if (IS_SYSDBA(ownerId))
		{
			foreach(cell_grantee, istmt->grantees)
			{
				Oid grantee = lfirst_oid(cell_grantee);
				
				if (IS_SYSSSO(grantee) || IS_SYSSAO(grantee))
				{
					ereport(ERROR,
							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							errmsg("permission denied"),
							errdetail("cannot grant privilege of sysdba_role member to"
										" syssso_role/syssao_role member")));
				}	
			}
		}
		else
		{
			foreach(cell_grantee, istmt->grantees)
			{
				Oid grantee = lfirst_oid(cell_grantee);
				
				if (IS_SYSSSO(grantee) || IS_SYSSAO(grantee))
				{
					ereport(ERROR,
							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							errmsg("permission denied"),
							errdetail("cannot grant privilege of normal user to "
										"syssso_role/syssao_role member")));
				}	
			}
		}

		ReleaseSysCache(tuple);
	}

	table_close(relation, RowExclusiveLock);

	/*if the grantor don't have privilege, it will blocked by following function*/
}

static void
grant_relation_check(InternalGrant *istmt)
{
	Relation	relation;
	ListCell   *cell;

	relation = table_open(RelationRelationId, RowShareLock);

	foreach(cell, istmt->objects)
	{
		Oid			relOid = lfirst_oid(cell);
		Form_pg_class pg_class_tuple;
		Oid			ownerId;
		HeapTuple	tuple;
		ListCell   *cell_grantee;

		tuple = SearchSysCache1(RELOID, ObjectIdGetDatum(relOid));
		if (!HeapTupleIsValid(tuple))
			elog(ERROR, "cache lookup failed for relation %u", relOid);
		pg_class_tuple = (Form_pg_class) GETSTRUCT(tuple);
		ownerId = pg_class_tuple->relowner;
		
		if(IS_SYSSSO(ownerId) || IS_SYSSAO(ownerId))
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					errmsg("permission denied"),
					errdetail("cannot grant privilege of syssso_role/syssao_role"
								" member to anyone")));

		}
		else if (IS_SYSDBA(ownerId))
		{
			foreach(cell_grantee, istmt->grantees)
			{
				Oid grantee = lfirst_oid(cell_grantee);
				
				if (IS_SYSSSO(grantee) || IS_SYSSAO(grantee))
				{
					ereport(ERROR,
							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							errmsg("permission denied"),
							errdetail("cannot grant privilege of sysdba_role member to"
										" syssso_role/syssao_role member")));
				}	
			}
		}
		else
		{
			foreach(cell_grantee, istmt->grantees)
			{
				Oid grantee = lfirst_oid(cell_grantee);
				
				if (IS_SYSSSO(grantee) || IS_SYSSAO(grantee))
				{
					ereport(ERROR,
							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							errmsg("permission denied"),
							errdetail("cannot grant privilege of normal user to "
										"syssso_role/syssao_role member")));
				}	
			}
		}

		ReleaseSysCache(tuple);
	}


	table_close(relation, RowShareLock);
}

static void
grant_proc_check(InternalGrant *istmt)
{
	Relation	relation;
	ListCell   *cell;

	relation = table_open(ProcedureRelationId, RowShareLock);

	foreach(cell, istmt->objects)
	{
		Oid			procid = lfirst_oid(cell);
		Form_pg_proc pg_proc_tuple;
		Oid			ownerid;
		HeapTuple	tuple;
		ListCell   *cell_grantee;

		tuple = SearchSysCache1(PROCOID, ObjectIdGetDatum(procid));
		if (!HeapTupleIsValid(tuple))
			elog(ERROR, "cache lookup failed for function %u", procid);
		pg_proc_tuple = (Form_pg_proc) GETSTRUCT(tuple);
		ownerid = pg_proc_tuple->proowner;
		
		if(IS_SYSSSO(ownerid) || IS_SYSSAO(ownerid))
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					errmsg("permission denied"),
					errdetail("cannot grant privilege of syssso_role/syssao_role"
								" member to anyone")));

		}
		else if (IS_SYSDBA(ownerid))
		{
			foreach(cell_grantee, istmt->grantees)
			{
				Oid grantee = lfirst_oid(cell_grantee);
				
				if (IS_SYSSSO(grantee) || IS_SYSSAO(grantee))
				{
					ereport(ERROR,
							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							errmsg("permission denied"),
							errdetail("cannot grant privilege of sysdba_role member to"
										" syssso_role/syssao_role member")));
				}	
			}
		}
		else
		{
			foreach(cell_grantee, istmt->grantees)
			{
				Oid grantee = lfirst_oid(cell_grantee);
				
				if (IS_SYSSSO(grantee) || IS_SYSSAO(grantee))
				{
					ereport(ERROR,
							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							errmsg("permission denied"),
							errdetail("cannot grant privilege of normal user to "
										"syssso_role/syssao_role member")));
				}	
			}
		}

		ReleaseSysCache(tuple);
	}

	table_close(relation, RowShareLock);
}


/*
 * getRelationsInNamespace
 *
 * Return Oid list of relations in given namespace filtered by relation kind
 */
static List *
getRelationsInNamespace(Oid namespaceId, char relkind)
{
	List	   *relations = NIL;
	ScanKeyData key[2];
	Relation	rel;
	TableScanDesc scan;
	HeapTuple	tuple;

	ScanKeyInit(&key[0],
				Anum_pg_class_relnamespace,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(namespaceId));
	ScanKeyInit(&key[1],
				Anum_pg_class_relkind,
				BTEqualStrategyNumber, F_CHAREQ,
				CharGetDatum(relkind));

	rel = table_open(RelationRelationId, AccessShareLock);
	scan = table_beginscan_catalog(rel, 2, key);

	while ((tuple = heap_getnext(scan, ForwardScanDirection)) != NULL)
	{
		Oid			oid = ((Form_pg_class) GETSTRUCT(tuple))->oid;

		relations = lappend_oid(relations, oid);
	}

	table_endscan(scan);
	table_close(rel, AccessShareLock);

	return relations;
}

/*
 * objectNamesToOids
 *
 * Turn a list of object names of a given type into an Oid list.
 *
 * XXX: This function doesn't take any sort of locks on the objects whose
 * names it looks up.  In the face of concurrent DDL, we might easily latch
 * onto an old version of an object, causing the GRANT or REVOKE statement
 * to fail.
 */
static List *
objectNamesToOids(ObjectType objtype, List *objnames)
{
	List	   *objects = NIL;
	ListCell   *cell;

	Assert(objnames != NIL);

	switch (objtype)
	{
		case OBJECT_TABLE:
		case OBJECT_SEQUENCE:
			foreach(cell, objnames)
			{
				RangeVar   *relvar = (RangeVar *) lfirst(cell);
				Oid			relOid;

				relOid = RangeVarGetRelid(relvar, NoLock, false);
				objects = lappend_oid(objects, relOid);
			}
			break;
		case OBJECT_DATABASE:
			foreach(cell, objnames)
			{
				char	   *dbname = strVal(lfirst(cell));
				Oid			dbid;

				dbid = get_database_oid(dbname, false);
				objects = lappend_oid(objects, dbid);
			}
			break;
		case OBJECT_DOMAIN:
		case OBJECT_TYPE:
			foreach(cell, objnames)
			{
				List	   *typname = (List *) lfirst(cell);
				Oid			oid;

				oid = typenameTypeId(NULL, makeTypeNameFromNameList(typname));
				objects = lappend_oid(objects, oid);
			}
			break;
		case OBJECT_FUNCTION:
			foreach(cell, objnames)
			{
				ObjectWithArgs *func = (ObjectWithArgs *) lfirst(cell);
				Oid			funcid;

				funcid = LookupFuncWithArgs(OBJECT_FUNCTION, func, false);
				objects = lappend_oid(objects, funcid);
			}
			break;
		/* Begin - ReqID:SRS-SQL-PACKAGE */
		case OBJECT_PACKAGE:
			foreach(cell, objnames)
			{
				List	   *pkgname = (List *) lfirst(cell);
				Oid			relOid;

				relOid = LookupPackageByNames(castNode(List, pkgname), false);
				objects = lappend_oid(objects, relOid);
			}
			break;
		case OBJECT_PACKAGE_BODY:
			foreach(cell, objnames)
			{
				List	   *pkgname = (List *) lfirst(cell);
				Oid			relOid;

				relOid = LookupPackageBodyByNames(castNode(List, pkgname), false);
				objects = lappend_oid(objects, relOid);
			}
			break;
		/* End - ReqID:SRS-SQL-PACKAGE */
		case OBJECT_LANGUAGE:
			foreach(cell, objnames)
			{
				char	   *langname = strVal(lfirst(cell));
				Oid			oid;

				oid = get_language_oid(langname, false);
				objects = lappend_oid(objects, oid);
			}
			break;
		case OBJECT_LARGEOBJECT:
			foreach(cell, objnames)
			{
				Oid			lobjOid = oidparse(lfirst(cell));

				if (!LargeObjectExists(lobjOid))
					ereport(ERROR,
							(errcode(ERRCODE_UNDEFINED_OBJECT),
							 errmsg("large object %u does not exist",
									lobjOid)));

				objects = lappend_oid(objects, lobjOid);
			}
			break;
		case OBJECT_SCHEMA:
			foreach(cell, objnames)
			{
				char	   *nspname = strVal(lfirst(cell));
				Oid			oid;

				oid = get_namespace_oid(nspname, false);
				objects = lappend_oid(objects, oid);
			}
			break;
		case OBJECT_PROCEDURE:
			foreach(cell, objnames)
			{
				ObjectWithArgs *func = (ObjectWithArgs *) lfirst(cell);
				Oid			procid;

				procid = LookupFuncWithArgs(OBJECT_PROCEDURE, func, false);
				objects = lappend_oid(objects, procid);
			}
			break;
		case OBJECT_ROUTINE:
			foreach(cell, objnames)
			{
				ObjectWithArgs *func = (ObjectWithArgs *) lfirst(cell);
				Oid			routid;

				routid = LookupFuncWithArgs(OBJECT_ROUTINE, func, false);
				objects = lappend_oid(objects, routid);
			}
			break;
		case OBJECT_TABLESPACE:
			foreach(cell, objnames)
			{
				char	   *spcname = strVal(lfirst(cell));
				Oid			spcoid;

				spcoid = get_tablespace_oid(spcname, false);
				objects = lappend_oid(objects, spcoid);
			}
			break;
		case OBJECT_FDW:
			foreach(cell, objnames)
			{
				Oid			fdwid = InvalidOid;
				char	   *fdwname = strVal(lfirst(cell));

				/* Begin - BUG#Z1331 */
				if (ORA_PARSER == compatible_db)
				{
					fdwid = get_foreign_data_wrapper_oid(fdwname, true);
					if (OidIsValid(fdwid))
						objects = lappend_oid(objects, fdwid);
				}
				else
				{
					fdwid = get_foreign_data_wrapper_oid(fdwname, false);
					objects = lappend_oid(objects, fdwid);
				}
				/* End - BUG#Z1331 */
			}
			break;
		case OBJECT_FOREIGN_SERVER:
			foreach(cell, objnames)
			{
				char	   *srvname = strVal(lfirst(cell));
				Oid			srvid = get_foreign_server_oid(srvname, false);

				objects = lappend_oid(objects, srvid);
			}
			break;
		default:
			elog(ERROR, "unrecognized GrantStmt.objtype: %d",
				 (int) objtype);
	}

	return objects;
}
/*
 * objectsInSchemaToOids
 *
 * Find all objects of a given type in specified schemas, and make a list
 * of their Oids.  We check USAGE privilege on the schemas, but there is
 * no privilege checking on the individual objects here.
 */
static List *
objectsInSchemaToOids(ObjectType objtype, List *nspnames)
{
	List	   *objects = NIL;
	ListCell   *cell;

	foreach(cell, nspnames)
	{
		char	   *nspname = strVal(lfirst(cell));
		Oid			namespaceId;
		List	   *objs;

		namespaceId = LookupExplicitNamespace(nspname, false);

		switch (objtype)
		{
			case OBJECT_TABLE:
				objs = getRelationsInNamespace(namespaceId, RELKIND_RELATION);
				objects = list_concat(objects, objs);
				objs = getRelationsInNamespace(namespaceId, RELKIND_VIEW);
				objects = list_concat(objects, objs);
				objs = getRelationsInNamespace(namespaceId, RELKIND_MATVIEW);
				objects = list_concat(objects, objs);
				objs = getRelationsInNamespace(namespaceId, RELKIND_FOREIGN_TABLE);
				objects = list_concat(objects, objs);
				objs = getRelationsInNamespace(namespaceId, RELKIND_PARTITIONED_TABLE);
				objects = list_concat(objects, objs);
				break;
			case OBJECT_SEQUENCE:
				objs = getRelationsInNamespace(namespaceId, RELKIND_SEQUENCE);
				objects = list_concat(objects, objs);
				break;
			case OBJECT_FUNCTION:
			case OBJECT_PROCEDURE:
			case OBJECT_ROUTINE:
				{
					ScanKeyData key[2];
					int			keycount;
					Relation	rel;
					TableScanDesc scan;
					HeapTuple	tuple;

					keycount = 0;
					ScanKeyInit(&key[keycount++],
								Anum_pg_proc_pronamespace,
								BTEqualStrategyNumber, F_OIDEQ,
								ObjectIdGetDatum(namespaceId));

					if (objtype == OBJECT_FUNCTION)
						/* includes aggregates and window functions */
						ScanKeyInit(&key[keycount++],
									Anum_pg_proc_prokind,
									BTEqualStrategyNumber, F_CHARNE,
									CharGetDatum(PROKIND_PROCEDURE));
					else if (objtype == OBJECT_PROCEDURE)
						ScanKeyInit(&key[keycount++],
									Anum_pg_proc_prokind,
									BTEqualStrategyNumber, F_CHAREQ,
									CharGetDatum(PROKIND_PROCEDURE));

					rel = table_open(ProcedureRelationId, AccessShareLock);
					scan = table_beginscan_catalog(rel, keycount, key);

					while ((tuple = heap_getnext(scan, ForwardScanDirection)) != NULL)
					{
						Oid			oid = ((Form_pg_proc) GETSTRUCT(tuple))->oid;

						objects = lappend_oid(objects, oid);
					}

					table_endscan(scan);
					table_close(rel, AccessShareLock);
				}
				break;
			/* Begin - ReqID:SRS-SQL-PACKAGE */
			case OBJECT_PACKAGE:
				{
					ScanKeyData key[1];
					Relation	rel;
					TableScanDesc scan;
					HeapTuple	tuple;

					ScanKeyInit(&key[0],
								Anum_pg_proc_pronamespace,
								BTEqualStrategyNumber, F_OIDEQ,
								ObjectIdGetDatum(namespaceId));

					rel = table_open(PackageRelationId, AccessShareLock);
					scan = table_beginscan_catalog(rel, 1, key);

					while ((tuple = heap_getnext(scan, ForwardScanDirection)) != NULL)
					{
						Oid			oid = ((Form_pg_package) GETSTRUCT(tuple))->oid;

						objects = lappend_oid(objects, oid);
					}

					table_endscan(scan);
					table_close(rel, AccessShareLock);
				}
				break;
			/* End - ReqID:SRS-SQL-PACKAGE */
			default:
				/* should not happen */
				elog(ERROR, "unrecognized GrantStmt.objtype: %d",
					 (int) objtype);
		}
	}

	return objects;
}




/*
 * Called to execute the utility commands GRANT and REVOKE
 */
bool
grant_object_check(GrantStmt *stmt)
{
	InternalGrant istmt;
	ListCell   *cell;
	// const char *errormsg;
	// AclMode		all_privileges;

	if (stmt->grantor)
	{
		Oid			grantor;

		grantor = get_rolespec_oid(stmt->grantor, false);

		/*
		 * Currently, this clause is only for SQL compatibility, not very
		 * interesting otherwise.
		 */
		if (grantor != GetUserId())
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("grantor must be current user")));
	}

	/*
	 * Turn the regular GrantStmt into the InternalGrant form.
	 */
	istmt.is_grant = stmt->is_grant;
	istmt.objtype = stmt->objtype;

	/* Collect the OIDs of the target objects */
	switch (stmt->targtype)
	{
		case ACL_TARGET_OBJECT:
			istmt.objects = objectNamesToOids(stmt->objtype, stmt->objects);
			break;
		case ACL_TARGET_ALL_IN_SCHEMA:
			istmt.objects = objectsInSchemaToOids(stmt->objtype, stmt->objects);
			break;
			/* ACL_TARGET_DEFAULTS should not be seen here */
		default:
			elog(ERROR, "unrecognized GrantStmt.targtype: %d",
				 (int) stmt->targtype);
	}

	/* all_privs to be filled below */
	/* privileges to be filled below */
	istmt.col_privs = NIL;		/* may get filled below */
	istmt.grantees = NIL;		/* filled below */
	istmt.grant_option = stmt->grant_option;
	istmt.behavior = stmt->behavior;

	/*
	 * Convert the RoleSpec list into an Oid list.  Note that at this point we
	 * insert an ACL_ID_PUBLIC into the list if appropriate, so downstream
	 * there shouldn't be any additional work needed to support this case.
	 */
	foreach(cell, stmt->grantees)
	{
		RoleSpec   *grantee = (RoleSpec *) lfirst(cell);
		Oid			grantee_uid;

		switch (grantee->roletype)
		{
			case ROLESPEC_PUBLIC:
				grantee_uid = ACL_ID_PUBLIC;
				break;
			default:
				grantee_uid = get_rolespec_oid(grantee, false);
				break;
		}
		istmt.grantees = lappend_oid(istmt.grantees, grantee_uid);
	}

	if (stmt->privileges == NIL)
	{
		istmt.all_privs = true;

		/*
		 * will be turned into ACL_ALL_RIGHTS_* by the internal routines
		 * depending on the object type
		 */
		istmt.privileges = ACL_NO_RIGHTS;
	}
	else
	{
		istmt.all_privs = false;
		istmt.privileges = ACL_NO_RIGHTS;
	}

	/*
	 * Convert stmt->privileges, a list of AccessPriv nodes, into an AclMode
	 * bitmask.  Note: objtype can't be OBJECT_COLUMN.
	 */
	switch (stmt->objtype)
	{
		case OBJECT_TABLE:
		case OBJECT_SEQUENCE:
			/*
			 * Because this might be a sequence, we test both relation and
			 * sequence bits, and later do a more limited test when we know
			 * the object type.
			 */
			// all_privileges = ACL_ALL_RIGHTS_RELATION | ACL_ALL_RIGHTS_SEQUENCE;
			grant_relation_check(&istmt);
			break;
		case OBJECT_DATABASE:
			// all_privileges = ACL_ALL_RIGHTS_DATABASE;
			// errormsg = gettext_noop("invalid privilege type %s for database");
			grant_database_check(&istmt);
			break;
		case OBJECT_DOMAIN:
			// all_privileges = ACL_ALL_RIGHTS_TYPE;
			// errormsg = gettext_noop("invalid privilege type %s for domain");
			break;
		case OBJECT_FUNCTION:
		case OBJECT_PROCEDURE:
		case OBJECT_ROUTINE:
			grant_proc_check(&istmt);
			break;
		/* Begin - ReqID:SRS-SQL-PACKAGE */
		case OBJECT_PACKAGE:
			// all_privileges = ACL_ALL_RIGHTS_PACKAGE;
			// errormsg = gettext_noop("invalid privilege type %s for package");
			break;
		/* End - ReqID:SRS-SQL-PACKAGE */
		case OBJECT_LANGUAGE:
			// all_privileges = ACL_ALL_RIGHTS_LANGUAGE;
			// errormsg = gettext_noop("invalid privilege type %s for language");
			break;
		case OBJECT_LARGEOBJECT:
			// all_privileges = ACL_ALL_RIGHTS_LARGEOBJECT;
			// errormsg = gettext_noop("invalid privilege type %s for large object");
			break;
		case OBJECT_SCHEMA:
			// all_privileges = ACL_ALL_RIGHTS_SCHEMA;
			// errormsg = gettext_noop("invalid privilege type %s for schema");
			grant_schema_check(&istmt);
			break;
		case OBJECT_TABLESPACE:
			// all_privileges = ACL_ALL_RIGHTS_TABLESPACE;
			// errormsg = gettext_noop("invalid privilege type %s for tablespace");
            grant_tablespace_check(&istmt);
			break;
		case OBJECT_TYPE:
			// all_privileges = ACL_ALL_RIGHTS_TYPE;
			// errormsg = gettext_noop("invalid privilege type %s for type");
			break;
		case OBJECT_FDW:
			// all_privileges = ACL_ALL_RIGHTS_FDW;
			// errormsg = gettext_noop("invalid privilege type %s for foreign-data wrapper");
			break;
		case OBJECT_FOREIGN_SERVER:
			// all_privileges = ACL_ALL_RIGHTS_FOREIGN_SERVER;
			// errormsg = gettext_noop("invalid privilege type %s for foreign server");
			break;
		default:
			elog(ERROR, "unrecognized GrantStmt.objtype: %d",
				 (int) stmt->objtype);
			/* keep compiler quiet */
			// all_privileges = ACL_NO_RIGHTS;
			// errormsg = NULL;
	}

	return true;
}


