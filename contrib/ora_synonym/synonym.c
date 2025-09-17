/*-------------------------------------------------------------------------
 *
 * synonym.c
 *	  support routines dealing with synonyms
 *
 * Abstract:
 * This file contains synonym CREATE/ALTER/DROP and synonym refrences.
 *
 *
 * Copyright:
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Authored by lanke@highgo.com 2025/4/24
 *
 * IDENTIFICATION
 *	  contrib/ora_synonym/synonym.c
 *
 *-------------------------------------------------------------------------
 */

#include "c.h"
#include "postgres.h"
#include "fmgr.h"
#include "catalog/namespace.h"
#include "catalog/catalog.h"
#include "catalog/pg_synonym.h"
#include "catalog/dependency.h"
#include "catalog/indexing.h"
#include "catalog/pg_namespace.h"
#include "catalog/objectaddress.h"
#include "catalog/objectaccess.h"
#include "catalog/pg_type.h"
#include "commands/event_trigger.h"
#include "commands/packagecmds.h"
#include "access/twophase.h"
#include "access/table.h"
#include "access/xact.h"
#include "tcop/utility.h"
#include "utils/rel.h"
#include "parser/parse_utilcmd.h"
#include "parser/parse_type.h"
#include "utils/syscache.h"
#include "utils/lsyscache.h"
#include "utils/builtins.h"
#include "nodes/nodes.h"
#include "miscadmin.h"
#include "parser/parse_node.h"
#include "synonym.h"

PG_MODULE_MAGIC;

static ProcessUtility_hook_type prev_ProcessUtility = NULL;
static SynnameGetRelname_hook_type pre_SynnameGetRelname = NULL;
static SynnameGetFuncInfo_hook_type pre_SynnameGetFuncInfo_hook = NULL;
static SynnameGettype_hook_type	pre_SynnameGettype_hook = NULL;
static SynnameGetPkgoid_recursive_hook_type pre_SynnameGetPkgoid_hook;
static SynIdGetname_hook_type pre_SynIdGetname_hook;
void		_PG_init(void);
void		_PG_fini(void);
static void syno_ProcessUtility(PlannedStmt *pstmt, const char *queryString,
						   bool readOnlyTree,
						   ProcessUtilityContext context, ParamListInfo params,
						   QueryEnvironment *queryEnv,
						   DestReceiver *dest, QueryCompletion *qc);

ObjectAddress CreateSynonym(CreateSynonymStmt *stmt);
ObjectAddress AlterSynonym(AlterSynonymStmt *stmt);
void DropSynonym(DropSynStmt *stmt);
void SynIdGetname(Oid synid, char **synname, char **synnsp);


void
_PG_init(void)
{
	/* 
	 * utility hooks for create/alter/drop synonym
	 */
	prev_ProcessUtility = ProcessUtility_hook;
    ProcessUtility_hook = syno_ProcessUtility;

	/*
	 * synonym reference is table object
	 */
	pre_SynnameGetRelname = SynnameGetRelname_hook;
	SynnameGetRelname_hook = SynnameGetRelname;

	/*
	 * synonym reference is func object
	 */
	pre_SynnameGetFuncInfo_hook = SynnameGetFuncInfo_hook;
	SynnameGetFuncInfo_hook = SynnameGetFuncInfo;

	/*
	 * synonym reference is user-define data type
	 */
	pre_SynnameGettype_hook = SynnameGettype_hook;
	SynnameGettype_hook = LookupTypeNameFromSynonym;

	/*
	 * synonym reference is package
	 */
	pre_SynnameGetPkgoid_hook = SynnameGetPkgoid_recursive_hook;
	SynnameGetPkgoid_recursive_hook = SynnameGetPkgoid_recursive;
	/*
	 * Get synname from syn oid
	 */
	pre_SynIdGetname_hook = SynIdGetname_hook;
	SynIdGetname_hook = SynIdGetname;
}

void
_PG_fini(void)
{
	/* Uninstall hooks. */
	ProcessUtility_hook = prev_ProcessUtility;
	SynnameGetRelname_hook = pre_SynnameGetRelname;
	SynnameGetFuncInfo_hook = pre_SynnameGetFuncInfo_hook;
	SynnameGettype_hook = pre_SynnameGettype_hook;
	SynnameGetPkgoid_recursive_hook = pre_SynnameGetPkgoid_hook;
	SynIdGetname_hook = pre_SynIdGetname_hook;
}

/*
 * ----------------------------------
 *	Process CREATE/ALTER/DROP synonym
 * ----------------------------------
 */

static void
syno_ProcessUtility(PlannedStmt *pstmt, const char *queryString,
						bool readOnlyTree,
						ProcessUtilityContext context, ParamListInfo params,
						QueryEnvironment *queryEnv,
						DestReceiver *dest, QueryCompletion *qc)
{
	Node       *parsetree = pstmt->utilityStmt;
	bool	needCleanup;
	bool	isCompleteQuery = (context != PROCESS_UTILITY_SUBCOMMAND);

	switch (nodeTag(parsetree))
	{
		case T_CreateSynonymStmt:
			{
				needCleanup = isCompleteQuery && EventTriggerBeginCompleteQuery();
				if (isCompleteQuery)
					EventTriggerDDLCommandStart(parsetree);

				CreateSynonym((CreateSynonymStmt *) parsetree);

				if (isCompleteQuery)
				{
					EventTriggerSQLDrop(parsetree);
					EventTriggerDDLCommandEnd(parsetree);
				}
				if (needCleanup)
					EventTriggerEndCompleteQuery();
			}
			break;
		case T_AlterSynonymStmt:
			{
				/* no event triggers for alter synonym */
				AlterSynonym((AlterSynonymStmt *)parsetree);
			}
			break;
		case T_DropSynStmt:
			{
				needCleanup = isCompleteQuery && EventTriggerBeginCompleteQuery();
				if (isCompleteQuery)
					EventTriggerDDLCommandStart(parsetree);

				DropSynonym((DropSynStmt *)parsetree);

				if (isCompleteQuery)
				{
					EventTriggerSQLDrop(parsetree);
					EventTriggerDDLCommandEnd(parsetree);
				}
				if (needCleanup)
					EventTriggerEndCompleteQuery();
			}
			break;
		default:
			{
				if (prev_ProcessUtility)
        			prev_ProcessUtility(pstmt, queryString, readOnlyTree,
                                                context, params, queryEnv,
                                                        dest, qc);
        		else
            		standard_ProcessUtility(pstmt, queryString, readOnlyTree,
                                                context, params, queryEnv,
                                                        dest, qc);

			}
			break;
	}
}


/*
 * ---------------------------------
 *		Create Synonym Statement
 * ---------------------------------
 */
ObjectAddress
CreateSynonym(CreateSynonymStmt *stmt)
{
	char	   *synname;
	char	   *objname;
	char	   *synnsp;
	char	   *objnsp;
	Relation	rel;
	TupleDesc	tupDesc;
	HeapTuple	tup;
	HeapTuple	oldtup;
	Oid			namespaceId;
	NameData	pg_synname;
	NameData	pg_objnsp;
	NameData	pg_objname;
	NameData	pg_dblinkname;
	NameData	pg_dblinkschema;	/* Bug#Z5823 */
	ObjectAddress myself,
				referenced;
	int			i;
	bool		nulls[Natts_pg_synonym];
	Datum		values[Natts_pg_synonym];
	bool		replaces[Natts_pg_synonym];
	Oid			objOwner = InvalidOid;
	Oid			synOwner = GetUserId();

	Assert(stmt->synonym);
	Assert(stmt->object);

	synname = stmt->synonym->relname;
	objname = stmt->object->relname;
	synnsp = stmt->synonym->schemaname;
	objnsp = stmt->object->schemaname;


	/*
	 * The synonym name cannot same as object name in the same namespace,
	 * later check whether namespace exists
	 */
	if (!synnsp && !objnsp)
	{
		if (pg_strcasecmp(synname, objname) == 0)
			ereport(ERROR,
                    (errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
					errmsg("cannot create a synonym with same as object")));
	}
	else if (synnsp && objnsp && pg_strcasecmp(synnsp, objnsp) == 0)
	{
		if (pg_strcasecmp(synname, objname) == 0)
			ereport(ERROR,
                    (errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
					errmsg("cannot create a synonym with same as object")));
	}

	/*
	 * If synonym scheme is specified, check if the schema exists.
	 */
	if (synnsp)
	{
		namespaceId = get_namespace_oid(synnsp, true);
		if (!OidIsValid(namespaceId))
			ereport(ERROR,
                    (errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
					errmsg("schema \"%s\" does not exist", synnsp)));
	}
	else
	{
		if (stmt->synispub)
			namespaceId = PG_PUBLIC_NAMESPACE;
		else
			namespaceId = QualifiedNameGetCreationNamespace(list_make1(makeString(synname)), &synname);
	}

	(void) fit_namespace_owner(namespaceId, &synOwner);

	/*
	 * when not specify synnsp or objnsp,check if the synonym name and object name with
	 * the same namespace
	 */
	if (objnsp)
	{
		if (pg_strcasecmp(get_namespace_name(namespaceId), objnsp) == 0 &&
					pg_strcasecmp(synname, objname) == 0)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
					errmsg("cannot create a synonym with same as object")));
	}
	else
	{
		/* if objnsp is null,get user default namespace */
		objOwner = get_namespace_oid(GetUserNameFromId(GetUserId(), true), true);
		if (!OidIsValid(objOwner))
			objOwner = QualifiedNameGetCreationNamespace(list_make1(makeString(objname)), &objname);
		if ((namespaceId == objOwner) && pg_strcasecmp(synname, objname) == 0)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
					errmsg("cannot create a synonym with same as object")));
	}

	for (i = 0; i < Natts_pg_synonym; ++i)
	{
		nulls[i] = false;
		values[i] = (Datum) 0;
		replaces[i] = false;
	}


	namestrcpy(&pg_synname, synname);
	namestrcpy(&pg_objname, objname);
	values[Anum_pg_synonym_synname - 1] = NameGetDatum(&pg_synname);
	values[Anum_pg_synonym_synnamespace - 1] = ObjectIdGetDatum(namespaceId);
	values[Anum_pg_synonym_objname - 1] = NameGetDatum(&pg_objname);
	values[Anum_pg_synonym_synowner - 1] = ObjectIdGetDatum(synOwner);
	if (objnsp)
	{
		namestrcpy(&pg_objnsp, objnsp);
		values[Anum_pg_synonym_table_namespace - 1] = NameGetDatum(&pg_objnsp);
	}
	else
	{
		namestrcpy(&pg_objnsp, get_namespace_name(objOwner));
		values[Anum_pg_synonym_table_namespace - 1] = NameGetDatum(&pg_objnsp);
	}

	if(stmt->synispub)
		values[Anum_pg_synonym_synispub - 1] = true;
	else
		values[Anum_pg_synonym_synispub - 1] = false;

	if(stmt->editable == FLAG_EDITIONABLE)
		values[Anum_pg_synonym_synisedit -1] = true;
	else
		values[Anum_pg_synonym_synisedit -1] = false;

	if (stmt->object->srvname != NULL)
	{
		namestrcpy(&pg_dblinkname, stmt->object->srvname);
		values[Anum_pg_synonym_db_link - 1] = NameGetDatum(&pg_dblinkname);
		/* Begin - BUG#Z5823 */
		if (stmt->object->schemaname != NULL)
			values[Anum_pg_synonym_table_namespace - 1] = CStringGetDatum(stmt->object->schemaname);
		else
		{
			/* If no schem is specified in the dblink, it should be null */
			namestrcpy(&pg_dblinkschema, "");
			values[Anum_pg_synonym_table_namespace - 1] = NameGetDatum(&pg_dblinkschema);
		}
		/* End - BUG#Z5823 */
	}
	else
	{
		namestrcpy(&pg_dblinkname, "");
		values[Anum_pg_synonym_db_link - 1] = NameGetDatum(&pg_dblinkname);
	}

	rel = table_open(SynonymRelationId, RowExclusiveLock);

	tupDesc = RelationGetDescr(rel);

	oldtup = SearchSysCache3(SYNONYMNAMENSP, PointerGetDatum(synname),
								ObjectIdGetDatum(namespaceId), BoolGetDatum(stmt->synispub));
	if (HeapTupleIsValid(oldtup))
	{
		/* already exist one */
		Form_pg_synonym oldsyn = (Form_pg_synonym) GETSTRUCT(oldtup);

		if (!stmt->replace)
			elog(ERROR, "name is already used by an existing object");

		replaces[Anum_pg_synonym_table_namespace - 1] = true;
		replaces[Anum_pg_synonym_synisedit - 1] = true;
		replaces[Anum_pg_synonym_objname - 1] = true;
		replaces[Anum_pg_synonym_db_link - 1] = true;

		tup = heap_modify_tuple(oldtup, tupDesc, values, nulls, replaces);
		CatalogTupleUpdate(rel, &tup->t_self, tup);

		myself.classId = SynonymRelationId;
		myself.objectId = oldsyn->oid;
		myself.objectSubId = 0;

		ReleaseSysCache(oldtup);
	}
	else
	{
		Oid newoid;
		/* create a new synonym */
		newoid = GetNewOidWithIndex(rel, SynonymOidIndexId,
										Anum_pg_synonym_oid);
		values[Anum_pg_synonym_oid - 1] = ObjectIdGetDatum(newoid);
		tup = heap_form_tuple(tupDesc, values, nulls);
		CatalogTupleInsert(rel, tup);

		myself.classId = SynonymRelationId;
		myself.objectId = newoid;
		myself.objectSubId = 0;
	}

	 /* dependency on namespace */
	referenced.classId = NamespaceRelationId;
	referenced.objectId = namespaceId;
	referenced.objectSubId = 0;
	recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);

	CommandCounterIncrement();

	table_close(rel, RowExclusiveLock);

	return myself;
}

/*
 * ---------------------------------
 *		Alter Synonym Statement
 * ---------------------------------
 */

ObjectAddress
AlterSynonym(AlterSynonymStmt *stmt)
{
	Oid synoid;
	Relation rel;
	HeapTuple tup;
	Datum	repl_val[Natts_pg_synonym];
	bool	repl_null[Natts_pg_synonym];
	bool	repl_repl[Natts_pg_synonym];
	ObjectAddress address;
	
	rel = table_open(SynonymRelationId, RowExclusiveLock);
	synoid = LookupSynonymByNames(stmt->synname, false);
	tup = SearchSysCacheCopy1(SYNONYMOID, ObjectIdGetDatum(synoid));
	if (!HeapTupleIsValid(tup)) /* should not happen */
		elog(ERROR, "cache lookup failed for synonym %u", synoid);
	
	/* TODO: synonym owner check */
	
	memset(repl_repl, false, sizeof(repl_repl));
	if (stmt->compile_flag)
	{
		repl_repl[Anum_pg_synonym_synisedit - 1] = true;
		repl_val[Anum_pg_synonym_synisedit - 1] = BoolGetDatum(stmt->compile_flag);
		repl_null[Anum_pg_synonym_synisedit - 1] = false;
	}
	else
	{
		bool		editionable;
		
		if(stmt->editable == FLAG_EDITIONABLE)
			editionable = true;
		else
			editionable = false;
		repl_repl[Anum_pg_synonym_synisedit - 1] = true;
		repl_val[Anum_pg_synonym_synisedit - 1] = BoolGetDatum(editionable);
		repl_null[Anum_pg_synonym_synisedit - 1] = false;
	}
	tup = heap_modify_tuple(tup, RelationGetDescr(rel),
								repl_val, repl_null, repl_repl);
	CatalogTupleUpdate(rel, &tup->t_self, tup);
	InvokeObjectPostAlterHook(SynonymRelationId, synoid, 0);

	ObjectAddressSet(address, SynonymRelationId, synoid);

	heap_freetuple(tup);
	table_close(rel, NoLock);

	return address;
}

/*
 * ---------------------------------
 *		Drop Synonym Statement
 * ---------------------------------
 */
void
DropSynonym(DropSynStmt *stmt)
{
	Relation rel;
	HeapTuple tup;
	char	*synname;
	Oid		namespaceId;

	rel = table_open(SynonymRelationId, RowExclusiveLock);
	/* Begin - BUG#Z5790 */
	if (stmt->ispub)
		namespaceId = PG_PUBLIC_NAMESPACE;
	else
		namespaceId = RangeVarGetAndCheckCreationNamespace(stmt->synname, NoLock, NULL);
	/* End - BUG#Z5790 */
	synname = stmt->synname->relname;
	tup = SearchSysCache3(SYNONYMNAMENSP, PointerGetDatum(synname),
								ObjectIdGetDatum(namespaceId), BoolGetDatum(stmt->ispub));
	if (HeapTupleIsValid(tup))
	{
		/* Begin - BUG#Z5798 */
		ObjectAddresses *objects;
		Relation	relation = NULL;
		ObjectAddress address;

		/* Lock and validate each relation; build a list of object addresses */
		objects = new_object_addresses();

		address = get_object_address(OBJECT_SYNONYM,
									(Node *)list_make2(makeString(get_namespace_name(namespaceId)), makeString(synname)),
									&relation,
									AccessExclusiveLock,
									true);
		Assert(relation == NULL);
		add_exact_object_address(&address, objects);

		/* really delete */
		performMultipleDeletions(objects, DROP_RESTRICT, 0);

		free_object_addresses(objects);
		/* End - BUG#Z5798 */
		ReleaseSysCache(tup);
	}
	else
	{
		if (stmt->ispub)
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_OBJECT),
					 errmsg("public synonym \"%s\" to be deleted does not exist", synname)));
		else
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_OBJECT),
					 errmsg("private synonym \"%s\" to be deleted does not exist", synname)));
	}

	table_close(rel, NoLock);
}

/*
 * Recursively get the final table object represented by the synonym
 */
Oid
SynnameGetRelname_recursive(const char *synname, Oid synnsp, char **objschema, char **objname, char **objdblnksrv, List **synoids)
{
	Oid synoid;
	Oid reloid;
	HeapTuple	tup;

	/* Priority search for private synonym */
	synoid = GetSysCacheOid3(SYNONYMNAMENSP, Anum_pg_synonym_oid,
						   PointerGetDatum(synname),
						   ObjectIdGetDatum(synnsp),
						   BoolGetDatum(false));
	if (!OidIsValid(synoid))
		synoid = GetSysCacheOid3(SYNONYMNAMENSP, Anum_pg_synonym_oid,
						   PointerGetDatum(synname),
						   ObjectIdGetDatum(synnsp),
						   BoolGetDatum(true));
	if (OidIsValid(synoid))
	{
		if (list_member_oid(*synoids, synoid))
			elog(ERROR, "looping chain of synonyms");
		else
			*synoids = lappend_oid(*synoids, synoid);

		tup = SearchSysCache1(SYNONYMOID, ObjectIdGetDatum(synoid));
		if (HeapTupleIsValid(tup))
		{
			char *objnsp;
			char *obj_name;
			char *dblinkname;
			Oid objnspoid;
			
			Form_pg_synonym syntup = (Form_pg_synonym) GETSTRUCT(tup);
			objnsp = pstrdup(NameStr(syntup->table_namespace));
			obj_name = pstrdup(NameStr(syntup->objname));
			dblinkname = pstrdup(NameStr(syntup->db_link));

			if (pg_strcasecmp(dblinkname, "") != 0)
			{
				*objschema = objnsp;
				*objname = obj_name;
				*objdblnksrv = dblinkname;
				
				ReleaseSysCache(tup);
				return synoid;
			}
			else
			{
				objnspoid = LookupExplicitNamespace(objnsp, true);
				if (!OidIsValid(objnspoid))
					reloid = RelnameGetRelid(obj_name);
				else
					reloid = get_relname_relid(obj_name, objnspoid);

				if (OidIsValid(reloid))
				{
					*objschema = objnsp;
					*objname = obj_name;
					*objdblnksrv = NULL;
					ReleaseSysCache(tup);
					return synoid;
				}
				else if (!OidIsValid(reloid) && OidIsValid(objnspoid))
					reloid = SynnameGetRelname_recursive(obj_name, objnspoid, objschema, objname, objdblnksrv, synoids);
				else if (!OidIsValid(reloid) && !OidIsValid(objnspoid))
					reloid = SynnameGetRelname(obj_name, NULL, objschema, objname, objdblnksrv, synoids);

				if (!OidIsValid(reloid))
					ereport(ERROR,
							(errcode(ERRCODE_UNDEFINED_OBJECT),
					 		errmsg("synonym translation is no longer valid")));
				ReleaseSysCache(tup);

				return synoid;
			}
		}
	}
	
	return InvalidOid;
}

/*
 * Find function/procedure object by synonym name
 */
bool
SynnameGetFuncInfo(const char *synname, const char *synnsp, char **funcname, char **funcnsp, List **synoids)
{
	Oid synnspoid;
	Oid	synoid;
	HeapTuple	tup;

	if (synnsp)
	{
		synnspoid = get_namespace_oid(synnsp, false);
		synoid = get_synonym_synid(synname, synnspoid);
	}
	else
	{
		synoid = synnameGetSynid(synname);
	}

	if (OidIsValid(synoid))
	{
		Form_pg_synonym syntup;

		if (list_member_oid(*synoids, synoid))
			elog(ERROR, "looping chain of synonyms");
		else
			*synoids = lappend_oid(*synoids, synoid);

		tup = SearchSysCache1(SYNONYMOID, ObjectIdGetDatum(synoid));
		syntup = (Form_pg_synonym) GETSTRUCT(tup);
		*funcnsp = pstrdup(NameStr(syntup->table_namespace));
		*funcname = pstrdup(NameStr(syntup->objname));

		ReleaseSysCache(tup);

		return true;
	}
	
	return false;
}

/*
 * Find user-define type object by synonym name
 */
Type 
LookupTypeNameFromSynonym(ParseState *pstate, const TypeName *typeName, int32 *typmod_p, List **synoids, bool missing_ok)
{
	Relation	pg_synonym_rel;
	List   		*synnamelist = typeName->names;
	char	   	*synname = NULL;
	char		*schemaname = NULL;
	Oid			scheamoid = InvalidOid;
	HeapTuple	tuple = NULL;
	Form_pg_synonym synonymform = NULL;
	TypeName 	*temptypeName = NULL;
	Form_pg_type formpgtype = NULL;
	Type		tup;
	
	pg_synonym_rel = table_open(SynonymRelationId, RowExclusiveLock);

	if (typeName->names == NIL)
	{
		table_close(pg_synonym_rel, NoLock);
		return NULL;
	}
	else
	{
		DeconstructQualifiedName(synnamelist, &schemaname, &synname);
		if (schemaname)
			scheamoid = LookupExplicitNamespace(schemaname, missing_ok);
		else
			scheamoid = SynnameGetSynnsp(synname);
	}

	if (!OidIsValid(scheamoid))
	{
		table_close(pg_synonym_rel, NoLock);
		return NULL;
	}
	
	tuple = SearchSysCache3(SYNONYMNAMENSP, PointerGetDatum(synname),
								PointerGetDatum(scheamoid), PointerGetDatum(false));

	if (!HeapTupleIsValid(tuple))
		tuple = SearchSysCache3(SYNONYMNAMENSP, PointerGetDatum(synname),
									PointerGetDatum(scheamoid), PointerGetDatum(true));

	if (!HeapTupleIsValid(tuple))
	{
		table_close(pg_synonym_rel, NoLock);
		return NULL;
	}
	else
	{
		char 	*typname;
		char	*typensp;
		Oid		synoid;
		
		synonymform = (Form_pg_synonym) GETSTRUCT(tuple);
		synoid = synonymform->oid;
		temptypeName = (TypeName *)copyObject(typeName);

		if (list_member_oid(*synoids, synoid))
			elog(ERROR, "looping chain of synonyms");
		else
			*synoids = lappend_oid(*synoids, synoid);

		typname = pstrdup(NameStr(synonymform->objname));
		typensp = pstrdup(NameStr(synonymform->table_namespace));

		/* deconstruct the name list */
		switch (list_length(temptypeName->names))
		{
			case 1:
				strVal(linitial(temptypeName->names)) = typname;
				break;
			case 2:
				strVal(linitial(temptypeName->names)) = typensp;
				strVal(lsecond(temptypeName->names)) = typname;
				break;
			case 3:
				strVal(lsecond(temptypeName->names)) = typensp;
				strVal(lthird(temptypeName->names)) = typname;
				break;
			default:
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("improper %%TYPE reference (too many dotted names): %s",
								NameListToString(temptypeName->names)),
						 parser_errposition(pstate, temptypeName->location)));
				break;
		}

		tup = LookupTypeName(pstate, temptypeName, typmod_p, false);

		if (tup == NULL)
			tup = LookupTypeNameFromSynonym(pstate, temptypeName, typmod_p, synoids, missing_ok);

		if (tup == NULL)
		{
			table_close(pg_synonym_rel, NoLock);
			return NULL;
		}

		formpgtype = (Form_pg_type) GETSTRUCT(tup);

		if (formpgtype != NULL)
		{
			/* the only support compsite type */
			if (formpgtype->typtype == 'b' || formpgtype->typtype  == 'd'  
				|| formpgtype->typtype  == 'e' || formpgtype->typtype  == 'p'
				|| formpgtype->typtype  == 'r')
			{
				ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("type \"%s\" does not exist",
						TypeNameToString(typeName)),
				 parser_errposition(pstate, typeName->location)));
			}
		}

		pfree(temptypeName);
		pfree(typname);
		pfree(typensp);
	}

	ReleaseSysCache(tuple);
	table_close(pg_synonym_rel, NoLock);

	return tup;
}

/*
 * find the package oid by synonym name
 */
Oid
SynnameGetPkgoid_recursive(const char *synname, Oid nspkg, List **synoids)
{
	Oid synoid;
	Oid pkgoid;
	HeapTuple	tup;

	/* Priority search for private synonym */
	synoid = GetSysCacheOid3(SYNONYMNAMENSP, Anum_pg_synonym_oid,
						   PointerGetDatum(synname),
						   ObjectIdGetDatum(nspkg),
						   BoolGetDatum(false));
	if (!OidIsValid(synoid))
		synoid = GetSysCacheOid3(SYNONYMNAMENSP, Anum_pg_synonym_oid,
						   PointerGetDatum(synname),
						   ObjectIdGetDatum(nspkg),
						   BoolGetDatum(true));
	if (OidIsValid(synoid))
	{
		/* check if synonyms form rings */
		if (list_member_oid(*synoids, synoid))
			elog(ERROR, "looping chain of synonyms");
		else
			*synoids = lappend_oid(*synoids, synoid);

		tup = SearchSysCache1(SYNONYMOID, ObjectIdGetDatum(synoid));
		if (HeapTupleIsValid(tup))
		{
			char *objnsp;
			char *obj_name;
			Oid objnspoid;
			List *tmp;

			Form_pg_synonym syntup = (Form_pg_synonym) GETSTRUCT(tup);
			objnsp = pstrdup(NameStr(syntup->table_namespace));
			obj_name = pstrdup(NameStr(syntup->objname));
			objnspoid = LookupExplicitNamespace(objnsp, true);
			tmp = lappend(list_make1(makeString(objnsp)), makeString(obj_name));
			pkgoid = LookupPackageByNames(tmp, true);
			if (OidIsValid(pkgoid))
			{
				list_free(tmp);
				ReleaseSysCache(tup);
				return pkgoid;
			}
			else
				pkgoid = SynnameGetPkgoid_recursive(obj_name, objnspoid, synoids);
				
			if (!OidIsValid(pkgoid))
					ereport(ERROR,
							(errcode(ERRCODE_UNDEFINED_OBJECT),
					 		errmsg("synonym translation is no longer valid")));
			pfree(objnsp);
			pfree(obj_name);
			list_free(tmp);
			ReleaseSysCache(tup);
			
			return pkgoid;
		}
	}

	return InvalidOid;
}

void
SynIdGetname(Oid synid, char **synname, char **synnsp)
{
	HeapTuple	tup;
	tup = SearchSysCache1(SYNONYMOID, ObjectIdGetDatum(synid));
	if (HeapTupleIsValid(tup))
	{
		Form_pg_synonym syntup = (Form_pg_synonym) GETSTRUCT(tup);
		*synnsp = pstrdup(NameStr(syntup->table_namespace));
		*synname = pstrdup(NameStr(syntup->synname));
		ReleaseSysCache(tup);
	}

	return;
}

/*
 * Get synonym name according to relation name
 */
Oid
SynnameGetRelname(const char *synname, const char *synnsp,
				char **objschema, char **objname, char **objdblnksrv, List	**synoids)
{
	Oid			relid;
	Oid			synnspoid;

	if (synnsp != NULL)
	{
		synnspoid = get_namespace_oid(synnsp, true);
		if (!OidIsValid(synnspoid))
			return InvalidOid;
		relid = SynnameGetRelname_recursive(synname, synnspoid, objschema, objname, objdblnksrv, synoids);
		if (OidIsValid(relid))
			return relid;
	}
	else
	{
		synnspoid = SynnameGetSynnsp(synname);

		relid = SynnameGetRelname_recursive(synname, synnspoid, objschema, objname, objdblnksrv, synoids);
		if (OidIsValid(relid))
			return relid;
	}
	/* Not found in path */
	return InvalidOid;
}

