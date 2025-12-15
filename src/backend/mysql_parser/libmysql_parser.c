/*-------------------------------------------------------------------------
 *
 * libmysql_parser.c
 *		Main entry point/driver for MySQL grammar
 *
 * Note that the grammar is not allowed to perform any table access
 * (since we need to be able to do basic parsing even while inside an
 * aborted transaction).  Therefore, the data structures returned by
 * the grammar are "raw" parsetrees that still need to be analyzed by
 * analyze.c and related files.
 *
 *
 * Portions Copyright (c) 2023, IvorySQL-2.0
 *
 * IDENTIFICATION
 *	  src/backend/mysql_parser/libmysql_parser.c
 *
 * add the file for requirement "SRS-MISC-MYSQL-FRAME"
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
/* BEGIN - SQL PARSER */
#include "fmgr.h"

#include "catalog/pg_type.h"
#include "mysql_parser/mys_keywords.h"
/* END - SQL PARSER */
#include "mb/pg_wchar.h"
/* BEGIN - SQL PARSER */
#include "funcapi.h"
#include "mysql_parser/mys_scanner.h"
#include "mysql_parser/mys_gramparse.h"
#include "parser/parser.h"
#include "utils/builtins.h"
#include "oracle_parser/ora_parser_hook.h"
/* END - SQL PARSER */
#include "parser/parser.h"
#include "parser/scansup.h"
/* BEGIN - SQL PARSER */
#include "utils/ora_compatible.h"
#include "utils/guc.h"	/* ReqID: SRS-MYSQL-CHARACTERTYPE-CONVERT */

PG_MODULE_MAGIC;

/*
 * Struct for tracking locations/lengths of constants during normalization
 */
typedef struct pgssLocationLen
{
	int			location;		/* start offset in query text */
	int			length;			/* length in bytes, or -1 to ignore */
} pgssLocationLen;

/*
 * Working state for computing a query jumble and producing a normalized
 * query string
 */
typedef struct JumbleState
{
	/* Jumble of current query tree */
	unsigned char *jumble;

	/* Number of bytes used in jumble[] */
	Size		jumble_len;

	/* Array of locations of constants that should be removed */
	pgssLocationLen *clocations;

	/* Allocated length of clocations array */
	int			clocations_buf_size;

	/* Current number of valid entries in clocations array */
	int			clocations_count;

	/* highest Param id we've seen, in order to start normalization correctly */
	int			highest_extern_param_id;
} JumbleState;

/* saved hook value */
static raw_parser_hook_type prev_raw_parser = NULL;
static get_keywords_hook_type prev_pg_get_keywords = NULL;
static fill_in_constant_lengths_hook_type prev_fill_in_contant_lengths = NULL;
quote_identifier_hook_type prev_quote_identifier = NULL;

void _PG_init(void);
void _PG_fini(void);

static int	mysql_comp_location(const void *a, const void *b);

static List * mysql_raw_parser(const char *str, RawParseMode mode);
static Datum mysql_pg_get_keywords(PG_FUNCTION_ARGS);
static void mysql_fill_in_constant_lengths(void *jjstate, const char *query, int query_loc);
static const char * mysql_quote_identifier(const char *ident);

/*
 * Module load callback
 */
void
_PG_init(void)
{
	prev_raw_parser 			  = mys_raw_parser;
	prev_pg_get_keywords 		  = get_keywords_hook;
	prev_fill_in_contant_lengths  = fill_in_constant_lengths_hook;
	prev_quote_identifier 		  = quote_identifier_hook;

	mys_raw_parser 				  = mysql_raw_parser;
	get_keywords_hook 			  = mysql_pg_get_keywords;
	fill_in_constant_lengths_hook = mysql_fill_in_constant_lengths;
	quote_identifier_hook		  = mysql_quote_identifier;
}

/*
 * Module unload callback
 */
void
_PG_fini(void)
{
	mys_raw_parser 			  = prev_raw_parser;
	get_keywords_hook 			  = prev_pg_get_keywords;
	fill_in_constant_lengths_hook = prev_fill_in_contant_lengths;
	quote_identifier_hook		  = prev_quote_identifier;
}

/*
 * comp_location: comparator for qsorting pgssLocationLen structs by location
 */
static int
mysql_comp_location(const void *a, const void *b)
{
	int			l = ((const pgssLocationLen *) a)->location;
	int			r = ((const pgssLocationLen *) b)->location;

	if (l < r)
		return -1;
	else if (l > r)
		return +1;
	else
		return 0;
}

static bool check_uescapechar(unsigned char escape);
static char *str_udeescape(const char *str, char escape,
						   int position, mys_core_yyscan_t yyscanner);


/*
 * mysql_raw_parser
 *		Given a query in string form, do lexical and grammatical analysis.
 *
 * Returns a list of raw (un-analyzed) parse trees.  The contents of the
 * list have the form required by the specified RawParseMode.
 */

static List *
mysql_raw_parser(const char *str, RawParseMode mode)
{
	mys_core_yyscan_t yyscanner;
	mys_base_yy_extra_type yyextra;
	int			yyresult;

	/* initialize the flex scanner */
	yyscanner = mys_scanner_init(str, &yyextra.core_yy_extra,
							 &MysScanKeywords, MysScanKeywordTokens);

	/* base_yylex() only needs us to initialize the lookahead token, if any */
	if (mode == RAW_PARSE_DEFAULT)
		yyextra.have_lookahead = false;
	else
	{
		/* this array is indexed by RawParseMode enum */
		static const int mode_token[] = {
			0,					/* RAW_PARSE_DEFAULT */
			MODE_TYPE_NAME,		/* RAW_PARSE_TYPE_NAME */
			MODE_PLPGSQL_EXPR,	/* RAW_PARSE_PLPGSQL_EXPR */
			MODE_PLPGSQL_ASSIGN1,	/* RAW_PARSE_PLPGSQL_ASSIGN1 */
			MODE_PLPGSQL_ASSIGN2,	/* RAW_PARSE_PLPGSQL_ASSIGN2 */
			MODE_PLPGSQL_ASSIGN3,	/* RAW_PARSE_PLPGSQL_ASSIGN3 */
			/* pad with 0 as a placeholder for oracle-added values to correctly fetch MySQL value */
			0,						/* RAW_PARSE_PLISQL_EXPR */
			0,						/* RAW_PARSE_PLISQL_ASSIGN1 */
			0,						/* RAW_PARSE_PLISQL_ASSIGN2 */
			0,						/* RAW_PARSE_PLISQL_ASSIGN3 */
			0,						/* RAW_PARSE_PLISQL_ASSIGNN */
			MODE_PLMYSQL_EXPR		/* RAW_PLMYSQL_EXPR, set user defined variable in PL */
		};

		yyextra.have_lookahead = true;
		yyextra.lookahead_token = mode_token[mode];
		yyextra.lookahead_yylloc = 0;
		yyextra.lookahead_end = NULL;
	}

	/* initialize the bison parser */
	mys_parser_init(&yyextra);

	/* Parse! */
	yyresult = mys_base_yyparse(yyscanner);

	/* Clean up (release memory) */
	mys_scanner_finish(yyscanner);

	if (yyresult)				/* error */
		return NIL;

	return yyextra.parsetree;
}


/*
 * Intermediate filter between parser and core lexer (core_yylex in scan.l).
 *
 * This filter is needed because in some cases the standard SQL grammar
 * requires more than one token lookahead.  We reduce these cases to one-token
 * lookahead by replacing tokens here, in order to keep the grammar LALR(1).
 *
 * Using a filter is simpler than trying to recognize multiword tokens
 * directly in scan.l, because we'd have to allow for comments between the
 * words.  Furthermore it's not clear how to do that without re-introducing
 * scanner backtrack, which would cost more performance than this filter
 * layer does.
 *
 * We also use this filter to convert UIDENT and USCONST sequences into
 * plain IDENT and SCONST tokens.  While that could be handled by additional
 * productions in the main grammar, it's more efficient to do it like this.
 *
 * The filter also provides a convenient place to translate between
 * the core_YYSTYPE and YYSTYPE representations (which are really the
 * same thing anyway, but notationally they're different).
 */
int
mys_base_yylex(YYSTYPE *lvalp, YYLTYPE *llocp, mys_core_yyscan_t yyscanner)
{
	mys_base_yy_extra_type *yyextra = pg_yyget_extra(yyscanner);
	int			cur_token;
	int			next_token;
	int			third_token = 0; /* ReqID:SRS-MYSQL-SQL-DELETE */
	int			cur_token_length;
	YYLTYPE		cur_yylloc;

	/* Get next token --- we might already have it */
	if (yyextra->have_lookahead)
	{
		cur_token = yyextra->lookahead_token;
		lvalp->core_yystype = yyextra->lookahead_yylval;
		*llocp = yyextra->lookahead_yylloc;
		if (yyextra->lookahead_end)
			*(yyextra->lookahead_end) = yyextra->lookahead_hold_char;
		yyextra->have_lookahead = false;
	}
	else
		cur_token = mys_core_yylex(&(lvalp->core_yystype), llocp, yyscanner);

	/*
	 * If this token isn't one that requires lookahead, just return it.  If it
	 * does, determine the token length.  (We could get that via strlen(), but
	 * since we have such a small set of possibilities, hardwiring seems
	 * feasible and more efficient --- at least for the fixed-length cases.)
	 */
	switch (cur_token)
	{
		case NOT:
			cur_token_length = 3;
			break;
		case NULLS_P:
			cur_token_length = 5;
			break;
		case WITH:
			cur_token_length = 4;
			break;
		case UIDENT:
		case USCONST:
			cur_token_length = strlen(yyextra->core_yy_extra.scanbuf + *llocp);
			break;
		/* Begin - ReqID:SRS-MYSQL-BINARYTYPE */
		case BINARY:
			cur_token_length = 6;
			break;
		/* End - ReqID:SRS-MYSQL-BINARYTYPE */
		/* Begin - ReqID:SRS-MYSQL-DO */
		case DO:
			cur_token_length = 2;
			break;
		/* End - ReqID:SRS-MYSQL-DO */
		case DELETE_P:
			cur_token_length = 6;
			break;
		/* Begin - ReqID: SRS-MYSQL-CHARACTERTYPE-NCHAR VARCHAR(n) */
		case NCHAR:
			{
				/* The main purpose here is to solve the syntax problem of NCHAR VARCHAR,
				 * but in PG, N in N'aa''d\\ef' will be recognized as NCHAR,which should
				 * be returned directly, and subsequent processing may cause segmentation
				 * errors.
				 */
				char	   *tmpstr = pstrdup(yyextra->core_yy_extra.scanbuf);
				int			lenth = strlen(tmpstr);

				if (lenth >= 5 && pg_strcasecmp(tmpstr+(lenth - 5), "nchar") == 0)
					cur_token_length = 5;
				else
					return cur_token;
			}
			break;
		/* End - ReqID: SRS-MYSQL-CHARACTERTYPE-NCHAR VARCHAR(n) */
		/* Begin - ReqID: SRS-MYSQL-CHARACTERTYPE-CONVERT */
		case CONVERT:
			{
				cur_token_length = 0;
				if (mysql_convert == false)
					cur_token = IDENT;

				return cur_token;
			}
			break;
		/* End - ReqID: SRS-MYSQL-CHARACTERTYPE-CONVERT */

		default:
			return cur_token;
	}

	/*
	 * Identify end+1 of current token.  core_yylex() has temporarily stored a
	 * '\0' here, and will undo that when we call it again.  We need to redo
	 * it to fully revert the lookahead call for error reporting purposes.
	 */
	yyextra->lookahead_end = yyextra->core_yy_extra.scanbuf +
		*llocp + cur_token_length;
	Assert(*(yyextra->lookahead_end) == '\0');

	/*
	 * Save and restore *llocp around the call.  It might look like we could
	 * avoid this by just passing &lookahead_yylloc to core_yylex(), but that
	 * does not work because flex actually holds onto the last-passed pointer
	 * internally, and will use that for error reporting.  We need any error
	 * reports to point to the current token, not the next one.
	 */
	cur_yylloc = *llocp;

	/* Get next token, saving outputs into lookahead variables */
	next_token = mys_core_yylex(&(yyextra->lookahead_yylval), llocp, yyscanner);
	yyextra->lookahead_token = next_token;
	yyextra->lookahead_yylloc = *llocp;

	*llocp = cur_yylloc;

	/* Begin - ReqID:SRS-MYSQL-SQL-DELETE */
	/* Get the third token ,eg: delete quick/low_priority/ignore from */
	if (cur_token == DELETE_P &&
		(next_token == QUICK ||
		 next_token == LOW_PRIORITY ||
		 next_token == IGNORE_P))
	{
		int next_token_length;

		next_token_length = strlen(yyextra->core_yy_extra.scanbuf + *llocp);
		yyextra->lookahead_end = yyextra->core_yy_extra.scanbuf +
		*llocp + next_token_length;
		Assert(*(yyextra->lookahead_end) == '\0');
		cur_yylloc = *llocp;
		third_token = mys_core_yylex(&(yyextra->lookahead_yylval), llocp, yyscanner);
		yyextra->lookahead_token = third_token;
		yyextra->lookahead_yylloc = *llocp;
		*llocp = cur_yylloc;
	}
	/* End - ReqID:SRS-MYSQL-SQL-DELETE */

	/* Now revert the un-truncation of the current token */
	yyextra->lookahead_hold_char = *(yyextra->lookahead_end);
	*(yyextra->lookahead_end) = '\0';

	yyextra->have_lookahead = true;

	/* Replace cur_token if needed, based on lookahead */
	switch (cur_token)
	{
		/* Begin - ReqID:SRS-MYSQL-DO */
		case DO:
			{
				if (next_token == LANGUAGE || 
					next_token == NOTHING ||
					next_token == INSTEAD ||
					next_token == ALSO ||
					next_token == UPDATE ||
					next_token == SELECT ||
					next_token == INSERT ||
					next_token == DELETE_P ||
					next_token == NOTIFY ||
					next_token == VALUES || !use_mysql_do)
				{
					break;
				}
				cur_token = DO_P;
			}
			break;
		/* End - ReqID:SRS-MYSQL-DO */
		case NOT:
			/* Replace NOT by NOT_LA if it's followed by BETWEEN, IN, etc */
			switch (next_token)
			{
				case BETWEEN:
				case IN_P:
				case LIKE:
				case ILIKE:
				case SIMILAR:
					cur_token = NOT_LA;
					break;
			}
			break;

		case NULLS_P:
			/* Replace NULLS_P by NULLS_LA if it's followed by FIRST or LAST */
			switch (next_token)
			{
				case FIRST_P:
				case LAST_P:
					cur_token = NULLS_LA;
					break;
			}
			break;

		case WITH:
			/* Replace WITH by WITH_LA if it's followed by TIME or ORDINALITY */
			switch (next_token)
			{
				case TIME:
				case ORDINALITY:
					cur_token = WITH_LA;
					break;
				/* Begin - ReqID:SRS-MYSQL-SELECT */
				case ROLLUP:
					cur_token = WITH_ROLLUP;
					break;
				/* End - ReqID:SRS-MYSQL-SELECT */
			}
			break;

		case UIDENT:
		case USCONST:
			/* Look ahead for UESCAPE */
			if (next_token == UESCAPE)
			{
				/* Yup, so get third token, which had better be SCONST */
				const char *escstr;

				/* Again save and restore *llocp */
				cur_yylloc = *llocp;

				/* Un-truncate current token so errors point to third token */
				*(yyextra->lookahead_end) = yyextra->lookahead_hold_char;

				/* Get third token */
				next_token = mys_core_yylex(&(yyextra->lookahead_yylval),
										llocp, yyscanner);

				/* If we throw error here, it will point to third token */
				if (next_token != SCONST)
					mys_scanner_yyerror("UESCAPE must be followed by a simple string literal",
									yyscanner);

				escstr = yyextra->lookahead_yylval.str;
				if (strlen(escstr) != 1 || !check_uescapechar(escstr[0]))
					mys_scanner_yyerror("invalid Unicode escape character",
									yyscanner);

				/* Now restore *llocp; errors will point to first token */
				*llocp = cur_yylloc;

				/* Apply Unicode conversion */
				lvalp->core_yystype.str =
					str_udeescape(lvalp->core_yystype.str,
								  escstr[0],
								  *llocp,
								  yyscanner);

				/*
				 * We don't need to revert the un-truncation of UESCAPE.  What
				 * we do want to do is clear have_lookahead, thereby consuming
				 * all three tokens.
				 */
				yyextra->have_lookahead = false;
			}
			else
			{
				/* No UESCAPE, so convert using default escape character */
				lvalp->core_yystype.str =
					str_udeescape(lvalp->core_yystype.str,
								  '\\',
								  *llocp,
								  yyscanner);
			}

			if (cur_token == UIDENT)
			{
				/* It's an identifier, so truncate as appropriate */
				truncate_identifier(lvalp->core_yystype.str,
									strlen(lvalp->core_yystype.str),
									true);
				cur_token = IDENT;
			}
			else if (cur_token == USCONST)
			{
				cur_token = SCONST;
			}
			break;
		/* Begin - ReqID:SRS-MYSQL-SQL-DELETE */
		case DELETE_P:
			switch (next_token)
			{
				case QUICK:
				case LOW_PRIORITY:
				case IGNORE_P:
					{
						if (third_token == FROM)
							cur_token = DELETE_FROM;
					}
					break;
				case FROM:
					cur_token = DELETE_FROM;
					break;
			}
			break;
		/* End - ReqID:SRS-MYSQL-SQL-DELETE */
		/* Begin - ReqID: SRS-MYSQL-CHARACTERTYPE-NCHAR VARCHAR(n) */
		case NCHAR:
			switch (next_token)
			{
				case VARCHAR:
					cur_token = NCHAR_P;
					break;
			}
			break;
		/* End - ReqID: SRS-MYSQL-CHARACTERTYPE-NCHAR VARCHAR(n) */
	}

	return cur_token;
}

/* convert hex digit (caller should have verified that) to value */
static unsigned int
hexval(unsigned char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 0xA;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 0xA;
	elog(ERROR, "invalid hexadecimal digit");
	return 0;					/* not reached */
}

/* is Unicode code point acceptable? */
static void
check_unicode_value(pg_wchar c)
{
	if (!is_valid_unicode_codepoint(c))
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("invalid Unicode escape value")));
}

/* is 'escape' acceptable as Unicode escape character (UESCAPE syntax) ? */
static bool
check_uescapechar(unsigned char escape)
{
	if (isxdigit(escape)
		|| escape == '+'
		|| escape == '\''
		|| escape == '"'
		|| scanner_isspace(escape))
		return false;
	else
		return true;
}

/*
 * Process Unicode escapes in "str", producing a palloc'd plain string
 *
 * escape: the escape character to use
 * position: start position of U&'' or U&"" string token
 * yyscanner: context information needed for error reports
 */
static char *
str_udeescape(const char *str, char escape,
			  int position, mys_core_yyscan_t yyscanner)
{
	const char *in;
	char	   *new,
			   *out;
	size_t		new_len;
	pg_wchar	pair_first = 0;
	MysScannerCallbackState scbstate;

	/*
	 * Guesstimate that result will be no longer than input, but allow enough
	 * padding for Unicode conversion.
	 */
	new_len = strlen(str) + MAX_UNICODE_EQUIVALENT_STRING + 1;
	new = palloc(new_len);

	in = str;
	out = new;
	while (*in)
	{
		/* Enlarge string if needed */
		size_t		out_dist = out - new;

		if (out_dist > new_len - (MAX_UNICODE_EQUIVALENT_STRING + 1))
		{
			new_len *= 2;
			new = repalloc(new, new_len);
			out = new + out_dist;
		}

		if (in[0] == escape)
		{
			/*
			 * Any errors reported while processing this escape sequence will
			 * have an error cursor pointing at the escape.
			 */
			mys_setup_scanner_errposition_callback(&scbstate, yyscanner,
											   in - str + position + 3);	/* 3 for U&" */
			if (in[1] == escape)
			{
				if (pair_first)
					goto invalid_pair;
				*out++ = escape;
				in += 2;
			}
			else if (isxdigit((unsigned char) in[1]) &&
					 isxdigit((unsigned char) in[2]) &&
					 isxdigit((unsigned char) in[3]) &&
					 isxdigit((unsigned char) in[4]))
			{
				pg_wchar	unicode;

				unicode = (hexval(in[1]) << 12) +
					(hexval(in[2]) << 8) +
					(hexval(in[3]) << 4) +
					hexval(in[4]);
				check_unicode_value(unicode);
				if (pair_first)
				{
					if (is_utf16_surrogate_second(unicode))
					{
						unicode = surrogate_pair_to_codepoint(pair_first, unicode);
						pair_first = 0;
					}
					else
						goto invalid_pair;
				}
				else if (is_utf16_surrogate_second(unicode))
					goto invalid_pair;

				if (is_utf16_surrogate_first(unicode))
					pair_first = unicode;
				else
				{
					pg_unicode_to_server(unicode, (unsigned char *) out);
					out += strlen(out);
				}
				in += 5;
			}
			else if (in[1] == '+' &&
					 isxdigit((unsigned char) in[2]) &&
					 isxdigit((unsigned char) in[3]) &&
					 isxdigit((unsigned char) in[4]) &&
					 isxdigit((unsigned char) in[5]) &&
					 isxdigit((unsigned char) in[6]) &&
					 isxdigit((unsigned char) in[7]))
			{
				pg_wchar	unicode;

				unicode = (hexval(in[2]) << 20) +
					(hexval(in[3]) << 16) +
					(hexval(in[4]) << 12) +
					(hexval(in[5]) << 8) +
					(hexval(in[6]) << 4) +
					hexval(in[7]);
				check_unicode_value(unicode);
				if (pair_first)
				{
					if (is_utf16_surrogate_second(unicode))
					{
						unicode = surrogate_pair_to_codepoint(pair_first, unicode);
						pair_first = 0;
					}
					else
						goto invalid_pair;
				}
				else if (is_utf16_surrogate_second(unicode))
					goto invalid_pair;

				if (is_utf16_surrogate_first(unicode))
					pair_first = unicode;
				else
				{
					pg_unicode_to_server(unicode, (unsigned char *) out);
					out += strlen(out);
				}
				in += 8;
			}
			else
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("invalid Unicode escape"),
						 errhint("Unicode escapes must be \\XXXX or \\+XXXXXX.")));

			mys_cancel_scanner_errposition_callback(&scbstate);
		}
		else
		{
			if (pair_first)
				goto invalid_pair;

			*out++ = *in++;
		}
	}

	/* unfinished surrogate pair? */
	if (pair_first)
		goto invalid_pair;

	*out = '\0';
	return new;

	/*
	 * We might get here with the error callback active, or not.  Call
	 * scanner_errposition to make sure an error cursor appears; if the
	 * callback is active, this is duplicative but harmless.
	 */
invalid_pair:
	ereport(ERROR,
			(errcode(ERRCODE_SYNTAX_ERROR),
			 errmsg("invalid Unicode surrogate pair"),
			 mys_scanner_errposition(in - str + position + 3,	/* 3 for U&" */
								 yyscanner)));
	return NULL;				/* keep compiler quiet */
}

static Datum
mysql_pg_get_keywords(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;

	if (SRF_IS_FIRSTCALL())
	{
		MemoryContext oldcontext;
		TupleDesc	tupdesc;

		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		tupdesc = CreateTemplateTupleDesc(3, false);
		TupleDescInitEntry(tupdesc, (AttrNumber) 1, "word",
						   TEXTOID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 2, "catcode",
						   CHAROID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 3, "catdesc",
						   TEXTOID, -1, 0);

		funcctx->attinmeta = TupleDescGetAttInMetadata(tupdesc);

		MemoryContextSwitchTo(oldcontext);
	}

	funcctx = SRF_PERCALL_SETUP();

	if (funcctx->call_cntr < MysScanKeywords.num_keywords)
	{
		char	   *values[3];
		HeapTuple	tuple;

		/* cast-away-const is ugly but alternatives aren't much better */
		values[0] = unconstify(char *,
							   GetScanKeyword(funcctx->call_cntr,
											  &MysScanKeywords));

		switch (MysScanKeywordCategories[funcctx->call_cntr])
		{
			case UNRESERVED_KEYWORD:
				values[1] = "U";
				values[2] = _("unreserved");
				break;
			case COL_NAME_KEYWORD:
				values[1] = "C";
				values[2] = _("unreserved (cannot be function or type name)");
				break;
			case TYPE_FUNC_NAME_KEYWORD:
				values[1] = "T";
				values[2] = _("reserved (can be function or type name)");
				break;
			case RESERVED_KEYWORD:
				values[1] = "R";
				values[2] = _("reserved");
				break;
			default:			/* shouldn't be possible */
				values[1] = NULL;
				values[2] = NULL;
				break;
		}

		tuple = BuildTupleFromCStrings(funcctx->attinmeta, values);

		SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
	}

	SRF_RETURN_DONE(funcctx);
}

static void
mysql_fill_in_constant_lengths(void *jjstate, const char *query,
						 int query_loc)
{
	JumbleState	*jstate;
	pgssLocationLen *locs;
	mys_core_yyscan_t yyscanner;
	mys_core_yy_extra_type yyextra;
	mys_core_YYSTYPE yylval;
	YYLTYPE		yylloc;
	int			last_loc = -1;
	int			i;

	/*
	 * Sort the records by location so that we can process them in order while
	 * scanning the query text.
	 */
	jstate = (JumbleState *)jjstate;
	if (jstate->clocations_count > 1)
		qsort(jstate->clocations, jstate->clocations_count,
			  sizeof(pgssLocationLen), mysql_comp_location);
	locs = jstate->clocations;

	/* initialize the flex scanner --- should match oracle_raw_parser() */
	yyscanner = mys_scanner_init(query,
							 &yyextra,
							 &MysScanKeywords,
							 MysScanKeywordTokens);

	/* we don't want to re-emit any escape string warnings */
	yyextra.escape_string_warning = false;

	/* Search for each constant, in sequence */
	for (i = 0; i < jstate->clocations_count; i++)
	{
		int			loc = locs[i].location;
		int			tok;

		/* Adjust recorded location if we're dealing with partial string */
		loc -= query_loc;

		Assert(loc >= 0);

		if (loc <= last_loc)
			continue;			/* Duplicate constant, ignore */

		/* Lex tokens until we find the desired constant */
		for (;;)
		{
			tok = mys_core_yylex(&yylval, &yylloc, yyscanner);

			/* We should not hit end-of-string, but if we do, behave sanely */
			if (tok == 0)
				break;			/* out of inner for-loop */

			/*
			 * We should find the token position exactly, but if we somehow
			 * run past it, work with that.
			 */
			if (yylloc >= loc)
			{
				if (query[loc] == '-')
				{
					/*
					 * It's a negative value - this is the one and only case
					 * where we replace more than a single token.
					 *
					 * Do not compensate for the core system's special-case
					 * adjustment of location to that of the leading '-'
					 * operator in the event of a negative constant.  It is
					 * also useful for our purposes to start from the minus
					 * symbol.  In this way, queries like "select * from foo
					 * where bar = 1" and "select * from foo where bar = -2"
					 * will have identical normalized query strings.
					 */
					tok = mys_core_yylex(&yylval, &yylloc, yyscanner);
					if (tok == 0)
						break;	/* out of inner for-loop */
				}

				/*
				 * We now rely on the assumption that flex has placed a zero
				 * byte after the text of the current token in scanbuf.
				 */
				locs[i].length = strlen(yyextra.scanbuf + loc);
				break;			/* out of inner for-loop */
			}
		}

		/* If we hit end-of-string, give up, leaving remaining lengths -1 */
		if (tok == 0)
			break;

		last_loc = loc;
	}

	mys_scanner_finish(yyscanner);
}

static const char *
mysql_quote_identifier(const char *ident)
{
	/*
	 * Can avoid quoting if ident starts with a lowercase letter or underscore
	 * and contains only lowercase letters, digits, and underscores, *and* is
	 * not any SQL keyword.  Otherwise, supply quotes.
	 */
	int			nquotes = 0;
	bool		safe;
	const char *ptr;
	char	   *result;
	char	   *optr;

	/*
	 * would like to use <ctype.h> macros here, but they might yield unwanted
	 * locale-specific results...
	 */
	safe = ((ident[0] >= 'a' && ident[0] <= 'z') || ident[0] == '_');

	for (ptr = ident; *ptr; ptr++)
	{
		char		ch = *ptr;

		if ((ch >= 'a' && ch <= 'z') ||
			(ch >= '0' && ch <= '9') ||
			(ch == '_'))
		{
			/* okay */
		}
		else
		{
			safe = false;
			if (ch == '"')
				nquotes++;
		}
	}

	if (quote_all_identifiers)
		safe = false;

	if (safe)
	{
		/*
		 * Check for keyword.  We quote keywords except for unreserved ones.
		 * (In some cases we could avoid quoting a col_name or type_func_name
		 * keyword, but it seems much harder than it's worth to tell that.)
		 *
		 * Note: ScanKeywordLookup() does case-insensitive comparison, but
		 * that's fine, since we already know we have all-lower-case.
		 */
		int			kwnum = ScanKeywordLookup(ident, &MysScanKeywords);
		bool		tempsafe = safe; /* ReqID: SRS-MYSQL-CHARACTERTYPE-TEXT */

		if (kwnum >= 0 && MysScanKeywordCategories[kwnum] != UNRESERVED_KEYWORD)
			safe = false;

		/* Begin - ReqID: SRS-MYSQL-CHARACTERTYPE-TEXT */
		/*
		 * Due to the change of the text keyword's attribute - from a non-reserved
		 * keyword to a column name keyword - the display of text is now enclosed
		 * in double quotes. Here, we remove the display of the double quotes.
		 */
		if ((compatible_mode == MYS_MODE) && (pg_strcasecmp("text", ident) == 0))
			safe = tempsafe;
		/* End - ReqID: SRS-MYSQL-CHARACTERTYPE-TEXT */
	}

	if (safe)
		return ident;			/* no change needed */

	result = (char *) palloc(strlen(ident) + nquotes + 2 + 1);

	optr = result;
	*optr++ = '"';
	for (ptr = ident; *ptr; ptr++)
	{
		char		ch = *ptr;

		if (ch == '"')
			*optr++ = '"';
		*optr++ = ch;
	}
	*optr++ = '"';
	*optr = '\0';

	return result;
}

