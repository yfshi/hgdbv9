/*
 * contrib/mys_btree_gist/btree_year.c
 */
#include "postgres.h"

#include <limits.h>

#include "btree_gist.h"
#include "btree_utils_num.h"
#include "utils/builtins.h"
#include "utils/datetime.h"
#include "utils/date.h"
#include "utils/float.h"
#include "../../contrib/ivorysql_my/src/datatype/myyear.h"

typedef struct
{
	YearADT	lower;
	YearADT	upper;
} YearKEY;

/*
 * year ops
 */
PG_FUNCTION_INFO_V1(gbt_year_consistent);
PG_FUNCTION_INFO_V1(gbt_year_union);
PG_FUNCTION_INFO_V1(gbt_year_compress);
// PG_FUNCTION_INFO_V1(gbt_year_decompress);
PG_FUNCTION_INFO_V1(gbt_year_penalty);
PG_FUNCTION_INFO_V1(gbt_year_picksplit);
PG_FUNCTION_INFO_V1(gbt_year_same);

static int year_cmp_internal(YearADT y1, YearADT y2);

static bool
gbt_year_gt(const void *a, const void *b, FmgrInfo *flinfo)
{
	const YearADT *aa = (const YearADT *) a;
	const YearADT *bb = (const YearADT *) b;

	(void) flinfo;		/* not used */

	return DatumGetBool(year_cmp_internal(*aa, *bb) > 0);
}

static bool
gbt_year_ge(const void *a, const void *b, FmgrInfo *flinfo)
{
	const TimeADT *aa = (const TimeADT *) a;
	const TimeADT *bb = (const TimeADT *) b;

	(void) flinfo;		/* not used */
	return DatumGetBool(year_cmp_internal(*aa, *bb) >= 0);
}

static bool
gbt_year_eq(const void *a, const void *b, FmgrInfo *flinfo)
{
	const TimeADT *aa = (const TimeADT *) a;
	const TimeADT *bb = (const TimeADT *) b;

	(void) flinfo;		/* not used */
	return DatumGetBool(year_cmp_internal(*aa, *bb) == 0);
}

static bool
gbt_year_lt(const void *a, const void *b, FmgrInfo *flinfo)
{
	const TimeADT *aa = (const TimeADT *) a;
	const TimeADT *bb = (const TimeADT *) b;

	(void) flinfo;		/* not used */
	return DatumGetBool(year_cmp_internal(*aa, *bb) < 0);
}

static bool
gbt_year_le(const void *a, const void *b, FmgrInfo *flinfo)
{
	const TimeADT *aa = (const TimeADT *) a;
	const TimeADT *bb = (const TimeADT *) b;

	(void) flinfo;		/* not used */
	return DatumGetBool(year_cmp_internal(*aa, *bb) <= 0);
}

static int
gbt_yearkey_cmp(const void *a, const void *b, FmgrInfo *flinfo)
{
	YearKEY	   *ia = (YearKEY *) (((const Nsrt *) a)->t);
	YearKEY	   *ib = (YearKEY *) (((const Nsrt *) b)->t);
	int			res;

	(void) flinfo;		/* not used */
	res = year_cmp_internal(ia->lower, ib->lower);
	if (res == 0)
		return year_cmp_internal(ia->upper, ib->upper);

	return res;
}

static const gbtree_ninfo tinfo =
{
	gbt_t_uint1,
	sizeof(YearADT),
	sizeof(YearKEY),
	gbt_year_gt,
	gbt_year_ge,
	gbt_year_eq,
	gbt_year_le,
	gbt_year_lt,
	gbt_yearkey_cmp,
	NULL
};

/**************************************************
 * year ops
 **************************************************/

Datum
gbt_year_consistent(PG_FUNCTION_ARGS)
{
	GISTENTRY  *entry = (GISTENTRY *) PG_GETARG_POINTER(0);
	YearADT	query = PG_GETARG_TIMEADT(1);
	StrategyNumber strategy = (StrategyNumber) PG_GETARG_UINT16(2);

	/* Oid		subtype = PG_GETARG_OID(3); */
	bool	   *recheck = (bool *) PG_GETARG_POINTER(4);
	YearKEY	   *kkk = (YearKEY *) DatumGetPointer(entry->key);
	GBT_NUMKEY_R key;

	/* All cases served by this function are exact */
	*recheck = false;

	key.lower = (GBT_NUMKEY *) &kkk->lower;
	key.upper = (GBT_NUMKEY *) &kkk->upper;

	PG_RETURN_BOOL(gbt_num_consistent(&key, (void *) &query, &strategy,
									  GIST_LEAF(entry), &tinfo, fcinfo->flinfo));
}

Datum
gbt_year_union(PG_FUNCTION_ARGS)
{
	GistEntryVector *entryvec = (GistEntryVector *) PG_GETARG_POINTER(0);
	void	   *out = palloc(sizeof(YearKEY));

	*(int *) PG_GETARG_POINTER(1) = sizeof(YearKEY);
	PG_RETURN_POINTER(gbt_num_union((void *) out, entryvec, &tinfo, fcinfo->flinfo));
}

Datum
gbt_year_compress(PG_FUNCTION_ARGS)
{
	GISTENTRY  *entry = (GISTENTRY *) PG_GETARG_POINTER(0);

	PG_RETURN_POINTER(gbt_num_compress(entry, &tinfo));
}

#define penalty_check_max_float(val) \
	do { \
		if ( val > FLT_MAX ) \
				val = FLT_MAX; \
		if ( val < -FLT_MAX ) \
				val = -FLT_MAX; \
	} while (0)

Datum
gbt_year_penalty(PG_FUNCTION_ARGS)
{
	YearKEY	   *origentry = (YearKEY *) DatumGetPointer(((GISTENTRY *) PG_GETARG_POINTER(0))->key);
	YearKEY	   *newentry  = (YearKEY *) DatumGetPointer(((GISTENTRY *) PG_GETARG_POINTER(1))->key);
	float	   *result = (float *) PG_GETARG_POINTER(2);

	double		orgdbl[2],
				newdbl[2];

	/*
	 * We are always using "double" timestamps here. Precision should be good
	 * enough.
	 */
	orgdbl[0] = ((double) origentry->lower);
	orgdbl[1] = ((double) origentry->upper);
	newdbl[0] = ((double) newentry->lower);
	newdbl[1] = ((double) newentry->upper);

	penalty_check_max_float(orgdbl[0]);
	penalty_check_max_float(orgdbl[1]);
	penalty_check_max_float(newdbl[0]);
	penalty_check_max_float(newdbl[1]);

	penalty_num(result, orgdbl[0], orgdbl[1], newdbl[0], newdbl[1]);

	PG_RETURN_POINTER(result);
}

Datum
gbt_year_picksplit(PG_FUNCTION_ARGS)
{
	PG_RETURN_POINTER(gbt_num_picksplit((GistEntryVector *) PG_GETARG_POINTER(0),
										(GIST_SPLITVEC *) PG_GETARG_POINTER(1),
										&tinfo, fcinfo->flinfo));
}

Datum
gbt_year_same(PG_FUNCTION_ARGS)
{
	YearKEY	   *b1 = (YearKEY *) PG_GETARG_POINTER(0);
	YearKEY	   *b2 = (YearKEY *) PG_GETARG_POINTER(1);
	bool	   *result = (bool *) PG_GETARG_POINTER(2);

	*result = gbt_num_same((void *) b1, (void *) b2, &tinfo, fcinfo->flinfo);
	PG_RETURN_POINTER(result);
}

static int
year_cmp_internal(YearADT y1, YearADT y2)
{
	if (IS_YEAR2(y1))
		YEAR_CONVERT(y1);
	if (IS_YEAR2(y2))
		YEAR_CONVERT(y2);

	return (y1 < y2) ? -1 : ((y1 > y2) ? 1 : 0);
}
