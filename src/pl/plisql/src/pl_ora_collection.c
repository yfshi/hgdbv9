/*-------------------------------------------------------------------------
  *
  * File: pl_ora_collection.c
  *
  * Abstract:
  * 	 Handler for the PLiSQL collection
  *
  * ReqID:SRS-PLSQL-Collection
  *
  *
  * Copyright:
  * Copyright (c) 2009-2025, HighGo Software Co.,Ltd. All rights reserved.
  *
  * IDENTIFICATION
  *    src/pl/plisql/src/pl_ora_collection.c
  *
  *-------------------------------------------------------------------------
  */
#include "postgres.h"
#include "plisql.h"
#include "pl_udt_type.h"
#include "pl_package.h"
#include "catalog/pg_package.h"
#include "catalog/pg_type.h"
#include "utils/memutils.h"
#include "utils/builtins.h"
#include "utils/syscache.h"
#include "parser/parse_type.h"
#include "nodes/nodeFuncs.h"
#include "nodes/makefuncs.h"
#include "utils/lsyscache.h"

/*
 * build user defined collection type.
 */
PLiSQL_type *
plisql_build_ora_collection_type(char *refname, int lineno,
							  Oid typoid,
							  char typtype,
							  PLiSQL_type *keytype,
							  PLiSQL_type *valuetype,
							  int varraylimit,
							  bool notnull)
{
	PLiSQL_datum_udt_type *datum_type;
	PLiSQL_ora_collection_type *ora_collect;
	Oid		pkgoid;

	if (keytype->typoid != INT8OID &&
		keytype->typoid != INT4OID &&
		keytype->typoid != INT2OID &&
		keytype->typoid != TEXTOID &&
		keytype->typoid != VARCHAROID &&
		keytype->typoid != ORAVARCHARBYTEOID &&
		keytype->typoid != ORAVARCHARCHAROID)
		ereport(ERROR,
			(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
			 errmsg("Implementation restriction: unsupported table index type")));

	if (ISCURSOROID(valuetype->typoid))
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				errmsg("Index Tables of Cursor Variables are disallowed")));

	if (valuetype->typoid == RECORDOID &&
		valuetype->ttype == PLISQL_TTYPE_REC &&
		valuetype->typtype == 'p')
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				errmsg("pg record type is disallowed")));

	ora_collect = (PLiSQL_ora_collection_type *) palloc0(sizeof(PLiSQL_ora_collection_type));

	pkgoid = (plisql_compile_packageitem == NULL ? InvalidOid : plisql_compile_packageitem->source.fn_oid);
	if (pkgoid != InvalidOid)
	{
		char		typname[(NAMEDATALEN * 2) + 100];
		HeapTuple 	pkgTup;

		/*
		 * Lookup the pg_package tuple by Oid
		 */
		pkgTup = SearchSysCache1(PKGOID, ObjectIdGetDatum(pkgoid));
		if (!HeapTupleIsValid(pkgTup))
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_OBJECT),
					 errmsg("package with OID %u does not exist", pkgoid)));

		snprintf(typname, sizeof(typname), "%s.%s.%s",
				get_namespace_name(((Form_pg_package) GETSTRUCT(pkgTup))->pkgnamespace),
				NameStr(((Form_pg_package) GETSTRUCT(pkgTup))->pkgname),
				refname);

		ReleaseSysCache(pkgTup);

		ora_collect->type.typname = pstrdup(typname);
	}
	else
		ora_collect->type.typname = pstrdup(refname);


	ora_collect->type.typoid = typoid;
	ora_collect->type.ttype = PLISQL_TTYPE_COLLECT_TYPE;
	ora_collect->type.typlen = -1;
	ora_collect->type.typbyval = false;
	ora_collect->type.typtype = typtype;
	ora_collect->type.collation = 0;
	ora_collect->type.typisarray = false;
	ora_collect->type.atttypmod = -1;
	ora_collect->type.notnull = notnull;
	ora_collect->type.pctrowtypname = NULL;
	ora_collect->type.origtypname = NULL;
	ora_collect->type.tcache = NULL;
	ora_collect->type.tupdesc_id = 0;
	ora_collect->typkey = keytype;
	ora_collect->typdatatype = valuetype;
	ora_collect->limit = varraylimit;

	/* build a PLiSQL_datum_udt_type, and add it to plisql_Datums */
	datum_type = palloc0(sizeof(PLiSQL_datum_udt_type));

	datum_type->dtype = PLISQL_DTYPE_TYPE;
	datum_type->udt_type = (PLiSQL_type *) ora_collect;
	datum_type->pkgoid = pkgoid;

	/* add udt datum */
	plisql_adddatum((PLiSQL_datum *) datum_type);
	plisql_ns_additem(PLISQL_NSTYPE_TYPE,
					datum_type->dno, refname);
	/* set atttypmod to dno */
	ora_collect->type.atttypmod = datum_type->dno;

	return (PLiSQL_type *) datum_type;
}

/*
 *  transform struct plisql_type to struct collectinfo.
 */
Datum
plisql_type_to_collectinfo(PLiSQL_type *type)
{
	Datum	collectinfo = (Datum) 0;
	Datum  	val_type = (Datum) 0;
	Datum	key_type = (Datum) 0;
	int 	limit = -1;
	bool 	datanotnull = false;

	if (IS_PLSQL_COLLECTYPE(type->typtype))
	{
		PLiSQL_ora_collection_type *collectype = (PLiSQL_ora_collection_type *)type;

		key_type = plisql_type_to_collectinfo(collectype->typkey);

		val_type = plisql_type_to_collectinfo(collectype->typdatatype);

		limit = collectype->limit;
		datanotnull = collectype->type.notnull;
	}

	collectinfo = collection_funcs.CreateCollectInfo(type->typname,
								type->typoid,
								type->atttypmod,
								type->typtype,
								type->typbyval,
								type->typlen,
								key_type,
								val_type,
								limit,
								datanotnull);

	return collectinfo;
}

/*
 * do check for the collection variable assignment
 */
void
exec_check_assign_collect_value(PLiSQL_var *target,
								Datum value,
								Oid valtype,
								int32 valtypmod)
{
	TypeName	*target_tname;
	TypeName	*value_tname;
	PkgType 	*target_pkgtype;
	PkgType 	*value_pkgtype;
	Oid			target_pkgoid = InvalidOid;
	Oid			value_pkgoid = InvalidOid;
	int32 		target_typmod;
	int32		value_typmod;
	struct varlena *vl;

	if (!IS_COLLECTOID(valtype))
		ereport(ERROR,
			(errcode(ERRCODE_CANNOT_COERCE),
			errmsg("expression is of wrong type")));

	/*
	 * Check the datatype of the target and the value.
	 * If they are collection type in packages, then
	 * compare the package oid and collection type name.
	 * If the aren't, compare the typmod.
	 */
	target_tname = typeStringToTypeName(target->datatype->typname);
	target_pkgtype = LookupPkgTypeByTypename(target_tname->names, false);
	if (target_pkgtype != NULL)
	{
		target_pkgoid = target_pkgtype->pkgoid;
		target_typmod = ((PLiSQL_type *)(target_pkgtype->value))->atttypmod;
		pfree(target_pkgtype);
	}

	if (collection_funcs.loadstate != LOADED)
		elog(ERROR, "collection functions have not been loaded");

	vl = (struct varlena *) DatumGetPointer(value);
	if (!VARATT_IS_EXTERNAL_COLLECT(vl))
		ereport(ERROR,
			(errcode(ERRCODE_CANNOT_COERCE),
			errmsg("expression is of wrong type")));

	value_tname = typeStringToTypeName(DatumGetCString(collection_funcs.GetCollectypeName(value)));
	value_pkgtype = LookupPkgTypeByTypename(value_tname->names, false);
	if (value_pkgtype != NULL)
	{
		value_pkgoid = value_pkgtype->pkgoid;
		value_typmod = ((PLiSQL_type *)(value_pkgtype->value))->atttypmod;
		pfree(value_pkgtype);
	}

	PG_TRY();
	{
		if (target_pkgoid != InvalidOid && value_pkgoid != InvalidOid)
		{
			if (target_pkgoid != value_pkgoid ||
				(target_pkgoid == value_pkgoid &&
				target_typmod != value_typmod))
				ereport(ERROR,
					(errcode(ERRCODE_CANNOT_COERCE),
					errmsg("expression is of wrong type")));
		}
		else if (target_pkgoid != InvalidOid || value_pkgoid != InvalidOid)
			ereport(ERROR,
				(errcode(ERRCODE_CANNOT_COERCE),
				errmsg("expression is of wrong type")));
		else
		{
			target_typmod = target->datatype->atttypmod;
			value_typmod = DatumGetInt32(collection_funcs.GetCollectypeMode(value));

			if (target_typmod != value_typmod)
				ereport(ERROR,
					(errcode(ERRCODE_CANNOT_COERCE),
					errmsg("expression is of wrong type")));
		}
	}
	PG_FINALLY();
	{
		pfree(target_tname);
		pfree(value_tname);
	}
	PG_END_TRY();

	return;
}
