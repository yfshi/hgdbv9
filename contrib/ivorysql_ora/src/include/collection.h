/*-------------------------------------------------------------------------
 *
 * File:collection.h
 *
 * Abstract:
 *		Definitions for the  collection
 *
 * ReqID:SRS-PLSQL-Collection
 *
 *
 * Copyright:
 * Copyright (c) 2009-2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  contrib/ivorysql_ora/src/include/collection.h
 *
 *-------------------------------------------------------------------------
 */

#ifndef __COLLECTION_H__
#define __COLLECTION_H__
#include "fmgr.h"
#include "../tstl2cl/c_pair.h"
#include "../tstl2cl/include/c_iterator.h"

#define VARRAY_RESERVE_AMOUNT(x) (x > 100 ? x / 10 : 10)

#define CAN_VARRAY_REDUCE(varray_total_index, varray_current_used) \
	(varray_total_index > varray_current_used + \
		2 * VARRAY_RESERVE_AMOUNT(varray_current_used))

#define CALC_VARRAY_MAX_RESERVE_INDEX(varray_current_used) \
	( varray_current_used + VARRAY_RESERVE_AMOUNT(varray_current_used))

#define CALC_VARRAY_TOTAL_INDEX(varray_current_used, limit_idx) \
	(CALC_VARRAY_MAX_RESERVE_INDEX(varray_current_used) > limit_idx ? \
		limit_idx : CALC_VARRAY_MAX_RESERVE_INDEX(varray_current_used))


#define IS_VARRAY(data) \
	(data->collect_typtype == VARRAY)
#define IS_NESTED_TABLE(data) \
	(data->collect_typtype == NESTED_TABLE )
#define IS_ASSOCIATIVE_ARRAY(data) \
	(data->collect_typtype == ASSOCIATIVE_ARRAY)

#define CHECK_COLLECTION_IS_NULL(x) \
		if ((x) == NULL) \
		ereport(ERROR,  \
			(errcode(ERRCODE_COLLECTION_IS_NULL), \
			errmsg("Reference to uninitialized collection")));

#define GET_COLLECT_ELEMENT_VALUE(x) ((c_ppair)ITER_REF(x))->second
#define GET_COLLECT_ELEMENT_KEY(x)   ((c_ppair)ITER_REF(x))->first

enum
{
	ASSOCIATIVE_ARRAY,
	NESTED_TABLE,
	VARRAY
};

/* store the type information of the collection type data */
typedef struct CollectTypeInfo
{
	char 					*typname;
	Oid						typoid;
	/* Begin - ReqID:SRS-SQL-Collection */
	Oid 					sqlcollectionoid;	/* OID of the collection type
												 * created by CREATE TYPE statement
												 */
	/* End - ReqID:SRS-SQL-Collection */
	int32					typmod;
	char					typtype;
	bool					typbyval;
	int32					typlen;

	struct CollectTypeInfo	*typkey;
	struct CollectTypeInfo	*typdatatype;

	int						limit;
	bool					datanotnull;
} CollectTypeInfo;

typedef struct CollectTypeInfo*		CollectInfo;

/* store the collection type all data */
typedef struct CollectTypeData
{
	uint8			va_header;		/* Always 0x80 or 0x01 */
	uint8			va_tag;			/* Type of datum */
	int32			va_data_len;	/* data length */

	int32			collect_typtype;/* 0:Associative array, 1: Nested table , 2: Varray */
	CollectTypeInfo	*type;

	int32			max_index;		/* nested table next key id */
	int32			limit_idx;		/* the limit of the varray */

	MemoryContext	valCxt;			/* memory context of the value */

	Datum 			values;			/* use stl map or CollectVarrayValue strore the data,
									 * values is a key-value store
									 */
} CollectTypeData;

typedef struct CollectTypeData*			CollectType;

/* store the key information of the collect type data element */
typedef struct CollecTypeKey
{
	Datum		value;  			/* key */
} CollecTypeKey;

/* store the value information of the collect type data element */
typedef struct CollecTypeValue
{
	Datum		value;
	bool		isnull;
	bool		placeholder;	/* true: after nested table executed DELETE(n) or DELETE(m, n) */
} CollecTypeValue;


/* store VARRAY elements */
typedef struct CollectVarrayValue
{
	int32			total_index;
	int32			current_used;
	Datum			*value;
} CollectVarrayValue;

/* Begin - ReqID:SRS-PLSQL-BULKCOLLECT */
/* the collect data of BULK COLLECT INTO */
typedef struct
{
	Datum			target_value;		/* the last dim of collection value */
	void 			*target_datatype;	/* the datatype of target collection */
} BulkCollectIntoData;

/* the parameters of BULK COLLECT INTO */
typedef struct
{
	uint32			collect_num;		/* the number of collect targets */
	Datum			*collect_data;		/* BulkCollectIntoData datum array */
} BulkCollectIntoParams;

/*
 * Specialized DestReceiver for BULK COLLECT INTO
 */
typedef struct
{
	DestReceiver	 		pub;			/* publicly-known function pointers */
	BulkCollectIntoParams	*bulk_params;
	/* Begin - ReqID:SRS-PLSQL-FORALL */
	bool					in_forall;		/* in a FORALL statement? */
	/* End - ReqID:SRS-PLSQL-FORALL */
} DR_bulkcollect;
/* End - ReqID:SRS-PLSQL-BULKCOLLECT */

/* functions */
extern Datum collection_init(PG_FUNCTION_ARGS);
extern void collection_init_internal(void);
extern Datum init_collectype_var(Datum collectype_info);
extern Datum create_collecttype_info(const char *typname,
				   Oid typoid,
				   Oid sqlcollectionoid, /* ReqID:SRS-SQL-Collection */
				   int32 typmod,
				   char typtype,
				   bool typbyval,
				   int typlen,
				   Datum typkey,
				   Datum typdatatype,
				   int limit,
				   bool datanotnull);
extern int collectype_int_comparer(void *a, void *b);
extern int collectype_varchar_comparer(void *a, void *b);
extern int collectype_key_compare(Oid keyoid, void *key1, void *key2);
extern Datum copy_collectype_datum(Datum src_value);
extern Datum collectype_element_ref(Datum datum, int nSubscripts,
			Datum *indx, bool *isNull);
extern Datum collectype_element_assign(Datum datum,
					int nSubscripts,
					Datum *indx,
					Datum dataValue,
					bool *isNull);
extern Datum get_collect_value(Datum datum, int nsubscripts,
					Datum *subscriptvals);
extern void set_collect_value(Datum datum, int nsubscripts,
					Datum *subscriptvals,
					int *nth,
					List **fieldnames,
					void *top_datatype,
					Datum assign_value,
					bool assign_value_isnull);
extern bool get_collectype_value(Datum collect_datum,
		Datum key_value, Datum *val_value, bool *isnull);
extern char get_collectyptype(Datum collect_datum);
extern void collect_insert(Datum collect_data, Datum key,
			  Datum value, bool null_value);
extern bool collectype_cast_Check(Datum value, int32 reqtypmod);
extern Datum get_collectype_name(Datum value);
extern Datum get_collectype_mode(Datum value);
extern MemoryContext get_collectype_mcxt(Datum value);
/* Begin - ReqID:SRS-PLSQL-BULKCOLLECT */
extern Datum init_bulk_collect_data(Datum datum,
					int nSubscripts,
					Datum *subscriptvals,
					List **fieldnames,
					Datum *keytypes,
					void *root_datatype,
					void *target_datatype,
					MemoryContext cxt);
extern Datum init_bulk_collect_params(int num, Datum *collect_data);
extern void set_bulk_collect_destreceiver_params(Datum dest, Datum bulk_params);
extern Datum create_bulk_collect_receiver(void);
/* End - ReqID:SRS-PLSQL-BULKCOLLECT */
/* Begin - ReqID:SRS-PLSQL-FORALL */
extern void set_bulk_collect_destreceiver_inforall(Datum dest);
extern List *get_collect_all_values(Datum collect_value);
extern List *get_collect_all_key_values(Datum collect_value, bool has_between_and,
						int lower_bound, int upper_bound);
/* End - ReqID:SRS-PLSQL-FORALL */
/* Begin - ReqID:SRS-SQL-Collection */
Oid get_sqlcollectype_oid(Datum collect_datum);
/* End - ReqID:SRS-SQL-Collection */
#endif
