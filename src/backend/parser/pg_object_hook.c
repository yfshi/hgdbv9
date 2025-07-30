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
#include "utils/rel.h"

pg_object_create_obj_hook_type      pg_object_create_obj_hook = NULL;
pg_object_alter_obj_hook_type       pg_object_alter_obj_hook = NULL;
pg_object_drop_obj_hook_type        pg_object_drop_obj_hook = NULL;
pg_object_get_ori_infor_hook_type   pg_object_get_ori_infor_hook = NULL;
pg_object_bulid_cache_obj_hook_type pg_object_bulid_relcache = NULL;
pg_object_bulid_cache_obj_hook_type pg_object_bulid_cache_obj_hook = NULL;

/* Begin - ReqID:RSR-SQL-Partition-interval */
#define INTERVAL_KEY_VAL		"INTERVAL"
#define INTERVAL_KEY_LEN		8
/* End - ReqID:RSR-SQL-Partition-interval */

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

/*
 * ReqID:RSR-SQL-Partition-interval
 *
 * GetPgObjectOptionItem - get value string from pg_object by key.
 */
static char *
GetPgObjectOptionItem(ObjectAddress address, char *keyVal, int keyLen, int *retLen)
{
	PgObjectInfo	info;
	JsonbValue		key;
	JsonbValue	   *val = NULL;

	if (!pg_object_get_ori_infor_hook)
		elog(ERROR, "pg_object extension missing!");

	info = pg_object_get_ori_infor_hook(address);
	if (info == NULL || info->orioption == NULL)
		return NULL;

	key.type = jbvString;
	key.val.string.val = keyVal;
	key.val.string.len = keyLen;

	val = findJsonbValueFromContainer(&info->orioption->root,
									  JB_FOBJECT | JB_FARRAY,
									  &key);
	if (val == NULL || val->type != jbvString)
		return NULL;

	if (retLen != NULL)
		*retLen = val->val.string.len;

	return val->val.string.val;
}

/*
 * ReqID:RSR-SQL-Partition-interval
 *
 * SetPgObjectOptionItems - set json items in pg_object.
 */
static void
SetPgObjectOptionItems(ObjectAddress address, char *objname, Oid objnsp, char objkind, char **keystrs, int *keylens, char **valuestrs, int *valuelens, int num)
{
	int				i;
	PgObjectInfo	info;
	Jsonb	*jb;

	if (!pg_object_create_obj_hook ||
		!pg_object_alter_obj_hook  ||
		!pg_object_get_ori_infor_hook)
	{
		elog(ERROR, "pg_object extension missing!");
	}

	info = pg_object_get_ori_infor_hook(address);

	if (info != NULL && info->orioption != NULL)
		jb = info->orioption;
	else
	{
		JsonbValue		   *res;
		JsonbParseState	   *state = NULL;

		pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		res = pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		jb = JsonbValueToJsonb(res);
	}

	for (i = 0; i < num; i++)
	{
		Datum path, new, ret;
		Oid typinput, typioparam;
		StringInfoData keypath;
		StringInfoData vstr;

		initStringInfo(&keypath);
		appendStringInfo(&keypath, "{");
		appendBinaryStringInfo(&keypath, keystrs[i], keylens[i]);
		appendStringInfo(&keypath, "}");

		initStringInfo(&vstr);
		appendStringInfo(&vstr, "\"");
		appendBinaryStringInfo(&vstr, valuestrs[i], valuelens[i]);
		appendStringInfo(&vstr, "\"");

		// call array_in
		getTypeInputInfo(TEXTARRAYOID, &typinput, &typioparam);
		path = OidInputFunctionCall(typinput,
									keypath.data,
									typioparam,
									-1);
		// call jsonb_in
		getTypeInputInfo(JSONBOID, &typinput, &typioparam);
		new = OidInputFunctionCall(typinput,
								   vstr.data,
								   typioparam,
								   -1);

		// call jsonb_set to alter old value
		ret = DirectFunctionCall4(jsonb_set,
								  JsonbPGetDatum(jb),
								  path,
								  JsonbPGetDatum(new),
								  BoolGetDatum(true));
		jb = DatumGetJsonbP(ret);
	}

	if (info == NULL)
		pg_object_create_obj_hook(objname,
								  objnsp,
								  true,
								  objkind,
								  address,
								  jb,
								  NULL);
	else
		pg_object_alter_obj_hook(address,
								 objname,
								 objnsp,
								 jb,
								 NULL);
}

/*
 * ReqID:RSR-SQL-Partition-interval
 *
 * DeletePgObjectOptionItems - delte json items from pg_object.
 */
static void
DeletePgObjectOptionItems(ObjectAddress address, char **keystrs, int num)
{
	int 			i;
	PgObjectInfo	info;
    
	if (!pg_object_create_obj_hook ||
		!pg_object_alter_obj_hook  ||
		!pg_object_get_ori_infor_hook)
	{
		elog(ERROR, "pg_object extension missing!");
	}

	info = pg_object_get_ori_infor_hook(address);

	if (info == NULL || info->orioption == NULL)
    {
		return;
	}

	for (i = 0; i < num; i++)
	{
		Datum	path,
				ret;

		path = CStringGetTextDatum(keystrs[i]);

		// call jsonb_delete
		ret = DirectFunctionCall2(jsonb_delete, JsonbPGetDatum(info->orioption), path);

		pg_object_alter_obj_hook(address,
								 info->oriname,
								 info->orischema,
								 DatumGetJsonbP(ret),
								 NULL);
	}
}

/*
 * ReqID:RSR-SQL-Partition-interval
 *
 * GetRelationInterval
 *		Get value from pg_object by key "INTERVAL".
 */
char *
GetRelationInterval(Oid relid)
{
	ObjectAddress	address;

	ObjectAddressSet(address, RelationRelationId, relid);

	return GetPgObjectOptionItem(address, INTERVAL_KEY_VAL, INTERVAL_KEY_LEN, NULL);
}

/*
 * ReqID:RSR-SQL-Partition-interval
 *
 * SetRelaitonInterval
 *		Update or set value of pg_object by key "INTERVAL".
 */
void
SetRelationInterval(Oid relid, char *relname, Oid namespace, char relkind, char *value)
{
	ObjectAddress	address;
	char	*keys;
	int		keylens;
	char	*values;
	int		valuelens;

	ObjectAddressSet(address, RelationRelationId, relid);

	keys = INTERVAL_KEY_VAL;
	keylens= INTERVAL_KEY_LEN;
	values = value;
	valuelens = strlen(values);

	SetPgObjectOptionItems(address, relname, namespace, relkind,
						   &keys, &keylens, &values, &valuelens, 1);
}

/*
 * ReqID:RSR-SQL-Partition-interval
 *
 * RemovRelationInterval
 *		Delete from pg_object by key "INTERVAL".
 */
void
DeleteRelationInterval(Oid relid)
{
	ObjectAddress address;
	char *keys = INTERVAL_KEY_VAL;

	ObjectAddressSet(address, RelationRelationId, relid);

	DeletePgObjectOptionItems(address, &keys, 1);
}