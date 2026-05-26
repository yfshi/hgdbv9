/*-------------------------------------------------------------------------
 *
 * File:collection.c
 *
 * Abstract:
 *		Definitions for the collection type
 *
 * ReqID:SRS-PLSQL-Collection
 *
 *
 * Copyright:
 * Copyright (c) 2009-2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  contrib/ivorysql_ora/src/collection/collection.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include <ctype.h>
#include "../tstl2cl/include/c_map.h"
#include "catalog/pg_collation.h"
#include "executor/execExpr.h"
#include "executor/spi.h"
#include <limits.h>
#include "parser/parse_type.h"	/* ReqID:SRS-PLSQL-BULKCOLLECT */
#include "parser/scansup.h"
#include "utils/datum.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"	/* ReqID:SRS-PLSQL-BULKCOLLECT */
#include "utils/typcache.h"
#include "../include/collection.h"
#include "utils/builtins.h"
#include "utils/ora_compatible.h"
#include "utils/varlena.h"
#include "libpq/pqformat.h"
#include "funcapi.h"
#include "../pl/plisql/src/plisql.h"
#include "../pl/plisql/src/pl_udt_type.h"
#include "common/int.h"	/* BUG#7715 */

static CollectTypeData *create_collectype_data(CollectTypeInfo *type, int32 limit_idx, int max_index);
static int32 GetCollecType(char typtype);
static CollectTypeData *copy_collectype_data(CollectTypeData *src_data);
static CollectTypeInfo *copy_collectype_info(CollectTypeInfo *src_type);
static Datum pl_type_to_collectinfo(PLiSQL_type *type);
static void copy_collectype_elements(CollectTypeData *src, CollectTypeData *dst);
static void insert_collectype_data(CollectTypeData *collect_data,
				Datum key, Datum value, bool null_value, bool null_check,
				bool placeholder);
static void insert_collectype_element(CollectTypeData *collect_data, Datum key, Datum value);
static void free_collectype_value(CollecTypeValue *elem, CollectTypeInfo *type);
static void free_collectype_data(CollectTypeData *collect_data, bool src_isnull);
static void reset_collectype_data(CollectTypeData *collect_data, bool src_isnull);
static void update_varray_memory(CollectTypeData *collect_data);
static HeapTupleHeader set_rec_field_value(HeapTupleHeader value,
					PLiSQL_type **pl_datatype,
					List *cur_fieldnames,
					int rec_len,
					int nth_field,
					int nsubscripts,
					Datum *subscriptvals,
					int *nth,
					List **all_fieldnames,
					Datum assign_value,
					bool assign_value_isnull);
static void get_collectype_typeinfo(CollectTypeInfo *type,
				Oid *keyoid,
				int32 *keymod,
				Oid *valueoid,
				int32 *valuemod,
				bool *notnull);
static bool collection_value_compare(Datum value1, Datum value2);
static bool collection_element_compare(Datum value1, Datum value2, Oid typoid);
static Datum init_heaptuple(Oid typoid,
						int32 typmod,
						PLiSQL_type *pl_datatype);
/* Begin - ReqID:SRS-PLSQL-BULKCOLLECT */
static Datum get_collect_value2(Datum datum,
				int nsubscripts,
				Datum *subscriptvals,
				List **fieldnames,
				void *top_datatype);
static void check_bulkcollect_target_number_match(int target_number, int source_number);
static bool bulk_collect_into_receive(TupleTableSlot *slot, DestReceiver *self);
static void bulk_collect_into_startup(DestReceiver *self, int operation, TupleDesc typeinfo);
static void bulk_collect_into_shutdown(DestReceiver *self);
static void bulk_collect_into_destroy(DestReceiver *self);
static Datum datum_type_cast(Datum value,
				Oid valtype, int32 valtypmod,
				Oid reqtype, int32 reqtypmod);
/* End - ReqID:SRS-PLSQL-BULKCOLLECT */
/* Begin - BUG#Z7715 */
static void check_extend(CollectTypeData *collect_data, int extend_num);
/* End - BUG#Z7715 */

PG_FUNCTION_INFO_V1(collection_init);
PG_FUNCTION_INFO_V1(collection_first);
PG_FUNCTION_INFO_V1(collection_last);
PG_FUNCTION_INFO_V1(collection_count);
PG_FUNCTION_INFO_V1(collection_limit);
PG_FUNCTION_INFO_V1(collection_prior);
PG_FUNCTION_INFO_V1(collection_next);
PG_FUNCTION_INFO_V1(collection_exists);
PG_FUNCTION_INFO_V1(collection_delete);
PG_FUNCTION_INFO_V1(collection_delete1);
PG_FUNCTION_INFO_V1(collection_delete2);
PG_FUNCTION_INFO_V1(collection_trim);
PG_FUNCTION_INFO_V1(collection_trim_n);
PG_FUNCTION_INFO_V1(collection_extend);
PG_FUNCTION_INFO_V1(collection_extend_n);
PG_FUNCTION_INFO_V1(collection_extend_ni);
PG_FUNCTION_INFO_V1(collection_constructor);
PG_FUNCTION_INFO_V1(nested_table_eq);
PG_FUNCTION_INFO_V1(nested_table_ne);

/*
 * init collection function which will be used in plisql
 */
Datum
collection_init(PG_FUNCTION_ARGS)
{
	collection_init_internal();

	PG_RETURN_VOID();
}

/*
 * init collection function which will be used
 * in plisql
 */
void
collection_init_internal(void)
{
	collection_funcs.InitCollectypeVar = init_collectype_var;
	collection_funcs.CreateCollectInfo = create_collecttype_info;
	collection_funcs.CopyCollectypeDatum = copy_collectype_datum;
	collection_funcs.CollectypeElementRef = collectype_element_ref;
	collection_funcs.CollectypeCastCheck = collectype_cast_Check;
	collection_funcs.GetCollectypeName = get_collectype_name;
	collection_funcs.GetCollectypeMode = get_collectype_mode;
	collection_funcs.SetCollectValue = set_collect_value;
	collection_funcs.GetCollectypeMcxt = get_collectype_mcxt;
	/* Begin - ReqID:SRS-PLSQL-BULKCOLLECT */
	collection_funcs.InitBulkCollectData = init_bulk_collect_data;
	collection_funcs.InitBulkCollectParams = init_bulk_collect_params;
	collection_funcs.SetBulkCollectDestReceiverParams = set_bulk_collect_destreceiver_params;
	collection_funcs.CreateBulkCollectReceiver = create_bulk_collect_receiver;
	/* End - ReqID:SRS-PLSQL-BULKCOLLECT */
	/* Begin - ReqID:SRS-PLSQL-FORALL */
	collection_funcs.SetBulkCollectDestReceiverInForall = set_bulk_collect_destreceiver_inforall;
	collection_funcs.GetCollectAllValues = get_collect_all_values;
	collection_funcs.GetCollectAllKeyValues = get_collect_all_key_values;
	/* End - ReqID:SRS-PLSQL-FORALL */

	collection_funcs.loadstate = LOADED;
}


/*
 * init a collection variable
 */
Datum
init_collectype_var(Datum collectype_info)
{
	CollectTypeInfo *typinfo = (CollectTypeInfo *)DatumGetPointer(collectype_info);

	return PointerGetDatum(create_collectype_data(typinfo, typinfo->limit, 0));
}

/*
 * create a CollectTypeData to store a collection
 */
static CollectTypeData *
create_collectype_data(CollectTypeInfo *type, int32 limit_idx, int max_index)
{
	Datum			datum;
	char 			typtype;
	MemoryContext	old;
	Oid				key_oid;
	CollectTypeData		*data = (CollectTypeData *) palloc0(sizeof(CollectTypeData));

	SET_VARTAG_EXTERNAL(data, VARTAG_COLLECTYPE);
	data->va_data_len = sizeof(CollectTypeData);

	data->limit_idx = limit_idx;
	data->type = type;

	data->valCxt = AllocSetContextCreate(CurrentMemoryContext,
										"CollecTypeValue",
										ALLOCSET_DEFAULT_MINSIZE,
										ALLOCSET_DEFAULT_INITSIZE,
										ALLOCSET_DEFAULT_MAXSIZE);

	typtype = type->typtype;
	if (!IS_PLSQL_COLLECTYPE(typtype))
		elog(ERROR, "can't create collection type for the data typtype %c", typtype);

	data->collect_typtype = GetCollecType(typtype);
	key_oid = type->typkey->typoid;

	old = MemoryContextSwitchTo(data->valCxt);

	/* initiate the collection value */
	if (!IS_PLSQL_VARRAY(typtype))
	{
		c_map 	*map = palloc(sizeof(c_map));

		if (key_oid == INT4OID)
			c_map_create(map, collectype_int_comparer);
		else
			c_map_create(map, collectype_varchar_comparer);

		datum = PointerGetDatum(map);
	}
	else
	{
		CollectVarrayValue *tmp;

		tmp = (CollectVarrayValue *) palloc(sizeof(CollectVarrayValue));

		tmp->current_used = 0;
		tmp->total_index = CALC_VARRAY_TOTAL_INDEX(tmp->current_used, limit_idx);

		tmp->value = (Datum*) palloc(tmp->total_index * sizeof(Datum));

		datum = PointerGetDatum(tmp);
	}

	data->values = datum;
	data->max_index = max_index;

	MemoryContextSwitchTo(old);

	return data;
}

/*
 * create CollectTypeInfo for the collection type
 */
Datum
create_collecttype_info(const char *typname,
				   Oid typoid,
				   int32 typmod,
				   char typtype,
				   bool typbyval,
				   int typlen,
				   Datum typkey,
				   Datum typdatatype,
				   int limit,
				   bool datanotnull)
{
	CollectInfo collect = palloc(sizeof(CollectTypeInfo));

	collect->typname = pstrdup(typname);
	collect->typoid = typoid;
	collect->typmod = typmod;
	collect->typtype = typtype;
	collect->typbyval = typbyval;
	collect->typlen = typlen;
	collect->typkey = (CollectTypeInfo *)DatumGetPointer(typkey);
	collect->typdatatype = (CollectTypeInfo *)DatumGetPointer(typdatatype);
	collect->limit = limit;
	collect->datanotnull = datanotnull;

	return PointerGetDatum(collect);
}

/*
 *  transform struct plisql_type to struct collectinfo.
 */
static Datum
pl_type_to_collectinfo(PLiSQL_type *type)
{
	Datum	collectinfo = (Datum) 0;
	Datum  	val_type = (Datum) 0;
	Datum	key_type = (Datum) 0;
	int 	limit = -1;
	bool 	datanotnull = false;

	if (IS_PLSQL_COLLECTYPE(type->typtype))
	{
		PLiSQL_ora_collection_type *collectype = (PLiSQL_ora_collection_type *)type;

		key_type = pl_type_to_collectinfo(collectype->typkey);

		val_type = pl_type_to_collectinfo(collectype->typdatatype);

		limit = collectype->limit;
		datanotnull = collectype->type.notnull;
	}

	collectinfo = create_collecttype_info(type->typname,
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
 * get the type of collection
 */
static int32
GetCollecType(char typtype)
{
	switch (typtype)
	{
		case 'n':
			return NESTED_TABLE;
		case 'a':
			return ASSOCIATIVE_ARRAY;
		case 'v':
			return VARRAY;
		default:
			elog(ERROR, "the data typtype %c of collection type is not support", typtype);
			return -1;
	}
};

/*
 * compare two keys that datatype are int
 */
int
collectype_int_comparer(void *a, void *b)
{
	int a_int, b_int;
	CollecTypeKey *a_key, *b_key;

	a_key = (CollecTypeKey *) a;
	b_key = (CollecTypeKey *) b;
	a_int = DatumGetInt32(a_key->value);
	b_int = DatumGetInt32(b_key->value);

	/* return value should not be result of a expression */
	if (a_int > b_int)
		return 1;
	else if (a_int < b_int)
		return -1;
	else
		return 0;
}

/*
 * compare two keys that datatype are varchar
 */
int
collectype_varchar_comparer(void *a, void *b)
{
	int				result;
	Oid				typoutput;
	bool			typIsVarlena;
	char 			*a_char, *b_char;
	CollecTypeKey	*a_key, *b_key;

	a_key = (CollecTypeKey *) a;
	b_key = (CollecTypeKey *) b;

	getTypeOutputInfo(VARCHAROID, &typoutput, &typIsVarlena);

	a_char = OidOutputFunctionCall(typoutput, a_key->value);
	b_char = OidOutputFunctionCall(typoutput, b_key->value);

	Assert(a_char != NULL && b_char != NULL);

	/*
	 * The initialization parameters NLS_SORT and NLS_COMP determine
	 * the storage order of string indexes of an associative array.
	 */
	result = varstr_cmp(a_char, strlen(a_char),
						b_char, strlen(b_char),
						nls_sort_oid);

	return result;
}

/*
 * compare two keys that datatype are keyoid
 */
int
collectype_key_compare(Oid keyoid, void *key1, void *key2)
{
	int	result;

	if (keyoid== INT4OID ||
		keyoid== INT8OID ||
		keyoid== INT2OID)
		result = collectype_int_comparer(key1, key2);
	else
		result = collectype_varchar_comparer(key1, key2);

	return result;
}

/*
 * copy_collectype_datum
 *	  this function is used to expland the CopyDatum to copy CollectType datum.
 *     deep copy the CollectType datum.
 */
Datum
copy_collectype_datum(Datum src_value)
{
	CollectType	src_data;
	Datum		result;

	src_data = (CollectType)DatumGetPointer(src_value);

	if (src_data == NULL)
		return (Datum) 0;

	result = PointerGetDatum(copy_collectype_data(src_data));

	return result;
}

/*
 * copy the CollectTypeData data.
 */
static CollectTypeData *
copy_collectype_data(CollectTypeData *src_data)
{
	CollectTypeData		*dst_data;
	CollectTypeInfo		*dst_collect_type;

	/* check max limit for type VARRAY */
	if (IS_VARRAY(src_data))
	{
		if (src_data->max_index > src_data->limit_idx ||
			src_data->limit_idx <= 0)
			ereport(ERROR,
				(errcode(ERRCODE_SUBSCRIPT_OUTSIDE_LIMIT),
				errmsg("subscript outside limit")));
	}

	dst_collect_type = copy_collectype_info(src_data->type);

	/* construct value header */
	dst_data = create_collectype_data(dst_collect_type,
							src_data->limit_idx,
							src_data->max_index);

	copy_collectype_elements(src_data, dst_data);

	return dst_data;
}

/*
 * copy the CollectTypeInfo.
 */
static CollectTypeInfo *
copy_collectype_info(CollectTypeInfo *src_type)
{
	CollectTypeInfo* dst_type;

	dst_type = palloc(sizeof(CollectTypeInfo));

	dst_type->typname = pstrdup(src_type->typname);
	dst_type->typoid = src_type->typoid;
	dst_type->typmod = src_type->typmod;
	dst_type->typtype = src_type->typtype;
	dst_type->typbyval = src_type->typbyval;
	dst_type->typlen = src_type->typlen;
	if (IS_PLSQL_COLLECTYPE(src_type->typtype))
	{
		dst_type->typkey = copy_collectype_info(src_type->typkey);
		dst_type->typdatatype = copy_collectype_info(src_type->typdatatype);
	}
	else
	{
		dst_type->typkey = NULL;
		dst_type->typdatatype = NULL;
	}

	dst_type->limit = src_type->limit;
	dst_type->datanotnull = src_type->datanotnull;

	return dst_type;
}

/*
 * copy the elements of source CollectTypeData
 */
static void
copy_collectype_elements(CollectTypeData *src, CollectTypeData *dst)
{
	MemoryContext	old;
	CollectTypeInfo	*src_type = src->type;
	CollectTypeInfo	*key_type = src_type->typkey;
	CollectTypeInfo	*val_type = src_type->typdatatype;

	/* store element values in a seperately context */
	old = MemoryContextSwitchTo(dst->valCxt);

	if (!IS_VARRAY(src))
	{
		c_map			*mapsrc;
		c_iterator 		iter;
		c_iterator 		end;

		mapsrc = (c_map *) DatumGetPointer(src->values);

		iter = c_map_begin(mapsrc);
		end = c_map_end(mapsrc);

		/* loop each element */
		for(; !ITER_EQUAL(iter, end); ITER_INC(iter))
		{
			Datum 	dst_value = (Datum) 0;

			CollecTypeKey 		*src_key;
			CollecTypeValue		*src_value;

			src_key = (CollecTypeKey *) GET_COLLECT_ELEMENT_KEY(iter);
			src_value = (CollecTypeValue *) GET_COLLECT_ELEMENT_VALUE(iter);

			if (!src_value->isnull)
			{
				if (val_type->typoid == ORARECORDOID)
					dst_value = rowDatumCopy(src_value->value, src_value->isnull);
				else
					dst_value = datumCopy(src_value->value,
										val_type->typbyval, val_type->typlen);
			}

			insert_collectype_data(dst,
					datumCopy(src_key->value,
							   key_type->typbyval, key_type->typlen),
					dst_value,
					src_value->isnull,
					false,
					src_value->placeholder);
		}
	}
	else
	{
		int				idx;
		CollectVarrayValue	*varray_src;
		CollectVarrayValue	*varray_dst;

		varray_src = (CollectVarrayValue*) (src->values);
		varray_dst = (CollectVarrayValue*) (dst->values);

		/* init dest varray value */
		varray_dst->total_index = varray_src->total_index;
		varray_dst->current_used = varray_src->current_used;
		varray_dst->value = (Datum*) palloc0(varray_dst->total_index * sizeof(Datum));

		/* copy elements */
		for (idx = 0; idx < varray_src->current_used; idx++)
		{
			CollecTypeValue 	*src_value;
			CollecTypeValue 	*dst_value;

			src_value =
				(CollecTypeValue*) DatumGetPointer(varray_src->value[idx]);

			/* copy value */
			dst_value = (CollecTypeValue*) palloc0(sizeof(CollecTypeValue));
			dst_value->value = (Datum) 0;

			if (!src_value->isnull)
			{
				if (val_type->typoid == ORARECORDOID)
					dst_value->value = rowDatumCopy(src_value->value, src_value->isnull);
				else
					dst_value->value = datumCopy(src_value->value,
												 val_type->typbyval,
												 val_type->typlen);
			}

			dst_value->isnull = src_value->isnull;

			/* check the NOT NULL constraint of dest collect_data */
			if (dst->type->datanotnull && dst_value->isnull)
				ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					errmsg("conflicting NOT NULL constraints")));

			varray_dst->value[idx] = PointerGetDatum(dst_value);
		}
	}

	MemoryContextSwitchTo(old);
}

/*
 * insert the key and value to CollectTypeData
 */
static void
insert_collectype_data(CollectTypeData *collect_data,
				Datum key, Datum value, bool null_value, bool null_check,
				bool placeholder)
{
	CollecTypeKey   	*p_key;
	CollecTypeValue 	*p_value;
	MemoryContext	 	old;

	old = MemoryContextSwitchTo(collect_data->valCxt);

	if (null_check && null_value && collect_data->type->datanotnull)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				errmsg("conflicting NOT NULL constraints")));

	p_key = (CollecTypeKey *) palloc(sizeof(CollecTypeKey));
	p_key->value = key;
	p_value = (CollecTypeValue *) palloc0(sizeof(CollecTypeValue));

	p_value->isnull = null_value;
	p_value->value = value;
	p_value->placeholder = placeholder;

	insert_collectype_element(collect_data,
					PointerGetDatum(p_key),
					PointerGetDatum(p_value));

	MemoryContextSwitchTo(old);
}

/*
 * insert (key, value) pair to collection.
 */
static void
insert_collectype_element(CollectTypeData *collect_data, Datum key, Datum value)
{
	CollectTypeInfo *collect_type = collect_data->type;
	Datum 			collect_values = collect_data->values;

	if (!IS_PLSQL_VARRAY(collect_type->typtype))
	{
		c_map		*map;
		c_pair		*pair;
		c_iterator	iter,
					end;

		map = (c_map *) DatumGetPointer(collect_values);
		if (IS_PLSQL_NESTED_TABLE(collect_type->typtype) &&
			(((CollecTypeKey *)key)->value <= 0 ||
			((CollecTypeKey *)key)->value > collect_data->max_index))
			ereport(ERROR,
				(errcode(ERRCODE_SUBSCRIPT_OUTSIDE_LIMIT),
				errmsg("subscript outside limit")));

		pair = palloc(sizeof(c_pair));

		pair->first = (void *) DatumGetPointer(key);
		pair->second = (void *) DatumGetPointer(value);
		iter = c_map_find(map, DatumGetPointer(key));
		end = c_map_end(map);

		/* if key exists, then delete the <key, value> */
		if(!ITER_EQUAL(end, iter))
		{
			CollecTypeValue		*elem;
			CollecTypeKey		*old_key;

			elem = (CollecTypeValue*) GET_COLLECT_ELEMENT_VALUE(iter);
			old_key = (CollecTypeKey*) GET_COLLECT_ELEMENT_KEY(iter);

			free_collectype_value(elem, collect_type->typdatatype);
			pfree(old_key);
			c_map_erase(map, iter);
		}

		c_map_insert(map, pair);
	}
	else
	{
		int32			key_val;
		CollectVarrayValue	*varray;
		int32			limit_index;

		varray = (CollectVarrayValue*) DatumGetPointer(collect_values);
		key_val = DatumGetInt32(((CollecTypeKey*) DatumGetPointer(key))->value);
		limit_index = collect_data->limit_idx;

		if (key_val > limit_index ||
			key_val > collect_data->max_index ||
			key_val <= 0)
			ereport(ERROR,
				(errcode(ERRCODE_SUBSCRIPT_OUTSIDE_LIMIT),
				errmsg("subscript outside limit")));

		if (key_val <= varray->current_used)
		{
			CollecTypeValue	*elem;

			/* replace element */
			elem = (CollecTypeValue*) DatumGetPointer(varray->value[key_val - 1]);
			free_collectype_value(elem, collect_type->typdatatype);

			varray->value[key_val - 1] = value;
		}
		else
		{
			update_varray_memory(collect_data);

			varray->current_used++;
			varray->value[varray->current_used - 1] = value;
		}
	}
}

/*
 * free the collection elemnet value.
 */
static void
free_collectype_value(CollecTypeValue *elem, CollectTypeInfo *type)
{
	Assert(elem != NULL);
	if (!elem->isnull)
	{
		if (IS_PLSQL_COLLECTYPE(type->typtype))
			free_collectype_data((CollectTypeData *)elem->value, elem->isnull);
		else if (!type->typbyval)
			pfree(DatumGetPointer(elem->value));
	}
	pfree(elem);
}

/*
 * free the collection value.
 */
static void
free_collectype_data(CollectTypeData *collect_data, bool src_isnull)
{
	reset_collectype_data(collect_data, src_isnull);

	if (IS_VARRAY(collect_data))
	{
		CollectVarrayValue* values = (CollectVarrayValue*)
						DatumGetPointer(collect_data->values);

		pfree(values->value);
		pfree(values);
	}
	else
	{
		c_map	*map = (c_map *)DatumGetPointer(collect_data->values);

		c_map_destroy(map);
		pfree(map);
	}

	MemoryContextDelete(collect_data->valCxt);
	pfree(collect_data);
}


/*
 * reset the collection value.
 */
static void
reset_collectype_data(CollectTypeData *collect_data, bool src_isnull)
{
	CollectTypeInfo		*datatype;

	Assert(collect_data != NULL);

	datatype = collect_data->type->typdatatype;
	if (src_isnull)
		return;

	if (!IS_VARRAY(collect_data))
	{
		c_map				*map;
		c_iterator			iter;
		c_iterator			end;
		CollecTypeValue		*elem;
		CollecTypeKey		*key;

		map = (c_map*) DatumGetPointer(collect_data->values);

		if (c_map_empty(map))
			return;

		iter = c_map_begin(map);
		end = c_map_end(map);

		/* free every element */
		while (!ITER_EQUAL(iter, end))
		{
			elem = (CollecTypeValue *) GET_COLLECT_ELEMENT_VALUE(iter);
			key = (CollecTypeKey *) GET_COLLECT_ELEMENT_KEY(iter);

			free_collectype_value(elem, datatype);

			pfree(key);
			pfree(ITER_REF(iter));

			ITER_INC(iter);
		}

		c_map_clear(map);
	}
	else
	{
		int					idx;
		CollectVarrayValue	*varray;
		CollecTypeValue		*elem;
		MemoryContext		old;
		int					limit_idx;

		varray = (CollectVarrayValue *) DatumGetPointer(collect_data->values);
		limit_idx = collect_data->limit_idx;

		/* free every elements */
		idx = 0;
		while (idx < varray->current_used)
		{
			elem = (CollecTypeValue *) DatumGetPointer(varray->value[idx]);

			free_collectype_value(elem, datatype);
			varray->value[idx] = (Datum) 0;
			idx++;
		}

		pfree(varray->value);

		varray->current_used = 0;
		varray->total_index = CALC_VARRAY_TOTAL_INDEX(
									varray->current_used,
									limit_idx);

		old = MemoryContextSwitchTo(collect_data->valCxt);
		varray->value = palloc(sizeof(Datum) * varray->total_index);
		MemoryContextSwitchTo(old);
	}
	collect_data->max_index = 0;
}


/*
 * update_varray_memory
 * resize the memory size of the varray.
 * if memory size > current used size + 2 * reserve size, then
 *	reduce memory and set memory size = current used size + reserve size;
 * if memory size == current used size ,then
 *  expand memory and set memory size = current used size + reserve size;
*/
static void
update_varray_memory(CollectTypeData *collect_data)
{
	CollectVarrayValue	*varray = NULL;
	int				limit_idx = collect_data->limit_idx;
	bool			reduce = false;

	if (!IS_VARRAY(collect_data))
		return;

	varray = (CollectVarrayValue*) DatumGetPointer(collect_data->values);

	if (CAN_VARRAY_REDUCE(varray->total_index, varray->current_used))
		reduce = true;

	if (reduce ||
		varray->current_used == varray->total_index)
	{
		MemoryContext	old;

		/* set varray reserve amount */
		varray->total_index = CALC_VARRAY_TOTAL_INDEX(
									varray->current_used, limit_idx);

		old = MemoryContextSwitchTo(collect_data->valCxt);

		if (reduce)
		{
			Datum		   *new;

			new = (Datum*) palloc0(sizeof(Datum) * varray->total_index);
			memcpy(new, varray->value, varray->current_used * sizeof(Datum));

			pfree(varray->value);
			varray->value = new;
		}
		else
			varray->value = (Datum*) repalloc(varray->value, sizeof(Datum) * varray->total_index);

		MemoryContextSwitchTo(old);
	}
}

/*
 * collectype_element_ref
 * get the collect element.
 * nSubscripts is the dim of collection,
 * indx is the values of the collection key.
 */
Datum
collectype_element_ref(Datum datum, int nSubscripts,
			Datum *indx, bool *isNull)
{
	Datum		target;
	Datum		val_value;
	bool		find_key;

	/* get the target value */
	target = get_collect_value(datum, nSubscripts - 1, indx);

	find_key = get_collectype_value(target, indx[nSubscripts - 1], &val_value, isNull);

	if (!find_key)
		ereport(ERROR,
			(errcode(ERRCODE_NO_DATA_FOUND),
			errmsg("no data found")));

	return val_value;
}

/*
 * set the field value of a record or a row
 */
static HeapTupleHeader
set_rec_field_value(HeapTupleHeader value,
					PLiSQL_type **pl_datatype,
					List *cur_fieldnames,
					int rec_len,
					int nth_field,
					int nsubscripts,
					Datum *subscriptvals,
					int *nth,
					List **all_fieldnames,
					Datum assign_value,
					bool assign_value_isnull)
{
	Datum		val;
	char		*fieldname;
	HeapTupleHeader td;
	Oid 		tupType;
	int32		tupTypmod;
	TupleDesc	tupdesc;
	HeapTupleData tmptup;
	HeapTuple	tup;
	int 		fno = 0;
	bool		isnull;
	int 		natts;
	Datum	   *values;
	bool	   *nulls;
	bool	   *replace;
	HeapTuple	newtup;
	Oid 		fieldOid;
	HeapTupleHeader newheader;

	if (pl_datatype == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_VALUE_ERROR),
				errmsg("the datatype is null")));

	/* get the nth field name in the fieldnames list*/
	fieldname = (char *)list_nth(cur_fieldnames, nth_field);

	td = value;

	/* Extract rowtype info and find a tupdesc */
	tupType = HeapTupleHeaderGetTypeId(td);
	tupTypmod = HeapTupleHeaderGetTypMod(td);
	tupdesc = lookup_rowtype_tupdesc(tupType, tupTypmod);

	/* Build a temporary HeapTuple control structure */
	tmptup.t_len = HeapTupleHeaderGetDatumLength(td);
	ItemPointerSetInvalid(&(tmptup.t_self));
	tmptup.t_tableOid = InvalidOid;
	tmptup.t_data = td;
	tup = &tmptup;

	fno = SPI_fnumber(tupdesc, fieldname);
	if (fno <= 0)
	{
		char	name[(NAMEDATALEN * 2) + 100];

		snprintf(name, sizeof(name), "%s.%s", (*pl_datatype)->typname, fieldname);
		fno = SPI_fnumber(tupdesc, name);
	}
	if (fno <= 0)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_COLUMN),
				 errmsg("record  has no field \"%s\"", fieldname)));

	val = heap_getattr(tup, fno, tupdesc, &isnull);
	fieldOid = tupdesc->attrs[fno - 1].atttypid;
	if ((*pl_datatype)->typoid == ORARECORDOID)
	{
		PLiSQL_ora_record_type *ora_rec_type = (PLiSQL_ora_record_type *) (*pl_datatype);
		ListCell	*lc;
		PLiSQL_type *field_datatype = NULL;

		foreach(lc, ora_rec_type->args)
		{
			PLiSQL_ora_record_argitem	*argitem = (PLiSQL_ora_record_argitem*)lfirst(lc);

			 if (strcmp(fieldname, argitem->argname) == 0)
			 {
				field_datatype = argitem->type;
				break;
			 }
		}
		if (field_datatype == NULL)
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_COLUMN),
					 errmsg("record  has no field \"%s\"", fieldname)));

		*pl_datatype = field_datatype;
	}

	if (nth_field < rec_len - 1)
	{
		if (!type_is_rowtype(fieldOid))
				ereport(ERROR,
						(errcode(ERRCODE_UNDEFINED_COLUMN),
						 errmsg("field \"%s\" is not a record or a row ", fieldname)));

		if (isnull)
		{
			/* init a heap */
			val = init_heaptuple(tupdesc->attrs[fno - 1].atttypid,
							tupdesc->attrs[fno - 1].atttypmod,
							*pl_datatype);
		}

		val = PointerGetDatum(set_rec_field_value(DatumGetHeapTupleHeader(val),
							pl_datatype,
							cur_fieldnames,
							rec_len,
							nth_field + 1,
							nsubscripts,
							subscriptvals,
							nth,
							all_fieldnames,
							assign_value,
							assign_value_isnull));

	}
	else
	{
		if (IS_COLLECTOID(fieldOid))
		{
			if (isnull)
			{
				if (fieldOid == ASSOCIATIVE_ARRAYOID)
				{
					if (*nth == nsubscripts)
					{
						/* it is the last subscript */
						val = assign_value;
					}
					else
					{
						(*nth)++;
						val = init_collectype_var(pl_type_to_collectinfo(*pl_datatype));
						set_collect_value(val,
										nsubscripts,
										subscriptvals,
										nth,
										all_fieldnames,
										(void*)(*pl_datatype),
										assign_value,
										assign_value_isnull);
					}
				}
				else
				{
					if (*nth == nsubscripts)
					{
						/* it is the last subscript */
						val = assign_value;
					}
					else
						ereport(ERROR,
							(errcode(ERRCODE_NO_DATA_FOUND),
							errmsg("data not found")));
				}
			}
			else
			{
				if (*nth == nsubscripts)
				{
					/* it is the last subscript */
					val = assign_value;
				}
				else
				{
					(*nth)++;
					set_collect_value(val,
									nsubscripts,
									subscriptvals,
									nth,
									all_fieldnames,
									(void*)(*pl_datatype),
									assign_value,
									assign_value_isnull);
				}

			}
		}
		else
		{
			if (*nth == nsubscripts)
			{
				/* it is the last subscript */
				val = assign_value;
			}
		}
	}

	natts = tupdesc->natts;
	values = palloc0(sizeof(Datum) * natts);
	nulls = palloc0(sizeof(bool) * natts);
	replace = (bool *) palloc0(tupdesc->natts * sizeof(bool));

	heap_deform_tuple(tup, tupdesc, values, nulls);
	values[fno - 1] = val;
	nulls[fno - 1] = assign_value_isnull;
	replace[fno - 1] = true;

	newtup = heap_modify_tuple(&tmptup, tupdesc, values, nulls, replace);

	/* free old tuple */
	pfree((void *)value);
	pfree(values);
	pfree(nulls);
	pfree(replace);

	/* copy newtup->t_data to newheader */
	newheader = (HeapTupleHeader) palloc(HeapTupleHeaderGetDatumLength(newtup->t_data));
	memcpy(newheader, newtup->t_data, HeapTupleHeaderGetDatumLength(newtup->t_data));
	pfree(newtup);

	ReleaseTupleDesc(tupdesc);

	return newheader;
}


/*
 *  get_collect_value
 *	return collect value in certain dimensional by nsubscripts.
 *  val: top dimensional collect value.
 *  nsubscripts: the count of the subscripts
 *  subscriptvals: subscript values
 */
Datum
get_collect_value(Datum datum,
				int nsubscripts,
				Datum *subscriptvals)
{
	int 			i = 0;
	CollectTypeInfo *type;
	CollectType 	val = (CollectType)PointerGetDatum(datum);
	CollecTypeKey	*key = NULL;
	CollecTypeValue *elem_val = NULL;

	CHECK_COLLECTION_IS_NULL(val);
	type = val->type;

	Assert(nsubscripts >= 0);
	while (i < nsubscripts)
	{
		Assert(val->valCxt != NULL);
		Assert(IS_PLSQL_COLLECTYPE(type->typtype));

		/* sanity check */
		if (val->collect_typtype == NESTED_TABLE ||
			val->collect_typtype == VARRAY)
		{
			if (DatumGetInt32(subscriptvals[i]) <= 0)
				ereport(ERROR,
					(errcode(ERRCODE_SUBSCRIPT_OUTSIDE_LIMIT),
					errmsg("subscript outside limit")));
			else if (DatumGetInt32(subscriptvals[i]) > val->max_index)
				ereport(ERROR,
					(errcode(ERRCODE_SUBSCRIPT_BEYOND_COUNT),
					errmsg("subscript beyond count")));
		}

		if (!IS_VARRAY(val))
		{
			c_map			*map;
			c_iterator		iter;
			c_iterator		end;

			/* assign key value */

			key = (CollecTypeKey *) palloc0(sizeof(CollecTypeKey));
			key->value = subscriptvals[i];

			map = (c_map*) DatumGetPointer(val->values);
			end = c_map_end(map);

			iter = c_map_find(map, (void*) key);
			if (ITER_EQUAL(iter, end))
			{
				pfree(key);
				ereport(ERROR,
					(errcode(ERRCODE_NO_DATA_FOUND),
					errmsg("data not found")));
			}

			elem_val = (CollecTypeValue *) PointerGetDatum(GET_COLLECT_ELEMENT_VALUE(iter));
			pfree(key);
		}
		else
		{
			int32			key_val;
			CollectVarrayValue	*varray;

			key_val = DatumGetInt32(subscriptvals[i]);
			varray = (CollectVarrayValue *) DatumGetPointer(val->values);

			/* Begin - BUG#Z7714 */
			if (key_val <= 0 || key_val > varray->current_used)
				ereport(ERROR,
					(errcode(ERRCODE_SUBSCRIPT_OUTSIDE_LIMIT),
					errmsg("subscript outside limit")));
			/* End - BUG#Z7714 */

			elem_val = (CollecTypeValue *) DatumGetPointer(varray->value[key_val - 1]);
		}

		val = (CollectType) DatumGetPointer(elem_val->value);
		type =	type->typdatatype;
		i++;
	}

	return PointerGetDatum(val);
}

/*
 * set the value for a collect.
 * datum is the collect value,
 * nsubscripts is the number of subscripts,
 * subscriptvals is the value of subscripts array,
 * nth is the index of subscripts,
 * fieldnames is the array of field names,
 * top_datatype is the datatype of collection
 * assign_value is the value of assign,
 * assign_value_isnull is true if the assign_value is NULL.
 */
void
set_collect_value(Datum datum,
				int nsubscripts,
				Datum *subscriptvals,
				int *nth,
				List **fieldnames,
				void *top_datatype,
				Datum assign_value,
				bool assign_value_isnull)
{
	CollectTypeInfo	*type;
	CollectType		val = (CollectType)DatumGetPointer(datum);
	CollecTypeKey	*key = NULL;
	CollecTypeValue	*elem_val = NULL;
	PLiSQL_type		*pl_datatype = (PLiSQL_type *) top_datatype;
	Datum			next_value;

	CHECK_COLLECTION_IS_NULL(val)
	if (pl_datatype == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_VALUE_ERROR),
				errmsg("the datatype is null")));

	type = val->type;

	Assert(nsubscripts >= 0);
	Assert(val->valCxt != NULL);
	Assert(IS_PLSQL_COLLECTYPE(type->typtype));

	/* sanity check */
	if (val->collect_typtype == NESTED_TABLE ||
		val->collect_typtype == VARRAY)
	{
		if (DatumGetInt32(subscriptvals[(*nth) - 1]) <= 0)
			ereport(ERROR,
				(errcode(ERRCODE_SUBSCRIPT_OUTSIDE_LIMIT),
				errmsg("subscript outside limit")));
		else if (DatumGetInt32(subscriptvals[(*nth) - 1]) > val->max_index)
			ereport(ERROR,
				(errcode(ERRCODE_SUBSCRIPT_BEYOND_COUNT),
				errmsg("subscript beyond count")));
	}

	if (!IS_VARRAY(val))
	{
		c_map			*map;
		c_iterator		iter;
		c_iterator		end;
		Oid				keyoid;
		Datum	val_datum = PointerGetDatum(NULL);
		bool	val_isnull;

		/* assign key value */
		keyoid = type->typkey->typoid;

		key = (CollecTypeKey *) palloc0(sizeof(CollecTypeKey));
		key->value = datumCopy(subscriptvals[(*nth) - 1], get_typbyval(keyoid), get_typlen(keyoid));

		map = (c_map*) DatumGetPointer(val->values);
		end = c_map_end(map);

		iter = c_map_find(map, (void*) key);

		if (ITER_EQUAL(iter, end))
		{
			/*
			 * can't find the associate array or nested table element by the key.
			 */
			if (val->collect_typtype == ASSOCIATIVE_ARRAY)
			{
				if (*nth == nsubscripts && /* if the nth is the last dim of a collection */
					(fieldnames == NULL || /* the fieldnames of a associate array is NULL */
					(fieldnames != NULL && /* the fieldnames of a associate array is NOT NULL */
					fieldnames[nsubscripts - (*nth)] == NULL)/* the last dim collection has no field */))
				{
					/*
					 * it's a associate array, insert the key and value to associate array.
					 */
					val_datum = assign_value;
					val_isnull = assign_value_isnull;
					collect_insert(PointerGetDatum(val), key->value, val_datum, val_isnull);
					iter = c_map_find(map, (void*) key);
					elem_val = (CollecTypeValue *) DatumGetPointer(GET_COLLECT_ELEMENT_VALUE(iter));
				}
				else
				{
					if (IS_PLSQL_ASSOCIATIVE_ARRAY(type->typdatatype->typtype))
					{
						/*
						 * the element of associate array is also a associate array,
						 * init it for the associate array element.
						 */
						val_datum = init_collectype_var(PointerGetDatum(type->typdatatype));
						val_isnull = false;
					}
					else if (type_is_rowtype(type->typdatatype->typoid) &&
						fieldnames != NULL &&
						fieldnames[nsubscripts - (*nth)] != NULL)
					{
						int		i = 0;

						/*
						 * the element of associate array is a record,
						 * init a heaptuple for the associate array element.
						 */
						val_datum = init_heaptuple(type->typdatatype->typoid,
												type->typdatatype->typmod,
												((PLiSQL_ora_collection_type*)pl_datatype)->typdatatype);
						val_isnull = false;
						pl_datatype = ((PLiSQL_ora_collection_type*)pl_datatype)->typdatatype;
						val_datum = PointerGetDatum(set_rec_field_value(DatumGetHeapTupleHeader(val_datum), &pl_datatype,
											fieldnames[nsubscripts - (*nth)],
											list_length(fieldnames[nsubscripts - (*nth)]),
											i,
											nsubscripts,
											subscriptvals,
											nth,
											fieldnames,
											assign_value,
											assign_value_isnull));
					}
					else
						ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							errmsg("not a collection")));

					collect_insert(PointerGetDatum(val), key->value, val_datum, val_isnull);
					iter = c_map_find(map, (void*) key);
				}
			}
			else
			{
				if (key != NULL)
					pfree(key);

				ereport(ERROR,
					(errcode(ERRCODE_NO_DATA_FOUND),
					errmsg("no data found")));
			}
			elem_val = (CollecTypeValue *) DatumGetPointer(GET_COLLECT_ELEMENT_VALUE(iter));
			next_value = elem_val->value;
		}
		else
		{
			elem_val = (CollecTypeValue *) DatumGetPointer(GET_COLLECT_ELEMENT_VALUE(iter));
			next_value = elem_val->value;
			if (*nth == nsubscripts &&
				(fieldnames == NULL ||
				(fieldnames != NULL &&
				fieldnames[nsubscripts - (*nth)] == NULL)))
			{
				val_datum = assign_value;
				val_isnull = assign_value_isnull;
				collect_insert(PointerGetDatum(val), key->value, val_datum, val_isnull);
				iter = c_map_find(map, (void*) key);
				elem_val = (CollecTypeValue *) DatumGetPointer(GET_COLLECT_ELEMENT_VALUE(iter));
			}
			else
			{
				if (type_is_rowtype(type->typdatatype->typoid) &&
					fieldnames != NULL &&
					fieldnames[nsubscripts - (*nth)] != NULL)
				{
					int 	i = 0;

					pl_datatype = ((PLiSQL_ora_collection_type*)pl_datatype)->typdatatype;
					elem_val->value = PointerGetDatum(set_rec_field_value(DatumGetHeapTupleHeader(elem_val->value), &pl_datatype,
										fieldnames[nsubscripts - (*nth)],
										list_length(fieldnames[nsubscripts - (*nth)]),
										i,
										nsubscripts,
										subscriptvals,
										nth,
										fieldnames,
										assign_value,
										assign_value_isnull));
					next_value = elem_val->value;
				}
			}
		}
		pfree(key);
	}
	else
	{
		int32			key_val;

		key_val = DatumGetInt32(subscriptvals[(*nth) - 1]);
		if (*nth == nsubscripts &&
			(fieldnames == NULL ||
			(fieldnames != NULL &&
			fieldnames[nsubscripts - (*nth)] == NULL)))
		{
			collect_insert(PointerGetDatum(val), key_val, assign_value, assign_value_isnull);
			return;
		}
		else
		{
			CollectVarrayValue	*varray;

			varray = (CollectVarrayValue *) DatumGetPointer(val->values);
			elem_val = (CollecTypeValue *) DatumGetPointer(varray->value[key_val - 1]);

			if (type_is_rowtype(type->typdatatype->typoid) &&
				fieldnames != NULL &&
				fieldnames[nsubscripts - (*nth)] != NULL)
			{
				int 	i = 0;

				pl_datatype = ((PLiSQL_ora_collection_type*)pl_datatype)->typdatatype;
				elem_val->value = PointerGetDatum(set_rec_field_value(DatumGetHeapTupleHeader(elem_val->value), &pl_datatype,
									fieldnames[nsubscripts - (*nth)],
									list_length(fieldnames[nsubscripts - (*nth)]),
									i,
									nsubscripts,
									subscriptvals,
									nth,
									fieldnames,
									assign_value,
									assign_value_isnull));
			}
			next_value = elem_val->value;
		}
	}

	if ((*nth) < nsubscripts)
	{
		(*nth)++;
		if (pl_datatype != NULL)
			pl_datatype = ((PLiSQL_ora_collection_type*)pl_datatype)->typdatatype;

		set_collect_value(next_value,
						nsubscripts,
						subscriptvals,
						nth,
						fieldnames,
						(void*)pl_datatype,
						assign_value,
						assign_value_isnull);
	}
	return;
}

/*
 * get_collectype_value
 *	get the value of the associate array, nested table, varray,
 *	if find the key_value in the collect_data, then return true and
 *	set the element value into val_value and isnull.
 *	if can not find the key_value in the collect_data, then return false
 *	and set *val_value = 0 and *isnull = false.
 *
 */
bool
get_collectype_value(Datum collect_datum,
		Datum key_value, Datum *val_value, bool *isnull)
{
	CollectType 			collect_data;

	collect_data = (CollectType)DatumGetPointer(collect_datum);

	*isnull = true;
	*val_value = (Datum)0;

	if (!IS_VARRAY(collect_data))
	{
		c_map		*map;
		c_iterator	iter,
					end;
		CollecTypeKey	*key;

		map = (c_map *) collect_data->values;
		Assert (map != NULL);

		/* construct keys */
		key = (CollecTypeKey *) palloc0(sizeof(CollecTypeKey));
		key->value = key_value;

		iter = c_map_find(map, key);
		end = c_map_end(map);

		if (!ITER_EQUAL(iter, end))
		{
			CollecTypeValue		*element;

			element = (CollecTypeValue *) GET_COLLECT_ELEMENT_VALUE(iter);
			if (element->placeholder)
				ereport(ERROR,
					(errcode(ERRCODE_NO_DATA_FOUND),
					errmsg("data not found")));

			*isnull = element->isnull;
			*val_value = PointerGetDatum(element->value);
			pfree(key);
			return true;
		}
		pfree(key);
	}
	else
	{
		int32			int_key_val;
		CollectVarrayValue	*varray;

		varray = (CollectVarrayValue*) DatumGetPointer(collect_data->values);
		int_key_val = DatumGetInt32(key_value);

		/* Begin - BUG#Z7714 */
		if (int_key_val <= 0 || int_key_val > varray->current_used)
			ereport(ERROR,
				(errcode(ERRCODE_SUBSCRIPT_OUTSIDE_LIMIT),
				errmsg("subscript outside limit")));
		else
		/* End - BUG#Z7714 */
		{
			CollecTypeValue		*element;

			element = (CollecTypeValue*) DatumGetPointer(varray->value[int_key_val - 1]);
			*isnull = element->isnull;
			*val_value = PointerGetDatum(element->value);
			return true;
		}
	}

	return false;
}

char
get_collectyptype(Datum collect_datum)
{
	CollectType 		collect_data;

	collect_data = (CollectType)DatumGetPointer(collect_datum);
	return collect_data->type->typtype;
}

/*
 * collect_insert
 * insert data into the collection value.
 */
void
collect_insert(Datum collect_data, Datum key,
			  Datum value, bool null_value)
{
	insert_collectype_data((CollectType)DatumGetPointer(collect_data),
						key, value, null_value, true, false);
}

/*
 * collectype_cast_Check
 * check the collection value type typmod is the same as reqtypmod.
 */
bool
collectype_cast_Check(Datum value, int32 reqtypmod)
{
	CollectType collect_data = (CollectType)DatumGetPointer(value);

	if (collect_data->type->typmod != reqtypmod)
		ereport(ERROR,
			(errcode(ERRCODE_CANNOT_COERCE),
			errmsg("expression is of wrong type")));

	return true;
}

/*
 * collection constructor. it applies only to varray and nested table
 */
Datum
collection_constructor(PG_FUNCTION_ARGS)
{
	Oid				datatypoid,
					keytypoid;
	int32			datatypemod,
					keytypmod;
	bool			notnull;
	int32			idx;
	int16			typlen;
	char			typalign;
	bool			typbyval;
	bool			isnull;
	Datum			datum;
	CollectTypeData		*collect_data;
	CollectTypeInfo	*collectype;
	MemoryContext	old;
	ArrayType		*array = (ArrayType*) PG_GETARG_POINTER(1);
	int32			array_len = 0;

	/* the first argument is the collect type */
	datum = (Datum) PG_GETARG_DATUM(0);
	collectype = (CollectTypeInfo *) DatumGetPointer(datum);

	/* get the collect type info */
	get_collectype_typeinfo(collectype, &keytypoid, &keytypmod,
						&datatypoid, &datatypemod, &notnull);

	/* array->ndim is 0 when array is empty */
	Assert(array->ndim == 1 || array->ndim == 0);
	if (array->ndim > 0)
		array_len = DirectFunctionCall2(array_length, PointerGetDatum(array),
													Int32GetDatum(array->ndim));

	/* init a empty collect */
	collect_data = create_collectype_data(collectype, collectype->limit, array_len);
	old = MemoryContextSwitchTo(collect_data->valCxt);

	get_typlenbyvalalign(array->elemtype, &typlen, &typbyval, &typalign);
	for (idx = 1; idx <= array_len; idx++)
	{
		Datum		tmp;
		Datum		ud_key;
		Datum		ud_val = (Datum) 0;

		/* create the value of the element */
		tmp = array_ref(array, 1, &idx, -1,
					typlen, typbyval, typalign, &isnull);

		ud_key = Int32GetDatum(idx);

		if (!typbyval && !isnull)
			tmp = datumCopy(tmp, typbyval, typlen);

		if (!isnull)
		{
			if (array->elemtype == datatypoid &&
				IS_PLSQL_COLLECTYPE(collectype->typdatatype->typtype))
			{
				if (collectype_cast_Check(tmp, datatypemod))
					ud_val = tmp;
			}
			else if (array->elemtype == datatypoid &&
				(datatypoid == RECORDOID ||
				datatypoid == ORARECORDOID ||
				get_typtype(datatypoid) == TYPTYPE_COMPOSITE))
				ud_val = rowDatumCopy(tmp, isnull);
			else
				ud_val = doCastValue(tmp, array->elemtype, datatypoid, datatypemod);
		}
		else if (notnull)
			ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				errmsg("cannot pass NULL to a NOT NULL constrained formal parameter")));

		insert_collectype_data(collect_data, ud_key, ud_val, isnull, true, false);
	}

	MemoryContextSwitchTo(old);

	PG_RETURN_POINTER(collect_data);
}

/*
 * get the key and elements value informations of a collection type
 */
static void
get_collectype_typeinfo(CollectTypeInfo *type,
				Oid *keyoid,
				int32 *keymod,
				Oid *valueoid,
				int32 *valuemod,
				bool *notnull)
{
	if (!type || !IS_PLSQL_COLLECTYPE(type->typtype))
	{
		*keyoid = InvalidOid;
		*keymod = -1;
		*valueoid = InvalidOid;
		*valuemod = -1;

		if (type)
			*notnull = type->datanotnull;
		else
			*notnull = false;
		return;
	}

	*keyoid = type->typkey->typoid;
	*keymod = type->typkey->typmod;

	*valueoid = type->typdatatype->typoid;
	*valuemod = type->typdatatype->typmod;

	*notnull = type->datanotnull;
}

/*
 * FIRST Collection Method
 */
Datum
collection_first(PG_FUNCTION_ARGS)
{
	CollectTypeData 	*collect_value = (CollectTypeData *)PointerGetDatum(PG_GETARG_DATUM(0));

	CHECK_COLLECTION_IS_NULL(collect_value);

	if (!IS_VARRAY(collect_value))
	{
		c_map			*map;
		c_iterator 		iter;
		CollecTypeKey	*key;

		map = (c_map *) DatumGetPointer(collect_value->values);

		if (c_map_empty(map))
			PG_RETURN_NULL();

		iter = c_map_begin(map);
		if (IS_ASSOCIATIVE_ARRAY(collect_value))
		{
			key = (CollecTypeKey *) GET_COLLECT_ELEMENT_KEY(iter);
			PG_RETURN_DATUM(key->value);
		}
		else
		{
			c_iterator		end;
			CollecTypeValue *elem_val;

			end = c_map_end(map);

			while (!ITER_EQUAL(iter, end))
			{
				elem_val = (CollecTypeValue *) PointerGetDatum(GET_COLLECT_ELEMENT_VALUE(iter));
				if (!elem_val->placeholder)
				{
					key = (CollecTypeKey *) GET_COLLECT_ELEMENT_KEY(iter);
					PG_RETURN_DATUM(key->value);
				}
				else
					ITER_INC(iter);
			}
			PG_RETURN_NULL();
		}
	}
	else
	{
		CollectVarrayValue	*varray = (CollectVarrayValue*) DatumGetPointer(collect_value->values);

		if (varray->current_used == 0)
			PG_RETURN_NULL();
		else
			PG_RETURN_DATUM(Int32GetDatum(1));
	}
}

/*
 * LAST Collection Method
 */
Datum
collection_last(PG_FUNCTION_ARGS)
{
	CollectTypeData 	*collect_value = (CollectTypeData *)PointerGetDatum(PG_GETARG_DATUM(0));

	CHECK_COLLECTION_IS_NULL(collect_value);

	if (!IS_VARRAY(collect_value))
	{
		c_map			*map;
		c_iterator 		iter;
		CollecTypeKey	*key;

		map = (c_map *) DatumGetPointer(collect_value->values);

		if (c_map_empty(map))
			PG_RETURN_NULL();

		iter = c_map_end(map);
		ITER_DEC(iter);
		if (IS_ASSOCIATIVE_ARRAY(collect_value))
		{
			key = (CollecTypeKey *) GET_COLLECT_ELEMENT_KEY(iter);
			PG_RETURN_DATUM(key->value);
		}
		else
		{
			c_iterator		begin;
			CollecTypeValue *elem_val;

			begin = c_map_begin(map);

			while (1)
			{
				elem_val = (CollecTypeValue *) PointerGetDatum(GET_COLLECT_ELEMENT_VALUE(iter));
				if (!elem_val->placeholder)
				{
					key = (CollecTypeKey *) GET_COLLECT_ELEMENT_KEY(iter);
					PG_RETURN_DATUM(key->value);
				}
				else
				{
					if (ITER_EQUAL(iter, begin))
						PG_RETURN_NULL();
					else
						ITER_DEC(iter);
				}
			}
			PG_RETURN_NULL();
		}
	}
	else
	{
		CollectVarrayValue	*varray = (CollectVarrayValue*) DatumGetPointer(collect_value->values);

		if (varray->current_used == 0)
			PG_RETURN_NULL();
		else
			PG_RETURN_DATUM(Int32GetDatum(varray->current_used));
	}
}

/*
 * COUNT Collection Method
 */
Datum
collection_count(PG_FUNCTION_ARGS)
{
	CollectTypeData 	*collect_value = (CollectTypeData *)PointerGetDatum(PG_GETARG_DATUM(0));

	CHECK_COLLECTION_IS_NULL(collect_value);

	if (!IS_VARRAY(collect_value))
	{
		c_map			*map;

		map = (c_map *) DatumGetPointer(collect_value->values);

		if (IS_ASSOCIATIVE_ARRAY(collect_value))
			PG_RETURN_DATUM(c_map_size(map));
		else
		{
			int 		num;
			c_iterator	begin;
			c_iterator	end;
			c_iterator	iter;

			begin = c_map_begin(map);
			end = c_map_end(map);
			iter = begin;

			num = 0;
			while (!ITER_EQUAL(iter, end))
			{
				CollecTypeValue *elem_val = (CollecTypeValue *) PointerGetDatum(GET_COLLECT_ELEMENT_VALUE(iter));

				if (!elem_val->placeholder)
					num++;
				ITER_INC(iter);
			}
			PG_RETURN_INT32(num);
		}
	}
	else
	{
		CollectVarrayValue	*varray = (CollectVarrayValue*) DatumGetPointer(collect_value->values);

		PG_RETURN_DATUM(varray->current_used);
	}
}

/*
 * LIMIT Collection Method
 */
Datum
collection_limit(PG_FUNCTION_ARGS)
{
	CollectTypeData 	*collect_value = (CollectTypeData *)PointerGetDatum(PG_GETARG_DATUM(0));

	CHECK_COLLECTION_IS_NULL(collect_value);

	if (!IS_VARRAY(collect_value))
		PG_RETURN_NULL();
	else
		PG_RETURN_DATUM(collect_value->limit_idx);
}

/*
 * PRIOR Collection Method
 */
Datum
collection_prior(PG_FUNCTION_ARGS)
{
	CollectTypeData 	*collect_value = (CollectTypeData *)PointerGetDatum(PG_GETARG_DATUM(0));
	Datum		index = PointerGetDatum(PG_GETARG_DATUM(1));

	CHECK_COLLECTION_IS_NULL(collect_value);

	if (PG_ARGISNULL(1))
		PG_RETURN_NULL();

	if (!IS_VARRAY(collect_value))
	{
		c_map			*map;
		CollecTypeKey	*key;
		CollecTypeKey	*prior_key;

		map = (c_map *) DatumGetPointer(collect_value->values);

		if (c_map_empty(map))
			PG_RETURN_NULL();

		/* construct the key */
		key = (CollecTypeKey *) palloc0(sizeof(CollecTypeKey));
		key->value = index;

		prior_key = (CollecTypeKey *)c_map_collection_prior(map, key);
		if (prior_key == NULL)
			PG_RETURN_NULL();

		/*
		 * After execute the delete method for the associative array,
		 * the elements are directly deleted. However, for nested tables,
		 * after execute the delete method, it keeps a placeholder.
		 * Therefore, if the prior method finds the previous key
		 * based on the specified key value, it still needs to continue
		 * the judgment until the found element is not a placeholder,
		 * and then returns key->value.
		 */
		if (IS_ASSOCIATIVE_ARRAY(collect_value))
			PG_RETURN_DATUM(prior_key->value);
		else
		{
			c_iterator	begin;
			c_iterator	iter;

			iter = c_map_find(map, prior_key);
			begin = c_map_begin(map);

			while (1)
			{
				CollecTypeValue *elem_val = (CollecTypeValue *) PointerGetDatum(GET_COLLECT_ELEMENT_VALUE(iter));

				if (!elem_val->placeholder)
				{
					CollecTypeKey	*tmpkey = (CollecTypeKey *) GET_COLLECT_ELEMENT_KEY(iter);

					PG_RETURN_DATUM(tmpkey->value);
				}
				else
				{
					if (ITER_EQUAL(iter, begin))
						PG_RETURN_NULL();
					else
						ITER_DEC(iter);
				}
			}
			PG_RETURN_NULL();
		}
	}
	else
	{
		CollectVarrayValue	*varray = (CollectVarrayValue*) DatumGetPointer(collect_value->values);

		if (varray->current_used == 0)
			PG_RETURN_NULL();
		else
		{
			int key = DatumGetInt32(index);

			if (key <= 1)
				PG_RETURN_NULL();

			if (key > varray->current_used)
				PG_RETURN_DATUM(Int32GetDatum(varray->current_used));

			PG_RETURN_DATUM(Int32GetDatum(key - 1));
		}
	}
}

/*
 * NEXT Collection Method
 */
Datum
collection_next(PG_FUNCTION_ARGS)
{
	CollectTypeData	*collect_value = (CollectTypeData *)PointerGetDatum(PG_GETARG_DATUM(0));
	Datum		index = PointerGetDatum(PG_GETARG_DATUM(1));

	CHECK_COLLECTION_IS_NULL(collect_value);

	if (PG_ARGISNULL(1))
		PG_RETURN_NULL();

	if (!IS_VARRAY(collect_value))
	{
		c_map			*map;
		CollecTypeKey		*key;
		CollecTypeKey		*next_key;

		map = (c_map *) DatumGetPointer(collect_value->values);

		if (c_map_empty(map))
			PG_RETURN_NULL();

		/* construct the key */
		key = (CollecTypeKey *) palloc0(sizeof(CollecTypeKey));
		key->value = index;

		next_key = (CollecTypeKey *)c_map_collection_next(map, key);
		if (next_key == NULL)
			PG_RETURN_NULL();
		if (IS_ASSOCIATIVE_ARRAY(collect_value))
			PG_RETURN_DATUM(next_key->value);
		else
		{
			c_iterator	end;
			c_iterator	iter;

			iter = c_map_find(map, next_key);
			end = c_map_end(map);

			while (!ITER_EQUAL(iter, end))
			{
				CollecTypeValue *elem_val = (CollecTypeValue *) PointerGetDatum(GET_COLLECT_ELEMENT_VALUE(iter));

				if (!elem_val->placeholder)
				{
					CollecTypeKey	*tmpkey = (CollecTypeKey *) GET_COLLECT_ELEMENT_KEY(iter);

					PG_RETURN_DATUM(tmpkey->value);
				}
				else
				{
					if (ITER_EQUAL(iter, end))
						PG_RETURN_NULL();
					else
						ITER_INC(iter);
				}
			}
			PG_RETURN_NULL();
		}
	}
	else
	{
		CollectVarrayValue	*varray = (CollectVarrayValue*) DatumGetPointer(collect_value->values);

		if (varray->current_used == 0)
			PG_RETURN_NULL();
		else
		{
			int key = DatumGetInt32(index);

			if (key < 1)
				PG_RETURN_DATUM(Int32GetDatum(1));

			if (key >= varray->current_used)
				PG_RETURN_NULL();

			PG_RETURN_DATUM(Int32GetDatum(key + 1));
		}
	}
}

/*
 * EXISTS Collection Method
 */
Datum
collection_exists(PG_FUNCTION_ARGS)
{
	CollectTypeData	*collect_value = (CollectTypeData *)PointerGetDatum(PG_GETARG_DATUM(0));
	Datum		index = PointerGetDatum(PG_GETARG_DATUM(1));

	if (collect_value == NULL)
		PG_RETURN_BOOL(false);

	if (PG_ARGISNULL(1))
		PG_RETURN_BOOL(false);

	if (!IS_VARRAY(collect_value))
	{
		c_map			*map;
		CollecTypeKey		*key;
		c_iterator		iter_end;
		c_iterator		iter;

		map = (c_map *) DatumGetPointer(collect_value->values);

		if (c_map_empty(map))
			PG_RETURN_BOOL(false);

		/* construct the key */
		key = (CollecTypeKey *) palloc0(sizeof(CollecTypeKey));
		key->value = index;

		iter_end = c_map_end(map);
		iter = c_map_find(map, key);

		pfree(key);
		if (ITER_EQUAL(iter, iter_end))
			PG_RETURN_BOOL(false);
		else
		{
			CollecTypeValue *elem_val;

			elem_val = (CollecTypeValue *) PointerGetDatum(GET_COLLECT_ELEMENT_VALUE(iter));

			if (elem_val->placeholder)
				PG_RETURN_BOOL(false);
			else
				PG_RETURN_BOOL(true);
		}
	}
	else
	{
		CollectVarrayValue	*varray = (CollectVarrayValue*) DatumGetPointer(collect_value->values);

		if (varray->current_used == 0)
			PG_RETURN_BOOL(false);
		else
		{
			int key = DatumGetInt32(index);

			if (key >= 1 && key <= varray->current_used)
				PG_RETURN_BOOL(true);
			else
				PG_RETURN_BOOL(false);
		}
	}
}

/*
 * DELETE Collection Method
 */
Datum
collection_delete(PG_FUNCTION_ARGS)
{
	Datum		collect = PG_GETARG_DATUM(0);
	CollectTypeData	*collect_value = (CollectTypeData *)PointerGetDatum(collect);
	CollectTypeInfo	*datatype;

	CHECK_COLLECTION_IS_NULL(collect_value);

	datatype = collect_value->type->typdatatype;

	if (!IS_VARRAY(collect_value))
	{
		c_map			*map;
		c_iterator		end;
		c_iterator		iter;
		CollecTypeValue		*elem;
		CollecTypeKey		*key;

		map = (c_map *) DatumGetPointer(collect_value->values);

		if (c_map_empty(map))
			PG_RETURN_DATUM(collect);

		iter = c_map_begin(map);
		end = c_map_end(map);


		/* free every element */
		while (!ITER_EQUAL(iter, end))
		{
			elem = (CollecTypeValue *) GET_COLLECT_ELEMENT_VALUE(iter);
			key = (CollecTypeKey *) GET_COLLECT_ELEMENT_KEY(iter);

			free_collectype_value(elem, datatype);

			pfree(key);
			pfree(ITER_REF(iter));

			ITER_INC(iter);
		}

		c_map_clear(map);
		collect_value->max_index = 0;
		PG_RETURN_DATUM(collect);
	}
	else
	{
		CollectVarrayValue	*varray = (CollectVarrayValue*) DatumGetPointer(collect_value->values);
		int				idx;
		CollecTypeValue		*elem;
		MemoryContext	old;
		int				limit_idx;

		limit_idx = collect_value->limit_idx;

		/* free the elements of varray */
		idx = 0;
		while (idx < varray->current_used)
		{
			elem = (CollecTypeValue *) DatumGetPointer(varray->value[idx]);

			free_collectype_value(elem, datatype);
			varray->value[idx] = (Datum) 0;
			idx++;
		}

		pfree(varray->value);
		varray->current_used = 0;
		varray->total_index = CALC_VARRAY_TOTAL_INDEX(
									varray->current_used,
									limit_idx);

		old = MemoryContextSwitchTo(collect_value->valCxt);
		varray->value = palloc(sizeof(Datum) * varray->total_index);
		collect_value->max_index = 0;
		MemoryContextSwitchTo(old);

		PG_RETURN_DATUM(collect);
	}
}

/*
 * DELETE(n) Collection Method
 */
Datum
collection_delete1(PG_FUNCTION_ARGS)
{
	Datum		collect = PG_GETARG_DATUM(0);
	Datum		index = PointerGetDatum(PG_GETARG_DATUM(1));
	CollectTypeData	*collect_value = (CollectTypeData *)PointerGetDatum(collect);
	CollectTypeInfo	*datatype;

	CHECK_COLLECTION_IS_NULL(collect_value);

	if (PG_ARGISNULL(1))
		PG_RETURN_DATUM(collect);

	datatype = collect_value->type->typdatatype;

	if (!IS_VARRAY(collect_value))
	{
		c_map			*map;
		c_iterator		end;
		c_iterator		iter;
		CollecTypeValue 	*elem;
		CollecTypeKey		*key;

		map = (c_map *) DatumGetPointer(collect_value->values);

		if (c_map_empty(map))
			PG_RETURN_DATUM(collect);

		end = c_map_end(map);

		/* construct the key */
		key = (CollecTypeKey *) palloc0(sizeof(CollecTypeKey));
		key->value = index;

		iter = c_map_find(map, key);

		if (!ITER_EQUAL(iter, end))
		{
			CollecTypeKey *key_internal;

			elem = (CollecTypeValue *) GET_COLLECT_ELEMENT_VALUE(iter);
			key_internal = (CollecTypeKey *) GET_COLLECT_ELEMENT_KEY(iter);

			if (IS_ASSOCIATIVE_ARRAY(collect_value))
			{
				free_collectype_value(elem, datatype);

				pfree(key_internal);
				pfree(ITER_REF(iter));
				c_map_erase(map, iter);
			}
			else
			{
				if (!elem->isnull)
				{
					if (IS_PLSQL_COLLECTYPE(datatype->typtype))
						free_collectype_data((CollectTypeData *)elem->value, elem->isnull);
					else if (!datatype->typbyval)
						pfree(DatumGetPointer(elem->value));
					elem->value = 0;
					elem->isnull = true;
				}
				elem->placeholder = true;
			}
		}
		pfree(key);
	}
	else
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("wrong number or types of arguments in call to 'DELETE'")));

	PG_RETURN_DATUM(collect);
}

/*
 * DELETE(m, n) Collection Method
 */
Datum
collection_delete2(PG_FUNCTION_ARGS)
{
	Datum		collect = PG_GETARG_DATUM(0);
	Datum		index1 = PointerGetDatum(PG_GETARG_DATUM(1));
	Datum		index2 = PointerGetDatum(PG_GETARG_DATUM(2));
	CollectTypeData	*collect_value = (CollectTypeData *)PointerGetDatum(collect);
	CollectTypeInfo	*datatype;
	int 		comp_result;

	CHECK_COLLECTION_IS_NULL(collect_value);

	if (PG_ARGISNULL(1) || PG_ARGISNULL(2))
		PG_RETURN_DATUM(collect);

	datatype = collect_value->type->typdatatype;

	if (!IS_VARRAY(collect_value))
	{
		c_map			*map;
		c_iterator		iter1;
		c_iterator		iter2;
		CollecTypeValue 	*elem;
		CollecTypeKey		*key1;
		CollecTypeKey		*key2;
		Oid				key_oid;
		Oid				value_typoid;
		int				key_typmod;
		int				value_typmod;
		bool			notnull;

		map = (c_map *) DatumGetPointer(collect_value->values);

		if (c_map_empty(map))
			PG_RETURN_DATUM(collect);

		/* construct the key */
		key1 = (CollecTypeKey *) palloc0(sizeof(CollecTypeKey));
		key1->value = index1;
		key2 = (CollecTypeKey *) palloc0(sizeof(CollecTypeKey));
		key2->value = index2;

		get_collectype_typeinfo(collect_value->type, &key_oid, &key_typmod,
									&value_typoid, &value_typmod, &notnull);
		comp_result = collectype_key_compare(key_oid, (void*) key1, (void*) key2);

		if (comp_result > 0)
		{
			pfree(key1);
			pfree(key2);

			PG_RETURN_DATUM(collect);
		}

		if (comp_result == 0)
		{
			iter2 = c_map_end(map);
			iter1 = c_map_find(map, key1);
			if (ITER_EQUAL(iter1, iter2))
				PG_RETURN_DATUM(collect);

			if (!ITER_EQUAL(iter1, iter2))
			{
				CollecTypeKey *key_internal;

				elem = (CollecTypeValue *) GET_COLLECT_ELEMENT_VALUE(iter1);
				key_internal = (CollecTypeKey *) GET_COLLECT_ELEMENT_KEY(iter1);

				if (IS_ASSOCIATIVE_ARRAY(collect_value))
				{
					free_collectype_value(elem, datatype);

					pfree(key_internal);
					pfree(ITER_REF(iter1));
					c_map_erase(map, iter1);
				}
				else
				{
					if (!elem->isnull)
					{
						if (IS_PLSQL_COLLECTYPE(datatype->typtype))
							free_collectype_data((CollectTypeData *)elem->value, elem->isnull);
						else if (!datatype->typbyval)
							pfree(DatumGetPointer(elem->value));
						elem->value = 0;
						elem->isnull = true;
					}
					elem->placeholder = true;
				}
			}
		}
		else
		{
			CollecTypeKey	*key_first, *key_last;
			c_iterator		end;
			bool			find_firstkey = false;

			/* compare key2 to the first element key */
			iter1 = c_map_begin(map);
			key_first = (CollecTypeKey *) GET_COLLECT_ELEMENT_KEY(iter1);

			/* if the second parameter of DELETE function is samller than the first element key, do nothing */
			if (collectype_key_compare(key_oid, (void*) key2, (void*) key_first) < 0)
				PG_RETURN_DATUM(collect);

			/* compare key1 to the last element key */
			iter2 = c_map_end(map);
			ITER_DEC(iter2);
			key_last = (CollecTypeKey *) GET_COLLECT_ELEMENT_KEY(iter2);

			/* if the first parameter of DELETE function is larger than the last element key, do nothing */
			if (collectype_key_compare(key_oid, (void*) key1, (void*) key_last) > 0)
				PG_RETURN_DATUM(collect);

			end = c_map_end(map);

			while (!ITER_EQUAL(iter1, end))
			{
				bool			do_delete = false;
				CollecTypeKey	*cur_key;
				c_iterator		old;

				cur_key = (CollecTypeKey *) GET_COLLECT_ELEMENT_KEY(iter1);

				if (!find_firstkey)
				{
					if (collectype_key_compare(key_oid, (void*) key1, (void*) cur_key) > 0)
					{
						ITER_INC(iter1);
						continue;
					}
					else
					{
						find_firstkey = true;
						do_delete = true;
					}
				}
				else if (collectype_key_compare(key_oid, (void*) cur_key, (void*) key2) <= 0)
					do_delete = true;

				if (do_delete)
				{
					CollecTypeKey	*key_internal;
					elem = (CollecTypeValue *) GET_COLLECT_ELEMENT_VALUE(iter1);
					key_internal = (CollecTypeKey *) GET_COLLECT_ELEMENT_KEY(iter1);

					if (IS_ASSOCIATIVE_ARRAY(collect_value))
					{
						free_collectype_value(elem, datatype);
						old = iter1;
						pfree(key_internal);
						pfree(ITER_REF(iter1));
						ITER_INC(iter1);
						c_map_erase(map, old);
					}
					else
					{
						if (!elem->isnull)
						{
							if (IS_PLSQL_COLLECTYPE(datatype->typtype))
								free_collectype_data((CollectTypeData *)elem->value, elem->isnull);
							else if (!datatype->typbyval)
								pfree(DatumGetPointer(elem->value));
							elem->value = 0;
							elem->isnull = true;
						}
						elem->placeholder = true;
						ITER_INC(iter1);
					}
				}
				else
					ITER_INC(iter1);
			}
		}
		pfree(key1);
		pfree(key2);
	}
	else
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("wrong number or types of arguments in call to 'DELETE'")));

	PG_RETURN_DATUM(collect);
}

/*
 * TRIM Collection Method
 */
Datum
collection_trim(PG_FUNCTION_ARGS)
{
	Datum		collect = PG_GETARG_DATUM(0);
	CollectTypeData	*collect_value = (CollectTypeData *)PointerGetDatum(collect);
	CollectTypeInfo	*datatype;

	CHECK_COLLECTION_IS_NULL(collect_value);

	if (collect_value->max_index < 1)
		ereport(ERROR,
				(errcode(ERRCODE_SUBSCRIPT_BEYOND_COUNT),
				errmsg("subscript beyond count")));

	datatype = collect_value->type->typdatatype;

	if (!IS_VARRAY(collect_value))
	{
		c_map			*map;
		c_iterator		end;
		CollecTypeKey		*key;
		CollecTypeValue 	*elem;

		map = (c_map*) DatumGetPointer(collect_value->values);

		/* if the map is empty, do nothing */
		if (c_map_empty(map))
			PG_RETURN_DATUM(collect);

		end = c_map_end(map);
		ITER_DEC(end);

		key = (CollecTypeKey *) GET_COLLECT_ELEMENT_KEY(end);
		elem = (CollecTypeValue *) GET_COLLECT_ELEMENT_VALUE(end);
		free_collectype_value(elem, datatype);
		pfree(key);
		pfree(ITER_REF(end));
		c_map_erase(map, end);

		collect_value->max_index--;
	}
	else
	{
		CollectVarrayValue	*varray = (CollectVarrayValue*) DatumGetPointer(collect_value->values);
		CollecTypeValue		*elem;

		if (varray->current_used == 0)
			PG_RETURN_DATUM(collect);

		/* free the last element of varray */
		elem = (CollecTypeValue *) DatumGetPointer(varray->value[varray->current_used - 1]);
		free_collectype_value(elem, datatype);
		varray->value[varray->current_used - 1] = (Datum) 0;

		varray->current_used--;
		collect_value->max_index--;

		update_varray_memory(collect_value);
	}

	PG_RETURN_DATUM(collect);
}

/*
 * TRIM(n) Collection Method
 */
Datum
collection_trim_n(PG_FUNCTION_ARGS)
{
	Datum		collect = PG_GETARG_DATUM(0);
	int32		num = DatumGetInt32(PG_GETARG_DATUM(1));
	CollectTypeData	*collect_value = (CollectTypeData *)PointerGetDatum(collect);
	CollectTypeInfo		*datatype;
	int32		size;

	CHECK_COLLECTION_IS_NULL(collect_value);

	if (PG_ARGISNULL(1))
		PG_RETURN_DATUM(collect);

	if (!IS_VARRAY(collect_value))
	{
		c_map		*map;

		map = (c_map *) DatumGetPointer(collect_value->values);
		size = c_map_size(map);

		if (num < 0)
			ereport(ERROR,
				(errcode(ERRCODE_VALUE_ERROR),
				errmsg("numeric or value error")));
		if (num > size)
			ereport(ERROR,
				(errcode(ERRCODE_SUBSCRIPT_BEYOND_COUNT),
				errmsg("subscript beyond count")));
		datatype = collect_value->type->typdatatype;
		for (; num > 0; num--)
		{
			c_iterator		end;
			CollecTypeKey	*key;
			CollecTypeValue	*elem;

			end = c_map_end(map);
			ITER_DEC(end);

			key = (CollecTypeKey *) GET_COLLECT_ELEMENT_KEY(end);
			elem = (CollecTypeValue *) GET_COLLECT_ELEMENT_VALUE(end);
			free_collectype_value(elem, datatype);
			pfree(key);
			pfree(ITER_REF(end));
			c_map_erase(map, end);
		}
	}
	else
	{
		CollectVarrayValue	*varray;
		int 	index;
		int		i;

		varray = (CollectVarrayValue*) DatumGetPointer(collect_value->values);
		index = varray->current_used - 1;

		size = varray->current_used;

		if (num < 0)
			ereport(ERROR,
				(errcode(ERRCODE_VALUE_ERROR),
				errmsg("numeric or value error")));
		if (num > size)
			ereport(ERROR,
				(errcode(ERRCODE_SUBSCRIPT_BEYOND_COUNT),
				errmsg("subscript beyond count")));
		datatype = collect_value->type->typdatatype;

		for (i = num; i > 0; i--)
		{
			CollecTypeValue		*elem;

			elem = (CollecTypeValue *) DatumGetPointer(varray->value[index]);
			free_collectype_value(elem, datatype);
			varray->value[index--] = (Datum) 0;
		}

		varray->current_used = varray->current_used - num;
		collect_value->max_index = collect_value->max_index - num;
		update_varray_memory(collect_value);
	}

	PG_RETURN_DATUM(collect);
}

/*
 * EXTEND Collection Method
 */
Datum
collection_extend(PG_FUNCTION_ARGS)
{
	Datum				collect = PG_GETARG_DATUM(0);
	MemoryContext		old;
	CollectTypeData		*collect_value;
	/* Begin - BUG#Z7715 */
	Oid 				keytypoid,
						valuetypoid;
	int 				keytypmod,
						valuetypmod;
	bool				notnull;
	Datum				new_value = PointerGetDatum(NULL);
	bool				null_value = true;
	/* End - BUG#Z7715 */

	collect_value = (CollectTypeData *) DatumGetPointer(collect);

	CHECK_COLLECTION_IS_NULL(collect_value);

	/* Begin - BUG#Z7715 */
	check_extend(collect_value, 1);

	/* get type info */
	get_collectype_typeinfo(collect_value->type, &keytypoid, &keytypmod,
								&valuetypoid, &valuetypmod, &notnull);

	/* End - BUG#Z7715 */

	Assert(collect_value->valCxt != NULL);
	old = MemoryContextSwitchTo(collect_value->valCxt);

	collect_value->max_index++;

	/* Begin - BUG#Z7715 */
	if (valuetypoid == ASSOCIATIVE_ARRAYOID)
	{
		/* do init for associate array */
		new_value = init_collectype_var(PointerGetDatum(collect_value->type->typdatatype));
		null_value = false;
	}
	/* End - BUG#Z7715 */

	insert_collectype_data(collect_value,
			Int32GetDatum(collect_value->max_index),
			/* Begin - BUG#Z7715 */
			new_value,
			null_value,
			/* End - BUG#Z7715 */
			false,
			false);

	MemoryContextSwitchTo(old);

	PG_RETURN_DATUM(collect);
}

/*
 * EXTEND(n) Collection Method
 */
Datum
collection_extend_n(PG_FUNCTION_ARGS)
{
	Datum			collect = PG_GETARG_DATUM(0);
	int32			num;
	Oid 			num_oid = get_fn_expr_argtype(fcinfo->flinfo, 1);
	CollectTypeData		*collect_value;
	MemoryContext	old;
	Oid 			keytypoid,
					valuetypoid;
	int 			keytypmod,
					valuetypmod;
	bool			notnull;

	collect_value = (CollectTypeData*) DatumGetPointer(collect);

	CHECK_COLLECTION_IS_NULL(collect_value);

	/* if the second argument is NULL, do nothing */
	if (PG_ARGISNULL(1))
		PG_RETURN_DATUM(PointerGetDatum(collect_value));

	/* do cast the num */
	num = DatumGetInt32(doCastValue(PG_GETARG_DATUM(1), num_oid, INT4OID, -1));

	/* the number of extend is large than 0 */
	if (num < 0)
		ereport(ERROR,
			(errcode(ERRCODE_VALUE_ERROR),
			errmsg("numeric or value error")));

	Assert(collect_value->valCxt != NULL);

	/* get type info */
	get_collectype_typeinfo(collect_value->type, &keytypoid, &keytypmod,
								&valuetypoid, &valuetypmod, &notnull);
	/* Begin - BUG#Z7715 */
	check_extend(collect_value, num);
	/* End - BUG#Z7715 */

	old = MemoryContextSwitchTo(collect_value->valCxt);

	for (; 0 < num; num--)
	{
		Datum 	new_value = PointerGetDatum(NULL);
		bool	null_value = true;

		collect_value->max_index++;

		if (valuetypoid == ASSOCIATIVE_ARRAYOID)
		{
			/* do init for associate array */
			new_value = init_collectype_var(PointerGetDatum(collect_value->type->typdatatype));
			null_value = false;
		}

		insert_collectype_data(collect_value,
						   Int32GetDatum(collect_value->max_index),
						   new_value,
						   null_value,
						   false,
						   false);
	}

	MemoryContextSwitchTo(old);

	PG_RETURN_DATUM(collect);
}

/*
 * EXTEND(n, i) Collection Method
 */
Datum
collection_extend_ni(PG_FUNCTION_ARGS)
{
	Datum				collect = PG_GETARG_DATUM(0);
	int32				num;
	int32				idx = DatumGetInt32(PG_GETARG_DATUM(2));
	Oid 				num_oid = get_fn_expr_argtype(fcinfo->flinfo, 1);
	Oid 				idx_oid = get_fn_expr_argtype(fcinfo->flinfo, 2);
	CollectTypeData			*collect_value;
	MemoryContext		old;
	bool				byval;
	int32				typlen;
	Oid 				keytypoid,
						valuetypoid;
	int 				keytypmod,
						valuetypmod;
	bool				notnull;
	Datum				value = (Datum)0;

	collect_value = (CollectTypeData *) DatumGetPointer(collect);

	CHECK_COLLECTION_IS_NULL(collect_value);

	/* if the second or third arguments is NULL, do nothing */
	if (PG_ARGISNULL(1))
		PG_RETURN_DATUM(collect);

	if (PG_ARGISNULL(2))
		PG_RETURN_DATUM(collect);

	/* get type info */
	get_collectype_typeinfo(collect_value->type, &keytypoid, &keytypmod,
								&valuetypoid, &valuetypmod, &notnull);

	/* do cast */
	num = DatumGetInt32(doCastValue(PG_GETARG_DATUM(1),
								num_oid,
								INT4OID,
								-1));
	idx = doCastValue(idx,
					idx_oid,
					keytypoid,
					keytypmod);

	/* the num must be larger than 0 */
	if (num < 0)
		ereport(ERROR,
			(errcode(ERRCODE_VALUE_ERROR),
			errmsg("numeric or value error")));

	if (num == 0)
		PG_RETURN_DATUM(PointerGetDatum(collect));

	if (idx <= 0)
		ereport(ERROR,
			(errcode(ERRCODE_SUBSCRIPT_OUTSIDE_LIMIT),
			errmsg("subscript outside limit")));

	if (idx > collect_value->max_index)
		ereport(ERROR,
			(errcode(ERRCODE_SUBSCRIPT_BEYOND_COUNT),
			errmsg("subscript beyond count")));

	if (!IS_VARRAY(collect_value))
	{
		c_map			*map;
		c_iterator		iter;
		c_iterator		end;
		CollecTypeKey 		*key;

		map = (c_map*) DatumGetPointer(collect_value->values);

		key = (CollecTypeKey *) palloc0(sizeof(CollecTypeKey));
		key->value = Int32GetDatum(idx);

		end = c_map_end(map);
		iter = c_map_find(map, (void*) key);

		pfree(key);

		if (ITER_EQUAL(iter, end))
			ereport(ERROR,
				(errcode(ERRCODE_NO_DATA_FOUND),
				errmsg("data not found")));
		else
			value = PointerGetDatum(GET_COLLECT_ELEMENT_VALUE(iter));
	}
	else
	{
		CollectVarrayValue	*varray = (CollectVarrayValue*) DatumGetPointer(collect_value->values);
		int				idx_value = Int32GetDatum(idx);

		if (idx_value > varray->current_used)
			ereport(ERROR,
				(errcode(ERRCODE_SUBSCRIPT_BEYOND_COUNT),
				errmsg("subscript beyond count")));

		value = varray->value[idx_value - 1];
	}

	byval = get_typbyval(valuetypoid);
	typlen = get_typlen(valuetypoid);

	/* Begin - BUG#Z7715 */
	check_extend(collect_value, num);
	/* End - BUG#Z7715 */

	Assert(collect_value->valCxt != NULL);
	old = MemoryContextSwitchTo(collect_value->valCxt);

	/* copy the given index elements */
	for (; 0 < num; num--)
	{
		CollecTypeValue *elem_i = (CollecTypeValue*) value;
		Datum		 dst_value = (Datum)0;

		collect_value->max_index++;
		if (!elem_i->isnull)
			dst_value = datumCopy(elem_i->value, byval, typlen);

		insert_collectype_data(collect_value,
							Int32GetDatum(collect_value->max_index),
							dst_value,
							elem_i->isnull,
							false,
							elem_i->placeholder);
	}

	MemoryContextSwitchTo(old);

	PG_RETURN_DATUM(collect);
}

/*
 * compare two nested tables are equality
 */
Datum
nested_table_eq(PG_FUNCTION_ARGS)
{
	Datum collect1_value = PG_GETARG_DATUM(0);
	Datum collect2_value = PG_GETARG_DATUM(1);

	return BoolGetDatum(collection_value_compare(collect1_value, collect2_value));
}

/*
 * compare two nested tables are inequality
 */
Datum
nested_table_ne(PG_FUNCTION_ARGS)
{
	Datum collect1_value = PG_GETARG_DATUM(0);
	Datum collect2_value = PG_GETARG_DATUM(1);

	return BoolGetDatum(!collection_value_compare(collect1_value, collect2_value));
}

/*
 * do compare two nested tables
 */
static bool
collection_value_compare(Datum value1, Datum value2)
{
	CollectTypeData	*collect_data1 = (CollectTypeData *)PointerGetDatum(value1);
	CollectTypeData	*collect_data2 = (CollectTypeData *)PointerGetDatum(value2);
	CollectTypeInfo	*datatype1;
	int32			size1;
	int32			size2;
	c_iterator		iter1;
	c_iterator		end1;
	c_iterator		end2;
	c_map			*map1;
	c_map			*map2;
	int 			matched_count = 0;
	bool			*visited;

	if (collect_data1 == NULL || collect_data2 == NULL)
		return false;

	if (!IS_NESTED_TABLE(collect_data1) || !IS_NESTED_TABLE(collect_data2))
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				errmsg("unsupported operation")));

	datatype1 = collect_data1->type->typdatatype;

	/* Begin - BUG#Z7710 */
	if (collect_data1->type->typoid != collect_data2->type->typoid)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("wrong number or types of arguments")));
	else if (pg_strcasecmp(collect_data1->type->typname, collect_data2->type->typname) != 0)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("wrong number or types of arguments")));
	/* End - BUG#Z7710 */

	map1 = (c_map *) DatumGetPointer((c_map *) collect_data1->values);
	map2 = (c_map *) DatumGetPointer((c_map *) collect_data2->values);
	size1 = c_map_size(map1);
	size2 = c_map_size(map2);

	if (size1 != size2)
		return false;

	iter1 = c_map_begin(map1);
	end1 = c_map_end(map1);
	end2 = c_map_end(map2);

	visited = (bool *)palloc0(sizeof(bool) * size2);

	/* loop each elements of the first nested table */
	for(; !ITER_EQUAL(iter1, end1); ITER_INC(iter1))
	{
		CollecTypeValue 	*src1_value;
		c_iterator		iter2;
		bool			found = false;

		int 			idx;

		memset(visited, 0, sizeof(bool) * size2);
		src1_value = (CollecTypeValue *) GET_COLLECT_ELEMENT_VALUE(iter1);

		iter2 = c_map_begin(map2);
		idx = 0;

		for(; !ITER_EQUAL(iter2, end2); ITER_INC(iter2))
		{
			CollecTypeValue 	*src2_value;

			src2_value = (CollecTypeValue *) GET_COLLECT_ELEMENT_VALUE(iter2);

			if (!visited[idx])
			{
				if (IS_PLSQL_NESTED_TABLE(datatype1->typtype))
				{
					if (collection_value_compare(src1_value->value,
											src2_value->value))
					{
						visited[idx++] = true;
						found = true;
						matched_count++;
						break;
					}
					else
						idx++;
				}
				else if ((collection_element_compare(src1_value->value,
											src2_value->value,
											datatype1->typoid)) &&
											(src1_value->placeholder == src2_value->placeholder))
				{
					visited[idx++] = true;
					found = true;
					matched_count++;
					break;
				}
			}
			else
				idx++;
		}

		/* the element of first nested table is not found in the second nested table */
		if (!found)
			return false;
	}
	pfree(visited);

	return  matched_count == size1;
}

static bool
collection_element_compare(Datum value1, Datum value2, Oid typoid)
{
	TypeCacheEntry		*typentry = NULL;
	bool				result = false;

	typentry = lookup_type_cache(typoid, TYPECACHE_EQ_OPR_FINFO);
	if (typentry == NULL || !OidIsValid(typentry->eq_opr_finfo.fn_oid))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_FUNCTION),
				 errmsg("could not identify an equality operator for type %s",
						format_type_be(typoid))));

	result = FunctionCall2Coll(&(typentry->eq_opr_finfo), typentry->typcollation, value1, value2);

	return result;
}

/*
 * get the collection typename from the CollectType
 */
Datum
get_collectype_name(Datum value)
{
	CollectType collect_data = (CollectType)DatumGetPointer(value);

	PG_RETURN_DATUM(CStringGetDatum(collect_data->type->typname));
}

/*
 * get the collection typmod from the CollectType
 */
Datum
get_collectype_mode(Datum value)
{
	CollectType collect_data = (CollectType)DatumGetPointer(value);

	PG_RETURN_DATUM(Int32GetDatum(collect_data->type->typmod));
}

/*
 * get the collection valCxt from the CollectType
 */
MemoryContext
get_collectype_mcxt(Datum value)
{
	CollectType collect_data = (CollectType)DatumGetPointer(value);

	return collect_data->valCxt;
}

/*
 * init a heaptuple if the collect element datatype is record or row
 * typoid is the element datatype oid,
 * typmod is the element datatype typmod,
 * pl_datatype is the element datatype.
 */
static Datum
init_heaptuple(Oid typoid,
			int32 typmod,
			PLiSQL_type *pl_datatype)
{
	Datum val_datum;

	if (pl_datatype == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_VALUE_ERROR),
				errmsg("the datatype is null")));

	if (typoid == ORARECORDOID)
	{
		int 		natts;
		HeapTuple	newtuple;
		Datum	   *dvalues;
		bool	   *nulls;
		int 		i;
		TupleDesc	newDesc;
		ListCell	*cell;
		PLiSQL_ora_record_type *ora_rec_type;
		HeapTupleHeader newheader;

		ora_rec_type = (PLiSQL_ora_record_type *)pl_datatype;
		natts = list_length(ora_rec_type->args) + 1;

		newDesc = CreateTemplateTupleDesc(natts, false);
		newDesc->tdtypeid = ORARECORDOID;
		TupleDescInitEntry(newDesc, 1,
						"__###__",
						UDTPOINTEROID, -1,
						0);
		i = 1;
		foreach(cell, ora_rec_type->args)
		{
			PLiSQL_ora_record_argitem *argitem;

			argitem = (PLiSQL_ora_record_argitem *) lfirst(cell);
			TupleDescInitEntry(newDesc, i + 1,
							argitem->argname,
							argitem->type->typoid,
							argitem->type->atttypmod,
							0);
			i++;
		}

		dvalues = (Datum *) palloc0(natts * sizeof(Datum));
		nulls = (bool *) palloc0(natts * sizeof(bool));
		for (i = 0; i < natts; i++)
			nulls[i] = true;

		dvalues[0] = PointerGetDatum(ora_rec_type);
		nulls[0] = false;

		assign_record_type_typmod(newDesc);
		newtuple = heap_form_tuple(newDesc, dvalues, nulls);

		newheader = (HeapTupleHeader) palloc(HeapTupleHeaderGetDatumLength(newtuple->t_data));
		memcpy(newheader, newtuple->t_data, HeapTupleHeaderGetDatumLength(newtuple->t_data));
		pfree(newtuple);
		val_datum = PointerGetDatum(newheader);

		pfree(dvalues);
		pfree(nulls);
	}
	else if (get_typtype(typoid) == TYPTYPE_COMPOSITE ||
		(typoid == RECORDOID && pl_datatype->ttype == PLISQL_TTYPE_EXPLICIT_CURSOR))
	{
		int 		natts;
		HeapTuple	newtuple;
		Datum	    *dvalues;
		bool	    *nulls;
		int 		i;
		TupleDesc	tupdesc;
		HeapTupleHeader newheader;

		switch(pl_datatype->ttype)
		{
			case PLISQL_TTYPE_UDT_REF_CURSOR:
				{
					PLiSQL_ref_cursor_type *ref_type = (PLiSQL_ref_cursor_type *) pl_datatype;

					elog(ERROR, "invalid cursor return type; '%s' must be a record type",
							ref_type->type.typname);
				}
				break;
			case PLISQL_TTYPE_EXPLICIT_CURSOR:
				{
					PLiSQL_explicit_cursor_type  *cursor_type = (PLiSQL_explicit_cursor_type *) pl_datatype;

					tupdesc = cursor_type->retdesc;
				}
				break;
			default:
				/* table or view */
				{
					tupdesc = lookup_rowtype_tupdesc(typoid, typmod);
				}
				break;
		}

		natts = tupdesc->natts;
		dvalues = (Datum *) palloc0(natts * sizeof(Datum));
		nulls = (bool *) palloc0(natts * sizeof(bool));
		for (i = 0; i < natts; i++)
			nulls[i] = true;

		if (tupdesc != NULL &&
			tupdesc->tdtypeid == RECORDOID)
			assign_record_type_typmod(tupdesc);

		newtuple = heap_form_tuple(tupdesc, dvalues, nulls);

		newheader = (HeapTupleHeader) palloc(HeapTupleHeaderGetDatumLength(newtuple->t_data));
		memcpy(newheader, newtuple->t_data, HeapTupleHeaderGetDatumLength(newtuple->t_data));
		pfree(newtuple);
		val_datum = PointerGetDatum(newheader);

		ReleaseTupleDesc(tupdesc);

		pfree(dvalues);
		pfree(nulls);
	}
	else
		ereport(ERROR,
				(errcode(ERRCODE_VALUE_ERROR),
				errmsg("the datatype is not a row or record")));

	return val_datum;
}

/* Begin - ReqID:SRS-PLSQL-BULKCOLLECT */
/*
 *  get_collect_value2 is similar to get_collect_value,
 *  except that it contains the case of record field.
 *	return collect value in certain dimensional by nsubscripts.
 *  datum: top dimensional collect value.
 *  nsubscripts: the count of the subscripts
 *  subscriptvals: subscript values
 *  fieldnames is the array of field names,
 *  top_datatype is the datatype of top collection
 */
Datum
get_collect_value2(Datum datum,
				int nsubscripts,
				Datum *subscriptvals,
				List **fieldnames,
				void *top_datatype)
{
	int				i = 0;
	CollectTypeInfo	*type;
	CollectType		val = (CollectType)PointerGetDatum(datum);
	CollecTypeKey	*key = NULL;
	CollecTypeValue	*elem_val = NULL;

	CHECK_COLLECTION_IS_NULL(val);
	type = val->type;

	Assert(nsubscripts >= 0);
	while (i < nsubscripts)
	{
		Assert(val->valCxt != NULL);
		Assert(IS_PLSQL_COLLECTYPE(type->typtype));

		/* sanity check */
		if (val->collect_typtype == NESTED_TABLE ||
			val->collect_typtype == VARRAY)
		{
			if (DatumGetInt32(subscriptvals[i]) <= 0)
				ereport(ERROR,
					(errcode(ERRCODE_SUBSCRIPT_OUTSIDE_LIMIT),
					errmsg("subscript outside limit")));
			else if (DatumGetInt32(subscriptvals[i]) > val->max_index)
				ereport(ERROR,
					(errcode(ERRCODE_SUBSCRIPT_BEYOND_COUNT),
					errmsg("subscript beyond count")));
		}

		if (!IS_VARRAY(val))
		{
			c_map			*map;
			c_iterator		iter;
			c_iterator		end;

			key = (CollecTypeKey *) palloc0(sizeof(CollecTypeKey));
			key->value = subscriptvals[i];

			map = (c_map*) DatumGetPointer(val->values);
			end = c_map_end(map);

			iter = c_map_find(map, (void*) key);
			if (ITER_EQUAL(iter, end))
			{
				pfree(key);
				ereport(ERROR,
					(errcode(ERRCODE_NO_DATA_FOUND),
					errmsg("data not found")));
			}

			elem_val = (CollecTypeValue *) PointerGetDatum(GET_COLLECT_ELEMENT_VALUE(iter));
			pfree(key);
		}
		else
		{
			int32			key_val;
			CollectVarrayValue	*varray;

			key_val = DatumGetInt32(subscriptvals[i]);
			varray = (CollectVarrayValue *) DatumGetPointer(val->values);

			/* Begin - BUG#Z7714 */
			if (key_val <= 0 || key_val > varray->current_used)
				ereport(ERROR,
					(errcode(ERRCODE_SUBSCRIPT_OUTSIDE_LIMIT),
					errmsg("subscript outside limit")));
			/* End - BUG#Z7714 */

			elem_val = (CollecTypeValue *) DatumGetPointer(varray->value[key_val - 1]);
		}

		if (IS_PLSQL_COLLECTYPE(type->typdatatype->typtype))
		{
			val = (CollectType) DatumGetPointer(elem_val->value);
			type = type->typdatatype;
		}
		else if (type_is_rowtype(type->typdatatype->typoid) &&
			fieldnames != NULL &&
			fieldnames[nsubscripts - i - 1] != NULL)
		{
			ListCell 	*lc;
			List 		*names = fieldnames[nsubscripts - i - 1];
			Datum		cur_val;
			int			j;
			int			len;
			Oid 		fieldOid = InvalidOid;

			cur_val = elem_val->value;
			len = list_length(names);

			j = 0;

			/* there are fields after the subscript */
			foreach(lc, names)
			{
				HeapTupleHeader td;
				Oid 		tupType;
				int32		tupTypmod;
				TupleDesc	tupdesc;
				HeapTupleData tmptup;
				HeapTuple	tup;
				int 		fno = 0;
				bool		isnull;
				char		*fieldname = (char *)lfirst(lc);

				td = DatumGetHeapTupleHeader(cur_val);

				/* Extract rowtype info and find a tupdesc */
				tupType = HeapTupleHeaderGetTypeId(td);
				tupTypmod = HeapTupleHeaderGetTypMod(td);
				tupdesc = lookup_rowtype_tupdesc(tupType, tupTypmod);

				/* Build a temporary HeapTuple control structure */
				tmptup.t_len = HeapTupleHeaderGetDatumLength(td);
				ItemPointerSetInvalid(&(tmptup.t_self));
				tmptup.t_tableOid = InvalidOid;
				tmptup.t_data = td;
				tup = &tmptup;

				fno = SPI_fnumber(tupdesc, fieldname);
				if (fno <= 0)
				{
					char	name[(NAMEDATALEN * 2) + 100];

					snprintf(name, sizeof(name), "%s.%s", type->typdatatype->typname, fieldname);
					fno = SPI_fnumber(tupdesc, name);
				}
				if (fno <= 0)
					ereport(ERROR,
							(errcode(ERRCODE_UNDEFINED_COLUMN),
							 errmsg("record  has no field \"%s\"", fieldname)));

				cur_val = heap_getattr(tup, fno, tupdesc, &isnull);
				fieldOid = tupdesc->attrs[fno - 1].atttypid;
				j++;
				if (j < len  && !type_is_rowtype(fieldOid))
					ereport(ERROR,
							(errcode(ERRCODE_UNDEFINED_COLUMN),
							 errmsg("field \"%s\" is not a record or a row ", fieldname)));
				ReleaseTupleDesc(tupdesc);
			}

			if (IS_COLLECTOID(fieldOid))
			{
				val = (CollectType) DatumGetPointer(cur_val);
				CHECK_COLLECTION_IS_NULL(val);
				type = val->type;
			}
			else if (i < nsubscripts)
				ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("not a collection")));
			else
				ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("not a collection")));
		}
		i++;
	}

	return PointerGetDatum(val);
}

/*
 * init the collection target IN BULK COLLECT INTO statement
 */
Datum
init_bulk_collect_data(Datum datum,
					int nSubscripts,
					Datum *subscriptvals,
					List **fieldnames,
					Datum *keytypes,
					void *root_datatype,
					void *target_datatype,
					MemoryContext cxt)
{
	BulkCollectIntoData *collect_data;
	Datum		init_value;
	int			nth;

	collect_data = (BulkCollectIntoData *)palloc0(sizeof(BulkCollectIntoData));
	collect_data->target_datatype = target_datatype;

	if (nSubscripts == 0)
		collect_data->target_value = datum;
	else
	{
		init_value = init_collectype_var(pl_type_to_collectinfo((PLiSQL_type *)target_datatype));

		nth = 1;
		set_collect_value(datum,
						nSubscripts,
						subscriptvals,
						&nth,
						fieldnames,
						root_datatype,
						init_value,
						false);

		collect_data->target_value = get_collect_value2(datum,
														nSubscripts,
														subscriptvals,
														fieldnames,
														root_datatype);
	}

	return PointerGetDatum(collect_data);
}

/*
 * init the collection target in BULK COLLECT INTO statement
 */
Datum
init_bulk_collect_params(int num, Datum *collect_data)
{
	BulkCollectIntoParams *params;

	params = (BulkCollectIntoParams *)palloc0(sizeof(BulkCollectIntoParams));
	params->collect_num = num;
	params->collect_data = collect_data;

	return PointerGetDatum(params);
}

/*
 * set the DR_bulkcollect->bulk_params for BULK COLLECT statement
 */
void
set_bulk_collect_destreceiver_params(Datum dest, Datum bulk_params)
{
	DR_bulkcollect *self = (DR_bulkcollect *)DatumGetPointer(dest);

	self->bulk_params = (BulkCollectIntoParams *)DatumGetPointer(bulk_params);
}


/*
 * create_bulk_collect_receiver -- create a DestReceiver object for BULK COLLECT statement
 */
Datum
create_bulk_collect_receiver(void)
{
	DR_bulkcollect *self = (DR_bulkcollect *) palloc0(sizeof(DR_bulkcollect));

	self->pub.receiveSlot = bulk_collect_into_receive;
	self->pub.rStartup = bulk_collect_into_startup;
	self->pub.rShutdown = bulk_collect_into_shutdown;
	self->pub.rDestroy = bulk_collect_into_destroy;
	self->pub.mydest = DestBulkCollect;
	self->bulk_params = NULL;
	/* Begin - ReqID:SRS-PLSQL-FORALL */
	self->in_forall = false;
	/* End - ReqID:SRS-PLSQL-FORALL */

	return PointerGetDatum(self);
}

/* Begin - ReqID:SRS-PLSQL-FORALL */
/*
 * set_bulk_collect_destreceiver_inforall -- set in_forall to true
 */
void
set_bulk_collect_destreceiver_inforall(Datum dest)
{
	DR_bulkcollect *self = (DR_bulkcollect *)DatumGetPointer(dest);

	self->in_forall = true;
}
/* End - ReqID:SRS-PLSQL-FORALL */

/*
 * Check whether the numbers of the bulk collect target
 * matches the number of result columns.
 */
static void
check_bulkcollect_target_number_match(int target_number, int source_number)
{
	if (target_number > source_number)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				errmsg("too many values")));

	if (target_number < source_number)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				errmsg("not enough values")));
}

/*
 * bulk_collect_into_receive
 *		store tuple retrieved by Executor into collection(BULK COLLECT INTO)
 */
static bool
bulk_collect_into_receive(TupleTableSlot *slot, DestReceiver *self)
{
	DR_bulkcollect			*bulk_self = (DR_bulkcollect*)self;
	BulkCollectIntoParams	*bulk_params;
	TupleDesc				tupdesc;
	int 					i = 0;

	bulk_params = bulk_self->bulk_params;
	tupdesc = slot->tts_tupleDescriptor;

	/* make sure the tuple is fully deconstructed */
	slot_getallattrs(slot);

	/* loop by the target list */
	for (i = 0; i < bulk_params->collect_num; i++)
	{
		MemoryContext			oldcxt = NULL;
		BulkCollectIntoData 	*collect_data;
		CollectType 			val;
		Oid 					value_typoid;
		int32					value_typmod;
		PLiSQL_type 			*value_datatype;
		bool					isnull;
		Datum					value;

		collect_data = (BulkCollectIntoData *)DatumGetPointer(bulk_params->collect_data[i]);

		/* get the collect variable value */
		val = (CollectType)DatumGetPointer(collect_data->target_value);

		value_typoid = val->type->typdatatype->typoid;
		value_typmod = val->type->typdatatype->typmod;
		value_datatype = ((PLiSQL_ora_collection_type *)collect_data->target_datatype)->typdatatype;

		oldcxt = MemoryContextSwitchTo(val->valCxt);

		/*
		 * handle the cases when the collection value type are %ROWTYPE or record, e.g.
		 * DECLARE
		 *	 TYPE dept_tab_type IS TABLE OF departments%ROWTYPE;
		 *	 v_depts dept_tab_type;
		 * BEGIN
		 *	 SELECT * BULK COLLECT INTO v_depts FROM departments;
		 * END;
		 */
		if (type_is_rowtype(value_typoid))
		{
			int 		rowtype_natts;

			/* get the numbers of row type's field */
			if (value_typoid == ORARECORDOID)
			{
				PLiSQL_ora_record_type *type;

				type = (PLiSQL_ora_record_type *)value_datatype;
				rowtype_natts = list_length(type->args);

				check_bulkcollect_target_number_match(rowtype_natts, slot->tts_nvalid);
				if (TTS_EMPTY(slot) || slot->tts_isnull[i])
				{
					isnull = true;
					value = (Datum) 0;
				}
				else if (value_typoid == ORARECORDOID)
				{
					Datum		*values;
					bool		*nulls;
					int 		natts;
					int 		j;
					HeapTuple	newtup = NULL;
					TupleDesc	newtupdesc;

					natts = tupdesc->natts + 1;
					values = palloc(sizeof(Datum) * natts);
					nulls = palloc(sizeof(bool) * natts);
					memset(nulls, false, sizeof(bool) * natts);
					newtupdesc = CreateTemplateTupleDesc(natts, false);
					newtupdesc->tdtypeid = ORARECORDOID;
					TupleDescInitEntry(newtupdesc, 1,
									"__###__",
									UDTPOINTEROID, -1,
									0);
					TupleDescInitEntryCollation(newtupdesc,
												1,
												InvalidOid);

					values[0] = PointerGetDatum(value_datatype);
					nulls[0] = false;

					for (j = 1; j < natts; j++)
					{
						PLiSQL_type *field_type;

						field_type = ((PLiSQL_ora_record_argitem *)list_nth(((PLiSQL_ora_record_type *)value_datatype)->args, j - 1))->type;

						/* cast the value */
						values[j] = datum_type_cast(slot->tts_values[j - 1],
												tupdesc->attrs[j - 1].atttypid, tupdesc->attrs[j - 1].atttypmod,
												field_type->typoid, field_type->atttypmod);

						/* copy the value */
						copy_tupleAttrsValues(values[j],
										slot->tts_isnull[j - 1],
										field_type->typoid,
										field_type->typbyval,
										field_type->typlen,
										&(values[j]));
						nulls[j] = slot->tts_isnull[j - 1];

						TupleDescInitEntry(newtupdesc, j + 1,
									   field_type->typname,
									   field_type->typoid,
									   field_type->atttypmod,
									   0);
						TupleDescInitEntryCollation(newtupdesc,
													j + 1,
													field_type->collation);
					}

					BlessTupleDesc(newtupdesc);
					newtup = heap_form_tuple(newtupdesc, values, nulls);
					value = heap_copy_tuple_as_datum(newtup, newtupdesc);
					isnull = slot->tts_isnull[i];

					heap_freetuple(newtup);
					pfree(values);
					pfree(nulls);
				}
			}
			else if (get_typtype(value_typoid) == TYPTYPE_COMPOSITE ||
				(value_typoid == RECORDOID &&
				value_datatype->ttype == PLISQL_TTYPE_EXPLICIT_CURSOR))
			{
				TupleDesc	target_tupdesc;

				if (get_typtype(value_typoid) == TYPTYPE_COMPOSITE)
				{
					HeapTuple	tp;

					tp = SearchSysCache1(RELOID, ObjectIdGetDatum(typeidTypeRelid(value_typoid)));
					if (!HeapTupleIsValid(tp))
						ereport(ERROR,
								(errcode(ERRCODE_WRONG_OBJECT_TYPE),
								 errmsg("relation does not have a composite type")));

					rowtype_natts = ((Form_pg_class) GETSTRUCT(tp))->relnatts;
					target_tupdesc = lookup_rowtype_tupdesc(value_typoid, value_typmod);
					ReleaseSysCache(tp);
				}
				else
				{
					PLiSQL_explicit_cursor_type *cursor_type;

					cursor_type = (PLiSQL_explicit_cursor_type *) value_datatype;
					target_tupdesc = cursor_type->retdesc;
					rowtype_natts = target_tupdesc->natts;
				}

				check_bulkcollect_target_number_match(rowtype_natts, slot->tts_nvalid);

				if (TTS_EMPTY(slot) || slot->tts_isnull[i])
				{
					isnull = true;
					value = (Datum) 0;
				}
				else
				{
					Datum		*values;
					bool		*nulls;
					int 		j;
					HeapTuple	newtup = NULL;

					values = palloc(sizeof(Datum) * rowtype_natts);
					nulls = palloc(sizeof(bool) * rowtype_natts);
					for (j = 0; j < rowtype_natts; j++)
					{
						/* cast the value */
						values[j] = datum_type_cast(slot->tts_values[j],
												tupdesc->attrs[j].atttypid, tupdesc->attrs[j].atttypmod,
												target_tupdesc->attrs[j].atttypid, target_tupdesc->attrs[j].atttypmod);

						/* copy the value */
						copy_tupleAttrsValues(values[j],
										slot->tts_isnull[j],
										target_tupdesc->attrs[j].atttypid,
										target_tupdesc->attrs[j].attbyval,
										target_tupdesc->attrs[j].attlen,
										&(values[j]));
						nulls[j] = slot->tts_isnull[j];
					}

					BlessTupleDesc(target_tupdesc);
					newtup = heap_form_tuple(target_tupdesc, values, nulls);
					value = heap_copy_tuple_as_datum(newtup, target_tupdesc);
					isnull = slot->tts_isnull[i];

					if (get_typtype(value_typoid) == TYPTYPE_COMPOSITE)
						ReleaseTupleDesc(target_tupdesc);

					heap_freetuple(newtup);
					pfree(values);
					pfree(nulls);
				}
			}
			else
				ereport(ERROR,
						(errcode(ERRCODE_WRONG_OBJECT_TYPE),
						 errmsg("not a composite type or record type")));

			collect_insert(PointerGetDatum(val), Int32GetDatum(++(val->max_index)), value, isnull);
			break;
		}
		else
		{
			/*
			 * Check whether the number of BULK COLLECT INTO target lists
			 * matches the number of result columns.
			 */
			check_bulkcollect_target_number_match(bulk_params->collect_num, slot->tts_nvalid);

			if (TTS_EMPTY(slot) || slot->tts_isnull[i])
			{
				isnull = true;
				value = (Datum) 0;
			}
			else
			{
				value = datum_type_cast(slot->tts_values[i],
										tupdesc->attrs[i].atttypid, tupdesc->attrs[i].atttypmod,
										value_typoid, value_typmod);
				value = datumCopy(value, get_typbyval(value_typoid), get_typlen(value_typoid));
				isnull = slot->tts_isnull[i];
			}
			collect_insert(PointerGetDatum(val), Int32GetDatum(++(val->max_index)), value, isnull);
		}

		MemoryContextSwitchTo(oldcxt);
	}

	return true;
}

/*
 * bulk_collect_into_startup --- query startup for BULK COLLECT INTO
 */
static void
bulk_collect_into_startup(DestReceiver *self, int operation, TupleDesc typeinfo)
{
	/* no-op */
}

/*
 * bulk_collect_into_shutdown --- executor end
 */
static void
bulk_collect_into_shutdown(DestReceiver *self)
{
	BulkCollectIntoParams *bulk_params = ((DR_bulkcollect *)self)->bulk_params;

	if (bulk_params != NULL && !((DR_bulkcollect *)self)->in_forall/* ReqID:SRS-PLSQL-FORALL */)
	{
		int i = 0;

		for (i = 0; i < bulk_params->collect_num; i++)
		{
			BulkCollectIntoData	*collect_data = (BulkCollectIntoData *)DatumGetPointer(bulk_params->collect_data[i]);
			if (collect_data)
				pfree(collect_data);
		}

		pfree(bulk_params->collect_data);
		bulk_params->collect_data = NULL;

		pfree(bulk_params);
		bulk_params = NULL;
	}

	pfree(self);
}

/*
 * bulk_collect_into_destroy --- release DestReceiver object
 */
static void
bulk_collect_into_destroy(DestReceiver *self)
{
	if (self != NULL)
		bulk_collect_into_shutdown(self);
}

/*
 * datum_type_cast --- Cast a value if required
 */
static Datum
datum_type_cast(Datum value,
				Oid valtype, int32 valtypmod,
				Oid reqtype, int32 reqtypmod)
{
	Oid 		typOutput;
	bool		typIsVarlena;
	Oid 		typInput;
	Oid 		typIOParam;
	char		*str_value;
	Datum		retult = value;

	if (valtype == reqtype && valtypmod == reqtypmod)
		return retult;

	if (!OidIsValid(valtype))
		elog(ERROR, "could not determine data source type(%d) input", valtype);

	getTypeOutputInfo(valtype, &typOutput, &typIsVarlena);
	str_value = OidOutputFunctionCall(typOutput, value);

	getTypeInputInfo(reqtype, &typInput, &typIOParam);
	retult = OidInputFunctionCall(typInput, str_value, typIOParam, reqtypmod);

	return retult;
}
/* End - ReqID:SRS-PLSQL-BULKCOLLECT */

/* Begin - ReqID:SRS-PLSQL-FORALL */
/*
 * get_collect_all_values - get all the collect element values
 */
List *
get_collect_all_values(Datum collect_value)
{
	List 			*values_list = NIL;
	CollectType		val = (CollectType)PointerGetDatum(collect_value);

	CHECK_COLLECTION_IS_NULL(val);
	Assert(val->valCxt != NULL);

	if (!IS_VARRAY(val))
	{
		c_map			*map;
		c_iterator		iter;
		c_iterator		end;

		map = (c_map*) DatumGetPointer(val->values);

		if (c_map_empty(map))
			return values_list;

		iter = c_map_begin(map);
		end = c_map_end(map);

		/* loop each elements of associate array or nested table */
		for(; !ITER_EQUAL(iter, end); ITER_INC(iter))
		{
			CollecTypeValue 	*elem_val;

			elem_val = (CollecTypeValue *) GET_COLLECT_ELEMENT_VALUE(iter);

			if (!elem_val->placeholder)
				values_list= lappend_int(values_list, DatumGetInt64(elem_val->value));
		}
	}
	else
	{
		CollectVarrayValue	*varray;
		int		i = 0;

		varray = (CollectVarrayValue *) DatumGetPointer(val->values);

		for (i = 0; i < varray->current_used; i++)
		{
			CollecTypeValue 	*elem_val;

			elem_val = (CollecTypeValue *) DatumGetPointer(varray->value[i]);
			values_list= lappend_int(values_list, DatumGetInt64(elem_val->value));
		}
	}

	return values_list;
}

/*
 * get_collect_all_key_values - get all the collect key values
 */
List *
get_collect_all_key_values(Datum collect_value, bool has_between_and,
						int lower_bound, int upper_bound)
{
	List 			*keys_list = NIL;
	CollectType		val = (CollectType)PointerGetDatum(collect_value);

	CHECK_COLLECTION_IS_NULL(val);
	Assert(val->valCxt != NULL);

	if (!IS_VARRAY(val))
	{
		c_map			*map;
		c_iterator		iter;
		c_iterator		begin;
		c_iterator		end;

		map = (c_map*) DatumGetPointer(val->values);

		if (c_map_empty(map))
			return keys_list;

		begin = c_map_begin(map);
		end = c_map_end(map);

		if (has_between_and)
		{
			CollecTypeKey	*lowerbound_key;
			CollecTypeKey	*upperbound_key;
			c_iterator		iter_lowerbound;
			c_iterator		iter_upperbound;

			/* construct the key according the lower_bound value */
			lowerbound_key = (CollecTypeKey *) palloc0(sizeof(CollecTypeKey));
			lowerbound_key->value = lower_bound;

			iter_lowerbound = c_map_find(map, lowerbound_key);

			pfree(lowerbound_key);
			if (!ITER_EQUAL(iter_lowerbound, end))
				begin = iter_lowerbound;

			/* construct the key according the upper_bound value */
			upperbound_key = (CollecTypeKey *) palloc0(sizeof(CollecTypeKey));
			upperbound_key->value = upper_bound;

			iter_upperbound = c_map_find(map, upperbound_key);

			pfree(upperbound_key);
			if (!ITER_EQUAL(iter_upperbound, end))
			{
				end = iter_upperbound;
				ITER_INC(end);
			}
		}

		/* loop each keys of associate array or nested table */
		for(iter = begin; !ITER_EQUAL(iter, end); ITER_INC(iter))
		{
			CollecTypeValue 	*elem_val;

			elem_val = (CollecTypeValue *) GET_COLLECT_ELEMENT_VALUE(iter);

			if (!elem_val->placeholder)
			{
				CollecTypeKey	*key;

				key = (CollecTypeKey *) GET_COLLECT_ELEMENT_KEY(iter);
				keys_list = lappend_int(keys_list, DatumGetInt64(key->value));
			}
		}
	}
	else
	{
		CollectVarrayValue	*varray;
		int		i = 0;
		int		begin;
		int		end;

		varray = (CollectVarrayValue *) DatumGetPointer(val->values);

		begin = 0;
		end = varray->current_used;

		if (has_between_and)
		{
			if (lower_bound > 0)
				begin = lower_bound;

			if (upper_bound > varray->current_used)
				end = varray->current_used;
			else
				end = upper_bound;
		}

		for (i = begin; i < end; i++)
			keys_list = lappend_int(keys_list, DatumGetInt64(i + 1));
	}

	return keys_list;
}
/* End - ReqID:SRS-PLSQL-FORALL */

/* Begin - BUG#Z7715 */
/*
 * Check whether the number of extend elements exceeds the limit of collection
 */
static void
check_extend(CollectTypeData *collect_data, int extend_num)
{
	CollectTypeInfo *collect_type = collect_data->type;
	Datum  collect_values = collect_data->values;

	if (!IS_PLSQL_VARRAY(collect_type->typtype))
	{
		c_map		*map;
		int32		size;
		int32           res;

		map = (c_map *) DatumGetPointer(collect_values);
		size = c_map_size(map);
		if (pg_add_s32_overflow(size, extend_num, &res))
			ereport(ERROR,
				(errcode(ERRCODE_SUBSCRIPT_BEYOND_COUNT),
				errmsg("subscript beyond count")));
	}
	else
	{
		CollectVarrayValue	*varray;

		varray = (CollectVarrayValue*) DatumGetPointer(collect_values);

		if (varray->current_used + extend_num > collect_data->limit_idx)
			ereport(ERROR,
				(errcode(ERRCODE_SUBSCRIPT_OUTSIDE_LIMIT),
				errmsg("subscript outside limit")));
	}
}
/* End - BUG#Z7715 */
