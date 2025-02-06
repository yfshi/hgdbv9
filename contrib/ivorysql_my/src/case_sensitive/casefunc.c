/*-------------------------------------------------------------------------
 *
 * casefuncs.c
 *	  MySQL case routines
 *
 * Portions Copyright (c) 2024-, Highgo MySQL Compatible Development Group
 *
 *
 * IDENTIFICATION
 *	  contrib/ivorysql_my/src/case_sensitive/casefunc.c
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
#include "fmgr.h"

#include "access/xact.h"
#include "access/table.h"
#include "access/heapam.h"
#include "catalog/pg_object.h"
#include "catalog/indexing.h"
#include "catalog/namespace.h"
#include "nodes/parsenodes.h"
#include "nodes/value.h"
#include "utils/builtins.h"
#include "utils/catcache.h"
#include "utils/syscache.h"
#include "utils/rel.h"
#include "utils/lsyscache.h"
#include "utils/jsonb.h"

static text *JsonbValueAsText(JsonbValue *v);

PG_FUNCTION_INFO_V1(myscaselower);

Datum
myscaselower(PG_FUNCTION_ARGS)
{
	Oid			refclassid = PG_GETARG_OID(0);
	Oid			objid = PG_GETARG_OID(1);
	int			subid = PG_GETARG_INT32(2);
	HeapTuple	tuple;
	int			ret = 3;

	tuple = SearchSysCache3(OBJECTREFCLASSIDOIDSUBID,
							ObjectIdGetDatum(refclassid),
							ObjectIdGetDatum(objid),
							Int32GetDatum(subid));

	if (HeapTupleIsValid(tuple))
	{
		Datum		val;
		bool		isNull;
		bool		isNullc;
		char		*str = "lower_case_table_names";
		text		*txt;
		char		*lowerval = NULL;
		char		compatible_mode;

		compatible_mode = DatumGetChar(SysCacheGetAttr(OBJECTREFCLASSIDOIDSUBID, tuple,
									   Anum_pg_object_compatible_mode,
									   &isNullc));

		val = SysCacheGetAttr(OBJECTREFCLASSIDOIDSUBID, tuple,
							  Anum_pg_object_option,
							  &isNull);
		if (!isNull && compatible_mode == 'm')
		{
			Jsonb		   *jb = DatumGetJsonbP(val);
			text		   *key = DatumGetTextPP(cstring_to_text(str));
			JsonbValue	   *v;
			JsonbValue		vbuf;

			v = getKeyJsonValueFromContainer(&jb->root,
											 VARDATA_ANY(key),
											 VARSIZE_ANY_EXHDR(key),
											 &vbuf);
			if (v != NULL && v->type != jbvNull)
			{
				txt = JsonbValueAsText(v);
				lowerval = text_to_cstring(txt);
				if (lowerval != NULL && strlen(lowerval) == 1)
					ret = pg_strtoint32(lowerval);
			}
		}

		ReleaseSysCache(tuple);
	}

	PG_RETURN_INT32(ret);
}

/*
 * Return the text representation of the given JsonbValue.
 */
static text *
JsonbValueAsText(JsonbValue *v)
{
	switch (v->type)
	{
		case jbvNull:
			return NULL;

		case jbvBool:
			return v->val.boolean ?
				cstring_to_text_with_len("true", 4) :
				cstring_to_text_with_len("false", 5);

		case jbvString:
			return cstring_to_text_with_len(v->val.string.val,
											v->val.string.len);

		case jbvNumeric:
			{
				Datum		cstr;

				cstr = DirectFunctionCall1(numeric_out,
										   PointerGetDatum(v->val.numeric));

				return cstring_to_text(DatumGetCString(cstr));
			}

		case jbvBinary:
			{
				StringInfoData jtext;

				initStringInfo(&jtext);
				(void) JsonbToCString(&jtext, v->val.binary.data,
									  v->val.binary.len);

				return cstring_to_text_with_len(jtext.data, jtext.len);
			}

		default:
			elog(ERROR, "unrecognized jsonb type: %d", (int) v->type);
			return NULL;
	}
}

