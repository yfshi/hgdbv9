%{
/*-------------------------------------------------------------------------
 *
 * pl_gram.y			- Parser for the PL/iSQL procedural language
 *
 * Portions Copyright (c) 2022, IvorySQL-2.0
 *
 *
 * IDENTIFICATION
 *	  src/pl/plisql/src/pl_gram.y
 *
 * add the file for requirement "SQL PARSER"
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "catalog/namespace.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_type.h"
#include "parser/parser.h"
#include "parser/parse_type.h"
#include "oracle_parser/ora_scanner.h"
#include "parser/scansup.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h" /* ReqID:SRS-PLSQL-Collection */

#include "plisql.h"

/* Begin - SRS-PLSQL-SUBPROC */
#include "pl_subproc_function.h"
/* End - SRS-PLSQL-SUBPROC */

/* Begin - ReqID:SRS-SQL-PACKAGE */
#include "pl_package.h"
#include "pg_config.h"
/* End - ReqID:SRS-SQL-PACKAGE */


/* Begin - BUG #M0000304 */
#include "scan_reserved_kwlist.h"
/* End - BUG #M0000304 */

/* Begin - ReqID:SRS-PLSQL-CURSOR */
#include "pl_udt_type.h"
/* End - ReqID:SRS-PLSQL-CURSOR */

/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
#include "pl_udt_exception.h"
/* End - ReqID:SRS-PLSQL-EXCEPTION */


#include "utils/ora_compatible.h"	/* case sensitive indentify */
/* Begin - BUG#M0000376 */
#include "parser/parse_param.h"
/* End - BUG#M0000376 */

/* Location tracking support --- simpler than bison's default */
#define YYLLOC_DEFAULT(Current, Rhs, N) \
	do { \
		if (N) \
			(Current) = (Rhs)[1]; \
		else \
			(Current) = (Rhs)[0]; \
	} while (0)

/*
 * Bison doesn't allocate anything that needs to live across parser calls,
 * so we can easily have it use palloc instead of malloc.  This prevents
 * memory leaks if we error out during parsing.  Note this only works with
 * bison >= 2.0.  However, in bison 1.875 the default is to use alloca()
 * if possible, so there's not really much problem anyhow, at least if
 * you're building with gcc.
 */
#define YYMALLOC palloc
#define YYFREE   pfree


typedef struct
{
	int			location;
} sql_error_callback_arg;

#define parser_errposition(pos)  plisql_scanner_errposition(pos)

union YYSTYPE;					/* need forward reference for tok_is_keyword */

static	bool			tok_is_keyword(int token, union YYSTYPE *lval,
									   int kw_token, const char *kw_str);
static	void			word_is_not_variable(PLword *word, int location);
static	void			cword_is_not_variable(PLcword *cword, int location);
static	void			current_token_is_not_variable(int tok);
static	PLiSQL_expr	*read_sql_construct(int until,
											int until2,
											int until3,
											int until4,	/* ReqID:SRS-PLSQL-BULKCOLLECT */
											const char *expected,
											RawParseMode parsemode,
											bool isexpression,
											bool valid_sql,
											bool trim,
											int *startloc,
											int *endtoken,
											bool check_assignable_datum, /* ReqID:SRS-PLSQL-DSQL */
											bool *ismulticollectmethod /* ReqID:SRS-PLSQL-Collection */);




static	PLiSQL_expr	*read_sql_expression(int until,
											 const char *expected);
static	PLiSQL_expr	*read_sql_expression2(int until, int until2,
											  const char *expected,
											  int *endtoken);
static	PLiSQL_expr	*read_sql_stmt(void);
static	PLiSQL_type	*read_datatype(int tok);
static	PLiSQL_stmt	*make_execsql_stmt(int firsttoken, int location);
static	PLiSQL_stmt_fetch *read_fetch_direction(void);
static	void			 complete_direction(PLiSQL_stmt_fetch *fetch,
											bool *check_FROM);
static	PLiSQL_stmt	*make_return_stmt(int location);
static	PLiSQL_stmt	*make_return_next_stmt(int location);
static	PLiSQL_stmt	*make_return_query_stmt(int location);
static  PLiSQL_stmt	*make_case(int location, PLiSQL_expr *t_expr,
								   List *case_when_list, List *else_stmts);
static	char			*NameOfDatum(PLwdatum *wdatum);
static	void			 check_assignable(PLiSQL_datum *datum, int location);
/* Begin - ReqID:SRS-SQL-PACKAGE */
static void check_packagedatum_assignable(PLiSQL_pkg_datum *datum,
											int location);
/* End - ReqID:SRS-SQL-PACKAGE */


static	void			 read_into_target(PLiSQL_variable **target,
										  bool *strict);
static	PLiSQL_row		*read_into_scalar_list(char *initial_name,
											   PLiSQL_datum *initial_datum,
											   int initial_location);
static	PLiSQL_row		*make_scalar_list1(char *initial_name,
										   PLiSQL_datum *initial_datum,
										   int lineno, int location);
static	void			 check_sql_expr(const char *stmt,
										RawParseMode parseMode, int location);
static	void			 plisql_sql_error_callback(void *arg);
static	PLiSQL_type	*parse_datatype(const char *string, int location);
static	void			 check_labels(const char *start_label,
									  const char *end_label,
									  int end_location);
static	PLiSQL_expr	*read_cursor_args(PLiSQL_var *cursor,
										  int until,
										  int *outdno); /* ReqID:SRS-PLSQL-CURSOR */
static	List			*read_raise_options(void);
static	void			check_raise_parameters(PLiSQL_stmt_raise *stmt);

/* Begin - BUG #M0000304 */
static PLiSQL_stmt *
	build_call_expr(int firsttoken,const char *str, int location)
;
static const ScanKeyword *
ScanReservedKeywordLookup(const char *text,
				  const ScanKeyword *keywords,
				  int num_keywords);
/* End - BUG #M0000304 */
/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
static PLiSQL_nsitem *plisql_ns_lookup_exception(PLiSQL_nsitem *ns_cur,
									const char *except_name);
/* End - ReqID:SRS-PLSQL-EXCEPTION */


/* Begin - BUG#M0000376 */
static void check_oraparam_mode(PLiSQL_datum *datum, int location, char mode);
static void check_package_oraparam_mode(PLiSQL_pkg_datum *pkg_datum, int location, char mode);
/* End - BUG#M0000376 */
/* Begin - ReqID:SRS-PLSQL-Collection */
static int read_collection_type_expr(int start_token, int startlocation,
						StringInfo expr_ds, Oid *pkgoid, int *datum_dno);
static bool is_collect_method(int token);
static char *get_collect_method_name(int token);
static int get_collect_method_tok(char *method_name);
static void read_collect_method(int start_token, const char *varname,
					PLiSQL_type *datatype, StringInfo res,
					int *startlocation);
static PLiSQL_datum *reparse_cword(List *wdnames);
static void read_collection_constructor(PLiSQL_ora_collection_type *type,
								StringInfo ds,
								int *startlocation);
static List *read_collect_variable(PLiSQL_datum *top_datum,
					int startlocation,
					StringInfo var_ds,
					StringInfo method_ds,
					bool *is_method);
static PLiSQL_stmt*get_collect_stmt(PLiSQL_datum *top_datum, int startlocation,
								int lineno);
static int read_out_scalar(char **fieldnames,
				int *varnos,
				char param_info,
				PLiSQL_expr **expr);
/* End - ReqID:SRS-PLSQL-Collection */
/* Begin - ReqID:SRS-PLSQL-BULKCOLLECT */
static void read_bulk_collect_into_target(PLiSQL_variable **target);
static PLiSQL_row *read_bulk_collect_into_udt_list(char *initial_name,
												PLiSQL_datum *initial_datum,
												int initial_location);
/* End - ReqID:SRS-PLSQL-BULKCOLLECT */


%}

%expect 0
%name-prefix="plisql_yy"
%locations

%union {
		ora_core_YYSTYPE			core_yystype;
		/* these fields must match ora_core_YYSTYPE: */
		int						ival;
		char					*str;
		const char				*keyword;

		PLword					word;
		PLcword					cword;
		PLwdatum				wdatum;
		bool					boolean;
		Oid						oid;
		struct
		{
			char *name;
			int  lineno;
		}						varname;
		struct
		{
			char *name;
			int  lineno;
			PLiSQL_datum   *scalar;
			PLiSQL_datum   *row;
		}						forvariable;
		struct
		{
			char *label;
			int  n_initvars;
			int  *initvarnos;
			/* Begin - ReqID:SRS-SQL-PACKAGE */
			bool popname;
			/* End - ReqID:SRS-SQL-PACKAGE */
		}						declhdr;
		struct
		{
			List *stmts;
			char *end_label;
			int   end_label_location;
		}						loop_body;
		List					*list;
		PLiSQL_type			*dtype;
		PLiSQL_datum			*datum;
		PLiSQL_var				*var;
		PLiSQL_expr			*expr;
		PLiSQL_stmt			*stmt;
		PLiSQL_condition		*condition;
		PLiSQL_exception		*exception;
		PLiSQL_exception_block	*exception_block;
		PLiSQL_nsitem			*nsitem;
		PLiSQL_diag_item		*diagitem;
		PLiSQL_stmt_fetch		*fetch;
		PLiSQL_case_when		*casewhen;
		/* Begin - SRS-PLSQL-SUBPROC */
		PLiSQL_function_argitem *func_argitem;
		PLiSQL_subproc_function *subproc_func;
		PLiSQL_subprocfunc_proper *func_proper;
		AccessProperItem *access_proper_item;
		AccessProperItemType access_proper_kind;
		/* End - SRS-PLSQL-SUBPROC */
		/* Begin - ReqID:SRS-PLSQL-CURSOR */
		struct
		{
			char *name;
			int flags;
			List*arglist;
			int	argno;
			bool oracursor;
		}	cursor_heading_info;
		/* End - ReqID:SRS-PLSQL-CURSOR */
		/* Begin - ReqID:SRS-PLSQL-RECORD */
		PLiSQL_ora_record_argitem	*recorditem;
		/* End - ReqID:SRS-PLSQL-RECORD */
}

%type <declhdr> decl_sect
%type <varname> decl_varname
%type <boolean>	decl_const decl_notnull exit_type
%type <expr>	decl_defval decl_cursor_query
%type <dtype>	decl_datatype
%type <oid>		decl_collate
%type <list>	decl_cursor_arglist
%type <nsitem>	decl_aliasitem

%type <expr>	expr_until_semi
%type <expr>	expr_until_then expr_until_loop opt_expr_until_when
%type <expr>	opt_exitcond

%type <var>		cursor_variable
%type <forvariable>	for_variable
%type <ival>	foreach_slice
%type <stmt>	for_control

%type <str>		any_identifier opt_block_label opt_loop_label opt_label
%type <str>		opt_stmt_label	/* ReqID:SRS-PLSQL-GOTO */
%type <str>		option_value

/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
%type <str>		exception_any_identifier
%type <ival> 	SignedIconst
/* End - ReqID:SRS-PLSQL-EXCEPTION */


%type <list>	proc_sect stmt_elsifs stmt_else
%type <loop_body>	loop_body
%type <stmt>	proc_stmt pl_block
%type <stmt>	stmt_assign stmt_if stmt_loop stmt_while stmt_exit
%type <stmt>	stmt_return stmt_raise stmt_assert stmt_execsql
%type <stmt>	stmt_dynexecute stmt_for stmt_perform stmt_call stmt_getdiag
%type <stmt>	stmt_open stmt_fetch stmt_move stmt_close stmt_null
%type <stmt>	stmt_commit stmt_rollback
%type <stmt>	stmt_case stmt_foreach_a
%type <stmt>	stmt_goto	/* ReqID:SRS-PLSQL-GOTO */

%type <list>	proc_exceptions
%type <exception_block> exception_sect
%type <exception>	proc_exception
%type <condition>	proc_conditions proc_condition

%type <casewhen>	case_when
%type <list>	case_when_list opt_case_else

%type <boolean>	getdiag_area_opt
%type <list>	getdiag_list
%type <diagitem> getdiag_list_item
%type <datum>	getdiag_target
%type <ival>	getdiag_item

%type <ival>	opt_scrollable
%type <fetch>	opt_fetch_direction

%type <ival>	opt_transaction_chain

%type <keyword>	unreserved_keyword

/* Begin - SRS-PLSQL-SUBPROC */
%type <keyword> unit_name_keyword
%type <str> ora_function_name unit_name authid_user extral_unit_name
%type <list> func_arg_list func_args procedure_properties_list accessor_list
%type <list> function_result_cache_relies data_source procedure_properties
%type <list> function_properite_list function_properties
%type <func_argitem> func_arg_item
%type <subproc_func>function_heading procedure_heading
%type <func_proper> function_properite_item invoker_rights_clause
%type <func_proper> procedure_properite_item accessible_by_clause default_collation_clause
%type <access_proper_item> accessor
%type <access_proper_kind> unit_kind
%type <keyword> ora_function_is_as
%type <ival> arg_mode
%type <expr> arg_decl_defval
%type <boolean> function_arg_nocopy
/* End - SRS-PLSQL-SUBPROC */

/* Begin - ReqID:SRS-SQL-PROC */
%type <stmt> ora_outermost_pl_block
%type <declhdr> ora_decl_sect
%type <boolean> opt_ora_decl_stmts
/* End - ReqID:SRS-SQL-PROC */

/* Begin - ReqID:SRS-SQL-PACKAGE */
%type <stmt> ora_pl_package
%type <str> ora_first_opt_block_label
%type <boolean> opt_ora_decl_start
/* End - ReqID:SRS-SQL-PACKAGE */

/* Begin - ReqID:SRS-PLSQL-CURSOR */
%type <dtype> ora_rowtype opt_return_ora_rowtype
%type <cursor_heading_info>  cursor_heading
%type <func_argitem>	decl_cursor_arg
%type <list>	decl_cursor_args
/* End - ReqID:SRS-PLSQL-CURSOR */

/* Begin - ReqID:SRS-PLSQL-RECORD */
%type <list>		record_args_list
%type <recorditem>	record_arg_item
/* End - ReqID:SRS-PLSQL-RECORD */

/* Begin - ReqID:SRS-PLSQL-SUBTYPE */
%type <dtype>	subtype_basetype
%type <list>	subtype_range
/* End - ReqID:SRS-PLSQL-SUBTYPE */

/* Begin - ReqID:SRS-PLSQL-Collection */
%type <ival>	varray_limit
%type <keyword> varray_or_varying_array
/* End - ReqID:SRS-PLSQL-Collection */

/*
 * Basic non-keyword token types.  These are hard-wired into the core lexer.
 * They must be listed first so that their numeric codes do not depend on
 * the set of keywords.  Keep this list in sync with backend/parser/ora_gram.y!
 *
 * Some of these are not directly referenced in this file, but they must be
 * here anyway.
 */
%token <str>	IDENT UIDENT FCONST SCONST USCONST BCONST XCONST Op
%token <str>	BFCONST	BDCONST /* BUG:Z093 */
%token <ival>	ICONST PARAM
/* Begin - ReqID:SRS-SQL-ANONYOUT */
%token <str>	ORAPARAM
/* End - ReqID:SRS-SQL-ANONYOUT */

%token			TYPECAST DOT_DOT COLON_EQUALS EQUALS_GREATER
%token			LESS_EQUALS GREATER_EQUALS NOT_EQUALS LESS_LESS GREATER_GREATER /* ReqID:SRS-SQL-PROC */
/* Begin - BUG#M0000201: support question marks as binding variables */ 
%token			QUESTION_MARK;
/* End - BUG#M0000201: support question marks as binding variables */	
%token			OUTER_JOIN	/* ReqID:SRS-SQL-OUTERJOIN(+) */
/* Begin - ReqID:SRS-RECOVERY-FLASHBACK */
%token			AS_LA
/* End - ReqID:SRS-RECOVERY-FLASHBACK */

/*
 * Other tokens recognized by plisql's lexer interface layer (pl_scanner.c).
 */
%token <word>		T_WORD		/* unrecognized simple identifier */
%token <cword>		T_CWORD		/* unrecognized composite identifier */
%token <wdatum>		T_DATUM		/* a VAR, ROW, REC, or RECFIELD variable */

/*
 * Keyword tokens.  Some of these are reserved and some are not;
 * see pl_scanner.c for info.  Be sure unreserved keywords are listed
 * in the "unreserved_keyword" production below.
 */
%token <keyword>	K_ABSOLUTE
/* Begin - SRS-PLSQL-SUBPROC */
%token <keyword>	K_ACCESSIBLE
/* End - SRS-PLSQL-SUBPROC */
%token <keyword>	K_ALIAS
%token <keyword>	K_ALL
%token <keyword>	K_AND
%token <keyword>	K_ARRAY
/* Begin - SRS-PLSQL-SUBPROC */
%token <keyword>	K_AS
/* End - SRS-PLSQL-SUBPROC */
%token <keyword>	K_ASSERT
/* Begin - SRS-PLSQL-SUBPROC */
%token <keyword>	K_AUTHID
/* End - SRS-PLSQL-SUBPROC */
%token <keyword>	K_BACKWARD
%token <keyword>	K_BEGIN
/* Begin - ReqID:SRS-PLSQL-BULKCOLLECT */
%token <keyword>	K_BULK
/* End - ReqID:SRS-PLSQL-BULKCOLLECT */
%token <keyword>	K_BY
%token <keyword>	K_CALL
%token <keyword>	K_CASE
%token <keyword>	K_CHAIN
%token <keyword>	K_CLOSE
%token <keyword>	K_COLLATE
/* Begin - SRS-PLSQL-SUBPROC */
%token <keyword>	K_COLLATION
/* End - SRS-PLSQL-SUBPROC */
/* Begin - ReqID:SRS-PLSQL-BULKCOLLECT */
%token <keyword>	K_COLLECT
/* End - ReqID:SRS-PLSQL-BULKCOLLECT */
%token <keyword>	K_COLUMN
%token <keyword>	K_COLUMN_NAME
%token <keyword>	K_COMMIT
%token <keyword>	K_CONSTANT
%token <keyword>	K_CONSTRAINT
%token <keyword>	K_CONSTRAINT_NAME
%token <keyword>	K_CONTINUE
/* Begin - ReqID:SRS-PLSQL-Collection */
%token <keyword>	K_COUNT
/* End - ReqID:SRS-PLSQL-Collection */
%token <keyword>	K_CURRENT
/* Begin - SRS-PLSQL-SUBPROC */
%token <keyword>	K_CURRENT_USER
/* End - SRS-PLSQL-SUBPROC */
%token <keyword>	K_CURSOR
%token <keyword>	K_DATATYPE
%token <keyword>	K_DEBUG
%token <keyword>	K_DECLARE
%token <keyword>	K_DELETING
%token <keyword>	K_DEFAULT
/* Begin - SRS-PLSQL-SUBPROC */
%token <keyword>	K_DEFINER
/* End - SRS-PLSQL-SUBPROC */
%token <keyword>	K_DELETE	/* ReqID:SRS-PLSQL-Collection */
%token <keyword>	K_DETAIL
/* Begin - SRS-PLSQL-SUBPROC */
%token <keyword>	K_DETERMINISTIC
/* End - SRS-PLSQL-SUBPROC */
%token <keyword>	K_DIAGNOSTICS
%token <keyword>	K_DO
%token <keyword>	K_DUMP
%token <keyword>	K_ELSE
%token <keyword>	K_ELSIF
%token <keyword>	K_END
%token <keyword>	K_ERRCODE
%token <keyword>	K_ERROR
%token <keyword>	K_EXCEPTION
/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
%token <keyword>	K_EXCEPTION_INIT
/* End - ReqID:SRS-PLSQL-EXCEPTION */
%token <keyword>	K_EXECUTE
%token <keyword>	K_EXISTS	/* ReqID:SRS-PLSQL-Collection */
%token <keyword>	K_EXIT
%token <keyword>	K_EXTEND	/* ReqID:SRS-PLSQL-Collection */
%token <keyword>	K_FETCH
%token <keyword>	K_FIRST
%token <keyword>	K_FOR
%token <keyword>	K_FOREACH
%token <keyword>	K_FORWARD
%token <keyword>	K_FROM
/* Begin - SRS-PLSQL-SUBPROC */
%token <keyword>	K_FUNCTION
/* End - SRS-PLSQL-SUBPROC */
%token <keyword>	K_GET
%token <keyword>	K_GOTO		/* ReqID:SRS-PLSQL-GOTO */
%token <keyword>	K_HINT
%token <keyword>	K_IF
%token <keyword>	K_IMMEDIATE	/* BUG#Z1438 */
%token <keyword>	K_IMPORT
%token <keyword>	K_IN
%token <keyword>	K_INDEX /* ReqID:SRS-PLSQL-Collection */
%token <keyword>	K_INSERTING
%token <keyword>	K_INFO
%token <keyword>	K_INSERT
%token <keyword>	K_INTO
%token <keyword>	K_IS
%token <keyword>	K_LAST
%token <keyword>	K_LIMIT	/* ReqID:SRS-PLSQL-Collection */
%token <keyword>	K_LOG
%token <keyword>	K_LOOP
%token <keyword>	K_MERGE		/* K_MERGE */
%token <keyword>	K_MESSAGE
%token <keyword>	K_MESSAGE_TEXT
%token <keyword>	K_MOVE
%token <keyword>	K_NEXT
%token <keyword>	K_NO
/* Begin - SRS-PLSQL-SUBPROC */
%token <keyword>	K_NOCOPY
/* End - SRS-PLSQL-SUBPROC */
%token <keyword>	K_NOT
%token <keyword>	K_NOTICE
%token <keyword>	K_NULL
%token <keyword>	K_OF	/* ReqID:SRS-PLSQL-Collection */
%token <keyword>	K_OPEN
%token <keyword>	K_OPTION
%token <keyword>	K_OR
/* Begin - SRS-PLSQL-SUBPROC */
%token <keyword>	K_OUT
%token <keyword>	K_PACKAGE
%token <keyword>	K_PARALLEL_ENABLE
/* End - SRS-PLSQL-SUBPROC */
%token <keyword>	K_PERFORM
%token <keyword>	K_PG_CONTEXT
%token <keyword>	K_PG_DATATYPE_NAME
%token <keyword>	K_PG_EXCEPTION_CONTEXT
%token <keyword>	K_PG_EXCEPTION_DETAIL
%token <keyword>	K_PG_EXCEPTION_HINT
/* Begin - SRS-PLSQL-SUBPROC */
%token <keyword>	K_PIPELINED
/* End - SRS-PLSQL-SUBPROC*/
/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
%token <keyword>	K_PRAGMA
/* End - ReqID:SRS-PLSQL-EXCEPTION */
%token <keyword>	K_PRINT_STRICT_PARAMS
%token <keyword>	K_PRIOR
/* Begin - SRS-PLSQL-SUBPROC */
%token <keyword>	K_PROCEDURE
/* End - SRS-PLSQL-SUBPROC */
%token <keyword>	K_QUERY
%token <keyword>	K_RAISE
/* Begin - ReqID:SRS-PLSQL-SUBTYPE */
%token <keyword>	K_RANGE
/* End - ReqID:SRS-PLSQL-SUBTYPE */
/* Begin - ReqID:SRS-PLSQL-RECORD */
%token <keyword>	K_RECORD
/* End - ReqID:SRS-PLSQL-RECORD */
/* Begin - ReqID:SRS-PLSQL-CURSOR */
%token <keyword>	K_REF
/* End - ReqID:SRS-PLSQL-CURSOR */
%token <keyword>	K_RELATIVE
/* Begin - SRS-PLSQL-SUBPROC */
%token <keyword>	K_RELIES_ON
%token <keyword>	K_RESULT_CACHE
/* End - SRS-PLSQL-SUBPROC*/
%token <keyword>	K_RETURN
%token <keyword>	K_RETURNED_SQLSTATE
/* Begin - ReqID:SRS-PLSQL-DSQL */
%token <keyword>	K_RETURNING
/* End - ReqID:SRS-PLSQL-DSQL */
%token <keyword>	K_REVERSE
%token <keyword>	K_ROLLBACK
%token <keyword>	K_ROW_COUNT
%token <keyword>	K_ROWNUM	/* BUG#M0000450 */
%token <keyword>	K_ROWTYPE
%token <keyword>	K_SCHEMA
%token <keyword>	K_SCHEMA_NAME
%token <keyword>	K_SCROLL
/* Begin - ReqID:SRS-PLSQL-CURSOR */
%token <keyword>	K_SELECT
/* End - ReqID:SRS-PLSQL-CURSOR */
%token <keyword>	K_SLICE
%token <keyword>	K_SQLSTATE
%token <keyword>	K_STACKED
%token <keyword>	K_STRICT
/* Begin - ReqID:SRS-PLSQL-SUBTYPE */
%token <keyword>	K_SUBTYPE
/* End - ReqID:SRS-PLSQL-SUBTYPE */
%token <keyword>	K_TABLE
%token <keyword>	K_TABLE_NAME
%token <keyword>	K_THEN
%token <keyword>	K_TO
/* Begin - SRS-PLSQL-SUBPROC */
%token <keyword>	K_TRIGGER
/* End - SRS-PLSQL-SUBPROC */
%token <keyword>	K_TRIM	/* ReqID:SRS-PLSQL-Collection */
%token <keyword>	K_TYPE
%token <keyword>	K_UPDATING
%token <keyword>	K_USE_COLUMN
%token <keyword>	K_USE_VARIABLE
%token <keyword>	K_USING
/* Begin - SRS-PLSQL-SUBPROC*/
%token <keyword>	K_USING_NLS_COMP
/* End - SRS-PLSQL-SUBPROC */
/* Begin - BUG#Z375 */
%token <keyword>	K_VALUES
/* End - BUG#Z375 */
%token <keyword>	K_VARIABLE_CONFLICT
/* Begin - ReqID:SRS-PLSQL-Collection */
%token <keyword>	K_VARRAY
%token <keyword>	K_VARYING
/* End - ReqID:SRS-PLSQL-Collection */
%token <keyword>	K_WARNING
%token <keyword>	K_WHEN
%token <keyword>	K_WHILE


/* Begin - ReqID:SRS-PLSQL-CURSOR */
%nonassoc K_TYPE
%left K_CURSOR
%nonassoc K_ALIAS K_COLLATE K_CONSTANT K_DEFAULT K_NO K_SCROLL
/* End - ReqID:SRS-PLSQL-CURSOR */

/* Begin - ReqID:SRS-PLSQL-SUBTYPE */
%nonassoc K_SUBTYPE
/* End - ReqID:SRS-PLSQL-SUBTYPE */

%%

pl_function		: comp_options ora_outermost_pl_block opt_semi	/* ReqID:SRS-SQL-PROC */
					{
						plisql_parse_result = (PLiSQL_stmt_block *) $2;
					}
				/* Begin - ReqID:SRS-SQL-PACKAGE */
				| comp_options ora_pl_package opt_semi
					{
						plisql_parse_result = (PLiSQL_stmt_block *) $2;
					}
				/* End - ReqID:SRS-SQL-PACKAGE */
				;

comp_options	:
				| comp_options comp_option
				;

comp_option		: '#' K_OPTION K_DUMP
					{
						plisql_DumpExecTree = true;
					}
				| '#' K_PRINT_STRICT_PARAMS option_value
					{
						if (strcmp($3, "on") == 0)
							plisql_curr_compile->print_strict_params = true;
						else if (strcmp($3, "off") == 0)
							plisql_curr_compile->print_strict_params = false;
						else
							elog(ERROR, "unrecognized print_strict_params option %s", $3);
					}
				| '#' K_VARIABLE_CONFLICT K_ERROR
					{
						plisql_curr_compile->resolve_option = PLISQL_RESOLVE_ERROR;
					}
				| '#' K_VARIABLE_CONFLICT K_USE_VARIABLE
					{
						plisql_curr_compile->resolve_option = PLISQL_RESOLVE_VARIABLE;
					}
				| '#' K_VARIABLE_CONFLICT K_USE_COLUMN
					{
						plisql_curr_compile->resolve_option = PLISQL_RESOLVE_COLUMN;
					}
				;

option_value : T_WORD
				{
					$$ = $1.ident;
				}
			 | unreserved_keyword
				{
					$$ = pstrdup($1);
				}

opt_semi		:
				| ';'
				;

/* Begin - ReqID:SRS-SQL-PROC */
/*
 * The declaration section of the outermost block
 * in Oracle does not have the DECLARE keyword.
 */
ora_outermost_pl_block: ora_decl_sect K_BEGIN proc_sect exception_sect K_END opt_label
					{
						PLiSQL_stmt_block *new;

						/* Begin - ReqID:SRS-SQL-PACKAGE */
						/* package specification doesn't be allowed to include begin */
						if (plisql_compile_packageitem != NULL &&
							!plisql_compile_packageitem->finish_compile_special)
							yyerror("syntax error for package specificaion have a body");

						/* we only consider package body level */
						if (plisql_compile_packageitem != NULL &&
							cur_compile_func_level == 0 &&
							plisql_saved_compile[cur_compile_func_level] == (&plisql_compile_packageitem->source)) /* BUG#M0000167 */
						{
							plisql_check_package_refcursor_var(plisql_compile_packageitem->source.sql_varno); /* ReqID:SRS-PLSQL-CURSOR */
							plisql_compile_packageitem->last_globaldno =
										plisql_get_package_last_globaldno($4 == NULL ? -1 : $4->sqlstate_varno,
																		$4 == NULL ? -1 : $4->sqlerrm_varno);
							/* Begin - ReqID:SRS-PLSQL-CURSOR */
							plisql_handle_package_explicit_cursor_retdesc(plisql_compile_packageitem->last_globaldno);
							/* End - ReqID:SRS-PLSQL-CURSOR */
						}
						/* End - ReqID:SRS-SQL-PACKAGE */

						new = palloc0(sizeof(PLiSQL_stmt_block));

						new->cmd_type	= PLISQL_STMT_BLOCK;
						new->lineno		= plisql_location_to_lineno(@2);
						new->stmtid		= ++plisql_curr_compile->nstatements;
						new->label		= $1.label;
						new->n_initvars = $1.n_initvars;
						new->initvarnos = $1.initvarnos;
						new->body		= $3;
						new->exceptions	= $4;

						check_labels($1.label, $6, @6);
						/* Begin - ReqID:SRS-SQL-PACKAGE */
						/*
						 * no decalre keyword, we don't add lable block namespace
						 */
						if ($1.popname)
							plisql_ns_pop();
						/* End - ReqID:SRS-SQL-PACKAGE */

						$$ = (PLiSQL_stmt *)new;
					}
				;

/* Begin - ReqID:SRS-SQL-PACKAGE */
/*
 * package and its body define for which
 * has no init block
 */
ora_pl_package: ora_decl_sect K_END opt_label
			{
				PLiSQL_stmt_block *new;

				if (plisql_compile_packageitem == NULL)
					yyerror("syntax error");

				/* Begin - ReqID:SRS-PLSQL-AUTONOMOUSTRANSACTION */
				/* package can not set autonomous, but subproc in package can. */
				if (plisql_curr_compile->autonomous)
					elog(ERROR, "Pragma AUTONOMOUS_TRANSACTION cannot be specified here");
				/* End - ReqID:SRS-PLSQL-AUTONOMOUSTRANSACTION */

				new = palloc0(sizeof(PLiSQL_stmt_block));

				new->cmd_type	= PLISQL_STMT_BLOCK;
				new->lineno		= plisql_location_to_lineno(@2);
				new->stmtid		= ++plisql_curr_compile->nstatements;
				new->label	= $1.label;
				new->n_initvars = $1.n_initvars;
				new->initvarnos = $1.initvarnos;

				/* Begin - BUG#M0000113 */
				/*
				 * compile package body doesn't set special_cur
				 */
				if (!plisql_compile_packageitem->finish_compile_special)
					plisql_compile_packageitem->special_cur = plisql_ns_top();
				/* End - BUG#M0000113 */
				plisql_IdentifierLookup = IDENTIFIER_LOOKUP_NORMAL;
				/* Remember variables declared in decl_stmts */
				check_labels($1.label, $3, @3);
				plisql_check_package_refcursor_var(plisql_compile_packageitem->source.sql_varno); /* ReqID:SRS-PLSQL-CURSOR */
				plisql_compile_packageitem->last_globaldno =
										plisql_get_package_last_globaldno(-1, -1);
				/* Begin - ReqID:SRS-PLSQL-CURSOR */
				plisql_handle_package_explicit_cursor_retdesc(plisql_compile_packageitem->last_globaldno);
				/* End - ReqID:SRS-PLSQL-CURSOR */
				if (!plisql_compile_packageitem->finish_compile_special)
					plisql_compile_packageitem->status =
								plisql_set_package_compile_status();
				/*
				 * no decalre keyword, we don't add lable block namespace
				 */
				if ($1.popname)
					plisql_ns_pop();

				$$ = (PLiSQL_stmt *)new;
			}
/* End - ReqID:SRS-SQL-PACKAGE */
;
/* Begin - ReqID:SRS-SQL-PACKAGE */
ora_decl_sect: ora_first_opt_block_label opt_ora_decl_start
				{
					if ($2)
					{
						if ($1 == NULL)
						{
							plisql_ns_push(NULL, PLISQL_LABEL_BLOCK);
						}
					}
				}
				opt_ora_decl_stmts
				{
					if ($4)
					{
						plisql_IdentifierLookup = IDENTIFIER_LOOKUP_NORMAL;
						$$.label	  = ($1 == NULL ?  plisql_curr_compile->namelabel : $1);
						if ($2 && $1 == NULL)
							$$.popname = true;
						else
							$$.popname = false;
						/* Remember variables declared in decl_stmts */
						$$.n_initvars = plisql_add_initdatums(&($$.initvarnos));
					}
					else
					{
						plisql_IdentifierLookup = IDENTIFIER_LOOKUP_NORMAL;
						$$.label	  = ($1 == NULL ?  plisql_curr_compile->namelabel : $1);
						$$.n_initvars = 0;
						if ($2 && $1 == NULL)
							$$.popname = true;
						else
							$$.popname = false;
						$$.initvarnos = NULL;
					}
				}
		;

ora_first_opt_block_label:
					{
						$$ = NULL;
					}
				| LESS_LESS any_identifier GREATER_GREATER
					{
						plisql_ns_push($2, PLISQL_LABEL_BLOCK);
						$$ = $2;
					}
				;
/* End - ReqID:SRS-SQL-PACKAGE */


opt_ora_decl_start: K_DECLARE
					{
						/* Forget any variables created before block */
						plisql_add_initdatums(NULL);
						/*
						 * Disable scanner lookup of identifiers while
						 * we process the decl_stmts
						 */
						plisql_IdentifierLookup = IDENTIFIER_LOOKUP_DECLARE;
						/* Begin - ReqID:SRS-SQL-PACKAGE */
						$$ = true;
						/* End - ReqID:SRS-SQL-PACKAGE */
					}
				| /*EMPTY*/
					{
						/* Forget any variables created before block */
						plisql_add_initdatums(NULL);
						/*
						 * Disable scanner lookup of identifiers while
						 * we process the decl_stmts
						 */
						plisql_IdentifierLookup = IDENTIFIER_LOOKUP_DECLARE;
						/* Begin - ReqID:SRS-SQL-PACKAGE */
						$$ = false;
						/* End - ReqID:SRS-SQL-PACKAGE */
					}
				;

opt_ora_decl_stmts:
				ora_decl_stmts
					{
						$$ = true;
					}
				| /*EMPTY*/
					{
						$$ = false;
					}

ora_decl_stmts: ora_decl_stmts ora_decl_stmt
				| ora_decl_stmt
				;

ora_decl_stmt: decl_statement
				/* Begin - ReqID:SRS-PLSQL-AUTONOMOUSTRANSACTION */
				| K_PRAGMA any_identifier ';'
					{
						/* only support autonomous_transaction pragma currently. */
						if (pg_strcasecmp($2, "autonomous_transaction") == 0)
						{
							/* check autonomous flags exists. */
							if (plisql_curr_compile->autonomous)
								elog(ERROR, "Pragma AUTONOMOUS_TRANSACTION cannot be declared twice");
							plisql_curr_compile->autonomous = true;
						}
						else
							elog(ERROR, "Pragma %s is not a supported pragma", $2);
					}
				/* End - ReqID:SRS-PLSQL-AUTONOMOUSTRANSACTION */
				;
/* End - ReqID:SRS-SQL-PROC */
	
pl_block		: decl_sect K_BEGIN proc_sect exception_sect K_END opt_label
					{
						PLiSQL_stmt_block *new;

						new = palloc0(sizeof(PLiSQL_stmt_block));

						new->cmd_type	= PLISQL_STMT_BLOCK;
						new->lineno		= plisql_location_to_lineno(@2);
						new->stmtid		= ++plisql_curr_compile->nstatements;
						new->label		= $1.label;
						new->n_initvars = $1.n_initvars;
						new->initvarnos = $1.initvarnos;
						new->body		= $3;
						new->exceptions	= $4;

						check_labels($1.label, $6, @6);
						plisql_ns_pop();
						/* Begin - ReqID:SRS-PLSQL-GOTO */
						plisql_lscope_pop();
						plisql_register_label($1.label);
						/* End - ReqID:SRS-PLSQL-GOTO */

						$$ = (PLiSQL_stmt *)new;
					}
				;


decl_sect		: opt_block_label
					{
						/* done with decls, so resume identifier lookup */
						plisql_IdentifierLookup = IDENTIFIER_LOOKUP_NORMAL;
						$$.label	  = $1;
						$$.n_initvars = 0;
						$$.initvarnos = NULL;
					}
				| opt_block_label decl_start
					{
						plisql_IdentifierLookup = IDENTIFIER_LOOKUP_NORMAL;
						$$.label	  = $1;
						$$.n_initvars = 0;
						$$.initvarnos = NULL;
					}
				| opt_block_label decl_start decl_stmts
					{
						plisql_IdentifierLookup = IDENTIFIER_LOOKUP_NORMAL;
						$$.label	  = $1;
						/* Remember variables declared in decl_stmts */
						$$.n_initvars = plisql_add_initdatums(&($$.initvarnos));
					}
				;

decl_start		: K_DECLARE
					{
						/* Forget any variables created before block */
						plisql_add_initdatums(NULL);
						/*
						 * Disable scanner lookup of identifiers while
						 * we process the decl_stmts
						 */
						plisql_IdentifierLookup = IDENTIFIER_LOOKUP_DECLARE;
					}
				;

decl_stmts		: decl_stmts decl_stmt
				| decl_stmt
				;

decl_stmt		: decl_statement
				| K_DECLARE
					{
						/* We allow useless extra DECLAREs */
					}
				| LESS_LESS any_identifier GREATER_GREATER
					{
						/*
						 * Throw a helpful error if user tries to put block
						 * label just before BEGIN, instead of before DECLARE.
						 */
						ereport(ERROR,
								(errcode(ERRCODE_SYNTAX_ERROR),
								 errmsg("block label must be placed before DECLARE, not after"),
								 parser_errposition(@1)));
					}
				;

decl_statement	: decl_varname decl_const decl_datatype decl_collate decl_notnull decl_defval
					{
						PLiSQL_variable	*var;

						/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
						/*
						 * exception var only support xxx exception
						 * ora-6550
						 */
						if (ISEXCEPTIONOID($3->typoid))
						{
							if ($2 || $4 != InvalidOid ||
								$5 || $6 != NULL)
								ereport(ERROR,
									(errcode(ERRCODE_SYNTAX_ERROR),
									errmsg("exception variable should only declare as xxxx exception"),
									parser_errposition(@1)));
						}
						/* End - ReqID:SRS-PLSQL-EXCEPTION */

						/*
						 * If a collation is supplied, insert it into the
						 * datatype.  We assume decl_datatype always returns
						 * a freshly built struct not shared with other
						 * variables.
						 */
						if (OidIsValid($4))
						{
							if (!OidIsValid($3->collation))
								ereport(ERROR,
										(errcode(ERRCODE_DATATYPE_MISMATCH),
										 errmsg("collations are not supported by type %s",
												format_type_be($3->typoid)),
										 parser_errposition(@4)));
							$3->collation = $4;
						}

						var = plisql_build_variable($1.name, $1.lineno,
													 $3, true);
						var->isconst = $2;
						var->notnull = $5;
						/* Begin - ReqID:SRS-PLSQL-TYPE */
						/*
						 * IF decl_datatype is variable%TYPE,
						 * and the variable has a NOT NULL constraint,
						 * var inherits the NOT NULL constraint(unless the decl_datatype is a table.column%TYPE).
						 */
						if (!var->notnull &&
							((PLiSQL_type *)($3))->ttype != PLISQL_TTYPE_COLLECT_TYPE && /* ReqID:SRS-PLSQL-Collection */
							$3->notnull == true)
							var->notnull = $3->notnull;
						/* End - ReqID:SRS-PLSQL-TYPE */
						var->default_val = $6;

						/*
						 * The combination of NOT NULL without an initializer
						 * can't work, so let's reject it at compile time.
						 */
						if (var->notnull && var->default_val == NULL)
							ereport(ERROR,
									(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
									 errmsg("variable \"%s\" must have a default value, since it's declared NOT NULL",
											var->refname),
									 parser_errposition(@5)));
					}
				| decl_varname K_ALIAS K_FOR decl_aliasitem ';'
					{
						plisql_ns_additem($4->itemtype,
										   $4->itemno, $1.name);
					}
				/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
				| K_PRAGMA K_EXCEPTION_INIT '(' any_identifier ',' SignedIconst ')' ';'
					{
						PLiSQL_var *exception_var;
						PLiSQL_nsitem *nsi;

						nsi = plisql_ns_lookup(plisql_ns_top(), true,
													$4, NULL, NULL, NULL);
						/* raise ORA-06550 */
						if (nsi == NULL)
							ereport(ERROR,
									(errcode(ERRCODE_SYNTAX_ERROR),
									errmsg("PRAGMA EXCEPTION_INIT of %s must follow declaration of its"
									" exception in the same declarative part", $4),
									parser_errposition(@4)));
						if (nsi->itemtype != PLISQL_NSTYPE_VAR)
							ereport(ERROR,
									(errcode(ERRCODE_SYNTAX_ERROR),
									errmsg("'%s' must be declared as an exception", $4),
									parser_errposition(@4)));
						exception_var = (PLiSQL_var *) plisql_Datums[nsi->itemno];
						if (!ISEXCEPTIONOID(exception_var->datatype->typoid))
							ereport(ERROR,
									(errcode(ERRCODE_SYNTAX_ERROR),
									errmsg("'%s' must be declared as an exception", $4),
									parser_errposition(@4)));
						plisql_set_udt_exception_errcode(exception_var, $6);
					}
				/* End - ReqID:SRS-PLSQL-EXCEPTION */
				/* Begin - ReqID:SRS-PLSQL-CURSOR */
				| K_TYPE decl_varname K_IS K_REF K_CURSOR opt_return_ora_rowtype ';'
					{
						plisql_build_udt_cursor_type($2.name, $6, $2.lineno, true);
					}
				| cursor_heading K_RETURN ora_rowtype ';'
					{
						PLiSQL_var *new;
						PLiSQL_type *cursor_type;

						/*
						 * Check to make sure name isn't already declared
						 * in the current block.
						 */
						if (plisql_ns_lookup(plisql_ns_top(), true,
											  $1.name, NULL, NULL,
											  NULL) != NULL)
							yyerror("duplicate declaration");

						/*
						 * only support oracursor
						 */
						if (!$1.oracursor)
							yyerror("pg cursor doesn't support declaration");

						cursor_type = plisql_build_datatype(SYS_REFCURSOROID,
															  -1,
															  InvalidOid,
															  NULL);
						new = (PLiSQL_var *)
							plisql_build_variable($1.name, plisql_location_to_lineno(@1),
											   cursor_type,
											   true);
						new->cursor_explicit_rettype = $3;
						new->arglist = $1.arglist;
						new->is_explicit_cursor = true;
						new->cursor_rettype_tupledesc = plisql_get_rowtype_tupledesc($3);
				}
				| cursor_heading opt_return_ora_rowtype
					{
						plisql_ns_push($1.name, PLISQL_LABEL_OTHER);

						if ($1.arglist != NIL)
							$1.argno = plisql_build_cursor_args($1.arglist,
												plisql_location_to_lineno(@1));
						else
							$1.argno = -1;
					}
				  decl_is_for decl_cursor_query
				 /* End - ReqID:SRS-PLSQL-CURSOR */
					{
						PLiSQL_var *new;
						PLiSQL_nsitem *nsitem;

						/* pop local namespace for cursor args */
						plisql_ns_pop();

						nsitem = plisql_ns_lookup(plisql_ns_top(), true,
											  $1.name, NULL, NULL,
											  NULL);
						if (nsitem == NULL)
						{
							PLiSQL_type *cursor_type;

							if ($1.oracursor)
								cursor_type = plisql_build_datatype(SYS_REFCURSOROID,
															  -1,
															  InvalidOid,
															  NULL);
							else
								cursor_type = plisql_build_datatype(REFCURSOROID,
													  -1,
													  InvalidOid,
													  NULL);
							new = (PLiSQL_var *) plisql_build_variable($1.name,
													plisql_location_to_lineno(@1),
												   cursor_type,
												   true);
							new->arglist = $1.arglist;
							new->cursor_explicit_rettype = $2;
							new->is_explicit_cursor = true;
						}
						else if (nsitem->itemtype == PLISQL_NSTYPE_VAR && $1.oracursor)
						{
							int retno;

							new = (PLiSQL_var *) plisql_Datums[nsitem->itemno];

							if (!ISORACURSOROID(new->datatype->typoid))
								yyerror("duplicate declaration");
							if (new->cursor_explicit_expr != NULL)
								yyerror("duplicate declaration");

							retno = plisql_check_cursor_define(new, $2, $1.arglist);

							if (retno != 0)
								yyerror("duplicate declaration");
						}
						else
							yyerror("duplicate declaration");

						new->cursor_explicit_expr = $5;
						new->cursor_explicit_argrow = $1.argno; /* ReqID:SRS-PLSQL-CURSOR */
						new->cursor_options = CURSOR_OPT_FAST_PLAN | $1.flags;
						/* Begin - ReqID:SRS-PLSQL-CURSOR */
						if ($2 && new->cursor_rettype_tupledesc == NULL)
							new->cursor_rettype_tupledesc = plisql_get_rowtype_tupledesc($2);
						/* End - ReqID:SRS-PLSQL-CURSOR */
					}
				/* Begin - SRS-PLSQL-SUBPROC */
				/* function or procedure declare or define */
				| function_heading function_properties ';'
					{
						/* check does it duplicate declare */
						if ($1->has_declare)
							yyerror("duplicate declaration");
						$1->has_declare = true;
						/* check function properties */
						plisql_check_subprocfunc_properties($1, $2, true);
						if ($2 != NIL)
							list_free($2);
					}
				| function_heading function_properties ora_function_is_as
					{
						int found_varno = plisql_curr_compile->found_varno;

						plisql_check_subprocfunc_properties($1,$2, false);
						if ($2 != NIL)
							list_free($2);

						$1->lastoutsubprocfno = plisql_nsubprocFuncs;
						/* Begin - BUG#M0000168 */
						$1->lastoutvardno = plisql_nDatums;
						plisql_push_subproc_info();
						/* End - BUG#M0000168 */
						plisql_start_label_compile($1->func_name);	/* ReqID:SRS-PLSQL-GOTO */
						$1->global_cur = plisql_ns_top();
						plisql_ns_push($1->func_name, PLISQL_LABEL_BLOCK);
						/* Begin - BUG#M0000168 */
						plisql_start_datums();
						/* End - BUG#M0000168 */
						plisql_start_subproc_func();
						plisql_curr_compile = $1->function;
						plisql_build_variable_from_funcargs($1, true, NULL, found_varno);
						plisql_add_initdatums(NULL);
					}
				ora_outermost_pl_block ';'
					{
						StringInfoData ds;

						plisql_set_subprocfunc_action($1, (PLiSQL_stmt_block *) $5);
						plisql_ns_pop();
						plisql_finish_label_compile();	/* ReqID:SRS-PLSQL-GOTO */
						plisql_finish_datums($1->function);
						plisql_finish_subproc_func($1->function);
						/* Begin - BUG#M0000168 */
						plisql_pop_subproc_info();
						/* End - BUG#M0000168 */
						initStringInfo(&ds);
						plisql_append_source_text(&ds, @5 + 2, @6);
						$1->src = ds.data;
						plisql_IdentifierLookup = IDENTIFIER_LOOKUP_DECLARE;
					}
				| procedure_heading procedure_properties ';'
					{
						/* check does it duplicate declare */
						if ($1->has_declare)
							yyerror("duplicate declaration");
						plisql_check_subprocfunc_properties($1,$2, false);
						if ($2 != NIL)
							list_free($2);
						$1->has_declare = true;
						$1->properties = $2;
					}
				| procedure_heading procedure_properties ora_function_is_as
					{
						int found_varno = plisql_curr_compile->found_varno;

						plisql_check_subprocfunc_properties($1,$2, false);
						if ($2 != NIL)
							list_free($2);

						$1->lastoutsubprocfno = plisql_nsubprocFuncs;
						/* Begin - BUG#M0000168 */
						$1->lastoutvardno = plisql_nDatums;
						plisql_push_subproc_info();
						/* End - BUG#M0000168 */
						plisql_start_label_compile($1->func_name);	/* ReqID:SRS-PLSQL-GOTO */
						$1->global_cur = plisql_ns_top();
						plisql_ns_push($1->func_name, PLISQL_LABEL_BLOCK);
						/* Begin - BUG#M0000168 */
						plisql_start_datums();
						/* End - BUG#M0000168 */
						plisql_start_subproc_func();
						plisql_curr_compile = $1->function;
						plisql_build_variable_from_funcargs($1, true, NULL, found_varno);
						plisql_add_initdatums(NULL);
					}
				ora_outermost_pl_block ';'
					{
						StringInfoData ds;

						plisql_set_subprocfunc_action($1, (PLiSQL_stmt_block *) $5);
						plisql_ns_pop();
						plisql_finish_label_compile();	/* ReqID:SRS-PLSQL-GOTO */
						plisql_finish_datums($1->function);
						plisql_finish_subproc_func($1->function);
						/* Begin - BUG#M0000168 */
						plisql_pop_subproc_info();
						/* End - BUG#M0000168 */
						initStringInfo(&ds);
						plisql_append_source_text(&ds, @5 + 2, @6);
						$1->src = ds.data;
						plisql_IdentifierLookup = IDENTIFIER_LOOKUP_DECLARE;
					}
				/* End - SRS-PLSQL-SUBPROC */
				/* Begin - ReqID:SRS-PLSQL-RECORD */
				| K_TYPE decl_varname K_IS K_RECORD '(' record_args_list ')' ';'
					{
						plisql_build_ora_rec_type($2.name, $2.lineno, $6);
					}
				/* End - ReqID:SRS-PLSQL-RECORD */

				/* Begin - ReqID:SRS-PLSQL-SUBTYPE */
				| K_SUBTYPE decl_varname K_IS subtype_basetype subtype_range decl_notnull ';'
					{
						plisql_build_subtype($2.name, $2.lineno, $4, $5, $6);
					}
				/* End - ReqID:SRS-PLSQL-SUBTYPE */

				/* Begin - ReqID:SRS-PLSQL-Collection */
				| K_TYPE decl_varname K_IS K_TABLE K_OF decl_datatype decl_notnull K_INDEX K_BY decl_datatype ';'
					{
						plisql_build_ora_collection_type($2.name, $2.lineno, ASSOCIATIVE_ARRAYOID, TYPTYPE_ASSOCIATIVE_ARRAY,
														$10, $6, 0, $7);
					}
				| K_TYPE decl_varname K_IS K_TABLE K_OF decl_datatype decl_notnull ';'
					{
						PLiSQL_type	   *keytype;

						keytype = plisql_build_datatype(INT4OID, -1, InvalidOid, NULL);
						plisql_build_ora_collection_type($2.name, $2.lineno, NESTED_TABLEOID, TYPTYPE_NESTEDTABLE,
														keytype, $6, 0, $7);
					}
				| K_TYPE decl_varname K_IS varray_or_varying_array '(' varray_limit ')' K_OF decl_datatype decl_notnull ';'
					{
						PLiSQL_type	   *keytype;

						keytype = plisql_build_datatype(INT4OID, -1, InvalidOid, NULL);
						plisql_build_ora_collection_type($2.name, $2.lineno, VARRAYOID, TYPTYPE_VARRAY,
														keytype, $9, $6, $10);
					}
				/* End - ReqID:SRS-PLSQL-Collection */
				;

/* Begin - ReqID:SRS-PLSQL-SUBTYPE */
subtype_basetype: decl_datatype
				{
					$$ = $1;
				}
			;

subtype_range : 
				{
					$$ = NULL;
				}
			| K_RANGE SignedIconst DOT_DOT SignedIconst
				{

					$$ = list_make2_int($2, $4);
				}
			;

/* End - ReqID:SRS-PLSQL-SUBTYPE */

/* Begin - ReqID:SRS-PLSQL-CURSOR */
cursor_heading: ora_function_name opt_scrollable K_CURSOR decl_cursor_args
					{
						$$.name = $1;
						$$.flags = $2;
						$$.arglist = $4;
						$$.argno = -1;
						$$.oracursor = false;
					}
				| K_CURSOR ora_function_name decl_cursor_args
					{
						$$.name = $2;
						$$.flags = 0;
						$$.arglist = $3;
						$$.argno = -1;
						$$.oracursor = true;
					}
			;

ora_rowtype: decl_datatype
				{
					$$ = $1;
				}
	;

opt_return_ora_rowtype : K_RETURN ora_rowtype
					{
						$$ = $2;
					}
				| /* EMPTY */ { $$ = NULL;}
				;

/* End - ReqID:SRS-PLSQL-CURSOR */

/* Begin - ReqID:SRS-PLSQL-Collection */
varray_or_varying_array: K_VARRAY				{ $$ = $1; }
						| K_ARRAY				{ $$ = $1; }
						| K_VARYING K_ARRAY		{ $$ = $2; }
						;
varray_limit	: ICONST
					{
						$$ = $1;
					}
					;
/* End - ReqID:SRS-PLSQL-Collection */

opt_scrollable :
					{
						$$ = 0;
					}
				| K_NO K_SCROLL
					{
						$$ = CURSOR_OPT_NO_SCROLL;
					}
				| K_SCROLL
					{
						$$ = CURSOR_OPT_SCROLL;
					}
				;

decl_cursor_query :
					{
						$$ = read_sql_stmt();
					}
				;

decl_cursor_args :
					{
						$$ = NULL;
					}
				| '(' decl_cursor_arglist ')'
					{
						$$ = $2;
					}
				;

decl_cursor_arglist : decl_cursor_arg
					{
						$$ = list_make1($1);
					}
				| decl_cursor_arglist ',' decl_cursor_arg
					{
						$$ = lappend($1, $3);
					}
				;

decl_cursor_arg : func_arg_item /* ReqID:SRS-PLSQL-CURSOR */
					{
						$$ = $1;
					}
				;

decl_is_for		:	K_IS |		/* Oracle */
					K_FOR;		/* SQL standard */

decl_aliasitem	: T_WORD
					{
						PLiSQL_nsitem *nsi;

						nsi = plisql_ns_lookup(plisql_ns_top(), false,
												$1.ident, NULL, NULL,
												NULL);
						if (nsi == NULL)
							ereport(ERROR,
									(errcode(ERRCODE_UNDEFINED_OBJECT),
									 errmsg("variable \"%s\" does not exist",
											$1.ident),
									 parser_errposition(@1)));
						$$ = nsi;
					}
				| unreserved_keyword
					{
						PLiSQL_nsitem *nsi;

						nsi = plisql_ns_lookup(plisql_ns_top(), false,
												$1, NULL, NULL,
												NULL);
						if (nsi == NULL)
							ereport(ERROR,
									(errcode(ERRCODE_UNDEFINED_OBJECT),
									 errmsg("variable \"%s\" does not exist",
											$1),
									 parser_errposition(@1)));
						$$ = nsi;
					}
				| T_CWORD
					{
						PLiSQL_nsitem *nsi;

						if (list_length($1.idents) == 2)
							nsi = plisql_ns_lookup(plisql_ns_top(), false,
													strVal(linitial($1.idents)),
													strVal(lsecond($1.idents)),
													NULL,
													NULL);
						else if (list_length($1.idents) == 3)
							nsi = plisql_ns_lookup(plisql_ns_top(), false,
													strVal(linitial($1.idents)),
													strVal(lsecond($1.idents)),
													strVal(lthird($1.idents)),
													NULL);
						else
							nsi = NULL;
						if (nsi == NULL)
							ereport(ERROR,
									(errcode(ERRCODE_UNDEFINED_OBJECT),
									 errmsg("variable \"%s\" does not exist",
											NameListToString($1.idents)),
									 parser_errposition(@1)));
						$$ = nsi;
					}
				;

decl_varname	: T_WORD
					{
						$$.name = $1.ident;
						$$.lineno = plisql_location_to_lineno(@1);
						/*
						 * Check to make sure name isn't already declared
						 * in the current block.
						 */
						if (plisql_ns_lookup(plisql_ns_top(), true,
											  $1.ident, NULL, NULL,
											  NULL) != NULL)
							yyerror("duplicate declaration");

						if (plisql_curr_compile->extra_warnings & PLISQL_XCHECK_SHADOWVAR ||
							plisql_curr_compile->extra_errors & PLISQL_XCHECK_SHADOWVAR)
						{
							PLiSQL_nsitem *nsi;
							nsi = plisql_ns_lookup(plisql_ns_top(), false,
													$1.ident, NULL, NULL, NULL);
							if (nsi != NULL)
								ereport(plisql_curr_compile->extra_errors & PLISQL_XCHECK_SHADOWVAR ? ERROR : WARNING,
										(errcode(ERRCODE_DUPLICATE_ALIAS),
										 errmsg("variable \"%s\" shadows a previously defined variable",
												$1.ident),
										 parser_errposition(@1)));
						}

					}
				| unreserved_keyword
					{
						$$.name = pstrdup($1);
						$$.lineno = plisql_location_to_lineno(@1);
						/*
						 * Check to make sure name isn't already declared
						 * in the current block.
						 */
						if (plisql_ns_lookup(plisql_ns_top(), true,
											  $1, NULL, NULL,
											  NULL) != NULL)
							yyerror("duplicate declaration");

						if (plisql_curr_compile->extra_warnings & PLISQL_XCHECK_SHADOWVAR ||
							plisql_curr_compile->extra_errors & PLISQL_XCHECK_SHADOWVAR)
						{
							PLiSQL_nsitem *nsi;
							nsi = plisql_ns_lookup(plisql_ns_top(), false,
													$1, NULL, NULL, NULL);
							if (nsi != NULL)
								ereport(plisql_curr_compile->extra_errors & PLISQL_XCHECK_SHADOWVAR ? ERROR : WARNING,
										(errcode(ERRCODE_DUPLICATE_ALIAS),
										 errmsg("variable \"%s\" shadows a previously defined variable",
												$1),
										 parser_errposition(@1)));
						}

					}
				/* Begin - BUG#M0000450 */
				| K_ROWNUM
					{
						$$.name = pstrdup("rownum");
						$$.lineno = plisql_location_to_lineno(@1);
						/*
						 * Check to make sure name isn't already declared
						 * in the current block.
						 */
						if (plisql_ns_lookup(plisql_ns_top(), true,
											  $1, NULL, NULL,
											  NULL) != NULL)
							yyerror("duplicate declaration");

						if (plisql_curr_compile->extra_warnings & PLISQL_XCHECK_SHADOWVAR ||
							plisql_curr_compile->extra_errors & PLISQL_XCHECK_SHADOWVAR)
						{
							PLiSQL_nsitem *nsi;
							nsi = plisql_ns_lookup(plisql_ns_top(), false,
													$1, NULL, NULL, NULL);
							if (nsi != NULL)
								ereport(plisql_curr_compile->extra_errors & PLISQL_XCHECK_SHADOWVAR ? ERROR : WARNING,
										(errcode(ERRCODE_DUPLICATE_ALIAS),
										 errmsg("variable \"%s\" shadows a previously defined variable",
												$1),
										 parser_errposition(@1)));
						}
					}
				/* End - BUG#M0000450 */
				;

decl_const		:
					{ $$ = false; }
				| K_CONSTANT
					{ $$ = true; }
				;

decl_datatype	:
					{
						/*
						 * If there's a lookahead token, read_datatype
						 * should consume it.
						 */
						$$ = read_datatype(yychar);
						yyclearin;
					}
				;

decl_collate	:
					{ $$ = InvalidOid; }
				| K_COLLATE T_WORD
					{
						$$ = get_collation_oid(list_make1(makeString($2.ident)),
											   false);
					}
				| K_COLLATE unreserved_keyword
					{
						$$ = get_collation_oid(list_make1(makeString(pstrdup($2))),
											   false);
					}
				| K_COLLATE T_CWORD
					{
						$$ = get_collation_oid($2.idents, false);
					}
				;

decl_notnull	:
					{ $$ = false; }
				| K_NOT K_NULL
					{ $$ = true; }
				;

decl_defval		: ';'
					{ $$ = NULL; }
				| decl_defkey
					{
						$$ = read_sql_expression(';', ";");
					}
				;

decl_defkey		: assign_operator
				| K_DEFAULT
				;

/* Begin - SRS-PLSQL-SUBPROC */
function_heading	:	K_FUNCTION ora_function_name func_args K_RETURN decl_datatype
						{
							PLiSQL_subproc_function *subprocfunc;

							subprocfunc = plisql_build_subproc_function($2, $3, $5, @2); /* ReqID:SRS-SQL-PACKAGE */

							$$ = subprocfunc;
						}
						;

function_properties :	function_properite_list { $$ = $1; }
					|	{ $$ = NIL; }
					;

function_properite_list: function_properite_item
						{
							$$ = list_make1($1);
						}
						|function_properite_list function_properite_item
						{
							$$ = lappend($1, $2);
						}
						;

function_properite_item : K_DETERMINISTIC
							{
								PLiSQL_subprocfunc_proper *proper = palloc0(sizeof(PLiSQL_subprocfunc_proper));

								proper->proper_type = FUNC_PROPER_DETERMINISTIC;

								$$ = proper;
							}
						| K_RESULT_CACHE function_result_cache_relies
							{
								PLiSQL_subprocfunc_proper *proper = palloc0(sizeof(PLiSQL_subprocfunc_proper));

								/* anonymous blocks doesn't support result cache */
								if ((cur_compile_func_level != 0 &&
									plisql_saved_compile[0]->fn_oid == InvalidOid) ||
									(cur_compile_func_level == 0 && plisql_curr_compile->fn_oid == InvalidOid))
									yyerror("RESULT_CACHE is disallowed on subprograms in anonymous blocks");

								proper->proper_type = FUNC_PROPER_RESULT_CACHE;
								proper->value = $2;

								$$ = proper;
							}
						| K_PARALLEL_ENABLE
							{
								PLiSQL_subprocfunc_proper *proper = palloc0(sizeof(PLiSQL_subprocfunc_proper));

								proper->proper_type = FUNC_PROPER_PARALLEL_ENABLE;

								$$ = proper;
							}
						| K_PIPELINED
							{
								PLiSQL_subprocfunc_proper *proper = palloc0(sizeof(PLiSQL_subprocfunc_proper));

								proper->proper_type = FUNC_PROPER_PIPELINED;

								$$ = proper;
							}
						;

function_result_cache_relies : K_RELIES_ON '(' data_source ')'
								{
									$$ = $3;
								}
							| { $$ = NIL; }
							;

data_source	:	ora_function_name
					{
						$$ = list_make1(makeString($1));
					}
				|data_source ',' ora_function_name
					{
						$$ = lappend($1, makeString($3));
					}
				;

procedure_heading	: K_PROCEDURE ora_function_name func_args
						{
							PLiSQL_subproc_function *subprocfunc;

							subprocfunc = plisql_build_subproc_function($2, $3, NULL, @2); /* ReqID:SRS-SQL-PACKAGE */

							$$ = subprocfunc;
						}
						;

procedure_properties: procedure_properties_list	{ $$ = $1; }
					| { $$ = NIL; }
				;

procedure_properties_list: procedure_properite_item { $$ = list_make1($1); }
					| procedure_properties_list procedure_properite_item
						{
							$$ = lappend($1, $2);
						}
				;

procedure_properite_item: accessible_by_clause	{ $$ = $1;}
					|default_collation_clause		{ $$ = $1;}
					|invoker_rights_clause			{ $$ = $1;}
					;

accessible_by_clause: K_ACCESSIBLE K_BY '(' accessor_list ')'
						{
							PLiSQL_subprocfunc_proper *proper = palloc0(sizeof(PLiSQL_subprocfunc_proper));

							yyerror("Only schema-level programs allow ACCESSIBLE BY");
							proper->proper_type = PROC_PROPER_ACCESSIBLE_BY;
							proper->value = (void *) $4;

							$$ = proper;
						}
					;

accessor_list: accessor	{ $$ = list_make1($1); }
				|accessor_list ',' accessor { $$ = lappend($1, $3); }
				;

accessor: unit_kind unit_name unit_name
				{
					AccessProperItem *acproper = (AccessProperItem *) palloc0(sizeof(AccessProperItem));

					acproper->proper_type = $1;
					acproper->schema_name = $2;
					acproper->unit_name = $3;

					$$ = acproper;
				}
			|unit_name unit_name
				{
					AccessProperItem *acproper = (AccessProperItem *) palloc0(sizeof(AccessProperItem));

					acproper->proper_type = access_proper_unknow;
					acproper->schema_name = $1;
					acproper->unit_name = $2;

					$$ = acproper;
				}
			|unit_kind unit_name
				{
					AccessProperItem *acproper = (AccessProperItem *) palloc0(sizeof(AccessProperItem));

					acproper->proper_type = $1;
					acproper->schema_name = NULL;
					acproper->unit_name = $2;

					$$ = acproper;
				}
			| unit_name
				{
					AccessProperItem *acproper = (AccessProperItem *) palloc0(sizeof(AccessProperItem));

					acproper->proper_type = access_proper_unknow;
					acproper->schema_name = NULL;
					acproper->unit_name = $1;

					$$ = acproper;
				}
			|unit_kind extral_unit_name extral_unit_name
				{
					AccessProperItem *acproper = (AccessProperItem *) palloc0(sizeof(AccessProperItem));

					acproper->proper_type = $1;
					acproper->schema_name = $2;
					acproper->unit_name = $3;

					$$ = acproper;
				}
			|unit_kind extral_unit_name
				{
					AccessProperItem *acproper = (AccessProperItem *) palloc0(sizeof(AccessProperItem));

					acproper->proper_type = $1;
					acproper->schema_name = NULL;
					acproper->unit_name = $2;

					$$ = acproper;
				}
			|extral_unit_name
				{
					AccessProperItem *acproper = (AccessProperItem *) palloc0(sizeof(AccessProperItem));

					acproper->proper_type = access_proper_unknow;
					acproper->schema_name = NULL;
					acproper->unit_name = $1;

					$$ = acproper;
				}
			;

unit_name:  T_WORD
				{
					$$ = $1.ident;

				}
			| unit_name_keyword
				{
					$$ = pstrdup($1);
				}
		;

unit_kind: K_FUNCTION	{ $$ = access_proper_function; }
			|K_PROCEDURE { $$ = access_proper_procedure; }
			|K_PACKAGE	{ $$ = access_proper_package; }
			|K_TRIGGER	{ $$ = access_proper_trigger; }
			|K_TYPE		{ $$ = access_proper_type; }
		;

extral_unit_name:
			K_PACKAGE { $$ = pstrdup($1);}
			|K_TRIGGER { $$ = pstrdup($1);}
			|K_TYPE { $$ = pstrdup($1);}
		;

default_collation_clause: K_DEFAULT K_COLLATION K_USING_NLS_COMP
					{
						PLiSQL_subprocfunc_proper *proper = palloc0(sizeof(PLiSQL_subprocfunc_proper));

						proper->proper_type = PROC_PROPER_DEFAULT_COLLATION;
						proper->value = (void *) pstrdup($3);

						$$ = proper;
					}
				;

invoker_rights_clause: K_AUTHID authid_user
					{
						PLiSQL_subprocfunc_proper *proper = palloc0(sizeof(PLiSQL_subprocfunc_proper));

						yyerror("Only schema-level programs allow AUTHID");

						proper->proper_type = PROC_PROPER_INVOKER_RIGHT;
						proper->value = (void *) $2;

						$$ = proper;
					}
				;

authid_user: K_CURRENT_USER	{ $$ = pstrdup($1); }
				|K_DEFINER		{ $$ = pstrdup($1); }
			;


ora_function_name	: T_WORD
					{
						$$ = $1.ident;

					}
				| unreserved_keyword
					{
						$$ = pstrdup($1);
					}
			;

function_arg_nocopy:
					K_NOCOPY	{ $$ = true; }
					| { $$ = false; }
				;

ora_function_is_as:
					K_IS		{ $$ = $1; }
					|K_AS		{ $$ = $1; }
				;
func_args:
					'(' func_arg_list ')' { $$ = $2; }
					|'(' ')'	{ $$ = NIL; }
					| { $$ = NIL; }
					;
func_arg_list:
					func_arg_item
						{
							$$ = list_make1($1);
						}
					| func_arg_list ',' func_arg_item
						{
							$$ = lappend($1, $3);
						}
					;
func_arg_item:
					ora_function_name arg_mode function_arg_nocopy decl_datatype arg_decl_defval
						{
							PLiSQL_function_argitem *new = NULL;

							if ($2 != ARGMODE_IN && $5 != NULL)
								yyerror("OUT and IN OUT formal parameters may not have default expressions");

							if ($3 && $2 == ARGMODE_IN)
								yyerror("only out mode argument allow to have nocopy proper");

							new = (PLiSQL_function_argitem *) palloc0(sizeof(PLiSQL_function_argitem));
							new->argname = $1;
							new->type = $4;
							new->argmode = $2;
							new->defexpr = $5;
							new->nocopy = $3;
							$$ = new;
						}
					/* Begin - BUG#M0000451 */
					| K_ROWNUM arg_mode function_arg_nocopy decl_datatype arg_decl_defval
						{
							PLiSQL_function_argitem *new = NULL;

							if ($2 != ARGMODE_IN && $5 != NULL)
								yyerror("OUT and IN OUT formal parameters may not have default expressions");

							if ($3 && $2 == ARGMODE_IN)
								yyerror("only out mode argument allow to have nocopy proper");

							new = (PLiSQL_function_argitem *) palloc0(sizeof(PLiSQL_function_argitem));
							new->argname = pstrdup("rownum");;
							new->type = $4;
							new->argmode = $2;
							new->defexpr = $5;
							new->nocopy = $3;
							$$ = new;
						}
					/* End - BUG#M0000451 */
					;
arg_mode:
					K_IN	{ $$ = ARGMODE_IN; }
					|K_IN K_OUT	{ $$ = ARGMODE_INOUT; }
					|K_OUT	{ $$ = ARGMODE_OUT; }
					|	{ $$ = ARGMODE_IN; }
					;

arg_decl_defval:	decl_defkey
						{
							int token;

							$$ =  read_sql_expression2(',',	')', ", or )", &token);
							plisql_push_back_token(token);
						}
					| { $$ = NULL; }
				;

/* End - SRS-PLSQL-SUBPROC */

/* Begin - ReqID:SRS-PLSQL-RECORD */
record_args_list : record_arg_item
						{
							$$ = list_make1($1);
						}
					| record_args_list ',' record_arg_item
						{
							$$ = lappend($1, $3);
						}
					;

record_arg_item: decl_varname decl_datatype decl_notnull arg_decl_defval
						{
							PLiSQL_ora_record_argitem *new = NULL;

							new = (PLiSQL_ora_record_argitem *) palloc0(sizeof(PLiSQL_ora_record_argitem));
							new->argname = pstrdup($1.name);
							new->type = $2;
							new->notnull = $3;
							new->defexpr = $4;
							$$ = new;
						}
					;
/* End - ReqID:SRS-PLSQL-RECORD */

/*
 * Ada-based PL/iSQL uses := for assignment and variable defaults, while
 * the SQL standard uses equals for these cases and for GET
 * DIAGNOSTICS, so we support both.  FOR and OPEN only support :=.
 */
assign_operator	: '='
				| COLON_EQUALS
				;

proc_sect		:
					{
						$$ = NIL;
						plisql_blkpath_update_nstatements(true);		/* ReqID:SRS-PLSQL-GOTO */
					}
				| proc_sect proc_stmt
					{
						/* don't bother linking null statements into list */
						if ($2 == NULL)
							$$ = $1;
						else
						{
							$$ = lappend($1, $2);
							plisql_blkpath_update_nstatements(false);	/* ReqID:SRS-PLSQL-GOTO */
						}
					}
				;

proc_stmt		: pl_block ';'
						{ $$ = $1; }
				| stmt_assign
						{ $$ = $1; }
				| stmt_if
						{ $$ = $1; }
				| stmt_case
						{ $$ = $1; }
				| stmt_loop
						{ $$ = $1; }
				| stmt_while
						{ $$ = $1; }
				| stmt_for
						{ $$ = $1; }
				| stmt_foreach_a
						{ $$ = $1; }
				| stmt_exit
						{ $$ = $1; }
				| stmt_return
						{ $$ = $1; }
				| stmt_raise
						{ $$ = $1; }
				| stmt_assert
						{ $$ = $1; }
				| stmt_execsql
						{ $$ = $1; }
				| stmt_dynexecute
						{ $$ = $1; }
				| stmt_perform
						{ $$ = $1; }
				| stmt_call
						{ $$ = $1; }
				| stmt_getdiag
						{ $$ = $1; }
				| stmt_open
						{ $$ = $1; }
				| stmt_fetch
						{ $$ = $1; }
				| stmt_move
						{ $$ = $1; }
				| stmt_close
						{ $$ = $1; }
				| stmt_null
						{ $$ = $1; }
				| stmt_commit
						{ $$ = $1; }
				| stmt_rollback
						{ $$ = $1; }
				/* Begin - ReqID:SRS-PLSQL-GOTO */
				| stmt_goto
						{ $$ = $1; }
				/* End - ReqID:SRS-PLSQL-GOTO */
				;

stmt_perform	: opt_stmt_label K_PERFORM
					{
						PLiSQL_stmt_perform *new;
						int		startloc;

						new = palloc0(sizeof(PLiSQL_stmt_perform));
						new->cmd_type = PLISQL_STMT_PERFORM;
						new->lineno   = plisql_location_to_lineno(@2);
						new->stmtid = ++plisql_curr_compile->nstatements;
						plisql_push_back_token(K_PERFORM);

						/*
						 * Since PERFORM isn't legal SQL, we have to cheat to
						 * the extent of substituting "SELECT" for "PERFORM"
						 * in the parsed text.  It does not seem worth
						 * inventing a separate parse mode for this one case.
						 * We can't do syntax-checking until after we make the
						 * substitution.
						 */
						new->expr = read_sql_construct(';', 0, 0, 0, ";",
													   RAW_PARSE_DEFAULT,
													   false, false, true,
													   &startloc, NULL, false,
													   NULL/* ReqID:SRS-PLSQL-Collection */);
						/* overwrite "perform" ... */
						memcpy(new->expr->query, " SELECT", 7);
						/* left-justify to get rid of the leading space */
						memmove(new->expr->query, new->expr->query + 1,
								strlen(new->expr->query));
						/* offset syntax error position to account for that */
						check_sql_expr(new->expr->query, new->expr->parseMode,
									   startloc + 1);

						plisql_register_label($1);	/* ReqID:SRS-PLSQL-GOTO */
						$$ = (PLiSQL_stmt *)new;
					}
				;

stmt_call		: opt_stmt_label K_CALL
					{
						PLiSQL_stmt_call *new;

						new = palloc0(sizeof(PLiSQL_stmt_call));
						new->cmd_type = PLISQL_STMT_CALL;
						new->lineno = plisql_location_to_lineno(@2);
						new->stmtid = ++plisql_curr_compile->nstatements;
						plisql_push_back_token(K_CALL);
						new->expr = read_sql_stmt();
						new->is_call = true;

						/* Remember we may need a procedure resource owner */
						plisql_curr_compile->requires_procedure_resowner = true;

						plisql_register_label($1);	/* ReqID:SRS-PLSQL-GOTO */
						$$ = (PLiSQL_stmt *)new;

					}
				| opt_stmt_label K_DO
					{
						/* use the same structures as for CALL, for simplicity */
						PLiSQL_stmt_call *new;

						new = palloc0(sizeof(PLiSQL_stmt_call));
						new->cmd_type = PLISQL_STMT_CALL;
						new->lineno = plisql_location_to_lineno(@2);
						new->stmtid = ++plisql_curr_compile->nstatements;
						plisql_push_back_token(K_DO);
						new->expr = read_sql_stmt();
						new->is_call = false;

						/* Remember we may need a procedure resource owner */
						plisql_curr_compile->requires_procedure_resowner = true;

						plisql_register_label($1);	/* ReqID:SRS-PLSQL-GOTO */
						$$ = (PLiSQL_stmt *)new;

					}
				;

stmt_assign		: opt_stmt_label T_DATUM
					{
						/* Begin - ReqID:SRS-PLSQL-Collection */
						if (($2.datum->dtype == PLISQL_DTYPE_VAR &&
							((PLiSQL_var *)$2.datum)->datatype != NULL &&
							((PLiSQL_var *)$2.datum)->datatype->ttype == PLISQL_TTYPE_COLLECT_TYPE) ||
							($2.datum->dtype == PLISQL_DTYPE_PACKAGE_DATUM &&
							((PLiSQL_pkg_datum *)($2.datum))->pkgvar->dtype == PLISQL_DTYPE_VAR &&
							((PLiSQL_var *)(((PLiSQL_pkg_datum *)($2.datum))->pkgvar))->datatype != NULL &&
							((PLiSQL_var *)(((PLiSQL_pkg_datum *)($2.datum))->pkgvar))->datatype->ttype == PLISQL_TTYPE_COLLECT_TYPE))
						{
							check_assignable($2.datum, @2);
							$$ = (PLiSQL_stmt *)get_collect_stmt($2.datum, yylloc, plisql_location_to_lineno(@2));
						}
						else
						/* End - ReqID:SRS-PLSQL-Collection */
						{
							RawParseMode pmode;
							/* Begin - ReqID:SRS-PLSQL-Collection */
							bool ismulticollectmethod = false;
							PLiSQL_expr *expr;
							/* End - ReqID:SRS-PLSQL-Collection */

							/* see how many names identify the datum */
							switch ($2.ident ? 1 : list_length($2.idents))
							{
								case 1:
									pmode = RAW_PARSE_PLISQL_ASSIGN1; /* BUG:M0000079 */
									break;
								case 2:
									pmode = RAW_PARSE_PLISQL_ASSIGN2; /* BUG:M0000079 */
									break;
								case 3:
									pmode = RAW_PARSE_PLISQL_ASSIGN3; /* BUG:M0000079 */
									break;
								default:
									/* Begin - ReqID:SRS-PLSQL-RECORD */
									pmode = RAW_PARSE_PLISQL_ASSIGNN; /* BUG:M0000079 */
									break;
									/* End - ReqID:SRS-PLSQL-RECORD */
							}

							check_assignable($2.datum, @2);

							/* Begin - BUG#M0000376 */
							check_oraparam_mode($2.datum, @2, PROARGMODE_OUT);
							/* End - BUG#M0000376 */

							/* Push back the head name to include it in the stmt */
							plisql_push_back_token(T_DATUM);
							plisql_location_to_lineno(@2);

							expr = read_sql_construct(';', 0, 0, 0, ";",
												   pmode,
												   false, true, true,
												   NULL, NULL, false, &ismulticollectmethod/* ReqID:SRS-PLSQL-Collection */);

							/* Begin - ReqID:SRS-PLSQL-Collection */
							if (ismulticollectmethod)
							{
								PLiSQL_stmt_perform *new;

								new = palloc0(sizeof(PLiSQL_stmt_perform));
								new->cmd_type = PLISQL_STMT_PERFORM;
								new->lineno   = plisql_location_to_lineno(@2);
								new->stmtid = ++plisql_curr_compile->nstatements;
								new->expr  = expr;
								$$ = (PLiSQL_stmt *)new;
							}
							else
							/* End - ReqID:SRS-PLSQL-Collection */
							{
								PLiSQL_stmt_assign *new;

								new = palloc0(sizeof(PLiSQL_stmt_assign));
								new->cmd_type = PLISQL_STMT_ASSIGN;
								new->lineno   = plisql_location_to_lineno(@2);
								new->stmtid = ++plisql_curr_compile->nstatements;
								new->varno = $2.datum->dno;
								new->expr = expr;
								$$ = (PLiSQL_stmt *)new;
							}
							plisql_register_label($1);	/* ReqID:SRS-PLSQL-GOTO */
						}
					}
				;

stmt_getdiag	: opt_stmt_label K_GET getdiag_area_opt K_DIAGNOSTICS getdiag_list ';'
					{
						PLiSQL_stmt_getdiag	 *new;
						ListCell		*lc;

						new = palloc0(sizeof(PLiSQL_stmt_getdiag));
						new->cmd_type = PLISQL_STMT_GETDIAG;
						new->lineno   = plisql_location_to_lineno(@2);
						new->stmtid	  = ++plisql_curr_compile->nstatements;
						new->is_stacked = $3;
						new->diag_items = $5;

						/*
						 * Check information items are valid for area option.
						 */
						foreach(lc, new->diag_items)
						{
							PLiSQL_diag_item *ditem = (PLiSQL_diag_item *) lfirst(lc);

							switch (ditem->kind)
							{
								/* these fields are disallowed in stacked case */
								case PLISQL_GETDIAG_ROW_COUNT:
									if (new->is_stacked)
										ereport(ERROR,
												(errcode(ERRCODE_SYNTAX_ERROR),
												 errmsg("diagnostics item %s is not allowed in GET STACKED DIAGNOSTICS",
														plisql_getdiag_kindname(ditem->kind)),
												 parser_errposition(@2)));
									break;
								/* these fields are disallowed in current case */
								case PLISQL_GETDIAG_ERROR_CONTEXT:
								case PLISQL_GETDIAG_ERROR_DETAIL:
								case PLISQL_GETDIAG_ERROR_HINT:
								case PLISQL_GETDIAG_RETURNED_SQLSTATE:
								case PLISQL_GETDIAG_COLUMN_NAME:
								case PLISQL_GETDIAG_CONSTRAINT_NAME:
								case PLISQL_GETDIAG_DATATYPE_NAME:
								case PLISQL_GETDIAG_MESSAGE_TEXT:
								case PLISQL_GETDIAG_TABLE_NAME:
								case PLISQL_GETDIAG_SCHEMA_NAME:
									if (!new->is_stacked)
										ereport(ERROR,
												(errcode(ERRCODE_SYNTAX_ERROR),
												 errmsg("diagnostics item %s is not allowed in GET CURRENT DIAGNOSTICS",
														plisql_getdiag_kindname(ditem->kind)),
												 parser_errposition(@2)));
									break;
								/* these fields are allowed in either case */
								case PLISQL_GETDIAG_CONTEXT:
									break;
								default:
									elog(ERROR, "unrecognized diagnostic item kind: %d",
										 ditem->kind);
									break;
							}
						}

						plisql_register_label($1);	/* ReqID:SRS-PLSQL-GOTO */
						$$ = (PLiSQL_stmt *)new;
					}
				;

getdiag_area_opt :
					{
						$$ = false;
					}
				| K_CURRENT
					{
						$$ = false;
					}
				| K_STACKED
					{
						$$ = true;
					}
				;

getdiag_list : getdiag_list ',' getdiag_list_item
					{
						$$ = lappend($1, $3);
					}
				| getdiag_list_item
					{
						$$ = list_make1($1);
					}
				;

getdiag_list_item : getdiag_target assign_operator getdiag_item
					{
						PLiSQL_diag_item *new;

						new = palloc(sizeof(PLiSQL_diag_item));
						new->target = $1->dno;
						new->kind = $3;

						$$ = new;
					}
				;

getdiag_item :
					{
						int	tok = yylex();

						if (tok_is_keyword(tok, &yylval,
										   K_ROW_COUNT, "row_count"))
							$$ = PLISQL_GETDIAG_ROW_COUNT;
						else if (tok_is_keyword(tok, &yylval,
												K_PG_CONTEXT, "pg_context"))
							$$ = PLISQL_GETDIAG_CONTEXT;
						else if (tok_is_keyword(tok, &yylval,
												K_PG_EXCEPTION_DETAIL, "pg_exception_detail"))
							$$ = PLISQL_GETDIAG_ERROR_DETAIL;
						else if (tok_is_keyword(tok, &yylval,
												K_PG_EXCEPTION_HINT, "pg_exception_hint"))
							$$ = PLISQL_GETDIAG_ERROR_HINT;
						else if (tok_is_keyword(tok, &yylval,
												K_PG_EXCEPTION_CONTEXT, "pg_exception_context"))
							$$ = PLISQL_GETDIAG_ERROR_CONTEXT;
						else if (tok_is_keyword(tok, &yylval,
												K_COLUMN_NAME, "column_name"))
							$$ = PLISQL_GETDIAG_COLUMN_NAME;
						else if (tok_is_keyword(tok, &yylval,
												K_CONSTRAINT_NAME, "constraint_name"))
							$$ = PLISQL_GETDIAG_CONSTRAINT_NAME;
						else if (tok_is_keyword(tok, &yylval,
												K_PG_DATATYPE_NAME, "pg_datatype_name"))
							$$ = PLISQL_GETDIAG_DATATYPE_NAME;
						else if (tok_is_keyword(tok, &yylval,
												K_MESSAGE_TEXT, "message_text"))
							$$ = PLISQL_GETDIAG_MESSAGE_TEXT;
						else if (tok_is_keyword(tok, &yylval,
												K_TABLE_NAME, "table_name"))
							$$ = PLISQL_GETDIAG_TABLE_NAME;
						else if (tok_is_keyword(tok, &yylval,
												K_SCHEMA_NAME, "schema_name"))
							$$ = PLISQL_GETDIAG_SCHEMA_NAME;
						else if (tok_is_keyword(tok, &yylval,
												K_RETURNED_SQLSTATE, "returned_sqlstate"))
							$$ = PLISQL_GETDIAG_RETURNED_SQLSTATE;
						else
							yyerror("unrecognized GET DIAGNOSTICS item");
					}
				;

getdiag_target	: T_DATUM
					{
						/*
						 * In principle we should support a getdiag_target
						 * that is an array element, but for now we don't, so
						 * just throw an error if next token is '['.
						 */
						if (is_row_record_datum($1.datum) || /* ReqID:SRS-SQL-PACKAGE */
							plisql_peek() == '[')
							ereport(ERROR,
									(errcode(ERRCODE_SYNTAX_ERROR),
									 errmsg("\"%s\" is not a scalar variable",
											NameOfDatum(&($1))),
									 parser_errposition(@1)));
						check_assignable($1.datum, @1);
						$$ = $1.datum;
					}
				| T_WORD
					{
						/* just to give a better message than "syntax error" */
						word_is_not_variable(&($1), @1);
					}
				| T_CWORD
					{
						/* just to give a better message than "syntax error" */
						cword_is_not_variable(&($1), @1);
					}
				;

stmt_if			: opt_stmt_label K_IF expr_until_then
					{	/* ReqID:SRS-PLSQL-GOTO */
						plisql_blkpath_push($1 ? $1 : "[IF]");
					}
				proc_sect
					{	/* ReqID:SRS-PLSQL-GOTO */
						plisql_blkpath_pop();
					}
				stmt_elsifs stmt_else K_END K_IF ';'
					{
						PLiSQL_stmt_if *new;

						new = palloc0(sizeof(PLiSQL_stmt_if));
						new->cmd_type	= PLISQL_STMT_IF;
						new->lineno		= plisql_location_to_lineno(@2);
						new->stmtid		= ++plisql_curr_compile->nstatements;
						new->cond		= $3;
						new->then_body	= $5;
						new->elsif_list = $7;
						new->else_body  = $8;

						plisql_register_label($1);	/* ReqID:SRS-PLSQL-GOTO */
						$$ = (PLiSQL_stmt *)new;
					}
				;

stmt_elsifs		:
					{
						$$ = NIL;
					}
				| stmt_elsifs K_ELSIF expr_until_then
					{	/* ReqID:SRS-PLSQL-GOTO */
						plisql_blkpath_push("[elsif]");
					}
				proc_sect
					{
						PLiSQL_if_elsif *new;

						new = palloc0(sizeof(PLiSQL_if_elsif));
						new->lineno = plisql_location_to_lineno(@2);
						new->cond   = $3;
						new->stmts  = $5;
						plisql_blkpath_pop();	/* ReqID:SRS-PLSQL-GOTO */

						$$ = lappend($1, new);
					}
				;

stmt_else		:
					{
						$$ = NIL;
					}
				| K_ELSE
					{	/* ReqID:SRS-PLSQL-GOTO */
						plisql_blkpath_push("[else]");
					}
				proc_sect
					{
						$$ = $3;
						plisql_blkpath_pop();	/* ReqID:SRS-PLSQL-GOTO */
					}
				;

stmt_case		: opt_stmt_label K_CASE opt_expr_until_when case_when_list opt_case_else K_END K_CASE ';'
					{
						$$ = make_case(@2, $3, $4, $5);
						plisql_register_label($1);	/* ReqID:SRS-PLSQL-GOTO */
					}
				;

opt_expr_until_when	:
					{
						PLiSQL_expr *expr = NULL;
						int	tok = yylex();

						if (tok != K_WHEN)
						{
							plisql_push_back_token(tok);
							expr = read_sql_expression(K_WHEN, "WHEN");
						}
						plisql_push_back_token(K_WHEN);
						$$ = expr;
					}
				;

case_when_list	: case_when_list case_when
					{
						$$ = lappend($1, $2);
					}
				| case_when
					{
						$$ = list_make1($1);
					}
				;

case_when		: K_WHEN expr_until_then
					{	/* ReqID:SRS-PLSQL-GOTO */
						plisql_blkpath_push("[casewhen]");
					}
				proc_sect
					{
						PLiSQL_case_when *new = palloc(sizeof(PLiSQL_case_when));

						new->lineno	= plisql_location_to_lineno(@1);
						new->expr	= $2;
						new->stmts	= $4;
						plisql_blkpath_pop();	/* ReqID:SRS-PLSQL-GOTO */
						$$ = new;
					}
				;

opt_case_else	:
					{
						$$ = NIL;
					}
				| K_ELSE
					{	/* ReqID:SRS-PLSQL-GOTO */
						plisql_blkpath_push("[caseelse]");
					}
				proc_sect
					{
						/*
						 * proc_sect could return an empty list, but we
						 * must distinguish that from not having ELSE at all.
						 * Simplest fix is to return a list with one NULL
						 * pointer, which make_case() must take care of.
						 */
						if ($3 != NIL)
							$$ = $3;
						else
							$$ = list_make1(NULL);
						plisql_blkpath_pop();	/* ReqID:SRS-PLSQL-GOTO */
					}
				;

stmt_loop		: opt_loop_label K_LOOP loop_body
					{
						PLiSQL_stmt_loop *new;

						new = palloc0(sizeof(PLiSQL_stmt_loop));
						new->cmd_type = PLISQL_STMT_LOOP;
						new->lineno   = plisql_location_to_lineno(@2);
						new->stmtid   = ++plisql_curr_compile->nstatements;
						new->label	  = $1;
						new->body	  = $3.stmts;

						check_labels($1, $3.end_label, $3.end_label_location);
						plisql_ns_pop();

						/* Begin - ReqID:SRS-PLSQL-GOTO */
						plisql_lscope_pop();
						plisql_register_label($1);
						/* End - ReqID:SRS-PLSQL-GOTO */

						$$ = (PLiSQL_stmt *)new;
					}
				;

stmt_while		: opt_loop_label K_WHILE expr_until_loop loop_body
					{
						PLiSQL_stmt_while *new;

						new = palloc0(sizeof(PLiSQL_stmt_while));
						new->cmd_type = PLISQL_STMT_WHILE;
						new->lineno   = plisql_location_to_lineno(@2);
						new->stmtid	  = ++plisql_curr_compile->nstatements;
						new->label	  = $1;
						new->cond	  = $3;
						new->body	  = $4.stmts;

						check_labels($1, $4.end_label, $4.end_label_location);
						plisql_ns_pop();

						/* Begin - ReqID:SRS-PLSQL-GOTO */
						plisql_lscope_pop();
						plisql_register_label($1);
						/* End - ReqID:SRS-PLSQL-GOTO */

						$$ = (PLiSQL_stmt *)new;
					}
				;

stmt_for		: opt_loop_label K_FOR for_control loop_body
					{
						/* This runs after we've scanned the loop body */
						if ($3->cmd_type == PLISQL_STMT_FORI)
						{
							PLiSQL_stmt_fori		*new;

							new = (PLiSQL_stmt_fori *) $3;
							new->lineno   = plisql_location_to_lineno(@2);
							new->label	  = $1;
							new->body	  = $4.stmts;
							$$ = (PLiSQL_stmt *) new;
						}
						else
						{
							PLiSQL_stmt_forq		*new;

							Assert($3->cmd_type == PLISQL_STMT_FORS ||
								   $3->cmd_type == PLISQL_STMT_FORC ||
								   $3->cmd_type == PLISQL_STMT_DYNFORS);
							/* forq is the common supertype of all three */
							new = (PLiSQL_stmt_forq *) $3;
							new->lineno   = plisql_location_to_lineno(@2);
							new->label	  = $1;
							new->body	  = $4.stmts;
							$$ = (PLiSQL_stmt *) new;
						}

						check_labels($1, $4.end_label, $4.end_label_location);
						/* close namespace started in opt_loop_label */
						plisql_ns_pop();

						/* Begin - ReqID:SRS-PLSQL-GOTO */
						plisql_lscope_pop();
						plisql_register_label($1);
						/* End - ReqID:SRS-PLSQL-GOTO */
					}
				;

for_control		: for_variable K_IN
					{
						int			tok = yylex();
						int			tokloc = yylloc;


						/* Begin - ReqID:SRS-PLSQL-CURSOR */
						/* for r1 in 'xx' */
						if (tok == SCONST)
						{
							int tok1 = yylex();

							if (tok1 == DOT_DOT)
							{
								plisql_push_back_token(tok1);
								yylloc = tokloc;
							}
							else
							{
								plisql_push_back_token(tok1);
								yylloc = tokloc;
								plisql_push_back_token(tok);
								tok = K_EXECUTE;
							}
						}
						else if (tok == T_DATUM &&
							plisql_yylval.wdatum.datum->dtype == PLISQL_DTYPE_VAR)
						{
							PLiSQL_var* var = (PLiSQL_var*)(plisql_yylval.wdatum.datum);
							Oid typoid = var->datatype->typoid;
							int tok1;
							YYSTYPE lval;

							/* Begin - BUG#M0000376 */
							check_oraparam_mode(plisql_yylval.wdatum.datum, yylloc, PROARGMODE_IN);
							/* End - BUG#M0000376 */

							lval = yylval;
							tok1 = yylex();

							if (tok1 == DOT_DOT)
							{
								plisql_push_back_token(tok1);
								yylloc = tokloc;
							}
							else if (typoid == TEXTOID || typoid == VARCHAROID)
							{
								plisql_push_back_token(tok1);
								yylloc = tokloc;
								plisql_push_back_token(tok);
								tok = K_EXECUTE;
							}
							else
							{
								plisql_push_back_token(tok1);
								yylloc = tokloc;
							}
							yylval = lval;
						}
						/* End - ReqID:SRS-PLSQL-CURSOR */

						if (tok == K_EXECUTE)
						{
							/* EXECUTE means it's a dynamic FOR loop */
							PLiSQL_stmt_dynfors	*new;
							PLiSQL_expr			*expr;
							int						term;

							expr = read_sql_expression2(K_LOOP, K_USING,
														"LOOP or USING",
														&term);

							new = palloc0(sizeof(PLiSQL_stmt_dynfors));
							new->cmd_type = PLISQL_STMT_DYNFORS;
							new->stmtid	  = ++plisql_curr_compile->nstatements;
							if ($1.row)
							{
								new->var = (PLiSQL_variable *) $1.row;
								check_assignable($1.row, @1);
							}
							else if ($1.scalar)
							{
								/* convert single scalar to list */
								new->var = (PLiSQL_variable *)
									make_scalar_list1($1.name, $1.scalar,
													  $1.lineno, @1);
								/* make_scalar_list1 did check_assignable */
							}
							else
							{
								/* Begin - ReqID:SRS-PLSQL-CURSOR */
								/* create loop's private RECORD variable */
								new->var = (PLiSQL_variable *)  plisql_build_record($1.name,
															$1.lineno,
															NULL,
															RECORDOID,
															true);
								/* End - ReqID:SRS-PLSQL-CURSOR */
							}
							new->query = expr;

							if (term == K_USING)
							{
								do
								{
									expr = read_sql_expression2(',', K_LOOP,
																", or LOOP",
																&term);
									new->params = lappend(new->params, expr);
								} while (term == ',');
							}

							$$ = (PLiSQL_stmt *) new;
						}
						else if (tok == T_DATUM &&
								 yylval.wdatum.datum->dtype == PLISQL_DTYPE_VAR &&
								 ISCURSOROID(((PLiSQL_var *) yylval.wdatum.datum)->datatype->typoid)) /* ReqID:SRS-PLSQL-CURSOR */
						{
							/* It's FOR var IN cursor */
							PLiSQL_stmt_forc	*new;
							PLiSQL_var			*cursor = (PLiSQL_var *) yylval.wdatum.datum;

							/* Begin - BUG#M0000376 */
							check_oraparam_mode(yylval.wdatum.datum, yylloc, PROARGMODE_IN);
							/* End - BUG#M0000376 */

							new = (PLiSQL_stmt_forc *) palloc0(sizeof(PLiSQL_stmt_forc));
							new->cmd_type = PLISQL_STMT_FORC;
							new->stmtid = ++plisql_curr_compile->nstatements;
							new->curvar = cursor->dno;

							/* Should have had a single variable name */
							if ($1.scalar && $1.row)
								ereport(ERROR,
										(errcode(ERRCODE_SYNTAX_ERROR),
										 errmsg("cursor FOR loop must have only one target variable"),
										 parser_errposition(@1)));

							/* can't use an unbound cursor this way */
							if (cursor->cursor_explicit_expr == NULL)
								ereport(ERROR,
										(errcode(ERRCODE_SYNTAX_ERROR),
										 errmsg("cursor FOR loop must use a bound cursor variable"),
										 parser_errposition(tokloc)));

							/* collect cursor's parameters if any */
							new->argquery = read_cursor_args(cursor,
															 K_LOOP,
															 &new->outrowno);/* ReqID:SRS-PLSQL-CURSOR */

							/* create loop's private RECORD variable */
							new->var = (PLiSQL_variable *)
								plisql_build_record($1.name,
													 $1.lineno,
													 NULL,
													 RECORDOID,
													 true);

							$$ = (PLiSQL_stmt *) new;
						}
						else
						{
							PLiSQL_expr	*expr1;
							int				expr1loc;
							bool			reverse = false;

							/*
							 * We have to distinguish between two
							 * alternatives: FOR var IN a .. b and FOR
							 * var IN query. Unfortunately this is
							 * tricky, since the query in the second
							 * form needn't start with a SELECT
							 * keyword.  We use the ugly hack of
							 * looking for two periods after the first
							 * token. We also check for the REVERSE
							 * keyword, which means it must be an
							 * integer loop.
							 */
							if (tok_is_keyword(tok, &yylval,
											   K_REVERSE, "reverse"))
								reverse = true;
							else
								plisql_push_back_token(tok);

							/*
							 * Read tokens until we see either a ".."
							 * or a LOOP.  The text we read may be either
							 * an expression or a whole SQL statement, so
							 * we need to invoke read_sql_construct directly,
							 * and tell it not to check syntax yet.
							 */
							expr1 = read_sql_construct(DOT_DOT,
													   K_LOOP,
													   0,
													   0,
													   "LOOP",
													   RAW_PARSE_DEFAULT,
													   true,
													   false,
													   true,
													   &expr1loc,
													   &tok,
													   false,
													   NULL/* ReqID:SRS-PLSQL-Collection */);

							if (tok == DOT_DOT)
							{
								/* Saw "..", so it must be an integer loop */
								PLiSQL_expr		*expr2;
								PLiSQL_expr		*expr_by;
								PLiSQL_var			*fvar;
								PLiSQL_stmt_fori	*new;

								/*
								 * Relabel first expression as an expression;
								 * then we can check its syntax.
								 */
								expr1->parseMode = RAW_PARSE_PLISQL_EXPR; /* BUG:M0000079 */
								check_sql_expr(expr1->query, expr1->parseMode,
											   expr1loc);

								/* Read and check the second one */
								expr2 = read_sql_expression2(K_LOOP, K_BY,
															 "LOOP",
															 &tok);

								/* Get the BY clause if any */
								if (tok == K_BY)
									expr_by = read_sql_expression(K_LOOP,
																  "LOOP");
								else
									expr_by = NULL;

								/* Should have had a single variable name */
								if ($1.scalar && $1.row)
									ereport(ERROR,
											(errcode(ERRCODE_SYNTAX_ERROR),
											 errmsg("integer FOR loop must have only one target variable"),
											 parser_errposition(@1)));

								/* create loop's private variable */
								fvar = (PLiSQL_var *)
									plisql_build_variable($1.name,
														   $1.lineno,
														   plisql_build_datatype(INT4OID,
																				  -1,
																				  InvalidOid,
																				  NULL),
														   true);

								new = palloc0(sizeof(PLiSQL_stmt_fori));
								new->cmd_type = PLISQL_STMT_FORI;
								new->stmtid	  = ++plisql_curr_compile->nstatements;
								new->var	  = fvar;
								new->reverse  = reverse;
								new->lower	  = expr1;
								new->upper	  = expr2;
								new->step	  = expr_by;

								$$ = (PLiSQL_stmt *) new;
							}
							else
							{
								/*
								 * No "..", so it must be a query loop.
								 */
								PLiSQL_stmt_fors	*new;

								if (reverse)
									ereport(ERROR,
											(errcode(ERRCODE_SYNTAX_ERROR),
											 errmsg("cannot specify REVERSE in query FOR loop"),
											 parser_errposition(tokloc)));

								/* Check syntax as a regular query */
								check_sql_expr(expr1->query, expr1->parseMode,
											   expr1loc);

								new = palloc0(sizeof(PLiSQL_stmt_fors));
								new->cmd_type = PLISQL_STMT_FORS;
								new->stmtid = ++plisql_curr_compile->nstatements;
								if ($1.row)
								{
									new->var = (PLiSQL_variable *) $1.row;
									check_assignable($1.row, @1);
								}
								else if ($1.scalar)
								{
									/* convert single scalar to list */
									new->var = (PLiSQL_variable *)
										make_scalar_list1($1.name, $1.scalar,
														  $1.lineno, @1);
									/* make_scalar_list1 did check_assignable */
								}
								else
								{
									/* Begin - ReqID:SRS-PLSQL-CURSOR */
									/* create loop's private RECORD variable */
									new->var = (PLiSQL_variable *)  plisql_build_record($1.name,
															$1.lineno,
															NULL,
															RECORDOID,
															true);
									/* End - ReqID:SRS-PLSQL-CURSOR */
								}

								new->query = expr1;
								$$ = (PLiSQL_stmt *) new;
							}
						}
					}
				;

/*
 * Processing the for_variable is tricky because we don't yet know if the
 * FOR is an integer FOR loop or a loop over query results.  In the former
 * case, the variable is just a name that we must instantiate as a loop
 * local variable, regardless of any other definition it might have.
 * Therefore, we always save the actual identifier into $$.name where it
 * can be used for that case.  We also save the outer-variable definition,
 * if any, because that's what we need for the loop-over-query case.  Note
 * that we must NOT apply check_assignable() or any other semantic check
 * until we know what's what.
 *
 * However, if we see a comma-separated list of names, we know that it
 * can't be an integer FOR loop and so it's OK to check the variables
 * immediately.  In particular, for T_WORD followed by comma, we should
 * complain that the name is not known rather than say it's a syntax error.
 * Note that the non-error result of this case sets *both* $$.scalar and
 * $$.row; see the for_control production.
 */
for_variable	: T_DATUM
					{
						$$.name = NameOfDatum(&($1));
						$$.lineno = plisql_location_to_lineno(@1);
						if (is_row_record_datum($1.datum)) /* ReqID:SRS-SQL-PACKAGE */
						{
							$$.scalar = NULL;
							$$.row = $1.datum;
						}
						else
						{
							int			tok;

							$$.scalar = $1.datum;
							$$.row = NULL;
							/* check for comma-separated list */
							tok = yylex();
							plisql_push_back_token(tok);
							if (tok == ',')
								$$.row = (PLiSQL_datum *)
									read_into_scalar_list($$.name,
														  $$.scalar,
														  @1);
						}
						/* Begin - BUG#M0000376 */
						check_oraparam_mode($1.datum, @1, PROARGMODE_IN);
						/* End - BUG#M0000376 */
					}
				| T_WORD
					{
						int			tok;

						$$.name = $1.ident;
						$$.lineno = plisql_location_to_lineno(@1);
						$$.scalar = NULL;
						$$.row = NULL;
						/* check for comma-separated list */
						tok = yylex();
						plisql_push_back_token(tok);
						if (tok == ',')
							word_is_not_variable(&($1), @1);
					}
				| T_CWORD
					{
						/* just to give a better message than "syntax error" */
						cword_is_not_variable(&($1), @1);
					}
				;

stmt_foreach_a	: opt_loop_label K_FOREACH for_variable foreach_slice K_IN K_ARRAY expr_until_loop loop_body
					{
						PLiSQL_stmt_foreach_a *new;

						new = palloc0(sizeof(PLiSQL_stmt_foreach_a));
						new->cmd_type = PLISQL_STMT_FOREACH_A;
						new->lineno = plisql_location_to_lineno(@2);
						new->stmtid = ++plisql_curr_compile->nstatements;
						new->label = $1;
						new->slice = $4;
						new->expr = $7;
						new->body = $8.stmts;

						if ($3.row)
						{
							new->varno = $3.row->dno;
							check_assignable($3.row, @3);
						}
						else if ($3.scalar)
						{
							new->varno = $3.scalar->dno;
							check_assignable($3.scalar, @3);
							/* Begin - BUG#M0000376 */
							check_oraparam_mode($3.scalar, @3, PROARGMODE_IN);
							/* End - BUG#M0000376 */
						}
						else
						{
							ereport(ERROR,
									(errcode(ERRCODE_SYNTAX_ERROR),
									 errmsg("loop variable of FOREACH must be a known variable or list of variables"),
											 parser_errposition(@3)));
						}

						check_labels($1, $8.end_label, $8.end_label_location);
						plisql_ns_pop();

						/* Begin - ReqID:SRS-PLSQL-GOTO */
						plisql_lscope_pop();
						plisql_register_label($1);
						/* End - ReqID:SRS-PLSQL-GOTO */

						$$ = (PLiSQL_stmt *) new;
					}
				;

foreach_slice	:
					{
						$$ = 0;
					}
				| K_SLICE ICONST
					{
						$$ = $2;
					}
				;

stmt_exit		: opt_stmt_label exit_type opt_label opt_exitcond
					{
						PLiSQL_stmt_exit *new;

						new = palloc0(sizeof(PLiSQL_stmt_exit));
						new->cmd_type = PLISQL_STMT_EXIT;
						new->stmtid	  = ++plisql_curr_compile->nstatements;
						new->is_exit  = $2;
						new->lineno	  = plisql_location_to_lineno(@1);
						new->label	  = $3;
						new->cond	  = $4;

						if ($3)
						{
							/* We have a label, so verify it exists */
							PLiSQL_nsitem *label;

							label = plisql_ns_lookup_label(plisql_ns_top(), $3);
							if (label == NULL)
								ereport(ERROR,
										(errcode(ERRCODE_SYNTAX_ERROR),
										 errmsg("there is no label \"%s\" "
												"attached to any block or loop enclosing this statement",
												$3),
										 parser_errposition(@3)));
							/* CONTINUE only allows loop labels */
							if (label->itemno != PLISQL_LABEL_LOOP && !new->is_exit)
								ereport(ERROR,
										(errcode(ERRCODE_SYNTAX_ERROR),
										 errmsg("block label \"%s\" cannot be used in CONTINUE",
												$3),
										 parser_errposition(@3)));
						}
						else
						{
							/*
							 * No label, so make sure there is some loop (an
							 * unlabeled EXIT does not match a block, so this
							 * is the same test for both EXIT and CONTINUE)
							 */
							if (plisql_ns_find_nearest_loop(plisql_ns_top()) == NULL)
								ereport(ERROR,
										(errcode(ERRCODE_SYNTAX_ERROR),
										 new->is_exit ?
										 errmsg("EXIT cannot be used outside a loop, unless it has a label") :
										 errmsg("CONTINUE cannot be used outside a loop"),
										 parser_errposition(@2)));
						}

						plisql_register_label($1);	/* ReqID:SRS-PLSQL-GOTO */
						$$ = (PLiSQL_stmt *)new;
					}
				;

exit_type		: K_EXIT
					{
						$$ = true;
					}
				| K_CONTINUE
					{
						$$ = false;
					}
				;

stmt_return		: opt_stmt_label K_RETURN
					{
						int	tok;

						tok = yylex();
						if (tok == 0)
							yyerror("unexpected end of function definition");

						if (tok_is_keyword(tok, &yylval,
										   K_NEXT, "next"))
						{
							$$ = make_return_next_stmt(@2);
						}
						else if (tok_is_keyword(tok, &yylval,
												K_QUERY, "query"))
						{
							$$ = make_return_query_stmt(@2);
						}
						else
						{
							plisql_push_back_token(tok);
							$$ = make_return_stmt(@2);
						}

						plisql_register_label($1);	/* ReqID:SRS-PLSQL-GOTO */
					}
				;

stmt_raise		: opt_stmt_label K_RAISE
					{
						PLiSQL_stmt_raise		*new;
						int	tok;

						new = palloc(sizeof(PLiSQL_stmt_raise));

						new->cmd_type	= PLISQL_STMT_RAISE;
						new->lineno		= plisql_location_to_lineno(@2);
						new->stmtid		= ++plisql_curr_compile->nstatements;
						new->elog_level = ERROR;	/* default */
						new->condname	= NULL;
						new->message	= NULL;
						new->params		= NIL;
						new->options	= NIL;
						/* Begin - ReqID:SRS-PLSQL-CURSOR */
						new->excepdno = -1;
						/* End - ReqID:SRS-PLSQL-CURSOR */

						tok = yylex();
						if (tok == 0)
							yyerror("unexpected end of function definition");

						/*
						 * We could have just RAISE, meaning to re-throw
						 * the current error.
						 */
						if (tok != ';')
						{
							/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
							/*
							 * we should check user define exception first
							 * because serverity level maybe a exception
							 * name
							 */
							if (tok == T_DATUM)
							{
								PLiSQL_datum *except_datum;

								if (yylval.wdatum.datum->dtype == PLISQL_DTYPE_PACKAGE_DATUM)
									except_datum = ((PLiSQL_pkg_datum *) yylval.wdatum.datum)->pkgvar;
								else
									except_datum = yylval.wdatum.datum;

								/*
								 * raise ORA-06550 exception
								 */
								if (except_datum->dtype != PLISQL_DTYPE_VAR)
									yyerror("in exception handler, raise'var must be an exception name");

								if (!ISEXCEPTIONOID(((PLiSQL_var *) except_datum)->datatype->typoid))
									yyerror("in exception handler, raise'var must be an exception name");

								new->excepdno = yylval.wdatum.datum->dno;
								tok = yylex();
								if (tok != ';')
									yyerror("syntax error");
							}
							/* End - ReqID:SRS-PLSQL-EXCEPTION */
							/*
							 * First is an optional elog severity level.
							 */
							else if (tok_is_keyword(tok, &yylval,
											   K_EXCEPTION, "exception"))
							{
								new->elog_level = ERROR;
								tok = yylex();
							}
							else if (tok_is_keyword(tok, &yylval,
													K_WARNING, "warning"))
							{
								new->elog_level = WARNING;
								tok = yylex();
							}
							else if (tok_is_keyword(tok, &yylval,
													K_NOTICE, "notice"))
							{
								new->elog_level = NOTICE;
								tok = yylex();
							}
							else if (tok_is_keyword(tok, &yylval,
													K_INFO, "info"))
							{
								new->elog_level = INFO;
								tok = yylex();
							}
							else if (tok_is_keyword(tok, &yylval,
													K_LOG, "log"))
							{
								new->elog_level = LOG;
								tok = yylex();
							}
							else if (tok_is_keyword(tok, &yylval,
													K_DEBUG, "debug"))
							{
								new->elog_level = DEBUG1;
								tok = yylex();
							}
							if (tok == 0)
								yyerror("unexpected end of function definition");

							/*
							 * Next we can have a condition name, or
							 * equivalently SQLSTATE 'xxxxx', or a string
							 * literal that is the old-style message format,
							 * or USING to start the option list immediately.
							 */
							if (tok == SCONST)
							{
								/* old style message and parameters */
								new->message = yylval.str;
								/*
								 * We expect either a semi-colon, which
								 * indicates no parameters, or a comma that
								 * begins the list of parameter expressions,
								 * or USING to begin the options list.
								 */
								tok = yylex();
								if (tok != ',' && tok != ';' && tok != K_USING)
									yyerror("syntax error");

								while (tok == ',')
								{
									PLiSQL_expr *expr;

									expr = read_sql_construct(',', ';', 0, K_USING,
															  ", or ; or USING",
															  RAW_PARSE_PLISQL_EXPR, /* BUG:M0000079 */
															  true, true, true,
															  NULL, &tok, false,
															  NULL/* ReqID:SRS-PLSQL-Collection */);
									new->params = lappend(new->params, expr);
								}
							}
							else if (tok != K_USING &&
									tok != ';') /* ReqID:SRS-PLSQL-CURSOR */
							{
								/* must be condition name or SQLSTATE */
								if (tok_is_keyword(tok, &yylval,
												   K_SQLSTATE, "sqlstate"))
								{
									/* next token should be a string literal */
									char   *sqlstatestr;

									if (yylex() != SCONST)
										yyerror("syntax error");
									sqlstatestr = yylval.str;

									if (strlen(sqlstatestr) != 5)
										yyerror("invalid SQLSTATE code");
									if (strspn(sqlstatestr, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ") != 5)
										yyerror("invalid SQLSTATE code");
									new->condname = sqlstatestr;
								}
								else
								{
									if (tok == T_WORD)
										new->condname = yylval.word.ident;
									else if (plisql_token_is_unreserved_keyword(tok))
										new->condname = pstrdup(yylval.keyword);
									else
										yyerror("syntax error");
									plisql_recognize_err_condition(new->condname,
																	false);
								}
								tok = yylex();
								if (tok != ';' && tok != K_USING)
									yyerror("syntax error");
							}

							if (tok == K_USING)
								new->options = read_raise_options();
						}

						check_raise_parameters(new);

						plisql_register_label($1);	/* ReqID:SRS-PLSQL-GOTO */
						$$ = (PLiSQL_stmt *)new;
					}
				;

stmt_assert		: opt_stmt_label K_ASSERT
					{
						PLiSQL_stmt_assert		*new;
						int	tok;

						new = palloc(sizeof(PLiSQL_stmt_assert));

						new->cmd_type	= PLISQL_STMT_ASSERT;
						new->lineno		= plisql_location_to_lineno(@2);
						new->stmtid		= ++plisql_curr_compile->nstatements;

						new->cond = read_sql_expression2(',', ';',
														 ", or ;",
														 &tok);

						if (tok == ',')
							new->message = read_sql_expression(';', ";");
						else
							new->message = NULL;

						plisql_register_label($1);	/* ReqID:SRS-PLSQL-GOTO */
						$$ = (PLiSQL_stmt *) new;
					}
				;

loop_body		: proc_sect K_END K_LOOP opt_label ';'
					{
						$$.stmts = $1;
						$$.end_label = $4;
						$$.end_label_location = @4;
					}
				;

/*
 * T_WORD+T_CWORD match any initial identifier that is not a known plisql
 * variable.  (The composite case is probably a syntax error, but we'll let
 * the core parser decide that.)  Normally, we should assume that such a
 * word is a SQL statement keyword that isn't also a plisql keyword.
 * However, if the next token is assignment or '[' or '.', it can't be a valid
 * SQL statement, and what we're probably looking at is an intended variable
 * assignment.  Give an appropriate complaint for that, instead of letting
 * the core parser throw an unhelpful "syntax error".
 */
stmt_execsql	: opt_stmt_label K_IMPORT
					{
						$$ = make_execsql_stmt(K_IMPORT, @2);
						plisql_register_label($1);	/* ReqID:SRS-PLSQL-GOTO */
					}
				| opt_stmt_label K_INSERT
					{
						$$ = make_execsql_stmt(K_INSERT, @2);
						plisql_register_label($1);	/* ReqID:SRS-PLSQL-GOTO */
					}
				/* Begin - BUG#Z1389 */
				| opt_stmt_label K_MERGE
					{
						$$ = make_execsql_stmt(K_MERGE, @2);
						plisql_register_label($1);	/* ReqID:SRS-PLSQL-GOTO */
					}
				/* End - BUG#Z1389 */
				/* Begin - ReqID:SRS-PLSQL-CURSOR */
				| opt_stmt_label K_SELECT
					{
						/*
						 * there use T_WORD for handle into stmt
						 * see details for make_execsql_stmt
						 */
						$$ = make_execsql_stmt(T_WORD, @2);
						plisql_register_label($1);	/* ReqID:SRS-PLSQL-GOTO */
					}
				/* End - ReqID:SRS-PLSQL-CURSOR */
				| opt_stmt_label T_WORD
					{
						int			tok;

						tok = yylex();
						plisql_push_back_token(tok);
						if (tok == '=' || tok == COLON_EQUALS ||
							tok == '[' || tok == '.')
							word_is_not_variable(&($2), @2);

						/* Begin - BUG #M0000304 */
						if (tok == '(' || tok == ';')
							$$ = build_call_expr(T_WORD, $2.ident, @2);
						else
						/* End - BUG #M0000304 */
							$$ = make_execsql_stmt(T_WORD, @2);
						plisql_register_label($1);	/* ReqID:SRS-PLSQL-GOTO */
					}
				| opt_stmt_label T_CWORD
					{
						int			tok;
						/* Begin - ReqID:SRS-PLSQL-Collection */
						int				collect_method_token;
						PLiSQL_datum	*datum;
						char	   		*func_name;
						List		   *wdnames = yylval.cword.idents;
						/* End - ReqID:SRS-PLSQL-Collection */

						tok = yylex();
						plisql_push_back_token(tok);
						if (tok == '=' || tok == COLON_EQUALS ||
							tok == '[' || tok == '.')
							cword_is_not_variable(&($2), @2);

						/* Begin - ReqID:SRS-PLSQL-Collection */
						func_name = strVal(llast(wdnames));
						collect_method_token = get_collect_method_tok(func_name);
						if (collect_method_token != -1)
						{
							datum = reparse_cword(wdnames);
							if (datum != NULL)
							{
								PLiSQL_type 	*datatype = NULL;

								if (datum->dtype == PLISQL_DTYPE_TYPE)
									ereport(ERROR,
												(errcode(ERRCODE_SYNTAX_ERROR),
												 errmsg("Invalid variable.")));

								if (datum->dtype == PLISQL_DTYPE_VAR)
									datatype = (PLiSQL_type *)((PLiSQL_var *)datum)->datatype;
								else if (datum->dtype == PLISQL_DTYPE_PACKAGE_DATUM)
								{
									PLiSQL_datum *pkgvar = ((PLiSQL_pkg_datum *)datum)->pkgvar;

									if (pkgvar->dtype == PLISQL_DTYPE_VAR)
										datatype = (PLiSQL_type *)((PLiSQL_var *)pkgvar)->datatype;
								}

								if (datatype && datatype->ttype == PLISQL_TTYPE_COLLECT_TYPE)
								{
									StringInfoData	nameinfo;
									ListCell	*lc;
									int 		n = list_length(wdnames);
									int 		i;
									StringInfoData		ds;
									PLiSQL_expr 		*expr;
									PLiSQL_stmt_perform *new;

									initStringInfo(&nameinfo);
									i = 1;
									foreach(lc, wdnames)
									{
										if (i == n)
											break;
										else
										{
											char *name = strVal(lfirst(lc));

											appendStringInfoString(&nameinfo, name);
											if (i < n - 1)
												appendStringInfoChar(&nameinfo, '.');
											i++;
										}
									}

									initStringInfo(&ds);
									appendStringInfoString(&ds, "SELECT ");
									read_collect_method(collect_method_token, nameinfo.data,
													   datatype, &ds, &@1);
									pfree(nameinfo.data);

									tok = yylex();
									while(tok != ';' && tok != 0)
										tok = yylex();
									expr = palloc0(sizeof(PLiSQL_expr));
									expr->query	 = pstrdup(ds.data);
									expr->plan 		 = NULL;
									expr->paramnos 	 = NULL;
									expr->ns			 = plisql_ns_top();

									new = palloc0(sizeof(PLiSQL_stmt_perform));
									new->cmd_type = PLISQL_STMT_PERFORM;
									new->lineno   = plisql_location_to_lineno(@1);
									new->stmtid = ++plisql_curr_compile->nstatements;
									new->expr	= expr;
									$$ = (PLiSQL_stmt *)new;
								}
							}
							else if (datum != NULL && datum->dtype == PLISQL_DTYPE_TYPE)
								ereport(ERROR,
											(errcode(ERRCODE_SYNTAX_ERROR),
											 errmsg("Invalid variable.")));
						}
						else
						/* End - ReqID:SRS-PLSQL-Collection */
						{
							/* Begin - BUG #M0000304 */
							if (tok == '(' || tok == ';')
								$$ = build_call_expr(T_WORD, NULL, @2);
							else
							/* End - BUG #M0000304 */
								$$ = make_execsql_stmt(T_WORD, @2);
						}
						plisql_register_label($1);	/* ReqID:SRS-PLSQL-GOTO */
					}
				;

stmt_dynexecute : opt_stmt_label K_EXECUTE
					{
						PLiSQL_stmt_dynexecute *new;
						PLiSQL_expr *expr;
						int endtoken;
						/* Begin - ReqID:SRS-PLSQL-DSQL */
						bool has_returning = false;
						int num_out_param = 0;
						/* End - ReqID:SRS-PLSQL-DSQL */
						/* Begin - ReqID:SRS-PLSQL-Collection */
						char	*fieldnames[1024];
						int		varnos[1024];
						/* End - ReqID:SRS-PLSQL-Collection */

						/* Begin - BUG#Z1438 */
						int	tok;
						tok = yylex();

						/* Eat IMMEDIATE otherwise put token back into read-ahead stack */
						if(tok != K_IMMEDIATE)
							plisql_push_back_token(tok);
						/* End - BUG#Z1438 */

						expr = read_sql_construct(K_INTO, K_USING, ';',
												  K_BULK, /* ReqID:SRS-PLSQL-BULKCOLLECT */
												  "INTO or USING or ; or BULK",
												  RAW_PARSE_PLISQL_EXPR, /* BUG:M0000079 */
												  true, true, true,
												  NULL, &endtoken, false, NULL/* ReqID:SRS-PLSQL-Collection */);

						new = palloc(sizeof(PLiSQL_stmt_dynexecute));
						new->cmd_type = PLISQL_STMT_DYNEXECUTE;
						new->lineno = plisql_location_to_lineno(@2);
						new->stmtid = ++plisql_curr_compile->nstatements;
						new->query = expr;
						new->into = false;
						new->strict = false;
						new->target = NULL;
						new->params = NIL;
						/* Begin - ReqID:SRS-SQL-ANONYOUT */
						new->haveout = false;
						new->out = NULL;
						/* End - ReqID:SRS-SQL-ANONYOUT */
						/* Begin - ReqID:SRS-PLSQL-BULKCOLLECT */
						new->bulk_collect_into = false;
						/* End - ReqID:SRS-PLSQL-BULKCOLLECT */

						/*
						 * We loop to allow the INTO and USING clauses to
						 * appear in either order, since people easily get
						 * that wrong.  This coding also prevents "INTO foo"
						 * from getting absorbed into a USING expression,
						 * which is *really* confusing.
						 */
						for (;;)
						{
							if (endtoken == K_INTO)
							{
								if (new->into)			/* multiple INTO */
									yyerror("syntax error");
								new->into = true;
								read_into_target(&new->target, &new->strict);
								endtoken = yylex();
							}
							else if (endtoken == K_USING)
							{
								if (new->params)		/* multiple USING */
									yyerror("syntax error");
								do
								{
									/* Begin - ReqID:SRS-PLSQL-DSQL */
									char	param_info = PROARGMODE_IN;
									bool	is_out_param = false;

									/* Begin - ReqID:SRS-PLSQL-Collection */
									/* Check for array overflow */
									if (num_out_param >= 1024)
										ereport(ERROR,
												(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
												 errmsg("too many OUT parameters specified"),
												 parser_errposition(yylloc)));
									/* End - ReqID:SRS-PLSQL-Collection */

									/*
									 * USING [IN] [OUT] variable 
									 */
									tok = yylex();
									if (tok == K_IN)
									{
										tok = yylex();
										if (tok == K_OUT)
										{
											param_info = PROARGMODE_INOUT;
											is_out_param = true;
											/* Begin - ReqID:SRS-PLSQL-Collection */
											endtoken = read_out_scalar(&fieldnames[num_out_param],
																&varnos[num_out_param], param_info, &expr);
											num_out_param += 1;
											new->params = lappend(new->params, expr);
											if (endtoken == ';')
												break;
											else
												continue;
											/* End - ReqID:SRS-PLSQL-Collection */
										}
										else
										{
											plisql_push_back_token(tok);
										}
									}
									else if (tok == K_OUT) 
									{
										param_info = PROARGMODE_OUT;
										is_out_param = true;
										/* Begin - ReqID:SRS-PLSQL-Collection */
										endtoken = read_out_scalar(&fieldnames[num_out_param],
															&varnos[num_out_param], param_info, &expr);
										num_out_param += 1;
										new->params = lappend(new->params, expr);
										if (endtoken == ';')
											break;
										else
											continue;
										/* End - ReqID:SRS-PLSQL-Collection */
									}
									else
									{
										plisql_push_back_token(tok);
									}
									/* End - ReqID:SRS-PLSQL-DSQL */

									expr = read_sql_construct(',', ';', K_INTO, 0,
															  ", or ; or INTO",
															  RAW_PARSE_PLISQL_EXPR, /* BUG:M0000079 */
															  true, true, true,
															  NULL, &endtoken, is_out_param,
															  NULL/* ReqID:SRS-PLSQL-Collection */);
									/* Begin - ReqID:SRS-PLSQL-DSQL */
									expr->info = param_info;
									/* End - ReqID:SRS-PLSQL-DSQL */
									new->params = lappend(new->params, expr);
								} while (endtoken == ',');
							}
							/* Begin - ReqID:SRS-PLSQL-DSQL */
							else if (endtoken == K_RETURN || endtoken == K_RETURNING)
							{
								ListCell *lc;

								tok = yylex();
								/* Eat INTO otherwise put token back into read-ahead stack */
								if(tok != K_INTO && tok != K_BULK /* ReqID:SRS-PLSQL-BULKCOLLECT */)
									plisql_push_back_token(tok);

								if (has_returning)		/* multiple RETURN|RETURNING */
									yyerror("syntax error");

								/* Already have OUT param? */
								foreach(lc, new->params)
								{
									PLiSQL_expr *prm = (PLiSQL_expr *)lfirst(lc);
									if (prm->info == PROARGMODE_OUT || prm->info == PROARGMODE_INOUT)
										elog(ERROR, "could not use OUT or IN/OUT mode when using returning clause");
								}

								has_returning = true;
								if (tok == K_INTO)
								{
									/* Begin - ReqID:SRS-PLSQL-Collection */
									new->into = true;
									read_into_target(&new->target, &new->strict);
								}
								/* Begin - ReqID:SRS-PLSQL-BULKCOLLECT */
								else if (tok == K_BULK)
								{
									endtoken = yylex();
									if (endtoken == K_COLLECT)
									{
										endtoken = yylex();
										if (endtoken == K_INTO)
										{
											if (new->bulk_collect_into || new->into)	/* multiple INTO or BULK COLLECT INTO */
												yyerror("syntax error");
											new->bulk_collect_into = true;
											read_bulk_collect_into_target(&new->target);
										}
										else
											yyerror("syntax error");
									}
									else
										yyerror("syntax error");
								}
								/* End - ReqID:SRS-PLSQL-BULKCOLLECT */
								endtoken = yylex();
								/* End - ReqID:SRS-PLSQL-Collection */
							}
							/* End - ReqID:SRS-PLSQL-DSQL */
							/* Begin - ReqID:SRS-PLSQL-BULKCOLLECT */
							else if (endtoken == K_BULK)
							{
								endtoken = yylex();
								if (endtoken == K_COLLECT)
								{
									endtoken = yylex();
									if (endtoken == K_INTO)
									{
										if (new->bulk_collect_into || new->into)	/* multiple INTO or BULK COLLECT INTO */
											yyerror("syntax error");
										new->bulk_collect_into = true;
										read_bulk_collect_into_target(&new->target);
										endtoken = yylex();
									}
									else
										yyerror("syntax error");
								}
								else
									yyerror("syntax error");
							}
							/* End - ReqID:SRS-PLSQL-BULKCOLLECT */
							else if (endtoken == ';')
								break;
							else
								yyerror("syntax error");
						}

						/* Begin - ReqID:SRS-PLSQL-DSQL */
						/*
						 * Considering that "INTO", "USING OUT" and "RETURNING INTO" couldn't appear
						 * at the same time, we treat the "OUT" parameters in the last two cases as
						 * the "INTO" target, and thus we can reuse the codes.
						 */
						if (num_out_param > 0)
						{
							PLiSQL_row *row = palloc0(sizeof(PLiSQL_row));
							int	nfield = 0;

							row->dtype = PLISQL_DTYPE_ROW;
							row->refname = "(unnamed row)";
							row->lineno = plisql_location_to_lineno(yylloc);
							row->rowtupdesc = NULL;
							row->nfields = num_out_param;
							row->fieldnames = palloc(sizeof(char *) * num_out_param);
							row->varnos = palloc(sizeof(int) * num_out_param);

							/* Begin - ReqID:SRS-PLSQL-Collection */
							while (nfield < num_out_param)
							{
								row->fieldnames[nfield] = fieldnames[nfield];
								row->varnos[nfield] = varnos[nfield];
								nfield++;
							}
							/* End - ReqID:SRS-PLSQL-Collection */

							plisql_adddatum((PLiSQL_datum *)row);
							new->into = true;
							new->target = (PLiSQL_variable*) row;
						}
						/* End - ReqID:SRS-PLSQL-DSQL */

						plisql_register_label($1);	/* ReqID:SRS-PLSQL-GOTO */
						$$ = (PLiSQL_stmt *)new;
					}
				;


stmt_open		: opt_stmt_label K_OPEN cursor_variable
					{
						PLiSQL_stmt_open *new;
						int				  tok;
						/* Begin - ReqID:SRS-SQL-PACKAGE */
						PLiSQL_var *cursorvar;
						/* End - ReqID:SRS-SQL-PACKAGE */

						new = palloc0(sizeof(PLiSQL_stmt_open));
						new->cmd_type = PLISQL_STMT_OPEN;
						new->lineno = plisql_location_to_lineno(@2);
						new->stmtid = ++plisql_curr_compile->nstatements;
						new->curvar = $3->dno;
						new->cursor_options = CURSOR_OPT_FAST_PLAN;

						/* Begin - ReqID:SRS-SQL-PACKAGE */
						if ($3->dtype == PLISQL_DTYPE_PACKAGE_DATUM)
							cursorvar = (PLiSQL_var *) ((PLiSQL_pkg_datum *) $3)->pkgvar;
						else
							cursorvar = $3;
						/* End - ReqID:SRS-SQL-PACKAGE */

						/* Begin - ReqID:SRS-PLSQL-CURSOR */
						/*
						 * explicit cursor has not query, raise error
						 */
						if (cursorvar->is_explicit_cursor &&
							cursorvar->cursor_explicit_expr == NULL)
							yyerror("the declaration of the type of this expression "
										"is incomplete or malformed");
						new->outrowno = -1;
						/* End - ReqID:SRS-PLSQL-CURSOR */

						if (cursorvar->cursor_explicit_expr == NULL) /* ReqID:SRS-SQL-PACKAGE */
						{
							/* be nice if we could use opt_scrollable here */
							tok = yylex();
							if (tok_is_keyword(tok, &yylval,
											   K_NO, "no"))
							{
								tok = yylex();
								if (tok_is_keyword(tok, &yylval,
												   K_SCROLL, "scroll"))
								{
									new->cursor_options |= CURSOR_OPT_NO_SCROLL;
									tok = yylex();
								}
							}
							else if (tok_is_keyword(tok, &yylval,
													K_SCROLL, "scroll"))
							{
								new->cursor_options |= CURSOR_OPT_SCROLL;
								tok = yylex();
							}

							if (tok != K_FOR)
								yyerror("syntax error, expected \"FOR\"");

							tok = yylex();
							/* Begin - ReqID:SRS-PLSQL-CURSOR */
							/*
							 * support for open var for queryvar,so we check
							 * wether is select keyword
							 */
							if (tok != K_EXECUTE)
							{
								if (tok != K_SELECT)
									tok = K_EXECUTE;
								plisql_push_back_token(tok);
							}
							/* End - ReqID:SRS-PLSQL-CURSOR */
							if (tok == K_EXECUTE)
							{
								int		endtoken;

								new->dynquery =
									read_sql_expression2(K_USING, ';',
														 "USING or ;",
														 &endtoken);

								/* If we found "USING", collect argument(s) */
								if (endtoken == K_USING)
								{
									PLiSQL_expr *expr;

									do
									{
										/* Begin - BUG#Z0001351 */
										/* support using in xxx */
										tok = yylex();
										if (tok == K_IN)
										{
											tok = yylex();
											if (tok == K_OUT)
												yyerror("OUT and IN/OUT modes cannot be used in this context");
											plisql_push_back_token(tok);
										}
										else if (tok == K_OUT)
											yyerror("OUT and IN/OUT modes cannot be used in this context");
										else
											plisql_push_back_token(tok);
										/* End - BUG#Z00001351 */

										expr = read_sql_expression2(',', ';',
																	", or ;",
																	&endtoken);
										new->params = lappend(new->params,
															  expr);
									} while (endtoken == ',');
								}
							}
							else
							{
								int		endtoken; /* ReqID:SRS-PLSQL-CURSOR */

								plisql_push_back_token(tok);
								/* Begin - ReqID:SRS-PLSQL-CURSOR */
								/*
								 * maybe is select xxx from yy using
								 */
								new->query = read_sql_construct(';', K_USING, 0, 0, ";",
												RAW_PARSE_DEFAULT,
												false, true, true,
												NULL, &endtoken, false,
												NULL/* ReqID:SRS-PLSQL-Collection */);
								if (endtoken == K_USING)
									endtoken = yylex();
								if (endtoken != ';')
									yyerror("syntax error, expected \";\"");
								/* End - ReqID:SRS-PLSQL-CURSOR */
							}
						}
						else
						{
							/* predefined cursor query, so read args */
							new->argquery = read_cursor_args($3, ';',
										&new->outrowno); /* ReqID:SRS-PLSQL-CURSOR */
						}

						/* Begin - ReqID:SRS-PLSQL-CURSOR */
						/*
						 * check return type match the query
						 */
						if (cursorvar->is_explicit_cursor &&
							cursorvar->cursor_rettype_tupledesc != NULL)
						{
							if (cursorvar->cursor_resulttupdesc == NULL)
							{
								/* not pkg_datum */
								Assert($3->dtype != PLISQL_DTYPE_PACKAGE_DATUM);
								cursorvar->cursor_resulttupdesc =
									plisql_get_cursor_tupdesc_incompile(cursorvar->cursor_explicit_expr,
																		cursorvar->cursor_options);
							}
							ora_test_tupdesc_match(cursorvar->cursor_rettype_tupledesc,
										cursorvar->cursor_resulttupdesc, true);
						}
						else if (cursorvar->cursor_rettype_tupledesc != NULL &&
								new->query != NULL)
						{
							TupleDesc query_tupdesc;

							query_tupdesc = plisql_get_cursor_tupdesc_incompile(new->query,
																		new->cursor_options);
							ora_test_tupdesc_match(cursorvar->cursor_rettype_tupledesc,
										query_tupdesc, true);
							FreeTupleDesc(query_tupdesc);
						}
						/* End - ReqID:SRS-PLSQL-CURSOR */

						plisql_register_label($1);	/* ReqID:SRS-PLSQL-GOTO */
						$$ = (PLiSQL_stmt *)new;
					}
				;

stmt_fetch		: opt_stmt_label K_FETCH opt_fetch_direction cursor_variable K_INTO
					{
						PLiSQL_stmt_fetch *fetch = $3;
						PLiSQL_variable *target;

						/* We have already parsed everything through the INTO keyword */
						read_into_target(&target, NULL);

						if (yylex() != ';')
							yyerror("syntax error");

						/*
						 * We don't allow multiple rows in PL/iSQL's FETCH
						 * statement, only in MOVE.
						 */
						if (fetch->returns_multiple_rows)
							ereport(ERROR,
									(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
									 errmsg("FETCH statement cannot return multiple rows"),
									 parser_errposition(@2)));

						fetch->lineno = plisql_location_to_lineno(@2);
						fetch->target	= target;
						fetch->curvar	= $4->dno;
						fetch->is_move	= false;
						/* Begin - ReqID:SRS-PLSQL-BULKCOLLECT */
						fetch->bulk_collect_into = false;
						fetch->limit_expr = NULL;
						/* End - ReqID:SRS-PLSQL-BULKCOLLECT */

						plisql_register_label($1);	/* ReqID:SRS-PLSQL-GOTO */
						$$ = (PLiSQL_stmt *)fetch;
					}
				/* Begin - ReqID:SRS-PLSQL-BULKCOLLECT */
				| opt_stmt_label K_FETCH opt_fetch_direction cursor_variable K_BULK K_COLLECT K_INTO
					{
						PLiSQL_stmt_fetch *fetch = $3;
						PLiSQL_variable *target;
						int	tok;

						/* We have already parsed everything through the BULK COLLECT INTO keyword */
						read_bulk_collect_into_target(&target);

						tok = yylex();
						if (tok == K_LIMIT)
							fetch->limit_expr = read_sql_expression(';', ";");

						/*
						 * We don't allow multiple rows in PL/iSQL's FETCH
						 * statement, only in MOVE.
						 */
						if (fetch->returns_multiple_rows)
							ereport(ERROR,
									(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
									 errmsg("FETCH statement cannot return multiple rows"),
									 parser_errposition(@1)));

						fetch->lineno = plisql_location_to_lineno(@1);
						fetch->target	= target;
						fetch->curvar	= $4->dno;
						fetch->is_move	= false;
						fetch->bulk_collect_into = true;

						$$ = (PLiSQL_stmt *)fetch;
					}
				/* End - ReqID:SRS-PLSQL-BULKCOLLECT */
				;

stmt_move		: opt_stmt_label K_MOVE opt_fetch_direction cursor_variable ';'
					{
						PLiSQL_stmt_fetch *fetch = $3;

						fetch->lineno = plisql_location_to_lineno(@2);
						fetch->curvar	= $4->dno;
						fetch->is_move	= true;

						plisql_register_label($1);	/* ReqID:SRS-PLSQL-GOTO */
						$$ = (PLiSQL_stmt *)fetch;
					}
				;

opt_fetch_direction	:
					{
						$$ = read_fetch_direction();
					}
				;

stmt_close		: opt_stmt_label K_CLOSE cursor_variable ';'
					{
						PLiSQL_stmt_close *new;

						new = palloc(sizeof(PLiSQL_stmt_close));
						new->cmd_type = PLISQL_STMT_CLOSE;
						new->lineno = plisql_location_to_lineno(@2);
						new->stmtid = ++plisql_curr_compile->nstatements;
						new->curvar = $3->dno;

						plisql_register_label($1);	/* ReqID:SRS-PLSQL-GOTO */
						$$ = (PLiSQL_stmt *)new;
					}
				;

/* Begin - ReqID:SRS-PLSQL-GOTO */
stmt_null		: opt_stmt_label K_NULL ';'
					{
						if ($1 != NULL)
						{
							PLiSQL_stmt_null *new = palloc(sizeof(PLiSQL_stmt_null));

							new->cmd_type	= PLISQL_STMT_NULL;
							new->lineno		= plisql_location_to_lineno(@2);
							new->stmtid		= ++plisql_curr_compile->nstatements;

							plisql_register_label($1);
							$$ = (PLiSQL_stmt *) new;
						}
						else
						{
							/* We do not bother building a node for NULL */
							$$ = NULL;
						}
					}
				;
/* End - ReqID:SRS-PLSQL-GOTO */

stmt_commit		: opt_stmt_label K_COMMIT opt_transaction_chain ';'
					{
						PLiSQL_stmt_commit *new;

						new = palloc(sizeof(PLiSQL_stmt_commit));
						new->cmd_type = PLISQL_STMT_COMMIT;
						new->lineno = plisql_location_to_lineno(@2);
						new->stmtid = ++plisql_curr_compile->nstatements;
						new->chain = $3;

						plisql_register_label($1);	/* ReqID:SRS-PLSQL-GOTO */
						$$ = (PLiSQL_stmt *)new;
					}
				;

stmt_rollback	: opt_stmt_label K_ROLLBACK opt_transaction_chain ';'
					{
						PLiSQL_stmt_rollback *new;

						new = palloc(sizeof(PLiSQL_stmt_rollback));
						new->cmd_type = PLISQL_STMT_ROLLBACK;
						new->lineno = plisql_location_to_lineno(@2);
						new->stmtid = ++plisql_curr_compile->nstatements;
						new->chain = $3;

						plisql_register_label($1);	/* ReqID:SRS-PLSQL-GOTO */
						$$ = (PLiSQL_stmt *)new;
					}
				;

/* Begin - ReqID:SRS-PLSQL-GOTO */
stmt_goto: opt_stmt_label K_GOTO any_identifier ';'
					{
						PLiSQL_stmt_goto	*new;

						new = palloc0(sizeof(PLiSQL_stmt_goto));
						new->cmd_type	= PLISQL_STMT_GOTO;
						new->lineno		= plisql_location_to_lineno(@2);
						new->stmtid		= ++plisql_curr_compile->nstatements;
						new->target_label = $3;
						plisql_register_label($1);
						plisql_register_goto(new);

						$$ = (PLiSQL_stmt *) new;
					}
				;
/* End - ReqID:SRS-PLSQL-GOTO */

opt_transaction_chain:
			K_AND K_CHAIN			{ $$ = true; }
			| K_AND K_NO K_CHAIN	{ $$ = false; }
			| /* EMPTY */			{ $$ = false; }
				;


cursor_variable	: T_DATUM
					{
						/*
						 * In principle we should support a cursor_variable
						 * that is an array element, but for now we don't, so
						 * just throw an error if next token is '['.
						 *
						 * ReqID:SRS-SQL-PACKAGE: maybe a package variable
						 */
						if (($1.datum->dtype != PLISQL_DTYPE_VAR &&
							$1.datum->dtype != PLISQL_DTYPE_PACKAGE_DATUM) ||
							plisql_peek() == '[')
							ereport(ERROR,
									(errcode(ERRCODE_DATATYPE_MISMATCH),
									 errmsg("cursor variable must be a simple variable"),
									 parser_errposition(@1)));

						/* Begin - BUG#M0000376 */
						check_oraparam_mode($1.datum, @1, PROARGMODE_IN);
						/* End - BUG#M0000376 */

						/* Begin - ReqID:SRS-SQL-PACKAGE */
						if ($1.datum->dtype == PLISQL_DTYPE_PACKAGE_DATUM)
						{
							PLiSQL_pkg_datum *pkg_datum = (PLiSQL_pkg_datum *) $1.datum;
							PLiSQL_var *pkg_var = (PLiSQL_var *) pkg_datum->pkgvar;

							if (pkg_var->dtype != PLISQL_DTYPE_VAR)
								ereport(ERROR,
									(errcode(ERRCODE_DATATYPE_MISMATCH),
									 errmsg("cursor variable must be a simple variable"),
									 parser_errposition(@1)));
							if (!ISCURSOROID(pkg_var->datatype->typoid )) /* ReqID:SRS-PLSQL-CURSOR */
								ereport(ERROR,
									(errcode(ERRCODE_DATATYPE_MISMATCH),
									 errmsg("variable \"%s\" must be of type cursor or refcursor",
											pkg_var->refname),
									 parser_errposition(@1)));
						}
						else if (!ISCURSOROID(((PLiSQL_var *) $1.datum)->datatype->typoid)) /* ReqID:SRS-PLSQL-CURSOR */
							ereport(ERROR,
									(errcode(ERRCODE_DATATYPE_MISMATCH),
									 errmsg("variable \"%s\" must be of type cursor or refcursor",
											((PLiSQL_var *) $1.datum)->refname),
									 parser_errposition(@1)));
						/* End - ReqID:SRS-SQL-PACKAGE */
						$$ = (PLiSQL_var *) $1.datum;
					}
				| T_WORD
					{
						/* just to give a better message than "syntax error" */
						word_is_not_variable(&($1), @1);
					}
				| T_CWORD
					{
						/* just to give a better message than "syntax error" */
						cword_is_not_variable(&($1), @1);
					}
				;

exception_sect	:
					{ $$ = NULL; }
				| K_EXCEPTION
					{
						/*
						 * We use a mid-rule action to add these
						 * special variables to the namespace before
						 * parsing the WHEN clauses themselves.  The
						 * scope of the names extends to the end of the
						 * current block.
						 */
						int			lineno = plisql_location_to_lineno(@1);
						PLiSQL_exception_block *new = palloc(sizeof(PLiSQL_exception_block));
						PLiSQL_variable *var;

						var = plisql_build_variable("sqlstate", lineno,
													 plisql_build_datatype(TEXTOID,
																			-1,
																			plisql_curr_compile->fn_input_collation,
																			NULL),
													 true);
						var->isconst = true;
						new->sqlstate_varno = var->dno;

						var = plisql_build_variable("sqlerrm", lineno,
													 plisql_build_datatype(TEXTOID,
																			-1,
																			plisql_curr_compile->fn_input_collation,
																			NULL),
													 true);
						var->isconst = true;
						new->sqlerrm_varno = var->dno;

						$<exception_block>$ = new;

						/* Begin - ReqID:SRS-PLSQL-GOTO */
						plisql_blkpath_pop();
						plisql_blkpath_push("[exception]");
						plisql_blkpath_set_exception(true);
						plisql_blkpath_update_nbranches(true);
						/* End - ReqID:SRS-PLSQL-GOTO */
					}
					proc_exceptions
					{
						PLiSQL_exception_block *new = $<exception_block>2;
						ListCell *lc; /* ReqID:SRS-PLSQL-EXCEPTION */

						new->exc_list = $3;
						/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
						/*
						 * check others must be last conditions
						 * ORA-06550
						 */
						foreach (lc, new->exc_list)
						{
							PLiSQL_exception *exception = (PLiSQL_exception *) lfirst(lc);
							ListCell *nextlc = lnext(new->exc_list, lc);

							if (exception->conditions->condname != NULL &&
								strcmp(exception->conditions->condname, "others") == 0)
							{
								/*
								 * others should the last conditions except for
								 * query_canceled or assert_failure
								 */
								if (nextlc != NULL)
								{
									PLiSQL_exception *nexcep = (PLiSQL_exception *) lfirst(nextlc);

									if (nexcep->conditions->condname != NULL &&
										strcmp(nexcep->conditions->condname, "query_canceled") != 0 &&
										strcmp(nexcep->conditions->condname, "assert_failure") != 0)
										yyerror("OTHERS handler must be last among the exception handlers of a block");
								}
							}
						}
						/* End - ReqID:SRS-PLSQL-EXCEPTION */

						$$ = new;
					}
				;

proc_exceptions	: proc_exceptions proc_exception
						{
							$$ = lappend($1, $2);
							plisql_blkpath_update_nbranches(false);	/* ReqID:SRS-PLSQL-GOTO */
						}
				| proc_exception
						{
							$$ = list_make1($1);
							plisql_blkpath_update_nbranches(false);	/* ReqID:SRS-PLSQL-GOTO */
						}
				;

proc_exception	: K_WHEN proc_conditions K_THEN proc_sect
					{
						PLiSQL_exception *new;

						new = palloc0(sizeof(PLiSQL_exception));
						new->lineno = plisql_location_to_lineno(@1);
						new->conditions = $2;
						new->action = $4;

						$$ = new;
					}
				;

proc_conditions	: proc_conditions K_OR proc_condition
						{
							PLiSQL_condition	*old;
							/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
							bool	assert_failue_or_cancel_query = false;
							bool	has_common_exception = false;

							/*
							 * The assert_failure and cancel_query exceptions are not included
							 * in the others exception. Therefore, we allow these two exceptions
							 * to be mixed with others.
							 */
							if ($3->condname != NULL &&
								(strcmp($3->condname, "query_canceled") == 0 ||
								strcmp($3->condname, "assert_failure") == 0))
								assert_failue_or_cancel_query = true;

							for (old = $1; old->next != NULL; old = old->next)
							{
								/*
								 * check not others conditions
								 * ORA-06550
								 */
								if (!assert_failue_or_cancel_query &&
									old->condname != NULL &&
									strcmp(old->condname, "others") == 0)
									yyerror("no choices may appear with choice OTHERS in an exception handler");
								if (old->condname != NULL &&
									strcmp(old->condname, "query_canceled") != 0 &&
									strcmp(old->condname, "assert_failure") != 0)
									has_common_exception = true;
							}
							if (!assert_failue_or_cancel_query &&
								old->condname != NULL &&
								strcmp(old->condname, "others") == 0)
								yyerror("no choices may appear with choice OTHERS in an exception handler");
							if (old->condname != NULL &&
								strcmp(old->condname, "query_canceled") != 0 &&
								strcmp(old->condname, "assert_failure") != 0)
								has_common_exception = true;
							if (has_common_exception &&
								$3->condname != NULL &&
								strcmp($3->condname, "others") == 0)
								yyerror("no choices may appear with choice OTHERS in an exception handler");
							/* End - ReqID:SRS-PLSQL-EXCEPTION */
							old->next = $3;
							$$ = $1;
						}
				| proc_condition
						{
							$$ = $1;
						}
				;

proc_condition	: exception_any_identifier
						{
							if (strcmp($1, "sqlstate") != 0)
							{
								$$ = plisql_parse_err_condition($1);
							}
							else
							{
								PLiSQL_condition *new;
								char   *sqlstatestr;

								/* next token should be a string literal */
								if (yylex() != SCONST)
									yyerror("syntax error");
								sqlstatestr = yylval.str;

								if (strlen(sqlstatestr) != 5)
									yyerror("invalid SQLSTATE code");
								if (strspn(sqlstatestr, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ") != 5)
									yyerror("invalid SQLSTATE code");

								new = palloc(sizeof(PLiSQL_condition));
								new->sqlerrstate =
									MAKE_SQLSTATE(sqlstatestr[0],
												  sqlstatestr[1],
												  sqlstatestr[2],
												  sqlstatestr[3],
												  sqlstatestr[4]);
								new->condname = sqlstatestr;
								new->next = NULL;
								/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
								new->excepdno = -1;
								/* End - ReqID:SRS-PLSQL-EXCEPTION */

								$$ = new;
							}
						}
				/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
				| T_DATUM
					{
						PLiSQL_datum *datum = $1.datum;

						if (datum->dtype == PLISQL_DTYPE_PACKAGE_DATUM)
							datum = ((PLiSQL_pkg_datum *) datum)->pkgvar;

						/*
						 * check is exception or we handle like exception_any_identifier
						 * others is special and have high priority.
						 */
						if (datum->dtype == PLISQL_DTYPE_VAR &&
							((PLiSQL_var *) datum)->refname != NULL &&
							strcmp(((PLiSQL_var *) datum)->refname, "others") != 0 &&
							ISEXCEPTIONOID(((PLiSQL_var *) datum)->datatype->typoid))
						{
							PLiSQL_condition *new;

							new = palloc0(sizeof(PLiSQL_condition));
							new->next = NULL;
							new->condname = $1.ident;
							new->excepdno = $1.datum->dno;

							$$ = new;
						}
						else
						{
							/* like exception_any_identifier handle */
							if ($1.ident == NULL) /* composite name not OK */
								yyerror("syntax error");
							if (strcmp($1.ident, "sqlstate") != 0 &&
								strcmp($1.ident, "sqlcode") != 0 &&
								strcmp($1.ident, "sqlerrm") != 0)
							{
								$$ = plisql_parse_err_condition($1.ident);
							}
							else
							{
								PLiSQL_condition *new;
								char   *sqlstatestr;
								int		token;

								token = yylex();

								/*
								 * next token should be a string literal
								 * or a udt exception var
								 */
								if (token == K_THEN)
								{
									PLiSQL_datum *exception_datum = NULL;
									PLiSQL_nsitem *nse = NULL;

									nse = plisql_ns_lookup_exception(plisql_ns_top(),
																		$1.ident);
									if (nse != NULL)
										exception_datum = plisql_Datums[nse->itemno];
									else
										yyerror("invalid exception");
									new = palloc0(sizeof(PLiSQL_condition));
									new->next = NULL;
									new->condname = $1.ident;
									new->excepdno = exception_datum->dno;
									plisql_push_back_token(token);
								}
								else if (token != SCONST)
									yyerror("syntax error");
								else
								{
									sqlstatestr = yylval.str;

									if (strlen(sqlstatestr) != 5)
										yyerror("invalid SQLSTATE code");
									if (strspn(sqlstatestr, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ") != 5)
										yyerror("invalid SQLSTATE code");

									new = palloc(sizeof(PLiSQL_condition));
									new->sqlerrstate =
										MAKE_SQLSTATE(sqlstatestr[0],
													  sqlstatestr[1],
													  sqlstatestr[2],
													  sqlstatestr[3],
													  sqlstatestr[4]);
									new->condname = sqlstatestr;
									new->next = NULL;
									new->excepdno = -1;
								}
								$$ = new;
							}
						}
					}
				/* End - ReqID:SRS-PLSQL-EXCEPTION */
				;

expr_until_semi :
					{ $$ = read_sql_expression(';', ";"); }
				;

expr_until_then :
					{ $$ = read_sql_expression(K_THEN, "THEN"); }
				;

expr_until_loop :
					{ $$ = read_sql_expression(K_LOOP, "LOOP"); }
				;

opt_block_label	:
					{
						plisql_ns_push(NULL, PLISQL_LABEL_BLOCK);
						plisql_lscope_push(NULL);	/* ReqID:SRS-PLSQL-GOTO */
						$$ = NULL;
					}
				| LESS_LESS any_identifier GREATER_GREATER
					{
						plisql_ns_push($2, PLISQL_LABEL_BLOCK);
						plisql_lscope_push($2);		/* ReqID:SRS-PLSQL-GOTO */
						$$ = $2;
					}
				;

opt_loop_label	:
					{
						plisql_ns_push(NULL, PLISQL_LABEL_LOOP);
						plisql_lscope_push(NULL);	/* ReqID:SRS-PLSQL-GOTO */
						$$ = NULL;
					}
				| LESS_LESS any_identifier GREATER_GREATER
					{
						plisql_ns_push($2, PLISQL_LABEL_LOOP);
						plisql_lscope_push($2);		/* ReqID:SRS-PLSQL-GOTO */
						$$ = $2;
					}
				;

/* Begin - ReqID:SRS-PLSQL-GOTO */
opt_stmt_label:
					{ $$ = NULL; }
				| LESS_LESS any_identifier GREATER_GREATER
					{
						$$ = $2;
					}
				;
/* End - ReqID:SRS-PLSQL-GOTO */

opt_label	:
					{
						$$ = NULL;
					}
				| any_identifier
					{
						/* label validity will be checked by outer production */
						$$ = $1;
					}
				;

opt_exitcond	: ';'
					{ $$ = NULL; }
				| K_WHEN expr_until_semi
					{ $$ = $2; }
				;

/*
 * need to allow DATUM because scanner will have tried to resolve as variable
 */
any_identifier	: T_WORD
					{
						$$ = $1.ident;
					}
				| unreserved_keyword
					{
						$$ = pstrdup($1);
					}
				| T_DATUM
					{
						if ($1.ident == NULL) /* composite name not OK */
							yyerror("syntax error");
						$$ = $1.ident;
					}
				;

/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
/*
 * like any_identifier but we shoud
 * split T_DATUM for user define exception
 */
exception_any_identifier	: T_WORD
					{
						$$ = $1.ident;
					}
				| unreserved_keyword
					{
						$$ = pstrdup($1);
					}
				;

SignedIconst:	'-' ICONST	{$$ = 0 - $2;}
						|'+' ICONST	{$$ = $2;}
						|ICONST { $$ = $1;}
						;

/* End - ReqID:SRS-PLSQL-EXCEPTION */


unreserved_keyword	:
				K_ABSOLUTE
				/* Begin - SRS-PLSQL-SUBPROC */
				| K_ACCESSIBLE
				/* End - SRS-PLSQL-SUBPROC */
				| K_ALIAS
				| K_AND
				| K_ARRAY
				/* Begin - SRS-PLSQL-SUBPROC */
				| K_AS
				/* End - SRS-PLSQL-SUBPROC*/
				| K_ASSERT
				/* Begin - SRS-PLSQL-SUBPROC */
				| K_AUTHID
				/* End - SRS-PLSQL-SUBPROC */
				| K_BACKWARD
				/* Begin - ReqID:SRS-PLSQL-BULKCOLLECT */
				| K_BULK
				/* End - ReqID:SRS-PLSQL-BULKCOLLECT */
				| K_CALL
				| K_CHAIN
				| K_CLOSE
				| K_COLLATE
				/* Begin - SRS-PLSQL-SUBPROC */
				| K_COLLATION
				/* End - SRS-PLSQL-SUBPROC */
				/* Begin - ReqID:SRS-PLSQL-BULKCOLLECT */
				| K_COLLECT
				/* End - ReqID:SRS-PLSQL-BULKCOLLECT */
				| K_COLUMN
				| K_COLUMN_NAME
				| K_COMMIT
				| K_CONSTANT
				| K_CONSTRAINT
				| K_CONSTRAINT_NAME
				| K_CONTINUE
				| K_COUNT	/* ReqID:SRS-PLSQL-Collection */
				| K_CURRENT
				/* Begin - SRS-PLSQL-SUBPROC */
				| K_CURRENT_USER
				/* End - SRS-PLSQL-SUBPROC */
				| K_CURSOR
				| K_DATATYPE
				| K_DEBUG
				| K_DEFAULT
				/* Begin - SRS-PLSQL-SUBPROC */
				| K_DEFINER
				/* End - SRS-PLSQL-SUBPROC */
				| K_DETAIL
				/* Begin - SRS-PLSQL-SUBPROC */
				| K_DETERMINISTIC
				/* End - SRS-PLSQL-SUBPROC */
				| K_DIAGNOSTICS
				| K_DO
				| K_DUMP
				| K_ELSIF
				| K_ERRCODE
				| K_ERROR
				| K_EXCEPTION
				/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
				| K_EXCEPTION_INIT
				/* End - ReqID:SRS-PLSQL-EXCEPTION */
				| K_EXISTS	/* ReqID:SRS-PLSQL-Collection */
				| K_EXIT
				| K_EXTEND	/* ReqID:SRS-PLSQL-Collection */
				| K_FETCH
				| K_FIRST
				| K_FORWARD
				| K_GET
				| K_GOTO		/* ReqID:SRS-PLSQL-GOTO */
				| K_HINT
				| K_IMMEDIATE	/* BUG#Z1438 */
				| K_IMPORT
				| K_INDEX	/* ReqID:SRS-PLSQL-Collection */
				| K_INFO
				| K_INSERT
				| K_IS
				| K_LAST
				| K_LIMIT	/* ReqID:SRS-PLSQL-Collection */
				| K_LOG
				| K_MERGE	/* BUG#Z1389 */
				| K_MESSAGE
				| K_MESSAGE_TEXT
				| K_MOVE
				| K_NEXT
				| K_NO
				/* Begin - SRS-PLSQL-SUBPROC */
				| K_NOCOPY
				/* End - SRS-PLSQL-SUBPROC */
				| K_NOTICE
				| K_OPEN
				| K_OPTION
				/* Begin - SRS-PLSQL-SUBPROC */
				| K_OUT
				| K_PACKAGE
				| K_PARALLEL_ENABLE
				/* End - SRS-PLSQL-SUBPROC */
				| K_PERFORM
				| K_PG_CONTEXT
				| K_PG_DATATYPE_NAME
				| K_PG_EXCEPTION_CONTEXT
				| K_PG_EXCEPTION_DETAIL
				| K_PG_EXCEPTION_HINT
				/* Begin - SRS-PLSQL-SUBPROC */
				| K_PIPELINED
				/* End - SRS-PLSQL-SUBPROC */
				| K_PRINT_STRICT_PARAMS
				| K_PRIOR
				| K_QUERY
				| K_RAISE
				| K_RANGE	/* ReqID:SRS-PLSQL-SUBTYPE */
				/* Begin - ReqID:SRS-PLSQL-CURSOR */
				| K_REF
				/* End - ReqID:SRS-PLSQL-CURSOR */
				| K_RELATIVE
				/* Begin - SRS-PLSQL-SUBPROC */
				| K_RELIES_ON
				| K_RESULT_CACHE
				/* End - SRS-PLSQL-SUBPROC */
				| K_RETURN
				| K_RETURNED_SQLSTATE
				| K_REVERSE
				| K_ROLLBACK
				| K_ROW_COUNT
				| K_ROWTYPE
				| K_SCHEMA
				| K_SCHEMA_NAME
				| K_SCROLL
				| K_SELECT /* ReqID:SRS-PLSQL-CURSOR */
				| K_SLICE
				| K_SQLSTATE
				| K_STACKED
				| K_SUBTYPE	/* ReqID:SRS-PLSQL-SUBTYPE */
				| K_TABLE
				| K_TABLE_NAME
				/* Begin - SRS-PLSQL-SUBPROC */
				| K_TRIGGER
				/* End - SRS-PLSQL-SUBPROC */
				| K_TRIM	/* ReqID:SRS-PLSQL-Collection */
				| K_TYPE
				| K_USE_COLUMN
				| K_USE_VARIABLE
				/* Begin - SRS-PLSQL-SUBPROC */
				| K_USING_NLS_COMP
				/* End - SRS-PLSQL-SUBPROC */
				| K_VARIABLE_CONFLICT
				/* Begin - ReqID:SRS-PLSQL-Collection */
				| K_VARRAY
				| K_VARYING
				/* End - ReqID:SRS-PLSQL-Collection */
				| K_WARNING
				;

/* Begin - SRS-PLSQL-SUBPROC */
/*
 * This is to solve the shift/reduce conflict of the access by which
 * accept package/trigger/type package/trigger/type as unit_kind or unit_name
 * which removes the three keywords K_PACAGE, K_TYPE, K_TRIGGER on the basis
 * of the unreserved_keyword
 */
unit_name_keyword:
				K_ABSOLUTE
				| K_ACCESSIBLE
				| K_ALIAS
				| K_AND
				| K_ARRAY
				| K_AS
				| K_ASSERT
				| K_AUTHID
				| K_BACKWARD
				| K_CALL
				| K_CHAIN
				| K_CLOSE
				| K_COLLATE
				| K_COLLATION
				| K_COLUMN
				| K_COLUMN_NAME
				| K_COMMIT
				| K_CONSTANT
				| K_CONSTRAINT
				| K_CONSTRAINT_NAME
				| K_CONTINUE
				| K_COUNT	/* ReqID:SRS-PLSQL-Collection */
				| K_CURRENT
				| K_CURRENT_USER
				| K_CURSOR
				| K_DATATYPE
				| K_DEBUG
				| K_DEFAULT
				| K_DEFINER
				| K_DETAIL
				| K_DETERMINISTIC
				| K_DIAGNOSTICS
				| K_DO
				| K_DUMP
				| K_ELSIF
				| K_ERRCODE
				| K_ERROR
				| K_EXCEPTION
				/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
				| K_EXCEPTION_INIT
				/* End - ReqID:SRS-PLSQL-EXCEPTION */
				| K_EXISTS	/* ReqID:SRS-PLSQL-Collection */
				| K_EXIT
				| K_EXTEND	/* ReqID:SRS-PLSQL-Collection */
				| K_FETCH
				| K_FIRST
				| K_FORWARD
				| K_GET
				| K_GOTO		/* ReqID:SRS-PLSQL-GOTO */
				| K_HINT
				| K_IMMEDIATE	/* BUG#Z1438 */
				| K_IMPORT
				| K_INFO
				| K_INSERT
				| K_IS
				| K_LAST
				| K_LOG
				| K_MESSAGE
				| K_MESSAGE_TEXT
				| K_MOVE
				| K_NEXT
				| K_NO
				| K_NOCOPY
				| K_NOTICE
				| K_OPEN
				| K_OPTION
				| K_OUT
				| K_PARALLEL_ENABLE
				| K_PERFORM
				| K_PG_CONTEXT
				| K_PG_DATATYPE_NAME
				| K_PG_EXCEPTION_CONTEXT
				| K_PG_EXCEPTION_DETAIL
				| K_PG_EXCEPTION_HINT
				| K_PIPELINED
				| K_PRINT_STRICT_PARAMS
				| K_PRIOR
				| K_QUERY
				| K_RAISE
				| K_RANGE	/* ReqID:SRS-PLSQL-SUBTYPE */
				/* Begin - ReqID:SRS-PLSQL-CURSOR */
				| K_REF
				/* End - ReqID:SRS-PLSQL-CURSOR */
				| K_RELATIVE
				| K_RELIES_ON
				| K_RESULT_CACHE
				| K_RETURN
				| K_RETURNED_SQLSTATE
				| K_REVERSE
				| K_ROLLBACK
				| K_ROW_COUNT
				| K_ROWTYPE
				| K_SCHEMA
				| K_SCHEMA_NAME
				| K_SCROLL
				| K_SELECT /* ReqID:SRS-PLSQL-CURSOR */
				| K_SLICE
				| K_SQLSTATE
				| K_STACKED
				| K_SUBTYPE	/* ReqID:SRS-PLSQL-SUBTYPE */
				| K_TABLE
				| K_TABLE_NAME
				| K_USE_COLUMN
				| K_USE_VARIABLE
				| K_USING_NLS_COMP
				| K_VARIABLE_CONFLICT
				| K_WARNING
				;
/* End - SRS-PLSQL-SUBPROC */


%%

/*
 * Check whether a token represents an "unreserved keyword".
 * We have various places where we want to recognize a keyword in preference
 * to a variable name, but not reserve that keyword in other contexts.
 * Hence, this kluge.
 */
static bool
tok_is_keyword(int token, union YYSTYPE *lval,
			   int kw_token, const char *kw_str)
{
	if (token == kw_token)
	{
		/* Normal case, was recognized by scanner (no conflicting variable) */
		return true;
	}
	else if (token == T_DATUM)
	{
		/*
		 * It's a variable, so recheck the string name.  Note we will not
		 * match composite names (hence an unreserved word followed by "."
		 * will not be recognized).
		 */
		if (!lval->wdatum.quoted && lval->wdatum.ident != NULL &&
			strcmp(lval->wdatum.ident, kw_str) == 0)
			return true;
	}
	return false;				/* not the keyword */
}

/*
 * Convenience routine to complain when we expected T_DATUM and got T_WORD,
 * ie, unrecognized variable.
 */
static void
word_is_not_variable(PLword *word, int location)
{
	ereport(ERROR,
			(errcode(ERRCODE_SYNTAX_ERROR),
			 errmsg("\"%s\" is not a known variable",
					word->ident),
			 parser_errposition(location)));
}

/* Same, for a CWORD */
static void
cword_is_not_variable(PLcword *cword, int location)
{
	ereport(ERROR,
			(errcode(ERRCODE_SYNTAX_ERROR),
			 errmsg("\"%s\" is not a known variable",
					NameListToString(cword->idents)),
			 parser_errposition(location)));
}

/*
 * Convenience routine to complain when we expected T_DATUM and got
 * something else.  "tok" must be the current token, since we also
 * look at yylval and yylloc.
 */
static void
current_token_is_not_variable(int tok)
{
	if (tok == T_WORD)
		word_is_not_variable(&(yylval.word), yylloc);
	else if (tok == T_CWORD)
		cword_is_not_variable(&(yylval.cword), yylloc);
	else
		yyerror("syntax error");
}

/* Convenience routine to read an expression with one possible terminator */
static PLiSQL_expr *
read_sql_expression(int until, const char *expected)
{
	return read_sql_construct(until, 0, 0, 0, expected,
							  RAW_PARSE_PLISQL_EXPR, /* BUG:M0000079 */
							  true, true, true, NULL, NULL, false, NULL/* ReqID:SRS-PLSQL-Collection */);
}

/* Convenience routine to read an expression with two possible terminators */
static PLiSQL_expr *
read_sql_expression2(int until, int until2, const char *expected,
					 int *endtoken)
{
	return read_sql_construct(until, until2, 0, 0, expected,
							  RAW_PARSE_PLISQL_EXPR, /* BUG:M0000079 */
							  true, true, true, NULL, endtoken, false, NULL/* ReqID:SRS-PLSQL-Collection */);
}

/* Convenience routine to read a SQL statement that must end with ';' */
static PLiSQL_expr *
read_sql_stmt(void)
{
	return read_sql_construct(';', 0, 0, 0, ";",
							  RAW_PARSE_DEFAULT,
							  false, true, true, NULL, NULL, false, NULL/* ReqID:SRS-PLSQL-Collection */);
}

/*
 * Read a SQL construct and build a PLiSQL_expr for it.
 *
 * until:		token code for expected terminator
 * until2:		token code for alternate terminator (pass 0 if none)
 * until3:		token code for another alternate terminator (pass 0 if none)
 * until4:		token code for another alternate terminator (pass 0 if none)
 * expected:	text to use in complaining that terminator was not found
 * parsemode:	raw_parser() mode to use
 * isexpression: whether to say we're reading an "expression" or a "statement"
 * valid_sql:   whether to check the syntax of the expr
 * trim:		trim trailing whitespace
 * startloc:	if not NULL, location of first token is stored at *startloc
 * endtoken:	if not NULL, ending token is stored at *endtoken
 *				(this is only interesting if until2 or until3 isn't zero)
 */
static PLiSQL_expr *
read_sql_construct(int until,
				   int until2,
				   int until3,
				   int until4,	/* ReqID:SRS-PLSQL-BULKCOLLECT */
				   const char *expected,
				   RawParseMode parsemode,
				   bool isexpression,
				   bool valid_sql,
				   bool trim,
				   int *startloc,
				   int *endtoken,
				   bool check_assignable_datum, /* ReqID:SRS-PLSQL-DSQL */
				   bool *ismulticollectmethod /* ReqID:SRS-PLSQL-Collection */)
{
	int					tok;
	StringInfoData		ds;
	IdentifierLookup	save_IdentifierLookup;
	int					startlocation = -1;
	int					parenlevel = 0;
	PLiSQL_expr		*expr;
	/* Begin - ReqID:SRS-PLSQL-CURSOR */
	int					append_startlocation = -1;
	bool				pre_token_isoracursor = false;
	bool				pre_token_is_explicitcursor = false;
	/* End - ReqID:SRS-PLSQL-CURSOR */
	int					datum_dno = -1;
	/* Begin - ReqID:SRS-PLSQL-Collection */
	Oid 				pkgoid = InvalidOid;
	int					collect_startlocation = -1;
	bool				hascollect = false;
	bool				has_colon_equals = false;
	/* End - ReqID:SRS-PLSQL-Collection */

	initStringInfo(&ds);

	/* special lookup mode for identifiers within the SQL text */
	save_IdentifierLookup = plisql_IdentifierLookup;
	plisql_IdentifierLookup = IDENTIFIER_LOOKUP_NORMAL; /* ReqID:SRS-PLSQL-CURSOR */

	for (;;)
	{
		tok = yylex();
		if (startlocation < 0)			/* remember loc of first token */
			startlocation = yylloc;
		if (tok == until && parenlevel == 0)
			break;
		if (tok == until2 && parenlevel == 0)
			break;
		if (tok == until3 && parenlevel == 0)
			break;
		/* Begin - ReqID:SRS-PLSQL-BULKCOLLECT */
		if (tok == until4 && parenlevel == 0)
			break;
		/* End - ReqID:SRS-PLSQL-BULKCOLLECT */
		/* Begin - ReqID:SRS-PLSQL-DSQL */
		if (parsemode == RAW_PARSE_PLISQL_EXPR && 
			(tok == K_RETURN || tok == K_RETURNING) && 
			parenlevel == 0)
			break;
		/* End - ReqID:SRS-PLSQL-DSQL */
		if (tok == '(' || tok == '[')
			parenlevel++;
		else if (tok == ')' || tok == ']')
		{
			parenlevel--;
			if (parenlevel < 0)
				yyerror("mismatched parentheses");
		}

		/* Begin - ReqID:SRS-PLSQL-Collection */
		if (tok == '=' ||
			tok == COLON_EQUALS)
			has_colon_equals = true;
		/* End - ReqID:SRS-PLSQL-Collection */

		/*
		 * End of function definition is an error, and we don't expect to
		 * hit a semicolon either (unless it's the until symbol, in which
		 * case we should have fallen out above).
		 */
		if (tok == 0 || tok == ';')
		{
			if (parenlevel != 0)
				yyerror("mismatched parentheses");
			if (isexpression)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("missing \"%s\" at end of SQL expression",
								expected),
						 parser_errposition(yylloc)));
			else
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("missing \"%s\" at end of SQL statement",
								expected),
						 parser_errposition(yylloc)));
		}
		/* Begin - ReqID:SRS-PLSQL-CURSOR */
		if (append_startlocation < 0)
			append_startlocation = startlocation;
		if (tok == '%' && pre_token_isoracursor)
		{
			/* replace % to . */
			plisql_append_source_text(&ds, append_startlocation, yylloc);
			appendBinaryStringInfo(&ds, ".", 1);
			append_startlocation = yylloc + 1;
		}
		else if (pre_token_is_explicitcursor)
			elog(ERROR, "expression is of wrong type of");
		if (tok == T_DATUM && yylval.wdatum.datum != NULL /* ReqID:SRS-PLSQL-Collection */)
		{
			PLiSQL_datum *datum = yylval.wdatum.datum;

			if (is_oracursor_datum(datum))
			{
				PLiSQL_var *cursor_var = NULL;

				if (datum->dtype == PLISQL_DTYPE_PACKAGE_DATUM)
				{
					PLiSQL_pkg_datum *pkg_datum = (PLiSQL_pkg_datum *) datum;

					cursor_var = (PLiSQL_var *) pkg_datum->pkgvar;
				}
				else
					cursor_var = (PLiSQL_var *) datum;
				pre_token_is_explicitcursor = cursor_var->is_explicit_cursor;
				pre_token_isoracursor = true;
			}

			/* Begin - ReqID:SRS-PLSQL-DSQL */
			datum_dno = yylval.wdatum.datum->dno;

			/* Check OUT/INOUT datum. */
			if (check_assignable_datum)
				check_assignable(datum, yylloc);
			/* End - ReqID:SRS-PLSQL-DSQL */
		}
		else
		{
			pre_token_isoracursor = false;
			pre_token_is_explicitcursor = false;
		}
		/* End - ReqID:SRS-PLSQL-CURSOR */

		/* Begin - ReqID:SRS-PLSQL-Collection */
		collect_startlocation = read_collection_type_expr(tok, startlocation, &ds, &pkgoid, &datum_dno);
		if (collect_startlocation > startlocation)
		{
			startlocation = collect_startlocation;
			hascollect = true;
		}
		/* End - ReqID:SRS-PLSQL-Collection */
	}

	/* Begin - ReqID:SRS-PLSQL-CURSOR */
	if (pre_token_is_explicitcursor)
		elog(ERROR, "expression is of wrong type");
	/* End - ReqID:SRS-PLSQL-CURSOR */

	plisql_IdentifierLookup = save_IdentifierLookup;

	if (startloc)
		*startloc = startlocation;
	if (endtoken)
		*endtoken = tok;

	/* give helpful complaint about empty input */
	if (append_startlocation >= yylloc && !hascollect) /* ReqID:SRS-PLSQL-CURSOR */
	{
		if (isexpression)
			yyerror("missing expression");
		else
			yyerror("missing SQL statement");
	}

	/* Begin - ReqID:SRS-PLSQL-Collection */
	if (hascollect)
	{
		if (startlocation <= yylloc)
		{
			plisql_append_source_text(&ds, startlocation, yylloc);

			if (!has_colon_equals &&
				(parsemode == RAW_PARSE_PLISQL_ASSIGN1 ||
				parsemode == RAW_PARSE_PLISQL_ASSIGN2 ||
				parsemode == RAW_PARSE_PLISQL_ASSIGN3 ||
				parsemode == RAW_PARSE_PLISQL_ASSIGNN) &&
				ismulticollectmethod != NULL)
			{
				int		original_len = ds.len;

				if (original_len > 0)
				{
					char	*original_data = pstrdup(ds.data);

					/* reset the ds, but not free the memory */
					resetStringInfo(&ds);

					/* add "SELECT" to the begin of the ds */
					appendStringInfoString(&ds, "SELECT ");
					appendBinaryStringInfo(&ds, original_data, original_len);
					pfree(original_data);
					parsemode = RAW_PARSE_DEFAULT;
					*ismulticollectmethod = true;
				}
			}
		}
	}
	else
	/* End - ReqID:SRS-PLSQL-Collection */
		plisql_append_source_text(&ds, append_startlocation, yylloc); /* ReqID:SRS-PLSQL-CURSOR */

	/* trim any trailing whitespace, for neatness */
	if (trim)
	{
		while (ds.len > 0 && scanner_isspace(ds.data[ds.len - 1]))
			ds.data[--ds.len] = '\0';
	}

	expr = palloc0(sizeof(PLiSQL_expr));
	expr->query			= pstrdup(ds.data);
	expr->parseMode		= parsemode;
	expr->plan			= NULL;
	expr->paramnos		= NULL;
	expr->target_param	= -1;
	expr->ns			= plisql_ns_top();
	expr->datum_dno		= datum_dno;
	pfree(ds.data);

	if (valid_sql)
		check_sql_expr(expr->query, expr->parseMode, startlocation);

	/* Begin - BUG#M0000376 */
	/*
	 * if enable_get_oraparam_mode is true, and plisql_curr_compile is an anonymous block,
	 * do transform the expr to get the placeholder mode.
	 * e.g., id = func(:x, :y); the :x and :y are placeholder,
	 * to get the mode, must transform the function func.
	 */
	if (enable_get_oraparam_mode &&
		plisql_curr_compile->fn_prokind == PROKIND_ANONYMOUS_BLOCK)
	{
		do_transform_for_anonyblock_expr(expr);
	}
	/* End - BUG#M0000376 */

	return expr;
}

static PLiSQL_type *
read_datatype(int tok)
{
	StringInfoData		ds;
	char			   *type_name;
	int					startlocation;
	PLiSQL_type		*result;
	int					parenlevel = 0;

	/* Should only be called while parsing DECLARE sections */
	Assert(plisql_IdentifierLookup == IDENTIFIER_LOOKUP_DECLARE);

	/* Often there will be a lookahead token, but if not, get one */
	if (tok == YYEMPTY)
		tok = yylex();

	startlocation = yylloc;

	/*
	 * If we have a simple or composite identifier, check for %TYPE
	 * and %ROWTYPE constructs.
	 */
	if (tok == T_WORD)
	{
		char   *dtname = yylval.word.ident;

		tok = yylex();
		if (tok == '%')
		{
			tok = yylex();
			if (tok_is_keyword(tok, &yylval,
							   K_TYPE, "type"))
			{
				result = plisql_parse_wordtype(dtname);
				if (result)
					return result;
			}
			else if (tok_is_keyword(tok, &yylval,
									K_ROWTYPE, "rowtype"))
			{
				result = plisql_parse_wordrowtype(dtname);
				if (result)
					return result;
			}
		}
	}
	else if (plisql_token_is_unreserved_keyword(tok))
	{
		char   *dtname = pstrdup(yylval.keyword);

		tok = yylex();
		if (tok == '%')
		{
			tok = yylex();
			if (tok_is_keyword(tok, &yylval,
							   K_TYPE, "type"))
			{
				result = plisql_parse_wordtype(dtname);
				if (result)
					return result;
			}
			else if (tok_is_keyword(tok, &yylval,
									K_ROWTYPE, "rowtype"))
			{
				result = plisql_parse_wordrowtype(dtname);
				if (result)
					return result;
			}
		}
	}
	else if (tok == T_CWORD)
	{
		List   *dtnames = yylval.cword.idents;

		tok = yylex();
		if (tok == '%')
		{
			tok = yylex();
			if (tok_is_keyword(tok, &yylval,
							   K_TYPE, "type"))
			{
				result = plisql_parse_cwordtype(dtnames);
				if (result)
					return result;
			}
			else if (tok_is_keyword(tok, &yylval,
									K_ROWTYPE, "rowtype"))
			{
				result = plisql_parse_cwordrowtype(dtnames);
				if (result)
					return result;
			}
		}
	}

	while (tok != ';')
	{
		if (tok == 0)
		{
			if (parenlevel != 0)
				yyerror("mismatched parentheses");
			else
				yyerror("incomplete data type declaration");
		}
		/* Possible followers for datatype in a declaration */
		if (tok == K_COLLATE || tok == K_NOT ||
			tok == '=' || tok == COLON_EQUALS || tok == K_DEFAULT)
			break;
		/* Possible followers for datatype in a cursor_arg list */
		if ((tok == ',' || tok == ')') && parenlevel == 0)
			break;
		/* Begin - SRS-PLSQL-SUBPROC */
		if ((tok == K_AS || tok == K_IS || tok == K_RESULT_CACHE ||
			tok == K_PARALLEL_ENABLE || tok == K_DETERMINISTIC ||
			tok == K_PIPELINED) && parenlevel == 0)
			break;
		/* End - SRS-PLSQL-SUBPROC */

		/* Begin - ReqID:SRS-PLSQL-SUBTYPE */
		if (tok == K_RANGE && parenlevel == 0)
			break;
		/* End - ReqID:SRS-PLSQL-SUBTYPE */

		/* Begin - ReqID:SRS-PLSQL-Collection */
		if (tok == K_INDEX)
			break;
		/* End - ReqID:SRS-PLSQL-Collection */

		if (tok == '(')
			parenlevel++;
		else if (tok == ')')
			parenlevel--;

		tok = yylex();
	}

	/* set up ds to contain complete typename text */
	initStringInfo(&ds);
	plisql_append_source_text(&ds, startlocation, yylloc);
	type_name = ds.data;

	if (type_name[0] == '\0')
		yyerror("missing data type declaration");

	result = parse_datatype(type_name, startlocation);

	pfree(ds.data);

	plisql_push_back_token(tok);

	return result;
}

static PLiSQL_stmt *
make_execsql_stmt(int firsttoken, int location)
{
	StringInfoData		ds;
	IdentifierLookup	save_IdentifierLookup;
	PLiSQL_stmt_execsql *execsql;
	PLiSQL_expr		*expr;
	PLiSQL_variable	*target = NULL;
	int					tok;
	int					prev_tok = 0;	/* BUG#Z375 */	
	bool				have_into = false;
	bool				have_strict = false;
	int					into_start_loc = -1;
	int					into_end_loc = -1;
	/* Begin - ReqID:SRS-PLSQL-Collection */
	bool				have_collect = false;
	int					collect_start_loc = -1;
	int					collect_end_loc = -1;
	StringInfoData		ds_collect;
	Oid					pkgoid;
	/* End - ReqID:SRS-PLSQL-Collection */
	/* Begin - ReqID:SRS-PLSQL-BULKCOLLECT */
	bool				have_bulk_collect_into = false;
	int					bulkcollect_start_loc = -1;
	int					bulkcollect_end_loc = -1;
	/* End - ReqID:SRS-PLSQL-BULKCOLLECT */

	/* Begin - SRS-SQL-MULTITABLEINSERT */
	bool				is_multi_insert = false;
	/* End - SRS-SQL-MULTITABLEINSERT */

	initStringInfo(&ds);
	initStringInfo(&ds_collect); /* ReqID:SRS-PLSQL-Collection */

	/* special lookup mode for identifiers within the SQL text */
	save_IdentifierLookup = plisql_IdentifierLookup;

	/* Begin - ReqID:SRS-PLSQL-Collection */
	/*
	 * because need to parse the the collection variable,
	 * so change the plisql_IdentifierLookup from IDENTIFIER_LOOKUP_EXPR
	 * to IDENTIFIER_LOOKUP_NORMAL.
	 */
	plisql_IdentifierLookup = IDENTIFIER_LOOKUP_NORMAL;
	/* End - ReqID:SRS-PLSQL-Collection */

	/*
	 * Scan to the end of the SQL command.  Identify any INTO-variables
	 * clause lurking within it, and parse that via read_into_target().
	 *
	 * Because INTO is sometimes used in the main SQL grammar, we have to be
	 * careful not to take any such usage of INTO as a PL/iSQL INTO clause.
	 * There are currently three such cases:
	 *
	 * 1. SELECT ... INTO.  We don't care, we just override that with the
	 * PL/iSQL definition.
	 *
	 * 2. INSERT INTO.  This is relatively easy to recognize since the words
	 * must appear adjacently; but we can't assume INSERT starts the command,
	 * because it can appear in CREATE RULE or WITH.  Unfortunately, INSERT is
	 * *not* fully reserved, so that means there is a chance of a false match;
	 * but it's not very likely.
	 *
	 * 3. IMPORT FOREIGN SCHEMA ... INTO.  This is not allowed in CREATE RULE
	 * or WITH, so we just check for IMPORT as the command's first token.
	 * (If IMPORT FOREIGN SCHEMA returned data someone might wish to capture
	 * with an INTO-variables clause, we'd have to work much harder here.)
	 *
	 * Fortunately, INTO is a fully reserved word in the main grammar, so
	 * at least we need not worry about it appearing as an identifier.
	 *
	 * Any future additional uses of INTO in the main grammar will doubtless
	 * break this logic again ... beware!
	 */
	tok = firsttoken;
	for (;;)
	{
		prev_tok = tok;
		tok = yylex();
		if (have_into && into_end_loc < 0)
			into_end_loc = yylloc;		/* token after the INTO part */
		/* Begin - ReqID:SRS-PLSQL-BULKCOLLECT */
		if (have_bulk_collect_into && bulkcollect_end_loc < 0)
			bulkcollect_end_loc = yylloc;
		/* End - ReqID:SRS-PLSQL-BULKCOLLECT */
		if (tok == ';')
			break;
		if (tok == 0)
			yyerror("unexpected end of function definition");

		/* Begin - SRS-SQL-MULTITABLEINSERT */
		/*
		 * record insert all/first
		 */
		if (prev_tok == K_INSERT &&
			(tok == K_ALL || tok == K_FIRST))
		{
			is_multi_insert = true;
		}
		/* End - SRS-SQL-MULTITABLEINSERT */

		if (tok == K_INTO)
		{
			if (prev_tok == K_INSERT)
				continue;		/* INSERT INTO is not an INTO-target */
			if (prev_tok == K_MERGE)	/* BUG#Z1389 */
				continue;		/* MERGE INTO is not an INTO-target */
			if (firsttoken == K_IMPORT)
				continue;		/* IMPORT ... INTO is not an INTO-target */

			/* Begin - SRS-SQL-MULTITABLEINSERT */
			if (is_multi_insert)
				continue;	/* insert all/first is not an INTO-target */

			/* End - SRS-SQL-MULTITABLEINSERT */
			if (have_into)
				yyerror("INTO specified more than once");
			have_into = true;
			into_start_loc = yylloc;
			read_into_target(&target, &have_strict);
		}

		/* Begin - ReqID:SRS-PLSQL-BULKCOLLECT */
		if (tok == K_BULK)
		{
			bulkcollect_start_loc = yylloc;

			tok = yylex();
			if (tok == K_COLLECT)
			{
				tok = yylex();
				if (tok == K_INTO)
				{
					if (have_bulk_collect_into || have_into)
						ereport(ERROR,
								(errcode(ERRCODE_SYNTAX_ERROR),
								 errmsg("INTO and BULK COLLECT INTO can't be used together")));

					have_bulk_collect_into = true;
					read_bulk_collect_into_target(&target);
				}
				else
					plisql_push_back_token(tok);
			}
			else
				plisql_push_back_token(tok);
		}
		/* End - ReqID:SRS-PLSQL-BULKCOLLECT */

		/* Begin - ReqID:SRS-PLSQL-Collection */
		if (tok == T_DATUM || tok == T_CWORD)
		{
			if (!have_collect && collect_end_loc == -1)
			{
				int current_loc = yylloc;

				collect_start_loc = yylloc;
				collect_end_loc = read_collection_type_expr(tok, yylloc, &ds_collect, &pkgoid, NULL);
				if (collect_end_loc > current_loc)
					have_collect = true;
				else
				{
					collect_start_loc = -1;
					collect_end_loc = -1;
				}
			}
			else
			{
				int 	current_loc = yylloc;
				int 	tmp_loc = -1;
				StringInfoData		ds_tmp;

				initStringInfo(&ds_tmp);
				tmp_loc = read_collection_type_expr(tok, yylloc, &ds_tmp, &pkgoid, NULL);
				if (tmp_loc > current_loc)
				{
					appendStringInfoSpaces(&ds_collect, 1);
					plisql_append_source_text(&ds_collect, collect_end_loc, current_loc);

					appendBinaryStringInfo(&ds_collect, ds_tmp.data, ds_tmp.len);
					collect_end_loc = tmp_loc;
				}
				pfree(ds_tmp.data);
			}
		}
		/* End - ReqID:SRS-PLSQL-Collection */
	}

	plisql_IdentifierLookup = save_IdentifierLookup;

	if (have_into)
	{
		/* Begin - ReqID:SRS-PLSQL-Collection */
		if (have_collect)
		{
			if (into_start_loc < collect_start_loc)
			{
				plisql_append_source_text(&ds, location, into_start_loc);
				appendStringInfoSpaces(&ds, into_end_loc - into_start_loc);

				plisql_append_source_text(&ds, into_end_loc, collect_start_loc);
				appendBinaryStringInfo(&ds, ds_collect.data, ds_collect.len);
				if (collect_end_loc != yylloc)
				{
					appendStringInfoSpaces(&ds, 1);
					plisql_append_source_text(&ds, collect_end_loc, yylloc);
				}
			}
			else
			{
				plisql_append_source_text(&ds, location, collect_start_loc);
				appendBinaryStringInfo(&ds, ds_collect.data, ds_collect.len);
				appendStringInfoSpaces(&ds, 1);
				plisql_append_source_text(&ds, collect_end_loc, into_start_loc);
				appendStringInfoSpaces(&ds, into_end_loc - into_start_loc);
			}
		}
		else
		{
		/* End - ReqID:SRS-PLSQL-Collection */
			plisql_append_source_text(&ds, location, into_start_loc);
			appendStringInfoSpaces(&ds, into_end_loc - into_start_loc);
			plisql_append_source_text(&ds, into_end_loc, yylloc);
		}
	}
	/* Begin - ReqID:SRS-PLSQL-BULKCOLLECT */
	else if (have_bulk_collect_into)
	{
		/*
		 * Insert an appropriate number of spaces corresponding to the
		 * BULK COLLECT INTO text, so that locations within the redacted SQL statement
		 * still line up with those in the original source text.
		 */
		if (have_collect)
		{
			if (bulkcollect_start_loc < collect_start_loc)
			{
				plisql_append_source_text(&ds, location, bulkcollect_start_loc);
				appendStringInfoSpaces(&ds, bulkcollect_end_loc - bulkcollect_start_loc);

				plisql_append_source_text(&ds, bulkcollect_end_loc, collect_start_loc);
				appendBinaryStringInfo(&ds, ds_collect.data, ds_collect.len);
				if (collect_end_loc != yylloc)
				{
					appendStringInfoSpaces(&ds, 1);
					plisql_append_source_text(&ds, collect_end_loc, yylloc);
				}
			}
			else
			{
				plisql_append_source_text(&ds, location, collect_start_loc);
				appendBinaryStringInfo(&ds, ds_collect.data, ds_collect.len);
				appendStringInfoSpaces(&ds, 1);
				plisql_append_source_text(&ds, collect_end_loc, bulkcollect_start_loc);
				appendStringInfoSpaces(&ds, bulkcollect_end_loc - bulkcollect_start_loc);
			}
		}
		else
		{
			plisql_append_source_text(&ds, location, bulkcollect_start_loc);
			appendStringInfoSpaces(&ds, bulkcollect_end_loc - bulkcollect_start_loc);
			plisql_append_source_text(&ds, bulkcollect_end_loc, yylloc);
		}
	}
	/* End - ReqID:SRS-PLSQL-BULKCOLLECT */
	else
	{
		/* Begin - ReqID:SRS-PLSQL-Collection */
		if (have_collect)
		{
			plisql_append_source_text(&ds, location, collect_start_loc);
			appendBinaryStringInfo(&ds, ds_collect.data, ds_collect.len);
			if (collect_end_loc != yylloc)
			{
				appendStringInfoSpaces(&ds, 1);
				plisql_append_source_text(&ds, collect_end_loc, yylloc);
			}
		}
		else
		/* End - ReqID:SRS-PLSQL-Collection */
			plisql_append_source_text(&ds, location, yylloc);
	}

	/* trim any trailing whitespace, for neatness */
	while (ds.len > 0 && scanner_isspace(ds.data[ds.len - 1]))
		ds.data[--ds.len] = '\0';

	expr = palloc0(sizeof(PLiSQL_expr));
	expr->query			= pstrdup(ds.data);
	expr->parseMode		= RAW_PARSE_DEFAULT;
	expr->plan			= NULL;
	expr->paramnos		= NULL;
	expr->target_param	= -1;
	expr->ns			= plisql_ns_top();
	expr->datum_dno		= -1;
	pfree(ds.data);
	pfree(ds_collect.data); /* ReqID:SRS-PLSQL-Collection */

	check_sql_expr(expr->query, expr->parseMode, location);

	execsql = palloc0(sizeof(PLiSQL_stmt_execsql));
	execsql->cmd_type = PLISQL_STMT_EXECSQL;
	execsql->lineno  = plisql_location_to_lineno(location);
	execsql->stmtid  = ++plisql_curr_compile->nstatements;
	execsql->sqlstmt = expr;
	execsql->into	 = have_into;
	execsql->strict	 = have_strict;
	execsql->target	 = target;
	/* Begin - ReqID:SRS-PLSQL-BULKCOLLECT */
	execsql->bulk_collect_into = have_bulk_collect_into;
	/* End - ReqID:SRS-PLSQL-BULKCOLLECT */

	/* Begin - BUG#M0000376 */
	/*
	 * if enable_get_oraparam_mode is true, and plisql_curr_compile is an anonymous block,
	 * do transform the expr to get the placeholder mode.
	 * e.g., id = func(:x, :y); the :x and :y are placeholder,
	 * to get the mode, must transform the function func.
	 */
	if (enable_get_oraparam_mode &&
		plisql_curr_compile->fn_prokind == PROKIND_ANONYMOUS_BLOCK)
	{
		do_transform_for_anonyblock_expr(expr);
	}
	/* End - BUG#M0000376 */

	return (PLiSQL_stmt *) execsql;
}


/*
 * Read FETCH or MOVE direction clause (everything through FROM/IN).
 */
static PLiSQL_stmt_fetch *
read_fetch_direction(void)
{
	PLiSQL_stmt_fetch *fetch;
	int			tok;
	bool		check_FROM = true;

	/*
	 * We create the PLiSQL_stmt_fetch struct here, but only fill in
	 * the fields arising from the optional direction clause
	 */
	fetch = (PLiSQL_stmt_fetch *) palloc0(sizeof(PLiSQL_stmt_fetch));
	fetch->cmd_type = PLISQL_STMT_FETCH;
	fetch->stmtid	= ++plisql_curr_compile->nstatements;
	/* set direction defaults: */
	fetch->direction = FETCH_FORWARD;
	fetch->how_many  = 1;
	fetch->expr		 = NULL;
	fetch->returns_multiple_rows = false;

	tok = yylex();
	if (tok == 0)
		yyerror("unexpected end of function definition");

	if (tok_is_keyword(tok, &yylval,
					   K_NEXT, "next"))
	{
		/* use defaults */
	}
	else if (tok_is_keyword(tok, &yylval,
							K_PRIOR, "prior"))
	{
		fetch->direction = FETCH_BACKWARD;
	}
	else if (tok_is_keyword(tok, &yylval,
							K_FIRST, "first"))
	{
		fetch->direction = FETCH_ABSOLUTE;
	}
	else if (tok_is_keyword(tok, &yylval,
							K_LAST, "last"))
	{
		fetch->direction = FETCH_ABSOLUTE;
		fetch->how_many  = -1;
	}
	else if (tok_is_keyword(tok, &yylval,
							K_ABSOLUTE, "absolute"))
	{
		fetch->direction = FETCH_ABSOLUTE;
		fetch->expr = read_sql_expression2(K_FROM, K_IN,
										   "FROM or IN",
										   NULL);
		check_FROM = false;
	}
	else if (tok_is_keyword(tok, &yylval,
							K_RELATIVE, "relative"))
	{
		fetch->direction = FETCH_RELATIVE;
		fetch->expr = read_sql_expression2(K_FROM, K_IN,
										   "FROM or IN",
										   NULL);
		check_FROM = false;
	}
	else if (tok_is_keyword(tok, &yylval,
							K_ALL, "all"))
	{
		fetch->how_many = FETCH_ALL;
		fetch->returns_multiple_rows = true;
	}
	else if (tok_is_keyword(tok, &yylval,
							K_FORWARD, "forward"))
	{
		complete_direction(fetch, &check_FROM);
	}
	else if (tok_is_keyword(tok, &yylval,
							K_BACKWARD, "backward"))
	{
		fetch->direction = FETCH_BACKWARD;
		complete_direction(fetch, &check_FROM);
	}
	else if (tok == K_FROM || tok == K_IN)
	{
		/* empty direction */
		check_FROM = false;
	}
	else if (tok == T_DATUM)
	{
		/* Assume there's no direction clause and tok is a cursor name */
		plisql_push_back_token(tok);
		check_FROM = false;
	}
	else
	{
		/*
		 * Assume it's a count expression with no preceding keyword.
		 * Note: we allow this syntax because core SQL does, but we don't
		 * document it because of the ambiguity with the omitted-direction
		 * case.  For instance, "MOVE n IN c" will fail if n is a variable.
		 * Perhaps this can be improved someday, but it's hardly worth a
		 * lot of work.
		 */
		plisql_push_back_token(tok);
		fetch->expr = read_sql_expression2(K_FROM, K_IN,
										   "FROM or IN",
										   NULL);
		fetch->returns_multiple_rows = true;
		check_FROM = false;
	}

	/* check FROM or IN keyword after direction's specification */
	if (check_FROM)
	{
		tok = yylex();
		if (tok != K_FROM && tok != K_IN)
			yyerror("expected FROM or IN");
	}

	return fetch;
}

/*
 * Process remainder of FETCH/MOVE direction after FORWARD or BACKWARD.
 * Allows these cases:
 *   FORWARD expr,  FORWARD ALL,  FORWARD
 *   BACKWARD expr, BACKWARD ALL, BACKWARD
 */
static void
complete_direction(PLiSQL_stmt_fetch *fetch,  bool *check_FROM)
{
	int			tok;

	tok = yylex();
	if (tok == 0)
		yyerror("unexpected end of function definition");

	if (tok == K_FROM || tok == K_IN)
	{
		*check_FROM = false;
		return;
	}

	if (tok == K_ALL)
	{
		fetch->how_many = FETCH_ALL;
		fetch->returns_multiple_rows = true;
		*check_FROM = true;
		return;
	}

	plisql_push_back_token(tok);
	fetch->expr = read_sql_expression2(K_FROM, K_IN,
									   "FROM or IN",
									   NULL);
	fetch->returns_multiple_rows = true;
	*check_FROM = false;
}


static PLiSQL_stmt *
make_return_stmt(int location)
{
	PLiSQL_stmt_return *new;

	new = palloc0(sizeof(PLiSQL_stmt_return));
	new->cmd_type = PLISQL_STMT_RETURN;
	new->lineno   = plisql_location_to_lineno(location);
	new->stmtid	  = ++plisql_curr_compile->nstatements;
	new->expr	  = NULL;
	new->retvarno = -1;

	if (plisql_curr_compile->fn_retset)
	{
		if (yylex() != ';')
			ereport(ERROR,
					(errcode(ERRCODE_DATATYPE_MISMATCH),
					 errmsg("RETURN cannot have a parameter in function returning set"),
					 errhint("Use RETURN NEXT or RETURN QUERY."),
					 parser_errposition(yylloc)));
	}
	else if (plisql_curr_compile->fn_rettype == VOIDOID)
	{
		if (yylex() != ';')
		{
			if (plisql_curr_compile->fn_prokind == PROKIND_PROCEDURE)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("RETURN cannot have a parameter in a procedure"),
						 parser_errposition(yylloc)));
			else
				ereport(ERROR,
						(errcode(ERRCODE_DATATYPE_MISMATCH),
						 errmsg("RETURN cannot have a parameter in function returning void"),
						 parser_errposition(yylloc)));
		}
	}
	else if (plisql_curr_compile->out_param_varno >= 0)
	{
		/* Begin - ReqID:SRS-SQL-FUNCOUT */
		int		tok = yylex();

		if (tok != ';')
		{
			if (plisql_curr_compile->fn_prokind == PROKIND_PROCEDURE)
				ereport(ERROR,
								(errcode(ERRCODE_DATATYPE_MISMATCH),
								 errmsg("RETURN cannot have a parameter in procedure"),
								 parser_errposition(yylloc)));

			/* treat the return variable as expression */
			plisql_push_back_token(tok);
			new->expr = read_sql_expression(';', ";");
			new->retvarno = plisql_curr_compile->fn_ret_vardno;
		}
		else
		{
			if (plisql_curr_compile->fn_prokind == PROKIND_FUNCTION &&
				plisql_curr_compile->fn_ret_vardno != -1 &&
				plisql_curr_compile->datums != NULL &&
				((PLiSQL_var *)plisql_curr_compile->datums[plisql_curr_compile->fn_ret_vardno])->datatype->typoid != VOIDOID)
				ereport(ERROR,
								(errcode(ERRCODE_DATATYPE_MISMATCH),
								 errmsg("RETURN cannot have a parameter in function with OUT parameters"),
								 parser_errposition(yylloc)));
			new->retvarno = plisql_curr_compile->out_param_varno;
		}
		/* End - ReqID:SRS-SQL-FUNCOUT */
	}
	else
	{
		/*
		 * We want to special-case simple variable references for efficiency.
		 * So peek ahead to see if that's what we have.
		 */
		int		tok = yylex();

		if (tok == T_DATUM && plisql_peek() == ';' &&
			(yylval.wdatum.datum->dtype == PLISQL_DTYPE_VAR ||
			yylval.wdatum.datum->dtype == PLISQL_DTYPE_PACKAGE_DATUM || /* ReqID:SRS-SQL-PACKAGE */
			yylval.wdatum.datum->dtype == PLISQL_DTYPE_ATTRIBUTE || /* ReqID:SRS-PLSQL-CURSOR */
			 yylval.wdatum.datum->dtype == PLISQL_DTYPE_PROMISE ||
			 yylval.wdatum.datum->dtype == PLISQL_DTYPE_ROW ||
			 yylval.wdatum.datum->dtype == PLISQL_DTYPE_REC))
		{
			new->retvarno = yylval.wdatum.datum->dno;
			/* eat the semicolon token that we only peeked at above */
			tok = yylex();
			Assert(tok == ';');
		}
		else
		{
			/*
			 * Not (just) a variable name, so treat as expression.
			 *
			 * Note that a well-formed expression is _required_ here;
			 * anything else is a compile-time error.
			 */
			plisql_push_back_token(tok);
			new->expr = read_sql_expression(';', ";");
		}
	}

	return (PLiSQL_stmt *) new;
}


static PLiSQL_stmt *
make_return_next_stmt(int location)
{
	PLiSQL_stmt_return_next *new;

	if (!plisql_curr_compile->fn_retset)
		ereport(ERROR,
				(errcode(ERRCODE_DATATYPE_MISMATCH),
				 errmsg("cannot use RETURN NEXT in a non-SETOF function"),
				 parser_errposition(location)));

	new = palloc0(sizeof(PLiSQL_stmt_return_next));
	new->cmd_type	= PLISQL_STMT_RETURN_NEXT;
	new->lineno		= plisql_location_to_lineno(location);
	new->stmtid		= ++plisql_curr_compile->nstatements;
	new->expr		= NULL;
	new->retvarno	= -1;

	if (plisql_curr_compile->out_param_varno >= 0)
	{
		if (yylex() != ';')
			ereport(ERROR,
					(errcode(ERRCODE_DATATYPE_MISMATCH),
					 errmsg("RETURN NEXT cannot have a parameter in function with OUT parameters"),
					 parser_errposition(yylloc)));
		new->retvarno = plisql_curr_compile->out_param_varno;
	}
	else
	{
		/*
		 * We want to special-case simple variable references for efficiency.
		 * So peek ahead to see if that's what we have.
		 */
		int		tok = yylex();

		if (tok == T_DATUM && plisql_peek() == ';' &&
			(yylval.wdatum.datum->dtype == PLISQL_DTYPE_VAR ||
			 yylval.wdatum.datum->dtype == PLISQL_DTYPE_PACKAGE_DATUM || /* ReqID:SRS-SQL-PACKAGE */
			 yylval.wdatum.datum->dtype == PLISQL_DTYPE_ATTRIBUTE || /* ReqID:SRS-PLSQL-CURSOR */
			 yylval.wdatum.datum->dtype == PLISQL_DTYPE_PROMISE ||
			 yylval.wdatum.datum->dtype == PLISQL_DTYPE_ROW ||
			 yylval.wdatum.datum->dtype == PLISQL_DTYPE_REC))
		{
			new->retvarno = yylval.wdatum.datum->dno;
			/* eat the semicolon token that we only peeked at above */
			tok = yylex();
			Assert(tok == ';');
		}
		else
		{
			/*
			 * Not (just) a variable name, so treat as expression.
			 *
			 * Note that a well-formed expression is _required_ here;
			 * anything else is a compile-time error.
			 */
			plisql_push_back_token(tok);
			new->expr = read_sql_expression(';', ";");
		}
	}

	return (PLiSQL_stmt *) new;
}


static PLiSQL_stmt *
make_return_query_stmt(int location)
{
	PLiSQL_stmt_return_query *new;
	int			tok;

	if (!plisql_curr_compile->fn_retset)
		ereport(ERROR,
				(errcode(ERRCODE_DATATYPE_MISMATCH),
				 errmsg("cannot use RETURN QUERY in a non-SETOF function"),
				 parser_errposition(location)));

	new = palloc0(sizeof(PLiSQL_stmt_return_query));
	new->cmd_type = PLISQL_STMT_RETURN_QUERY;
	new->lineno = plisql_location_to_lineno(location);
	new->stmtid = ++plisql_curr_compile->nstatements;

	/* check for RETURN QUERY EXECUTE */
	if ((tok = yylex()) != K_EXECUTE)
	{
		/* ordinary static query */
		plisql_push_back_token(tok);
		new->query = read_sql_stmt();
	}
	else
	{
		/* dynamic SQL */
		int		term;

		new->dynquery = read_sql_expression2(';', K_USING, "; or USING",
											 &term);
		if (term == K_USING)
		{
			do
			{
				PLiSQL_expr *expr;

				expr = read_sql_expression2(',', ';', ", or ;", &term);
				new->params = lappend(new->params, expr);
			} while (term == ',');
		}
	}

	return (PLiSQL_stmt *) new;
}


/* convenience routine to fetch the name of a T_DATUM */
static char *
NameOfDatum(PLwdatum *wdatum)
{
	if (wdatum->ident)
		return wdatum->ident;
	Assert(wdatum->idents != NIL);
	return NameListToString(wdatum->idents);
}

static void
check_assignable(PLiSQL_datum *datum, int location)
{
	switch (datum->dtype)
	{
		case PLISQL_DTYPE_VAR:
		case PLISQL_DTYPE_PROMISE:
		case PLISQL_DTYPE_REC:
			if (((PLiSQL_variable *) datum)->isconst)
				ereport(ERROR,
						(errcode(ERRCODE_ERROR_IN_ASSIGNMENT),
						 errmsg("variable \"%s\" is declared CONSTANT",
								((PLiSQL_variable *) datum)->refname),
						 parser_errposition(location)));
			/* Begin - ReqID:SRS-SQL-FUNCOUT */
			if (datum->dtype == PLISQL_DTYPE_VAR)
			{
				PLiSQL_var *var = (PLiSQL_var *) datum;

				if (var->info == PROARGMODE_IN)
					ereport(ERROR,
							(errcode(ERRCODE_PLPGSQL_ERROR),
							 errmsg("expression \"%s\" cannot be used as an assignment target",
									var->refname)));
			}
			else if (datum->dtype ==  PLISQL_DTYPE_REC)
			{
				/*
				 * Target is a record variable
				 */
				PLiSQL_rec *rec = (PLiSQL_rec *) datum;

				if (rec->info == PROARGMODE_IN)
					ereport(ERROR,
							(errcode(ERRCODE_PLPGSQL_ERROR),
							 errmsg("expression \"%s\" cannot be used as an assignment target",
									rec->refname)));
			}
			/* End - ReqID:SRS-SQL-FUNCOUT */
			break;
		case PLISQL_DTYPE_ROW:
			/* always assignable; member vars were checked at compile time */
			break;
		case PLISQL_DTYPE_RECFIELD:
			/* assignable if parent record is */
			check_assignable(plisql_Datums[((PLiSQL_recfield *) datum)->recparentno],
							 location);
			break;
		/* Begin - ReqID:SRS-SQL-PACKAGE */
		case PLISQL_DTYPE_PACKAGE_DATUM:
			check_packagedatum_assignable((PLiSQL_pkg_datum *) datum, location);
			break;
		/* End - ReqID:SRS-SQL-PACKAGE */
		/* Begin - ReqID:SRS-PLSQL-RECORD */
		case PLISQL_DTYPE_ORA_RECORD:
			/* always assignable; member vars were checked at compile time */
			break;
		/* End - ReqID:SRS-PLSQL-RECORD */

		/* Begin - ReqID:SRS-PLSQL-CURSOR */
		case PLISQL_DTYPE_ATTRIBUTE:
			ereport(ERROR,
						(errcode(ERRCODE_ERROR_IN_ASSIGNMENT),
						 errmsg("variable \"%s\" is a cursor attribute",
								((PLiSQL_attribute *) datum)->fieldname),
						 parser_errposition(location)));
			break;
		/* End - ReqID:SRS-PLSQL-CURSOR */
		/* Begin - ReqID:SRS-PLSQL-Collection */
		case PLISQL_DTYPE_COLLECT_ARRAYELEM:
			/* always assignable ? */
			break;
		/* End - ReqID:SRS-PLSQL-Collection */

		default:
			elog(ERROR, "unrecognized dtype: %d", datum->dtype);
			break;
	}
}


/* Begin - ReqID:SRS-SQL-PACKAGE */
/*
 * check package datum can be assigned
 */
static void
check_packagedatum_assignable(PLiSQL_pkg_datum *pkg_datum, int location)
{
	PLiSQL_datum *datum = pkg_datum->pkgvar;
	PackageCacheItem *item = pkg_datum->item;
	PLiSQL_package *psource = (PLiSQL_package *) item->source;
	PLiSQL_function *func = (PLiSQL_function *) &psource->source;

	switch (datum->dtype)
	{
		case PLISQL_DTYPE_PACKAGE_DATUM:
			{
				PLiSQL_pkg_datum *pkg1_datum = (PLiSQL_pkg_datum *) datum;

				check_packagedatum_assignable(pkg1_datum, location);
			}
			break;
		case PLISQL_DTYPE_VAR:
		case PLISQL_DTYPE_PROMISE:
		case PLISQL_DTYPE_REC:
			if (((PLiSQL_variable *) datum)->isconst)
				ereport(ERROR,
						(errcode(ERRCODE_ERROR_IN_ASSIGNMENT),
						 errmsg("variable \"%s\" is declared CONSTANT",
								((PLiSQL_variable *) datum)->refname),
						 parser_errposition(location)));
			/* Begin - ReqID:SRS-SQL-FUNCOUT */
			if (datum->dtype == PLISQL_DTYPE_VAR)
			{
				PLiSQL_var *var = (PLiSQL_var *) datum;

				if (var->info == PROARGMODE_IN)
					ereport(ERROR,
							(errcode(ERRCODE_PLPGSQL_ERROR),
							 errmsg("expression \"%s\" cannot be used as an assignment target",
									var->refname)));
			}
			else if (datum->dtype ==  PLISQL_DTYPE_REC)
			{
				/*
				 * Target is a record variable
				 */
				PLiSQL_rec *rec = (PLiSQL_rec *) datum;

				if (rec->info == PROARGMODE_IN)
					ereport(ERROR,
							(errcode(ERRCODE_PLPGSQL_ERROR),
							 errmsg("expression \"%s\" cannot be used as an assignment target",
									rec->refname)));
			}
			/* End - ReqID:SRS-SQL-FUNCOUT */
			break;
		case PLISQL_DTYPE_ROW:
		/* Begin - ReqID:SRS-PLSQL-RECORD */
		case PLISQL_DTYPE_ORA_RECORD:
		/* End - ReqID:SRS-PLSQL-RECORD */
			/* always assignable; member vars were checked at compile time */
			break;
		case PLISQL_DTYPE_RECFIELD:
			/* assignable if parent record is */
			check_assignable(func->datums[((PLiSQL_recfield *) datum)->recparentno],
							 location);
			break;
		/* Begin - ReqID:SRS-PLSQL-CURSOR */
		case PLISQL_DTYPE_ATTRIBUTE:
			ereport(ERROR,
						(errcode(ERRCODE_ERROR_IN_ASSIGNMENT),
						 errmsg("variable \"%s\" is a cursor attribute",
								((PLiSQL_attribute *) datum)->fieldname),
						 parser_errposition(location)));
			break;
		/* End - ReqID:SRS-PLSQL-CURSOR */
		default:
			elog(ERROR, "unrecognized dtype: %d", datum->dtype);
			break;
	}
}
/* End - ReqID:SRS-SQL-PACKAGE */


/*
 * Read the argument of an INTO clause.  On entry, we have just read the
 * INTO keyword.
 */
static void
read_into_target(PLiSQL_variable **target, bool *strict)
{
	int			tok;

	/* Set default results */
	*target = NULL;
	if (strict)
		*strict = plisql_default_into_strict; /* ReqID:SRS-PLSQL-EXCEPTION */

	tok = yylex();
	if (strict && tok == K_STRICT)
	{
		*strict = true;
		tok = yylex();
	}

	/*
	 * Currently, a row or record variable can be the single INTO target,
	 * but not a member of a multi-target list.  So we throw error if there
	 * is a comma after it, because that probably means the user tried to
	 * write a multi-target list.  If this ever gets generalized, we should
	 * probably refactor read_into_scalar_list so it handles all cases.
	 */
	switch (tok)
	{
		case T_DATUM:
			if (is_row_record_datum(yylval.wdatum.datum)) /* ReqID:SRS-SQL-PACKAGE */
			{
				check_assignable(yylval.wdatum.datum, yylloc);
				*target = (PLiSQL_variable *) yylval.wdatum.datum;
				/* Begin - BUG#M0000376 */
				check_oraparam_mode(yylval.wdatum.datum, yylloc, PROARGMODE_OUT);
				/* End - BUG#M0000376 */

				if ((tok = yylex()) == ',')
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("record variable cannot be part of multiple-item INTO list"),
							 parser_errposition(yylloc)));
				plisql_push_back_token(tok);
			}
			else
			{
				*target = (PLiSQL_variable *)
					read_into_scalar_list(NameOfDatum(&(yylval.wdatum)),
										  yylval.wdatum.datum, yylloc);
			}
			break;

		default:
			/* just to give a better message than "syntax error" */
			current_token_is_not_variable(tok);
	}
}

/*
 * Given the first datum and name in the INTO list, continue to read
 * comma-separated scalar variables until we run out. Then construct
 * and return a fake "row" variable that represents the list of
 * scalars.
 */
static PLiSQL_row *
read_into_scalar_list(char *initial_name,
					  PLiSQL_datum *initial_datum,
					  int initial_location)
{
	int 			 nfields;
	char			*fieldnames[1024];
	int 			 varnos[1024];
	PLiSQL_row		*row;
	int 			 tok;
	PLiSQL_datum	*cur_datum;
	char			*fieldname;

	nfields 	  = 0;
	cur_datum	  = initial_datum;
	fieldname	  = initial_name;
	do
	{
		/* Check for array overflow */
		if (nfields >= 1024)
			ereport(ERROR,
					(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
					 errmsg("too many INTO variables specified"),
					 parser_errposition(yylloc)));
		if (is_row_record_datum(cur_datum))
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("record variable cannot be part of multiple-item INTO list"),
					 parser_errposition(yylloc)));

		check_assignable(cur_datum, yylloc);
		/* Begin - BUG#M0000376 */
		check_oraparam_mode(cur_datum, yylloc, PROARGMODE_OUT);
		/* End - BUG#M0000376 */
		if (nfields > 0 &&
			is_row_record_datum(cur_datum)) /* ReqID:SRS-SQL-PACKAGE */
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("\"%s\" is not a scalar variable",
							fieldname),
					 parser_errposition(yylloc)));
		/* Begin - ReqID:SRS-PLSQL-Collection */
		if ((cur_datum->dtype == PLISQL_DTYPE_VAR &&
			((PLiSQL_var *)cur_datum)->datatype->ttype == PLISQL_TTYPE_COLLECT_TYPE) ||
			(cur_datum->dtype == PLISQL_DTYPE_PACKAGE_DATUM &&
			((PLiSQL_var *)(((PLiSQL_pkg_datum *)cur_datum)->pkgvar))->datatype->ttype == PLISQL_TTYPE_COLLECT_TYPE))

		{
			StringInfoData	var_ds;
			StringInfoData	method_ds;
			List			*list_elems = NIL;
			bool			is_method = false;
			PLiSQL_datum	*top_datum;

			initStringInfo(&var_ds);
			initStringInfo(&method_ds);

			top_datum = cur_datum;
			list_elems = read_collect_variable(top_datum, initial_location,
											&var_ds, &method_ds, &is_method);

			if (nfields > 0 && is_method)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("\"%s\" is not a scalar variable",
								fieldname),
							parser_errposition(yylloc)));

			/* put the arrayelem into plisql_Datums array if any */
			if (list_elems != NIL)
			{
				ListCell	*lc;
				int 		parent_dno = top_datum->dno;
				PLiSQL_arrayelem *elem;
				int 		nlist = list_length(list_elems);
				int 		i;

				i = 0;
				foreach(lc, list_elems)
				{
					elem = (PLiSQL_arrayelem *) lfirst(lc);

					plisql_adddatum((PLiSQL_datum *) elem);
					elem->arrayparentno = parent_dno;
					parent_dno = elem->dno;
					i++;

					if (i == nlist)
					{
						fieldnames[nfields] = fieldname;
						varnos[nfields++]	= elem->dno;
					}
				}
			}
			else
			{
				fieldnames[nfields] = fieldname;
				varnos[nfields++]	= cur_datum->dno;
			}
			pfree(var_ds.data);
			pfree(method_ds.data);
		}
		else
		{
		/* End - ReqID:SRS-PLSQL-Collection */
			fieldnames[nfields] = fieldname;
			varnos[nfields++]	= cur_datum->dno;
		}

		tok = yylex();

		if (tok != ',')
			break;

		tok = yylex();
		if (tok != T_DATUM)
		{
			/* just to give a better message than "syntax error" */
			current_token_is_not_variable(tok);
		}
		cur_datum = yylval.wdatum.datum;
		fieldname = NameOfDatum(&yylval.wdatum);
	}while(1); /* ReqID:SRS-PLSQL-Collection */

	/*
	 * We read an extra, non-comma token from yylex(), so push it
	 * back onto the input stream
	 */
	plisql_push_back_token(tok);

	row = palloc0(sizeof(PLiSQL_row));
	row->dtype = PLISQL_DTYPE_ROW;
	row->refname = "(unnamed row)";
	row->lineno = plisql_location_to_lineno(initial_location);
	row->rowtupdesc = NULL;
	row->nfields = nfields;
	row->fieldnames = palloc(sizeof(char *) * nfields);
	row->varnos = palloc(sizeof(int) * nfields);
	while (--nfields >= 0)
	{
		row->fieldnames[nfields] = fieldnames[nfields];
		row->varnos[nfields] = varnos[nfields];
	}

	plisql_adddatum((PLiSQL_datum *)row);

	return row;
}

/*
 * Convert a single scalar into a "row" list.  This is exactly
 * like read_into_scalar_list except we never consume any input.
 *
 * Note: lineno could be computed from location, but since callers
 * have it at hand already, we may as well pass it in.
 */
static PLiSQL_row *
make_scalar_list1(char *initial_name,
				  PLiSQL_datum *initial_datum,
				  int lineno, int location)
{
	PLiSQL_row		*row;

	check_assignable(initial_datum, location);

	row = palloc0(sizeof(PLiSQL_row));
	row->dtype = PLISQL_DTYPE_ROW;
	row->refname = "(unnamed row)";
	row->lineno = lineno;
	row->rowtupdesc = NULL;
	row->nfields = 1;
	row->fieldnames = palloc(sizeof(char *));
	row->varnos = palloc(sizeof(int));
	row->fieldnames[0] = initial_name;
	row->varnos[0] = initial_datum->dno;

	plisql_adddatum((PLiSQL_datum *)row);

	return row;
}

/*
 * When the PL/iSQL parser expects to see a SQL statement, it is very
 * liberal in what it accepts; for example, we often assume an
 * unrecognized keyword is the beginning of a SQL statement. This
 * avoids the need to duplicate parts of the SQL grammar in the
 * PL/iSQL grammar, but it means we can accept wildly malformed
 * input. To try and catch some of the more obviously invalid input,
 * we run the strings we expect to be SQL statements through the main
 * SQL parser.
 *
 * We only invoke the raw parser (not the analyzer); this doesn't do
 * any database access and does not check any semantic rules, it just
 * checks for basic syntactic correctness. We do this here, rather
 * than after parsing has finished, because a malformed SQL statement
 * may cause the PL/iSQL parser to become confused about statement
 * borders. So it is best to bail out as early as we can.
 *
 * It is assumed that "stmt" represents a copy of the function source text
 * beginning at offset "location".  We use this assumption to transpose
 * any error cursor position back to the function source text.
 * If no error cursor is provided, we'll just point at "location".
 */
static void
check_sql_expr(const char *stmt, RawParseMode parseMode, int location)
{
	sql_error_callback_arg cbarg;
	ErrorContextCallback  syntax_errcontext;
	MemoryContext oldCxt;

	if (!plisql_check_syntax)
		return;

	cbarg.location = location;

	syntax_errcontext.callback = plisql_sql_error_callback;
	syntax_errcontext.arg = &cbarg;
	syntax_errcontext.previous = error_context_stack;
	error_context_stack = &syntax_errcontext;

	oldCxt = MemoryContextSwitchTo(plisql_compile_tmp_cxt);
	(void)raw_parser(stmt, parseMode);
	MemoryContextSwitchTo(oldCxt);

	/* Restore former ereport callback */
	error_context_stack = syntax_errcontext.previous;
}

static void
plisql_sql_error_callback(void *arg)
{
	sql_error_callback_arg *cbarg = (sql_error_callback_arg *) arg;
	int			errpos;

	/*
	 * First, set up internalerrposition to point to the start of the
	 * statement text within the function text.  Note this converts
	 * location (a byte offset) to a character number.
	 */
	parser_errposition(cbarg->location);

	/*
	 * If the core parser provided an error position, transpose it.
	 * Note we are dealing with 1-based character numbers at this point.
	 */
	errpos = geterrposition();
	if (errpos > 0)
	{
		int		myerrpos = getinternalerrposition();

		if (myerrpos > 0)		/* safety check */
			internalerrposition(myerrpos + errpos - 1);
	}

	/* In any case, flush errposition --- we want internalerrposition only */
	errposition(0);
}

/*
 * Parse a SQL datatype name and produce a PLiSQL_type structure.
 *
 * The heavy lifting is done elsewhere.  Here we are only concerned
 * with setting up an errcontext link that will let us give an error
 * cursor pointing into the plisql function source, if necessary.
 * This is handled the same as in check_sql_expr(), and we likewise
 * expect that the given string is a copy from the source text.
 */
static PLiSQL_type *
parse_datatype(const char *string, int location)
{
	TypeName   *typeName;
	Oid			type_id;
	int32		typmod;
	sql_error_callback_arg cbarg;
	ErrorContextCallback  syntax_errcontext;
	/* Begin - ReqID:SRS-SQL-PACKAGE */
	PLiSQL_type *result = NULL;
	/* End - ReqID:SRS-SQL-PACKAGE */

	cbarg.location = location;

	syntax_errcontext.callback = plisql_sql_error_callback;
	syntax_errcontext.arg = &cbarg;
	syntax_errcontext.previous = error_context_stack;
	error_context_stack = &syntax_errcontext;

	/* Begin - ReqID:SRS-PLSQL-CURSOR */
	/* lookup udt type */
	result = plisql_parse_udt_rowtype(string, false);
	if (result != NULL)
	{
		error_context_stack = syntax_errcontext.previous;
		return result;
	}
	/* End - ReqID:SRS-PLSQL-CURSOR */

	/* Let the main parser try to parse it under standard SQL rules */
	typeName = typeStringToTypeName(string);

	/* Begin - ReqID:SRS-SQL-PACKAGE */
	if (list_length(typeName->names) < 2)
	{
		/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
		if (!is_compile_standard_package())
		{
			/* may be a standard.type */
			size_t len = 8 + strlen(string) + 2;
			char *refname = (char *) palloc0(len);
			TypeName *newtypName;

			snprintf(refname, len, "standard.%s", string);
			newtypName = typeStringToTypeName(refname);
			pfree(refname);
			result = plisql_parse_package_type(newtypName, parse_by_pkg_type, true);
			pfree(newtypName);
		}
		/* lookup dbms_standard package */
		if (result == NULL &&
			!is_compile_dbms_standard_package())
		{
			/* may be a dbms_standard.type */
			size_t len = 13 + strlen(string) + 2;
			char *refname = (char *) palloc0(len);
			TypeName *newtypName;

			snprintf(refname, len, "dbms_standard.%s", string);
			newtypName = typeStringToTypeName(refname);
			pfree(refname);
			result = plisql_parse_package_type(newtypName, parse_by_pkg_type, true);
			pfree(newtypName);
		}
		/* End - ReqID:SRS-PLSQL-EXCEPTION */
	}
	else
		result = plisql_parse_package_type(typeName, parse_by_pkg_type, false);
	if (result != NULL)
	{
		error_context_stack = syntax_errcontext.previous;
		return result;
	}
	/* End - ReqID:SRS-SQL-PACKAGE */

	typenameTypeIdAndMod(NULL, typeName, &type_id, &typmod);

	/* Restore former ereport callback */
	error_context_stack = syntax_errcontext.previous;

	/* Okay, build a PLiSQL_type data structure for it */
	/* Begin - BUG:M0000068 */
	if ((plisql_curr_compile->fn_intisnumber ||
		(strcmp(plisql_error_funcname, "inline_code_block") == 0 &&
		 convert_plisql_anonymous_int_to_number)) &&
		(type_id == INT2OID || type_id == INT4OID))
		return plisql_build_datatype(NUMBEROID, NUMBER38TYPMOD,
									  plisql_curr_compile->fn_input_collation,
									  typeName);
	else
	/* End - BUG:M0000068 */
		return plisql_build_datatype(type_id, typmod,
									  plisql_curr_compile->fn_input_collation,
									  typeName);
}

/*
 * Check block starting and ending labels match.
 */
static void
check_labels(const char *start_label, const char *end_label, int end_location)
{
	if (end_label)
	{
		if (!start_label)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("end label \"%s\" specified for unlabeled block",
							end_label),
					 parser_errposition(end_location)));

		if (strcmp(start_label, end_label) != 0)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("end label \"%s\" differs from block's label \"%s\"",
							end_label, start_label),
					 parser_errposition(end_location)));
	}
}

/*
 * Read the arguments (if any) for a cursor, followed by the until token
 *
 * If cursor has no args, just swallow the until token and return NULL.
 * If it does have args, we expect to see "( arg [, arg ...] )" followed
 * by the until token, where arg may be a plain expression, or a named
 * parameter assignment of the form argname := expr. Consume all that and
 * return a SELECT query that evaluates the expression(s) (without the outer
 * parens).
 */
static PLiSQL_expr *
read_cursor_args(PLiSQL_var *cursor, int until, int *outrowno) /* ReqID:SRS-PLSQL-CURSOR */
{
	PLiSQL_expr *expr;
	PLiSQL_row *row;
	int			tok;
	int			argc;
	char	  **argv;
	StringInfoData ds;
	bool		any_named = false;
	/* Begin - ReqID:SRS-SQL-PACKAGE */
	PLiSQL_var	*real_cursor = NULL;
	bool		is_packagevar = false;
	/* End - ReqID:SRS-SQL-PACKAGE */
	/* Begin - ReqID:SRS-PLSQL-CURSOR */
	int nouts = 0;
	int *out_dnos = NULL;
	/* End - ReqID:SRS-PLSQL-CURSOR */

	tok = yylex();
	/* Begin - ReqID:SRS-SQL-PACKAGE */
	if (cursor->dtype == PLISQL_DTYPE_PACKAGE_DATUM)
	{
		real_cursor = (PLiSQL_var *) ((PLiSQL_pkg_datum *) cursor)->pkgvar;
		is_packagevar = true;
	}
	else
		real_cursor = cursor;
	/* End - ReqID:SRS-SQL-PACKAGE */
	/* Begin - ReqID:SRS-PLSQL-CURSOR */
	*outrowno = -1;
	/* End - ReqID:SRS-PLSQL-CURSOR */
	if (real_cursor->cursor_explicit_argrow < 0)
	{
		/* No arguments expected */
		if (tok == '(')
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("cursor \"%s\" has no arguments",
							cursor->refname),
					 parser_errposition(yylloc)));

		if (tok != until)
			yyerror("syntax error");

		return NULL;
	}

	/* Else better provide arguments */
	if (tok != '(' &&
		(!check_all_args_has_default(real_cursor->arglist))) /* ReqID:SRS-PLSQL-CURSOR */
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("cursor \"%s\" has arguments",
						cursor->refname),
				 parser_errposition(yylloc)));

	/*
	 * Read the arguments, one by one.
	 */
	/* Begin - ReqID:SRS-SQL-PACKAGE */
	if (is_packagevar)
	{
		PLiSQL_pkg_datum *pkg_datum = (PLiSQL_pkg_datum *) cursor;
		PLiSQL_package *psource = (PLiSQL_package *) pkg_datum->item->source;

		row = (PLiSQL_row *) psource->source.datums[real_cursor->cursor_explicit_argrow];
	}
	else
		row = (PLiSQL_row *) plisql_Datums[cursor->cursor_explicit_argrow];
	/* End - ReqID:SRS-SQL-PACKAGE */
	argv = (char **) palloc0(row->nfields * sizeof(char *));
	/* Begin - ReqID:SRS-PLSQL-CURSOR */
	out_dnos = (int *) palloc0(row->nfields * sizeof(int));
	/* End - ReqID:SRS-PLSQL-CURSOR */

	/* Begin - ReqID:SRS-PLSQL-CURSOR */
	if (tok != '(')
	{
		/* use default value */
		for (argc = 0; argc < row->nfields; argc++)
		{
			argv[argc] = get_args_default_query(real_cursor->arglist, argc);
			if (argv[argc] == NULL)
				ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("not enough arguments for cursor \"%s\"",
									cursor->refname),
							 parser_errposition(yylloc)));
		}
		goto finish_args_query;
	}
	else
	{
		/* maybe cursor() */
		int  tok1;

		tok1 = plisql_peek();
		if (tok1 == ')')
		{
			/* use default value */
			for (argc = 0; argc < row->nfields; argc++)
			{
				argv[argc] = get_args_default_query(real_cursor->arglist, argc);
				if (argv[argc] == NULL)
					ereport(ERROR,
								(errcode(ERRCODE_SYNTAX_ERROR),
								 errmsg("not enough arguments for cursor \"%s\"",
										cursor->refname),
								 parser_errposition(yylloc)));
			}
			tok = yylex();
			goto finish_args_query;
		}
	}
	/* End - ReqID:SRS-PLSQL-CURSOR */
	for (argc = 0; argc < row->nfields; argc++)
	{
		PLiSQL_expr *item;
		int		endtoken;
		int		argpos;
		int		tok1,
				tok2;
		int		arglocation;

		/*
		 * Check if it's a named parameter: "param := value"
		 * or "param => values" for ReqID:SRS-PLSQL-CURSOR
		 */
		plisql_peek2(&tok1, &tok2, &arglocation, NULL);
		if (tok1 == IDENT &&
			(tok2 == COLON_EQUALS || tok2 == EQUALS_GREATER)) /* ReqID:SRS-PLSQL-CURSOR */
		{
			char   *argname;
			IdentifierLookup save_IdentifierLookup;

			/* Read the argument name, ignoring any matching variable */
			save_IdentifierLookup = plisql_IdentifierLookup;
			plisql_IdentifierLookup = IDENTIFIER_LOOKUP_DECLARE;
			yylex();
			argname = yylval.str;
			plisql_IdentifierLookup = save_IdentifierLookup;

			/* Match argument name to cursor arguments */
			for (argpos = 0; argpos < row->nfields; argpos++)
			{
				if (strcmp(row->fieldnames[argpos], argname) == 0)
					break;
			}
			if (argpos == row->nfields)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("cursor \"%s\" has no argument named \"%s\"",
								cursor->refname, argname),
						 parser_errposition(yylloc)));

			/*
			 * Eat the ":=" or "=>". We already peeked, so the error should never
			 * happen.
			 */
			tok2 = yylex();
			if (tok2 != COLON_EQUALS && tok2 != EQUALS_GREATER) /* ReqID:SRS-PLSQL-CURSOR */
				yyerror("syntax error");

			any_named = true;
		}
		else
		{
			if (any_named)
				yyerror("a positional parameter association may not follow a named");
			argpos = argc;
		}
		if (argv[argpos] != NULL)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("value for parameter \"%s\" of cursor \"%s\" specified more than once",
							row->fieldnames[argpos], cursor->refname),
					 parser_errposition(arglocation)));

		/*
		 * Read the value expression. To provide the user with meaningful
		 * parse error positions, we check the syntax immediately, instead of
		 * checking the final expression that may have the arguments
		 * reordered. Trailing whitespace must not be trimmed, because
		 * otherwise input of the form (param -- comment\n, param) would be
		 * translated into a form where the second parameter is commented
		 * out.
		 */
		/* Begin - ReqID:SRS-PLSQL-CURSOR */
		if (ISORACURSOROID(real_cursor->datatype->typoid))
		{
			PLiSQL_function_argitem *argitem;

			argitem = (PLiSQL_function_argitem *) list_nth(real_cursor->arglist, argc);

			if (argitem->argmode != ARGMODE_IN)
			{
				int token;
				IdentifierLookup save_IdentifierLookup;
				PLiSQL_datum *datum;

				save_IdentifierLookup = plisql_IdentifierLookup;
				plisql_IdentifierLookup = IDENTIFIER_LOOKUP_NORMAL;
				token = yylex();
				if (token != T_DATUM)
					yyerror("expression cannot be used as an assignment target");
				datum = yylval.wdatum.datum;
				check_assignable(datum, yylloc);
				plisql_IdentifierLookup = save_IdentifierLookup;
				plisql_push_back_token(token);
				if (argitem->argmode == ARGMODE_OUT)
					out_dnos[nouts++] = datum->dno;
			}
			item = read_sql_construct(',', ')', 0, 0,
								  ",\" or \")",
								  RAW_PARSE_PLISQL_EXPR, /* BUG:M0000079 */
								  true, true,
								  false, /* do not trim */
								  NULL, &endtoken, false, NULL/* ReqID:SRS-PLSQL-Collection */);
			/* out parameter assign its null */
			if (argitem->argmode == ARGMODE_OUT)
				argv[argpos] = pstrdup("NULL");
			else
				argv[argpos] = item->query;
		}
		else
		{
			item = read_sql_construct(',', ')', 0, 0,
								  ",\" or \")",
								  RAW_PARSE_PLISQL_EXPR, /* BUG:M0000079 */
								  true, true,
								  false, /* do not trim */
								  NULL, &endtoken, false, NULL/* ReqID:SRS-PLSQL-Collection */);

			argv[argpos] = item->query;
		}
		/* End - ReqID:SRS-PLSQL-CURSOR */

		/* Begin - ReqID:SRS-PLSQL-CURSOR */
		if (endtoken == ')')
		{
			int argno;
			for (argno = 0; argno < row->nfields; argno++)
			{
				if (argv[argno] == NULL)
				{
					argv[argno] = get_args_default_query(real_cursor->arglist, argno);
					if (argv[argno] == NULL)
						ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("not enough arguments for cursor \"%s\"",
									cursor->refname),
							 parser_errposition(yylloc)));
				}
			}
			break;
		}
		/* End - ReqID:SRS-PLSQL-CURSOR */
		if (endtoken == ',' && (argc == row->nfields - 1))
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("too many arguments for cursor \"%s\"",
							cursor->refname),
					 parser_errposition(yylloc)));
	}

finish_args_query:

	/* Make positional argument list */
	initStringInfo(&ds);
	for (argc = 0; argc < row->nfields; argc++)
	{
		Assert(argv[argc] != NULL);

		/*
		 * Because named notation allows permutated argument lists, include
		 * the parameter name for meaningful runtime errors.
		 */
		appendStringInfoString(&ds, argv[argc]);
		if (any_named)
			appendStringInfo(&ds, " AS %s",
							 quote_identifier(row->fieldnames[argc]));
		if (argc < row->nfields - 1)
			appendStringInfoString(&ds, ", ");
	}

	expr = palloc0(sizeof(PLiSQL_expr));
	expr->query			= pstrdup(ds.data);
	expr->parseMode		= RAW_PARSE_PLISQL_EXPR; /* BUG:M0000079 */
	expr->plan			= NULL;
	expr->paramnos		= NULL;
	expr->target_param	= -1;
	expr->ns            = plisql_ns_top();
	expr->datum_dno		= -1;
	pfree(ds.data);

	/* Next we'd better find the until token */
	/* Begin - ReqID:SRS-PLSQL-CURSOR */
	if (tok == '(' || tok == ')')
		tok = yylex();
	/* End - ReqID:SRS-PLSQL-CURSOR */
	if (tok != until)
		yyerror("syntax error");

	/* Begin - ReqID:SRS-PLSQL-CURSOR */
	/*
	 * make out dnos
	 */
	if (nouts > 0)
		*outrowno = plisql_buld_rows_from_datums(out_dnos, nouts);
	pfree(out_dnos);
	/* End - ReqID:SRS-PLSQL-CURSOR */

	return expr;
}

/*
 * Parse RAISE ... USING options
 */
static List *
read_raise_options(void)
{
	List	   *result = NIL;

	for (;;)
	{
		PLiSQL_raise_option *opt;
		int		tok;

		if ((tok = yylex()) == 0)
			yyerror("unexpected end of function definition");

		opt = (PLiSQL_raise_option *) palloc(sizeof(PLiSQL_raise_option));

		if (tok_is_keyword(tok, &yylval,
						   K_ERRCODE, "errcode"))
			opt->opt_type = PLISQL_RAISEOPTION_ERRCODE;
		else if (tok_is_keyword(tok, &yylval,
								K_MESSAGE, "message"))
			opt->opt_type = PLISQL_RAISEOPTION_MESSAGE;
		else if (tok_is_keyword(tok, &yylval,
								K_DETAIL, "detail"))
			opt->opt_type = PLISQL_RAISEOPTION_DETAIL;
		else if (tok_is_keyword(tok, &yylval,
								K_HINT, "hint"))
			opt->opt_type = PLISQL_RAISEOPTION_HINT;
		else if (tok_is_keyword(tok, &yylval,
								K_COLUMN, "column"))
			opt->opt_type = PLISQL_RAISEOPTION_COLUMN;
		else if (tok_is_keyword(tok, &yylval,
								K_CONSTRAINT, "constraint"))
			opt->opt_type = PLISQL_RAISEOPTION_CONSTRAINT;
		else if (tok_is_keyword(tok, &yylval,
								K_DATATYPE, "datatype"))
			opt->opt_type = PLISQL_RAISEOPTION_DATATYPE;
		else if (tok_is_keyword(tok, &yylval,
								K_TABLE, "table"))
			opt->opt_type = PLISQL_RAISEOPTION_TABLE;
		else if (tok_is_keyword(tok, &yylval,
								K_SCHEMA, "schema"))
			opt->opt_type = PLISQL_RAISEOPTION_SCHEMA;
		else
			yyerror("unrecognized RAISE statement option");

		tok = yylex();
		if (tok != '=' && tok != COLON_EQUALS)
			yyerror("syntax error, expected \"=\"");

		opt->expr = read_sql_expression2(',', ';', ", or ;", &tok);

		result = lappend(result, opt);

		if (tok == ';')
			break;
	}

	return result;
}

/*
 * Check that the number of parameter placeholders in the message matches the
 * number of parameters passed to it, if a message was given.
 */
static void
check_raise_parameters(PLiSQL_stmt_raise *stmt)
{
	char	   *cp;
	int			expected_nparams = 0;

	if (stmt->message == NULL)
		return;

	for (cp = stmt->message; *cp; cp++)
	{
		if (cp[0] == '%')
		{
			/* ignore literal % characters */
			if (cp[1] == '%')
				cp++;
			else
				expected_nparams++;
		}
	}

	if (expected_nparams < list_length(stmt->params))
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				errmsg("too many parameters specified for RAISE")));
	if (expected_nparams > list_length(stmt->params))
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				errmsg("too few parameters specified for RAISE")));
}

/*
 * Fix up CASE statement
 */
static PLiSQL_stmt *
make_case(int location, PLiSQL_expr *t_expr,
		  List *case_when_list, List *else_stmts)
{
	PLiSQL_stmt_case	*new;

	new = palloc(sizeof(PLiSQL_stmt_case));
	new->cmd_type = PLISQL_STMT_CASE;
	new->lineno = plisql_location_to_lineno(location);
	new->stmtid = ++plisql_curr_compile->nstatements;
	new->t_expr = t_expr;
	new->t_varno = 0;
	new->case_when_list = case_when_list;
	new->have_else = (else_stmts != NIL);
	/* Get rid of list-with-NULL hack */
	if (list_length(else_stmts) == 1 && linitial(else_stmts) == NULL)
		new->else_stmts = NIL;
	else
		new->else_stmts = else_stmts;

	/*
	 * When test expression is present, we create a var for it and then
	 * convert all the WHEN expressions to "VAR IN (original_expression)".
	 * This is a bit klugy, but okay since we haven't yet done more than
	 * read the expressions as text.  (Note that previous parsing won't
	 * have complained if the WHEN ... THEN expression contained multiple
	 * comma-separated values.)
	 */
	if (t_expr)
	{
		char	varname[32];
		PLiSQL_var *t_var;
		ListCell *l;

		/* use a name unlikely to collide with any user names */
		snprintf(varname, sizeof(varname), "__Case__Variable_%d__",
				 plisql_nDatums);

		/*
		 * We don't yet know the result datatype of t_expr.  Build the
		 * variable as if it were INT4; we'll fix this at runtime if needed.
		 */
		t_var = (PLiSQL_var *)
			plisql_build_variable(varname, new->lineno,
								   plisql_build_datatype(INT4OID,
														  -1,
														  InvalidOid,
														  NULL),
								   true);
		new->t_varno = t_var->dno;

		foreach(l, case_when_list)
		{
			PLiSQL_case_when *cwt = (PLiSQL_case_when *) lfirst(l);
			PLiSQL_expr *expr = cwt->expr;
			StringInfoData	ds;

			/* We expect to have expressions not statements */
			Assert(expr->parseMode == RAW_PARSE_PLISQL_EXPR); /* BUG:M0000079 */

			/* Do the string hacking */
			initStringInfo(&ds);

			appendStringInfo(&ds, "\"%s\" IN (%s)",
							 varname, expr->query);

			pfree(expr->query);
			expr->query = pstrdup(ds.data);
			/* Adjust expr's namespace to include the case variable */
			expr->ns = plisql_ns_top();

			pfree(ds.data);
		}
	}

	return (PLiSQL_stmt *) new;
}

/* Begin - BUG #M0000304 */
/* this function look like make_execsql_stmt */
static PLiSQL_stmt *
build_call_expr(int firsttoken,const char *str, int location)
{
	StringInfoData		ds;
	IdentifierLookup	save_IdentifierLookup;
	PLiSQL_stmt_execsql *execsql;
	PLiSQL_expr		*expr;
	PLiSQL_variable	*target = NULL;
	int					tok;
	int					prev_tok = 0;	/* BUG#Z375 */
	bool				have_into = false;
	bool				have_strict = false;
	int					into_start_loc = -1;
	int					into_end_loc = -1;
	/* Begin - BUG#Z7305 */
	bool				have_collect = false;
	int					collect_start_loc = -1;
	int					collect_end_loc = -1;
	StringInfoData		ds_collect;
	Oid					pkgoid;
	/* End - BUG#Z7305 */
	int					parenlevel = 0;
	PLiSQL_CALL				call_process = PLISQL_IS_INVALID;
	bool is_process = false;
	PLiSQL_stmt_call *callsql = NULL;
	bool get_first_tok = true;

	initStringInfo(&ds);
	initStringInfo(&ds_collect); /* BUG#Z7305 */

	/* special lookup mode for identifiers within the SQL text */
	save_IdentifierLookup = plisql_IdentifierLookup;
	/* Begin - BUG#Z7305 */
	/*
	 * because need to parse the the collection variable,
	 * so change the plisql_IdentifierLookup from IDENTIFIER_LOOKUP_EXPR
	 * to IDENTIFIER_LOOKUP_NORMAL.
	 */
	plisql_IdentifierLookup = IDENTIFIER_LOOKUP_NORMAL;
	/* End - BUG#Z7305 */

	/*
	 * Scan to the end of the SQL command.  Identify any INTO-variables
	 * clause lurking within it, and parse that via read_into_target().
	 *
	 * Because INTO is sometimes used in the main SQL grammar, we have to be
	 * careful not to take any such usage of INTO as a PL/iSQL INTO clause.
	 * There are currently three such cases:
	 *
	 * 1. SELECT ... INTO.  We don't care, we just override that with the
	 * PL/iSQL definition.
	 *
	 * 2. INSERT INTO.  This is relatively easy to recognize since the words
	 * must appear adjacently; but we can't assume INSERT starts the command,
	 * because it can appear in CREATE RULE or WITH.  Unfortunately, INSERT is
	 * *not* fully reserved, so that means there is a chance of a false match;
	 * but it's not very likely.
	 *
	 * 3. IMPORT FOREIGN SCHEMA ... INTO.  This is not allowed in CREATE RULE
	 * or WITH, so we just check for IMPORT as the command's first token.
	 * (If IMPORT FOREIGN SCHEMA returned data someone might wish to capture
	 * with an INTO-variables clause, we'd have to work much harder here.)
	 *
	 * Fortunately, INTO is a fully reserved word in the main grammar, so
	 * at least we need not worry about it appearing as an identifier.
	 *
	 * Any future additional uses of INTO in the main grammar will doubtless
	 * break this logic again ... beware!
	 */
	tok = firsttoken;
	for (;;)
	{
		prev_tok = tok;
		tok = yylex();

		if (get_first_tok)
		{
			get_first_tok = false;
			if (tok == ';')
			{
				/* if 'str' is reserved keywords, it is not processed */
				if (str != NULL && ScanReservedKeywordLookup(str,
										scan_reserved_keywords,
										num_scan_reserved_keywords) != NULL)
					call_process = PLISQL_NOT_PROCESS;
				else
					call_process = PLISQL_IS_NO_BRACKETS_PROCESS;
				break;
			}
			else if (tok == '(')
			{
				/* if 'str' is reserved keywords, it is not processed */
				if (str != NULL && ScanReservedKeywordLookup(str,
										scan_reserved_keywords,
										num_scan_reserved_keywords) != NULL)
					call_process = PLISQL_NOT_PROCESS;
				else
					call_process = PLISQL_IS_MABY_PROCESS;
			}
			else
			{
				call_process = PLISQL_NOT_PROCESS;
			}
		}

		if (tok == '(')
		{
			parenlevel++;
		}
		else if (tok == ')')
		{
			parenlevel--;
			if (parenlevel < 0)
				yyerror("mismatched parentheses");
		}

		if (have_into && into_end_loc < 0)
			into_end_loc = yylloc;		/* token after the INTO part */
		if (tok == ';')
		{
			if (call_process == PLISQL_IS_MABY_PROCESS && parenlevel == 0 && prev_tok == ')')
			{
				call_process = PLISQL_IS_PROCESS;
			}
			else if (parenlevel != 0)
			{
				call_process = PLISQL_NOT_PROCESS;
			}

			break;
		}
		if (tok == 0)
			yyerror("unexpected end of function definition");

		if (tok == K_INTO)
		{
			if (prev_tok == K_INSERT)
				continue;		/* INSERT INTO is not an INTO-target */
			if (firsttoken == K_IMPORT)
				continue;		/* IMPORT ... INTO is not an INTO-target */
			if (have_into)
				yyerror("INTO specified more than once");
			have_into = true;
			into_start_loc = yylloc;
			read_into_target(&target, &have_strict);
		}

		/* Begin - BUG#Z7305 */
		if (tok == T_DATUM || tok == T_CWORD)
		{
			if (!have_collect && collect_end_loc == -1)
			{
				int	current_loc = yylloc;

				collect_start_loc = yylloc;
				collect_end_loc = read_collection_type_expr(tok, yylloc, &ds_collect, &pkgoid, NULL);
				if (collect_end_loc > current_loc)
					have_collect = true;
				else
				{
					collect_start_loc = -1;
					collect_end_loc = -1;
				}
			}
			else
			{
				int		current_loc = yylloc;
				int		tmp_loc = -1;
				StringInfoData		ds_tmp;

				initStringInfo(&ds_tmp);
				tmp_loc = read_collection_type_expr(tok, yylloc, &ds_tmp, &pkgoid, NULL);
				if (tmp_loc > current_loc)
				{
					appendStringInfoSpaces(&ds_collect, 1);
					plisql_append_source_text(&ds_collect, collect_end_loc, current_loc);

					appendBinaryStringInfo(&ds_collect, ds_tmp.data, ds_tmp.len);
					collect_end_loc = tmp_loc;
				}
				pfree(ds_tmp.data);
			}
		}
		/* End - BUG#Z7305 */
	}

	plisql_IdentifierLookup = save_IdentifierLookup;

	if (have_into)
	{
		/*
		 * Insert an appropriate number of spaces corresponding to the
		 * INTO text, so that locations within the redacted SQL statement
		 * still line up with those in the original source text.
		 */
		/* Begin - BUG#Z7305 */
		if (have_collect)
		{
			if (into_start_loc < collect_start_loc)
			{
				plisql_append_source_text(&ds, location, into_start_loc);
				appendStringInfoSpaces(&ds, into_end_loc - into_start_loc);

				plisql_append_source_text(&ds, into_end_loc, collect_start_loc);
				appendBinaryStringInfo(&ds, ds_collect.data, ds_collect.len);
				if (collect_end_loc != yylloc)
				{
					appendStringInfoSpaces(&ds, 1);
					plisql_append_source_text(&ds, collect_end_loc, yylloc);
				}
			}
			else
			{
				plisql_append_source_text(&ds, location, collect_start_loc);
				appendBinaryStringInfo(&ds, ds_collect.data, ds_collect.len);
				appendStringInfoSpaces(&ds, 1);
				plisql_append_source_text(&ds, collect_end_loc, into_start_loc);
				appendStringInfoSpaces(&ds, into_end_loc - into_start_loc);
			}
		}
		else
		{
		/* End - BUG#Z7305 */
			plisql_append_source_text(&ds, location, into_start_loc);
			appendStringInfoSpaces(&ds, into_end_loc - into_start_loc);
			plisql_append_source_text(&ds, into_end_loc, yylloc);
		}
	}
	else
	{
		if (call_process == PLISQL_IS_PROCESS || call_process == PLISQL_IS_MABY_PROCESS
				|| call_process == PLISQL_IS_NO_BRACKETS_PROCESS)
		{
			appendBinaryStringInfo(&ds, "CALL ", strlen("CALL "));

			/* Begin - BUG#Z7305 */
			if (have_collect)
			{
				plisql_append_source_text(&ds, location, collect_start_loc);
				appendBinaryStringInfo(&ds, ds_collect.data, ds_collect.len);
				if (collect_end_loc != yylloc)
					plisql_append_source_text(&ds, collect_end_loc, yylloc);
			}
			else
			/* End - BUG#Z7305 */
				plisql_append_source_text(&ds, location, yylloc);

			/* trim any trailing whitespace, for neatness */
			while (ds.len > 0 && scanner_isspace(ds.data[ds.len - 1]))
				ds.data[--ds.len] = '\0';

			if (call_process == PLISQL_IS_NO_BRACKETS_PROCESS)
				appendBinaryStringInfo(&ds, "()", strlen("()"));
			is_process = true;
		}
		else
		{
			/* Begin - BUG#Z7305 */
			if (have_collect)
			{
				plisql_append_source_text(&ds, location, collect_start_loc);
				appendBinaryStringInfo(&ds, ds_collect.data, ds_collect.len);
				if (collect_end_loc != yylloc)
				{
					appendStringInfoSpaces(&ds, 1);
					plisql_append_source_text(&ds, collect_end_loc, yylloc);
				}
			}
			else
			/* End - BUG#Z7305 */
				plisql_append_source_text(&ds, location, yylloc);
		}
	}

	/* trim any trailing whitespace, for neatness */
	while (ds.len > 0 && scanner_isspace(ds.data[ds.len - 1]))
		ds.data[--ds.len] = '\0';

	expr = palloc0(sizeof(PLiSQL_expr));
	expr->query			= pstrdup(ds.data);
	expr->parseMode		= RAW_PARSE_DEFAULT;
	expr->plan			= NULL;
	expr->paramnos		= NULL;
	expr->target_param	= -1;
	expr->ns			= plisql_ns_top();
	expr->datum_dno		= -1;
	pfree(ds.data);

	check_sql_expr(expr->query, expr->parseMode, location);

	/* Begin - BUG#M0000376 */
	/*
	 * if enable_get_oraparam_mode is true, and plisql_curr_compile is an anonymous block,
	 * do transform the expr to get the placeholder mode.
	 * e.g., proc(:x, :y); the :x and :y are placeholder,
	 * to get the mode, must transform the procedure proc.
	 */
	if (enable_get_oraparam_mode &&
		plisql_curr_compile->fn_prokind == PROKIND_ANONYMOUS_BLOCK)
	{
		do_transform_for_anonyblock_expr(expr);
	}
	/* End - BUG#M0000376 */

	if (is_process)
	{
		callsql = palloc0(sizeof(PLiSQL_stmt_call));
		callsql->cmd_type = PLISQL_STMT_CALL;
		callsql->lineno = plisql_location_to_lineno(location);
		callsql->stmtid = ++plisql_curr_compile->nstatements;
		callsql->expr = expr;
		callsql->is_call = true;

		/* Remember we may need a procedure resource owner */
		plisql_curr_compile->requires_procedure_resowner = true;

		return (PLiSQL_stmt *) callsql;
	}
	else
	{
		execsql = palloc0(sizeof(PLiSQL_stmt_execsql));
		execsql->cmd_type = PLISQL_STMT_EXECSQL;
		execsql->lineno  = plisql_location_to_lineno(location);
		execsql->stmtid  = ++plisql_curr_compile->nstatements;
		execsql->sqlstmt = expr;
		execsql->into	 = have_into;
		execsql->strict	 = have_strict;
		execsql->target	 = target;

		return (PLiSQL_stmt *) execsql;
	}

}

/* scan gram.y 's reserved key word */
const ScanKeyword *
ScanReservedKeywordLookup(const char *text,
				  const ScanKeyword *keywords,
				  int num_keywords)
{
	int			len,
				i;
	char		word[NAMEDATALEN];
	const ScanKeyword *low;
	const ScanKeyword *high;

	len = strlen(text);
	/* We assume all keywords are shorter than NAMEDATALEN. */
	if (len >= NAMEDATALEN)
		return NULL;

	/*
	 * Apply an ASCII-only downcasing.  We must not use tolower() since it may
	 * produce the wrong translation in some locales (eg, Turkish).
	 */
	for (i = 0; i < len; i++)
	{
		char		ch = text[i];

		if (ch >= 'A' && ch <= 'Z')
			ch += 'a' - 'A';
		word[i] = ch;
	}
	word[len] = '\0';

	/*
	 * Now do a binary search using plain strcmp() comparison.
	 */
	low = keywords;
	high = keywords + (num_keywords - 1);
	while (low <= high)
	{
		const ScanKeyword *middle;
		int			difference;

		middle = low + (high - low) / 2;
		difference = strcmp(middle->name, word);
		if (difference == 0)
			return middle;
		else if (difference < 0)
			low = middle + 1;
		else
			high = middle - 1;
	}

	return NULL;
}
/* End - BUG #M0000304 */


/* Begin - BUG#M0000376 */
/*
 * check_oraparam_mode
 * if the current compiled function is an anonymous block,
 * the variable is a placeholder, such as :id,
 * set the mode.
 */
static void
check_oraparam_mode(PLiSQL_datum *datum, int location, char mode)
{
	if (!enable_get_oraparam_mode &&
		plisql_curr_compile->fn_prokind != PROKIND_ANONYMOUS_BLOCK)
		return;

	switch (datum->dtype)
	{
		case PLISQL_DTYPE_VAR:
		case PLISQL_DTYPE_PROMISE:
		case PLISQL_DTYPE_REC:
			if (((PLiSQL_variable *) datum)->isconst)
				ereport(ERROR,
						(errcode(ERRCODE_ERROR_IN_ASSIGNMENT),
						 errmsg("variable \"%s\" is declared CONSTANT",
								((PLiSQL_variable *) datum)->refname),
						 parser_errposition(location)));
			/* Begin - ReqID:SRS-SQL-FUNCOUT */
			if (datum->dtype == PLISQL_DTYPE_VAR)
			{
				PLiSQL_var *var = (PLiSQL_var *) datum;

				if (var->info == PROARGMODE_IN)
					ereport(ERROR,
							(errcode(ERRCODE_PLPGSQL_ERROR),
							 errmsg("expression \"%s\" cannot be used as an assignment target",
									var->refname)));

				/*
				 * if the current compiled function is an anonymous block,
				 * the variable is a placeholder, such as :id,
				 * set the mode.
				 */
				set_oraparam_mode(var->refname, mode);
			}
			else if (datum->dtype ==  PLISQL_DTYPE_REC)
			{
				/*
				 * Target is a record variable
				 */
				PLiSQL_rec *rec = (PLiSQL_rec *) datum;

				if (rec->info == PROARGMODE_IN)
					ereport(ERROR,
							(errcode(ERRCODE_PLPGSQL_ERROR),
							 errmsg("expression \"%s\" cannot be used as an assignment target",
									rec->refname)));

				/*
				 * if the current compiled function is an anonymous block,
				 * the variable is a placeholder, such as :id,
				 * set the mode.
				 */
				set_oraparam_mode(rec->refname, mode);
			}
			/* End - ReqID:SRS-SQL-FUNCOUT */
			break;
		case PLISQL_DTYPE_ROW:
			/* always assignable; member vars were checked at compile time */
			break;
		case PLISQL_DTYPE_RECFIELD:
			/* assignable if parent record is */
			check_assignable(plisql_Datums[((PLiSQL_recfield *) datum)->recparentno],
							 location);
			check_oraparam_mode(plisql_Datums[((PLiSQL_recfield *) datum)->recparentno],
							 location, mode);
			break;
		/* Begin - ReqID:SRS-SQL-PACKAGE */
		case PLISQL_DTYPE_PACKAGE_DATUM:
			check_packagedatum_assignable((PLiSQL_pkg_datum *) datum, location);
			check_package_oraparam_mode((PLiSQL_pkg_datum *) datum, location, mode);
			break;
		/* End - ReqID:SRS-SQL-PACKAGE */

		/* Begin - ReqID:SRS-PLSQL-CURSOR */
		case PLISQL_DTYPE_ATTRIBUTE:
			ereport(ERROR,
						(errcode(ERRCODE_ERROR_IN_ASSIGNMENT),
						 errmsg("variable \"%s\" is a cursor attribute",
								((PLiSQL_attribute *) datum)->fieldname),
						 parser_errposition(location)));
			break;
		/* End - ReqID:SRS-PLSQL-CURSOR */

		default:
			elog(ERROR, "unrecognized dtype: %d", datum->dtype);
			break;
	}
}

static void
check_package_oraparam_mode(PLiSQL_pkg_datum *pkg_datum, int location, char mode)
{
	PLiSQL_datum *datum = pkg_datum->pkgvar;
	PackageCacheItem *item = pkg_datum->item;
	PLiSQL_package *psource = (PLiSQL_package *) item->source;
	PLiSQL_function *func = (PLiSQL_function *) &psource->source;

	switch (datum->dtype)
	{
		case PLISQL_DTYPE_PACKAGE_DATUM:
			{
				PLiSQL_pkg_datum *pkg1_datum = (PLiSQL_pkg_datum *) datum;

				check_package_oraparam_mode(pkg1_datum, location, mode);
			}
			break;
		case PLISQL_DTYPE_VAR:
		case PLISQL_DTYPE_PROMISE:
		case PLISQL_DTYPE_REC:
			if (((PLiSQL_variable *) datum)->isconst)
				ereport(ERROR,
						(errcode(ERRCODE_ERROR_IN_ASSIGNMENT),
						 errmsg("variable \"%s\" is declared CONSTANT",
								((PLiSQL_variable *) datum)->refname),
						 parser_errposition(location)));
			/* Begin - ReqID:SRS-SQL-FUNCOUT */
			if (datum->dtype == PLISQL_DTYPE_VAR)
			{
				PLiSQL_var *var = (PLiSQL_var *) datum;

				if (var->info == PROARGMODE_IN)
					ereport(ERROR,
							(errcode(ERRCODE_PLPGSQL_ERROR),
							 errmsg("expression \"%s\" cannot be used as an assignment target",
									var->refname)));
				/*
				 * if the current compiled function is an anonymous block,
				 * the variable is a placeholder, such as :id,
				 * set the mode.
				 */
				set_oraparam_mode(var->refname, mode);
			}
			else if (datum->dtype == PLISQL_DTYPE_REC)
			{
				/*
				 * Target is a record variable
				 */
				PLiSQL_rec *rec = (PLiSQL_rec *) datum;

				if (rec->info == PROARGMODE_IN)
					ereport(ERROR,
							(errcode(ERRCODE_PLPGSQL_ERROR),
							 errmsg("expression \"%s\" cannot be used as an assignment target",
									rec->refname)));

				/*
				 * if the current compiled function is an anonymous block,
				 * the variable is a placeholder, such as :id,
				 * set the mode.
				 */
				set_oraparam_mode(rec->refname, mode);
			}
			/* End - ReqID:SRS-SQL-FUNCOUT */
			break;
		case PLISQL_DTYPE_ROW:
			/* always assignable; member vars were checked at compile time */
			break;
		case PLISQL_DTYPE_RECFIELD:
			/* assignable if parent record is */
			check_assignable(func->datums[((PLiSQL_recfield *) datum)->recparentno],
							 location);
			check_oraparam_mode(func->datums[((PLiSQL_recfield *) datum)->recparentno],
							 location,
							 mode);

			break;
		/* Begin - ReqID:SRS-PLSQL-CURSOR */
		case PLISQL_DTYPE_ATTRIBUTE:
			ereport(ERROR,
						(errcode(ERRCODE_ERROR_IN_ASSIGNMENT),
						 errmsg("variable \"%s\" is a cursor attribute",
								((PLiSQL_attribute *) datum)->fieldname),
						 parser_errposition(location)));
			break;
		/* End - ReqID:SRS-PLSQL-CURSOR */
		default:
			elog(ERROR, "unrecognized dtype: %d", datum->dtype);
			break;
	}
}
/* End - BUG#M0000376 */


/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
/*
 * like plisql_ns_lookup,
 * but we only find exception and lookup all
 * block level
 */
static PLiSQL_nsitem*
plisql_ns_lookup_exception(PLiSQL_nsitem *ns_cur,
									const char *except_name)
{
	/* Outer loop iterates once per block level in the namespace chain */
	while (ns_cur != NULL)
	{
		PLiSQL_nsitem *nsitem;

		/* Check this level for unqualified match to variable name */
		for (nsitem = ns_cur;
			 nsitem->itemtype != PLISQL_NSTYPE_LABEL;
			 nsitem = nsitem->prev)
		{
			if (strcmp(nsitem->name, except_name) == 0 &&
				nsitem->itemtype == PLISQL_NSTYPE_VAR)
			{
				PLiSQL_var *var = (PLiSQL_var *) plisql_Datums[nsitem->itemno];

				if (ISEXCEPTIONOID(var->datatype->typoid))
					return nsitem;
			}
		}
		ns_cur = nsitem->prev;
	}
	return NULL;
}

/* End - ReqID:SRS-PLSQL-EXCEPTION */

/* Begin - ReqID:SRS-PLSQL-Collection */
/*
 * If the current tok is a collection variable and the next token is '(',
 * replace the '(' to '[', and then execute the above processing in a loop.
 * If the current tok is a collection type, determine whether it is a
 * collection constructor and return the constructor call statement.
 */
static int
read_collection_type_expr(int start_token, int startlocation,
						StringInfo expr_ds, Oid *pkgoid, int *datum_dno)
{
	int tok = start_token;

	if (tok == T_DATUM)
	{
		/* may be a collection variable */
		if (yylval.wdatum.datum != NULL)
		{
			PLiSQL_datum	*yyl_datum = yylval.wdatum.datum;
			PLiSQL_datum	*datum = yyl_datum;

			if (yyl_datum->dtype == PLISQL_DTYPE_VAR ||
				yyl_datum->dtype == PLISQL_DTYPE_PACKAGE_DATUM)
			{
				PLiSQL_type *data_type = NULL;
				int 		parent_dno = -1;

				if (yyl_datum->dtype == PLISQL_DTYPE_VAR)
				{
					data_type = ((PLiSQL_var *)yyl_datum)->datatype;
					parent_dno = yyl_datum->dno;
				}
				else if (yyl_datum->dtype == PLISQL_DTYPE_PACKAGE_DATUM)
				{
					/* reference a package variable */
					PLiSQL_pkg_datum *pkg_datum = (PLiSQL_pkg_datum *)yyl_datum;

					if (pkg_datum->pkgvar->dtype == PLISQL_DTYPE_VAR)
					{
						data_type = ((PLiSQL_var *)pkg_datum->pkgvar)->datatype;
						parent_dno = ((PLiSQL_var *)pkg_datum->pkgvar)->dno;
					}
				}

				if (data_type && data_type->ttype == PLISQL_TTYPE_COLLECT_TYPE)
				{
					StringInfoData	   var_ds;

					initStringInfo(&var_ds);

					plisql_append_source_text(expr_ds, startlocation, yylloc);
					startlocation = yylloc;

					tok = yylex();
					plisql_append_source_text(&var_ds, startlocation, yylloc);
					startlocation = yylloc;

				collect_loop:

					while(tok == '(')
					{
						PLiSQL_arrayelem	   *new = NULL;
						/* get current dimensional value type */
						if (!IS_PLSQL_COLLECTYPE(data_type->typtype))
							ereport(ERROR,
								(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
								errmsg("number of subscripts exceeds the maximum collection type dimensions.")));
						/* convert subscript '(' to ']' */
						appendStringInfoChar(&var_ds, '[');
						startlocation = yylloc + 1;

						new = palloc0(sizeof(PLiSQL_arrayelem));
						new->dtype	   = PLISQL_DTYPE_COLLECT_ARRAYELEM;
						new->subscript  = read_sql_expression(')', ")");
						new->arrayparentno = parent_dno;
						new->parenttypoid = InvalidOid;
						plisql_adddatum((PLiSQL_datum *)new);
						parent_dno = new->dno;

						datum = (PLiSQL_datum *) new;

						appendStringInfoString(&var_ds, new->subscript->query);
						appendStringInfoChar(&var_ds, ']');

						tok = yylex();
						startlocation = yylloc;

						data_type = ((PLiSQL_ora_collection_type*)data_type)->typdatatype;
						if (datum_dno != NULL)
							*datum_dno = new->dno;
					}

					if (tok == '.')
					{
						tok = yylex();

						/* maybe collection methods */
						if (is_collect_method(tok))
						{
							read_collect_method(tok, var_ds.data, data_type, expr_ds, &startlocation);
						}
						else if (tok == T_WORD || tok == T_DATUM)
						{
							char   *name;
							if (tok == T_WORD)
								name = yylval.word.ident;
							else
								name = yylval.wdatum.ident;

							appendStringInfoChar(&var_ds, '.');
							appendStringInfoString(&var_ds, name);

							if (data_type->ttype == PLISQL_TTYPE_ORARECORD_TYPE)
								data_type = get_record_field_type(data_type, name);
							else if (data_type->ttype != PLISQL_TTYPE_REC &&
								data_type->ttype != PLISQL_TTYPE_EXPLICIT_CURSOR)
								ereport(ERROR,
										(errcode(ERRCODE_SYNTAX_ERROR),
										errmsg("Invalid record variable.")));

							if (datum->dtype == PLISQL_DTYPE_COLLECT_ARRAYELEM)
							{
								if (((PLiSQL_arrayelem *) datum)->indirection == NIL)
									((PLiSQL_arrayelem *) datum)->indirection = list_make1(pstrdup(name));
								else
									((PLiSQL_arrayelem *) datum)->indirection = lappend(((PLiSQL_arrayelem *) datum)->indirection,
																	pstrdup(name));
							}
							tok = yylex();
							goto collect_loop;
						}
						else if (tok == T_CWORD)
						{
							List		*wdnames = yylval.cword.idents;
							ListCell	*lc;
							void		*list_last = llast(wdnames);
							int 		method_token = -1;
							int			i;
							int			nlen;

							nlen = list_length(wdnames);
							i = 0;
							foreach(lc, wdnames)
							{
								void *cur = lfirst(lc);
								char *name = strVal(cur);

								i++;
								if (datum->dtype == PLISQL_DTYPE_COLLECT_ARRAYELEM)
								{
									if (((PLiSQL_arrayelem *) datum)->indirection == NIL)
										((PLiSQL_arrayelem *) datum)->indirection = list_make1(pstrdup(name));
									else
										((PLiSQL_arrayelem *) datum)->indirection = lappend(((PLiSQL_arrayelem *) datum)->indirection,
																		pstrdup(name));
								}

								if (data_type->ttype == PLISQL_TTYPE_ORARECORD_TYPE)
								{
									appendStringInfoChar(&var_ds, '.');
									appendStringInfoString(&var_ds, name);
									data_type = get_record_field_type(data_type, name);
								}
								else if (data_type->ttype == PLISQL_TTYPE_REC ||
									data_type->ttype == PLISQL_TTYPE_EXPLICIT_CURSOR)
								{
									appendStringInfoChar(&var_ds, '.');
									appendStringInfoString(&var_ds, name);

									if (i < nlen)
										ereport(ERROR,
												(errcode(ERRCODE_SYNTAX_ERROR),
												 errmsg("the record field is wrong"),
												 parser_errposition(startlocation)));

									break;
								}
								else if (data_type->ttype == PLISQL_TTYPE_COLLECT_TYPE &&
										cur == list_last)
								{
									method_token = get_collect_method_tok(name);
									if (method_token != -1)
										read_collect_method(method_token, var_ds.data,
													   data_type, expr_ds, &startlocation);
								}
							}
							if (method_token == -1)
							{
								tok = yylex();
								goto collect_loop;
							}
						}
					}
					else
					{
						appendBinaryStringInfo(expr_ds, var_ds.data, var_ds.len);
						plisql_push_back_token(tok);
						startlocation = yylloc;
					}
				}
			}
			else if (yyl_datum->dtype == PLISQL_DTYPE_TYPE)
			{
				/* It might be a collection constructor */
				PLiSQL_datum_udt_type *type = (PLiSQL_datum_udt_type *)yyl_datum;

				if (type->udt_type != NULL && type->udt_type->ttype == PLISQL_TTYPE_COLLECT_TYPE)
				{
					plisql_append_source_text(expr_ds, startlocation, yylloc);
					read_collection_constructor((PLiSQL_ora_collection_type *)(type->udt_type),
											expr_ds,
											&startlocation);
				}
			}
		}
		else if (yylval.wdatum.collectype != NULL)
		{
			/* It might be a collection constructor */
			PLiSQL_type *type = (PLiSQL_type *)yylval.wdatum.collectype;

			if (type != NULL && type->ttype == PLISQL_TTYPE_COLLECT_TYPE)
			{
				plisql_append_source_text(expr_ds, startlocation, yylloc);
				read_collection_constructor((PLiSQL_ora_collection_type *)type,
										expr_ds,
										&startlocation);
			}
		}
	}
	else if (tok == T_CWORD)
	{
		List		*wdnames = yylval.cword.idents;
		PLiSQL_datum	*datum;
		int			method_token = get_collect_method_tok(strVal(llast(wdnames)));
		if (method_token != -1)
		{
			/*
			 * collect.method
			 */
			datum = reparse_cword(wdnames);

			if (datum != NULL)
			{
				PLiSQL_type 	*datatype = NULL;

				if (datum->dtype == PLISQL_DTYPE_TYPE)
					ereport(ERROR,
								(errcode(ERRCODE_SYNTAX_ERROR),
								 errmsg("Invalid variable.")));

				if (datum->dtype == PLISQL_DTYPE_VAR)
					datatype = (PLiSQL_type *)((PLiSQL_var *)datum)->datatype;
				else if (datum->dtype == PLISQL_DTYPE_PACKAGE_DATUM)
				{
					PLiSQL_datum *pkgvar = ((PLiSQL_pkg_datum *)datum)->pkgvar;

					if (pkgvar->dtype == PLISQL_DTYPE_VAR)
						datatype = (PLiSQL_type *)((PLiSQL_var *)pkgvar)->datatype;
				}

				if (datatype && datatype->ttype == PLISQL_TTYPE_COLLECT_TYPE)
				{
					StringInfoData 	nameinfo;
					ListCell 	*lc;
					int			n = list_length(wdnames);
					int			i;

					initStringInfo(&nameinfo);

					i = 1;
					foreach(lc, wdnames)
					{
						if (i == n)
							break;
						else
						{
							char *name = strVal(lfirst(lc));

							appendStringInfoString(&nameinfo, name);
							if (i < n - 1)
								appendStringInfoChar(&nameinfo, '.');
							i++;
						}
					}

					plisql_append_source_text(expr_ds, startlocation, yylloc);
					read_collect_method(method_token, nameinfo.data,
									   datatype, expr_ds, &startlocation);
					pfree(nameinfo.data);
				}
			}
		}
	}

	return startlocation;
}

/*
 * check whether the token is a collection method token
 */
static bool
is_collect_method(int token)
{
	bool res = false;

	switch (token)
	{
		case K_FIRST:
		case K_LAST:
		case K_COUNT:
		case K_LIMIT:
		case K_PRIOR:
		case K_NEXT:
		case K_EXISTS:
		case K_DELETE:
		case K_TRIM:
		case K_EXTEND:
			res = true;
			break;
		default:
			break;
	}

	return res;
}

/*
 * check whether the token is a collection method token
 */
static char *
get_collect_method_name(int token)
{
	char *method_name;

	switch(token)
	{
		case K_FIRST:
			method_name = pstrdup("FIRST");
			break;
		case K_LAST:
			method_name = pstrdup("LAST");
			break;
		case K_COUNT:
			method_name = pstrdup("COUNT");
			break;
		case K_LIMIT:
			method_name = pstrdup("LIMIT");
			break;
		case K_PRIOR:
			method_name = pstrdup("PRIOR");
			break;
		case K_NEXT:
			method_name = pstrdup("NEXT");
			break;
		case K_EXISTS:
			method_name = pstrdup("EXISTS");
			break;
		case K_DELETE:
			method_name = pstrdup("DELETE");
			break;
		case K_TRIM:
			method_name = pstrdup("TRIM");
			break;
		case K_EXTEND:
			method_name = pstrdup("EXTEND");
			break;
		default:
			method_name = NULL;
			break;
	}

	if (method_name == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_FUNCTION),
				 errmsg("collection method %s does not supported",
				  method_name)));

	return method_name;
}

/*
 * get the token according a collection method name
 */
static int
get_collect_method_tok(char *method_name)
{
	int token = -1;

	if (pg_strcasecmp(method_name, "FIRST") == 0)
		token = K_FIRST;
	else if (pg_strcasecmp(method_name, "LAST") == 0)
		token = K_LAST;
	else if (pg_strcasecmp(method_name, "COUNT") == 0)
		token = K_COUNT;
	else if (pg_strcasecmp(method_name, "LIMIT") == 0)
		token = K_LIMIT;
	else if (pg_strcasecmp(method_name, "PRIOR") == 0)
		token = K_PRIOR;
	else if (pg_strcasecmp(method_name, "NEXT") == 0)
		token = K_NEXT;
	else if (pg_strcasecmp(method_name, "EXISTS") == 0)
		token = K_EXISTS;
	else if (pg_strcasecmp(method_name, "DELETE") == 0)
		token = K_DELETE;
	else if (pg_strcasecmp(method_name, "TRIM") == 0)
		token = K_TRIM;
	else if (pg_strcasecmp(method_name, "EXTEND") == 0)
		token = K_EXTEND;

	return token;
}

/*
 *  read_collect_method - read a collection method
 */
static void
read_collect_method(int start_token, const char *varname,
					PLiSQL_type *datatype, StringInfo res,
					int *startlocation)
{
	int			tok;
	const char *methodname;
	Oid			keytypoid = InvalidOid;
	char		typtype = datatype->typtype;

	methodname = get_collect_method_name(start_token);
	if (!IS_PLSQL_COLLECTYPE(typtype))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_FUNCTION),
				errmsg("collect variable %s not support method %s",
						varname, methodname)));

	keytypoid = ((PLiSQL_ora_collection_type *)datatype)->typkey->typoid;

	tok = yylex();
	if (tok != '(')
	{
		plisql_push_back_token(tok);
		*startlocation = yylloc;
	}

	appendStringInfo(res, "SYS.COLLECTION_%s(%s", methodname, varname);

	switch(start_token)
	{
		case K_FIRST:
		case K_LAST:
		case K_COUNT:
		case K_LIMIT:
			if (tok == '(')
			{
				tok = yylex();
				if (tok != ')')
					ereport(ERROR,
							(errcode(ERRCODE_UNDEFINED_FUNCTION),
							 errmsg("collection method %s has no parameter",
							  methodname)));
				*startlocation = yylloc + 1;
			}

			if (start_token == K_FIRST || start_token == K_LAST)
			{
				if (keytypoid == INT4OID)
					appendStringInfo(res, ", null::int4");
				else if (keytypoid == INT8OID)
					appendStringInfo(res, ", null::int8");
				else if (keytypoid == INT2OID)
					appendStringInfo(res, ", null::int2");
				else
					appendStringInfo(res, ", null::varchar");
			}
			break;
		case K_PRIOR:
		case K_NEXT:
		case K_EXISTS:
			{
				PLiSQL_expr	*index_expr = NULL;
				if (tok != '(')
					ereport(ERROR,
							(errcode(ERRCODE_UNDEFINED_FUNCTION),
							 errmsg("collection method %s has no parameter",
							  methodname)));

				index_expr = read_sql_construct(')', 0, 0, 0,
							 ")",
							 RAW_PARSE_PLISQL_EXPR,
							 true, false, false, NULL, &tok, false, NULL);
				if (keytypoid == INT4OID)
					appendStringInfo(res, ", (%s)::int4", index_expr->query);
				else if (keytypoid == INT8OID)
					appendStringInfo(res, ", (%s)::int8", index_expr->query);
				else if (keytypoid == INT2OID)
					appendStringInfo(res, ", (%s)::int2", index_expr->query);
				else
					appendStringInfo(res,", (%s)::varchar", index_expr->query);

				*startlocation = yylloc + 1;
			}
			break;
		case K_DELETE:
		case K_TRIM:
		case K_EXTEND:
			{
				if (tok == '(')
				{
					tok = yylex();
					while (tok != ')' && tok != ';' && tok != 0)
					{
						PLiSQL_expr	*new = NULL;

						if (tok != ',' )
							plisql_push_back_token(tok);

						new = read_sql_construct(',', ')', 0, 0,
											")",
											RAW_PARSE_PLISQL_EXPR,
											true, false, false, NULL, &tok, false, NULL);

						if (keytypoid == INT4OID)
							appendStringInfo(res, ", (%s)::int4", new->query);
						else if (keytypoid == INT8OID)
							appendStringInfo(res, ", (%s)::int8", new->query);
						else if (keytypoid == INT2OID)
							appendStringInfo(res, ", (%s)::int2", new->query);
						else
							appendStringInfo(res,", (%s)::varchar", new->query);
					}

					*startlocation = yylloc + 1;
				}
			}
			break;
		default:
			break;
	}

	appendStringInfoString(res, ")");
}

static PLiSQL_datum *
reparse_cword(List *wdnames)
{
	char			   *first_name;
	PLwdatum			wdatum;
	PLword				plword;
	PLcword				plcword;
	bool				is_datum = false;

	if (list_length(wdnames) < 2)
		return NULL;

	first_name = strVal(linitial(wdnames));

	switch(list_length(wdnames))
	{
		case 2:
			is_datum = plisql_parse_word(first_name,first_name, first_name, true,
			   &wdatum, &plword);
			if (is_datum)
					return wdatum.datum;
			break;
		case 3:
			is_datum = plisql_parse_dblword(first_name, first_name, strVal(lsecond(wdnames)),
				&wdatum, &plcword);
			if (is_datum)
					return wdatum.datum;
			break;
		case 4:
			is_datum = plisql_parse_tripword(first_name, first_name, strVal(lsecond(wdnames)),
				strVal(lthird(wdnames)), &wdatum, &plcword);
			if (is_datum)
					return wdatum.datum;
			break;
		default:
			{
				int nnames;
				List *newlist;

				newlist = list_delete_last(list_copy(wdnames));
				is_datum = plisql_parse_moreword(newlist, &wdatum, &plcword, '.', yylloc, &nnames);
				if (is_datum)
				{
					if (wdatum.datum != NULL)
					{
						if (wdatum.datum->dtype == PLISQL_DTYPE_ORA_RECORD)
						{
							PLwdatum *pl_wdatum;
							int nlen = list_length(newlist);

							/*
							 * check if the maximum number of layers that record nest
							 * is greater than PLISQL_MAXNUM_NESTED_RECORD
							 */
							plisql_check_maxnum_record_nest(nlen, nnames);
							pl_wdatum = plisql_get_ora_record_field(wdatum.datum,
														newlist,
														nlen,
														nnames,
														yylloc);
							list_free(newlist);
							if (pl_wdatum != NULL)
								return pl_wdatum->datum;
						}
						else if (wdatum.datum->dtype == PLISQL_DTYPE_PACKAGE_DATUM)
						{
							list_free(newlist);
							return ((PLiSQL_pkg_datum *)(wdatum.datum))->pkgvar;
						}
					}
				}
				list_free(newlist);
			}
			break;
	}

	return NULL;
}

/*
 *  check whether the datum is a collection
 */
bool
is_collection_variable(PLiSQL_datum *datum)
{
	if (datum && datum->dtype == PLISQL_DTYPE_VAR &&
		IS_PLSQL_COLLECTYPE(((PLiSQL_var *) datum)->datatype->typtype))
		return true;

	return false;
}

/*
 * build the constructor call statement.
 * for a nested table constructor that has empty element, the statement is roughly like this
 * "SYS.NESTED_TABLE_CONSTRUCTOR(type_address, ARRAY[]::int[])",
 * for a nested table constructor that has initial elements,  the statement is roughly like this
 * "SYS.NESTED_TABLE_CONSTRUCTOR(type_address, ARRAY[element1,...,elementn]::element_typname[])",
 * for the varray constructor, the statement is roughly like this
 * for a varray constructor that has empty element, the statement is roughly like this
 * "SYS.VARRAY_CONSTRUCTOR(type_address, ARRAY[]::int[])",
 * for a varray constructor that has initial elements,  the statement is roughly like this
 * "SYS.VARRAY_CONSTRUCTOR(type_address, ARRAY[element1,...,elementn]::element_typname[])",
 */
static void
read_collection_constructor(PLiSQL_ora_collection_type *collect_type,
							StringInfo ds,
							int *startlocation)
{
	int		tok;

	tok = yylex();
	if (tok != '(')
	{
		plisql_push_back_token(tok);
	}
	else if (IS_PLSQL_NESTED_TABLE(collect_type->type.typtype) ||
		IS_PLSQL_VARRAY(collect_type->type.typtype))
	{
		if (IS_PLSQL_NESTED_TABLE(collect_type->type.typtype))
			appendStringInfoString(ds, "SYS.NESTED_TABLE_CONSTRUCTOR(");
		else
			appendStringInfoString(ds, "SYS.VARRAY_CONSTRUCTOR(");

		appendStringInfo(ds, UINT64_FORMAT,
						(uint64)PointerGetDatum(plisql_type_to_collectinfo((PLiSQL_type *)collect_type)));
		appendStringInfoString(ds, ", ");

		tok = yylex();

		if (tok == ')')
			appendStringInfoString(ds, "ARRAY[]::int[]");
		else
		{
			PLiSQL_type *datatype = collect_type->typdatatype;

			appendStringInfoString(ds, "ARRAY[");

			plisql_push_back_token(tok);

			while(true)
			{
				PLiSQL_expr		*new = NULL;

				new = read_sql_construct(')', ',', 0, 0,
							 ")",
							 RAW_PARSE_PLISQL_EXPR,
							 true, false, false, NULL, &tok, false, NULL);
				appendStringInfoString(ds, new->query);
				if (tok == ',')
					appendStringInfoString(ds, ", ");
				else
					break;
			}
			appendStringInfoString(ds, "]");

			if (!IS_PLSQL_COLLECTYPE(datatype->typtype) &&
				datatype->ttype != PLISQL_TTYPE_ORARECORD_TYPE &&
				OidIsValid(get_array_type(datatype->typoid)))
			{
				appendStringInfo(ds, "::%s", datatype->typname);

				if (datatype->typoid == NUMERICOID)
				{
					char	buf[64];
					int 	mod1,
							mod2;
					int 	typmod = datatype->atttypmod;

					if (typmod > 0)
					{
						mod1 = ((typmod - VARHDRSZ) >> 16) & 0xffff;
						mod2 = ((typmod - VARHDRSZ) & 0xffff);

						if (mod2 > 0)
							snprintf(buf, sizeof(buf), "(%d, %d)", mod1, mod2);
						else
							snprintf(buf, sizeof(buf), "(%d)", mod1);

						appendStringInfoString(ds, buf);
					}
				}

				appendStringInfoString(ds, "[]");
			}
		}
	}
	else
		ereport(ERROR,
			(errcode(ERRCODE_SYNTAX_ERROR),
				errmsg("syntax error.")));

	appendStringInfoString(ds, ")");
	*startlocation = yylloc + 1;
}

/*
 * get a List that builed for the collection subscirpt,
 * if it is a collection method, then is_method is set to true.
 */
static List*
read_collect_variable(PLiSQL_datum *top_datum,
					int startlocation,
					StringInfo var_ds,
					StringInfo method_ds,
					bool *is_method)
{
	PLiSQL_datum 	*datum;
	PLiSQL_type 	*data_type = NULL;
	int 			tok;
	List			*list_elems = NIL;

	datum = top_datum;
	if (datum->dtype == PLISQL_DTYPE_VAR)
	{
		data_type = ((PLiSQL_var *)datum)->datatype;
		appendStringInfoString(var_ds, ((PLiSQL_var *)datum)->refname);
	}
	else if (datum->dtype == PLISQL_DTYPE_PACKAGE_DATUM)
	{
		/* reference a package variable */
		PLiSQL_pkg_datum *pkg_datum = (PLiSQL_pkg_datum *)datum;

		data_type = ((PLiSQL_var *)pkg_datum->pkgvar)->datatype;
		appendStringInfoString(var_ds, ((PLiSQL_var *)pkg_datum->pkgvar)->refname);
	}
	else
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("not a valid variable")));

	tok = yylex();
	while (tok == '(' || tok == '.')
	{
		if (tok == '(')
		{
			PLiSQL_arrayelem	*new;

			/* get current dimensional value type */
			if (!IS_PLSQL_COLLECTYPE(data_type->typtype))
				ereport(ERROR,
					(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
					errmsg("number of subscripts exceeds the maximum collection type dimensions.")));

			new = palloc0(sizeof(PLiSQL_arrayelem));
			new->dtype		= PLISQL_DTYPE_COLLECT_ARRAYELEM;
			new->subscript	= read_sql_expression(')', ")");
			new->arrayparentno = datum->dno;
			new->parenttypoid = InvalidOid;

			list_elems = lappend(list_elems, (void *)new);
			datum = (PLiSQL_datum *) new;
			data_type = ((PLiSQL_ora_collection_type*)data_type)->typdatatype;

			/* convert subscript '(' to ']' */
			appendStringInfoChar(var_ds, '[');
			appendStringInfoString(var_ds, new->subscript->query);
			appendStringInfoChar(var_ds, ']');

			tok = yylex();
		}
		else if (tok == '.')
		{
			if (datum->dtype == PLISQL_DTYPE_COLLECT_ARRAYELEM)
			{
				PLiSQL_arrayelem	*elem;
				List		*wdnames = NIL;
				ListCell	*lc;
				int			i;
				int			nlen;

				tok = yylex();

				if (tok == T_WORD)
					wdnames = lappend(wdnames, (void *)(yylval.word.ident));
				else if (tok == T_DATUM)
				{
					if (yylval.wdatum.ident == NULL)
						ereport(ERROR,
								(errcode(ERRCODE_SYNTAX_ERROR),
								 errmsg("syntax error")));

					wdnames = lappend(wdnames, (void *)pstrdup(yylval.wdatum.ident));
				}
				else if (plisql_token_is_unreserved_keyword(tok))
					wdnames = lappend(wdnames, (void *)pstrdup(yylval.keyword));
				else if (tok == T_CWORD)
				{
					ListCell *l;

					foreach(l, yylval.cword.idents)
						wdnames = lappend(wdnames, (void *)pstrdup(strVal(lfirst(l))));
				}
				else
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("syntax error")));

				elem = (PLiSQL_arrayelem *) datum;
				datum = (PLiSQL_datum *) elem;
				i = 0;
				nlen = list_length(wdnames);

				foreach(lc, wdnames)
				{
					char *fieldname = (char *)lfirst(lc);

					i++;
					if (elem->indirection == NIL)
						elem->indirection = list_make1(pstrdup(fieldname));
					else
						elem->indirection = lappend(elem->indirection, pstrdup(fieldname));

					if (data_type->ttype == PLISQL_TTYPE_ORARECORD_TYPE)
					{
						data_type = get_record_field_type(data_type, fieldname);
						appendStringInfoChar(var_ds, '.');
						appendStringInfoString(var_ds, fieldname);
					}
					else if (data_type->ttype == PLISQL_TTYPE_REC ||
						data_type->ttype == PLISQL_TTYPE_EXPLICIT_CURSOR)
					{
						appendStringInfoChar(var_ds, '.');
						appendStringInfoString(var_ds, fieldname);
						list_free(wdnames);
						if (i < nlen)
							ereport(ERROR,
									(errcode(ERRCODE_SYNTAX_ERROR),
									 errmsg("the record field is wrong"),
									 parser_errposition(startlocation)));

						return list_elems;
					}
					else if (data_type->ttype == PLISQL_TTYPE_COLLECT_TYPE)
					{
						int		method_token;

						/* maybe collection methods */
						method_token = get_collect_method_tok(fieldname);
						if (is_collect_method(method_token))
						{
							read_collect_method(method_token, var_ds->data, data_type, method_ds, &startlocation);
							*is_method = true;
						}
						else
							ereport(ERROR,
									(errcode(ERRCODE_SYNTAX_ERROR),
									 errmsg("it might not be a collection assignment"),
									 parser_errposition(startlocation)));
					}
					else
						ereport(ERROR,
								(errcode(ERRCODE_SYNTAX_ERROR),
								 errmsg("it might not be a collection assignment"),
									 parser_errposition(startlocation)));
					if (data_type == NULL)
							ereport(ERROR,
									(errcode(ERRCODE_SYNTAX_ERROR),
									 errmsg("the datatype is wrong"),
									 parser_errposition(startlocation)));
				}
				tok = yylex();
				list_free(wdnames);
			}
			else if (data_type && data_type->ttype == PLISQL_TTYPE_COLLECT_TYPE)
			{
				/* maybe collection methods */
				if (is_collect_method(tok))
				{
					read_collect_method(tok, var_ds->data, data_type, method_ds, &startlocation);
					*is_method = true;
				}
				else
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("it might not be a collection assignment"),
							 parser_errposition(startlocation)));
			}
			else
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("it might not be a collection assignment"),
						 parser_errposition(startlocation)));
		}
	}
	plisql_push_back_token(tok);

	return list_elems;
}

/*
 * get_collect_stmt
 * build the collect variable assign statment or collection method call statment
 */
static PLiSQL_stmt*
get_collect_stmt(PLiSQL_datum *top_datum, int startlocation, int lineno)
{
	StringInfoData	var_ds;
	StringInfoData	method_ds;
	List			*list_elems = NIL;
	bool 			is_method = false;
	PLiSQL_expr 	*expr;
	int				tok;

	initStringInfo(&var_ds);
	initStringInfo(&method_ds);

	list_elems = read_collect_variable(top_datum, startlocation,
									&var_ds, &method_ds, &is_method);
	tok = yylex();
	if (is_method)
	{
		PLiSQL_stmt_perform *new;

		/* free the builded elems */
		if (list_elems != NIL)
		{
			ListCell *lc;

			foreach(lc, list_elems)
			{
				PLiSQL_arrayelem *elem = (PLiSQL_arrayelem *) lfirst(lc);

				pfree(elem->subscript);
				if (elem->indirection)
					list_free(elem->indirection);

				pfree(elem);
			}
			pfree(list_elems);
		}

		expr = palloc0(sizeof(PLiSQL_expr));
		expr->query 		= pstrdup(method_ds.data);
		expr->parseMode 	= RAW_PARSE_PLISQL_EXPR;
		expr->plan			= NULL;
		expr->paramnos		= NULL;
		expr->target_param	= -1;
		expr->ns			= plisql_ns_top();

		new = palloc0(sizeof(PLiSQL_stmt_perform));
		new->cmd_type = PLISQL_STMT_PERFORM;
		new->lineno   = lineno;
		new->stmtid = ++plisql_curr_compile->nstatements;
		new->expr  = expr;

		pfree(var_ds.data);
		pfree(method_ds.data);

		return (PLiSQL_stmt *)new;
	}
	else
	{
		PLiSQL_stmt_assign *new;
		PLiSQL_datum	*datum = top_datum;

		if (tok != '=' && tok != COLON_EQUALS)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("it might not be a collection assignment"),
					 parser_errposition(startlocation)));

		/* put the arrayelem into plisql_Datums array if any */
		if (list_elems != NIL)
		{
			ListCell 	*lc;
			int			parent_dno = top_datum->dno;
			PLiSQL_arrayelem *elem;
			int 		nlist = list_length(list_elems);
			int 		i;

			i = 0;
			foreach(lc, list_elems)
			{
				elem = (PLiSQL_arrayelem *) lfirst(lc);

				plisql_adddatum((PLiSQL_datum *) elem);

				elem->arrayparentno = parent_dno;
				parent_dno = elem->dno;
				i++;
				if (i == nlist)
					datum = (PLiSQL_datum *)elem;
			}
		}

		/* build a PLiSQL_stmt_assign expr */
		expr = read_sql_expression(';', ";");

		new = palloc0(sizeof(PLiSQL_stmt_assign));
		new->cmd_type = PLISQL_STMT_ASSIGN;
		new->lineno   = lineno;
		new->stmtid = ++plisql_curr_compile->nstatements;
		new->varno = datum->dno;
		new->expr  = expr;

		pfree(var_ds.data);
		pfree(method_ds.data);

		return (PLiSQL_stmt *)new;
	}
}

/*
 * Given the datum and name after the OUT/INOUT keyword, put them
 * in fieldnames and varnos. It should be noted that a new datum
 * should be constructed for the collection element.
 */
static int
read_out_scalar(char **fieldnames,
				int *varnos,
				char param_info,
				PLiSQL_expr **expr)
{
	int				 tok;
	PLiSQL_datum 	*cur_datum;
	char			*fieldname;
	char			*querystr;
	int 			loc;

	tok = yylex();
	if (tok != T_DATUM)
		current_token_is_not_variable(tok);

	cur_datum = yylval.wdatum.datum;
	check_assignable(cur_datum, yylloc);
	fieldname = NameOfDatum(&(yylval.wdatum));
	loc = yylloc;

	if ((cur_datum->dtype == PLISQL_DTYPE_VAR &&
		((PLiSQL_var *)cur_datum)->datatype->ttype == PLISQL_TTYPE_COLLECT_TYPE) ||
		(cur_datum->dtype == PLISQL_DTYPE_PACKAGE_DATUM &&
		((PLiSQL_var *)(((PLiSQL_pkg_datum *)cur_datum)->pkgvar))->datatype->ttype == PLISQL_TTYPE_COLLECT_TYPE))
	{
		StringInfoData	var_ds;
		StringInfoData	method_ds;
		List			*list_elems = NIL;
		bool			is_method = false;
		PLiSQL_datum	*top_datum;

		initStringInfo(&var_ds);
		initStringInfo(&method_ds);

		top_datum = cur_datum;
		list_elems = read_collect_variable(top_datum, loc,
										&var_ds, &method_ds, &is_method);

		if (is_method)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("\"%s\" is not a scalar variable",
							fieldname),
						parser_errposition(loc)));

		/* put the arrayelem into plisql_Datums array if any */
		if (list_elems != NIL)
		{
			ListCell	*lc;
			int 		parent_dno = top_datum->dno;
			PLiSQL_arrayelem *elem;
			int 		nlist = list_length(list_elems);
			int 		i;

			i = 0;
			foreach(lc, list_elems)
			{
				elem = (PLiSQL_arrayelem *) lfirst(lc);

				plisql_adddatum((PLiSQL_datum *) elem);
				elem->arrayparentno = parent_dno;
				parent_dno = elem->dno;
				i++;
				if (i == nlist)
				{
					*fieldnames = pstrdup(fieldname);
					*varnos = elem->dno;
				}
			}
			querystr = pstrdup(var_ds.data);
		}
		else
		{
			*fieldnames = pstrdup(fieldname);
			*varnos = cur_datum->dno;
			querystr = pstrdup(fieldname);
		}

		pfree(var_ds.data);
		pfree(method_ds.data);
	}
	else
	{
		*fieldnames = pstrdup(fieldname);
		*varnos	= cur_datum->dno;
		querystr = pstrdup(fieldname);
	}

	*expr = palloc0(sizeof(PLiSQL_expr));
	(*expr)->query		= querystr;
	(*expr)->parseMode	= RAW_PARSE_PLISQL_EXPR;
	(*expr)->plan		= NULL;
	(*expr)->paramnos	= NULL;
	(*expr)->target_param	= -1;
	(*expr)->ns 		= plisql_ns_top();
	(*expr)->info		= param_info;
	(*expr)->datum_dno	= *varnos;

	check_sql_expr((*expr)->query, (*expr)->parseMode, loc);
	tok = yylex();

	return tok;
}
/* End - ReqID:SRS-PLSQL-Collection */

/* Begin - ReqID:SRS-PLSQL-BULKCOLLECT */
/*
 * Read the argument of an BULK COLLECT INTO clause.  On entry, we have just read the
 * BULK COLLECT INTO keyword.
 */
static void
read_bulk_collect_into_target(PLiSQL_variable **target)
{
	int			tok;

	/* Set default results */
	*target = NULL;

	tok = yylex();

	switch (tok)
	{
		case T_DATUM:
			{
				*target = (PLiSQL_variable *)
					read_bulk_collect_into_udt_list(NameOfDatum(&(yylval.wdatum)),
												yylval.wdatum.datum, yylloc);
			}
			break;

		default:
			/* just to give a better message than "syntax error" */
			current_token_is_not_variable(tok);
	}
}

/*
 * Given the first datum and name in the BULK COLLECT INTO list, continue to read
 * comma-separated scalar variables until we run out. Then construct
 * and return a fake "row" variable that represents the list of
 * collections.
 */
static PLiSQL_row *
read_bulk_collect_into_udt_list(char *initial_name,
								PLiSQL_datum *initial_datum,
								int initial_location)
{
	int				 nfields;
	char			*fieldnames[1024];
	int				 varnos[1024];
	PLiSQL_row		*row;
	int				 tok;
	PLiSQL_datum 	*cur_datum;
	char 			*fieldname;

	nfields		  = 0;
	cur_datum 	  = initial_datum;
	fieldname 	  = initial_name;
	do
	{
		/* Check for array overflow */
		if (nfields >= 1024)
			ereport(ERROR,
					(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
					 errmsg("too many BULK COLLECT INTO variables specified"),
					 parser_errposition(yylloc)));

		check_assignable(cur_datum, yylloc);

		if (nfields > 0 &&
			is_row_record_datum(cur_datum))
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("\"%s\" is not a collection variable",
							fieldname),
					 parser_errposition(yylloc)));

		if ((cur_datum->dtype == PLISQL_DTYPE_VAR &&
			((PLiSQL_var *)cur_datum)->datatype->ttype == PLISQL_TTYPE_COLLECT_TYPE) ||
			(cur_datum->dtype == PLISQL_DTYPE_PACKAGE_DATUM &&
			((PLiSQL_var *)(((PLiSQL_pkg_datum *)cur_datum)->pkgvar))->datatype->ttype == PLISQL_TTYPE_COLLECT_TYPE))
		{
			StringInfoData	var_ds;
			StringInfoData	method_ds;
			List			*list_elems = NIL;
			bool			is_method = false;
			PLiSQL_datum	*top_datum;

			initStringInfo(&var_ds);
			initStringInfo(&method_ds);

			top_datum = cur_datum;
			list_elems = read_collect_variable(top_datum, initial_location,
											&var_ds, &method_ds, &is_method);

			if (nfields > 0 && is_method)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("\"%s\" is not a collection variable",
								fieldname),
							parser_errposition(yylloc)));

			/* put the arrayelem into plisql_Datums array if any */
			if (list_elems != NIL)
			{
				ListCell	*lc;
				int 		parent_dno = top_datum->dno;
				PLiSQL_arrayelem *elem;
				int			nlist = list_length(list_elems);
				int			i;

				i = 0;
				foreach(lc, list_elems)
				{
					elem = (PLiSQL_arrayelem *) lfirst(lc);
					plisql_adddatum((PLiSQL_datum *) elem);

					elem->arrayparentno = parent_dno;
					parent_dno = elem->dno;
					i++;
					if (i == nlist)
					{
						fieldnames[nfields] = fieldname;
						varnos[nfields++]	= elem->dno;
					}
				}
			}
			else
			{
				fieldnames[nfields] = fieldname;
				varnos[nfields++]	= cur_datum->dno;
			}
			pfree(var_ds.data);
			pfree(method_ds.data);
		}
		else
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("\"%s\" is not a collection variable",
							fieldname),
					 parser_errposition(yylloc)));

		tok = yylex();

		if (tok != ',')
			break;

		tok = yylex();
		if (tok != T_DATUM)
		{
			/* just to give a better message than "syntax error" */
			current_token_is_not_variable(tok);
		}
		cur_datum = yylval.wdatum.datum;
		fieldname = NameOfDatum(&yylval.wdatum);
	} while(1);

	/*
	 * We read an extra, non-comma token from yylex(), so push it
	 * back onto the input stream
	 */
	plisql_push_back_token(tok);

	row = palloc0(sizeof(PLiSQL_row));
	row->dtype = PLISQL_DTYPE_ROW;
	row->refname = "(unnamed row)";
	row->lineno = plisql_location_to_lineno(initial_location);
	row->rowtupdesc = NULL;
	row->nfields = nfields;
	row->fieldnames = palloc(sizeof(char *) * nfields);
	row->varnos = palloc(sizeof(int) * nfields);
	while (--nfields >= 0)
	{
		row->fieldnames[nfields] = fieldnames[nfields];
		row->varnos[nfields] = varnos[nfields];
	}

	plisql_adddatum((PLiSQL_datum *)row);

	return row;
}
/* End - ReqID:SRS-PLSQL-BULKCOLLECT */
