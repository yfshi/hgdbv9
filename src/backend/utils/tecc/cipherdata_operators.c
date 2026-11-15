/*-------------------------------------------------------------------------
 *
 * cipherdata_operators.c
 *	  Operator functions for encrypted data types
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/utils/tecc/cipherdata_operators.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include <string.h>

#include "catalog/pg_type.h"
#include "catalog/pg_type_enc.h"
#include "utils/builtins.h"
#include "utils/cipherdatatype.h"
#include "utils/tecc/tecc_typeicu.h"
#include "common/hashfn.h"  /* for hash_any() */
#include "fmgr.h"           /* for DirectFunctionCall2Coll */

/*
 * Varlena-header-independent field accessors for CipherDataType.
 *
 * Stored cipherdata may use a 1-byte or 4-byte varlena header.  The
 * CipherDataType struct assumes a 4-byte header, so direct struct member
 * access is wrong for 1-byte-header data.  These helpers use VARDATA_ANY
 * to skip whichever header is present, then access fields at known offsets.
 *
 * Layout after varlena header:
 *   [version:4] [dek_dist_oid:4] [rnd_len:4] [det_len:4] [ope_len:4] [search_len:4] [ct...]
 *    +0          +4              +8         +12        +16         +20             +24
 *
 * Use the unified CIPHERDATA_DATUM_* macros from cipherdatatype.h.
 */

/*
 * cipherdata_check_dek_consistency
 *		Reject comparing cipherdata values encrypted with different DEKs.
 *
 *		Comparing ciphertexts produced under different DEKs is meaningless
 *		(the ciphertexts are incomparable) and would silently return wrong
 *		results -- e.g. an equality JOIN on columns with different DEKs
 *		returns no rows instead of an error.  We therefore reject any such
 *		comparison explicitly with ERRCODE_TECC_DEK_MISMATCH.
 *
 *		Float sentinels (RANGE partition boundaries, see
 *		CIPHERDATA_IS_FLOAT_SENTINEL) are synthetic values with no real
 *		DEK identity, so they are skipped.
 */
static void
cipherdata_check_dek_consistency(const void *left, const void *right)
{
	/* Float sentinels are synthetic partition boundaries, no DEK to compare */
	if (CIPHERDATA_IS_FLOAT_SENTINEL(left) || CIPHERDATA_IS_FLOAT_SENTINEL(right))
		return;

	if (CIPHERDATA_DEK_DIST_OID(left) != CIPHERDATA_DEK_DIST_OID(right))
		ereport(ERROR,
				(errcode(ERRCODE_TECC_DEK_MISMATCH),
				 errmsg("cannot compare encrypted values with different DEKs")));
}

/*
 * Helper function to compare two CipherDataType structures.
 * Uses const void * and CD_* macros for varlena-header independence.
 *
 * When OPE ciphertext is available, comparison is based on OPE
 * (order-preserving).  Otherwise, falls back to DET ciphertext
 * comparison for equality and consistent byte ordering.
 * DET byte ordering does not preserve plaintext order, but is
 * sufficient for binary search-based lookups (e.g., LIST partition
 * routing) and btree indexes used with equality predicates.
 *
 * Returns: -1 if left < right, 0 if equal, 1 if left > right
 */
static int
cipherdata_cmp_internal(const void *left, const void *right)
{
	bool		left_sentinel;
	bool		right_sentinel;
	int			result;

	/*
	 * ReqID:TECC4.0 - Float sentinel comparison (Infinity/-Infinity).
	 *
	 * When a float4_enc/float8_enc column uses Infinity/-Infinity as a
	 * RANGE partition boundary, the value is stored as a sentinel
	 * CipherDataType whose OPE section contains raw IEEE 754 float bytes.
	 *
	 * Ordering guarantee: -Infinity < any real encrypted value < +Infinity.
	 * This matches PostgreSQL's native float4/float8 partition semantics.
	 */
	left_sentinel  = CIPHERDATA_IS_FLOAT_SENTINEL(left);
	right_sentinel = CIPHERDATA_IS_FLOAT_SENTINEL(right);

	if (left_sentinel || right_sentinel)
	{
		if (left_sentinel && right_sentinel)
		{
			/* Both sentinels — compare IEEE 754 float values directly.
			 * Promote float4 to float8 for comparison (Infinity is exact). */
			double	lv, rv;

			if (CIPHERDATA_DATUM_OPE_LEN(left) == sizeof(float))
				lv = (double) (*(const float *)CIPHERDATA_DATUM_OPE_PTR(left));
			else
				lv = *(const double *)CIPHERDATA_DATUM_OPE_PTR(left);

			if (CIPHERDATA_DATUM_OPE_LEN(right) == sizeof(float))
				rv = (double) (*(const float *)CIPHERDATA_DATUM_OPE_PTR(right));
			else
				rv = *(const double *)CIPHERDATA_DATUM_OPE_PTR(right);

			if (lv < rv) return -1;
			if (lv > rv) return 1;
			return 0;
		}

		/*
		 * One sentinel, one regular encrypted value.
		 * -Infinity sentinel (sign bit set) < any real value.
		 * +Infinity sentinel (sign bit clear) > any real value.
		 */
		if (left_sentinel)
			return CIPHERDATA_SENTINEL_IS_NEGATIVE(left) ? -1 : 1;
		else
			return CIPHERDATA_SENTINEL_IS_NEGATIVE(right) ? 1 : -1;
	}

	/* ReqID:TECC4.0 - Both operands must be encrypted with the same DEK,
	 * otherwise the comparison is meaningless (silently wrong results). */
	cipherdata_check_dek_consistency(left, right);

	/* Compare OPE ciphertexts for ordering.
	 * OPE is the only valid method for ordering encrypted values. */
	if (CIPHERDATA_DATUM_OPE_LEN(left) > 0 && CIPHERDATA_DATUM_OPE_LEN(right) > 0)
	{
		Size		min_len = Min(CIPHERDATA_DATUM_OPE_LEN(left), CIPHERDATA_DATUM_OPE_LEN(right));
		result = memcmp(CIPHERDATA_DATUM_OPE_PTR(left), CIPHERDATA_DATUM_OPE_PTR(right), min_len);

		if (result != 0)
			return (result < 0) ? -1 : 1;

		if (CIPHERDATA_DATUM_OPE_LEN(left) < CIPHERDATA_DATUM_OPE_LEN(right)) return -1;
		if (CIPHERDATA_DATUM_OPE_LEN(left) > CIPHERDATA_DATUM_OPE_LEN(right)) return 1;
		return 0;
	}

	/* Neither OPE available for ordering.
	 * Distinguish: DET-only (has equality but not ordering) vs
	 * no comparable segment at all. */
	if (CIPHERDATA_DATUM_DET_LEN(left) > 0 && CIPHERDATA_DATUM_DET_LEN(right) > 0)
		ereport(ERROR,
				(errcode(ERRCODE_TECC_NO_OPE_CIPHERTEXT),
				 errmsg("cannot compare encrypted values for ordering: "
						"column has DET but not OPE encryption"),
				 errhint("DET only supports equality comparison. "
						 "Add OPE encryption to the column to support ordering.")));
	ereport(ERROR,
			(errcode(ERRCODE_TECC_NO_OPE_CIPHERTEXT),
			 errmsg("cannot compare encrypted values for ordering: "
					"no comparable ciphertext available"),
			 errhint("Enable DET or OPE encryption on the column to support comparison.")));

	return 0;	/* unreachable */
}

/*
 * ReqID:TECC4.0 - Strip text trailing-space format from DET/OPE section.
 * Format: always [format_byte:1][encrypted_core][space_count:4], strip removes all 5 bytes
 * Returns core pointer and length (without format_byte and space_count).
 */
static inline void
cipherdata_text_strip_format(const unsigned char *det_ptr, int32 det_len,
                              const unsigned char **core_ptr, int32 *core_len)
{
	if (det_len >= 5)
	{
		*core_ptr = det_ptr + 1;
		*core_len = det_len - 5;
	}
	else
	{
		*core_ptr = det_ptr;
		*core_len = det_len;
	}
}

/* ReqID:TECC4.0 - Same as cipherdata_text_strip_format but also returns
 * space_count from the last 4 bytes (network byte order). */
static inline void
cipherdata_text_strip_format_with_space(const unsigned char *ptr, int32 len,
                                         const unsigned char **core_ptr,
                                         int32 *core_len, uint32 *space_count)
{
	if (len >= 5)
	{
		*core_ptr = ptr + 1;
		*core_len = len - 5;
		*space_count = ((uint32)ptr[len - 4] << 24) |
		               ((uint32)ptr[len - 3] << 16) |
		               ((uint32)ptr[len - 2] << 8)  |
		               ((uint32)ptr[len - 1]);
	}
	else
	{
		*core_ptr = ptr;
		*core_len = len;
		*space_count = 0;
	}
}

/*
 * ReqID:TECC4.0 - BPCHAR-aware equality comparison
 *
 * BPCHAR semantic rules:
 *   - BPCHAR ignores trailing spaces in comparisons
 *   - 'abc'::bpchar(10) = 'abc   '::bpchar(10)   → t  (A2)
 *   - '   '::bpchar(10) = ''::bpchar(10)          → t  (D8)
 *   - '   '::bpchar(10) = '   '::varchar(10)      → t  (D1)
 *
 * PG operator resolution rules for cross-type comparisons:
 *   - text_enc has typispreferred='t' in 'U' category
 *   - bpchar_enc = text_enc: text_enc is preferred → converts bpchar_enc to
 *     text_enc and uses text_enc_eq (text semantics, keeps trailing spaces)
 *   - bpchar_enc = varchar_enc: via explicit pg_operator
 *     =(bpchar_enc,varchar_enc)→bpchar_enc_eq (bpchar semantics)
 *   - varchar_enc = bpchar_enc: same → bpchar_enc_eq
 *   - text_enc = varchar_enc: via =(text_enc,varchar_enc)→text_enc_eq
 *   - varchar_enc = text_enc: via =(varchar_enc,text_enc)→varchar_enc_eq
 *
 * Implementation: strip both format_byte and space_count from DET/OPE
 * ciphertext, compare only the encrypted core. This makes bpchar_enc
 * ignore space_count, matching PG BPCHAR semantics.
 */
static Datum
cipherdata_eq_bpchar(PG_FUNCTION_ARGS)
{
	Datum ld = PG_GETARG_DATUM(0), rd = PG_GETARG_DATUM(1);
	const void *l = PG_DETOAST_DATUM(ld), *r = PG_DETOAST_DATUM(rd);
	/* ReqID:TECC4.0 - Both operands must use the same DEK */
	cipherdata_check_dek_consistency(l, r);
	if (CIPHERDATA_DATUM_DET_LEN(l) > 0 && CIPHERDATA_DATUM_DET_LEN(r) > 0) {
		const unsigned char *lc, *rc; int32 ll, rl;
		cipherdata_text_strip_format(CIPHERDATA_DATUM_DET_PTR(l), CIPHERDATA_DATUM_DET_LEN(l), &lc, &ll);
		cipherdata_text_strip_format(CIPHERDATA_DATUM_DET_PTR(r), CIPHERDATA_DATUM_DET_LEN(r), &rc, &rl);
		if (ll != rl) PG_RETURN_BOOL(false);
		PG_RETURN_BOOL(memcmp(lc, rc, ll) == 0);
	}
	if (CIPHERDATA_DATUM_OPE_LEN(l) > 0 && CIPHERDATA_DATUM_OPE_LEN(r) > 0) {
		const unsigned char *lc, *rc; int32 ll, rl; int cmp;
		cipherdata_text_strip_format(CIPHERDATA_DATUM_OPE_PTR(l), CIPHERDATA_DATUM_OPE_LEN(l), &lc, &ll);
		cipherdata_text_strip_format(CIPHERDATA_DATUM_OPE_PTR(r), CIPHERDATA_DATUM_OPE_LEN(r), &rc, &rl);
		cmp = memcmp(lc, rc, Min(ll, rl));
		if (cmp != 0) PG_RETURN_BOOL(false);
		PG_RETURN_BOOL(ll == rl);
	}
	ereport(ERROR,(errcode(ERRCODE_TECC_NO_DET_CIPHERTEXT),
		errmsg("neither DET nor OPE available for BPCHAR comparison")));
	PG_RETURN_BOOL(false);
}

/*
 * ReqID:TECC4.0 - text_enc / varchar_enc equality (trailing spaces significant)
 *
 * Shared implementation for the two text-family types that treat trailing
 * spaces as part of the value:
 *   - 'abc'::varchar(10) = 'abc   '::varchar(10) -> f (A6)
 *   - '   '::varchar(10) = ''::varchar(10)       -> f
 *   - '   '::varchar(10) = '   '::varchar(10)    -> t (D5)
 *
 * The encrypted core is compared first and the space count afterwards, so
 * values differing only in trailing blanks are not equal.  bpchar_enc has its
 * own variant (cipherdata_eq_bpchar) that ignores trailing spaces.
 */
static Datum
cipherdata_eq_text(PG_FUNCTION_ARGS)
{
	Datum		ld = PG_GETARG_DATUM(0);
	Datum		rd = PG_GETARG_DATUM(1);
	const void *l = PG_DETOAST_DATUM(ld);
	const void *r = PG_DETOAST_DATUM(rd);

	/* ReqID:TECC4.0 - Both operands must use the same DEK */
	cipherdata_check_dek_consistency(l, r);

	if (CIPHERDATA_DATUM_DET_LEN(l) > 0 && CIPHERDATA_DATUM_DET_LEN(r) > 0)
	{
		const unsigned char *lc;
		const unsigned char *rc;
		int32		ll;
		int32		rl;
		uint32		lspc;
		uint32		rspc;

		cipherdata_text_strip_format_with_space(
			CIPHERDATA_DATUM_DET_PTR(l), CIPHERDATA_DATUM_DET_LEN(l), &lc, &ll, &lspc);
		cipherdata_text_strip_format_with_space(
			CIPHERDATA_DATUM_DET_PTR(r), CIPHERDATA_DATUM_DET_LEN(r), &rc, &rl, &rspc);
		if (ll != rl || memcmp(lc, rc, ll) != 0)
			PG_RETURN_BOOL(false);
		PG_RETURN_BOOL(lspc == rspc);
	}

	if (CIPHERDATA_DATUM_OPE_LEN(l) > 0 && CIPHERDATA_DATUM_OPE_LEN(r) > 0)
	{
		const unsigned char *lc;
		const unsigned char *rc;
		int32		ll;
		int32		rl;
		uint32		lspc;
		uint32		rspc;
		int			cmp;

		cipherdata_text_strip_format_with_space(
			CIPHERDATA_DATUM_OPE_PTR(l), CIPHERDATA_DATUM_OPE_LEN(l), &lc, &ll, &lspc);
		cipherdata_text_strip_format_with_space(
			CIPHERDATA_DATUM_OPE_PTR(r), CIPHERDATA_DATUM_OPE_LEN(r), &rc, &rl, &rspc);
		cmp = memcmp(lc, rc, Min(ll, rl));
		if (cmp != 0 || ll != rl)
			PG_RETURN_BOOL(false);
		PG_RETURN_BOOL(lspc == rspc);
	}

	ereport(ERROR,
			(errcode(ERRCODE_TECC_NO_DET_CIPHERTEXT),
			 errmsg("neither DET nor OPE available for equality comparison")));
	PG_RETURN_BOOL(false);
}

/*
 * ReqID:TECC4.0 - text_enc / varchar_enc ordering (trailing spaces significant)
 *
 * Shared implementation for the two text-family types whose ordering treats
 * trailing spaces as part of the value:
 *   - 'abc'::varchar(10) < 'abc   '::varchar(10) -> t (spaces make larger)
 *   - ''::varchar(10) < '   '::varchar(10)       -> t
 *
 * The format byte and the space count are stripped before comparing: the
 * encrypted core decides first, and only when the cores tie does the space
 * count break it (more spaces = larger).  Comparing the raw OPE section
 * instead (cipherdata_cmp_internal) would fold the format byte into the
 * ordering and give wrong results.  bpchar_enc has its own variant
 * (cipherdata_cmp_bpchar) that ignores trailing spaces.
 */
static Datum
cipherdata_cmp_text(PG_FUNCTION_ARGS)
{
	Datum		ld = PG_GETARG_DATUM(0);
	Datum		rd = PG_GETARG_DATUM(1);
	const void *l = PG_DETOAST_DATUM(ld);
	const void *r = PG_DETOAST_DATUM(rd);
	int			result;

	/* ReqID:TECC4.0 - Both operands must use the same DEK */
	cipherdata_check_dek_consistency(l, r);

	if (CIPHERDATA_DATUM_OPE_LEN(l) > 0 && CIPHERDATA_DATUM_OPE_LEN(r) > 0)
	{
		const unsigned char *lc;
		const unsigned char *rc;
		int32		ll;
		int32		rl;
		uint32		lspc;
		uint32		rspc;
		Size		min_len;

		cipherdata_text_strip_format_with_space(
			CIPHERDATA_DATUM_OPE_PTR(l), CIPHERDATA_DATUM_OPE_LEN(l), &lc, &ll, &lspc);
		cipherdata_text_strip_format_with_space(
			CIPHERDATA_DATUM_OPE_PTR(r), CIPHERDATA_DATUM_OPE_LEN(r), &rc, &rl, &rspc);
		min_len = Min(ll, rl);
		result = memcmp(lc, rc, min_len);
		if (result != 0)
			PG_RETURN_INT32((result < 0) ? -1 : 1);
		if (ll != rl)
			PG_RETURN_INT32((ll < rl) ? -1 : 1);
		/* Core equal: more trailing spaces means larger. */
		if (lspc != rspc)
			PG_RETURN_INT32((lspc < rspc) ? -1 : 1);
		PG_RETURN_INT32(0);
	}

	ereport(ERROR,
			(errcode(ERRCODE_TECC_NO_OPE_CIPHERTEXT),
			 errmsg("cannot compare encrypted values for ordering: no OPE ciphertext available")));
	PG_RETURN_INT32(0);
}

/*
 * ReqID:TECC4.0 - bpchar_enc ordering (trailing spaces ignored)
 *
 * BPCHAR ordering rule: trailing spaces do not affect ordering.
 *   - 'abc'::bpchar(10) < 'abc   '::bpchar(10) -> f (equal, spaces ignored)
 *   - ''::bpchar(10) < '   '::bpchar(10)       -> f (equal, both empty)
 *
 * Only the encrypted core (format byte and space count stripped) is compared.
 */
static Datum
cipherdata_cmp_bpchar(PG_FUNCTION_ARGS)
{
	Datum		ld = PG_GETARG_DATUM(0);
	Datum		rd = PG_GETARG_DATUM(1);
	const void *l = PG_DETOAST_DATUM(ld);
	const void *r = PG_DETOAST_DATUM(rd);
	int			result;

	/* ReqID:TECC4.0 - Both operands must use the same DEK */
	cipherdata_check_dek_consistency(l, r);

	if (CIPHERDATA_DATUM_OPE_LEN(l) > 0 && CIPHERDATA_DATUM_OPE_LEN(r) > 0)
	{
		const unsigned char *lc;
		const unsigned char *rc;
		int32		ll;
		int32		rl;
		Size		min_len;

		cipherdata_text_strip_format(
			CIPHERDATA_DATUM_OPE_PTR(l), CIPHERDATA_DATUM_OPE_LEN(l), &lc, &ll);
		cipherdata_text_strip_format(
			CIPHERDATA_DATUM_OPE_PTR(r), CIPHERDATA_DATUM_OPE_LEN(r), &rc, &rl);
		min_len = Min(ll, rl);
		result = memcmp(lc, rc, min_len);
		if (result != 0)
			PG_RETURN_INT32((result < 0) ? -1 : 1);
		if (ll != rl)
			PG_RETURN_INT32((ll < rl) ? -1 : 1);
		/* Core equal: bpchar ignores trailing spaces. */
		PG_RETURN_INT32(0);
	}

	ereport(ERROR,
			(errcode(ERRCODE_TECC_NO_OPE_CIPHERTEXT),
			 errmsg("cannot compare encrypted values for ordering: no OPE ciphertext available")));
	PG_RETURN_INT32(0);
}

/*
 * Generic equality comparison function
 *
 * For encrypted types, equality is determined by comparing DET or OPE
 * ciphertexts.  DET and OPE are mutually exclusive per column — both
 * are deterministic, so either one suffices for equality comparison.
 * Prefer DET when available (it leaks less information than OPE).
 */
static Datum
cipherdata_eq(PG_FUNCTION_ARGS)
{
	/* ReqID:TECC4.0 - Detoast to ensure 4-byte varlena header.
	 * PG_GETARG_POINTER may return a 1-byte short-header varlena
	 * for data < 126 bytes, shifting all CIPHERDATA_DATUM_* macro
	 * field offsets by 3 bytes.  PG_DETOAST_DATUM always returns
	 * a 4-byte header, fixing the offset assumptions. */
	Datum		left_datum = PG_GETARG_DATUM(0);
	Datum		right_datum = PG_GETARG_DATUM(1);
	const void *left = PG_DETOAST_DATUM(left_datum);
	const void *right = PG_DETOAST_DATUM(right_datum);
	Size		min_len;
	int			cmp;

	/* ReqID:TECC4.0 - Both operands must use the same DEK */
	cipherdata_check_dek_consistency(left, right);

	/* If both have DET ciphertexts, use DET for equality */
	if (CIPHERDATA_DATUM_DET_LEN(left) > 0 && CIPHERDATA_DATUM_DET_LEN(right) > 0)
	{
		if (CIPHERDATA_DATUM_DET_LEN(left) != CIPHERDATA_DATUM_DET_LEN(right))
			PG_RETURN_BOOL(false);
		PG_RETURN_BOOL(memcmp(CIPHERDATA_DATUM_DET_PTR(left), CIPHERDATA_DATUM_DET_PTR(right),
							 CIPHERDATA_DATUM_DET_LEN(left)) == 0);
	}

	/* DET not available on one or both sides — try OPE.
	 * DET and OPE are mutually exclusive; OPE is also deterministic
	 * and valid for equality comparison. */
	if (CIPHERDATA_DATUM_OPE_LEN(left) > 0 && CIPHERDATA_DATUM_OPE_LEN(right) > 0)
	{
		min_len = Min(CIPHERDATA_DATUM_OPE_LEN(left), CIPHERDATA_DATUM_OPE_LEN(right));
		cmp = memcmp(CIPHERDATA_DATUM_OPE_PTR(left), CIPHERDATA_DATUM_OPE_PTR(right), min_len);
		if (cmp != 0)
			PG_RETURN_BOOL(false);
		PG_RETURN_BOOL(CIPHERDATA_DATUM_OPE_LEN(left) == CIPHERDATA_DATUM_OPE_LEN(right));
	}

	/* Neither DET nor OPE available on both sides */
	ereport(ERROR,
			(errcode(ERRCODE_TECC_NO_DET_CIPHERTEXT),
			 errmsg("cannot compare encrypted values for equality: neither DET nor OPE ciphertext available"),
			 errhint("Enable DET or OPE encryption on the column to support equality comparison.")));

	PG_RETURN_BOOL(false);	/* unreachable */
}

/*
 * Generic inequality comparison function
 */
static Datum
cipherdata_ne(PG_FUNCTION_ARGS)
{
	bool		result;

	result = DatumGetBool(cipherdata_eq(fcinfo));

	PG_RETURN_BOOL(!result);
}

/*
 * Generic less-than comparison function
 *
 * For encrypted types, less-than is determined by comparing OPE ciphertexts.
 * If OPE ciphertexts are not available, the comparison returns false.
 */
static Datum
cipherdata_lt(PG_FUNCTION_ARGS)
{
	/* ReqID:TECC4.0 - Detoast to ensure 4-byte varlena header.
	 * PG_GETARG_POINTER may return a 1-byte short-header varlena
	 * for data < 126 bytes, shifting all CIPHERDATA_DATUM_* macro
	 * field offsets by 3 bytes.  PG_DETOAST_DATUM always returns
	 * a 4-byte header, fixing the offset assumptions. */
	Datum		left_datum = PG_GETARG_DATUM(0);
	Datum		right_datum = PG_GETARG_DATUM(1);
	const void *left = PG_DETOAST_DATUM(left_datum);
	const void *right = PG_DETOAST_DATUM(right_datum);
	int			cmp;

	if (CIPHERDATA_DATUM_OPE_LEN(left) == 0 || CIPHERDATA_DATUM_OPE_LEN(right) == 0)
		ereport(ERROR,
				(errcode(ERRCODE_TECC_NO_OPE_CIPHERTEXT),
				 errmsg("cannot compare encrypted values for ordering: OPE ciphertext not available"),
				 errhint("Enable OPE encryption on the column to support range comparison.")));

	cmp = cipherdata_cmp_internal(left, right);
	PG_RETURN_BOOL(cmp < 0);
}

/*
 * ReqID:TECC4.0 - min/max(int*_enc/float*_enc/numeric_enc/text_enc/
 * varchar_enc/bpchar_enc) 聚合 transfn（需求 4.4.1.4）：
 * 依赖 OPE 密文比较（对齐 cipherdata_lt 语义），返回较小/较大入参。
 * 平局返回第二个参数（对齐原生 int4smaller/int4larger 语义）。
 */
PG_FUNCTION_INFO_V1(cipherdata_smaller);
Datum
cipherdata_smaller(PG_FUNCTION_ARGS)
{
	Datum		left_datum = PG_GETARG_DATUM(0);
	Datum		right_datum = PG_GETARG_DATUM(1);
	const void *left = PG_DETOAST_DATUM(left_datum);
	const void *right = PG_DETOAST_DATUM(right_datum);
	int			cmp;

	if (CIPHERDATA_DATUM_OPE_LEN(left) == 0 || CIPHERDATA_DATUM_OPE_LEN(right) == 0)
		ereport(ERROR,
				(errcode(ERRCODE_TECC_NO_OPE_CIPHERTEXT),
				 errmsg("cannot compare encrypted values for ordering: OPE ciphertext not available"),
				 errhint("Enable OPE encryption on the column to support min/max aggregation.")));

	cmp = cipherdata_cmp_internal(left, right);
	return (cmp < 0) ? left_datum : right_datum;
}

PG_FUNCTION_INFO_V1(cipherdata_larger);
Datum
cipherdata_larger(PG_FUNCTION_ARGS)
{
	Datum		left_datum = PG_GETARG_DATUM(0);
	Datum		right_datum = PG_GETARG_DATUM(1);
	const void *left = PG_DETOAST_DATUM(left_datum);
	const void *right = PG_DETOAST_DATUM(right_datum);
	int			cmp;

	if (CIPHERDATA_DATUM_OPE_LEN(left) == 0 || CIPHERDATA_DATUM_OPE_LEN(right) == 0)
		ereport(ERROR,
				(errcode(ERRCODE_TECC_NO_OPE_CIPHERTEXT),
				 errmsg("cannot compare encrypted values for ordering: OPE ciphertext not available"),
				 errhint("Enable OPE encryption on the column to support min/max aggregation.")));

	cmp = cipherdata_cmp_internal(left, right);
	return (cmp > 0) ? left_datum : right_datum;
}

/*
 * Generic less-than-or-equal comparison function
 */
static Datum
cipherdata_le(PG_FUNCTION_ARGS)
{
	/* ReqID:TECC4.0 - Detoast to ensure 4-byte varlena header.
	 * PG_GETARG_POINTER may return a 1-byte short-header varlena
	 * for data < 126 bytes, shifting all CIPHERDATA_DATUM_* macro
	 * field offsets by 3 bytes.  PG_DETOAST_DATUM always returns
	 * a 4-byte header, fixing the offset assumptions. */
	Datum		left_datum = PG_GETARG_DATUM(0);
	Datum		right_datum = PG_GETARG_DATUM(1);
	const void *left = PG_DETOAST_DATUM(left_datum);
	const void *right = PG_DETOAST_DATUM(right_datum);
	int			cmp;

	if (CIPHERDATA_DATUM_OPE_LEN(left) == 0 || CIPHERDATA_DATUM_OPE_LEN(right) == 0)
		ereport(ERROR,
				(errcode(ERRCODE_TECC_NO_OPE_CIPHERTEXT),
				 errmsg("cannot compare encrypted values for ordering: OPE ciphertext not available"),
				 errhint("Enable OPE encryption on the column to support range comparison.")));

	cmp = cipherdata_cmp_internal(left, right);
	PG_RETURN_BOOL(cmp <= 0);
}

/*
 * Generic greater-than comparison function
 */
static Datum
cipherdata_gt(PG_FUNCTION_ARGS)
{
	/* ReqID:TECC4.0 - Detoast to ensure 4-byte varlena header.
	 * PG_GETARG_POINTER may return a 1-byte short-header varlena
	 * for data < 126 bytes, shifting all CIPHERDATA_DATUM_* macro
	 * field offsets by 3 bytes.  PG_DETOAST_DATUM always returns
	 * a 4-byte header, fixing the offset assumptions. */
	Datum		left_datum = PG_GETARG_DATUM(0);
	Datum		right_datum = PG_GETARG_DATUM(1);
	const void *left = PG_DETOAST_DATUM(left_datum);
	const void *right = PG_DETOAST_DATUM(right_datum);
	int			cmp;

	if (CIPHERDATA_DATUM_OPE_LEN(left) == 0 || CIPHERDATA_DATUM_OPE_LEN(right) == 0)
		ereport(ERROR,
				(errcode(ERRCODE_TECC_NO_OPE_CIPHERTEXT),
				 errmsg("cannot compare encrypted values for ordering: OPE ciphertext not available"),
				 errhint("Enable OPE encryption on the column to support range comparison.")));

	cmp = cipherdata_cmp_internal(left, right);
	PG_RETURN_BOOL(cmp > 0);
}

/*
 * Generic greater-than-or-equal comparison function
 */
static Datum
cipherdata_ge(PG_FUNCTION_ARGS)
{
	/* ReqID:TECC4.0 - Detoast to ensure 4-byte varlena header.
	 * PG_GETARG_POINTER may return a 1-byte short-header varlena
	 * for data < 126 bytes, shifting all CIPHERDATA_DATUM_* macro
	 * field offsets by 3 bytes.  PG_DETOAST_DATUM always returns
	 * a 4-byte header, fixing the offset assumptions. */
	Datum		left_datum = PG_GETARG_DATUM(0);
	Datum		right_datum = PG_GETARG_DATUM(1);
	const void *left = PG_DETOAST_DATUM(left_datum);
	const void *right = PG_DETOAST_DATUM(right_datum);
	int			cmp;

	if (CIPHERDATA_DATUM_OPE_LEN(left) == 0 || CIPHERDATA_DATUM_OPE_LEN(right) == 0)
		ereport(ERROR,
				(errcode(ERRCODE_TECC_NO_OPE_CIPHERTEXT),
				 errmsg("cannot compare encrypted values for ordering: OPE ciphertext not available"),
				 errhint("Enable OPE encryption on the column to support range comparison.")));

	cmp = cipherdata_cmp_internal(left, right);
	PG_RETURN_BOOL(cmp >= 0);
}

/*
 * Generic LIKE matching function for SEARCH ciphertexts
 *
 * For encrypted types, LIKE is determined by delegating to the native
 * textlike/textnlike functions on the raw SEARCH ciphertext bytes.
 * The SEARCH ciphertext format is: [version:4][data_len:4][char_hashes...]
 * We skip the 8-byte header and compare only the character hash sequence,
 * since the client-side encryption produces deterministic per-character
 * hashes that enable substring matching via native textlike.
 * If SEARCH ciphertexts are not available, the values are considered not matching.
 */

/*
 * ReqID:TECC4.0 - SEARCH raw byte-stream LIKE matching.
 *
 * Matches raw encrypted LIKE pattern against unescaped stored data.
 * Text is pre-unescaped. Pattern is raw with escapes and wildcards.
 */
static bool
tecc_search_match(const char *text, int tlen, const char *pat, int plen)
{
	int ti = 0, pi = 0;
	while (pi < plen)
	{
		if (pat[pi] == '\\' && pi + 1 < plen)
		{
			pi++;
			if (ti >= tlen || text[ti] != pat[pi]) return false;
			ti++; pi++;
		}
		else if (pat[pi] == '%')
		{
			int t;
			pi++;
			if (pi == plen) return true;

			/*
			 * ReqID:TECC4.0 - Each plaintext char maps to exactly 3
			 * hash bytes.  When the remaining pattern starts with a
			 * literal byte (not wildcard/escape), it begins a 3-byte
			 * character hash.  Try only 3-byte-aligned text positions
			 * to avoid futile per-byte comparisons.
			 */
			if (pi < plen &&
				pat[pi] != '%' && pat[pi] != '_' && pat[pi] != '\\')
			{
				int start_t = ti;

				/* snap to next char boundary */
				if (start_t % 3 != 0)
					start_t += 3 - (start_t % 3);
				for (t = start_t; t <= tlen; t += 3)
					if (tecc_search_match(text + t, tlen - t,
									 pat + pi, plen - pi))
						return true;
				return false;
			}

			for (t = ti; t <= tlen; t++)
				if (tecc_search_match(text + t, tlen - t,
								 pat + pi, plen - pi))
					return true;
			return false;
		}
		else if (pat[pi] == '_')
		{
			if (ti >= tlen) return false;
			ti++; pi++;
		}
		else
		{
			if (ti >= tlen || text[ti] != pat[pi]) return false;
			ti++; pi++;
		}
	}
	return (ti == tlen);
}

static Datum
cipherdata_like(PG_FUNCTION_ARGS)
{
	/* ReqID:TECC4.0 - Detoast to force 4-byte varlena header.
	 * PG_GETARG_POINTER may return a 1-byte short-header varlena
	 * for data < 126 bytes.  The CIPHERDATA_DATUM_* macros assume
	 * VARDATA_ANY returns ptr+4 (4-byte header), but for 1-byte
	 * headers VARDATA_ANY returns ptr+1, shifting all field offsets
	 * by 3 bytes and causing garbage SEARCH_LEN/SEARCH_PTR reads. */
	Datum		left_datum = PG_GETARG_DATUM(0);
	Datum		right_datum = PG_GETARG_DATUM(1);
	const void *left = PG_DETOAST_DATUM(left_datum);
	const void *right = PG_DETOAST_DATUM(right_datum);
	text	   *str;
	text	   *pat;
	bool		result;
	int32		search_len;
	const char *search_ptr;

	/* ReqID:TECC4.0 - Both operands must use the same DEK */
	cipherdata_check_dek_consistency(left, right);

	/* Check if both have SEARCH ciphertexts */
	if (CIPHERDATA_DATUM_SEARCH_LEN(left) == 0 || CIPHERDATA_DATUM_SEARCH_LEN(right) == 0)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("cannot perform LIKE matching on encrypted column without SEARCH ciphertext"),
				 errhint("Enable SEARCH encryption on the column to support LIKE matching.")));

	/* Skip SEARCH header (data_len:4) to compare raw char hashes.
	 * Allow search_len == 0 (empty string). ReqID:TECC4.0 */
	search_len = CIPHERDATA_DATUM_SEARCH_LEN(left) - 4;
	search_ptr = CIPHERDATA_DATUM_SEARCH_PTR(left) + 4;
	if (search_len < 0)
		PG_RETURN_BOOL(false);
	str = (text *) palloc(VARHDRSZ + search_len);
	SET_VARSIZE(str, VARHDRSZ + search_len);
	memcpy(VARDATA(str), search_ptr, search_len);

	/* Build text from right operand\'s SEARCH ciphertext (the LIKE pattern) */
	search_len = CIPHERDATA_DATUM_SEARCH_LEN(right) - 4;
	search_ptr = CIPHERDATA_DATUM_SEARCH_PTR(right) + 4;
	if (search_len < 0)
	{
		pfree(str);
		PG_RETURN_BOOL(false);
	}
	pat = (text *) palloc(VARHDRSZ + search_len);
	SET_VARSIZE(pat, VARHDRSZ + search_len);
	memcpy(VARDATA(pat), search_ptr, search_len);

	/*
	 * ReqID:TECC4.0 - Unescape text, match with raw pattern.
	 * Use stack buffer for common case (\u2264256 bytes) to avoid palloc.
	 */
	{
		const char *s = VARDATA_ANY(str);
		int slen = VARSIZE_ANY_EXHDR(str), k, uslen = 0;
		char us_buf[256];
		char   *us = (slen <= (int) sizeof(us_buf)) ? us_buf :
					  (char *) palloc(slen);

		for (k = 0; k < slen; k++)
		{
			if (s[k] == '\\' && k + 1 < slen)
				us[uslen++] = s[++k];
			else
				us[uslen++] = s[k];
		}
		result = tecc_search_match(us, uslen,
					   VARDATA_ANY(pat), VARSIZE_ANY_EXHDR(pat));
		if (us != us_buf)
			pfree(us);
	}
	pfree(str);
	pfree(pat);

	PG_RETURN_BOOL(result);
}




/*
 * Generic NOT LIKE matching function for SEARCH ciphertexts
 *
 * Delegates to native textnlike for pattern matching on SEARCH ciphertexts.
 */
static Datum
cipherdata_nlike(PG_FUNCTION_ARGS)
{
	/* ReqID:TECC4.0 - Detoast to force 4-byte varlena header. */
	Datum		left_datum = PG_GETARG_DATUM(0);
	Datum		right_datum = PG_GETARG_DATUM(1);
	const void *left = PG_DETOAST_DATUM(left_datum);
	const void *right = PG_DETOAST_DATUM(right_datum);
	text	   *str;
	text	   *pat;
	bool		result;
	int32		search_len;
	const char *search_ptr;

	/* ReqID:TECC4.0 - Both operands must use the same DEK */
	cipherdata_check_dek_consistency(left, right);

	/* Check if both have SEARCH ciphertexts */
	if (CIPHERDATA_DATUM_SEARCH_LEN(left) == 0 || CIPHERDATA_DATUM_SEARCH_LEN(right) == 0)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("cannot perform LIKE matching on encrypted column without SEARCH ciphertext"),
				 errhint("Enable SEARCH encryption on the column to support LIKE matching.")));

	/* Skip SEARCH header (data_len:4), ReqID:TECC4.0 */
	search_len = CIPHERDATA_DATUM_SEARCH_LEN(left) - 4;
	search_ptr = CIPHERDATA_DATUM_SEARCH_PTR(left) + 4;
	if (search_len < 0)
		PG_RETURN_BOOL(false);
	str = (text *) palloc(VARHDRSZ + search_len);
	SET_VARSIZE(str, VARHDRSZ + search_len);
	memcpy(VARDATA(str), search_ptr, search_len);

	/* Build text from right operand\'s SEARCH ciphertext */
	search_len = CIPHERDATA_DATUM_SEARCH_LEN(right) - 4;
	search_ptr = CIPHERDATA_DATUM_SEARCH_PTR(right) + 4;
	if (search_len < 0)
	{
		pfree(str);
		PG_RETURN_BOOL(false);
	}
	pat = (text *) palloc(VARHDRSZ + search_len);
	SET_VARSIZE(pat, VARHDRSZ + search_len);
	memcpy(VARDATA(pat), search_ptr, search_len);

	/*
	 * ReqID:TECC4.0 - Unescape text and NOT-match.
	 * Use stack buffer for common case (\u2264256 bytes) to avoid palloc.
	 */
	{
		const char *s = VARDATA_ANY(str);
		int slen = VARSIZE_ANY_EXHDR(str), k, uslen = 0;
		char us_buf[256];
		char   *us = (slen <= (int) sizeof(us_buf)) ? us_buf :
					  (char *) palloc(slen);

		for (k = 0; k < slen; k++)
		{
			if (s[k] == '\\' && k + 1 < slen)
				us[uslen++] = s[++k];
			else
				us[uslen++] = s[k];
		}
		result = tecc_search_match(us, uslen,
					   VARDATA_ANY(pat), VARSIZE_ANY_EXHDR(pat));
		if (us != us_buf)
			pfree(us);
	}
	pfree(str);
	pfree(pat);

	PG_RETURN_BOOL(!result);
}




/*
 * Generic comparison function for B-tree indexing
 *
 * Returns: -1 if left < right, 0 if equal, 1 if left > right
 */
static Datum
cipherdata_cmp(PG_FUNCTION_ARGS)
{
	/* ReqID:TECC4.0 - Detoast to ensure 4-byte varlena header.
	 * PG_GETARG_POINTER may return a 1-byte short-header varlena
	 * for data < 126 bytes, shifting all CIPHERDATA_DATUM_* macro
	 * field offsets by 3 bytes.  PG_DETOAST_DATUM always returns
	 * a 4-byte header, fixing the offset assumptions. */
	Datum		left_datum = PG_GETARG_DATUM(0);
	Datum		right_datum = PG_GETARG_DATUM(1);
	const void *left = PG_DETOAST_DATUM(left_datum);
	const void *right = PG_DETOAST_DATUM(right_datum);

	PG_RETURN_INT32(cipherdata_cmp_internal(left, right));
}

/*
 * Type-specific operator functions
 * These are thin wrappers around the generic cipherdata operator functions
 */

/* int2_enc operator functions */
PG_FUNCTION_INFO_V1(int2_enc_eq);
Datum
int2_enc_eq(PG_FUNCTION_ARGS)
{
	return cipherdata_eq(fcinfo);
}

PG_FUNCTION_INFO_V1(int2_enc_ne);
Datum
int2_enc_ne(PG_FUNCTION_ARGS)
{
	return cipherdata_ne(fcinfo);
}

PG_FUNCTION_INFO_V1(int2_enc_lt);
Datum
int2_enc_lt(PG_FUNCTION_ARGS)
{
	return cipherdata_lt(fcinfo);
}

PG_FUNCTION_INFO_V1(int2_enc_le);
Datum
int2_enc_le(PG_FUNCTION_ARGS)
{
	return cipherdata_le(fcinfo);
}

PG_FUNCTION_INFO_V1(int2_enc_gt);
Datum
int2_enc_gt(PG_FUNCTION_ARGS)
{
	return cipherdata_gt(fcinfo);
}

PG_FUNCTION_INFO_V1(int2_enc_ge);
Datum
int2_enc_ge(PG_FUNCTION_ARGS)
{
	return cipherdata_ge(fcinfo);
}

PG_FUNCTION_INFO_V1(int2_enc_cmp);
Datum
int2_enc_cmp(PG_FUNCTION_ARGS)
{
	return cipherdata_cmp(fcinfo);
}

/* int4_enc operator functions */
PG_FUNCTION_INFO_V1(int4_enc_eq);
Datum
int4_enc_eq(PG_FUNCTION_ARGS)
{
	return cipherdata_eq(fcinfo);
}

PG_FUNCTION_INFO_V1(int4_enc_ne);
Datum
int4_enc_ne(PG_FUNCTION_ARGS)
{
	return cipherdata_ne(fcinfo);
}

PG_FUNCTION_INFO_V1(int4_enc_lt);
Datum
int4_enc_lt(PG_FUNCTION_ARGS)
{
	return cipherdata_lt(fcinfo);
}

PG_FUNCTION_INFO_V1(int4_enc_le);
Datum
int4_enc_le(PG_FUNCTION_ARGS)
{
	return cipherdata_le(fcinfo);
}

PG_FUNCTION_INFO_V1(int4_enc_gt);
Datum
int4_enc_gt(PG_FUNCTION_ARGS)
{
	return cipherdata_gt(fcinfo);
}

PG_FUNCTION_INFO_V1(int4_enc_ge);
Datum
int4_enc_ge(PG_FUNCTION_ARGS)
{
	return cipherdata_ge(fcinfo);
}

PG_FUNCTION_INFO_V1(int4_enc_cmp);
Datum
int4_enc_cmp(PG_FUNCTION_ARGS)
{
	return cipherdata_cmp(fcinfo);
}

/* int8_enc operator functions */
PG_FUNCTION_INFO_V1(int8_enc_eq);
Datum
int8_enc_eq(PG_FUNCTION_ARGS)
{
	return cipherdata_eq(fcinfo);
}

PG_FUNCTION_INFO_V1(int8_enc_ne);
Datum
int8_enc_ne(PG_FUNCTION_ARGS)
{
	return cipherdata_ne(fcinfo);
}

PG_FUNCTION_INFO_V1(int8_enc_lt);
Datum
int8_enc_lt(PG_FUNCTION_ARGS)
{
	return cipherdata_lt(fcinfo);
}

PG_FUNCTION_INFO_V1(int8_enc_le);
Datum
int8_enc_le(PG_FUNCTION_ARGS)
{
	return cipherdata_le(fcinfo);
}

PG_FUNCTION_INFO_V1(int8_enc_gt);
Datum
int8_enc_gt(PG_FUNCTION_ARGS)
{
	return cipherdata_gt(fcinfo);
}

PG_FUNCTION_INFO_V1(int8_enc_ge);
Datum
int8_enc_ge(PG_FUNCTION_ARGS)
{
	return cipherdata_ge(fcinfo);
}

PG_FUNCTION_INFO_V1(int8_enc_cmp);
Datum
int8_enc_cmp(PG_FUNCTION_ARGS)
{
	return cipherdata_cmp(fcinfo);
}

/* float4_enc operator functions */
PG_FUNCTION_INFO_V1(float4_enc_eq);
Datum
float4_enc_eq(PG_FUNCTION_ARGS)
{
	return cipherdata_eq(fcinfo);
}

PG_FUNCTION_INFO_V1(float4_enc_ne);
Datum
float4_enc_ne(PG_FUNCTION_ARGS)
{
	return cipherdata_ne(fcinfo);
}

PG_FUNCTION_INFO_V1(float4_enc_lt);
Datum
float4_enc_lt(PG_FUNCTION_ARGS)
{
	return cipherdata_lt(fcinfo);
}

PG_FUNCTION_INFO_V1(float4_enc_le);
Datum
float4_enc_le(PG_FUNCTION_ARGS)
{
	return cipherdata_le(fcinfo);
}

PG_FUNCTION_INFO_V1(float4_enc_gt);
Datum
float4_enc_gt(PG_FUNCTION_ARGS)
{
	return cipherdata_gt(fcinfo);
}

PG_FUNCTION_INFO_V1(float4_enc_ge);
Datum
float4_enc_ge(PG_FUNCTION_ARGS)
{
	return cipherdata_ge(fcinfo);
}

PG_FUNCTION_INFO_V1(float4_enc_cmp);
Datum
float4_enc_cmp(PG_FUNCTION_ARGS)
{
	return cipherdata_cmp(fcinfo);
}

/* float8_enc operator functions */
PG_FUNCTION_INFO_V1(float8_enc_eq);
Datum
float8_enc_eq(PG_FUNCTION_ARGS)
{
	return cipherdata_eq(fcinfo);
}

PG_FUNCTION_INFO_V1(float8_enc_ne);
Datum
float8_enc_ne(PG_FUNCTION_ARGS)
{
	return cipherdata_ne(fcinfo);
}

PG_FUNCTION_INFO_V1(float8_enc_lt);
Datum
float8_enc_lt(PG_FUNCTION_ARGS)
{
	return cipherdata_lt(fcinfo);
}

PG_FUNCTION_INFO_V1(float8_enc_le);
Datum
float8_enc_le(PG_FUNCTION_ARGS)
{
	return cipherdata_le(fcinfo);
}

PG_FUNCTION_INFO_V1(float8_enc_gt);
Datum
float8_enc_gt(PG_FUNCTION_ARGS)
{
	return cipherdata_gt(fcinfo);
}

PG_FUNCTION_INFO_V1(float8_enc_ge);
Datum
float8_enc_ge(PG_FUNCTION_ARGS)
{
	return cipherdata_ge(fcinfo);
}

PG_FUNCTION_INFO_V1(float8_enc_cmp);
Datum
float8_enc_cmp(PG_FUNCTION_ARGS)
{
	return cipherdata_cmp(fcinfo);
}

/* numeric_enc operator functions */
PG_FUNCTION_INFO_V1(numeric_enc_eq);
Datum
numeric_enc_eq(PG_FUNCTION_ARGS)
{
	return cipherdata_eq(fcinfo);
}

PG_FUNCTION_INFO_V1(numeric_enc_ne);
Datum
numeric_enc_ne(PG_FUNCTION_ARGS)
{
	return cipherdata_ne(fcinfo);
}

PG_FUNCTION_INFO_V1(numeric_enc_lt);
Datum
numeric_enc_lt(PG_FUNCTION_ARGS)
{
	return cipherdata_lt(fcinfo);
}

PG_FUNCTION_INFO_V1(numeric_enc_le);
Datum
numeric_enc_le(PG_FUNCTION_ARGS)
{
	return cipherdata_le(fcinfo);
}

PG_FUNCTION_INFO_V1(numeric_enc_gt);
Datum
numeric_enc_gt(PG_FUNCTION_ARGS)
{
	return cipherdata_gt(fcinfo);
}

PG_FUNCTION_INFO_V1(numeric_enc_ge);
Datum
numeric_enc_ge(PG_FUNCTION_ARGS)
{
	return cipherdata_ge(fcinfo);
}

PG_FUNCTION_INFO_V1(numeric_enc_cmp);
Datum
numeric_enc_cmp(PG_FUNCTION_ARGS)
{
	return cipherdata_cmp(fcinfo);
}

/*
 * ReqID:TECC4.0 - varchar_enc equality comparison
 *
 * VARCHAR semantic rules (same as TEXT): trailing spaces are significant.
 * Semantics and the shared, DEK-checked implementation live in
 * cipherdata_eq_text.
 */
PG_FUNCTION_INFO_V1(varchar_enc_eq);
Datum
varchar_enc_eq(PG_FUNCTION_ARGS)
{
	return cipherdata_eq_text(fcinfo);
}

PG_FUNCTION_INFO_V1(varchar_enc_ne);
Datum
varchar_enc_ne(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(!DatumGetBool(varchar_enc_eq(fcinfo)));
}

/*
 * ReqID:TECC4.0 - varchar_enc range operators use varchar_enc_cmp to ensure
 * OPE format_byte and space_count are properly stripped before comparison.
 */
PG_FUNCTION_INFO_V1(varchar_enc_lt);
Datum
varchar_enc_lt(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(DatumGetInt32(varchar_enc_cmp(fcinfo)) < 0);
}

PG_FUNCTION_INFO_V1(varchar_enc_le);
Datum
varchar_enc_le(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(DatumGetInt32(varchar_enc_cmp(fcinfo)) <= 0);
}

PG_FUNCTION_INFO_V1(varchar_enc_gt);
Datum
varchar_enc_gt(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(DatumGetInt32(varchar_enc_cmp(fcinfo)) > 0);
}

PG_FUNCTION_INFO_V1(varchar_enc_ge);
Datum
varchar_enc_ge(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(DatumGetInt32(varchar_enc_cmp(fcinfo)) >= 0);
}

/*
 * ReqID:TECC4.0 - varchar_enc range comparison (ORDER BY, <, >, BETWEEN)
 *
 * VARCHAR ordering rule: trailing spaces matter (more spaces = larger).
 * Semantics and the shared, DEK-checked implementation live in
 * cipherdata_cmp_text.
 */
PG_FUNCTION_INFO_V1(varchar_enc_cmp);
Datum
varchar_enc_cmp(PG_FUNCTION_ARGS)
{
	return cipherdata_cmp_text(fcinfo);
}

PG_FUNCTION_INFO_V1(varchar_enc_like);
Datum
varchar_enc_like(PG_FUNCTION_ARGS)
{
	return cipherdata_like(fcinfo);
}

PG_FUNCTION_INFO_V1(varchar_enc_nlike);
Datum
varchar_enc_nlike(PG_FUNCTION_ARGS)
{
	return cipherdata_nlike(fcinfo);
}

/*
 * ReqID:TECC4.0 - bpchar_enc equality comparison
 *
 * BPCHAR semantic rule: ignore trailing spaces in comparisons.
 * Delegates to cipherdata_eq_bpchar which strips format_byte+space_count
 * and compares only the encrypted core.
 */
PG_FUNCTION_INFO_V1(bpchar_enc_eq);
Datum
bpchar_enc_eq(PG_FUNCTION_ARGS)
{
	return cipherdata_eq_bpchar(fcinfo);
}

PG_FUNCTION_INFO_V1(bpchar_enc_ne);
Datum
bpchar_enc_ne(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(!DatumGetBool(bpchar_enc_eq(fcinfo)));
}

/*
 * ReqID:TECC4.0 - bpchar_enc range operators use bpchar_enc_cmp to ensure
 * OPE format_byte and space_count are properly stripped before comparison.
 * bpchar_enc_cmp ignores space_count (BPCHAR semantic: trailing spaces
 * don't affect ordering).
 */
PG_FUNCTION_INFO_V1(bpchar_enc_lt);
Datum
bpchar_enc_lt(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(DatumGetInt32(bpchar_enc_cmp(fcinfo)) < 0);
}

PG_FUNCTION_INFO_V1(bpchar_enc_le);
Datum
bpchar_enc_le(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(DatumGetInt32(bpchar_enc_cmp(fcinfo)) <= 0);
}

PG_FUNCTION_INFO_V1(bpchar_enc_gt);
Datum
bpchar_enc_gt(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(DatumGetInt32(bpchar_enc_cmp(fcinfo)) > 0);
}

PG_FUNCTION_INFO_V1(bpchar_enc_ge);
Datum
bpchar_enc_ge(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(DatumGetInt32(bpchar_enc_cmp(fcinfo)) >= 0);
}

/*
 * ReqID:TECC4.0 - bpchar_enc range comparison (ORDER BY, <, >, BETWEEN)
 *
 * BPCHAR ordering rule: trailing spaces are ignored.
 * Semantics and the shared, DEK-checked implementation live in
 * cipherdata_cmp_bpchar.
 */
PG_FUNCTION_INFO_V1(bpchar_enc_cmp);
Datum
bpchar_enc_cmp(PG_FUNCTION_ARGS)
{
	return cipherdata_cmp_bpchar(fcinfo);
}

PG_FUNCTION_INFO_V1(bpchar_enc_like);
Datum
bpchar_enc_like(PG_FUNCTION_ARGS)
{
	return cipherdata_like(fcinfo);
}

PG_FUNCTION_INFO_V1(bpchar_enc_nlike);
Datum
bpchar_enc_nlike(PG_FUNCTION_ARGS)
{
	return cipherdata_nlike(fcinfo);
}

/*
 * ReqID:TECC4.0 - text_enc equality comparison
 *
 * TEXT semantic rule: retain trailing spaces in comparisons.
 *   - 'abc'::text = 'abc   '::text → f (A4)
 *   - '   '::text = '   '::text    → t (D6)
 *   - '   '::text = ''::text       → f
 *
 * Cross-type: text_enc is the preferred type in 'U' category.
 * When bpchar_enc = text_enc, PG converts bpchar_enc→text_enc and
 * uses text_enc_eq, giving TEXT semantics (trailing spaces matter).
 *   - tx('abc') = bp('abc   ') → t  (B8: bp→text, rtrim to 'abc')
 *   - tx('abc   ') = bp('abc') → f  (B9: tx keeps trailing spaces)
 *
 * text_enc = varchar_enc: explicit operator → text_enc_eq
 * Implementation: shared with varchar_enc, see cipherdata_eq_text.
 */
PG_FUNCTION_INFO_V1(text_enc_eq);
Datum
text_enc_eq(PG_FUNCTION_ARGS)
{
	return cipherdata_eq_text(fcinfo);
}

PG_FUNCTION_INFO_V1(text_enc_ne);
Datum
text_enc_ne(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(!DatumGetBool(text_enc_eq(fcinfo)));
}

/*
 * ReqID:TECC4.0 - text_enc range operators use text_enc_cmp to ensure
 * OPE format_byte and space_count are properly stripped before comparison.
 * Generic cipherdata_cmp_internal compares raw OPE ciphertexts (including
 * format_byte and space_count), which gives wrong results for text types
 * where trailing spaces must be handled per-type semantics.
 */
PG_FUNCTION_INFO_V1(text_enc_lt);
Datum
text_enc_lt(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(DatumGetInt32(text_enc_cmp(fcinfo)) < 0);
}

PG_FUNCTION_INFO_V1(text_enc_le);
Datum
text_enc_le(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(DatumGetInt32(text_enc_cmp(fcinfo)) <= 0);
}

PG_FUNCTION_INFO_V1(text_enc_gt);
Datum
text_enc_gt(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(DatumGetInt32(text_enc_cmp(fcinfo)) > 0);
}

PG_FUNCTION_INFO_V1(text_enc_ge);
Datum
text_enc_ge(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(DatumGetInt32(text_enc_cmp(fcinfo)) >= 0);
}

/*
 * ReqID:TECC4.0 - text_enc range comparison (ORDER BY, <, >, BETWEEN)
 *
 * TEXT ordering rule: trailing spaces matter (more spaces = larger).
 *   - 'abc'::text < 'abc   '::text → t (spaces make larger)
 *   - ''::text < '   '::text       → t
 *
 * Implementation: shared with varchar_enc, see cipherdata_cmp_text.
 */
PG_FUNCTION_INFO_V1(text_enc_cmp);
Datum
text_enc_cmp(PG_FUNCTION_ARGS)
{
	return cipherdata_cmp_text(fcinfo);
}

PG_FUNCTION_INFO_V1(text_enc_like);
Datum
text_enc_like(PG_FUNCTION_ARGS)
{
	return cipherdata_like(fcinfo);
}

PG_FUNCTION_INFO_V1(text_enc_nlike);
Datum
text_enc_nlike(PG_FUNCTION_ARGS)
{
	return cipherdata_nlike(fcinfo);
}

/* ReqID:TECC4.0 - LIKE ESCAPE passthrough for encrypted types.
 * PG parser rewrites `LIKE ... ESCAPE '\'` to `~~ like_escape(pattern, esc)`.
 * SEARCH ciphertext must NOT be processed by standard like_escape (which
 * does text-level escape transformation). cipherdata_like's tecc_search_match
 * already handles \ as escape internally. So this is a pure passthrough. */
PG_FUNCTION_INFO_V1(varchar_enc_like_escape);
Datum
varchar_enc_like_escape(PG_FUNCTION_ARGS)
{
	PG_RETURN_DATUM(PG_GETARG_DATUM(0));
}

PG_FUNCTION_INFO_V1(bpchar_enc_like_escape);
Datum
bpchar_enc_like_escape(PG_FUNCTION_ARGS)
{
	PG_RETURN_DATUM(PG_GETARG_DATUM(0));
}

PG_FUNCTION_INFO_V1(text_enc_like_escape);
Datum
text_enc_like_escape(PG_FUNCTION_ARGS)
{
	PG_RETURN_DATUM(PG_GETARG_DATUM(0));
}

/* Begin - ReqID:TECC4.0 */

/*
 * Generic hash function for encrypted types
 *
 * Hash principle: Use DET ciphertext for hash calculation
 * Returns: 32-bit hash value
 */
static Datum
cipherdata_hash_func(PG_FUNCTION_ARGS)
{
	/* ReqID:TECC4.0 - Detoast to ensure 4-byte varlena header. */
	Datum		cipherdata_datum = PG_GETARG_DATUM(0);
	const void *cipherdata = PG_DETOAST_DATUM(cipherdata_datum);
	uint32		hash_value;

	/* Validate input (uses header-independent field access) */
	if (!cipherdata_validate((const CipherDataType *) cipherdata))
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_BINARY_REPRESENTATION),
					 errmsg("invalid cipherdata pointer")));

	/* Use DET ciphertext if available, otherwise try OPE (both deterministic) */
	if (CIPHERDATA_DATUM_DET_LEN(cipherdata) > 0)
	{
		hash_value = hash_any((unsigned char *) CIPHERDATA_DATUM_DET_PTR(cipherdata),
							  CIPHERDATA_DATUM_DET_LEN(cipherdata));
	}
	else if (CIPHERDATA_DATUM_OPE_LEN(cipherdata) > 0)
	{
		hash_value = hash_any((unsigned char *) CIPHERDATA_DATUM_OPE_PTR(cipherdata),
							  CIPHERDATA_DATUM_OPE_LEN(cipherdata));
	}
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("cannot hash encrypted value without DET or OPE ciphertext"),
				 errhint("Enable DET or OPE encryption for the column to support Hash indexes.")));
	}

	PG_RETURN_UINT32(hash_value);
}

/*
 * Type-specific hash functions for Hash index support
 */

/* int2_enc hash function */
PG_FUNCTION_INFO_V1(hashint2_enc);
Datum
hashint2_enc(PG_FUNCTION_ARGS)
{
	return cipherdata_hash_func(fcinfo);
}

/* int4_enc hash function */
PG_FUNCTION_INFO_V1(hashint4_enc);
Datum
hashint4_enc(PG_FUNCTION_ARGS)
{
	return cipherdata_hash_func(fcinfo);
}

/* int8_enc hash function */
PG_FUNCTION_INFO_V1(hashint8_enc);
Datum
hashint8_enc(PG_FUNCTION_ARGS)
{
	return cipherdata_hash_func(fcinfo);
}

/* float4_enc hash function */
PG_FUNCTION_INFO_V1(hashfloat4_enc);
Datum
hashfloat4_enc(PG_FUNCTION_ARGS)
{
	return cipherdata_hash_func(fcinfo);
}

/* float8_enc hash function */
PG_FUNCTION_INFO_V1(hashfloat8_enc);
Datum
hashfloat8_enc(PG_FUNCTION_ARGS)
{
	return cipherdata_hash_func(fcinfo);
}

/* numeric_enc hash function */
PG_FUNCTION_INFO_V1(hashnumeric_enc);
Datum
hashnumeric_enc(PG_FUNCTION_ARGS)
{
	return cipherdata_hash_func(fcinfo);
}

/* varchar_enc hash function */
PG_FUNCTION_INFO_V1(hashvarchar_enc);
Datum
hashvarchar_enc(PG_FUNCTION_ARGS)
{
	return cipherdata_hash_func(fcinfo);
}

/* bpchar_enc hash function */
PG_FUNCTION_INFO_V1(hashbpchar_enc);
Datum
hashbpchar_enc(PG_FUNCTION_ARGS)
{
	return cipherdata_hash_func(fcinfo);
}

/* text_enc hash function */
PG_FUNCTION_INFO_V1(hashtext_enc);
Datum
hashtext_enc(PG_FUNCTION_ARGS)
{
	return cipherdata_hash_func(fcinfo);
}

/*
 * equalimage support function for encrypted types
 *
 * ReqID:TECC4.0 - PostgreSQL btree deduplication requires an opclass to
 * guarantee that keys equal under its comparison semantics can safely share
 * one physical index key image. TECC encrypted datums do not promise that
 * image-equivalence contract across encryption formats and metadata, so
 * report false and keep btree deduplication disabled for TECC opclasses.
 * This function is used as BTEQUALIMAGE_PROC (support function 4)
 * in btree operator families for encrypted data types.
 */
PG_FUNCTION_INFO_V1(tecc_equalimage);
Datum
tecc_equalimage(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(false);
}

/* End - ReqID:TECC4.0 */