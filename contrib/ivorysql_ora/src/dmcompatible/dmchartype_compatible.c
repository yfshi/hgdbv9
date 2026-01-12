/*------------------------------------------------------------------
 * 
 * File: dmchartype_compatible.c
 *
 * Abstract: 
 * 		Compatible with DM character data type.
 *
 * Authored by huawenbo@highgo.com, 20240407.
 *
 * Copyright:
 * Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_ora/src/dmcompatible/dmchartype_compatible.c
 *
 *------------------------------------------------------------------
 */
/* ReqId: SRS-MISC-COMPATDM-SQL */
#include "postgres.h"
#include "access/hash.h"
#include "access/detoast.h"
#include "catalog/pg_collation.h"
#include "libpq/pqformat.h"
#include "nodes/nodeFuncs.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/ora_compatible.h" /* BUG#Z091 */
#include "mb/pg_wchar.h"
#include "utils/varlena.h"
#include "fmgr.h"
#include "catalog/namespace.h"
#include "utils/xml.h"
#include "utils/pg_locale.h"
#include "lib/hyperloglog.h"
#include "utils/memutils.h"
#define MaxVarcharSize 32767	/*BUG:000116*/
#define TEXTBUFLEN		1024
#define DatumGetVarStringPP(X)		((VarString *) PG_DETOAST_DATUM_PACKED(X))

PG_FUNCTION_INFO_V1(oravarchareq_explicit);
PG_FUNCTION_INFO_V1(oravarcharne_explicit);
PG_FUNCTION_INFO_V1(oravarcharlt_explicit);
PG_FUNCTION_INFO_V1(oravarchargt_explicit);
PG_FUNCTION_INFO_V1(oravarcharle_explicit);
PG_FUNCTION_INFO_V1(oravarcharge_explicit);
PG_FUNCTION_INFO_V1(texteq_explicit);
PG_FUNCTION_INFO_V1(textne_explicit);
PG_FUNCTION_INFO_V1(text_lt_explicit);
PG_FUNCTION_INFO_V1(text_gt_explicit);
PG_FUNCTION_INFO_V1(text_ge_explicit);
PG_FUNCTION_INFO_V1(text_le_explicit);
PG_FUNCTION_INFO_V1(hashoratext);
PG_FUNCTION_INFO_V1(bt_oravarchar_cmp_explicit);
PG_FUNCTION_INFO_V1(orabttextcmp_explicit);
PG_FUNCTION_INFO_V1(btoravarcharcharsortsupport_explicit);
PG_FUNCTION_INFO_V1(btoravarcharbytesortsupport_explicit);
PG_FUNCTION_INFO_V1(btoratextsortsupport_explicit);

typedef struct
{
	char	   *buf1;			/* 1st string, or abbreviation original string
								 * buf */
	char	   *buf2;			/* 2nd string, or abbreviation strxfrm() buf */
	int			buflen1;		/* Allocated length of buf1 */
	int			buflen2;		/* Allocated length of buf2 */
	int			last_len1;		/* Length of last buf1 string/strxfrm() input */
	int			last_len2;		/* Length of last buf2 string/strxfrm() blob */
	int			last_returned;	/* Last comparison result (cache) */
	bool		cache_blob;		/* Does buf2 contain strxfrm() blob, etc? */
	bool		collate_c;
	Oid			typid;			/* Actual datatype (text/bpchar/bytea/name) */
	hyperLogLogState abbr_card; /* Abbreviated key cardinality state */
	hyperLogLogState full_card; /* Full key cardinality state */
	double		prop_card;		/* Required cardinality proportion */
	pg_locale_t locale;
} VarStringSortSupport;

typedef struct varlena VarString;

static int	varstrfastcmp_c_explicit(Datum x, Datum y, SortSupport ssup);
static int	varlenafastcmp_locale_explicit(Datum x, Datum y, SortSupport ssup);
static void varstr_sortsupport_explicit(SortSupport ssup, Oid typid, Oid collid);

static void
check_collation_set(Oid collid)
{
	if (!OidIsValid(collid))
	{
		/*
		 * This typically means that the parser could not resolve a conflict
		 * of implicit collations, so report it that way.
		 */
		ereport(ERROR,
				(errcode(ERRCODE_INDETERMINATE_COLLATION),
				 errmsg("could not determine which collation to use for string comparison"),
				 errhint("Use the COLLATE clause to set the collation explicitly.")));
	}
}

 /* This is like oravarcharchar_cmp(),but if !enable_blankspace_compare,
 * do not compare blank space in the tail.
 * Currently this is used just for oravarcharlt,oravarchargt,oravarcharle,oravarcharge.
 * Internal comparison function for oravarchar.
 * Returns -1, 0 or 1
 */
static int
oravarcharchar_cmp_plain(VarChar *arg1, VarChar *arg2, Oid collid)
{
	char	   *a1p,
			   *a2p;
	int			len1,
				len2;
	/* if not compare space,we need to calculate length without blankspace in the tail. */
	a1p = VARDATA_ANY(arg1);
	a2p = VARDATA_ANY(arg2);

	len1 = bpchartruelen(a1p,VARSIZE_ANY_EXHDR(arg1));
	len2 = bpchartruelen(a2p,VARSIZE_ANY_EXHDR(arg2));

	return varstr_cmp(a1p, len1, a2p, len2, collid);
}


Datum
oravarchareq_explicit(PG_FUNCTION_ARGS)
{
	Oid 		collid = PG_GET_COLLATION();
	Datum		arg1 = PG_GETARG_DATUM(0);
	Datum		arg2 = PG_GETARG_DATUM(1);
	bool		result;
	Size		len1,
				len2;
	VarChar		*targ1,
				*targ2;		
	pg_locale_t locale = 0;
	len1 = toast_raw_datum_size(arg1);
	len2 = toast_raw_datum_size(arg2);
	targ1 = DatumGetVarCharPP(arg1);
	targ2 = DatumGetVarCharPP(arg2);
	/* Get the length reduced space in the tail,excluding VARHDR. */
	len1 = bpchartruelen(VARDATA_ANY(targ1),VARSIZE_ANY_EXHDR(targ1));
	len2 = bpchartruelen(VARDATA_ANY(targ2),VARSIZE_ANY_EXHDR(targ2));
			
	
	check_collation_set(collid);
	
	if (lc_collate_is_c(collid) ||
		collid == DEFAULT_COLLATION_OID ||
		((locale = pg_newlocale_from_collation(collid)) && locale->deterministic))
	{				
		if (len1 != len2)
			result = false;
		else
		{
			VarChar	   *targ1 = DatumGetVarCharPP(arg1);
			VarChar	   *targ2 = DatumGetVarCharPP(arg2);

			result = (memcmp(VARDATA_ANY(targ1), VARDATA_ANY(targ2),
							 len1) == 0);

			PG_FREE_IF_COPY(targ1, 0);
			PG_FREE_IF_COPY(targ2, 1);
		}
	}
	else
	{
		/* Begin - Req: SRS-SQL-NLSSORT */
		VarChar	   *arg1 = PG_GETARG_VARCHAR_PP(0);
		VarChar    *arg2 = PG_GETARG_VARCHAR_PP(1);

		result = (oravarcharchar_cmp_plain(arg1, arg2, collid) == 0);

		PG_FREE_IF_COPY(arg1, 0);
		PG_FREE_IF_COPY(arg2, 1);
		/* End - Req: SRS-SQL-NLSSORT */
	}

	PG_RETURN_BOOL(result);

}

Datum
oravarcharne_explicit(PG_FUNCTION_ARGS)
{
	Oid 		collid = PG_GET_COLLATION();
	Datum		arg1 = PG_GETARG_DATUM(0);
	Datum		arg2 = PG_GETARG_DATUM(1);
	bool		result;
	Size		len1,
				len2;
	VarChar		*targ1,
				*targ2;		
	pg_locale_t locale = 0;
	targ1 = DatumGetVarCharPP(arg1);
	targ2 = DatumGetVarCharPP(arg2);
	/* Get the length reduced space in the tail,excluding VARHDR. */
	len1 = bpchartruelen(VARDATA_ANY(targ1),VARSIZE_ANY_EXHDR(targ1));
	len2 = bpchartruelen(VARDATA_ANY(targ2),VARSIZE_ANY_EXHDR(targ2));
			
	
	check_collation_set(collid);
	
	if (lc_collate_is_c(collid) ||
		collid == DEFAULT_COLLATION_OID ||
		((locale = pg_newlocale_from_collation(collid)) && locale->deterministic))
	{				
		if (len1 != len2)
			result = true;
		else
		{
			VarChar	   *targ1 = DatumGetVarCharPP(arg1);
			VarChar	   *targ2 = DatumGetVarCharPP(arg2);

			result = (memcmp(VARDATA_ANY(targ1), VARDATA_ANY(targ2),
							 len1) != 0);

			PG_FREE_IF_COPY(targ1, 0);
			PG_FREE_IF_COPY(targ2, 1);
		}
	}
	else
	{
		/* Begin - Req: SRS-SQL-NLSSORT */
		VarChar	   *arg1 = PG_GETARG_VARCHAR_PP(0);
		VarChar    *arg2 = PG_GETARG_VARCHAR_PP(1);

		result = (oravarcharchar_cmp_plain(arg1, arg2, collid) != 0);

		PG_FREE_IF_COPY(arg1, 0);
		PG_FREE_IF_COPY(arg2, 1);
		/* End - Req: SRS-SQL-NLSSORT */
	}

	PG_RETURN_BOOL(result);
}

Datum
oravarcharlt_explicit(PG_FUNCTION_ARGS)
{
	VarChar	   *arg1 = PG_GETARG_VARCHAR_PP(0);
	VarChar	   *arg2 = PG_GETARG_VARCHAR_PP(1);
	bool		result;

	result = (oravarcharchar_cmp_plain(arg1, arg2, PG_GET_COLLATION()) < 0);

	PG_FREE_IF_COPY(arg1, 0);
	PG_FREE_IF_COPY(arg2, 1);

	PG_RETURN_BOOL(result);
}

Datum
oravarcharle_explicit(PG_FUNCTION_ARGS)
{
	VarChar	   *arg1 = PG_GETARG_VARCHAR_PP(0);
	VarChar	   *arg2 = PG_GETARG_VARCHAR_PP(1);
	bool		result;

	result = (oravarcharchar_cmp_plain(arg1, arg2, PG_GET_COLLATION()) <= 0);

	PG_FREE_IF_COPY(arg1, 0);
	PG_FREE_IF_COPY(arg2, 1);

	PG_RETURN_BOOL(result);
}

Datum
oravarchargt_explicit(PG_FUNCTION_ARGS)
{
	VarChar	   *arg1 = PG_GETARG_VARCHAR_PP(0);
	VarChar	   *arg2 = PG_GETARG_VARCHAR_PP(1);
	bool		result;

	result = (oravarcharchar_cmp_plain(arg1, arg2, PG_GET_COLLATION()) > 0);

	PG_FREE_IF_COPY(arg1, 0);
	PG_FREE_IF_COPY(arg2, 1);

	PG_RETURN_BOOL(result);
}

Datum
oravarcharge_explicit(PG_FUNCTION_ARGS)
{
	VarChar	   *arg1 = PG_GETARG_VARCHAR_PP(0);
	VarChar	   *arg2 = PG_GETARG_VARCHAR_PP(1);
	bool		result;

	result = (oravarcharchar_cmp_plain(arg1, arg2, PG_GET_COLLATION()) >= 0);

	PG_FREE_IF_COPY(arg1, 0);
	PG_FREE_IF_COPY(arg2, 1);

	PG_RETURN_BOOL(result);
}

/* Text explicit compare operators */

/* 
 * This is like text_cmp(),but if !enable_blankspace_compare,
 * do not compare blank space in the tail.
 * Currently this is used just for texteq,textneq,textlt,textle,textgt,textge.
 * Internal comparison function for text strings.
 * Returns -1, 0 or 1
 */


static int
text_cmp_plain(text *arg1, text *arg2, Oid collid)
{
	char	   *a1p,
			   *a2p;
	int			len1,
				len2;
	a1p = VARDATA_ANY(arg1);
	a2p = VARDATA_ANY(arg2);
	/* if not compare blank space in the tail,get the 'true' length then let varstr_cmp handle it. */
	len1 = bpchartruelen(a1p,VARSIZE_ANY_EXHDR(arg1));
	len2 = bpchartruelen(a2p,VARSIZE_ANY_EXHDR(arg2));

	return varstr_cmp(a1p, len1, a2p, len2, collid);
}


/*
 * Comparison functions for text strings.
 *
 * Note: btree indexes need these routines not to leak memory; therefore,
 * be careful to free working copies of toasted datums.  Most places don't
 * need to be so careful.
 */

Datum
texteq_explicit(PG_FUNCTION_ARGS)
{
	Oid			collid = PG_GET_COLLATION();
	bool		result;
	pg_locale_t locale = 0;

	check_collation_set(collid);

	if (lc_collate_is_c(collid) ||
		collid == DEFAULT_COLLATION_OID ||
		((locale = pg_newlocale_from_collation(collid)) && locale->deterministic))
	{
		Datum		arg1 = PG_GETARG_DATUM(0);
		Datum		arg2 = PG_GETARG_DATUM(1);
		Size		len1,
					len2;
		text		*targ1,
					*targ2;
		/*
		 * Since we only care about equality or not-equality, we can avoid all
		 * the expense of strcoll() here, and just do bitwise comparison.  In
		 * fact, we don't even have to do a bitwise comparison if we can show
		 * the lengths of the strings are unequal; which might save us from
		 * having to detoast one or both values.
		 */
		len1 = toast_raw_datum_size(arg1);
		len2 = toast_raw_datum_size(arg2);
		targ1 = DatumGetTextPP(arg1);
		targ2 = DatumGetTextPP(arg2);
		
		len1 = bpchartruelen(VARDATA_ANY(targ1),VARSIZE_ANY_EXHDR(targ1));
		len2 = bpchartruelen(VARDATA_ANY(targ2),VARSIZE_ANY_EXHDR(targ2));
		if (len1 != len2)
			result = false;
		else
		{
			targ1 = DatumGetTextPP(arg1);
			targ2 = DatumGetTextPP(arg2);

			result = (memcmp(VARDATA_ANY(targ1), VARDATA_ANY(targ2),
							 len1) == 0);

			PG_FREE_IF_COPY(targ1, 0);
			PG_FREE_IF_COPY(targ2, 1);
		}
	}
	else
	{
		text	   *arg1 = PG_GETARG_TEXT_PP(0);
		text	   *arg2 = PG_GETARG_TEXT_PP(1);

		result = (text_cmp_plain(arg1, arg2, collid) == 0);

		PG_FREE_IF_COPY(arg1, 0);
		PG_FREE_IF_COPY(arg2, 1);
	}

	PG_RETURN_BOOL(result);
}

Datum
textne_explicit(PG_FUNCTION_ARGS)
{
	Oid			collid = PG_GET_COLLATION();
	bool		result;
	pg_locale_t locale = 0;

	check_collation_set(collid);

	if (lc_collate_is_c(collid) ||
		collid == DEFAULT_COLLATION_OID ||
		((locale = pg_newlocale_from_collation(collid)) && locale->deterministic))
	{
		Datum		arg1 = PG_GETARG_DATUM(0);
		Datum		arg2 = PG_GETARG_DATUM(1);
		Size		len1,
					len2;
		text		*targ1,
					*targ2;
		/* See comment in texteq() */
		len1 = toast_raw_datum_size(arg1);
		len2 = toast_raw_datum_size(arg2);
		targ1 = DatumGetTextPP(arg1);
		targ2 = DatumGetTextPP(arg2);
		
		len1 = bpchartruelen(VARDATA_ANY(targ1),VARSIZE_ANY_EXHDR(targ1));
		len2 = bpchartruelen(VARDATA_ANY(targ2),VARSIZE_ANY_EXHDR(targ2));
		if (len1 != len2)
			result = true;
		else
		{
			targ1 = DatumGetTextPP(arg1);
			targ2 = DatumGetTextPP(arg2);

			result = (memcmp(VARDATA_ANY(targ1), VARDATA_ANY(targ2),
							 len1) != 0);

			PG_FREE_IF_COPY(targ1, 0);
			PG_FREE_IF_COPY(targ2, 1);
		}
	}
	else
	{
		text	   *arg1 = PG_GETARG_TEXT_PP(0);
		text	   *arg2 = PG_GETARG_TEXT_PP(1);

		result = (text_cmp_plain(arg1, arg2, collid) != 0);

		PG_FREE_IF_COPY(arg1, 0);
		PG_FREE_IF_COPY(arg2, 1);
	}

	PG_RETURN_BOOL(result);
}

Datum
text_lt_explicit(PG_FUNCTION_ARGS)
{
	text	   *arg1 = PG_GETARG_TEXT_PP(0);
	text	   *arg2 = PG_GETARG_TEXT_PP(1);
	bool		result;

	result = (text_cmp_plain(arg1, arg2, PG_GET_COLLATION()) < 0);	

	PG_FREE_IF_COPY(arg1, 0);
	PG_FREE_IF_COPY(arg2, 1);

	PG_RETURN_BOOL(result);
}

Datum
text_le_explicit(PG_FUNCTION_ARGS)
{
	text	   *arg1 = PG_GETARG_TEXT_PP(0);
	text	   *arg2 = PG_GETARG_TEXT_PP(1);
	bool		result;

	result = (text_cmp_plain(arg1, arg2, PG_GET_COLLATION()) <= 0);	

	PG_FREE_IF_COPY(arg1, 0);
	PG_FREE_IF_COPY(arg2, 1);

	PG_RETURN_BOOL(result);
}

Datum
text_gt_explicit(PG_FUNCTION_ARGS)
{
	text	   *arg1 = PG_GETARG_TEXT_PP(0);
	text	   *arg2 = PG_GETARG_TEXT_PP(1);
	bool		result;

	result = (text_cmp_plain(arg1, arg2, PG_GET_COLLATION()) > 0);	

	PG_FREE_IF_COPY(arg1, 0);
	PG_FREE_IF_COPY(arg2, 1);

	PG_RETURN_BOOL(result);
}

Datum
text_ge_explicit(PG_FUNCTION_ARGS)
{
	text	   *arg1 = PG_GETARG_TEXT_PP(0);
	text	   *arg2 = PG_GETARG_TEXT_PP(1);
	bool		result;

	result = (text_cmp_plain(arg1, arg2, PG_GET_COLLATION()) >= 0);	

	PG_FREE_IF_COPY(arg1, 0);
	PG_FREE_IF_COPY(arg2, 1);

	PG_RETURN_BOOL(result);
}


/**********************************************************************
 *
 * Index support procedure 
 *
 *********************************************************************/

/* B-tree support procedure for type 'oravarcharbyte' and 'oravarcharchar' for explicit-named opclass */

/* B-tree compare precudure,like oravarcharchar_cmp but Not comparing spaces */
static int
oravarcharchar_cmp_explicit(VarChar *arg1, VarChar *arg2, Oid collid)
{
	char	   *a1p,
			   *a2p;
	int			len1,
				len2;

	a1p = VARDATA_ANY(arg1);
	a2p = VARDATA_ANY(arg2);

	len1 = bpchartruelen(VARDATA_ANY(arg1),VARSIZE_ANY_EXHDR(arg1));
	len2 = bpchartruelen(VARDATA_ANY(arg2),VARSIZE_ANY_EXHDR(arg2));

	return varstr_cmp(a1p, len1, a2p, len2, collid);
}

Datum
bt_oravarchar_cmp_explicit(PG_FUNCTION_ARGS)
{
	VarChar	   *arg1 = PG_GETARG_VARCHAR_PP(0);
	VarChar	   *arg2 = PG_GETARG_VARCHAR_PP(1);
	int32		result;

	result = oravarcharchar_cmp_explicit(arg1, arg2, PG_GET_COLLATION());

	PG_FREE_IF_COPY(arg1, 0);
	PG_FREE_IF_COPY(arg2, 1);

	PG_RETURN_INT32(result);
}

static int
text_cmp_explicit(text *arg1, text *arg2, Oid collid)
{
	char	   *a1p,
			   *a2p;
	int			len1,
				len2;

	a1p = VARDATA_ANY(arg1);
	a2p = VARDATA_ANY(arg2);

	len1 = bpchartruelen(VARDATA_ANY(arg1),VARSIZE_ANY_EXHDR(arg1));
	len2 = bpchartruelen(VARDATA_ANY(arg2),VARSIZE_ANY_EXHDR(arg2));

	return varstr_cmp(a1p, len1, a2p, len2, collid);
}

Datum
orabttextcmp_explicit(PG_FUNCTION_ARGS)
{
	text	   *arg1 = PG_GETARG_TEXT_PP(0);
	text	   *arg2 = PG_GETARG_TEXT_PP(1);
	int32		result;

	result = text_cmp_explicit(arg1, arg2, PG_GET_COLLATION());

	PG_FREE_IF_COPY(arg1, 0);
	PG_FREE_IF_COPY(arg2, 1);

	PG_RETURN_INT32(result);
}


/* B-tree sort support procedure,like btoravarcharcharsortsupport but Not comparing spaces */
static int
varstrfastcmp_c_explicit(Datum x, Datum y, SortSupport ssup)
{
	VarString  *arg1 = DatumGetVarStringPP(x);
	VarString  *arg2 = DatumGetVarStringPP(y);
	char	   *a1p,
			   *a2p;
	int			len1,
				len2,
				result;

	a1p = VARDATA_ANY(arg1);
	a2p = VARDATA_ANY(arg2);

	len1 = bpchartruelen(a1p,VARSIZE_ANY_EXHDR(arg1));
	len2 = bpchartruelen(a2p,VARSIZE_ANY_EXHDR(arg2));

	result = memcmp(a1p, a2p, Min(len1, len2));
	if ((result == 0) && (len1 != len2))
		result = (len1 < len2) ? -1 : 1;

	/* We can't afford to leak memory here. */
	if (PointerGetDatum(arg1) != x)
		pfree(arg1);
	if (PointerGetDatum(arg2) != y)
		pfree(arg2);

	return result;
}

/*
 * sortsupport comparison func for locale cases
 */
static int
varstrfastcmp_locale(char *a1p, int len1, char *a2p, int len2, SortSupport ssup)
{
	VarStringSortSupport *sss = (VarStringSortSupport *) ssup->ssup_extra;
	int			result;
	bool		arg1_match;

	/* Fast pre-check for equality, as discussed in varstr_cmp() */
	if (len1 == len2 && memcmp(a1p, a2p, len1) == 0)
	{
		/*
		 * No change in buf1 or buf2 contents, so avoid changing last_len1 or
		 * last_len2.  Existing contents of buffers might still be used by
		 * next call.
		 *
		 * It's fine to allow the comparison of BpChar padding bytes here,
		 * even though that implies that the memcmp() will usually be
		 * performed for BpChar callers (though multibyte characters could
		 * still prevent that from occurring).  The memcmp() is still very
		 * cheap, and BpChar's funny semantics have us remove trailing spaces
		 * (not limited to padding), so we need make no distinction between
		 * padding space characters and "real" space characters.
		 */
		return 0;
	}

	if (sss->typid == BPCHAROID ||
		(database_mode == DB_ORACLE && (sss->typid == ORACHARCHAROID || sss->typid == ORACHARBYTEOID)))	/* ReqID:SRS-GKC-DATATYPE_001 */
	{
		/* Get true number of bytes, ignoring trailing spaces */
		len1 = bpchartruelen(a1p, len1);
		len2 = bpchartruelen(a2p, len2);
	}

	if (len1 >= sss->buflen1)
	{
		sss->buflen1 = Max(len1 + 1, Min(sss->buflen1 * 2, MaxAllocSize));
		sss->buf1 = repalloc(sss->buf1, sss->buflen1);
	}
	if (len2 >= sss->buflen2)
	{
		sss->buflen2 = Max(len2 + 1, Min(sss->buflen2 * 2, MaxAllocSize));
		sss->buf2 = repalloc(sss->buf2, sss->buflen2);
	}

	/*
	 * We're likely to be asked to compare the same strings repeatedly, and
	 * memcmp() is so much cheaper than strcoll() that it pays to try to cache
	 * comparisons, even though in general there is no reason to think that
	 * that will work out (every string datum may be unique).  Caching does
	 * not slow things down measurably when it doesn't work out, and can speed
	 * things up by rather a lot when it does.  In part, this is because the
	 * memcmp() compares data from cachelines that are needed in L1 cache even
	 * when the last comparison's result cannot be reused.
	 */
	arg1_match = true;
	if (len1 != sss->last_len1 || memcmp(sss->buf1, a1p, len1) != 0)
	{
		arg1_match = false;
		memcpy(sss->buf1, a1p, len1);
		sss->buf1[len1] = '\0';
		sss->last_len1 = len1;
	}

	/*
	 * If we're comparing the same two strings as last time, we can return the
	 * same answer without calling strcoll() again.  This is more likely than
	 * it seems (at least with moderate to low cardinality sets), because
	 * quicksort compares the same pivot against many values.
	 */
	if (len2 != sss->last_len2 || memcmp(sss->buf2, a2p, len2) != 0)
	{
		memcpy(sss->buf2, a2p, len2);
		sss->buf2[len2] = '\0';
		sss->last_len2 = len2;
	}
	else if (arg1_match && !sss->cache_blob)
	{
		/* Use result cached following last actual strcoll() call */
		return sss->last_returned;
	}

	if (sss->locale)
	{
		if (sss->locale->provider == COLLPROVIDER_ICU)
		{
#ifdef USE_ICU
#ifdef HAVE_UCOL_STRCOLLUTF8
			if (GetDatabaseEncoding() == PG_UTF8)
			{
				UErrorCode	status;

				status = U_ZERO_ERROR;
				result = ucol_strcollUTF8(sss->locale->info.icu.ucol,
										  a1p, len1,
										  a2p, len2,
										  &status);
				if (U_FAILURE(status))
					ereport(ERROR,
							(errmsg("collation failed: %s", u_errorName(status))));
			}
			else
#endif
			{
				int32_t		ulen1,
							ulen2;
				UChar	   *uchar1,
						   *uchar2;

				ulen1 = icu_to_uchar(&uchar1, a1p, len1);
				ulen2 = icu_to_uchar(&uchar2, a2p, len2);

				result = ucol_strcoll(sss->locale->info.icu.ucol,
									  uchar1, ulen1,
									  uchar2, ulen2);

				pfree(uchar1);
				pfree(uchar2);
			}
#else							/* not USE_ICU */
			/* shouldn't happen */
			elog(ERROR, "unsupported collprovider: %c", sss->locale->provider);
#endif							/* not USE_ICU */
		}
		else
		{
#ifdef HAVE_LOCALE_T
			result = strcoll_l(sss->buf1, sss->buf2, sss->locale->info.lt);
#else
			/* shouldn't happen */
			elog(ERROR, "unsupported collprovider: %c", sss->locale->provider);
#endif
		}
	}
	else
		result = strcoll(sss->buf1, sss->buf2);

	/* Break tie if necessary. */
	if (result == 0 &&
		(!sss->locale || sss->locale->deterministic))
		result = strcmp(sss->buf1, sss->buf2);

	/* Cache result, perhaps saving an expensive strcoll() call next time */
	sss->cache_blob = false;
	sss->last_returned = result;
	return result;
}

static int
varlenafastcmp_locale_explicit(Datum x, Datum y, SortSupport ssup)
{
	VarString  *arg1 = DatumGetVarStringPP(x);
	VarString  *arg2 = DatumGetVarStringPP(y);
	char	   *a1p,
			   *a2p;
	int			len1,
				len2,
				result;

	a1p = VARDATA_ANY(arg1);
	a2p = VARDATA_ANY(arg2);
	

	len1 = bpchartruelen(a1p,VARSIZE_ANY_EXHDR(arg1));
	len2 = bpchartruelen(a2p,VARSIZE_ANY_EXHDR(arg2));

	result = varstrfastcmp_locale(a1p, len1, a2p, len2, ssup);

	/* We can't afford to leak memory here. */
	if (PointerGetDatum(arg1) != x)
		pfree(arg1);
	if (PointerGetDatum(arg2) != y)
		pfree(arg2);

	return result;
}

/*
 * Abbreviated key comparison func
 */
static int
varstrcmp_abbrev(Datum x, Datum y, SortSupport ssup)
{
	/*
	 * When 0 is returned, the core system will call varstrfastcmp_c()
	 * (bpcharfastcmp_c() in BpChar case) or varlenafastcmp_locale().  Even a
	 * strcmp() on two non-truncated strxfrm() blobs cannot indicate *equality*
	 * authoritatively, for the same reason that there is a strcoll()
	 * tie-breaker call to strcmp() in varstr_cmp().
	 */
	if (x > y)
		return 1;
	else if (x == y)
		return 0;
	else
		return -1;
}

static Datum
varstr_abbrev_convert_explicit(Datum original, SortSupport ssup)
{
	VarStringSortSupport *sss = (VarStringSortSupport *) ssup->ssup_extra;
	VarString  *authoritative = DatumGetVarStringPP(original);
	char	   *authoritative_data = VARDATA_ANY(authoritative);

	/* working state */
	Datum		res;
	char	   *pres;
	int			len;
	uint32		hash;

	pres = (char *) &res;
	/* memset(), so any non-overwritten bytes are NUL */
	memset(pres, 0, sizeof(Datum));
	len = VARSIZE_ANY_EXHDR(authoritative);
	
	/* Get number of bytes, ignoring trailing spaces */
	if ( sss->typid == ORAVARCHARCHAROID || sss->typid == ORAVARCHARBYTEOID || sss->typid == TEXTOID)	
		len = bpchartruelen(authoritative_data, len);
	else
		ereport(ERROR,errmsg("Other type %d should not choose this as sort support prodecure!",sss->typid));	/* This should not happen,but just check it. */

	/*
	 * If we're using the C collation, use memcpy(), rather than strxfrm(), to
	 * abbreviate keys.  The full comparator for the C locale is always
	 * memcmp().  It would be incorrect to allow bytea callers (callers that
	 * always force the C collation -- bytea isn't a collatable type, but this
	 * approach is convenient) to use strxfrm().  This is because bytea
	 * strings may contain NUL bytes.  Besides, this should be faster, too.
	 *
	 * More generally, it's okay that bytea callers can have NUL bytes in
	 * strings because varstrcmp_abbrev() need not make a distinction between
	 * terminating NUL bytes, and NUL bytes representing actual NULs in the
	 * authoritative representation.  Hopefully a comparison at or past one
	 * abbreviated key's terminating NUL byte will resolve the comparison
	 * without consulting the authoritative representation; specifically, some
	 * later non-NUL byte in the longer string can resolve the comparison
	 * against a subsequent terminating NUL in the shorter string.  There will
	 * usually be what is effectively a "length-wise" resolution there and
	 * then.
	 *
	 * If that doesn't work out -- if all bytes in the longer string
	 * positioned at or past the offset of the smaller string's (first)
	 * terminating NUL are actually representative of NUL bytes in the
	 * authoritative binary string (perhaps with some *terminating* NUL bytes
	 * towards the end of the longer string iff it happens to still be small)
	 * -- then an authoritative tie-breaker will happen, and do the right
	 * thing: explicitly consider string length.
	 */
	if (sss->collate_c)
		memcpy(pres, authoritative_data, Min(len, sizeof(Datum)));
	else
	{
		Size		bsize;
#ifdef USE_ICU
		int32_t		ulen = -1;
		UChar	   *uchar = NULL;
#endif

		/*
		 * We're not using the C collation, so fall back on strxfrm or ICU
		 * analogs.
		 */

		/* By convention, we use buffer 1 to store and NUL-terminate */
		if (len >= sss->buflen1)
		{
			sss->buflen1 = Max(len + 1, Min(sss->buflen1 * 2, MaxAllocSize));
			sss->buf1 = repalloc(sss->buf1, sss->buflen1);
		}

		/* Might be able to reuse strxfrm() blob from last call */
		if (sss->last_len1 == len && sss->cache_blob &&
			memcmp(sss->buf1, authoritative_data, len) == 0)
		{
			memcpy(pres, sss->buf2, Min(sizeof(Datum), sss->last_len2));
			/* No change affecting cardinality, so no hashing required */
			goto done;
		}

		memcpy(sss->buf1, authoritative_data, len);

		/*
		 * Just like strcoll(), strxfrm() expects a NUL-terminated string. Not
		 * necessary for ICU, but doesn't hurt.
		 */
		sss->buf1[len] = '\0';
		sss->last_len1 = len;

#ifdef USE_ICU
		/* When using ICU and not UTF8, convert string to UChar. */
		if (sss->locale && sss->locale->provider == COLLPROVIDER_ICU &&
			GetDatabaseEncoding() != PG_UTF8)
			ulen = icu_to_uchar(&uchar, sss->buf1, len);
#endif

		/*
		 * Loop: Call strxfrm() or ucol_getSortKey(), possibly enlarge buffer,
		 * and try again.  Both of these functions have the result buffer
		 * content undefined if the result did not fit, so we need to retry
		 * until everything fits, even though we only need the first few bytes
		 * in the end.  When using ucol_nextSortKeyPart(), however, we only
		 * ask for as many bytes as we actually need.
		 */
		for (;;)
		{
#ifdef USE_ICU
			if (sss->locale && sss->locale->provider == COLLPROVIDER_ICU)
			{
				/*
				 * When using UTF8, use the iteration interface so we only
				 * need to produce as many bytes as we actually need.
				 */
				if (GetDatabaseEncoding() == PG_UTF8)
				{
					UCharIterator iter;
					uint32_t	state[2];
					UErrorCode	status;

					uiter_setUTF8(&iter, sss->buf1, len);
					state[0] = state[1] = 0;	/* won't need that again */
					status = U_ZERO_ERROR;
					bsize = ucol_nextSortKeyPart(sss->locale->info.icu.ucol,
												 &iter,
												 state,
												 (uint8_t *) sss->buf2,
												 Min(sizeof(Datum), sss->buflen2),
												 &status);
					if (U_FAILURE(status))
						ereport(ERROR,
								(errmsg("sort key generation failed: %s",
										u_errorName(status))));
				}
				else
					bsize = ucol_getSortKey(sss->locale->info.icu.ucol,
											uchar, ulen,
											(uint8_t *) sss->buf2, sss->buflen2);
			}
			else
#endif
#ifdef HAVE_LOCALE_T
			if (sss->locale && sss->locale->provider == COLLPROVIDER_LIBC)
				bsize = strxfrm_l(sss->buf2, sss->buf1,
								  sss->buflen2, sss->locale->info.lt);
			else
#endif
				bsize = strxfrm(sss->buf2, sss->buf1, sss->buflen2);

			sss->last_len2 = bsize;
			if (bsize < sss->buflen2)
				break;

			/*
			 * Grow buffer and retry.
			 */
			sss->buflen2 = Max(bsize + 1,
							   Min(sss->buflen2 * 2, MaxAllocSize));
			sss->buf2 = repalloc(sss->buf2, sss->buflen2);
		}

		/*
		 * Every Datum byte is always compared.  This is safe because the
		 * strxfrm() blob is itself NUL terminated, leaving no danger of
		 * misinterpreting any NUL bytes not intended to be interpreted as
		 * logically representing termination.
		 *
		 * (Actually, even if there were NUL bytes in the blob it would be
		 * okay.  See remarks on bytea case above.)
		 */
		memcpy(pres, sss->buf2, Min(sizeof(Datum), bsize));

#ifdef USE_ICU
		if (uchar)
			pfree(uchar);
#endif
	}

	/*
	 * Maintain approximate cardinality of both abbreviated keys and original,
	 * authoritative keys using HyperLogLog.  Used as cheap insurance against
	 * the worst case, where we do many string transformations for no saving
	 * in full strcoll()-based comparisons.  These statistics are used by
	 * varstr_abbrev_abort().
	 *
	 * First, Hash key proper, or a significant fraction of it.  Mix in length
	 * in order to compensate for cases where differences are past
	 * PG_CACHE_LINE_SIZE bytes, so as to limit the overhead of hashing.
	 */
	hash = DatumGetUInt32(hash_any((unsigned char *) authoritative_data,
								   Min(len, PG_CACHE_LINE_SIZE)));

	if (len > PG_CACHE_LINE_SIZE)
		hash ^= DatumGetUInt32(hash_uint32((uint32) len));

	addHyperLogLog(&sss->full_card, hash);

	/* Hash abbreviated key */
#if SIZEOF_DATUM == 8
	{
		uint32		lohalf,
					hihalf;

		lohalf = (uint32) res;
		hihalf = (uint32) (res >> 32);
		hash = DatumGetUInt32(hash_uint32(lohalf ^ hihalf));
	}
#else							/* SIZEOF_DATUM != 8 */
	hash = DatumGetUInt32(hash_uint32((uint32) res));
#endif

	addHyperLogLog(&sss->abbr_card, hash);

	/* Cache result, perhaps saving an expensive strxfrm() call next time */
	sss->cache_blob = true;
done:

	/*
	 * Byteswap on little-endian machines.
	 *
	 * This is needed so that varstrcmp_abbrev() (an unsigned integer 3-way
	 * comparator) works correctly on all platforms.  If we didn't do this,
	 * the comparator would have to call memcmp() with a pair of pointers to
	 * the first byte of each abbreviated key, which is slower.
	 */
	res = DatumBigEndianToNative(res);

	/* Don't leak memory here */
	if (PointerGetDatum(authoritative) != original)
		pfree(authoritative);

	return res;
}

/*
 * Callback for estimating effectiveness of abbreviated key optimization, using
 * heuristic rules.  Returns value indicating if the abbreviation optimization
 * should be aborted, based on its projected effectiveness.
 */
static bool
varstr_abbrev_abort(int memtupcount, SortSupport ssup)
{
	VarStringSortSupport *sss = (VarStringSortSupport *) ssup->ssup_extra;
	double		abbrev_distinct,
				key_distinct;

	Assert(ssup->abbreviate);

	/* Have a little patience */
	if (memtupcount < 100)
		return false;

	abbrev_distinct = estimateHyperLogLog(&sss->abbr_card);
	key_distinct = estimateHyperLogLog(&sss->full_card);

	/*
	 * Clamp cardinality estimates to at least one distinct value.  While
	 * NULLs are generally disregarded, if only NULL values were seen so far,
	 * that might misrepresent costs if we failed to clamp.
	 */
	if (abbrev_distinct <= 1.0)
		abbrev_distinct = 1.0;

	if (key_distinct <= 1.0)
		key_distinct = 1.0;

	/*
	 * In the worst case all abbreviated keys are identical, while at the same
	 * time there are differences within full key strings not captured in
	 * abbreviations.
	 */
#ifdef TRACE_SORT
	if (trace_sort)
	{
		double		norm_abbrev_card = abbrev_distinct / (double) memtupcount;

		elog(LOG, "varstr_abbrev: abbrev_distinct after %d: %f "
			 "(key_distinct: %f, norm_abbrev_card: %f, prop_card: %f)",
			 memtupcount, abbrev_distinct, key_distinct, norm_abbrev_card,
			 sss->prop_card);
	}
#endif

	/*
	 * If the number of distinct abbreviated keys approximately matches the
	 * number of distinct authoritative original keys, that's reason enough to
	 * proceed.  We can win even with a very low cardinality set if most
	 * tie-breakers only memcmp().  This is by far the most important
	 * consideration.
	 *
	 * While comparisons that are resolved at the abbreviated key level are
	 * considerably cheaper than tie-breakers resolved with memcmp(), both of
	 * those two outcomes are so much cheaper than a full strcoll() once
	 * sorting is underway that it doesn't seem worth it to weigh abbreviated
	 * cardinality against the overall size of the set in order to more
	 * accurately model costs.  Assume that an abbreviated comparison, and an
	 * abbreviated comparison with a cheap memcmp()-based authoritative
	 * resolution are equivalent.
	 */
	if (abbrev_distinct > key_distinct * sss->prop_card)
	{
		/*
		 * When we have exceeded 10,000 tuples, decay required cardinality
		 * aggressively for next call.
		 *
		 * This is useful because the number of comparisons required on
		 * average increases at a linearithmic rate, and at roughly 10,000
		 * tuples that factor will start to dominate over the linear costs of
		 * string transformation (this is a conservative estimate).  The decay
		 * rate is chosen to be a little less aggressive than halving -- which
		 * (since we're called at points at which memtupcount has doubled)
		 * would never see the cost model actually abort past the first call
		 * following a decay.  This decay rate is mostly a precaution against
		 * a sudden, violent swing in how well abbreviated cardinality tracks
		 * full key cardinality.  The decay also serves to prevent a marginal
		 * case from being aborted too late, when too much has already been
		 * invested in string transformation.
		 *
		 * It's possible for sets of several million distinct strings with
		 * mere tens of thousands of distinct abbreviated keys to still
		 * benefit very significantly.  This will generally occur provided
		 * each abbreviated key is a proxy for a roughly uniform number of the
		 * set's full keys. If it isn't so, we hope to catch that early and
		 * abort.  If it isn't caught early, by the time the problem is
		 * apparent it's probably not worth aborting.
		 */
		if (memtupcount > 10000)
			sss->prop_card *= 0.65;

		return false;
	}

	/*
	 * Abort abbreviation strategy.
	 *
	 * The worst case, where all abbreviated keys are identical while all
	 * original strings differ will typically only see a regression of about
	 * 10% in execution time for small to medium sized lists of strings.
	 * Whereas on modern CPUs where cache stalls are the dominant cost, we can
	 * often expect very large improvements, particularly with sets of strings
	 * of moderately high to high abbreviated cardinality.  There is little to
	 * lose but much to gain, which our strategy reflects.
	 */
#ifdef TRACE_SORT
	if (trace_sort)
		elog(LOG, "varstr_abbrev: aborted abbreviation at %d "
			 "(abbrev_distinct: %f, key_distinct: %f, prop_card: %f)",
			 memtupcount, abbrev_distinct, key_distinct, sss->prop_card);
#endif

	return true;
}


/*
 * Generic sortsupport interface for character type's operator classes.
 * Includes locale support, and support for BpChar semantics (i.e. removing
 * trailing spaces before comparison).
 *
 * Relies on the assumption that text, VarChar, BpChar, and bytea all have the
 * same representation.  Callers that always use the C collation (e.g.
 * non-collatable type callers like bytea) may have NUL bytes in their strings;
 * this will not work with any other collation, though.
 * this is like varstr_sortsupport,
 * but ignore space when comapre if typeid is oravarcharchar,oravarcharbyte,text
 * for compatible DM space_compare_mode.
 */
static void
varstr_sortsupport_explicit(SortSupport ssup, Oid typid, Oid collid)
{
	bool		abbreviate = ssup->abbreviate;
	bool		collate_c = false;
	VarStringSortSupport *sss;
	pg_locale_t locale = 0;

	check_collation_set(collid);

	/*
	 * If possible, set ssup->comparator to a function which can be used to
	 * directly compare two datums.  If we can do this, we'll avoid the
	 * overhead of a trip through the fmgr layer for every comparison, which
	 * can be substantial.
	 *
	 * Most typically, we'll set the comparator to varlenafastcmp_locale,
	 * which uses strcoll() to perform comparisons.  We use that for the
	 * BpChar case too, but type NAME uses namefastcmp_locale. However, if
	 * LC_COLLATE = C, we can make things quite a bit faster with
	 * varstrfastcmp_c, bpcharfastcmp_c, or namefastcmp_c, all of which use
	 * memcmp() rather than strcoll().
	 */
	if (typid != ORAVARCHARCHAROID && typid != ORAVARCHARBYTEOID && typid != TEXTOID)
		ereport(ERROR,errmsg("Other type %d should not choose this as sort support prodecure!",typid));	/* This should not happen,but just check it. */

	if (lc_collate_is_c(collid))
	{
		ssup->comparator = varstrfastcmp_c_explicit;
		collate_c = true;
	}
	else
	{
		/*
		 * We need a collation-sensitive comparison.  To make things faster,
		 * we'll figure out the collation based on the locale id and cache the
		 * result.
		 */
		if (collid != DEFAULT_COLLATION_OID)
			locale = pg_newlocale_from_collation(collid);

		/*
		 * There is a further exception on Windows.  When the database
		 * encoding is UTF-8 and we are not using the C collation, complex
		 * hacks are required.  We don't currently have a comparator that
		 * handles that case, so we fall back on the slow method of having the
		 * sort code invoke bttextcmp() (in the case of text) via the fmgr
		 * trampoline.  ICU locales work just the same on Windows, however.
		 */
#ifdef WIN32
		if (GetDatabaseEncoding() == PG_UTF8 &&
			!(locale && locale->provider == COLLPROVIDER_ICU))
			return;
#endif

		ssup->comparator = varlenafastcmp_locale_explicit;
	}

	/*
	 * Unfortunately, it seems that abbreviation for non-C collations is
	 * broken on many common platforms; testing of multiple versions of glibc
	 * reveals that, for many locales, strcoll() and strxfrm() do not return
	 * consistent results, which is fatal to this optimization.  While no
	 * other libc other than Cygwin has so far been shown to have a problem,
	 * we take the conservative course of action for right now and disable
	 * this categorically.  (Users who are certain this isn't a problem on
	 * their system can define TRUST_STRXFRM.)
	 *
	 * Even apart from the risk of broken locales, it's possible that there
	 * are platforms where the use of abbreviated keys should be disabled at
	 * compile time.  Having only 4 byte datums could make worst-case
	 * performance drastically more likely, for example.  Moreover, macOS's
	 * strxfrm() implementation is known to not effectively concentrate a
	 * significant amount of entropy from the original string in earlier
	 * transformed blobs.  It's possible that other supported platforms are
	 * similarly encumbered.  So, if we ever get past disabling this
	 * categorically, we may still want or need to disable it for particular
	 * platforms.
	 */
#ifndef TRUST_STRXFRM
	if (!collate_c && !(locale && locale->provider == COLLPROVIDER_ICU))
		abbreviate = false;
#endif

	/*
	 * If we're using abbreviated keys, or if we're using a locale-aware
	 * comparison, we need to initialize a VarStringSortSupport object. Both
	 * cases will make use of the temporary buffers we initialize here for
	 * scratch space (and to detect requirement for BpChar semantics from
	 * caller), and the abbreviation case requires additional state.
	 */
	if (abbreviate || !collate_c)
	{
		sss = palloc(sizeof(VarStringSortSupport));
		sss->buf1 = palloc(TEXTBUFLEN);
		sss->buflen1 = TEXTBUFLEN;
		sss->buf2 = palloc(TEXTBUFLEN);
		sss->buflen2 = TEXTBUFLEN;
		/* Start with invalid values */
		sss->last_len1 = -1;
		sss->last_len2 = -1;
		/* Initialize */
		sss->last_returned = 0;
		sss->locale = locale;

		/*
		 * To avoid somehow confusing a strxfrm() blob and an original string,
		 * constantly keep track of the variety of data that buf1 and buf2
		 * currently contain.
		 *
		 * Comparisons may be interleaved with conversion calls.  Frequently,
		 * conversions and comparisons are batched into two distinct phases,
		 * but the correctness of caching cannot hinge upon this.  For
		 * comparison caching, buffer state is only trusted if cache_blob is
		 * found set to false, whereas strxfrm() caching only trusts the state
		 * when cache_blob is found set to true.
		 *
		 * Arbitrarily initialize cache_blob to true.
		 */
		sss->cache_blob = true;
		sss->collate_c = collate_c;
		sss->typid = typid;
		ssup->ssup_extra = sss;

		/*
		 * If possible, plan to use the abbreviated keys optimization.  The
		 * core code may switch back to authoritative comparator should
		 * abbreviation be aborted.
		 */
		if (abbreviate)
		{
			sss->prop_card = 0.20;
			initHyperLogLog(&sss->abbr_card, 10);
			initHyperLogLog(&sss->full_card, 10);
			ssup->abbrev_full_comparator = ssup->comparator;
			ssup->comparator = varstrcmp_abbrev;
			ssup->abbrev_converter = varstr_abbrev_convert_explicit;
			ssup->abbrev_abort = varstr_abbrev_abort;
		}
	}
}

Datum
btoravarcharcharsortsupport_explicit(PG_FUNCTION_ARGS)
{
	SortSupport ssup = (SortSupport) PG_GETARG_POINTER(0);
	Oid			collid = ssup->ssup_collation;
	MemoryContext oldcontext;

	oldcontext = MemoryContextSwitchTo(ssup->ssup_cxt);

	/* Use generic string SortSupport */
	varstr_sortsupport_explicit(ssup, ORAVARCHARCHAROID, collid);

	MemoryContextSwitchTo(oldcontext);

	PG_RETURN_VOID();
}

Datum
btoravarcharbytesortsupport_explicit(PG_FUNCTION_ARGS)
{
	SortSupport ssup = (SortSupport) PG_GETARG_POINTER(0);
	Oid			collid = ssup->ssup_collation;
	MemoryContext oldcontext;

	oldcontext = MemoryContextSwitchTo(ssup->ssup_cxt);

	/* Use generic string SortSupport */
	varstr_sortsupport_explicit(ssup, ORAVARCHARBYTEOID, collid);

	MemoryContextSwitchTo(oldcontext);

	PG_RETURN_VOID();
}

Datum
btoratextsortsupport_explicit(PG_FUNCTION_ARGS)
{
	SortSupport ssup = (SortSupport) PG_GETARG_POINTER(0);
	Oid			collid = ssup->ssup_collation;
	MemoryContext oldcontext;

	oldcontext = MemoryContextSwitchTo(ssup->ssup_cxt);

	/* Use generic string SortSupport */
	varstr_sortsupport_explicit(ssup, TEXTOID, collid);

	MemoryContextSwitchTo(oldcontext);

	PG_RETURN_VOID();
}












/* Hash support procedure for type oracle text for explicit compare operator #=,#>,etc.*/
static int
text_Truelen(text *arg)
{
	char	   *s = VARDATA_ANY(arg);
	int			i;
	int			len;

	len = VARSIZE_ANY_EXHDR(arg);
	for (i = len - 1; i >= 0; i--)
	{
		if (s[i] != ' ')
			break;
	}
	return i + 1;
}

/* Like hashtext,but without blank space */
Datum
hashoratext(PG_FUNCTION_ARGS)
{
	text	   *key = PG_GETARG_TEXT_PP(0);
	Oid			collid = PG_GET_COLLATION();
	pg_locale_t mylocale = 0;
	Datum		result;
	int			keylen = text_Truelen(key);

	if (!collid)
		ereport(ERROR,
				(errcode(ERRCODE_INDETERMINATE_COLLATION),
				 errmsg("could not determine which collation to use for string hashing"),
				 errhint("Use the COLLATE clause to set the collation explicitly.")));

	if (!lc_collate_is_c(collid) && collid != DEFAULT_COLLATION_OID)
		mylocale = pg_newlocale_from_collation(collid);

	if (!mylocale || mylocale->deterministic)
	{
		result = hash_any((unsigned char *) VARDATA_ANY(key),
						  keylen);
	}
	else
	{
#ifdef USE_ICU
		if (mylocale->provider == COLLPROVIDER_ICU)
		{
			int32_t		ulen = -1;
			UChar	   *uchar = NULL;
			Size		bsize;
			uint8_t    *buf;

			ulen = icu_to_uchar(&uchar, VARDATA_ANY(key), keylen);

			bsize = ucol_getSortKey(mylocale->info.icu.ucol,
									uchar, ulen, NULL, 0);
			buf = palloc(bsize);
			ucol_getSortKey(mylocale->info.icu.ucol,
							uchar, ulen, buf, bsize);
			pfree(uchar);

			result = hash_any(buf, bsize);

			pfree(buf);
		}
		else
#endif
			/* shouldn't happen */
			elog(ERROR, "unsupported collprovider: %c", mylocale->provider);
	}

	/* Avoid leaking memory for toasted inputs */
	PG_FREE_IF_COPY(key, 0);

	return result;
}
