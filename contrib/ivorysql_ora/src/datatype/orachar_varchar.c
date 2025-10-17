#include "postgres.h"
#include "access/hash.h"
#include "access/detoast.h"
#include "catalog/pg_collation.h"
#include "libpq/pqformat.h"
#include "nodes/nodeFuncs.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/ora_compatible.h" 
#include "utils/sortsupport.h"
#include "utils/varlena.h"
#include "mb/pg_wchar.h"
#include "fmgr.h"

#define MaxVarcharSize 32767

/* char(char) */
PG_FUNCTION_INFO_V1(oracharcharin_2);
PG_FUNCTION_INFO_V1(oracharcharout_2);

/* char(byte) */
PG_FUNCTION_INFO_V1(oracharbytein_2);
PG_FUNCTION_INFO_V1(oracharbyteout_2);	

/* varchar(char) */ 
PG_FUNCTION_INFO_V1(oravarcharcharin_2);
PG_FUNCTION_INFO_V1(oravarcharcharout_2);

/* varchar(byte) */
PG_FUNCTION_INFO_V1(oravarcharbytein_2);
PG_FUNCTION_INFO_V1(oravarcharbyteout_2);



/* Begin - char(char) */

/* returns the length (counted in wchars) of a multibyte string
 * (not necessarily NULL terminated)
 * Chinese : 瀚高\0基础软件
 */
static int
pg_mbstrlen_with_len_2(const char *mbstr, int limit)
{
	int			len = 0;

	/* optimization for single byte encoding */
	if (pg_database_encoding_max_length() == 1)
		return limit;

	/* while (limit > 0 && *mbstr) */
	while (limit > 0)
	{
		int	l = pg_mblen(mbstr);
		limit -= l;
		mbstr += l;
		len++;
	}
	return len;
}

/*******************************************************************
 * bpchar_input -- common guts of oracharcharin
 *
 * s is the input text of length len (may not be null-terminated)
 * atttypmod is the typmod value to apply
 *
 * Note that atttypmod is measured in characters, which
 * is not necessarily the same as the number of bytes.
 * 
 ******************************************************************/
static BpChar *
bpchar_input_char(const char *s, size_t len, int32 atttypmod)
{
	BpChar	   *result;
	char	   *r;
	size_t		maxlen;

	/* If typmod is -1 (or invalid), use the actual string length */
	if (atttypmod < (int32) VARHDRSZ)
		maxlen = len;
	else
	{
		size_t		charlen;	/* number of CHARACTERS in the input */

		maxlen = atttypmod - VARHDRSZ;

		/* charlen = pg_mbstrlen_with_len(s, len); */
		charlen = pg_mbstrlen_with_len_2(s, len);

		if (charlen > maxlen)
		{
			ereport(ERROR,
				(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
				 errmsg("value too long for type char(%d char)",
				(int) maxlen)));
		}
		else
		{
			/*
 			 * Now we set maxlen to the necessary byte length, not the number
 			 * of CHARACTERS!
 			 */
			maxlen = len + (maxlen - charlen);
		}
	}

	result = (BpChar *) palloc(maxlen + VARHDRSZ);
	SET_VARSIZE(result, maxlen + VARHDRSZ);
	r = VARDATA(result);
	memcpy(r, s, len);

	/* blank pad the string if necessary */
	if (maxlen > len)
		memset(r + len, ' ', maxlen - len);

	return result;
}

/*********************************************************************
 *
 * Convert a C string to CHARACTER internal representation.
 * atttypmod is the declared length of the type plus VARHDRSZ.
 *
 *********************************************************************/
Datum
oracharcharin_2(PG_FUNCTION_ARGS)
{
	char       *s = PG_GETARG_CSTRING(0);

#ifdef NOT_USED
	Oid                     typelem = PG_GETARG_OID(1);
#endif
	int32           atttypmod = PG_GETARG_INT32(2);

	uint32          lens = PG_GETARG_UINT64(3);
	BpChar     *result;

	/* result = bpchar_input(s, strlen(s), atttypmod); */
	result = bpchar_input_char(s, lens, atttypmod);
	PG_RETURN_BPCHAR_P(result);
}

/*********************************************************************
 *
 * Convert a CHARACTER value to a C string.
 * Uses the text conversion functions, which is only appropriate
 * if oracharchar and text are equivalent types.
 *
 *********************************************************************/
Datum
oracharcharout_2(PG_FUNCTION_ARGS)
{
	Datum           txt = PG_GETARG_DATUM(0);

	PG_RETURN_CSTRING(TextDatumGetCString(txt));
}

/* End - char(char) */



/* Begin - char(byte) */

/*******************************************************************
 * bpchar_input -- common guts of oracharbytein
 *
 * s is the input text of length len (may not be null-terminated)
 * atttypmod is the typmod value to apply
 *
 * Note that atttypmod is measured in bytes
 *
 ******************************************************************/
static BpChar *
bpchar_input_byte(const char *s, size_t len, int32 atttypmod)
{
	BpChar	   *result;
	char	   *r;
	size_t		maxlen;

	/* If typmod is -1 (or invalid), use the actual string length */
	if (atttypmod < (int32) VARHDRSZ)
		maxlen = len;
	else
	{
		maxlen = atttypmod - VARHDRSZ;
		if (len > maxlen)
		{
			ereport(ERROR,
				(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
				 errmsg("value too long for type char(%d byte)",
				(int) maxlen)));
		}
	}

	result = (BpChar *) palloc(maxlen + VARHDRSZ);
	SET_VARSIZE(result, maxlen + VARHDRSZ);
	r = VARDATA(result);
	memcpy(r, s, len);

	/* blank pad the string if necessary */
	if (maxlen > len)
		memset(r + len, ' ', maxlen - len);

	return result;
}

/*********************************************************************
 *
 * Convert a C string to CHARACTER internal representation. 
 * atttypmod is the declared length of the type plus VARHDRSZ.
 *
 *********************************************************************/
Datum
oracharbytein_2(PG_FUNCTION_ARGS)
{
	char	   *s = PG_GETARG_CSTRING(0);

#ifdef NOT_USED
	Oid			typelem = PG_GETARG_OID(1);
#endif
	int32		atttypmod = PG_GETARG_INT32(2);
	uint32		lens = PG_GETARG_UINT64(3);
	BpChar	   *result;

	/* result = bpchar_input(s, strlen(s), atttypmod); */
	result = bpchar_input_byte(s, lens, atttypmod);
	PG_RETURN_BPCHAR_P(result);
}

/*********************************************************************
 * 
 * Convert a CHARACTER value to a C string.
 * Uses the text conversion functions, which is only appropriate 
 * if oracharbyte and text are equivalent types.
 *
 *********************************************************************/
Datum
oracharbyteout_2(PG_FUNCTION_ARGS)
{
	Datum		txt = PG_GETARG_DATUM(0);

	PG_RETURN_CSTRING(TextDatumGetCString(txt));
}

/* End - char(byte) */



/* Begin - varchar(char) */

/* mbcliplen for any single-byte encoding */
static int
cliplen_2(const char *str, int len, int limit)
{
        int                     l = 0;

        len = Min(len, limit);
        /* while (l < len && str[l]) */
        while (l < len)
                l++;
        return l;
}


/*
 * Similar to pg_mbcliplen except the limit parameter specifies the
 * character length, not the byte length.
 */
static int
pg_mbcharcliplen_2(const char *mbstr, int len, int limit)
{
	int			clen = 0;
	int			nch = 0;
	int			l;

	/* optimization for single byte encoding */
	if (pg_database_encoding_max_length() == 1)
		return cliplen_2(mbstr, len, limit);

	/* while (len > 0 && *mbstr) */
	while (len > 0)
	{
		l = pg_mblen(mbstr);
		nch++;
		if (nch > limit)
			break;
		clen += l;
		len -= l;
		mbstr += l;
	}
	return clen;
}

/*
 * varchar_input -- common guts of oravarcharcharin and oravarcharcharrecv
 *
 * "atttypmod" is measured in characters ,but for "oravarcharbyte" type is   
 * measured in bytes.
 * 
 * Remove the Verify of extra spaces characters
 * 
 */
static VarChar *
varchar_input_char(const char *s, size_t len, int32 atttypmod)
{
	VarChar    *result;
	size_t		maxlen;
	
	maxlen = atttypmod - VARHDRSZ;

	if (atttypmod >= (int32) VARHDRSZ && len > maxlen)
	{
		/* Verify that the length of multi-byte characters equal len */
		size_t		mbmaxlen = pg_mbcharcliplen_2(s, len, maxlen);

		if (len != mbmaxlen)
			ereport(ERROR,
				(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
				 errmsg("value too long for type varchar2(%d char)",
				 (int) maxlen)));
	}

	result = (VarChar *) palloc(len + VARHDRSZ);
	SET_VARSIZE(result, len + VARHDRSZ);
	memcpy(VARDATA(result), s, len);
	return result;
}

/*
 * Convert a C string to VARCHAR2 internal representation.
 * atttypmod is the declared length of the type plus VARHDRSZ.
 */
Datum
oravarcharcharin_2(PG_FUNCTION_ARGS)
{
	char       *s = PG_GETARG_CSTRING(0);

#ifdef NOT_USED
	Oid                     typelem = PG_GETARG_OID(1);
#endif
	int32           atttypmod = PG_GETARG_INT32(2);

	uint32           lens = PG_GETARG_UINT64(3);

	VarChar    *result;

	/* result = varchar_input(s, strlen(s), atttypmod); */
	result = varchar_input_char(s, lens, atttypmod);
	PG_RETURN_VARCHAR_P(result);
}

/*
 * Convert a VARCHAR2 value to a C string.
 *
 * Uses the text to C string conversion function, which is only appropriate
 * if VarChar and text are equivalent types.
 */
Datum
oravarcharcharout_2(PG_FUNCTION_ARGS)
{
	Datum           txt = PG_GETARG_DATUM(0);

	PG_RETURN_CSTRING(TextDatumGetCString(txt));
}

/* End - varchar(char) */



/* Begin - varchar(byte) */

/*
 * varchar_input -- common guts of oravarcharbytein and oravarcharbyterecv
 *
 * "atttypmod" is measured in bytes
 * 
 * Remove the Verify of extra spaces characters
 * 
 */
static VarChar *
varchar_input_byte(const char *s, size_t len, int32 atttypmod)
{
	VarChar    *result;
	size_t		maxlen;
	
	maxlen = atttypmod - VARHDRSZ;

	if (atttypmod >= (int32) VARHDRSZ && len > maxlen)
	{
		ereport(ERROR,
			(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
			  errmsg("value too long for type varchar2(%d byte)",
					 (int) maxlen)));
	}

	result = (VarChar *) palloc(len + VARHDRSZ);
	SET_VARSIZE(result, len + VARHDRSZ);
	memcpy(VARDATA(result), s, len);
	return result;
}

/*
 * Convert a C string to VARCHAR2 internal representation.  
 * atttypmod is the declared length of the type plus VARHDRSZ.
 */
Datum
oravarcharbytein_2(PG_FUNCTION_ARGS)
{
	char	   *s = PG_GETARG_CSTRING(0);

#ifdef NOT_USED
	Oid			typelem = PG_GETARG_OID(1);
#endif
	int32		atttypmod = PG_GETARG_INT32(2);
	uint32		lens = PG_GETARG_UINT64(3);
	VarChar    *result;

	/* result = varchar_input(s, strlen(s), atttypmod); */
	result = varchar_input_byte(s, lens, atttypmod);
	PG_RETURN_VARCHAR_P(result);
}

/*
 * Convert a VARCHAR2 value to a C string.
 *
 * Uses the text to C string conversion function, which is only appropriate
 * if VarChar and text are equivalent types.
 */
Datum
oravarcharbyteout_2(PG_FUNCTION_ARGS)
{
	Datum		txt = PG_GETARG_DATUM(0);

	PG_RETURN_CSTRING(TextDatumGetCString(txt));
}

/* End - varchar(byte) */

