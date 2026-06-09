/*-------------------------------------------------------------------------
 *
 * casesensitive.c
 *	  MySQL case routines
 *
 * Portions Copyright (c) 2023-, Highgo MySQL Compatible Development Group
 *
 *
 * IDENTIFICATION
 *	  contrib/ivorysql_my/src/case_sensitive/casesenstive.c
 *
 * NOTES
 *	  These routines allow all to perform
 *
 *	  see contrib/ivorysql_my/ivorysql_my.h for a list of the cache IDs
 *
 * add the file for requirement "SRS-MYSQL-CASE"
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "c.h"

#include "access/xact.h"
#include "access/table.h"
#include "access/heapam.h"
#include "catalog/dependency.h"
#include "catalog/objectaccess.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_object.h"
#include "catalog/indexing.h"
#include "catalog/namespace.h"
#include "catalog/pg_namespace.h"
#include "catalog/pg_database.h"
#include "utils/guc.h"
#include "utils/builtins.h"
#include "utils/ora_compatible.h"
#include "utils/syscache.h"
#include "utils/timestamp.h"
#include "utils/rel.h"
#include "utils/lsyscache.h"
#include "parser/scansup.h"
#include "mysql_parser/mys_case_sensitive.h"
#include "../include/ivorysql_my.h"
#include "../include/casesensitive.h"
#include "nodes/parsenodes.h"
#include "nodes/pg_list.h"
#include "nodes/value.h"


/* pg_compatible_mysql_lower()
 * Get whether the system is mysql and whether uppercase
 * and lowercase letters are turned on, database, schema and
 * same object transfor to case.
 *
 * return a bool value
 */
bool pg_compatible_mysql_lower(void)
{
	return lower_case_table_names == 1 || lower_case_table_names == 2;
}

/* pg_compatible_mysql_ojblower()
 * Get whether the system is mysql and whether uppercase
 * and lowercase letters are turned on, and current status
 * of lower_case_table_names.
 *
 * return a enum value
 */
MysLowerCaseMode pg_compatible_mysql_ojblower(void)
{
	if (lower_case_table_names == 0)
		return MYS_LOWER_SENSITIVE;
	if (lower_case_table_names == 1)
		return MYS_LOWER_INSENSITIVE;
	if (lower_case_table_names == 2)
		return MYS_LOWER_MIXED;

	return MYS_LOWER_INVALID;
}

/* pg_compatible_mysql_case()
 * Convert string to lowercase.
 *
 * return lowercase string
 */
char *pg_compatible_mysql_case(char *str)
{
	if (str)
		return downcase_truncate_identifier(str, strlen(str), true);

	return str;
}

/* pg_compatible_mysql_list_case()
 * Convert list node to lowercase.
 *
 * return lowercase string
 */
void pg_compatible_mysql_list_case(List *list)
{
	ListCell *cell;

	if (list == NIL)
		return;

	foreach (cell, list)
	{
		if (IsA(lfirst(cell), String))
		{
			char * str = strVal(lfirst(cell));

			strVal(lfirst(cell)) = pg_compatible_mysql_case(str);
		}
		else if (IsA(lfirst(cell), RangeVar))
		{
			RangeVar *rvar;

			rvar = (RangeVar *)lfirst(cell);
			if (rvar->relname)
				((RangeVar *)lfirst(cell))->relname = mysql_string_case_hook(rvar->relname);
		}
	}
}

/* pg_compatible_mysql_rangvar_case()
 * Convert RangeVar to lowercase.
 *
 * return
 */
void pg_compatible_mysql_rangvar_case(RangeVar            *relation)
{
	if (relation == NULL)
		return;

	if (relation->schemaname)
		relation->schemaname = pg_compatible_mysql_case(relation->schemaname);
	if (relation->relname)
		relation->relname = pg_compatible_mysql_case(relation->relname);
}

/* pg_compatible_mysql_get_typoid()
 * In mysql mode, get the oid of type.
 *
 * return type oid
 */
Oid pg_compatible_mysql_get_typoid(ParseState *pstate, const TypeName *typeName, bool temp_ok,
												char *schemaname, char *typname, bool missing_ok)
{
	List	   *names = typeName->names;
	Oid			typoid = InvalidOid;
	char	   *tmpschema = NULL;
	char	   *tmptypname = NULL;

	switch (list_length(names))
	{
		case 1:
			typname = strVal(linitial(names));
			break;
		case 2:
			schemaname = strVal(linitial(names));
			typname = strVal(lsecond(names));
			break;
		case 3:
			schemaname = strVal(lsecond(names));
			typname = strVal(lthird(names));
			break;
		default:
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("improper qualified name (too many dotted names): %s",
							NameListToString(names))));
			break;
	}

	if (schemaname)
	{
		/* Look in specific schema only */
		Oid			namespaceId;
		ParseCallbackState pcbstate;

		tmpschema = pg_compatible_mysql_case(schemaname);
		tmptypname = pg_compatible_mysql_case(typname);

		setup_parser_errposition_callback(&pcbstate, pstate, typeName->location);

		namespaceId = LookupExplicitNamespace(tmpschema, true);
		if (OidIsValid(namespaceId))
		{
			typoid = GetSysCacheOid2(TYPENAMENSP, Anum_pg_type_oid,
									 PointerGetDatum(tmptypname),
									 ObjectIdGetDatum(namespaceId));
			if (!OidIsValid(typoid))
				typoid = GetSysCacheOid2(TYPENAMENSP, Anum_pg_type_oid,
										 PointerGetDatum(typname),
										 ObjectIdGetDatum(namespaceId));
		}
		else
			typoid = InvalidOid;

		if (!OidIsValid(typoid))
		{
			namespaceId = LookupExplicitNamespace(schemaname, missing_ok);

			if (OidIsValid(namespaceId))
			{
				typoid = GetSysCacheOid2(TYPENAMENSP, Anum_pg_type_oid,
										 PointerGetDatum(tmptypname),
										 ObjectIdGetDatum(namespaceId));
				if (!OidIsValid(typoid))
					typoid = GetSysCacheOid2(TYPENAMENSP, Anum_pg_type_oid,
											 PointerGetDatum(typname),
											 ObjectIdGetDatum(namespaceId));
			}
			else
				typoid = InvalidOid;
		}

		cancel_parser_errposition_callback(&pcbstate);
	}
	else
	{
		tmptypname = pg_compatible_mysql_case(typname);
		typoid = TypenameGetTypidExtended(tmptypname, temp_ok);

		/* Unqualified type name, so search the search path */
		if (!OidIsValid(typoid))
			typoid = TypenameGetTypidExtended(typname, temp_ok);
	}
	return typoid;
}

/* mysql_get_namespace_oid()
 * get object schema oid.
 *
 * return
 */
Oid mysql_get_namespace_oid(const char *nspname)
{
	Oid			oid = InvalidOid;
	char	   *casenspname = NULL;

	if (!nspname)
		return oid;

	if (compatible_mode == MYS_MODE)
	{
		/* BUG#5975, If the schema is information_schema, and MySQL
		 * case sensitivity is enabled, convert it entirely to lowercase.
		 */
		if ((lower_case_table_names == 1 || lower_case_table_names == 2) ||
			pg_strcasecmp(nspname, "information_schema") == 0)		/* BUG#5975 */
		{
			casenspname = downcase_truncate_identifier(pstrdup(nspname), 
														strlen(pstrdup(nspname)), true);
			oid = GetSysCacheOid1(NAMESPACENAME, Anum_pg_namespace_oid,
							  CStringGetDatum(casenspname));
		}
		else
			oid = InvalidOid;
	}

	return oid;
}

/* get_myproc_obj_nameinfo_bybodyid()
 * get object and schema name from pg_object.
 *
 * return
 */
void
get_obj_nameinfo_bybodyid(ObjectAddress address, char **objname, char **schema)
{
	HeapTuple tup;

	if (objname)
		*objname = NULL;
	if (schema)
		*schema = NULL;

	tup = SearchSysCache3(OBJECTREFCLASSIDOIDSUBID,
						  ObjectIdGetDatum(address.classId),
						  ObjectIdGetDatum(address.objectId),
						  Int32GetDatum(address.objectSubId));
	if (HeapTupleIsValid(tup))
	{
		Oid		schemaoid = InvalidOid;
		/* Begin - BUG#Z00004347 */
		Form_pg_object	object_struct = (Form_pg_object) GETSTRUCT(tup);
		HeapTuple newtup;


		*objname = pstrdup(object_struct->objname.data);
		schemaoid = object_struct->schemaoid;

		newtup = SearchSysCache3(OBJECTREFCLASSIDOIDSUBID,
								 ObjectIdGetDatum(NamespaceRelationId),
								 ObjectIdGetDatum(schemaoid),
								 Int32GetDatum(0));
		if (HeapTupleIsValid(newtup))
		{
			Form_pg_object	object_struct1 = (Form_pg_object) GETSTRUCT(newtup);

			*schema = pstrdup(object_struct1->objname.data);

			ReleaseSysCache(newtup);
		}
		else
		{
			newtup = SearchSysCache1(NAMESPACEOID, ObjectIdGetDatum(schemaoid));
			if (HeapTupleIsValid(newtup))
			{
				Form_pg_namespace  namespace_struct = (Form_pg_namespace) GETSTRUCT(newtup);

				*schema = pstrdup(namespace_struct->nspname.data);

				ReleaseSysCache(newtup);
			}
		}
		/* End - BUG#Z00004347 */
		
		ReleaseSysCache(tup);
	}
}

/* mysql_SearchAttName()
 * get column infomation, hrough table structure.
 *
 * return column information.
 */
HeapTuple
mysql_SearchAttName(Oid relid, char *attname)
{
	Relation	rel;
	TupleDesc	tupDesc;
	int			natts;
	int			attno;
	bool		nameexist = false;
	char	   *realattname;
	HeapTuple	classTuple; /* BUG:Z4315 Z4316 */

	if (!OidIsValid(relid) || attname == NULL)
		return NULL;

	/* Begin - BUG:Z4315 Z4316 */
	classTuple = SearchSysCache1(RELOID, ObjectIdGetDatum(relid));
	if (!HeapTupleIsValid(classTuple))
		return NULL;
	else if (((Form_pg_class) GETSTRUCT(classTuple))->relkind == RELKIND_COMPOSITE_TYPE)
	{
		ReleaseSysCache(classTuple);
		return NULL;
	}
	else
		ReleaseSysCache(classTuple);
	/* Begin - BUG:Z4315 Z4316 */

	rel = table_open(relid, RowExclusiveLock);
	tupDesc = RelationGetDescr(rel);
	natts = tupDesc->natts;

	for (attno = 1; attno <= natts; attno++)
	{
		Form_pg_attribute attribute = TupleDescAttr(tupDesc, attno - 1);
		char	   *attributeName = NameStr(attribute->attname);
		if (pg_strcasecmp(attributeName, attname) == 0)
		{
			realattname = pstrdup(attributeName);
			nameexist = true;
			break;
		}
	}

	table_close(rel, RowExclusiveLock);

	if (nameexist)
	{
		/* Begin - BUG:Z4315 Z4316 */
		HeapTuple	tuple;

		tuple = SearchSysCache2(ATTNAME,
								ObjectIdGetDatum(relid),
								CStringGetDatum(realattname));
		if (!HeapTupleIsValid(tuple))
		{
			return NULL;
		}
		else if (((Form_pg_attribute) GETSTRUCT(tuple))->attisdropped)
		{
			ReleaseSysCache(tuple);
			return NULL;
		}
		else
			return tuple;

		/* End - BUG:Z4315 Z4316 */
	}

	return NULL;
}

/* Begin - BUG#Z00004315/4316 */
/*
 * If a new or renamed column will collide with the name of an existing
 * column and if_not_exists is false then error out, else do nothing.
 */
bool
mys_check_for_column_name_collision(Relation rel, const char *colname,
								bool if_not_exists, bool caseflg)
{
	HeapTuple	attTuple;
	int			attnum;
	TupleDesc	tupdesc = NULL;

	tupdesc = RelationGetDescr(rel);
	if (tupdesc)
	{
		int i;

		for (i = 0; i < tupdesc->natts; i++)
		{
			Form_pg_attribute att = TupleDescAttr(tupdesc, i);

			if (caseflg)
			{
				if (strcmp(colname, NameStr(att->attname)) == 0)
				{

					if (if_not_exists)
					{
						return false;
					}

					ereport(ERROR,
							(errcode(ERRCODE_DUPLICATE_COLUMN),
							 errmsg("column \"%s\" of relation \"%s\" already exists",
									colname, RelationGetRelationName(rel))));
				}
			}
			else
			{
				if (pg_strcasecmp(colname, NameStr(att->attname)) == 0)
				{

					if (if_not_exists)
					{
						ereport(NOTICE,
								(errcode(ERRCODE_DUPLICATE_COLUMN),
								 errmsg("column \"%s\" of relation \"%s\" already exists, skipping",
										colname, RelationGetRelationName(rel))));
						return false;
					}

					ereport(ERROR,
							(errcode(ERRCODE_DUPLICATE_COLUMN),
							 errmsg("column \"%s\" of relation \"%s\" already exists",
									colname, RelationGetRelationName(rel))));
				}
			}
		}
	}

	/*
	 * this test is deliberately not attisdropped-aware, since if one tries to
	 * add a column matching a dropped column name, it's gonna fail anyway.
	 */
	attTuple = SearchSysCache2(ATTNAME,
							   ObjectIdGetDatum(RelationGetRelid(rel)),
							   PointerGetDatum(colname));
	if (!HeapTupleIsValid(attTuple))
		return true;

	attnum = ((Form_pg_attribute) GETSTRUCT(attTuple))->attnum;
	ReleaseSysCache(attTuple);

	/*
	 * We throw a different error message for conflicts with system column
	 * names, since they are normally not shown and the user might otherwise
	 * be confused about the reason for the conflict.
	 */
	if (attnum <= 0)
		ereport(ERROR,
				(errcode(ERRCODE_DUPLICATE_COLUMN),
				 errmsg("column name \"%s\" conflicts with a system column name",
						colname)));
	else
	{
		if (if_not_exists)
		{
			ereport(NOTICE,
					(errcode(ERRCODE_DUPLICATE_COLUMN),
					 errmsg("column \"%s\" of relation \"%s\" already exists, skipping",
							colname, RelationGetRelationName(rel))));
			return false;
		}

		ereport(ERROR,
				(errcode(ERRCODE_DUPLICATE_COLUMN),
				 errmsg("column \"%s\" of relation \"%s\" already exists",
						colname, RelationGetRelationName(rel))));
	}

	return true;
}
/* End - BUG#Z00004315/4316 */
