/*-------------------------------------------------------------------------
 *
 * tecc_admin_funcs.c
 *	  Administrator diagnostic functions for TECC encrypted data.
 *
 * src/backend/utils/tecc/tecc_admin_funcs.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/genam.h"
#include "access/htup_details.h"
#include "access/table.h"
#include "access/tupmacs.h"
#include "catalog/hg_encrypted_columns.h"
#include "catalog/namespace.h"
#include "catalog/pg_attribute.h"
#include "catalog/pg_type.h"
#include "catalog/pg_type_enc.h"
#include "common/tecc_algorithm/tecc_ec_elgamal.h"
#include "common/tecc_algorithm/tecc_elgamal.h"
#include "fmgr.h"
#include "funcapi.h"
#include "lib/stringinfo.h"
#include "nodes/makefuncs.h"
#include "storage/bufpage.h"
#include "storage/itemid.h"
#include "miscadmin.h"          /* ReqID:TECC4.0 - work_mem 声明（tecc_setup_tuplestore 使用） */
#include "utils/acl.h"
#include "utils/builtins.h"
#include "utils/cipherdatatype.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"
#include "utils/snapmgr.h"
#include "utils/tecc/cipherdata_hex.h"
#include "utils/tuplestore.h"
#include "utils/typcache.h"
#include "utils/varlena.h"

PG_FUNCTION_INFO_V1(tecc_enc_info);
PG_FUNCTION_INFO_V1(tecc_enc_ratio);

extern Datum float8_numeric(PG_FUNCTION_ARGS);

static bool tecc_is_encrypted_type(Oid type_oid);
static char *tecc_encryption_type_to_cstring(int32 encryption_type);
static Datum tecc_cipher_segment_hex(const char *ptr, int32 len, bool *isnull);
static HeapTuple tecc_get_encrypted_column_tuple(Oid table_oid, const char *column_name);
static ReturnSetInfo *tecc_setup_tuplestore(FunctionCallInfo fcinfo,
											Tuplestorestate **tupstore,
											TupleDesc *tupdesc);
static Oid tecc_text_to_relid(text *relname);
static int32 tecc_estimate_source_len(Oid type_oid, int32 typmod, int32 default_len);
static int32 tecc_estimate_plain_size(Oid type_oid, int32 typmod, int32 default_len);
static int32 tecc_estimate_encrypted_size(int32 plain_len, int32 encryption_type);
static int64 tecc_align_size(int64 size, char align);
static Datum tecc_numeric_from_double(double value);
static int64 tecc_estimate_disk_1m(int64 row_size);

static bool
tecc_is_encrypted_type(Oid type_oid)
{
	switch (type_oid)
	{
		case INT2_ENCOID:
		case INT4_ENCOID:
		case INT8_ENCOID:
		case FLOAT4_ENCOID:
		case FLOAT8_ENCOID:
		case NUMERIC_ENCOID:
		case VARCHAR_ENCOID:
		case BPCHAR_ENCOID:
		case TEXT_ENCOID:
			return true;
		default:
			return false;
	}
}

static char *
tecc_encryption_type_to_cstring(int32 encryption_type)
{
	StringInfoData buf;
	int32		unknown;

	initStringInfo(&buf);

	appendStringInfoString(&buf, "RND");
	if ((encryption_type & 0x01) != 0)
		appendStringInfoString(&buf, ",DET");
	if ((encryption_type & 0x02) != 0)
		appendStringInfoString(&buf, ",OPE");
	if ((encryption_type & 0x04) != 0)
		appendStringInfoString(&buf, ",SEARCH");
	if ((encryption_type & 0x08) != 0)
		appendStringInfoString(&buf, ",HE_ADD");
	if ((encryption_type & 0x10) != 0)
		appendStringInfoString(&buf, ",HE_MUL");

	unknown = encryption_type & ~0x1f;
	if (unknown != 0)
		appendStringInfo(&buf, ",UNKNOWN(0x%x)", unknown);

	return buf.data;
}

static Datum
tecc_cipher_segment_hex(const char *ptr, int32 len, bool *isnull)
{
	char	   *hex;
	text	   *result;

	if (len <= 0 || ptr == NULL)
	{
		*isnull = true;
		return (Datum) 0;
	}

	*isnull = false;
	hex = binary_to_hex((const unsigned char *) ptr, len);
	if (hex == NULL)
	{
		*isnull = true;
		return (Datum) 0;
	}

	/* Segment hex is displayed without the bytea-style "\x" prefix. */
	result = cstring_to_text(hex + HEX_PREFIX_LEN);
	pfree(hex);
	return PointerGetDatum(result);
}

static HeapTuple
tecc_get_encrypted_column_tuple(Oid table_oid, const char *column_name)
{
	Relation	rel;
	SysScanDesc scan;
	ScanKeyData key[1];
	HeapTuple	tuple;
	HeapTuple	result = NULL;

	rel = table_open(HgEncryptedColumnsRelationId, AccessShareLock);
	ScanKeyInit(&key[0],
				Anum_hg_encrypted_columns_table_oid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(table_oid));
	scan = systable_beginscan(rel, InvalidOid, false,
							  GetActiveSnapshot(), 1, key);

	while (HeapTupleIsValid(tuple = systable_getnext(scan)))
	{
		Form_hg_encrypted_columns form;

		form = (Form_hg_encrypted_columns) GETSTRUCT(tuple);
		if (namestrcmp(&form->column_name, column_name) == 0)
		{
			result = heap_copytuple(tuple);
			break;
		}
	}

	systable_endscan(scan);
	table_close(rel, AccessShareLock);

	return result;
}

static ReturnSetInfo *
tecc_setup_tuplestore(FunctionCallInfo fcinfo, Tuplestorestate **tupstore,
					  TupleDesc *tupdesc)
{
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	MemoryContext oldcontext;

	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("set-valued function called in context that cannot accept a set")));

	if (!(rsinfo->allowedModes & SFRM_Materialize) ||
		rsinfo->expectedDesc == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("materialize mode required, but it is not allowed in this context")));

	oldcontext = MemoryContextSwitchTo(rsinfo->econtext->ecxt_per_query_memory);
	*tupdesc = CreateTupleDescCopy(rsinfo->expectedDesc);
	*tupstore = tuplestore_begin_heap(rsinfo->allowedModes & SFRM_Materialize_Random,
									  false, work_mem);
	MemoryContextSwitchTo(oldcontext);

	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = *tupstore;
	rsinfo->setDesc = *tupdesc;

	return rsinfo;
}

static Oid
tecc_text_to_relid(text *relname)
{
	RangeVar   *relrv;

	relrv = makeRangeVarFromNameList(textToQualifiedNameList(relname));
	return RangeVarGetRelid(relrv, AccessShareLock, false);
}

static int32
tecc_estimate_source_len(Oid type_oid, int32 typmod, int32 default_len)
{
	if (type_oid == INT2OID)
		return 2;
	if (type_oid == INT4OID || type_oid == FLOAT4OID)
		return 4;
	if (type_oid == INT8OID || type_oid == FLOAT8OID)
		return 8;

	if (type_oid == VARCHAROID || type_oid == BPCHAROID)
	{
		if (typmod >= VARHDRSZ)
			return typmod - VARHDRSZ;
		return default_len;
	}

	if (type_oid == TEXTOID || type_oid == NUMERICOID)
		return default_len;

	{
		int16		typlen;
		bool		typbyval;
		char		typalign;

		get_typlenbyvalalign(type_oid, &typlen, &typbyval, &typalign);
		if (typlen > 0)
			return typlen;
	}

	return default_len;
}

static int32
tecc_estimate_plain_size(Oid type_oid, int32 typmod, int32 default_len)
{
	int16		typlen;
	bool		typbyval;
	char		typalign;
	int32		source_len;

	get_typlenbyvalalign(type_oid, &typlen, &typbyval, &typalign);
	source_len = tecc_estimate_source_len(type_oid, typmod, default_len);

	if (typlen > 0)
		return source_len;

	return VARHDRSZ + source_len;
}

static int32
tecc_estimate_encrypted_size(int32 plain_len, int32 encryption_type)
{
	int32		padded_plain = ((plain_len + 15) / 16) * 16;
	uint16		mask;
	int32		size;

	if (padded_plain <= 0)
		padded_plain = 16;

	/* Compute segment mask: RND always present, plus per encryption_type */
	mask = CIPHERDATA_SEG_RND;
	if (encryption_type & 0x01)		mask |= CIPHERDATA_SEG_DET;
	if (encryption_type & 0x02)		mask |= CIPHERDATA_SEG_OPE;
	if (encryption_type & 0x04)		mask |= CIPHERDATA_SEG_SEARCH;
	if (encryption_type & 0x08)		mask |= CIPHERDATA_SEG_EG_ADD;
	if (encryption_type & 0x10)		mask |= CIPHERDATA_SEG_EG_MUL;

	/* Compact header: vl_len_ + base(8) + 4 * popcount(mask) */
	size = sizeof(int32) + CIPHERDATA_FIELDS_BASE_SIZE +
		   CIPHERDATA_POPCOUNT16(mask) * (int32)sizeof(int32);

	/* RND ciphertext: 56 (auth+iv+len) + padded data */
	size += 56 + padded_plain;

	/* DET ciphertext */
	if (mask & CIPHERDATA_SEG_DET)
		size += padded_plain;

	/* OPE ciphertext */
	if (mask & CIPHERDATA_SEG_OPE)
		size += padded_plain * 2;

	/* SEARCH ciphertext */
	if (mask & CIPHERDATA_SEG_SEARCH)
		size += 4 + plain_len * 3;

	/* EG_ADD ciphertext */
	if (mask & CIPHERDATA_SEG_EG_ADD)
		size += TECC_EC_CRT_MAX_CIPHERTEXT_SIZE;

	/* EG_MUL ciphertext */
	if (mask & CIPHERDATA_SEG_EG_MUL)
		size += TECC_ELGAMAL_CIPHERTEXT_SIZE;

	return size;
}

static int64
tecc_align_size(int64 size, char align)
{
	switch (align)
	{
		case 'd':
			return TYPEALIGN(8, size);
		case 'i':
			return TYPEALIGN(4, size);
		case 's':
			return TYPEALIGN(2, size);
		case 'c':
		default:
			return size;
	}
}

static Datum
tecc_numeric_from_double(double value)
{
	return DirectFunctionCall1(float8_numeric, Float8GetDatum(value));
}

static int64
tecc_estimate_disk_1m(int64 row_size)
{
	int64		tuple_size;
	int64		rows_per_page;
	int64		pages;

	tuple_size = MAXALIGN(row_size);
	rows_per_page = (BLCKSZ - SizeOfPageHeaderData) / (sizeof(ItemIdData) + tuple_size);
	if (rows_per_page <= 0)
		rows_per_page = 1;

	pages = (1000000 + rows_per_page - 1) / rows_per_page;
	return pages * BLCKSZ;
}

Datum
tecc_enc_info(PG_FUNCTION_ARGS)
{
	Oid			argtype;
	CipherDataType *cipherdata;
	TupleDesc	tupdesc;
	Datum		values[15];
	bool		nulls[15];
	HeapTuple	tuple;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	argtype = get_fn_expr_argtype(fcinfo->flinfo, 0);
	if (!tecc_is_encrypted_type(argtype))
		ereport(ERROR,
				(errcode(ERRCODE_DATATYPE_MISMATCH),
				 errmsg("tecc_enc_info requires a TECC encrypted value")));

	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("record type has not been registered")));

	cipherdata = (CipherDataType *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	if (!cipherdata_validate(cipherdata))
		ereport(ERROR,
				(errcode(ERRCODE_TECC_CIPHERDATA_INVALID),
				 errmsg("invalid CipherDataType structure")));

	memset(values, 0, sizeof(values));
	memset(nulls, false, sizeof(nulls));

	values[0] = Int32GetDatum(CIPHERDATA_DATUM_VERSION(cipherdata));
	values[1] = ObjectIdGetDatum(CIPHERDATA_DATUM_DEK_DIST_OID(cipherdata));
	values[2] = Int32GetDatum(CIPHERDATA_DATUM_RND_LEN(cipherdata));
	values[3] = tecc_cipher_segment_hex(CIPHERDATA_DATUM_RND_PTR(cipherdata),
										CIPHERDATA_DATUM_RND_LEN(cipherdata),
										&nulls[3]);
	values[4] = Int32GetDatum(CIPHERDATA_DATUM_DET_LEN(cipherdata));
	values[5] = tecc_cipher_segment_hex(CIPHERDATA_DATUM_DET_PTR(cipherdata),
										CIPHERDATA_DATUM_DET_LEN(cipherdata),
										&nulls[5]);
	values[6] = Int32GetDatum(CIPHERDATA_DATUM_OPE_LEN(cipherdata));
	values[7] = tecc_cipher_segment_hex(CIPHERDATA_DATUM_OPE_PTR(cipherdata),
										CIPHERDATA_DATUM_OPE_LEN(cipherdata),
										&nulls[7]);
	values[8] = Int32GetDatum(CIPHERDATA_DATUM_SEARCH_LEN(cipherdata));
	values[9] = tecc_cipher_segment_hex(CIPHERDATA_DATUM_SEARCH_PTR(cipherdata),
										CIPHERDATA_DATUM_SEARCH_LEN(cipherdata),
										&nulls[9]);
	values[10] = Int32GetDatum(CIPHERDATA_DATUM_EG_ADD_LEN(cipherdata));
	values[11] = tecc_cipher_segment_hex(CIPHERDATA_DATUM_EG_ADD_PTR(cipherdata),
										 CIPHERDATA_DATUM_EG_ADD_LEN(cipherdata),
										 &nulls[11]);
	values[12] = Int32GetDatum(CIPHERDATA_DATUM_EG_MUL_LEN(cipherdata));
	values[13] = tecc_cipher_segment_hex(CIPHERDATA_DATUM_EG_MUL_PTR(cipherdata),
										 CIPHERDATA_DATUM_EG_MUL_LEN(cipherdata),
										 &nulls[13]);
	values[14] = Int32GetDatum(VARSIZE_ANY(cipherdata));

	tuple = heap_form_tuple(tupdesc, values, nulls);
	PG_FREE_IF_COPY(cipherdata, 0);
	PG_RETURN_DATUM(HeapTupleGetDatum(tuple));
}

Datum
tecc_enc_ratio(PG_FUNCTION_ARGS)
{
	text	   *relname;
	int32		default_len;
	Oid			table_oid;
	AclResult	aclresult;
	Relation	table_rel;
	TupleDesc	table_desc;
	Tuplestorestate *tupstore;
	TupleDesc	out_desc;
	int64		plain_data_size = 0;
	int64		enc_data_size = 0;
	int			natts = 0;
	int			i;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	relname = PG_GETARG_TEXT_PP(0);
	default_len = (PG_NARGS() >= 2 && !PG_ARGISNULL(1)) ? PG_GETARG_INT32(1) : 20;

	if (default_len <= 0)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("varlena_length must be greater than zero")));

	tecc_setup_tuplestore(fcinfo, &tupstore, &out_desc);

	table_oid = tecc_text_to_relid(relname);

	/* ReqID:TECC4.0 - The estimator exposes all encrypted-column metadata. */
	aclresult = pg_class_aclcheck(table_oid, GetUserId(), ACL_SELECT);
	if (aclresult != ACLCHECK_OK)
		aclcheck_error(aclresult, OBJECT_TABLE, get_rel_name(table_oid));

	table_rel = table_open(table_oid, AccessShareLock);
	table_desc = RelationGetDescr(table_rel);
	natts = table_desc->natts;

	for (i = 0; i < natts; i++)
	{
		Form_pg_attribute attr = TupleDescAttr(table_desc, i);
		HeapTuple	enc_tuple;
		Oid			source_type;
		int32		source_typmod;
		int32		source_len;
		int32		plain_size;
		int32		enc_size;
		char		plain_align;
		char		enc_align;

		if (attr->attisdropped || attr->attnum <= 0)
			continue;

		enc_tuple = tecc_get_encrypted_column_tuple(table_oid, NameStr(attr->attname));
		if (HeapTupleIsValid(enc_tuple))
		{
			Form_hg_encrypted_columns enc_form;
			Datum		values[11];
			bool		nulls[11];
			char	   *source_type_name;
			char	   *enc_type_name;
			char	   *alg_text;
			double		ratio;

			enc_form = (Form_hg_encrypted_columns) GETSTRUCT(enc_tuple);
			source_type = enc_form->data_type_original_oid;
			source_typmod = enc_form->data_type_original_mod;
			source_len = tecc_estimate_source_len(source_type, source_typmod, default_len);
			plain_size = tecc_estimate_plain_size(source_type, source_typmod, default_len);
			enc_size = tecc_estimate_encrypted_size(source_len, enc_form->encryption_type);
			{
				int16		typlen;
				bool		typbyval;

				get_typlenbyvalalign(source_type, &typlen, &typbyval, &plain_align);
			}
			enc_align = attr->attalign;

			memset(values, 0, sizeof(values));
			memset(nulls, false, sizeof(nulls));

			source_type_name = format_type_be(source_type);
			enc_type_name = format_type_be(attr->atttypid);
			alg_text = tecc_encryption_type_to_cstring(enc_form->encryption_type);
			ratio = (plain_size > 0) ? ((double) enc_size / (double) plain_size) : 0.0;

			values[0] = CStringGetTextDatum(NameStr(attr->attname));
			values[1] = CStringGetTextDatum(source_type_name);
			values[2] = Int32GetDatum(source_len);
			values[3] = CStringGetTextDatum(enc_type_name);
			values[4] = CStringGetTextDatum(alg_text);
			values[5] = Int32GetDatum(plain_size);
			values[6] = Int32GetDatum(enc_size);
			values[7] = tecc_numeric_from_double(ratio);
			nulls[8] = true;
			nulls[9] = true;
			nulls[10] = true;

			tuplestore_putvalues(tupstore, out_desc, values, nulls);

			pfree(source_type_name);
			pfree(enc_type_name);
			pfree(alg_text);
			heap_freetuple(enc_tuple);
		}
		else
		{
			source_type = attr->atttypid;
			source_typmod = attr->atttypmod;
			plain_size = tecc_estimate_plain_size(source_type, source_typmod, default_len);
			enc_size = plain_size;
			plain_align = attr->attalign;
			enc_align = attr->attalign;
		}

		plain_data_size = tecc_align_size(plain_data_size, plain_align) + plain_size;
		enc_data_size = tecc_align_size(enc_data_size, enc_align) + enc_size;
	}

	{
		Datum		values[11];
		bool		nulls[11];
		int64		plain_row_size;
		int64		enc_row_size;
		int64		total_plain_1m;
		int64		total_enc_1m;
		double		overall_ratio;

		memset(values, 0, sizeof(values));
		memset(nulls, true, sizeof(nulls));

		plain_row_size = MAXALIGN(SizeofHeapTupleHeader + BITMAPLEN(natts)) + plain_data_size;
		enc_row_size = MAXALIGN(SizeofHeapTupleHeader + BITMAPLEN(natts)) + enc_data_size;
		total_plain_1m = tecc_estimate_disk_1m(plain_row_size);
		total_enc_1m = tecc_estimate_disk_1m(enc_row_size);
		overall_ratio = (plain_row_size > 0) ?
			((double) enc_row_size / (double) plain_row_size) : 0.0;

		values[0] = CStringGetTextDatum("TOTAL");
		nulls[0] = false;
		values[8] = Int64GetDatum(total_plain_1m);
		nulls[8] = false;
		values[9] = Int64GetDatum(total_enc_1m);
		nulls[9] = false;
		values[10] = tecc_numeric_from_double(overall_ratio);
		nulls[10] = false;

		tuplestore_putvalues(tupstore, out_desc, values, nulls);
	}

	table_close(table_rel, AccessShareLock);
	tuplestore_donestoring(tupstore);

	return (Datum) 0;
}
