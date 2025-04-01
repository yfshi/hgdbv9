/*
 * contrib/ora_btree_gist/btree_timestamp.c
 */
#include "postgres.h"

#include <limits.h>

#include "btree_gist.h"
#include "btree_utils_num.h"
#include "utils/builtins.h"
#include "utils/datetime.h"
#include "utils/date.h"
#include "utils/float.h"

typedef struct
{
	TimeADT	lower;
	TimeADT	upper;
} datetimeKEY;

/*
** timestamp ops
*/
PG_FUNCTION_INFO_V1(gbt_mystime_compress);
PG_FUNCTION_INFO_V1(gbt_mystime_fetch);
PG_FUNCTION_INFO_V1(gbt_mystime_union);
PG_FUNCTION_INFO_V1(gbt_mystime_picksplit);
PG_FUNCTION_INFO_V1(gbt_mystime_consistent);
PG_FUNCTION_INFO_V1(gbt_mystime_distance);
PG_FUNCTION_INFO_V1(gbt_mystime_penalty);
PG_FUNCTION_INFO_V1(gbt_mystime_same);


#ifdef USE_FLOAT8_BYVAL
#define DatetimeGetDatumFast(X) TimeADTGetDatum(X)
#else
#define DatetimeGetDatumFast(X) PointerGetDatum(&(X))
#endif


static bool
gbt_timegt(const void *a, const void *b, FmgrInfo *flinfo)
{
	const TimeADT *aa = (const TimeADT *) a;
	const TimeADT *bb = (const TimeADT *) b;

	(void) flinfo;		/* not used */
	return DatumGetBool(DirectFunctionCall2(timestamp_gt,
											DatetimeGetDatumFast(*aa),
											DatetimeGetDatumFast(*bb)));
}

static bool
gbt_timege(const void *a, const void *b, FmgrInfo *flinfo)
{
	const TimeADT *aa = (const TimeADT *) a;
	const TimeADT *bb = (const TimeADT *) b;

	(void) flinfo;		/* not used */
	return DatumGetBool(DirectFunctionCall2(timestamp_ge,
											DatetimeGetDatumFast(*aa),
											DatetimeGetDatumFast(*bb)));
}

static bool
gbt_timeeq(const void *a, const void *b, FmgrInfo *flinfo)
{
	const TimeADT *aa = (const TimeADT *) a;
	const TimeADT *bb = (const TimeADT *) b;

	(void) flinfo;		/* not used */
	return DatumGetBool(DirectFunctionCall2(timestamp_eq,
											DatetimeGetDatumFast(*aa),
											DatetimeGetDatumFast(*bb)));
}

static bool
gbt_timele(const void *a, const void *b, FmgrInfo *flinfo)
{
	const TimeADT *aa = (const TimeADT *) a;
	const TimeADT *bb = (const TimeADT *) b;

	(void) flinfo;		/* not used */
	return DatumGetBool(DirectFunctionCall2(timestamp_le,
											DatetimeGetDatumFast(*aa),
											DatetimeGetDatumFast(*bb)));
}

static bool
gbt_timelt(const void *a, const void *b, FmgrInfo *flinfo)
{
	const TimeADT *aa = (const TimeADT *) a;
	const TimeADT *bb = (const TimeADT *) b;

	(void) flinfo;		/* not used */
	return DatumGetBool(DirectFunctionCall2(timestamp_lt,
											DatetimeGetDatumFast(*aa),
											DatetimeGetDatumFast(*bb)));
}


static int
gbt_timekey_cmp(const void *a, const void *b, FmgrInfo *flinfo)
{
	datetimeKEY	   *ia = (datetimeKEY *) (((const Nsrt *) a)->t);
	datetimeKEY	   *ib = (datetimeKEY *) (((const Nsrt *) b)->t);
	int			res;

	(void) flinfo;		/* not used */
	res = DatumGetInt32(DirectFunctionCall2(timestamp_cmp, DatetimeGetDatumFast(ia->lower), DatetimeGetDatumFast(ib->lower)));
	if (res == 0)
		return DatumGetInt32(DirectFunctionCall2(timestamp_cmp, DatetimeGetDatumFast(ia->upper), DatetimeGetDatumFast(ib->upper)));

	return res;
}

static float8
gbt_time_dist(const void *a, const void *b, FmgrInfo *flinfo)
{
	const TimeADT *aa = (const TimeADT *) a;
	const TimeADT *bb = (const TimeADT *) b;
	Interval   *i;

	(void) flinfo;		/* not used */
	if (TIMESTAMP_NOT_FINITE(*aa) || TIMESTAMP_NOT_FINITE(*bb))
		return get_float8_infinity();

	i = DatumGetIntervalP(DirectFunctionCall2(timestamp_mi,
											  DatetimeGetDatumFast(*aa),
											  DatetimeGetDatumFast(*bb)));
	return fabs(INTERVAL_TO_SEC(i));
}


static const gbtree_ninfo tinfo =
{
	gbt_t_ts,
	sizeof(TimeADT),
	16,							/* sizeof(gbtreekey16) */
	gbt_timegt,
	gbt_timege,
	gbt_timeeq,
	gbt_timele,
	gbt_timelt,
	gbt_timekey_cmp,
	gbt_time_dist
};


PG_FUNCTION_INFO_V1(mystime_dist);
Datum
mystime_dist(PG_FUNCTION_ARGS)
{
	TimeADT	a = PG_GETARG_TIMEADT(0);
	TimeADT	b = PG_GETARG_TIMEADT(1);
	Interval   *r;

	if (TIMESTAMP_NOT_FINITE(a) || TIMESTAMP_NOT_FINITE(b))
	{
		Interval   *p = palloc(sizeof(Interval));

		p->day = INT_MAX;
		p->month = INT_MAX;
		p->time = PG_INT64_MAX;
		PG_RETURN_INTERVAL_P(p);
	}
	else
		r = DatumGetIntervalP(DirectFunctionCall2(timestamp_mi,
												  PG_GETARG_DATUM(0),
												  PG_GETARG_DATUM(1)));
	PG_RETURN_INTERVAL_P(abs_interval(r));
}


/**************************************************
 * timestamp ops
 **************************************************/
Datum
gbt_mystime_compress(PG_FUNCTION_ARGS)
{
	GISTENTRY  *entry = (GISTENTRY *) PG_GETARG_POINTER(0);

	PG_RETURN_POINTER(gbt_num_compress(entry, &tinfo));
}

Datum
gbt_mystime_fetch(PG_FUNCTION_ARGS)
{
	GISTENTRY  *entry = (GISTENTRY *) PG_GETARG_POINTER(0);

	PG_RETURN_POINTER(gbt_num_fetch(entry, &tinfo));
}

Datum
gbt_mystime_consistent(PG_FUNCTION_ARGS)
{
	GISTENTRY  *entry = (GISTENTRY *) PG_GETARG_POINTER(0);
	TimeADT	query = PG_GETARG_TIMEADT(1);
	StrategyNumber strategy = (StrategyNumber) PG_GETARG_UINT16(2);

	/* Oid		subtype = PG_GETARG_OID(3); */
	bool	   *recheck = (bool *) PG_GETARG_POINTER(4);
	datetimeKEY	   *kkk = (datetimeKEY *) DatumGetPointer(entry->key);
	GBT_NUMKEY_R key;

	/* All cases served by this function are exact */
	*recheck = false;

	key.lower = (GBT_NUMKEY *) &kkk->lower;
	key.upper = (GBT_NUMKEY *) &kkk->upper;

	PG_RETURN_BOOL(gbt_num_consistent(&key, (void *) &query, &strategy,
									  GIST_LEAF(entry), &tinfo, fcinfo->flinfo));
}

Datum
gbt_mystime_distance(PG_FUNCTION_ARGS)
{
	GISTENTRY  *entry = (GISTENTRY *) PG_GETARG_POINTER(0);
	TimeADT	query = PG_GETARG_TIMEADT(1);

	/* Oid		subtype = PG_GETARG_OID(3); */
	datetimeKEY	   *kkk = (datetimeKEY *) DatumGetPointer(entry->key);
	GBT_NUMKEY_R key;

	key.lower = (GBT_NUMKEY *) &kkk->lower;
	key.upper = (GBT_NUMKEY *) &kkk->upper;

	PG_RETURN_FLOAT8(gbt_num_distance(&key, (void *) &query, GIST_LEAF(entry),
									  &tinfo, fcinfo->flinfo));
}

Datum
gbt_mystime_union(PG_FUNCTION_ARGS)
{
	GistEntryVector *entryvec = (GistEntryVector *) PG_GETARG_POINTER(0);
	void	   *out = palloc(sizeof(datetimeKEY));

	*(int *) PG_GETARG_POINTER(1) = sizeof(datetimeKEY);
	PG_RETURN_POINTER(gbt_num_union((void *) out, entryvec, &tinfo, fcinfo->flinfo));
}


#define penalty_check_max_float(val) \
	do { \
		if ( val > FLT_MAX ) \
				val = FLT_MAX; \
		if ( val < -FLT_MAX ) \
				val = -FLT_MAX; \
	} while (0)


Datum
gbt_mystime_penalty(PG_FUNCTION_ARGS)
{
	datetimeKEY	   *origentry = (datetimeKEY *) DatumGetPointer(((GISTENTRY *) PG_GETARG_POINTER(0))->key);
	datetimeKEY	   *newentry = (datetimeKEY *) DatumGetPointer(((GISTENTRY *) PG_GETARG_POINTER(1))->key);
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
gbt_mystime_picksplit(PG_FUNCTION_ARGS)
{
	PG_RETURN_POINTER(gbt_num_picksplit((GistEntryVector *) PG_GETARG_POINTER(0),
										(GIST_SPLITVEC *) PG_GETARG_POINTER(1),
										&tinfo, fcinfo->flinfo));
}

Datum
gbt_mystime_same(PG_FUNCTION_ARGS)
{
	datetimeKEY	   *b1 = (datetimeKEY *) PG_GETARG_POINTER(0);
	datetimeKEY	   *b2 = (datetimeKEY *) PG_GETARG_POINTER(1);
	bool	   *result = (bool *) PG_GETARG_POINTER(2);

	*result = gbt_num_same((void *) b1, (void *) b2, &tinfo, fcinfo->flinfo);
	PG_RETURN_POINTER(result);
}
