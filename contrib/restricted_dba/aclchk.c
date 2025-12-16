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

static AclMode
pg_attribute_aclmask_ext_nosuper(Oid table_oid, AttrNumber attnum, Oid roleid,
						 AclMode mask, AclMaskHow how, bool *is_missing);

static AclMode
pg_attribute_aclmask_nosuper(Oid table_oid, AttrNumber attnum, Oid roleid,
					 AclMode mask, AclMaskHow how);

static AclResult
pg_attribute_aclcheck_ext_nosuper(Oid table_oid, AttrNumber attnum,
						  Oid roleid, AclMode mode, bool *is_missing);


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
	
	if (istmt->all_privs && istmt->privileges == ACL_NO_RIGHTS)
		istmt->privileges = ACL_ALL_RIGHTS_SCHEMA;

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
			if (se_feature.restricted_dba && (ownerId != GetUserId()))
				ereport(ERROR,
						(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
						errmsg("permission denied"),
						errdetail("sysdba_role member can only grant privilege of itself")));

			foreach(cell_grantee, istmt->grantees)
			{
				Oid grantee = lfirst_oid(cell_grantee);

				if (IS_SYSSSO(grantee) || IS_SYSSAO(grantee))
				{
					if(istmt->privileges & ACL_CREATE)
						ereport(ERROR,
								(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
								 errmsg("permission denied"),
								 errdetail("cannot grant create on schema of sysdba_role member to "
											"syssso_role/syssao_role member")));
				}
				else if(se_feature.restricted_dba && !IS_SYSDBA(grantee))
				{
					ereport(ERROR,
							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							 errmsg("permission denied"),
							 errdetail("cannot grant privilege of sysdba_role member to "
									   "normal user")));
				}
			}
		}
		else
		{
			foreach(cell_grantee, istmt->grantees)
			{
				Oid grantee = lfirst_oid(cell_grantee);
				
				if ((IS_SYSSSO(grantee) || IS_SYSSAO(grantee)) &&
					(istmt->privileges & ACL_CREATE))
				{
					ereport(ERROR,
							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							 errmsg("permission denied"),
							 errdetail("cannot grant create on normal user schema to "
										"syssso_role/syssao_role member")));
				}

				if (se_feature.restricted_dba &&
					IS_SYSDBA(grantee) &&
					IS_SYSDBA(GetUserId()))/*bug#z6269*/
				{
					ereport(ERROR,
							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							errmsg("permission denied"),
							errdetail("cannot grant privilege of normal user to "
									  "sysdba_role member")));
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
			if (se_feature.restricted_dba && (ownerId != GetUserId()))
				ereport(ERROR,
						(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
						errmsg("permission denied"),
						errdetail("sysdba_role member can only grant privilege of itself")));

			foreach(cell_grantee, istmt->grantees)
			{
				Oid grantee = lfirst_oid(cell_grantee);
				
				if (IS_SYSSSO(grantee) || IS_SYSSAO(grantee))
				{
					ereport(ERROR,
							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							errmsg("permission denied"),
							errdetail("cannot grant privilege of sysdba_role member to "
										"syssso_role/syssao_role member")));
				}	
				else if(se_feature.restricted_dba && !IS_SYSDBA(grantee))
				{
					ereport(ERROR,
							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							errmsg("permission denied"),
							errdetail("cannot grant privilege of sysdba_role member to "
									  "normal user")));
				}	
			}
		}
		else
		{
			if (se_feature.restricted_dba && IS_SYSDBA(GetUserId()))
				ereport(ERROR,
						(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
						errmsg("permission denied"),
						errdetail("sysdba_role member can only grant privilege of itself")));
			
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
			if (se_feature.restricted_dba && (ownerid != GetUserId()))
				ereport(ERROR,
						(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
						errmsg("permission denied"),
						errdetail("sysdba_role member can only grant privilege of itself")));

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
				else if(se_feature.restricted_dba && !IS_SYSDBA(grantee))
				{
					ereport(ERROR,
							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							errmsg("permission denied"),
							errdetail("cannot grant privilege of sysdba_role member to "
									  "normal user")));
				}	
			}
		}
		else
		{
			foreach(cell_grantee, istmt->grantees)
			{
				Oid grantee = lfirst_oid(cell_grantee);
				
				if (se_feature.restricted_dba && IS_SYSDBA(GetUserId()))
					ereport(ERROR,
							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							errmsg("permission denied"),
							errdetail("sysdba_role member can only grant privilege of itself")));

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
				if (ORA_MODE == compatible_mode)
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

static AclMode
string_to_privilege(const char *privname)
{
	if (strcmp(privname, "insert") == 0)
		return ACL_INSERT;
	if (strcmp(privname, "select") == 0)
		return ACL_SELECT;
	if (strcmp(privname, "update") == 0)
		return ACL_UPDATE;
	if (strcmp(privname, "delete") == 0)
		return ACL_DELETE;
	if (strcmp(privname, "truncate") == 0)
		return ACL_TRUNCATE;
	if (strcmp(privname, "references") == 0)
		return ACL_REFERENCES;
	if (strcmp(privname, "trigger") == 0)
		return ACL_TRIGGER;
	if (strcmp(privname, "execute") == 0)
		return ACL_EXECUTE;
	if (strcmp(privname, "usage") == 0)
		return ACL_USAGE;
	if (strcmp(privname, "create") == 0)
		return ACL_CREATE;
	if (strcmp(privname, "temporary") == 0)
		return ACL_CREATE_TEMP;
	if (strcmp(privname, "temp") == 0)
		return ACL_CREATE_TEMP;
	if (strcmp(privname, "connect") == 0)
		return ACL_CONNECT;
	if (strcmp(privname, "rule") == 0)
		return 0;				/* ignore old RULE privileges */
	ereport(ERROR,
			(errcode(ERRCODE_SYNTAX_ERROR),
			 errmsg("unrecognized privilege type \"%s\"", privname)));
	return 0;					/* appease compiler */
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
		/*Begin -- Bug#z4750*/
		foreach(cell, stmt->privileges)
		{
			AccessPriv *privnode = (AccessPriv *) lfirst(cell);
			AclMode		priv;

			/*
			 * If it's a column-level specification, we just set it aside in
			 * col_privs for the moment; but insist it's for a relation.
			 */
			if (privnode->cols)
			{
				if (stmt->objtype != OBJECT_TABLE)
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_GRANT_OPERATION),
							 errmsg("column privileges are only valid for relations")));
				istmt.col_privs = lappend(istmt.col_privs, privnode);
				continue;
			}

			if (privnode->priv_name == NULL)	/* parser mistake? */
				elog(ERROR, "AccessPriv node must specify privilege or columns");
			priv = string_to_privilege(privnode->priv_name);

			istmt.privileges |= priv;
		}
		/*End -- Bug#z4750*/
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

/*
 * pg_attribute_aclmask_nosuper
 *
 * same as pg_attribute_aclmask() but not influenced by superuser.
 */
static AclMode
pg_attribute_aclmask_nosuper(Oid table_oid, AttrNumber attnum, Oid roleid,
					 AclMode mask, AclMaskHow how)
{
	return pg_attribute_aclmask_ext_nosuper(table_oid, attnum, roleid,
									mask, how, NULL);
}

/*
 * Exported routine for examining a user's privileges for a column
 *
 * Does the bulk of the work for pg_attribute_aclmask(), and allows other
 * callers to avoid the missing attribute ERROR when is_missing is non-NULL.
 */
static AclMode
pg_attribute_aclmask_ext_nosuper(Oid table_oid, AttrNumber attnum, Oid roleid,
						 AclMode mask, AclMaskHow how, bool *is_missing)
{
	AclMode		result;
	HeapTuple	classTuple;
	HeapTuple	attTuple;
	Form_pg_class classForm;
	Form_pg_attribute attributeForm;
	Datum		aclDatum;
	bool		isNull;
	Acl		   *acl;
	Oid			ownerId;

	/*
	 * First, get the column's ACL from its pg_attribute entry
	 */
	attTuple = SearchSysCache2(ATTNUM,
							   ObjectIdGetDatum(table_oid),
							   Int16GetDatum(attnum));
	if (!HeapTupleIsValid(attTuple))
	{
		if (is_missing != NULL)
		{
			/* return "no privileges" instead of throwing an error */
			*is_missing = true;
			return 0;
		}
		else
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_COLUMN),
					 errmsg("attribute %d of relation with OID %u does not exist",
							attnum, table_oid)));
	}

	attributeForm = (Form_pg_attribute) GETSTRUCT(attTuple);

	/* Check dropped columns, too */
	if (attributeForm->attisdropped)
	{
		if (is_missing != NULL)
		{
			/* return "no privileges" instead of throwing an error */
			*is_missing = true;
			ReleaseSysCache(attTuple);
			return 0;
		}
		else
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_COLUMN),
					 errmsg("attribute %d of relation with OID %u does not exist",
							attnum, table_oid)));
	}

	aclDatum = SysCacheGetAttr(ATTNUM, attTuple, Anum_pg_attribute_attacl,
							   &isNull);

	/*
	 * Here we hard-wire knowledge that the default ACL for a column grants no
	 * privileges, so that we can fall out quickly in the very common case
	 * where attacl is null.
	 */
	if (isNull)
	{
		ReleaseSysCache(attTuple);
		return 0;
	}

	/*
	 * Must get the relation's ownerId from pg_class.  Since we already found
	 * a pg_attribute entry, the only likely reason for this to fail is that a
	 * concurrent DROP of the relation committed since then (which could only
	 * happen if we don't have lock on the relation).  We prefer to report "no
	 * privileges" rather than failing in such a case, so as to avoid unwanted
	 * failures in has_column_privilege() tests.
	 */
	classTuple = SearchSysCache1(RELOID, ObjectIdGetDatum(table_oid));
	if (!HeapTupleIsValid(classTuple))
	{
		ReleaseSysCache(attTuple);
		return 0;
	}
	classForm = (Form_pg_class) GETSTRUCT(classTuple);

	ownerId = classForm->relowner;

	ReleaseSysCache(classTuple);

	/* detoast column's ACL if necessary */
	acl = DatumGetAclP(aclDatum);

	result = aclmask_nosuper(acl, roleid, ownerId, mask, how);

	/* if we have a detoasted copy, free it */
	if (acl && (Pointer) acl != DatumGetPointer(aclDatum))
		pfree(acl);

	ReleaseSysCache(attTuple);

	return result;
}

/*
 * pg_attribute_aclcheck_nosuper
 *
 * same as pg_attribute_aclcheck() but not influenced by superuser.
 */
AclResult
pg_attribute_aclcheck_nosuper(Oid table_oid, AttrNumber attnum,
					  Oid roleid, AclMode mode)
{
	return pg_attribute_aclcheck_ext_nosuper(table_oid, attnum, roleid, mode, NULL);
}


/*
 * pg_attribute_aclcheck_ext_nosuper
 *
 * same as pg_attribute_aclcheck_ext() but not influenced by superuser.
 */
static AclResult
pg_attribute_aclcheck_ext_nosuper(Oid table_oid, AttrNumber attnum,
						  Oid roleid, AclMode mode, bool *is_missing)
{
	if (pg_attribute_aclmask_ext_nosuper(table_oid, attnum, roleid, mode,
								 ACLMASK_ANY, is_missing) != 0)
		return ACLCHECK_OK;
	else
		return ACLCHECK_NO_PRIV;
}

/*
 * pg_attribute_aclcheck_all_nosuper
 *
 * same as is_admin_of_role() but not influenced by superuser.
 */
AclResult
pg_attribute_aclcheck_all_nosuper(Oid table_oid, Oid roleid, AclMode mode,
						  AclMaskHow how)
{
	AclResult	result;
	HeapTuple	classTuple;
	Form_pg_class classForm;
	AttrNumber	nattrs;
	AttrNumber	curr_att;

	/*
	 * Must fetch pg_class row to check number of attributes.  As in
	 * pg_attribute_aclmask, we prefer to return "no privileges" instead of
	 * throwing an error if we get any unexpected lookup errors.
	 */
	classTuple = SearchSysCache1(RELOID, ObjectIdGetDatum(table_oid));
	if (!HeapTupleIsValid(classTuple))
		return ACLCHECK_NO_PRIV;
	classForm = (Form_pg_class) GETSTRUCT(classTuple);

	nattrs = classForm->relnatts;

	ReleaseSysCache(classTuple);

	/*
	 * Initialize result in case there are no non-dropped columns.  We want to
	 * report failure in such cases for either value of 'how'.
	 */
	result = ACLCHECK_NO_PRIV;

	for (curr_att = 1; curr_att <= nattrs; curr_att++)
	{
		HeapTuple	attTuple;
		AclMode		attmask;

		attTuple = SearchSysCache2(ATTNUM,
								   ObjectIdGetDatum(table_oid),
								   Int16GetDatum(curr_att));
		if (!HeapTupleIsValid(attTuple))
			continue;

		/* ignore dropped columns */
		if (((Form_pg_attribute) GETSTRUCT(attTuple))->attisdropped)
		{
			ReleaseSysCache(attTuple);
			continue;
		}

		/*
		 * Here we hard-wire knowledge that the default ACL for a column
		 * grants no privileges, so that we can fall out quickly in the very
		 * common case where attacl is null.
		 */
		if (heap_attisnull(attTuple, Anum_pg_attribute_attacl, NULL))
			attmask = 0;
		else
			attmask = pg_attribute_aclmask_nosuper(table_oid, curr_att, roleid,
										   mode, ACLMASK_ANY);

		ReleaseSysCache(attTuple);

		if (attmask != 0)
		{
			result = ACLCHECK_OK;
			if (how == ACLMASK_ANY)
				break;			/* succeed on any success */
		}
		else
		{
			result = ACLCHECK_NO_PRIV;
			if (how == ACLMASK_ALL)
				break;			/* fail on any failure */
		}
	}

	return result;
}

/*
 * Exported routine for examining a user's privileges for a table
 */
AclMode
pg_class_aclmask_nosuper(Oid table_oid, Oid roleid,
				 AclMode mask, AclMaskHow how)
{
	return pg_class_aclmask_ext_nosuper(table_oid, roleid, mask, how, NULL);
}

/*
 * Exported routine for examining a user's privileges for a table
 *
 * Does the bulk of the work for pg_class_aclmask(), and allows other
 * callers to avoid the missing relation ERROR when is_missing is non-NULL.
 */
AclMode
pg_class_aclmask_ext_nosuper(Oid table_oid, Oid roleid, AclMode mask,
					 AclMaskHow how, bool *is_missing)
{
	AclMode		result;
	HeapTuple	tuple;
	Form_pg_class classForm;
	Datum		aclDatum;
	bool		isNull;
	Acl		   *acl;
	Oid			ownerId;

	/*
	 * Must get the relation's tuple from pg_class
	 */
	tuple = SearchSysCache1(RELOID, ObjectIdGetDatum(table_oid));
	if (!HeapTupleIsValid(tuple))
	{
		if (is_missing != NULL)
		{
			/* return "no privileges" instead of throwing an error */
			*is_missing = true;
			return 0;
		}
		else
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_TABLE),
					 errmsg("relation with OID %u does not exist",
							table_oid)));
	}

	classForm = (Form_pg_class) GETSTRUCT(tuple);

	/*
	 * Normal case: get the relation's ACL from pg_class
	 */
	ownerId = classForm->relowner;

	aclDatum = SysCacheGetAttr(RELOID, tuple, Anum_pg_class_relacl,
							   &isNull);
	if (isNull)
	{
		/* No ACL, so build default ACL */
		switch (classForm->relkind)
		{
			case RELKIND_SEQUENCE:
				acl = acldefault(OBJECT_SEQUENCE, ownerId);
				break;
			default:
				acl = acldefault(OBJECT_TABLE, ownerId);
				break;
		}
		aclDatum = (Datum) 0;
	}
	else
	{
		/* detoast rel's ACL if necessary */
		acl = DatumGetAclP(aclDatum);
	}

	result = aclmask_nosuper(acl, roleid, ownerId, mask, how);

	/* if we have a detoasted copy, free it */
	if (acl && (Pointer) acl != DatumGetPointer(aclDatum))
		pfree(acl);

	ReleaseSysCache(tuple);

	/*
	 * Check if ACL_SELECT is being checked and, if so, and not set already as
	 * part of the result, then check if the user is a member of the
	 * pg_read_all_data role, which allows read access to all relations.
	 */
	if (mask & ACL_SELECT && !(result & ACL_SELECT) &&
		has_privs_of_role_nosuper(roleid, ROLE_PG_READ_ALL_DATA))
		result |= ACL_SELECT;

	/*
	 * Check if ACL_INSERT, ACL_UPDATE, or ACL_DELETE is being checked and, if
	 * so, and not set already as part of the result, then check if the user
	 * is a member of the pg_write_all_data role, which allows
	 * INSERT/UPDATE/DELETE access to all relations (except system catalogs,
	 * which requires superuser, see above).
	 */
	if (mask & (ACL_INSERT | ACL_UPDATE | ACL_DELETE) &&
		!(result & (ACL_INSERT | ACL_UPDATE | ACL_DELETE)) &&
		has_privs_of_role_nosuper(roleid, ROLE_PG_WRITE_ALL_DATA))
		result |= (mask & (ACL_INSERT | ACL_UPDATE | ACL_DELETE));

	return result;
}


/*
 * Exported routine for examining a user's privileges for a namespace
 */
AclMode
pg_namespace_aclmask_nosuper(Oid nsp_oid, Oid roleid,
					 AclMode mask, AclMaskHow how)
{
	AclMode		result;
	HeapTuple	tuple;
	Datum		aclDatum;
	bool		isNull;
	Acl		   *acl;
	Oid			ownerId;

	/*
	 * If we have been assigned this namespace as a temp namespace, check to
	 * make sure we have CREATE TEMP permission on the database, and if so act
	 * as though we have all standard (but not GRANT OPTION) permissions on
	 * the namespace.  If we don't have CREATE TEMP, act as though we have
	 * only USAGE (and not CREATE) rights.
	 *
	 * This may seem redundant given the check in InitTempTableNamespace, but
	 * it really isn't since current user ID may have changed since then. The
	 * upshot of this behavior is that a SECURITY DEFINER function can create
	 * temp tables that can then be accessed (if permission is granted) by
	 * code in the same session that doesn't have permissions to create temp
	 * tables.
	 *
	 * XXX Would it be safe to ereport a special error message as
	 * InitTempTableNamespace does?  Returning zero here means we'll get a
	 * generic "permission denied for schema pg_temp_N" message, which is not
	 * remarkably user-friendly.
	 */
	if (isTempNamespace(nsp_oid))
	{
		if (pg_database_aclcheck(MyDatabaseId, roleid,
								 ACL_CREATE_TEMP) == ACLCHECK_OK)
			return mask & ACL_ALL_RIGHTS_SCHEMA;
		else
			return mask & ACL_USAGE;
	}

	/*
	 * Get the schema's ACL from pg_namespace
	 */
	tuple = SearchSysCache1(NAMESPACEOID, ObjectIdGetDatum(nsp_oid));
	if (!HeapTupleIsValid(tuple))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_SCHEMA),
				 errmsg("schema with OID %u does not exist", nsp_oid)));

	ownerId = ((Form_pg_namespace) GETSTRUCT(tuple))->nspowner;

	aclDatum = SysCacheGetAttr(NAMESPACEOID, tuple, Anum_pg_namespace_nspacl,
							   &isNull);
	if (isNull)
	{
		/* No ACL, so build default ACL */
		acl = acldefault(OBJECT_SCHEMA, ownerId);
		aclDatum = (Datum) 0;
	}
	else
	{
		/* detoast ACL if necessary */
		acl = DatumGetAclP(aclDatum);
	}

	result = aclmask_nosuper(acl, roleid, ownerId, mask, how);

	/* if we have a detoasted copy, free it */
	if (acl && (Pointer) acl != DatumGetPointer(aclDatum))
		pfree(acl);

	ReleaseSysCache(tuple);

	/*
	 * Check if ACL_USAGE is being checked and, if so, and not set already as
	 * part of the result, then check if the user is a member of the
	 * pg_read_all_data or pg_write_all_data roles, which allow usage access
	 * to all schemas.
	 */
	if (mask & ACL_USAGE && !(result & ACL_USAGE) &&
		(has_privs_of_role_nosuper(roleid, ROLE_PG_READ_ALL_DATA) ||
		 has_privs_of_role_nosuper(roleid, ROLE_PG_WRITE_ALL_DATA)))
		result |= ACL_USAGE;
	return result;
}

/*
 * Exported routine for examining a user's privileges for a function.
 * Mostly same as pg_proc_aclmask but not influenced by superuser.
 * 
 * Note: the origin version give EXECUTE priv by default, we don't allow it.
 */
AclMode
pg_proc_aclmask_nosuper(Oid proc_oid, Oid roleid,
						AclMode mask, AclMaskHow how)
{
	AclMode		result;
	HeapTuple	tuple;
	Datum		aclDatum;
	bool		isNull;
	Acl		   *acl;
	Oid			ownerId;

	/*
	 * Get the function's ACL from pg_proc
	 */
	tuple = SearchSysCache1(PROCOID, ObjectIdGetDatum(proc_oid));
	if (!HeapTupleIsValid(tuple))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_FUNCTION),
				 errmsg("function with OID %u does not exist", proc_oid)));

	ownerId = ((Form_pg_proc) GETSTRUCT(tuple))->proowner;

	aclDatum = SysCacheGetAttr(PROCOID, tuple, Anum_pg_proc_proacl,
							   &isNull);
	if (isNull)
	{
		/* No ACL, so build default ACL */
		acl = func_acldefault(OBJECT_FUNCTION, ownerId);
		aclDatum = (Datum) 0;
	}
	else
	{
		/* detoast ACL if necessary */
		acl = DatumGetAclP(aclDatum);
	}

	result = aclmask_nosuper(acl, roleid, ownerId, mask, how);

	/* if we have a detoasted copy, free it */
	if (acl && (Pointer) acl != DatumGetPointer(aclDatum))
		pfree(acl);

	ReleaseSysCache(tuple);

	return result;
}

AclResult
pg_proc_aclcheck_nosuper(Oid proc_oid, Oid roleid, AclMode mode)
{
	if (pg_proc_aclmask_nosuper(proc_oid, roleid, mode, ACLMASK_ANY) != 0)
		return ACLCHECK_OK;
	else
		return ACLCHECK_NO_PRIV;
}


/*
 * This function will be called in those sequence manipulate
 * functions, like nextval() and others, when se_feature is on.
 */
bool 
se_use_sequence_check (Oid seq_oid, Oid roleid, AclMode mode)
{
	AclMode		privs;
	HeapTuple	tuple;
	Form_pg_class classForm;
	Oid			ownerId;

	Assert(se_feature.mode);

	tuple = SearchSysCache1(RELOID, ObjectIdGetDatum(seq_oid));
	classForm = (Form_pg_class) GETSTRUCT(tuple);
	ownerId = classForm->relowner;

	ReleaseSysCache(tuple);
	
	
	if (IS_SYSSSO(ownerId) || IS_SYSSAO(ownerId))
	{
		/* we always check for sso/sao user's sequence */
		privs = pg_class_aclmask_nosuper(seq_oid,roleid,mode, ACLMASK_ANY);
	}
	else if (se_feature.restricted_dba && IS_SYSDBA(roleid))
	{
		/* and check for sysdba when restricted_dba is on */
		privs = pg_class_aclmask_nosuper(seq_oid,roleid,mode, ACLMASK_ANY);
	}
	else
	{
		/* we don' case other case and leave them to originl check */
		privs = pg_class_aclmask(seq_oid,roleid,mode, ACLMASK_ANY);
	}

	if (privs != 0)
		return true;
	else
		return false;
}