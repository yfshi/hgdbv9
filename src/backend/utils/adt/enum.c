/*-------------------------------------------------------------------------
 *
 * enum.c
 *	  I/O functions, operators, aggregates etc for enum types
 *
 * Copyright (c) 2006-2021, PostgreSQL Global Development Group
 *
 *
 * IDENTIFICATION
 *	  src/backend/utils/adt/enum.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/genam.h"
#include "access/htup_details.h"
#include "access/table.h"
#include "catalog/pg_enum.h"
#include "common/int.h"
#include "libpq/pqformat.h"
#include "parser/parse_coerce.h"
#include "storage/procarray.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/catcache.h"
#include "utils/fmgroids.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "utils/numeric.h"
#include "utils/ora_compatible.h"
#include "utils/snapmgr.h"
#include "utils/syscache.h"
#include "utils/typcache.h"

static Oid	enum_endpoint(Oid enumtypoid, ScanDirection direction);
static ArrayType *enum_range_internal(Oid enumtypoid, Oid lower, Oid upper);
static Oid check_enumlabel_value(Oid enumtypid, char *enumlabel);

/* Begin - ReqID:SRS-MYSQL-ENUMTYPE */
/* covert a string to a number */
static uint64
my_strntoul_8bit(const char *str, size_t length, int base, char **endptr, int *err)
{
	int 			negative;
	uint32 			cutoff;
	unsigned int 	cutlim;
	uint32 			i = 0;
	unsigned char 	c;
	const char 		*save, *s, *e;
	int 			overflow = 0;

	*err = 0; /* Initialize error indicator */

	s = str;
	e = str + length;

	for (; s < e && (*str == ' ' || *str == '\t'); s++)
		;

	if (s == e)
	{
		goto noconv;
	}

	if (*s == '-')
	{
		negative = 1;
		++s;
	}
	else if (*s == '+')
	{
		negative = 0;
		++s;
	}
	else
	{
		negative = 0;
	}

	save = s;
	cutoff = ((uint32) ~0L) / (uint32) base;
	cutlim = (unsigned int) (((uint32) ~0L) % (uint32) base);

	for (c = *s; s != e; c = *++s)
	{
		if (c >= '0' && c <= '9')
			c -= '0';
		else if (c >= 'A' && c <= 'Z')
			c = c - 'A' + 10;
		else if (c >= 'a' && c <= 'z')
			c = c - 'a' + 10;
		else
			break;

		if (c >= base)
			break;

		if (i > cutoff || (i == cutoff && c > cutlim))
			overflow = 1;
		else
		{
			i *= (uint32) base;
			i += c;
		}
	}

	if (s == save)
		goto noconv;

	if (endptr != NULL)
		*endptr = (char *)s;

	if (overflow)
	{
		err[0] = ERANGE;
		return (~(uint32) 0);
	}

	return (negative ? -((long) i) : (long) i);

noconv:
	err[0] = EDOM;
	if (endptr != NULL)
		*endptr = (char *) str;

	return 0L;
}

static int32 
myenum_atoi_int4(const char *s, bool *iswarning)
{
	int 	tmp = 0;
	int 	save_errno = 0;
	char 	*from = (char *) s,
			*end = NULL;
	int64 	length = strlen(s);

	tmp = my_strntoul_8bit(s, length, 10, &end, &save_errno);

	if (save_errno || end != from + length)
	{
		tmp = 0;
		*iswarning = true;
	}

	return (int32) tmp;
}

/* get enum oid by enumtypid and enumsortorder */
Oid
get_enum_oid(Oid enumtypid, int enumsortorder)
{
	Relation 	enum_rel;
	ScanKeyData skey;
	SysScanDesc scan;
	HeapTuple 	maptup;

	if (enumsortorder <= 0)
		return InvalidOid;

	enum_rel = table_open(EnumRelationId, AccessShareLock);

	ScanKeyInit(&skey,
				Anum_pg_enum_enumtypid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(enumtypid));

	scan = systable_beginscan(enum_rel,
							  EnumTypIdLabelIndexId,
							  true, NULL,
							  1, &skey);

	while (HeapTupleIsValid(maptup = systable_getnext(scan)))
	{
		Form_pg_enum en = (Form_pg_enum) GETSTRUCT(maptup);

		/* If this is the target superclass, we're done */
		if (enumsortorder == en->enumsortorder)
		{
			Oid		resultoid = en->oid;
			systable_endscan(scan);
			table_close(enum_rel, AccessShareLock);
			return resultoid;
		}
	}

	systable_endscan(scan);

	table_close(enum_rel, AccessShareLock);

	return InvalidOid;
}

/* get enum oid by enumtypid and enumlabel */
Oid
get_enum_oid_by_label(Oid enumtypid, const char *enumlabel)
{
	Relation 	enum_rel;
	ScanKeyData skey;
	SysScanDesc scan;
	HeapTuple 	maptup;

	enum_rel = table_open(EnumRelationId, AccessShareLock);

	ScanKeyInit(&skey,
				Anum_pg_enum_enumtypid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(enumtypid));

	scan = systable_beginscan(enum_rel,
							  EnumTypIdLabelIndexId,
							  true, NULL,
							  1, &skey);

	while (HeapTupleIsValid(maptup = systable_getnext(scan)))
	{
		Form_pg_enum en = (Form_pg_enum) GETSTRUCT(maptup);

		/* If this is the target superclass, we're done */
		if (strcmp(en->enumlabel.data, enumlabel) == 0)
		{
			Oid		resultoid = en->oid;
			systable_endscan(scan);
			table_close(enum_rel, AccessShareLock);
			return resultoid;
		}
	}

	systable_endscan(scan);

	table_close(enum_rel, AccessShareLock);

	return InvalidOid;
}

/* get enumsortorder by oid */
float4
get_enum_sortorder(Oid oid)
{
	HeapTuple tup;
	Form_pg_enum en;
	float4 enumsortorder;

	tup = SearchSysCache1(ENUMOID, ObjectIdGetDatum(oid));
	if (!HeapTupleIsValid(tup))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_BINARY_REPRESENTATION),
				 errmsg("invalid internal value for enum: %u",
						oid)));
	en = (Form_pg_enum)GETSTRUCT(tup);

	enumsortorder = en->enumsortorder;

	ReleaseSysCache(tup);

	return enumsortorder;
}

/* get enumtypid by oid */
Oid
get_enum_enumtypid(Oid oid)
{
	HeapTuple tup;
	Form_pg_enum en;
	Oid enumtypid;

	tup = SearchSysCache1(ENUMOID, ObjectIdGetDatum(oid));
	if (!HeapTupleIsValid(tup))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_BINARY_REPRESENTATION),
				 errmsg("invalid internal value for enum: %u",
						oid)));
	en = (Form_pg_enum)GETSTRUCT(tup);

	enumtypid = en->enumtypid;

	ReleaseSysCache(tup);

	return enumtypid;
}

/* get enum enumlabel by oid  */
char *
get_enum_label(Oid oid)
{
	char 			*enumlabel = NULL;
	Form_pg_enum 	en;
	HeapTuple 		tup;

	if (InvalidOid == oid)
	{
		enumlabel = pstrdup("");
		return enumlabel;
	}

	tup = SearchSysCache1(ENUMOID, ObjectIdGetDatum(oid));
	if (!HeapTupleIsValid(tup))
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_BINARY_REPRESENTATION),
				 errmsg("invalid internal value for enum: %u",
						oid)));
	}

	en = (Form_pg_enum) GETSTRUCT(tup);

	enumlabel = pstrdup(NameStr(en->enumlabel));
	ReleaseSysCache(tup);

	return enumlabel;
}

/* compare two enum value by oid*/
int
compare_two_enum_value(Oid oid1, Oid oid2)
{
	char *val1 = NULL;
	char *val2 = NULL;

	val1 = get_enum_label(oid1);
	val2 = get_enum_label(oid2);

	return strcmp(val1, val2);
}
/* End - ReqID:SRS-MYSQL-ENUMTYPE */

/*
 * Disallow use of an uncommitted pg_enum tuple.
 *
 * We need to make sure that uncommitted enum values don't get into indexes.
 * If they did, and if we then rolled back the pg_enum addition, we'd have
 * broken the index because value comparisons will not work reliably without
 * an underlying pg_enum entry.  (Note that removal of the heap entry
 * containing an enum value is not sufficient to ensure that it doesn't appear
 * in upper levels of indexes.)  To do this we prevent an uncommitted row from
 * being used for any SQL-level purpose.  This is stronger than necessary,
 * since the value might not be getting inserted into a table or there might
 * be no index on its column, but it's easy to enforce centrally.
 *
 * However, it's okay to allow use of uncommitted values belonging to enum
 * types that were themselves created in the same transaction, because then
 * any such index would also be new and would go away altogether on rollback.
 * We don't implement that fully right now, but we do allow free use of enum
 * values created during CREATE TYPE AS ENUM, which are surely of the same
 * lifespan as the enum type.  (This case is required by "pg_restore -1".)
 * Values added by ALTER TYPE ADD VALUE are currently restricted, but could
 * be allowed if the enum type could be proven to have been created earlier
 * in the same transaction.  (Note that comparing tuple xmins would not work
 * for that, because the type tuple might have been updated in the current
 * transaction.  Subtransactions also create hazards to be accounted for.)
 *
 * This function needs to be called (directly or indirectly) in any of the
 * functions below that could return an enum value to SQL operations.
 */
static void
check_safe_enum_use(HeapTuple enumval_tup)
{
	TransactionId xmin;
	Form_pg_enum en = (Form_pg_enum) GETSTRUCT(enumval_tup);

	/*
	 * If the row is hinted as committed, it's surely safe.  This provides a
	 * fast path for all normal use-cases.
	 */
	if (HeapTupleHeaderXminCommitted(enumval_tup->t_data))
		return;

	/*
	 * Usually, a row would get hinted as committed when it's read or loaded
	 * into syscache; but just in case not, let's check the xmin directly.
	 */
	xmin = HeapTupleHeaderGetXmin(enumval_tup->t_data);
	if (!TransactionIdIsInProgress(xmin) &&
		TransactionIdDidCommit(xmin))
		return;

	/*
	 * Check if the enum value is uncommitted.  If not, it's safe, because it
	 * was made during CREATE TYPE AS ENUM and can't be shorter-lived than its
	 * owning type.  (This'd also be false for values made by other
	 * transactions; but the previous tests should have handled all of those.)
	 */
	if (!EnumUncommitted(en->oid))
		return;

	/*
	 * There might well be other tests we could do here to narrow down the
	 * unsafe conditions, but for now just raise an exception.
	 */
	ereport(ERROR,
			(errcode(ERRCODE_UNSAFE_NEW_ENUM_VALUE_USAGE),
			 errmsg("unsafe use of new value \"%s\" of enum type %s",
					NameStr(en->enumlabel),
					format_type_be(en->enumtypid)),
			 errhint("New enum values must be committed before they can be used.")));
}


/* Basic I/O support */

Datum
enum_in(PG_FUNCTION_ARGS)
{
	char	   *name = PG_GETARG_CSTRING(0);
	Oid			enumtypoid = PG_GETARG_OID(1);
	Oid			enumoid;
	HeapTuple	tup;
	Oid 		result = InvalidOid;
	char	   *trimname = NULL;

	/* Begin - ReqID:SRS-MYSQL-ENUMTYPE */
	if (MYS_MODE == compatible_mode)
	{
		int32 	enumidx = 0;
		bool 	iswarnins = false;
		int 	err_flag = is_strict_mode() ? ERROR : WARNING;

		/* trim the right space for set label */
		Datum trimtxt = DirectFunctionCall1(rtrim1, CStringGetTextDatum(name));
		trimname = TextDatumGetCString(trimtxt);

		/* must check length to prevent Assert failure within SearchSysCache */
		if (strlen(trimname) >= NAMEDATALEN)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					 errmsg("invalid input value for enum %s: \"%s\"",
							format_type_be(enumtypoid),
							name)));

		tup = SearchSysCache2(ENUMTYPOIDNAME,
							  ObjectIdGetDatum(enumtypoid),
							  CStringGetDatum(trimname));

		if (!HeapTupleIsValid(tup))
		{
			if (strlen(trimname) == 0)
			{
				ereport(err_flag,
					(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
						 errmsg(" Data truncated for column")));
				PG_RETURN_NULL();
			}
			/* 3rd: can not get enum oid by name , alter converting the string to a numeric type
			 *      it is queried in pg_enum table according to typmod method
			 */
			enumidx = myenum_atoi_int4(name, &iswarnins);

			if (!iswarnins)
			{
				if (0 == enumidx)
					PG_RETURN_NULL();

				result = get_enum_oid(enumtypoid, enumidx);

				if (InvalidOid != result)
					PG_RETURN_OID(result);
			}

			ereport(err_flag,
					(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					 errmsg("invalid input value for enum %s: \"%s\"",
							format_type_be(enumtypoid),
							name)));

			PG_RETURN_NULL();
		}
	}
	else /* End - ReqID:SRS-MYSQL-ENUMTYPE */
	{
		/* must check length to prevent Assert failure within SearchSysCache */
		if (strlen(name) >= NAMEDATALEN)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					 errmsg("invalid input value for enum %s: \"%s\"",
							format_type_be(enumtypoid),
							name)));

		tup = SearchSysCache2(ENUMTYPOIDNAME,
							  ObjectIdGetDatum(enumtypoid),
							  CStringGetDatum(name));
		if (!HeapTupleIsValid(tup))
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					 errmsg("invalid input value for enum %s: \"%s\"",
							format_type_be(enumtypoid),
							name)));
		}
	}
	
	/* check it's safe to use in SQL */
	check_safe_enum_use(tup);

	/*
	 * This comes from pg_enum.oid and stores system oids in user tables. This
	 * oid must be preserved by binary upgrades.
	 */
	enumoid = ((Form_pg_enum) GETSTRUCT(tup))->oid;

	ReleaseSysCache(tup);

	PG_RETURN_OID(enumoid);
}

Datum
enum_out(PG_FUNCTION_ARGS)
{
	Oid			enumval = PG_GETARG_OID(0);
	char	   *result;
	HeapTuple	tup;
	Form_pg_enum en;

	/* Begin - ReqID:SRS-MYSQL-ENUMTYPE */
	if (MYS_MODE == compatible_mode)
	{
		if (enumval == InvalidOid)
			PG_RETURN_CSTRING("");
	}
	/* Begin - ReqID:SRS-MYSQL-ENUMTYPE */

	tup = SearchSysCache1(ENUMOID, ObjectIdGetDatum(enumval));
	if (!HeapTupleIsValid(tup))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_BINARY_REPRESENTATION),
				 errmsg("invalid internal value for enum: %u",
						enumval)));
	en = (Form_pg_enum) GETSTRUCT(tup);

	result = pstrdup(NameStr(en->enumlabel));

	ReleaseSysCache(tup);

	PG_RETURN_CSTRING(result);
}

/* Binary I/O support */
Datum
enum_recv(PG_FUNCTION_ARGS)
{
	StringInfo	buf = (StringInfo) PG_GETARG_POINTER(0);
	Oid			enumtypoid = PG_GETARG_OID(1);
	Oid			enumoid;
	HeapTuple	tup;
	char	   *name;
	int			nbytes;

	name = pq_getmsgtext(buf, buf->len - buf->cursor, &nbytes);

	/* must check length to prevent Assert failure within SearchSysCache */
	if (strlen(name) >= NAMEDATALEN)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input value for enum %s: \"%s\"",
						format_type_be(enumtypoid),
						name)));

	tup = SearchSysCache2(ENUMTYPOIDNAME,
						  ObjectIdGetDatum(enumtypoid),
						  CStringGetDatum(name));
	if (!HeapTupleIsValid(tup))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input value for enum %s: \"%s\"",
						format_type_be(enumtypoid),
						name)));

	/* check it's safe to use in SQL */
	check_safe_enum_use(tup);

	enumoid = ((Form_pg_enum) GETSTRUCT(tup))->oid;

	ReleaseSysCache(tup);

	pfree(name);

	PG_RETURN_OID(enumoid);
}

Datum
enum_send(PG_FUNCTION_ARGS)
{
	Oid			enumval = PG_GETARG_OID(0);
	StringInfoData buf;
	HeapTuple	tup;
	Form_pg_enum en;

	tup = SearchSysCache1(ENUMOID, ObjectIdGetDatum(enumval));
	if (!HeapTupleIsValid(tup))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_BINARY_REPRESENTATION),
				 errmsg("invalid internal value for enum: %u",
						enumval)));
	en = (Form_pg_enum) GETSTRUCT(tup);

	pq_begintypsend(&buf);
	pq_sendtext(&buf, NameStr(en->enumlabel), strlen(NameStr(en->enumlabel)));

	ReleaseSysCache(tup);

	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

/* Comparison functions and related */

/*
 * enum_cmp_internal is the common engine for all the visible comparison
 * functions, except for enum_eq and enum_ne which can just check for OID
 * equality directly.
 */
int
enum_cmp_internal(Oid arg1, Oid arg2, FunctionCallInfo fcinfo)
{
	TypeCacheEntry *tcache;

	/*
	 * We don't need the typcache except in the hopefully-uncommon case that
	 * one or both Oids are odd.  This means that cursory testing of code that
	 * fails to pass flinfo to an enum comparison function might not disclose
	 * the oversight.  To make such errors more obvious, Assert that we have a
	 * place to cache even when we take a fast-path exit.
	 */
	Assert(fcinfo->flinfo != NULL);

	/* Equal OIDs are equal no matter what */
	if (arg1 == arg2)
		return 0;

	/* Fast path: even-numbered Oids are known to compare correctly */
	if ((arg1 & 1) == 0 && (arg2 & 1) == 0)
	{
		if (arg1 < arg2)
			return -1;
		else
			return 1;
	}

	/* Locate the typcache entry for the enum type */
	tcache = (TypeCacheEntry *) fcinfo->flinfo->fn_extra;
	if (tcache == NULL)
	{
		HeapTuple	enum_tup;
		Form_pg_enum en;
		Oid			typeoid;

		/* Get the OID of the enum type containing arg1 */
		enum_tup = SearchSysCache1(ENUMOID, ObjectIdGetDatum(arg1));
		if (!HeapTupleIsValid(enum_tup))
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_BINARY_REPRESENTATION),
					 errmsg("invalid internal value for enum: %u",
							arg1)));
		en = (Form_pg_enum) GETSTRUCT(enum_tup);
		typeoid = en->enumtypid;
		ReleaseSysCache(enum_tup);
		/* Now locate and remember the typcache entry */
		tcache = lookup_type_cache(typeoid, 0);
		fcinfo->flinfo->fn_extra = (void *) tcache;
	}

	/* The remaining comparison logic is in typcache.c */
	return compare_values_of_enum(tcache, arg1, arg2);
}

Datum
enum_lt(PG_FUNCTION_ARGS)
{
	Oid			a = PG_GETARG_OID(0);
	Oid			b = PG_GETARG_OID(1);

	/* Begin - ReqID:SRS-MYSQL-ENUMTYPE */
	if (MYS_MODE == compatible_mode) 
	{
		PG_RETURN_BOOL(compare_two_enum_value(a, b) < 0);
	}
	/* End - ReqID:SRS-MYSQL-ENUMTYPE */

	PG_RETURN_BOOL(enum_cmp_internal(a, b, fcinfo) < 0);
}

Datum
enum_le(PG_FUNCTION_ARGS)
{
	Oid			a = PG_GETARG_OID(0);
	Oid			b = PG_GETARG_OID(1);

	/* Begin - ReqID:SRS-MYSQL-ENUMTYPE */
	if (MYS_MODE == compatible_mode)
	{
		PG_RETURN_BOOL(compare_two_enum_value(a, b) <= 0);
	}
	/* End - ReqID:SRS-MYSQL-ENUMTYPE */

	PG_RETURN_BOOL(enum_cmp_internal(a, b, fcinfo) <= 0);
}

Datum
enum_eq(PG_FUNCTION_ARGS)
{
	Oid			a = PG_GETARG_OID(0);
	Oid			b = PG_GETARG_OID(1);

	/* Begin - ReqID:SRS-MYSQL-ENUMTYPE */
	if (MYS_MODE == compatible_mode)
	{
		PG_RETURN_BOOL(!compare_two_enum_value(a, b));
	}
	/* End - ReqID:SRS-MYSQL-ENUMTYPE */

	PG_RETURN_BOOL(a == b);
}

Datum
enum_ne(PG_FUNCTION_ARGS)
{
	Oid			a = PG_GETARG_OID(0);
	Oid			b = PG_GETARG_OID(1);

	/* Begin - ReqID:SRS-MYSQL-ENUMTYPE */
	if (MYS_MODE == compatible_mode)
	{
		PG_RETURN_BOOL(compare_two_enum_value(a, b) != 0);
	}
	/* End - ReqID:SRS-MYSQL-ENUMTYPE */

	PG_RETURN_BOOL(a != b);
}

Datum
enum_ge(PG_FUNCTION_ARGS)
{
	Oid			a = PG_GETARG_OID(0);
	Oid			b = PG_GETARG_OID(1);

	/* Begin - ReqID:SRS-MYSQL-ENUMTYPE */
	if (MYS_MODE == compatible_mode)
	{
		PG_RETURN_BOOL(compare_two_enum_value(a, b) >= 0);
	}
	/* End - ReqID:SRS-MYSQL-ENUMTYPE */

	PG_RETURN_BOOL(enum_cmp_internal(a, b, fcinfo) >= 0);
}

Datum
enum_gt(PG_FUNCTION_ARGS)
{
	Oid			a = PG_GETARG_OID(0);
	Oid			b = PG_GETARG_OID(1);

	/* Begin - ReqID:SRS-MYSQL-ENUMTYPE */
	if (MYS_MODE == compatible_mode)
	{
		PG_RETURN_BOOL(compare_two_enum_value(a, b) > 0);
	}
	/* End - ReqID:SRS-MYSQL-ENUMTYPE */

	PG_RETURN_BOOL(enum_cmp_internal(a, b, fcinfo) > 0);
}

Datum
enum_smaller(PG_FUNCTION_ARGS)
{
	Oid			a = PG_GETARG_OID(0);
	Oid			b = PG_GETARG_OID(1);
	/* Begin - ReqID:SRS-MYSQL-ENUMTYPE */
	if (MYS_MODE == compatible_mode)
	{
		PG_RETURN_OID(compare_two_enum_value(a, b) < 0 ? a : b);
	}
	/* End - ReqID:SRS-MYSQL-ENUMTYPE */

	PG_RETURN_OID(enum_cmp_internal(a, b, fcinfo) < 0 ? a : b);
}

Datum
enum_larger(PG_FUNCTION_ARGS)
{
	Oid			a = PG_GETARG_OID(0);
	Oid			b = PG_GETARG_OID(1);
	/* Begin - ReqID:SRS-MYSQL-ENUMTYPE */
	if (MYS_MODE == compatible_mode)
	{
		PG_RETURN_OID(compare_two_enum_value(a, b) > 0 ? a : b);
	}
	/* End - ReqID:SRS-MYSQL-ENUMTYPE */

	PG_RETURN_OID(enum_cmp_internal(a, b, fcinfo) > 0 ? a : b);
}

Datum
enum_cmp(PG_FUNCTION_ARGS)
{
	Oid			a = PG_GETARG_OID(0);
	Oid			b = PG_GETARG_OID(1);

	PG_RETURN_INT32(enum_cmp_internal(a, b, fcinfo));
}

/* Enum programming support functions */

/*
 * enum_endpoint: common code for enum_first/enum_last
 */
static Oid
enum_endpoint(Oid enumtypoid, ScanDirection direction)
{
	Relation	enum_rel;
	Relation	enum_idx;
	SysScanDesc enum_scan;
	HeapTuple	enum_tuple;
	ScanKeyData skey;
	Oid			minmax;

	/*
	 * Find the first/last enum member using pg_enum_typid_sortorder_index.
	 * Note we must not use the syscache.  See comments for RenumberEnumType
	 * in catalog/pg_enum.c for more info.
	 */
	ScanKeyInit(&skey,
				Anum_pg_enum_enumtypid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(enumtypoid));

	enum_rel = table_open(EnumRelationId, AccessShareLock);
	enum_idx = index_open(EnumTypIdSortOrderIndexId, AccessShareLock);
	enum_scan = systable_beginscan_ordered(enum_rel, enum_idx, NULL,
										   1, &skey);

	enum_tuple = systable_getnext_ordered(enum_scan, direction);
	if (HeapTupleIsValid(enum_tuple))
	{
		/* check it's safe to use in SQL */
		check_safe_enum_use(enum_tuple);
		minmax = ((Form_pg_enum) GETSTRUCT(enum_tuple))->oid;
	}
	else
	{
		/* should only happen with an empty enum */
		minmax = InvalidOid;
	}

	systable_endscan_ordered(enum_scan);
	index_close(enum_idx, AccessShareLock);
	table_close(enum_rel, AccessShareLock);

	return minmax;
}

Datum
enum_first(PG_FUNCTION_ARGS)
{
	Oid			enumtypoid;
	Oid			min;

	/*
	 * We rely on being able to get the specific enum type from the calling
	 * expression tree.  Notice that the actual value of the argument isn't
	 * examined at all; in particular it might be NULL.
	 */
	enumtypoid = get_fn_expr_argtype(fcinfo->flinfo, 0);
	if (enumtypoid == InvalidOid)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("could not determine actual enum type")));

	/* Get the OID using the index */
	min = enum_endpoint(enumtypoid, ForwardScanDirection);

	if (!OidIsValid(min))
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("enum %s contains no values",
						format_type_be(enumtypoid))));

	PG_RETURN_OID(min);
}

Datum
enum_last(PG_FUNCTION_ARGS)
{
	Oid			enumtypoid;
	Oid			max;

	/*
	 * We rely on being able to get the specific enum type from the calling
	 * expression tree.  Notice that the actual value of the argument isn't
	 * examined at all; in particular it might be NULL.
	 */
	enumtypoid = get_fn_expr_argtype(fcinfo->flinfo, 0);
	if (enumtypoid == InvalidOid)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("could not determine actual enum type")));

	/* Get the OID using the index */
	max = enum_endpoint(enumtypoid, BackwardScanDirection);

	if (!OidIsValid(max))
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("enum %s contains no values",
						format_type_be(enumtypoid))));

	PG_RETURN_OID(max);
}

/* 2-argument variant of enum_range */
Datum
enum_range_bounds(PG_FUNCTION_ARGS)
{
	Oid			lower;
	Oid			upper;
	Oid			enumtypoid;

	if (PG_ARGISNULL(0))
		lower = InvalidOid;
	else
		lower = PG_GETARG_OID(0);
	if (PG_ARGISNULL(1))
		upper = InvalidOid;
	else
		upper = PG_GETARG_OID(1);

	/*
	 * We rely on being able to get the specific enum type from the calling
	 * expression tree.  The generic type mechanism should have ensured that
	 * both are of the same type.
	 */
	enumtypoid = get_fn_expr_argtype(fcinfo->flinfo, 0);
	if (enumtypoid == InvalidOid)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("could not determine actual enum type")));

	PG_RETURN_ARRAYTYPE_P(enum_range_internal(enumtypoid, lower, upper));
}

/* 1-argument variant of enum_range */
Datum
enum_range_all(PG_FUNCTION_ARGS)
{
	Oid			enumtypoid;

	/*
	 * We rely on being able to get the specific enum type from the calling
	 * expression tree.  Notice that the actual value of the argument isn't
	 * examined at all; in particular it might be NULL.
	 */
	enumtypoid = get_fn_expr_argtype(fcinfo->flinfo, 0);
	if (enumtypoid == InvalidOid)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("could not determine actual enum type")));

	PG_RETURN_ARRAYTYPE_P(enum_range_internal(enumtypoid,
											  InvalidOid, InvalidOid));
}

static ArrayType *
enum_range_internal(Oid enumtypoid, Oid lower, Oid upper)
{
	ArrayType  *result;
	Relation	enum_rel;
	Relation	enum_idx;
	SysScanDesc enum_scan;
	HeapTuple	enum_tuple;
	ScanKeyData skey;
	Datum	   *elems;
	int			max,
				cnt;
	bool		left_found;

	/*
	 * Scan the enum members in order using pg_enum_typid_sortorder_index.
	 * Note we must not use the syscache.  See comments for RenumberEnumType
	 * in catalog/pg_enum.c for more info.
	 */
	ScanKeyInit(&skey,
				Anum_pg_enum_enumtypid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(enumtypoid));

	enum_rel = table_open(EnumRelationId, AccessShareLock);
	enum_idx = index_open(EnumTypIdSortOrderIndexId, AccessShareLock);
	enum_scan = systable_beginscan_ordered(enum_rel, enum_idx, NULL, 1, &skey);

	max = 64;
	elems = (Datum *) palloc(max * sizeof(Datum));
	cnt = 0;
	left_found = !OidIsValid(lower);

	while (HeapTupleIsValid(enum_tuple = systable_getnext_ordered(enum_scan, ForwardScanDirection)))
	{
		Oid			enum_oid = ((Form_pg_enum) GETSTRUCT(enum_tuple))->oid;

		if (!left_found && lower == enum_oid)
			left_found = true;

		if (left_found)
		{
			/* check it's safe to use in SQL */
			check_safe_enum_use(enum_tuple);

			if (cnt >= max)
			{
				max *= 2;
				elems = (Datum *) repalloc(elems, max * sizeof(Datum));
			}

			elems[cnt++] = ObjectIdGetDatum(enum_oid);
		}

		if (OidIsValid(upper) && upper == enum_oid)
			break;
	}

	systable_endscan_ordered(enum_scan);
	index_close(enum_idx, AccessShareLock);
	table_close(enum_rel, AccessShareLock);

	/* and build the result array */
	/* note this hardwires some details about the representation of Oid */
	result = construct_array(elems, cnt, enumtypoid,
							 sizeof(Oid), true, TYPALIGN_INT);

	pfree(elems);

	return result;
}


/**
 * Begin - ReqID:SRS-MYSQL-ENUMTYPE
 *
 * Check whether the enum type Oid1 and Oid2 are consistent
 */
bool
enum_is_equal(Oid enumOid1, Oid enumOid2)
{
	
	bool 			isEquivalent = true;
	Relation 		enumRel;
	CatCList 		*items1 = NULL;
	CatCList 		*items2 = NULL;
	int 			nelems = 0;
	HeapTuple		*existing1 = NULL;
	HeapTuple		*existing2 = NULL;
	int				i = 0;

	if (!type_is_enum(enumOid1) || !type_is_enum(enumOid2))
	{
		return false;
	}

	if (enumOid1 == enumOid2)
	{
		return true;
	}

	enumRel = table_open(EnumRelationId, AccessShareLock);
	items1 = SearchSysCacheList1(ENUMTYPOIDNAME, ObjectIdGetDatum(enumOid1));
	items2 = SearchSysCacheList1(ENUMTYPOIDNAME, ObjectIdGetDatum(enumOid2));
	nelems = items1->n_members;

	if (items1->n_members != items2->n_members)
	{
		ReleaseSysCacheList(items1);
		ReleaseSysCacheList(items2);
		table_close(enumRel, AccessShareLock);
		return false;
	}
	/* Sort the existing members by enumsortorder */
	existing1 = (HeapTuple *) palloc(nelems * sizeof(HeapTuple));
	for (i = 0; i < nelems; i++)
		existing1[i] = &(items1->members[i]->tuple);

	qsort(existing1, nelems, sizeof(HeapTuple), sort_order_cmp);

	/* Sort the existing members by enumsortorder */
	existing2 = (HeapTuple *) palloc(nelems * sizeof(HeapTuple));
	for (i = 0; i < nelems; i++)
		existing2[i] = &(items2->members[i]->tuple);

	qsort(existing2, nelems, sizeof(HeapTuple), sort_order_cmp);

	for (int i = 0; i < nelems; ++i)
	{
		HeapTuple	enumTup1 = existing1[i];
		HeapTuple	enumTup2 = existing2[i];
		Form_pg_enum exists_en1 = (Form_pg_enum) GETSTRUCT(enumTup1);
		Form_pg_enum exists_en2 = (Form_pg_enum) GETSTRUCT(enumTup2);
		char 			*enumlabel1 = NameStr(exists_en1->enumlabel);
		char 			*enumlabel2 = NameStr(exists_en2->enumlabel);

		if (strcmp(enumlabel1, enumlabel2))
		{
			isEquivalent = false;
			break;
		}
	}

	pfree(existing1);
	pfree(existing2);
	ReleaseSysCacheList(items1);
	ReleaseSysCacheList(items2);
	
	table_close(enumRel, AccessShareLock);

	return isEquivalent;
}

inline static void 
ENUM_INSERT_CHECK(Oid enumelenmentoid)
{
	if (OidIsValid(enumelenmentoid))
		return;

	ereport(is_strict_mode() ? ERROR : WARNING,
			(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				errmsg("Data truncated for column")));
}

static bool 
number_2_enum_interel(Oid enumtypeoid, uint64 num, Oid *penumelenmentoid)
{
	int32 tempvalue = (int32) num;
	if (tempvalue <= 0)
	{
		int err_flag = is_strict_mode() ? ERROR : WARNING;
		ereport(err_flag,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input value for enum %s: \"%llu\"",
						format_type_be(enumtypeoid),
						(long long unsigned) num)));
		
		return false;
	}
	else
	{
		*penumelenmentoid = get_enum_oid(enumtypeoid, tempvalue);

		ENUM_INSERT_CHECK(*penumelenmentoid);
	}

	return true;
}

Datum
enum_to_enum(PG_FUNCTION_ARGS)
{
	Oid			oid = PG_GETARG_OID(0);
	Oid	enumtypeoid = PG_GETARG_OID(1);
	Oid	enumelenmentoid;

	if (oid == InvalidOid)
		PG_RETURN_OID(InvalidOid);

	enumelenmentoid = check_enumlabel_value(enumtypeoid, get_enum_label(oid));
	ENUM_INSERT_CHECK(enumelenmentoid);

	PG_RETURN_OID(enumelenmentoid);
}

Datum
enum_to_int2(PG_FUNCTION_ARGS)
{
	Oid			oid = PG_GETARG_OID(0);
	int16		val;

	if (oid == InvalidOid)
	{
		val = 0;
	}
	else
	{
		val = (int16) get_enum_sortorder(oid);
	}

	PG_RETURN_INT16(val);
}

Datum
int2_to_enum(PG_FUNCTION_ARGS)
{
	int16 	num = PG_GETARG_INT16(0);
	Oid 	enumtypeoid = PG_GETARG_OID(1);
	Oid 	enumelenmentoid = InvalidOid;

	if (num <= 0)
	{
		ereport(is_strict_mode() ? ERROR : WARNING,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input value for enum %s: \"%d\"",
						format_type_be(enumtypeoid),
						num)));
		
		PG_RETURN_NULL();
	}
	else
	{
		enumelenmentoid = get_enum_oid(enumtypeoid, (int32) num);
		
		ENUM_INSERT_CHECK(enumelenmentoid);
	}

	PG_RETURN_OID(enumelenmentoid);
}

Datum
int4_to_enum(PG_FUNCTION_ARGS)
{
	int32 	num = PG_GETARG_INT32(0);
	Oid		enumtypeoid = PG_GETARG_OID(1);
	Oid 	enumelenmentoid = InvalidOid;

	if (num <= 0)
	{
		ereport(is_strict_mode() ? ERROR : WARNING,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input value for enum %s: \"%d\"",
						format_type_be(enumtypeoid),
						num)));

		PG_RETURN_VOID();
	}
	else
	{
		enumelenmentoid = get_enum_oid(enumtypeoid, (int32) num);

		ENUM_INSERT_CHECK(enumelenmentoid);
	}
	
	PG_RETURN_OID(enumelenmentoid);
}

Datum
enum_to_int4(PG_FUNCTION_ARGS)
{
	Oid			oid = PG_GETARG_OID(0);
	int32		val;

	if (oid == InvalidOid)
	{
		val = 0;
	}
	else
	{
		val = (int32) get_enum_sortorder(oid);
	}

	PG_RETURN_INT32(val);
}

Datum
int8_to_enum(PG_FUNCTION_ARGS)
{
	int64 	num = PG_GETARG_INT64(0);
	Oid		enumtypeoid = PG_GETARG_OID(1);
	Oid		enumelenmentoid;

	if (num <= 0 || num > PG_INT32_MAX)
	{
		ereport(is_strict_mode() ? ERROR : WARNING, 
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input value for enum %s: \"%lld\"", 
						format_type_be(enumtypeoid), 
						(long long) num)));
		
		PG_RETURN_NULL();
	}
	else
	{
		enumelenmentoid = get_enum_oid(enumtypeoid, (int32) num);

		ENUM_INSERT_CHECK(enumelenmentoid);
	}

	PG_RETURN_OID(enumelenmentoid);
}

Datum
enum_to_int8(PG_FUNCTION_ARGS)
{
	Oid			oid = PG_GETARG_OID(0);
	int64		val;

	if (oid == InvalidOid)
	{
		val = 0;
	}
	else
	{
		val = (int64) get_enum_sortorder(oid);
	}

	PG_RETURN_INT64(val);
}

Datum
float4_to_enum(PG_FUNCTION_ARGS)
{
	float4 	num = PG_GETARG_FLOAT4(0);
	Oid		enumtypeoid = PG_GETARG_OID(1);
	Oid 	enumelenmentoid = InvalidOid;

	if (num <= 0)
	{
		ereport(is_strict_mode() ? ERROR : WARNING,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input value for enum %s: \"%f\"",
						format_type_be(enumtypeoid),
						num)));

		PG_RETURN_NULL();
	}
	else
	{
		enumelenmentoid = get_enum_oid(enumtypeoid, (int32) num);

		ENUM_INSERT_CHECK(enumelenmentoid);
	}
	
	PG_RETURN_OID(enumelenmentoid);
}

Datum
enum_to_float4(PG_FUNCTION_ARGS)
{
	Oid			oid = PG_GETARG_OID(0);
	float4		val;

	if (oid == InvalidOid)
	{
		val = 0;
	}
	else
	{
		val = (float4) get_enum_sortorder(oid);
	}

	PG_RETURN_FLOAT4(val);
}

Datum
enum_to_float8(PG_FUNCTION_ARGS)
{
	Oid			oid = PG_GETARG_OID(0);
	float8		val;

	if (oid == InvalidOid)
	{
		val = 0;
	}
	else
	{
		val = get_enum_sortorder(oid);
	}

	PG_RETURN_FLOAT8(val);
}

Datum
float8_to_enum(PG_FUNCTION_ARGS)
{
	float8 	num = PG_GETARG_FLOAT8(0);
	Oid		enumtypeoid = PG_GETARG_OID(1);
	Oid 	enumelenmentoid = InvalidOid;

	if (num <= 0)
	{
		ereport(is_strict_mode() ? ERROR : WARNING,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input value for enum %s: \"%f\"",
						format_type_be(enumtypeoid),
						num)));

		PG_RETURN_NULL();
	}
	else
	{
		enumelenmentoid = get_enum_oid(enumtypeoid, (int32) num);

		ENUM_INSERT_CHECK(enumelenmentoid);
	}
	
	PG_RETURN_OID(enumelenmentoid);
}

Datum
numeric_to_enum(PG_FUNCTION_ARGS)
{
	Oid		enumtypeoid = PG_GETARG_OID(1);
	Oid 	enumelenmentoid = InvalidOid;
	
	Numeric num = DatumGetNumeric(DirectFunctionCall2(numeric_sub, 
													  PG_GETARG_DATUM(0),
											  		  DirectFunctionCall1(float8_numeric, 
																		  Float8GetDatum(0.5))));
	Datum	retValue = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(num)));

	if (retValue <= 0)
	{
		ereport(is_strict_mode() ? ERROR : WARNING,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input value for enum %s: \"%d\"",
						format_type_be(enumtypeoid),
						DatumGetInt32(retValue))));

		PG_RETURN_NULL();
	}
	else
	{
		enumelenmentoid = get_enum_oid(enumtypeoid, DatumGetInt32(retValue));

		ENUM_INSERT_CHECK(enumelenmentoid);
	}
	
	PG_RETURN_OID(enumelenmentoid);
}

Datum
enum_to_numeric(PG_FUNCTION_ARGS)
{
	Numeric		result;
	Oid			oid = PG_GETARG_OID(0);
	float8		val;

	if (oid == InvalidOid)
	{
		val = 0;
	}
	else
	{
		val = get_enum_sortorder(oid);
	}

	result = DatumGetNumeric(DirectFunctionCall1(float8_numeric, Float8GetDatum(val)));

	PG_RETURN_NUMERIC(result);
}

Datum
uint1_to_enum(PG_FUNCTION_ARGS)
{
	uint8	num = PG_GETARG_UINT32(0);
	Oid		enumtypeoid = PG_GETARG_OID(1);
	Oid 	enumelenmentoid = InvalidOid;

	if (!number_2_enum_interel(enumtypeoid, num, &enumelenmentoid))
	{
		PG_RETURN_NULL();
	}
	
	PG_RETURN_OID(enumelenmentoid);
}

Datum
enum_to_uint1(PG_FUNCTION_ARGS)
{
	Oid			oid = PG_GETARG_OID(0);
	uint8		val;

	if (oid == InvalidOid)
	{
		val = 0;
	}
	else
	{
		val = (uint8) get_enum_sortorder(oid);
	}

	PG_RETURN_UINT8(val);
}

Datum
uint2_to_enum(PG_FUNCTION_ARGS)
{
	uint16	num = PG_GETARG_UINT32(0);
	Oid		enumtypeoid = PG_GETARG_OID(1);
	Oid 	enumelenmentoid = InvalidOid;

	if (!number_2_enum_interel(enumtypeoid, num, &enumelenmentoid))
	{
		PG_RETURN_NULL();
	}
	
	PG_RETURN_OID(enumelenmentoid);
}

Datum
enum_to_uint2(PG_FUNCTION_ARGS)
{
	Oid			oid = PG_GETARG_OID(0);
	uint16		val;

	if (oid == InvalidOid)
	{
		val = 0;
	}
	else
	{
		val = (uint16) get_enum_sortorder(oid);
	}

	PG_RETURN_UINT16(val);
}

Datum
uint3_to_enum(PG_FUNCTION_ARGS)
{
	uint32	num = PG_GETARG_UINT32(0);
	Oid		enumtypeoid = PG_GETARG_OID(1);
	Oid		enumelenmentoid = InvalidOid;

	if (!number_2_enum_interel(enumtypeoid, num, &enumelenmentoid))
	{
		PG_RETURN_NULL();
	}
	
	PG_RETURN_OID(enumelenmentoid);
}

Datum
enum_to_uint3(PG_FUNCTION_ARGS)
{
	Oid oid = PG_GETARG_OID(0);
	uint32 val;

	if (oid == InvalidOid)
	{
		val = 0;
	}
	else
	{
		val = (uint32) get_enum_sortorder(oid);
	}

	PG_RETURN_UINT32(val);
}

Datum
uint4_to_enum(PG_FUNCTION_ARGS)
{
	uint32	num = PG_GETARG_UINT32(0);
	Oid		enumtypeoid = PG_GETARG_OID(1);
	Oid		enumelenmentoid = InvalidOid;

	if (!number_2_enum_interel(enumtypeoid, num, &enumelenmentoid))
	{
		PG_RETURN_NULL();
	}
	
	PG_RETURN_OID(enumelenmentoid);
}

Datum
enum_to_uint4(PG_FUNCTION_ARGS)
{
	Oid oid = PG_GETARG_OID(0);
	uint32 val;

	if (oid == InvalidOid)
	{
		val = 0;
	}
	else
	{
		val = (uint32) get_enum_sortorder(oid);
	}

	PG_RETURN_UINT32(val);
}

Datum
uint8_to_enum(PG_FUNCTION_ARGS)
{
	uint64	num = PG_GETARG_UINT64(0);
	Oid		enumtypeoid = PG_GETARG_OID(1);
	Oid		enumelenmentoid;

	if (!number_2_enum_interel(enumtypeoid, num, &enumelenmentoid))
	{
		PG_RETURN_NULL();
	}

	PG_RETURN_OID(enumelenmentoid);
}

Datum
enum_to_uint8(PG_FUNCTION_ARGS)
{
	Oid oid = PG_GETARG_OID(0);
	uint64 val;

	if (oid == InvalidOid)
	{
		val = 0;
	}
	else
	{
		val = (uint64) get_enum_sortorder(oid);
	}

	PG_RETURN_UINT64(val);
}

static Oid
check_enumlabel_value(Oid enumtypid, char *enumlabel)
{
	HeapTuple tup;
	Form_pg_enum en;
	Oid enumoid;

	tup = SearchSysCache2(ENUMTYPOIDNAME, ObjectIdGetDatum(enumtypid), CStringGetDatum(enumlabel));
	if (!HeapTupleIsValid(tup))
	{
		return InvalidOid;
	}
	en = (Form_pg_enum) GETSTRUCT(tup);

	enumoid = en->oid;
	ReleaseSysCache(tup);

	return enumoid;
}

Datum
text_to_enum(PG_FUNCTION_ARGS)
{
	text	*val = PG_GETARG_TEXT_P(0);
	Oid		enumtypeoid = PG_GETARG_OID(1);
	char	*str1 = text_to_cstring(val);
	Oid 	enumelenmentoid = check_enumlabel_value(enumtypeoid, str1);
	bool	is_insert = CALL_IN_INSERT();

	if (enumelenmentoid == InvalidOid)
	{
		int32  idx = mysql_int4_datatype_in_hook(str1, is_insert);

		if (idx <= 0 || idx > PG_INT32_MAX)
		{
			ereport(is_strict_mode() ? ERROR : WARNING, 
					(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					 errmsg("invalid input value for enum %s: \"%d\"", 
							format_type_be(enumtypeoid), 
							idx)));

			PG_RETURN_NULL();
		}
		else
		{
			enumelenmentoid = get_enum_oid(enumtypeoid, idx);

			ENUM_INSERT_CHECK(enumelenmentoid);
		}
	}

	PG_RETURN_OID(enumelenmentoid);
}

Datum
enum_to_text(PG_FUNCTION_ARGS)
{
	Oid 	oid = PG_GETARG_OID(0);
	char 	*enumlabel = get_enum_label(oid);

	PG_RETURN_TEXT_P(cstring_to_text(enumlabel));
}

Datum
bool_to_enum(PG_FUNCTION_ARGS)
{
	bool 	val = PG_GETARG_BOOL(0);
	Oid		enumtypeoid = PG_GETARG_OID(1);
	Oid enumelenmentoid = get_enum_oid(enumtypeoid, val ? 1 : 0);

	ENUM_INSERT_CHECK(enumelenmentoid);
	
	PG_RETURN_OID(enumelenmentoid);
}

Datum
enum_to_bool(PG_FUNCTION_ARGS)
{
	Oid oid1 = PG_GETARG_OID(0);

	if (0 == oid1)
	{
		PG_RETURN_BOOL(false);
	}

	PG_RETURN_BOOL(true);
}
/* End - ReqID:SRS-MYSQL-ENUMTYPE */

/* Begin - bug#z5768 */
Datum
enum_to_varbinary(PG_FUNCTION_ARGS)
{
	Oid 	oid = PG_GETARG_OID(0);
	char 	*enumlabel = get_enum_label(oid);

	PG_RETURN_BYTEA_P(cstring_to_text(enumlabel));
}
/* End - bug#z5768 */

/* Begin - bug#6027 */
Datum
hex_to_enum(PG_FUNCTION_ARGS)
{
	Datum ret = DirectFunctionCall2(text_to_enum,
									PG_GETARG_DATUM(0),
									PG_GETARG_DATUM(1));

	PG_RETURN_DATUM(ret);
}
/* End - bug#6027 */