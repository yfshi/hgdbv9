/*--------------------------------------------------------------------
 * 
 * File: pg_object_hook.c
 *
 * Abstract: 
 *    This file contains hooks declarations for pg_object.
 *
 * Authored by qinshiyu@highgo.com, 20240410.
 *
 * Copyright:
 * Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *    src/backend/parser/pg_object_hook.c
 *
 * add the file for requirement "SRS-SQL-Object"
 *
 *--------------------------------------------------------------------
 */

#include <assert.h>

#include "postgres.h"

#include "catalog/pg_class.h"
#include "catalog/pg_collation.h"
#include "catalog/pg_namespace.h"
#include "nodes/makefuncs.h"
#include "nodes/parsenodes.h"
#include "parser/pg_object_hook.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "miscadmin.h"  /* ReqID:SRS-SQL-SCHEMA */
#include "utils/rel.h"

pg_object_create_obj_hook_type      pg_object_create_obj_hook = NULL;
pg_object_alter_obj_hook_type       pg_object_alter_obj_hook = NULL;
pg_object_drop_obj_hook_type        pg_object_drop_obj_hook = NULL;
pg_object_get_ori_infor_hook_type   pg_object_get_ori_infor_hook = NULL;
pg_object_bulid_cache_obj_hook_type pg_object_bulid_relcache = NULL;
pg_object_bulid_cache_obj_hook_type pg_object_bulid_cache_obj_hook = NULL;

/* Begin - Req:SRS-SQL-NLSSORT */
#define COLLATION_KEY_VAL   "collation"
#define COLLATION_KEY_LEN   9
#define COLLATION_KEY_PATH "{collation}"

static void
CreateOrUpdatePgObjectCollationOption(char *relname, Oid namespaceId, Oid coll, ObjectAddress address, char objectKind)
{
    PgObjectInfo     info;
    
    if (!pg_object_create_obj_hook || 
        !pg_object_alter_obj_hook  || 
        !pg_object_get_ori_infor_hook)
    {
        elog(ERROR, "pg_object extension missing!");
    }

    info = pg_object_get_ori_infor_hook(address);

    if (info == NULL || info->orioption == NULL)
    {
        JsonbValue      *res;
        JsonbParseState *state = NULL;
        JsonbValue	     key, 
                         val;

        key.type = jbvString;
        key.val.string.val = COLLATION_KEY_VAL;
        key.val.string.len = COLLATION_KEY_LEN; // length of "collation";

        val.type = jbvNumeric;
        val.val.numeric = int64_to_numeric(coll);

        pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
        pushJsonbValue(&state, WJB_KEY, &key);
        pushJsonbValue(&state, WJB_VALUE, &val);
        res = pushJsonbValue(&state, WJB_END_OBJECT, NULL);

        if (info == NULL)
            pg_object_create_obj_hook(relname, 
                                      namespaceId, 
                                      true, 
                                      objectKind, 
                                      address, 
                                      JsonbValueToJsonb(res), 
                                      NULL);
        else
            pg_object_alter_obj_hook(address,
                                     relname, 
                                     namespaceId, 
                                     JsonbValueToJsonb(res), 
                                     NULL);
    }
    else
    {
        Datum   path, new, ret;
        Oid     typinput, typioparam;
        Jsonb   *old = info->orioption;
        char    collstr[16] = {0};

        // call array_in
        getTypeInputInfo(TEXTARRAYOID, &typinput, &typioparam);
        path = OidInputFunctionCall(typinput, 
                                    COLLATION_KEY_PATH, 
                                    typioparam, 
                                    -1);
        // call jsonb_in
        pg_ultostr(collstr, coll);
        getTypeInputInfo(JSONBOID, &typinput, &typioparam);
        new = OidInputFunctionCall(typinput, 
                                   collstr, 
                                   typioparam, 
                                   -1);

        // call jsonb_set to alter old value
        ret = DirectFunctionCall4(jsonb_set,
                                  JsonbPGetDatum(old),
                                  path,
                                  JsonbPGetDatum(new),
                                  BoolGetDatum(true));

        pg_object_alter_obj_hook(address,
                                 relname, 
                                 namespaceId, 
                                 DatumGetJsonbP(ret), 
                                 NULL);
    }
}

void
SetRelationCollation(char *relname, Oid namespaceId, Oid coll, ObjectAddress address)
{
    CreateOrUpdatePgObjectCollationOption(relname, namespaceId, coll, address, PG_OBJECT_RELATION);
}

void
UpdateRelationCollation(char *relname, Oid namespaceId, Oid coll, ObjectAddress address)
{
    CreateOrUpdatePgObjectCollationOption(relname, namespaceId, coll, address, PG_OBJECT_RELATION);
}

void
SetNamespaceCollation(char *namespaceName, Oid namespaceId, Oid coll, ObjectAddress address)
{
    CreateOrUpdatePgObjectCollationOption(namespaceName, namespaceId, coll, address, PG_OBJECT_SCHEMA);   
}

static Oid
GetPgObjectCollationOption(ObjectAddress address)
{
    Oid              coll;
    PgObjectInfo     info;
    JsonbValue	     key;
    JsonbValue      *val = NULL;
    char            *collstr;

    if (!pg_object_get_ori_infor_hook)
        elog(ERROR, "pg_object extension missing!");

    info = pg_object_get_ori_infor_hook(address);

    if (info == NULL || info->orioption == NULL)
        return InvalidOid;
    
    key.type = jbvString;
	key.val.string.val = COLLATION_KEY_VAL;
	key.val.string.len = COLLATION_KEY_LEN;

    val = findJsonbValueFromContainer(&info->orioption->root,
                                      JB_FOBJECT | JB_FARRAY,
							          &key);

    if (val == NULL)
        return InvalidOid;

    assert(val->type == jbvNumeric);

    collstr = DatumGetCString(DirectFunctionCall1(numeric_out,
											      NumericGetDatum(val->val.numeric)));

    coll = pg_strtoint32(collstr);

    return coll;
}

Oid
GetRelationCollation(Relation relation)
{
    ObjectAddress   address;
    ObjectAddressSet(address, RelationRelationId, RelationGetRelid(relation));
    return GetPgObjectCollationOption(address);
}

Oid
GetNamespaceCollation(Oid namespaceId)
{
    Oid             coll;
    ObjectAddress   address;
    
    if (default_collation_oid != InvalidOid)
        return default_collation_oid;

    ObjectAddressSet(address, NamespaceRelationId, namespaceId);

    coll = GetPgObjectCollationOption(address);

    if (OidIsValid(coll))
        return coll;
    
    return ORA_COLL_OID(USING_NLS_COMP);
}
/* End - Req:SRS-SQL-NLSSORT */


/* Begin - ReqID:SRS-SQL-SCHEMA */

#define TABLESPACE_KEY_VAL	"tablespace"
#define TABLESPACE_KEY_LEN	10
#define TABLESPACE_KEY_PATH	"{tablespace}"

static void
CreateOrUpdatePgObjectTablespaceOption(char *name, Oid namespaceId, Oid tablespace, ObjectAddress address, char objectKind)
{
	PgObjectInfo	info;

	if (!pg_object_create_obj_hook ||
		!pg_object_alter_obj_hook ||
		!pg_object_get_ori_infor_hook)
	{
		elog(ERROR, "pg_object extension missing!");
	}

	info = pg_object_get_ori_infor_hook(address);

	if (info == NULL || info->orioption == NULL)
	{
		JsonbValue		   *res;
		JsonbParseState	   *state = NULL;
		JsonbValue			key,
							val;

		key.type = jbvString;
		key.val.string.val = TABLESPACE_KEY_VAL;
		key.val.string.len = TABLESPACE_KEY_LEN; // length of "tablespace";

		val.type = jbvNumeric;
		val.val.numeric = int64_to_numeric(tablespace);

		pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		pushJsonbValue(&state, WJB_KEY, &key);
		pushJsonbValue(&state, WJB_VALUE, &val);
		res = pushJsonbValue(&state, WJB_END_OBJECT, NULL);

		if (info == NULL)
			pg_object_create_obj_hook(name,
									  namespaceId,
									  true,
									  objectKind,
									  address,
									  JsonbValueToJsonb(res),
									  NULL);
		else
			pg_object_alter_obj_hook(address,
									 name,
									 namespaceId,
									 JsonbValueToJsonb(res),
									 NULL);
	}
	else
	{
		Datum	path, new, ret;
		Oid		typinput, typioparam;
		Jsonb  *old = info->orioption;
		char	tbspcstr[16] = {0};

		// call array_in
		getTypeInputInfo(TEXTARRAYOID, &typinput, &typioparam);
		path = OidInputFunctionCall(typinput,
									TABLESPACE_KEY_PATH,
									typioparam,
									-1);
		// call jsonb_in
		pg_ultostr(tbspcstr, tablespace);
		getTypeInputInfo(JSONBOID, &typinput, &typioparam);
		new = OidInputFunctionCall(typinput,
								   tbspcstr,
								   typioparam,
								   -1);

		// call jsonb_set to alter old value
		ret = DirectFunctionCall4(jsonb_set,
								  JsonbPGetDatum(old),
								  path,
								  JsonbPGetDatum(new),
								  BoolGetDatum(true));

		pg_object_alter_obj_hook(address,
								 name,
								 namespaceId,
								 DatumGetJsonbP(ret),
								 NULL);
	}
}

/*
 * Get option->tablespace from pg_object by address.
 */
static Oid
GetPgObjectTablespaceOption(ObjectAddress address)
{
	Oid				tablespace;
	PgObjectInfo	info;
	JsonbValue		key;
	JsonbValue	   *val = NULL;
	char		   *tbspcstr;

	if (!pg_object_get_ori_infor_hook)
		elog(ERROR, "pg_object extension missing!");

	info = pg_object_get_ori_infor_hook(address);

	if (info == NULL || info->orioption == NULL)
		return InvalidOid;

	key.type = jbvString;
	key.val.string.val = TABLESPACE_KEY_VAL;
	key.val.string.len = TABLESPACE_KEY_LEN;

	val = findJsonbValueFromContainer(&info->orioption->root,
									  JB_FOBJECT | JB_FARRAY,
									  &key);

	if (val == NULL)
		return InvalidOid;

	assert(val->type == jbvNumeric);

	tbspcstr = DatumGetCString(DirectFunctionCall1(numeric_out,
												   NumericGetDatum(val->val.numeric)));

	tablespace = pg_strtoint32(tbspcstr);

	return tablespace;
}

/*
 * Set or update option->tablespace in pg_object for schema.
 */
void
SetNamespaceTablespace(char *namespaceName, Oid namespaceId, Oid tablespace, ObjectAddress address)
{
    CreateOrUpdatePgObjectTablespaceOption(namespaceName, namespaceId, tablespace, address, PG_OBJECT_SCHEMA);
}

/*
 * Get schema's tablespace from pg_object->option->tablespace.
 */
Oid
GetNamespaceTablespace(Oid namespaceId, char relpersistence, bool partitioned)
{
	Oid				result;
	ObjectAddress	address;

	/* The temp-table case is handled elsewhere */
	if (relpersistence == RELPERSISTENCE_TEMP)
		return InvalidOid;

	ObjectAddressSet(address, NamespaceRelationId, namespaceId);
	result = GetPgObjectTablespaceOption(address);

	/*
	 * Allow explicit specification of database's default tablespace in
	 * default_tablespace without triggering permissions checks.  Don't allow
	 * specifying that when creating a partitioned table, however, since the
	 * result is confusing.
	 */
	if (result == MyDatabaseTableSpace)
	{
		if (partitioned)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("cannot specify default tablespace for partitioned relations")));
		result = InvalidOid;
	}

	return result;
}

/* End - ReqID:SRS-SQL-SCHEMA */
