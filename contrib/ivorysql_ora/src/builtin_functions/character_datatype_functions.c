/*-------------------------------------------------------------------------------
 * 
 * File: character_datatype_functions.c
 *
 * Abstract: 
 * 		This file contains the implementation of Oracle's
 *		character data type related built-in functions.
 *
 * Authored by zhenmingyang@highgo.com, 20220510.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_ora/src/builtin_functions/character_datatype_functions.c
 *
 *-------------------------------------------------------------------------------
 */

#include "postgres.h"
#include "fmgr.h"

#include "access/detoast.h"
#include "catalog/namespace.h"
#include "catalog/pg_collation.h"
#include "lib/stringinfo.h"
#include "mb/pg_wchar.h"
#include "miscadmin.h"
#include "regex/regex.h"    /* SRS-CKC-Oracle-SupportOra_Regexp_245 */
#include "utils/builtins.h"
#include "utils/numeric.h"
#include "utils/varlena.h"

#include "../include/common_datatypes.h"

PG_FUNCTION_INFO_V1(oracharlen);
PG_FUNCTION_INFO_V1(oracharoctetlen);
PG_FUNCTION_INFO_V1(oravarcharlen);
PG_FUNCTION_INFO_V1(oravarcharoctetlen);
/* Begin - Bug#796 */
PG_FUNCTION_INFO_V1(rtrim1);
PG_FUNCTION_INFO_V1(rtrim2);
PG_FUNCTION_INFO_V1(ltrim1);
PG_FUNCTION_INFO_V1(ltrim2);
/* End - Bug#796 */
/* Begin - BUG#828 */
PG_FUNCTION_INFO_V1(trim1);
PG_FUNCTION_INFO_V1(trim2);
/* End - BUG#828 */
/* Begin - SRS-CKC-Oracle-SupportOra_Regexp_245 */
PG_FUNCTION_INFO_V1(ora_regexp_replace);
PG_FUNCTION_INFO_V1(ora_regexp_substr);
PG_FUNCTION_INFO_V1(ora_regexp_instr);
PG_FUNCTION_INFO_V1(ora_regexp_like);
PG_FUNCTION_INFO_V1(ora_regexp_like_no_flags);
PG_FUNCTION_INFO_V1(ora_regexp_count);	/* BUG:0000891 */
/* Begin - SRS-CKC-Oracle-SupportOra_Regexp_245 */
PG_FUNCTION_INFO_V1(ora_substrb_no_length);
PG_FUNCTION_INFO_V1(ora_substrb);
PG_FUNCTION_INFO_V1(ora_replace);
PG_FUNCTION_INFO_V1(ora_instrb);
/* Begin - BUG:M0000084 */
PG_FUNCTION_INFO_V1(oracle_substr2);
PG_FUNCTION_INFO_V1(oracle_substr3);
PG_FUNCTION_INFO_V1(oracle_instr2);
PG_FUNCTION_INFO_V1(oracle_instr3);
PG_FUNCTION_INFO_V1(oracle_instr4);
/* End - BUG:M0000084 */
/* Begin - BUG#M0000194 */
PG_FUNCTION_INFO_V1(orafce_to_multi_byte);
PG_FUNCTION_INFO_V1(orafce_to_single_byte);
PG_FUNCTION_INFO_V1(ora_asciistr);
PG_FUNCTION_INFO_V1(ora_compose);
PG_FUNCTION_INFO_V1(ora_decompose);
PG_FUNCTION_INFO_V1(ora_encname_mapping);
PG_FUNCTION_INFO_V1(ora_convert);
/* End - BUG#M0000194 */

/* Begin - BUGID:0000467 */
#define PG_STR_GET_TEXT(str_) \
	DatumGetTextP(DirectFunctionCall1(textin, CStringGetDatum(str_)))
/* End - BUGID:0000467 */

typedef struct
{
	bool		use_wchar;		/* T if multibyte encoding */
	char	   *str1;			/* use these if not use_wchar */
	char	   *str2;			/* note: these point to original texts */
	pg_wchar   *wstr1;			/* use these if use_wchar */
	pg_wchar   *wstr2;			/* note: these are palloc'd */
	int			len1;			/* string lengths in logical characters */
	int			len2;
	/* Skip table for Boyer-Moore-Horspool search algorithm: */
	int			skiptablemask;	/* mask for ANDing with skiptable subscripts */
	int			skiptable[256]; /* skip distance for given mismatched char */
	Oid			coll;			/* BUG#Z1464 */
} TextPositionState;

static text *text_substring_byte(Datum str, int32 start, int32 length, bool length_not_specified);
static void text_position_setup(text *t1, text *t2, TextPositionState *state, bool isbyte, Oid coll);
static void text_position_cleanup(TextPositionState *state);
static int	text_position_next(int start_pos, TextPositionState *state);
static int text_instring(text *str, text *search_str, int32 position, int32 occurrence, bool isByte, Oid coll);
static int	text_position_prev(int start_pos, TextPositionState *state);
static void appendStringInfoText(StringInfo str, const text *t);
/* Begin - BUG:M0000084 */
static text *ora_substr(Datum str, int start, int len);
static int ora_instr(text *txt, text *pattern, int start, int nth, Oid coll /* BUG#Z1463 */);
/* End - BUG:M0000084 */

/* Begin - bug#00008827 */
static void ora_check_regex_options(text *flags, bool check_g);
/* End - bug#00008827 */


/* Begin - OA-REQ-2451 */
static int32
ora_number_arg_to_int32(Datum arg)
{
	return DatumGetInt32(DirectFunctionCall1(numeric_int4,
						 DirectFunctionCall2(numeric_trunc, arg,
											Int32GetDatum(0))));
}
/* End - OA-REQ-2451 */

/*
 * length/lengthb function for 'oracharchar' and 'oracharbyte' type.
 */
Datum
oracharlen(PG_FUNCTION_ARGS)
{
	BpChar	   *arg = PG_GETARG_BPCHAR_PP(0);
	int			len;

	len = VARSIZE_ANY_EXHDR(arg);
	
	/* in multibyte encoding, convert to number of characters */
	if (pg_database_encoding_max_length() != 1)
		len = pg_mbstrlen_with_len(VARDATA_ANY(arg), len);

	PG_RETURN_INT32(len);
}

Datum
oracharoctetlen(PG_FUNCTION_ARGS)
{
	Datum		arg = PG_GETARG_DATUM(0);

	/* We need not detoast the input at all */
	PG_RETURN_INT32(toast_raw_datum_size(arg) - VARHDRSZ);
}

/*
 * length/lengthb function for 'oravarcharchar' and 'oravarcharbyte' type.
 */
Datum
oravarcharlen(PG_FUNCTION_ARGS)
{
	Datum	   str = PG_GETARG_DATUM(0);
	
	/* fastpath when max encoding length is one */
	if (pg_database_encoding_max_length() == 1)
		PG_RETURN_INT32(toast_raw_datum_size(str) - VARHDRSZ);
	else
	{
		VarChar	   *t = DatumGetVarCharPP(str);

		PG_RETURN_INT32(pg_mbstrlen_with_len(VARDATA_ANY(t),
											 VARSIZE_ANY_EXHDR(t)));
	}
}

Datum
oravarcharoctetlen(PG_FUNCTION_ARGS)
{
	Datum		str = PG_GETARG_DATUM(0);

	/* We need not detoast the input at all */
	PG_RETURN_INT32(toast_raw_datum_size(str) - VARHDRSZ);
}

/* Begin - Bug#796 */
Datum
rtrim1(PG_FUNCTION_ARGS)
{
	VarChar	*string = PG_GETARG_VARCHAR_P(0);
	VarChar *ret;

	/**
	 * Begin - BUG#Z1467
	 * If we wanna convert char(n) to text, we will call text() function,
	 * whose oid is 401 and collation is 0.
	 * However, we will through error when collation is 0 in varstr_cmp(),
	 * so we use C_COLLATION_OID as collation to ensure logic is same as before this bug.
	 */
	Oid 		coll = PseudoCollationReplace_BNB(PG_GET_COLLATION());

	if (!OidIsValid(coll))
		coll = C_COLLATION_OID;
	/* End - BUG#Z1467 */

	ret = ora_dotrim(VARDATA_ANY(string), VARSIZE_ANY_EXHDR(string),
				 	" ", 1,
				 	false, true,
					coll /* BUG#Z1465 */);

	/* Begin - BUG#6021 */
	if (ret == NULL)
		PG_RETURN_NULL();
	else
		PG_RETURN_VARCHAR_P(ret);
	/* End - BUG#6021 */
}

Datum
rtrim2(PG_FUNCTION_ARGS)
{
	VarChar	*string = PG_GETARG_VARCHAR_P(0);
	VarChar	*set = PG_GETARG_VARCHAR_P(1);
	VarChar *ret;

	/**
	 * Begin - BUG#Z1467
	 * If we wanna convert char(n) to text, we will call text() function,
	 * whose oid is 401 and collation is 0.
	 * However, we will through error when collation is 0 in varstr_cmp(),
	 * so we use C_COLLATION_OID as collation to ensure logic is same as before this bug.
	 */
	Oid 		coll = PseudoCollationReplace_BNB(PG_GET_COLLATION());

	if (!OidIsValid(coll))
		coll = C_COLLATION_OID;
	/* End - BUG#Z1467 */
	
	ret = ora_dotrim(VARDATA_ANY(string), VARSIZE_ANY_EXHDR(string),
				 	VARDATA_ANY(set), VARSIZE_ANY_EXHDR(set),
				 	false, true,
					coll /* BUG#Z1465 */);

	/* Begin - BUG#6021 */
	if (ret == NULL)
		PG_RETURN_NULL();
	else
		PG_RETURN_VARCHAR_P(ret);
	/* End - BUG#6021 */
}

Datum
ltrim2(PG_FUNCTION_ARGS)
{
	VarChar	   *string = PG_GETARG_VARCHAR_P(0);
	VarChar	   *set = PG_GETARG_VARCHAR_P(1);
	VarChar	   *ret;

	/**
	 * Begin - BUG#Z1467
	 * If we wanna convert char(n) to text, we will call text() function,
	 * whose oid is 401 and collation is 0.
	 * However, we will through error when collation is 0 in varstr_cmp(),
	 * so we use C_COLLATION_OID as collation to ensure logic is same as before this bug.
	 */
	Oid 		coll = PseudoCollationReplace_BNB(PG_GET_COLLATION());

	if (!OidIsValid(coll))
		coll = C_COLLATION_OID;
	/* End - BUG#Z1467 */
	
	ret = ora_dotrim(VARDATA_ANY(string), VARSIZE_ANY_EXHDR(string),
				 	VARDATA_ANY(set), VARSIZE_ANY_EXHDR(set),
				 	true, false,
					coll /* BUG#Z1465 */);

	/* Begin - BUG#6021 */
	if (ret == NULL)
		PG_RETURN_NULL();
	else
		PG_RETURN_VARCHAR_P(ret);
	/* End - BUG#6021 */
}

Datum
ltrim1(PG_FUNCTION_ARGS)
{
	VarChar	   *string = PG_GETARG_VARCHAR_P(0);
	VarChar	   *ret;

	/**
	 * Begin - BUG#Z1467
	 * If we wanna convert char(n) to text, we will call text() function,
	 * whose oid is 401 and collation is 0.
	 * However, we will through error when collation is 0 in varstr_cmp(),
	 * so we use C_COLLATION_OID as collation to ensure logic is same as before this bug.
	 */
	Oid 		coll = PseudoCollationReplace_BNB(PG_GET_COLLATION());

	if (!OidIsValid(coll))
		coll = C_COLLATION_OID;
	/* End - BUG#Z1467 */
	
	ret = ora_dotrim(VARDATA_ANY(string), VARSIZE_ANY_EXHDR(string),
				 	" ", 1,
				 	true, false,
					coll /* BUG#Z1465 */);

	/* Begin - BUG#6021 */
	if (ret == NULL)
		PG_RETURN_NULL();
	else
		PG_RETURN_VARCHAR_P(ret);
	/* End - BUG#6021 */
}
/* End - Bug#796 */

/* Begin - BUG#833 */
Datum
trim2(PG_FUNCTION_ARGS)
{
	VarChar	   *string = PG_GETARG_VARCHAR_P(0);
	VarChar	   *set = PG_GETARG_VARCHAR_P(1);
	VarChar	   *ret;

	/**
	 * Begin - BUG#Z1467
	 * If we wanna convert char(n) to text, we will call text() function,
	 * whose oid is 401 and collation is 0.
	 * However, we will through error when collation is 0 in varstr_cmp(),
	 * so we use C_COLLATION_OID as collation to ensure logic is same as before this bug.
	 */
	Oid 		coll = PseudoCollationReplace_BNB(PG_GET_COLLATION());

	if (!OidIsValid(coll))
		coll = C_COLLATION_OID;
	/* End - BUG#Z1467 */
	
	ret = ora_dotrim(VARDATA_ANY(string), VARSIZE_ANY_EXHDR(string),
				 	VARDATA_ANY(set), VARSIZE_ANY_EXHDR(set),
				 	true, true,
					coll /* BUG#Z1465 */);

	/* Begin - BUG#6021 */
	if (ret == NULL)
		PG_RETURN_NULL();
	else
		PG_RETURN_VARCHAR_P(ret);
	/* End - BUG#6021 */
}

Datum
trim1(PG_FUNCTION_ARGS)
{
	VarChar	   *string = PG_GETARG_VARCHAR_P(0);
	VarChar	   *ret;

	/**
	 * Begin - BUG#Z1467
	 * If we wanna convert char(n) to text, we will call text() function,
	 * whose oid is 401 and collation is 0.
	 * However, we will through error when collation is 0 in varstr_cmp(),
	 * so we use C_COLLATION_OID as collation to ensure logic is same as before this bug.
	 */
	Oid 		coll = PseudoCollationReplace_BNB(PG_GET_COLLATION());

	if (!OidIsValid(coll))
		coll = C_COLLATION_OID;
	/* End - BUG#Z1467 */
	
	ret = ora_dotrim(VARDATA_ANY(string), VARSIZE_ANY_EXHDR(string),
				 	" ", 1,
				 	true, true,
					coll /* BUG#Z1465 */);

	/* Begin - BUG#6021 */
	if (ret == NULL)
		PG_RETURN_NULL();
	else
		PG_RETURN_VARCHAR_P(ret);
	/* End - BUG#6021 */
}
/* End - BUG#833 */

/* Begin - SRS-CKC-Oracle-SupportOra_Regexp_245 */
#define MATCHPARAM(ch) (ch == 'i' || ch == 'c' || ch == 'n' || ch == 'm' || ch == 'x')

/*
 * regexp_replace
 * letting you search a string for a regular expression pattern
 * By default, the function returns source_char with every occurrence of the regular 
 * expression pattern replaced with replace_string. 
 * The string returned is in the same character set as source_char. 
 */
Datum
ora_regexp_replace(PG_FUNCTION_ARGS)
{
	text	   *src_text = NULL;       /* source_char */
	text	   *pattern_arg = NULL;    /* pattern */
	text	   *replace_str = NULL;    /* replace_string */ 
	int         start_posn = 1;        /* position */
	int  	    occur_posn = 0;        /* occurrence */
	text       *match_para = NULL;     /* match_parameter */
	text       *tem = NULL;
	regex_t    *re;
	pg_re_flags flags; /* bug#00008827 */
	
	/*
 	 *When there is only one parameter, error processing is performed 	 
 	 */	
	if (PG_NARGS() == 1)
		ereport(ERROR,
			(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			 errmsg("not enough arguments for function")));
	else if (PG_NARGS() > 1 && !PG_ARGISNULL(0))
		src_text = PG_GETARG_TEXT_PP(0);

	/*
 	 *Returns null if the argument is greater than or 
	 *equal to 2 and the first argument(Source string) is empty 	
 	 */
	if (PG_NARGS() >= 2)
	{
		if(PG_ARGISNULL(0))
		 	PG_RETURN_NULL();

		/*
	 	 * Return source string when the pattern_arg is empty
	 	 */
		if(PG_ARGISNULL(1))
			PG_RETURN_TEXT_P(PG_GETARG_TEXT_PP(0));
		else
	 		pattern_arg = PG_GETARG_TEXT_PP(1);	
	}
	
	if (PG_NARGS() >= 3 && !PG_ARGISNULL(2))
		replace_str = PG_GETARG_TEXT_PP(2);

	if (PG_NARGS() >= 4 )
	{	
		if(!PG_ARGISNULL(3))
		{	
			start_posn = ora_number_arg_to_int32(PG_GETARG_DATUM(3));	/* OA-REQ-2451 */
			if(start_posn <= 0)
				ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("argument '%d' is out of range",start_posn)));
		}
		/*
		 *the fourth parameter  is empty
		 *return empty
	 	 */
		else
			PG_RETURN_NULL();
	}
	
	if (PG_NARGS() >= 5)
	{	
		if(!PG_ARGISNULL(4))
		{
			occur_posn = ora_number_arg_to_int32(PG_GETARG_DATUM(4));	/* OA-REQ-2451 */
			if(occur_posn < 0)
				ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("argument '%d' is out of range",occur_posn)));
		}	
		/*
		 *the fifth parameter  is empty
		 *return empty
	 	 */
		else
			PG_RETURN_NULL();
	}

	if (PG_NARGS() >= 6 && !PG_ARGISNULL(5))
	{
		match_para = PG_GETARG_TEXT_PP(5);
		/* Begin - bug#00008827 */
		ora_check_regex_options(match_para, false);
		/* End - bug#00008827 */
	}
	
	if (PG_NARGS() > 6 )
		ereport(ERROR,
			(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			 errmsg("too many arguments for function")));		
	
	ora_parse_re_flags(&flags, match_para);
	re = ora_re_compile_and_cache(pattern_arg, flags.cflags, PG_GET_COLLATION());
	tem = ora_replace_text_regexp(src_text, re, replace_str, start_posn, occur_posn);
	/* Begin - BUG#6021 */
	if (tem == NULL)
		PG_RETURN_NULL();
	else
		PG_RETURN_TEXT_P(tem);
	/* End - BUG#6021 */
}

/*
 * REGEXP_SUBSTR extends the functionality of the SUBSTR function by letting you search a string 
 * for a regular expression pattern. It is also similar to REGEXP_INSTR, but instead of returning 
 * the position of the substring, it returns the substring itself. This function is useful if you
 * need the contents of a match string but not its position in the source string.
 * The function returns the string as VARCHAR2 or CLOB data in the same character 
 * set as source_char. 
 */
Datum
ora_regexp_substr(PG_FUNCTION_ARGS)
{
	text	   *src_text = NULL;       /* source_char */
	text	   *pattern_arg = NULL;    /* pattern */
	int         start_posn = 1;        /* position */
	int  	    occur_posn = 1;        /* occurrence */
	text       *match_para = NULL;     /* match_parameter */
	int	    	subexpr_pos = 0;	   /* subexpr for pattern */
	bool	   find_subpattern = false;
	Datum      substr;

	/*
 	 *When there is only one parameter, error processing is performed 	 
 	 */	
	if (PG_NARGS() == 1)
		ereport(ERROR,
			(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			 errmsg("not enough arguments for function")));
	else if (PG_NARGS() > 1 && !PG_ARGISNULL(0))
		src_text = PG_GETARG_TEXT_PP(0);

	/*
 	 *Returns null if the argument is greater than or 
	 *equal to 2 and the first argument(Source string) is empty 	
 	 */
	if (PG_NARGS() >= 2)
	{
		if(PG_ARGISNULL(0) || PG_ARGISNULL(1))
		 	PG_RETURN_NULL();
		else
	 		pattern_arg = PG_GETARG_TEXT_PP(1);	
	}

	if (PG_NARGS() >= 3)
	{	
		if (!PG_ARGISNULL(2))
		{
			/* Begin - OA-REQ-2348 */
			Numeric	floor_num = DatumGetNumeric(DirectFunctionCall1(numeric_floor,
												 NumericGetDatum(PG_GETARG_NUMERIC(2))));
			start_posn = numeric_int4_opt_error(floor_num, NULL);

			if (start_posn <= 0)
				ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("argument '%d' is out of range", start_posn)));
			/* End - OA-REQ-2348 */
		}
		/*
		 *the third parameter  is empty
		 *return empty
	 	 */
		else
			PG_RETURN_NULL();
	}
	
	if (PG_NARGS() >= 4)
	{	
		if (!PG_ARGISNULL(3))
		{
			/* Begin - OA-REQ-2348 */
			Numeric	floor_num = DatumGetNumeric(DirectFunctionCall1(numeric_floor,
												 NumericGetDatum(PG_GETARG_NUMERIC(3))));
			occur_posn = numeric_int4_opt_error(floor_num, NULL);

			if (occur_posn <= 0)
				ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("argument '%d' is out of range", occur_posn)));
			/* End - OA-REQ-2348 */
		}	
		/*
		 *the forth parameter  is empty
		 *return empty
	 	 */
		else
			PG_RETURN_NULL();
	}

	if (PG_NARGS() >= 5 && !PG_ARGISNULL(4))
	{
		match_para = PG_GETARG_TEXT_PP(4);
		/* Begin - bug#00008827 */
		ora_check_regex_options(match_para, false);
		/* End - bug#00008827 */
	}

	if (PG_NARGS() >= 6)
	{
		if (!PG_ARGISNULL(5))
		{
			/* Begin - OA-REQ-2348 */
			Numeric	floor_num = DatumGetNumeric(DirectFunctionCall1(numeric_floor,
												 NumericGetDatum(PG_GETARG_NUMERIC(5))));
			subexpr_pos = numeric_int4_opt_error(floor_num, NULL);

			if (subexpr_pos < 0)
				ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("argument '%d' is out of range", subexpr_pos)));
			/* End - OA-REQ-2348 */
			else if (subexpr_pos > 9)
				PG_RETURN_NULL();			
		}
		else
			PG_RETURN_NULL();			
	}
	
	find_subpattern = ora_substr_text_regexp(src_text, pattern_arg,
						match_para,PG_GET_COLLATION(),
						start_posn, occur_posn,
						subexpr_pos,&substr );
	if (find_subpattern == true)
		return substr;
	else
		PG_RETURN_NULL();
}

Datum
ora_regexp_instr(PG_FUNCTION_ARGS)
{
	text	*src_text = NULL;       /* source_char */
	text	*pattern_arg = NULL;    /* pattern */
	int		 start_posn = 1;        /* position */
	int		 occur_posn = 1;        /* occurrence */
	int	     ret_opt = 0;           /* return_opt*/
	text	*match_para = NULL;     /* match_parameter */
	int	     subexpr_pos = 0;	    /* subexpr for pattern */
	bool	 find_subpattern = false;
	Datum	 substr;

	/*
	 * When there is only one parameter, error processing is performed
	 */
	if (PG_NARGS() == 1)
		ereport(ERROR,
			(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			 errmsg("not enough arguments for function")));
	else if (PG_NARGS() > 1 && !PG_ARGISNULL(0))
		src_text = PG_GETARG_TEXT_PP(0);

	/*
	 * Returns null if the argument is greater than or
	 * equal to 2 and the first argument(Source string) is empty
	 */
	if (PG_NARGS() >= 2)
	{
		if(PG_ARGISNULL(0) || PG_ARGISNULL(1))
			PG_RETURN_NULL();
		else
			pattern_arg = PG_GETARG_TEXT_PP(1);
	}

	if (PG_NARGS() >= 3)
	{
		if(!PG_ARGISNULL(2))
		{
			start_posn = ora_number_arg_to_int32(PG_GETARG_DATUM(2));	/* OA-REQ-2451 */
			if(start_posn <= 0)
				ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("argument '%d' is out of range",start_posn)));
		}
		else
			PG_RETURN_NULL(); /* if the third parameter is empty, return empty */
	}

	if (PG_NARGS() >= 4)
	{
		if(!PG_ARGISNULL(3))
		{
			occur_posn = ora_number_arg_to_int32(PG_GETARG_DATUM(3));	/* OA-REQ-2451 */
			if(occur_posn <= 0)
				ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("argument '%d' is out of range",occur_posn)));
		}
		else
			PG_RETURN_NULL(); /* if the forth parameter is empty, return empty */
	}

	if (PG_NARGS() >= 5)
	{
		if(!PG_ARGISNULL(4))
		{
			ret_opt = ora_number_arg_to_int32(PG_GETARG_DATUM(4));	/* OA-REQ-2451 */
			if(ret_opt < 0)
				ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("argument '%d' is out of range",ret_opt)));
		}
		else
			PG_RETURN_NULL(); /* if the fifth parameter is empty, return empty */
	}

	if (PG_NARGS() >= 6 && !PG_ARGISNULL(5))
	{
		match_para = PG_GETARG_TEXT_PP(5);
		/* Begin - bug#00008827 */
		ora_check_regex_options(match_para, false);
		/* End - bug#00008827 */
	}

	if (PG_NARGS() >= 7)
	{
		if (!PG_ARGISNULL(6))
		{
			subexpr_pos = ora_number_arg_to_int32(PG_GETARG_DATUM(6));	/* OA-REQ-2451 */
			if(subexpr_pos < 0)
				ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("argument '%d' is out of range", subexpr_pos)));
			else if (subexpr_pos >= 10)
				return PointerGetDatum(cstring_to_text("0"));
		}
		else
			PG_RETURN_NULL();
	}

	find_subpattern = ora_instr_text_regexp(src_text, pattern_arg,
						match_para,PG_GET_COLLATION(),
						start_posn, occur_posn,ret_opt,
						subexpr_pos,&substr);
	if(find_subpattern == true)
		return substr;
	else
		return PointerGetDatum(cstring_to_text("0"));
}

/*
 * Oracle-compatible REGEXP_LIKE implementation
 *
 * if source string matched by a regular expression return true or return false;
 *
 * Supports the following flags:
 *
 *	'i' - case-insensitive matching (REG_ICASE)
 *	'c' - case-sensitive matching (clears REG_ICASE)
 *	'n' - dot matches newline (REG_DOTALL in PostgreSQL, opposite of REG_NEWLINE!)
 *	'm' - treat string as multiple lines (REG_NEWLINE for ^/$ matching)
 *	'x' - ignore whitespace in pattern (REG_EXPANDED)
 */
Datum
ora_regexp_like(PG_FUNCTION_ARGS)
{
	text		*s = PG_ARGISNULL(0) ? NULL :PG_GETARG_TEXT_P(0);
	text		*p = PG_ARGISNULL(1) ? NULL :PG_GETARG_TEXT_P(1);
	text		*in_flag = PG_ARGISNULL(2) ? PG_STR_GET_TEXT("") : PG_GETARG_TEXT_P(2);
	bool		match;
	regmatch_t	pmatch[2];
	pg_re_flags flags; /* bug#00008827 */

	if ((NULL == s) || (NULL == p))
	{
		PG_RETURN_BOOL(false);
	}


	/* Begin - bug#00008827 */
	ora_check_regex_options(in_flag, false);
	ora_parse_re_flags(&flags, in_flag);
	/* End - bug#00008827 */

	/*
	 * We pass two regmatch_t structs to get info about the overall match and
	 * the match for the first parenthesized subexpression (if any). If there
	 * is a parenthesized subexpression, we return what it matched; else
	 * return what the whole regexp matched.
	 */
	match = RE_compile_and_execute(p,
								   VARDATA(s),
								   VARSIZE(s) - VARHDRSZ,
								   flags.cflags, /* bug#00008827 */
								   PG_GET_COLLATION(),
								   2,
								   pmatch);

	/* match? then return the substring matching the pattern */
	if (match)
		PG_RETURN_BOOL(true);
	else
		PG_RETURN_BOOL(false);
}

/*
 * ora_regexp_like_no_flag()
 * if source string matched by a regular expression return true or return false;
 * the flag defalut is PATTERN_FLAG;
 */
Datum
ora_regexp_like_no_flags(PG_FUNCTION_ARGS)
{
	text		*s = PG_ARGISNULL(0) ? NULL :PG_GETARG_TEXT_P(0);
	text		*p = PG_ARGISNULL(1) ? NULL :PG_GETARG_TEXT_P(1);
	int			out_flag;
	bool		match;
	regmatch_t	pmatch[2];

	if ((NULL == s) || (NULL == p) )
	{
		PG_RETURN_BOOL(false);
	}

	/*
	 * Oracle default (NO FLAGS):
	 *	- '.' does NOT match newline  (no dotall)
	 *	- '^' and '$' match ONLY start/end of entire string (no multiline)
	 *
	 * PostgreSQL equivalent:
	 *	- DO NOT set REG_NEWLINE
	 */
	out_flag = REG_ADVANCED;
	out_flag |= REG_NLSTOP;

	/*
	 * We pass two regmatch_t structs to get info about the overall match and
	 * the match for the first parenthesized subexpression (if any). If there
	 * is a parenthesized subexpression, we return what it matched; else
	 * return what the whole regexp matched.
	 */
	match = RE_compile_and_execute(p,
								   VARDATA(s),
								   VARSIZE(s) - VARHDRSZ,
								   out_flag,
								   PG_GET_COLLATION(),
								   2,
								   pmatch);

	/* match? then return the substring matching the pattern */
	if (match)
		PG_RETURN_BOOL(true);
	else
		PG_RETURN_BOOL(false);
}
/* End - SRS-CKC-Oracle-SupportOra_Regexp_245 */

/* Begin - BUG:0000891 */
/********************************************************************
 * regexp_count
 *
 * Purpose:
 *	 Returning the number of times a pattern occurs 
 *	 in a source string.
 ********************************************************************/
Datum
ora_regexp_count(PG_FUNCTION_ARGS)
{
	text	   *s = NULL;
	text	   *p = NULL;
	int32		position = 0;
	text	   *match_param = NULL;
	regex_t    *re;
	regmatch_t	pmatch[REGEXP_REPLACE_BACKREF_CNT]; /* bug#00008827 */
	int			src_text_len;
	int			search_start;
	int			occurrence_cnt = 0;
	pg_wchar   *data;
	size_t		data_len;
	pg_re_flags flags;

	if (PG_ARGISNULL(2))
		PG_RETURN_NULL();

	/* get the value of the parameters safely */
	position = ora_number_arg_to_int32(PG_GETARG_DATUM(2));	/* OA-REQ-2451 */

	/* position and occurrence must be positive integer */
	if (position <= 0)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("argument \"%d\" is out of range", position)));

	/* End - ReqID:SRS-SQL-SYSFUNC */
	if (PG_ARGISNULL(3))
	{
		flags.cflags = REG_ADVANCED;
		flags.glob = false;	
	}
	else
	{
		match_param = PG_GETARG_TEXT_PP(3);
		/* Begin - bug#00008827 */
		/*
		 * 'true' is passwd here due to historical reasons;
		 * the function's default value was set to 'g'
		 */
		ora_check_regex_options(match_param, true);
		/* End - bug#00008827 */
		
		ora_parse_re_flags(&flags, match_param);
	}

	if (PG_ARGISNULL(0) ||
		PG_ARGISNULL(1))
		PG_RETURN_NULL();

	s = PG_GETARG_TEXT_PP(0);
	p = PG_GETARG_TEXT_PP(1);
	src_text_len = VARSIZE_ANY_EXHDR(s);

	/* Compile RE */
	re = RE_compile_and_cache(p, flags.cflags, PG_GET_COLLATION());

	/* Convert data string to wide characters. */
	data = (pg_wchar *) palloc((src_text_len + 1) * sizeof(pg_wchar));
	data_len = pg_mb2wchar_with_len(VARDATA_ANY(s), data, src_text_len);

	/* Begin - bug#00008827 */
	if (position > data_len)
	{
		pfree(data);
		PG_RETURN_INT32(occurrence_cnt);
	}
	/* End - bug#00008827 */

	search_start = position - 1;

	while (search_start <= data_len)
	{
		int 	regexec_result;

		CHECK_FOR_INTERRUPTS();

		regexec_result = pg_regexec(re,
									data,
									data_len,
									search_start,
									NULL,
									REGEXP_REPLACE_BACKREF_CNT,
									pmatch,
									0);

		if (regexec_result == REG_NOMATCH)
			break;

		if (regexec_result != REG_OKAY)
		{
			char	errMsg[100];

			CHECK_FOR_INTERRUPTS();
			pg_regerror(regexec_result, re, errMsg, sizeof(errMsg));
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_REGULAR_EXPRESSION),
					 errmsg("regular expression failed: %s", errMsg)));
		}

		occurrence_cnt++;

		/* Advance by length or 1 if zero-length match */
		search_start = pmatch[0].rm_eo;
		if (pmatch[0].rm_so == pmatch[0].rm_eo)
		{
			search_start++;

			/* Begin - bug#00008827 */
			/*
			 * In this case, the last character of the search string
			 * is a newline, which does not match eighter '^' or '$' or a.*
			 * the final match should be excluded
			 */
			if (pmatch[0].rm_eo == data_len &&
				data[data_len - 1] == '\n' &&
				occurrence_cnt > 1)
				occurrence_cnt--;
			/* End - bug#00008827 */
		}
	}

	
	pfree(data);

	PG_RETURN_INT32(occurrence_cnt);
}
/* End - BUG:0000891 */

/********************************************************************
 * ReqID:SRS-GKC-DATATYPE_001
 * ora_substrb
 *
 * Purpose:
 *	 Return a substring starting at the specified position in bytes.
 *
 ********************************************************************/
Datum
ora_substrb(PG_FUNCTION_ARGS)
{
	text	*res;
	int32	len;
	int32 	start = DatumGetInt32(DirectFunctionCall1(numeric_int4, PG_GETARG_DATUM(1)));
	int32	length = DatumGetInt32(DirectFunctionCall1(numeric_int4, PG_GETARG_DATUM(2)));

	res = text_substring_byte(PG_GETARG_DATUM(0), start, length, false);
	len = VARSIZE_ANY_EXHDR(res);
	
	if (len == 0)
	{
		pfree(res);
		PG_RETURN_NULL();
	}
	
	PG_RETURN_TEXT_P(res);
}

Datum
ora_substrb_no_length(PG_FUNCTION_ARGS)
{
	text	*res;
	int32	len;
	int32 	start = DatumGetInt32(DirectFunctionCall1(numeric_int4, PG_GETARG_DATUM(1)));

	res = text_substring_byte(PG_GETARG_DATUM(0), start, -1, true);
	len = VARSIZE_ANY_EXHDR(res);
	
	if (len == 0)
	{
		pfree(res);
		PG_RETURN_NULL();
	}

	PG_RETURN_TEXT_P(res);
}

/********************************************************************
 *
 * ora_replace
 *
 * from:
 *	 text replace_text(text src_text, 
 *					   text from_sub_text, 
 *					   text to_sub_text)
 *
 * Purpose:
 *	 Replace all occurrences of 'from_sub_text' in 'src_text' with 
 *	 'to_sub_text'.
 *
 * Modify:
 * 	1. Change the 'strict' property of the function from 'true' to 
 *	   'false'.
 *	2. Returns 'src_text' if 'from_sub_text' == NULL or 'src_text' == NULL.
 *	3. If 'to_sub_text' is omitted or null, then all occurrences of 
 *	   'from_sub_text' are removed.
 *
 ********************************************************************/
Datum
ora_replace(PG_FUNCTION_ARGS)
{
	text	   *src_text = NULL;
	text	   *from_sub_text = NULL;
	text	   *to_sub_text = NULL;
	int			src_text_len;
	int			from_sub_text_len;
	TextPositionState state;
	text	   *ret_text;
	int			start_posn;
	int			curr_posn;
	int			chunk_len;
	char	   *start_ptr;
	StringInfoData str;

	/*
	 * ReqID:SRS-GKC-DATATYPE_001
	 * Compatible with oracle, if arg1 or agr2 is null return 'src_text'.
	 * if arg3 is null, then all occurrences of arg2 are removed.
	 */
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	
	if (PG_ARGISNULL(1))
		PG_RETURN_TEXT_P(PG_GETARG_TEXT_PP(0));
	
	src_text = PG_GETARG_TEXT_PP(0);
	from_sub_text = PG_GETARG_TEXT_PP(1);

	if (!PG_ARGISNULL(2))
		to_sub_text = PG_GETARG_TEXT_PP(2);

	text_position_setup(src_text, from_sub_text, &state, false, 
						PseudoCollationReplace_BNB(PG_GET_COLLATION()) /* BUG#Z1464 */);

	/*
	 * Note: we check the converted string length, not the original, because
	 * they could be different if the input contained invalid encoding.
	 */
	src_text_len = state.len1;
	from_sub_text_len = state.len2;

	/* Return unmodified source string if empty source or pattern */
	if (src_text_len < 1 || from_sub_text_len < 1)
	{
		text_position_cleanup(&state);
		PG_RETURN_TEXT_P(src_text);
	}

	start_posn = 1;
	curr_posn = text_position_next(1, &state);

	/* When the from_sub_text is not found, there is nothing to do. */
	if (curr_posn == 0)
	{
		text_position_cleanup(&state);
		PG_RETURN_TEXT_P(src_text);
	}

	/* start_ptr points to the start_posn'th character of src_text */
	start_ptr = VARDATA_ANY(src_text);

	initStringInfo(&str);

	do
	{
		CHECK_FOR_INTERRUPTS();

		/* copy the data skipped over by last text_position_next() */
		chunk_len = charlen_to_bytelen(start_ptr, curr_posn - start_posn);
		appendBinaryStringInfo(&str, start_ptr, chunk_len);

		if (to_sub_text != NULL)	/* ReqID:SRS-GKC-DATATYPE_001 */
			appendStringInfoText(&str, to_sub_text);

		start_posn = curr_posn;
		start_ptr += chunk_len;
		start_posn += from_sub_text_len;
		start_ptr += charlen_to_bytelen(start_ptr, from_sub_text_len);

		curr_posn = text_position_next(start_posn, &state);
	}
	while (curr_posn > 0);

	/* copy trailing data */
	chunk_len = ((char *) src_text + VARSIZE_ANY(src_text)) - start_ptr;
	appendBinaryStringInfo(&str, start_ptr, chunk_len);

	text_position_cleanup(&state);

	ret_text = cstring_to_text_with_len(str.data, str.len);
	pfree(str.data);

	PG_RETURN_TEXT_P(ret_text);
}

/**********************************************************************
 *
 * ReqID:SRS-GKC-DATATYPE_001
 * ora_instrb --- calculates strings using bytes instead of characters
 *
 **********************************************************************/
Datum
ora_instrb(PG_FUNCTION_ARGS)
{
	text	*str = NULL;
	text	*search_str = NULL;
	/* Begin - ReqID:SRS-SQL-SYSFUNC:Change the way you get parameters. */
	int32	position = DatumGetInt32(DirectFunctionCall1(numeric_int4, DirectFunctionCall2(numeric_trunc, PG_GETARG_DATUM(2),Int32GetDatum(0))));
	int32	occurrence = DatumGetInt32(DirectFunctionCall1(numeric_int4, DirectFunctionCall2(numeric_trunc, PG_GETARG_DATUM(3),Int32GetDatum(0))));
	/* End - ReqID:SRS-SQL-SYSFUNC. */
	
	if (position == 0)
		PG_RETURN_INT32(0);
	
	if (occurrence <= 0)
		ereport(ERROR,
					(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
					 errmsg("The parameter of occurrence must be positive")));

	str = PG_GETARG_TEXT_PP(0);
	search_str = PG_GETARG_TEXT_PP(1);

	PG_RETURN_INT32(text_instring(str,
								  search_str,
								  position,
								  occurrence,
								  true,
								  PseudoCollationReplace_BNB(PG_GET_COLLATION()) /* BUG#Z1464 */));
}


/*
 * text_substring_byte - 
 *	Does the real work for ora_substrb().
 *
 *  The byte version of text_substring function.
 */
static text *
text_substring_byte(Datum str, int32 start, int32 length, bool length_not_specified)
{
	int32		eml = pg_database_encoding_max_length();
	int32		S = start;		/* start position */
	int32		S1;				/* adjusted start position */
	int32		L1;				/* adjusted substring length */

	/* Begin - ReqID:SRS-GKC-DATATYPE_001 */
	if (length_not_specified == false && length < 1)
		return cstring_to_text("");	
	/* End - ReqID:SRS-GKC-DATATYPE_001 */

	/* life is easy if the encoding max length is 1 */
	if (eml == 1)
	{	
		/* Begin - ReqID:SRS-GKC-DATATYPE_001 */
		if (S < 0)
		{
			int		blen;
			blen = (int) (toast_raw_datum_size(str) - VARHDRSZ);
			S1 = S + blen + 1;

			if (S1 < 1)
				return cstring_to_text("");
		}
		else
		{
			S1 = Max(S, 1);
		}
		/* End - ReqID:SRS-GKC-DATATYPE_001 */

		if (length_not_specified)		/* special case - get length to end of
										 * string */
			L1 = -1;
		else
			L1 = length;	/* ReqID:SRS-GKC-DATATYPE_001 */

		/*
		 * If the start position is past the end of the string, SQL99 says to
		 * return a zero-length string -- PG_GETARG_TEXT_P_SLICE() will do
		 * that for us. Convert to zero-based starting position
		 */
		return DatumGetTextPSlice(str, S1 - 1, L1);
	}
	else if (eml > 1)
	{
		/*
		 * When encoding max length is > 1, we can't get LC without
		 * detoasting, so we'll grab a conservatively large slice now and go
		 * back later to do the right thing
		 */
		int32	slice_start;
		int32	slice_size;
		int32	slice_strlen;
		text	*slice;
		int32	E1;
		char	*p;
		char	*sliceBegin;
		char	*subStrBegin;	/* real beginning position */
		char	*subStrEnd;		/* real ending position */
		bool	prefixSpace = false;	/* if attach a prefix space */
		bool	suffixSpace = false;	/* if attach a suffix space */
		int32	prefixSpaceCnt = 0;
		int32	suffixSpaceCnt = 0;
		text	*ret;

		/* Begin - ReqID:SRS-GKC-DATATYPE_001 */
		if (S < 0)
		{
			int		blen;
			blen = VARSIZE_ANY_EXHDR(DatumGetTextPP(str));
			S1 = blen + S + 1;

			if (S1 < 1)
				return cstring_to_text("");
		}
		else
		{
			S1 = Max(S, 1);
		}
		/* End - ReqID:SRS-GKC-DATATYPE_001 */

		/*
		 * We need to start at position zero because there is no way to know
		 * in advance which byte offset corresponds to the supplied start
		 * position.
		 */
		slice_start = 0;

		if (length_not_specified)		/* special case - get length to end of
										 * string */
			slice_size = L1 = -1;
		/* Begin - ReqID:SRS-GKC-DATATYPE_001 */
		else
		{
			L1 = length;
			slice_size = S1 + L1 + eml;
		}
		/* End - ReqID:SRS-GKC-DATATYPE_001 */

		/*
		 * If we're working with an untoasted source, no need to do an extra
		 * copying step.
		 */
		if (VARATT_IS_COMPRESSED(DatumGetPointer(str)) ||
			VARATT_IS_EXTERNAL(DatumGetPointer(str)))
			slice = DatumGetTextPSlice(str, slice_start, slice_size);
		else
			slice = (text *) DatumGetPointer(str);

		/* Now we can get the actual length of the slice in bytes */
		slice_strlen = VARSIZE_ANY_EXHDR(slice);

		/* see if we got back an empty string */
		if (slice_strlen == 0)
		{
			if (slice != (text *) DatumGetPointer(str))
				pfree(slice);
			return cstring_to_text("");
		}

		/*
		 * Check that the start position wasn't > slice_strlen. If so, SQL99
		 * says to return a zero-length string.
		 */
		if (S1 > slice_strlen)
		{
			if (slice != (text *) DatumGetPointer(str))
				pfree(slice);
			return cstring_to_text("");
		}

		/*
		 * Adjust L1 and E1 now that we know the slice string length. Again
		 * remember that S1 is one based, and slice_start is zero based.
		 */
		if (L1 > -1)
			E1 = Min(S1 + L1, slice_start + 1 + slice_strlen);
		else
			E1 = slice_start + 1 + slice_strlen;

		/* Begin - ReqID:SRS-GKC-DATATYPE_001 */
		sliceBegin = subStrBegin = VARDATA_ANY(slice);
		while (subStrBegin < sliceBegin + S1 - 1)
			subStrBegin += pg_mblen(subStrBegin);

		if (subStrBegin > sliceBegin + S1 - 1)
		{
			prefixSpace = true;
			prefixSpaceCnt = subStrBegin - (sliceBegin + S1 - 1);
		}
		
		subStrEnd = subStrBegin;
		while (subStrEnd + pg_mblen(subStrEnd) <= sliceBegin + E1 - 1)
			subStrEnd += pg_mblen(subStrEnd);

		if (subStrEnd < sliceBegin + E1 - 1)
		{
			suffixSpace = true;
			suffixSpaceCnt = sliceBegin + E1 - 1 - subStrEnd;
		}
		
		if (subStrEnd == subStrBegin)
		{
			if (prefixSpace && suffixSpace)
				return cstring_to_text("  ");
			else
				return cstring_to_text(" ");
		}

		ret = (text *) palloc(VARHDRSZ + (subStrEnd - subStrBegin + prefixSpaceCnt + suffixSpaceCnt));
		SET_VARSIZE(ret, VARHDRSZ + (subStrEnd - subStrBegin + prefixSpaceCnt + suffixSpaceCnt));
		
		p = VARDATA(ret);

		/* attach prefix spaces */
		if (prefixSpace)
		{
			while(prefixSpaceCnt--)
				*p++ = ' ';
		}
		
		/* copy the visible characters to slice result */
		memcpy(p, subStrBegin, (subStrEnd - subStrBegin));
		p += subStrEnd - subStrBegin;

		/* attach suffix spaces */
		if (suffixSpace)
		{
			while(suffixSpaceCnt--)
				*p++ = ' ';
		}
		/* End - ReqID:SRS-GKC-DATATYPE_001 */

		if (slice != (text *) DatumGetPointer(str))
			pfree(slice);

		return ret;
	}
	else
		elog(ERROR, "invalid backend encoding: encoding max length < 1");

	/* not reached: suppress compiler warning */
	return NULL;
}

/*
 * text_position_setup, text_position_next, text_position_cleanup -
 *	Component steps of text_position()
 *
 * These are broken out so that a string can be efficiently searched for
 * multiple occurrences of the same pattern.  text_position_next may be
 * called multiple times with increasing values of start_pos, which is
 * the 1-based character position to start the search from. The "state"
 * variable is normally just a local variable in the caller.
 *
 * Modify:
 *  Add the boolean type parameter isbyte, if isbyte is true then processed
 *	in single byte encoding.
 */
static void
text_position_setup(text *t1, text *t2, TextPositionState *state, bool isbyte, Oid coll)
{
	int			len1 = VARSIZE_ANY_EXHDR(t1);
	int			len2 = VARSIZE_ANY_EXHDR(t2);

	/* Begin - BUG#M0000207 */
	state->use_wchar = false;
	state->str1 = NULL;
	state->str2 = NULL;
	state->wstr1 = NULL;
	state->wstr2 = NULL;
	state->len1 = 0;
	state->len2 = 0;
	state->skiptablemask = 0;
	memset(state->skiptable, (int) 0, 255);
	/* End - BUG#M0000207 */

	/* Begin - BUG#Z1464 */
	state->coll = coll;
	/* End - BUG#Z1464 */

	if (pg_database_encoding_max_length() == 1 || 
		isbyte)	/* ReqID:SRS-GKC-DATATYPE_001 */
	{
		/* simple case - single byte encoding */
		state->use_wchar = false;
		state->str1 = VARDATA_ANY(t1);
		state->str2 = VARDATA_ANY(t2);
		state->len1 = len1;
		state->len2 = len2;
	}
	else
	{
		/* not as simple - multibyte encoding */
		pg_wchar   *p1,
				   *p2;

		p1 = (pg_wchar *) palloc((len1 + 1) * sizeof(pg_wchar));
		len1 = pg_mb2wchar_with_len(VARDATA_ANY(t1), p1, len1);
		p2 = (pg_wchar *) palloc((len2 + 1) * sizeof(pg_wchar));
		len2 = pg_mb2wchar_with_len(VARDATA_ANY(t2), p2, len2);

		state->use_wchar = true;
		state->wstr1 = p1;
		state->wstr2 = p2;
		state->len1 = len1;
		state->len2 = len2;
	}

	/*
	 * Prepare the skip table for Boyer-Moore-Horspool searching.  In these
	 * notes we use the terminology that the "haystack" is the string to be
	 * searched (t1) and the "needle" is the pattern being sought (t2).
	 *
	 * If the needle is empty or bigger than the haystack then there is no
	 * point in wasting cycles initializing the table.  We also choose not to
	 * use B-M-H for needles of length 1, since the skip table can't possibly
	 * save anything in that case.
	 */
	if (len1 >= len2 && len2 > 1)
	{
		int			searchlength = len1 - len2;
		int			skiptablemask;
		int			last;
		int			i;

		/*
		 * First we must determine how much of the skip table to use.  The
		 * declaration of TextPositionState allows up to 256 elements, but for
		 * short search problems we don't really want to have to initialize so
		 * many elements --- it would take too long in comparison to the
		 * actual search time.  So we choose a useful skip table size based on
		 * the haystack length minus the needle length.  The closer the needle
		 * length is to the haystack length the less useful skipping becomes.
		 *
		 * Note: since we use bit-masking to select table elements, the skip
		 * table size MUST be a power of 2, and so the mask must be 2^N-1.
		 */
		if (searchlength < 16)
			skiptablemask = 3;
		else if (searchlength < 64)
			skiptablemask = 7;
		else if (searchlength < 128)
			skiptablemask = 15;
		else if (searchlength < 512)
			skiptablemask = 31;
		else if (searchlength < 2048)
			skiptablemask = 63;
		else if (searchlength < 4096)
			skiptablemask = 127;
		else
			skiptablemask = 255;
		state->skiptablemask = skiptablemask;

		/*
		 * Initialize the skip table.  We set all elements to the needle
		 * length, since this is the correct skip distance for any character
		 * not found in the needle.
		 */
		for (i = 0; i <= skiptablemask; i++)
			state->skiptable[i] = len2;

		/*
		 * Now examine the needle.  For each character except the last one,
		 * set the corresponding table element to the appropriate skip
		 * distance.  Note that when two characters share the same skip table
		 * entry, the one later in the needle must determine the skip
		 * distance.
		 */
		last = len2 - 1;

		if (!state->use_wchar)
		{
			const char *str2 = state->str2;

			for (i = 0; i < last; i++)
				state->skiptable[(unsigned char) str2[i] & skiptablemask] = last - i;
		}
		else
		{
			const pg_wchar *wstr2 = state->wstr2;

			for (i = 0; i < last; i++)
				state->skiptable[wstr2[i] & skiptablemask] = last - i;
		}
	}
}

static void
text_position_cleanup(TextPositionState *state)
{
	if (state->use_wchar)
	{
		pfree(state->wstr1);
		pfree(state->wstr2);
	}
}

static int
text_position_next(int start_pos, TextPositionState *state)
{
	int			haystack_len = state->len1;
	int			needle_len = state->len2;
	int			skiptablemask = state->skiptablemask;
	Oid			coll = state->coll; /* BUG#Z1464 */

	Assert(start_pos > 0);		/* else caller error */

	if (needle_len <= 0)
		return start_pos;		/* result for empty pattern */

	start_pos--;				/* adjust for zero based arrays */

	/* Done if the needle can't possibly fit */
	if (haystack_len < start_pos + needle_len)
		return 0;

	if (!state->use_wchar)
	{
		/* simple case - single byte encoding */
		const char *haystack = state->str1;
		const char *needle = state->str2;
		const char *haystack_end = &haystack[haystack_len];
		const char *hptr;

		if (needle_len == 1)
		{
			/* No point in using B-M-H for a one-character needle */
			char		nchar = *needle;

			hptr = &haystack[start_pos];
			while (hptr < haystack_end)
			{
				if (varstr_cmp(hptr, 1, &nchar, 1, coll) == 0) /* BUG#Z1464 */
					return hptr - haystack + 1;
				hptr++;
			}
		}
		else
		{
			const char *needle_last = &needle[needle_len - 1];

			/* Start at startpos plus the length of the needle */
			hptr = &haystack[start_pos + needle_len - 1];
			while (hptr < haystack_end)
			{
				/* Match the needle scanning *backward* */
				const char *nptr;
				const char *p;

				nptr = needle_last;
				p = hptr;
				while (varstr_cmp(nptr, 1, p, 1, coll) == 0) /* BUG#Z1464 */
				{
					/* Matched it all?	If so, return 1-based position */
					if (nptr == needle)
						return p - haystack + 1;
					nptr--, p--;
				}

				/*
				 * No match, so use the haystack char at hptr to decide how
				 * far to advance.  If the needle had any occurrence of that
				 * character (or more precisely, one sharing the same
				 * skiptable entry) before its last character, then we advance
				 * far enough to align the last such needle character with
				 * that haystack position.  Otherwise we can advance by the
				 * whole needle length.
				 */
				hptr += state->skiptable[(unsigned char) *hptr & skiptablemask];
			}
		}
	}
	else
	{
		/* The multibyte char version. This works exactly the same way. */
		const pg_wchar *haystack = state->wstr1;
		const pg_wchar *needle = state->wstr2;
		const pg_wchar *haystack_end = &haystack[haystack_len];
		const pg_wchar *hptr;

		if (needle_len == 1)
		{
			/* No point in using B-M-H for a one-character needle */
			pg_wchar	nchar = *needle;

			hptr = &haystack[start_pos];
			while (hptr < haystack_end)
			{
				if (varstr_cmp((char *) hptr, sizeof (pg_wchar), (char *) &nchar, sizeof (pg_wchar), coll) == 0) /* BUG#Z1464 */
					return hptr - haystack + 1;
				hptr++;
			}
		}
		else
		{
			const pg_wchar *needle_last = &needle[needle_len - 1];

			/* Start at startpos plus the length of the needle */
			hptr = &haystack[start_pos + needle_len - 1];
			while (hptr < haystack_end)
			{
				/* Match the needle scanning *backward* */
				const pg_wchar *nptr;
				const pg_wchar *p;

				nptr = needle_last;
				p = hptr;
				while (varstr_cmp((char *) nptr, sizeof (pg_wchar), (char *) p, sizeof (pg_wchar), coll) == 0) /* BUG#Z1464 */
				{
					/* Matched it all?	If so, return 1-based position */
					if (nptr == needle)
						return p - haystack + 1;
					nptr--, p--;
				}

				/*
				 * No match, so use the haystack char at hptr to decide how
				 * far to advance.  If the needle had any occurrence of that
				 * character (or more precisely, one sharing the same
				 * skiptable entry) before its last character, then we advance
				 * far enough to align the last such needle character with
				 * that haystack position.  Otherwise we can advance by the
				 * whole needle length.
				 */
				hptr += state->skiptable[*hptr & skiptablemask];
			}
		}
	}

	return 0;					/* not found */
}

/*
 * ReqID:SRS-GKC-DATATYPE_001
 * text_instring - 
 *	Does the real work for ora_instr() and ora_instrb().
 */
static int
text_instring(text *str, text *search_str, int32 position, int32 occurrence, bool isByte, Oid coll)
{
	int32	maxTimes;
	int32	i;
	int32	src_text_len;	/* length of src text */
	int32	sub_text_len;	/* length of search_str text */
	int32	result;
	TextPositionState	state;

	text_position_setup(str, search_str, &state, isByte, coll);

	src_text_len = state.len1;
	sub_text_len = state.len2;

	if (src_text_len <= 0 || sub_text_len <= 0)
	{
		text_position_cleanup(&state);
		return 0;
	}

	/*
	 * If occurrence is greater than 1, then the database searches for the
	 * second occurrence beginning with the second character in the first 
	 * occurrence of string. So, if position is positive, then the maxinum 
	 * occurrences is:
	 *
	 *     src_text_len = src_text_len - position + 1
	 *     maxTimes = src_text_len - sub_text_len + 1
	 *
	 * But if position is negative, then Oracle counts backward from the end
	 * of string and then searches backward from the resulting position. An
	 * additional search forward from the resulting positionis performed at 
	 * the result position. So maxTimes need to plus 1. 
	 */
	if (position > 0)
		maxTimes = src_text_len - sub_text_len - position + 2;
	else
		maxTimes = src_text_len - Max(sub_text_len, -position) + 1;

	if (abs(position) > src_text_len || src_text_len < sub_text_len || occurrence > maxTimes)
	{
		text_position_cleanup(&state);
		return 0;
	}
	
	if (position > 0)
	{
		/*searching from begin with the specific times */
		result = position - 1;
		
		for (i = 0; i < occurrence; i++)
		{
			result = text_position_next(result + 1, &state);
			if (result == 0)
				break;
		}
	}
	else
	{
		/*searching from back with the specific times */
		result = src_text_len + position + 2;
		
		for (i = 0; i < occurrence; i++)
		{
			result = text_position_prev(result - 1, &state);
			if (result == 0)
				break;
		}
	}

	text_position_cleanup(&state);

	return result;
}

/*
 * appendStringInfoText
 *
 * Append a text to str.
 * Like appendStringInfoString(str, text_to_cstring(t)) but faster.
 */
static void
appendStringInfoText(StringInfo str, const text *t)
{
	appendBinaryStringInfo(str, VARDATA_ANY(t), VARSIZE_ANY_EXHDR(t));
}

/*
 * ReqID:SRS-GKC-DATATYPE_001
 * text_position_prev - 
 *	searches substring backward from specify position.
 *
 *	This function is used to implement oracle 'instr' function, 
 *	when the instr function's position parameter is negative.
 */
static int
text_position_prev(int start_pos, TextPositionState *state)
{
	int			pos = 0,
				p,
				px;

	/* start_pos is one-based */
	Assert(start_pos > 0);

	if (state->len2 <= 0)
		return start_pos;	/* result for empty pattern */
	
	if (!state->use_wchar)
	{
		/* simple case - single byte encoding */
		char	   *p1 = state->str1;
		char	   *p2 = state->str2;
		
		/*
		 *	Assuming that the end of the string is a matching substring, 
		 *	calculate the start of the last substring, that is the value
		 *	of 'px'. If the specified starting position(start_pos) is 
		 *	greater than 'px', then 'px' equal to the position of the 
		 *	last substring. otherwise 'px' equal to the specified start 
		 *	position(start_pos).
		 *
		 *	Note that 'p' and 'px' is zero-based.
		 */
		px = (state->len1 - state->len2);
		if (start_pos - 1 < px)
			px = start_pos - 1;

		p1 += px;

		if (state->len2 == 1)
		{
			for (p = px; p >= 0; p--)
			{
				if (*p1 == *p2)
				{
					pos = p + 1;
					break;
				}
				p1--;
			}
		}
		else
		{
			for (p = px; p >= 0; p--)
			{
				if ((*p1 == *p2) && (strncmp(p1, p2, state->len2) == 0))
				{
					pos = p + 1;
					break;
				}
				p1--;
			}
		}
	}
	else
	{
		/* not as simple - multibyte encoding */
		pg_wchar   *p1 = state->wstr1;
		pg_wchar   *p2 = state->wstr2;

		px = (state->len1 - state->len2);
		if (start_pos - 1 < px)
			px = start_pos - 1;

		p1 += px;

		if (state->len2 == 1)
		{
			for (p = px; p >= 0; p--)
			{
				if (*p1 == *p2)
				{
					pos = p + 1;
					break;
				}
				p1--;
			}
		}
		else
		{
			for (p = px; p >= 0; p--)
			{
				if ((*p1 == *p2) && (pg_wchar_strncmp(p1, p2, state->len2) == 0))
				{
					pos = p + 1;
					break;
				}
				p1--;
			}
		}
	}

	return pos;
}

/* Begin - BUG:M0000084 */
/****************************************************************
 * substr
 *
 * Syntax:
 *   substr (char, position, [substring_length])
 *
 * Purpouse:
 *   return a portion of char, beginning at character position,
 *	 substring_length characters long, compatible with Oracle.
 *
 ****************************************************************/
Datum
oracle_substr3(PG_FUNCTION_ARGS)
{
	int32	len = PG_GETARG_INT32(2);
	/* Begin - req#00001426 */
	text *res;

	if (len < 0)
		PG_RETURN_NULL();
	res = ora_substr(PG_GETARG_DATUM(0), PG_GETARG_INT32(1), len);

	if (res == NULL)
		PG_RETURN_NULL();
	else
		PG_RETURN_TEXT_P(res);
	/* End - req#00001426 */
}

Datum
oracle_substr2(PG_FUNCTION_ARGS)
{
	/* Begin - req#00001426 */
	text *res;

	res = ora_substr(PG_GETARG_DATUM(0), PG_GETARG_INT32(1), -1);

	if (res == NULL)
		PG_RETURN_NULL();
	else
		PG_RETURN_TEXT_P(res);
	/* End - req#00001426 */
}

/*
 * len < 0 means "length is not specified".
 */
static text *
ora_substr(Datum str, int start, int len)
{
	/* Begin - req#00001426 */
	text *result;
	/* End - req#00001426 */

	if (start == 0)
		start = 1;	/* 0 is interpreted as 1 */
	else if (start < 0)
	{
		text   *t;
		int32	n;

		t = DatumGetTextPP(str);
		n = pg_mbstrlen_with_len(VARDATA_ANY(t), VARSIZE_ANY_EXHDR(t));
		start = n + start + 1;
		if (start <= 0)
			return NULL; /* req#00001426 */
		str = PointerGetDatum(t);	/* save detoasted text */
	}

	/* Begin - req#00001426 */
	/*
	 * empty string is considered as NULL in oracle
	 */
	if (len < 0)
		result =  DatumGetTextP(DirectFunctionCall2(text_substr_no_len,
			str, Int32GetDatum(start)));
	else
		result =  DatumGetTextP(DirectFunctionCall3(text_substr,
			str, Int32GetDatum(start), Int32GetDatum(len)));
	if (result != NULL &&
		VARSIZE_ANY_EXHDR(result) == 0)
	{
		pfree(result);
		return NULL;
	}
	else
		return DatumGetTextP(result);
	/* End - req#00001426 */
}

/****************************************************************
 * instr
 *
 * Syntax:
 *   instr (string VARCHAR, substring VARCHAR)
 *   instr (string VARCHAR, substring VARCHAR, position INTEGER)
 *   instr (string VARCHAR, substring VARCHAR, position INTEGER, occurrence INTEGER)
 *
 * Purpouse:
 *   search string for substring, compatible with Oracle.
 *
 ****************************************************************/
Datum
oracle_instr2 (PG_FUNCTION_ARGS)
{
	text *arg1 = PG_GETARG_TEXT_PP(0);
	text *arg2 = PG_GETARG_TEXT_PP(1);

	PG_RETURN_INT32(ora_instr(arg1, arg2, 1, 1, PseudoCollationReplace_BNB(PG_GET_COLLATION())));
}

Datum
oracle_instr3 (PG_FUNCTION_ARGS)
{
	text *arg1 = PG_GETARG_TEXT_PP(0);
	text *arg2 = PG_GETARG_TEXT_PP(1);
	int arg3 = PG_GETARG_INT32(2);

	PG_RETURN_INT32(ora_instr(arg1, arg2, arg3, 1, PseudoCollationReplace_BNB(PG_GET_COLLATION())));
}

Datum
oracle_instr4 (PG_FUNCTION_ARGS)
{
	text *arg1 = PG_GETARG_TEXT_PP(0);
	text *arg2 = PG_GETARG_TEXT_PP(1);
	int arg3 = PG_GETARG_INT32(2);
	int arg4 = PG_GETARG_INT32(3);

	PG_RETURN_INT32(ora_instr(arg1, arg2, arg3, arg4, PseudoCollationReplace_BNB(PG_GET_COLLATION())));
}

static int
ora_mb_strlen(text *str, char **sizes, int **positions)
{
	int r_len;
	int cur_size = 0;
	int sz;
	char *p;
	int cur = 0;

	p = VARDATA_ANY(str);
	r_len = VARSIZE_ANY_EXHDR(str);

	if (NULL != sizes)
		*sizes = palloc(r_len * sizeof(char));
	if (NULL != positions)
		*positions = palloc(r_len * sizeof(int));

	while (cur < r_len)
	{
		sz = pg_mblen(p);
		if (sizes)
			(*sizes)[cur_size] = sz;
		if (positions)
			(*positions)[cur_size] = cur;
		cur += sz;
		p += sz;
		cur_size += 1;
	}

	return cur_size;
}

static int
ora_instr_mb(text *txt, text *pattern, int start, int nth, Oid coll /* BUG#Z1463 */)
{
	int			c_len_txt, c_len_pat;
	int			b_len_pat;
	int		   *pos_txt;
	const char *str_txt, *str_pat;
	int			beg, end, i, dx;

	str_txt = VARDATA_ANY(txt);
	c_len_txt = ora_mb_strlen(txt, NULL, &pos_txt);
	str_pat = VARDATA_ANY(pattern);
	b_len_pat = VARSIZE_ANY_EXHDR(pattern);
	c_len_pat = pg_mbstrlen_with_len(str_pat, b_len_pat);

	if (start > 0)
	{
		dx = 1;
		beg = start - 1;
		end = c_len_txt - c_len_pat + 1;
		if (beg >= end)
			return 0;	/* out of range */
	}
	else if (start == 0) /* req#00001426 */
	{
		return 0;
	}
	else
	{
		dx = -1;
		beg = Min(c_len_txt + start, c_len_txt - c_len_pat);
		end = -1;
		if (beg <= end)
			return 0;	/* out of range */
	}

	for (i = beg; i != end; i += dx)
	{
		if (varstr_cmp(str_txt + pos_txt[i], b_len_pat, str_pat, b_len_pat, coll) == 0) /* BUG#Z1463 */
		{
			if (--nth == 0)
				return i + 1;
		}
	}

	return 0;
}

static int
ora_instr(text *txt, text *pattern, int start, int nth, Oid coll /* BUG#Z1463 */)
{
	int			len_txt, len_pat;
	const char *str_txt, *str_pat;
	int			beg, end, i, dx;

	if (nth <= 0)
		ereport(ERROR,
			(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			 errmsg("invalid parameter"), \
			 errdetail("Four parameter isn't positive.")));

	/* Forward for multibyte strings */
	if (pg_database_encoding_max_length() > 1)
		return ora_instr_mb(txt, pattern, start, nth, coll /* BUG#Z1463 */);

	str_txt = VARDATA_ANY(txt);
	len_txt = VARSIZE_ANY_EXHDR(txt);
	str_pat = VARDATA_ANY(pattern);
	len_pat = VARSIZE_ANY_EXHDR(pattern);

	if (start > 0)
	{
		dx = 1;
		beg = start - 1;
		end = len_txt - len_pat + 1;
		if (beg >= end)
			return 0;	/* out of range */
	}
	else if (start == 0) /* req#00001426 */
	{
		return 0;
	}
	else
	{
		dx = -1;
		beg = Min(len_txt + start, len_txt - len_pat);
		end = -1;
		if (beg <= end)
			return 0;	/* out of range */
	}

	for (i = beg; i != end; i += dx)
	{
		if (varstr_cmp(str_txt + i, len_pat, str_pat, len_pat, coll) == 0) /* BUG#Z1463 */
		{
			if (--nth == 0)
				return i + 1;
		}
	}

	return 0;
}
/* End - BUG:M0000084 */

/* Begin - BUG#M0000194 */
/*
 * Convert a tilde (~) to ...
 *	1: a full width tilde. (same as JA16EUCTILDE in oracle)
 *	0: a full width overline. (same as JA16EUC in oracle)
 */
#define JA_TO_FULL_WIDTH_TILDE	1

static const char *
TO_MULTI_BYTE_UTF8[95] =
{
	"\343\200\200",
	"\357\274\201",
	"\342\200\235",
	"\357\274\203",
	"\357\274\204",
	"\357\274\205",
	"\357\274\206",
	"\342\200\231",
	"\357\274\210",
	"\357\274\211",
	"\357\274\212",
	"\357\274\213",
	"\357\274\214",
	"\357\274\215",
	"\357\274\216",
	"\357\274\217",
	"\357\274\220",
	"\357\274\221",
	"\357\274\222",
	"\357\274\223",
	"\357\274\224",
	"\357\274\225",
	"\357\274\226",
	"\357\274\227",
	"\357\274\230",
	"\357\274\231",
	"\357\274\232",
	"\357\274\233",
	"\357\274\234",
	"\357\274\235",
	"\357\274\236",
	"\357\274\237",
	"\357\274\240",
	"\357\274\241",
	"\357\274\242",
	"\357\274\243",
	"\357\274\244",
	"\357\274\245",
	"\357\274\246",
	"\357\274\247",
	"\357\274\250",
	"\357\274\251",
	"\357\274\252",
	"\357\274\253",
	"\357\274\254",
	"\357\274\255",
	"\357\274\256",
	"\357\274\257",
	"\357\274\260",
	"\357\274\261",
	"\357\274\262",
	"\357\274\263",
	"\357\274\264",
	"\357\274\265",
	"\357\274\266",
	"\357\274\267",
	"\357\274\270",
	"\357\274\271",
	"\357\274\272",
	"\357\274\273",
	"\357\277\245",
	"\357\274\275",
	"\357\274\276",
	"\357\274\277",
	"\342\200\230",
	"\357\275\201",
	"\357\275\202",
	"\357\275\203",
	"\357\275\204",
	"\357\275\205",
	"\357\275\206",
	"\357\275\207",
	"\357\275\210",
	"\357\275\211",
	"\357\275\212",
	"\357\275\213",
	"\357\275\214",
	"\357\275\215",
	"\357\275\216",
	"\357\275\217",
	"\357\275\220",
	"\357\275\221",
	"\357\275\222",
	"\357\275\223",
	"\357\275\224",
	"\357\275\225",
	"\357\275\226",
	"\357\275\227",
	"\357\275\230",
	"\357\275\231",
	"\357\275\232",
	"\357\275\233",
	"\357\275\234",
	"\357\275\235",
#if JA_TO_FULL_WIDTH_TILDE
	"\357\275\236"
#else
	"\357\277\243"
#endif
};

static const char *
TO_MULTI_BYTE_EUCJP[95] =
{
	"\241\241",
	"\241\252",
	"\241\311",
	"\241\364",
	"\241\360",
	"\241\363",
	"\241\365",
	"\241\307",
	"\241\312",
	"\241\313",
	"\241\366",
	"\241\334",
	"\241\244",
	"\241\335",
	"\241\245",
	"\241\277",
	"\243\260",
	"\243\261",
	"\243\262",
	"\243\263",
	"\243\264",
	"\243\265",
	"\243\266",
	"\243\267",
	"\243\270",
	"\243\271",
	"\241\247",
	"\241\250",
	"\241\343",
	"\241\341",
	"\241\344",
	"\241\251",
	"\241\367",
	"\243\301",
	"\243\302",
	"\243\303",
	"\243\304",
	"\243\305",
	"\243\306",
	"\243\307",
	"\243\310",
	"\243\311",
	"\243\312",
	"\243\313",
	"\243\314",
	"\243\315",
	"\243\316",
	"\243\317",
	"\243\320",
	"\243\321",
	"\243\322",
	"\243\323",
	"\243\324",
	"\243\325",
	"\243\326",
	"\243\327",
	"\243\330",
	"\243\331",
	"\243\332",
	"\241\316",
	"\241\357",
	"\241\317",
	"\241\260",
	"\241\262",
	"\241\306",
	"\243\341",
	"\243\342",
	"\243\343",
	"\243\344",
	"\243\345",
	"\243\346",
	"\243\347",
	"\243\350",
	"\243\351",
	"\243\352",
	"\243\353",
	"\243\354",
	"\243\355",
	"\243\356",
	"\243\357",
	"\243\360",
	"\243\361",
	"\243\362",
	"\243\363",
	"\243\364",
	"\243\365",
	"\243\366",
	"\243\367",
	"\243\370",
	"\243\371",
	"\243\372",
	"\241\320",
	"\241\303",
	"\241\321",
#if JA_TO_FULL_WIDTH_TILDE
	"\241\301"
#else
	"\241\261"
#endif
};

Datum
orafce_to_multi_byte(PG_FUNCTION_ARGS)
{
	text	   *src;
	text	   *dst;
	const char *s;
	char	   *d;
	int			srclen;
#if defined(_MSC_VER) && (defined(_M_X64) || defined(__amd64__))
	__int64		dstlen;
#else
	int			dstlen;
#endif
	int			i;
	const char **map;

	switch (GetDatabaseEncoding())
	{
		case PG_UTF8:
			map = TO_MULTI_BYTE_UTF8;
			break;
		case PG_EUC_JP:
		case PG_EUC_JIS_2004:
			map = TO_MULTI_BYTE_EUCJP;
			break;
		/*
		 * TODO: Add converter for encodings.
		 */
		default:	/* no need to convert */
			PG_RETURN_DATUM(PG_GETARG_DATUM(0));
	}

	src = PG_GETARG_TEXT_PP(0);
	s = VARDATA_ANY(src);
	srclen = VARSIZE_ANY_EXHDR(src);
	dst = (text *) palloc(VARHDRSZ + srclen * MAX_CONVERSION_GROWTH);
	d = VARDATA(dst);

	for (i = 0; i < srclen; i++)
	{
		unsigned char	u = (unsigned char) s[i];
		if (0x20 <= u && u <= 0x7e)
		{
			const char *m = map[u - 0x20];
			while (*m)
			{
				*d++ = *m++;
			}
		}
		else
		{
			*d++ = s[i];
		}
	}

	dstlen = d - VARDATA(dst);
	SET_VARSIZE(dst, VARHDRSZ + dstlen);

	PG_RETURN_TEXT_P(dst);
}

static int
getindex(const char **map, char *mbchar, int mblen)
{
	int		i;

	for (i = 0; i < 95; i++)
	{
		if (!memcmp(map[i], mbchar, mblen))
			return i;
	}

	return -1;
}

Datum
orafce_to_single_byte(PG_FUNCTION_ARGS)
{
	text	   *src;
	text	   *dst;
	char	   *s;
	char	   *d;
	int			srclen;
#if defined(_MSC_VER) && (defined(_M_X64) || defined(__amd64__))
	__int64		dstlen;
#else
	int			dstlen;
#endif
	const char **map;

	switch (GetDatabaseEncoding())
	{
		case PG_UTF8:
			map = TO_MULTI_BYTE_UTF8;
			break;
		case PG_EUC_JP:
		case PG_EUC_JIS_2004:
			map = TO_MULTI_BYTE_EUCJP;
			break;
		/*
		 * TODO: Add converter for encodings.
		 */
		default:	/* no need to convert */
			PG_RETURN_DATUM(PG_GETARG_DATUM(0));
	}

	src = PG_GETARG_TEXT_PP(0);
	s = VARDATA_ANY(src);
	srclen = VARSIZE_ANY_EXHDR(src);

	/* XXX - The output length should be <= input length */
	dst = (text *) palloc0(VARHDRSZ + srclen);
	d = VARDATA(dst);

	while (*s && (s - VARDATA_ANY(src) < srclen))
	{
		char   *u = s;
		int		clen;
		int		mapindex;

		clen = pg_mblen(u);
		s += clen;

		if (clen == 1)
			*d++ = *u;
		else if ((mapindex = getindex(map, u, clen)) >= 0)
		{
			const char m = 0x20 + mapindex;
			*d++ = m;
		}
		else
		{
			memcpy(d, u, clen);
			d += clen;
		}
	}

	dstlen = d - VARDATA(dst);
	SET_VARSIZE(dst, VARHDRSZ + dstlen);

	PG_RETURN_TEXT_P(dst);
}

Datum
ora_asciistr(PG_FUNCTION_ARGS)
{
	Datum		utf8bytea;
	char	   *retval;

	unsigned char  *p;
	unsigned int	r;
	int				i;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	/* convert input string encoding to utf-8 */
	utf8bytea = DirectFunctionCall2(pg_convert_to,
									PG_GETARG_DATUM(0),
									DirectFunctionCall1(namein,
														CStringGetDatum("UTF8")));

	retval = (char *) palloc0(VARSIZE_ANY(utf8bytea) * 3);
	p = (unsigned char *) VARDATA_ANY(utf8bytea);

	/* convert utf-8 to utf-16(be) except ascii characters */
	for (i = 0; i < VARSIZE_ANY_EXHDR(utf8bytea); ++i)
	{
		r = 0x00;

		if (0x20 <= *p && *p <= 0x7E)
		{
			sprintf(retval, "%s%c", retval, *p);
		}
		else if (0xC0 <= *p && *p <= 0xDF)
		{
			r |= ((*p & 0x1F) << 6);
			r |= (*(++p) & 0x3F);
			sprintf(retval, "%s\\%04X", retval, r);
			++i;
		}
		else if (0xE0 <= *p && *p <= 0xEF)
		{
			r |= ((*p & 0x0F) << 12);
			r |= ((*(++p) & 0x3F) << 6);
			r |= (*(++p) & 0x3F);
			sprintf(retval, "%s\\%04X", retval, r);
			i += 2;
		}
		else if (0xF0 <= *p && *p <= 0xF7)
		{
			r |= ((*p & 0x07) << 18);
			r |= ((*(++p) & 0x3F) << 12);
			r |= ((*(++p) & 0x3F) << 6);
			r |= (*(++p) & 0x3F);
			r -= 0x10000;
			sprintf(retval, "%s\\%04X\\%04X", retval, (0xD800 | (r >> 10)),
													  (0xDC00 | (r & 0x03FF)));
			i += 3;
		}
		++p;
	}

	PG_RETURN_TEXT_P(CStringGetTextDatum(retval));
}

/*
 * Oracle:		compose(char)
 * Postgres:	normalize(text, NFC)
 */
Datum
ora_compose(PG_FUNCTION_ARGS)
{
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	PG_RETURN_DATUM(DirectFunctionCall2(unicode_normalize_func,
										PG_GETARG_DATUM(0),
										PointerGetDatum(cstring_to_text("NFC"))));
}

/*
 * Oracle:		decompose(string[, 'canonical' | 'compatibility'])
 * Postgres:	normalize(text, NFD | NFKD)
 */
Datum
ora_decompose(PG_FUNCTION_ARGS)
{
	if (PG_ARGISNULL(0) || (PG_NARGS() > 1 && PG_ARGISNULL(1)))
		PG_RETURN_NULL();

	if (PG_NARGS() > 1)
	{
		char   *form = text_to_cstring(PG_GETARG_TEXT_PP(1));

		/* 'compatibility' <=> NFKD */
		if (!pg_strcasecmp(form, "compatibility"))
			PG_RETURN_DATUM(DirectFunctionCall2(
								unicode_normalize_func,
								PG_GETARG_DATUM(0),
								PointerGetDatum(cstring_to_text("NFKD"))));
		/* 'canonical' (default) <=> NFD */
		else if (!pg_strcasecmp(form, "canonical"))
			;	/* do it later */
		else
			ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("Invalid NLS Parameter String Used In SQL Function")));
	}

	PG_RETURN_DATUM(DirectFunctionCall2(unicode_normalize_func,
										PG_GETARG_DATUM(0),
										PointerGetDatum(cstring_to_text("NFD"))));
}

#define N_CHARSET_MAP	37

static const char * charsets_mapping[N_CHARSET_MAP][2] =
{
	/* character set in Postgres, character set in Oracle (sort by ALPHABET) */
	{"utf8",				"AL32UTF8"},
	{"iso88596",			"AR8ISO8859P6"},
	{"win1256",				"AR8MSWIN1256"},

	{"latin7",				"BLT8ISO8859P13"},
	{"win1257",				"BLT8MSWIN1257"},

	{"latin8",				"CEL8ISO8859P14"},
	{"iso88595",			"CL8ISO8859P5"},
	{"koi8r",				"CL8KOI8R"},
	{"koi8u",				"CL8KOI8U"},
	{"win1251",				"CL8MSWIN1251"},

	{"latin2",				"EE8ISO8859P2"},
	{"win1250",				"EE8MSWIN1250"},
	{"iso88597",			"EL8ISO8859P7"},
	{"win1253",				"EL8MSWIN1253"},

	{"iso88598",			"IW8ISO8859P8"},
	{"win1255",				"IW8MSWIN1255"},

	{"euc_jp",				"JA16EUC"},
	{"euc_jis_2004",		"JA16EUCTILDE"},
	{"sjis",				"JA16SJIS"},
	{"shiftjis2004",		"JA16SJISTILDE"},

	{"johab",				"KO16KSCCS"},
	{"uhc",					"KO16MSWIN949"},

	{"latin4",				"NEE8ISO8859P4"},
	{"latin6",				"NE8ISO8859P10"},

	{"win866",				"RU8PC866"},

	{"latin3",				"SE8ISO8859P3"},

	{"win1254",				"TR8MSWIN1254"},

	{"sqlascii",			"US7ASCII"},

	{"win1258",				"VN8MSWIN1258"},

	{"latin1",				"WE8ISO8859P1"},
	{"latin5",				"WE8ISO8859P9"},
	{"latin9",				"WE8ISO8859P15"},
	{"win1252",				"WE8MSWIN1252"},

	{"big5",				"ZHT16BIG5"},
	{"gbk",					"ZHS16GBK"},
	{"euc_tw",				"ZHT32EUC"},
	{"gb18030",				"ZHS32GB18030"}
};

Datum
ora_encname_mapping(PG_FUNCTION_ARGS)
{
	char   *input_name = NameStr(*PG_GETARG_NAME(0));
	int8	idx;
	Datum	result = PG_GETARG_DATUM(0);

	for (idx = 0; idx < N_CHARSET_MAP; ++idx)
	{
		if (!pg_strcasecmp(input_name, charsets_mapping[idx][1]))
		{
			result = DirectFunctionCall1(namein,
										 CStringGetDatum(charsets_mapping[idx][0]));
			break;
		}
	}

	PG_RETURN_DATUM(result);
}

/*
 * Oracle:		convert(char, dest_char_set, source_char_set)
 * Postgres:	convert(bytea, name(source), name(dest))
 *
 * convert(varchar2, name(dest), name(source)) -> varchar2
 */
Datum
ora_convert(PG_FUNCTION_ARGS)
{
	Datum		string = PG_GETARG_DATUM(0);
	Datum		dest_name = PG_GETARG_DATUM(1);
	Datum		src_name = PG_GETARG_DATUM(2);
	Datum		result;

	result = DirectFunctionCall3(pg_convert, string,
								 DirectFunctionCall1(ora_encname_mapping, src_name),
								 DirectFunctionCall1(ora_encname_mapping, dest_name));

	PG_RETURN_DATUM(result);
}
/* End - BUG#M0000194 */


/* Begin - bug#00008827 */
static void
ora_check_regex_options(text *flags, bool check_g)
{
	char	*paramstr = NULL;
	char *b;

	if (flags == NULL)
		return;

	paramstr = text_to_cstring(flags);

	for (b = paramstr; *b != '\0'; b++)
		if (!MATCHPARAM(*b) &&
		 (!check_g ||
		 (check_g && *b != 'g')))
			ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("illegal argument for function")));
	pfree(paramstr);

	return;
}
/* End - bug#00008827 */

